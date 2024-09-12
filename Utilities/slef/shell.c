/****************************************Copyright (c)****************************************************
**               
*********************************************************************************************************/
#define SHELL_GLOBALS 
#include "include_slef.H"
#include "ucos_ii.H"
#pragma  diag_suppress 870

#define  MAX_PARAM                 4

static void cmd_Help(void);
static void cmd_Reset(void);
static void cmd_Test(void);
static void cmd_Test2(void);
static void cmd_CLS(void);

static INT32U cmd_ChgPara2DEC(INT8U* para,INT8U paralen);

static  INT8U  cmdbuf[100];
static  FILO   sfilo;

typedef struct {
	INT8U  paranum;
    INT8U  cmdlen;
	INT8U  *cmd;
	INT8U  *param[MAX_PARAM];
	INT8U  paramlen[MAX_PARAM];
}SH_CMD;


static  SH_CMD   incmd;
static BOOLEAN SHELL_Paramcheck(INT8U expect);

typedef struct
{
  char  * command;
  void (*cmdfunc)(void);
  INT8U  ParaNum;
  char  *info;
}SHELLMAP;


static const SHELLMAP  shellmap[] = 
{
	{"HELP",cmd_Help,0,"你可以在命令后加'?'，获得其可输入的参数,例如 : 'beep ?'"},
	
    {"RESET",cmd_Reset,0},
    {"TEST",cmd_Test,1},
    {"TEST2",cmd_Test2,3,"可接收三个参数"},
    {"CLS",cmd_CLS,0,"会输出一些空行，和之前的显示内容分开\n"},
};


static void cmd_CLS(void)
{
	SHELL_DEBUG(("\n\n\n\n\n\n\n\n\n\n\n"));
}
static void cmd_Help(void)
{
    INT16U i,cnt = 0;
	SHELL_DEBUG((":>所有支持的命令:\n"));
	for (i=0;i<sizeof(shellmap)/sizeof(SHELLMAP);i++)
	{
		SHELL_DEBUG(("  <%m>  ",shellmap[i].command,strlen(shellmap[i].command)));
		
		if(((cnt++)%5) == 0) 
		{
			SHELL_DEBUG(("\n"));
		}
	}
	SHELL_DEBUG(("\n"));
}

static INT32U cmd_ChgPara2DEC(INT8U* para,INT8U paralen)
{
	INT32U	Cnt=0,i=0,res;
	while(paralen--){
		if((*para != ' ')&&(*para != NULL))
		{
			if((*para >= '0')&& (*para <= '9'))
			{
				i <<= 4;
				i |= ((*para++)-'0');
			}else{
				SHELL_DEBUG((":> 请使用十进制数\n"));
				return 0;
			}
			if(Cnt++ >= 8) break;
		}
	}
	res = Radix_HexToBcd(i);
	//SHELL_DEBUG((":> cmd_ChgPara2DEC = [%l]\n",res));
	return res;
}
		
static void cmd_Test2(void)
{	
	#define LENPARA_NUM 7
	#define LENPARA_SNED 3
	static INT8U i,a[LENPARA_SNED];
	for(i=0;i<(LENPARA_NUM-LENPARA_SNED);i+=LENPARA_SNED)
	{
		a[i] = i;
		a[i+1] = i+1;
		a[i+2] = i+2;
		SHELL_DEBUG((":> cmd_Test2 a = %h\n",a,sizeof(a)));
	}
	SHELL_DEBUG((":> cmd_Test2 = %o\n",i));
}

void cmd_StackFlow(void)
{
	volatile int a[3],i;

	for(i=0; i<10000; i++)
	{
		a[i]=1;
	}
}
static void cmd_Reset(void)
{	
	//((void (*)())0)();
	RTC_DelayXms(100);
	SHELL_DEBUG(("\n\n*************************\n"));
	SHELL_DEBUG(("准备复位系统\n"));
	RTC_DelayXms(500);
	//cmd_StackFlow();
	
	NVIC_SystemReset();//NVIC_GenerateSystemReset();
}

static void cmd_Test(void)
{
    INT8U tmp;
    
    tmp = *(incmd.param[0])-'0';
	switch(tmp)
	{
		case 0:
			 break;
		case 1:
			 break;
		case 2:
				if(0){
					void (*pFuncVoid)(void);
					pFuncVoid = &(cmd_Test);
					pFuncVoid();
				}
			 break;	
		case 3:
			 break;	
		default:
			 break;
	}
}

//judge param number is valid
//expect:当函数有几个参数，就是几，能少不能多
static BOOLEAN SHELL_Paramcheck(INT8U expect)
{
    if (incmd.paranum > expect)
	{
		if (incmd.paranum > expect)
    	SHELL_DEBUG((":>Tips:此命令只能带 %o 个参数!!!",expect));
		else
    	SHELL_DEBUG((":>Tips:此命令需要 %o 个参数!!!",expect));
    	return false;
	}  
	return true;
}
INT16U SHELL_KeyLocation(INT8U *sptr, char key, INT8U keyindex,INT16U limit)
{
    INT16U loc;
    
    loc = 0;
    while(1){
       if (*sptr++ == key) {
          if (keyindex == 1) break;
          else keyindex--;
       }
       if (limit == 0) break;
       else loc++;
       limit--;
    }
    return loc;
}
static BOOLEAN SHELL_ParseCommandParam(void)
{
	INT16U  inlen,index,tlen;
    INT8U   cnt;
	
	inlen = FILO_Occupy(&sfilo);
	if (inlen == 0) return false;
    cnt = 1;
    
	incmd.cmd = cmdbuf;	  
	if((*incmd.cmd == KEY_LF)||(*incmd.cmd == KEY_CR)){
		incmd.cmd++;//将最前面的移除掉
		inlen--;
	}
    incmd.paranum = 0;
	
    index = SHELL_KeyLocation(incmd.cmd,' ',cnt,inlen);
	incmd.cmdlen = index;
    //SHELL_DEBUG(("  输入指令:>%m :",incmd.cmd,incmd.cmdlen));
    if (index == inlen)  //无空格
	{
		SHELL_DEBUG(("\n"));
       return true;
	}
	cnt++;
	while (index != inlen) //有空格
	{
		if (incmd.cmd[index] == ' ') 
		{
			index++;
		} 
		else          //空格后的参数存放
		{
			tlen = index;
			incmd.param[incmd.paranum] = &incmd.cmd[index];
			index = SHELL_KeyLocation(incmd.cmd,' ',cnt,inlen);
			incmd.paramlen[incmd.paranum] = index - tlen;
		  
			SHELL_DEBUG((":>第%l个参数:%m ; ",incmd.paranum+1,incmd.param[incmd.paranum],incmd.paramlen[incmd.paranum]));

			incmd.paranum++;
			if (incmd.paranum >=  MAX_PARAM) break;
			cnt++;
		}   
	}
    SHELL_DEBUG(("\n"));
	return true;
}

static BOOLEAN SHELL_Commad(void)
{
    INT16U i;
	void (*cmdfunc)(void);

	memset( ((INT8U*)&incmd),0,sizeof(incmd));
	//memset( ((INT8U*)&cmdbuf),0,sizeof(cmdbuf));
	//	SHELL_Reset();
    if (SHELL_ParseCommandParam() == false) return false;
	Radix_UpCaseChar(incmd.cmd,incmd.cmdlen);
    for (i=0;i<sizeof(shellmap)/sizeof(SHELLMAP);i++)
    {
		if (incmd.cmdlen != strlen(shellmap[i].command)){

			if((i == 0)&&(incmd.cmdlen == 4)){
				SHELL_DEBUG((":>Tips:Input[%S]",shellmap[i].command,strlen(shellmap[i].command)));
				SHELL_DEBUG((":>Tips:Input[%S]",incmd.cmdlen,incmd.cmdlen));
			}
		 	continue;
		 }
		if (memcmp(incmd.cmd,shellmap[i].command,incmd.cmdlen) == 0) 
		{
			if(*(incmd.param[0]) == '?'){
				SHELL_DEBUG((":>Tips:%s",shellmap[i].info));//,sizeof(shellmap[i].info)
				return true;
			}
			else if(SHELL_Paramcheck(shellmap[i].ParaNum) == false){
				return false;
			}
			
			cmdfunc = shellmap[i].cmdfunc;
			if (cmdfunc != NULL) 
			{
				SHELL_DEBUG((":>执行指令:%m...\n",incmd.cmd,incmd.cmdlen));
				(*cmdfunc)();	 
			}
			SHELL_DEBUG((":>命令执行完毕!\n\n"));
			return true;
		}
	}	
	SHELL_DEBUG(("\n:>无法识别的指令!\n\n"));
	return false;
}


static void  SHELL_Reset(void)
{
	FILO_Reset(&sfilo);
}

static void SHELL_Monitor(void *ptmr, void *parg)
{		
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif
	while(USART_received(DBG_UART))
	{
		OS_ENTER_CRITICAL();
		OSIntEnter();
		OSSemPost(OSSem_Shell);
		OSIntExit();
		OS_EXIT_CRITICAL();	
		return;
	}
}
void SHELL_TestProcess(void)
{    
    INT8U ch;

	while(USART_received(DBG_UART)) 
	{  
		ch = USART_read(DBG_UART);
		if (ch == KEY_DEL) 
		{
			if(FILO_Occupy(&sfilo) == 1)  
			{
 				SHELL_DEBUG(("\n  :<当前命令:"));
			}
			FILO_Read(&sfilo);
			if(FILO_Occupy(&sfilo) >= 1)
			{
				SHELL_DEBUG(("\n  :<当前命令:%m",FILO_StartPos(&sfilo),FILO_Occupy(&sfilo)));
			}
		} 
		else if (ch == KEY_CR) 
		{
			SHELL_DEBUG(("\n"));
			SHELL_Commad();
			SHELL_Reset();
		} 
		else 
		{
			FILO_Write(&sfilo,ch);
			if(FILO_Occupy(&sfilo) == 1)
			{
				SHELL_DEBUG(("\n  当前命令:>"));
			}
				SHELL_DEBUG(("%c",ch));
			}  
	}
}


void SHELL_init(void)
{
	INT8U err;
	OS_TMR *time;
    FILO_Init(&sfilo,cmdbuf,sizeof(cmdbuf));
	
	//shellscantmr = CreateTimer(SHELL_Monitor);
	//StartTimer(shellscantmr,_MS(20));
	
    time = OSTmrCreate(Tmr_Xs(1), Tmr_Xms(20), OS_TMR_OPT_PERIODIC, SHELL_Monitor, NULL, "SHELL_Monitor", &err);    
	if(time == ((OS_TMR *)0)){
	    SHELL_DEBUG(("\n Shell>  CreateTimer  Failed!!!!>\n"));
	}
	else{
	    OSTmrStart(time, &err); //OS_TMR_CFG_TICKS_PER_SEC
	}
	
}
