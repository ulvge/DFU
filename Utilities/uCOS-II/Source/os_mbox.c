/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*                                       MESSAGE MAILBOX MANAGEMENT
*
*                              (c) Copyright 1992-2012, Micrium, Weston, FL
*                                           All Rights Reserved
*
* File    : OS_MBOX.C
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

#if OS_MBOX_EN > 0u
/*
*********************************************************************************************************
*                                        ACCEPT MESSAGE FROM MAILBOX
*                                           （检查(接收)邮箱消息）
*
* Description: 这个函数是检查邮箱中是否有消息，或者是否有事件发生。
*              不像OSSemPend(), 如果邮箱中没有消息也不挂起任务。
*
* Arguments  : pevent        指向消息邮箱（事件）的指针。
*
* Returns    : != (void *)0  邮箱中有消息。
*              == (void *)0  邮箱中没有消息。
*                            事件'pevent' 为空。
*                            事件类型不是消息邮箱。
*********************************************************************************************************
*/

#if OS_MBOX_ACCEPT_EN > 0u
void  *OSMboxAccept (OS_EVENT *pevent)
{
    void      *pmsg;
#if OS_CRITICAL_METHOD == 3u                              /* Allocate storage for CPU status register  */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                        /* Validate 'pevent'                         */
        return ((void *)0);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_MBOX) {      /* Validate event block type                 */
        return ((void *)0);
    }
    OS_ENTER_CRITICAL();
    pmsg               = pevent->OSEventPtr;
    pevent->OSEventPtr = (void *)0;                       /* Clear the mailbox                         */
    OS_EXIT_CRITICAL();
    return (pmsg);                                        /* Return the message received (or NULL)     */
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                                          CREATE A MESSAGE MAILBOX
*                                            （创建一个消息邮箱）
*
* Description: 这个函数用于创建一个消息邮箱（如果有空ECB可用的情况下）。
*
* Arguments  : pmsg          邮件指针（你希望存入邮箱中的邮件）。
*                            如果你设为空指针(如: (void *)0) ，则这个邮箱被认为是空。
*
* Returns    : != (OS_EVENT *)0  返回创建成功的邮箱事件指针。
*              == (OS_EVENT *)0  创建消息邮箱失败。
*********************************************************************************************************
*/

OS_EVENT  *OSMboxCreate (void *pmsg)
{
    OS_EVENT  *pevent;
#if OS_CRITICAL_METHOD == 3u                           /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0u;
#endif


#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_EVENT *)0);
    }
#endif

    if (OSIntNesting > 0u) {                           /* 若从ISR中创建消息邮箱 ...                    */
        return ((OS_EVENT *)0);                        /* ... 则创建消息邮箱失败                       */
    }
    OS_ENTER_CRITICAL();
    pevent = OSEventFreeList;                          /* 获取当前空ECB                                */
    if (OSEventFreeList != (OS_EVENT *)0) {            /* 若还有剩余ECB(初始化时开辟出来的空间)・・      */
        OSEventFreeList = (OS_EVENT *)OSEventFreeList->OSEventPtr;
    }
    OS_EXIT_CRITICAL();
    if (pevent != (OS_EVENT *)0) {
        pevent->OSEventType    = OS_EVENT_TYPE_MBOX;   /* 事件类型为OS_EVENT_TYPE_MBOX                 */
        pevent->OSEventCnt     = 0u;
        pevent->OSEventPtr     = pmsg;                 /* 在ECB中存放邮件                              */
#if OS_EVENT_NAME_EN > 0u
        pevent->OSEventName    = (INT8U *)(void *)"?";
#endif
        OS_EventWaitListInit(pevent);
    }
    return (pevent);                                   /* 返回创建成功的ECB指针                        */
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                           DELETE A MAIBOX
*                                           （删除一个邮箱）
*
* Description: 这个函数用于删除一个消息邮箱，分不同情况来删除。
*
* Arguments  : pevent        指向消息邮箱（事件）的指针。
*
*              opt           删除选项如下:
*                            opt == OS_DEL_NO_PEND   没有任务等待该消息邮箱时才能删除成功; 若有任务等待时，则删除不成功。
*                            opt == OS_DEL_ALWAYS    任何情况下删除消息邮箱（即使有任务等待该消息邮箱）。
*
*              perr          指向错误代码的指针，包含如下:
*                            OS_ERR_NONE             The call was successful and the mailbox was deleted
*                            OS_ERR_DEL_ISR          If you attempted to delete the mailbox from an ISR
*                            OS_ERR_INVALID_OPT      An invalid option was specified
*                            OS_ERR_TASK_WAITING     One or more tasks were waiting on the mailbox
*                            OS_ERR_EVENT_TYPE       If you didn't pass a pointer to a mailbox
*                            OS_ERR_PEVENT_NULL      If 'pevent' is a NULL pointer.
*
* Returns    : pevent        upon error
*              (OS_EVENT *)0 if the mailbox was successfully deleted.
*
* Note(s)    : 1) This function must be used with care.  Tasks that would normally expect the presence of
*                 the mailbox MUST check the return code of OSMboxPend().
*              2) OSMboxAccept() callers will not know that the intended mailbox has been deleted!
*              3) This call can potentially disable interrupts for a long time.  The interrupt disable
*                 time is directly proportional to the number of tasks waiting on the mailbox.
*              4) Because ALL tasks pending on the mailbox will be readied, you MUST be careful in
*                 applications where the mailbox is used for mutual exclusion because the resource(s)
*                 will no longer be guarded by the mailbox.
*              5) All tasks that were waiting for the mailbox will be readied and returned an 
*                 OS_ERR_PEND_ABORT if OSMboxDel() was called with OS_DEL_ALWAYS
*********************************************************************************************************
*/

#if OS_MBOX_DEL_EN > 0u
OS_EVENT  *OSMboxDel (OS_EVENT  *pevent,
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
    if (pevent->OSEventType != OS_EVENT_TYPE_MBOX) {       /* Validate event block type                */
        *perr = OS_ERR_EVENT_TYPE;
        return (pevent);
    }
    if (OSIntNesting > 0u) {                               /* See if called from ISR ...               */
        *perr = OS_ERR_DEL_ISR;                            /* ... can't DELETE from an ISR             */
        return (pevent);
    }
    OS_ENTER_CRITICAL();
    if (pevent->OSEventGrp != 0u) {                        /* 查看是否有其他任务在等待该消息邮箱       */
        tasks_waiting = OS_TRUE;                           /* Yes                                      */
    } else {
        tasks_waiting = OS_FALSE;                          /* No                                       */
    }
    switch (opt) {
        case OS_DEL_NO_PEND:                               /* Delete mailbox only if no task waiting   */
             if (tasks_waiting == OS_FALSE) {
#if OS_EVENT_NAME_EN > 0u
                 pevent->OSEventName = (INT8U *)(void *)"?";
#endif
                 pevent->OSEventType = OS_EVENT_TYPE_UNUSED;
                 pevent->OSEventPtr  = OSEventFreeList;    /* Return Event Control Block to free list  */
                 pevent->OSEventCnt  = 0u;
                 OSEventFreeList     = pevent;             /* Get next free event control block        */
                 OS_EXIT_CRITICAL();
                 *perr               = OS_ERR_NONE;
                 pevent_return       = (OS_EVENT *)0;      /* Mailbox has been deleted                 */
             } else {
                 OS_EXIT_CRITICAL();
                 *perr               = OS_ERR_TASK_WAITING;
                 pevent_return       = pevent;
             }
             break;

        case OS_DEL_ALWAYS:                                /* Always delete the mailbox                */
             while (pevent->OSEventGrp != 0u) {            /* Ready ALL tasks waiting for mailbox      */
                 (void)OS_EventTaskRdy(pevent, (void *)0, OS_STAT_MBOX, OS_STAT_PEND_ABORT);
             }
#if OS_EVENT_NAME_EN > 0u
             pevent->OSEventName    = (INT8U *)(void *)"?";
#endif
             pevent->OSEventType    = OS_EVENT_TYPE_UNUSED;
             pevent->OSEventPtr     = OSEventFreeList;     /* Return Event Control Block to free list  */
             pevent->OSEventCnt     = 0u;
             OSEventFreeList        = pevent;              /* Get next free event control block        */
             OS_EXIT_CRITICAL();
             if (tasks_waiting == OS_TRUE) {               /* Reschedule only if task(s) were waiting  */
                 OS_Sched();                               /* Find highest priority task ready to run  */
             }
             *perr         = OS_ERR_NONE;
             pevent_return = (OS_EVENT *)0;                /* Mailbox has been deleted                 */
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
*                                    PEND ON MAILBOX FOR A MESSAGE
*                                         （等待邮箱中的邮件）
*
* Description: 这个函数用于等待发送到邮箱中的邮件。
*
* Arguments  : pevent        指向消息邮箱（事件）的指针。
*
*              timeout       一个可选的超时时间 (时钟滴答数)。
*                            != 0  你的任务将等待指定的资源（消息邮箱）到超时时间。
*                            == 0  你的任务将等待指定的资源（消息邮箱），直到资源可用（或事件发生）为止。
*
*              perr          指向包含错误码的变量的指针。  可能错误的消息:
*
*                            OS_ERR_NONE         The call was successful and your task received a
*                                                message.
*                            OS_ERR_TIMEOUT      A message was not received within the specified 'timeout'.
*                            OS_ERR_PEND_ABORT   The wait on the mailbox was aborted.
*                            OS_ERR_EVENT_TYPE   Invalid event type
*                            OS_ERR_PEND_ISR     If you called this function from an ISR and the result
*                                                would lead to a suspension.
*                            OS_ERR_PEVENT_NULL  If 'pevent' is a NULL pointer
*                            OS_ERR_PEND_LOCKED  If you called this function when the scheduler is locked
*
* Returns    : != (void *)0  is a pointer to the message received
*              == (void *)0  if no message was received or,
*                            if 'pevent' is a NULL pointer or,
*                            if you didn't pass the proper pointer to the event control block.
*********************************************************************************************************
*/
/*$PAGE*/
void  *OSMboxPend (OS_EVENT  *pevent,
                   INT32U     timeout,
                   INT8U     *perr)
{
    void      *pmsg;
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((void *)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                    /* 验证 'pevent'                                 */
        *perr = OS_ERR_PEVENT_NULL;
        return ((void *)0);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_MBOX) {  /* 验证ECB类型，若不是消息邮箱类型...            */
        *perr = OS_ERR_EVENT_TYPE;
        return ((void *)0);
    }
    if (OSIntNesting > 0u) {                          /* 若从ISR中调用该函数 ...                       */
        *perr = OS_ERR_PEND_ISR;                      /* ... can't PEND from an ISR                    */
        return ((void *)0);
    }
    if (OSLockNesting > 0u) {                         /* See if called with scheduler locked ...       */
        *perr = OS_ERR_PEND_LOCKED;                   /* ... can't PEND when locked                    */
        return ((void *)0);
    }
    OS_ENTER_CRITICAL();
    pmsg = pevent->OSEventPtr;
    if (pmsg != (void *)0) {                          /* 若邮箱中已经有邮件了...                       */
        pevent->OSEventPtr = (void *)0;               /* 清除邮箱中的邮件                              */
        OS_EXIT_CRITICAL();
        *perr = OS_ERR_NONE;
        return (pmsg);                                /* 返回该邮件                                    */
    }
    OSTCBCur->OSTCBStat     |= OS_STAT_MBOX;          /* Message not available, task will pend         */
    OSTCBCur->OSTCBStatPend  = OS_STAT_PEND_OK;
    OSTCBCur->OSTCBDly       = timeout;               /* Load timeout in TCB                           */
    OS_EventTaskWait(pevent);                         /* Suspend task until event or timeout occurs    */
    OS_EXIT_CRITICAL();
    OS_Sched();                                       /* Find next highest priority task ready to run  */
    OS_ENTER_CRITICAL();
    switch (OSTCBCur->OSTCBStatPend) {                /* See if we timed-out or aborted                */
        case OS_STAT_PEND_OK:
             pmsg =  OSTCBCur->OSTCBMsg;
            *perr =  OS_ERR_NONE;
             break;

        case OS_STAT_PEND_ABORT:
             pmsg = (void *)0;
            *perr =  OS_ERR_PEND_ABORT;               /* Indicate that we aborted                      */
             break;

        case OS_STAT_PEND_TO:
        default:
             OS_EventTaskRemove(OSTCBCur, pevent);
             pmsg = (void *)0;
            *perr =  OS_ERR_TIMEOUT;                  /* Indicate that we didn't get event within TO   */
             break;
    }
    OSTCBCur->OSTCBStat          =  OS_STAT_RDY;      /* 设置任务状态为就绪态                          */
    OSTCBCur->OSTCBStatPend      =  OS_STAT_PEND_OK;  /* 清除等待状态                                  */
    OSTCBCur->OSTCBEventPtr      = (OS_EVENT  *)0;    /* 清除事件                                      */
#if (OS_EVENT_MULTI_EN > 0u)
    OSTCBCur->OSTCBEventMultiPtr = (OS_EVENT **)0;
#endif
    OSTCBCur->OSTCBMsg           = (void      *)0;    /* Clear  received message                       */
    OS_EXIT_CRITICAL();
    return (pmsg);                                    /* 返回邮箱中接收到的邮件                        */
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                     ABORT WAITING ON A MESSAGE MAILBOX
*                                          （中断邮箱消息的等待）
*
* Description: 这个函数会中断目前在等待的邮箱消息，让任务不再等待。
*              这个函数应用于故障而中断任务的等待。
*              有点类似于OSMboxPost()这个函数的功能，发送邮箱消息让任务不再等待这一次。
*
* Arguments  : pevent        指向消息邮箱（事件）的指针。
*
*              opt           中段执行的类型:
*                            OS_PEND_OPT_NONE         中断等待事件(消息邮箱)中HPT
*                            OS_PEND_OPT_BROADCAST    中断所有等待事件(消息邮箱)，即广播。
*
*              perr          指向存入错误消息的指针。 可能错误的消息:
*
*                            OS_ERR_NONE         No tasks were     waiting on the mailbox.
*                            OS_ERR_PEND_ABORT   At least one task waiting on the mailbox was readied
*                                                and informed of the aborted wait; check return value
*                                                for the number of tasks whose wait on the mailbox
*                                                was aborted.
*                            OS_ERR_EVENT_TYPE   If you didn't pass a pointer to a mailbox.
*                            OS_ERR_PEVENT_NULL  If 'pevent' is a NULL pointer.
*
* Returns    : == 0          if no tasks were waiting on the mailbox, or upon error.
*              >  0          if one or more tasks waiting on the mailbox are now readied and informed.
*********************************************************************************************************
*/

#if OS_MBOX_PEND_ABORT_EN > 0u
INT8U  OSMboxPendAbort (OS_EVENT  *pevent,
                        INT8U      opt,
                        INT8U     *perr)
{
    INT8U      nbr_tasks;
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                         /* Validate 'pevent'                        */
        *perr = OS_ERR_PEVENT_NULL;
        return (0u);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_MBOX) {       /* Validate event block type                */
        *perr = OS_ERR_EVENT_TYPE;
        return (0u);
    }
    OS_ENTER_CRITICAL();
    if (pevent->OSEventGrp != 0u) {                        /* 若有任务在等到信号量...                  */
        nbr_tasks = 0u;
        switch (opt) {
            case OS_PEND_OPT_BROADCAST:                    /* 中断所有等待的任务                       */
                 while (pevent->OSEventGrp != 0u) {        /* Yes, ready ALL tasks waiting on mailbox  */
                     (void)OS_EventTaskRdy(pevent, (void *)0, OS_STAT_MBOX, OS_STAT_PEND_ABORT);
                     nbr_tasks++;
                 }
                 break;

            case OS_PEND_OPT_NONE:
            default:                                       /* No,  ready HPT       waiting on mailbox  */
                 (void)OS_EventTaskRdy(pevent, (void *)0, OS_STAT_MBOX, OS_STAT_PEND_ABORT);
                 nbr_tasks++;
                 break;
        }
        OS_EXIT_CRITICAL();
        OS_Sched();                                        /* Find HPT ready to run                    */
        *perr = OS_ERR_PEND_ABORT;
        return (nbr_tasks);
    }
    OS_EXIT_CRITICAL();
    *perr = OS_ERR_NONE;
    return (0u);                                           /* No tasks waiting on mailbox              */
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                      POST MESSAGE TO A MAILBOX
*                                       （发送一个消息到邮箱）
*
* Description: 这个函数功能是将消息发送到邮箱。
*
* Arguments  : pevent        指向接收消息邮箱（事件）的指针。
*
*              pmsg          发送消息的指针。 （你不能发送一个空指针）
*
* Returns    : OS_ERR_NONE          消息发送成功。
*              OS_ERR_MBOX_FULL     邮箱中包含了消息。
*                                   你只能往空邮箱中发送消息，若邮箱里有消息了，则发送不成功。
*              OS_ERR_EVENT_TYPE    事件类型错误。
*              OS_ERR_PEVENT_NULL   'pevent'是空指针。
*              OS_ERR_POST_NULL_PTR  “空消息” pmsg不能发送。
*
* Note(s)    : 1) HPT 意思是(Highest Priority Task)最高优先级任务。
*********************************************************************************************************
*/

#if OS_MBOX_POST_EN > 0u
INT8U  OSMboxPost (OS_EVENT  *pevent,
                   void      *pmsg)
{
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                    /* 验证'pevent'                                  */
        return (OS_ERR_PEVENT_NULL);
    }
    if (pmsg == (void *)0) {                          /* 确保消息指针pmsg不能为空                      */
        return (OS_ERR_POST_NULL_PTR);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_MBOX) {  /* 验证事件类型                                  */
        return (OS_ERR_EVENT_TYPE);
    }
    OS_ENTER_CRITICAL();
    if (pevent->OSEventGrp != 0u) {                   /* 查看是否有任务等待该邮箱                      */
                                                      /* 就绪等待事件中HPT                             */
        (void)OS_EventTaskRdy(pevent, pmsg, OS_STAT_MBOX, OS_STAT_PEND_OK);
        OS_EXIT_CRITICAL();
        OS_Sched();                                   /* 查找HPT，使其就绪                             */
        return (OS_ERR_NONE);
    }
    if (pevent->OSEventPtr != (void *)0) {            /* 若邮箱中包含了消息...                         */
        OS_EXIT_CRITICAL();
        return (OS_ERR_MBOX_FULL);
    }
    pevent->OSEventPtr = pmsg;                        /* 往邮箱中发送消息                              */
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                      POST MESSAGE TO A MAILBOX
*                                       （发送一个消息到邮箱）
*
* Description: 这个函数将发送一个消息到邮箱中。
*
* Arguments  : pevent        is a pointer to the event control block associated with the desired mailbox
*
*              pmsg          is a pointer to the message to send.  You MUST NOT send a NULL pointer.
*
*              opt           选项类型:
*                            OS_POST_OPT_NONE         POST to a single waiting task
*                                                     (Identical to OSMboxPost())
*                            OS_POST_OPT_BROADCAST    POST to ALL tasks that are waiting on the mailbox
*
*                            OS_POST_OPT_NO_SCHED     Indicates that the scheduler will NOT be invoked
*
* Returns    : OS_ERR_NONE          The call was successful and the message was sent
*              OS_ERR_MBOX_FULL     If the mailbox already contains a message.  You can can only send one
*                                   message at a time and thus, the message MUST be consumed before you
*                                   are allowed to send another one.
*              OS_ERR_EVENT_TYPE    If you are attempting to post to a non mailbox.
*              OS_ERR_PEVENT_NULL   If 'pevent' is a NULL pointer
*              OS_ERR_POST_NULL_PTR If you are attempting to post a NULL pointer
*
* Note(s)    : 1) HPT means Highest Priority Task
*
* Warning    : Interrupts can be disabled for a long time if you do a 'broadcast'.  In fact, the
*              interrupt disable time is proportional to the number of tasks waiting on the mailbox.
*********************************************************************************************************
*/

#if OS_MBOX_POST_OPT_EN > 0u
INT8U  OSMboxPostOpt (OS_EVENT  *pevent,
                      void      *pmsg,
                      INT8U      opt)
{
#if OS_CRITICAL_METHOD == 3u                          /* Allocate storage for CPU status register      */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#if OS_ARG_CHK_EN > 0u
    if (pevent == (OS_EVENT *)0) {                    /* Validate 'pevent'                             */
        return (OS_ERR_PEVENT_NULL);
    }
    if (pmsg == (void *)0) {                          /* Make sure we are not posting a NULL pointer   */
        return (OS_ERR_POST_NULL_PTR);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_MBOX) {  /* Validate event block type                     */
        return (OS_ERR_EVENT_TYPE);
    }
    OS_ENTER_CRITICAL();
    if (pevent->OSEventGrp != 0u) {                   /* See if any task pending on mailbox            */
        if ((opt & OS_POST_OPT_BROADCAST) != 0x00u) { /* Do we need to post msg to ALL waiting tasks ? */
            while (pevent->OSEventGrp != 0u) {        /* Yes, Post to ALL tasks waiting on mailbox     */
                (void)OS_EventTaskRdy(pevent, pmsg, OS_STAT_MBOX, OS_STAT_PEND_OK);
            }
        } else {                                      /* No,  Post to HPT waiting on mbox              */
            (void)OS_EventTaskRdy(pevent, pmsg, OS_STAT_MBOX, OS_STAT_PEND_OK);
        }
        OS_EXIT_CRITICAL();
        if ((opt & OS_POST_OPT_NO_SCHED) == 0u) {     /* See if scheduler needs to be invoked          */
            OS_Sched();                               /* Find HPT ready to run                         */
        }
        return (OS_ERR_NONE);
    }
    if (pevent->OSEventPtr != (void *)0) {            /* Make sure mailbox doesn't already have a msg  */
        OS_EXIT_CRITICAL();
        return (OS_ERR_MBOX_FULL);
    }
    pevent->OSEventPtr = pmsg;                        /* Place message in mailbox                      */
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                       QUERY A MESSAGE MAILBOX
*                                           （查询邮箱状态）
*
* Description: 这个函数用于获取邮箱的信息。
*
* Arguments  : pevent        指向消息邮箱（事件）的指针。
*
*              p_mbox_data   指向消息邮箱数据结构的指针。
*
* Returns    : OS_ERR_NONE         The call was successful and the message was sent
*              OS_ERR_EVENT_TYPE   If you are attempting to obtain data from a non mailbox.
*              OS_ERR_PEVENT_NULL  If 'pevent'      is a NULL pointer
*              OS_ERR_PDATA_NULL   If 'p_mbox_data' is a NULL pointer
*********************************************************************************************************
*/

#if OS_MBOX_QUERY_EN > 0u
INT8U  OSMboxQuery (OS_EVENT      *pevent,
                    OS_MBOX_DATA  *p_mbox_data)
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
    if (p_mbox_data == (OS_MBOX_DATA *)0) {                /* Validate 'p_mbox_data'                   */
        return (OS_ERR_PDATA_NULL);
    }
#endif
    if (pevent->OSEventType != OS_EVENT_TYPE_MBOX) {       /* Validate event block type                */
        return (OS_ERR_EVENT_TYPE);
    }
    OS_ENTER_CRITICAL();
    p_mbox_data->OSEventGrp = pevent->OSEventGrp;          /* Copy message mailbox wait list           */
    psrc                    = &pevent->OSEventTbl[0];
    pdest                   = &p_mbox_data->OSEventTbl[0];
    for (i = 0u; i < OS_EVENT_TBL_SIZE; i++) {
        *pdest++ = *psrc++;
    }
    p_mbox_data->OSMsg = pevent->OSEventPtr;               /* Get message from mailbox                 */
    OS_EXIT_CRITICAL();
    return (OS_ERR_NONE);
}
#endif                                                     /* OS_MBOX_QUERY_EN                         */
#endif                                                     /* OS_MBOX_EN                               */
