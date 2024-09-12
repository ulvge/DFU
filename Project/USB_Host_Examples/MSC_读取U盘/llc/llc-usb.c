/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @addtogroup cohda_llc_usb LLC 'USB' functionality
 * @{
 *
 * @section cohda_llc_LLC_USBInterface Interfacing with the Linux USB subsystem
 *
 * @file
  * LLC: Linux USB subsystem interfacing functionality
*/

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <linux/usb.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <net/sock.h> // for gfp_any()

#include "llc-internal.h"
#include "llc-usb.h"

#define D_SUBMODULE LLC_USB
#include "debug-levels.h"

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

#define MKX_USB_VENDOR_ID      (0x1fc9)
#define MKX_USB_PRODUCT_ID     (0x0103)

#define LLC_USB_JIFFIES_TIMEOUT_100_USEC   (usecs_to_jiffies(100))
#define LLC_USB_JIFFIES_TIMEOUT_1_MSEC     (msecs_to_jiffies(1))
#define LLC_USB_JIFFIES_TIMEOUT_10_MSEC    (msecs_to_jiffies(10))
#define LLC_USB_JIFFIES_TIMEOUT_100_MSEC   (msecs_to_jiffies(100))
#define LLC_USB_JIFFIES_TIMEOUT_DEFAULT    (LLC_USB_JIFFIES_TIMEOUT_1_MSEC)

#define LLC_USB_CTRL_ENDPOINT (0)
#define LLC_USB_CTRL_MAX_LEN (64)

// USB parameters specificed by NXP for its USB on the SAF5100 chip
// Value field values required for transfers are located in TODO.h
#define LLC_USB_CTRL_OUT_REQUEST_TYPE 0x40
#define LLC_USB_CTRL_IN_REQUEST_TYPE  0xC0
#define LLC_USB_CTRL_OUT_REQUEST      0x01
#define LLC_USB_CTRL_IN_REQUEST       0x02

/// Number of buffers in the USB InQ
#define LLC_USB_INQ_LEN (8)

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

// Table of devices that work with this driver
static struct usb_device_id MKxIdTable[] =
{
  { USB_DEVICE(MKX_USB_VENDOR_ID, MKX_USB_PRODUCT_ID) },
  { } // Terminating entry
};
MODULE_DEVICE_TABLE(usb, MKxIdTable);

//------------------------------------------------------------------------------
// Function prototypes
//------------------------------------------------------------------------------
static int LLC_USBInSubmit (struct LLCDev *pDev, struct LLCInCtx *pCtx);

//------------------------------------------------------------------------------
// Functions: USB transfers
//------------------------------------------------------------------------------

/**
 * @brief
 */
static void LLC_USBOutComplete(struct LLCDev *pDev, struct LLCOutCtx *pCtx)
{
  d_fnstart(D_TST, NULL, "(pDev %p pCtx %p)\n", pDev, pCtx);
  d_assert(pDev != NULL);
  d_assert(pCtx != NULL);

  // If set, cleanup the USB urb
  if (pCtx->pBulkUrb != NULL)
  {
    // NULL out the pointer so we don't double free on the original Tx path if
    // we race on SemUp() vs SemDown() timeout
    struct urb *pUrb = pCtx->pBulkUrb;
    pCtx->pBulkUrb = NULL;//将申请的内存 和 pBulkUrb结构指针 取消关联
    usb_free_urb(pUrb);//"试着"在内核中释放此内存
  }

  // Peek at the message type and do message specific stuff
  if (pCtx->pBuf != NULL)
  {
    uint16_t MsgType = ((struct MKxIFMsg *)(pCtx->pBuf))->Type;

    // Do message specific stuff
    switch (MsgType)
    {
      case MKXIF_TXPACKET:
        if (pCtx->Ret != 0)
        {
          (void)LLC_TxCnf(&(pDev->MKx),
                          (struct MKxTxPacket *)(pCtx->pBuf),
                          pCtx->pPriv,
                          MKXSTATUS_FAILURE_INTERNAL_ERROR);
        }
        break;
      case MKXIF_SET_TSF:
      case MKXIF_RADIOACFG:
      case MKXIF_RADIOBCFG:
      case MKXIF_FLUSHQ:
      case MKXIF_DEBUG:
      case MKXIF_CAL:
      case MKXIF_C2XSEC:
        ; // TODO... maybe
        break;
      case MKXIF_RXPACKET:
      case MKXIF_TXEVENT:
      case MKXIF_RADIOASTATS:
      case MKXIF_RADIOBSTATS:
      default:
        d_printf(D_ERR, NULL, "Corrupted *Req(%d) buffer: Unknown MsgType: %d\n",
                 pCtx->Id, MsgType);
        if (pCtx->pBuf != NULL)
          d_dump(D_ERR, NULL, pCtx->pBuf, sizeof(struct MKxIFMsg));
        break;
    }
  }

  // Set the semaphore to unblock the API call since we are safely not
  // accessing pCtx anymore
  LLC_SemUp(&(pCtx->Semaphore));

  d_fnend(D_TST, NULL, "() = void\n");
  return;
}

/**
 * @brief Submit the next buffer in the OutQ for USB transmission
 */
static int LLC_USBOutSubmitNext(struct LLCDev *pDev)
{
  int Res = -ENOSYS;
  struct LLCOutCtx *pNext = NULL;

  d_fnstart(D_TST, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  spin_lock(&(pDev->USB.OutQLock));
  // If the OutQ is empty: We're done!
  if (LLC_ListEmpty((struct LLCList *)&(pDev->USB.OutQ)))
  {
    spin_unlock(&(pDev->USB.OutQLock));
    d_printf(D_TST, NULL, "LLC_ListEmpty() = true\n");
    Res = 0;
    goto Success;
  }

  // Get the next entry from the head of the OutQ
  Res = LLC_ListItemGet((struct LLCListItem **)&pNext,
                        (struct LLCList *)&(pDev->USB.OutQ));
  spin_unlock(&(pDev->USB.OutQLock));
  if (Res != 0)
  {
    d_printf(D_WARN, NULL, "LLC_ListItemGet() = %d\n", Res);
    goto Error;
  }
  d_assert(pNext != NULL);

  // Submit the URB
  if (pNext->pBulkUrb == NULL)
  {
    d_printf(D_WARN, NULL, "pNext->pBulkUrb = %p\n", pNext->pBulkUrb);
    goto Error;
  }
  Res = usb_submit_urb(pNext->pBulkUrb, gfp_any());
  if (Res != 0)
  {
    d_printf(D_WARN, NULL, "usb_submit_urb() = %d\n", Res);
    goto Error;
  }
  goto Success;

Error:
  if (pNext != NULL)
  {
    // Failure notifications
    pNext->Ret = Res;
    LLC_USBOutComplete(pDev, pNext);
  }
Success:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return Res;
}


/**
 * @brief Part 4 of 4: USB *Req completion callback (thread context)
 */
static void LLC_USBOutReqWorkCallback(struct work_struct *pWork)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;
  struct LLCOutCtx *pCtx = NULL;

  d_fnstart(D_TST, NULL, "(pWork %p)\n", pWork);
  d_assert(pWork != NULL);

  Res = LLC_DevicePointerCheck((void *)pWork);
  if (Res != 0)
  {
    d_printf(D_WARN, NULL, "pWork %p\n", pWork);
    goto Error;
  }

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  d_assert(pDev != NULL);

  pCtx = container_of(pWork, struct LLCOutCtx, BulkWork);
  d_printf(D_DEBUG, NULL, "pCtx->Id %d\n", pCtx->Id);

  // Result notifications (if any)
  pCtx->Ret = 0;
  LLC_USBOutComplete(pDev, pCtx);

  // Optionally calculate the elapsed time
  if (d_test(D_TST))
  {
    pCtx->After = current_kernel_time();
    d_printf(D_TST, NULL, "*Req(%d) elapsed %ld nsec\n",
             pCtx->Id, (pCtx->After.tv_nsec - pCtx->Before.tv_nsec));
  }

  // Send the next buffer in the OutQ (if there is one)
  Res = LLC_USBOutSubmitNext(pDev);
  if (Res != 0)
  {
    d_printf(D_WARN, NULL, "LLC_USBOutSubmitNext() = %d\n", Res);
    goto Error;
  }

Error:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return;
}

/**
 * @brief Part 3 of 4: USB *Req bulk completion callback (IRQ context)
 */
static void LLC_USBOutReqBulkCallback(struct urb *pUrb)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;
  struct LLCOutCtx *pCtx = NULL;

  d_fnstart(D_TST, NULL, "()\n");

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  d_assert(pDev != NULL);

  pCtx = pUrb->context;
  if (pCtx == NULL)
  {
    d_printf(D_WARN, NULL, "pUrb->context == NULL\n");
    goto Error;
  }
  Res = LLC_DevicePointerCheck((void *)pCtx);
  if (Res != 0)
  {
    d_printf(D_WARN, NULL, "pCtx %p\n", pCtx);
    goto Error;
  }
  if (pCtx->pBulkUrb != pUrb)
  {
    d_printf(D_WARN, NULL, "pCtx->pBulkUrb %p != pUrb %p\n",
             pCtx->pBulkUrb, pUrb);
  }

  Res = -EFAULT;
  d_printf(D_DBG, NULL, "pUrb->status %d\n", pUrb->status);
  if ((pUrb->status == 0) ||
      (pUrb->status &&
       !(pUrb->status == -ENOENT ||
         pUrb->status == -ECONNRESET ||
         pUrb->status == -ESHUTDOWN)))
  {
    // USB link faults aren't considered to be errors
    Res = 0;
    if (pUrb->transfer_buffer_length != pUrb->actual_length)
      Res = -EAGAIN;
  }

  if (Res != 0)
    d_printf(D_WARN, NULL, "URB failed %d\n", pUrb->status);
  pCtx->Ret = Res;

  // The URB is complete, but still referenced.
  // It will be released in LLC_USBOutComplete()

  // Do the rest in the kernel thread
  Res = queue_work(pDev->Thread.pQueue, &(pCtx->BulkWork));
  if (Res == 0)
    d_printf(D_WARN, NULL, "queue_work() = %d\n", Res);

Error:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return;
}

/**
 * @brief Part 2 of 4: Submit the URB (thread context)
 */
void LLC_USBOutReqSubmit(struct work_struct *pWork)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;

  d_fnstart(D_TST, NULL, "()\n");

  // Get a handle to the overarching device
  pDev = LLC_DeviceGet();
  d_assert(pDev != NULL);

  // Send the next buffer in the OutQ
  Res = LLC_USBOutSubmitNext(pDev);
  if (Res != 0)
  {
    d_printf(D_WARN, NULL, "LLC_USBOutSubmitNext() = %d\n", Res);
  }

  d_fnend(D_TST, NULL, "() = void\n");
}

/**
 * @brief Part 1 of 4: Queue the *Req message for sending (client context)
 */
int LLC_USBOutReq(struct LLCDev *pDev,
                   struct LLCOutCtx *pCtx)
{
  int Res = -ENOSYS;

  d_fnstart(D_TST, NULL, "()\n");
  if (pDev->USB.pDevice == NULL)
  {
    d_printf(D_WARN, NULL, "USB device not present\n");
    Res = -ENODEV;
    goto Error;
  }

  if (d_test(D_TST))
    pCtx->Before = current_kernel_time();

  Res = -ENOMEM;

  // Create a bulk urb
  pCtx->pBulkUrb = usb_alloc_urb(0, gfp_any());
  if (pCtx->pBulkUrb == NULL)
    goto Error;

  // Fill the bulk urb
  usb_fill_bulk_urb(pCtx->pBulkUrb,
                    pDev->USB.pDevice,
                    usb_sndbulkpipe(pDev->USB.pDevice,
                                    pDev->USB.BulkOutEndpointAddr),
                    pCtx->pBuf,
                    pCtx->Len,
                    LLC_USBOutReqBulkCallback,
                    pCtx);
  pCtx->pBulkUrb->transfer_flags |= URB_ZERO_PACKET;
  //@ todo Why is this still here?
  //pCtx->pBulkUrb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

  // Sanity check that the work structures aren't in use elsewhere
  if (work_pending(&(pCtx->BulkWork)))
  {
    d_printf(D_WARN, NULL, "work_pending(BulkWork)\n");
    goto Error;
  }
  if (work_pending(&(pCtx->Work)))
  {
    d_printf(D_WARN, NULL, "work_pending(Work)\n");
    goto Error;
  }

  INIT_WORK(&(pCtx->BulkWork), LLC_USBOutReqWorkCallback);//pCtx->BulkWork->func=LLC_USBOutReqWorkCallback;

  // Initialize a semaphore (released once the buffer is sent)
  LLC_SemInit(&(pCtx->Semaphore), 0);

  spin_lock(&(pDev->USB.OutQLock));
  // Add the item to the tail of the pending OutQ
  Res = LLC_ListItemAddTail((struct LLCListItem *)pCtx,
                            (struct LLCList *)&(pDev->USB.OutQ));
  spin_unlock(&(pDev->USB.OutQLock));
  if (Res != 0)
    goto Error;

  // Do the rest in the kernel thread
  INIT_WORK(&(pCtx->Work), LLC_USBOutReqSubmit);
  Res = queue_work(pDev->Thread.pQueue, &(pCtx->Work));
  if (Res == 0)
    d_printf(D_WARN, NULL, "queue_work() = %d\n", Res);

  // Wait for the semaphore (released once the buffer is sent)
  if (LLC_SemDown(&(pCtx->Semaphore)))
  {
    d_printf(D_WARN, NULL, "down_timeout()\n");
    if (pCtx->pBulkUrb != NULL)
    {
      // As per drivers/usb/core/urb.c:
      // "The URB must not be deallocated while this routine is running"
      // So set it to NULL (prevents the usb_free_urb() from running)
      // and then kill it!
      struct urb *pUrb = pCtx->pBulkUrb;
      pCtx->pBulkUrb = NULL;
      // cancel transfer request and wait for it to finish
      usb_kill_urb(pUrb);
      // make sure we don't leak memory
      usb_free_urb(pUrb);
    }
    Res = -EAGAIN;
    goto Error;
  }
  Res = pCtx->Ret;
  goto Success;

Error:
  if (pCtx->pBulkUrb != NULL)
  {
    usb_free_urb(pCtx->pBulkUrb);
    pCtx->pBulkUrb = NULL;
  }
Success:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return Res;
}


/**
 * @brief Part 3 of 3: USB receive callback (thread context)
 *
 */
static void LLC_USBInWorkCallback(struct work_struct *pWork)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;
  struct LLCInCtx *pCtx = NULL;

  d_fnstart(D_TST, NULL, "(pWork %p)\n", pWork);
  d_assert(pWork != NULL);

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  d_assert(pDev != NULL);

  pCtx = container_of(pWork, struct LLCInCtx, BulkWork);
  d_printf(D_DEBUG, NULL, "pCtx->Id %d\n", pCtx->Id);

  if (pCtx->Ret == 0)
  {
    // Process the message
    Res = LLC_MsgRecv(pDev, pCtx);
    if (Res != 0)
      d_printf(D_WARN, NULL, "LLC_MsgRecv()= %d\n", Res);
  }
  else
  {
    d_printf(D_WARN, NULL, "urb status %d\n", pCtx->Ret);
  }
  // This urb is complete: remove it from the context
  usb_free_urb(pCtx->pBulkUrb);
  pCtx->pBulkUrb = NULL;

  // Resubmit the buffer for the next message
  Res = LLC_USBInSubmit(pDev, pCtx);
  if (Res != 0)
    d_printf(D_INFO, NULL, "LLC_USBInSubmit()= %d\n", Res);

  d_fnend(D_TST, NULL, "(pWork %p) = void\n", pWork);
}

/**
 * @brief Part 2 of 3: USB receive callback (IRQ context)
 */
static void LLC_USBInUrbCallback(struct urb *pUrb)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;
  struct LLCInCtx *pCtx = NULL;

  d_fnstart(D_TST, NULL, "()\n");

  d_assert(pUrb != NULL);
  d_printf(D_DBG, NULL, "pUrb status %d length %d\n",
           pUrb->status, pUrb->actual_length);

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  d_assert(pDev != NULL);

  if ((pUrb->status != 0) && (pDev->USB.State == LLC_USB_STATE_ENABLED))
    d_printf(D_WARN, NULL, "URB failed %d\n", pUrb->status);

  pCtx = pUrb->context;
  if (pCtx == NULL)
  {
    d_printf(D_WARN, NULL, "pUrb->context == NULL\n");
    goto Error;
  }
  if (pCtx->pBulkUrb != pUrb)
  {
    d_printf(D_WARN, NULL, "pCtx->pBulkUrb %p != pUrb %p\n",
             pCtx->pBulkUrb, pUrb);
  }
  pCtx->Ret = pUrb->status;

  // Do the rest in the kernel thread
  Res = queue_work(pDev->Thread.pQueue, &(pCtx->BulkWork));//LLC_USBInWorkCallback
  if (Res == 0)
    d_printf(D_WARN, NULL, "queue_work() = %d\n", Res);

Error:
  d_fnend(D_TST, NULL, "() = %d\n", Res);
  return;
}

/**
 * @brief Part 1 of 3: (Re)submit the buffer for USB reception
 */
static int LLC_USBInSubmit(struct LLCDev *pDev, struct LLCInCtx *pCtx)
{
  int Res = 0;

  if (pDev->USB.State != LLC_USB_STATE_ENABLED)
    goto Error;

  // Do we need to re-alloc the buffer?
  if (pCtx->pBuf == NULL)
  {
    pCtx->Len = 4096; //pDev->USB.BulkInSize;
    Res = LLC_RxAlloc(&(pDev->MKx), pCtx->Len,
                      &(pCtx->pBuf), &(pCtx->pPriv));
    if (Res != 0)
    {
      // The periodic USB processing will walk the InQ to re-alloc
      d_printf(D_WARN, NULL, "Failed RxAlloc! %d\n", Res);
      goto Error;
    }
  }

  // Do we need to re-alloc and re-submint the urb?
  if (pCtx->pBulkUrb == NULL)
  {
    // Create a bulk urb
    pCtx->pBulkUrb = usb_alloc_urb(0, gfp_any());
    if (pCtx->pBulkUrb == NULL)
      goto Error;

    // Fill the bulk urb
    usb_fill_bulk_urb(pCtx->pBulkUrb,
                      pDev->USB.pDevice,
                      usb_rcvbulkpipe(pDev->USB.pDevice,
                                      pDev->USB.BulkInEndpointAddr),
                      pCtx->pBuf,
                      pCtx->Len,
                      LLC_USBInUrbCallback,
                      pCtx);
    //pCtx->pBulkUrb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    Res = usb_submit_urb(pCtx->pBulkUrb, gfp_any());
    if (Res != 0)
    {
      // The periodic USB processing will walk the InQ to re-submit
      d_printf(D_WARN, NULL, "usb_submit_urb(Rx)= %d\n", Res);
      usb_free_urb(pCtx->pBulkUrb);
      pCtx->pBulkUrb = NULL;
      goto Error;
    }
  }

Error:
  return Res;
}

/**
 * @brief Periodic workqueue handler (runs in the cw-llc thread context)
 * @param pWork The work structure that was scheduled by schedule_work()
 *
 */
static void LLC_USBPeriodicWorkCallback(struct work_struct *pWork)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;
  struct LLCInCtx *pCursor, *pNext;

  d_fnstart(D_IRQ, NULL, "()\n");

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  if (pDev == NULL)
    goto Error;

  // Initialize the InQ list (only happens on 1st run)
  if (LLC_ListEmpty(&(pDev->USB.InQ)))
  {
    int i;
    for (i = 0; i < LLC_USB_INQ_LEN; i++)
    {
      struct LLCInCtx *pCtx = NULL;
      // Try get an item but skip if it failed
      Res = LLC_ListItemAlloc((struct LLCListItem **)&pCtx);
      if ((Res != 0) || (pCtx == NULL))
        continue;
      // Initialise the item
      pCtx->pBuf = NULL;
      pCtx->Len = 4096; //pDev->USB.BulkInSize;
      pCtx->pPriv = NULL;
      pCtx->pBulkUrb = NULL;
      INIT_WORK(&(pCtx->BulkWork), LLC_USBInWorkCallback);
      // Add the item to the 'in' queue
      Res = LLC_ListItemAdd((struct LLCListItem *)pCtx, &(pDev->USB.InQ));
      if (Res != 0)
      {
        if (LLC_ListItemFree((struct LLCListItem *)pCtx) != 0)
          d_printf(D_WARN, NULL, "ListItemFree() != 0\n");
        continue;
      }
    }
  }

  // Walk the InQ and resubmit any outstanding items
  LLC_ListForEachEntrySafe(pCursor, pNext, &(pDev->USB.InQ), List)
  {
    Res = LLC_USBInSubmit(pDev, pCursor);
    if (Res != 0)
      d_printf(D_INFO, NULL, "LLC_USBInSubmit()= %d\n", Res);
  }
  ;// TODO

Error:
  // Always re-schedule if the USB is connected and the thread is still running
  if ((pDev != NULL) &&
      (pDev->USB.pDevice != NULL) &&
      (pDev->Thread.pQueue != NULL))
  {
    (void)queue_delayed_work(pDev->Thread.pQueue,
                             &(pDev->USB.Periodic),
                             LLC_USB_JIFFIES_TIMEOUT_100_MSEC);
  }
  d_fnend(D_IRQ, NULL, "() = void\n");
  return;
}
//------------------------------------------------------------------------------
// Functions: USB Interface (device registration)
//------------------------------------------------------------------------------

static void LLC_USBInterfaceDelete(struct kref *pKnlRef)
{
  struct LLCDev *pDev = NULL;

  d_fnstart(D_TST, NULL, "()\n");

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  if (pDev == NULL)
    goto Error;

  if (pDev->USB.pDevice != NULL)
    usb_put_dev(pDev->USB.pDevice);
  pDev->USB.pDevice = NULL;

Error:
  d_fnend(D_TST, NULL, "");
  return;
}


static int LLC_USBInterfaceProbe(struct usb_interface *pInterface,
                                  const struct usb_device_id *pId)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;
  struct usb_host_interface *pHostInterface;
  int NbrEndpoints;
  int i;

  d_fnstart(D_DBG, NULL, "(pInterface %p pId %p)\n", pInterface, pId);
  d_assert(pInterface != NULL);
  d_assert(pId != NULL);

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  if (pDev == NULL)
  {
    Res = -ENODEV;
    goto Error;
  }

  d_assert(pDev->USB.pDriver != NULL);
  // Initialize and increment the reference counter
  kref_init(&(pDev->USB.KnlRef));

  // Initialize the 'in' and 'out' queue
  Res = LLC_ListHeadInit(&(pDev->USB.InQ));
  if (Res != 0)
  {
    d_printf(D_WARN, NULL, "LLC_ListHeadInit(InQ) = %d\n", Res);
    goto Error;
  }
  d_printf(D_TST, NULL, "Empty %d Head %p Next %p\n",
           LLC_ListEmpty(&(pDev->USB.InQ)),
           &(pDev->USB.InQ),
           pDev->USB.InQ.List.next);
  spin_lock_init(&pDev->USB.OutQLock);
  Res = LLC_ListHeadInit(&(pDev->USB.OutQ));
  if (Res != 0)
  {
    d_printf(D_WARN, NULL, "LLC_ListHeadInit(OutQ) = %d\n", Res);
    goto Error;
  }

  pDev->USB.pInterface = pInterface;//LLC_USBInterfaceProbe
  pDev->USB.pDevice = usb_get_dev(interface_to_usbdev(pInterface));
  d_printf(D_TST, NULL, "usb_get_dev() = %p\n", pDev->USB.pDevice);

  // Set up the endpoint information:
  //  Use only the first bulk-in and bulk-out endpoints
  pHostInterface = pInterface->cur_altsetting;
  d_printf(D_TST, NULL, "NumEndpoints %d\n",
           pHostInterface->desc.bNumEndpoints);
  NbrEndpoints = pHostInterface->desc.bNumEndpoints;

  for (i = 0; i < NbrEndpoints; i++)
  {
    struct usb_endpoint_descriptor *pEndpoint;
    uint8_t EpDir;
    uint8_t EpType;

    pEndpoint = &(pHostInterface->endpoint[i].desc);
    d_assert(pEndpoint != NULL);

    EpDir  = pEndpoint->bEndpointAddress & USB_DIR_IN;
    EpType = pEndpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
    d_printf(D_DBG, NULL, "Endpoint[%d] in/out?:%d bulk/control?:%d\n",
             i, (int)EpDir, (int)EpType);

    // Check for a 'bulk in' endpoint
    if ((pDev->USB.BulkInEndpointAddr == 0) &&
        (EpDir == USB_DIR_IN) &&
        (EpType == USB_ENDPOINT_XFER_BULK))
    {
      d_printf(D_TST, NULL, "BulkIn: Addr %u Size %d\n",
               pEndpoint->bEndpointAddress, pEndpoint->wMaxPacketSize);
      pDev->USB.BulkInEndpointAddr = pEndpoint->bEndpointAddress;
      pDev->USB.BulkInSize         = pEndpoint->wMaxPacketSize;
    }

    // Check for a 'bulk out' endpoint
    if ((pDev->USB.BulkOutEndpointAddr == 0) &&
        (EpDir != USB_DIR_IN) &&
        (EpType == USB_ENDPOINT_XFER_BULK))
    {
      d_printf(D_TST, NULL, "BulkOut: Addr %u Size %d\n",
               pEndpoint->bEndpointAddress, pEndpoint->wMaxPacketSize);
      pDev->USB.BulkOutEndpointAddr = pEndpoint->bEndpointAddress;
    }
  }

  if ((pDev->USB.BulkInEndpointAddr == 0) ||
      (pDev->USB.BulkOutEndpointAddr == 0))
  {
    d_error(D_ERR, NULL, "Could not find both bulk-in & bulk-out endpoints\n");
    goto Error;
  }

  // Save the data pointer in this interface device
  usb_set_intfdata(pInterface, pDev);

  // Let the user know what node this device is now attached
  d_printf(D_WARN, NULL, "MKx now attached to cw-llc%d", pInterface->minor);

  // Start the periodic processing
  INIT_DELAYED_WORK(&(pDev->USB.Periodic), LLC_USBPeriodicWorkCallback);
  Res = queue_delayed_work(pDev->Thread.pQueue,
                           &(pDev->USB.Periodic), 1);
  if (Res == 0) // 'Res == 0' means that the work is already on the queue... ??
  {
    d_printf(D_WARN, NULL, "queue_delayed_work() = %d\n", Res);
    goto Error;
  }

  ; // TODO
  pDev->USB.State = LLC_USB_STATE_ENABLED;
  Res = 0;
  goto Success;

Error:
  if (pDev != NULL)
    kref_put(&(pDev->USB.KnlRef), LLC_USBInterfaceDelete);

Success:
  d_fnend(D_DBG, NULL, "(pInterface %p pId %p) = %d\n",
          pInterface, pId, Res);
  return Res;
}


static void LLC_USBInterfaceDisconnect(struct usb_interface *pInterface)
{
  struct LLCDev *pDev = NULL;

  d_fnstart(D_DBG, NULL, "(pInterface %p)\n", pInterface);
  d_assert(pInterface != NULL);

  // Get a handle of the overarching device
  pDev = usb_get_intfdata(pInterface);
  if (pDev == NULL)
    goto Error;

  d_assert(pDev == LLC_DeviceGet());
  pDev->USB.State = LLC_USB_STATE_DISABLED;

  // Stop the periodic processing
  (void)cancel_delayed_work_sync(&(pDev->USB.Periodic));

  // Empty the InQ
  if (LLC_ListEmpty(&(pDev->USB.InQ)))
  {
    int Res = -ENOSYS;
    struct LLCInCtx *pCtx = NULL;
    Res = LLC_ListItemGet((struct LLCListItem **)&pCtx, &(pDev->USB.InQ));
    while (Res == 0)
    {
      if (pCtx->pBulkUrb)
      {
        Res = usb_unlink_urb(pCtx->pBulkUrb);
        if (Res != 0)
          d_printf(D_WARN, NULL, "usb_unlink_urb(%p) = %d",
                   pCtx->pBulkUrb, Res);
      }
      // Don't cancel the work as that will flush out via the urb unlink process
      if (0)
        cancel_work_sync(&(pCtx->BulkWork));
      if (LLC_ListItemFree((struct LLCListItem *)pCtx) != 0)
        d_printf(D_WARN, NULL, "ListItemFree() != 0\n");
      Res = LLC_ListItemGet((struct LLCListItem **)&pCtx, &(pDev->USB.InQ));
    }
  }

  usb_set_intfdata(pInterface, NULL);
  d_printf(D_TST, NULL, "MKx USB cw-llc%d now disconnected", pInterface->minor);

Error:
  // decrement our usage count
  if (pDev != NULL)
    kref_put(&(pDev->USB.KnlRef), LLC_USBInterfaceDelete);
  return;
}

//------------------------------------------------------------------------------
// Functions: USB device
//------------------------------------------------------------------------------


/**
 * @brief Setup the device structure's interface processing
 * @param pDev The LLC device pointer
 * @return Zero (LLC_STATUS_SUCCESS) or a negative errno
 */
int LLC_USBDelayedSetup(struct LLCDev *pDev)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_DBG, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  // appears in /sys/bus/usb/drivers
  pDev->USB.Driver.name = "cw-llc",
                          // use this driver for this vendor and product
                          pDev->USB.Driver.id_table = MKxIdTable;
  // function called by kernel if things this driver should get used.
  // If it accepts it then init the driver and return 0
  pDev->USB.Driver.probe = LLC_USBInterfaceProbe;
  pDev->USB.Driver.disconnect = LLC_USBInterfaceDisconnect;

  /// Register this driver with the USB subsystem
  d_assert(pDev->USB.pDriver == NULL);
  pDev->USB.pDriver = &(pDev->USB.Driver);
  Res = usb_register(pDev->USB.pDriver);
  if (Res != 0)
  {
    d_error(D_ERR, NULL, "usb_register() = %d\n", Res);
    pDev->USB.pDriver = NULL;
    goto Error;
  }

  ; // TODO

  Res = LLC_STATUS_SUCCESS;
  goto Success;

Error:
  if (pDev->USB.pDriver != NULL)
  {
    usb_deregister(pDev->USB.pDriver);
    pDev->USB.pDriver = NULL;
  }
Success:
  d_fnend(D_DBG, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

static void LLC_USBDelayedSetupCallback(struct work_struct *pWork)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;

  d_fnstart(D_TST, NULL, "()\n");

  // Get a handle of the overarching device
  pDev = LLC_DeviceGet();
  if (pDev == NULL)
    goto Error;

  Res = LLC_USBDelayedSetup(pDev);

Error:
  return;
}

/**
 * @brief Delay the setup of the USB
 */
int LLC_USBSetup(struct LLCDev *pDev)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_TST, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  pDev->USB.State = LLC_USB_STATE_DISABLED;

  // Setup the delayed processing
  INIT_DELAYED_WORK(&(pDev->USB.DelayedSetup), LLC_USBDelayedSetupCallback);
  Res = queue_delayed_work(pDev->Thread.pQueue,
                           &(pDev->USB.DelayedSetup), msecs_to_jiffies(100));
  if (Res == 0) // 'Res == 0' means that the work is already on the queue... ??
  {
    d_printf(D_WARN, NULL, "queue_delayed_work() = %d\n", Res);
    goto Error;
  }
  Res = 0;

Error:
  d_fnend(D_DBG, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Release (cleanup) the device structure's interface processing
 * @param pDev LLC device pointer
 * @return Zero (LLC_STATUS_SUCCESS) or a negative errno
 */
int LLC_USBRelease(struct LLCDev *pDev)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_DBG, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  pDev->USB.State = LLC_USB_STATE_DISABLED;

  // Deregister this driver with the USB subsystem
  if (pDev->USB.pDriver != NULL)
  {
    usb_deregister(pDev->USB.pDriver);
    pDev->USB.pDriver = NULL;
  }
  Res = LLC_STATUS_SUCCESS;

  d_fnend(D_DBG, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @}
 * @}
 * @}
 */

