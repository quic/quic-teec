// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <stdlib.h>
#include <sys/time.h>

#include "qcbor.h"

#include "credentials.h"

#define CREDENTIALS_BUF_SIZE_INC 4096

typedef enum {
	attr_uid = 1,
	attr_pkg_flags,
	attr_pkg_name,
	attr_pkg_cert,
	attr_permissions,
	attr_system_time
} cred_attr;

void *get_credentials(size_t *encoded_buf_len)
{
	QCBOREncodeContext e_ctx;

	uint8_t *credential_buf = NULL;
	UsefulBuf creds_useful_buf;
	creds_useful_buf.ptr = NULL;
	creds_useful_buf.len = 0;
	struct timeval curr_time = { 0 };
	int64_t time_since_epoch_ms = 0;

	int error = QCBOR_ERR_BUFFER_TOO_SMALL;
	UsefulBufC enc;

	while (error == QCBOR_ERR_BUFFER_TOO_SMALL) {
		creds_useful_buf.len += CREDENTIALS_BUF_SIZE_INC;
		free(creds_useful_buf.ptr);
		creds_useful_buf.ptr = (void *)calloc(1, creds_useful_buf.len);
		if (!creds_useful_buf.ptr) {
			MSGE("calloc failed to allocatefor %zd bytes\n",
			     creds_useful_buf.len);
			return NULL;
		}

		gettimeofday(&curr_time, NULL);
		time_since_epoch_ms = ((int64_t)curr_time.tv_sec * 1000) +
				      ((int64_t)curr_time.tv_usec / 1000);

		// Write UID and system time and create a CBOR buf for QTEE
		QCBOREncode_Init(&e_ctx, creds_useful_buf);
		QCBOREncode_OpenMap(&e_ctx);
		QCBOREncode_AddInt64ToMapN(&e_ctx, attr_uid, getuid());
		QCBOREncode_AddInt64ToMapN(&e_ctx, attr_system_time,
					   time_since_epoch_ms);
		QCBOREncode_CloseMap(&e_ctx);
		error = QCBOREncode_Finish(&e_ctx, &enc);
	}

	*encoded_buf_len = enc.len;
	credential_buf = (uint8_t *)enc.ptr;
	return credential_buf;
}
