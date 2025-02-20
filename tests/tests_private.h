// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _TESTS_PRIVATE_H
#define _TESTS_PRIVATE_H

#include <string.h>
#include <stdlib.h>

#include <qcomtee_object_types.h>

/* Driver's file.*/
#define DEV_TEE "/dev/tee0"

#ifdef __GLIBC__
int tee_call(int fd, unsigned long op, ...);
#else
int tee_call(int fd, int op, ...);
#endif

#define test_object_invoke(...) qcomtee_object_invoke(__VA_ARGS__, tee_call)

/**
 * @brief Get a root object.
 * @return On success, returns the object;
 *         Otherwise, returns @ref QCOMTEE_OBJECT_NULL.
 */
struct qcomtee_object *test_get_root(void);

/**
 * @brief Get a client environment object.
 * @param root_object The root object to get the environment object for.
 * @return On success, returns the object;
 *         Otherwise, returns @ref QCOMTEE_OBJECT_NULL.
 */
struct qcomtee_object *test_get_client_env_object(struct qcomtee_object *root);

/**
 * @brief Get a service object in an environment.
 * 
 * @param client_env_object The environment object to obtain the service in.
 * @param uid UID of the service.
 * @return On success, returns the object;
 *         Otherwise, returns @ref QCOMTEE_OBJECT_NULL. 
 */
struct qcomtee_object *
test_get_service_object(struct qcomtee_object *client_env_object, uint32_t uid);

/* File stuff. */
size_t test_get_file_size(FILE *file);
int test_read_file(const char *filename, char **buffer, size_t *size);
int test_read_file2(const char *pathname, const char *name, char **buffer,
		    size_t *size);

#define PRINT(fmt, ...) \
	printf("[%s][%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define UBUF_INIT(u) ((struct qcomtee_ubuf){ (u), sizeof(*(u)) })

/* ''TESTS:'' */

/* diagnostics.c. */
void test_print_diagnostics_info(void);

/* ta_load.c. */
#define TEST_TA "smcinvoke_skeleton_ta64.mbn"

void test_load_sample_ta(const char *pathname, int cmd);

#endif // _TESTS_PRIVATE_H
