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

#endif // _QCOMTEE_OBJECT_TYPES_H
