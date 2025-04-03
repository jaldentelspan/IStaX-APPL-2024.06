

/******************************************************************************
*
*  $Id: 6517a3b4707c071ac26ad8fcef236bd9d8d99889
*
*  Copyright (c) 2006-2022 Microchip Technology Inc. and its subsidiaries, all rights reserved.
*  Subject to the terms of the license that accompanies the software and controls as it relates to the software and any conflicting terms herein, you may use this Microchip software and any derivatives exclusively with Microchip products.
*  You are responsible for complying with third party license terms applicable to your use of third party software (including open source software) that may accompany this Microchip software.
*  SOFTWARE IS 'AS IS'. NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
*  IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.
*  TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
*  Module Description:
*     The zl303xx API relies on various functions that need to be provided by
*     the 'host' operating system.  In order to make porting the API to a different
*     'host' easier to achieve an abstraction layer has been used to provide
*     the required functionality.
*
******************************************************************************/
#ifndef _ZL_OSLINUX_H_
#define _ZL_OSLINUX_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef OS_LINUX

/* DEFINES YOU CAN MODIFY */
#define USE_MEM_LOCK                                /* Lock pages into memory mlock() to eliminate page faults */
#define NO_DIRECT_ACCESS_TO_SYSTEM_TICKS             /* This affects osSysTimestampFreq() and osSysTimestamp()! */
                                                    /* If your system can provide better, then #undef and call your own */

/* Realtime priorities are recommended for performance (SCHED_POLICY = SCHED_FIFO or SCHED_RR).
   Alternatively, see define OS_LINUX_USE_SCHED_OTHER_MAPPED_NICE to force use
   of SCHED_OTHER with mapped or computed nice values (performance may be impacted).
*/
/*#define OS_LINUX_USE_SCHED_OTHER_MAPPED_NICE*/

#if !defined OS_LINUX_USE_SCHED_OTHER_MAPPED_NICE &&  defined VALGRIND_TESTING
/* These systems only support SCHED_OTHER so force that */
#define OS_LINUX_USE_SCHED_OTHER_MAPPED_NICE
#endif

#define SCHED_POLICY        SCHED_FIFO              /* SCHED_FIFO or SCHED_RR */

#undef ZL_DEBUG_TASKS
#undef ZL_DEBUG_TASK_TOOLS
/* The following defines are used to control deletion of pthread resources */
#undef USE_LEGACY_PTHREAD_DETACH    /* Used pthread_detach() rather than pthread_join() */
#undef OS_LINUX_USE_ASYNC_CANCEL    /* Synch is the correct method for shutdown */
#undef USE_DEFERRED_FREE            /* Defunct - defer pthread mutex and condv deletion during resource shutdown */


/* DEFINES YOU PROBABLY SHOULDN'T MODIFY */
/*
**  Timeout options
*/
#define NO_WAIT                         0
#define WAIT_FOREVER                    -1
#define WAIT_TIMEOUT                    -2

#define OS_INVALID (-1)
#define NANOSECONDS_IN_1SEC 1000000000L


/*****************   INCLUDE FILES   ******************************************/
#include <linux/types.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <asm/param.h>
#include <sys/mman.h>
#include <signal.h>

#include "zl303xx_Global.h"
#include "zl303xx_OsHeaders.h"
#include "zl303xx_Os.h"
#include "zl303xx_LnxVariants.h"
#include "zl303xx_Error.h"
#include "zl303xx_HWTimer.h"

/* Task information structure */
typedef struct taskInfoS
{
   pthread_t        thread_id;
   pthread_attr_t   attr;
   OS_TASK_FUNC_PTR entryPt;
   struct taskInfoS *nxtTask;   /* Linked list of tasks */
   SINT_T           taskArg;
   SINT_T           taskPri;
   SINT_T           returnVal;
   zl303xx_BooleanE   isDone;
   Uint8T           taskName[32];
} taskInfoS;

typedef struct deferredFreeT
{
    pthread_cond_t *deferredConvToFree;
    void * deferredMemToFree;
    void * deferredMemToFree2;
    struct deferredFreeT *nxtFree;
} deferredFreeT;

typedef enum
{
    SEM_TYPE_COUNT,
    SEM_TYPE_BINARY
} semTypeE;

/* Semaphore information structure */
typedef struct
{
    semTypeE         semType;
    pthread_mutex_t  lock;
    pthread_cond_t   condv;
    SINT_T           count;
    zl303xx_BooleanE   inShutdown;
} semInfoS;


/* Message queue information structure */
typedef struct
{
    pthread_mutex_t   lock;
    pthread_cond_t    condv;
    UINT_T            inPtr;
    UINT_T            outPtr;
    UINT_T            numPendingMsgs;
    UINT_T            maxMsgSize;
    UINT_T            numBufs;
    char *            msgBufs;
    zl303xx_BooleanE    inShutdown;
} msgQInfoS;

extern Uint32T parentTaskId  ;      /*  The main task ID */

Uint8T* taskName(pthread_t threadId);
UnativeT taskIdSelf(void);
UnativeT osTaskSelfIdGet(void);
OS_TASK_ID osTaskSpawn(const char *name, Sint32T priority, Sint32T unused, Sint32T stackSize, OS_TASK_FUNC_PTR entryPt, Sint32T taskArg);
osStatusT osTaskDelete (OS_TASK_ID tid);
osStatusT osTaskDelay(Sint32T ticks);
OS_SEM_ID osSema4Create(Sint32T initialCount);
OS_SEM_ID osSema4CreateBinary(osMiscT initialState);
osStatusT osSema4Give(OS_SEM_ID semId);
osStatusT osSema4Take(OS_SEM_ID semId, Sint32T timeout);
osStatusT osSema4Delete(OS_SEM_ID semId);
OS_SEM_ID osMutexCreate(void);
osStatusT osMutexGive(OS_MUTEX_ID mutex);
osStatusT osMutexTake(OS_MUTEX_ID mutex);
osStatusT osMutexTakeT(OS_MUTEX_ID mutex, Sint32T timeout);
osStatusT osMutexDelete(OS_MUTEX_ID mutex);
OS_MSG_Q_ID osMsgQCreate(Sint32T maxMsgs, Sint32T maxMsgLength);
osStatusT osMsgQSend(OS_MSG_Q_ID msgQId, Sint8T *buffer, Uint32T nBytes, Sint32T timeout);
osStatusT osMsgQReceive(OS_MSG_Q_ID msgQId, Sint8T *buffer, Uint32T maxNBytes, Sint32T timeout);
osStatusT osMsgQDelete(OS_MSG_Q_ID msgQId);
Sint32T osMsgQNumMsgs(OS_MSG_Q_ID msgQId);
void *osCalloc(size_t NumberOfElements, size_t size);
void osFree(void *ptrToMem);
osStatusT zl303xx_DeferredConVFree(void);


#if defined _ZL303XX_OS_SIGNAL_HANDLER
/* Signal handlers */
zlStatusE osSignalHandlerRegister(Uint32T sigNum, void (*callout)(Sint32T sigNum, Sint32T pid));
zlStatusE osSignalHandlerUnregister(Uint32T sigNum);
zlStatusE osSignalTaskStart(void);
zlStatusE osSignalTaskStop(void);

#define OS_SIGHNDLR_NAME     "zlOsSignalTask"
#define OS_SIGHNDLR_PRIORITY 96
#define OS_SIGHNDLR_STACK_SZ 8192

#if defined _LINUX_NP
OS_TASK_ID getSignalTaskId(void);   /* pTheadId of handler */
#endif
#endif


#endif /* OS_LINUX */



#ifdef __cplusplus
}
#endif

#endif
