/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @file
 * LLC: Device management related functionality
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __DRIVERS__COHDA__LLC__LLC_DEVICE_H__
#define __DRIVERS__COHDA__LLC__LLC_DEVICE_H__
#ifdef __KERNEL__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <linux/types.h>
#include "cohda/llc/llc-api.h"
#include "llc-internal.h"

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// 'Magic' value inside a #tLLCDev
#define LLC_DEV_MAGIC          (0xC0DA)

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

/// LLC configuration
typedef struct LLCConfig
{
  /// cw-mon-* format (0: lmac, else 802.11)
  uint32_t MonitorFormat;

} tLLCConfig;

/// LLC device structure
typedef struct LLCDev
{
  /// Value is LLC_DEV_MAGIC if this is a valid structure
  uint16_t Magic;
  /// 16 bits reserved for sundry status information
  uint16_t Unused;
  /// LLC configuration (storage)
  struct LLCConfig Cfg;
  /// The MKx API object
  struct MKx MKx;
  /// The 'cw-llc' netdev interface
  struct net_device *pNetDev;
  /// Monitor interfaces 'cw-mon-*'
  struct LLCMonitor Monitor;
  /// Kernel thread (context for NotifInd, TxCnf, RxAlloc & RxInd)
  struct LLCThread Thread;
  /// The 'cw-llc' USB interface to the SAF5100
  struct LLCUSB USB;
  /// The list item pool
  struct LLCListItemPool ListPool;

  /// The firmware programming of the SAF5100
  struct LLCFirmware Firmware;
  /// SPI interface for firmware programming of the SAF5100
  struct LLCSPI SPI;

} tLLCDev;

//------------------------------------------------------------------------------
// Function declarations
//------------------------------------------------------------------------------

int LLC_DeviceSetup (struct LLCDev *pDev);
int LLC_DeviceRelease (struct LLCDev *pDev);

struct LLCDev *LLC_DeviceGet (void);
struct LLCDev *LLC_Device (struct MKx *pMKx);

int LLC_DevicePointerCheck (void *pPtr);

int LLC_Ioctl (struct LLCDev *pDev,
               struct ifreq *pReq,
               int Cmd);

#else
#error This file should not be included from user space.
#endif // #ifdef __KERNEL__
#endif // #ifndef __DRIVERS__COHDA__LLC__LLC_DEVICE_H__
/**
 * @}
 * @}
 */
