/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @section cohda_llc_intern_spi SPI download functionality
 *
 * The download function sends data to MKx via SPI using the Linux SPI driver
 * framework.
 *
 * Under the Linux SPI framework, a SPI device is needed in order to send data
 * over SPI bus. The SPI device can be created during system initialization
 * based on the statically defined board information, or it can be created after
 * system initialization.
 *
 * The user application needs a SPI device driver to interact with the SPI
 * device. This can be done with the general "spidev" driver which is already
 * implemented in the Linux SPI framework.
 *
 * In our case, we don't need to use "spidev" or implement a new driver.
 * This means no particular driver will be bind to the SPI device.
 * We just implement a transfer function on top of the SPI framework within
 * the LLC driver. This submodule is just responsible for the transfer function
 * implementation.
 *
 * The PSL submodule will use gpiolib to control the GPIOs of the PSL interface
 * and use this SPI submodule to transfer the data on DCLK/DATA0.
 *
 * @file
 * LLC: SPI interfacing functionality
 *
 * The SPI submodule functionalities to MKx firmware loading via SPI.
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <linux/version.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/errno.h>

#include "llc-internal.h"
#include "llc-spi.h"

#define D_SUBMODULE LLC_Firmware
#include "debug-levels.h"

//------------------------------------------------------------------------------
// Local Macros & Constants
//------------------------------------------------------------------------------

// Is the SPI interface used on this board?
#if (defined(BOARD_MK4) || defined(BOARD_I686))
  #define LLC_SPI_ENABLE 1
#endif

// SPI defaults

/// The SPI master bus number
#define LLC_SPI_BUS_NUM      -1
/// The SPI chipselect
#define LLC_SPI_CHIP_SELECT  -1
/// 6Mbps is the max according to SAF5100 datasheet
#define LLC_SPI_MAX_SPEED    6*1024*1024
/// SPI transfer 'chunk' size
#define LLC_SPI_MAX_CHUNK    2*1024*1024
/// SPI download of SAF5100 is always mode 0
#define LLC_SPI_MODE         SPI_MODE_0

#if (defined(BOARD_MK4))
#undef LLC_SPI_BUS_NUM
#undef LLC_SPI_CHIP_SELECT
#undef LLC_SPI_MAX_SPEED
#undef LLC_SPI_MAX_CHUNK

#define LLC_SPI_BUS_NUM      3
#define LLC_SPI_CHIP_SELECT  2
#define LLC_SPI_MAX_SPEED    3*1000*1000
#define LLC_SPI_MAX_CHUNK    2*1024*1024
#endif

#if (defined (BOARD_I686))
#undef LLC_SPI_BUS_NUM
#undef LLC_SPI_CHIP_SELECT
#undef LLC_SPI_MAX_SPEED
#undef LLC_SPI_MAX_CHUNK

#define LLC_SPI_BUS_NUM      2210
#define LLC_SPI_CHIP_SELECT  0
#define LLC_SPI_MAX_SPEED    3*1000*1000
#define LLC_SPI_MAX_CHUNK    4*1024
#endif


//------------------------------------------------------------------------------
// Local Type definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Function definitions
//------------------------------------------------------------------------------

#if (defined(LLC_SPI_ENABLE))

static void LLC_SPIComplete (void *pArg)
{
  complete(pArg);
}

/**
 * Send data over the SPI
 *
 * @param dev The MKx device structure
 * @param buf The data buffer
 * @param count The number of bytes in the buffer
 * @return Zero for success or a negative errno on failure
 *
 */
static int LLC_SPITransfer (struct LLCDev *pDev,
                            char *pBuf,
                            size_t Len)
{
  int Res = -ENOSYS;
  struct spi_device *pSPIDevice = pDev->SPI.pDevice;
  struct spi_transfer Transfer = { 0, };
  struct spi_message  Msg;
  DECLARE_COMPLETION_ONSTACK(Done);
  int Cnt = 0;

  while (Cnt < Len)
  {
    Transfer.tx_buf        = pBuf + Cnt;
    Transfer.rx_buf        = NULL;
    if ((Len - Cnt) > LLC_SPI_MAX_CHUNK)
      Transfer.len         = LLC_SPI_MAX_CHUNK;
    else
      Transfer.len         = (Len - Cnt);

    spi_message_init(&Msg);
    spi_message_add_tail(&Transfer, &Msg);
    Msg.complete = LLC_SPIComplete;
    Msg.context = &Done;

    Res = spi_async(pSPIDevice, &Msg);
    if (Res == 0)
    {
      wait_for_completion(&Done);
      Res = Msg.status;

      if ((Res == 0) && (Transfer.len == Msg.actual_length))
      {
        Cnt += Msg.actual_length;
      }
      else
      {
        d_printf(D_ERR, NULL, "SPI transfer failed: %d %d vs %d\n",
                 Cnt, Len, Msg.actual_length);
        break;
      }
    }
  }

  return Res;
}

/**
 * @brief Send the provided binary over the SPI
 * @param pDev The LLC device structure
 * @param pBuf The (already copied) bitfile buffer
 * @param Len The number of bytes in the buffer
 * @return Zero for success or a negative errno on failure
 *
 * @sa psl_cfg_procedure
 */
int LLC_SPISendData (struct LLCDev *pDev,
                     char *pBuf,
                     int Len)
{
  // send the data
  if (LLC_SPITransfer(pDev, pBuf, Len))
  {
    d_printf(D_ERR, NULL, "failed to send data via SPI\n");
    return -EINVAL;
  }
  return 0;
}

//------------------------------------------------------------------------------
// Submodule initialization and cleanup functions
//------------------------------------------------------------------------------

static struct spi_board_info LLCSPIInfo =
{
  .modalias           = "LLC SPI Interface",
  .platform_data      = NULL,
  .controller_data    = NULL,
  .bus_num            = LLC_SPI_BUS_NUM,
  .chip_select        = LLC_SPI_CHIP_SELECT,
  .max_speed_hz       = LLC_SPI_MAX_SPEED, // 12Mbps is the max according to Linux RM
  .mode               = LLC_SPI_MODE,
};

/**
 * @brief Setup the SPI controller
 * @param pDev The LLC device structure
 * @return Zero for success or a negative errno on failure
 *
 * This function initialize the hardware first, and then create the spi_device.
 * Creating the spi_device is basically the same as what spi_new_device() does.
 *
 * This function makes the module re-loadable even the device already exists
 * in the system. If the device already exists, it will return the pointer of
 * it which needs to be saved in tLLCDev.
 *
 */
int LLC_SPISetup (struct LLCDev *pDev)
{
  int Res = 0;
  struct spi_master *pSPIMaster;
  struct spi_device *pSPIDevice;
  struct device *pD;

  pSPIMaster = spi_busnum_to_master(LLCSPIInfo.bus_num);
  if (pSPIMaster == NULL)
  {
    d_printf(D_ERR, NULL, "can't find spi_master of bus: %d\n",
             LLCSPIInfo.bus_num);
    Res = -EINVAL;
    goto Error;
  }
  d_printf(D_TST, NULL, "cs%d (%d availible) on bus %d\n",
           LLCSPIInfo.chip_select, pSPIMaster->num_chipselect, LLCSPIInfo.bus_num);
  if (LLCSPIInfo.chip_select >= pSPIMaster->num_chipselect)
  {
    d_printf(D_ERR, NULL, "cs%d >= max %d\n",
             LLCSPIInfo.chip_select,
             pSPIMaster->num_chipselect);
    Res = -EINVAL;
    goto Error;
  }

  // The below part is much like spi_new_device().
  // But we can detect the existing spi_device, and find it
  // if it does exist already.
  pSPIDevice = spi_alloc_device(pSPIMaster);
  if (pSPIDevice == NULL)
  {
    d_printf(D_ERR, NULL, "can't alloc memory for spi_device\n");
    Res = -ENOMEM;
    goto Error;
  }

  // Init spi_device
  pSPIDevice->chip_select  = LLCSPIInfo.chip_select;
  pSPIDevice->max_speed_hz = LLCSPIInfo.max_speed_hz;
  pSPIDevice->mode         = LLCSPIInfo.mode;
  pSPIDevice->irq          = LLCSPIInfo.irq;
  d_printf(D_TST, NULL, "speed=%d, mode=0x%x, bpw=%d\n",
           pSPIDevice->max_speed_hz, pSPIDevice->mode, pSPIDevice->bits_per_word);
  strlcpy(pSPIDevice->modalias, LLCSPIInfo.modalias,
          sizeof(pSPIDevice->modalias));
  pSPIDevice->dev.platform_data = (void *) LLCSPIInfo.platform_data;
  pSPIDevice->controller_data = LLCSPIInfo.controller_data;
  pSPIDevice->controller_state = NULL;

  // Set the bus ID string
  #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
    dev_set_name(&pSPIDevice->dev, "%s.%u", dev_name(&pSPIMaster->dev),
                pSPIDevice->chip_select);
  #else
    snprintf(pSPIDevice->dev.bus_id, sizeof(pSPIDevice->dev.bus_id),
             "%s.%u", dev_name(&(pSPIMaster->dev)), pSPIDevice->chip_select);
  #endif

  // find whether the spi_device already registered
  pD = bus_find_device_by_name(&spi_bus_type, NULL,
                               dev_name(&pSPIDevice->dev));
  if (pD != NULL)
  {
    // In this case, we use the existing one without re-setup
    d_printf(D_INFO, NULL, "use the existing spi_device\n");
    spi_dev_put(pSPIDevice); // release the allocated spi_device
    pSPIDevice = to_spi_device(pD);
  }
  else
  {
    Res = spi_add_device(pSPIDevice);
    if (Res < 0)
    {
      d_printf(D_ERR, NULL, "can't add spi_device\n");
      spi_dev_put(pSPIDevice); // release the allocated spi_device
      Res = -EINVAL;
      goto Error;
    }
  }

  // save the spi_master and spi_device information for future use
  pDev->SPI.pMaster = pSPIMaster;
  pDev->SPI.pDevice = pSPIDevice;

Error:
  return Res;
}

/**
 * @brief Release any resources used by the SPI interface
 * @param pDev The LLC device structure
 * @return Zero for success or a negative errno on failure
 *
 */
int LLC_SPIRelease (struct LLCDev *pDev)
{
  ; // TODO: Deactivate the SPI
  return 0;
}

#else //#if (defined(LLC_SPI_ENABLE))

int LLC_SPISendData (struct LLCDev *pDev,
                     char *pBuf,
                     int Len)
{
  (void)pDev;
  (void)pBuf;
  (void)Len;
  return -ENOSYS;
}


int LLC_SPISetup (struct LLCDev *pDev)
{
  (void)pDev;
  return 0;
}

int LLC_SPIRelease (struct LLCDev *pDev)
{
  (void)pDev;
  return 0;
}

#endif //#if (defined(LLC_SPI_ENABLE))

// Close the Doxygen group
/**
 * @}
 * @}
 */

