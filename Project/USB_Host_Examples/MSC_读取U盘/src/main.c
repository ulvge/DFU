/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V2.1.0
  * @date    19-March-2012
  * @brief   USB host MSC class demo main file
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
#include "usbh_core.h"
#include "usbh_usr.h"
#include "usbh_msc_core.h"
#include "xprintf.h"
#include "app_task.h"
#include "include_slef.H"


/** @addtogroup USBH_USER
* @{
*/

/** @defgroup USBH_USR_MAIN
* @brief This file is the MSC demo main file
* @{
*/ 

/** @defgroup USBH_USR_MAIN_Private_TypesDefinitions
* @{
*/ 
/**
* @}
*/ 

/** @defgroup USBH_USR_MAIN_Private_Defines
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_USR_MAIN_Private_Macros
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_USR_MAIN_Private_FunctionPrototypes
* @{
*/ 
/**
* @}
*/ 


/** @defgroup USBH_USR_MAIN_Private_Functions
* @{
*/ 

/** @defgroup debug something by ZB
* @{
*/ 

uint8_t debug(void)
{
	uint8_t	len = 0;
	#if 0
	char str[]="123456";
	static uint8_t	step=0;
	
	len = strlen(str);
	len += sizeof(str);
	switch(step){
        case 0:
        case 1:
            step++;
    	    LCD_DrawRect(LCD_PIXEL_HEIGHT/2,LCD_LINE_4,50,50);
    	    LCD_DrawCircle(LCD_PIXEL_HEIGHT/2+80,LCD_PIXEL_WIDTH/2+80,20);
            break;
        case 2:
            step++;
    	    LCD_DrawFullRect(LCD_LINE_4,LCD_PIXEL_HEIGHT/2,50,50);
    	    LCD_DrawFullCircle(LCD_PIXEL_HEIGHT/2+70,LCD_PIXEL_WIDTH/2+70,20);
            break;
        default:
            step = 0;
            break;
    }
    #endif    
	return len;	
}

/*******************************************************************************
* Function Name  : SysTick_Configuration
* Description    : Configure a SysTick Base time to 10 ms.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

void SysTick_Configuration(void)
{
	//SetSysClock();//已经2分频
	/* Setup SysTick Timer for 10 msec interrupts  */
	if (SysTick_Config((SystemCoreClock) / OS_TICKS_PER_SEC)) //SysTick配置函数
	{ 
		/* Capture error */ 
		xprintf(">Sys Err SysTick_Configuration: !!!\n");
		//while (1);
	}  
}
/**
* @brief  Main routine for MSC class application
* @param  None
* @retval int
*/



#include 	"UART.H"
#include 	"timer.H"
#include	"ucos_ii.h"

#define STARTUP_TASK_PRIO     4
#define STARTUP_TASK_STK_SIZE 80

static OS_STK starup_task_stk[STARTUP_TASK_STK_SIZE];

#include "app_task.h"
int main(void)
{
  __IO uint32_t i = 1;
  
    //xPrintfCom1_Init();//与USB IO冲突	
    xPrintfCom2_Init();//USART3
    #if  PRINTF_ME   
    USART_main(FIFO_Chan_USART);
    #endif
    xPrintfCom1_SysInfo();
	debug();
	
  
	//xprintf("\r\n!!!!!文件系统只支持短文件名 ,读文件时，文件名不能为中文，否则读出错follow_path()!!!");


	OSInit(); 
	
	//AppTaskStart(0);
	
    (void)OSTaskCreate(AppTaskStart,
                       (void *)0,
                       &starup_task_stk[STARTUP_TASK_STK_SIZE - 1],
                       STARTUP_TASK_PRIO);
					   
	OSStart();

}


#ifdef USE_FULL_ASSERT
/**
* @brief  assert_failed
*         Reports the name of the source file and the source line number
*         where the assert_param error has occurred.
* @param  File: pointer to the source file name
* @param  Line: assert_param error line source number
* @retval None
*/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  /* Infinite loop */
  while (1)
  {}
}
#endif


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
