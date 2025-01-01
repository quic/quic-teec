// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _QCOMTEE_ERRNO_H
#define _QCOMTEE_ERRNO_H

#include <errno.h>
#include <stdint.h>

/* ''PUBLIC ERRORS'' */

/**
 * @def QCOMTEE_OK
 * @brief It indicates that the invocation has succeeded.
 */
#define QCOMTEE_OK 0

/**
 * @def QCOMTEE_ERROR
 * @brief It is generic error.
 */
#define QCOMTEE_ERROR 1

/**
 * @def QCOMTEE_ERROR_INVALID
 * @brief It is an invalid request.
 * 
 * This happens if (1) The parameters passed to the other domain were invalid,
 * (2) operation is unrecognized, or (3) The number and/or sizes of arguments
 * does not match what is expected for the operation.
 */
#define QCOMTEE_ERROR_INVALID 2

/**
 * @def QCOMTEE_ERROR_SIZE_IN
 * @brief It indicates that the input buffer was too large to be marshalled.
 */
#define QCOMTEE_ERROR_SIZE_IN 3

/**
 * @def QCOMTEE_ERROR_SIZE_OUT
 * @brief It indicates that the output buffer was too large to be marshalled.
 */
#define QCOMTEE_ERROR_SIZE_OUT 4

/**
 * @def QCOMTEE_ERROR_MEM
 * @brief It indicates a failure of memory allocation.
 */
#define QCOMTEE_ERROR_MEM 5

/* ''USER DEFINED ERRORS'' */

/**
 * @def QCOMTEE_ERROR_USERBASE
 * @brief It is the beginning of the user-defined range.
 * 
 * Error codes in this range can be defined on an object-by-object or
 * interface-by-interface basis.
 */
#define QCOMTEE_ERROR_USERBASE 10

/* ''TRANSPORT ERRORS'' */

/**
 * @def QCOMTEE_ERROR_DEFUNCT
 * @brief It indicates that the object reference is no longer accessible.
 */
#define QCOMTEE_ERROR_DEFUNCT -90

/**
 * @def QCOMTEE_ERROR_ABORT
 * @brief It indicates that the caller must exit.
 */
#define QCOMTEE_ERROR_ABORT -91

/**
 * @def QCOMTEE_ERROR_BADOBJ
 * @brief It indicates that the caller made a malformed invocation.
 */
#define QCOMTEE_ERROR_BADOBJ -92

/**
 * @def QCOMTEE_ERROR_NOSLOTS
 * @brief It indicates that the calling domain has reached the maximum number
 *        of object references it can export.
 */
#define QCOMTEE_ERROR_NOSLOTS -93

/**
 * @def QCOMTEE_ERROR_MAXARGS
 * @brief It indicates that the @ref qcomtee_param array exceeds the maximum
 *        arguments supported by the object or by the transport.
 */
#define QCOMTEE_ERROR_MAXARGS -94

/**
 * @def QCOMTEE_ERROR_MAXDATA
 * @brief It indicates that the payload (input buffers and/or output buffers)
 *        exceeds the supported size.
 */
#define QCOMTEE_ERROR_MAXDATA -95

/**
 * @def QCOMTEE_ERROR_UNAVAIL
 * @brief It indicates that the destination process is unavailable, but
 *        retrying the operation might result in success.
 *
 * This may be a result of resource constraints, such as exhaustion of the
 * object table in the destination process.
 */
#define QCOMTEE_ERROR_UNAVAIL -96

/**
 * @def QCOMTEE_ERROR_KMEM
 * @brief It indicates a failure of memory allocation outside of the caller's
 *        or the destination domain.
 *
 * This may occur when marshalling objects. It may also occur when passing
 * strings or other buffers that must be copied for security reasons in the
 * destination domain.
 */
#define QCOMTEE_ERROR_KMEM -97

/**
 * @def QCOMTEE_ERROR_REMOTE
 * @brief It indicates that a local operation has been requested when the
 *        target object is remote.
 *
 * Transports do not forward local operations.
 */
#define QCOMTEE_ERROR_REMOTE -98

/**
 * @def QCOMTEE_ERROR_BUSY
 * @brief It indicates that the destination process is busy and cannot
 *        currently accept an invocation.
 */
#define QCOMTEE_ERROR_BUSY -99

/**
 * @def QCOMTEE_ERROR_TIMEOUT
 * @brief It indicates that the invocation for object of type
 *        @ref qcomtee_object_type_t::QCOMTEE_OBJECT_TYPE_CB "Callback Object"
 *        has timed out.
 */
#define QCOMTEE_ERROR_TIMEOUT -103

typedef uint32_t qcomtee_result_t;

#endif // _QCOMTEE_ERRNO_H
