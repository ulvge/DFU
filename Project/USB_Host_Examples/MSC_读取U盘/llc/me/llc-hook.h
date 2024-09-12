/**
  ********************************  STM32F103ZE  *******************************
  **********************************  uC/OS-II  ********************************
  * @文件名     ： llc-hook.h
  * @作者       ： zb
  * @库版本     ： V1
  * @系统版本   ： V2.92
  * @文件版本   ： V1.0.01
  * @日期       ： 2015年10月20日
  * @摘要       ： 移植LLC时，为了保持格式一致，所用到的，和相应处理函数
  
  ******************************************************************************/
#ifndef  __APPTASK_H__
#define  __APPTASK_H__

//#include "typedef.h"

#ifndef APPTASK_LLC_HOOK
#define EXT_LLC_HOOK                   extern
#else
#define EXT_LLC_HOOK
#endif


#define     D_TST       0
#define     D_ERR       1
#define     D_INTERN    2
#define     D_API       3
#define     D_DBG       4




#define     GFP_KERNEL   0

#define     d_assert(x)      do{;            \
                                }while(x);  


EXT_LLC_HOOK    void d_fnstart(const char * fmt,...);
EXT_LLC_HOOK    void d_error(const char * fmt,...);
EXT_LLC_HOOK    void d_printf(const char * fmt,...);
EXT_LLC_HOOK    void* kmalloc(int PoolSize, int mode);








