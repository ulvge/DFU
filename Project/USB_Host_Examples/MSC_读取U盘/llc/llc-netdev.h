/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @file
 * LLC: netdev submodule
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __DRIVER__COHDA__LLC__LLC_NETDEV_H_
#define __DRIVER__COHDA__LLC__LLC_NETDEV_H_
#ifdef __KERNEL__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <linux/netdevice.h>
#include <linux/workqueue.h>

#include "llc-internal.h"

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

// Forward decl
struct LLCDev;

/// A 'cw-llc' networking device's private storage structure
typedef struct LLCNetDevPriv
{
  /// Pointer back to the over-arching llc device structure
  struct LLCDev *pDev;

} tLLCNetDevPriv;

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------

/**
 * @brief Extract the LLC handle from within the net device's private storage
 * @param pNetDev Net device pointer
 * @return Pointer to the LLC structure
 *
 */
static inline struct LLCDev *LLC_NetDevToDev (struct net_device *pNetDev)
{
  struct LLCNetDevPriv *pLLCNetDevPriv = netdev_priv(pNetDev);
  return pLLCNetDevPriv->pDev;
}

int LLC_NetDevSetup (struct LLCDev *pDev);
int LLC_NetDevRelease (struct LLCDev *pDev);

void LLC_NetDevRx (struct LLCDev *pDev,
                   struct sk_buff *pRxSkb);
int LLC_NetDevInd (struct LLCDev *pDev,
                   struct MKxIFMsg *pMsg);

#else
#error This file should not be included from user space.
#endif // #ifdef __KERNEL__
#endif // #ifndef __DRIVER__COHDA__LLC__LLC_NETDEV_H_
/**
 * @}
 * @}
 */
