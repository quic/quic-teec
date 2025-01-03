// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _QCOM_TEE_OBJ_H
#define _QCOM_TEE_OBJ_H

#include "qcom_tee_defs.h"

/*
===========================================================================
                         ROOT OBJECT
===========================================================================
*/
void acquire_root_obj_lock(void);

void release_root_obj_lock(void);

QCOMTEE_Result init_qcom_tee_root_object(char *devname,
					   QCOMTEE_Object **object);

QCOMTEE_Object *get_root_ptr(void);

/*
===========================================================================
                         REMOTE OBJECT
===========================================================================
*/
QCOMTEE_Result init_qcom_tee_remote_object(uint64_t object_id,
					     QCOMTEE_Object **remote_object);

void release_remote_objects(uint32_t num_params,
			    QCOMTEE_Param *qcom_tee_params, uint64_t attr);

/*
===========================================================================
                         CALLBACK OBJECT
===========================================================================
*/
uint64_t alloc_callback_object_id(void);

#endif // _QCOM_TEE_OBJ_H
