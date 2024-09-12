/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @section cohda_llc_msg
 *
 * @file
 * LLC: Transport agnostic LLC <-> SAF5100 messaging functionality
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include "cohda/llc/llc.h"
#include "llc-internal.h"
#include "llc-msg.h"

#define D_SUBMODULE LLC_Msg
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

static int LLC_MsgTxCnf (struct LLCDev *pDev,
                         struct LLCInCtx *pCtx,
                         struct MKxIFMsg *pMsg);
static int LLC_MsgRxPkt (struct LLCDev *pDev,
                         struct LLCInCtx *pCtx,
                         struct MKxIFMsg *pMsg);
static int LLC_MsgCfgInd (struct LLCDev *pDev,
                          struct LLCInCtx *pCtx,
                          struct MKxIFMsg *pMsg);
static int LLC_MsgDbgInd (struct LLCDev *pDev,
                          struct LLCInCtx *pCtx,
                          struct MKxIFMsg *pMsg);
static int LLC_MsgStatsInd (struct LLCDev *pDev,
                            struct LLCInCtx *pCtx,
                            struct MKxIFMsg *pMsg);
static int LLC_MsgSecInd (struct LLCDev *pDev,
                          struct LLCInCtx *pCtx,
                          struct MKxIFMsg *pMsg);


/**
 * @brief Setup the LLC messaging handling
 * @param pDev LLC device pointer
 * @return Zero (LLC_STATUS_SUCCESS) or a negative errno
 */
int LLC_MsgSetup(struct LLCDev *pDev)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  if (sizeof(struct LLCListItem) < sizeof(struct LLCMsgCtx))
  {
    d_error(D_CRIT, NULL, "tLLCMsgCtx too big! %d:%d\n",
	    sizeof(struct LLCListItem), sizeof(struct LLCMsgCtx));
    Res = -ENODEV;
    goto Error;
  }
  if (sizeof(struct LLCListItem) < sizeof(struct LLCOutCtx))
  {
    d_error(D_CRIT, NULL, "tLLCOutCtx too big! %d:%d\n",
	    sizeof(struct LLCListItem), sizeof(struct LLCOutCtx));
    Res = -ENODEV;
    goto Error;
  }
  if (sizeof(struct LLCListItem) < sizeof(struct LLCInCtx))
  {
    d_error(D_CRIT, NULL, "tLLCInCtx too big! %d:%d\n",
	    sizeof(struct LLCListItem), sizeof(struct LLCInCtx));
    Res = -ENODEV;
    goto Error;
  }

  Res = 0;

Error:
  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Release (cleanup) the LLC messaging
 * @param pDev LLC device pointer
 * @return Zero (LLC_STATUS_SUCCESS) or a negative errno
 */
int LLC_MsgRelease (struct LLCDev *pDev)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  ; // TODO
  Res = 0;

  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}


/**
 * @brief Handle a message from the MKx
 */
int LLC_MsgRecv(struct LLCDev *pDev, struct LLCInCtx *pCtx)
{
  int Res = LLC_STATUS_ERROR;
  struct MKxIFMsg *pMsg = NULL;

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  pMsg = (struct MKxIFMsg *)(pCtx->pBuf);
  // Sanity check the lengths then trim off the header
  if ((pCtx->Len < (int32_t)sizeof(struct MKxIFMsg)) || (pCtx->Len < pMsg->Len))
  {
    d_error(D_WARN, NULL, "Truncated message: pCtx %d pMsg %d\n",
	    pCtx->Len, pMsg->Len);
    //d_dump(D_WARN, NULL, pMsg, pMsg->Len);
    goto Error;
  }

  d_printf(D_DBG, NULL, "Msg: Type:%d Len:%d Seq:%d Ret:%d\n",
           pMsg->Type, pMsg->Len, pMsg->Seq, pMsg->Ret);

  // Echo the message to the netdev
  Res = LLC_NetDevInd(pDev, pMsg);//接收帧
  if (Res != 0)
    d_error(D_WARN, NULL, "Failed to echo message to netdev\n");

  switch (pMsg->Type)
  {
    case MKXIF_TXEVENT: // A transmit confirm (message data is tMKxTxEvent)
      Res = LLC_MsgTxCnf(pDev, pCtx, pMsg);
      break;

    case MKXIF_RXPACKET: // A received packet (message data is tMKxRxPacket)
      Res = LLC_MsgRxPkt(pDev, pCtx, pMsg);
      break;

    case MKXIF_RADIOASTATS: // Radio A stats (message data is tMKxRadioStats)
    case MKXIF_RADIOBSTATS: // Radio B stats (message data is tMKxRadioStats)
      Res = LLC_MsgStatsInd(pDev, pCtx, pMsg);
      break;

    case MKXIF_RADIOACFG: // Radio A config (message data is tMKxRadioConfig)
    case MKXIF_RADIOBCFG: // Radio B config (message data is tMKxRadioConfig)
      Res = LLC_MsgCfgInd(pDev, pCtx, pMsg);
      break;

    case MKXIF_DEBUG: // Generic debug container (message data is tMKxDebug)
    case MKXIF_CAL: // Generic calibration container (message data is tMKxCalMsg)
      Res = LLC_MsgDbgInd(pDev, pCtx, pMsg);
      break;

    case MKXIF_C2XSEC: // A C2X security response (message data is tMKxC2XSec)
      Res = LLC_MsgSecInd(pDev, pCtx, pMsg);
      break;

    case MKXIF_TXPACKET: // Transmit packet data
    case MKXIF_SET_TSF: // New UTC Time (obtained from NMEA data)
    case MKXIF_FLUSHQ: // Flush a single queue or all queueus
      d_printf(D_ERR, NULL, "Unexpected recv message type %d\n", pMsg->Type);
      break;
    default:
      d_printf(D_ERR, NULL, "Unknown recv message type %d\n", pMsg->Type);
      break;
  }

Error:
  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Send on a message verbatim (comes from 'cw-llc' netdev)
 */
int LLC_MsgSend (struct LLCDev *pDev, struct MKxIFMsg *pMsg)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);
  d_assert(pMsg != NULL);

  d_printf(D_DBG, NULL, "Msg: Type:%d Len:%d Seq:%d Ret:%d\n",
           pMsg->Type, pMsg->Len, pMsg->Seq, pMsg->Ret);

  switch (pMsg->Type)
  {
    case MKXIF_TXPACKET: // Transmit packet data
      {
        tMKxTxPacket *pTxPkt = (tMKxTxPacket *)pMsg;
        void *pPriv = (void *)((int)(pMsg->Ret)); // <- Note the use of Ret for userspace pPriv

        Res = LLC_MsgTxReq(pDev, pTxPkt, pPriv);
      }
      break;

    case MKXIF_RADIOACFG: // Radio A config (message data is tMKxRadioConfig)
    case MKXIF_RADIOBCFG: // Radio B config (message data is tMKxRadioConfig)
      {
        tMKxRadio Radio = MKX_RADIO_A;
        tMKxRadioConfig *pConfig = (tMKxRadioConfig *)pMsg;

        if (pMsg->Type == MKXIF_RADIOBCFG)
          Radio = MKX_RADIO_B;

        Res = LLC_MsgCfgReq(pDev, Radio, pConfig);
      }
      break;

    case MKXIF_SET_TSF: // New UTC Time (obtained from NMEA data)
      {
        tMKxTSF *pTSF = (tMKxTSF *)pMsg->Data;
        Res = LLC_MsgSetTSFReq(pDev, *pTSF);
      }
      break;

    case MKXIF_FLUSHQ: // Flush a single queue or all queueus
      d_error(D_ERR, NULL, "Unsuported send message type %d\n", pMsg->Type);
      break;

    case MKXIF_DEBUG: // Generic debug container (message data is tMKxDebug)
    case MKXIF_CAL: // Generic calibration container (message data is tMKxCalMsg)
      Res = LLC_MsgDbgReq(pDev, pMsg);
      break;

    case MKXIF_C2XSEC: // A C2X security command (message data is tMKxC2XSec)
      Res = LLC_MsgSecReq(pDev, (tMKxC2XSec *)pMsg);
      break;

    case MKXIF_TXEVENT: // A transmit confirm (message data is tMKxTxEvent)
    case MKXIF_RXPACKET: // A received packet (message data is tMKxRxPacket)
    case MKXIF_RADIOASTATS: // Radio A stats (message data is tMKxRadioStats)
    case MKXIF_RADIOBSTATS: // Radio A stats (message data is tMKxRadioStats)
      d_error(D_ERR, NULL, "Unexpected send message type %d\n", pMsg->Type);
      break;
    default:
      d_error(D_ERR, NULL, "Unknown send message type %d\n", pMsg->Type);
      break;
  }

  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Create a TxReq message and pass it to the underlying 
 *   transport
 *
 */
int LLC_MsgTxReq (struct LLCDev *pDev,
                  tMKxTxPacket *pTxPkt,
                  void *pPriv)
{
  int Res = LLC_STATUS_ERROR;
  struct LLCOutCtx *pCtx = NULL;

  d_fnstart(D_TST, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);
  d_assert(pTxPkt != NULL);

  // Check whether we are silent
  if (LLC_IsSilent())
  {
    d_error(D_INFO, NULL, "LLC silent, dropping transmit frame\n");
    goto Error;
  }

  // 'Sniff' this packet on the cw-mon-t[a|b][0|1] monitoring interface
  LLC_MonitorTx(pDev, pTxPkt);

  // Local loopback for testing purposes
  if (LLC_GetLoopback() != 0) //返回值在这里为 0
  {
    do {
      int RxLen = sizeof(struct MKxRxPacket) + \
                  pTxPkt->TxPacketData.TxFrameLength;
      tMKxRxPacket *pRxPkt = NULL;
      void *pRxPriv = NULL;

      Res = LLC_RxAlloc(&(pDev->MKx), RxLen, (void *)&(pRxPkt), &(pRxPriv));
      if (Res != 0)
        break;

      // Populate the fake 'rx' bits that we know
      pRxPkt->Hdr.Type = MKXIF_RXPACKET;
      pRxPkt->Hdr.Len = sizeof(struct MKxRxPacket) + \
                        pTxPkt->TxPacketData.TxFrameLength;
      pRxPkt->Hdr.Seq = 0;
      pRxPkt->Hdr.Ret = 0;
      pRxPkt->RxPacketData.RadioID = pTxPkt->TxPacketData.RadioID;
      pRxPkt->RxPacketData.ChannelID = pTxPkt->TxPacketData.ChannelID;
      pRxPkt->RxPacketData.MCS = pTxPkt->TxPacketData.MCS;
      pRxPkt->RxPacketData.RxPowerA = pTxPkt->TxPacketData.TxPower - 100;
      pRxPkt->RxPacketData.RxPowerB = pTxPkt->TxPacketData.TxPower - 100;
      pRxPkt->RxPacketData.RxNoiseA = -100;
      pRxPkt->RxPacketData.RxNoiseB = -100;
      pRxPkt->RxPacketData.RxTSF = 0xdeadbeefcafebabeULL;
      pRxPkt->RxPacketData.RxFrameLength = pTxPkt->TxPacketData.TxFrameLength;
      memcpy(pRxPkt->RxPacketData.RxFrame, pTxPkt->TxPacketData.TxFrame, RxLen);

      // 'Sniff' this packet on the cw-mon-r[a|b][0|1] monitoring interface
      LLC_MonitorRx(pDev, pRxPkt);

      // Notify the client via the API
      Res = LLC_RxInd(&(pDev->MKx), pRxPkt, pRxPriv);
      if (Res != 0)
        break;

    } while(0);
  }

  // Allocate an 'out' item
  Res = LLC_ListItemAlloc((struct LLCListItem **)&pCtx);
  if (Res != 0)
    goto Error;
  d_assert(pCtx != NULL);

  // Populate the 'tx' bits that we know
  {
    struct MKxIFMsg *pMsg = (struct MKxIFMsg *)(pTxPkt);
    pMsg->Type = MKXIF_TXPACKET;
    pMsg->Len = sizeof(struct MKxTxPacket) + \
                pTxPkt->TxPacketData.TxFrameLength;
    pMsg->Seq = pCtx->Id;
    pMsg->Ret = 0;

    (void)LLC_ListHeadInit(&(pCtx->List));
    pCtx->pBuf = (uint8_t *)pTxPkt;
    pCtx->Len = pMsg->Len;
    pCtx->pPriv = pPriv;
  }

  // Pass it to the underlying transport layer (blocks until sent)
  Res = LLC_USBOutReq(pDev, pCtx);

Error:
  // If all was okay, don't free the context (done in TxCnf)
  // If anything went 'pear shaped': Clean up!
  if ((pCtx != NULL) && (Res != 0))
  {
    Res += pCtx->Ret;
    pCtx->pBuf = NULL;
    pCtx->Len = 0;
    pCtx->pPriv = NULL;
    d_printf(D_WARN, NULL, "Error on LLC_USBOutReq for *pReq(%d): %d\n",
             pCtx->Id, Res);
    if (LLC_ListItemFree((struct LLCListItem *)pCtx) != 0)
    {
      d_printf(D_WARN, NULL, "ListItemFree() != 0\n");
      // Eek! The undelying skb may have already been freed via TxCnf()
      //.. So leak instead of a potential double free
      Res = 0;
    }
  }
  d_fnend(D_TST, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Parse a TxCnf message and pass it to the API
 *
 */
static int LLC_MsgTxCnf (struct LLCDev *pDev,
                         struct LLCInCtx *pCtx,
                         struct MKxIFMsg *pMsg)
{
  int Res = LLC_STATUS_ERROR;
  int Len;
  struct MKxTxEvent *pEvt = (struct MKxTxEvent *)pMsg;
  struct LLCOutCtx *pTxCtx = NULL;
  tMKxTxPacket *pTxPkt = NULL;
  void *pPriv = NULL;
  tMKxStatus Result = LLC_STATUS_ERROR;

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  (void)pCtx; // unused

  Len = pMsg->Len - sizeof(struct MKxIFMsg);
  if (Len != sizeof(struct MKxTxEventData))
  {
    d_printf(D_WARN, NULL, "Len %d != %d)\n",
             Len, sizeof(struct MKxTxEventData));
    goto Error;
  }

  // Try lookup the function parameter values using 'Seq'
  Res = LLC_ListItemPeek((struct LLCListItem **)&pTxCtx, pEvt->Hdr.Seq);
  if (Res != 0)
  {
    d_printf(D_NOTICE, NULL, "LLC_ListItemPeek() = %d\n", Res);
    goto Error;
  }
  // Remember the neccessary LLC_TxCnf() params
  pTxPkt = (tMKxTxPacket *)(pTxCtx->pBuf);
  pPriv = pTxCtx->pPriv;
  // Get the general result from the message header
  Result = pEvt->Hdr.Ret;
  // If that's okay, get the actual transmit result from the message payload
  if (Result == MKXSTATUS_SUCCESS)
    Result = pEvt->TxEventData.TxStatus;

  // Release the context
  pTxCtx->pBuf = NULL;
  pTxCtx->Len = 0;
  pTxCtx->pPriv = NULL;
  if (LLC_ListItemSendToPool((struct LLCListItem *)pTxCtx) != 0)
    d_printf(D_WARN, NULL, "ListItemFree() != 0\n");

  // Call the TxCnf API function
  Res = LLC_TxCnf(&(pDev->MKx), pTxPkt, pPriv, Result);
  // Put pPriv back into the 'Seq' @sa MsgSend()
  pEvt->Hdr.Seq = (uint16_t)((int)pPriv);

Error:
  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Parse a RxPkt message and pass it to the API
 *
 */
static int LLC_MsgRxPkt (struct LLCDev *pDev,
                         struct LLCInCtx *pCtx,
                         struct MKxIFMsg *pMsg)
{
  int Res = LLC_STATUS_ERROR;
  int Len;
  tMKxRxPacket *pRxPkt = (struct MKxRxPacket *)pMsg;
  void *pPriv = pCtx->pPriv;

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  // Check whether we are silent
  if (LLC_IsSilent())
  {
    d_error(D_INFO, NULL, "LLC silent, dropping received frame\n");
    goto Error;
  }

  // Sanity check the packet lengths etc.
  Len = pMsg->Len - sizeof(struct MKxIFMsg);
  if (Len < sizeof(struct MKxRxPacketData))
  {
    d_printf(D_WARN, NULL, "Len %d < %d)\n",
             Len, sizeof(struct MKxRxPacketData));
    goto Error;
  }
  ; // TODO more

  // 'Sniff' this packet on the cw-mon-r[a|b][0|1] monitoring interface
  LLC_MonitorRx(pDev, pRxPkt);

  // Notify the client via the API
  Res = LLC_RxInd(&(pDev->MKx), pRxPkt, pPriv);
  if (Res != 0)
  {
    d_printf(D_WARN, NULL, "LLC_RxInd(%p %p) = %d\n", pRxPkt, pPriv, Res);
  }

  // Always re-allocate the buffer even if the RxInd() above failed
  // (there's no telling what the upper layers have done to it, so don't re-use it)
  pCtx->pBuf = NULL;

Error:
  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Create a CfgReq message and pass it to the underlying transport
 *
 */
int LLC_MsgCfgReq (struct LLCDev *pDev,
                   tMKxRadio Radio,
                   tMKxRadioConfig *pConfig)
{
  int Res = LLC_STATUS_ERROR;
  struct LLCOutCtx *pCtx = NULL;

  d_fnstart(D_TST, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  // Allocate a 'out' item
  Res = LLC_ListItemAlloc((struct LLCListItem **)&pCtx);
  if (Res != 0)
    goto Error;
  d_assert(pCtx != NULL);

  // Populate the 'out' bits that we know
  {
    struct MKxIFMsg *pMsg = (struct MKxIFMsg *)(pConfig);//只分析前面那部分包头信息

    switch (Radio)
    {
      case MKX_RADIO_A:
        pMsg->Type = MKXIF_RADIOACFG;
        break;
      case MKX_RADIO_B:
        pMsg->Type = MKXIF_RADIOBCFG;
        break;
      default:
        Res = MKXSTATUS_FAILURE_INVALID_PARAM;
        goto Error;
        break;
    }
    pMsg->Len = sizeof(struct MKxRadioConfig);
    pMsg->Seq = pCtx->Id;
    pMsg->Ret = 0;

    (void)LLC_ListHeadInit(&(pCtx->List));
    pCtx->pBuf = (uint8_t *)pConfig;
    pCtx->Len = pMsg->Len;
  }

    // Pass it to the underlying transport layer (blocks until sent)
  Res = LLC_USBOutReq(pDev, pCtx);

  // Optimistically apply the requested config to the MKx structure
  // (eventually overwritten by the contents of the reply message)
  // N.B. If this is not present then dual radio tests will fail
  if (1)
  {
    memcpy((void *)&(pDev->MKx.Config.Radio[Radio]),
           &(pConfig->RadioConfigData),
           sizeof(struct MKxRadioConfigData));
  }

Error:
  // Clean up!
  if (pCtx != NULL)
  {
    Res += pCtx->Ret;
    pCtx->pBuf = NULL;
    pCtx->Len = 0;
    pCtx->pPriv = NULL;
    if (LLC_ListItemFree((struct LLCListItem *)pCtx) != 0)
      d_printf(D_WARN, NULL, "ListItemFree() != 0\n");
  }
  d_fnend(D_TST, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Parse a CfgInd message and pass it to the API
 *
 */
static int LLC_MsgCfgInd (struct LLCDev *pDev,
                          struct LLCInCtx *pCtx,
                          struct MKxIFMsg *pMsg)
{
  int Res = LLC_STATUS_ERROR;
  int Len;
  tMKxNotif Notif = MKX_NOTIF_MASK_RADIOA;
  tMKxRadio Radio = MKX_RADIO_A;

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);

  (void)pCtx; // unused

  Len = pMsg->Len - sizeof(struct MKxIFMsg);
  if (Len != sizeof(struct MKxRadioConfigData))
  {
    d_printf(D_WARN, NULL, "Len %d != %d)\n",
             Len, sizeof(struct MKxRadioConfigData));
    goto Error;
  }

  if (pMsg->Type == MKXIF_RADIOBCFG)
  {
    Notif = MKX_NOTIF_MASK_RADIOB;
    Radio = MKX_RADIO_B;
  }

  { // Collect the config into the MKx structure
    tMKxRadioConfig *pRadioConfig = (tMKxRadioConfig *)pMsg;
    memcpy((void *)&(pDev->MKx.Config.Radio[Radio]),
           &(pRadioConfig->RadioConfigData),
           sizeof(struct MKxRadioConfigData));
  }
  // Set the notification
  if (pMsg->Ret != 0)
  {
    d_printf(D_ERR, NULL, "ERROR, Radio %c Config Failed\n", 'A'+Radio);
    Notif |= MKX_NOTIF_ERROR;
  }
  Res = LLC_NotifInd(&(pDev->MKx), Notif);

Error:
  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}


/**
 * @brief Create a DbgReq message and pass it to the underlying transport
 *
 */
int LLC_MsgDbgReq (struct LLCDev *pDev,
                   struct MKxIFMsg *pMsg)
{
  int Res = LLC_STATUS_ERROR;
  struct LLCOutCtx *pCtx = NULL;

  d_fnstart(D_TST, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  // Allocate a 'out' item
  Res = LLC_ListItemAlloc((struct LLCListItem **)&pCtx);
  if (Res != 0)
  { // Eek! Use the static (emergency backup) item instead
    Res = LLC_ListItemStatic((struct LLCListItem **)&pCtx);
    if (Res != 0)
      goto Error;
  }
  d_assert(pCtx != NULL);

  // Init the header and populate the context bits that we know
  {
    switch (pMsg->Type)
    {
      case MKXIF_DEBUG:
      case MKXIF_CAL:
        break;

      default:
        pMsg->Type = MKXIF_DEBUG;
        break;
    }
    pMsg->Seq = pCtx->Id;
    pMsg->Ret = 0;

    d_printf(D_DBG, NULL, "Msg: Type:%d Len:%d Seq:%d Ret:%d\n",
             pMsg->Type, pMsg->Len, pMsg->Seq, pMsg->Ret);

    (void)LLC_ListHeadInit(&(pCtx->List));
    pCtx->pBuf = (uint8_t *)pMsg;
    pCtx->Len = pMsg->Len;
  }

  // Pass it to the underlying transport layer (blocks until sent)
  Res = LLC_USBOutReq(pDev, pCtx);

Error:
  // Clean up!
  if (pCtx != NULL)
  {
    Res += pCtx->Ret;
    pCtx->pBuf = NULL;
    pCtx->Len = 0;
    pCtx->pPriv = NULL;
    if (LLC_ListItemFree((struct LLCListItem *)pCtx) != 0)
      d_printf(D_WARN, NULL, "ListItemFree() != 0\n");
  }
  d_fnend(D_TST, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Parse a DbgInd message and pass it to the API
 *
 */
static int LLC_MsgDbgInd (struct LLCDev *pDev,
                          struct LLCInCtx *pCtx,
                          struct MKxIFMsg *pMsg)
{
  int Res = LLC_STATUS_ERROR;
  int Len;

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);
  d_assert(pCtx != NULL);
  d_assert(pMsg != NULL);

  (void)pCtx; // unused

  // Sanity check the packet lengths etc.
  Len = pMsg->Len - sizeof(struct MKxIFMsg);
  if (Len <= 0)
  {
    d_printf(D_WARN, NULL, "Len %d\n", Len);
    goto Error;
  }

  // Call the DbgInd API function
  Res = LLC_DebugInd(&(pDev->MKx), pMsg);

Error:
  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Create a C2X security command message and pass it to the underlying transport
 *
 */
int LLC_MsgSecReq (struct LLCDev *pDev,
                   tMKxC2XSec *pMsg)
{
  int Res = LLC_STATUS_ERROR;
  struct LLCOutCtx *pCtx = NULL;

  d_fnstart(D_TST, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  // Allocate a 'out' item
  Res = LLC_ListItemAlloc((struct LLCListItem **)&pCtx);
  if (Res != 0)
    goto Error;
  d_assert(pCtx != NULL);

  // Init the header and populate the context bits that we know
  {
    pMsg->Hdr.Type = MKXIF_C2XSEC;
    pMsg->Hdr.Seq = pCtx->Id;
    pMsg->Hdr.Ret = 0;

    d_printf(D_DBG, NULL, "Msg: Type:%d Len:%d Seq:%d Ret:%d\n",
             pMsg->Hdr.Type, pMsg->Hdr.Len, pMsg->Hdr.Seq, pMsg->Hdr.Ret);

    (void)LLC_ListHeadInit(&(pCtx->List));
    pCtx->pBuf = (uint8_t *)pMsg;
    pCtx->Len = pMsg->Hdr.Len;
  }

  // Pass it to the underlying transport layer (blocks until sent)
  Res = LLC_USBOutReq(pDev, pCtx);

Error:
  // Clean up!
  if (pCtx != NULL)
  {
    Res += pCtx->Ret;
    pCtx->pBuf = NULL;
    pCtx->Len = 0;
    pCtx->pPriv = NULL;
    if (LLC_ListItemFree((struct LLCListItem *)pCtx) != 0)
      d_printf(D_WARN, NULL, "ListItemFree() != 0\n");
  }
  d_fnend(D_TST, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Parse a C2X security response message and pass it to the API
 *
 */
static int LLC_MsgSecInd (struct LLCDev *pDev,
                          struct LLCInCtx *pCtx,
                          struct MKxIFMsg *pMsg)
{
  int Res = LLC_STATUS_ERROR;
  int Len;

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);
  d_assert(pCtx != NULL);
  d_assert(pMsg != NULL);

  (void)pCtx; // unused

  // Sanity check the packet lengths etc.
  Len = pMsg->Len - sizeof(struct MKxIFMsg);
  if (Len <= 0)
  {
    d_printf(D_WARN, NULL, "Len %d\n", Len);
    goto Error;
  }

  // Call the C2XSecInd API function
  Res = LLC_C2XSecInd(&(pDev->MKx), (tMKxC2XSec *)pMsg);

Error:
  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}


/**
 * @brief Parse a DbgInd message and pass it to the API
 *
 */
static int LLC_MsgStatsInd (struct LLCDev *pDev,
                            struct LLCInCtx *pCtx,
                            struct MKxIFMsg *pMsg)
{
  int Res = LLC_STATUS_ERROR;
  int Len;
  tMKxNotif Notif = MKX_NOTIF_NONE;
  tMKxRadio Radio = MKX_RADIO_A;

  d_fnstart(D_VERBOSE, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);
  d_assert(pCtx != NULL);
  d_assert(pMsg != NULL);

  (void)pCtx; // unused

  // Sanity check the packet lengths etc.
  Len = pMsg->Len - sizeof(struct MKxIFMsg);
  if (Len < sizeof(tMKxRadioStatsData))
  {
    d_printf(D_WARN, NULL, "Len %d\n", Len);
    goto Error;
  }

  // Collect the stats into the MKx structure
  if (pMsg->Type == MKXIF_RADIOASTATS)
    Radio = MKX_RADIO_A;
  if (pMsg->Type == MKXIF_RADIOBSTATS)
    Radio = MKX_RADIO_B;
  memcpy((void *)&(pDev->MKx.State.Stats[Radio].RadioStatsData),
         (struct tMKxRadioStatsData *)(pMsg->Data),
         sizeof(tMKxRadioStatsData));

  // Send the 'stats' notification
  Notif = MKX_NOTIF_MASK_STATS;
  if (Radio == MKX_RADIO_A)
    Notif |= MKX_NOTIF_MASK_RADIOA;
  if (Radio == MKX_RADIO_B)
    Notif |= MKX_NOTIF_MASK_RADIOB;
  if (pMsg->Seq == 0)
    Notif |= MKX_NOTIF_MASK_CHANNEL0;
  else
    Notif |= MKX_NOTIF_MASK_CHANNEL1;
  if (pMsg->Ret != 0)
    Notif |= MKX_NOTIF_ERROR;
  Res = LLC_NotifInd(&(pDev->MKx), Notif);

  // Send the 'active' notification
  Notif = MKX_NOTIF_MASK_ACTIVE;
  if (Radio == MKX_RADIO_A)
    Notif |= MKX_NOTIF_MASK_RADIOA;
  if (Radio == MKX_RADIO_B)
    Notif |= MKX_NOTIF_MASK_RADIOB;
  if (pMsg->Seq == 0)
  { // X0 just ended
    if (pDev->MKx.Config.Radio[Radio].Mode == MKX_MODE_OFF)
      Notif = MKX_NOTIF_NONE; // Nothing restarted
    else if (pDev->MKx.Config.Radio[Radio].Mode == MKX_MODE_CHANNEL_0)
      Notif |= MKX_NOTIF_MASK_CHANNEL0; // X0 just restarted
    else if (pDev->MKx.Config.Radio[Radio].Mode == MKX_MODE_SWITCHED)
      Notif |= MKX_NOTIF_MASK_CHANNEL1; // X1 just restarted
    else
      d_printf(D_WARN, NULL, "Stats %c0 event, but Mode is %d\n",
               Radio ? 'A':'B', pDev->MKx.Config.Radio[Radio].Mode);
  }
  else
  { // X1 just ended
    if (pDev->MKx.Config.Radio[Radio].Mode == MKX_MODE_OFF)
      Notif = MKX_NOTIF_NONE; // Nothing restarted
    else if (pDev->MKx.Config.Radio[Radio].Mode == MKX_MODE_CHANNEL_1)
      Notif |= MKX_NOTIF_MASK_CHANNEL1; // X1 just restarted
    else if (pDev->MKx.Config.Radio[Radio].Mode == MKX_MODE_SWITCHED)
      Notif |= MKX_NOTIF_MASK_CHANNEL0; // X0 just restarted
    else
      d_printf(D_WARN, NULL, "Stats %c1 event, but Mode is %d\n",
	       Radio ? 'A':'B', pDev->MKx.Config.Radio[Radio].Mode);
   }
   if (Notif != MKX_NOTIF_NONE) // Don't notify for nothing
     Res = LLC_NotifInd(&(pDev->MKx), Notif);

Error:
  d_fnend(D_VERBOSE, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Create a SET_TSF message and pass it to the underlying transport
 *
 */
int LLC_MsgSetTSFReq (struct LLCDev *pDev,
                      tMKxTSF TSF)
{
  int Res = LLC_STATUS_ERROR;
  struct LLCOutCtx *pCtx = NULL;

  d_fnstart(D_TST, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  // Allocate a 'out' item
  Res = LLC_ListItemAlloc((struct LLCListItem **)&pCtx);
  if (Res != 0)
    goto Error;
  d_assert(pCtx != NULL);

  // Init the header and populate the context bits that we know
  {
    uint8_t Msg[sizeof(tMKxIFMsg) + sizeof(tMKxTSF)];
    struct MKxIFMsg *pMsg = (struct MKxIFMsg *)(Msg);

    memset(Msg, 0, sizeof(Msg));
    pMsg->Type = MKXIF_SET_TSF;
    pMsg->Len = sizeof(struct MKxIFMsg) + sizeof(tMKxTSF);
    pMsg->Seq = pCtx->Id;
    pMsg->Ret = 0;
    memcpy(pMsg->Data, &TSF, sizeof(tMKxTSF));
    d_printf(D_DBG, NULL, "Msg: Type:%d Len:%d Seq:%d Ret:%d\n",
             pMsg->Type, pMsg->Len, pMsg->Seq, pMsg->Ret);

    (void)LLC_ListHeadInit(&(pCtx->List));
    pCtx->pBuf = (uint8_t *)pMsg;
    pCtx->Len = pMsg->Len;
  }

  // Pass it to the underlying transport layer (blocks until sent)
  Res = LLC_USBOutReq(pDev, pCtx);

Error:
  // Clean up!
  if (pCtx != NULL)
  {
    pCtx->pBuf = NULL;
    pCtx->Len = 0;
    pCtx->pPriv = NULL;
    if (LLC_ListItemFree((struct LLCListItem *)pCtx) != 0)
      d_printf(D_WARN, NULL, "ListItemFree() != 0\n");
  }
  d_fnend(D_TST, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @}
 * @}
 */

