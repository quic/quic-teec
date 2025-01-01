// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _CREDENTIALS_H
#define _CREDENTIALS_H

#include "qcom_tee_defs.h"

/**
 * get_credentials() - Generate a buffer containing the client's credentials
 * to establish communication with QTEE. The buffer contents are encoded in
 * a QCBOR format.
 *
 * @param encoded_buf_len   The length of the returned credential buffer
 *
 * @return credentials      Pointer to a buffer containing the client's
 *                          credentials.
 */
void *get_credentials(size_t *encoded_buf_len);

#endif // _CREDENTIALS_H
