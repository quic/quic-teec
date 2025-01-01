// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include "qcom_tee_obj.h"

/*
===========================================================================
                          GLOBAL DATA STRUCTURES
===========================================================================
*/

static uint64_t CALLBACK_OBJ_ID_COUNTER = CALLBACK_OBJ_ID_MIN;

/*
===========================================================================
                         PUBLIC FUNCTIONS
===========================================================================
*/
uint64_t alloc_callback_object_id(void)
{
	return atomic_fetch_add(&CALLBACK_OBJ_ID_COUNTER, 1);
}
