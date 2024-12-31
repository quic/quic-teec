// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <linux/tee.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "qcom_tee_api.h"
#include "qcom_tee_obj.h"

/*
===========================================================================
                          PRIVATE HELPER FUNCTIONS
===========================================================================
*/
static inline void acquire_obj_lock(QCOMTEE_Object *x)
{
	if (x->object_type == QCOM_TEE_OBJECT_TYPE_ROOT)
		acquire_root_obj_lock();
}

static inline void release_obj_lock(QCOMTEE_Object *x)
{
	if (x->object_type == QCOM_TEE_OBJECT_TYPE_ROOT)
		release_root_obj_lock();
}

static QCOMTEE_Result pre_process_object_invoke(uint32_t num_params,
						QCOMTEE_Param *qcom_tee_params,
						struct tee_ioctl_param *params)
{
	for (int n = 0; n < num_params; n++) {
		switch (qcom_tee_params[n].attr) {
		case QCOMTEE_UBUF_INPUT:
			params[n].attr = TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_INPUT;
			params[n].a = (uint64_t)qcom_tee_params[n].ubuf.buffer;
			params[n].b = (uint64_t)qcom_tee_params[n].ubuf.size;
			break;
		case QCOMTEE_UBUF_OUTPUT:
			params[n].attr = TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_OUTPUT;
			params[n].a = (uint64_t)qcom_tee_params[n].ubuf.buffer;
			params[n].b = (uint64_t)qcom_tee_params[n].ubuf.size;
			break;
		case QCOMTEE_OBJREF_OUTPUT:
			params[n].attr =
				TEE_IOCTL_PARAM_ATTR_TYPE_OBJREF_OUTPUT;
			break;
		default:
			return QCOMTEE_MSG_ERROR;
		}
	}

	return QCOMTEE_MSG_OK;
}

static QCOMTEE_Result post_process_object_invoke(uint32_t num_params,
						 struct tee_ioctl_param *params,
						 QCOMTEE_Param *qcom_tee_params)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	QCOMTEE_Object *remote_object = QCOMTEE_Object_NULL;

	for (int n = 0; n < num_params; n++) {
		switch (params[n].attr) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_OUTPUT:
			qcom_tee_params[n].attr = QCOMTEE_UBUF_OUTPUT;
			qcom_tee_params[n].ubuf.size = (size_t)params[n].b;
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_OBJREF_OUTPUT:
			res = init_qcom_tee_remote_object(params[n].a,
							  &remote_object);
			if (res != QCOMTEE_MSG_OK) {
				return res;
			}

			qcom_tee_params[n].attr = QCOMTEE_OBJREF_OUTPUT;
			qcom_tee_params[n].object = remote_object;
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_INPUT:
			break;
		default:
			return QCOMTEE_MSG_ERROR;
		}
	}

	return QCOMTEE_MSG_OK;
}

/*
===========================================================================
                         PUBLIC FUNCTIONS
===========================================================================
*/

QCOMTEE_Result QCOMTEE_GetRootObject(QCOMTEE_Object **root_object)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;

	res = init_qcom_tee_root_object("/dev/tee0", root_object);
	if (res != QCOMTEE_MSG_OK) {
		MSGE("init_qcom_tee_root_object failed: 0x%x\n", res);
	}

	return res;
}

void QCOMTEE_ReleaseObject(QCOMTEE_Object *object)
{
	acquire_obj_lock(object);

	if (atomic_fetch_sub(&object->refs, 1) == 1)
		object->release(object);

	release_obj_lock(object);
}

QCOMTEE_Object *QCOMTEE_RetainObject(QCOMTEE_Object *object)
{
	QCOMTEE_Object *return_obj = QCOMTEE_Object_NULL;

	acquire_obj_lock(object);

	if (atomic_load(&object->refs) == 0)
		goto exit;

	atomic_fetch_add(&object->refs, 1);
	return_obj = object;

exit:
	release_obj_lock(object);

	return return_obj;
}

QCOMTEE_Result QCOMTEE_InvokeObject(QCOMTEE_Object *object, uint32_t op,
				    uint32_t num_params,
				    QCOMTEE_Param *qcom_tee_params)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	int fd = -1;
	int rc = 0;

	struct tee_ioctl_object_invoke_arg *arg = NULL;
	struct tee_ioctl_param *params = NULL;
	struct tee_ioctl_buf_data buf_data = { 0 };
	const size_t arg_size = sizeof(struct tee_ioctl_object_invoke_arg) +
				num_params * sizeof(struct tee_ioctl_param);

	if (object == NULL) {
		res = QCOMTEE_MSG_ERROR_INVALID;
		MSGE("Invalid object: 0x%x\n", res);
		goto out;
	}

	switch (object->object_type) {
	case QCOM_TEE_OBJECT_TYPE_ROOT:
		fd = object->fd;
		break;
	case QCOM_TEE_OBJECT_TYPE_TEE:
		fd = ((QCOMTEE_Object *)(object->data))->fd;
		break;
	default:
		res = QCOMTEE_MSG_ERROR_INVALID;
		MSGE("Invalid object type: 0x%x\n", res);
		goto out;
	}

	arg = (struct tee_ioctl_object_invoke_arg *)malloc(arg_size);
	if (arg == NULL) {
		res = QCOMTEE_MSG_ERROR_MEM;
		goto out;
	}

	memset(arg, 0, arg_size);

	arg->op = op;
	arg->object = object->object_id;
	arg->num_params = num_params;
	arg->ret = 0;
	if (num_params > 0) {
		params = (struct tee_ioctl_param *)(arg + 1);
	}

	buf_data.buf_ptr = (__u64)arg;
	buf_data.buf_len = (__u64)arg_size;

	// Marshal In
	res = pre_process_object_invoke(num_params, qcom_tee_params, params);
	if (res != QCOMTEE_MSG_OK) {
		MSGE("pre_process_object_invoke failed: 0x%x\n", res);
		goto out;
	}

	rc = ioctl(fd, TEE_IOC_OBJECT_INVOKE, &buf_data);
	if (rc) {
		res = ioctl_errno_to_res(errno);
		MSGE("TEE_IOC_OBJECT_INVOKE: errno = %d\n", errno);
		goto out;
	}

	if (arg->ret) {
		res = arg->ret;
		MSGE("Error from QTEE. ret = %d\n", arg->ret);
		goto out;
	}

	// Marshal Out
	res = post_process_object_invoke(num_params, params, qcom_tee_params);
	if (res != QCOMTEE_MSG_OK) {
		MSGE("post_process_object_invoke failed: 0x%x\n", res);
		goto out;
	}

out:
	if (arg)
		free(arg);

	return res;
}
