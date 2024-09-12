/**
  ******************************************************************************
  * @file    usbh_usr.c
  * @author  MCD Application Team
  * @version V2.1.0
  * @date    19-March-2012
  * @brief   This file includes the usb host library user callbacks
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "usbh_usr.h"
#include "lcd_log.h"
#include "ff.h"       /* FATFS */
#include "usbh_msc_core.h"
#include "usbh_msc_scsi.h"
#include "usbh_msc_bot.h"


#if (DUG_PRINTF == xprintf)
  #include <xprintf.h>
#endif


/** @addtogroup USBH_USER
* @{
*/

/** @addtogroup USBH_MSC_DEMO_USER_CALLBACKS
* @{
*/

/** @defgroup USBH_USR 
* @brief    This file includes the usb host stack user callbacks
* @{
*/ 

/** @defgroup USBH_USR_Private_TypesDefinitions
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Defines
* @{
*/ 
#define IMAGE_BUFFER_SIZE    512
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Macros
* @{
*/ 
extern USB_OTG_CORE_HANDLE          USB_OTG_Core;
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Variables
* @{
*/ 
uint8_t USBH_USR_ApplicationState = USH_USR_FS_INIT;
uint8_t filenameString[15]  = {0};

FATFS fatfs;
FIL file;
uint8_t Image_Buf[IMAGE_BUFFER_SIZE];
uint8_t line_idx = 0;   

/*  Points to the DEVICE_PROP structure of current device */
/*  The purpose of this register is to speed up the execution */

USBH_Usr_cb_TypeDef USR_cb =
{
  USBH_USR_Init,						//Init		
  USBH_USR_DeInit,						//DeInit
  USBH_USR_DeviceAttached,				//DeviceAttached
  USBH_USR_ResetDevice,					//ResetDevice
  USBH_USR_DeviceDisconnected,			//DeviceDisconnected
  USBH_USR_OverCurrentDetected,			//OverCurrentDetected
  USBH_USR_DeviceSpeedDetected,			//DeviceSpeedDetected
  USBH_USR_Device_DescAvailable,		//DeviceDescAvailable
  USBH_USR_DeviceAddressAssigned,		//DeviceAddressAssigned
  USBH_USR_Configuration_DescAvailable,	//ConfigurationDescAvailable
  USBH_USR_Manufacturer_String,			//ManufacturerString
  USBH_USR_Product_String,				//ProductString
  USBH_USR_SerialNum_String,			//SerialNumString
  USBH_USR_EnumerationDone,				//EnumerationDone
  USBH_USR_UserInput,					//UserInput
  USBH_USR_MSC_Application,				//UserApplication
  USBH_USR_DeviceNotSupported,			//DeviceNotSupported
  USBH_USR_UnrecoveredError				//UnrecoveredError
    
};

/**
* @}
*/

/** @defgroup USBH_USR_Private_Constants
* @{
*/ 
/*--------------- LCD Messages ---------------*/
const uint8_t MSG_HOST_INIT[]        = "> Host Library Initialized\n";
const uint8_t MSG_DEV_ATTACHED[]     = "> Device Attached \n";
const uint8_t MSG_DEV_DISCONNECTED[] = "> Device Disconnected\n";
const uint8_t MSG_DEV_ENUMERATED[]   = "> Enumeration completed \n";
const uint8_t MSG_DEV_HIGHSPEED[]    = "> High speed device detected\n";
const uint8_t MSG_DEV_FULLSPEED[]    = "> Full speed device detected\n";
const uint8_t MSG_DEV_LOWSPEED[]     = "> Low speed device detected\n";
const uint8_t MSG_DEV_ERROR[]        = "> Device fault \n";

const uint8_t MSG_MSC_CLASS[]        = "> Mass storage device connected\n";
const uint8_t MSG_HID_CLASS[]        = "> HID device connected\n";
const uint8_t MSG_DISK_SIZE[]        = "> Size of the disk in MBytes: \n";
const uint8_t MSG_LUN[]              = "> LUN Available in the device:\n";
const uint8_t MSG_ROOT_CONT[]        = "> Exploring disk flash ...\n";
const uint8_t MSG_WR_PROTECT[]       = "> The disk is write protected\n";
const uint8_t MSG_UNREC_ERROR[]      = "> UNRECOVERED ERROR STATE\n";

const uint8_t MSG_DFU_CLASS[]        = "> DFU device connected\n";





/**
* @}
*/


/** @defgroup USBH_USR_Private_FunctionPrototypes
* @{
*/
static uint8_t Explore_Disk (char* path , uint8_t recu_level);
static uint8_t Image_Browser (char* path);
static void     Show_Image(void);
static void     Toggle_Leds(void);
/**
* @}
*/ 


/** @defgroup USBH_USR_Private_Functions
* @{
*/ 


/**
* @brief  USBH_USR_Init 
*         Displays the message on LCD for host lib initialization
* @param  None
* @retval None
*/
void USBH_USR_Init(void)
{
  static uint8_t startup = 0;  
  
  if(startup == 0 )
  {
    startup = 1;
    /* Configure the LEDs */
    STM_EVAL_LEDInit(LED1);
    STM_EVAL_LEDInit(LED2);
    STM_EVAL_LEDInit(LED3); 
    STM_EVAL_LEDInit(LED4); 
    
    STM_EVAL_PBInit(BUTTON_KEY, BUTTON_MODE_GPIO);
    
#if defined (USE_STM322xG_EVAL)
  STM322xG_LCD_Init();
#elif defined(USE_STM324xG_EVAL)
  STM324xG_LCD_Init();
#elif defined (USE_STM3210C_EVAL)
  STM3210C_LCD_Init();
#else
 #error "Missing define: Evaluation board (ie. USE_STM322xG_EVAL)"
#endif
    
    LCD_LOG_Init();
    xprintf("\n ==> ARMJISHU神舟STM32开发板，USB HOST实验之U盘的访问 <==");  
    xprintf("\n ==> USB读取U盘目录，LCD显示/Media/文件夹中的BMP图片. <==\n");
	      
#ifdef USE_USB_OTG_HS 
    LCD_LOG_SetHeader(" USB OTG HS MSC Host");
#else
    LCD_LOG_SetHeader(" USB OTG FS MSC Host");
#endif
    LCD_UsrLog("> USB Host library started.\n"); 
    LCD_LOG_SetFooter (" ARMJISHU.COM USB Host Library v2.1.0" );
  }
}

/**
* @brief  USBH_USR_DeviceAttached 
*         Displays the message on LCD on device attached
* @param  None
* @retval None
*/
void USBH_USR_DeviceAttached(void)
{
  LCD_UsrLog((void *)MSG_DEV_ATTACHED);
}


/**
* @brief  USBH_USR_UnrecoveredError
* @param  None
* @retval None
*/
void USBH_USR_UnrecoveredError (void)
{
  
  /* Set default screen color*/ 
  LCD_ErrLog((void *)MSG_UNREC_ERROR); 
}


/**
* @brief  USBH_DisconnectEvent
*         Device disconnect event
* @param  None
* @retval Staus
*/
void USBH_USR_DeviceDisconnected (void)
{
  
  LCD_LOG_ClearTextZone();
  
  LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 42, "                                      ");
  LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 30, "                                      ");  
  
  /* Set default screen color*/
  LCD_ErrLog((void *)MSG_DEV_DISCONNECTED);
}
/**
* @brief  USBH_USR_ResetUSBDevice 
* @param  None
* @retval None
*/
void USBH_USR_ResetDevice(void)
{
  /* callback for USB-Reset */
}


/**
* @brief  USBH_USR_DeviceSpeedDetected 
*         Displays the message on LCD for device speed
* @param  Device speed
* @retval None
*/
void USBH_USR_DeviceSpeedDetected(uint8_t DeviceSpeed)
{
  if(DeviceSpeed == HPRT0_PRTSPD_HIGH_SPEED)
  {
    LCD_UsrLog((void *)MSG_DEV_HIGHSPEED);
  }  
  else if(DeviceSpeed == HPRT0_PRTSPD_FULL_SPEED)
  {
    LCD_UsrLog((void *)MSG_DEV_FULLSPEED);
  }
  else if(DeviceSpeed == HPRT0_PRTSPD_LOW_SPEED)
  {
    LCD_UsrLog((void *)MSG_DEV_LOWSPEED);
  }
  else
  {
    LCD_UsrLog((void *)MSG_DEV_ERROR);
  }
}

/**
* @brief  USBH_USR_Device_DescAvailable 
*         Displays the message on LCD for device descriptor
* @param  device descriptor
* @retval None
*/
void USBH_USR_Device_DescAvailable(void *DeviceDesc)
{ 
  USBH_DevDesc_TypeDef *hs;
  hs = DeviceDesc;  
  
  
  LCD_UsrLog("\n VID : %d\n" , (uint32_t)(*hs).idVendor); 
  LCD_UsrLog(" PID : %d\n" , (uint32_t)(*hs).idProduct);
}

/**
* @brief  USBH_USR_DeviceAddressAssigned 
*         USB device is successfully assigned the Address 
* @param  None
* @retval None
*/
void USBH_USR_DeviceAddressAssigned(void)
{
  
}


/**
* @brief  USBH_USR_Conf_Desc 
*         Displays the message on LCD for configuration descriptor
* @param  Configuration descriptor
* @retval None
*/
void USBH_USR_Configuration_DescAvailable(USBH_CfgDesc_TypeDef * cfgDesc,
                                          USBH_InterfaceDesc_TypeDef *itfDesc,
                                          USBH_EpDesc_TypeDef *epDesc)
{
  USBH_InterfaceDesc_TypeDef *id;
  
  id = itfDesc;  
  
  if((*id).bInterfaceClass  == 0x08)
  {
    LCD_UsrLog((void *)MSG_MSC_CLASS);
  }
  else if((*id).bInterfaceClass  == 0x03)
  {
    LCD_UsrLog((void *)MSG_HID_CLASS);
  }    
  else if((*id).bInterfaceClass  == APP_SPECIFIC_CLASS)
  {
    LCD_UsrLog((void *)MSG_DFU_CLASS);
  }    
  else
  {
    LCD_UsrLog((void *)">> 暂不支持的USB类");
  }    
}

/**
* @brief  USBH_USR_Manufacturer_String 
*         Displays the message on LCD for Manufacturer String 
* @param  Manufacturer String 
* @retval None
*/
void USBH_USR_Manufacturer_String(void *ManufacturerString)
{
  LCD_UsrLog("\nManufacturer : %s\n", (char *)ManufacturerString);
}

/**
* @brief  USBH_USR_Product_String 
*         Displays the message on LCD for Product String
* @param  Product String
* @retval None
*/
void USBH_USR_Product_String(void *ProductString)
{
  LCD_UsrLog("Product : %s\n", (char *)ProductString);
}

/**
* @brief  USBH_USR_SerialNum_String 
*         Displays the message on LCD for SerialNum_String 
* @param  SerialNum_String 
* @retval None
*/
void USBH_USR_SerialNum_String(void *SerialNumString)
{
  LCD_UsrLog( "Serial Number : %s\n", (char *)SerialNumString); 
} 



/**
* @brief  EnumerationDone 
*         User response request is displayed to ask application jump to class
* @param  None
* @retval None
*/
void USBH_USR_EnumerationDone(void)
{
  
  /* Enumeration complete */
  LCD_UsrLog((void *)MSG_DEV_ENUMERATED);
  
  LCD_SetTextColor(Green);
  LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 42, "To see the root content of the disk : " );
  LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 30, "Press Key2...                       ");
  LCD_SetTextColor(LCD_LOG_DEFAULT_COLOR); 
  DUG_PRINTF("\r\n To see the root content of the disk : Press Key2...\r\n" );
  
} 


/**
* @brief  USBH_USR_DeviceNotSupported
*         Device is not supported
* @param  None
* @retval None
*/
void USBH_USR_DeviceNotSupported(void)
{
  LCD_ErrLog ("> Device not supported."); 
}  


/**
* @brief  USBH_USR_UserInput
*         User Action for application state entry
* @param  None
* @retval USBH_USR_Status : User response for key button
*/
USBH_USR_Status USBH_USR_UserInput(void)
{
  USBH_USR_Status usbh_usr_status;
  
  usbh_usr_status = USBH_USR_NO_RESP;  
  
  /*Key B3 is in polling mode to detect user action */
  if(STM_EVAL_PBGetState(Button_KEY) == RESET) 
  {
    
    usbh_usr_status = USBH_USR_RESP_OK;
    
  } 
  return usbh_usr_status;
}  

/**
* @brief  USBH_USR_OverCurrentDetected
*         Over Current Detected on VBUS
* @param  None
* @retval Staus
*/
void USBH_USR_OverCurrentDetected (void)
{
  LCD_ErrLog ("Overcurrent detected.");
}


/**
* @brief  USBH_USR_MSC_Application 
*         Demo application for mass storage
* @param  None
* @retval Staus
*/
int USBH_USR_MSC_Application(void)
{
  FRESULT res;
  uint8_t writeTextBuff[] = "WWW.ARMJISHU.COM STM32F207ZGT\r\nSTM32 Connectivity line Host Demo application using FAT_FS   ";
  uint16_t bytesWritten, bytesToWrite;
  
  switch(USBH_USR_ApplicationState)
  {
  case USH_USR_FS_INIT: 
    
    /* Initialises the File System*/
    if ( f_mount( 0, &fatfs ) != FR_OK ) 
    {
      /* efs initialisation fails*/
      LCD_ErrLog("> Cannot initialize File System.\n");
      return(-1);
    }
    LCD_UsrLog("> File System initialized.\n");
    LCD_UsrLog("> Disk capacity : %d M Bytes\n", USBH_MSC_Param.MSCapacity * \
      USBH_MSC_Param.MSPageLength/1024/1024); 
    
    if(USBH_MSC_Param.MSWriteProtect == DISK_WRITE_PROTECTED)
    {
      LCD_ErrLog((void *)MSG_WR_PROTECT);
    }
    
    USBH_USR_ApplicationState = USH_USR_FS_READLIST;
    break;
    
  case USH_USR_FS_READLIST:
    
    LCD_UsrLog((void *)MSG_ROOT_CONT);
	DUG_PRINTF("\r\n %s\r\n", (void *)MSG_ROOT_CONT);
    Explore_Disk("0:/", 1);
    line_idx = 0;   
    USBH_USR_ApplicationState = USH_USR_FS_WRITEFILE;
    
    break;
    
  case USH_USR_FS_WRITEFILE:
    
    LCD_SetTextColor(Green);
    LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 42, "                                              ");
    LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 30, "Press Key2 to write file");
    LCD_SetTextColor(LCD_LOG_DEFAULT_COLOR); 
    USB_OTG_BSP_mDelay(100);
    
    /*Key B3 in polling*/
    while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
      (STM_EVAL_PBGetState (BUTTON_KEY) == SET))          
    {
      Toggle_Leds();
    }
    /* Writes a text file, STM32.TXT in the disk*/
    LCD_UsrLog("> Writing File to disk flash ...\n");
    if(USBH_MSC_Param.MSWriteProtect == DISK_WRITE_PROTECTED)
    {
      
      LCD_ErrLog ( "> Disk flash is write protected \n");
      USBH_USR_ApplicationState = USH_USR_FS_DRAW;
      break;
    }
    
    /* Register work area for logical drives */
    f_mount(0, &fatfs);
    
    if(f_open(&file, "0:STM32.TXT",FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
    { 
      /* Write buffer to file */
      bytesToWrite = sizeof(writeTextBuff); 
      res= f_write (&file, writeTextBuff, bytesToWrite, (void *)&bytesWritten);   
      
      if((bytesWritten == 0) || (res != FR_OK)) /*EOF or Error*/
      {
        LCD_ErrLog("> STM32.TXT CANNOT be writen.\n");
      }
      else
      {
        LCD_UsrLog("> 'STM32.TXT' file created\n");
      }
      
      /*close file and filesystem*/
      f_close(&file);
      f_mount(0, NULL); 
    }
    
    else
    {
      LCD_UsrLog ("> STM32.TXT created in the disk\n");
    }

    USBH_USR_ApplicationState = USH_USR_FS_DRAW; 
    
    LCD_SetTextColor(Green);
    LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 42, "                                              ");
    LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 30, "To start Image slide show Press Key2.");
    xprintf("\n To start Image slide show Press Key2.");
    LCD_SetTextColor(LCD_LOG_DEFAULT_COLOR); 
  
    break;
    
  case USH_USR_FS_DRAW:
    
    /*Key B3 in polling*/
    while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
      (STM_EVAL_PBGetState (BUTTON_KEY) == SET))
    {
      Toggle_Leds();
    }
  
    while(HCD_IsDeviceConnected(&USB_OTG_Core))
    {
      if ( f_mount( 0, &fatfs ) != FR_OK ) 
      {
        /* fat_fs initialisation fails*/
        return(-1);
      }
      return Image_Browser("0:/Media");
    }
    break;
  default: break;
  }
  return(0);
}

/**
* @brief  Explore_Disk 
*         Displays disk content
* @param  path: pointer to root path
* @retval None
*/
static uint8_t Explore_Disk (char* path , uint8_t recu_level)
{

  FRESULT res;
  FILINFO fno;
  DIR dir;
  char *fn;
  char tmp[14];
  
  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    while(HCD_IsDeviceConnected(&USB_OTG_Core)) 
    {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) 
      {
        break;
      }
      if (fno.fname[0] == '.')
      {
        continue;
      }

      fn = fno.fname;
      strcpy(tmp, fn); 

      line_idx++;
      if(line_idx > 9)
      {
        line_idx = 0;
        LCD_SetTextColor(Green);
        LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 42, "                                              ");
        LCD_DisplayStringLine( LCD_PIXEL_HEIGHT - 30, "Press Key2 to continue...");
        LCD_SetTextColor(LCD_LOG_DEFAULT_COLOR); 
		  
		DUG_PRINTF("\r\n Press Key2 to continue...\r\n");
        
        /*Key B3 in polling*/
        while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
          (STM_EVAL_PBGetState (BUTTON_KEY) == SET))
        {
          Toggle_Leds();
        }
      } 
      
      if(recu_level == 1)
      {
        LCD_DbgLog("   |__");
      }
      else if(recu_level == 2)
      {
        LCD_DbgLog("   |   |__");
      }
      if((fno.fattrib & AM_MASK) == AM_DIR)
      {
        strcat(tmp, "\n"); 
        LCD_UsrLog((void *)tmp);
      }
      else
      {
        strcat(tmp, "\n"); 
        LCD_DbgLog((void *)tmp);
      }

      if(((fno.fattrib & AM_MASK) == AM_DIR)&&(recu_level == 1))
      {
        Explore_Disk(fn, 2);
      }
    }
  }
  return res;
}

static uint8_t Image_Browser (char* path)
{
  FRESULT res;
  uint8_t ret = 1;
  FILINFO fno;
  DIR dir;
  char *fn;
  char tmp[30];

  
  res = f_opendir(&dir, path);
  if (res == FR_OK) {
    
    for (;;) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) break;
      if (fno.fname[0] == '.') continue;

      fn = fno.fname;
 
      if (fno.fattrib & AM_DIR) 
      {
        continue;
      } 
      else 
      {
        if((strstr(fn, "bmp")) || (strstr(fn, "BMP")))
        {
          tmp[0] = '\0';
          strcpy(tmp, path);
          strcat(tmp, "/");
          strcat(tmp, fn);
          res = f_open(&file, tmp, FA_OPEN_EXISTING | FA_READ);
          xprintf("\n\n ARMJISHU神舟STM32开发板，显示BMP图片: %s.", tmp);
          Show_Image();
          USB_OTG_BSP_mDelay(100);
          ret = 0;
          while((HCD_IsDeviceConnected(&USB_OTG_Core)) && \
            (STM_EVAL_PBGetState (BUTTON_KEY) == SET))
          {
            Toggle_Leds();
          }
          f_close(&file);
          
        }
      }
    }  
  }
  
  #ifdef USE_USB_OTG_HS 
  LCD_LOG_SetHeader(" USB OTG HS MSC Host");
#else
  LCD_LOG_SetHeader(" USB OTG FS MSC Host");
#endif
  LCD_LOG_SetFooter (" ARMJISHU.COM USB Host Library v2.1.0" );
  LCD_UsrLog("> Disk capacity : %d M Bytes\n", USBH_MSC_Param.MSCapacity * \
      USBH_MSC_Param.MSPageLength/1024/1024); 
  USBH_USR_ApplicationState = USH_USR_FS_READLIST;
  return ret;
}

/**
* @brief  Show_Image 
*         Displays BMP image
* @param  None
* @retval None
*/
static void Show_Image(void)
{
  
  uint16_t i = 0;
  uint16_t numOfReadBytes = 0;
  FRESULT res; 
  uint16_t PictureWidth, PictureHeight, PictureBitsPerPixel;
  uint16_t Xpos = 0, Ypos = 0;
  
  //LCD_SetDisplayWindow(239, 319, 240, 320);
  //LCD_WriteReg(R3, 0x1008);
  //LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
  res = f_read(&file, Image_Buf, IMAGE_BUFFER_SIZE, (void *)&numOfReadBytes);

  xprintf("\n f_read file return is %d.", res);

  PictureWidth = (Image_Buf[0x13] << 8) | Image_Buf[0x12];
  PictureHeight  = (Image_Buf[0x17] << 8) | Image_Buf[0x16];  
  xprintf("\n PictureWidth is %d, PictureHeight is %d.", PictureWidth, PictureHeight);
  PictureBitsPerPixel  = (Image_Buf[0x1D] << 8) | Image_Buf[0x1C];
  xprintf("\n PictureBitsPerPixel is %dbpp.", PictureBitsPerPixel);
  
  /* Bypass Bitmap header */ 
  f_lseek (&file, 54);

  if((PictureWidth>320) || (PictureHeight>320))
  {
    xprintf("\n Picture too Large.");
    LCD_LOG_SetHeader(" Picture too Large..");
    return;
  }

  if(PictureBitsPerPixel == 24)
  {
      while (HCD_IsDeviceConnected(&USB_OTG_Core))
      {
        res = f_read(&file, Image_Buf, 510, (void *)&numOfReadBytes);
        if((numOfReadBytes == 0) || (res != FR_OK)) /*EOF or Error*/
        {
          break; 
        }

        if(PictureWidth < PictureHeight)
        {
          for(i = 0 ; i < numOfReadBytes; i+= 3)
          {
             //LCD_SetPoint(Xpos, Ypos, Image_Buf[i+1] << 8 | Image_Buf[i]);
             LCD_SetPoint(Xpos, Ypos, (Image_Buf[i+2]& 0xF8) << 8 | (Image_Buf[i+1] & 0xFC) << 3 | Image_Buf[i]>>3);
             Xpos++;
             if(Xpos >= PictureWidth)
             {
               Xpos = 0;
               Ypos++;
             }
          }
        }
        else
        {
          for(i = 0 ; i < numOfReadBytes; i+= 3)
          {          
             LCD_SetPoint(239-Xpos, Ypos, (Image_Buf[i+2]& 0xF8) << 8 | (Image_Buf[i+1] & 0xFC) << 3 | Image_Buf[i]>>3);
             Ypos++;
             if(Ypos >= PictureWidth)
             {
               Ypos = 0;
               Xpos++;
             }
          }
        }
      }
	  //xprintf("\n Xpos %d, Ypos %d.", Xpos, Ypos);
  }
  else  if(PictureBitsPerPixel == 16)
  {
      while (HCD_IsDeviceConnected(&USB_OTG_Core))
      {
        res = f_read(&file, Image_Buf, IMAGE_BUFFER_SIZE, (void *)&numOfReadBytes);
        if((numOfReadBytes == 0) || (res != FR_OK)) /*EOF or Error*/
        {
          break; 
        }

        if(PictureWidth < PictureHeight)
        {
          for(i = 0 ; i < numOfReadBytes; i+= 2)
          {
             //LCD_SetPoint(Xpos, Ypos, Image_Buf[i+1] << 8 | Image_Buf[i]);
             LCD_SetPoint(Xpos, Ypos, Image_Buf[i+1] << 8 | Image_Buf[i]);
             Xpos++;
             if(Xpos >= PictureWidth)
             {
               Xpos = 0;
               Ypos++;
             }
          }
        }
        else
        {
          for(i = 0 ; i < numOfReadBytes; i+= 2)
          {          
             LCD_SetPoint(239-Xpos, Ypos, Image_Buf[i+1] << 8 | Image_Buf[i]);
             Ypos++;
             if(Ypos >= PictureWidth)
             {
               Ypos = 0;
               Xpos++;
             }
          }
        }
      }
  }
  
}

/**
* @brief  Toggle_Leds
*         Toggle leds to shows user input state
* @param  None
* @retval None
*/
static void Toggle_Leds(void)
{
  static uint32_t i;
  if (i++ == 0x10000)
  {
    STM_EVAL_LEDToggle(LED1);
    STM_EVAL_LEDToggle(LED2);
    STM_EVAL_LEDToggle(LED3);
    STM_EVAL_LEDToggle(LED4);
    i = 0;
  }  
}
/**
* @brief  USBH_USR_DeInit
*         Deint User state and associated variables
* @param  None
* @retval None
*/
void USBH_USR_DeInit(void)
{
  USBH_USR_ApplicationState = USH_USR_FS_INIT;
}


/**
* @}
*/ 

/**
* @}
*/ 

/**
* @}
*/

/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

