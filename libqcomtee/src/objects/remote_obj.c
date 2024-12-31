// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <stdlib.h>

#include "qcom_tee_obj.h"

/*
===========================================================================
                          PRIVATE HELPER/CALLBACK FUNCTIONS
===========================================================================
*/

static void release_remote_obj(QCOMTEE_Object *remote_obj)
{
	QCOMTEE_Object *root = (QCOMTEE_Object *)remote_obj->data;

	free(remote_obj);
	QCOMTEE_OBJECT_RELEASE(root);
}

/*
===========================================================================
                         PUBLIC FUNCTIONS
===========================================================================
*/

QCOMTEE_Result init_qcom_tee_remote_object(uint64_t object_id,
					   QCOMTEE_Object **remote_object)
{
	*remote_object = (QCOMTEE_Object *)malloc(sizeof(QCOMTEE_Object));
	if (*remote_object == NULL)
		return QCOMTEE_MSG_ERROR_MEM;

	atomic_init(&((*remote_object)->refs), 1);

	(*remote_object)->object_id = object_id;
	(*remote_object)->object_type = QCOM_TEE_OBJECT_TYPE_TEE;

	/* All Remote Objects hold a reference to the
	 * Root Object with which they are associated
	 * to form an Object Tree
	 */
	(*remote_object)->data = (void *)get_root_ptr();
	(*remote_object)->release = release_remote_obj;

	return QCOMTEE_MSG_OK;
}
