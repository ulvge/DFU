/****************************************Copyright (c)****************************************************
**                            Shenzhen HONGMEN electronics Co.,LTD.
**
**                                 http://www.hong-men.com
**
**--------------File Info---------------------------------------------------------------------------------
** File Name          : TIMER.C
** Last modified Date : 2011-06-02
** Last Version       : V1.00
** Descriptions       : TIMER File
**
**--------------------------------------------------------------------------------------------------------
** Created By         : Michael.He
** Created date       : 2011-06-02
** Version            : V1.00
** Descriptions       : First version
**
**--------------------------------------------------------------------------------------------------------
** Modified by        :        
** Modified date      :       
** Version            :             
** Descriptions       :       
**
*********************************************************************************************************/
#define TIME_GLOBALS 
#include "include_slef.H"

//================ DEFINE TIMER PARAMETERS =================//       
//#define  TIMERNUM                3
static TIMER  timertcb[TIMERNUM];
static TIMER  *tmrlist,*tmrfreelist;

//===============================================================================================================//
// Function name:           TimerEntry()
// Descriptions:            初始化T0后，每次T0中断进入查询是否有定时函数
// input parameters:        none
// output parameters:       none
// Returned value:          none
// Created by:              michael.he
// Created Date:            2011-06-03
//---------------------------------------------------------------------------------------------------------------//
// Modified by:
// Modified date:
//===============================================================================================================//
static void TimerEntry(void)
{
    void (*tmrfunc)(void);
    TIMER  *tmr;
	
    if (tmrlist == NULL) return;
    tmr = tmrlist;
    do 
	{
       if (tmr->CycTime != 0) 
	   {
          if (--tmr->RunTime == 0) 
		  {
             tmr->RunTime = tmr->CycTime;
             tmrfunc = tmr->tmrfunc;
             if(tmrfunc != NULL) 
             {
			 	(*tmrfunc)();
			 }
          }
       }
       tmr = tmr->tmr_next;
    }
	 while (tmr != NULL);
}


//Notice! Is add OSIntEnter & OSIntExit, the timer handle of whole peoject must clear ...
//So i clear it..
void SysTickIsr (void)
{
   TimerEntry();
}


static void Timer1Init (void)
{			 
	//5mS中断1次            
}

TIMER *CreateTimer(void (*timerfunc)(void))
{
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif
    TIMER *tmr;

    tmr = tmrfreelist;
    if (tmr != NULL) 
	{
       if (tmr == NULL) 
	   {
          return NULL;
       } 
	   else 
	   {
          OS_ENTER_CRITICAL();
          tmrfreelist = tmr->tmr_next;  //updata freelist point make it point to next 
          tmr->tmr_next = tmrlist;      //below 2 lines add new semhpore to list
          tmrlist = tmr;
     	  tmr->tmrfunc = timerfunc;
     	  tmr->CycTime = 0;
     	  OS_EXIT_CRITICAL();
    	  return tmr;
     	}
    } 
    else 
	{
       return NULL;
    }
}

void StartTimer(TIMER *tmr,INT32U Attrib, INT32U time)
{
    if (tmr == NULL)      return;
    if (tmrlist == NULL)  return;
    tmr->CycTime = Attrib * time;
    tmr->RunTime   = tmr->CycTime;
}


void StopTimer(TIMER *tmr)
{
    if (tmr == NULL)      return;
    if (tmrlist == NULL)  return;
    tmr->CycTime  = 0;
    tmr->RunTime    = 0;
}

//===============================================================================================================//
// Function name:           TimerSwitch(TIMER *tmr)  
// Descriptions:            查询某个定时任务是否已经开启
// input parameters:        TIMER *tmr
// output parameters:       none
// Returned value:          1 - ON      0 - OFF
// Created by:              michael.he
// Created Date:            2011-06-03
//---------------------------------------------------------------------------------------------------------------//
// Modified by:
// Modified date:
//===============================================================================================================//
BOOLEAN TimerSwitch(TIMER *tmr)   
{
    if (tmr == NULL)      return OFF;
    if (tmrlist == NULL)  return OFF;
    
    if (tmr->CycTime > 0) 
    {
       return ON;
    } 
    else 
    {
       return OFF;
    }   
}	

void TimerInit(void)
{
    INT8U   i;
    for (i=0;i<TIMERNUM-1;i++) 
	{
       timertcb[i].tmr_next = &timertcb[i+1];
       timertcb[i].CycTime = 0x0;
       timertcb[i].RunTime   = 0x0;
       timertcb[i].tmrfunc  = NULL;
    }
    timertcb[i].tmr_next = NULL;
    tmrfreelist          = &timertcb[0];
    tmrlist              = NULL;
    Timer1Init();
}



