// Copyright (c) 2025, Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause

#include "CSMCIExample_open.h"
#include "object.h"
#include "qsee_log.h"
#include "qsee_ta_entry.h"
#include <stdint.h>

/**
 * @brief tz_app_init() is called on application start.
 *
 * Any initialization required before the TA is ready to handle commands
 * should be placed here.
 */
void tz_app_init(void)
{
  /* Get the current log mask */
  uint8_t log_mask = qsee_log_get_mask();

  /* Enable debug level logs */
  qsee_log_set_mask(log_mask | QSEE_LOG_MSG_DEBUG);
  qsee_log(QSEE_LOG_MSG_DEBUG, "App Start");
}

/**
 * @brief tz_app_shutdown() is called on application shutdown.
 *
 *  Any deinitialization required before the TA is unloaded should be placed
 *  here.
 */
void tz_app_shutdown(void)
{
  qsee_log(QSEE_LOG_MSG_DEBUG, "App shutdown");
}

/**
 * @brief app_getAppObject() return an object for 'ISMCIExample' interface
 *
 * The CA gets to this point using the IAppController interface; specifically
 * IAppController_getAppObject(). The CA implementation shows an example
 * of the steps involved in reaching this point.
 */
int32_t app_getAppObject(Object credentials, Object *obj_ptr)
{
  (void)credentials;
  return CSMCIExample_open(obj_ptr);
}
