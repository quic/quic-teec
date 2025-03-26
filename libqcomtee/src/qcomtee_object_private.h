// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _QCOMTEE_OBJECT_PRIVATE_H
#define _QCOMTEE_OBJECT_PRIVATE_H

#include <pthread.h>
#include <string.h>
#include <qcomtee_object.h>

/**
 * @def TABLE_SIZE
 * @brief The number of objects that can be exported to QTEE in each root object.
 */
#define TABLE_SIZE 1024

/**
 * @brief Object namespace.
 *
 * Every time a root object is created, a new namespace is initiated.
 * This guarantees that callback objects exported to QTEE using a root object
 * can only be referenced by QTEE in the namespace maintained by the root
 * object. For QTEE objects, the kernel driver guarantees that objects
 * received using a root object are not visible to others.
 */
struct qcomtee_object_namespace {
	int current_idx; /**< Index to start searching for free entry. */
	struct qcomtee_object *entries[TABLE_SIZE]; /**< Callback object table. */
	pthread_mutex_t lock; /**< lock to protect members of this struct. */
};

/**
 * @brief Root object.
 *
 * Use @ref qcomtee_object_root_init to create a root object and a namespace.
 */
struct root_object {
	struct qcomtee_object object;
	struct qcomtee_object_namespace ns;
	int fd; /**< Driver's fd. */
	tee_call_t tee_call; /**< API to call to TEE driver (e.g. ioctl()). */
	void (*release)(void *);
	void *arg; /**< Argument passed to release. */
};

#define ROOT_OBJECT(ro) container_of((ro), struct root_object, object)
#define ROOT_OBJECT_NS(ro) (&ROOT_OBJECT(ro)->ns)

/**
 * @def OBJECT_NS
 * @brief Returns the namespace the object belongs to.
 */
#define OBJECT_NS(o) ROOT_OBJECT_NS((o)->root)

/**
 * @brief Initialize an object.
 * @param object Object to initialize.
 * @param object_type Type of the object.
 */
static inline void QCOMTEE_OBJECT_INIT(struct qcomtee_object *object,
				       qcomtee_object_type_t object_type)
{
	memset(object, 0, sizeof(*object));

	atomic_init(&object->refs, 1);
	object->object_type = object_type;
	object->root = QCOMTEE_OBJECT_NULL;
}

#endif // _QCOMTEE_OBJECT_PRIVATE_H
