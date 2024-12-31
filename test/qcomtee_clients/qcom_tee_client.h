// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <stdint.h>

typedef struct {
	uint32_t totalSize;
	uint32_t usedSize;
	uint32_t freeSize;
	uint32_t overheadSize;
	uint32_t wastedSize;
	uint32_t largestFreeBlockSize;
} IDiagnostics_HeapInfo;

struct option testopts[] = {
	{ "diagnostics", no_argument, NULL, 'd' },
	{ "skeletonta", no_argument, NULL, 'l' },
	{ "lifecycle", no_argument, NULL, 'o' },
};

enum test_types {
	PRINT_TZ_DIAGNOSTICS,
	SKELETON_TA_TEST,
	LIFECYCLE_TEST,
};

void usage(void);
