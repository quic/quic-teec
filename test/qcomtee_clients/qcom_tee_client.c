// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "credentials.h"

#include "qcom_tee_api.h"
#include "qcom_tee_client.h"

static int send_add_cmd(QCOMTEE_Object *app_object, int num1, int num2)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	uint32_t result = 0;

	struct {
		uint32_t val1;
		uint32_t val2;
	} i = { num1, num2 };

	uint32_t num_params = 2;
	QCOMTEE_Param params[num_params];

	memset(params, 0, sizeof(params));
	params[0].attr = QCOMTEE_UBUF_INPUT;
	params[0].ubuf.buffer = (void *)&i;
	params[0].ubuf.size = 8;

	params[1].attr = QCOMTEE_UBUF_OUTPUT;
	params[1].ubuf.buffer = (void *)&result;
	params[1].ubuf.size = sizeof(uint32_t);

	res = QCOMTEE_InvokeObject(app_object, 0 /* ISMCIExample_OP_add */,
				   num_params, params);
	if (res != QCOMTEE_MSG_OK) {
		MSGE("QCOMTEE_InvokeObject failed: 0x%x\n", res);
		return -1;
	}

	printf("Sum %d + %d = %d\n", i.val1, i.val2, result);

	return 0;
}

static int get_app_object(QCOMTEE_Object *app_controller_object,
			  QCOMTEE_Object **app_object)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	uint32_t num_params = 1;
	QCOMTEE_Param params[num_params];

	memset(params, 0, sizeof(params));
	params[0].attr = QCOMTEE_OBJREF_OUTPUT;

	res = QCOMTEE_InvokeObject(app_controller_object,
				   2 /* IAppController_OP_getAppObject */,
				   num_params, params);
	if (res != QCOMTEE_MSG_OK) {
		MSGE("QCOMTEE_InvokeObject failed: 0x%x\n", res);
		return -1;
	}

	*app_object = params[0].object;

	MSGD("Got app_object %ld\n", (*app_object)->object_id);

	return 0;
}

static int get_app_controller_object(QCOMTEE_Object *app_loader_object,
				     uint8_t *buffer, size_t size,
				     QCOMTEE_Object **app_controller_object)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	uint32_t num_params = 2;
	QCOMTEE_Param params[num_params];

	memset(params, 0, sizeof(params));
	params[0].attr = QCOMTEE_UBUF_INPUT;
	params[0].ubuf.buffer = (void *)buffer;
	params[0].ubuf.size = size;

	params[1].attr = QCOMTEE_OBJREF_OUTPUT;

	res = QCOMTEE_InvokeObject(app_loader_object,
				   0 /* IAppLoader_OP_loadFromBuffer */,
				   num_params, params);
	if (res != QCOMTEE_MSG_OK) {
		MSGE("QCOMTEE_InvokeObject failed: 0x%x\n", res);
		return -1;
	}

	*app_controller_object = params[1].object;

	MSGD("Got app_controller_object %ld\n",
	     (*app_controller_object)->object_id);

	return 0;
}

static int __readFile(const char *filename, size_t size, uint8_t *buffer)
{
	FILE *file = NULL;
	size_t readBytes = 0;
	int ret = 0;

	do {
		file = fopen(filename, "r");
		if (file == NULL) {
			MSGE("fopen(%s) failed: errno %d\n", filename, errno);
			ret = -1;
			break;
		}

		readBytes = fread(buffer, 1, size, file);
		if (readBytes != size) {
			MSGE("fread failed: %zu vs %zu bytes: %d\n", readBytes,
			     size, errno);

			ret = -1;
			break;
		}

		ret = size;
	} while (0);

	if (file) {
		fclose(file);
	}
	return ret;
}

static int __getFileSize(const char *filename)
{
	FILE *file = NULL;
	int size = 0;
	int ret = 0;

	do {
		file = fopen(filename, "r");
		if (file == NULL) {
			MSGE("fopen(%s) failed: errno %d\n", filename, errno);
			size = -1;
			break;
		}

		ret = fseek(file, 0L, SEEK_END);
		if (ret) {
			MSGE("fseek(%s) failed: errno %d\n", filename, errno);
			size = -1;
			break;
		}

		size = ftell(file);
		if (size == -1) {
			MSGE("ftell(%s) failed: %d\n", filename, errno);
			size = -1;
			break;
		}
	} while (0);

	if (file) {
		fclose(file);
	}

	return size;
}

int load_app(QCOMTEE_Object *app_loader_object, const char *path,
	     QCOMTEE_Object **app_controller_object,
	     QCOMTEE_Object **app_object)
{
	size_t size = 0;
	uint8_t *buffer = NULL;
	int ret = 0;

	ret = __getFileSize(path);
	if (ret <= 0) {
		MSGE("__getFileSize failed: %d\n", ret);
		goto exit;
	}

	size = (size_t)ret;
	buffer = (uint8_t *)malloc(size);

	ret = __readFile(path, size, buffer);
	if (ret < 0) {
		MSGE("__readFile failed: %d\n", ret);
		goto exit;
	}

	ret = get_app_controller_object(app_loader_object, buffer, size,
					app_controller_object);
	if (ret) {
		MSGE("Loading %s TA failed: %d\n", path, ret);
		goto exit;
	}

	ret = get_app_object(*app_controller_object, app_object);
	if (ret) {
		MSGE("get_app_object failed: %d\n", ret);
		QCOMTEE_OBJECT_RELEASE(*app_controller_object);
	}

exit:
	if (buffer)
		free(buffer);

	return ret;
}

static int get_app_loader_object(QCOMTEE_Object *client_env_object,
				 QCOMTEE_Object **app_loader_object)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	uint32_t num_params = 2;
	QCOMTEE_Param params[num_params];

	uint32_t app_loader_uid = 3;

	memset(params, 0, sizeof(params));
	params[0].attr = QCOMTEE_UBUF_INPUT;
	params[0].ubuf.buffer = (void *)&app_loader_uid;
	params[0].ubuf.size = sizeof(uint32_t);

	params[1].attr = QCOMTEE_OBJREF_OUTPUT;

	res = QCOMTEE_InvokeObject(client_env_object,
				   0 /* IClientEnv_OP_open */, num_params,
				   params);
	if (res != QCOMTEE_MSG_OK) {
		MSGE("QCOMTEE_InvokeObject failed: 0x%x\n", res);
		return -1;
	}

	*app_loader_object = params[1].object;

	MSGD("Got app_loader_object %ld\n", (*app_loader_object)->object_id);

	return 0;
}

static int query_diagnostics_info(QCOMTEE_Object *service_object,
				  IDiagnostics_HeapInfo *heapInfo)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	uint32_t num_params = 1;
	QCOMTEE_Param params[num_params];

	memset(params, 0, sizeof(params));
	params[0].attr = QCOMTEE_UBUF_OUTPUT;
	params[0].ubuf.buffer = (void *)heapInfo;
	params[0].ubuf.size = sizeof(IDiagnostics_HeapInfo);

	res = QCOMTEE_InvokeObject(service_object,
				   0 /* IDiagnostics_OP_queryHeapInfo */,
				   num_params, params);
	if (res != QCOMTEE_MSG_OK) {
		MSGE("QCOMTEE_InvokeObject failed: 0x%x\n", res);
		return -1;
	}

	return 0;
}

static int get_diagnostics_service_object(QCOMTEE_Object *client_env_object,
					  QCOMTEE_Object **service_object)
{
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	uint32_t num_params = 2;
	QCOMTEE_Param params[num_params];

	uint32_t diagnostics_uid = 143;

	memset(params, 0, sizeof(params));
	params[0].attr = QCOMTEE_UBUF_INPUT;
	params[0].ubuf.buffer = (void *)&diagnostics_uid;
	params[0].ubuf.size = sizeof(uint32_t);

	params[1].attr = QCOMTEE_OBJREF_OUTPUT;

	res = QCOMTEE_InvokeObject(client_env_object,
				   0 /* IClientEnv_OP_open */, num_params,
				   params);
	if (res != QCOMTEE_MSG_OK) {
		MSGE("QCOMTEE_InvokeObject failed: 0x%x\n", res);
		return -1;
	}

	*service_object = params[1].object;

	MSGD("Got service_object %ld\n", (*service_object)->object_id);

	return 0;
}

static int get_client_env_object(QCOMTEE_Object **client_env_object)
{
	int ret = 0;
	QCOMTEE_Result res = QCOMTEE_MSG_ERROR;
	QCOMTEE_Object *root_object = QCOMTEE_Object_NULL;
	uint32_t num_params = 2;
	QCOMTEE_Param params[num_params];

	size_t len = 0;
	void *creds = NULL;

	res = QCOMTEE_GetRootObject(&root_object);
	if (res != QCOMTEE_MSG_OK) {
		ret = -1;
		MSGE("QCOMTEE_GetRootObject failed: 0x%x\n", res);
		goto err_root_object;
	}

	/* The client needs to send it's credentials to QTEE in order to obtain
	 * a client env object.
	 */
	creds = get_credentials(&len);

	memset(params, 0, sizeof(params));
	params[0].attr = QCOMTEE_UBUF_INPUT;
	params[0].ubuf.buffer = (void *)creds;
	params[0].ubuf.size = len;

	params[1].attr = QCOMTEE_OBJREF_OUTPUT;

	res = QCOMTEE_InvokeObject(root_object,
				   1 /* IClientEnv_OP_registerLegacy */,
				   num_params, params);
	if (res != QCOMTEE_MSG_OK) {
		ret = -1;
		MSGE("QCOMTEE_InvokeObject failed: 0x%x\n", res);
		goto err_invoke_object;
	}

	*client_env_object = params[1].object;

	MSGD("Got client_env_object %ld\n", (*client_env_object)->object_id);

err_invoke_object:
	if (creds)
		free(creds);

	QCOMTEE_OBJECT_RELEASE(root_object);

err_root_object:
	return ret;
}

static int run_skeleton_ta_test(int argc, char *argv[])
{
	int ret = 0;
	char *appFullPath = NULL;
	int num1 = 0;
	int num2 = 0;
	int iterations = 1;
	if (argc >= 4) {
		appFullPath = argv[2];
		num1 = atoi(argv[3]);
		num2 = atoi(argv[4]);
		iterations = atoi(argv[5]);
	}

	if (appFullPath == NULL) {
		usage();
		return -1;
	}

	QCOMTEE_Object *client_env_object = QCOMTEE_Object_NULL;
	QCOMTEE_Object *app_loader_object = QCOMTEE_Object_NULL;
	QCOMTEE_Object *app_controller_object = QCOMTEE_Object_NULL;
	QCOMTEE_Object *app_object = QCOMTEE_Object_NULL;

	ret = get_client_env_object(&client_env_object);
	if (ret) {
		MSGE("get_client_env_object failed: %d\n", ret);
		goto err_client_env;
	}

	ret = get_app_loader_object(client_env_object, &app_loader_object);
	if (ret) {
		MSGE("get_app_loader_object failed: %d\n", ret);
		goto err_app_loader;
	}

	ret = load_app(app_loader_object, appFullPath, &app_controller_object,
		       &app_object);
	if (ret) {
		printf("load_app failed. ret: %d\n", ret);
		goto err_load_app;
	}

	for (int i = 0; i < iterations; i++) {
		ret = send_add_cmd(app_object, num1, num2);

		if (ret) {
			printf("send_add_cmd failed. ret: %d\n", ret);
			break;
		} else {
			printf("send_add_cmd succeeded! ret: %d\n", ret);
		}
	}

	QCOMTEE_OBJECT_RELEASE(app_object);
	QCOMTEE_OBJECT_RELEASE(app_controller_object);

err_load_app:
	QCOMTEE_OBJECT_RELEASE(app_loader_object);

err_app_loader:
	QCOMTEE_OBJECT_RELEASE(client_env_object);

err_client_env:
	return 0;
}

static int run_tz_diagnostics_test(int argc, char *argv[])
{
	int ret = 0;
	int iterations = 1;
	if (argc >= 3)
		iterations = atoi(argv[2]);

	QCOMTEE_Object *client_env_object = QCOMTEE_Object_NULL;
	QCOMTEE_Object *service_object = QCOMTEE_Object_NULL;

	IDiagnostics_HeapInfo heapInfo;
	memset((void *)&heapInfo, 0, sizeof(IDiagnostics_HeapInfo));

	ret = get_client_env_object(&client_env_object);
	if (ret) {
		MSGE("get_client_env_object failed: %d\n", ret);
		goto err_client_env_object;
	}

	ret = get_diagnostics_service_object(client_env_object,
					     &service_object);
	if (ret) {
		MSGE("get_diagnostics_service_object failed: %d\n", ret);
		goto err_service_object;
	}

	for (int i = 0; i < iterations; i++) {
		printf("Retrieve TZ heap info Iteration %d\n", i);

		ret = query_diagnostics_info(service_object, &heapInfo);
		if (ret)
			break;

		printf("%d = Total bytes as heap\n", heapInfo.totalSize);
		printf("%d = Total bytes allocated from heap\n",
		       heapInfo.usedSize);
		printf("%d = Total bytes free on heap\n", heapInfo.freeSize);
		printf("%d = Total bytes overhead\n", heapInfo.overheadSize);
		printf("%d = Total bytes wasted\n", heapInfo.wastedSize);
		printf("%d = Largest free block size\n\n",
		       heapInfo.largestFreeBlockSize);
	}

	QCOMTEE_OBJECT_RELEASE(service_object);

err_service_object:
	QCOMTEE_OBJECT_RELEASE(client_env_object);

err_client_env_object:
	return ret;
}

void usage(void)
{
	printf("\n\n---------------------------------------------------------\n"
	       "Usage: qcomtee_client -[OPTION] [ARGU_1] ...... [ARGU_n]\n\n"
	       "Runs the user space tests specified by option and arguments \n"
	       "parameter(s).\n"
	       "\n\n"
	       "OPTION can be:\n"
	       "\t-d - Run the TZ diagnostics test that prints basic info on"
	       " TZ heaps\n"
	       "\te.g. qcomtee_client -d <no_of_iterations>\n"
	       "\t-l - Load the skeleton test TA and send addition cmd 0.\n"
	       "\te.g. qcomtee_client -l <path to skeleton TA binary>"
	       " <num 1> <num 2> <no_of_iterations>\n"
	       "\t-h, --help - Print this help message and exit\n\n\n");
}

unsigned int parse_command(int argc, char *const argv[])
{
	unsigned int ret = 0;
	int command = getopt_long(argc, argv, "dlh", testopts, NULL);

	if (command == -1)
		usage();
	else {
		printf("command is: %d\n", command);
		switch (command) {
		case 'd':
			ret = 1 << PRINT_TZ_DIAGNOSTICS;
			break;
		case 'l':
			ret = 1 << SKELETON_TA_TEST;
			break;
		case 'h':
			usage();
			break;
		default:
			usage();
		}
	}

	optind = 1;
	return ret;
}

int main(int argc, char *argv[])
{
	unsigned int test_mask = parse_command(argc, argv);
	if ((test_mask & (1 << PRINT_TZ_DIAGNOSTICS)) ==
	    (1U << PRINT_TZ_DIAGNOSTICS)) {
		printf("Run TZ Diagnostics test...\n");
		return run_tz_diagnostics_test(argc, argv);
	} else if ((test_mask & (1 << SKELETON_TA_TEST)) ==
		   (1U << SKELETON_TA_TEST)) {
		printf("Run Skeleton TA adder test...\n");
		return run_skeleton_ta_test(argc, argv);
	} else {
		return -1;
	}
}
