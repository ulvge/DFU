/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @file
 * LLC internal header
 *
 * This header file is for declarations and definitions internal to
 * the llc module. For public APIs and documentation, see
 * include/cohda/llc/llc.h and include/linux/cohda/llc/llc.h
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __DRIVERS__COHDA__LLC__LLC_INTERNAL_H__
#define __DRIVERS__COHDA__LLC__LLC_INTERNAL_H__
#ifdef __KERNEL__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include "cohda/llc/llc.h"
#include "llc-netdev.h"
#include "llc-monitor.h"
#include "llc-thread.h"
#include "llc-list.h"
#include "llc-msg.h"
#include "llc-usb.h"
#include "llc-firmware.h"
#include "llc-spi.h"
#include "llc-device.h"
#include "llc-hook.h"

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

// Don't enable the Linux 'module' exports if we're unit testing
#ifdef UNITTEST
#undef module_init
#define module_init(ARG)
#undef module_exit
#define module_exit(ARG)
#undef MODULE_AUTHOR
#define MODULE_AUTHOR(ARG)
#undef MODULE_DESCRIPTION
#define MODULE_DESCRIPTION(ARG)
#undef MODULE_LICENSE
#define MODULE_LICENSE(ARG)
#undef MODULE_VERSION
#define MODULE_VERSION(ARG)
#endif

/// Generic error code
#define LLC_STATUS_ERROR (-EINVAL)
/// Generic success value
#define LLC_STATUS_SUCCESS (0)

/// cw-mon-* frames in 'LLC' format (raw tx & rx packets)
#define LLC_MON_FORMAT_LLC (2)
/// cw-mon-* frames in 802.11 format (no tx & rx header)
#define LLC_MON_FORMAT_80211 (1)
/// cw-mon-* frames in radiotap format (radiotap + tx/rx header + 802.11)
#define LLC_MON_FORMAT_RADIOTAP (0)
/// Module parameter default for: monitor frame format
#define LLC_MON_FORMAT_DEFAULT  (LLC_MON_FORMAT_RADIOTAP)
/// Module parameter for SPI download of DFU bootloader
#define LLC_SPI_DFU (0)
/// Module parameter for SPI download of full firmware
#define LLC_SPI_FULL (1)

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Function declarations
//------------------------------------------------------------------------------
// Kernel module related internal functions
int LLC_Setup (struct LLCDev *pDev);
int LLC_Release (struct LLCDev *pDev);
tMKxStatus LLC_Start (struct MKx *pMKx);
tMKxStatus LLC_Stop (struct MKx *pMKx);

// LLCRemote (MKx) API
tMKxStatus LLC_Config (struct MKx *pMKx,
                       tMKxRadio Radio,
                       tMKxRadioConfig *pConfig);
tMKxStatus LLC_TxReq (struct MKx *pMKx,
                      tMKxTxPacket *pTxPkt,
                      void *pFrmPriv);
tMKxStatus LLC_TxCnf (struct MKx *pMKx,
                      tMKxTxPacket *pTxPkt,
                      void *pPriv,
                      tMKxStatus Result);
tMKxStatus LLC_TxFlush (struct MKx *pMKx,
                        tMKxRadio Radio,
                        tMKxChannel Chan,
                        tMKxTxQueue TxQ);
tMKxStatus LLC_RxAlloc (struct MKx *pMKx,
                        int BufLen,
                        uint8_t **ppBuf,
                        void **ppPriv);
tMKxStatus LLC_RxInd (struct MKx *pMKx,
                      tMKxRxPacket *pRxPkt,
                      void *pPriv);
tMKxStatus LLC_NotifInd (struct MKx *pMKx,
                         tMKxNotif Notif);
tMKxTSF LLC_GetTSF (struct MKx *pMKx);
tMKxStatus LLC_SetTSF (struct MKx *pMKx, tMKxTSF TSF);
tMKxStatus LLC_DebugReq (struct MKx *pMKx,
                         struct MKxIFMsg *pMsg);
tMKxStatus LLC_DebugInd (struct MKx *pMKx,
                         struct MKxIFMsg *pMsg);
tMKxStatus LLC_C2XSecReq (struct MKx *pMKx,
                          tMKxC2XSec *pMsg);
tMKxStatus LLC_C2XSecInd (struct MKx *pMKx,
                          tMKxC2XSec *pMsg);
int32_t LLC_GetMonitorFormat (void);
int32_t LLC_GetLoopback (void);
bool LLC_IsSilent (void);
uint32_t LLC_Get_SPIDownload(void);

static inline void LLC_SemInit (struct semaphore *pSem, int Val)
{
  sema_init(pSem, Val);
}
//down_timeout接口的实现过程与down接口的实现过程差不多，只是此接口  
//可以自定义超时时间，也就是如果在超时间内不能得到信号量，调用者会因为超时而自行唤醒。
//其实现过程如下，请注意超时参数的传入。
static inline int LLC_SemDown (struct semaphore *pSem)
{
  if (1)
    return down_timeout(pSem, msecs_to_jiffies(1000));
  else
    return down_killable(pSem);
}
//up接口用于唤醒处于等待的线程，对于某些不能获取信号量的线程，如果不  
//强制唤醒，那么也许会造此线程的死掉，所以，才有up接口。此接口的实现比较简单，
//其实现如下。
static inline void LLC_SemUp (struct semaphore *pSem)
{
  up(pSem);
}

#else
#error This file should not be included from user space.
#endif // #ifdef __KERNEL__
#endif // #ifndef __DRIVERS__COHDA__LLC__LLC_INTERNAL_H__
/**
 * @}
 * @}
 */
