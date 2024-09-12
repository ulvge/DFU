/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @addtogroup cohda_llc_monitor LLC monitoring interfaces
 * @{
 *
 * @file
 * LLC: monitoring interfaces
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __DRIVERS__COHDA__LLC__LLC_MONITOR_H__
#define __DRIVERS__COHDA__LLC__LLC_MONITOR_H__
#ifdef __KERNEL__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <linux/netdevice.h>
#include "cohda/llc/llc-api.h"

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

// forward decl(s)
struct LLCDev;

/// Monitor type: TX or RX
typedef enum LLCMonType
{
  LLC_MONTYPE_TX = 0,
  LLC_MONTYPE_RX = 1,
  // ...
  /// Used for array dimensioning
  LLC_MONTYPE_COUNT,
  /// Used for bounds checking
  LLC_MONTYPE_MAX = LLC_MONTYPE_COUNT - 1,
} eLLCMonType;
/// @copydoc eLLCMonType
typedef int8_t tLLCMonType;

/// An LLC monitor entry. The table is included in @ref tLLCDev
typedef struct LLCMonitor
{
  /// Monitor netdev
  struct net_device *pNetDev[MKX_RADIO_COUNT+1][MKX_CHANNEL_COUNT+1][LLC_MONTYPE_COUNT+1];
} tLLCMonitor;

//------------------------------------------------------------------------------
// Function declarations
//------------------------------------------------------------------------------
int LLC_MonitorSetup (struct LLCDev *pDev);

int LLC_MonitorRelease (struct LLCDev *pDev);

void LLC_MonitorRx (struct LLCDev *pDev,
                    struct MKxRxPacket *pPkt);

void LLC_MonitorTx (struct LLCDev *pDev,
                    struct MKxTxPacket *pPkt);

#else
#error This file should not be included from user space.
#endif // #ifdef __KERNEL__
#endif // #ifndef __DRIVERS__COHDA__LLC__LLC_MONITOR_H__
/**
 * @}
 * @}
 * @}
 */
