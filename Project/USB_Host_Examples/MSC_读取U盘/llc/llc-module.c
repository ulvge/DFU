/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @section cohda_llc_ko LLC kernel module
 *
 * @file
 * LLC: kernel module
 *
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>

#include "cohda/llc/llc.h"
#include "cohda/llc/llc-api.h"
#include "llc-internal.h"

#define D_SUBMODULE LLC_Module
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

/// Debug framework control of debug levels
struct d_level D_LEVEL[] =
{
  D_SUBMODULE_DEFINE(LLC_Module),
  D_SUBMODULE_DEFINE(LLC_API),
  D_SUBMODULE_DEFINE(LLC_Device),
  D_SUBMODULE_DEFINE(LLC_NetDev),
  D_SUBMODULE_DEFINE(LLC_Netlink),
  D_SUBMODULE_DEFINE(LLC_Monitor),
  D_SUBMODULE_DEFINE(LLC_List),
  D_SUBMODULE_DEFINE(LLC_Msg),
  D_SUBMODULE_DEFINE(LLC_Thread),
  D_SUBMODULE_DEFINE(LLC_USB),
  D_SUBMODULE_DEFINE(LLC_Firmware),
  D_SUBMODULE_DEFINE(LLC_Debug),
};
size_t D_LEVEL_SIZE = ARRAY_SIZE(D_LEVEL);

// Used by LLC_ModuleInit() to pass to LLC_Setup()
static struct LLCDev __LLCDev;

// (Re)set by LLC_Setup() and used by LLC_Init()
static struct LLCDev *pLLCDev = &(__LLCDev);

/// Module parameter: LLC or 802.11 cw-mon-* frame format
static uint32_t MonitorFormat = LLC_MON_FORMAT_DEFAULT;
module_param (MonitorFormat, uint, S_IRUGO);

/// Module parameter: Time when to stop tx / rx for geofencing etc as
/// seconds from Unix epoch
static ulong SilentTime = 0;
module_param (SilentTime, ulong, 0644);

/// Module parameter: Loopback enabled?
static uint32_t loopback = 0;
module_param(loopback, uint, S_IRUGO);

/// Module parameter: SPI download full firmware or DFU bootloader
static uint32_t SPIDownload = LLC_SPI_DFU;
module_param (SPIDownload, uint, 0644);

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

/**
 * @brief Initialize the LLC and get a handle to it for further interaction
 * @param ppHdl Pointer to a LLC handle (populated by this function)
 * @return LLC_STATUS_SUCCESS (0) or a negative error code @sa eLLCStatus
 */
tMKxStatus MKx_Init(struct MKx **ppMKx)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_API, NULL, "(ppMKx %p)\n", ppMKx);

  *ppMKx = &(pLLCDev->MKx);
  Res = LLC_Start(*ppMKx);

  d_fnend(D_API, NULL, "(ppMKx %p [%p]) = %d\n", ppMKx, *ppMKx, Res);
  return Res;
}
EXPORT_SYMBOL(MKx_Init);

/**
 * @brief De-initialize the LLC
 * @param pHdl LLC handle obtained using LLC_Init()
 * @return LLC_STATUS_SUCCESS (0) or a negative error code @sa eLLCStatus
 */
tMKxStatus MKx_Exit(struct MKx *pMKx)
{
  int Res = LLC_STATUS_ERROR;

  d_fnstart(D_API, NULL, "(pMKx %p)\n", pMKx);
  d_assert(pMKx == &(pLLCDev->MKx));

  Res = LLC_Stop(pMKx);

  d_fnend(D_API, NULL, "(pMKx %p) = %d\n", pMKx, Res);
  return Res;
}
EXPORT_SYMBOL(MKx_Exit);

/**
 * @brief Setup function for the LLC module
 * @param pDev LLC structure to initialize
 * @return 0 for success, or a negative errno
 *
 * Initializes all the parts of the LLC module
 *
 */
int LLC_Setup(struct LLCDev *pDev)//此函数在dot4中没用呢!!!
{
  int Res = -ENODEV;
  struct MKx *pMKx = NULL;

  d_fnstart(D_INTERN, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  // Default HIL configuration based on module parameters
  pDev->Cfg.MonitorFormat = MonitorFormat;

  // pLLCDev is used within MKx_Init()
  pLLCDev = pDev;
  pDev->Magic = LLC_DEV_MAGIC;

  Res = LLC_DeviceSetup(pDev);//
  if (Res != 0)
    goto ErrorDevice;

  Res = MKx_Init(&pMKx);//在1609.4中有用
  if (Res != 0)
    goto ErrorLLCInit;

  Res = 0;
  goto Success;

  MKx_Exit(pMKx);
ErrorLLCInit:
  LLC_DeviceRelease(pDev);
ErrorDevice:
Success:
  d_fnend(D_INTERN, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}
EXPORT_SYMBOL(LLC_Setup);

/**
 * @brief Release the LLC resources
 * @param pDev LLC device pointer
 * @return 0 for success or negative errno
 *
 * Undoes any setup done by LLC_Setup()
 *
 */
int LLC_Release(struct LLCDev *pDev)
{
  int Res = -ENODEV;
  struct MKx *pMKx = NULL;

  d_fnstart(D_INTERN, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  // Release everything in the opposite order that it was created in
  // LLC_Setup()

  Res = 0;

  pMKx = &(pDev->MKx);
  if (pMKx != NULL)
    Res += MKx_Exit(pMKx);

  Res += LLC_DeviceRelease(pDev);

  d_fnend(D_INTERN, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}
EXPORT_SYMBOL(LLC_Release);


/**
 * @brief Access the module parameter 'MonitorFormat'
 */
int32_t LLC_GetMonitorFormat(void)
{
  return MonitorFormat;
}

/**
 * @brief Access the module parameter 'Loopback'
 */
int32_t LLC_GetLoopback(void)
{
  return loopback;
}

/**
 * @brief Determine if the LLC should block trnsmissions
 * This is done by accesing the module parameter 'SilentTime'
 */
bool LLC_IsSilent(void)
{
  bool Silent = false;

  if (SilentTime > 0)
  {
    struct timeval Now;

    do_gettimeofday(&Now);

    Silent = SilentTime <= Now.tv_sec;
  }
  return Silent;
}

uint32_t LLC_Get_SPIDownload(void)
{
  return SPIDownload;
}

/**
 * @brief Initialize the 'LLC' kernel module
 *
 */
static int LLC_ModuleInit(void)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = pLLCDev;

  d_init();
  d_fnstart(D_INTERN, NULL, "(void) [pDev %p]\n", pDev);

  memset(pDev, 0, sizeof(struct LLCDev));
  Res = LLC_Setup(pDev);

  d_fnend(D_INTERN, NULL, "(void) [pDev %p] = %d\n", pDev, Res);
  if (Res != 0)
    d_exit();
  return Res;
}

/**
 * @brief De-Initialize the 'LLC' kernel module
 *
 */
static void LLC_ModuleExit(void)
{
  struct LLCDev *pDev = pLLCDev;
  int Res = -ENOSYS;

  d_fnstart(D_INTERN, NULL, "(void) [pDev %p]\n", pDev);

  Res = LLC_Release(pDev);

  d_fnend(D_INTERN, NULL, "(void) [pDev %p] = void\n", pDev);
  d_exit();
  return;
}
#if 0
module_exit(LLC_ModuleExit);    //@模块卸载

MODULE_AUTHOR("Cohda Wireless");
MODULE_DESCRIPTION("Cohda LLCremote kernel module");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.0");
#endif
/**
 * @}
 * @}
 */

