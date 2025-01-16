// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <stdarg.h>
#include <sys/ioctl.h>
#include "tests_private.h"

/* op is TEE_IOC_SUPPL_RECV or TEE_IOC_SUPPL_SEND. */
static int tee_call(int fd, int op, struct tee_ioctl_buf_data *buf_data)
{
	int ret;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	ret = ioctl(fd, op, buf_data);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	if (ret)
		PRINT("ioctl: %s, %d\n", strerror(errno), op);

	return ret;
}

/* arg is the root object. */
static void *test_supplicant_worker(void *arg)
{
	while (1) {
		pthread_testcancel();
		if (qcomtee_object_process_one((struct qcomtee_object *)arg,
					       tee_call)) {
			PRINT("qcomtee_object_process_one.\n");
			break;
		}
	}

	return NULL;
}

/* arg is the instance of supplicant*/
static void test_supplicant_release(void *arg)
{
	struct supplicant *sup = (struct supplicant *)arg;
	int i;

	/* Here, we are sure there is no QTEE or callback object. In other
	 * words, there should not be anyone calling qcomtee_object_invoke
	 * or any request pending from QTEE. We issue the pthread_cancel
	 * and wait for the thread to get cancelled at pthread_testcancel or
	 * get cancelled in tee_call asynchronously.
	 */

	for (i = 0; i < sup->pthreads_num; i++) {
		if (!sup->pthreads[i].state)
			pthread_cancel(sup->pthreads[i].thread);
	}

	for (i = 0; i < sup->pthreads_num; i++)
		if (!sup->pthreads[i].state)
			pthread_join(sup->pthreads[i].thread, NULL);

	PRINT("test_supplicant_worker killed.\n");
	free(sup);
}

struct supplicant *test_supplicant_start(int pthreads_num)
{
	int i, success = 0;
	struct supplicant *sup;

	if (pthreads_num > SUPPLICANT_THREADS)
		return NULL;

	/* INIT all threads as SUPPLICANT_DEAD. */
	sup = calloc(1, sizeof(*sup));
	if (!sup)
		return NULL;

	/* Start a fresh namespace. */
	sup->root =
		qcomtee_object_root_init(DEV_TEE, test_supplicant_release, sup);
	if (sup->root == QCOMTEE_OBJECT_NULL)
		goto failed_out;

	sup->pthreads_num = pthreads_num;
	/* Start supplicant threads. */
	for (i = 0; i < sup->pthreads_num; i++) {
		if (!pthread_create(&sup->pthreads[i].thread, NULL,
				    test_supplicant_worker, sup->root)) {
			sup->pthreads[i].state = SUPPLICANT_RUNNING;
			success = 1;
		}
	}

	/* Success, if at least one thread has been started. */
	if (success)
		return sup;
failed_out:
	free(sup);

	return NULL;
}

struct qcomtee_object *test_get_client_env_object(struct qcomtee_object *root)
{
	struct qcomtee_object *creds_object;
	struct qcomtee_param params[2];
	qcomtee_result_t result;

	if (qcomtee_object_credentials_init(root, &creds_object))
		return QCOMTEE_OBJECT_NULL;

	/* INIT parameters and invoke object: */
	params[0].attr = QCOMTEE_OBJREF_INPUT;
	params[0].object = creds_object;
	params[1].attr = QCOMTEE_OBJREF_OUTPUT;
	/* 2 is IClientEnv_OP_registerAsClient. */
	if (qcomtee_object_invoke(root, 2, params, 2, &result)) {
		qcomtee_object_refs_dec(creds_object);
		return QCOMTEE_OBJECT_NULL;
	}

	/* qcomtee_object_invoke was successful; QTEE releases creds_object. */

	if (!result)
		return params[1].object;

	return QCOMTEE_OBJECT_NULL;
}

struct qcomtee_object *
test_get_service_object(struct qcomtee_object *client_env_object, uint32_t uid)
{
	struct qcomtee_param params[2];
	qcomtee_result_t result;

	/* INIT parameters and invoke object: */
	params[0].attr = QCOMTEE_UBUF_INPUT;
	params[0].ubuf = UBUF_INIT(&uid);
	params[1].attr = QCOMTEE_OBJREF_OUTPUT;
	/* 0 is IClientEnv_OP_open. */
	if (qcomtee_object_invoke(client_env_object, 0, params, 2, &result) ||
	    (result != QCOMTEE_OK))
		return QCOMTEE_OBJECT_NULL;

	return params[1].object;
}

/* File stuff. */

size_t test_get_file_size(FILE *file)
{
	ssize_t size;

	if (fseek(file, 0L, SEEK_END)) {
		PRINT("%s\n", strerror(errno));
		return -1;
	}

	size = ftell(file);
	if (size < 0) {
		PRINT("%s\n", strerror(errno));
		return 0;
	}

	return size;
}

int test_read_file(const char *filename, char **buffer, size_t *size)
{
	int ret = -1;
	FILE *file;
	char *file_buf;
	size_t file_size;

	file = fopen(filename, "r");
	if (!file) {
		PRINT("%s\n", strerror(errno));
		return -1;
	}

	file_size = test_get_file_size(file);
	if (!file_size)
		goto out;

	file_buf = malloc(file_size);
	if (!file_buf) {
		PRINT("malloc,\n");
		goto out;
	}

	if (fread(file_buf, 1, file_size, file) != file_size) {
		PRINT("fread.\n");
		free(file_buf);
		goto out;
	}

	*buffer = file_buf;
	*size = file_size;

	ret = 0;
out:
	fclose(file);

	return ret;
}

int test_read_file2(const char *pathname, const char *name, char **buffer,
		    size_t *size)
{
	char filename[1024] = { 0 };
	/* Hope it fits! */
	snprintf(filename, sizeof(filename), "%s/%s", pathname, name);

	return test_read_file(filename, buffer, size);
}
