/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @file
 * LLC: API functions
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include "cohda/llc/llc.h"
#include "cohda/llc/llc-api.h"
#include "llc-internal.h"
#include <net/sock.h> // for gfp_any()

#define D_SUBMODULE LLC_API
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
 * @brief Initialize the LLC
 * @param pMKx LLC handle obtained inside LLC_Init()
 * @return LLC_STATUS_SUCCESS (0) or a negative error code @sa eLLCStatus
 */
tMKxStatus LLC_Start (struct MKx *pMKx)
{
  int Res = LLC_STATUS_ERROR;
  struct LLCDev *pDev;
  int i;

  d_fnstart(D_INTERN, NULL, "(pMKx %p)\n", pMKx);
  d_assert(pMKx != NULL);

  pDev = LLC_Device(pMKx);
  d_assert(pDev != NULL);

  // Reset the stats counters
  for (i = 0; i < MKX_RADIO_COUNT; i++)
  {
    tMKxRadioStats Stats;
    memset(&(Stats), 0, sizeof(Stats));
    memcpy((void *)&(pMKx->State.Stats[i]), &(Stats), sizeof(Stats));
  }

  // Set the default channel configurations
  for (i = 0; i < MKX_RADIO_COUNT; i++)
  {
    tMKxRadioConfigData Cfg;
    memset(&Cfg, 0, sizeof(Cfg));
	/* 还偷懒 复制了 */
    // Setup MKX_CHANNEL_0 and then copy everything in to MKX_CHANNEL_1
    Cfg.Mode = MKX_MODE_OFF;
    Cfg.ChanConfig[MKX_CHANNEL_0].PHY.Bandwidth = MKXBW_10MHz;
    Cfg.ChanConfig[MKX_CHANNEL_0].PHY.ChannelFreq = 0; // 5000 + (5 * 178);
    Cfg.ChanConfig[MKX_CHANNEL_0].PHY.TxAntenna = MKX_ANT_1AND2;
    Cfg.ChanConfig[MKX_CHANNEL_0].PHY.RxAntenna = MKX_ANT_1AND2;
    Cfg.ChanConfig[MKX_CHANNEL_0].PHY.DefaultMCS = MKXMCS_R12QPSK;
    Cfg.ChanConfig[MKX_CHANNEL_0].PHY.DefaultTxPower = 64;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.DualTxControl = MKX_TXC_TXRX;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.CSThreshold = -65;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.SlotTime = 13;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.DIFSTime = 32 + (2 * 13);
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.SIFSTime = 32;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.EIFSTime = 90;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.MaxRetries = 7;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_VO].AIFS = 2;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_VO].CWMIN = (1 << 2) - 1;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_VO].CWMAX = (1 << 3) - 1;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_VO].TXOP = 0; // TODO
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_VI].AIFS = 3;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_VI].CWMIN = (1 << 3) - 1;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_VI].CWMAX = (1 << 4) - 1;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_VI].TXOP = 0; // TODO
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_BE].AIFS = 6;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_BE].CWMIN = (1 << 4) - 1;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_BE].CWMAX = (1 << 10) - 1;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_BE].TXOP = 0;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_BK].AIFS = 9;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_BK].CWMIN = (1 << 4) - 1;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_BK].CWMAX = (1 << 10) - 1;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_AC_BK].TXOP = 0;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_NON_QOS].AIFS = 2;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_NON_QOS].CWMIN = (1 << 2) - 1;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_NON_QOS].CWMAX = (1 << 3) - 1;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.TxQueue[MKX_TXQ_NON_QOS].TXOP = 0;
    // Accept broadcast frames, but do not respond
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[0].MatchCtrl = 0;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[0].Addr = 0xFFFFFFFFFFFFULL;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[0].Mask = 0xFFFFFFFFFFFFULL;
    // Accept anonymous frames, but do not respond
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[1].MatchCtrl = 0;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[1].Addr = 0x000000000000ULL;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[1].Mask = 0xFFFFFFFFFFFFULL;
    // Accept IPv6 multicast frames addressed to 33:33:xx:xx:xx:xx
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[2].MatchCtrl = 0;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[2].Addr = 0x000000003333ULL;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[2].Mask = 0x00000000ffffULL;
    // Accept frames addressed to our 1st MAC address (04:E5:48:00:00:00)
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[3].MatchCtrl = \
        (MKX_ADDRMATCH_RESPONSE_ENABLE | MKX_ADDRMATCH_LAST_ENTRY);
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[3].Addr = 0x00000048E504ULL;
    Cfg.ChanConfig[MKX_CHANNEL_0].MAC.AMSTable[3].Mask = 0xFFFFFFFFFFFFULL;
    Cfg.ChanConfig[MKX_CHANNEL_0].LLC.IntervalDuration = 50 * 1000; // 50ms
    Cfg.ChanConfig[MKX_CHANNEL_0].LLC.GuardDuration = 0;
	/* 和第一个设置相同的模式 */
    memmove((void *)&(Cfg.ChanConfig[MKX_CHANNEL_1]),
            (void *)&(Cfg.ChanConfig[MKX_CHANNEL_0]),
            sizeof(tMKxChanConfig));
    // Send the entire radio's configuration to the handle
    memcpy((void *)&(pMKx->Config.Radio[i]), &Cfg, sizeof(Cfg));
  }

  // Set the API function pointers
  pMKx->pPriv = NULL;
  pMKx->API.Functions.Config    	= LLC_Config;//MKx_Config
  pMKx->API.Functions.TxReq     	= LLC_TxReq;
  pMKx->API.Functions.TxFlush   	= LLC_TxFlush;
  pMKx->API.Functions.SetTSF    	= LLC_SetTSF;
  pMKx->API.Functions.GetTSF    	= LLC_GetTSF;
  pMKx->API.Functions.DebugReq  	= LLC_DebugReq;
  pMKx->API.Functions.C2XSecCmd= LLC_C2XSecReq;

  Res = LLC_STATUS_SUCCESS;

  d_fnend(D_INTERN, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief De-initialize the LLC
 * @param pMKx LLC handle obtained inside LLC_Init()
 * @return LLC_STATUS_SUCCESS (0) or a negative error code @sa eLLCStatus
 */
tMKxStatus LLC_Stop (struct MKx *pMKx)
{
  int Res = LLC_STATUS_ERROR;
  struct LLCDev *pDev;

  d_fnstart(D_INTERN, NULL, "(pMKx %p)\n", pMKx);
  d_assert(pMKx != NULL);

  pDev = LLC_Device(pMKx);
  d_assert(pDev != NULL);

  // Turn off the calllbacks
  pMKx->API.Callbacks.TxCnf     = NULL;
  pMKx->API.Callbacks.RxAlloc   = NULL;
  pMKx->API.Callbacks.RxInd     = NULL;
  pMKx->API.Callbacks.NotifInd  = NULL;
  pMKx->API.Callbacks.DebugInd  = NULL;
  pMKx->API.Callbacks.C2XSecRsp = NULL;
  pMKx->pPriv = NULL;

  Res = LLC_STATUS_SUCCESS;

  d_fnend(D_INTERN, NULL, "(pMKx %p) = %d\n", pMKx, Res);
  return Res;
}

/**
 * @copydoc fMKx_Config
 *
 */
tMKxStatus LLC_Config (struct MKx *pMKx,
                       tMKxRadio Radio,
                       tMKxRadioConfig *pConfig)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;

  d_fnstart(D_API, NULL, "(pMKx %p)\n", pMKx);

  if ((pMKx == NULL) || (pConfig == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  // Get a handle of the overarching device
  pDev = LLC_Device(pMKx);
  if (pDev == NULL)
  {
    Res = -ENODEV;
    goto Error;
  }

  Res = LLC_MsgCfgReq(pDev, Radio, pConfig);

Error:
  d_fnend(D_API, NULL, "(pMKx %p) = %d\n", pMKx, Res);
  return Res;
}

/**
 * @copydoc fMKx_TxReq
 *
 */
tMKxStatus LLC_TxReq (struct MKx *pMKx,
                      tMKxTxPacket *pTxPkt,
                      void *pPriv)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;

  d_fnstart(D_API, NULL, "(pMKx %p)\n", pMKx);
  if ((pMKx == NULL) || (pTxPkt == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  // Get a handle of the overarching device
  pDev = LLC_Device(pMKx);
  if (pDev == NULL)
  {
    Res = -ENODEV;
    goto Error;
  }

  Res = LLC_MsgTxReq(pDev, pTxPkt, pPriv);

Error:
  d_fnend(D_API, NULL, "(pMKx %p) = %d\n", pMKx, Res);
  return Res;
}


/**
 * @copydoc fMKx_TxFlush
 *
 */
tMKxStatus LLC_TxFlush (struct MKx *pMKx,
                        tMKxRadio Radio,
                        tMKxChannel Chan,
                        tMKxTxQueue TxQ)
{
  int Res = -ENOSYS;
  d_fnstart(D_API, NULL, "(pMKx %p)\n", pMKx);

  // TODO

  d_fnend(D_API, NULL, "(pMKx %p) = %d\n", pMKx, Res);
  return Res;
}

/**
 * @copydoc fMKx_GetTSF
 *
 */
tMKxTSF LLC_GetTSF (struct MKx *pMKx)
{
  int Res = -ENOSYS;
  d_fnstart(D_API, NULL, "(pMKx %p)\n", pMKx);

  // TODO

  d_fnend(D_API, NULL, "(pMKx %p) = %d\n", pMKx, Res);
  return Res;
}

/**
 * @copydoc fMKx_SetTSF
 *
 */
tMKxStatus LLC_SetTSF (struct MKx *pMKx,
                       tMKxTSF TSF)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;

  d_fnstart(D_API, NULL, "(pMKx %p)\n", pMKx);
  if (pMKx == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  // Get a handle of the overarching device
  pDev = LLC_Device(pMKx);
  if (pDev == NULL)
  {
    Res = -ENODEV;
    goto Error;
  }

  Res = LLC_MsgSetTSFReq(pDev, TSF);

Error:
  d_fnend(D_API, NULL, "(pMKx %p) = %d\n", pMKx, Res);
  return Res;
}


/**
 * @copydoc fMKx_TxCnf
 *
 * If the client has set the callback function pointer (non NULL) then call it!
 */
tMKxStatus LLC_TxCnf (struct MKx *pMKx,
                      tMKxTxPacket *pTxPkt,
                      void *pPriv,
                      tMKxStatus Result)
{
  int Res = LLC_STATUS_ERROR;
  d_fnstart(D_API, NULL, "(pMKx %p pTxPkt %p pPriv %p Result %d)\n",
            pMKx, pTxPkt, pPriv, Result);

  if (pMKx->API.Callbacks.TxCnf != NULL)
  {
#define DUMMY_PTR_FROM_APP_LLC_API 0xFFFFC0DA
    // Bypass dot4 callback if TxCnf corresponds to TX from app layer API
    // to avoid crash when accessing dummy pPriv pointer
    if (pPriv == (void *)DUMMY_PTR_FROM_APP_LLC_API)
    {
      Res = LLC_STATUS_SUCCESS;
    }
    else
    {
      Res = pMKx->API.Callbacks.TxCnf(pMKx, pTxPkt, pPriv, Result);
    }
  }
  else
  {
    // No callback should not mean a failure
    Res = LLC_STATUS_SUCCESS;
  }

  d_fnend(D_API, NULL, "(pMKx %p pTxPkt %p pPriv %p Result %d) = %d\n",
          pMKx, pTxPkt, pPriv, Result, Res);
  return Res;
}

/// Amount of space to reserve for stack headers at the front of the skb
#define LLC_ALLOC_EXTRA_HEADER (56)

/// Amount of space to reserve for stack footers at the end of the skb
#define LLC_ALLOC_EXTRA_FOOTER (16)

static tMKxStatus _LLC_RxAlloc (struct MKx *pMKx,
                                int BufLen,
                                uint8_t **ppBuf,
                                void **ppPriv)
{
  int Res = LLC_STATUS_ERROR;
  struct sk_buff *pSkb = NULL;

  d_fnstart(D_API, NULL, "(pMKx %p BufLen %d ppBuf %p ppPriv %p)\n",
            pMKx, BufLen, ppBuf, ppPriv);

  pSkb = alloc_skb(LLC_ALLOC_EXTRA_HEADER + BufLen + LLC_ALLOC_EXTRA_FOOTER,
                   gfp_any());
  if (pSkb != NULL)
  {
    // reserve space in line with what Dot4 does to keep things
    // consistent
    skb_reserve(pSkb, LLC_ALLOC_EXTRA_HEADER);
    // pre-insert data into skb
    skb_put(pSkb, BufLen);
    *ppPriv = (void *)pSkb;
    *ppBuf = (uint8_t *)(pSkb->data);
    Res = MKXSTATUS_SUCCESS;
  }

  d_fnend(D_API, NULL, "(pMKx %p BufLen %d ppBuf %p ppPriv %p) = %d\n",
          pMKx, BufLen, ppBuf, ppPriv, Res);
  return Res;
}

/**
 * @copydoc fMKx_RxAlloc
 *
 * If the client has set the callback function pointer (non NULL) then call it!
 * otherwise if we have no RxAlloc() or RxInd() registered by the client, use
 * our own internal implementation so that packets can still be received in
 * userspace via the llc netdev
 */
tMKxStatus LLC_RxAlloc (struct MKx *pMKx,
                        int BufLen,
                        uint8_t **ppBuf,
                        void **ppPriv)
{
  int Res = LLC_STATUS_ERROR;
  d_fnstart(D_API, NULL, "(pMKx %p BufLen %d ppBuf %p ppPriv %p)\n",
            pMKx, BufLen, ppBuf, ppPriv);

  if (pMKx->API.Callbacks.RxAlloc != NULL)
  {
    Res = pMKx->API.Callbacks.RxAlloc(pMKx, BufLen, ppBuf, ppPriv);
    if (*ppBuf == NULL)
      Res = -ENOMEM;
  }
  // if we have neither an RxAlloc or RxInd then use internal implementations so
  // packets can still be received via the llc netdev to userspace
  else if (pMKx->API.Callbacks.RxInd == NULL)
  {
    Res = _LLC_RxAlloc(pMKx, BufLen, ppBuf, ppPriv);
  }

  d_fnend(D_API, NULL, "(pMKx %p BufLen %d ppBuf %p ppPriv %p) = %d\n",
          pMKx, BufLen, ppBuf, ppPriv, Res);
  return Res;
}

static tMKxStatus _LLC_RxInd (struct MKx *pMKx,
                              tMKxRxPacket *pRxPkt,
                              void *pPriv)
{
  d_fnstart(D_API, NULL, "(pMKx %p pRxPkt %p pPriv %p)\n",
            pMKx, pRxPkt, pPriv);

  // free the packet which would have been allocated by _LLC_RxAlloc()
  kfree_skb((struct sk_buff *)pPriv);

  d_fnend(D_API, NULL, "(pMKx %p pRxPkt %p pPriv %p) = %d\n",
          pMKx, pRxPkt, pPriv, LLC_STATUS_SUCCESS);
  return LLC_STATUS_SUCCESS;
}

/**
 * @copydoc fMKx_RxInd
 *
 * If the client has set the callback function pointer (non NULL) then call it!
 * otherwise if we have no RxAlloc() or RxInd() registered by the client, use
 * our own internal implementation so that packets can still be received in
 * userspace via the llc netdev
 */
tMKxStatus LLC_RxInd (struct MKx *pMKx,
                      tMKxRxPacket *pRxPkt,
                      void *pPriv)
{
  int Res = LLC_STATUS_ERROR;
  d_fnstart(D_API, NULL, "(pMKx %p pRxPkt %p pPriv %p)\n",
            pMKx, pRxPkt, pPriv);

  if (pMKx->API.Callbacks.RxInd != NULL)
  {
    Res = pMKx->API.Callbacks.RxInd(pMKx, pRxPkt, pPriv);
  }
  // if we have neither an RxAlloc or RxInd then use internal implementations so
  // packets can still be received via the llc netdev to userspace
  else if (pMKx->API.Callbacks.RxAlloc == NULL)
  {
    Res = _LLC_RxInd(pMKx, pRxPkt, pPriv);
  }

  d_fnend(D_API, NULL, "(pMKx %p pRxPkt %p pPriv %p) = %d\n",
          pMKx, pRxPkt, pPriv, Res);
  return Res;
}


/**
 * @copydoc fMKx_NotifInd
 *
 * If the client has set the callback function pointer (non NULL) then call it!
 */
tMKxStatus LLC_NotifInd (struct MKx *pMKx,
                         tMKxNotif Notif)
{
  int Res = LLC_STATUS_ERROR;
  d_fnstart(D_VERBOSE, NULL, "(pMKx %p Notif 0x%08X)\n", pMKx, Notif);

  if (pMKx->API.Callbacks.NotifInd != NULL)
  {
    Res = pMKx->API.Callbacks.NotifInd(pMKx, Notif);
  }
  else
  {
    // No callback should not mean a failure
    Res = LLC_STATUS_SUCCESS;
  }

  d_fnend(D_VERBOSE, NULL, "(pMKx %p Notif 0x%08X) = %d\n", pMKx, Notif, Res);
  return Res;
}


/**
 * @copydoc fC2XSec_CommandReq
 *
 */
tMKxStatus LLC_C2XSecReq (struct MKx *pMKx,
                          tMKxC2XSec *pMsg)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;

  d_fnstart(D_API, NULL, "(pMKx %p)\n", pMKx);
  if ((pMKx == NULL) || (pMsg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  // Get a handle of the overarching device
  pDev = LLC_Device(pMKx);
  if (pDev == NULL)
  {
    Res = -ENODEV;
    goto Error;
  }

  Res = LLC_MsgSecReq(pDev, pMsg);

Error:
  d_fnend(D_API, NULL, "(pMKx %p) = %d\n", pMKx, Res);
  return Res;
}


/**
 * @copydoc fC2XSec_ReponseInd
 *
 * If the client has set the callback function pointer (non NULL) then call it!
 */
tMKxStatus LLC_C2XSecInd (struct MKx *pMKx,
                          tMKxC2XSec *pMsg)
{
  int Res = LLC_STATUS_ERROR;
  d_fnstart(D_API, NULL, "(pMKx %p pMsg %p)\n", pMKx, pMsg);

  if (pMKx->API.Callbacks.C2XSecRsp != NULL)
  {
    Res = pMKx->API.Callbacks.C2XSecRsp(pMKx, pMsg);
  }
  else
  {
    // No callback should not mean a failure
    Res = LLC_STATUS_SUCCESS;
  }

  d_fnend(D_API, NULL, "(pMKx %p pMsg %p) = %d\n", pMKx, pMsg, Res);
  return Res;
}


/**
 * @copydoc fMKx_DebugReq
 *
 */
tMKxStatus LLC_DebugReq (struct MKx *pMKx,
                         struct MKxIFMsg *pMsg)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;

  d_fnstart(D_API, NULL, "(pMKx %p)\n", pMKx);

  if ((pMKx == NULL) || (pMsg == NULL))
  {
    Res = -EINVAL;
    goto Error;
  }

  // Get a handle of the overarching device
  pDev = LLC_Device(pMKx);
  if (pDev == NULL)
  {
    Res = -ENODEV;
    goto Error;
  }

  Res = LLC_MsgDbgReq(pDev, pMsg);

Error:
  d_fnend(D_API, NULL, "(pMKx %p) = %d\n", pMKx, Res);
  return Res;
}

/**
 * @copydoc fMKx_DebugInd
 *
 * If the client has set the callback function pointer (non NULL) then call it!
 */
tMKxStatus LLC_DebugInd (struct MKx *pMKx,
                         struct MKxIFMsg *pMsg)
{
  int Res = LLC_STATUS_ERROR;
  d_fnstart(D_API, NULL, "(pMKx %p pMsg %p)\n", pMKx, pMsg);

  if (pMKx->API.Callbacks.DebugInd != NULL)
  {
    Res = pMKx->API.Callbacks.DebugInd(pMKx, pMsg);
  }
  else
  {
    // No callback should not mean a failure
    Res = LLC_STATUS_SUCCESS;
  }

  d_fnend(D_API, NULL, "(pMKx %p pMsg %p) = %d\n", pMKx, pMsg, Res);
  return Res;
}

/**
 * @}
 * @}
 */

