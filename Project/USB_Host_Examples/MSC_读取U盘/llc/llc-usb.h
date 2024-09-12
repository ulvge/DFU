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
 * @file
 * LLC: Linux USB subsystem interfacing functionality
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __DRIVERS__COHDA__LLC__LLC_USB_H__
#define __DRIVERS__COHDA__LLC__LLC_USB_H__
#ifdef __KERNEL__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <linux/types.h>
#include <linux/usb.h>
#include <linux/workqueue.h>

#include "llc-msg.h"
#include "llc-internal.h"

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

// USB status values
#define LLC_USB_STATE_DISABLED  (0x00000000)
#define LLC_USB_STATE_ENABLED   (0x00000001)

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

// Forward decls
struct kref;
struct LLCDev;

/**
 * @section cohda_usb_queue USB queueing
 *
 * @subsection cohda_usb_send Sending USB urbs
 * Just request urbs when the higher layers make an API call
 *
 * @subsection cohda_usb_recv Receiving USB urbs
 * The driver maintains a queue/list of allocated urbs for incoming bulk
 * messages (allocated with @ref RxAlloc) Non RxInd() packets are re-inserted
 * into the queue.
 */




/// Linux USB subsystem interfacing structure
typedef struct LLCUSB
{
  int32_t State;
  /// Driver instance registered with the Linux USB subsystem
  struct usb_driver Driver;
  /// Driver pointer (non NULL when registered)
  struct usb_driver *pDriver;
  /// The USB device for this instance
  struct usb_device *pDevice;
  /// The interface for this device
  struct usb_interface *pInterface;
  /// ?
  struct kref KnlRef;

  /// The address of the bulk in endpoint
  __u8 BulkInEndpointAddr;
  /// The address of the bulk out endpoint
  __u8 BulkOutEndpointAddr;
  /// Size of the receive buffer
  int BulkInSize;

  /// Incoming transfer queue
  struct LLCList InQ;
  /// Outgoing transfer queue
  struct LLCList OutQ;
  /// Protect OutQ from multiple writers
  spinlock_t OutQLock;

  /// Periodic (self re-scheduling) work in the LLC thread
  struct delayed_work Periodic;

  /// Delay the USB setup long enough for the driver init to complete
  /// @todo Remove once SPI loading is included in this driver
  struct delayed_work DelayedSetup;
} tLLCUSB;

//------------------------------------------------------------------------------
// Function declarations
//------------------------------------------------------------------------------

int LLC_USBSetup (struct LLCDev *pDev);
int LLC_USBRelease (struct LLCDev *pDev);


int LLC_USBOutReq (struct LLCDev *pDev,
                   struct LLCOutCtx *pCtx);

#else
#error This file should not be included from user space.
#endif // #ifdef __KERNEL__
#endif // #ifndef __DRIVERS__COHDA__LLC__LLC_USB_H__

/**
 * @}
 * @}
 * @}
 */
