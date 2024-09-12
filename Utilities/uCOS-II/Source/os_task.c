/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*                                            TASK MANAGEMENT
*
*                              (c) Copyright 1992-2012, Micrium, Weston, FL
*                                           All Rights Reserved
*
* File    : OS_TASK.C
* By      : Jean J. Labrosse
* Version : V2.92.08
*
* LICENSING TERMS:
* ---------------
*   uC/OS-II is provided in source form for FREE evaluation, for educational use or for peaceful research.
* If you plan on using  uC/OS-II  in a commercial product you need to contact Micrium to properly license
* its use in your product. We provide ALL the source code for your convenience and to help you experience
* uC/OS-II.   The fact that the  source is provided does  NOT  mean that you can use it without  paying a
* licensing fee.
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE

#ifndef  OS_MASTER_FILE
#include <ucos_ii.h>
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                      CHANGE PRIORITY OF A TASK
*                                        （任务优先级切换）
*
* Description: This function allows you to change the priority of a task dynamically.  Note that the new
*              priority MUST be available.
*
* Arguments  : oldp     is the old priority
*
*              newp     is the new priority
*
* Returns    : OS_ERR_NONE            is the call was successful
*              OS_ERR_PRIO_INVALID    if the priority you specify is higher that the maximum allowed
*                                     (i.e. >= OS_LOWEST_PRIO)
*              OS_ERR_PRIO_EXIST      if the new priority already exist.
*              OS_ERR_PRIO            there is no task with the specified OLD priority (i.e. the OLD task does
*                                     not exist.
*              OS_ERR_TASK_NOT_EXIST  if the task is assigned to a Mutex PIP.
*********************************************************************************************************
*/

#if OS_TASK_CHANGE_PRIO_EN > 0u
INT8U  OSTaskChangePrio (INT8U  oldprio,
                         INT8U  newprio)
{
#if (OS_EVENT_EN)
    OS_EVENT  *pevent;
#if (OS_EVENT_MULTI_EN > 0u)
    OS_EVENT **pevents;
#endif
#endif
    OS_TCB    *ptcb;
    INT8U      y_new;
    INT8U      x_new;
    INT8U      y_old;
    OS_PRIO    bity_new;
    OS_PRIO    bitx_new;
    OS_PRIO    bity_old;
    OS_PRIO    bitx_old;
#if OS_CRITICAL_METHOD == 3u                         /* 若临界区模式为第3种，则给CPU状态寄存器分配空间 */
    OS_CPU_SR  cpu_sr = 0u;
#endif


/*$PAGE*/
#if OS_ARG_CHK_EN > 0u
    if (oldprio >= OS_LOWEST_PRIO) {
        if (oldprio != OS_PRIO_SELF) {
            return (OS_ERR_PRIO_INVALID);
        }
    }
    if (newprio >= OS_LOWEST_PRIO) {
        return (OS_ERR_PRIO_INVALID);
    }
#endif
    OS_ENTER_CRITICAL();
    if (OSTCBPrioTbl[newprio] != (OS_TCB *)0) {             /* New priority must not already exist     */
        OS_EXIT_CRITICAL();
        return (OS_ERR_PRIO_EXIST);
    }
    if (oldprio == OS_PRIO_SELF) {                          /* See if changing self                    */
        oldprio = OSTCBCur->OSTCBPrio;                      /* Yes, get priority                       */
    }
    ptcb = OSTCBPrioTbl[oldprio];
    if (ptcb == (OS_TCB *)0) {                              /* Does task to change exist?              */
        OS_EXIT_CRITICAL();                                 /* No, can't change its priority!          */
        return (OS_ERR_PRIO);
    }
    if (ptcb == OS_TCB_RESERVED) {                          /* Is task assigned to Mutex               */
        OS_EXIT_CRITICAL();                                 /* No, can't change its priority!          */
        return (OS_ERR_TASK_NOT_EXIST);
    }
#if OS_LOWEST_PRIO <= 63u
    y_new                 = (INT8U)(newprio >> 3u);         /* Yes, compute new TCB fields             */
    x_new                 = (INT8U)(newprio & 0x07u);
#else
    y_new                 = (INT8U)((INT8U)(newprio >> 4u) & 0x0Fu);
    x_new                 = (INT8U)(newprio & 0x0Fu);
#endif
    bity_new              = (OS_PRIO)(1uL << y_new);
    bitx_new              = (OS_PRIO)(1uL << x_new);

    OSTCBPrioTbl[oldprio] = (OS_TCB *)0;                    /* Remove TCB from old priority            */
    OSTCBPrioTbl[newprio] =  ptcb;                          /* Place pointer to TCB @ new priority     */
    y_old                 =  ptcb->OSTCBY;
    bity_old              =  ptcb->OSTCBBitY;
    bitx_old              =  ptcb->OSTCBBitX;
    if ((OSRdyTbl[y_old] &   bitx_old) != 0u) {             /* If task is ready make it not            */
         OSRdyTbl[y_old] &= (OS_PRIO)~bitx_old;
         if (OSRdyTbl[y_old] == 0u) {
             OSRdyGrp &= (OS_PRIO)~bity_old;
         }
         OSRdyGrp        |= bity_new;                       /* Make new priority ready to run          */
         OSRdyTbl[y_new] |= bitx_new;
    }

#if (OS_EVENT_EN)
    pevent = ptcb->OSTCBEventPtr;
    if (pevent != (OS_EVENT *)0) {
        pevent->OSEventTbl[y_old] &= (OS_PRIO)~bitx_old;    /* Remove old task prio from wait list     */
        if (pevent->OSEventTbl[y_old] == 0u) {
            pevent->OSEventGrp    &= (OS_PRIO)~bity_old;
        }
        pevent->OSEventGrp        |= bity_new;              /* Add    new task prio to   wait list     */
        pevent->OSEventTbl[y_new] |= bitx_new;
    }
#if (OS_EVENT_MULTI_EN > 0u)
    if (ptcb->OSTCBEventMultiPtr != (OS_EVENT **)0) {
        pevents =  ptcb->OSTCBEventMultiPtr;
        pevent  = *pevents;
        while (pevent != (OS_EVENT *)0) {
            pevent->OSEventTbl[y_old] &= (OS_PRIO)~bitx_old;   /* Remove old task prio from wait lists */
            if (pevent->OSEventTbl[y_old] == 0u) {
                pevent->OSEventGrp    &= (OS_PRIO)~bity_old;
            }
            pevent->OSEventGrp        |= bity_new;          /* Add    new task prio to   wait lists    */
            pevent->OSEventTbl[y_new] |= bitx_new;
            pevents++;
            pevent                     = *pevents;
        }
    }
#endif
#endif

    ptcb->OSTCBPrio = newprio;                              /* Set new task priority                   */
    ptcb->OSTCBY    = y_new;
    ptcb->OSTCBX    = x_new;
    ptcb->OSTCBBitY = bity_new;
    ptcb->OSTCBBitX = bitx_new;
    OS_EXIT_CRITICAL();
    if (OSRunning == OS_TRUE) {
        OS_Sched();                                         /* Find new highest priority task          */
    }
    return (OS_ERR_NONE);
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                                            CREATE A TASK
*                                           （创建一个任务）
*
* Description: This function is used to have uC/OS-II manage the execution of a task.  Tasks can either
*              be created prior to the start of multitasking or by a running task.  A task cannot be
*              created by an ISR.
*
* 参数       : task     指向任务的指针。
*
*              p_arg    传递任务参数的指针（是一个'void'类型的指针，即可以传递任意类型数据）。
*                       调用并传递的参数如下:
*                           void Task (void *p_arg)
*                           {
*                               for (;;) {
*                                   Task code;
*                               }
*                           }
*
*              ptos     指向任务堆栈栈顶的指针。  任务堆栈用来保存局部变量,函数参数,返回地址以及任务被
*                       中断时的CPU寄存器内容.任务堆栈的大小决定于任务的需要及预计的中断嵌套层数。计算
*                       堆栈的大小,需要知道任务的局部变量所占的空间,可能产生嵌套调用的函数，及中断嵌套
*                       所需空间。如果初始化常量OS_STK_GROWTH设为1,堆栈被设为从内存高地址向低地址增长，
*                       此时ptos应该指向任务堆栈空间的最高地址。反之,如果OS_STK_GROWTH设为0,堆栈将从内
*                       存的低地址向高地址增长。
*
*              prio     任务的优先级。每个任务必须只有唯一的优先级作为标识。数字越小，优先级越高。
*
* Returns    : OS_ERR_NONE                      函数调用成功。
*              OS_PRIO_EXIT                     具有该优先级的任务已经存在。
*
*              OS_ERR_PRIO_INVALID              参数指定的优先级数值大于最低优先级数值 (OS_LOWEST_PRIO)
*
*              OS_ERR_TASK_CREATE_ISR           系统处于ISR中,没有给任务分配TCB。
*              OS_ERR_ILLEGAL_CREATE_RUN_TIME   创建任务安全后开始运行。
*********************************************************************************************************
*/

#if OS_TASK_CREATE_EN > 0u
INT8U  OSTaskCreate (void   (*task)(void *p_arg),
                     void    *p_arg,
                     OS_STK  *ptos,
                     INT8U    prio)
{
    OS_STK     *psp;                         /* 初始化TCB堆栈指针变量，即指向TCB栈顶的指针             */
    INT8U       err;                         /* 定义(获得并定义初始化任务控制块)是否成功               */
#if OS_CRITICAL_METHOD == 3u                 /* 若临界区模式为第3种，则给CPU状态寄存器分配空间         */
    OS_CPU_SR   cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (OS_ERR_ILLEGAL_CREATE_RUN_TIME);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (prio > OS_LOWEST_PRIO) {             /* 确保优先级是在我们允许的范围内                         */
        return (OS_ERR_PRIO_INVALID);
    }
#endif
    OS_ENTER_CRITICAL();                     /* 进入临界区（关中断）                                   */
    if (OSIntNesting > 0u) {                 /* 确保我们没有从ISR中创建任务，中断嵌套层数为0           */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_CREATE_ISR);
    }
    if (OSTCBPrioTbl[prio] == (OS_TCB *)0) { /* 确保这个任务优先级未被使用，即就绪态为0                */
        OSTCBPrioTbl[prio] = OS_TCB_RESERVED;/*保留这个优先级，防止再次创建相同优先级的任务            */

        OS_EXIT_CRITICAL();
        psp = OSTaskStkInit(task, p_arg, ptos, 0u);      /* 初始化任务堆栈                             */
        err = OS_TCBInit(prio, psp, (OS_STK *)0, 0u, 0u, (void *)0, 0u);
        if (err == OS_ERR_NONE) {
            if (OSRunning == OS_TRUE) {      /* 如果多任务已经开始，找到最高任务优先级                 */
                OS_Sched();                  /* 任务调度                                               */
            }
        } else {
            OS_ENTER_CRITICAL();
            OSTCBPrioTbl[prio] = (OS_TCB *)0;/* OS_TCBInit()初始化错误，把这个优先级还给别人           */
            OS_EXIT_CRITICAL();
        }
        return (err);
    }
    OS_EXIT_CRITICAL();
    return (OS_ERR_PRIO_EXIST);
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                                  CREATE A TASK (Extended Version)
*                                   （创建一个任务扩展版本）
*
* Description: This function is used to have uC/OS-II manage the execution of a task.  Tasks can either
*              be created prior to the start of multitasking or by a running task.  A task cannot be
*              created by an ISR.  This function is similar to OSTaskCreate() except that it allows
*              additional information about a task to be specified.
*
* Arguments  : task      指向任务的指针。
*
*              p_arg     传递任务参数的指针（是一个'void'类型的指针，即可以传递任意类型数据）。
*                        调用并传递的参数如下:
*                           void Task (void *p_arg)
*                           {
*                               for (;;) {
*                                   Task code;
*                               }
*                           }
*
*              ptos      指向任务堆栈栈顶的指针(top of stack).  If the configuration constant
*                        OS_STK_GROWTH is set to 1, the stack is assumed to grow downward (i.e. from high
*                        memory to low memory).  'ptos' will thus point to the highest (valid) memory
*                        location of the stack.  If OS_STK_GROWTH is set to 0, 'ptos' will point to the
*                        lowest memory location of the stack and the stack will grow with increasing
*                        memory locations.  'ptos' MUST point to a valid 'free' data item.
*
*              prio      任务的优先级.  A unique priority MUST be assigned to each task and the
*                        lower the number, the higher the priority.
*
*              id        任务的ID (0..65535)
*
*              pbos      指向任务堆栈栈底的指针(bottom of stack).  If the configuration constant
*                        OS_STK_GROWTH is set to 1, the stack is assumed to grow downward (i.e. from high
*                        memory to low memory).  'pbos' will thus point to the LOWEST (valid) memory
*                        location of the stack.  If OS_STK_GROWTH is set to 0, 'pbos' will point to the
*                        HIGHEST memory location of the stack and the stack will grow with increasing
*                        memory locations.  'pbos' MUST point to a valid 'free' data item.
*
*              stk_size  任务堆栈大小.  If OS_STK is set to INT8U,
*                        'stk_size' corresponds to the number of bytes available.  If OS_STK is set to
*                        INT16U, 'stk_size' contains the number of 16-bit entries available.  Finally, if
*                        OS_STK is set to INT32U, 'stk_size' contains the number of 32-bit entries
*                        available on the stack.
*
*              pext      指向用户提供的内存。这里是用作 TCB 的扩展。
*                        For example, this user memory can hold the contents of floating-point registers
*                        during a context switch, the time each task takes to execute, the number of times
*                        the task has been switched-in, etc.
*
*              opt       包含额外信息(或选项) 关于任务处理。
*                        The LOWER 8-bits are reserved by uC/OS-II while the upper 8 bits can be application
*                        specific.  See OS_TASK_OPT_??? in uCOS-II.H.  Current choices are:
*
*                        OS_TASK_OPT_STK_CHK      Stack checking to be allowed for the task
*                        OS_TASK_OPT_STK_CLR      Clear the stack when the task is created
*                        OS_TASK_OPT_SAVE_FP      If the CPU has floating-point registers, save them
*                                                 during a context switch.
*
* Returns    : OS_ERR_NONE                      if the function was successful.
*              OS_ERR_PRIO_EXIST                if the task priority already exist
*                                               (each task MUST have a unique priority).
*              OS_ERR_PRIO_INVALID              if the priority you specify is higher that the maximum
*                                               allowed (i.e. > OS_LOWEST_PRIO)
*              OS_ERR_TASK_CREATE_ISR           if you tried to create a task from an ISR.
*              OS_ERR_ILLEGAL_CREATE_RUN_TIME   if you tried to create a task after safety critical
*                                               operation started.
*********************************************************************************************************
*/
/*$PAGE*/
#if OS_TASK_CREATE_EXT_EN > 0u
INT8U  OSTaskCreateExt (void   (*task)(void *p_arg),
                        void    *p_arg,
                        OS_STK  *ptos,
                        INT8U    prio,
                        INT16U   id,
                        OS_STK  *pbos,
                        INT32U   stk_size,
                        void    *pext,
                        INT16U   opt)
{
    OS_STK     *psp;
    INT8U       err;
#if OS_CRITICAL_METHOD == 3u                 /* 若临界区模式为第3种，则给CPU状态寄存器分配空间         */
    OS_CPU_SR   cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (OS_ERR_ILLEGAL_CREATE_RUN_TIME);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (prio > OS_LOWEST_PRIO) {             /* Make sure priority is within allowable range           */
        return (OS_ERR_PRIO_INVALID);
    }
#endif
    OS_ENTER_CRITICAL();
    if (OSIntNesting > 0u) {                 /* Make sure we don't create the task from within an ISR  */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_CREATE_ISR);
    }
    if (OSTCBPrioTbl[prio] == (OS_TCB *)0) { /* 确保这个任务优先级未被使用，即就绪态为0                */
        OSTCBPrioTbl[prio] = OS_TCB_RESERVED;/*保留这个优先级，防止再次创建相同优先级的任务            */

        OS_EXIT_CRITICAL();

#if (OS_TASK_STAT_STK_CHK_EN > 0u)
        OS_TaskStkClr(pbos, stk_size, opt);                    /* 清除没用到的堆栈                     */
#endif

        psp = OSTaskStkInit(task, p_arg, ptos, opt);           /* 初始化任务堆栈                       */
        err = OS_TCBInit(prio, psp, pbos, id, stk_size, pext, opt);
        if (err == OS_ERR_NONE) {
            if (OSRunning == OS_TRUE) {                        /* 如果多任务已经开始，找到HPT使其运行  */
                OS_Sched();
            }
        } else {                                               /* 若创建TCB不成功...                   */
            OS_ENTER_CRITICAL();
            OSTCBPrioTbl[prio] = (OS_TCB *)0;                  /* 使这个优先级有效，留给别人使用       */
            OS_EXIT_CRITICAL();
        }
        return (err);
    }
    OS_EXIT_CRITICAL();
    return (OS_ERR_PRIO_EXIST);
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                                            DELETE A TASK
*                                           （删除一个任务）
*
* Description: 这个函数允许你删除一个任务.
*              可以通过自己任务来调用该函数删除自身。
*              删除任务返回到休眠态，可以通过创建任务来重新创建删除之后优先级的任务。
*
* Arguments  : prio    要删除的任务的优先级。 Note that you can explicitly delete
*                      1、可以是指定优先级。
*                      2、自身优先级"OS_PRIO_SELF"(当不知道自己优先级时)。
*
* Returns    : OS_ERR_NONE             if the call is successful
*              OS_ERR_TASK_DEL_IDLE    if you attempted to delete uC/OS-II's idle task
*              OS_ERR_PRIO_INVALID     if the priority you specify is higher that the maximum allowed
*                                      (i.e. >= OS_LOWEST_PRIO) or, you have not specified OS_PRIO_SELF.
*              OS_ERR_TASK_DEL         if the task is assigned to a Mutex PIP.
*              OS_ERR_TASK_NOT_EXIST   if the task you want to delete does not exist.
*              OS_ERR_TASK_DEL_ISR     if you tried to delete a task from an ISR
*
* Notes      : 1) To reduce interrupt latency, OSTaskDel() 'disables' the task:
*                    a) by making it not ready
*                    b) by removing it from any wait lists
*                    c) by preventing OSTimeTick() from making the task ready to run.
*                 The task can then be 'unlinked' from the miscellaneous structures in uC/OS-II.
*              2) The function OS_Dummy() is called after OS_EXIT_CRITICAL() because, on most processors,
*                 the next instruction following the enable interrupt instruction is ignored.
*              3) An ISR cannot delete a task.
*              4) The lock nesting counter is incremented because, for a brief instant, if the current
*                 task is being deleted, the current task would not be able to be rescheduled because it
*                 is removed from the ready list.  Incrementing the nesting counter prevents another task
*                 from being schedule.  This means that an ISR would return to the current task which is
*                 being deleted.  The rest of the deletion would thus be able to be completed.
*********************************************************************************************************
*/

#if OS_TASK_DEL_EN > 0u
INT8U  OSTaskDel (INT8U prio)
{
#if (OS_FLAG_EN > 0u) && (OS_MAX_FLAGS > 0u)
    OS_FLAG_NODE *pnode;
#endif
    OS_TCB       *ptcb;
#if OS_CRITICAL_METHOD == 3u                           /*若临界区模式为第3种，则给CPU状态寄存器分配空间*/
    OS_CPU_SR     cpu_sr = 0u;
#endif



    if (OSIntNesting > 0u) {                            /* See if trying to delete from ISR            */
        return (OS_ERR_TASK_DEL_ISR);
    }
    if (prio == OS_TASK_IDLE_PRIO) {                    /* 不允许删除系统自带的空闲任务                */
        return (OS_ERR_TASK_DEL_IDLE);
    }
#if OS_ARG_CHK_EN > 0u
    if (prio >= OS_LOWEST_PRIO) {                       /* 检查任务优先级是否有效 ?                       */
        if (prio != OS_PRIO_SELF) {
            return (OS_ERR_PRIO_INVALID);
        }
    }
#endif

/*$PAGE*/
    OS_ENTER_CRITICAL();
    if (prio == OS_PRIO_SELF) {                         /* See if requesting to delete self            */
        prio = OSTCBCur->OSTCBPrio;                     /* Set priority to delete to current           */
    }
    ptcb = OSTCBPrioTbl[prio];
    if (ptcb == (OS_TCB *)0) {                          /* Task to delete must exist                   */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_NOT_EXIST);
    }
    if (ptcb == OS_TCB_RESERVED) {                      /* Must not be assigned to Mutex               */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_DEL);
    }

    OSRdyTbl[ptcb->OSTCBY] &= (OS_PRIO)~ptcb->OSTCBBitX;
    if (OSRdyTbl[ptcb->OSTCBY] == 0u) {                 /* Make task not ready                         */
        OSRdyGrp           &= (OS_PRIO)~ptcb->OSTCBBitY;
    }

#if (OS_EVENT_EN)
    if (ptcb->OSTCBEventPtr != (OS_EVENT *)0) {
        OS_EventTaskRemove(ptcb, ptcb->OSTCBEventPtr);  /* Remove this task from any event   wait list */
    }
#if (OS_EVENT_MULTI_EN > 0u)
    if (ptcb->OSTCBEventMultiPtr != (OS_EVENT **)0) {   /* Remove this task from any events' wait lists*/
        OS_EventTaskRemoveMulti(ptcb, ptcb->OSTCBEventMultiPtr);
    }
#endif
#endif

#if (OS_FLAG_EN > 0u) && (OS_MAX_FLAGS > 0u)
    pnode = ptcb->OSTCBFlagNode;
    if (pnode != (OS_FLAG_NODE *)0) {                   /* If task is waiting on event flag            */
        OS_FlagUnlink(pnode);                           /* Remove from wait list                       */
    }
#endif

    ptcb->OSTCBDly      = 0u;                           /* Prevent OSTimeTick() from updating          */
    ptcb->OSTCBStat     = OS_STAT_RDY;                  /* Prevent task from being resumed             */
    ptcb->OSTCBStatPend = OS_STAT_PEND_OK;
    if (OSLockNesting < 255u) {                         /* Make sure we don't context switch           */
        OSLockNesting++;
    }
    OS_EXIT_CRITICAL();                                 /* Enabling INT. ignores next instruc.         */
    OS_Dummy();                                         /* ... Dummy ensures that INTs will be         */
    OS_ENTER_CRITICAL();                                /* ... disabled HERE!                          */
    if (OSLockNesting > 0u) {                           /* Remove context switch lock                  */
        OSLockNesting--;
    }
    OSTaskDelHook(ptcb);                                /* Call user defined hook                      */

#if defined(OS_TLS_TBL_SIZE) && (OS_TLS_TBL_SIZE > 0u)
    OS_TLS_TaskDel(ptcb);                               /* Call TLS hook                               */
#endif

    OSTaskCtr--;                                        /* One less task being managed                 */
    OSTCBPrioTbl[prio] = (OS_TCB *)0;                   /* Clear old priority entry                    */
    if (ptcb->OSTCBPrev == (OS_TCB *)0) {               /* Remove from TCB chain                       */
        ptcb->OSTCBNext->OSTCBPrev = (OS_TCB *)0;
        OSTCBList                  = ptcb->OSTCBNext;
    } else {
        ptcb->OSTCBPrev->OSTCBNext = ptcb->OSTCBNext;
        ptcb->OSTCBNext->OSTCBPrev = ptcb->OSTCBPrev;
    }
    ptcb->OSTCBNext     = OSTCBFreeList;                /* Return TCB to free TCB list                 */
    OSTCBFreeList       = ptcb;
#if OS_TASK_NAME_EN > 0u
    ptcb->OSTCBTaskName = (INT8U *)(void *)"?";
#endif
    OS_EXIT_CRITICAL();
    if (OSRunning == OS_TRUE) {
        OS_Sched();                                     /* Find new highest priority task              */
    }
    return (OS_ERR_NONE);
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                                  REQUEST THAT A TASK DELETE ITSELF
*                                 （请求一个任务删除其他任务或本身）
*
* Description: 这个函数是用来：
*                   a) 通知一个任务删除它本身。
*                   b) 删除当前任务本身。
*              This function is a little tricky to understand.  Basically, you have a task that needs
*              to be deleted however, this task has resources that it has allocated (memory buffers,
*              semaphores, mailboxes, queues etc.).  The task cannot be deleted otherwise these
*              resources would not be freed.  The requesting task calls OSTaskDelReq() to indicate that
*              the task needs to be deleted.  Deleting of the task is however, deferred to the task to
*              be deleted.  For example, suppose that task #10 needs to be deleted.  The requesting task
*              example, task #5, would call OSTaskDelReq(10).  When task #10 gets to execute, it calls
*              this function by specifying OS_PRIO_SELF and monitors the returned value.  If the return
*              value is OS_ERR_TASK_DEL_REQ, another task requested a task delete.  Task #10 would look like
*              this:
*
*                   void Task(void *p_arg)
*                   {
*                       .
*                       .
*                       while (1) {
*                           OSTimeDly(1);
*                           if (OSTaskDelReq(OS_PRIO_SELF) == OS_ERR_TASK_DEL_REQ) {
*                               Release any owned resources;
*                               De-allocate any dynamic memory;
*                               OSTaskDel(OS_PRIO_SELF);
*                           }
*                       }
*                   }
*
* Arguments  : prio    is the priority of the task to request the delete from
*
* Returns    : OS_ERR_NONE            if the task exist and the request has been registered
*              OS_ERR_TASK_NOT_EXIST  if the task has been deleted.  This allows the caller to know whether
*                                     the request has been executed.
*              OS_ERR_TASK_DEL        if the task is assigned to a Mutex.
*              OS_ERR_TASK_DEL_IDLE   if you requested to delete uC/OS-II's idle task
*              OS_ERR_PRIO_INVALID    if the priority you specify is higher that the maximum allowed
*                                     (i.e. >= OS_LOWEST_PRIO) or, you have not specified OS_PRIO_SELF.
*              OS_ERR_TASK_DEL_REQ    if a task (possibly another task) requested that the running task be
*                                     deleted.
*********************************************************************************************************
*/
/*$PAGE*/
#if OS_TASK_DEL_EN > 0u
INT8U  OSTaskDelReq (INT8U prio)
{
    INT8U      stat;
    OS_TCB    *ptcb;
#if OS_CRITICAL_METHOD == 3u                     /* 若临界区模式为第3种，则给CPU状态寄存器分配空间     */
    OS_CPU_SR  cpu_sr = 0u;
#endif



    if (prio == OS_TASK_IDLE_PRIO) {                            /* Not allowed to delete idle task     */
        return (OS_ERR_TASK_DEL_IDLE);
    }
#if OS_ARG_CHK_EN > 0u
    if (prio >= OS_LOWEST_PRIO) {                               /* Task priority valid ?               */
        if (prio != OS_PRIO_SELF) {
            return (OS_ERR_PRIO_INVALID);
        }
    }
#endif
    if (prio == OS_PRIO_SELF) {                                 /* See if a task is requesting to ...  */
        OS_ENTER_CRITICAL();                                    /* ... this task to delete itself      */
        stat = OSTCBCur->OSTCBDelReq;                           /* Return request status to caller     */
        OS_EXIT_CRITICAL();
        return (stat);
    }
    OS_ENTER_CRITICAL();
    ptcb = OSTCBPrioTbl[prio];
    if (ptcb == (OS_TCB *)0) {                                  /* Task to delete must exist           */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_NOT_EXIST);                         /* Task must already be deleted        */
    }
    if (ptcb == OS_TCB_RESERVED) {                              /* Must NOT be assigned to a Mutex     */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_DEL);
    }
    ptcb->OSTCBDelReq = OS_ERR_TASK_DEL_REQ;                    /* Set flag indicating task to be DEL. */
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                                       GET THE NAME OF A TASK
*                                       （获取一个任务名称）
*
* Description: This function is called to obtain the name of a task.
*
* Arguments  : prio      is the priority of the task that you want to obtain the name from.
*
*              pname     is a pointer to a pointer to an ASCII string that will receive the name of the task.
*
*              perr      is a pointer to an error code that can contain one of the following values:
*
*                        OS_ERR_NONE                if the requested task is resumed
*                        OS_ERR_TASK_NOT_EXIST      if the task has not been created or is assigned to a Mutex
*                        OS_ERR_PRIO_INVALID        if you specified an invalid priority:
*                                                   A higher value than the idle task or not OS_PRIO_SELF.
*                        OS_ERR_PNAME_NULL          You passed a NULL pointer for 'pname'
*                        OS_ERR_NAME_GET_ISR        You called this function from an ISR
*
*
* Returns    : The length of the string or 0 if the task does not exist.
*********************************************************************************************************
*/

#if OS_TASK_NAME_EN > 0u
INT8U  OSTaskNameGet (INT8U    prio,
                      INT8U  **pname,
                      INT8U   *perr)
{
    OS_TCB    *ptcb;
    INT8U      len;
#if OS_CRITICAL_METHOD == 3u                        /* 若临界区模式为第3种，则给CPU状态寄存器分配空间  */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (prio > OS_LOWEST_PRIO) {                         /* Task priority valid ?                      */
        if (prio != OS_PRIO_SELF) {
            *perr = OS_ERR_PRIO_INVALID;                 /* No                                         */
            return (0u);
        }
    }
    if (pname == (INT8U **)0) {                          /* Is 'pname' a NULL pointer?                 */
        *perr = OS_ERR_PNAME_NULL;                       /* Yes                                        */
        return (0u);
    }
#endif
    if (OSIntNesting > 0u) {                              /* See if trying to call from an ISR          */
        *perr = OS_ERR_NAME_GET_ISR;
        return (0u);
    }
    OS_ENTER_CRITICAL();
    if (prio == OS_PRIO_SELF) {                          /* See if caller desires it's own name        */
        prio = OSTCBCur->OSTCBPrio;
    }
    ptcb = OSTCBPrioTbl[prio];
    if (ptcb == (OS_TCB *)0) {                           /* Does task exist?                           */
        OS_EXIT_CRITICAL();                              /* No                                         */
        *perr = OS_ERR_TASK_NOT_EXIST;
        return (0u);
    }
    if (ptcb == OS_TCB_RESERVED) {                       /* Task assigned to a Mutex?                  */
        OS_EXIT_CRITICAL();                              /* Yes                                        */
        *perr = OS_ERR_TASK_NOT_EXIST;
        return (0u);
    }
    *pname = ptcb->OSTCBTaskName;
    len    = OS_StrLen(*pname);
    OS_EXIT_CRITICAL();
    *perr  = OS_ERR_NONE;
    return (len);
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                       ASSIGN A NAME TO A TASK
*                                     （分配一个名称给一个任务）
*
* Description: This function is used to set the name of a task.
*
* Arguments  : prio      is the priority of the task that you want the assign a name to.
*
*              pname     is a pointer to an ASCII string that contains the name of the task.
*
*              perr       is a pointer to an error code that can contain one of the following values:
*
*                        OS_ERR_NONE                if the requested task is resumed
*                        OS_ERR_TASK_NOT_EXIST      if the task has not been created or is assigned to a Mutex
*                        OS_ERR_PNAME_NULL          You passed a NULL pointer for 'pname'
*                        OS_ERR_PRIO_INVALID        if you specified an invalid priority:
*                                                   A higher value than the idle task or not OS_PRIO_SELF.
*                        OS_ERR_NAME_SET_ISR        if you called this function from an ISR
*
* Returns    : None
*********************************************************************************************************
*/
#if OS_TASK_NAME_EN > 0u
void  OSTaskNameSet (INT8U   prio,
                     INT8U  *pname,
                     INT8U  *perr)
{
    OS_TCB    *ptcb;
#if OS_CRITICAL_METHOD == 3u                         /* 若临界区模式为第3种，则给CPU状态寄存器分配空间 */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (prio > OS_LOWEST_PRIO) {                     /* Task priority valid ?                          */
        if (prio != OS_PRIO_SELF) {
            *perr = OS_ERR_PRIO_INVALID;             /* No                                             */
            return;
        }
    }
    if (pname == (INT8U *)0) {                       /* Is 'pname' a NULL pointer?                     */
        *perr = OS_ERR_PNAME_NULL;                   /* Yes                                            */
        return;
    }
#endif
    if (OSIntNesting > 0u) {                         /* See if trying to call from an ISR              */
        *perr = OS_ERR_NAME_SET_ISR;
        return;
    }
    OS_ENTER_CRITICAL();
    if (prio == OS_PRIO_SELF) {                      /* See if caller desires to set it's own name     */
        prio = OSTCBCur->OSTCBPrio;
    }
    ptcb = OSTCBPrioTbl[prio];
    if (ptcb == (OS_TCB *)0) {                       /* Does task exist?                               */
        OS_EXIT_CRITICAL();                          /* No                                             */
        *perr = OS_ERR_TASK_NOT_EXIST;
        return;
    }
    if (ptcb == OS_TCB_RESERVED) {                   /* Task assigned to a Mutex?                      */
        OS_EXIT_CRITICAL();                          /* Yes                                            */
        *perr = OS_ERR_TASK_NOT_EXIST;
        return;
    }
    ptcb->OSTCBTaskName = pname;
    OS_EXIT_CRITICAL();
    *perr               = OS_ERR_NONE;
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                       RESUME A SUSPENDED TASK
*                                 （唤醒一个用OSTaskSuspend()挂起的任务）
*
* Description: This function is called to resume a previously suspended task.  This is the only call that
*              will remove an explicit task suspension.
*
* Arguments  : prio     唤醒任务的优先级
*
* Returns    : OS_ERR_NONE                无错, 唤醒成功！
*              OS_ERR_PRIO_INVALID        给定的优先级无效！ 超出了最大允许范围。
*                                         (i.e. >= OS_LOWEST_PRIO)
*              OS_ERR_TASK_RESUME_PRIO    该优先级任务不存在。
*              OS_ERR_TASK_NOT_EXIST      if the task is assigned to a Mutex PIP
*              OS_ERR_TASK_NOT_SUSPENDED  if the task to resume has not been suspended
*********************************************************************************************************
*/

#if OS_TASK_SUSPEND_EN > 0u
INT8U  OSTaskResume (INT8U prio)
{
    OS_TCB    *ptcb;
#if OS_CRITICAL_METHOD == 3u                         /* 若临界区模式为第3种，则给CPU状态寄存器分配空间 */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (prio >= OS_LOWEST_PRIO) {                             /* Make sure task priority is valid      */
        return (OS_ERR_PRIO_INVALID);
    }
#endif
    OS_ENTER_CRITICAL();
    ptcb = OSTCBPrioTbl[prio];
    if (ptcb == (OS_TCB *)0) {                                /* Task to suspend must exist            */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_RESUME_PRIO);
    }
    if (ptcb == OS_TCB_RESERVED) {                            /* See if assigned to Mutex              */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_NOT_EXIST);
    }
    if ((ptcb->OSTCBStat & OS_STAT_SUSPEND) != OS_STAT_RDY) { /* Task must be suspended                */
        ptcb->OSTCBStat &= (INT8U)~(INT8U)OS_STAT_SUSPEND;    /* Remove suspension                     */
        if (ptcb->OSTCBStat == OS_STAT_RDY) {                 /* See if task is now ready              */
            if (ptcb->OSTCBDly == 0u) {
                OSRdyGrp               |= ptcb->OSTCBBitY;    /* Yes, Make task ready to run           */
                OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;
                OS_EXIT_CRITICAL();
                if (OSRunning == OS_TRUE) {
                    OS_Sched();                               /* Find new highest priority task        */
                }
            } else {
                OS_EXIT_CRITICAL();
            }
        } else {                                              /* Must be pending on event              */
            OS_EXIT_CRITICAL();
        }
        return (OS_ERR_NONE);
    }
    OS_EXIT_CRITICAL();
    return (OS_ERR_TASK_NOT_SUSPENDED);
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                                           STACK CHECKING
*                                  （从统计任务中检查任务堆栈状态）
*
* Description: This function is called to check the amount of free memory left on the specified task's
*              stack.
*
* Arguments  : prio          is the task priority
*
*              p_stk_data    is a pointer to a data structure of type OS_STK_DATA.
*
* Returns    : OS_ERR_NONE            upon success
*              OS_ERR_PRIO_INVALID    if the priority you specify is higher that the maximum allowed
*                                     (i.e. > OS_LOWEST_PRIO) or, you have not specified OS_PRIO_SELF.
*              OS_ERR_TASK_NOT_EXIST  if the desired task has not been created or is assigned to a Mutex PIP
*              OS_ERR_TASK_OPT        if you did NOT specified OS_TASK_OPT_STK_CHK when the task was created
*              OS_ERR_PDATA_NULL      if 'p_stk_data' is a NULL pointer
*********************************************************************************************************
*/
#if (OS_TASK_STAT_STK_CHK_EN > 0u) && (OS_TASK_CREATE_EXT_EN > 0u)
INT8U  OSTaskStkChk (INT8U         prio,
                     OS_STK_DATA  *p_stk_data)
{
    OS_TCB    *ptcb;
    OS_STK    *pchk;
    INT32U     nfree;
    INT32U     size;
#if OS_CRITICAL_METHOD == 3u                           /*若临界区模式为第3种，则给CPU状态寄存器分配空间*/
    OS_CPU_SR  cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (prio > OS_LOWEST_PRIO) {                       /* Make sure task priority is valid             */
        if (prio != OS_PRIO_SELF) {
            return (OS_ERR_PRIO_INVALID);
        }
    }
    if (p_stk_data == (OS_STK_DATA *)0) {              /* Validate 'p_stk_data'                        */
        return (OS_ERR_PDATA_NULL);
    }
#endif
    p_stk_data->OSFree = 0u;                           /* Assume failure, set to 0 size                */
    p_stk_data->OSUsed = 0u;
    OS_ENTER_CRITICAL();
    if (prio == OS_PRIO_SELF) {                        /* See if check for SELF                        */
        prio = OSTCBCur->OSTCBPrio;
    }
    ptcb = OSTCBPrioTbl[prio];
    if (ptcb == (OS_TCB *)0) {                         /* Make sure task exist                         */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_NOT_EXIST);
    }
    if (ptcb == OS_TCB_RESERVED) {
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_NOT_EXIST);
    }
    if ((ptcb->OSTCBOpt & OS_TASK_OPT_STK_CHK) == 0u) { /* Make sure stack checking option is set      */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_OPT);
    }
    nfree = 0u;
    size  = ptcb->OSTCBStkSize;
    pchk  = ptcb->OSTCBStkBottom;
    OS_EXIT_CRITICAL();
#if OS_STK_GROWTH == 1u
    while (*pchk++ == (OS_STK)0) {                    /* Compute the number of zero entries on the stk */
        nfree++;
    }
#else
    while (*pchk-- == (OS_STK)0) {
        nfree++;
    }
#endif
    p_stk_data->OSFree = nfree;                       /* Store   number of free entries on the stk     */
    p_stk_data->OSUsed = size - nfree;                /* Compute number of entries used on the stk     */
    return (OS_ERR_NONE);
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                                           SUSPEND A TASK
*                                          （挂起一个任务）
*
* Description: This function is called to suspend a task.  The task can be the calling task if the
*              priority passed to OSTaskSuspend() is the priority of the calling task or OS_PRIO_SELF.
*
* Arguments  : prio     挂起任务的优先级.  If you specify OS_PRIO_SELF, the
*                       calling task will suspend itself and rescheduling will occur.
*
* Returns    : OS_ERR_NONE               if the requested task is suspended
*              OS_ERR_TASK_SUSPEND_IDLE  if you attempted to suspend the idle task which is not allowed.
*              OS_ERR_PRIO_INVALID       if the priority you specify is higher that the maximum allowed
*                                        (i.e. >= OS_LOWEST_PRIO) or, you have not specified OS_PRIO_SELF.
*              OS_ERR_TASK_SUSPEND_PRIO  if the task to suspend does not exist
*              OS_ERR_TASK_NOT_EXITS     if the task is assigned to a Mutex PIP
*
* Note       : You should use this function with great care.  If you suspend a task that is waiting for
*              an event (i.e. a message, a semaphore, a queue ...) you will prevent this task from
*              running when the event arrives.
*********************************************************************************************************
*/

#if OS_TASK_SUSPEND_EN > 0u
INT8U  OSTaskSuspend (INT8U prio)
{
    BOOLEAN    self;
    OS_TCB    *ptcb;
    INT8U      y;
#if OS_CRITICAL_METHOD == 3u                         /* 若临界区模式为第3种，则给CPU状态寄存器分配空间 */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (prio == OS_TASK_IDLE_PRIO) {                            /* 不允许挂起系统自带的空闲任务        */
        return (OS_ERR_TASK_SUSPEND_IDLE);
    }
    if (prio >= OS_LOWEST_PRIO) {                               /* 任务优先级是否超过允许范围          */
        if (prio != OS_PRIO_SELF) {
            return (OS_ERR_PRIO_INVALID);
        }
    }
#endif
    OS_ENTER_CRITICAL();
    if (prio == OS_PRIO_SELF) {                                 /* See if suspend SELF                 */
        prio = OSTCBCur->OSTCBPrio;
        self = OS_TRUE;
    } else if (prio == OSTCBCur->OSTCBPrio) {                   /* See if suspending self              */
        self = OS_TRUE;
    } else {
        self = OS_FALSE;                                        /* No suspending another task          */
    }
    ptcb = OSTCBPrioTbl[prio];
    if (ptcb == (OS_TCB *)0) {                                  /* Task to suspend must exist          */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_SUSPEND_PRIO);
    }
    if (ptcb == OS_TCB_RESERVED) {                              /* See if assigned to Mutex            */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_NOT_EXIST);
    }
    y            = ptcb->OSTCBY;
    OSRdyTbl[y] &= (OS_PRIO)~ptcb->OSTCBBitX;                   /* Make task not ready                 */
    if (OSRdyTbl[y] == 0u) {
        OSRdyGrp &= (OS_PRIO)~ptcb->OSTCBBitY;
    }
    ptcb->OSTCBStat |= OS_STAT_SUSPEND;                         /* Status of task is 'SUSPENDED'       */
    OS_EXIT_CRITICAL();
    if (self == OS_TRUE) {                                      /* Context switch only if SELF         */
        OS_Sched();                                             /* Find new highest priority task      */
    }
    return (OS_ERR_NONE);
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                                            QUERY A TASK
*                                        （查询 获取任务信息）
* Description: 调用该函数将获得一份完整的TCB信息。
*
* Arguments  : prio         获得TCB信息的任务的优先级。
*
*              p_task_data  OS_TCB类型的指针！ 用于存储获取的TCB信息。
*
* Returns    : OS_ERR_NONE            if the requested task is suspended
*              OS_ERR_PRIO_INVALID    if the priority you specify is higher that the maximum allowed
*                                     (i.e. > OS_LOWEST_PRIO) or, you have not specified OS_PRIO_SELF.
*              OS_ERR_PRIO            if the desired task has not been created
*              OS_ERR_TASK_NOT_EXIST  if the task is assigned to a Mutex PIP
*              OS_ERR_PDATA_NULL      if 'p_task_data' is a NULL pointer
*********************************************************************************************************
*/

#if OS_TASK_QUERY_EN > 0u
INT8U  OSTaskQuery (INT8U    prio,
                    OS_TCB  *p_task_data)
{
    OS_TCB    *ptcb;
#if OS_CRITICAL_METHOD == 3u                     /* 若临界区模式为第3种，则给CPU状态寄存器分配空间     */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (prio > OS_LOWEST_PRIO) {                 /* Task priority valid ?                              */
        if (prio != OS_PRIO_SELF) {
            return (OS_ERR_PRIO_INVALID);
        }
    }
    if (p_task_data == (OS_TCB *)0) {            /* Validate 'p_task_data'                             */
        return (OS_ERR_PDATA_NULL);
    }
#endif
    OS_ENTER_CRITICAL();
    if (prio == OS_PRIO_SELF) {                  /* See if suspend SELF                                */
        prio = OSTCBCur->OSTCBPrio;
    }
    ptcb = OSTCBPrioTbl[prio];
    if (ptcb == (OS_TCB *)0) {                   /* Task to query must exist                           */
        OS_EXIT_CRITICAL();
        return (OS_ERR_PRIO);
    }
    if (ptcb == OS_TCB_RESERVED) {               /* Task to query must not be assigned to a Mutex      */
        OS_EXIT_CRITICAL();
        return (OS_ERR_TASK_NOT_EXIST);
    }
                                                 /* Copy TCB into user storage area                    */
    OS_MemCopy((INT8U *)p_task_data, (INT8U *)ptcb, sizeof(OS_TCB));
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                              GET THE CURRENT VALUE OF A TASK REGISTER
*                                   （获取当前任务寄存器的值）
*
* Description: This function is called to obtain the current value of a task register.  Task registers
*              are application specific and can be used to store task specific values such as 'error
*              numbers' (i.e. errno), statistics, etc.  Each task register can hold a 32-bit value.
*
* Arguments  : prio      is the priority of the task you want to get the task register from.  If you
*                        specify OS_PRIO_SELF then the task register of the current task will be obtained.
*
*              id        is the 'id' of the desired task register.  Note that the 'id' must be less
*                        than OS_TASK_REG_TBL_SIZE
*
*              perr      is a pointer to a variable that will hold an error code related to this call.
*
*                        OS_ERR_NONE            if the call was successful
*                        OS_ERR_PRIO_INVALID    if you specified an invalid priority
*                        OS_ERR_ID_INVALID      if the 'id' is not between 0 and OS_TASK_REG_TBL_SIZE-1
*
* Returns    : The current value of the task's register or 0 if an error is detected.
*
* Note(s)    : The maximum number of task variables is 254
*********************************************************************************************************
*/

#if OS_TASK_REG_TBL_SIZE > 0u
INT32U  OSTaskRegGet (INT8U   prio,
                      INT8U   id,
                      INT8U  *perr)
{
#if OS_CRITICAL_METHOD == 3u                     /* 若临界区模式为第3种，则给CPU状态寄存器分配空间     */
    OS_CPU_SR  cpu_sr = 0u;
#endif
    INT32U     value;
    OS_TCB    *ptcb;



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (prio >= OS_LOWEST_PRIO) {
        if (prio != OS_PRIO_SELF) {
            *perr = OS_ERR_PRIO_INVALID;
            return (0u);
        }
    }
    if (id >= OS_TASK_REG_TBL_SIZE) {
        *perr = OS_ERR_ID_INVALID;
        return (0u);
    }
#endif
    OS_ENTER_CRITICAL();
    if (prio == OS_PRIO_SELF) {                  /* See if need to get register from current task      */
        ptcb = OSTCBCur;
    } else {
        ptcb = OSTCBPrioTbl[prio];
    }
    value = ptcb->OSTCBRegTbl[id];
    OS_EXIT_CRITICAL();
    *perr = OS_ERR_NONE;
    return (value);
}
#endif

/*$PAGE*/
/*
************************************************************************************************************************
*                                    ALLOCATE THE NEXT AVAILABLE TASK REGISTER ID
*                                 （获得一个任务寄存器ID,从而给任务动态分配注册ID）
*
* Description: This function is called to obtain a task register ID.  This function thus allows task registers IDs to be
*              allocated dynamically instead of statically.
*
* Arguments  : p_err       is a pointer to a variable that will hold an error code related to this call.
*
*                            OS_ERR_NONE               if the call was successful
*                            OS_ERR_NO_MORE_ID_AVAIL   if you are attempting to assign more task register IDs than you 
*                                                           have available through OS_TASK_REG_TBL_SIZE.
*
* Returns    : The next available task register 'id' or OS_TASK_REG_TBL_SIZE if an error is detected.
************************************************************************************************************************
*/

#if OS_TASK_REG_TBL_SIZE > 0u
INT8U  OSTaskRegGetID (INT8U  *perr)
{
#if OS_CRITICAL_METHOD == 3u                                    /* 若临界区模式为第3种，则给CPU状态寄存器分配空间     */
    OS_CPU_SR  cpu_sr = 0u;
#endif
    INT8U      id;


#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((INT8U)OS_TASK_REG_TBL_SIZE);
    }
#endif

    OS_ENTER_CRITICAL();
    if (OSTaskRegNextAvailID >= OS_TASK_REG_TBL_SIZE) {         /* See if we exceeded the number of IDs available     */
       *perr = OS_ERR_NO_MORE_ID_AVAIL;                         /* Yes, cannot allocate more task register IDs        */
        OS_EXIT_CRITICAL();
        return ((INT8U)OS_TASK_REG_TBL_SIZE);
    }
     
    id   = OSTaskRegNextAvailID;                                /* Assign the next available ID                       */
    OSTaskRegNextAvailID++;                                     /* Increment available ID for next request            */
    OS_EXIT_CRITICAL();
   *perr = OS_ERR_NONE;
    return (id);
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                              SET THE CURRENT VALUE OF A TASK VARIABLE
*
* Description: This function is called to change the current value of a task register.  Task registers
*              are application specific and can be used to store task specific values such as 'error
*              numbers' (i.e. errno), statistics, etc.  Each task register can hold a 32-bit value.
*
* Arguments  : prio      is the priority of the task you want to set the task register for.  If you
*                        specify OS_PRIO_SELF then the task register of the current task will be obtained.
*
*              id        is the 'id' of the desired task register.  Note that the 'id' must be less
*                        than OS_TASK_REG_TBL_SIZE
*
*              value     is the desired value for the task register.
*
*              perr      is a pointer to a variable that will hold an error code related to this call.
*
*                        OS_ERR_NONE            if the call was successful
*                        OS_ERR_PRIO_INVALID    if you specified an invalid priority
*                        OS_ERR_ID_INVALID      if the 'id' is not between 0 and OS_TASK_REG_TBL_SIZE-1
*
* Returns    : The current value of the task's variable or 0 if an error is detected.
*
* Note(s)    : The maximum number of task variables is 254
*********************************************************************************************************
*/

#if OS_TASK_REG_TBL_SIZE > 0u
void  OSTaskRegSet (INT8U    prio,
                    INT8U    id,
                    INT32U   value,
                    INT8U   *perr)
{
#if OS_CRITICAL_METHOD == 3u                     /* 若临界区模式为第3种，则给CPU状态寄存器分配空间     */
    OS_CPU_SR  cpu_sr = 0u;
#endif
    OS_TCB    *ptcb;


#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (prio >= OS_LOWEST_PRIO) {
        if (prio != OS_PRIO_SELF) {
            *perr = OS_ERR_PRIO_INVALID;
            return;
        }
    }
    if (id >= OS_TASK_REG_TBL_SIZE) {
        *perr = OS_ERR_ID_INVALID;
        return;
    }
#endif
    OS_ENTER_CRITICAL();
    if (prio == OS_PRIO_SELF) {                  /* See if need to get register from current task      */
        ptcb = OSTCBCur;
    } else {
        ptcb = OSTCBPrioTbl[prio];
    }
    ptcb->OSTCBRegTbl[id] = value;
    OS_EXIT_CRITICAL();
    *perr                 = OS_ERR_NONE;
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                    CATCH ACCIDENTAL TASK RETURN
*                                       （任务异常返回）
* Description: This function is called if a task accidentally returns without deleting itself.  In other
*              words, a task should either be an infinite loop or delete itself if it's done.
*
* Arguments  : none
*
* Returns    : none
*
* Note(s)    : 这个函数是uC/OS-II系统内部调用的函数，你的应用程序不能调用它。
*********************************************************************************************************
*/

void  OS_TaskReturn (void)
{
    OSTaskReturnHook(OSTCBCur);                   /* Call hook to let user decide on what to do        */

#if OS_TASK_DEL_EN > 0u
    (void)OSTaskDel(OS_PRIO_SELF);                /* Delete task if it accidentally returns!           */
#else
    for (;;) {
        OSTimeDly(OS_TICKS_PER_SEC);
    }
#endif
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                          CLEAR TASK STACK
*                                     （清除任务中没用到的堆栈）
*
* Description: This function is used to clear the stack of a task (i.e. write all zeros)
*
* Arguments  : pbos     is a pointer to the task's bottom of stack.  If the configuration constant
*                       OS_STK_GROWTH is set to 1, the stack is assumed to grow downward (i.e. from high
*                       memory to low memory).  'pbos' will thus point to the lowest (valid) memory
*                       location of the stack.  If OS_STK_GROWTH is set to 0, 'pbos' will point to the
*                       highest memory location of the stack and the stack will grow with increasing
*                       memory locations.  'pbos' MUST point to a valid 'free' data item.
*
*              size     is the number of 'stack elements' to clear.
*
*              opt      contains additional information (or options) about the behavior of the task.  The
*                       LOWER 8-bits are reserved by uC/OS-II while the upper 8 bits can be application
*                       specific.  See OS_TASK_OPT_??? in uCOS-II.H.
*
* Returns    : none
*********************************************************************************************************
*/
#if (OS_TASK_STAT_STK_CHK_EN > 0u) && (OS_TASK_CREATE_EXT_EN > 0u)
void  OS_TaskStkClr (OS_STK  *pbos,
                     INT32U   size,
                     INT16U   opt)
{
    if ((opt & OS_TASK_OPT_STK_CHK) != 0x0000u) {      /* See if stack checking has been enabled       */
        if ((opt & OS_TASK_OPT_STK_CLR) != 0x0000u) {  /* 如果需要清楚堆栈                             */
#if OS_STK_GROWTH == 1u
            while (size > 0u) {                        /* 堆栈生长方向是从高到低的...                  */
                size--;
                *pbos++ = (OS_STK)0;                   /* 从底部开始清除                               */
            }
#else
            while (size > 0u) {                        /* Stack grows from LOW to HIGH memory          */
                size--;
                *pbos-- = (OS_STK)0;                   /* Clear from bottom of stack and down          */
            }
#endif
        }
    }
}

#endif
