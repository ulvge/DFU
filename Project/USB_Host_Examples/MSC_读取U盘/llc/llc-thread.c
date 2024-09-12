/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @section cohda_llc_thread
 *
 * @file
 * LLC: Kernel thread related functionality
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <linux/version.h>

#include "cohda/llc/llc.h"
#include "llc-internal.h"
#include "llc-thread.h"

#define D_SUBMODULE LLC_Thread
#include "debug-levels.h"
//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

/**
 * @brief Setup the LLC kernel thread
 * @param pDev LLC device pointer
 * @return Zero (LLC_STATUS_SUCCESS) or a negative errno
 */
int LLC_ThreadSetup (struct LLCDev *pDev)
{
  int Res = LLC_STATUS_ERROR;
  const char *pName = "cw-llc";

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  // Workqueue thread init
  Res = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
  pDev->Thread.pQueue = create_singlethread_workqueue(pName);
#else
  {
    int Realtime = 1;
    int SingleThread = 1;
    pDev->Thread.pQueue = __create_workqueue(pName, SingleThread, 0, Realtime);
  }
#endif
  if (pDev->Thread.pQueue == NULL)
  {
    d_error(D_WARN, NULL, "Failed to create %s kernel thread\n", pName);
    Res = -EAGAIN;
  }

  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Release (cleanup) the LLC thread
 * @param pDev LLC device pointer
 * @return Zero (LLC_STATUS_SUCCESS) or a negative errno
 */
int LLC_ThreadRelease (struct LLCDev *pDev)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  // Cleanup the workqueue
  if (pDev->Thread.pQueue != NULL)
  {
    flush_workqueue(pDev->Thread.pQueue);
    destroy_workqueue(pDev->Thread.pQueue);
  }
  Res = 0;

  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @}
 * @}
 */

