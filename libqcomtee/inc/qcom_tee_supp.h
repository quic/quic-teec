// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _QCOM_TEE_SUPP_H
#define _QCOM_TEE_SUPP_H

#include "qcom_tee_defs.h"

/**
 * The maximum number of threads associated with this Callback Supplicant.
 */
#define CBO_THREADS_MAX_LIMIT 4

/**
 * The size of the u-space buffer sent to QCOMTEE for receiving the callback
 * request.
 */
#define CBO_BUFFER_LENGTH (4 * 1024)

/**
 * The maximum number of parameters transferred between the Callback Supplicant
 * and the QCOMTEE driver.
 */
#define CBO_NUM_PARAMS 10

/**
 * The maximum number of callback objects stored by the Callback Supplicant.
 */
#define CBO_TABLE_SIZE 256

/**
 * Retrieve the ID of the next supplicant thread from the current one.
 */
#define NEXT_SUPP_ID(i) (i + 1) % CBO_THREADS_MAX_LIMIT

/**
 * The supplicant thread meta-data
 * and resources
 */
typedef struct {

	pthread_t tid;
	bool alive;

	QCOMTEE_Object *curr_cbo;
} SupplicantThread;

typedef struct {
	/**
	 * This lock is contended on between the supplicant and application
	 * threads. It is important that the worker threads do not frequently
	 * try to obtain this lock, and when they do so, release it quickly,
	 * otherwise, application threads wouldn't be able to shutdown
	 * the Callback Supplicant.
	 *
	 * This works for our scenario since our worker threads are mostly sleeping,
	 * and Callback Supplicant shutdown is a one-time operation.
	 */
	pthread_mutex_t supp_lock;

	int fd;

	/**
	* How many supplicant/worker threads are waiting in the kernel?
	 */
	size_t num_waiters;

	/**
	 * All supplicant threads associated with this Callback Supplicant.
	 * Entries in this table are filled when any supplicant/worker thread
	 * create a new thread. It must be synchronized with supp_lock.
	 */
	SupplicantThread threadTable[CBO_THREADS_MAX_LIMIT];

	/**
	 * A callback object table to host all the callback objects sent to
	 * QTEE by the client.
	*/
	QCOMTEE_Object *cbo_table[CBO_TABLE_SIZE];

} CallbackSupplicant;

static void *supplicant_thread_func(void *data);

QCOMTEE_Result add_callback_object(QCOMTEE_Object *object);

QCOMTEE_Result start_supplicant(int fd);

QCOMTEE_Result stop_supplicant(void);

#endif // _QCOM_TEE_SUPP_H
