// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include "qcom_tee_api.h"
#include "credentials.h"
#include "credentials_obj.h"
#include "utils/utils.h"

static void release_creds_obj(QCOMTEE_Object *creds_object)
{
	credentials *creds = (credentials *)creds_object->data;

	free(creds->cred_buffer);
	free(creds);
	free(creds_object);
}

static QCOMTEE_Result dispatch(QCOMTEE_Object *creds_object, uint32_t op,
			       uint32_t *num_params,
			       QCOMTEE_Param *qcom_tee_params,
			       uint8_t *cbo_buffer, size_t cbo_buffer_len)
{
	uint64_t cred_buf_len = 0;
	uint64_t offset_val = 0;
	size_t data_len = 0;
	size_t needed_len = 0;
	size_t data_lenout = 0;

	if (!creds_object)
		return QCOMTEE_MSG_ERROR_INVALID;

	credentials *creds = (credentials *)creds_object->data;

	switch (op) {
	case IIO_OP_getLength:

		cred_buf_len = creds->cred_buf_len;
		memset(cbo_buffer, 0, cbo_buffer_len);
		memscpy(cbo_buffer, sizeof(uint64_t), &cred_buf_len,
			sizeof(uint64_t));

		qcom_tee_params[0].attr = QCOMTEE_UBUF_OUTPUT;
		qcom_tee_params[0].ubuf.buffer = (void *)cbo_buffer;
		qcom_tee_params[0].ubuf.size = sizeof(uint64_t);

		*num_params = 1;
		break;
	case IIO_OP_readAtOffset:

		offset_val = *((uint64_t *)(qcom_tee_params[0].ubuf.buffer));

		if ((size_t)offset_val >= creds->cred_buf_len)
			return QCOMTEE_MSG_ERROR_INVALID;

		data_len = 10;
		needed_len = creds->cred_buf_len - (size_t)offset_val;

		memset(cbo_buffer, 0, cbo_buffer_len);
		data_lenout =
			memscpy(cbo_buffer, data_len,
				(const char *)creds->cred_buffer + offset_val,
				needed_len);

		qcom_tee_params[0].attr = QCOMTEE_UBUF_OUTPUT;
		qcom_tee_params[0].ubuf.buffer = (void *)cbo_buffer;
		qcom_tee_params[0].ubuf.size = data_lenout;

		*num_params = 1;
		break;
	default:
		return QCOMTEE_MSG_ERROR_INVALID;
	}

	return QCOMTEE_MSG_OK;
}

QCOMTEE_Result GetCredentialsObject(QCOMTEE_Object **creds_object)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	credentials *creds = NULL;

	*creds_object = (QCOMTEE_Object *)malloc(sizeof(QCOMTEE_Object));
	if (*creds_object == NULL)
		return QCOMTEE_MSG_ERROR_MEM;

	(*creds_object)->object_type = QCOM_TEE_OBJECT_TYPE_CB;
	(*creds_object)->queued = false;
	(*creds_object)->release = release_creds_obj;
	(*creds_object)->dispatch = dispatch;

	// Allocate the object_id for this CBO, mandatory!
	res = QCOMTEE_RegisterCallbackObject(*creds_object);
	if (res != QCOMTEE_MSG_OK) {
		free(*creds_object);
		return res;
	}

	(*creds_object)->data = (void *)malloc(sizeof(credentials));
	if ((*creds_object)->data == NULL) {
		free(*creds_object);
		return QCOMTEE_MSG_ERROR_MEM;
	}

	creds = (credentials *)((*creds_object)->data);
	creds->cred_buffer = (uint8_t *)get_credentials(&creds->cred_buf_len);
	if (!creds->cred_buffer) {
		free(creds);
		free(*creds_object);
		return QCOMTEE_MSG_ERROR_MEM;
	}

	atomic_init(&((*creds_object)->refs), 1);

	return QCOMTEE_MSG_OK;
}
