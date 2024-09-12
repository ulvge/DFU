/****************************************Copyright (c)****************************************************

*********************************************************************************************************/
#define UART_GLOBALS
//#include "config.h"

#include "include_slef.H"
#include "stdarg.h"



//#define UART_BUF_ADDR 0x2007C000 // AHB BANK 1
#if  0//DBG_SYS>0 //0x2008 2800   --  0x2008 3400 (2008 0000) 3K
	//#define UART_BUF_ADDR 0x20083000 // AHB BANK 1
	#define UART_BUF_ADDR 0x20082800 // AHB BANK 1 
	#define UART_TET_SIZE 512
	#define UART_BUF_SIZE       1024
	
	static  INT8U        s_buf0[UART_BUF_SIZE]   __at  (UART_BUF_ADDR + UART_BUF_SIZE*0 ) ;
	static  INT8U        r_buf0[UART_BUF_SIZE]   __at  (UART_BUF_ADDR + UART_BUF_SIZE*1 ) ;
	static  INT8U        s_buf1[UART_BUF_SIZE]   __at  (UART_BUF_ADDR + UART_BUF_SIZE*2 ) ;
	static  INT8U        r_buf1[UART_BUF_SIZE]   __at  (UART_BUF_ADDR + UART_BUF_SIZE*3 ) ;
	static  INT8U        s_buf2[UART_TET_SIZE]   __at  (UART_BUF_ADDR + UART_BUF_SIZE*4 + UART_TET_SIZE*0) ;
	static  INT8U        r_buf2[UART_TET_SIZE]   __at  (UART_BUF_ADDR + UART_BUF_SIZE*4 + UART_TET_SIZE*1) ;
	static  INT8U        s_buf3[UART_TET_SIZE]   __at  (UART_BUF_ADDR + UART_BUF_SIZE*4 + UART_TET_SIZE*2) ;
	static  INT8U        r_buf3[UART_TET_SIZE]   __at  (UART_BUF_ADDR + UART_BUF_SIZE*4 + UART_TET_SIZE*3) ;

#endif

						
#define SendCmdBuf_size 	1024
static INT8U s_FIFO_usart[SendCmdBuf_size];	 
static INT8U r_FIFO_usart[200];

void USART1_TX_DIS(void);
void USART1_TX_EN(void);
void USART_TX_Empty(USART_TypeDef* USARTx,FunctionalState NewState);

/************************************************************************************************************
   FIFO  底层接口层
******************************************************************/
void FIFO_Init(FIFO *fifo, INT8U *array, INT16U deepth)
{
    fifo->deepth    = deepth;
    fifo->occupy    = 0;
    fifo->array     = array;
    fifo->limit     = array + deepth;
    fifo->wp = fifo->array;
    fifo->rp = fifo->array;
}

void FIFO_Reset(FIFO *fifo)
{
    fifo->occupy = 0;
    fifo->rp = fifo->array;
    fifo->wp = fifo->array;
}

BOOLEAN FIFO_Write(FIFO *fifo, INT8U unit)
{
    
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
        OS_CPU_SR  cpu_sr = 0u;
#endif

    if (fifo->occupy >= fifo->deepth) return FALSE;
    OS_ENTER_CRITICAL();
    *fifo->wp++ = unit;
    if (fifo->wp >= fifo->limit) fifo->wp = fifo->array;

    //OS_ENTER_CRITICAL();
    fifo->occupy++;
    OS_EXIT_CRITICAL();
    return TRUE;
}

BOOLEAN FIFO_Writes(FIFO *fifo, INT8U *units, INT16U unitsize)
{
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
        OS_CPU_SR  cpu_sr = 0u;
#endif
    if (unitsize > fifo->deepth - fifo->occupy) return FALSE;
    for (; unitsize > 0; unitsize--) { 
       OS_ENTER_CRITICAL();
       *fifo->wp++ = *units++;
       if (fifo->wp >= fifo->limit) fifo->wp = fifo->array;
//       OS_ENTER_CRITICAL();
       fifo->occupy++;
       OS_EXIT_CRITICAL();
    }    
    return TRUE;
}

BOOLEAN FIFO_Empty(FIFO *fifo)
{
    if (fifo->occupy == 0) return true;
    else return false;
}    
 
INT8U FIFO_Read(FIFO *fifo)
{
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
        OS_CPU_SR  cpu_sr = 0u;
#endif
    int ret;
    if (fifo->occupy == 0) return 0xFF;
    OS_ENTER_CRITICAL();
    ret = *fifo->rp++;
    if (fifo->rp >= fifo->limit) fifo->rp = fifo->array;

//    OS_ENTER_CRITICAL();
    fifo->occupy--;
    OS_EXIT_CRITICAL();
    return ret;
}

INT8U FIFO_SendData(INT8U chan)
{
	if(!FIFO_Empty(&FIFO_Buf[chan].sfifo))
	{						  
		FIFO_Buf[chan].status |= UART_SENDING; //一定要放在TX_EN之前，因为是先判断SENDING
		if(chan == 1)		{	
			USART1->DR = FIFO_Read(&FIFO_Buf[chan].sfifo);	//sending data
			USART_TX_Empty(USART1,ENABLE);
		}
		else if(chan == 2){//FIFO_Chan_USART
			USART2->DR = FIFO_Read(&FIFO_Buf[chan].sfifo);	//sending data
			USART_TX_Empty(USART2,ENABLE);
		}
		else if(chan == 3){//FIFO_Chan_USART
			USART3->DR = FIFO_Read(&FIFO_Buf[chan].sfifo);	//sending data
			USART_TX_Empty(USART3,ENABLE);
		}
		
		return true;
	}
	return false ;
}



/******************************************************************
   FILO
******************************************************************/
void  FILO_Init(FILO *filo, INT8U *array, INT16U deepth)
{
    filo->deepth    = deepth;
    filo->array     = array;
    filo->wp = filo->array;
}
    
void  FILO_Reset(FILO *filo)
{
    filo->wp = filo->array;
}

INT8U*  FILO_StartPos(FILO *filo)
{
    return filo->array;
}    

BOOLEAN FILO_IsEmpty(FILO *filo)
{
    if (filo->wp == filo->array) return true;
    else return false;
}    
BOOLEAN FILO_IsFull(FILO *filo)
{
    if (filo->wp >= filo->array + filo->deepth) return true;
    else return false;
}    
    
BOOLEAN FILO_Write(FILO *filo, INT8U unit)
{
    if (FILO_IsFull(filo)) return false;
    else {
       *filo->wp = unit;
       filo->wp++;
       return true;
    }
}
    
INT8U  FILO_Read(FILO *filo)
{
    if (FILO_IsEmpty(filo)) return 0xff;
    filo->wp--;
    return *filo->wp;
}

INT16U  FILO_Occupy(FILO *filo)
{
    return filo->wp - filo->array;
}


//----------------------------------------------------------------
// Function name     :USART_print_byte
// Descriptions      :打印一字节
// input parameters  :UART口num  
// output parameters :无   
// Returned value    :TRUE   成功
//                    FALSE  失败         
//-----------------------------------------------------------------
INT8U USART_print_byte(INT8U chan, INT8U ch)
{
	if(FIFO_Write(&FIFO_Buf[chan].sfifo,ch)){
		if((FIFO_Buf[chan].status & UART_SENDING)  == false){
			FIFO_Buf[chan].status |= UART_SENDING;
			FIFO_SendData(chan);
		}
		return true;
	}
	return false;
}


//----------------------------------------------------------------
// Function name     :USART_print_bytehex
// Descriptions      :打印16进制数据
// input parameters  :UART口num  
// output parameters :无   
// Returned value    :TRUE   成功
//                    FALSE  失败         
//-----------------------------------------------------------------
void USART_print_bytehex(INT8U chan, INT8U ch)
{
    USART_print_byte(chan, Radix_HexToChar(ch >> 4));     //打印高位
    USART_print_byte(chan, Radix_HexToChar(ch));          //打印低位
    USART_print_byte(chan, ' ');                    //空格
}

void USART_print_mem(INT8U chan, INT8U *mem, INT16U memsize)
{
    INT16U i;
  
    for (i = 0; i < memsize; i++) USART_print_byte(chan, *mem++);
}


void USART_print_memhex(INT8U chan, INT8U *mem, INT16U memsize)
{
  
    for (; memsize > 0; memsize--)
        USART_print_bytehex(chan, *mem++);
}

void USART_print_string(INT8U chan, const char *str)
{
    while(*str)
    {
        if (*str == '\n')
		{
            USART_print_byte(chan, KEY_CR);
            USART_print_byte(chan, KEY_LF);
            str++;
        } 
		else 
		{
            USART_print_byte(chan, *str++);
        }
    }
}

void  USART_sprint_byte(INT8U chan, INT8U ch)
{
    if (ch == '\n') 
	{
       USART_print_byte(chan, KEY_CR);
       USART_print_byte(chan, KEY_LF);
    } 
	else 
	{
       USART_print_byte(chan,ch);
    }
} 
BOOLEAN USART_received(INT8U chan)
{
    return !FIFO_Empty(&FIFO_Buf[chan].rfifo);
}

INT8U USART_read(INT8U chan)
{
    return FIFO_Read(&FIFO_Buf[chan].rfifo);
}
/*
EXP.  %m  &   %h
exp[8] = {0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32};
printf("exp = %m\n",exp,sizeof(exp));  ====>  exp = 22222222
printf("exp = %h\n",exp,sizeof(exp));  ====>  exp = 32 32 32 32 32 32 32 32
---------------------------------------------------------------------------------

					0x1234	= 4660					
%d :HEX		INT16   EXP: 0x1234
%t :INT16	string           
%l :DEC		INT16 	EXP: 4660


%o :HEX - INT8U     EXP: 34
%x :HEX - INT32		EXP: 00001234
%c :char			EXP: 61 -> 'b'
%s :char string;	EXP: HPK	DPrint("%s",CURVER);//#define CURVER "HPK"
%h :HEX string；	EXP: 00 01 02 03 04		DPrint("%h",test,sizeof(test));
%m : memory
%p : Pointer        the address of point to...
*/
void DPrint(const char *fmt, ...)
{
    char *s;
    INT8U *ptr;
    INT16U *nptr;
    INT32U d,i;
    INT8U op,temp,sht;
	INT16U dint;
    INT8U buf[16];

    va_list ap;             //Create a new format 'ap'
    va_start(ap, fmt);      //make ap point to the address of format 'fmt'
    //va_arg(ap,int)       get the next para.
    sht = 0; 
    while (*fmt) 
	{
       if (*fmt != '%') 
	   {
          sht = 0;
          USART_sprint_byte(DBG_UART,*fmt++);
          continue;
       }
	   
       switch (*++fmt) 
	   {
          case 'd':// INT16 EXP: 0xFFFF
		  	dint = va_arg(ap,int); 
			USART_print_string(DBG_UART,"0x");
            for (i=0;i<2;i++) 
			{
               temp = dint >> ((1-i)*8);
               USART_print_byte(DBG_UART, Radix_HexToChar(temp >> 4));
               USART_print_byte(DBG_UART, Radix_HexToChar(temp));
             }
			 break;
		  case 't'://INT16 string EXP:1234
		  	ptr = va_arg(ap,INT8U *);
            nptr = (INT16U *)ptr;
            d = va_arg(ap,INT32U);
			for (i=0;i<d;i++) 
			{
			   temp = Radix_DecToAscii(buf,*nptr++,0);
               USART_print_mem(DBG_UART,buf,temp);
			   USART_print_byte(DBG_UART,' ');
		    }            
			break;
		  case 'l'://INT32
          	d    = va_arg(ap,int);
            temp = Radix_DecToAscii(buf,d,sht);
            USART_print_mem(DBG_UART,buf,temp);
            break;
          case 'o'://INT8U's value    
          	op = va_arg(ap,int);
            USART_print_bytehex(DBG_UART,op);
            break;
          case 'x':
          	d = va_arg(ap,int);
            for (i=0;i<4;i++) 
			{
               temp = d >> ((3-i)*8);
               USART_print_byte(DBG_UART, Radix_HexToChar(temp >> 4));
               USART_print_byte(DBG_UART, Radix_HexToChar(temp));
            }
            break;
          case 'c':
          	op = va_arg(ap,int);
            USART_print_byte(DBG_UART,(INT8U)op);
            break;
          case 'h':
          	ptr = va_arg(ap,INT8U *);
            d = va_arg(ap,INT32U);
            USART_print_memhex(DBG_UART,ptr,d);
            break;
          case 'm':
          	ptr = va_arg(ap,INT8U *);
            d   = va_arg(ap,INT32U);
            USART_print_mem(DBG_UART,ptr,d);
            break;           
          case 's':
             s = va_arg(ap, char *);
             USART_print_string(DBG_UART,s);
             break;    
          case 'S':
             ptr = va_arg(ap, INT8U *);
             d = va_arg(ap,INT32U);
             USART_print_mem(DBG_UART,ptr,d);
             break;
          case 'p':
             ptr = va_arg(ap, void *);
             d = (INT32U)ptr;
             USART_print_string(DBG_UART,"0x");
             for (i=0;i<4;i++) 
			 {
               temp = d >> ((3-i)*8);
               USART_print_byte(DBG_UART, Radix_HexToChar(temp >> 4));
               USART_print_byte(DBG_UART, Radix_HexToChar(temp));
             }
			 break;   
          default:  
             sht = *fmt;
             if (sht > '0' && sht < '9') 
			 {
                sht -= '0';
			 } 
			 else 
			 {
                sht = 0;
			    USART_print_byte(DBG_UART,'%'); 
			 }
             break;
        }
        fmt++;
    }
	va_end(ap);
}



/************************************************************************************************************
	底层驱动    USART1
******************************************************************/
    
__inline 	void USART1_TX_DIS(void)
{
	//USART2->CR1 &= 0xFFBF;
	USART1->CR1&=0xFF7F;
}
__inline 	void USART1_TX_EN(void)
{
	//USART2->CR1 |= 0x0040;		
	USART1->CR1|=0x0080;
}

void USART_TX_Empty(USART_TypeDef* USARTx,FunctionalState NewState)
{//发送缓存空,中断使用
    if (NewState != DISABLE){		
	    USARTx->CR1|=USART_CR1_TXEIE;
    }else{
        USARTx->CR1 &= (uint16_t)~((uint16_t)USART_CR1_TXEIE); 
    }
}

void USART1_IRQHandler(void)
{//FIFO_CHAN_ENUM
	static INT16U awFlag;
	volatile INT8U aubData;
	
	awFlag=USART1->SR;
	if(awFlag&0x0f){
		aubData = USART1->DR;
		awFlag=USART1->SR;
		return;
	}
	if(awFlag&0x20)//RXNE
	{
		aubData=USART1->DR;
		if(FIFO_Write(&FIFO_Buf[1].rfifo,aubData) == false){
			FIFO_Reset(&FIFO_Buf[1].rfifo);
			FIFO_Write(&FIFO_Buf[1].rfifo,aubData);
		}
	}
	if(awFlag&0x80)//Transmit data register empty
	{
		if(FIFO_SendData(1) == false){
			FIFO_Buf[1].status &= ~UART_SENDING;
			USART_TX_Empty(USART1,DISABLE);
		}
	}
}


/************************************************************************************************************
	底层驱动    USART2
******************************************************************/
void USART2_IRQHandler(void)
{//FIFO_CHAN_ENUM
	static INT16U awFlag;
	volatile INT8U aubData;
	
	awFlag=USART2->SR;
	if(awFlag&0x0f){
		aubData = USART2->DR;
		awFlag=USART2->SR;
		return;
	}
	if(awFlag&0x20)//RXNE
	{
		aubData=USART2->DR;
		if(FIFO_Write(&FIFO_Buf[2].rfifo,aubData) == false){
			FIFO_Reset(&FIFO_Buf[2].rfifo);
			FIFO_Write(&FIFO_Buf[2].rfifo,aubData);
		}
	}
	if(awFlag&0x80)//Transmit data register empty
	{
		if(FIFO_SendData(2) == false){
			FIFO_Buf[2].status &= ~UART_SENDING;
			USART_TX_Empty(USART2,DISABLE);
		}
	}
}
/************************************************************************************************************
	底层驱动    USART3
******************************************************************/
void USART3_IRQHandler(void)
{//FIFO_CHAN_ENUM
	static INT16U awFlag;
	volatile INT8U aubData;
	
	awFlag=USART3->SR;
	if(awFlag&0x0f){
		aubData = USART3->DR;
		awFlag=USART3->SR;
		return;
	}
	if(awFlag&0x20)//RXNE
	{
		aubData=USART3->DR;
		if(FIFO_Write(&FIFO_Buf[3].rfifo,aubData) == false){
			FIFO_Reset(&FIFO_Buf[3].rfifo);
			FIFO_Write(&FIFO_Buf[3].rfifo,aubData);
		}
	}
	if(awFlag&0x80)//Transmit data register empty
	{
		if(FIFO_SendData(3) == false){
			FIFO_Buf[3].status &= ~UART_SENDING;
			//USART3_TX_DIS();
			USART_TX_Empty(USART3,DISABLE);
		}
	}
}

/************************************************************************************************************
	用户应用层 
******************************************************************/

void USART_Monitor()
{
	
}

//static  TIMER  *TmrUsart;
#include "usb_bsp.h"
#define COMPILE_DATE	(__DATE__" "__TIME__) 
BOOLEAN USART_main(INT8U chan)
{						
	if(chan >= FIFO_NUM_TOTAL) return false;							    
	FIFO_Init(&FIFO_Buf[chan].sfifo,s_FIFO_usart,sizeof(s_FIFO_usart));	
	FIFO_Init(&FIFO_Buf[chan].rfifo,r_FIFO_usart,sizeof(r_FIFO_usart));
	
	DPrint("\n\n\n**************************************************\n");
	DPrint("软件编译时间:%s\n\n",COMPILE_DATE);	
	RTC_DelayXms(200);
	//USB_OTG_BSP_mDelay(2);
	
	//TmrUsart = CreateTimer(USART_Monitor);
	//StartTimer(TmrUsart,_MS(500));
	return true;
}
