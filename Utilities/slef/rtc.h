#include <stdint.h>

#ifndef    __RTC_H_
#define    __RTC_H_

#if  DBG_RTC
	#define RTC_DEBUG( X)   do {DPrint X ;} while(0)
#else
	#define RTC_DEBUG( X)    
#endif
		
#ifndef RTC_GLOBALS
	#define EXT_RTC                   extern
#else
	#define EXT_RTC
#endif
	
typedef struct {
	INT8U	second;
	INT8U	minute;
	INT8U	hour;
	INT8U	day;
	INT8U	month;
	INT8U	year;
	INT8U	week;
} RTC_ST;

typedef struct{
	INT32U Cnt;
	INT32U Sum;
	//INT8U	Run;
}SYS_TICK_ST;
EXT_RTC	SYS_TICK_ST Tick;
EXT_RTC	void RTC_DelayXms(INT32U dly);
EXT_RTC	void RTC_SysTickCount(void);
EXT_RTC INT32U RTC_SysTickIsReady(void);
EXT_RTC INT32U	RTC_SysTickOffSet_Update(INT32U* LastTick);
EXT_RTC INT32U	RTC_SysTickGetSum(void);
EXT_RTC INT32U	RTC_SysTickOffSet(INT32U LastTick);


#endif
