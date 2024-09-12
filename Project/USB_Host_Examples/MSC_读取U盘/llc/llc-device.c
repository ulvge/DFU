/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @section cohda_llc_device
 *
 * @file
 * LLC: Device management related functionality
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include "linux/cohda/llc/llc.h"
#include "cohda/llc/llc.h"
#include "llc-internal.h"
#include "llc-device.h"

#define D_SUBMODULE LLC_Device
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

/// Local copy
struct LLCDev *_pDev = NULL;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

/**
 * @brief Setup the LLC device structure
 * @param pDev LLC device pointer
 * @return Zero (LLC_STATUS_SUCCESS) or a negative errno
 */
int LLC_DeviceSetup(struct LLCDev *pDev)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_INTERN, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);
#if 0
  Res = LLC_FirmwareSetup(pDev);
  if (Res != 0)
    goto ErrorFirmware;
#endif   

#if 0
  // Setup the 'cw-llc' netdev
  Res = LLC_NetDevSetup(pDev);//
  if (Res != 0)
    goto ErrorNetDev;
  d_printf(D_DEBUG, NULL, "ifindex %d\n", pDev->pNetDev->ifindex);
#endif    
  Res = LLC_MonitorSetup(pDev);//
  if (Res != 0)
    goto ErrorMonitor; 
    
#if 0
  Res = LLC_ListSetup(pDev);
  if (Res != 0)
    goto ErrorList;
#endif    

#if 0
  Res = LLC_ThreadSetup(pDev);
  if (Res != 0)
    goto ErrorThread;
#endif    

#if 0
  Res = LLC_MsgSetup(pDev);
  if (Res != 0)
    goto ErrorMsg;
#endif    
  Res = LLC_USBSetup(pDev);//LLC_USBDelayedSetup(pDev);
  if (Res != 0)
    goto ErrorUSB;

  // Do this last
  Res = LLC_STATUS_SUCCESS;
  _pDev = pDev;
  goto Success;

  (void)LLC_USBRelease(pDev);
ErrorUSB:
  (void)LLC_MsgRelease(pDev);
ErrorMsg:
  (void)LLC_ThreadRelease(pDev);
ErrorThread:
  (void)LLC_ListRelease(pDev);
ErrorList:
  (void)LLC_MonitorRelease(pDev);
ErrorMonitor:
  (void)LLC_NetDevRelease(pDev);
ErrorNetDev:
  (void)LLC_FirmwareRelease(pDev);
ErrorFirmware:
Success:
  d_fnend(D_INTERN, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Release (cleanup) the LLC device structure
 * @param pDev LLC device pointer
 * @return Zero (LLC_STATUS_SUCCESS) or a negative errno
 */
int LLC_DeviceRelease(struct LLCDev *pDev)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_INTERN, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  if (pDev == _pDev)
  {
    Res = LLC_USBRelease(pDev);
    Res = LLC_MsgRelease(pDev);
    Res = LLC_ThreadRelease(pDev);
    Res = LLC_ListRelease(pDev);
    Res = LLC_MonitorRelease(pDev);
    Res = LLC_NetDevRelease(pDev);
    Res = LLC_FirmwareRelease(pDev);
    _pDev = NULL;
  }

  d_fnend(D_INTERN, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Check the pointer for LLC validity
 * @return LLC internal pointer
 */
int LLC_DevicePointerCheck(void *pPtr)
{
  char *pTmp = (char *)pPtr;

  // Within the bounds of the device structure?
  char *pStart = (char *)_pDev;
  char *pEnd = pStart + sizeof(struct LLCDev);
  if ((pTmp >= pStart) && (pTmp < pEnd))
    return 0;
  // Within the bounds of the list pool?
  pStart = (char *)(_pDev->ListPool.pPool);
  pEnd = (char *)(_pDev->ListPool.pEnd);
  if ((pTmp >= pStart) && (pTmp < pEnd))
    return 0;
  // Not valid!
  return -EINVAL;
}

/**
 * @brief Get the LLC device structure
 * @return LLC device pointer
 */
struct LLCDev *LLC_DeviceGet(void)
{
  return _pDev;
}

/**
 * @brief Morph the API pointer to the internal device strucuture
 * @param pMKx MKx handle
 * @return A handle to the internal device
 */
struct LLCDev *LLC_Device(struct MKx *pMKx)
{
  struct LLCDev *pDev;

  d_assert(pMKx != NULL);

  pDev = container_of(pMKx, struct LLCDev, MKx);

  if (pDev->Magic != LLC_DEV_MAGIC)
    pDev = NULL;

  d_assert(pDev == _pDev);
  return pDev;
}

/**
 * @brief Copy the pMKx contents into the provided buffer
 */
static int LLC_IoctlMKx(struct LLCDev *pDev,
                         struct sk_buff *pSkb)
{
  int Res = -ENOSYS;
  struct MKx *pSrc = NULL;

  d_assert(pDev != NULL);
  pSrc = &(pDev->MKx);

  if (0)
  { // dbg
    uint8_t Dbg[] = { 0xde, 0xad, 0xbe, 0xef };
    memcpy((void *)&(pSrc->State.Stats[MKX_RADIO_A].RadioStatsData.Chan[MKX_CHANNEL_0].ChannelBusyRatio), &(Dbg[0]), 1);
    memcpy((void *)&(pSrc->State.Stats[MKX_RADIO_A].RadioStatsData.Chan[MKX_CHANNEL_1].ChannelBusyRatio), &(Dbg[1]), 1);
    memcpy((void *)&(pSrc->State.Stats[MKX_RADIO_B].RadioStatsData.Chan[MKX_CHANNEL_0].ChannelBusyRatio), &(Dbg[2]), 1);
    memcpy((void *)&(pSrc->State.Stats[MKX_RADIO_B].RadioStatsData.Chan[MKX_CHANNEL_1].ChannelBusyRatio), &(Dbg[3]), 1);
  } // dbg

  { // Copy the pMKx contents into the provided buffer
    struct MKx *pDst = (struct MKx *)(pSkb->data);
    memcpy(pDst, pSrc, sizeof(struct MKx));
    Res = 0;
  }

  return Res;
}

/**
 * @brief Relay an ioctl from user space to the correct LLC_*() function
 * @param pDev 'llc' device descriptor
 * @param pReq pointer to the message bytes
 * @param Cmd passed by the app
 *
 * @return Result code of the called function
 *
 * This function is responsible for copying the message from userspace and
 * calling the relevant LLC API function.
 */
int LLC_Ioctl(struct LLCDev *pDev,
    			           struct ifreq *pReq,  int Cmd)
{
  int Res = -ENOSYS;
  int Len = -1;
  struct sk_buff *pSkb = NULL;

  d_fnstart(D_API, NULL, "(pDev %p, pReq %p, Cmd %x)\n", pDev, pReq, Cmd);

  // Figure out the incoming data length
  switch (Cmd)
  {
    case LLC_IOMSG_MKX:
      Len = sizeof(struct MKx);
      break;
    case LLC_IOMSG_CONFIG:
    case LLC_IOMSG_TXREQ:
    case LLC_IOMSG_TXFLUSH:
    case LLC_IOMSG_SETTSF:
    case LLC_IOMSG_GETTSF:
    case LLC_IOMSG_TXCNF:
    case LLC_IOMSG_RXALLOC:
    case LLC_IOMSG_RXIND:
    case LLC_IOMSG_NOTIFIND:
    case LLC_IOMSG_DEBUG:
    default:
      Len = -1; // TODO
      break;
  }
  if (Len < 0)
  {
    Res = -EINVAL;
    goto Error;
  }

  // Allocate the buffer (use an sk_buff for 'futureproof-ness')
  pSkb = alloc_skb(Len, GFP_KERNEL);
  if (pSkb == NULL)
  {
    Res = -ENOMEM;
    goto Error;
  }
  // reserve some extra message headers (if needed)
  skb_reserve(pSkb, 0);
  // Copy the incoming message from userspace
  Res = copy_from_user(pSkb->data, pReq->ifr_data, Len);
  if (Res != 0)
  {
    Res = -EIO;
    goto Error;
  }
  //d_dump(D_VERBOSE, pDev, pSkb->data, Len);
  skb_put(pSkb, Len);

  switch (Cmd)
  {
    case LLC_IOMSG_MKX:
      Res = LLC_IoctlMKx(pDev, pSkb);
      break;
    case LLC_IOMSG_CONFIG:
    case LLC_IOMSG_TXREQ:
    case LLC_IOMSG_TXFLUSH:
    case LLC_IOMSG_SETTSF:
    case LLC_IOMSG_GETTSF:
    case LLC_IOMSG_TXCNF:
    case LLC_IOMSG_RXALLOC:
    case LLC_IOMSG_RXIND:
    case LLC_IOMSG_NOTIFIND:
    case LLC_IOMSG_DEBUG:
      Res = -ENOSYS; // TODO
      break;
    default:
      Res = -ENOSYS;
      break;
  }
  if (Res < 0)
    goto Error;

  // Copy the response back to userspace
  if (copy_to_user(pReq->ifr_data, pSkb->data, pSkb->len))
  {
    Res = -EFAULT;
    goto Error;
  }

Error:
  kfree_skb(pSkb);
  d_fnend(D_API, NULL, "(pDev %p, pReq %p, Cmd %x) = %d\n",
          pDev, pReq, Cmd, Res);
  return Res;
}


/**
 * @}
 * @}
 */
