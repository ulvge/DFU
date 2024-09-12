/**
  ******************************************************************************
  * @file    usbh_dfu_core.h
  * @author  zb
  * @version V1.1.0
  * @date    2015-9-14
  * @brief   This file is the HID Layer Handlers for USB Host DFU class.
  *
  * @verbatim
  *      
  *          ===================================================================      
  *                                HID Class  Description
  *          =================================================================== 
  *           This module manages the MSC class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (HID) Version 1.11 Jun 27, 2001".
  *           This driver implements the following aspects of the specification:
  *             - The Boot Interface Subclass
  *             - The Mouse and Keyboard protocols
  *      
  *  @endverbatim
  *
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
#include "usbh_dfu_core.h"
#include "string.h"
#include "xprintf.h"
#include 	"include_slef.H"
#include 	"ucos_ii.h"

#if PRINTF_DFU_CORE
	#define DFU_core_xprintf( X)    do {xprintf X ;} while(0)
#else
	#define DFU_core_xprintf( X)      
#endif

	
void USBH_DFU_SetDelay(INT32U	Xms);
USBH_Status USBH_DFU_DownBin(USB_OTG_CORE_HANDLE *pdev,uint8_t *pSend,uint16_t lenth);
	
//#ifndef	Fireware
#if	1
uint8_t 	*Fireware 	= (uint8_t*)0x08010000;
uint32_t 	FirewareSize=354092;//0x0005672C 
#else
extern	const   char	Fireware;
extern  uint32_t 		FirewareSize;
#endif
//#include "usbh_hid_mouse.h"     //@should delete 
//#include "usbh_hid_keybd.h"

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
* @brief    This file includes HID Layer Handlers for USB Host HID class.
* @{
*/ 

/** @defgroup USBH_HID_CORE_Private_TypesDefinitions
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_HID_CORE_Private_Defines
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_HID_CORE_Private_Macros
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_HID_CORE_Private_Variables
* @{
*/
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN DFU_Machine_TypeDef        DFU_Machine __ALIGN_END ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
//@__ALIGN_BEGIN DFU_Report_TypeDef         DFU_Report __ALIGN_END ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN USB_Setup_TypeDef          DFU_Setup __ALIGN_END ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN USBH_HIDDesc_TypeDef       DFU_Desc __ALIGN_END ; 

__IO uint8_t start_toggle_dfu = 0;
/**
* @}
*/ 


/** @defgroup USBH_HID_CORE_Private_FunctionPrototypes
* @{
*/ 

static USBH_Status USBH_DFU_InterfaceInit  (USB_OTG_CORE_HANDLE *pdev , 
                                            void *phost);

static void USBH_DFU_InterfaceDeInit  (USB_OTG_CORE_HANDLE *pdev , 
                                       void *phost);

static USBH_Status USBH_DFU_Handle(USB_OTG_CORE_HANDLE *pdev , 
                                   void *phost);

static USBH_Status USBH_DFU_ClassRequest(USB_OTG_CORE_HANDLE *pdev , 
                                         void *phost);

static USBH_Status USBH_Get_DFU_ReportDescriptor (USB_OTG_CORE_HANDLE *pdev, 
                                                  USBH_HOST *phost,
                                                  uint16_t length);

static USBH_Status USBH_Get_DFU_Descriptor (USB_OTG_CORE_HANDLE *pdev,\
                                            USBH_HOST *phost,  
                                            uint16_t length);

static USBH_Status USBH_Set_Idle (USB_OTG_CORE_HANDLE *pdev, 
                                  USBH_HOST *phost,
                                  uint8_t duration,
                                  uint8_t reportId);

static USBH_Status USBH_Set_Protocol (USB_OTG_CORE_HANDLE *pdev, 
                                      USBH_HOST *phost,
                                      uint8_t protocol);


USBH_Class_cb_TypeDef  USBH_DFU_cb = 
{
  USBH_DFU_InterfaceInit,
  USBH_DFU_InterfaceDeInit,
  USBH_DFU_ClassRequest,
  USBH_DFU_Handle
};
/**
* @}brief
*/ 
    
DFU_ACK_ST  Dfu_Ack;


USBH_Status USBH_DFU_RxPara(USB_OTG_CORE_HANDLE *pdev,
                               USBH_HOST           *phost,                                
                               DFU_Req_State  request,
                                int16_t value_idx, //始终为0?
                               uint8_t* buff, 
                               uint16_t length )
{ 
    phost->Control.setup.b.bmRequestType = DFU_Req_TYPE_IN;
    phost->Control.setup.b.bRequest = (uint8_t)request;
    phost->Control.setup.b.wValue.w =  value_idx;
    phost->Control.setup.b.wIndex.w = 0;
    phost->Control.setup.b.wLength.w = length;   
    
    return USBH_CtlReq(pdev, phost, buff , length );     
}

USBH_Status USBH_DFU_TxPara(USB_OTG_CORE_HANDLE *pdev,
                               USBH_HOST           *phost,                                
                               DFU_Req_State  request,
                                int16_t value_idx, //始终为0?
                               uint8_t* Rxbuff, 
                               uint16_t Rxlength )
{ 
    phost->Control.setup.b.bmRequestType = DFU_Req_TYPE_OUT;
    phost->Control.setup.b.bRequest = (uint8_t)request;
    phost->Control.setup.b.wValue.w =  value_idx;
    phost->Control.setup.b.wIndex.w = 0;
    phost->Control.setup.b.wLength.w = Rxlength;   
    
    return USBH_CtlReq(pdev, phost, Rxbuff , Rxlength );     
}



/** @defgroup USBH_HID_CORE_Private_Functions
* @{
*/ 


static void  USBH_DFU_Parse_ACK (DFU_ACK_ST *pAck,uint8_t *buf)
{
    memcpy((uint8_t*)pAck,buf,sizeof(DFU_ACK_ST));
    //Dfu_Ack.un.b.PollTimeOut = LE24(Dfu_Ack.un.b.PollTimeOut);
}
typedef struct{
    uint8_t InitStep;   //下载初始化状态机
    uint8_t DownStep;   //下载状态机
    uint16_t LenPerPacket;   //下载单包长度  通常是最大值，最后一帧不足时小于最大值
    uint32_t IndexOfPacket;     //当前帧序号
    uint32_t Offset;     //连续传输时，每次相对Bin文件的偏移量
    uint32_t SizeOfBin;     //Bin的字节数
    uint32_t OccurTime;       //收到"连续两帧包之间需要延时"时，记录发生的时间,之后判断延时时间是否已经满足
    uint32_t DelayTick;     // 当前系统Tick时间，用来计算主动延时  主动：调试时，Host主动延时

    
        
    uint32_t remainingDataLength;
    uint32_t StallErrorCount;
    
}DFU_ST;

DFU_ST  DFU;

#define IsDelay_Busy 	0
#define IsDelay_OK 		1
#if 0
int32_t USBH_DFU_IsDelay(INT32U OccurTime)//单位:Tick
{
    static INT32U Cnt_PrintDot = 0,CurTick;
    INT32U num;
	INT32U DelayTick = (Dfu_Ack.un.PollTimeOut/SYSTICK_CYC)? (Dfu_Ack.un.PollTimeOut/SYSTICK_CYC)+1 : 0;
	
	num = RTC_SysTickOffSet_Update(&CurTick);
	
	if(num){
		if(DFU.DelayTick){
			DFU.DelayTick--;
			return false;
		}
		if((DelayTick == 0)||(RTC_SysTickOffSet(OccurTime) >= DelayTick)){
			Cnt_PrintDot = 0;
			//DFU_core_xprintf(("\n"));
			return true;
		}else{	
			return false;
		}
	}
	return false;
}
#else
int32_t USBH_DFU_IsDelay(INT32U OccurTime)//单位:Tick
{
    static INT32U CurTick;
    INT32U num;
	INT32U DelayTick = (Dfu_Ack.un.PollTimeOut/SYSTICK_CYC)? (Dfu_Ack.un.PollTimeOut/SYSTICK_CYC)+1 : 0;
	
	num = RTC_SysTickOffSet_Update(&CurTick);
	
	if(num){
		if(DFU.DelayTick){
			DFU.DelayTick--;
		}
	}
	if(DFU.DelayTick)	return false;
	if((DelayTick == 0)||(RTC_SysTickOffSet(OccurTime) >= DelayTick)){
		//DFU_core_xprintf(("\n"));
		return true;
	}
	return false;
}

#endif
void USBH_DFU_SetDelay(INT32U	Xms)
{
    DFU.DelayTick = (Xms/SYSTICK_CYC);
    //DFU.DelayTick = (Xms/SYSTICK_CYC)+1;
}
/**
* @brief   
*         The function init the DFU class.
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval  USBH_Status :Response for USB HID driver intialization
*/

//DFU extension protocol
//4.2.1 Introduction
//The DFU class uses the USB as a communication channel between the microcontroller and
//the programming tool, generally a PC host. The DFU class specification states that, all the
//commands, status and data exchanges have to be performed through Control Endpoint 0.
//The command set, as well as the basic protocol are also defined, but the higher level
//protocol (Data format, error message etc.) remain vendor-specific. This means that the DFU
//class does not define the format of the data transferred (.s19, .hex, pure binary etc.).
static uint8_t TxCmd[8]={0x21,0x01,0x00,0x00,0x00,0x00,0x00,0x00};
#include "usb_core.h"
#define END_POINT0_OUT  0
USBH_Status USBH_DFU_DownBin(USB_OTG_CORE_HANDLE *pdev,uint8_t *pSend,uint16_t lenth)
{
    URB_STATE URB_Status;
    static uint8_t *datapointer , *datapointer_prev;
    static uint32_t remainingDataLength;
    if(DFU.DownStep == 0){
        datapointer = pSend;
        remainingDataLength = lenth; 
        DFU.DownStep++;
        pdev->host.hc[0].toggle_out = 1; 
    }else{
      /* BOT DATA OUT stage */
      URB_Status = HCD_GetURB_State(pdev , END_POINT0_OUT);       
      if((URB_Status == URB_DONE))//||(URB_Status == URB_NOTREADY))
      {
        DFU.StallErrorCount = 0;
        //USBH_MSC_BOTXferParam.BOTStateBkp = USBH_MSC_BOT_DATAOUT_STATE;  
        if(remainingDataLength > USB_OTG_MAX_EP0_SIZE)  
        {
          USBH_DFUSendData (pdev,
                             datapointer, 
                             USB_OTG_MAX_EP0_SIZE 
                             );
                                    
          datapointer_prev = datapointer;
          datapointer = datapointer + USB_OTG_MAX_EP0_SIZE;
          
          remainingDataLength = remainingDataLength - USB_OTG_MAX_EP0_SIZE;
        }
        else if ( remainingDataLength == 0)
        {
          /* If value was 0, and successful transfer, then change the state */
          //USBH_MSC_BOTXferParam.BOTState = USBH_MSC_RECEIVE_CSW_STATE;
			
            return USBH_OK;
        }
        else
        {
          USBH_DFUSendData (pdev,
	                        datapointer, 
			                remainingDataLength 
			                );
          
          remainingDataLength = 0; /* Reset this value and keep in same state */   
        }      
      }
      
      else if(URB_Status == URB_NOTREADY)
      //else if(URB_Status == URB_NOTYET)
      {
        if(datapointer != datapointer_prev)
        {
          USBH_DFUSendData (pdev,
                             (datapointer - USB_OTG_MAX_EP0_SIZE), 
                             USB_OTG_MAX_EP0_SIZE );
        }
        else
        {
          USBH_DFUSendData (pdev,//
                             datapointer,
                             USB_OTG_MAX_EP0_SIZE );
        }
      }
      
      else if(URB_Status == URB_STALL)
      {
        
        //USBH_MSC_BOTXferParam.BOTStateBkp = USBH_MSC_RECEIVE_CSW_STATE;
        
          return USBH_FAIL;
      }
    }
    return USBH_BUSY;
}

static USBH_Status USBH_DFU_InterfaceInit ( USB_OTG_CORE_HANDLE *pdev, 
                                           void *phost)
{	
  USBH_HOST *pphost = phost;
    
  USBH_Status status = USBH_BUSY ;
  
  if(USBH_DFU_IsDelay(DFU.OccurTime) == IsDelay_Busy) return status;
	
	RTC_SysTickOffSet_Update(&DFU.OccurTime);
  if(pphost->device_prop.Itf_Desc[0].bInterfaceSubClass  == DEVICE_FIRMWARE_UPGRADE)//HID_BOOT_CODE
  {
    if(pphost->device_prop.Itf_Desc[0].bInterfaceProtocol == DFU_RUN_TIME)
    {
        switch(DFU.InitStep){              
            case 0:
                DFU.LenPerPacket    = 0x1000;
                DFU.IndexOfPacket   = 0;
                DFU.Offset          = 0;
                DFU.SizeOfBin       = FirewareSize;
                
                DFU.InitStep        = 10;
                break;
            case 10://Tx[A1 03 00 00 00 00 06 00 ]   //Rx[00 00 00 00 02 00 ]
                if(USBH_DFU_RxPara(pdev,phost,DFU_Req_GETSTATUS,0,pdev->host.Rx_Buffer,sizeof(Dfu_Ack)) == USBH_OK){
                    USBH_DFU_Parse_ACK(&Dfu_Ack,pdev->host.Rx_Buffer);
                    if(Dfu_Ack.ErrStatus == DFU_Err_OK){
                        switch(Dfu_Ack.un.RunState){
                            case DFU_APP_IDLE:
                            case DFU_DFU_IDLE:
                            case DFU_DFU_DN_SYNC:
                            case DFU_DFU_DN_BUSY:
                                //DFU_core_xprintf(("<<:DFU: Busy\n"));
                                //break;
                            case DFU_DFU_DN_IDLE:
                                //DFU_core_xprintf(("<<:DFU: No Err\n"));
                                 DFU.InitStep++;
                                break;
                                
                            case DFU_DFU_MANIFEST_WAIT_RESET:
                                DFU_core_xprintf(("<<:DFU: DFU_DFU_MANIFEST_WAIT_RESET\n"));
                                DFU_core_xprintf(("<<:DFU: DFU Send Succesed Realy!!!!!\n"));
								DFU.InitStep=20;
                                break;
                            case DFU_DFU_ERROR:
                                DFU.InitStep=30;
                                break;
							default:
								break;
                         }
                    }else{
                        DFU_core_xprintf(("<<:DFU: Init Err! Dfu_Ack.ErrStatus=%d\n",Dfu_Ack.ErrStatus));
                        switch(Dfu_Ack.ErrStatus){
                            case DFU_Err_TRGET:
                                DFU_core_xprintf(("<<:DFU: File is not targeted for use by this device\n"));
                                DFU.InitStep=20;
                                break;
                            case DFU_Err_FILE:
                                DFU_core_xprintf(("<<:DFU: File is for this device but fails some vendor-specific verification test\n"));
                                DFU.InitStep=20;
                                break;
                        }
                    }
                }
                break;
            case 11://Tx[A1 05 00 00 00 00 01 00 ]   //Rx[02 ]
                if(USBH_DFU_RxPara(pdev,phost,DFU_Req_GETSTATE,0,pdev->host.Rx_Buffer,1) == USBH_OK){
                    Dfu_Ack.un.RunState = pdev->host.Rx_Buffer[0];//状态只有这一个Byte
                    switch(Dfu_Ack.un.RunState){
                        case DFU_DFU_IDLE:
                            DFU_core_xprintf(("<<:DFU: DFU_DFU_IDLE\n"));
                            DFU.InitStep++;
                            break;
                        case DFU_DFU_DN_IDLE:
                            //DFU_core_xprintf(("<<:DFU: DFU_DFU_DN_IDLE\n"));
                            DFU.InitStep++;
                            break;
                        case DFU_DFU_DN_SYNC:
                            DFU.InitStep=10; 
                            //DFU_core_xprintf(("<<:DFU: DFU_DFU_DN_SYNC\n"));
                            break;
                        case DFU_DFU_MANIFEST_SYNC:
                            DFU.InitStep=10; 
                            DFU_core_xprintf(("<<:DFU: DFU_DFU_MANIFEST_SYNC\n"));
                            break;
                        case DFU_DFU_ERROR:
                            DFU.InitStep=30; 
                            DFU_core_xprintf(("<<:DFU: DFU_DFU_ERROR An error has occurred.Awaiting the DFU_CLRSTATUS\n"));
                            break;
                        default:
                            DFU_core_xprintf(("<<:DFU: DFU Not Idle! Dfu_Ack.RunState=%d\n",Dfu_Ack.un.RunState));                        
                            break;
                    }
                    
                }
                break;
            case 12://Tx[21 01 00 00 00 00 XX XX ]   //LE(XXXX)  = len
				TxCmd[1] = DFU_Req_DNLOAD;
				TxCmd[2] = DFU.IndexOfPacket&0xff;
				TxCmd[3] = (DFU.IndexOfPacket>>8)&0xff;
			
    			DFU.LenPerPacket=((DFU.SizeOfBin-DFU.Offset)/0x1000)?0x1000:((DFU.SizeOfBin-DFU.Offset)%0x1000);
				TxCmd[6] = DFU.LenPerPacket&0xff;
				TxCmd[7] = (DFU.LenPerPacket>>8)&0xff;
				DFU_core_xprintf(("<<:DFU: DFU_Send Cmd:[%h]",TxCmd,sizeof(TxCmd)));
				DFU_core_xprintf(("  IndexOfPacket=%l; DFU.SizeOfBin = [%x]; DFU.Offset=[%x]\n",DFU.IndexOfPacket,DFU.SizeOfBin,DFU.Offset));
                //if(USBH_DFU_TxPara(pdev,phost,DFU_Req_DNLOAD,DFU.IndexOfPacket,pdev->host.Rx_Buffer,DFU.LenPerPacket) == USBH_OK){
                if(USBH_DFUSendSetup(pdev,TxCmd,sizeof(TxCmd))== USBH_OK){
                    DFU.InitStep++;
                    DFU.IndexOfPacket++;
					DFU.DownStep = 0;
					if(DFU.LenPerPacket == 0){
						DFU_core_xprintf(("<<:DFU: DFU Send Succesed!!!!!\n"));
						DFU.InitStep=11;//根据查询状态来退出
					}
					USBH_DFU_SetDelay(2);
                }
                break;
            case 13://Tx[.......... ]
				if(1){
					//if(USBH_DFUSendData(pdev,(uint8_t*)&Fireware+DFU.Offset,USB_OTG_MAX_EP0_SIZE) == USBH_OK){
					if(USBH_DFU_DownBin(pdev,(uint8_t*)(Fireware+DFU.Offset),DFU.LenPerPacket) == USBH_OK){
						//0x1000 0x1000 ... 0x0001 0    //最后为0字节长度
    					DFU.Offset += DFU.LenPerPacket;
						DFU.InitStep=11;
                        DFU_core_xprintf(("<<:DFU: DFU Send one packed already**********************\n"));
					}
				}
                break;
			case 30://Tx[21 04 00 00 00 00 01 00 ]   //Rx[02 ] DFU_DFU_ERROR:clear
	                if(USBH_DFU_TxPara(pdev,phost,DFU_Req_CLRSTATUS,0,pdev->host.Rx_Buffer,1) == USBH_OK){
	                    DFU.InitStep=10; 
						break;  
					}
            case 20://OK
				    status = USBH_OK;
				    DFU.InitStep=0;
                break;
            default:
                break;
        }
    }
    start_toggle_dfu =0;
  }
  else
  {
	static INT32U Dly;
	if((RTC_SysTickOffSet_Update(&Dly)*SYSTICK_CYC) >= 1000){//连续调用多次，只执行一次
		pphost->usr_cb->DeviceNotSupported();   
	}
  }
  return status;
  
}



/**
* @brief   
*         The function DeInit the Host Channels used for the HID class.
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval None
*/
void USBH_DFU_InterfaceDeInit ( USB_OTG_CORE_HANDLE *pdev,
                               void *phost)
{	
   //USBH_HOST *pphost = phost;
  #if 0	//@
  if(HID_Machine.hc_num_in != 0x00)
  {   
    USB_OTG_HC_Halt(pdev, HID_Machine.hc_num_in);
    USBH_Free_Channel  (pdev, HID_Machine.hc_num_in);
    HID_Machine.hc_num_in = 0;     /* Reset the Channel as Free */  
  }
  
  if(HID_Machine.hc_num_out != 0x00)
  {   
    USB_OTG_HC_Halt(pdev, HID_Machine.hc_num_out);
    USBH_Free_Channel  (pdev, HID_Machine.hc_num_out);
    HID_Machine.hc_num_out = 0;     /* Reset the Channel as Free */  
  }
 
  start_toggle_dfu = 0;
#endif
}

/**
* @brief   
*         The function is responsible for handling HID Class requests
*         for HID class.
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval  USBH_Status :Response for USB Set Protocol request
*/
static USBH_Status USBH_DFU_ClassRequest(USB_OTG_CORE_HANDLE *pdev , 
                                         void *phost)
{   
    USBH_HOST *pphost = phost;
    
  USBH_Status status         = USBH_BUSY;
  USBH_Status classReqStatus = USBH_BUSY;
  
  
  return status; 
}


/**
* @brief   
*         The function is for managing state machine for HID data transfers 
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval USBH_Status
*/
static USBH_Status USBH_DFU_Handle(USB_OTG_CORE_HANDLE *pdev , 
                                   void   *phost)
{
  USBH_HOST *pphost = phost;
  USBH_Status status = USBH_OK;
   #if 0	//@
  switch (HID_Machine.state)
  {
    
  case HID_IDLE:
    HID_Machine.cb->Init();
    HID_Machine.state = HID_SYNC;
    
  case HID_SYNC:

    /* Sync with start of Even Frame */
    if(USB_OTG_IsEvenFrame(pdev) == TRUE)
    {
      HID_Machine.state = HID_GET_DATA;  
    }
    break;
    
  case HID_GET_DATA:

    USBH_InterruptReceiveData(pdev, 
                              HID_Machine.buff,
                              HID_Machine.length,
                              HID_Machine.hc_num_in);
    start_toggle_dfu = 1;
    
    HID_Machine.state = HID_POLL;
    HID_Machine.timer = HCD_GetCurrentFrame(pdev);
    break;
    
  case HID_POLL:
    if(( HCD_GetCurrentFrame(pdev) - HID_Machine.timer) >= HID_Machine.poll)
    {
      HID_Machine.state = HID_GET_DATA;
    }
    else if(HCD_GetURB_State(pdev , HID_Machine.hc_num_in) == URB_DONE)
    {
      if(start_toggle_dfu == 1) /* handle data once */
      {
        start_toggle_dfu = 0;
        HID_Machine.cb->Decode(HID_Machine.buff);
      }
    }
    else if(HCD_GetURB_State(pdev, HID_Machine.hc_num_in) == URB_STALL) /* IN Endpoint Stalled */
    {
      
      /* Issue Clear Feature on interrupt IN endpoint */ 
      if( (USBH_ClrFeature(pdev, 
                           pphost,
                           HID_Machine.ep_addr,
                           HID_Machine.hc_num_in)) == USBH_OK)
      {
        /* Change state to issue next IN token */
        HID_Machine.state = HID_GET_DATA;
        
      }
      
    }      
    break;
    
  default:
    break;
  }
#endif
  return status;
}


/**
* @brief  USBH_Get_HID_ReportDescriptor
*         Issue report Descriptor command to the device. Once the response 
*         received, parse the report descriptor and update the status.
* @param  pdev   : Selected device
* @param  Length : HID Report Descriptor Length
* @retval USBH_Status : Response for USB HID Get Report Descriptor Request
*/
static USBH_Status USBH_Get_DFU_ReportDescriptor (USB_OTG_CORE_HANDLE *pdev,
                                                  USBH_HOST *phost,
                                                  uint16_t length)
{
  
  USBH_Status status;
  
  status = USBH_GetDescriptor(pdev,
                              phost,
                              USB_REQ_RECIPIENT_INTERFACE
                                | USB_REQ_TYPE_STANDARD,                                  
                                USB_DESC_HID_REPORT, 
                                pdev->host.Rx_Buffer,
                                length);
  
  /* HID report descriptor is available in pdev->host.Rx_Buffer.
  In case of USB Boot Mode devices for In report handling ,
  HID report descriptor parsing is not required.
  In case, for supporting Non-Boot Protocol devices and output reports,
  user may parse the report descriptor*/
  
  
  return status;
}

/**
* @brief  
*         Issue HID Descriptor command to the device. Once the response 
*         received, parse the report descriptor and update the status.
* @param  pdev   : Selected device
* @param  Length : HID Descriptor Length
* @retval USBH_Status : Response for USB HID Get Report Descriptor Request
*/
static USBH_Status USBH_Get_DFU_Descriptor (USB_OTG_CORE_HANDLE *pdev,
                                            USBH_HOST *phost,
                                            uint16_t length)
{
  
  USBH_Status status;
  
  status = USBH_GetDescriptor(pdev, 
                              phost,
                              USB_REQ_RECIPIENT_INTERFACE
                                | USB_REQ_TYPE_STANDARD,                                  
                                USB_DESC_HID,
                                pdev->host.Rx_Buffer,
                                length);
 
  return status;
}

/**
* @brief  USBH_Set_Idle
*         Set Idle State. 
* @param  pdev: Selected device
* @param  duration: Duration for HID Idle request
* @param  reportID : Targetted report ID for Set Idle request
* @retval USBH_Status : Response for USB Set Idle request
*/
static USBH_Status USBH_Set_Idle (USB_OTG_CORE_HANDLE *pdev,
                                  USBH_HOST *phost,
                                  uint8_t duration,
                                  uint8_t reportId)
{
  
  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;
  
  
  phost->Control.setup.b.bRequest = USB_HID_SET_IDLE;
  phost->Control.setup.b.wValue.w = (duration << 8 ) | reportId;
  
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;
  
  return USBH_CtlReq(pdev, phost, 0 , 0 );
}



/**
* @brief  USBH_Set_Protocol
*         Set protocol State.
* @param  pdev: Selected device
* @param  protocol : Set Protocol for HID : boot/report protocol
* @retval USBH_Status : Response for USB Set Protocol request
*/
static USBH_Status USBH_Set_Protocol(USB_OTG_CORE_HANDLE *pdev,
                                     USBH_HOST *phost,
                                     uint8_t protocol)
{
  
  
  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;
  
  
  phost->Control.setup.b.bRequest = USB_HID_SET_PROTOCOL;
  
  if(protocol != 0)
  {
    /* Boot Protocol */
    phost->Control.setup.b.wValue.w = 0;
  }
  else
  {
    /*Report Protocol*/
    phost->Control.setup.b.wValue.w = 1;
  }
  
  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;
  
  return USBH_CtlReq(pdev, phost, 0 , 0 );
  
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


/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
