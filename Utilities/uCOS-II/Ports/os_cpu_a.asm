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
;                                              ��������
;********************************************************************************************************

    EXTERN  OSRunning                                           ; �ⲿ�ο����ⲿ����ı�����������������C�����е�extern����
    EXTERN  OSPrioCur
    EXTERN  OSPrioHighRdy
    EXTERN  OSTCBCur
    EXTERN  OSTCBHighRdy
    EXTERN  OSIntExit
    EXTERN  OSTaskSwHook
    EXTERN  OS_CPU_ExceptStkBase


    EXPORT  OS_CPU_SR_Save                                      ; ������ļ��������ĺ�����
    EXPORT  OS_CPU_SR_Restore
    EXPORT  OSStartHighRdy
    EXPORT  OSCtxSw
    EXPORT  OSIntCtxSw
;   �޸ģ���OS_CPU_PendSVHandler��ΪPendSV_Handler
    EXPORT  PendSV_Handler

;********************************************************************************************************
;                                                EQUATES
;********************************************************************************************************

NVIC_INT_CTRL   EQU     0xE000ED04                              ; �жϿ���״̬�Ĵ�����
NVIC_SYSPRI14   EQU     0xE000ED22                              ; ϵͳ�쳣���ȼ��Ĵ���PRI_14��������PendSV�����ȼ���
NVIC_PENDSV_PRI EQU           0xFF                              ; ����PendSV�Ŀɱ�����ȼ�Ϊ255������͡�
NVIC_PENDSVSET  EQU     0x10000000                              ; �жϿ��Ƽ�״̬�Ĵ���ICSR��λ28 д 1 ������ PendSV�жϡ���ȡ���򷵻� PendSV ��״̬��

;********************************************************************************************************
;                                                ��������ָ��
;********************************************************************************************************

    AREA |.text|, CODE, READONLY, ALIGN=2                       ; CODE      : ����Σ� READONLY : ֻ����ALIGN=2 : 4�ֽڶ���(2^n)��
	THUMB														; THUMB     : thumb
    REQUIRE8                                                    ; require8  : ָ����ǰ�ļ���ջҪ��8�ֽڶ��롣
    PRESERVE8                                                   ; preserve8 : ���浱ǰ�ļ���ջ8�ֽڶ��롣

;********************************************************************************************************
;                                   CRITICAL SECTION METHOD 3 FUNCTIONS
;                                     �����жϷ�ʽ3�йص�����ຯ����
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
; Note(s)    : 1) �������һ���������½ṹ:
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
; ��ȫ���ж�ǰ������ȫ���жϱ�־�������ٽ������˳��ٽ�����ָ��жϱ��
OS_CPU_SR_Save                                                  ; �����ٽ�������ͬ��OS_ENTER_CRITICAL()���������ֳ�����
    MRS     R0, PRIMASK                                         ; ��ȡ PRIMASK �� R0(����ȫ���жϱ�ǣ����˹����ж�)
    CPSID   I                                                   ; �쳣����Ĵ���PRIMASK=1�����ж�
    BX      LR                                                  ; ���أ���ת��LR���ӼĴ�������

OS_CPU_SR_Restore                                               ; �˳��ٽ�������ͬ��OS_EXIT_CRITICAL()�����ָ��ֳ�����  
    MSR     PRIMASK, R0                                         ; ��ȡ R0��PRIMASK��(�ָ�ȫ���жϱ��)��ͨ��R0���ݲ���
    BX      LR                                                  ; ���أ���ת��LR���ӼĴ�������

;********************************************************************************************************
;                                              ��ʼ������
;                                       void OSStartHighRdy(void)
;
; Note(s) : 1) �������������һ��PendSV�쳣(�������л�) ������һ������
;
;           2) OSStartHighRdy() MUST:
;              a) ����PendSV�쳣���ȼ����;
;              b) ���ó�ʼPSP=0,�����������л��������״�����;
;              c) S��������ջOS_CPU_ExceptStkBase;
;              d) ����OSRunning Ϊ TRUE;
;              e) ����PendSV�쳣;
;              f) ʹ���ж�(ͨ��ʹ���ж���������)��
;********************************************************************************************************

OSStartHighRdy
    LDR     R0, =NVIC_SYSPRI14                                  ; װ��ϵͳ�쳣���ȼ��Ĵ���PRI_14��
                                                                ; ������PendSV�ж����ȼ��ļĴ�����
    LDR     R1, =NVIC_PENDSV_PRI                                ; װ�� PendSV�Ŀɱ�����ȼ�(255)��
    STRB    R1, [R0]                                            ; �޷����ֽڼĴ����洢��R1��Ҫ�洢�ļĴ�����
                                                                ; �洢���ڴ��ַ�����ڵļĴ�����

    MOVS    R0, #0                                              ; ����PSP Ϊ0 ��Ϊ�˳�ʼ���������л�����.
    MSR     PSP, R0

    LDR     R0, =OS_CPU_ExceptStkBase                           ; ��ʼ��OS_CPU_ExceptStkBase�е�MSP
    LDR     R1, [R0]
    MSR     MSP, R1

    LDR     R0, =OSRunning                                      ; OSRunning = TRUE
    MOVS    R1, #1
    STRB    R1, [R0]

    LDR     R0, =NVIC_INT_CTRL                                  ; ����PendSV�쳣(����������л�)��
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]

    CPSIE   I                                                   ; ���жϣ������жϴ�����

OSStartHang
    B       OSStartHang                                         ; ��Ӧ��ִ�е���������


;********************************************************************************************************
;                                       ִ���������л�(�������л�)
;                                           void OSCtxSw(void)
;
; Note(s) : 1) ���������ִ���������л�ʱ�����á������������PendSV�쳣�����ָ������
;********************************************************************************************************

OSCtxSw
    LDR     R0, =NVIC_INT_CTRL                                  ; ����PendSV�쳣(����������л�)��
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR

;********************************************************************************************************
;                                       ִ���������л� (���жϼ��л�)
;                                         void OSIntCtxSw(void)
;
; Notes:    1) OSIntCtxSw()����OSIntExit()���á���ȷ����Ҫ�������л�����Ҫ��ֹ�жϡ�
;              �������ֻ�Ǵ���PendSV�쳣��
;********************************************************************************************************

OSIntCtxSw
    LDR     R0, =NVIC_INT_CTRL                                  ; ����PendSV�쳣(����������л�)��
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR

;********************************************************************************************************
;                                            ����PendSV�쳣
;                                     void OS_CPU_PendSVHandler(void)
;
; Note(s) : 1) PendSV�����������������л�.  This is a recommended method for performing
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

;�޸ģ���OS_CPU_PendSVHandler��ΪPendSV_Handler��ҪΪ�˼���ST���ļ�
PendSV_Handler
    CPSID   I                                                   ; ��ֹ�ڽ����������л��ڼ䱻�жϡ������ж�: "CPSID I" ��ͬ�� "PRIMASK=1"����
    MRS     R0, PSP                                             ; ��ȡ����ָ��PSP��R0��
    CBZ     R0, OS_CPU_PendSVHandler_nosave                     ; ��PSP = 0���Ƚ�R0��ʱ������ת��OS_CPU_PendSVHandler_nosave��

    SUBS    R0, R0, #0x20                                       ; R0-0x20,�����±�־��SUBS��"S"��׺��������R4-R11�Ĵ�����
    STM     R0, {R4-R11}                                        ; װ�أ������洢��R4-R11��

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
	
	