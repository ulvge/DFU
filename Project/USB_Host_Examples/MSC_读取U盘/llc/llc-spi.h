/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @file
 * LLC: SPI interfacing functionality
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __DRIVERS__COHDA__LLC__LLC_SPI_H__
#define __DRIVERS__COHDA__LLC__LLC_SPI_H__
#ifdef __KERNEL__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <linux/spi/spi.h>

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

// forward decl
struct LLCDev;

/// LLC SPI
typedef struct LLCSPI
{
  /// Pointer to the SPI master deivce
  struct spi_master *pMaster;
  /// Pointer to the actual SPI device
  struct spi_device *pDevice;
} tLLCSPI;

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------

int LLC_SPISetup (struct LLCDev *pDev);
int LLC_SPIRelease (struct LLCDev *pDev);

int LLC_SPISendData (struct LLCDev *pDev,
                     char *pBuf,
                     int Len);

#else
#error This file should not be included from user space.
#endif // #ifdef __KERNEL__
#endif // #ifndef __DRIVERS__COHDA__LLC__LLC_SPI_H__
/**
 * @}
 * @}
 */


