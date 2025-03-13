// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _QCOMTEE_OBJECT_TYPES_H
#define _QCOMTEE_OBJECT_TYPES_H

#include "qcomtee_object.h"

/**
 * @brief Get a credentials object.
 * @param root The root object to which this object belongs.
 * @param object Credentials object.
 * @return On success returns @ref QCOMTEE_OK;
 *         Otherwise @ref QCOMTEE_ERROR_MEM.
 */
qcomtee_result_t
qcomtee_object_credentials_init(struct qcomtee_object *root,
				struct qcomtee_object **object);

/**
 * @brief Allocate a memory object.
 *
 * Memory object is owned by the caller and should be released using
 * @ref qcomtee_memory_object_release.
 *
 * If sent to QTEE, a copy of the object is made and the owner keep their copy.
 * In most usecases, the object owner needs to share memory with QTEE rather
 * to donating it.
 *
 * For instance, if the memory object sent to QTEE in two different invocations,
 * their would be three copies of the object:
 *   - The owner copy.
 *   - The two copies sent to QTEE in the invocations.
 * Owner should release their copy using @ref qcomtee_memory_object_release.
 * QTEE will release its two copies.
 *
 * If the owner wants to donate the memory they can release their copy
 * @ref qcomtee_memory_object_release after the invocation to send the object.
 *
 * QTEE can only reference the memory objects that user owns. If the owner
 * releases the object (so they hold no copy), they can not recieve it
 * back from QTEE.
 *
 *   - If the owner donates a memory object to QTEE, they can not recieve
 *     it from QTEE as @ref QCOMTEE_OBJREF_OUTPUT.
 *   - If the owner shared a memory object to QTEE, they can recieve it
 *     from QTEE (a copy of the object is made and should be release
 *     seperately using @ref qcomtee_memory_object_release by the reciver).
 *
 * At any time, the owner can call @ref qcomtee_object_refs_inc to create a
 * copy of the object. This copy should be released using
 * @ref qcomtee_memory_object_release.
 *
 * @param size Size of the memory object.
 * @param root The root object to which this object belongs.
 * @param object Memory object.
 * @return On success, returns 0;
 *         Otherwise, returns -1.
 */
int qcomtet_memory_object_alloc(size_t size, struct qcomtee_object *root,
				struct qcomtee_object **object);

void *qcomtee_memory_object_addr(struct qcomtee_object *object);
size_t qcomtee_memory_object_size(struct qcomtee_object *object);

/**
 * @brief Release a memory object.
 * @param object The object being released.
 */
void qcomtee_memory_object_release(struct qcomtee_object *object);

#endif // _QCOMTEE_OBJECT_TYPES_H
