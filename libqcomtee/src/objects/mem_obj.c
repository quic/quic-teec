// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/tee.h>
#include <qcomtee_object_types.h>
#include <qcomtee_object_private.h>

/* Which TEE API was used to prepare the memory object: */
enum qcomtee_memory_type {
	QCOMTEE_MEMORY_TEE_ALLOC = 1,
	QCOMTEE_MEMORY_TEE_REGISTER
};

/**
 * @brief Structure representing a memory object in TEE diver.
 *
 * This structure encapsulates information about a memory object
 * used in TEE driver.
 */
struct qcomtee_memory {
	struct qcomtee_object object;
	enum qcomtee_memory_type type;
	int fd; /**< File descriptor for TEE driver shm. */
	struct {
		void *addr; /**< mmaped address. */
		size_t size; /**< size of memory. */
	} mem_info;
};

#define MEMORY(o) container_of((o), struct qcomtee_memory, object)

static void qcomtee_memory_release(struct qcomtee_object *object)
{
	struct qcomtee_memory *qcomtee_mem = MEMORY(object);

	if (qcomtee_mem->type == QCOMTEE_MEMORY_TEE_ALLOC)
		munmap(qcomtee_mem->mem_info.addr, qcomtee_mem->mem_info.size);

	/* Release TEE shm. */
	if (qcomtee_mem->fd != -1)
		close(qcomtee_mem->fd);

	free(qcomtee_mem);
}

/**
 * @brief Allocate and initialize an empty memory object.
 *
 * The object has no physical memory assigned to it.
 *
 * @return On success, returns @ref qcomtee_memory; Otherwise, NULL.
 */
static struct qcomtee_memory *qcomtee_memory_alloc(void)
{
	struct qcomtee_memory *qcomtee_mem;
	static struct qcomtee_object_ops ops = {
		.release = qcomtee_memory_release,
	};

	qcomtee_mem = calloc(1, sizeof(*qcomtee_mem));
	if (qcomtee_mem) {
		QCOMTEE_OBJECT_INIT(&qcomtee_mem->object,
				    QCOMTEE_OBJECT_TYPE_MEMORY);
		qcomtee_mem->object.ops = &ops;
		/* TEE shm not assigned yet. */
		qcomtee_mem->fd = -1;
	}

	return qcomtee_mem;
}

int qcomtee_memory_object_alloc(size_t size, struct qcomtee_object *root,
				struct qcomtee_object **object)
{
	struct root_object *root_object = ROOT_OBJECT(root);
	struct tee_ioctl_shm_alloc_data data;
	struct qcomtee_memory *qcomtee_mem;
	void *addr;
	int fd;

	qcomtee_mem = qcomtee_memory_alloc();
	if (!qcomtee_mem)
		return -1;

	data.size = size;
	data.flags = 0;
	data.id = 0;
	fd = root_object->tee_call(root_object->fd, TEE_IOC_SHM_ALLOC, &data);
	if (fd < 0)
		goto err_release;

	/* Assign TEE shm. */
	qcomtee_mem->object.tee_object_id = data.id;
	qcomtee_mem->fd = fd;

	addr = mmap(NULL, data.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
		goto err_release;

	/* INIT the memory object. */
	qcomtee_mem->mem_info.addr = addr;
	qcomtee_mem->mem_info.size = data.size;
	qcomtee_mem->type = QCOMTEE_MEMORY_TEE_ALLOC;
	/* Keep a copy of root object; released in qcomtee_object_refs_dec. */
	qcomtee_object_refs_inc(root);
	qcomtee_mem->object.root = root;

	*object = &qcomtee_mem->object;

	return 0;

err_release:
	qcomtee_memory_object_release(&qcomtee_mem->object);

	return -1;
}

void *qcomtee_memory_object_addr(struct qcomtee_object *object)
{
	return MEMORY(object)->mem_info.addr;
}

size_t qcomtee_memory_object_size(struct qcomtee_object *object)
{
	return MEMORY(object)->mem_info.size;
}

void qcomtee_memory_object_release(struct qcomtee_object *object)
{
	qcomtee_object_refs_dec(object);
}
