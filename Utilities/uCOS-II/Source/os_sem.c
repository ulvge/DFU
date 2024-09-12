/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*                                          SEMAPHORE MANAGEMENT
*
*                              (c) Copyright 1992-2012, Micrium, Weston, FL
*                                           All Rights Reserved
*
* File    : OS_SEM.C
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

#if OS_SEM_EN > 0u
/*$PAGE*/
/*
*********************************************************************************************************
*                                          ACCEPT SEMAPHORE
*                                        （检查(接收)信号量）
*
* Description: 这个函数是用于检查是否有信号量资源可用，或者是否有事件发生。
*              不像OSSemPend(), 如果没有信号量资源可用也不挂起任务。
*
* Arguments  : pevent     指向信号量（事件）的指针。
*
* Returns    : >  0       if the resource is available or the event did not occur the semaphore is
*                         decremented to obtain the resource.
*              == 0       if the resource is not available or the event did not occur or,
*                         if 'pevent' is a NULL pointer or,
*                         if you didn't pass a pointer to a semaphore
*********************************************************************************************************
*/

#if OS_SEM_ACCEPT_EN > 0u
INT16U  OSSemAccept (OS_EVENT *pevent)
{
    INT16U     cnt;
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                    /* 验证'pevent'                                  */
        return (0u);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_SEM) {   /* 验证ECB类型，是否为信号量                     */
        return (0u);
    }
    OS_ENTER_CRITICAL();
    cnt = pevent->OSEventCnt;
    if (cnt > 0u) {                                   /* 若有信号量资源可用...                         */
        pevent->OSEventCnt--;                         /* ...将信号量计数减1                            */
    }
    OS_EXIT_CRITICAL();
    return (cnt);                                     /* 返回信号量计数值                              */
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                         CREATE A SEMAPHORE
*                                         （创建一个信号量）
*
* Description: 这个函数用于创建一个信号量。
*
* Arguments  : cnt           信号量初始值.  如果值为0,没有资源可用（或没有事件发生）。
*                            非零值来指定有多少资源可利用。(e.g: 你有10个资源，你初始化信号量值为10)
*
* Returns    : != (void *)0  返回创建成功的信号量事件指针。
*              == (void *)0  创建信号量失败。
*********************************************************************************************************
*/

OS_EVENT  *OSSemCreate (INT16U cnt)
{
    OS_EVENT  *pevent;
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr = 0u;
#endif


#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_EVENT *)0);
    }
#endif

    if (OSIntNesting > 0u) {                               /* 若从ISR中创建信号量 ...                  */
        return ((OS_EVENT *)0);                            /* ... 则创建信号量失败                     */
    }
    OS_ENTER_CRITICAL();
    pevent = OSEventFreeList;                              /* 获取当前空ECB                            */
    if (OSEventFreeList != (OS_EVENT *)0) {                /* 若还有剩余ECB(初始化时开辟出来的空间)・・  */
        OSEventFreeList = (OS_EVENT *)OSEventFreeList->OSEventPtr;
    }
    OS_EXIT_CRITICAL();
    if (pevent != (OS_EVENT *)0) {                         /* 当前获得的ECB                            */
        pevent->OSEventType    = OS_EVENT_TYPE_SEM;        /* 事件类型为OS_EVENT_TYPE_SEM              */
        pevent->OSEventCnt     = cnt;                      /* 设置信号量的值                           */
        pevent->OSEventPtr     = (void *)0;                /* 从空ECB中删除链接（将其置为:(void *)0）  */
#if OS_EVENT_NAME_EN > 0u
        pevent->OSEventName    = (INT8U *)(void *)"?";
#endif
        OS_EventWaitListInit(pevent);                      /* 初始化等待的事件(Sem)                    */
    }
    return (pevent);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                         DELETE A SEMAPHORE
*                                         （删除一个信号量）
*
* Description: 这个函数用于删除一个信号量，分不同情况来删除。
*
* Arguments  : pevent        指向信号量（事件）的指针。
*
*              opt           删除选项如下:
*                            opt == OS_DEL_NO_PEND   没有任务等待该信号量时才能删除成功; 若有任务等待时，则删除不成功。
*                            opt == OS_DEL_ALWAYS    任何情况下删除信号量（即使有任务等待该信号量）。
*
*              perr          指向出错代码的指针, 为以下值之一:
*                            OS_ERR_NONE             The call was successful and the semaphore was deleted
*                            OS_ERR_DEL_ISR          If you attempted to delete the semaphore from an ISR
*                            OS_ERR_INVALID_OPT      An invalid option was specified
*                            OS_ERR_TASK_WAITING     One or more tasks were waiting on the semaphore
*                            OS_ERR_EVENT_TYPE       If you didn't pass a pointer to a semaphore
*                            OS_ERR_PEVENT_NULL      If 'pevent' is a NULL pointer.
*
* Returns    : pevent        upon error
*              (OS_EVENT *)0 if the semaphore was successfully deleted.
*
* Note(s)    : 1) This function must be used with care.  Tasks that would normally expect the presence of
*                 the semaphore MUST check the return code of OSSemPend().
*              2) OSSemAccept() callers will not know that the intended semaphore has been deleted unless
*                 they check 'pevent' to see that it's a NULL pointer.
*              3) This call can potentially disable interrupts for a long time.  The interrupt disable
*                 time is directly proportional to the number of tasks waiting on the semaphore.
*              4) Because ALL tasks pending on the semaphore will be readied, you MUST be careful in
*                 applications where the semaphore is used for mutual exclusion because the resource(s)
*                 will no longer be guarded by the semaphore.
*              5) All tasks that were waiting for the semaphore will be readied and returned an 
*                 OS_ERR_PEND_ABORT if OSSemDel() was called with OS_DEL_ALWAYS
*********************************************************************************************************
*/

#if OS_SEM_DEL_EN > 0u
OS_EVENT  *OSSemDel (OS_EVENT  *pevent,
                     INT8U      opt,
                     INT8U     *perr)
{
    BOOLEAN    tasks_waiting;
    OS_EVENT  *pevent_return;
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
    if (pevent->OSEventType != OS_EVENT_TYPE_SEM) {        /* Validate event block type                */
        *perr = OS_ERR_EVENT_TYPE;
        return (pevent);
    }
    if (OSIntNesting > 0u) {                               /* See if called from ISR ...               */
        *perr = OS_ERR_DEL_ISR;                            /* ... can't DELETE from an ISR             */
        return (pevent);
    }
    OS_ENTER_CRITICAL();
    if (pevent->OSEventGrp != 0u) {                        /* 查看是否有其他任务在等待该信号量         */
        tasks_waiting = OS_TRUE;                           /* Yes 有任务在等待该信号量                 */
    } else {
        tasks_waiting = OS_FALSE;                          /* No                                       */
    }
    switch (opt) {
        case OS_DEL_NO_PEND:                               /* 没有任务等待该信号量时,删除该信号量      */
             if (tasks_waiting == OS_FALSE) {              /* 没有任务等待时...                        */
#if OS_EVENT_NAME_EN > 0u
                 pevent->OSEventName    = (INT8U *)(void *)"?";
#endif
                                                           /* 腾空事件                                 */
                 pevent->OSEventType    = OS_EVENT_TYPE_UNUSED;
                 pevent->OSEventPtr     = OSEventFreeList;
                 pevent->OSEventCnt     = 0u;
                 OSEventFreeList        = pevent;          /* 空指针指向这个事件(事件已经腾空出来了)   */
                 OS_EXIT_CRITICAL();
                 *perr                  = OS_ERR_NONE;
                 pevent_return          = (OS_EVENT *)0;   /* 事件删除成功                             */
             } else {                                      /* 有任务等待时...                          */
                 OS_EXIT_CRITICAL();
                 *perr                  = OS_ERR_TASK_WAITING;
                 pevent_return          = pevent;          /* 事件删除失败                             */
             }
             break;

        case OS_DEL_ALWAYS:                                /* 任何情况下(即使有任务等待该Sem)删除Sem   */
             while (pevent->OSEventGrp != 0u) {            /* 使所有等待任务就绪                       */
                 (void)OS_EventTaskRdy(pevent, (void *)0, OS_STAT_SEM, OS_STAT_PEND_ABORT);
             }
#if OS_EVENT_NAME_EN > 0u
             pevent->OSEventName    = (INT8U *)(void *)"?";
#endif
                                                           /* 腾空事件                                 */
             pevent->OSEventType    = OS_EVENT_TYPE_UNUSED;
             pevent->OSEventPtr     = OSEventFreeList;
             pevent->OSEventCnt     = 0u;
             OSEventFreeList        = pevent;              /* 空指针指向这个事件(事件已经腾空出来了)   */
             OS_EXIT_CRITICAL();
             if (tasks_waiting == OS_TRUE) {               /* 若有任务在等待该事件...                  */
                 OS_Sched();                               /* 查找最高优先级就绪任务,使其运行          */
             }
             *perr                  = OS_ERR_NONE;
             pevent_return          = (OS_EVENT *)0;       /* Semaphore has been deleted               */
             break;

        default:
             OS_EXIT_CRITICAL();
             *perr                  = OS_ERR_INVALID_OPT;
             pevent_return          = pevent;
             break;
    }
    return (pevent_return);
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                          PEND ON SEMAPHORE
*                                         （等待一个信号量）
* Description: 功能是使任务挂起，等待一个信号量。
*
* Arguments  : pevent        指向等待信号量（事件）的指针。
*                            该信号指针的值在创建该信号时得到（参看OSSemCreate ()）。
*
*              timeout       一个可选的超时时间 (时钟滴答数)。
*                            != 0  你的任务将等待指定的资源（信号量）到超时时间。
*                            == 0  你的任务将等待指定的资源（信号量），直到资源可用（或事件发生）为止。
*
*              perr          指向出错代码的指针, 为以下值之一:
*
*                            OS_ERR_NONE         The call was successful and your task owns the resource
*                                                or, the event you are waiting for occurred.
*                            OS_ERR_TIMEOUT      The semaphore was not received within the specified
*                                                'timeout'.
*                            OS_ERR_PEND_ABORT   The wait on the semaphore was aborted.
*                            OS_ERR_EVENT_TYPE   If you didn't pass a pointer to a semaphore.
*                            OS_ERR_PEND_ISR     If you called this function from an ISR and the result
*                                                would lead to a suspension.
*                            OS_ERR_PEVENT_NULL  If 'pevent' is a NULL pointer.
*                            OS_ERR_PEND_LOCKED  If you called this function when the scheduler is locked
*
* Returns    : none
*********************************************************************************************************
*/
/*$PAGE*/
void  OSSemPend (OS_EVENT  *pevent,
                 INT32U     timeout,
                 INT8U     *perr)
{
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                    /* 验证'pevent',若指向空...                     */
        *perr = OS_ERR_PEVENT_NULL;
        return;
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_SEM) {   /* 验证ECB类型，若不是信号量类型...              */
        *perr = OS_ERR_EVENT_TYPE;
        return;
    }
    if (OSIntNesting > 0u) {                          /* 若从ISR中调用该函数...                        */
        *perr = OS_ERR_PEND_ISR;                      /* ...调用该函数失败                             */
        return;
    }
    if (OSLockNesting > 0u) {                         /* See if called with scheduler locked ...       */
        *perr = OS_ERR_PEND_LOCKED;                   /* ... can't PEND when locked                    */
        return;
    }
    OS_ENTER_CRITICAL();
    if (pevent->OSEventCnt > 0u) {                    /* 若信号量大于0, 则还有资源可用 ...             */
        pevent->OSEventCnt--;                         /* ... 信号量计数减1                             */
        OS_EXIT_CRITICAL();
        *perr = OS_ERR_NONE;
        return;
    }
                                                      /* 否则, 必须等待事件发生                        */
    OSTCBCur->OSTCBStat     |= OS_STAT_SEM;           /* 当没有资源可用时, 等待信号量                  */
    OSTCBCur->OSTCBStatPend  = OS_STAT_PEND_OK;
    OSTCBCur->OSTCBDly       = timeout;               /* 把超时时间保存在OSTCBDly                      */
    OS_EventTaskWait(pevent);                         /* 暂停任务，直到事件发生或超时                  */
    OS_EXIT_CRITICAL();
    OS_Sched();                                       /* 查找最高优先级就绪任务,使其运行               */

    /************ 任务挂起后,恢复执行开始处 ************************************************************/
    OS_ENTER_CRITICAL();
    switch (OSTCBCur->OSTCBStatPend) {                /* 挂起状态                                      */
        case OS_STAT_PEND_OK:
             *perr = OS_ERR_NONE;
             break;

        case OS_STAT_PEND_ABORT:
             *perr = OS_ERR_PEND_ABORT;               /* Indicate that we aborted                      */
             break;

        case OS_STAT_PEND_TO:
        default:
             OS_EventTaskRemove(OSTCBCur, pevent);
             *perr = OS_ERR_TIMEOUT;                  /* Indicate that we didn't get event within TO   */
             break;
    }
                                                      /* 恢复执行...                                   */
    OSTCBCur->OSTCBStat          =  OS_STAT_RDY;      /* 设置任务状态为就绪态                          */
    OSTCBCur->OSTCBStatPend      =  OS_STAT_PEND_OK;  /* 清除等待状态                                  */
    OSTCBCur->OSTCBEventPtr      = (OS_EVENT  *)0;    /* 清除任务事件指针                              */
#if (OS_EVENT_MULTI_EN > 0u)
    OSTCBCur->OSTCBEventMultiPtr = (OS_EVENT **)0;
#endif
    OS_EXIT_CRITICAL();
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                    ABORT WAITING ON A SEMAPHORE
*                                        （中断信号量的等待）
*
* Description: 这个函数会中断目前在等待的信号量，让任务不再等待。
*              这个函数应用于故障而中断任务的等待。
*              有点类似于OSSemPost()这个函数的功能，发送信号量让任务不再等待这一次。
*
* Arguments  : pevent        指向信号量（事件）的指针。
*
*              opt           中段执行的类型:
*                            OS_PEND_OPT_NONE         中断等待事件(信号量)中HPT
*                            OS_PEND_OPT_BROADCAST    中断所有等待事件(信号量)，即广播。
*
*              perr          指向出错代码的指针, 为以下值之一:
*
*                            OS_ERR_NONE         No tasks were     waiting on the semaphore.
*                            OS_ERR_PEND_ABORT   At least one task waiting on the semaphore was readied
*                                                and informed of the aborted wait; check return value
*                                                for the number of tasks whose wait on the semaphore
*                                                was aborted.
*                            OS_ERR_EVENT_TYPE   If you didn't pass a pointer to a semaphore.
*                            OS_ERR_PEVENT_NULL  If 'pevent' is a NULL pointer.
*
* Returns    : == 0          if no tasks were waiting on the semaphore, or upon error.
*              >  0          if one or more tasks waiting on the semaphore are now readied and informed.
*********************************************************************************************************
*/

#if OS_SEM_PEND_ABORT_EN > 0u
INT8U  OSSemPendAbort (OS_EVENT  *pevent,
                       INT8U      opt,
                       INT8U     *perr)
{
    INT8U      nbr_tasks;
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                    /* Validate 'pevent'                             */
        *perr = OS_ERR_PEVENT_NULL;
        return (0u);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_SEM) {   /* Validate event block type                     */
        *perr = OS_ERR_EVENT_TYPE;
        return (0u);
    }
    OS_ENTER_CRITICAL();
    if (pevent->OSEventGrp != 0u) {                   /* 若有任务在等到信号量...                       */
        nbr_tasks = 0u;
        switch (opt) {
            case OS_PEND_OPT_BROADCAST:               /* 中断所有等待的任务                            */
                 while (pevent->OSEventGrp != 0u) {   /* Yes, ready ALL tasks waiting on semaphore     */
                     (void)OS_EventTaskRdy(pevent, (void *)0, OS_STAT_SEM, OS_STAT_PEND_ABORT);
                     nbr_tasks++;
                 }
                 break;

            case OS_PEND_OPT_NONE:
            default:                                  /* No,  ready HPT       waiting on semaphore     */
                 (void)OS_EventTaskRdy(pevent, (void *)0, OS_STAT_SEM, OS_STAT_PEND_ABORT);
                 nbr_tasks++;
                 break;
        }
        OS_EXIT_CRITICAL();
        OS_Sched();                                   /* Find HPT ready to run                         */
        *perr = OS_ERR_PEND_ABORT;
        return (nbr_tasks);
    }
    OS_EXIT_CRITICAL();
    *perr = OS_ERR_NONE;
    return (0u);                                      /* No tasks waiting on semaphore                 */
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                         POST TO A SEMAPHORE
*                                   （发出一个信号量，FIFO先入先出）
*
* Description: 这个函数用于发出一个信号量。
*
* Arguments  : pevent              指向信号量（事件）的指针。
*
* Returns    : OS_ERR_NONE         调用该函数成功并发出信号量。
*              OS_ERR_SEM_OVF      If the semaphore count exceeded its limit. In other words, you have
*                                  signaled the semaphore more often than you waited on it with either
*                                  OSSemAccept() or OSSemPend().
*              OS_ERR_EVENT_TYPE   If you didn't pass a pointer to a semaphore
*              OS_ERR_PEVENT_NULL  If 'pevent' is a NULL pointer.
*********************************************************************************************************
*/

INT8U  OSSemPost (OS_EVENT *pevent)
{
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif


#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                    /* 验证'pevent',若指向空...                     */
        return (OS_ERR_PEVENT_NULL);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_SEM) {   /* 验证'pevent',若指向空...                     */
        return (OS_ERR_EVENT_TYPE);
    }
    OS_ENTER_CRITICAL();
    if (pevent->OSEventGrp != 0u) {                   /* 若事件等待组中有任务等待...                   */
                                                      /* 查找等待Sem事件HPT,使其就绪                   */
        (void)OS_EventTaskRdy(pevent, (void *)0, OS_STAT_SEM, OS_STAT_PEND_OK);
        OS_EXIT_CRITICAL();
        OS_Sched();                                   /* 查找HPT，使其运行                             */
        return (OS_ERR_NONE);
    }
    if (pevent->OSEventCnt < 65535u) {                /* 确保信号量没有溢出                            */
        pevent->OSEventCnt++;                         /* 信号量（事件）计数加1                         */
        OS_EXIT_CRITICAL();
        return (OS_ERR_NONE);
    }
    OS_EXIT_CRITICAL();                               /* Semaphore value has reached its maximum       */
    return (OS_ERR_SEM_OVF);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                          QUERY A SEMAPHORE
*                                         （查询信号量状态）
*
* Description: 这个函数用于获取信号量的信息。
*
* Arguments  : pevent        指向信号量（事件）的指针。
*
*              p_sem_data    指向信号量数据结构的指针。
*
* Returns    : OS_ERR_NONE         The call was successful and the message was sent
*              OS_ERR_EVENT_TYPE   If you are attempting to obtain data from a non semaphore.
*              OS_ERR_PEVENT_NULL  If 'pevent'     is a NULL pointer.
*              OS_ERR_PDATA_NULL   If 'p_sem_data' is a NULL pointer
*********************************************************************************************************
*/

#if OS_SEM_QUERY_EN > 0u
INT8U  OSSemQuery (OS_EVENT     *pevent,
                   OS_SEM_DATA  *p_sem_data)
{
    INT8U       i;
    OS_PRIO    *psrc;
    OS_PRIO    *pdest;
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register */
    OS_CPU_SR   cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                         /* Validate 'pevent'                        */
        return (OS_ERR_PEVENT_NULL);
    }
    if (p_sem_data == (OS_SEM_DATA *)0) {                  /* Validate 'p_sem_data'                    */
        return (OS_ERR_PDATA_NULL);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_SEM) {        /* Validate event block type                */
        return (OS_ERR_EVENT_TYPE);
    }
    OS_ENTER_CRITICAL();
    p_sem_data->OSEventGrp = pevent->OSEventGrp;           /* Copy message mailbox wait list           */
    psrc                   = &pevent->OSEventTbl[0];
    pdest                  = &p_sem_data->OSEventTbl[0];
    for (i = 0u; i < OS_EVENT_TBL_SIZE; i++) {
        *pdest++ = *psrc++;
    }
    p_sem_data->OSCnt = pevent->OSEventCnt;                /* Get semaphore count                      */
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}
#endif                                                     /* OS_SEM_QUERY_EN                          */

/*$PAGE*/
/*
*********************************************************************************************************
*                                            SET SEMAPHORE
*                                            （设置信号量）
*
* Description: This function sets the semaphore count to the value specified as an argument.  Typically,
*              this value would be 0.
*
*              You would typically use this function when a semaphore is used as a signaling mechanism
*              and, you want to reset the count value.
*
* Arguments  : pevent     is a pointer to the event control block
*
*              cnt        is the new value for the semaphore count.  You would pass 0 to reset the
*                         semaphore count.
*
*              perr       指向出错代码的指针, 为以下值之一:
*
*                            OS_ERR_NONE          The call was successful and the semaphore value was set.
*                            OS_ERR_EVENT_TYPE    If you didn't pass a pointer to a semaphore.
*                            OS_ERR_PEVENT_NULL   If 'pevent' is a NULL pointer.
*                            OS_ERR_TASK_WAITING  If tasks are waiting on the semaphore.
*********************************************************************************************************
*/

#if OS_SEM_SET_EN > 0u
void  OSSemSet (OS_EVENT  *pevent,
                INT16U     cnt,
                INT8U     *perr)
{
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                    /* Validate 'pevent'                             */
        *perr = OS_ERR_PEVENT_NULL;
        return;
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_SEM) {   /* Validate event block type                     */
        *perr = OS_ERR_EVENT_TYPE;
        return;
    }
    OS_ENTER_CRITICAL();
    *perr = OS_ERR_NONE;
    if (pevent->OSEventCnt > 0u) {                    /* See if semaphore already has a count          */
        pevent->OSEventCnt = cnt;                     /* Yes, set it to the new value specified.       */
    } else {                                          /* No                                            */
        if (pevent->OSEventGrp == 0u) {               /*      See if task(s) waiting?                  */
            pevent->OSEventCnt = cnt;                 /*      No, OK to set the value                  */
        } else {
            *perr              = OS_ERR_TASK_WAITING;
        }
    }
    OS_EXIT_CRITICAL();
}
#endif

#endif                                                /* OS_SEM_EN                                     */
