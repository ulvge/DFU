/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @file
 * LLC: Kernel thread functionality
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __DRIVERS__COHDA__LLC__LLC_THREAD_H__
#define __DRIVERS__COHDA__LLC__LLC_THREAD_H__
#ifdef __KERNEL__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include "llc-internal.h"

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

// forward decl
struct LLCDev;

/// LLC 'stub' timer
typedef struct LLCThread
{
  /// WorkQueue
  struct workqueue_struct *pQueue;

} tLLCThread;

//------------------------------------------------------------------------------
// Function declarations
//------------------------------------------------------------------------------

int LLC_ThreadSetup (struct LLCDev *pDev);
int LLC_ThreadRelease (struct LLCDev *pDev);

#else
#error This file should not be included from user space.
#endif // #ifdef __KERNEL__
#endif // #ifndef __DRIVERS__COHDA__LLC__LLC_THREAD_H__
/**
 * @}
 * @}
 */
