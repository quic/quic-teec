// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _QCOM_TEE_API_H
#define _QCOM_TEE_API_H

#include "qcom_tee_defs.h"

/**
 * QCOMTEE_GetRootObject() - Get a Root object which can be used to initiate
 * object-based IPC with QTEE.
 *
 * @param root_object        A Root object which can be used to initiate
 *                           object-based IPC with QTEE.
 *
 * @return QCOMTEE_SUCCESS   The Root object was successfully created.
 * @return QCOMTEE_Result    Something failed.
 */
QCOMTEE_Result QCOMTEE_GetRootObject(QCOMTEE_Object **root_object);

/**
 * QCOMTEE_InvokeObject() - Invoke a QTEE Object which represents a service
 * hosted in QTEE.
 *
 * @param object             The object to be invoked in QTEE.
 * @param op                 The operation to be invoked within the object.
 * @param num_params         The number of parameters in this request.
 * @param qcom_tee_params    The parameters to be exchanged in this invoke
 *                           request.
 *
 * @return QCOMTEE_SUCCESS   The object was successfully invoked.
 * @return QCOMTEE_Result    Something failed.
 */
QCOMTEE_Result QCOMTEE_InvokeObject(QCOMTEE_Object *object, uint32_t op,
				      uint32_t num_params,
				      QCOMTEE_Param *qcom_tee_params);

/**
 * QCOMTEE_RegisterCallbackObject() - Register a callback object with the
 * Callback Supplicant. The callback object represents a service hosted
 * in the client which can be invoked by QTEE.
 *
 * @param cb_object          The callback object to be registered.
 *
 * @return QCOMTEE_SUCCESS   The object was successfully registered.
 * @return QCOMTEE_Result    Something failed.
 */
QCOMTEE_Result QCOMTEE_RegisterCallbackObject(QCOMTEE_Object *cb_object);

#endif // _QCOM_TEE_API_H
