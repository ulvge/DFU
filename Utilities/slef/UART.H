/****************************************Copyright (c)****************************************************
**
*********************************************************************************************************/

#ifndef _UART_H_
#define _UART_H_


#ifndef UART_GLOBALS
#define   EXT_UART     extern
#else 
#define   EXT_UART
#endif


#include <stdint.h>

#if 0
#define OS_ENTER_CRITICAL()	__disable_irq()	  
#define OS_EXIT_CRITICAL()	__enable_irq()	
#else
#include "../uCOS-II/Ports/os_cpu.h"
#endif
#define OS_IntEnter()		__disable_irq()	  
#define OS_IntExit()		__enable_irq()

//FIFO_CHAN_ENUM 为FIFO_Buf[]数组中的下标
//FIFO_SendData


typedef enum{//有可能需要修改	FIFO_SendData
	FIFO_Chan_ESP = 1,      
	FIFO_Chan_BEEP = 2,  
	FIFO_Chan_USART = 3,
}FIFO_CHAN_ENUM;                

#define   DBG_UART           FIFO_Chan_USART	

typedef struct {
    INT16U      deepth;
    INT16U      occupy;
    INT8U       *array;
    INT8U       *limit;
    INT8U       *wp;
    INT8U       *rp;
} FIFO; 	     

typedef struct {
    INT32U          status;
    FIFO            sfifo;                    
    FIFO            rfifo;                    
}FIFO_Buf_STRUCT;	




//标准异步模式 : 8 bits , 1 stop , no parity 
#define UART_SENDING                0x01
#define UART_FLOWCTR                0x02
#define UART_INIT                   0x80

/******************************************************************
	USART
******************************************************************/

INT32U	Radix_swap_int32(INT32U temp);
INT16U	USART_AsciiToHex(INT8U *dptr, INT8U *sptr, INT16U len);
BOOLEAN	USART_received(INT8U chan);
INT8U	USART_read(INT8U chan);
/******************************************************************
	FIFO
******************************************************************/
#define		FIFO_NUM_TOTAL		4


EXT_UART	FIFO_Buf_STRUCT		FIFO_Buf[FIFO_NUM_TOTAL];
			
EXT_UART	BOOLEAN 		USART_main(INT8U chan);  
EXT_UART	void		FIFO_Init(FIFO *fifo, INT8U *array, INT16U deepth);
EXT_UART	void		FIFO_Reset(FIFO *fifo);
EXT_UART	BOOLEAN		FIFO_Write(FIFO *fifo, INT8U unit);
EXT_UART	BOOLEAN		FIFO_Writes(FIFO *fifo, INT8U *units, INT16U unitsize);
EXT_UART	BOOLEAN		FIFO_Empty(FIFO *fifo);
EXT_UART	INT8U		FIFO_Read(FIFO *fifo);
EXT_UART	INT8U 		FIFO_SendData(INT8U chan);
EXT_UART	void		USART1_IRQHandler_ISR(void);
EXT_UART	void		DPrint(const char *fmt, ...);

EXT_UART	INT8U 	USART_print_byte(INT8U chan, INT8U ch);


/******************************************************************
	FILO
******************************************************************/
typedef struct {
    INT16U      deepth;
    INT8U       *array;
    INT8U       *wp;    
} FILO;

EXT_UART	void	FILO_Init(FILO *filo, INT8U *array, INT16U deepth);
EXT_UART	void	FILO_Reset(FILO *filo);
EXT_UART	INT8U*  FILO_StartPos(FILO *filo);
EXT_UART	BOOLEAN FILO_IsEmpty(FILO *filo);
EXT_UART	BOOLEAN FILO_IsFull(FILO *filo);
EXT_UART	INT8U	FILO_Read(FILO *filo);
EXT_UART	BOOLEAN FILO_Write(FILO *filo, INT8U unit);
EXT_UART	INT16U  FILO_Occupy(FILO *filo);

#endif
