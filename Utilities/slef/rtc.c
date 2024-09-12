#define RTC_GLOBALS

 
#include "include_slef.H"

void RTC_DelayXms(INT32U dly)
{
	INT32U i,j;
	for(i=0;i<dly;i++)
	{
		for(j=0;j<2450;j++); 
	}
}

void RTC_SysTickCount(void)
{
	Tick.Cnt++;
	Tick.Sum++;
}
//连续调用时，会一直返回0
//避免一直刷新显示
INT32U	RTC_SysTickOffSet_Update(INT32U* LastTick)
{
	INT32U Res = Tick.Sum - *LastTick;

	*LastTick = Tick.Sum;
	return Res;
}
//return:how many tick has offsete
INT32U	RTC_SysTickOffSet(INT32U LastTick)
{
	INT32U Res = Tick.Sum - LastTick;
	return Res;
}


INT32U RTC_SysTickIsReady(void)
{
	if(Tick.Cnt) {
		Tick.Cnt--;
		return true;
	}
	return false;
}
INT32U	RTC_SysTickGetSum(void)
{
	return Tick.Sum;
}
