// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _CREDENTIALS_OBJ_H
#define _CREDENTIALS_OBJ_H

#include "qcom_tee_defs.h"

#define IIO_OP_getLength    0
#define IIO_OP_readAtOffset 1

/**
 * struct credentials - Private data for the credentials object, contains the
 * credentials of the client.
 *
 * @param   cred_buffer   Buffer holding the client's credentials
 * @param   cred_buf_len  Length of the crendential buffer.
 *
 */
typedef struct {
	uint8_t *cred_buffer;
	size_t cred_buf_len;

} credentials;

/**
 * GetCredentialsObject() - Get a credential object which can be sent to
 * QTEE to obtain a ClientEnv Object.
 *
 * @param creds_object       A credential object representing the client's
 *                           credentials.
 *
 * @return QCOMTEE_SUCCESS   The credential object was successfully created.
 * @return QCOMTEE_Result    Something failed.
 */
QCOMTEE_Result GetCredentialsObject(QCOMTEE_Object **creds_object);

#endif // _CREDENTIALS_OBJ_H
