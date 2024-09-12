//include_slef.
#ifndef _INCLUDES_H_
#define _INCLUDES_H_

               
#define     PRINTF_DFU_CORE		1
#define     PRINTF_IO_REQ		1
#define     PRINTF_DBG_SHELL	1

#include	<string.h>
#include 	"stm32f2xx.h"
#include 	"UART.H"
#include 	"timer.H"
#include 	"lib.H"
#include 	"rtc.h"
#include 	"shell.h"
#include 	"app_task.H"


#ifndef FALSE
#define FALSE 		0
#endif
#ifndef TRUE
#define TRUE		1
#endif

#ifndef false
#define false 		0
#endif
#ifndef true
#define true		1
#endif


#define  KEY_SPACE                      0x20
#define  KEY_CR                        	0x0D
#define  KEY_LF                        	0x0A
#define  KEY_DEL                   		0x08      //Del!

#endif
