// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <stdarg.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <qcomtee_object.h>

/**
 * @def TABLE_SIZE
 * @brief The number of objects that can be exported to QTEE in each root object.
 */
#define TABLE_SIZE 1024

/**
 * @def DISP_BUFFER
 * @brief The size of the buffer allocated for TEE_IOC_SUPPL_RECV
 *        and TEE_IOC_SUPPL_SEND IOCTLs, which require user buffers for
 *        @ref QCOMTEE_UBUF_INPUT and @ref QCOMTEE_UBUF_OUTPUT parameters.
 */
#define DISP_BUFFER 1024

/**
 * @brief Initialize an object.
 * @param object Object to initialize.
 * @param object_type Type of the object.
 */
static inline void QCOMTEE_OBJECT_INIT(struct qcomtee_object *object,
				       qcomtee_object_type_t object_type)
{
	memset(object, 0, sizeof(*object));

	atomic_init(&object->refs, 1);
	object->object_type = object_type;
	object->root = QCOMTEE_OBJECT_NULL;
}

/**
 * @defgroup ObjectNS Namespace
 * @brief Functions to manage @ref qcomtee_object_namespace "Namespace".
 * @{
 */

/**
 * @brief Object namespace.
 *
 * Every time a root object is created, a new namespace is initiated.
 * This guarantees that callback objects exported to QTEE using a root object
 * can only be referenced by QTEE in the namespace maintained by the root
 * object. For QTEE objects, the kernel driver guarantees that objects
 * received using a root object are not visible to others.
 */
struct qcomtee_object_namespace {
	int current_idx; /**< Index to start searching for free entry. */
	struct qcomtee_object *entries[TABLE_SIZE]; /**< Callback object table. */
	pthread_mutex_t lock; /**< lock to protect members of this struct. */
};

/**
 * @brief Initialize object ID.
 *
 * This allocates an ID for the object if and only if it does not
 * already have one. The caller should hold the lock.
 *
 * @param object Object to allocate an ID. 
 * @param ns The namespace to which this object belongs.
 * @return On success, returns the 0, or 1 if the object already has an ID;
 *         Otherwise, returns -1. 
 */
static int qcomtee_object_id_init(struct qcomtee_object *object,
				  struct qcomtee_object_namespace *ns)
{
	int i, idx;

	/* Own an ID?! */
	if (object->queued)
		return 1;

	/* Simple id allocator. */
	for (i = 0; i < TABLE_SIZE; i++) {
		idx = (ns->current_idx + i) % TABLE_SIZE;
		if (ns->entries[idx] == QCOMTEE_OBJECT_NULL) {
			object->object_id = idx;
			ns->current_idx = (idx + 1) % TABLE_SIZE;

			return 0;
		}
	}

	return -1;
}

/**
 * @brief Insert a callback object into the namespace.
 *
 * This is called as part of marshaling in to the QTEE; or as part of
 * marshaling out when submitting the callback response.
 *
 * We can reuse the same ID for an object and only increase the reference
 * counter for that object. However, the kernel driver ensures that QTEE
 * identifies these objects as different instances, so if QTEE releases
 * one copy, it cannot continue using it with the same ID.
 *
 * @param object Object to insert.
 * @param ns Namespace where to insert the object.
 * @return On success, returns the 0; Otherwise, returns -1. 
 */
static int qcomtee_object_ns_insert(struct qcomtee_object *object,
				    struct qcomtee_object_namespace *ns)
{
	int ret;

	pthread_mutex_lock(&ns->lock);
	ret = qcomtee_object_id_init(object, ns);
	if (ret == 0) { /* Enqueue object. */
		ns->entries[object->object_id] = object;
		object->queued = 1;
	}
	pthread_mutex_unlock(&ns->lock);

	return ret;
}

/**
 * @brief Search for an object in the namespace based on its ID.
 *
 * This calls @ref qcomtee_object_refs_inc, so the caller should ensure they
 * issue @ref qcomtee_object_refs_dec at the appropriate time.
 *
 * This is called as part of marshaling out when returning from QTEE; or
 * as part of marshaling in when QTEE is making a callback request.
 * So, it is always called on behalf of QTEE.
 *
 * This is not serialized with @ref qcomtee_object_ns_insert or
 * @ref qcomtee_object_ns_del, as QTEE only uses IDs that it assumes it owns
 * and would not release it while using it.
 * 
 * @param id Object ID to search for in the namespace.
 * @param ns Namespace to search for the object ID.
 * @return On success, returns the object;
 *         Otherwise, returns @ref QCOMTEE_OBJECT_NULL.
 */
static struct qcomtee_object *
qcomtee_object_ns_find(uint64_t id, struct qcomtee_object_namespace *ns)
{
	struct qcomtee_object *object = QCOMTEE_OBJECT_NULL;

	if (id < TABLE_SIZE) {
		object = ns->entries[id];
		if (object != QCOMTEE_OBJECT_NULL)
			qcomtee_object_refs_inc(object);
	}

	return object;
}

/**
 * @brief Delete an object from the namespace.
 *
 * This is only called when the last reference to the object is dropped. 
 *
 * @param object Object to delete.
 * @param ns Namespace to delete the object from.
 */
static void qcomtee_object_ns_del(struct qcomtee_object *object,
				  struct qcomtee_object_namespace *ns)
{
	pthread_mutex_lock(&ns->lock);
	if (object->queued) {
		ns->entries[object->object_id] = QCOMTEE_OBJECT_NULL;
		object->queued = 0;
	}
	pthread_mutex_unlock(&ns->lock);
}

/** @} */ // end of ObjectNS

/* OBJECT CLASSES: */
/*   - Root Object
 *   - QTEE Object
 *   - Callback Object
 */

/* ''ROOT OBJECT''. */

/**
 * @brief Root object.
 * 
 * Use @ref qcomtee_object_root_init to create a root object and a namespace.
 */
struct root_object {
	struct qcomtee_object object;
	struct qcomtee_object_namespace ns;
	int fd; /**< Driver's fd. */

	/**
	 * @brief Callback on root object destruction.
	 * @param Argument is @ref root_object::arg set by user.
	 *
	 * This is called when the root object reference count reaches zero,
	 * which guarantees that no QTEE or callback object exists in
	 * this namespace.
	 */
	void (*close_call)(void *);
	void *arg; /**< Argument passed to close_call. */
};

#define ROOT_OBJECT(ro) container_of((ro), struct root_object, object)
#define ROOT_OBJECT_NS(ro) (&ROOT_OBJECT(ro)->ns)

/**
 * @def OBJECT_NS
 * @brief Returns the namespace the object belongs to.
 */
#define OBJECT_NS(o) ROOT_OBJECT_NS((o)->root)

static void qcomtee_object_root_release(struct qcomtee_object *object)
{
	struct root_object *root_object = ROOT_OBJECT(object);

	if (root_object->close_call)
		root_object->close_call(root_object->arg);
	close(root_object->fd);
	pthread_mutex_destroy(&root_object->ns.lock);
	free(root_object);
}

struct qcomtee_object *qcomtee_object_root_init(const char *devname,
						void (*close_call)(void *),
						void *close_arg)
{
	struct root_object *root_object;
	static struct qcomtee_object_ops qcomtee_object_root_ops = {
		.release = qcomtee_object_root_release,
	};

	root_object = malloc(sizeof(*root_object));
	if (!root_object)
		return QCOMTEE_OBJECT_NULL;

	/* INIT the root object. */
	QCOMTEE_OBJECT_INIT(&root_object->object, QCOMTEE_OBJECT_TYPE_ROOT);
	root_object->object.object_id = TEE_OBJREF_NULL;
	root_object->object.ops = &qcomtee_object_root_ops;
	root_object->object.root = &root_object->object;
	/* Open the driver. */
	root_object->fd = open(devname, O_RDWR);
	if (root_object->fd < 0)
		goto failed_out;

	/* INIT the namespace. */
	root_object->ns.current_idx = 0;
	memset(root_object->ns.entries, 0, sizeof(root_object->ns.entries));
	pthread_mutex_init(&root_object->ns.lock, NULL);
	/* Cleanup. */
	root_object->close_call = close_call;
	root_object->arg = close_arg;

	return root_object->object.root;

failed_out:
	return QCOMTEE_OBJECT_NULL;
}

/* ''QTEE OBJECT''. */

static void qcomtee_object_tee_release(struct qcomtee_object *object)
{
	struct qcomtee_object *root = object->root;
	qcomtee_result_t result;

	if (qcomtee_object_invoke(object, QCOMTEE_OBJREF_OP_RELEASE, NULL, 0,
				  &result) ||
	    (result != QCOMTEE_OK))
		MSGE("QTEE object release failed!\n");

	free(object);
	/* qcomtee_object_refs_inc has been called in qcomtee_object_tee_init. */
	qcomtee_object_refs_dec(root);
}

/**
 * @brief Initialize a QTEE object.
 *
 * This is called as part of marshaling out when returning from QTEE; or
 * as part of marshaling in when QTEE is making a callback request.
 * So, it is always called on behalf of QTEE.
 *
 * @param root The root object this object belongs to.
 * @param id QTEE object ID.
 * @return On success, returns the object;
 *         Otherwise, returns @ref QCOMTEE_OBJECT_NULL.
 */
static struct qcomtee_object *
qcomtee_object_tee_init(struct qcomtee_object *root, uint64_t id)
{
	struct qcomtee_object *object;

	object = malloc(sizeof(*object));
	if (object == QCOMTEE_OBJECT_NULL)
		return QCOMTEE_OBJECT_NULL;

	QCOMTEE_OBJECT_INIT(object, QCOMTEE_OBJECT_TYPE_TEE);
	object->object_id = id;
	/* Keep a copy of root object; released in qcomtee_object_tee_release. */
	qcomtee_object_refs_inc(root);
	object->root = root;

	return object;
}

/* ''CALLBACK OBJECT''. */

int qcomtee_object_cb_init(struct qcomtee_object *object,
			   struct qcomtee_object_ops *ops,
			   struct qcomtee_object *root)
{
	if (ops->dispatch == NULL)
		return -1;

	QCOMTEE_OBJECT_INIT(object, QCOMTEE_OBJECT_TYPE_CB);
	object->ops = ops;
	/* Keep a copy of root object; released in qcomtee_object_refs_dec. */
	qcomtee_object_refs_inc(root);
	object->root = root;

	return 0;
}

void qcomtee_object_refs_dec(struct qcomtee_object *object)
{
	/* QCOMTEE_OBJECT_NULL is special and cannot be released. */
	if (object == QCOMTEE_OBJECT_NULL)
		return;

	if (atomic_fetch_sub(&object->refs, 1) == 1) {
		switch (qcomtee_object_typeof(object)) {
		case QCOMTEE_OBJECT_TYPE_ROOT:
			qcomtee_object_root_release(object);

			break;
		case QCOMTEE_OBJECT_TYPE_TEE:
			qcomtee_object_tee_release(object);

			break;
		case QCOMTEE_OBJECT_TYPE_CB: {
			struct qcomtee_object *root = object->root;

			/* It dequeues the object if it is already queued. */
			qcomtee_object_ns_del(object, OBJECT_NS(object));
			if (object->ops->release)
				object->ops->release(object);
			qcomtee_object_refs_dec(root);

			break;
		}
		default:
			/* NOTHING TO DO HERE. */
		}
	}
}

/* ''Object Marshaling''. */

/**
 * @brief Convert an object from @ref qcomtee_param to tee_ioctl_param.
 * @param tee_param Output parameter.
 * @param param Input parameter.
 * @param root The root object for which the conversion should happen.
 * @return On success, 0; Otherwise, returns -1.
 */
static int qcomtee_object_param_to_tee_param(struct tee_ioctl_param *tee_param,
					     struct qcomtee_param *param,
					     struct qcomtee_object *root)
{
	/* It assumes param is object. */
	struct qcomtee_object *object = param->object;

	/* It expects caller to set tee_param attribute. */
	if (tee_param->attr != TEE_IOCTL_PARAM_ATTR_TYPE_OBJREF_INPUT &&
	    tee_param->attr != TEE_IOCTL_PARAM_ATTR_TYPE_OBJREF_OUTPUT)
		return -1;

	switch (qcomtee_object_typeof(object)) {
	case QCOMTEE_OBJECT_TYPE_NULL:
		tee_param->a = TEE_OBJREF_NULL;
		tee_param->b = 0;

		break;
	case QCOMTEE_OBJECT_TYPE_TEE:
		tee_param->a = object->object_id;
		tee_param->b = 0;

		break;
	case QCOMTEE_OBJECT_TYPE_CB:
		/* Only accept object that belong to the same namespace. */
		if (object->root != root)
			return -1;

		/* Namespace is full?! */
		if (qcomtee_object_ns_insert(object, OBJECT_NS(object)))
			return -1;

		tee_param->a = object->object_id;
		tee_param->b = QCOMTEE_OBJREF_USER;

		break;
	default:
		return -1;
	}

	return 0;
}

/**
 * @brief Convert an object from tee_ioctl_param to @ref qcomtee_param.
 * @param param Output parameter.
 * @param tee_param Input parameter.
 * @param root The root object for which the conversion should happen.
 * @return On success, 0; Otherwise, returns -1.
 */
static int
qcomtee_object_param_from_tee_param(struct qcomtee_param *param,
				    struct tee_ioctl_param *tee_param,
				    struct qcomtee_object *root)
{
	struct qcomtee_object *object;

	/* It expects caller to set param attribute. */
	if (param->attr != QCOMTEE_OBJREF_INPUT &&
	    param->attr != QCOMTEE_OBJREF_OUTPUT)
		return -1;

	param->object = QCOMTEE_OBJECT_NULL;
	/* Is it a NULL object!?*/
	if (tee_param->a == TEE_OBJREF_NULL)
		return 0;

	/* Is it a callback object!? */
	if (tee_param->b == QCOMTEE_OBJREF_USER) {
		object = qcomtee_object_ns_find(
			tee_param->a,
			/* Search in the requested namespace. */
			ROOT_OBJECT_NS(root));
	} else {
		object = qcomtee_object_tee_init(root, tee_param->a);
	}

	/* On failure, returns QCOMTEE_OBJECT_NULL. */
	if (object == QCOMTEE_OBJECT_NULL)
		return -1;

	param->object = object;

	return 0;
}

/* ''Marshaling operations:'' */
/*   - qcomtee_object_marshal_in on direct path to QTEE
 *   - qcomtee_object_marshal_out on direct path from QTEE
 *   - qcomtee_object_cb_marshal_in on callback path from QTEE
 *   - qcomtee_object_cb_marshal_out on callback path to QTEE
 */

/**
 * @brief Convert array of @ref qcomtee_param to tee_ioctl_param.
 * @param tee_params Output parameter array.
 * @param params Input parameter array.
 * @param num_params Number of parameter in the input array.
 * @param root The root object for which the conversion should happen.
 * @return On success, 0; Otherwise, returns -1.
 */
static int qcomtee_object_marshal_in(struct tee_ioctl_param *tee_params,
				     struct qcomtee_param *params,
				     int num_params,
				     struct qcomtee_object *root)
{
	int i;

	for (i = 0; i < num_params; i++) {
		switch (params[i].attr) {
		case QCOMTEE_UBUF_INPUT:
		case QCOMTEE_UBUF_OUTPUT:
			tee_params[i].a = (uintptr_t)params[i].ubuf.addr;
			tee_params[i].b = params[i].ubuf.size;
			tee_params[i].attr =
				(params[i].attr == QCOMTEE_UBUF_INPUT) ?
					TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_INPUT :
					TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_OUTPUT;

			break;
		case QCOMTEE_OBJREF_INPUT:
			tee_params[i].attr =
				TEE_IOCTL_PARAM_ATTR_TYPE_OBJREF_INPUT;
			if (qcomtee_object_param_to_tee_param(&tee_params[i],
							      &params[i], root))
				return -1;

			break;
		case QCOMTEE_OBJREF_OUTPUT:
			tee_params[i].attr =
				TEE_IOCTL_PARAM_ATTR_TYPE_OBJREF_OUTPUT;

			break;
		default:
			return -1;
		}
	}

	return 0;
}

/**
 * @brief Convert array of tee_ioctl_param to @ref qcomtee_param.
 *
 * This consumes the input parameter array as it is initialized
 * by @ref qcomtee_object_marshal_in. On failure, it releases all QTEE
 * objects returned in input parameter array.
 * 
 * @param params Output parameter array.
 * @param tee_params Input parameter array.
 * @param num_params Number of parameter in the input array.
 * @param root The root object for which the conversion should happen.
 * @return On success, 0; Otherwise, returns -1.
 */
static int qcomtee_object_marshal_out(struct qcomtee_param *params,
				      struct tee_ioctl_param *tee_params,
				      int num_params,
				      struct qcomtee_object *root)
{
	int i, failed = 0;

	for (i = 0; i < num_params; i++) {
		switch (params[i].attr) {
		case QCOMTEE_UBUF_OUTPUT:
			params[i].ubuf.size = (size_t)tee_params[i].b;

			break;
		case QCOMTEE_OBJREF_OUTPUT:

			/* ''QTEE object Cleanup'' */
			/* On failure, continue processing tee_params so that
			 * we can release QTEE objects. If the failure is due
			 * to qcomtee_object_param_from_tee_param being unable
			 * to process QTEE objects (e.g. malloc fails), then
			 * they cannot be released.
			 */

			if (qcomtee_object_param_from_tee_param(
				    &params[i], &tee_params[i], root))
				failed = 1;

			break;
		case QCOMTEE_UBUF_INPUT:
		case QCOMTEE_OBJREF_INPUT:
			break;
		default:
			failed = 1;
		}
	}

	if (!failed)
		return 0;

	/* On failure, drop QTEE objects. */
	for (i = 0; i < num_params; i++) {
		if (params[i].attr == QCOMTEE_OBJREF_OUTPUT)
			qcomtee_object_refs_dec(params[i].object);
	}

	return -1;
}

/**
 * @brief Convert array of tee_ioctl_param to @ref qcomtee_param on callback path.
 *
 * On failure, it releases all QTEE objects in input parameter array.
 *
 * @param params Output parameter array.
 * @param tee_params Input parameter array.
 * @param num_params Number of parameter in the input array.
 * @param root The root object for which the conversion should happen.
 * @return On success, 0; Otherwise, returns -1.
 */
static int qcomtee_object_cb_marshal_in(struct qcomtee_param *params,
					struct tee_ioctl_param *tee_params,
					int num_params,
					struct qcomtee_object *root)
{
	int i, failed = 0;

	for (i = 0; i < num_params; i++) {
		switch (tee_params[i].attr) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_INPUT:
			params[i].attr = QCOMTEE_UBUF_INPUT;
			params[i].ubuf.addr = (void *)tee_params[i].a;
			params[i].ubuf.size = (size_t)tee_params[i].b;

			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_OBJREF_INPUT:
			params[i].attr = QCOMTEE_OBJREF_INPUT;
			/* See qcomtee_object_marshal_out comments. */
			if (qcomtee_object_param_from_tee_param(
				    &params[i], &tee_params[i], root))
				failed = 1;

			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_OBJREF_OUTPUT:
		default:
			failed = 1;
		}
	}

	if (!failed)
		return 0;

	/* On failure, drop QTEE objects. */
	for (i = 0; i < num_params; i++) {
		if (params[i].attr == QCOMTEE_OBJREF_INPUT)
			qcomtee_object_refs_dec(params[i].object);
	}

	return -1;
}

/**
 * @brief Convert array of @ref qcomtee_param to tee_ioctl_param on callback path.
 * @param tee_params Output parameter array.
 * @param params Input parameter array.
 * @param num_params Number of parameter in the input array.
 * @param root The root object for which the conversion should happen.
 * @return On success, 0; Otherwise, returns -1.
 */
static int qcomtee_object_cb_marshal_out(struct tee_ioctl_param *tee_params,
					 struct qcomtee_param *params,
					 int num_params,
					 struct qcomtee_object *root)
{
	int i;

	for (i = 0; i < num_params; i++) {
		switch (params[i].attr) {
		case QCOMTEE_UBUF_OUTPUT:
			tee_params[i].attr =
				TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_OUTPUT;
			tee_params[i].a = (uintptr_t)params[i].ubuf.addr;
			tee_params[i].b = params[i].ubuf.size;
			break;
		case QCOMTEE_OBJREF_OUTPUT:
			tee_params[i].attr =
				TEE_IOCTL_PARAM_ATTR_TYPE_OBJREF_OUTPUT;
			if (qcomtee_object_param_to_tee_param(&tee_params[i],
							      &params[i], root))
				return -1;

			break;
		case QCOMTEE_UBUF_INPUT:
		case QCOMTEE_OBJREF_INPUT:
		default:
			return -1;
		}
	}

	return 0;
}

/* ''OBJECT INVOKE APIs''. */

/**
 * @brief Driver arguments. 
 */
union tee_ioctl_arg {
	struct tee_ioctl_object_invoke_arg invoke; /**< TEE_IOC_OBJECT_INVOKE. */
	struct tee_iocl_supp_send_arg send; /**< TEE_IOC_SUPPL_SEND. */
	struct tee_iocl_supp_recv_arg recv; /**< TEE_IOC_SUPPL_RECV. */
};

#define TEE_IOCTL_ARG_SEND_INIT(arg, r, n)        \
	do {                                      \
		/* +1 for the output meta. */     \
		(arg)->send.num_params = (n) + 1; \
		(arg)->send.ret = (r);            \
	} while (0)

#define qcomtee_arg_alloca(np)                                     \
	({                                                         \
		size_t sz = sizeof(union tee_ioctl_arg) +          \
			    (np) * sizeof(struct tee_ioctl_param); \
		memset(alloca(sz), 0, sz);                         \
	})

/* Plus one for the meta parameters. */
#define DISP_PARAMS_MAX (QCOMTEE_OBJECT_PARAMS_MAX + 1)

/* Direct object invocation. */
int qcomtee_object_invoke(struct qcomtee_object *object, qcomtee_op_t op,
			  struct qcomtee_param *params, int num_params,
			  qcomtee_result_t *result)
{
	struct qcomtee_object *root = object->root;
	struct tee_ioctl_buf_data buf_data;
	struct tee_ioctl_param *tee_params;
	union tee_ioctl_arg *arg;

	/* Use can only invoke QTEE object ot root object. */
	if (object->object_type != QCOMTEE_OBJECT_TYPE_ROOT &&
	    object->object_type != QCOMTEE_OBJECT_TYPE_TEE)
		return -1;
	/* QTEE does not support more than 64 parameter. */
	if (num_params > 64)
		return -1;

	arg = qcomtee_arg_alloca(num_params);
	if (!arg)
		return -1;

	/* INIT IOCTL argument. */
	buf_data.buf_ptr = (uintptr_t)arg;
	buf_data.buf_len = sizeof(arg->invoke) +
			   sizeof(struct tee_ioctl_param) * num_params;

	/* INVOKE object: */
	arg->invoke.op = op;
	arg->invoke.object = object->object_id;
	arg->invoke.num_params = num_params;
	tee_params = (struct tee_ioctl_param *)(&arg->invoke + 1);

	if (qcomtee_object_marshal_in(tee_params, params, num_params, root))
		return -1;

	if (ioctl(ROOT_OBJECT(root)->fd, TEE_IOC_OBJECT_INVOKE, &buf_data))
		return -1;

	*result = arg->invoke.ret;
	/* Only marshal out on SUCCESS. */
	if (arg->invoke.ret)
		return 0;

	/* On failure, qcomtee_object_marshal_out does the cleanup; Override result. */
	if (qcomtee_object_marshal_out(params, tee_params, num_params, root))
		*result = QCOMTEE_ERROR_UNAVAIL;

	/* DONE!*/

	return 0;
}

/* See qcomtee_object_dispatch_request docs for return value. */
#define WITH_RESPONSE 0
#define WITH_RESPONSE_ERR 1
#define WITH_RESPONSE_NO_NOTIFY 2
#define WITHOUT_RESPONSE 3

/**
 * @brief Dispatch a request.
 *
 * This process a request and update the argument buffer if needed.
 * This also handles the reserved operations:
 *    - %ref QCOMTEE_OBJREF_OP_RELEASE
 *
 * @param object Object to dispatch the request for.
 * @param arg The argument buffer for the request.
 * @param root The root object that the request belongs.
 * @return Returns WITHOUT_RESPONSE if the argument buffer has not been updated;
 *         Returns WITH_RESPONSE, WITH_RESPONSE_ERR, or WITH_RESPONSE_NO_NOTIFY
 *         if the argument buffer has been updated, indicating whether there
 *         was a transport error and whether a notification should be sent
 *         to the object.
 */
static int qcomtee_object_dispatch_request(struct qcomtee_object *object,
					   union tee_ioctl_arg *arg,
					   struct qcomtee_object *root)
{
	struct qcomtee_param params[DISP_PARAMS_MAX];
	struct tee_ioctl_param *tee_params;
	qcomtee_result_t res;
	qcomtee_op_t op;
	int np;

	/* get request information. */
	op = arg->recv.func;
	np = arg->recv.num_params - 1;

	/* Before doing a heavy work, make sure it worth it. */
	if (object->ops->supported) {
		if (!object->ops->supported(op)) {
			TEE_IOCTL_ARG_SEND_INIT(arg, QCOMTEE_ERROR_BADOBJ, 0);
			return WITH_RESPONSE_NO_NOTIFY;
		}
	}

	/* Process request parameters. */
	tee_params = (struct tee_ioctl_param *)(&arg->recv + 1);
	if (qcomtee_object_cb_marshal_in(params, tee_params + 1, np, root)) {
		TEE_IOCTL_ARG_SEND_INIT(arg, QCOMTEE_ERROR_UNAVAIL, 0);
		return WITH_RESPONSE_NO_NOTIFY;
	}

	/* INVOKE the object: */
	switch (op) {
	case QCOMTEE_OBJREF_OP_RELEASE:
		qcomtee_object_refs_dec(object);
		/* No need to provide response. */
		return WITHOUT_RESPONSE;

	default:
		res = object->ops->dispatch(object, op, params, &np);
		if (res != QCOMTEE_OK) {
			TEE_IOCTL_ARG_SEND_INIT(arg, res, 0);
			return WITH_RESPONSE_NO_NOTIFY;
		}
	}

	/* Update response parameters. */
	tee_params = (struct tee_ioctl_param *)(&arg->send + 1);
	if (qcomtee_object_cb_marshal_out(tee_params + 1, params, np, root)) {
		TEE_IOCTL_ARG_SEND_INIT(arg, QCOMTEE_ERROR_UNAVAIL, 0);
		/* Object requires some form of cleanup. */
		return WITH_RESPONSE_ERR;
	}

	/* SUCCESS. */
	TEE_IOCTL_ARG_SEND_INIT(arg, QCOMTEE_OK, np);
	return WITH_RESPONSE;
}

int qcomtee_object_process_one(struct qcomtee_object *root,
			       int (*tee_call)(int, int,
					       struct tee_ioctl_buf_data *))
{
	struct tee_ioctl_buf_data buf_data;
	struct tee_ioctl_param *tee_params;
	struct qcomtee_object *object;
	union tee_ioctl_arg *arg;
	uint64_t request_id;
	int err;

	/* Buffer used for input parameter for dispatcher. */
	uint64_t buffer[DISP_BUFFER / sizeof(uint64_t)];

	arg = qcomtee_arg_alloca(DISP_PARAMS_MAX);
	if (!arg)
		return -1;

	buf_data.buf_ptr = (uintptr_t)arg;

	/* INIT IOCTL arguments for recv. */
	buf_data.buf_len = sizeof(arg->recv) +
			   sizeof(struct tee_ioctl_param) * DISP_PARAMS_MAX;

	/* RECV: */
	arg->recv.num_params = DISP_PARAMS_MAX;

	/* ''Prepare to receive request''.
	 * tee_params[0] is meta parameter to receive request:
	 *  - a is buffer for TEE_IOCTL_PARAM_ATTR_TYPE_UBUF_INPUT parameters,
	 *  - b is buffer size.
	 */
	tee_params = (struct tee_ioctl_param *)(&arg->recv + 1);
	tee_params[0].attr = (TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT |
			      TEE_IOCTL_PARAM_ATTR_META);
	tee_params[0].a = (uintptr_t)buffer;
	tee_params[0].b = DISP_BUFFER;
	tee_params[0].c = 0;

	/* Wait to receive a request ... */
	if (tee_call(ROOT_OBJECT(root)->fd, TEE_IOC_SUPPL_RECV, &buf_data))
		return -1;

	/* ''Process received request''.
	 * tee_params[0] is meta parameter for request information:
	 *  - a is object ID,
	 *  - b is request ID.
	 *  - c is reserved.
	 */
	request_id = tee_params[0].b;

	/* Find the requested object and call dispatcher: */

	object = qcomtee_object_ns_find(tee_params[0].a, ROOT_OBJECT_NS(root));
	if (object == QCOMTEE_OBJECT_NULL) {
		TEE_IOCTL_ARG_SEND_INIT(arg, QCOMTEE_ERROR_DEFUNCT, 0);
		err = WITH_RESPONSE_NO_NOTIFY;
	} else {
		/* Is there any response we should send!?*/
		err = qcomtee_object_dispatch_request(object, arg, root);
		if (err == WITHOUT_RESPONSE)
			return 0;
	}

	/* INIT IOCTL arguments for send. */
	buf_data.buf_len = sizeof(arg->send) + sizeof(struct tee_ioctl_param) *
						       arg->send.num_params;

	/* SEND: */
	/* arg->send.num_params and arg->send.ret has been initialized. */

	/* ''Process response''.
	 * tee_params[0] is meta parameter for response information:
	 *  - a is request ID.
	 *  - b is reserved.
	 *  - c is reserved.
	 */
	tee_params = (struct tee_ioctl_param *)(&arg->send + 1);
	tee_params[0].attr = (TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT |
			      TEE_IOCTL_PARAM_ATTR_META);
	tee_params[0].a = request_id;
	tee_params[0].b = 0;
	tee_params[0].c = 0;

	if (tee_call(ROOT_OBJECT(root)->fd, TEE_IOC_SUPPL_SEND, &buf_data))
		err = err == WITH_RESPONSE_NO_NOTIFY ? WITH_RESPONSE_NO_NOTIFY :
						       WITH_RESPONSE_ERR;

	/* DONE! */

	if (object != QCOMTEE_OBJECT_NULL && object->ops->error) {
		if (err == WITH_RESPONSE_ERR || err == WITH_RESPONSE)
			object->ops->error(object, err);
	}

	qcomtee_object_refs_dec(object);

	return 0;
}
