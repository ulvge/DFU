/****************************************Copyright (c)****************************************************
**                            Shenzhen HONGMEN electronics Co.,LTD.
**
**                                 http://www.hong-men.com
**
**--------------File Info---------------------------------------------------------------------------------
** File Name          : shell.h
** Last modified Date : 2011-04-13
** Last Version       : V1.00
** Descriptions       : SHELL File
**
**--------------------------------------------------------------------------------------------------------
** Created By         : Michael.He
** Created date       : 2011-04-13
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
#ifndef  __SHELL_H__
#define  __SHELL_H__

#if  PRINTF_DBG_SHELL
#define SHELL_DEBUG( X)   do {DPrint X ;} while(0)
#else
#define SHELL_DEBUG( X)    
#endif

#ifndef SHELL_GLOBALS
#define EXT_SHELL                   extern
#else
#define EXT_SHELL
#endif


#define  KEY_SPACE                       0x20
#define  KEY_CR                        0x0D
#define  KEY_LF                        0x0A
#define  KEY_DEL                   0x08      //ик???---Del!


EXT_SHELL	void SHELL_init(void);
EXT_SHELL	void SHELL_TestProcess(void);
#endif 

