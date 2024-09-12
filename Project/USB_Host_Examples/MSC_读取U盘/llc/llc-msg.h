/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @file
 * LLC: Transport agnostic LLC <-> SAF5100 messaging functionality
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __DRIVERS__COHDA__LLC__LLC_MSG_H__
#define __DRIVERS__COHDA__LLC__LLC_MSG_H__
#ifdef __KERNEL__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <linux/semaphore.h>
#include <linux/workqueue.h>

#include "llc-list.h"
#include "llc-internal.h"

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

// forward decl
struct LLCDev;

/// Common LLC message container (cast from tLLCListItem)
typedef struct LLCMsgCtx
{
  /// Next, Prev pointers
  struct LLCList List;
  /// Unique Id
  uint8_t Id;
  /// Reference counter
  int8_t RefCnt;
  /// Return code
  int16_t Ret;
  /// The buffer pointer
  uint8_t *pBuf;
  /// Length of the buffer
  int32_t Len;
  // Other contextual data...
} tLLCMsgCtx;

/// Config()/TxReq() message context (cast from tLLCListItem)
typedef struct LLCOutCtx
{
  /// Next, Prev pointers
  struct LLCList List;
  /// Unique Id
  uint8_t Id;
  /// Reference counter
  int8_t RefCnt;
  /// Return code
  int16_t Ret;
  /// The buffer pointer
  uint8_t *pBuf;
  /// Length of the buffer
  int32_t Len;

  /// The private buffer pointer
  void *pPriv;
  /// The USB bulk urb
  struct urb *pBulkUrb;
  /// Kernel work structure (handling USB out completions)
  struct work_struct BulkWork;
  /// Kernel work structure (submitting USB out requests)
  struct work_struct Work;
  /// Semaphore to wait on for completion
  struct semaphore Semaphore;
  // Debug
  struct timespec Before;
  struct timespec After;
} tLLCOutCtx;

/// RxAlloc()/*Ind()/*.Cnf() message context  (cast from tLLCListItem)
typedef struct LLCInCtx
{
  /// Next, Prev pointers
  struct LLCList List;
  /// Unique Id
  uint8_t Id;
  /// Reference counter
  int8_t RefCnt;
  /// Return code
  int16_t Ret;
  /// The buffer pointer
  uint8_t *pBuf;
  /// Length of the buffer
  int32_t Len;

  /// The private buffer pointer
  void *pPriv;
  /// The associated USB bulk urb
  struct urb *pBulkUrb;
  /// Kernel work structure
  struct work_struct BulkWork;
} tLLCInCtx;


//------------------------------------------------------------------------------
// Function declarations
//------------------------------------------------------------------------------

int LLC_MsgSetup (struct LLCDev *pDev);
int LLC_MsgRelease (struct LLCDev *pDev);

int LLC_MsgRecv (struct LLCDev *pDev, struct LLCInCtx *pCtx);
int LLC_MsgSend (struct LLCDev *pDev, struct MKxIFMsg *pMsg);

// Config()
int LLC_MsgCfgReq (struct LLCDev *pDev,
                   tMKxRadio Radio,
                   tMKxRadioConfig *pConfig);
// TxReq()
int LLC_MsgTxReq (struct LLCDev *pDev,
                  tMKxTxPacket *pTxPkt,
                  void *pPriv);
// DebugReq()
int LLC_MsgDbgReq (struct LLCDev *pDev,
                   struct MKxIFMsg *pMsg);
// SecReq()
int LLC_MsgSecReq (struct LLCDev *pDev,
                   tMKxC2XSec *pMsg);
// SetTSF()
int LLC_MsgSetTSFReq (struct LLCDev *pDev,
                      tMKxTSF TSF);

#else
#error This file should not be included from user space.
#endif // #ifdef __KERNEL__
#endif // #ifndef __DRIVERS__COHDA__LLC__LLC_MSG_H__
/**
 * @}
 * @}
 */
