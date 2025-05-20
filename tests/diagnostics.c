// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include "tests_private.h"

struct diagnostics_heap_info {
	uint32_t total_size;
	uint32_t used_size;
	uint32_t free_size;
	uint32_t overhead_size;
	uint32_t wasted_size;
	uint32_t largest_free_block_size;
};

void test_print_diagnostics_info(void)
{
	struct diagnostics_heap_info heap_info;
	struct qcomtee_object *root, *client_env_object, *service_object;
	struct qcomtee_param params[1];
	qcomtee_result_t result;

	MSG("Starting test_print_diagnostics_info\n");

	/* Get root + supplicant. */
	root = test_get_root();
	if (root == QCOMTEE_OBJECT_NULL)
		return;

	client_env_object = test_get_client_env_object(root);
	if (client_env_object == QCOMTEE_OBJECT_NULL)
		goto dec_root_object;

	/* 143 is UID of Diagnostics service. */
	service_object = test_get_service_object(client_env_object, 143);
	if (service_object == QCOMTEE_OBJECT_NULL)
		goto dec_client_env_object;

	/* INIT parameters and invoke object: */
	params[0].attr = QCOMTEE_UBUF_OUTPUT;
	params[0].ubuf = UBUF_INIT(&heap_info);
	/* 0 is IDiagnostics_OP_queryHeapInfo. */
	if (qcomtee_object_invoke(service_object, 0, params, 1, &result) ||
	    (result != QCOMTEE_OK)) {
		MSG_ERROR("Unable to obtain diagnostics info, result %d\n",
			  result);
		goto dec_service_object;
	}

	MSG_INFO("%-15d = Total bytes as heap\n", heap_info.total_size);
	MSG_INFO("%-15d = Total bytes allocated from heap\n",
		 heap_info.used_size);
	MSG_INFO("%-15d = Total bytes free on heap\n", heap_info.free_size);
	MSG_INFO("%-15d = Total bytes overhead\n", heap_info.overhead_size);
	MSG_INFO("%-15d = Total bytes wasted\n", heap_info.wasted_size);
	MSG_INFO("%-15d = Largest free block size\n\n",
		 heap_info.largest_free_block_size);

	MSG_INFO("SUCCESS.\n");

dec_service_object:
	qcomtee_object_refs_dec(service_object);
dec_client_env_object:
	qcomtee_object_refs_dec(client_env_object);
dec_root_object:
	qcomtee_object_refs_dec(root);
}
