// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <unistd.h>
#include "tests_private.h"

static void usage(char *name)
{
	printf("Usage: %s [OPTION][ARGS]\n", name);
	printf("OPTION are:\n"
	       "\t-d - Run the TZ diagnostics test that prints basic info on TZ heaps.\n"
	       "\t-l - Load the test TA and send command.\n"
	       "\t\t%s -l <path to TA binary> <command>\n"
	       "\t-h - Print this help message and exit\n\n", name);
}

int main(int argc, char *argv[])
{
	switch (getopt(argc, argv, "dlh")) {
	case 'd':
		test_print_diagnostics_info();
		break;
	case 'l':
		if (argc != 4)
			goto help;

		test_load_sample_ta(argv[2], atoi(argv[3]));
		break;
help:
	case 'h':
	default:
		usage(argv[0]);

		return 1;
	}
}