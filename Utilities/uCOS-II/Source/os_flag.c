/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*                                         EVENT FLAG  MANAGEMENT
*
*                              (c) Copyright 1992-2012, Micrium, Weston, FL
*                                           All Rights Reserved
*
* File    : OS_FLAG.C
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

#if (OS_FLAG_EN > 0u) && (OS_MAX_FLAGS > 0u)
/*
*********************************************************************************************************
*                                          LOCAL PROTOTYPES
*********************************************************************************************************
*/

static  void     OS_FlagBlock(OS_FLAG_GRP *pgrp, OS_FLAG_NODE *pnode, OS_FLAGS flags, INT8U wait_type, INT32U timeout);
static  BOOLEAN  OS_FlagTaskRdy(OS_FLAG_NODE *pnode, OS_FLAGS flags_rdy, INT8U pend_stat);

/*$PAGE*/
/*
*********************************************************************************************************
*                          CHECK THE STATUS OF FLAGS IN AN EVENT FLAG GROUP
*
* Description: This function is called to check the status of a combination of bits to be set or cleared
*              in an event flag group.  Your application can check for ANY bit to be set/cleared or ALL
*              bits to be set/cleared.
*
*              This call does not block if the desired flags are not present.
*
* Arguments  : pgrp          is a pointer to the desired event flag group.
*
*              flags         Is a bit pattern indicating which bit(s) (i.e. flags) you wish to check.
*                            The bits you want are specified by setting the corresponding bits in
*                            'flags'.  e.g. if your application wants to wait for bits 0 and 1 then
*                            'flags' would contain 0x03.
*
*              wait_type     specifies whether you want ALL bits to be set/cleared or ANY of the bits
*                            to be set/cleared.
*                            You can specify the following argument:
*
*                            OS_FLAG_WAIT_CLR_ALL   You will check ALL bits in 'flags' to be clear (0)
*                            OS_FLAG_WAIT_CLR_ANY   You will check ANY bit  in 'flags' to be clear (0)
*                            OS_FLAG_WAIT_SET_ALL   You will check ALL bits in 'flags' to be set   (1)
*                            OS_FLAG_WAIT_SET_ANY   You will check ANY bit  in 'flags' to be set   (1)
*
*                            NOTE: Add OS_FLAG_CONSUME if you want the event flag to be 'consumed' by
*                                  the call.  Example, to wait for any flag in a group AND then clear
*                                  the flags that are present, set 'wait_type' to:
*
*                                  OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME
*
*              perr          is a pointer to an error code and can be:
*                            OS_ERR_NONE               No error
*                            OS_ERR_EVENT_TYPE         You are not pointing to an event flag group
*                            OS_ERR_FLAG_WAIT_TYPE     You didn't specify a proper 'wait_type' argument.
*                            OS_ERR_FLAG_INVALID_PGRP  You passed a NULL pointer instead of the event flag
*                                                      group handle.
*                            OS_ERR_FLAG_NOT_RDY       The desired flags you are waiting for are not
*                                                      available.
*
* Returns    : The flags in the event flag group that made the task ready or, 0 if a timeout or an error
*              occurred.
*
* Called from: Task or ISR
*
* Note(s)    : 1) IMPORTANT, the behavior of this function has changed from PREVIOUS versions.  The
*                 function NOW returns the flags that were ready INSTEAD of the current state of the
*                 event flags.
*********************************************************************************************************
*/

#if OS_FLAG_ACCEPT_EN > 0u
OS_FLAGS  OSFlagAccept (OS_FLAG_GRP  *pgrp,
                        OS_FLAGS      flags,
                        INT8U         wait_type,
                        INT8U        *perr)
{
    OS_FLAGS      flags_rdy;
    INT8U         result;
    BOOLEAN       consume;
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register */
    OS_CPU_SR     cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_FLAGS)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pgrp == (OS_FLAG_GRP *)0) {                        /* Validate 'pgrp'                          */
        *perr = OS_ERR_FLAG_INVALID_PGRP;
        return ((OS_FLAGS)0);
    }
#endif
    if (pgrp->OSFlagType != OS_EVENT_TYPE_FLAG) {          /* Validate event block type                */
        *perr = OS_ERR_EVENT_TYPE;
        return ((OS_FLAGS)0);
    }
    result = (INT8U)(wait_type & OS_FLAG_CONSUME);
    if (result != (INT8U)0) {                              /* See if we need to consume the flags      */
        wait_type &= ~OS_FLAG_CONSUME;
        consume    = OS_TRUE;
    } else {
        consume    = OS_FALSE;
    }
/*$PAGE*/
    *perr = OS_ERR_NONE;                                   /* Assume NO error until proven otherwise.  */
    OS_ENTER_CRITICAL();
    switch (wait_type) {
        case OS_FLAG_WAIT_SET_ALL:                         /* See if all required flags are set        */
             flags_rdy = (OS_FLAGS)(pgrp->OSFlagFlags & flags);     /* Extract only the bits we want   */
             if (flags_rdy == flags) {                     /* Must match ALL the bits that we want     */
                 if (consume == OS_TRUE) {                 /* See if we need to consume the flags      */
                     pgrp->OSFlagFlags &= (OS_FLAGS)~flags_rdy;     /* Clear ONLY the flags we wanted  */
                 }
             } else {
                 *perr = OS_ERR_FLAG_NOT_RDY;
             }
             OS_EXIT_CRITICAL();
             break;

        case OS_FLAG_WAIT_SET_ANY:
             flags_rdy = (OS_FLAGS)(pgrp->OSFlagFlags & flags);     /* Extract only the bits we want   */
             if (flags_rdy != (OS_FLAGS)0) {               /* See if any flag set                      */
                 if (consume == OS_TRUE) {                 /* See if we need to consume the flags      */
                     pgrp->OSFlagFlags &= (OS_FLAGS)~flags_rdy;     /* Clear ONLY the flags we got     */
                 }
             } else {
                 *perr = OS_ERR_FLAG_NOT_RDY;
             }
             OS_EXIT_CRITICAL();
             break;

#if OS_FLAG_WAIT_CLR_EN > 0u
        case OS_FLAG_WAIT_CLR_ALL:                         /* See if all required flags are cleared    */
             flags_rdy = (OS_FLAGS)~pgrp->OSFlagFlags & flags;    /* Extract only the bits we want     */
             if (flags_rdy == flags) {                     /* Must match ALL the bits that we want     */
                 if (consume == OS_TRUE) {                 /* See if we need to consume the flags      */
                     pgrp->OSFlagFlags |= flags_rdy;       /* Set ONLY the flags that we wanted        */
                 }
             } else {
                 *perr = OS_ERR_FLAG_NOT_RDY;
             }
             OS_EXIT_CRITICAL();
             break;

        case OS_FLAG_WAIT_CLR_ANY:
             flags_rdy = (OS_FLAGS)~pgrp->OSFlagFlags & flags;   /* Extract only the bits we want      */
             if (flags_rdy != (OS_FLAGS)0) {               /* See if any flag cleared                  */
                 if (consume == OS_TRUE) {                 /* See if we need to consume the flags      */
                     pgrp->OSFlagFlags |= flags_rdy;       /* Set ONLY the flags that we got           */
                 }
             } else {
                 *perr = OS_ERR_FLAG_NOT_RDY;
             }
             OS_EXIT_CRITICAL();
             break;
#endif

        default:
             OS_EXIT_CRITICAL();
             flags_rdy = (OS_FLAGS)0;
             *perr     = OS_ERR_FLAG_WAIT_TYPE;
             break;
    }
    return (flags_rdy);
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                        CREATE AN EVENT FLAG
*                                       （创建一个事件标志组）
*
* Description: 这个函数用于创建一个事件标志组。
*
* Arguments  : flags         事件标志组标志的初值。
*
*              perr          指向出错代码的指针, 为以下值之一:
*                               OS_ERR_NONE               if the call was successful.
*                               OS_ERR_CREATE_ISR         if you attempted to create an Event Flag from an
*                                                         ISR.
*                               OS_ERR_FLAG_GRP_DEPLETED  if there are no more event flag groups
*
* Returns    :               返回一个事件标志组指针。
*
* Called from: Task ONLY
*********************************************************************************************************
*/

OS_FLAG_GRP  *OSFlagCreate (OS_FLAGS  flags,
                            INT8U    *perr)
{
    OS_FLAG_GRP *pgrp;
#if OS_CRITICAL_METHOD == 3u                        /* Allocate storage for CPU status register        */
    OS_CPU_SR    cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_FLAG_GRP *)0);
    }
#endif

#ifdef OS_SAFETY_CRITICAL_IEC61508
    if (OSSafetyCriticalStartFlag == OS_TRUE) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_FLAG_GRP *)0);
    }
#endif

    if (OSIntNesting > 0u) {                        /* See if called from ISR ...                      */
        *perr = OS_ERR_CREATE_ISR;                  /* ... can't CREATE from an ISR                    */
        return ((OS_FLAG_GRP *)0);
    }
    OS_ENTER_CRITICAL();
    pgrp = OSFlagFreeList;                          /* 获取空事件标志组                                */
    if (pgrp != (OS_FLAG_GRP *)0) {                 /* 若还有剩余FLAG_GRP(初始化时开辟出来的空间)・・    */
                                                    /* OSFlagFreeList指向下一空OSFlagTbl[]             */
        OSFlagFreeList       = (OS_FLAG_GRP *)OSFlagFreeList->OSFlagWaitList;
        pgrp->OSFlagType     = OS_EVENT_TYPE_FLAG;  /* 事件标志类型: OS_EVENT_TYPE_FLAG                */
        pgrp->OSFlagFlags    = flags;               /* 事件标志初始值                                  */
        pgrp->OSFlagWaitList = (void *)0;           /* 清除事件标志等待列表                            */
#if OS_FLAG_NAME_EN > 0u
        pgrp->OSFlagName     = (INT8U *)(void *)"?";
#endif
        OS_EXIT_CRITICAL();
        *perr                = OS_ERR_NONE;
    } else {                                        /* 没有剩余FLAG_GRP可用了                          */
        OS_EXIT_CRITICAL();
        *perr                = OS_ERR_FLAG_GRP_DEPLETED;
    }
    return (pgrp);                                  /* 返回指向创建的事件标志的指针                    */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                     DELETE AN EVENT FLAG GROUP
*                                       （删除一个事件标志组）
*
* Description: 这个函数用于删除一个事件标志组。
*
* Arguments  : pgrp          指向事件标志组的指针。
*
*              opt           删除选项如下:
*                            opt == OS_DEL_NO_PEND   Deletes the event flag group ONLY if no task pending
*                            opt == OS_DEL_ALWAYS    Deletes the event flag group even if tasks are
*                                                    waiting.  In this case, all the tasks pending will be
*                                                    readied.
*
*              perr          指向出错代码的指针, 为以下值之一:
*                            OS_ERR_NONE               The call was successful and the event flag group was
*                                                      deleted
*                            OS_ERR_DEL_ISR            If you attempted to delete the event flag group from
*                                                      an ISR
*                            OS_ERR_FLAG_INVALID_PGRP  If 'pgrp' is a NULL pointer.
*                            OS_ERR_EVENT_TYPE         If you didn't pass a pointer to an event flag group
*                            OS_ERR_INVALID_OPT        An invalid option was specified
*                            OS_ERR_TASK_WAITING       One or more tasks were waiting on the event flag
*                                                      group.
*
* Returns    : pgrp          upon error
*              (OS_EVENT *)0 if the event flag group was successfully deleted.
*
* Note(s)    : 1) This function must be used with care.  Tasks that would normally expect the presence of
*                 the event flag group MUST check the return code of OSFlagAccept() and OSFlagPend().
*              2) This call can potentially disable interrupts for a long time.  The interrupt disable
*                 time is directly proportional to the number of tasks waiting on the event flag group.
*              3) All tasks that were waiting for the event flag will be readied and returned an 
*                 OS_ERR_PEND_ABORT if OSFlagDel() was called with OS_DEL_ALWAYS
*********************************************************************************************************
*/

#if OS_FLAG_DEL_EN > 0u
OS_FLAG_GRP  *OSFlagDel (OS_FLAG_GRP  *pgrp,
                         INT8U         opt,
                         INT8U        *perr)
{
    BOOLEAN       tasks_waiting;
    OS_FLAG_NODE *pnode;
    OS_FLAG_GRP  *pgrp_return;
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register */
    OS_CPU_SR     cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_FLAG_GRP *)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pgrp == (OS_FLAG_GRP *)0) {                        /* Validate 'pgrp'                          */
        *perr = OS_ERR_FLAG_INVALID_PGRP;
        return (pgrp);
    }
#endif
    if (OSIntNesting > 0u) {                               /* See if called from ISR ...               */
        *perr = OS_ERR_DEL_ISR;                            /* ... can't DELETE from an ISR             */
        return (pgrp);
    }
    if (pgrp->OSFlagType != OS_EVENT_TYPE_FLAG) {          /* Validate event group type                */
        *perr = OS_ERR_EVENT_TYPE;
        return (pgrp);
    }
    OS_ENTER_CRITICAL();
    if (pgrp->OSFlagWaitList != (void *)0) {               /* See if any tasks waiting on event flags  */
        tasks_waiting = OS_TRUE;                           /* Yes                                      */
    } else {
        tasks_waiting = OS_FALSE;                          /* No                                       */
    }
    switch (opt) {
        case OS_DEL_NO_PEND:                               /* Delete group if no task waiting          */
             if (tasks_waiting == OS_FALSE) {
#if OS_FLAG_NAME_EN > 0u
                 pgrp->OSFlagName     = (INT8U *)(void *)"?";
#endif
                 pgrp->OSFlagType     = OS_EVENT_TYPE_UNUSED;
                 pgrp->OSFlagWaitList = (void *)OSFlagFreeList; /* Return group to free list           */
                 pgrp->OSFlagFlags    = (OS_FLAGS)0;
                 OSFlagFreeList       = pgrp;
                 OS_EXIT_CRITICAL();
                 *perr                = OS_ERR_NONE;
                 pgrp_return          = (OS_FLAG_GRP *)0;  /* Event Flag Group has been deleted        */
             } else {
                 OS_EXIT_CRITICAL();
                 *perr                = OS_ERR_TASK_WAITING;
                 pgrp_return          = pgrp;
             }
             break;

        case OS_DEL_ALWAYS:                                /* Always delete the event flag group       */
             pnode = (OS_FLAG_NODE *)pgrp->OSFlagWaitList;
             while (pnode != (OS_FLAG_NODE *)0) {          /* Ready ALL tasks waiting for flags        */
                 (void)OS_FlagTaskRdy(pnode, (OS_FLAGS)0, OS_STAT_PEND_ABORT);
                 pnode = (OS_FLAG_NODE *)pnode->OSFlagNodeNext;
             }
#if OS_FLAG_NAME_EN > 0u
             pgrp->OSFlagName     = (INT8U *)(void *)"?";
#endif
             pgrp->OSFlagType     = OS_EVENT_TYPE_UNUSED;
             pgrp->OSFlagWaitList = (void *)OSFlagFreeList;/* Return group to free list                */
             pgrp->OSFlagFlags    = (OS_FLAGS)0;
             OSFlagFreeList       = pgrp;
             OS_EXIT_CRITICAL();
             if (tasks_waiting == OS_TRUE) {               /* Reschedule only if task(s) were waiting  */
                 OS_Sched();                               /* Find highest priority task ready to run  */
             }
             *perr = OS_ERR_NONE;
             pgrp_return          = (OS_FLAG_GRP *)0;      /* Event Flag Group has been deleted        */
             break;

        default:
             OS_EXIT_CRITICAL();
             *perr                = OS_ERR_INVALID_OPT;
             pgrp_return          = pgrp;
             break;
    }
    return (pgrp_return);
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                                 GET THE NAME OF AN EVENT FLAG GROUP
*
* Description: This function is used to obtain the name assigned to an event flag group
*
* Arguments  : pgrp      is a pointer to the event flag group.
*
*              pname     is pointer to a pointer to an ASCII string that will receive the name of the event flag
*                        group.
*
*              perr      is a pointer to an error code that can contain one of the following values:
*
*                        OS_ERR_NONE                if the requested task is resumed
*                        OS_ERR_EVENT_TYPE          if 'pevent' is not pointing to an event flag group
*                        OS_ERR_PNAME_NULL          You passed a NULL pointer for 'pname'
*                        OS_ERR_FLAG_INVALID_PGRP   if you passed a NULL pointer for 'pgrp'
*                        OS_ERR_NAME_GET_ISR        if you called this function from an ISR
*
* Returns    : The length of the string or 0 if the 'pgrp' is a NULL pointer.
*********************************************************************************************************
*/

#if OS_FLAG_NAME_EN > 0u
INT8U  OSFlagNameGet (OS_FLAG_GRP   *pgrp,
                      INT8U        **pname,
                      INT8U         *perr)
{
    INT8U      len;
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return (0u);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pgrp == (OS_FLAG_GRP *)0) {              /* Is 'pgrp' a NULL pointer?                          */
        *perr = OS_ERR_FLAG_INVALID_PGRP;
        return (0u);
    }
    if (pname == (INT8U **)0) {                   /* Is 'pname' a NULL pointer?                         */
        *perr = OS_ERR_PNAME_NULL;
        return (0u);
    }
#endif
    if (OSIntNesting > 0u) {                     /* See if trying to call from an ISR                  */
        *perr = OS_ERR_NAME_GET_ISR;
        return (0u);
    }
    OS_ENTER_CRITICAL();
    if (pgrp->OSFlagType != OS_EVENT_TYPE_FLAG) {
        OS_EXIT_CRITICAL();
        *perr = OS_ERR_EVENT_TYPE;
        return (0u);
    }
    *pname = pgrp->OSFlagName;
    len    = OS_StrLen(*pname);
    OS_EXIT_CRITICAL();
    *perr  = OS_ERR_NONE;
    return (len);
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                ASSIGN A NAME TO AN EVENT FLAG GROUP
*
* Description: This function assigns a name to an event flag group.
*
* Arguments  : pgrp      is a pointer to the event flag group.
*
*              pname     is a pointer to an ASCII string that will be used as the name of the event flag
*                        group.
*
*              perr      is a pointer to an error code that can contain one of the following values:
*
*                        OS_ERR_NONE                if the requested task is resumed
*                        OS_ERR_EVENT_TYPE          if 'pevent' is not pointing to an event flag group
*                        OS_ERR_PNAME_NULL          You passed a NULL pointer for 'pname'
*                        OS_ERR_FLAG_INVALID_PGRP   if you passed a NULL pointer for 'pgrp'
*                        OS_ERR_NAME_SET_ISR        if you called this function from an ISR
*
* Returns    : None
*********************************************************************************************************
*/

#if OS_FLAG_NAME_EN > 0u
void  OSFlagNameSet (OS_FLAG_GRP  *pgrp,
                     INT8U        *pname,
                     INT8U        *perr)
{
#if OS_CRITICAL_METHOD == 3u                     /* Allocate storage for CPU status register           */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return;
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pgrp == (OS_FLAG_GRP *)0) {              /* Is 'pgrp' a NULL pointer?                          */
        *perr = OS_ERR_FLAG_INVALID_PGRP;
        return;
    }
    if (pname == (INT8U *)0) {                   /* Is 'pname' a NULL pointer?                         */
        *perr = OS_ERR_PNAME_NULL;
        return;
    }
#endif
    if (OSIntNesting > 0u) {                     /* See if trying to call from an ISR                  */
        *perr = OS_ERR_NAME_SET_ISR;
        return;
    }
    OS_ENTER_CRITICAL();
    if (pgrp->OSFlagType != OS_EVENT_TYPE_FLAG) {
        OS_EXIT_CRITICAL();
        *perr = OS_ERR_EVENT_TYPE;
        return;
    }
    pgrp->OSFlagName = pname;
    OS_EXIT_CRITICAL();
    *perr            = OS_ERR_NONE;
    return;
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                                     WAIT ON AN EVENT FLAG GROUP
*                                        （等待一个事件标志）
*
* Description: This function is called to wait for a combination of bits to be set in an event flag
*              group.  Your application can wait for ANY bit to be set or ALL bits to be set.
*
* Arguments  : pgrp          指向事件标志组的指针。
*
*              flags         事件标志对应的标志位。
*                            The bits you want are specified by setting the corresponding bits in
*                            'flags'.  e.g. if your application wants to wait for bits 0 and 1 then
*                            'flags' would contain 0x03.
*
*              wait_type     等待事件标志位设置的类型:
*
*                            OS_FLAG_WAIT_CLR_ALL   所有指定事件标志位清0
*                            OS_FLAG_WAIT_SET_ALL   所有指定事件标志位置1
*                            OS_FLAG_WAIT_CLR_ANY   任意指定事件标志位清0
*                            OS_FLAG_WAIT_SET_ANY   任意指定事件标志位置1
*
*                            NOTE: Add OS_FLAG_CONSUME if you want the event flag to be 'consumed' by
*                                  the call.  Example, to wait for any flag in a group AND then clear
*                                  the flags that are present, set 'wait_type' to:
*
*                                  OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME
*
*              timeout       一个可选的超时时间 (时钟滴答数)。
*                            != 0  你的任务将等待指定的资源（事件信号）到超时时间。
*                            == 0  你的任务将等待指定的资源（事件信号），直到资源可用（或事件发生）为止。
*
*              perr         指向出错代码的指针, 为以下值之一:
*                            OS_ERR_NONE               The desired bits have been set within the specified
*                                                      'timeout'.
*                            OS_ERR_PEND_ISR           If you tried to PEND from an ISR
*                            OS_ERR_FLAG_INVALID_PGRP  If 'pgrp' is a NULL pointer.
*                            OS_ERR_EVENT_TYPE         You are not pointing to an event flag group
*                            OS_ERR_TIMEOUT            The bit(s) have not been set in the specified
*                                                      'timeout'.
*                            OS_ERR_PEND_ABORT         The wait on the flag was aborted.
*                            OS_ERR_FLAG_WAIT_TYPE     You didn't specify a proper 'wait_type' argument.
*
* Returns    : The flags in the event flag group that made the task ready or, 0 if a timeout or an error
*              occurred.
*
* Called from: Task ONLY
*
* Note(s)    : 1) IMPORTANT, the behavior of this function has changed from PREVIOUS versions.  The
*                 function NOW returns the flags that were ready INSTEAD of the current state of the
*                 event flags.
*********************************************************************************************************
*/

OS_FLAGS  OSFlagPend (OS_FLAG_GRP  *pgrp,
                      OS_FLAGS      flags,
                      INT8U         wait_type,
                      INT32U        timeout,
                      INT8U        *perr)
{
    OS_FLAG_NODE  node;                                    /* 事件标记节点                             */
    OS_FLAGS      flags_rdy;
    INT8U         result;
    INT8U         pend_stat;
    BOOLEAN       consume;                                 /* '清除' 事件标志位                        */
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register */
    OS_CPU_SR     cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_FLAGS)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pgrp == (OS_FLAG_GRP *)0) {                        /* 验证'pgrp',若指向空...                   */
        *perr = OS_ERR_FLAG_INVALID_PGRP;
        return ((OS_FLAGS)0);
    }
#endif
    if (OSIntNesting > 0u) {                               /* See if called from ISR ...               */
        *perr = OS_ERR_PEND_ISR;                           /* ... can't PEND from an ISR               */
        return ((OS_FLAGS)0);
    }
    if (OSLockNesting > 0u) {                              /* See if called with scheduler locked ...  */
        *perr = OS_ERR_PEND_LOCKED;                        /* ... can't PEND when locked               */
        return ((OS_FLAGS)0);
    }
    if (pgrp->OSFlagType != OS_EVENT_TYPE_FLAG) {          /* 验证事件类型，若不是标志类型...          */
        *perr = OS_ERR_EVENT_TYPE;
        return ((OS_FLAGS)0);
    }
    result = (INT8U)(wait_type & OS_FLAG_CONSUME);
    if (result != (INT8U)0) {                              /* 若需要清除标志位...                      */
        wait_type &= (INT8U)~(INT8U)OS_FLAG_CONSUME;
        consume    = OS_TRUE;                              /* 要清除                                   */
    } else {
        consume    = OS_FALSE;                             /* 不清除                                   */
    }
/*$PAGE*/
    OS_ENTER_CRITICAL();
    switch (wait_type) {
        case OS_FLAG_WAIT_SET_ALL:                               /* 所有指定事件标志位置1              */
             flags_rdy = (OS_FLAGS)(pgrp->OSFlagFlags & flags);  /* 获取已经就绪的标志位               */
             if (flags_rdy == flags) {                           /* 若所有标志位都已经就绪...          */
                 if (consume == OS_TRUE) {                       /* 若我们需要清除标志位...            */
                     pgrp->OSFlagFlags &= (OS_FLAGS)~flags_rdy;  /* Clear ONLY the flags we wanted     */
                 }
                 OSTCBCur->OSTCBFlagsRdy = flags_rdy;            /* Save flags that were ready         */
                 OS_EXIT_CRITICAL();
                 *perr                   = OS_ERR_NONE;
                 return (flags_rdy);
             } else {                                            /* 任务快需要等待事件的发生或超时     */
                 OS_FlagBlock(pgrp, &node, flags, wait_type, timeout);
                 OS_EXIT_CRITICAL();
             }
             break;

        case OS_FLAG_WAIT_SET_ANY:                         /* 任意指定事件标志位置1                    */
             flags_rdy = (OS_FLAGS)(pgrp->OSFlagFlags & flags);    /* Extract only the bits we want    */
             if (flags_rdy != (OS_FLAGS)0) {               /* See if any flag set                      */
                 if (consume == OS_TRUE) {                 /* See if we need to consume the flags      */
                     pgrp->OSFlagFlags &= (OS_FLAGS)~flags_rdy;    /* Clear ONLY the flags that we got */
                 }
                 OSTCBCur->OSTCBFlagsRdy = flags_rdy;      /* Save flags that were ready               */
                 OS_EXIT_CRITICAL();                       /* Yes, condition met, return to caller     */
                 *perr                   = OS_ERR_NONE;
                 return (flags_rdy);
             } else {                                      /* Block task until events occur or timeout */
                 OS_FlagBlock(pgrp, &node, flags, wait_type, timeout);
                 OS_EXIT_CRITICAL();
             }
             break;

#if OS_FLAG_WAIT_CLR_EN > 0u
        case OS_FLAG_WAIT_CLR_ALL:                         /* 所有指定事件标志位清0                    */
             flags_rdy = (OS_FLAGS)~pgrp->OSFlagFlags & flags;    /* Extract only the bits we want     */
             if (flags_rdy == flags) {                     /* Must match ALL the bits that we want     */
                 if (consume == OS_TRUE) {                 /* See if we need to consume the flags      */
                     pgrp->OSFlagFlags |= flags_rdy;       /* Set ONLY the flags that we wanted        */
                 }
                 OSTCBCur->OSTCBFlagsRdy = flags_rdy;      /* Save flags that were ready               */
                 OS_EXIT_CRITICAL();                       /* Yes, condition met, return to caller     */
                 *perr                   = OS_ERR_NONE;
                 return (flags_rdy);
             } else {                                      /* Block task until events occur or timeout */
                 OS_FlagBlock(pgrp, &node, flags, wait_type, timeout);
                 OS_EXIT_CRITICAL();
             }
             break;

        case OS_FLAG_WAIT_CLR_ANY:                         /* 任意指定事件标志位清0                    */
             flags_rdy = (OS_FLAGS)~pgrp->OSFlagFlags & flags;   /* Extract only the bits we want      */
             if (flags_rdy != (OS_FLAGS)0) {               /* See if any flag cleared                  */
                 if (consume == OS_TRUE) {                 /* See if we need to consume the flags      */
                     pgrp->OSFlagFlags |= flags_rdy;       /* Set ONLY the flags that we got           */
                 }
                 OSTCBCur->OSTCBFlagsRdy = flags_rdy;      /* Save flags that were ready               */
                 OS_EXIT_CRITICAL();                       /* Yes, condition met, return to caller     */
                 *perr                   = OS_ERR_NONE;
                 return (flags_rdy);
             } else {                                      /* Block task until events occur or timeout */
                 OS_FlagBlock(pgrp, &node, flags, wait_type, timeout);
                 OS_EXIT_CRITICAL();
             }
             break;
#endif

        default:
             OS_EXIT_CRITICAL();
             flags_rdy = (OS_FLAGS)0;
             *perr      = OS_ERR_FLAG_WAIT_TYPE;
             return (flags_rdy);
    }
/*$PAGE*/
    OS_Sched();                                            /* 查找最高优先级就绪任务,使其运行          */

    /************ 任务挂起后,恢复执行开始处 ************************************************************/
    OS_ENTER_CRITICAL();
    if (OSTCBCur->OSTCBStatPend != OS_STAT_PEND_OK) {      /* 若任务超时或终止...                      */
        pend_stat                = OSTCBCur->OSTCBStatPend;
        OSTCBCur->OSTCBStatPend  = OS_STAT_PEND_OK;
        OS_FlagUnlink(&node);
        OSTCBCur->OSTCBStat      = OS_STAT_RDY;            /* Yes, make task ready-to-run              */
        OS_EXIT_CRITICAL();
        flags_rdy                = (OS_FLAGS)0;
        switch (pend_stat) {
            case OS_STAT_PEND_ABORT:
                 *perr = OS_ERR_PEND_ABORT;                /* Indicate that we aborted   waiting       */
                 break;

            case OS_STAT_PEND_TO:
            default:
                 *perr = OS_ERR_TIMEOUT;                   /* Indicate that we timed-out waiting       */
                 break;
        }
        return (flags_rdy);
    }
    flags_rdy = OSTCBCur->OSTCBFlagsRdy;
    if (consume == OS_TRUE) {                              /* 若需要清除标志位...                      */
        switch (wait_type) {
            case OS_FLAG_WAIT_SET_ALL:
            case OS_FLAG_WAIT_SET_ANY:                     /* 清除标志位                               */
                 pgrp->OSFlagFlags &= (OS_FLAGS)~flags_rdy;
                 break;

#if OS_FLAG_WAIT_CLR_EN > 0u
            case OS_FLAG_WAIT_CLR_ALL:
            case OS_FLAG_WAIT_CLR_ANY:                     /* 置位标志位                               */
                 pgrp->OSFlagFlags |=  flags_rdy;
                 break;
#endif
            default:
                 OS_EXIT_CRITICAL();
                 *perr = OS_ERR_FLAG_WAIT_TYPE;
                 return ((OS_FLAGS)0);
        }
    }
    OS_EXIT_CRITICAL();
    *perr = OS_ERR_NONE;                                   /* Event(s) must have occurred              */
    return (flags_rdy);
}
/*$PAGE*/
/*
*********************************************************************************************************
*                              GET FLAGS WHO CAUSED TASK TO BECOME READY
*
* Description: This function is called to obtain the flags that caused the task to become ready to run.
*              In other words, this function allows you to tell "Who done it!".
*
* Arguments  : None
*
* Returns    : The flags that caused the task to be ready.
*
* Called from: Task ONLY
*********************************************************************************************************
*/

OS_FLAGS  OSFlagPendGetFlagsRdy (void)
{
    OS_FLAGS      flags;
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register */
    OS_CPU_SR     cpu_sr = 0u;
#endif



    OS_ENTER_CRITICAL();
    flags = OSTCBCur->OSTCBFlagsRdy;
    OS_EXIT_CRITICAL();
    return (flags);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                       POST EVENT FLAG BIT(S)
*                                       （"发送"一事件标志位）
*
* Description: This function is called to set or clear some bits in an event flag group.  The bits to
*              set or clear are specified by a 'bit mask'.
*
* Arguments  : pgrp          is a pointer to the desired event flag group.
*
*              flags         If 'opt' (see below) is OS_FLAG_SET, each bit that is set in 'flags' will
*                            set the corresponding bit in the event flag group.  e.g. to set bits 0, 4
*                            and 5 you would set 'flags' to:
*
*                                0x31     (note, bit 0 is least significant bit)
*
*                            If 'opt' (see below) is OS_FLAG_CLR, each bit that is set in 'flags' will
*                            CLEAR the corresponding bit in the event flag group.  e.g. to clear bits 0,
*                            4 and 5 you would specify 'flags' as:
*
*                                0x31     (note, bit 0 is least significant bit)
*
*              opt           对标志位操作选项:
*                              OS_FLAG_SET --- 标志位置1
*                              OS_FLAG_CLR --- 标志位清0
*
*              perr          is a pointer to an error code and can be:
*                            OS_ERR_NONE                The call was successfull
*                            OS_ERR_FLAG_INVALID_PGRP   You passed a NULL pointer
*                            OS_ERR_EVENT_TYPE          You are not pointing to an event flag group
*                            OS_ERR_FLAG_INVALID_OPT    You specified an invalid option
*
* Returns    : the new value of the event flags bits that are still set.
*
* Called From: Task or ISR
*
* WARNING(s) : 1) The execution time of this function depends on the number of tasks waiting on the event
*                 flag group.
*              2) The amount of time interrupts are DISABLED depends on the number of tasks waiting on
*                 the event flag group.
*********************************************************************************************************
*/
OS_FLAGS  OSFlagPost (OS_FLAG_GRP  *pgrp,
                      OS_FLAGS      flags,
                      INT8U         opt,
                      INT8U        *perr)
{
    OS_FLAG_NODE *pnode;
    BOOLEAN       sched;
    OS_FLAGS      flags_cur;
    OS_FLAGS      flags_rdy;
    BOOLEAN       rdy;
#if OS_CRITICAL_METHOD == 3u                         /* Allocate storage for CPU status register       */
    OS_CPU_SR     cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_FLAGS)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pgrp == (OS_FLAG_GRP *)0) {                  /* Validate 'pgrp'                                */
        *perr = OS_ERR_FLAG_INVALID_PGRP;
        return ((OS_FLAGS)0);
    }
#endif
    if (pgrp->OSFlagType != OS_EVENT_TYPE_FLAG) {    /* Make sure we are pointing to an event flag grp */
        *perr = OS_ERR_EVENT_TYPE;
        return ((OS_FLAGS)0);
    }
/*$PAGE*/
    OS_ENTER_CRITICAL();
    switch (opt) {
        case OS_FLAG_CLR:
             pgrp->OSFlagFlags &= (OS_FLAGS)~flags;  /* 清除指定标志位                                 */
             break;

        case OS_FLAG_SET:
             pgrp->OSFlagFlags |=  flags;            /* 置位指定标志位                                 */
             break;

        default:
             OS_EXIT_CRITICAL();                     /* 无效选项                                       */
             *perr = OS_ERR_FLAG_INVALID_OPT;
             return ((OS_FLAGS)0);
    }
    sched = OS_FALSE;                                /* 复位调度标志                                   */
    pnode = (OS_FLAG_NODE *)pgrp->OSFlagWaitList;    /* 事件标志组节点                                 */
    while (pnode != (OS_FLAG_NODE *)0) {             /* 浏览所有等待事件标志任务                       */
        switch (pnode->OSFlagNodeWaitType) {
            case OS_FLAG_WAIT_SET_ALL:               /* 所有指定事件标志位置1                          */
                                                     /* flags_rdy等于OSFlagPend()参数中的flags,即就绪  */
                 flags_rdy = (OS_FLAGS)(pgrp->OSFlagFlags & pnode->OSFlagNodeFlags);
                 if (flags_rdy == pnode->OSFlagNodeFlags) {   /* 若事件标志组就绪了...                 */
                     rdy = OS_FlagTaskRdy(pnode, flags_rdy, OS_STAT_PEND_OK);  
                     if (rdy == OS_TRUE) {
                         sched = OS_TRUE;                     /* 将重新调度任务                        */
                     }
                 }
                 break;

            case OS_FLAG_WAIT_SET_ANY:               /* 任意指定事件标志位置1                          */
                 flags_rdy = (OS_FLAGS)(pgrp->OSFlagFlags & pnode->OSFlagNodeFlags);
                 if (flags_rdy != (OS_FLAGS)0) {              /* Make task RTR, event(s) Rx'd          */
                     rdy = OS_FlagTaskRdy(pnode, flags_rdy, OS_STAT_PEND_OK);  
                     if (rdy == OS_TRUE) {
                         sched = OS_TRUE;                     /* When done we will reschedule          */
                     }
                 }
                 break;

#if OS_FLAG_WAIT_CLR_EN > 0u
            case OS_FLAG_WAIT_CLR_ALL:               /* See if all req. flags are set for current node */
                 flags_rdy = (OS_FLAGS)~pgrp->OSFlagFlags & pnode->OSFlagNodeFlags;
                 if (flags_rdy == pnode->OSFlagNodeFlags) {   /* Make task RTR, event(s) Rx'd          */
                     rdy = OS_FlagTaskRdy(pnode, flags_rdy, OS_STAT_PEND_OK);  
                     if (rdy == OS_TRUE) {
                         sched = OS_TRUE;                     /* When done we will reschedule          */
                     }
                 }
                 break;

            case OS_FLAG_WAIT_CLR_ANY:               /* See if any flag set                            */
                 flags_rdy = (OS_FLAGS)~pgrp->OSFlagFlags & pnode->OSFlagNodeFlags;
                 if (flags_rdy != (OS_FLAGS)0) {              /* Make task RTR, event(s) Rx'd          */
                     rdy = OS_FlagTaskRdy(pnode, flags_rdy, OS_STAT_PEND_OK);  
                     if (rdy == OS_TRUE) {
                         sched = OS_TRUE;                     /* When done we will reschedule          */
                     }
                 }
                 break;
#endif
            default:
                 OS_EXIT_CRITICAL();
                 *perr = OS_ERR_FLAG_WAIT_TYPE;
                 return ((OS_FLAGS)0);
        }
        pnode = (OS_FLAG_NODE *)pnode->OSFlagNodeNext; /* Point to next task waiting for event flag(s) */
    }
    OS_EXIT_CRITICAL();
    if (sched == OS_TRUE) {
        OS_Sched();
    }
    OS_ENTER_CRITICAL();
    flags_cur = pgrp->OSFlagFlags;
    OS_EXIT_CRITICAL();
    *perr     = OS_ERR_NONE;
    return (flags_cur);
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                          QUERY EVENT FLAG
*
* Description: This function is used to check the value of the event flag group.
*
* Arguments  : pgrp         is a pointer to the desired event flag group.
*
*              perr          is a pointer to an error code returned to the called:
*                            OS_ERR_NONE                The call was successfull
*                            OS_ERR_FLAG_INVALID_PGRP   You passed a NULL pointer
*                            OS_ERR_EVENT_TYPE          You are not pointing to an event flag group
*
* Returns    : The current value of the event flag group.
*
* Called From: Task or ISR
*********************************************************************************************************
*/

#if OS_FLAG_QUERY_EN > 0u
OS_FLAGS  OSFlagQuery (OS_FLAG_GRP  *pgrp,
                       INT8U        *perr)
{
    OS_FLAGS   flags;
#if OS_CRITICAL_METHOD == 3u                      /* Allocate storage for CPU status register          */
    OS_CPU_SR  cpu_sr = 0u;
#endif



#ifdef OS_SAFETY_CRITICAL
    if (perr == (INT8U *)0) {
        OS_SAFETY_CRITICAL_EXCEPTION();
        return ((OS_FLAGS)0);
    }
#endif

#if OS_ARG_CHK_EN > 0u
    if (pgrp == (OS_FLAG_GRP *)0) {               /* Validate 'pgrp'                                   */
        *perr = OS_ERR_FLAG_INVALID_PGRP;
        return ((OS_FLAGS)0);
    }
#endif
    if (pgrp->OSFlagType != OS_EVENT_TYPE_FLAG) { /* Validate event block type                         */
        *perr = OS_ERR_EVENT_TYPE;
        return ((OS_FLAGS)0);
    }
    OS_ENTER_CRITICAL();
    flags = pgrp->OSFlagFlags;
    OS_EXIT_CRITICAL();
    *perr = OS_ERR_NONE;
    return (flags);                               /* Return the current value of the event flags       */
}
#endif

/*$PAGE*/
/*
*********************************************************************************************************
*                     SUSPEND TASK UNTIL EVENT FLAG(s) RECEIVED OR TIMEOUT OCCURS
*                                    （挂起任务, 直到收到事件标志或超时）
*
* Description: This function is internal to uC/OS-II and is used to put a task to sleep until the desired
*              event flag bit(s) are set.
*
* Arguments  : pgrp          is a pointer to the desired event flag group.
*
*              pnode         is a pointer to a structure which contains data about the task waiting for
*                            event flag bit(s) to be set.
*
*              flags         Is a bit pattern indicating which bit(s) (i.e. flags) you wish to check.
*                            The bits you want are specified by setting the corresponding bits in
*                            'flags'.  e.g. if your application wants to wait for bits 0 and 1 then
*                            'flags' would contain 0x03.
*
*              wait_type     specifies whether you want ALL bits to be set/cleared or ANY of the bits
*                            to be set/cleared.
*                            You can specify the following argument:
*
*                            OS_FLAG_WAIT_CLR_ALL   You will check ALL bits in 'mask' to be clear (0)
*                            OS_FLAG_WAIT_CLR_ANY   You will check ANY bit  in 'mask' to be clear (0)
*                            OS_FLAG_WAIT_SET_ALL   You will check ALL bits in 'mask' to be set   (1)
*                            OS_FLAG_WAIT_SET_ANY   You will check ANY bit  in 'mask' to be set   (1)
*
*              timeout       is the desired amount of time that the task will wait for the event flag
*                            bit(s) to be set.
*
* Returns    : none
*
* Called by  : OSFlagPend()  OS_FLAG.C
*
* Note(s)    : This function is INTERNAL to uC/OS-II and your application should not call it.
*********************************************************************************************************
*/

static  void  OS_FlagBlock (OS_FLAG_GRP  *pgrp,
                            OS_FLAG_NODE *pnode,
                            OS_FLAGS      flags,
                            INT8U         wait_type,
                            INT32U        timeout)
{
    OS_FLAG_NODE  *pnode_next;
    INT8U          y;


    OSTCBCur->OSTCBStat      |= OS_STAT_FLAG;         /* 任务状态                                      */
    OSTCBCur->OSTCBStatPend   = OS_STAT_PEND_OK;
    OSTCBCur->OSTCBDly        = timeout;              /* 任务超时值                                    */
#if OS_TASK_DEL_EN > 0u
    OSTCBCur->OSTCBFlagNode   = pnode;                /* TCB to link to node                           */
#endif
    pnode->OSFlagNodeFlags    = flags;                /* 保存事件标志位                                */
    pnode->OSFlagNodeWaitType = wait_type;            /* 保存事件等待类型                              */
    pnode->OSFlagNodeTCB      = (void *)OSTCBCur;     /* 保存等待的任务                                */
    pnode->OSFlagNodeNext     = pgrp->OSFlagWaitList; /* 添加节点                                      */
    pnode->OSFlagNodePrev     = (void *)0;
    pnode->OSFlagNodeFlagGrp  = (void *)pgrp;         /* 连接事件标志组                                */
    pnode_next                = (OS_FLAG_NODE *)pgrp->OSFlagWaitList;
    if (pnode_next != (void *)0) {                    /* 若这是第一个插入节点...                       */
        pnode_next->OSFlagNodePrev = pnode;           /* 连接节点                                      */
    }
    pgrp->OSFlagWaitList = (void *)pnode;             /* 事件标志等待列表(节点)                        */

    y            =  OSTCBCur->OSTCBY;                 /* 挂起任务,直到收到标志位                       */
    OSRdyTbl[y] &= (OS_PRIO)~OSTCBCur->OSTCBBitX;
    if (OSRdyTbl[y] == 0x00u) {
        OSRdyGrp &= (OS_PRIO)~OSTCBCur->OSTCBBitY;
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                  INITIALIZE THE EVENT FLAG MODULE
*                                      （初始化事件标志组）
*
* Description: 这个函数被uC/OS-II调用来初始化事件标志组。
*
* Arguments  : none
*
* Returns    : none
*
* WARNING    : 这个函数是uC/OS-II系统内部调用的函数，你的应用程序不能调用它。
*********************************************************************************************************
*/

void  OS_FlagInit (void)
{
#if OS_MAX_FLAGS == 1u
    OSFlagFreeList                 = (OS_FLAG_GRP *)&OSFlagTbl[0];  /* 如果只有一个事件标志组          */
    OSFlagFreeList->OSFlagType     = OS_EVENT_TYPE_UNUSED;
    OSFlagFreeList->OSFlagWaitList = (void *)0;
    OSFlagFreeList->OSFlagFlags    = (OS_FLAGS)0;
#if OS_FLAG_NAME_EN > 0u
    OSFlagFreeList->OSFlagName     = (INT8U *)"?";
#endif
#endif

#if OS_MAX_FLAGS >= 2u
    INT16U        ix;
    INT16U        ix_next;
    OS_FLAG_GRP  *pgrp1;
    OS_FLAG_GRP  *pgrp2;


    OS_MemClr((INT8U *)&OSFlagTbl[0], sizeof(OSFlagTbl));           /* 清零事件标志组                  */
    for (ix = 0u; ix < (OS_MAX_FLAGS - 1u); ix++) {                 /* 初始化事件标志组列表            */
        ix_next = ix + 1u;
        pgrp1 = &OSFlagTbl[ix];
        pgrp2 = &OSFlagTbl[ix_next];
        pgrp1->OSFlagType     = OS_EVENT_TYPE_UNUSED;
        pgrp1->OSFlagWaitList = (void *)pgrp2;
#if OS_FLAG_NAME_EN > 0u
        pgrp1->OSFlagName     = (INT8U *)(void *)"?";               /* Unknown name                    */
#endif
    }
    pgrp1                 = &OSFlagTbl[ix];
    pgrp1->OSFlagType     = OS_EVENT_TYPE_UNUSED;
    pgrp1->OSFlagWaitList = (void *)0;
#if OS_FLAG_NAME_EN > 0u
    pgrp1->OSFlagName     = (INT8U *)(void *)"?";                   /* Unknown name                    */
#endif
    OSFlagFreeList        = &OSFlagTbl[0];
#endif
}

/*$PAGE*/
/*
*********************************************************************************************************
*                              MAKE TASK READY-TO-RUN, EVENT(s) OCCURRED
*                                       （使事件标志的任务就绪）
*
* Description: This function is internal to uC/OS-II and is used to make a task ready-to-run because the
*              desired event flag bits have been set.
*
* Arguments  : pnode         is a pointer to a structure which contains data about the task waiting for
*                            event flag bit(s) to be set.
*
*              flags_rdy     contains the bit pattern of the event flags that cause the task to become
*                            ready-to-run.
*
*              pend_stat   is used to indicate the readied task's pending status:
*
*
* Returns    : OS_TRUE       If the task has been placed in the ready list and thus needs scheduling
*              OS_FALSE      The task is still not ready to run and thus scheduling is not necessary
*
* Called by  : OSFlagsPost() OS_FLAG.C
*
* Note(s)    : 1) This function assumes that interrupts are disabled.
*              2) 这个函数是uC/OS-II系统内部调用的函数，你的应用程序不能调用它。
*********************************************************************************************************
*/

static  BOOLEAN  OS_FlagTaskRdy (OS_FLAG_NODE *pnode,
                                 OS_FLAGS      flags_rdy,
                                 INT8U         pend_stat)
{
    OS_TCB   *ptcb;
    BOOLEAN   sched;


    ptcb                 = (OS_TCB *)pnode->OSFlagNodeTCB; /* 指向等待任务                             */
    ptcb->OSTCBDly       = 0u;
    ptcb->OSTCBFlagsRdy  = flags_rdy;
    ptcb->OSTCBStat     &= (INT8U)~(INT8U)OS_STAT_FLAG;
    ptcb->OSTCBStatPend  = pend_stat;
    if (ptcb->OSTCBStat == OS_STAT_RDY) {                  /* 若任务就绪...                            */
        OSRdyGrp               |= ptcb->OSTCBBitY;         /* 是任务进入就绪列表                       */
        OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;
        sched                   = OS_TRUE;
    } else {
        sched                   = OS_FALSE;
    }
    OS_FlagUnlink(pnode);
    return (sched);
}

/*$PAGE*/
/*
*********************************************************************************************************
*                              UNLINK EVENT FLAG NODE FROM WAITING LIST
*
* Description: This function is internal to uC/OS-II and is used to unlink an event flag node from a
*              list of tasks waiting for the event flag.
*
* Arguments  : pnode         is a pointer to a structure which contains data about the task waiting for
*                            event flag bit(s) to be set.
*
* Returns    : none
*
* Called by  : OS_FlagTaskRdy() OS_FLAG.C
*              OSFlagPend()     OS_FLAG.C
*              OSTaskDel()      OS_TASK.C
*
* Note(s)    : 1) This function assumes that interrupts are disabled.
*              2) This function is INTERNAL to uC/OS-II and your application should not call it.
*********************************************************************************************************
*/

void  OS_FlagUnlink (OS_FLAG_NODE *pnode)
{
#if OS_TASK_DEL_EN > 0u
    OS_TCB       *ptcb;
#endif
    OS_FLAG_GRP  *pgrp;
    OS_FLAG_NODE *pnode_prev;
    OS_FLAG_NODE *pnode_next;


    pnode_prev = (OS_FLAG_NODE *)pnode->OSFlagNodePrev;
    pnode_next = (OS_FLAG_NODE *)pnode->OSFlagNodeNext;
    if (pnode_prev == (OS_FLAG_NODE *)0) {                      /* Is it first node in wait list?      */
        pgrp                 = (OS_FLAG_GRP *)pnode->OSFlagNodeFlagGrp;
        pgrp->OSFlagWaitList = (void *)pnode_next;              /*      Update list for new 1st node   */
        if (pnode_next != (OS_FLAG_NODE *)0) {
            pnode_next->OSFlagNodePrev = (OS_FLAG_NODE *)0;     /*      Link new 1st node PREV to NULL */
        }
    } else {                                                    /* No,  A node somewhere in the list   */
        pnode_prev->OSFlagNodeNext = pnode_next;                /*      Link around the node to unlink */
        if (pnode_next != (OS_FLAG_NODE *)0) {                  /*      Was this the LAST node?        */
            pnode_next->OSFlagNodePrev = pnode_prev;            /*      No, Link around current node   */
        }
    }
#if OS_TASK_DEL_EN > 0u
    ptcb                = (OS_TCB *)pnode->OSFlagNodeTCB;
    ptcb->OSTCBFlagNode = (OS_FLAG_NODE *)0;
#endif
}
#endif
