/****************************************Copyright (c)****************************************************
**                            Shenzhen HONGMEN electronics Co.,LTD.
**
**                                 http://www.hong-men.com
**
**--------------File Info---------------------------------------------------------------------------------
** File Name          : TIMER.H
** Last modified Date : 2011-06-03
** Last Version       : V1.00
** Descriptions       : TIMER File
**
**--------------------------------------------------------------------------------------------------------
** Created By         : Michael.He
** Created date       : 2011-06-03
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
#ifndef _TIMER_H
#define _TIMER_H
#include "include_slef.H"


#if DBG_TIME
#define TIME_DEBUG   DPrint
#else
#define TIME_DEBUG
#endif

#ifndef  TIME_GLOBALS
#define EXT_TIME		extern 
#else 
#define EXT_TIME  
#endif 


#ifndef  OFF 
#define  OFF                   0
#endif

#ifndef  ON
#define  ON                    1
#endif
   
#define  TIMERNUM                10

/*****************************************************************
*                  DEFINE TIMER UNIT                             *
******************************************************************/
#define _TICK         1          //32MS
#define _MS(X)        _TICK , X/(1000/_SECOND)  //notic! -- x must meet x%5 = 0!
#define _SECOND       100	//30

/******************************************************************
*                  DEFINE TIMER STRUCTURE                         *
*******************************************************************/
typedef struct tmr_st {
    struct tmr_st    *tmr_next;                //point to next timer task
    INT32U           CycTime;		           //Record the time!
    INT32U           RunTime;                   //holdtime == 0 mean : the Overtime is OK
    void            (*tmrfunc)(void);          //when the time is OK, run the function!
}TIMER;

typedef unsigned char  BOOLEAN; 

EXT_TIME	void       SysTickIsr (void);
EXT_TIME	TIMER *    CreateTimer(void (*timerfunc)(void));
EXT_TIME	void       StartTimer(TIMER *tmr,INT32U Attrib, INT32U time);
EXT_TIME	void       StopTimer(TIMER *tmr);
EXT_TIME	BOOLEAN    TimerSwitch(TIMER *tmr);
EXT_TIME	void       TimerInit(void);

#endif  



