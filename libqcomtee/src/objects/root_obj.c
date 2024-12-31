// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <fcntl.h>

#include "qcom_tee_obj.h"

static void release_root_obj(QCOMTEE_Object *root);

/*
===========================================================================
                          GLOBAL DATA STRUCTURES
===========================================================================
*/

static pthread_mutex_t root_obj_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Each fd received upon opening a TEE device represents an object
 * namespace. Since all threads of the same process share objects
 * from the same namespace, they also share the same fd.
 * Hence, we define a static root object holding the fd to be
 * shared among all threads.
 */
static QCOMTEE_Object root_object = {
	.refs = 0,

	.object_id = QCOMTEE_OBJREF_NULL,
	.object_type = QCOM_TEE_OBJECT_TYPE_ROOT,

	.fd = -1,

	.release = release_root_obj,
};

/*
===========================================================================
                          PRIVATE HELPER/CALLBACK FUNCTIONS
===========================================================================
*/

static void release_root_obj(QCOMTEE_Object *root)
{
	close(root->fd);
	root->fd = -1;
}

/*
===========================================================================
                         PUBLIC FUNCTIONS
===========================================================================
*/

void acquire_root_obj_lock(void)
{
	pthread_mutex_lock(&root_obj_lock);
}

void release_root_obj_lock(void)
{
	pthread_mutex_unlock(&root_obj_lock);
}

QCOMTEE_Result init_qcom_tee_root_object(char *devname, QCOMTEE_Object **object)
{
	QCOMTEE_Result res = QCOMTEE_MSG_OK;
	int fd = -1;

	*object = QCOMTEE_OBJECT_RETAIN(&root_object);
	if (*object != QCOMTEE_Object_NULL)
		return res;

	acquire_root_obj_lock();

	if (devname == NULL) {
		res = QCOMTEE_MSG_ERROR_INVALID;
		MSGE("Invalid device name.\n");
		goto exit;
	}

	/* Every open() creates a new context; so if a process opens the
	 * device file two times, it gets two isolated namespaces, i.e.
	 * 1. TEE objects in one namespace are not available in another
	 *    namespace.
	 * 2. Callback object sent from a namespace is only received from
	 *    that namespace.
	 */
	fd = open(devname, O_RDWR);
	if (fd < 0) {
		res = QCOMTEE_MSG_ERROR;
		MSGE("Failed to open(%d) device. errno = %d\n", fd, errno);
		goto exit;
	}

	root_object.fd = fd;
	atomic_init(&root_object.refs, 1);

	*object = &root_object;

exit:
	release_root_obj_lock();

	return res;
}

QCOMTEE_Object *get_root_ptr(void)
{
	return QCOMTEE_OBJECT_RETAIN(&root_object);
}
