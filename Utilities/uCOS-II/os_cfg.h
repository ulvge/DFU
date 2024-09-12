/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*                                  uC/OS-II Configuration File for V2.9x
*
*                               (c) Copyright 2005-2012, Micrium, Weston, FL
*                                          All Rights Reserved
*
*
* File    : OS_CFG.H
* By      : JJL
*
* Version : V2.92.00
*
* LICENSING TERMS:
* ---------------
*   uC/OS-II is provided in source form for FREE evaluation, for educational use or for peaceful research.
* If you plan on using  uC/OS-II  in a commercial product you need to contact Micri�m to properly license
* its use in your product. We provide ALL the source code for your convenience and to help you experience
* uC/OS-II.   The fact that the  source is provided does  NOT  mean that you can use it without  paying a
* licensing fee.
*********************************************************************************************************
*/

#ifndef OS_CFG_H
#define OS_CFG_H
                        
                                       /* ------------------------- ������� ------------------------- */
#define OS_APP_HOOKS_EN           0u   /* APP���ӣ�hooks������                                         */
#define OS_ARG_CHK_EN             0u   /* Enable (1) or Disable (0) �������                           */
#define OS_CPU_HOOKS_EN           1u   /* CUP���ӣ�hooks������                                         */

#define OS_DEBUG_EN               0u   /* ����uC/OS �Դ��ĵ��Թ���                                     */

#define OS_EVENT_MULTI_EN         0u   /* ������OSEventPendMulti()����                                 */
#define OS_EVENT_NAME_EN          0u   /* Enable names for Sem, Mutex, Mbox and Q                      */

#define OS_LOWEST_PRIO            16u  /* ������Ϳɷ�������ȼ� ...                                   */
                                       /* ... ����С��254                                              */

#define OS_MAX_EVENTS             6u   /* �����Ӧ�ó����У��¼����ƿ�������                         */
#define OS_MAX_FLAGS              6u   /* �����Ӧ�ó����У��¼���־��������                         */
#define OS_MAX_MEM_PART           6u   /* �ڴ���������                                               */
#define OS_MAX_QS                 8u   /* �����Ӧ�ó����У����п��ƿ�������                         */
#define OS_MAX_TASKS              8u   /* �����Ӧ�ó����У�Ӧ�ó������񣩵������                   */

#define OS_SCHED_LOCK_EN          1u   /* ������OSSchedLock() and OSSchedUnlock()���ȼ�������          */

#define OS_TICK_STEP_EN           0u   /* Enable tick stepping feature for uC/OS-View                  */

#define   SYSTICK_CYC   1               /*ms                                                           */
 
#define OS_TICKS_PER_SEC        (1000/SYSTICK_CYC)   /* ����ÿ���ӵδ���  ÿ1s�����ٸ�Tick*/
                                       


                                       /* ----------------------- �����ջ��С ----------------------- */
#define OS_TASK_TMR_STK_SIZE     64u   /* Timer      task stack size (# of OS_STK wide entries)        */
#define OS_TASK_STAT_STK_SIZE    64u   /* Statistics task stack size (# of OS_STK wide entries)        */
#define OS_TASK_IDLE_STK_SIZE    64u   /* Idle       task stack size (# of OS_STK wide entries)        */


                                       /* ------------------------- ������� ------------------------- */
#define OS_TASK_CHANGE_PRIO_EN    0u   /* ?����OSTaskChangePrio()----------------�������ȼ��л�        */
#define OS_TASK_CREATE_EN         1u   /* ?����OSTaskCreate()--------------------����һ������          */
#define OS_TASK_CREATE_EXT_EN     1u   /* ?����OSTaskCreateExt()-----------------����һ��������չ�汾  */
#define OS_TASK_DEL_EN            1u   /* ?����OSTaskDel()-----------------------ɾ��һ������          */
#define OS_TASK_NAME_EN           0u   /* ?����OSTaskNameGet()��OSTaskNameSet()--Get,Set��������       */
#define OS_TASK_PROFILE_EN        0u   /* ?����������������ƿ�(TCB)�еķ������ж�                     */
#define OS_TASK_QUERY_EN          0u   /* ?����OSTaskQuery()---��ѯ��ȡ������Ϣ                        */
#define OS_TASK_REG_TBL_SIZE      0u   /* �����������Ĵ�С(#of INT32U entries)                       */
#define OS_TASK_STAT_EN           1u   /* ?����ͳ�ƣ�CPUʹ���ʣ�                                       */
#define OS_TASK_STAT_STK_CHK_EN   1u   /* ?����OSTaskStkChk()��OS_TaskStkClr-----��顢�������        */
#define OS_TASK_SUSPEND_EN        0u   /* ?����OSTaskSuspend()��OSTaskResume()---������𡢻ָ�        */
#define OS_TASK_SW_HOOK_EN        1u   /* ?����OSTaskSwHook()--------------------�����Ӻ���          */


                                       /* ------------------------- �¼���־ ------------------------- */
#define OS_FLAG_EN                0u   /* Enable (1) or Disable (0) code generation for EVENT FLAGS    */
#define OS_FLAG_ACCEPT_EN         0u   /*     Include code for OSFlagAccept()                          */
#define OS_FLAG_DEL_EN            0u   /*     Include code for OSFlagDel()                             */
#define OS_FLAG_NAME_EN           0u   /*     Enable names for event flag group                        */
#define OS_FLAG_QUERY_EN          0u   /*     Include code for OSFlagQuery()                           */
#define OS_FLAG_WAIT_CLR_EN       0u   /* Include code for Wait on Clear EVENT FLAGS                   */
#define OS_FLAGS_NBITS            8u   /* Size in #bits of OS_FLAGS data type (8, 16 or 32)            */


                                       /* ------------------------- ��Ϣ���� ------------------------- */
#define OS_MBOX_EN                0u   /* Enable (1) or Disable (0) code generation for MAILBOXES      */
#define OS_MBOX_ACCEPT_EN         0u   /*     Include code for OSMboxAccept()                          */
#define OS_MBOX_DEL_EN            0u   /*     Include code for OSMboxDel()                             */
#define OS_MBOX_PEND_ABORT_EN     0u   /*     Include code for OSMboxPendAbort()                       */
#define OS_MBOX_POST_EN           0u   /*     Include code for OSMboxPost()                            */
#define OS_MBOX_POST_OPT_EN       0u   /*     Include code for OSMboxPostOpt()                         */
#define OS_MBOX_QUERY_EN          0u   /*     Include code for OSMboxQuery()                           */


                                       /* ------------------------- �ڴ���� ------------------------- */
#define OS_MEM_EN                 0u   /* Enable (1) or Disable (0) code generation for MEMORY MANAGER */
#define OS_MEM_NAME_EN            0u   /*     Enable memory partition names                            */
#define OS_MEM_QUERY_EN           0u   /*     Include code for OSMemQuery()                            */


                                       /* ------------------------ �����ź��� ------------------------ */
#define OS_MUTEX_EN               0u   /* Enable (1) or Disable (0) code generation for MUTEX          */
#define OS_MUTEX_ACCEPT_EN        0u   /*     Include code for OSMutexAccept()                         */
#define OS_MUTEX_DEL_EN           0u   /*     Include code for OSMutexDel()                            */
#define OS_MUTEX_QUERY_EN         0u   /*     Include code for OSMutexQuery()                          */


                                       /* ------------------------- ��Ϣ���� ------------------------- */
#define OS_Q_EN                   0u   /* Enable (1) or Disable (0) code generation for QUEUES         */
#define OS_Q_ACCEPT_EN            0u   /*     Include code for OSQAccept()                             */
#define OS_Q_DEL_EN               0u   /*     Include code for OSQDel()                                */
#define OS_Q_FLUSH_EN             0u   /*     Include code for OSQFlush()                              */
#define OS_Q_PEND_ABORT_EN        0u   /*     Include code for OSQPendAbort()                          */
#define OS_Q_POST_EN              0u   /*     Include code for OSQPost()                               */
#define OS_Q_POST_FRONT_EN        0u   /*     Include code for OSQPostFront()                          */
#define OS_Q_POST_OPT_EN          0u   /*     Include code for OSQPostOpt()                            */
#define OS_Q_QUERY_EN             0u   /*     Include code for OSQQuery()                              */


                                       /* -------------------------- �ź��� -------------------------- */
#define OS_SEM_EN                 1u   /* Enable (1) or Disable (0) code generation for SEMAPHORES     */
#define OS_SEM_ACCEPT_EN          0u   /*    Include code for OSSemAccept()                            */
#define OS_SEM_DEL_EN             0u   /*    Include code for OSSemDel()                               */
#define OS_SEM_PEND_ABORT_EN      0u   /*    Include code for OSSemPendAbort()                         */
#define OS_SEM_QUERY_EN           0u   /*    Include code for OSSemQuery()                             */
#define OS_SEM_SET_EN             0u   /*    Include code for OSSemSet()                               */


                                       /* ------------------------- ʱ����� ------------------------- */
#define OS_TIME_DLY_HMSM_EN       0u   /* ?����OSTimeDlyHMSM()-------------------������ʱָ��ʱ��      */
#define OS_TIME_DLY_RESUME_EN     0u   /* ?����OSTimeDlyResume()-----------------������ʱ����          */
#define OS_TIME_GET_SET_EN        0u   /* ?����OSTimeGet() and OSTimeSet() ------Get,Setϵͳʱ����ֵ   */
#define OS_TIME_TICK_HOOK_EN      1u   /* ?����OSTimeTickHook()                                        */


                                       /* ------------------------ ��ʱ������ ------------------------ */
#define OS_TMR_EN                 1u   /* Enable (1) or Disable (0) code generation for TIMERS         */
#define OS_TMR_CFG_MAX            8u   /*     Maximum number of timers                                 */
#define OS_TMR_CFG_NAME_EN        0u   /*     Determine timer names                                    */
#define OS_TMR_CFG_WHEEL_SIZE     2u   /*     Size of timer wheel (#Spokes)                            */
#define OS_TMR_CFG_TICKS_PER_SEC 100u   /*     Rate at which timer management task runs (Hz)            */

#endif
