// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <qcbor/qcbor.h>
#include <qcomtee_object_types.h>

#define IIO_OP_GET_LENGTH 0
#define IIO_OP_READ_AT_OFFSET 1

static int64_t get_time_in_ms(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return (int64_t)(tv.tv_sec * 1000) + (int64_t)(tv.tv_usec / 1000);
}

enum {
	attr_uid = 1,
	attr_pkg_flags,
	attr_pkg_name,
	attr_pkg_cert,
	attr_permissions,
	attr_system_time,
};

#define CREDENTIALS_BUF_SIZE_INC 4096
static int realloc_useful_buf(UsefulBuf *buf)
{
	void *ptr;

	ptr = realloc(buf->ptr, buf->len + CREDENTIALS_BUF_SIZE_INC);
	if (!ptr)
		return -1;

	buf->ptr = ptr;
	buf->len += CREDENTIALS_BUF_SIZE_INC;
	memset(buf->ptr, 0, buf->len);

	return 0;
}

/* Get credentials. */
static int credentials_init(struct qcomtee_ubuf *ubuf)
{
	QCBOREncodeContext e_ctx;
	UsefulBufC enc;
	UsefulBuf creds_useful_buf = { NULL, 0 };

	do {
		if (realloc_useful_buf(&creds_useful_buf)) {
			free(creds_useful_buf.ptr);
			return -1;
		}

		/* Use UID and system time to create a CBOR buffer. */
		QCBOREncode_Init(&e_ctx, creds_useful_buf);
		QCBOREncode_OpenMap(&e_ctx);
		QCBOREncode_AddInt64ToMapN(&e_ctx, attr_uid, getuid());
		QCBOREncode_AddInt64ToMapN(&e_ctx, attr_system_time,
					   get_time_in_ms());
		QCBOREncode_CloseMap(&e_ctx);
	} while (QCBOREncode_Finish(&e_ctx, &enc) ==
		 QCBOR_ERR_BUFFER_TOO_SMALL);

	ubuf->addr = (void *)enc.ptr;
	ubuf->size = enc.len;

	return 0;
}

/* CREDENTIAL callback object. */
struct qcomtee_credentials {
	struct qcomtee_object object;
	struct qcomtee_ubuf ubuf;
};

#define CREDENTIALS(o) container_of((o), struct qcomtee_credentials, object)

static void qcomtee_object_credentials_release(struct qcomtee_object *object)
{
	struct qcomtee_credentials *qcomtee_cred = CREDENTIALS(object);

	free(qcomtee_cred->ubuf.addr);
	free(qcomtee_cred);
}

#define MIN_SIZE_T(a, b) ((a) < (b) ? (a) : (b))

static qcomtee_result_t
qcomtee_object_credentials_dispatch(struct qcomtee_object *object,
				    qcomtee_op_t op,
				    struct qcomtee_param *params, int num)
{
	struct qcomtee_credentials *qcomtee_cred = CREDENTIALS(object);

	switch (op) {
	case IIO_OP_GET_LENGTH:
		/* Expect one argument: ouput buffer. */
		if (num != 1 || params[0].attr != QCOMTEE_UBUF_OUTPUT)
			return QCOMTEE_ERROR_INVALID;

		params[0].ubuf.addr = &qcomtee_cred->ubuf.size;
		params[0].ubuf.size = sizeof(qcomtee_cred->ubuf.size);

		break;
	case IIO_OP_READ_AT_OFFSET: {
		uint64_t offset;

		/* Expect two arguments: input and outout buffer. */
		if (num != 2 || params[0].attr != QCOMTEE_UBUF_INPUT ||
		    params[1].attr != QCOMTEE_UBUF_OUTPUT)
			return QCOMTEE_ERROR_INVALID;

		offset = *((uint64_t *)(params[0].ubuf.addr));
		if (offset >= qcomtee_cred->ubuf.size)
			return QCOMTEE_ERROR_INVALID;

		params[1].ubuf.addr = qcomtee_cred->ubuf.addr + offset;
		params[1].ubuf.size = MIN_SIZE_T(
			qcomtee_cred->ubuf.size - offset, params[1].ubuf.size);

		break;
	}
	default:
		/* NEVER GET HERE! */
		return QCOMTEE_ERROR_INVALID;
	}

	return QCOMTEE_OK;
}

static struct qcomtee_object_ops ops = {
	.release = qcomtee_object_credentials_release,
	.dispatch = qcomtee_object_credentials_dispatch,
};

qcomtee_result_t
qcomtee_object_credentials_init(struct qcomtee_object *root_object,
				struct qcomtee_object **object)
{
	struct qcomtee_credentials *qcomtee_cred;

	qcomtee_cred = calloc(1, sizeof(*qcomtee_cred));
	if (!qcomtee_cred)
		return QCOMTEE_ERROR_MEM;

	qcomtee_object_cb_init(&qcomtee_cred->object, &ops, root_object);
	/* INIT the credentials buffer. */
	if (credentials_init(&qcomtee_cred->ubuf)) {
		free(qcomtee_cred);

		return QCOMTEE_ERROR_MEM;
	}

	/* Get the credentials object. */
	*object = &qcomtee_cred->object;

	return QCOMTEE_OK;
}
