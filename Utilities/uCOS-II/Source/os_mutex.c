/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*                                  MUTUAL EXCLUSION SEMAPHORE MANAGEMENT
*
*                              (c) Copyright 1992-2012, Micrium, Weston, FL
*                                           All Rights Reserved
*
* File    : OS_MUTEX.C
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


#if OS_MUTEX_EN > 0u
/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*                                             （局部常量）
*********************************************************************************************************
*/

#define  OS_MUTEX_KEEP_LOWER_8   ((INT16U)0x00FFu)
#define  OS_MUTEX_KEEP_UPPER_8   ((INT16U)0xFF00u)

#define  OS_MUTEX_AVAILABLE      ((INT16U)0x00FFu)

/*
*********************************************************************************************************
*                                           LOCAL FUNCTION
*                                             （局部函数）
*********************************************************************************************************
*/

static  void  OSMutex_RdyAtPrio(OS_TCB *ptcb, INT8U prio);

/*$PAGE*/
/*
*********************************************************************************************************
*                                  ACCEPT MUTUAL EXCLUSION SEMAPHORE
*                                         （检查(接收)互斥锁）
*
* Description: 这个函数是用于检查是否有互斥锁资源可用，或者是否有事件发生。
*
* Arguments  : pevent     is a pointer to the event control block
*
*              perr       is a pointer to an error code which will be returned to your application:
*                            OS_ERR_NONE         if the call was successful.
*                            OS_ERR_EVENT_TYPE   if 'pevent' is not a pointer to a mutex
*                            OS_ERR_PEVENT_NULL  'pevent' is a NULL pointer
*                            OS_ERR_PEND_ISR     if you called this function from an ISR
*                            OS_ERR_PCP_LOWER    If the priority of the task that owns the Mutex is
*                                                HIGHER (i.e. a lower number) than the PCP.  This error
*                                                indicates that you did not set the PCP higher (lower
*                                                number) than ALL the tasks that compete for the Mutex.
*                                                Unfortunately, this is something that could not be
*                                                detected when the Mutex is created because we don't know
*                                                what tasks will be using the Mutex.
*
* Returns    : == OS_TRUE    if the resource is available, the mutual exclusion semaphore is acquired
*              == OS_FALSE   a) if the resource is not available
*                            b) you didn't pass a pointer to a mutual exclusion semaphore
*                            c) you called this function from an ISR
*
* Warning(s) : This function CANNOT be called from an ISR because mutual exclusion semaphores are
*              intended to be used by tasks only.
*********************************************************************************************************
*/

#if OS_MUTEX_ACCEPT_EN > 0u
BOOLEAN  OSMutexAccept (OS_EVENT  *pevent,
                        INT8U     *perr)
{
    INT8U      pcp;                                    /* Priority Ceiling Priority (PCP)              */
#if OS_CRITICAL_METHOD == 3u                           /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (OS_FALSE);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                     /* Validate 'pevent'                            */
        *perr = OS_ERR_PEVENT_NULL;
        return (OS_FALSE);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_MUTEX) {  /* 验证'pevent',若指向空...                     */
        *perr = OS_ERR_EVENT_TYPE;
        return (OS_FALSE);
    }
    if (OSIntNesting > 0u) {                           /* Make sure it's not called from an ISR        */
        *perr = OS_ERR_PEND_ISR;
        return (OS_FALSE);
    }
    OS_ENTER_CRITICAL();                               /* Get value (0 or 1) of Mutex                  */
    pcp = (INT8U)(pevent->OSEventCnt >> 8u);           /* Get PCP from mutex                           */
    if ((pevent->OSEventCnt & OS_MUTEX_KEEP_LOWER_8) == OS_MUTEX_AVAILABLE) {
        pevent->OSEventCnt &= OS_MUTEX_KEEP_UPPER_8;   /*      Mask off LSByte (Acquire Mutex)         */
        pevent->OSEventCnt |= OSTCBCur->OSTCBPrio;     /*      Save current task priority in LSByte    */
        pevent->OSEventPtr  = (void *)OSTCBCur;        /*      Link TCB of task owning Mutex           */
        if ((pcp != OS_PRIO_MUTEX_CEIL_DIS) &&
            (OSTCBCur->OSTCBPrio <= pcp)) {            /*      PCP 'must' have a SMALLER prio ...      */
             OS_EXIT_CRITICAL();                       /*      ... than current task!                  */
            *perr = OS_ERR_PCP_LOWER;
        } else {
             OS_EXIT_CRITICAL();
            *perr = OS_ERR_NONE;
        }
        return (OS_TRUE);
    }
    OS_EXIT_CRITICAL();
    *perr = OS_ERR_NONE;
    return (OS_FALSE);
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                 CREATE A MUTUAL EXCLUSION SEMAPHORE
*                                       （创建一个互斥信号量）
*
* Description: 这个函数用于创建一个互斥信号量。
*
* Arguments  : prio          用于访问互斥信号量优先级。
*                            In other words, when the semaphore is acquired and a higher priority task
*                            attempts to obtain the semaphore then the priority of the task owning the
*                            semaphore is raised to this priority.  It is assumed that you will specify
*                            a priority that is LOWER in value than ANY of the tasks competing for the
*                            mutex. If the priority is specified as OS_PRIO_MUTEX_CEIL_DIS, then the
*                            priority ceiling promotion is disabled. This way, the tasks accessing the
*                            semaphore do not have their priority promoted.
*
*              perr          指向出错代码的指针, 为以下值之一:
*                               OS_ERR_NONE         没错,调用成功。
*                               OS_ERR_CREATE_ISR   if you attempted to create a MUTEX from an ISR
*                               OS_ERR_PRIO_EXIST   if a task at the priority ceiling priority
*                                                   already exist.
*                               OS_ERR_PEVENT_NULL  No more event control blocks available.
*                               OS_ERR_PRIO_INVALID if the priority you specify is higher that the
*                                                   maximum allowed (i.e. > OS_LOWEST_PRIO)
*
* Returns    : != (void *)0  返回创建成功的互斥信号量事件指针。
*              == (void *)0  创建互斥量失败。
*
* Note(s)    : 1) The LEAST significant 8 bits of '.OSEventCnt' hold the priority number of the task
*                 owning the mutex or 0xFF if no task owns the mutex.
*
*              2) The MOST  significant 8 bits of '.OSEventCnt' hold the priority number used to
*                 reduce priority inversion or 0xFF (OS_PRIO_MUTEX_CEIL_DIS) if priority ceiling
*                 promotion is disabled.
*********************************************************************************************************
*/

OS_EVENT  *OSMutexCreate (INT8U   prio,
                          INT8U  *perr)
{
    OS_EVENT  *pevent;
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_EVENT *)0);
    }
#endif

#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_EVENT *)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (prio != OS_PRIO_MUTEX_CEIL_DIS) {
        if (prio >= OS_LOWEST_PRIO) {                      /* 验证 PCP                                 */
           *perr = OS_ERR_PRIO_INVALID;
            return ((OS_EVENT *)0);
        }
    }
#endif
    if (OSIntNesting > 0u) {                               /* 若从ISR中调用该函数...                   */
        *perr = OS_ERR_CREATE_ISR;                         /* ...创建互斥信号量失败                    */
        return ((OS_EVENT *)0);
    }
    OS_ENTER_CRITICAL();
    if (prio != OS_PRIO_MUTEX_CEIL_DIS) {
        if (OSTCBPrioTbl[prio] != (OS_TCB *)0) {           /* 若互斥优先级任务存在(不允许),即已用TP... */
            OS_EXIT_CRITICAL();                            /* ...任务优先级已存在                      */
           *perr = OS_ERR_PRIO_EXIST;
            return ((OS_EVENT *)0);                        /* ...创建互斥信号量失败                    */
        }
        OSTCBPrioTbl[prio] = OS_TCB_RESERVED;              /*占用这个优先级(保留这个优先级任务列表)    */
    }

    pevent = OSEventFreeList;                              /* 获取当前空ECB                            */
    if (pevent == (OS_EVENT *)0) {                         /* 若当前ECB不可用...                       */
        if (prio != OS_PRIO_MUTEX_CEIL_DIS) {
            OSTCBPrioTbl[prio] = (OS_TCB *)0;              /* 释放这个优先级                           */
        }
        OS_EXIT_CRITICAL();
       *perr = OS_ERR_PEVENT_NULL;                         /* 没有多余可用ECB                          */
        return (pevent);                                   /* 创建互斥信号量失败                       */
    }
    OSEventFreeList     = (OS_EVENT *)OSEventFreeList->OSEventPtr; /* 获取空ECB(初始化开辟出来的空间)  */
    OS_EXIT_CRITICAL();
    pevent->OSEventType = OS_EVENT_TYPE_MUTEX;                     /* 事件类型为互斥型信号量           */
    pevent->OSEventCnt  = (INT16U)((INT16U)prio << 8u) | OS_MUTEX_AVAILABLE; /* 资源可用               */
    pevent->OSEventPtr  = (void *)0;                       /* 没有任务拥有互斥锁                       */
#if OS_EVENT_NAME_EN > 0u
    pevent->OSEventName = (INT8U *)(void *)"?";
#endif
    OS_EventWaitListInit(pevent);                          /* 初始化等待的ECB                          */
   *perr = OS_ERR_NONE;
    return (pevent);                                       /* 创建互斥信号量成功                       */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                           DELETE A MUTEX
*                                         （删除一个互斥锁）
*
* Description: 这个函数用于删除一个互斥信号量，分不同情况来删除。
*
* Arguments  : pevent        指向互斥信号量（事件）的指针。
*
*              opt           删除选项如下:
*                            opt == OS_DEL_NO_PEND   Delete mutex ONLY if no task pending
*                            opt == OS_DEL_ALWAYS    Deletes the mutex even if tasks are waiting.
*                                                    In this case, all the tasks pending will be readied.
*
*              perr          指向出错代码的指针, 为以下值之一:
*                            OS_ERR_NONE             The call was successful and the mutex was deleted
*                            OS_ERR_DEL_ISR          If you attempted to delete the MUTEX from an ISR
*                            OS_ERR_INVALID_OPT      An invalid option was specified
*                            OS_ERR_TASK_WAITING     One or more tasks were waiting on the mutex
*                            OS_ERR_EVENT_TYPE       If you didn't pass a pointer to a mutex
*                            OS_ERR_PEVENT_NULL      If 'pevent' is a NULL pointer.
*
* Returns    : pevent        upon error
*              (OS_EVENT *)0 if the mutex was successfully deleted.
*
* Note(s)    : 1) This function must be used with care.  Tasks that would normally expect the presence of
*                 the mutex MUST check the return code of OSMutexPend().
*
*              2) This call can potentially disable interrupts for a long time.  The interrupt disable
*                 time is directly proportional to the number of tasks waiting on the mutex.
*
*              3) Because ALL tasks pending on the mutex will be readied, you MUST be careful because the
*                 resource(s) will no longer be guarded by the mutex.
*
*              4) IMPORTANT: In the 'OS_DEL_ALWAYS' case, we assume that the owner of the Mutex (if there
*                            is one) is ready-to-run and is thus NOT pending on another kernel object or
*                            has delayed itself.  In other words, if a task owns the mutex being deleted,
*                            that task will be made ready-to-run at its original priority.
*********************************************************************************************************
*/

#if OS_MUTEX_DEL_EN > 0u
OS_EVENT  *OSMutexDel (OS_EVENT  *pevent,
                       INT8U      opt,
                       INT8U     *perr)
{
    BOOLEAN    tasks_waiting;
    OS_EVENT  *pevent_return;
    INT8U      pcp;                                        /* Priority ceiling priority                */
    INT8U      prio;
    OS_TCB    *ptcb;
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_EVENT *)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                         /* Validate 'pevent'                        */
        *perr = OS_ERR_PEVENT_NULL;
        return (pevent);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_MUTEX) {      /* Validate event block type                */
        *perr = OS_ERR_EVENT_TYPE;
        return (pevent);
    }
    if (OSIntNesting > 0u) {                               /* See if called from ISR ...               */
        *perr = OS_ERR_DEL_ISR;                             /* ... can't DELETE from an ISR             */
        return (pevent);
    }
    OS_ENTER_CRITICAL();
    if (pevent->OSEventGrp != 0u) {                        /* See if any tasks waiting on mutex        */
        tasks_waiting = OS_TRUE;                           /* Yes                                      */
    } else {
        tasks_waiting = OS_FALSE;                          /* No                                       */
    }
    switch (opt) {
        case OS_DEL_NO_PEND:                               /* 没有任务等待该互斥锁时,删除该互斥事件    */
             if (tasks_waiting == OS_FALSE) {
#if OS_EVENT_NAME_EN > 0u
                 pevent->OSEventName   = (INT8U *)(void *)"?";
#endif
                 pcp                   = (INT8U)(pevent->OSEventCnt >> 8u);
                 if (pcp != OS_PRIO_MUTEX_CEIL_DIS) {
                     OSTCBPrioTbl[pcp] = (OS_TCB *)0;      /* Free up the PCP                          */
                 }
                 pevent->OSEventType   = OS_EVENT_TYPE_UNUSED;
                 pevent->OSEventPtr    = OSEventFreeList;  /* Return Event Control Block to free list  */
                 pevent->OSEventCnt    = 0u;
                 OSEventFreeList       = pevent;
                 OS_EXIT_CRITICAL();
                 *perr                 = OS_ERR_NONE;
                 pevent_return         = (OS_EVENT *)0;    /* Mutex has been deleted                   */
             } else {
                 OS_EXIT_CRITICAL();
                 *perr                 = OS_ERR_TASK_WAITING;
                 pevent_return         = pevent;
             }
             break;

        case OS_DEL_ALWAYS:                              /* 任何情况下(即使有任务等待该Mutex)删除Mutex */
             pcp  = (INT8U)(pevent->OSEventCnt >> 8u);                       /* Get PCP of mutex       */
             if (pcp != OS_PRIO_MUTEX_CEIL_DIS) {
                 prio = (INT8U)(pevent->OSEventCnt & OS_MUTEX_KEEP_LOWER_8); /* Get owner's orig prio  */
                 ptcb = (OS_TCB *)pevent->OSEventPtr;
                 if (ptcb != (OS_TCB *)0) {                /* See if any task owns the mutex           */
                     if (ptcb->OSTCBPrio == pcp) {         /* See if original prio was changed         */
                         OSMutex_RdyAtPrio(ptcb, prio);    /* Yes, Restore the task's original prio    */
                     }
                 }
             }
             while (pevent->OSEventGrp != 0u) {            /* Ready ALL tasks waiting for mutex        */
                 (void)OS_EventTaskRdy(pevent, (void *)0, OS_STAT_MUTEX, OS_STAT_PEND_ABORT);
             }
#if OS_EVENT_NAME_EN > 0u
             pevent->OSEventName   = (INT8U *)(void *)"?";
#endif
             pcp                   = (INT8U)(pevent->OSEventCnt >> 8u);
             if (pcp != OS_PRIO_MUTEX_CEIL_DIS) {
                 OSTCBPrioTbl[pcp] = (OS_TCB *)0;          /* Free up the PCP                          */
             }
             pevent->OSEventType   = OS_EVENT_TYPE_UNUSED;
             pevent->OSEventPtr    = OSEventFreeList;      /* Return Event Control Block to free list  */
             pevent->OSEventCnt    = 0u;
             OSEventFreeList       = pevent;               /* Get next free event control block        */
             OS_EXIT_CRITICAL();
             if (tasks_waiting == OS_TRUE) {               /* Reschedule only if task(s) were waiting  */
                 OS_Sched();                               /* Find highest priority task ready to run  */
             }
             *perr         = OS_ERR_NONE;
             pevent_return = (OS_EVENT *)0;                /* Mutex has been deleted                   */
             break;

        default:
             OS_EXIT_CRITICAL();
             *perr         = OS_ERR_INVALID_OPT;
             pevent_return = pevent;
             break;
    }
    return (pevent_return);
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                 PEND ON MUTUAL EXCLUSION SEMAPHORE
*                                        （等待互斥信号量）
*
* Description: 这个函数用于等待一个互斥信号量。
*
* Arguments  : pevent        指向互斥信号量（事件）的指针。
*
*              timeout       一个可选的超时时间 (时钟滴答数)。
*                            != 0  你的任务将等待指定的资源（互斥信号量）到超时时间。
*                            == 0  你的任务将等待指定的资源（互斥信号量），直到资源可用（或事件发生）为止。
*
*              perr          指向包含错误码的变量的指针。  可能错误的消息:
*                               OS_ERR_NONE        The call was successful and your task owns the mutex
*                               OS_ERR_TIMEOUT     The mutex was not available within the specified 'timeout'.
*                               OS_ERR_PEND_ABORT  The wait on the mutex was aborted.
*                               OS_ERR_EVENT_TYPE  If you didn't pass a pointer to a mutex
*                               OS_ERR_PEVENT_NULL 'pevent' is a NULL pointer
*                               OS_ERR_PEND_ISR    If you called this function from an ISR and the result
*                                                  would lead to a suspension.
*                               OS_ERR_PCP_LOWER   If the priority of the task that owns the Mutex is
*                                                  HIGHER (i.e. a lower number) than the PCP.  This error
*                                                  indicates that you did not set the PCP higher (lower
*                                                  number) than ALL the tasks that compete for the Mutex.
*                                                  Unfortunately, this is something that could not be
*                                                  detected when the Mutex is created because we don't know
*                                                  what tasks will be using the Mutex.
*                               OS_ERR_PEND_LOCKED If you called this function when the scheduler is locked
*
* Returns    : none
*
* Note(s)    : 1) The task that owns the Mutex MUST NOT pend on any other event while it owns the mutex.
*
*              2) You MUST NOT change the priority of the task that owns the mutex
*********************************************************************************************************
*/

void  OSMutexPend (OS_EVENT  *pevent,
                   INT32U     timeout,
                   INT8U     *perr)
{
    INT8U      pcp;                                        /* Priority Ceiling Priority (PCP)          */
    INT8U      mprio;                                      /* Mutex owner priority                     */
    BOOLEAN    rdy;                                        /* Flag indicating task was ready           */
    OS_TCB    *ptcb;
    OS_EVENT  *pevent2;
    INT8U      y;
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                         /* 验证'pevent',若指向空...                 */
        *perr = OS_ERR_PEVENT_NULL;
        return;
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_MUTEX) {      /* 验证ECB类型，若不是互斥信号量类型...     */
        *perr = OS_ERR_EVENT_TYPE;
        return;
    }
    if (OSIntNesting > 0u) {                               /* See if called from ISR ...               */
        *perr = OS_ERR_PEND_ISR;                           /* ... can't PEND from an ISR               */
        return;
    }
    if (OSLockNesting > 0u) {                              /* See if called with scheduler locked ...  */
        *perr = OS_ERR_PEND_LOCKED;                        /* ... can't PEND when locked               */
        return;
    }
/*$PAGE*/
    OS_ENTER_CRITICAL();
    pcp = (INT8U)(pevent->OSEventCnt >> 8u);               /* 获取互斥事件优先级MutexPCP               */
                                                           /* 若互斥事件可用...                        */
    if ((INT8U)(pevent->OSEventCnt & OS_MUTEX_KEEP_LOWER_8) == OS_MUTEX_AVAILABLE) {
        pevent->OSEventCnt &= OS_MUTEX_KEEP_UPPER_8;       /* 保存互斥事件优先级(MutexPCP高8位)        */
        pevent->OSEventCnt |= OSTCBCur->OSTCBPrio;         /* 保存拥有Mutex任务优先级(OwnerPrio低8位)  */
        pevent->OSEventPtr  = (void *)OSTCBCur;            /* 事件指针指向占用Mutex的任务(OwnerTask)   */
        if ((pcp != OS_PRIO_MUTEX_CEIL_DIS) &&
            (OSTCBCur->OSTCBPrio <= pcp)) {                /* 若OwnerPrio < MutexPCP ...               */
             OS_EXIT_CRITICAL();
            *perr = OS_ERR_PCP_LOWER;                      /* MutexPCP太低                             */
        } else {
             OS_EXIT_CRITICAL();
            *perr = OS_ERR_NONE;                           /* 没有错误                                 */
        }
        return;
    }
    if (pcp != OS_PRIO_MUTEX_CEIL_DIS) {                             /*  若MutexPCP不可用...           */
        mprio = (INT8U)(pevent->OSEventCnt & OS_MUTEX_KEEP_LOWER_8); /*  获取OwnerPrio                 */
        ptcb  = (OS_TCB *)(pevent->OSEventPtr);                      /*  获取OwnerTask                 */
        if (ptcb->OSTCBPrio > pcp) {                              /* 若OwnerPrio > MutexPCP            */
            if (mprio > OSTCBCur->OSTCBPrio) {                    /* 若OwnerPrio > 当前任务的Prio      */
                y = ptcb->OSTCBY;                                 /* OwnerPrio组                       */
                if ((OSRdyTbl[y] & ptcb->OSTCBBitX) != 0u) {      /* 若OwnerTask处于就绪状态...        */
                    OSRdyTbl[y] &= (OS_PRIO)~ptcb->OSTCBBitX;     /*     Yes, Remove owner from Rdy ...*/
                    if (OSRdyTbl[y] == 0u) {                      /*          ... list at current prio */
                        OSRdyGrp &= (OS_PRIO)~ptcb->OSTCBBitY;
                    }
                    rdy = OS_TRUE;                                /* 就绪状态                          */
                } else {                                          /* OwnerTask处于未就绪状态...        */
                    pevent2 = ptcb->OSTCBEventPtr;
                    if (pevent2 != (OS_EVENT *)0) {               /* 从等待列表中移除                  */
                        y = ptcb->OSTCBY;
                        pevent2->OSEventTbl[y] &= (OS_PRIO)~ptcb->OSTCBBitX;
                        if (pevent2->OSEventTbl[y] == 0u) {
                            pevent2->OSEventGrp &= (OS_PRIO)~ptcb->OSTCBBitY;
                        }
                    }
                    rdy = OS_FALSE;                               /* 未就绪状态                          */
                }

                ptcb->OSTCBPrio = pcp;                            /* 改变OwnerPrio                     */
#if OS_LOWEST_PRIO <= 63u
                ptcb->OSTCBY    = (INT8U)( ptcb->OSTCBPrio >> 3u);
                ptcb->OSTCBX    = (INT8U)( ptcb->OSTCBPrio & 0x07u);
#else
                ptcb->OSTCBY    = (INT8U)((INT8U)(ptcb->OSTCBPrio >> 4u) & 0xFFu);
                ptcb->OSTCBX    = (INT8U)( ptcb->OSTCBPrio & 0x0Fu);
#endif
                ptcb->OSTCBBitY = (OS_PRIO)(1uL << ptcb->OSTCBY);
                ptcb->OSTCBBitX = (OS_PRIO)(1uL << ptcb->OSTCBX);

                if (rdy == OS_TRUE) {                      /* If task was ready at owner's priority ...*/
                    OSRdyGrp               |= ptcb->OSTCBBitY; /* ... make it ready at new priority.   */
                    OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;
                } else {
                    pevent2 = ptcb->OSTCBEventPtr;
                    if (pevent2 != (OS_EVENT *)0) {        /* Add to event wait list                   */
                        pevent2->OSEventGrp               |= ptcb->OSTCBBitY;
                        pevent2->OSEventTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;
                    }
                }
                OSTCBPrioTbl[pcp] = ptcb;
            }
        }
    }
    OSTCBCur->OSTCBStat     |= OS_STAT_MUTEX;         /* 互斥信号量不可用, 挂起当前任务                */
    OSTCBCur->OSTCBStatPend  = OS_STAT_PEND_OK;
    OSTCBCur->OSTCBDly       = timeout;               /* 把超时时间保存在OSTCBDly                      */
    OS_EventTaskWait(pevent);                         /* 暂停任务，直到事件发生或超时                  */
    OS_EXIT_CRITICAL();
    OS_Sched();                                       /* 查找最高优先级就绪任务,使其运行               */

    /************ 任务挂起后,恢复执行开始处 ************************************************************/
    OS_ENTER_CRITICAL();
    switch (OSTCBCur->OSTCBStatPend) {                /* See if we timed-out or aborted                */
        case OS_STAT_PEND_OK:
             *perr = OS_ERR_NONE;
             break;

        case OS_STAT_PEND_ABORT:
             *perr = OS_ERR_PEND_ABORT;               /* 表明中止了互斥锁                              */
             break;

        case OS_STAT_PEND_TO:
        default:
             OS_EventTaskRemove(OSTCBCur, pevent);
             *perr = OS_ERR_TIMEOUT;                  /* 表明任务超时                                  */
             break;
    }
    OSTCBCur->OSTCBStat          =  OS_STAT_RDY;      /* 设置任务状态：就绪                            */
    OSTCBCur->OSTCBStatPend      =  OS_STAT_PEND_OK;  /* 清除任务挂起状态                              */
    OSTCBCur->OSTCBEventPtr      = (OS_EVENT  *)0;    /* 清除事件指针                                  */
#if (OS_EVENT_MULTI_EN > 0u)
    OSTCBCur->OSTCBEventMultiPtr = (OS_EVENT **)0;
#endif
    OS_EXIT_CRITICAL();
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                POST TO A MUTUAL EXCLUSION SEMAPHORE
*                                      （释放一个互斥型信号量）
*
* Description: 这个函数用于释放一个互斥型信号量。
*
* Arguments  : pevent              指向互斥型信号量（事件）的指针。
*
* Returns    : OS_ERR_NONE             The call was successful and the mutex was signaled.
*              OS_ERR_EVENT_TYPE       If you didn't pass a pointer to a mutex
*              OS_ERR_PEVENT_NULL      'pevent' is a NULL pointer
*              OS_ERR_POST_ISR         Attempted to post from an ISR (not valid for MUTEXes)
*              OS_ERR_NOT_MUTEX_OWNER  The task that did the post is NOT the owner of the MUTEX.
*              OS_ERR_PCP_LOWER        If the priority of the new task that owns the Mutex is
*                                      HIGHER (i.e. a lower number) than the PCP.  This error
*                                      indicates that you did not set the PCP higher (lower
*                                      number) than ALL the tasks that compete for the Mutex.
*                                      Unfortunately, this is something that could not be
*                                      detected when the Mutex is created because we don't know
*                                      what tasks will be using the Mutex.
*********************************************************************************************************
*/

INT8U  OSMutexPost (OS_EVENT *pevent)
{
    INT8U      pcp;                                   /* Priority ceiling priority                     */
    INT8U      prio;
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif



    if (OSIntNesting > 0u) {                          /* See if called from ISR ...                    */
        return (OS_ERR_POST_ISR);                     /* ... can't POST mutex from an ISR              */
    }
#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                    /* 验证'pevent',若指向空...                      */
        return (OS_ERR_PEVENT_NULL);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_MUTEX) { /* 验证ECB类型，若不是互斥信号量类型...          */
        return (OS_ERR_EVENT_TYPE);
    }
    OS_ENTER_CRITICAL();
    pcp  = (INT8U)(pevent->OSEventCnt >> 8u);         /* 获取MutexPCP 及OwnerPrio                      */
    prio = (INT8U)(pevent->OSEventCnt & OS_MUTEX_KEEP_LOWER_8);
    if (OSTCBCur != (OS_TCB *)pevent->OSEventPtr) {   /* 若当前事件指针不指向OwnerTask...              */
        OS_EXIT_CRITICAL();                           /* 和Sem相比,多这一句判断! 不同的地方也就在次    */
        return (OS_ERR_NOT_MUTEX_OWNER);
    }
    if (pcp != OS_PRIO_MUTEX_CEIL_DIS) {
        if (OSTCBCur->OSTCBPrio == pcp) {             /* 若当前任务Prio == MutexPCP...                 */
            OSMutex_RdyAtPrio(OSTCBCur, prio);        /* 还原任务优先级                                */
        }
        OSTCBPrioTbl[pcp] = OS_TCB_RESERVED;          /* Reserve table entry                           */
    }
    if (pevent->OSEventGrp != 0u) {                   /* 若有任务等待该互斥型信号量...                 */
                                                      /* Yes, 查找等待互斥事件HPT,使其就绪             */
        prio                = OS_EventTaskRdy(pevent, (void *)0, OS_STAT_MUTEX, OS_STAT_PEND_OK);
        pevent->OSEventCnt &= OS_MUTEX_KEEP_UPPER_8;  /* 保存PCP                                       */
        pevent->OSEventCnt |= prio;                   /* 保存新OwnerPrio                               */
        pevent->OSEventPtr  = OSTCBPrioTbl[prio];     /* 连接新OwnerTask                               */
        if ((pcp  != OS_PRIO_MUTEX_CEIL_DIS) &&
            (prio <= pcp)) {                          /* 若OwnerPrio < MutexPCP ...                    */
            OS_EXIT_CRITICAL();                       /* ... 不允许                                    */
            OS_Sched();                               /* 查找最高优先级就绪任务,使其运行               */
            return (OS_ERR_PCP_LOWER);                /* MutexPCP太低, 返回错误                        */
        } else {
            OS_EXIT_CRITICAL();
            OS_Sched();                               /* 查找最高优先级就绪任务,使其运行               */
            return (OS_ERR_NONE);                     /* 返回无错误                                    */
        }
    }
    pevent->OSEventCnt |= OS_MUTEX_AVAILABLE;         /* 没有任务等待互斥量, 互斥量置为可用            */
    pevent->OSEventPtr  = (void *)0;
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                 QUERY A MUTUAL EXCLUSION SEMAPHORE
*                                        （查询互斥信号量）
*
* Description: 这个函数是用于获取互斥信号量信息。
*
* Arguments  : pevent          指向互斥信号量（事件）的指针。
*
*              p_mutex_data    指向互斥信号量数据结构的指针。
*
* Returns    : OS_ERR_NONE          The call was successful and the message was sent
*              OS_ERR_QUERY_ISR     If you called this function from an ISR
*              OS_ERR_PEVENT_NULL   If 'pevent'       is a NULL pointer
*              OS_ERR_PDATA_NULL    If 'p_mutex_data' is a NULL pointer
*              OS_ERR_EVENT_TYPE    If you are attempting to obtain data from a non mutex.
*********************************************************************************************************
*/

#if OS_MUTEX_QUERY_EN > 0u
INT8U  OSMutexQuery (OS_EVENT       *pevent,
                     OS_MUTEX_DATA  *p_mutex_data)
{
    INT8U       i;
    OS_PRIO    *psrc;
    OS_PRIO    *pdest;
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR   cpu_sr = 0u;
#endif



    if (OSIntNesting > 0u) {                               /* See if called from ISR ...               */
        return (OS_ERR_QUERY_ISR);                         /* ... can't QUERY mutex from an ISR        */
    }
#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                         /* Validate 'pevent'                        */
        return (OS_ERR_PEVENT_NULL);
    }
    if (p_mutex_data == (OS_MUTEX_DATA *)0) {              /* Validate 'p_mutex_data'                  */
        return (OS_ERR_PDATA_NULL);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_MUTEX) {      /* Validate event block type                */
        return (OS_ERR_EVENT_TYPE);
    }
    OS_ENTER_CRITICAL();
    p_mutex_data->OSMutexPCP  = (INT8U)(pevent->OSEventCnt >> 8u);
    p_mutex_data->OSOwnerPrio = (INT8U)(pevent->OSEventCnt & OS_MUTEX_KEEP_LOWER_8);
    if (p_mutex_data->OSOwnerPrio == 0xFFu) {
        p_mutex_data->OSValue = OS_TRUE;
    } else {
        p_mutex_data->OSValue = OS_FALSE;
    }
    p_mutex_data->OSEventGrp  = pevent->OSEventGrp;        /* Copy wait list                           */
    psrc                      = &pevent->OSEventTbl[0];
    pdest                     = &p_mutex_data->OSEventTbl[0];
    for (i = 0u; i < OS_EVENT_TBL_SIZE; i++) {
        *pdest++ = *psrc++;
    }
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}
#endif                                                     /* OS_MUTEX_QUERY_EN                        */

/*$PAGE*/
/*
*********************************************************************************************************
*                            RESTORE A TASK BACK TO ITS ORIGINAL PRIORITY
*                                    （还原一个任务原来的优先级）
*
* Description: This function makes a task ready at the specified priority
*
* Arguments  : ptcb            is a pointer to OS_TCB of the task to make ready
*
*              prio            is the desired priority
*
* Returns    : none
*********************************************************************************************************
*/

static  void  OSMutex_RdyAtPrio (OS_TCB  *ptcb,
                                 INT8U    prio)
{
    INT8U  y;


    y            =  ptcb->OSTCBY;                          /* 清除ptcb在列表中的优先级                 */
    OSRdyTbl[y] &= (OS_PRIO)~ptcb->OSTCBBitX;
    if (OSRdyTbl[y] == 0u) {
        OSRdyGrp &= (OS_PRIO)~ptcb->OSTCBBitY;
    }
    ptcb->OSTCBPrio         = prio;                        /* 设定ptcb的优先级为指定值                 */
    OSPrioCur               = prio;                        /* 并设定当前优先级为指定值                 */
#if OS_LOWEST_PRIO <= 63u
    ptcb->OSTCBY            = (INT8U)((INT8U)(prio >> 3u) & 0x07u);
    ptcb->OSTCBX            = (INT8U)(prio & 0x07u);
#else
    ptcb->OSTCBY            = (INT8U)((INT8U)(prio >> 4u) & 0x0Fu);
    ptcb->OSTCBX            = (INT8U) (prio & 0x0Fu);
#endif
    ptcb->OSTCBBitY         = (OS_PRIO)(1uL << ptcb->OSTCBY);
    ptcb->OSTCBBitX         = (OS_PRIO)(1uL << ptcb->OSTCBX);
    OSRdyGrp               |= ptcb->OSTCBBitY;             /* Make task ready at original priority     */
    OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;
    OSTCBPrioTbl[prio]      = ptcb;
}


#endif                                                     /* OS_MUTEX_EN                              */
