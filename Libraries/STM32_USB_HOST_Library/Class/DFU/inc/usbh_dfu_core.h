/**
  ******************************************************************************
  * @file    usbh_dfu_core.h
  * @author  zb
  * @version V1.1.0
  * @date    2015-9-14
  * @brief   This file contains all the prototypes for the usbh_dfu_core.c
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

/* Define to prevent recursive  ----------------------------------------------*/
#ifndef __USBH_HID_CORE_H
#define __USBH_HID_CORE_H

/* Includes ------------------------------------------------------------------*/
#include "usbh_core.h"
#include "usbh_stdreq.h"
#include "usb_bsp.h"
#include "usbh_ioreq.h"
#include "usbh_hcs.h"
 
/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_CLASS
  * @{
  */

/** @addtogroup USBH_HID_CLASS
  * @{
  */
  
/** @defgroup USBH_HID_CORE
  * @brief This file is the Header file for USBH_HID_CORE.c
  * @{
  */ 


/** @defgroup USBH_HID_CORE_Exported_Types
  * @{
  */ 

#define HID_MIN_POLL          10

/* States for HID State Machine */
typedef enum
{
  DFU_IDLE= 0,
  DFU_SEND_DATA,
  DFU_BUSY,
  DFU_GET_DATA,   
  DFU_SYNC,     
  DFU_POLL,
  DFU_ERROR,
}
HID_State;

typedef enum
{
  HID_REQ_IDLE = 0,
  HID_REQ_GET_REPORT_DESC,
  HID_REQ_GET_HID_DESC,
  HID_REQ_SET_IDLE,
  HID_REQ_SET_PROTOCOL,
  HID_REQ_SET_REPORT,

}
HID_CtlState;

typedef struct DFU_cb
{
  void  (*Init)   (void);             
  void  (*Decode) (uint8_t *data);       
  
} DFU_cb_TypeDef;

/* Structure for HID process */
typedef struct _DFU_Process
{
  uint8_t              buff[64];
  uint8_t              hc_num_in; 
  uint8_t              hc_num_out; 
  HID_State            state; 
  uint8_t              HIDIntOutEp;
  uint8_t              HIDIntInEp;
  HID_CtlState         ctl_state;
  uint16_t             length;
  uint8_t              ep_addr;
  uint16_t             poll; 
  __IO uint16_t        timer; 
  DFU_cb_TypeDef             *cb;
}
DFU_Machine_TypeDef;

/**
  * @}
  */ 

/** @defgroup USBH_HID_CORE_Exported_Defines
  * @{
  */ 

#define USB_HID_REQ_GET_REPORT       0x01
#define USB_HID_GET_IDLE             0x02
#define USB_HID_GET_PROTOCOL         0x03
#define USB_HID_SET_REPORT           0x09
#define USB_HID_SET_IDLE             0x0A
#define USB_HID_SET_PROTOCOL         0x0B    


extern USBH_Class_cb_TypeDef  USBH_DFU_cb;



typedef enum
{
    DFU_Err_OK              = 0,//no error
    DFU_Err_TRGET           = 1,//File is not targeted for use by this device
    DFU_Err_FILE            = 2,//File is for this device but fails some vendor-specific verification test
    DFU_Err_WRITE           = 3,//Device is unable to write memory 
    DFU_Err_ERASE           = 4,//Memory erase function failed  
    DFU_Err_CHECK_ERASED    = 5,//Memory erase check failed  
    DFU_Err_PROG            = 6,//Program memory function failed
    DFU_Err_VERIFY          = 7,//Program memory failed verification
    DFU_Err_ADDRESS         = 8,//Can't Program memory due to received address that is out of range
    DFU_Err_NOTDONE         = 9,//Received DFU_DNLOAD with wLength=0,but device does't think it has all of the data yet
    DFU_Err_FIRMWARE        = 0x0A,//Device's firmware is corrupt It can't return to run-time(non-DFU) operations
    DFU_Err_VENDOR          = 0x0B,//iString indicates a vendor-specific error
    DFU_Err_USBR            = 0x0C,//Device deteced unexpected USB reset signaling
    DFU_Err_POR             = 0x0D,//Device deteced unexpected power on reset
    DFU_Err_UNKNOWN         = 0x0E,//Something went wrong,but the device does't know why
    DFU_Err_STALLEDPKT      = 0x0F,//Device stalled an unexpected request
}
DFU_Err_Status;//p21


typedef enum
{
    DFU_APP_IDLE                = 0,//Device is running its normal app
    DFU_APP_DETACH              = 1,//Device is running its normal app,has received the request,and is waiting for a usb reset
   
    DFU_DFU_IDLE                = 2,//Device is operating in the DFU mode and is waiting for request
    DFU_DFU_DN_SYNC             = 3,//Device has received a block and is waiting for the host to solicit the staus via DFU_GETSTATUS 
    DFU_DFU_DN_BUSY             = 4,//Device is programming a control-write block into its nonvolatile memories
    DFU_DFU_DN_IDLE             = 5,//Device is processing a download operation,Expecting DFU_DNLOAD request
    
    DFU_DFU_MANIFEST_SYNC       = 6,//Program memory function failed
    DFU_DFU_MANIFEST            = 7,//Program memory failed verification
    DFU_DFU_MANIFEST_WAIT_RESET = 8,//Can't Program memory due to received address that is out of range
    DFU_DFU_UP_IDLE             = 9,//Received DFU_DNLOAD with wLength=0,but device does't think it has all of the data yet
    DFU_DFU_ERROR               = 0x0A,//An error has occurred.Awaiting the DFU_CLRSTATUS 
    }
DFU_Run_State;//p22

typedef enum
{
    DFU_Req_DETACH    = 0,
    DFU_Req_DNLOAD    = 1,
    DFU_Req_UPLOAD    = 2,      //DFU_Req_TYPE_IN
    DFU_Req_GETSTATUS = 3,      //DFU_Req_TYPE_IN
    DFU_Req_CLRSTATUS = 4,      //p10
    DFU_Req_GETSTATE  = 5,      //DFU_Req_TYPE_IN
    DFU_Req_ABORT     = 6,
}
DFU_Req_State;



#define    DFU_Req_TYPE_IN    (USB_D2H|USB_DESC_TYPE_Functional_DFU)//0xA1
#define    DFU_Req_TYPE_OUT   (USB_H2D|USB_DESC_TYPE_Functional_DFU)//0x21


typedef struct{
	uint32_t    PollTimeOut:24;
	uint8_t     RunState:8;
}DFU_ACK_UN_ST;

#pragma pack(1)	
typedef struct{
	uint8_t    		ErrStatus;
	DFU_ACK_UN_ST	un;
    uint8_t     	StringIndex;    //Index of status description in string table
}DFU_ACK_ST;
#pragma pack () 


extern USBH_Status USBH_DFUSendSetup( USB_OTG_CORE_HANDLE *pdev,uint8_t *buff,uint32_t len);


#endif /* __USBH_HID_CORE_H */



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

