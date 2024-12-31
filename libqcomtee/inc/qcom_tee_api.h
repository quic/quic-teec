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

#endif // _QCOM_TEE_API_H
