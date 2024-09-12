;********************************************************************************************************
;                                               uC/OS-II
;                                         The Real-Time Kernel
;
;                               (c) Copyright 1992-2006, Micrium, Weston, FL
;                                          All Rights Reserved
;
;                                           ARM Cortex-M3 Port
;
; File      : OS_CPU_A.ASM
; Version   : V2.89
; By        : Jean J. Labrosse
;             Brian Nagel
;
; For       : ARMv7M Cortex-M3
; Mode      : Thumb2
; Toolchain : RealView Development Suite
;             RealView Microcontroller Development Kit (MDK)
;             ARM Developer Suite (ADS)
;             Keil uVision
;********************************************************************************************************

;********************************************************************************************************
;                                              公共函数
;********************************************************************************************************

    EXTERN  OSRunning                                           ; 外部参考（外部定义的变量、函数，类似于C语言中的extern）。
    EXTERN  OSPrioCur
    EXTERN  OSPrioHighRdy
    EXTERN  OSTCBCur
    EXTERN  OSTCBHighRdy
    EXTERN  OSIntExit
    EXTERN  OSTaskSwHook
    EXTERN  OS_CPU_ExceptStkBase


    EXPORT  OS_CPU_SR_Save                                      ; 在这个文件中声明的函数。
    EXPORT  OS_CPU_SR_Restore
    EXPORT  OSStartHighRdy
    EXPORT  OSCtxSw
    EXPORT  OSIntCtxSw
;   修改，将OS_CPU_PendSVHandler改为PendSV_Handler
    EXPORT  PendSV_Handler

;********************************************************************************************************
;                                                EQUATES
;********************************************************************************************************

NVIC_INT_CTRL   EQU     0xE000ED04                              ; 中断控制状态寄存器。
NVIC_SYSPRI14   EQU     0xE000ED22                              ; 系统异常优先级寄存器PRI_14，即设置PendSV的优先级。
NVIC_PENDSV_PRI EQU           0xFF                              ; 定义PendSV的可编程优先级为255，即最低。
NVIC_PENDSVSET  EQU     0x10000000                              ; 中断控制及状态寄存器ICSR的位28 写 1 以悬起 PendSV中断。读取它则返回 PendSV 的状态。

;********************************************************************************************************
;                                                代码生成指令
;********************************************************************************************************

    AREA |.text|, CODE, READONLY, ALIGN=2                       ; CODE      : 代码段， READONLY : 只读，ALIGN=2 : 4字节对齐(2^n)。
	THUMB														; THUMB     : thumb
    REQUIRE8                                                    ; require8  : 指定当前文件堆栈要求8字节对齐。
    PRESERVE8                                                   ; preserve8 : 保存当前文件堆栈8字节对齐。

;********************************************************************************************************
;                                   CRITICAL SECTION METHOD 3 FUNCTIONS
;                                     （与中断方式3有关的两汇编函数）
;
; Description: Disable/Enable interrupts by preserving the state of interrupts.  Generally speaking you
;              would store the state of the interrupt disable flag in the local variable 'cpu_sr' and then
;              disable interrupts.  'cpu_sr' is allocated in all of uC/OS-II's functions that need to
;              disable interrupts.  You would restore the interrupt disable state by copying back 'cpu_sr'
;              into the CPU's status register.
;
; Prototypes :     OS_CPU_SR  OS_CPU_SR_Save(void);
;                  void       OS_CPU_SR_Restore(OS_CPU_SR cpu_sr);
;
;
; Note(s)    : 1) 这个函数一般用于以下结构:
;
;                 void Task (void *p_arg)
;                 {
;                 #if OS_CRITICAL_METHOD == 3          /* Allocate storage for CPU status register */
;                     OS_CPU_SR  cpu_sr;
;                 #endif
;
;                          :
;                          :
;                     OS_ENTER_CRITICAL();             /* cpu_sr = OS_CPU_SaveSR();                */
;                          :
;                          :
;                     OS_EXIT_CRITICAL();              /* OS_CPU_RestoreSR(cpu_sr);                */
;                          :
;                          :
;                 }
;********************************************************************************************************
; 关全局中断前，保存全局中断标志，进入临界区。退出临界区后恢复中断标记
OS_CPU_SR_Save                                                  ; 进入临界区（等同于OS_ENTER_CRITICAL()），保存现场环境
    MRS     R0, PRIMASK                                         ; 读取 PRIMASK 到 R0(保存全局中断标记，除了故障中断)
    CPSID   I                                                   ; 异常掩码寄存器PRIMASK=1，关中断
    BX      LR                                                  ; 返回（跳转到LR连接寄存器）。

OS_CPU_SR_Restore                                               ; 退出临界区（等同于OS_EXIT_CRITICAL()），恢复现场环境  
    MSR     PRIMASK, R0                                         ; 读取 R0到PRIMASK中(恢复全局中断标记)，通过R0传递参数
    BX      LR                                                  ; 返回（跳转到LR连接寄存器）。

;********************************************************************************************************
;                                              开始多任务
;                                       void OSStartHighRdy(void)
;
; Note(s) : 1) 这个函数将触发一个PendSV异常(上下文切换) 启动第一个任务
;
;           2) OSStartHighRdy() MUST:
;              a) 设置PendSV异常优先级最低;
;              b) 设置初始PSP=0,告诉上下文切换器这是首次运行;
;              c) S设置主堆栈OS_CPU_ExceptStkBase;
;              d) 设置OSRunning 为 TRUE;
;              e) 触发PendSV异常;
;              f) 使能中断(通过使能中断任务运行)。
;********************************************************************************************************

OSStartHighRdy
    LDR     R0, =NVIC_SYSPRI14                                  ; 装载系统异常优先级寄存器PRI_14。
                                                                ; 即设置PendSV中断优先级的寄存器。
    LDR     R1, =NVIC_PENDSV_PRI                                ; 装载 PendSV的可编程优先级(255)。
    STRB    R1, [R0]                                            ; 无符号字节寄存器存储。R1是要存储的寄存器。
                                                                ; 存储到内存地址所基于的寄存器。

    MOVS    R0, #0                                              ; 设置PSP 为0 ，为了初始化上下文切换调用.
    MSR     PSP, R0

    LDR     R0, =OS_CPU_ExceptStkBase                           ; 初始化OS_CPU_ExceptStkBase中的MSP
    LDR     R1, [R0]
    MSR     MSP, R1

    LDR     R0, =OSRunning                                      ; OSRunning = TRUE
    MOVS    R1, #1
    STRB    R1, [R0]

    LDR     R0, =NVIC_INT_CTRL                                  ; 触发PendSV异常(造成上下文切换)。
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]

    CPSIE   I                                                   ; 开中断（启动中断处理）。

OSStartHang
    B       OSStartHang                                         ; 不应该执行到这里来。


;********************************************************************************************************
;                                       执行上下文切换(从任务级切换)
;                                           void OSCtxSw(void)
;
; Note(s) : 1) 这个函数在执行上下文切换时被调用。这个函数触发PendSV异常，完成指定任务。
;********************************************************************************************************

OSCtxSw
    LDR     R0, =NVIC_INT_CTRL                                  ; 触发PendSV异常(造成上下文切换)。
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR

;********************************************************************************************************
;                                       执行上下文切换 (从中断级切换)
;                                         void OSIntCtxSw(void)
;
; Notes:    1) OSIntCtxSw()将被OSIntExit()调用。当确定需要上下文切换，需要禁止中断。
;              这个函数只是触发PendSV异常。
;********************************************************************************************************

OSIntCtxSw
    LDR     R0, =NVIC_INT_CTRL                                  ; 触发PendSV异常(造成上下文切换)。
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR

;********************************************************************************************************
;                                            处理PendSV异常
;                                     void OS_CPU_PendSVHandler(void)
;
; Note(s) : 1) PendSV是用来引起上下文切换.  This is a recommended method for performing
;              context switches with Cortex-M3.  This is because the Cortex-M3 auto-saves half of the
;              processor context on any exception, and restores same on return from exception.  So only
;              saving of R4-R11 is required and fixing up the stack pointers.  Using the PendSV exception
;              this way means that context saving and restoring is identical whether it is initiated from
;              a thread or occurs due to an interrupt or exception.
;
;           2) Pseudo-code is:
;              a) Get the process SP, if 0 then skip (goto d) the saving part (first context switch);
;              b) Save remaining regs r4-r11 on process stack;
;              c) Save the process SP in its TCB, OSTCBCur->OSTCBStkPtr = SP;
;              d) Call OSTaskSwHook();
;              e) Get current high priority, OSPrioCur = OSPrioHighRdy;
;              f) Get current ready thread TCB, OSTCBCur = OSTCBHighRdy;
;              g) Get new process SP from TCB, SP = OSTCBHighRdy->OSTCBStkPtr;
;              h) Restore R4-R11 from new process stack;
;              i) Perform exception return which will restore remaining context.
;
;           3) On entry into PendSV handler:
;              a) The following have been saved on the process stack (by processor):
;                 xPSR, PC, LR, R12, R0-R3
;              b) Processor mode is switched to Handler mode (from Thread mode)
;              c) Stack is Main stack (switched from Process stack)
;              d) OSTCBCur      points to the OS_TCB of the task to suspend
;                 OSTCBHighRdy  points to the OS_TCB of the task to resume
;
;           4) Since PendSV is set to lowest priority in the system (by OSStartHighRdy() above), we
;              know that it will only be run when no other exception or interrupt is active, and
;              therefore safe to assume that context being switched out was using the process stack (PSP).
;********************************************************************************************************

;修改，将OS_CPU_PendSVHandler改为PendSV_Handler主要为了兼容ST库文件
PendSV_Handler
    CPSID   I                                                   ; 防止在进行上下文切换期间被中断。（关中断: "CPSID I" 等同于 "PRIMASK=1"）。
    MRS     R0, PSP                                             ; 读取进程指针PSP到R0。
    CBZ     R0, OS_CPU_PendSVHandler_nosave                     ; 当PSP = 0（比较R0）时，就跳转到OS_CPU_PendSVHandler_nosave。

    SUBS    R0, R0, #0x20                                       ; R0-0x20,并更新标志（SUBS有"S"后缀）（保存R4-R11寄存器）
    STM     R0, {R4-R11}                                        ; 装载（连续存储）R4-R11。

    LDR     R1, =OSTCBCur                                       ; OSTCBCur->OSTCBStkPtr = SP;
    LDR     R1, [R1]
    STR     R0, [R1]                                            ; R0 is SP of process being switched out

                                                                ; At this point, entire context of process has been saved
OS_CPU_PendSVHandler_nosave
    PUSH    {R14}                                               ; Save LR exc_return value
    LDR     R0, =OSTaskSwHook                                   ; OSTaskSwHook();
    BLX     R0
    POP     {R14}

    LDR     R0, =OSPrioCur                                      ; OSPrioCur = OSPrioHighRdy;
    LDR     R1, =OSPrioHighRdy
    LDRB    R2, [R1]
    STRB    R2, [R0]

    LDR     R0, =OSTCBCur                                       ; OSTCBCur  = OSTCBHighRdy;
    LDR     R1, =OSTCBHighRdy
    LDR     R2, [R1]
    STR     R2, [R0]

    LDR     R0, [R2]                                            ; R0 is new process SP; SP = OSTCBHighRdy->OSTCBStkPtr;
    LDM     R0, {R4-R11}                                        ; Restore r4-11 from new process stack
    ADDS    R0, R0, #0x20
    MSR     PSP, R0                                             ; Load PSP with new process SP
    ORR     LR, LR, #0x04                                       ; Ensure exception return uses process stack
    CPSIE   I
    BX      LR                                                  ; Exception return will restore remaining context

    END
	
	