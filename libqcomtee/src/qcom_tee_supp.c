// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <assert.h>
#include <linux/tee.h>
#include <string.h>
#include <sys/ioctl.h>

#include "qcom_tee_obj.h"
#include "qcom_tee_supp.h"

/*
===========================================================================
                          GLOBAL DATA STRUCTURES
===========================================================================
*/

static CallbackSupplicant callback_supplicant = {
	PTHREAD_MUTEX_INITIALIZER,
	-1,
	0,
	{ { .alive = false, .curr_cbo = QCOMTEE_Object_NULL } },
	{ QCOMTEE_Object_NULL },
};

/*
===========================================================================
                          PRIVATE/HELPER FUNCTIONS
===========================================================================
*/
static QCOMTEE_Result post_process_suppl_recv(int fd, uint32_t num_params,
					      struct tee_ioctl_param *params,
					      QCOMTEE_Param *qcom_tee_params)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	QCOMTEE_Object *remote_object = QCOMTEE_Object_NULL;

	for (uint32_t n = 0; n < num_params; n++) {
		switch (params[n].attr) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_INPUT:
			qcom_tee_params[n].attr = QCOMTEE_UBUF_INPUT;
			qcom_tee_params[n].ubuf.buffer = (void *)params[n].a;
			qcom_tee_params[n].ubuf.size = (size_t)params[n].b;
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_OBJREF_INPUT:
			res = init_qcom_tee_remote_object(params[n].a,
							  &remote_object);
			if (res != QCOMTEE_MSG_OK) {
				return res;
			}

			qcom_tee_params[n].attr = QCOMTEE_OBJREF_INPUT;
			qcom_tee_params[n].object = remote_object;
			break;
		default:
			return QCOMTEE_MSG_ERROR;
		}
	}

	return QCOMTEE_MSG_OK;
}

static QCOMTEE_Result pre_process_suppl_send(uint32_t num_params,
					     QCOMTEE_Param *qcom_tee_params,
					     struct tee_ioctl_param *params)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;

	for (uint32_t n = 0; n < num_params; n++) {
		switch (qcom_tee_params[n].attr) {
		case QCOMTEE_UBUF_OUTPUT:
			params[n].attr = TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_OUTPUT;
			params[n].a =
				(uint64_t)(qcom_tee_params[n].ubuf.buffer);
			params[n].b = (uint64_t)(qcom_tee_params[n].ubuf.size);
			break;
		case QCOMTEE_OBJREF_OUTPUT:
			res = add_callback_object(qcom_tee_params[n].object);
			if (res != QCOMTEE_MSG_OK) {
				return res;
			}

			params[n].attr =
				TEE_IOCTL_PARAM_ATTR_TYPE_OBJREF_OUTPUT;
			params[n].a = (qcom_tee_params[n].object)->object_id;
			params[n].b = QCOMTEE_OBJREF_USER;
			break;
		default:
			return QCOMTEE_MSG_ERROR;
		}
	}

	return QCOMTEE_MSG_OK;
}

static QCOMTEE_Result get_callback_object(uint64_t object_id,
					  QCOMTEE_Object **callback_object,
					  uint8_t index)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR_NOT_FOUND;
	QCOMTEE_Object *cbo = QCOMTEE_Object_NULL;

	pthread_mutex_lock(&callback_supplicant.supp_lock);
	for (size_t i = 0; i < CBO_TABLE_SIZE; i++) {
		cbo = callback_supplicant.cbo_table[i];
		// Match?
		if (cbo && cbo->object_id == object_id) {
			QCOMTEE_OBJECT_RETAIN(cbo);
			*callback_object = cbo;

			// Account the CBO I am currently operating on
			callback_supplicant.threadTable[index].curr_cbo = cbo;

			res = QCOMTEE_MSG_OK;
			break;
		}
	}
	pthread_mutex_unlock(&callback_supplicant.supp_lock);

	return res;
}

static size_t inc_num_waiters(void)
{
	size_t waiters = 0;

	pthread_mutex_lock(&callback_supplicant.supp_lock);

	callback_supplicant.num_waiters++;
	assert(callback_supplicant.num_waiters);
	waiters = callback_supplicant.num_waiters;

	pthread_mutex_unlock(&callback_supplicant.supp_lock);

	return waiters;
}

static size_t dec_num_waiters(void)
{
	size_t waiters = 0;

	pthread_mutex_lock(&callback_supplicant.supp_lock);

	assert(callback_supplicant.num_waiters);
	callback_supplicant.num_waiters--;
	waiters = callback_supplicant.num_waiters;

	pthread_mutex_unlock(&callback_supplicant.supp_lock);

	return waiters;
}

/**
 * create_supplicant_thread() - Create a new supplicant
 * thread.
 *
 * MUST be called after locking supp_lock.
 * NO blocking calls allowed inside this function.
 */
static QCOMTEE_Result create_supplicant_thread(uint8_t index)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	int ret = 0;

	if (callback_supplicant.threadTable[index].alive)
		return res;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	// Create all threads as joinable
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&callback_supplicant.threadTable[index].tid, &attr,
			     supplicant_thread_func, (void *)index);
	if (ret) {
		MSGE("pthread_create(%d) failed: %d\n", index, ret);
		return res;
	}

	callback_supplicant.threadTable[index].alive = true;

	return QCOMTEE_MSG_OK;
}

static void *supplicant_thread_func(void *data)
{
	QCOMTEE_Result res = QCOMTEE_MSG_OK;
	int ret = -1;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	uint8_t index = (uint8_t)data;
	int fd = callback_supplicant.fd;

	// Values to be received from TEE
	uint32_t func = 0;
	uint64_t object_id = 0;
	uint64_t request_id = 0;
	uint32_t num_params = 0;

	// Local parameters
	QCOMTEE_Param qcom_tee_params[CBO_NUM_PARAMS] = { 0 };
	QCOMTEE_Object *callback_object = QCOMTEE_Object_NULL;

	uint8_t cbo_buffer[CBO_BUFFER_LENGTH] = { 0 };
	size_t cbo_buffer_len = CBO_BUFFER_LENGTH;

	struct tee_ioctl_param *params = NULL;
	const size_t arg_size =
		sizeof(struct tee_iocl_supp_send_arg) +
		((CBO_NUM_PARAMS + 1) * sizeof(struct tee_ioctl_param));
	union {
		struct tee_iocl_supp_recv_arg recv_arg;
		struct tee_iocl_supp_send_arg send_arg;
		uint8_t data[arg_size];
	} buf;

	struct tee_ioctl_buf_data buf_data = { 0 };
	buf_data.buf_ptr = (uintptr_t)&buf;
	buf_data.buf_len = sizeof(buf);

	do {
		memset(qcom_tee_params, 0, sizeof(QCOMTEE_Param));
		callback_object = NULL;
		memset(cbo_buffer, 0, cbo_buffer_len);
		memset(&buf, 0, sizeof(buf));

		/* The back-end driver only expects 1 parameter for
		 * meta-data
		 */
		buf.recv_arg.num_params = (1 + CBO_NUM_PARAMS);
		params = (struct tee_ioctl_param *)(&buf.recv_arg + 1);
		params->attr = (TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT |
				TEE_IOCTL_PARAM_ATTR_META);
		params->a = (__u64)cbo_buffer;
		params->b = (__u64)cbo_buffer_len;
		params->c = 0;

		inc_num_waiters();
		// We wait within the kernel for a callback request
		ret = ioctl(fd, TEE_IOC_SUPPL_RECV, &buf_data);
		/* Generate a new thread to execute supplicant_thread_func()
		 * if and only if, there are no waiting threads.
		 */
		if (!dec_num_waiters() && NEXT_SUPP_ID(index)) {
			pthread_mutex_lock(&callback_supplicant.supp_lock);
			create_supplicant_thread(NEXT_SUPP_ID(index));
			pthread_mutex_unlock(&callback_supplicant.supp_lock);
		}

		if (ret) {
			MSGE("TEE_IOC_SUPPL_RECV: errno = %d\n", errno);
			res = ioctl_errno_to_res(errno);
			break;
		}

		func = (uint32_t)buf.recv_arg.func;
		num_params = (uint32_t)buf.recv_arg.num_params - 1;
		object_id = (uint64_t)params->a;
		request_id = (uint64_t)params->b;
		params++;

		res = get_callback_object(object_id, &callback_object, index);
		if (res == QCOMTEE_MSG_OK) {
			res = post_process_suppl_recv(fd, num_params, params,
						      qcom_tee_params);
			if (res != QCOMTEE_MSG_OK) {
				MSGE("post_process_suppl_recv failed: 0x%x\n",
				     res);
			}

			if (res == QCOMTEE_MSG_OK) {
				res = QCOMTEE_OBJECT_DISPATCH(callback_object,
							      func, &num_params,
							      qcom_tee_params,
							      cbo_buffer,
							      cbo_buffer_len);

				if (res != QCOMTEE_MSG_OK) {
					MSGE("Dispatch failed for callback"
					     "object 0x%lx: 0x%x\n",
					     object_id, res);
				}
			}

			// We're done with this object reference
			pthread_mutex_lock(&callback_supplicant.supp_lock);
			QCOMTEE_OBJECT_RELEASE(callback_object);
			callback_supplicant.threadTable[index].curr_cbo = NULL;
			pthread_mutex_unlock(&callback_supplicant.supp_lock);
		} else {
			MSGE("Callback object 0x%lx not found.\n", object_id);
		}

		params = (struct tee_ioctl_param *)(&buf.send_arg + 1);
		params->attr = (TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT |
				TEE_IOCTL_PARAM_ATTR_META);
		params->a = request_id;
		params->b = 0;
		params->c = 0;
		params++;

		if (res == QCOMTEE_MSG_OK) {
			res = pre_process_suppl_send(num_params,
						     qcom_tee_params, params);

			if (res != QCOMTEE_MSG_OK) {
				MSGE("pre_process_suppl_send failed: 0x%x\n",
				     res);
			}
		}

		buf.send_arg.ret = res;
		buf.send_arg.num_params = num_params + 1;

		/* It is crucial for this IOCTL to succeed; otherwise,
		 * QTEE may be left waiting indefinitely, potentially
		 * causing a QTEE Busy scenario.
		 * If this fails, the only option is to terminate this thread.
		 */
		ret = ioctl(fd, TEE_IOC_SUPPL_SEND, &buf_data);
		if (ret) {
			MSGE("TEE_IOC_SUPPL_SEND: errno = %d\n", errno);
			res = ioctl_errno_to_res(errno);
			break;
		}

	} while (1);

	pthread_mutex_lock(&callback_supplicant.supp_lock);
	callback_supplicant.threadTable[index].alive = false;
	pthread_mutex_unlock(&callback_supplicant.supp_lock);

	pthread_exit((void *)&res);
}

/*
===========================================================================
                          PUBLIC FUNCTIONS
===========================================================================
*/

QCOMTEE_Result add_callback_object(QCOMTEE_Object *object)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR_INVALID;
	QCOMTEE_Object *cbo_obj = QCOMTEE_Object_NULL;

	if (object->object_type != QCOM_TEE_OBJECT_TYPE_CB) {
		MSGE("Invalid object type: 0x%x\n", res);
		return res;
	}

	if (!object->release || !object->dispatch) {
		MSGE("Object operations not defined: 0x%x\n", res);
		return res;
	}

	if (object->queued) {
		QCOMTEE_OBJECT_RETAIN(object);
		return QCOMTEE_MSG_OK;
	}

	res = QCOMTEE_MSG_ERROR_MEM;

	pthread_mutex_lock(&callback_supplicant.supp_lock);
	for (size_t i = 0; i < CBO_TABLE_SIZE; i++) {
		cbo_obj = callback_supplicant.cbo_table[i];
		if (!cbo_obj) {
			/* Increment the object's ref-count since a
			 * reference to it is now held in cbo_table
			 */
			QCOMTEE_OBJECT_RETAIN(object);
			callback_supplicant.cbo_table[i] = object;
			object->queued = true;

			res = QCOMTEE_MSG_OK;
			break;
		}
	}
	pthread_mutex_unlock(&callback_supplicant.supp_lock);

	return res;
}

QCOMTEE_Result start_supplicant(int fd)
{
	QCOMTEE_Result res = QCOMTEE_MSG_OK;

	callback_supplicant.fd = fd;

	res = create_supplicant_thread(0);
	if (res != QCOMTEE_MSG_OK) {
		MSGE("create_supplicant_thread failed: 0x%x\n", res);
		return res;
	}

	return res;
}

QCOMTEE_Result stop_supplicant(void)
{
	QCOMTEE_Result res = QCOMTEE_MSG_OK;
	pthread_t tid;
	QCOMTEE_Object *cbo;

	/* When stopping supplicat threads
	* 1. Kill the threads and wait for them to die
	* 2. Release all resources (mutexes, objects)
	*    held by the threads inside and outside their scope.
	* 3. Clear out CBO Table and call release.
	*/
	pthread_mutex_lock(&callback_supplicant.supp_lock);

	for (size_t i = 0; i < CBO_THREADS_MAX_LIMIT; i++) {
		if (callback_supplicant.threadTable[i].alive) {
			tid = callback_supplicant.threadTable[i].tid;
			cbo = callback_supplicant.threadTable[i].curr_cbo;

			pthread_cancel(tid);
			pthread_join(tid, NULL);

			/* If the supplicant thread held a reference
			 * to a CBO, release it
			 */
			if (cbo)
				QCOMTEE_OBJECT_RELEASE(cbo);

			callback_supplicant.threadTable[i].alive = false;
		}
	}

	/* Release all existing references to CBOs in
	 * the CBO Table
	 */
	for (size_t i = 0; i < CBO_TABLE_SIZE; i++) {
		cbo = callback_supplicant.cbo_table[i];
		if (cbo)
			QCOMTEE_OBJECT_RELEASE(cbo);
	}

	callback_supplicant.fd = -1;

	pthread_mutex_unlock(&callback_supplicant.supp_lock);

	return res;
}
