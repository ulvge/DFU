/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @file
 * LLC: SAF5100 firmware programing
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __DRIVERS__COHDA__LLC__LLC_FIRMWARE_H__
#define __DRIVERS__COHDA__LLC__LLC_FIRMWARE_H__
#ifdef __KERNEL__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include "llc-internal.h"

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

// Forward decl
struct LLCDev;

/// Firmware download modes
typedef enum LLCFirmwareMode
{
  LLC_FIRMWARE_MODE_NONE,
  LLC_FIRMWARE_MODE_BITBANG,
  LLC_FIRMWARE_MODE_SPI,
  LLC_FIRMWARE_MODE_DFU,
} eLLCFirmwareMode;
/// @copydoc eLLCFirmwareMode
typedef uint8_t tLLCFirmwareMode;

/// LLC SPI
typedef struct LLCFirmware
{
  tLLCFirmwareMode Mode;

} tLLCFirmware;

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------

int LLC_FirmwareSetup (struct LLCDev *pDev);
int LLC_FirmwareRelease (struct LLCDev *pDev);

#else
#error This file should not be included from user space.
#endif // #ifdef __KERNEL__
#endif // #ifndef __DRIVERS__COHDA__LLC__LLC_FIRMWARE_H__
/**
 * @}
 * @}
 */
