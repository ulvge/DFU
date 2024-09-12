/**
  ********************************  STM32F103ZE  *******************************
  **********************************  uC/OS-II  ********************************
  * @文件名     ： led_beep.h
  * @作者       ： Huang Fugui
  * @库版本     ： V3.5.0
  * @系统版本   ： V2.92
  * @文件版本   ： V1.0.01
  * @日期       ： 2013年6月22日
  * @摘要       ： app_task应用程序任务的头文件
  ******************************************************************************/
#ifndef  __APPTASK_H__
#define  __APPTASK_H__

//#include "typedef.h"

#ifndef APPTASK_GLOBALS
#define EXT_APPTASK                   extern
#else
#define EXT_APPTASK
#endif

/* 包含的头文件 --------------------------------------------------------------*/
#include "ucos_ii.h"

/* 函数申明 ------------------------------------------------------------------*/
EXT_APPTASK void AppTask_USB(void *p_arg);
EXT_APPTASK void AppTask2_Shell(void *p_arg);
EXT_APPTASK void AppTask3_Debug(void *p_arg);

EXT_APPTASK void AppTaskStart(void *p_arg);
EXT_APPTASK	void	App_TaskIdleHook	(void);



/* 宏定义 --------------------------------------------------------------------*/
/*
*********************************************************************************************************
*                                            任务优先级
*********************************************************************************************************
*/
    
//#define  OS_TASK_TMR_PRIO                       2u

#define TASK_START_PRIO                        3
#define TASK1_PRIO                             5
#define TASK2_PRIO                             6
#define TASK3_PRIO                             14	//15:统计;16;idle

/*
*********************************************************************************************************
*                                            任务堆栈大小
*********************************************************************************************************
*/
#define TASK_START_STK_SIZE                    64
#define TASK1_STK_SIZE                         256
#define TASK2_STK_SIZE                         256
#define TASK3_STK_SIZE                         256


/* 任务堆栈变量 */
EXT_APPTASK OS_STK TaskStartStk[TASK_START_STK_SIZE];
EXT_APPTASK OS_STK Task1_Stk[TASK1_STK_SIZE];
EXT_APPTASK OS_STK Task2_Stk[TASK2_STK_SIZE];
EXT_APPTASK OS_STK Task3_Stk[TASK3_STK_SIZE];


EXT_APPTASK    OS_EVENT        *OSSem_USBDly;
EXT_APPTASK    OS_EVENT        *OSSem_UCOMM;
EXT_APPTASK    OS_EVENT        *OSSem_Shell;



#endif /* _APP_TASK_H */

/*** Copyright (C) 2011 - 2013 KeyShare. All Rights Reserved ***END OF FILE***/
