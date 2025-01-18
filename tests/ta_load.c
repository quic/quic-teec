// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <time.h>
#include "tests_private.h"

static int test_ta_cmd_0(struct qcomtee_object *ta)
{
	struct qcomtee_param params[2];
	qcomtee_result_t result;

	struct {
		uint32_t num1;
		uint32_t num2;
	} num;
	uint32_t sum;

	/* Get two number smaller than 100. */
	srand(time(NULL));
	num.num1 = rand() % 100;
	num.num2 = rand() % 100;

	/* INIT parameters and invoke object: */
	params[0].attr = QCOMTEE_UBUF_INPUT;
	params[0].ubuf = UBUF_INIT(&num);
	params[1].attr = QCOMTEE_UBUF_OUTPUT;
	params[1].ubuf = UBUF_INIT(&sum);
	/* 0 is ISMCIExample_OP_add. */
	if (qcomtee_object_invoke(ta, 0, params, 2, &result) ||
	    (result != QCOMTEE_OK))
		return -1;

	if (num.num1 + num.num2 == sum)
		return 0;

	PRINT("%u + %u is %u but TA returned %u\n", num.num1, num.num2,
	      num.num1 + num.num2, sum);

	return -1;
}

struct ta {
	struct qcomtee_object *ta_controller;
	struct qcomtee_object *ta;
};

static struct ta test_load_ta(struct qcomtee_object *service_object,
			      const char *pathname)
{
	struct ta ta = { QCOMTEE_OBJECT_NULL, QCOMTEE_OBJECT_NULL };
	struct qcomtee_param params[2];
	qcomtee_result_t result;

	char *buffer;
	size_t size;
	/* Read the TA file. */
	if (test_read_file2(pathname, TEST_TA, &buffer, &size))
		return ta;

	/* INIT parameters and invoke object: */
	params[0].attr = QCOMTEE_UBUF_INPUT;
	params[0].ubuf.addr = buffer;
	params[0].ubuf.size = size;
	params[1].attr = QCOMTEE_OBJREF_OUTPUT;
	/* 0 is IAppLoader_OP_loadFromBuffer. */
	if (qcomtee_object_invoke(service_object, 0, params, 2, &result) ||
	    (result != QCOMTEE_OK)) {
		PRINT("qcomtee_object_invoke.\n");
		goto failed_out;
	}

	ta.ta_controller = params[1].object;

	/* INIT parameters and invoke object: */
	params[0].attr = QCOMTEE_OBJREF_OUTPUT;
	/* 2 is IAppController_OP_getAppObject . */
	if (qcomtee_object_invoke(ta.ta_controller, 2, params, 1, &result) ||
	    (result != QCOMTEE_OK)) {
		PRINT("qcomtee_object_invoke.\n");
		goto failed_out;
	}

	ta.ta = params[0].object;

failed_out:
	free(buffer);

	return ta;
}

void test_load_sample_ta(const char *pathname, int cmd)
{
	struct ta test_ta;
	struct qcomtee_object *client_env_object, *service_object;

	struct supplicant *sup;

	/* Start a fresh namespace with supplicant. */
	sup = test_supplicant_start(1);
	if (!sup) {
		PRINT("test_supplicant_start.\n");
		return;
	}

	client_env_object = test_get_client_env_object(sup->root);
	if (client_env_object == QCOMTEE_OBJECT_NULL) {
		PRINT("test_get_client_env_object.\n");
		goto dec_root_object;
	}

	/* 3 is UID of App. Loader service. */
	service_object = test_get_service_object(client_env_object, 3);
	if (service_object == QCOMTEE_OBJECT_NULL) {
		PRINT("test_get_service_object.\n");
		goto dec_client_env_object;
	}

	/* Load the TA. */
	test_ta = test_load_ta(service_object, pathname);
	if (test_ta.ta == QCOMTEE_OBJECT_NULL) {
		PRINT("test_load_ta.\n");
		goto dec_service_object;
	}

	/* TEST TA: */
	switch (cmd) {
	case 0:
		if (test_ta_cmd_0(test_ta.ta)) {
			PRINT("test_ta_cmd_0.\n");
			goto dec_unload_ta;
		}

		break;
	default:
		PRINT("Unknown command (%d).\n", cmd);
		goto dec_unload_ta;
	}

	PRINT("SUCCESS.\n");

dec_unload_ta:
	qcomtee_object_refs_dec(test_ta.ta);
dec_service_object:
	qcomtee_object_refs_dec(test_ta.ta_controller);
	qcomtee_object_refs_dec(service_object);
dec_client_env_object:
	qcomtee_object_refs_dec(client_env_object);
dec_root_object:
	qcomtee_object_refs_dec(sup->root);
}
