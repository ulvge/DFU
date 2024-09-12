/**
  ********************************  STM32F103ZE  *******************************
  **********************************  uC/OS-II  ********************************
  * @file    ： app_task.c
  * @author  KeyShare Team
  * @version V0.0.1
  * @date    2015-8-28
  * @brief   Main program body
  ******************************************************************************/
#define		APPTASK_GLOBALS 

/* 包含的头文件 --------------------------------------------------------------*/

#ifndef  OS_MASTER_FILE
#include <ucos_ii.h>
#endif
#include "app_task.h"
#include <ucos_ii.h>


#include "usbh_core.h"
#include "usbh_usr.h"
#include "usbh_msc_core.h"
#include "usbh_dfu_core.h"
#include "include_slef.H"

/* 全局变量 ------------------------------------------------------------------*/
OS_STK TaskStartStk[TASK_START_STK_SIZE];
OS_STK Task1_Stk[TASK1_STK_SIZE];
OS_STK Task2_Stk[TASK2_STK_SIZE];


typedef struct{
    INT8U App1_Cnt;
    INT8U App2_Cnt;
}APP_ST;

APP_ST App;


void	App_TaskIdleHook	(void)
{
	if(RTC_SysTickIsReady()){
		static INT32U Cnt;
		if(++Cnt >= OS_TICKS_PER_SEC){		
			Cnt = 0;
			STM_EVAL_LEDToggle(LED1);
			//STM_EVAL_LEDToggle(LED2);
			//STM_EVAL_LEDToggle(LED3);
			//STM_EVAL_LEDToggle(LED4);
			//xprintf(">sys: Tick: [%l]\n!",RTC_SysTickGetSum());
		}
	}
}

/** @defgroup USBH_USR_MAIN_Private_Variables
* @{
*/
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN USB_OTG_CORE_HANDLE      USB_OTG_Core __ALIGN_END;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN USBH_HOST                USB_Host __ALIGN_END;
/**
* @}
*/ 

/**************************************************
函数名称 ： AppTask_USB
功    能 ： 应用任务1
参    数 ： p_arg --- 可选参数
返 回 值 ： 无
作    者 ： Huang Fugui
***************************************************/
void AppTask_USB(void *pdata)
{
    uint32_t i = 1;//debug
	INT8U err;
	pdata = pdata;
	
  /* Init Host Library */
	if(i){
	  USBH_Init(&USB_OTG_Core,USB_OTG_FS_CORE_ID,&USB_Host,
				&USBH_MSC_cb, 
				//&HID_KEYBRD_cb, 
				//&HID_MOUSE_cb, 
				//&USBH_DFU_cb, 
				&USR_cb);
	}else{
	  USBH_Init(&USB_OTG_Core,USB_OTG_FS_CORE_ID,&USB_Host,
			  //&USBH_MSC_cb, 
			  //&HID_KEYBRD_cb, 
			  //&HID_MOUSE_cb, 
			  &USBH_DFU_cb, 
			  &USR_cb);
	}
	
    while (1) 
	{
        OSSemPend(OSSem_USBDly, 1, &err);
        /* Host Task handler */
        USBH_Process(&USB_OTG_Core, &USB_Host);
        
		if(err == OS_ERR_NONE){		
			App.App1_Cnt++;
		}
		else if(err == OS_ERR_TIMEOUT){
			App.App1_Cnt++;
		}
    }
}

/**************************************************
函数名称 ： AppTask2_Shell
功    能 ： 串口输入命令，解析执行，输出
参    数 ： p_arg --- 可选参数
返 回 值 ： 无
作    者 ： Huang Fugui
***************************************************/
void AppTask2_Shell(void *pdata)
{
	INT8U err;
	pdata = pdata;

	SHELL_init();
    while (1) 
	{
        OSSemPend(OSSem_Shell, 100, &err);
		if(err == OS_ERR_NONE){
	        SHELL_TestProcess();
		}
		else if(err == OS_ERR_TIMEOUT){
			App.App2_Cnt++;
		}
		//OSTimeDly(200);
    }
}
void time1_callback(void *ptmr, void *parg)
{
	STM_EVAL_LEDToggle(LED2);
}
void time2_callback(void *ptmr, void *parg)
{
	STM_EVAL_LEDToggle(LED3);
}
/**************************************************
函数名称 ： AppTask3_Debug
功    能 ： 调试学习所用
参    数 ： p_arg --- 可选参数
返 回 值 ： 无
作    者 ： Huang Fugui
***************************************************/
void AppTask3_Debug(void *pdata)
{
	INT8U err;
	OS_TMR *time;
	pdata = pdata;

    time = OSTmrCreate(Tmr_Xs(1), Tmr_Xms(200), OS_TMR_OPT_ONE_SHOT, time1_callback, NULL, "time1", &err);
	OSTmrStart(time, &err); //OS_TMR_CFG_TICKS_PER_SEC
    time = OSTmrCreate(Tmr_Xs(5), Tmr_Xms(500), OS_TMR_OPT_PERIODIC, time2_callback, NULL, "time2", &err);
	OSTmrStart(time, &err); 
    while (1) 
	{
		OSTimeDly(200);
    }
}


/**************************************************
函数名称 ： OSTick_Init
功    能 ： 操作系统滴答时钟初始化
参    数 ： 无
返 回 值 ： 无
作    者 ： 
***************************************************/
#include "stm32f2xx.h"
void OSTick_Init(void)
{
  RCC_ClocksTypeDef RCC_ClocksStructure;
  RCC_GetClocksFreq(&RCC_ClocksStructure);  //获取系统时钟频率
  /* 初始化并启动SysTick和它的中断 */
  SysTick_Config(RCC_ClocksStructure.HCLK_Frequency / OS_TICKS_PER_SEC);
}



/**************************************************
函数名称 ： Startup_Task
功    能 ： 启动任务
参    数 ： p_arg --- 可选参数
返 回 值 ： 无
作    者 ： Huang Fugui
***************************************************/
void AppTaskStart(void *p_arg)
{
	(void)p_arg;
	OSTick_Init();     //初始化滴答时钟
    
    //SysTick_Configuration();

	#if (OS_TASK_STAT_EN > 0)
	OSStatInit();    //CPU使用率
	#endif
	/* 创建任务1 */
	OSTaskCreateExt((void (*)(void *)) AppTask_USB,
				  (void           *) 0,
				  (OS_STK         *)&Task1_Stk[TASK1_STK_SIZE-1],
				  (INT8U           ) TASK1_PRIO,
				  (INT16U          ) TASK1_PRIO,
				  (OS_STK         *)&Task1_Stk[0],
				  (INT32U          ) TASK1_STK_SIZE,
				  (void           *) 0,
				  (INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));

	/* 创建任务2 */
	OSTaskCreateExt((void (*)(void *)) AppTask2_Shell,
				  (void           *) 0,
				  (OS_STK         *)&Task2_Stk[TASK2_STK_SIZE-1],
				  (INT8U           ) TASK2_PRIO,
				  (INT16U          ) TASK2_PRIO,
				  (OS_STK         *)&Task2_Stk[0],
				  (INT32U          ) TASK2_STK_SIZE,
				  (void           *) 0,
				  (INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));
				  
	/* 创建任务3 */
	OSTaskCreateExt((void (*)(void *)) AppTask3_Debug,
				  (void           *) 0,
				  (OS_STK         *)&Task3_Stk[TASK3_STK_SIZE-1],
				  (INT8U           ) TASK3_PRIO,
				  (INT16U          ) TASK3_PRIO,
				  (OS_STK         *)&Task3_Stk[0],
				  (INT32U          ) TASK3_STK_SIZE,
				  (void           *) 0,
				  (INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));

  
	OSSem_USBDly     = OSSemCreate(0);
	//OSSem_UCOMM      = OSSemCreate(0);
	OSSem_Shell      = OSSemCreate(0);
	
	OSTaskDel(OS_PRIO_SELF);
}

/*** Copyright (C) 2011 - 2013 HuangFugui. All Rights Reserved ***END OF FILE***/
