// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include <stddef.h>
#include <stdint.h>

#include "CSMCIExample_open.h"
#include "ISMCIExample_invoke.h"
#include "object.h"
#include "qsee_heap.h"
#include "qsee_log.h"

/** @cond */
typedef struct CSMCIExample {
  int refs;
  int index;
/** @endcond */
} CSMCIExample;

/**
 * @brief Decrement the reference counter of this object.
 *
 * When the reference count reaches 0 (i.e. everything retaining a reference to it has
 * called release), the object is freed.
 */
static int32_t CSMCIExample_release(CSMCIExample *me)
{
  if (--me->refs == 0) {
    qsee_log(QSEE_LOG_MSG_DEBUG, "Freeing last instance of index: %d", me->index);
    QSEE_FREE_PTR(me);
  }
  return Object_OK;
}

/**
 * @brief Increment the reference counter of this object.
 *
 * This would be called when keeping a new reference to this object.
 */
static int32_t CSMCIExample_retain(CSMCIExample *me)
{
  me->refs++;
  return Object_OK;
}

/**
 * @brief A simple example showing how to add two values inside the TA and give the
 * result back to the CA. */
static int32_t CSMCIExample_add(CSMCIExample *me,
                                uint32_t val1,
                                uint32_t val2,
                                uint32_t *result_ptr)
{
  qsee_log(QSEE_LOG_MSG_DEBUG, "CSMCIExample_add called for instance index %d.", me->index);

  /* Here we use saturated math to ensure that our value doesn't overflow. */
  if (val1 >= UINT32_MAX - val2) {
    *result_ptr = UINT32_MAX;
  }

  else {
    *result_ptr = val1 + val2;
  }

  return Object_OK;
}

/**
 * @brief Function declaration for the invoke function of the ISMCIExample interface.
 */
static ISMCIExample_DEFINE_INVOKE(CSMCIExample_invoke, CSMCIExample_, CSMCIExample *);

int32_t CSMCIExample_open(Object *objOut)
{
  /* we can use this index to identify specific instances of ISMCIExample. */
  static int global_index = 0;

  CSMCIExample *me = QSEE_ZALLOC_TYPE(CSMCIExample);
  if (!me) {
    qsee_log(QSEE_LOG_MSG_ERROR, "Memory allocation for CSMCIExample failed!");
    return Object_ERROR_MEM;
  }

  me->refs = 1;
  me->index = global_index++;

  qsee_log(QSEE_LOG_MSG_DEBUG, "ISMCIExample instance number %d!", me->index);
  *objOut = (Object){CSMCIExample_invoke, me};
  return Object_OK;
}
