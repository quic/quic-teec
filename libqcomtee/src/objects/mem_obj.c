// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/tee.h>
#include <qcomtee_object_types.h>
#include <qcomtee_object_private.h>

/* MEMORY Object. */
struct qcomtee_memmory {
	struct qcomtee_object object; /**< Object base. */
	int fd; /**< File descriptor for TEE_IOC_SHM_ALLOC. */
	struct {
		void *addr; /**< mmaped address. */
		size_t size; /**< size of memory. */
	} mem_info;
};

#define MEMORY(o) container_of((o), struct qcomtee_memmory, object)

static void qcomtee_memmory_release(struct qcomtee_object *object)
{
	struct qcomtee_memmory *qcomtee_mem = MEMORY(object);

	munmap(qcomtee_mem->mem_info.addr, qcomtee_mem->mem_info.size);
	close(qcomtee_mem->fd);
}

static struct qcomtee_object_ops ops = {
	.release = qcomtee_memmory_release,
};

int qcomtet_memory_object_alloc(size_t size, struct qcomtee_object *root,
				struct qcomtee_object **object)
{
	struct root_object *root_object = ROOT_OBJECT(root);
	struct tee_ioctl_shm_alloc_data data;
	struct qcomtee_memmory *qcomtee_mem;
	void *addr;
	int fd;

	qcomtee_mem = calloc(1, sizeof(*qcomtee_mem));
	if (!qcomtee_mem)
		return -1;

	data.size = size;
	data.flags = 0;
	data.id = 0;
	fd = root_object->tee_call(root_object->fd, TEE_IOC_SHM_ALLOC, &data);
	if (fd < 0)
		goto err_free;

	addr = mmap(NULL, data.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		close(fd);
		goto err_free;
	}

	/* INIT the memory object. */
	QCOMTEE_OBJECT_INIT(&qcomtee_mem->object, QCOMTEE_OBJECT_TYPE_MEMORY);

	qcomtee_mem->object.ops = &ops;
	qcomtee_mem->object.tee_object_id = data.id;
	qcomtee_mem->mem_info.addr = addr;
	qcomtee_mem->mem_info.size = data.size;
	qcomtee_mem->fd = fd;

	/* Keep a copy of root object; released in qcomtee_object_refs_dec. */
	qcomtee_object_refs_inc(root);
	qcomtee_mem->object.root = root;

	*object = &qcomtee_mem->object;

	return 0;

err_free:
	free(qcomtee_mem);

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
