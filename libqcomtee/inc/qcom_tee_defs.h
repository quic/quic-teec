// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _QCOM_TEE_DEFS_H
#define _QCOM_TEE_DEFS_H

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#ifdef OE // OpenEmbedded
#include <syslog.h>
#define MSGV(...) syslog(LOG_NOTICE, "INFO:" __VA_ARGS__)
#define MSGD(...) syslog(LOG_DEBUG, "INFO:" __VA_ARGS__)
#define MSGE(...) syslog(LOG_ERR, "ERR:" __VA_ARGS__)
#else
#include <stdio.h>
#define MSGV printf
#define MSGD printf
#define MSGE printf
#endif

/*
===========================================================================
                         ERROR DEFINITIONS
===========================================================================
*/
/**
 * Error Values - Type is QCOMTEE_Result
 *
 * A value of zero (QCOMTEE_MSG_OK) indicates that the invocation has
 * succeeded.
 *
 * Negative values are reserved for use by transports and indicate problems
 * communicating between domains.
 *
 * Positive error codes are sub-divided into generic and user-defined errors.
 * User-case-specific error codes should be allocated from the user-defined
 * range.
 */

/**
 * QCOMTEE_MSG_OK indicates that the invocation has succeeded.
 */
#define QCOMTEE_MSG_OK                  0

/**
 * QCOM_TEE_MSG_ERROR is non-specific, and can be used whenever the error
 * condition does not need to be distinguished from others in the interface.
 */
#define QCOMTEE_MSG_ERROR               1

/**
 * QCOMTEE_MSG_ERROR_INVALID indicates that the request was not understood by
 * the remote domain. This can result when:
 * 1. The parameters passed to the other domain were invalid.
 * 2. `op` is unrecognized, or
 * 3. The number and/or sizes of arguments does not match what is expected for
 *    `op`.
 */
#define QCOMTEE_MSG_ERROR_INVALID       2

/**
 * QCOMTEE_MSG_ERROR_SIZE_IN that an input buffer was too large to be
 * marshalled.
 */
#define QCOMTEE_MSG_ERROR_SIZE_IN       3

/**
 * QCOMTEE_MSG_ERROR_SIZE_OUT that an output buffer was too large to be
 * marshalled.
 */
#define QCOMTEE_MSG_ERROR_SIZE_OUT      4

/**
 * QCOMTEE_MSG_ERROR_MEM indicates a failure of memory allocation.
 */
#define QCOMTEE_MSG_ERROR_MEM           5

/**
 * QCOMTEE_MSG_ERROR_USERBASE is the beginning of the user-defined range.
 * Error codes in this rangecan be defined on an object-by-object or
 * interface-by-interface basis.
 */
#define QCOMTEE_MSG_ERROR_USERBASE     10

/**
 * QCOMTEE_MSG_ERROR_NOT_FOUND indicates that the requested object or item
 * was not found.
 */
#define QCOMTEE_MSG_ERROR_NOT_FOUND    11

/**
 * QCOMTEE_MSG_ERROR_DEFUNCT indicates that the object reference will no
 * longer work.  This is returned when the process hosting the object has
 * terminated, or when the communication link to the object is unrecoverably
 * lost.
 */
#define QCOMTEE_MSG_ERROR_DEFUNCT     -90

/**
 * QCOMTEE_MSG_ERROR_ABORT indicates that the caller should return to the point
 * at which it was invoked from a remote domain.  Unlike other error codes,
 * this pertains to the state of the calling thread, not the state of the
 * target object or transport.
 *
 * For example, when a process is terminated while a kernel thread is
 * executing on its behalf, that kernel thread should return back to its
 * entry point so that it can be reaped safely. (Synchronously killing the
 * thread could leave kernel data structures in a corrupt state.) If it
 * attempts to invoke an object that would result in it blocking on another
 * thread (or if it is already blocking in an invokcation) the invocation
 * will immediately return this error code.
 */
#define QCOMTEE_MSG_ERROR_ABORT       -91

/**
 * QCOMTEE_MSG_ERROR_BADOBJ indicates that the caller provided a mal-formed
 * Object structure as a target object or an input parameter.
 */
#define QCOMTEE_MSG_ERROR_BADOBJ      -92

/**
 * QCOMTEE_MSG_ERROR_NOSLOTS indicates that an object could not be returned
 * because the calling domain has reached the maximum number of remote
 * object references on this transport.
 */
#define QCOMTEE_MSG_ERROR_NOSLOTS     -93

/**
 * QCOMTEE_MSG_ERROR_MAXARGS indicates that the `qcom_tee_params` length
 * exceeds the maximum supported by the object or by some transport between the
 * caller and the object.
 */
#define QCOMTEE_MSG_ERROR_MAXARGS     -94

/**
 * QCOMTEE_MSG_ERROR_MAXDATA indicates the the complete payload (input buffers
 * and/or output buffers) exceed
 */
#define QCOMTEE_MSG_ERROR_MAXDATA     -95

/**
 * QCOMTEE_MSG_ERROR_UNAVAIL indicates that the destination process cannot
 * fulfill the request at the current time, but that retrying the operation
 * in the future might result in success.
 *
 * This may be a result of resource constraints, such as exhaustion of the
 * object table in the destination process.
 */
#define QCOMTEE_MSG_ERROR_UNAVAIL     -96

/**
 * QCOMTEE_MSG_ERROR_KMEM indicates a failure of memory allocation outside of
 * the caller's domain and outside of the destination domain.
 *
 * This may occur when marshalling objects. It may also occur when passing
 * strings or other buffers that must be copied for security reasons in the
 * destination domain.
 */
#define QCOMTEE_MSG_ERROR_KMEM        -97

/**
 * QCOMTEE_MSG_ERROR_REMOTE indicates that a *local* operation has been requested
 * when the target object is remote. Transports do not forward local
 * operations.
 */
#define QCOMTEE_MSG_ERROR_REMOTE      -98

/**
 * QCOMTEE_MSG_ERROR_BUSY indicates that the target domain or process is busy and
 * cannot currently accept an invocation.
 */
#define QCOMTEE_MSG_ERROR_BUSY        -99

/**
 * QCOMTEE_MSG_TIMEOUT indicates that the Callback Object invocation has timed
 * out.
 */
#define QCOMTEE_MSG_TIMEOUT          -103

typedef uint32_t QCOMTEE_Result;

static inline QCOMTEE_Result ioctl_errno_to_res(int err)
{
	switch (err) {
	case ENOMEM:
		return QCOMTEE_MSG_ERROR_MEM;
	case EINVAL:
		return QCOMTEE_MSG_ERROR_INVALID;
        case EBUSY:
                return QCOMTEE_MSG_ERROR_BUSY;
	default:
		return QCOMTEE_MSG_ERROR;
	}
}

/*
===========================================================================
                         OBJECT DEFINITIONS
===========================================================================
*/
#define QCOMTEE_OBJREF_NULL         (uint64_t)(-1)
#define QCOMTEE_OBJREF_USER         (1 << 0)

#define CALLBACK_OBJ_ID_MIN         0x80000000

typedef struct QCOMTEE_Param QCOMTEE_Param;

#define QCOMTEE_Object_NULL         ((QCOMTEE_Object *)(NULL))

/**
 * The type of the Object Reference being exchanged
 * between the client and QTEE.
 *
 * QCOM_TEE_OBJECT_TYPE_TEE       - Remote Object hosted in the QTEE.
 * QCOM_TEE_OBJECT_TYPE_CB        - Callback Object hosted in client.
 * QCOM_TEE_OBJECT_TYPE_ROOT      - Root Object which can be used to initiate
 *                                  object exchange with QTEE.
 */
typedef enum {
	QCOM_TEE_OBJECT_TYPE_TEE,
	QCOM_TEE_OBJECT_TYPE_ROOT,
	QCOM_TEE_OBJECT_TYPE_CB,
} QCOMTEE_ObjectType;

typedef struct QCOMTEE_Object QCOMTEE_Object;
struct QCOMTEE_Object {
	atomic_int refs;

	uint64_t object_id;
	QCOMTEE_ObjectType object_type;

	bool queued;

	union {
		void *data;

		int fd;
	};

	void (*release)(QCOMTEE_Object *object);

	QCOMTEE_Result (*dispatch)(QCOMTEE_Object *object,
				    uint32_t op,
				    uint32_t *num_params,
				    QCOMTEE_Param *qcom_tee_params,
				    uint8_t *cbo_buffer,
				    size_t cbo_len);
};

/*
===========================================================================
                         OBJECT HELPER FUNCTIONS/MACROS
===========================================================================
*/

/**
 * QCOMTEE_ReleaseObject() - Release the reference to the given object.
 *
 * @param object   The object for which the reference needs to be released.
 *
 */
void QCOMTEE_ReleaseObject(QCOMTEE_Object *object);

/**
 * QCOMTEE_RetainObject() - Retain the reference to the given object.
 *
 * @param object                  The object for which the reference needs
 *                                to be retained.
 *
 * @return QCOMTEE_Object*       The object was successfully retained.
 * @return QCOMTEE_Object_NULL   Failed to retain reference to the object.
 */
QCOMTEE_Object *QCOMTEE_RetainObject(QCOMTEE_Object *object);

#define QCOMTEE_OBJECT_RELEASE(o) \
        if (o) QCOMTEE_ReleaseObject(o)

#define QCOMTEE_OBJECT_RETAIN(o) \
        (o) ? QCOMTEE_RetainObject(o) : QCOMTEE_Object_NULL

#define QCOMTEE_OBJECT_DISPATCH(o, op, n, params, buf, len) \
        (o)->dispatch(o, op, n, params, buf, len)
/*
===========================================================================
                         MEMORY DEFINITIONS
===========================================================================
*/

/**
 * struct QCOMTEE_UserBuffer - A user allocated Memory Buffer to transfer data
 * between a client and QTEE, only used for the duration of the object invocation.
 *
 * @param buffer  The Memory Buffer which is to be, or has been shared with
 *                QTEE.
 * @param size    The size, in bytes, of the Memory Buffer.
 */
typedef struct {
	void *buffer;
	size_t size;
} QCOMTEE_UserBuffer;

/*
===========================================================================
                         TYPE DEFINITIONS
===========================================================================
*/

/**
 * Flag constants indicating the type of parameters encoded inside the
 * invoke request (QCOMTEE_Param), Type is uint64_t.
 *
 * QCOMTEE_UBUF_INPUT           A Memory Buffer allocated by the user for
 *                               sharing data with QTEE for the duration of the
 *                               object invocation and is tagged as input.
 *
 * QCOMTEE_UBUF_OUTPUT          Same as QCOMTEE_UBUF_INPUT, but the Memory
 *                               Buffer is tagged as output. The implementation
 *                               may update the size field to reflect the
 *                               required output size in some use cases.
 *
 * QCOMTEE_UBUF_INOUT           A Memory Buffer tagged as both input and
 *                               output, i.e., for which both the behaviors
 *                               of QCOMTEE_UBUF_INPUT and QCOMTEE_UBUF_OUTPUT
 *                               apply.
 *
 * QCOMTEE_OBJREF_INPUT         An Object Reference tagged as input,
 *                               representing a service hosted in the client,
 *                               which can be sent to QTEE. QTEE can invoke the
 *                               Object Reference later to request the service.
 *
 * QCOMTEE_OBJREF_OUTPUT        Same as QCOMTEE_OBJREF_INPUT, but the Object
 *                               Reference is tagged as output. The client can
 *                               invoke this Object Reference to request a
 *                               service hosted in QTEE.
 *
 * QCOMTEE_OBJREF_INOUT         An Object Reference tagged as both input and
 *                               output, i.e., for which both the behaviors of
 *                               QCOMTEE_OBJREF_INPUT and QCOMTEE_OBJREF_OUTPUT
 *                               apply
 */
#define QCOMTEE_UBUF_INPUT     0x00000008
#define QCOMTEE_UBUF_OUTPUT    0x00000009
#define QCOMTEE_UBUF_INOUT     0x0000000A
#define QCOMTEE_OBJREF_INPUT   0x0000000B
#define QCOMTEE_OBJREF_OUTPUT  0x0000000C
#define QCOMTEE_OBJREF_INOUT   0x0000000D

/**
 * union QCOMTEE_Param - Container to be used when passing data between client
 *                        application and QTEE.
 *
 * Either the client uses a Memory Buffer or an Object Reference.
 *
 * @param ubuf     A user allocated Memory Buffer valid for the duration of the
 *                 object invocation, tagged as output.
 *
 * @param object   An Object Reference exchanged between the client and QTEE,
 *                 tagged as either input or output.
 */

struct QCOMTEE_Param {
	uint64_t attr;

	union {
		QCOMTEE_UserBuffer ubuf;
		QCOMTEE_Object *object;
	};
};

#endif // _QCOM_TEE_DEFS_H
