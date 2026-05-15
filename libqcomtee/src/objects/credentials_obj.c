// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <qcomtee_object_types.h>
#ifdef USE_QCBOR
#include <qcbor/qcbor.h>
#else
#include <cbor.h>
#include <string.h>
#endif

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
#ifdef USE_QCBOR
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
#else
static int credentials_init(struct qcomtee_ubuf *ubuf)
{
	cbor_mutable_data buffer = NULL;
	cbor_item_t *creds_map = NULL;
	struct cbor_pair map_pair;
	size_t buffer_size = CREDENTIALS_BUF_SIZE_INC;

	buffer = (unsigned char *)calloc(buffer_size, sizeof(unsigned char));
	if (buffer == NULL)
		return -1;

	creds_map = cbor_new_definite_map(2);
	if (creds_map == NULL)
		goto map_init_fail;

	map_pair.key = cbor_build_uint8(attr_uid);
	map_pair.value = cbor_build_uint32((uint32_t)getuid());
	if (!cbor_map_add(creds_map, map_pair))
		goto map_add_fail;

	map_pair.key = cbor_build_uint8(attr_system_time);
	map_pair.value = cbor_build_uint64((uint64_t)get_time_in_ms());
	if (!cbor_map_add(creds_map, map_pair))
		goto map_add_fail;

	/*
	 * On failure in serialization we jump to map_serialize_fail
	 * because cbor_decref() recursively frees all items already
	 * added to the map. However, if cbor_map_add() fails, we
	 * must manually decref the current key/value as ownership
	 * is not transferred.
	 */
	buffer_size = cbor_serialize_map(creds_map, buffer, buffer_size);
	if (buffer_size == 0)
		goto map_serialize_fail;

	ubuf->addr = (void *)buffer;
	ubuf->size = buffer_size;

	cbor_decref(&creds_map);
	return 0;

map_add_fail:
	cbor_decref(&map_pair.key);
	cbor_decref(&map_pair.value);
map_serialize_fail:
	cbor_decref(&creds_map);
map_init_fail:
	free(buffer);
	return -1;
}
#endif

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

int qcomtee_object_credentials_init(struct qcomtee_object *root,
				    struct qcomtee_object **object)
{
	struct qcomtee_credentials *qcomtee_cred;

	qcomtee_cred = calloc(1, sizeof(*qcomtee_cred));
	if (!qcomtee_cred)
		return -1;

	qcomtee_object_cb_init(&qcomtee_cred->object, &ops, root);
	/* INIT the credentials buffer. */
	if (credentials_init(&qcomtee_cred->ubuf)) {
		free(qcomtee_cred);

		return -1;
	}

	/* Get the credentials object. */
	*object = &qcomtee_cred->object;

	return 0;
}
