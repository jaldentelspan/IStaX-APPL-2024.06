

/******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     The zl303xx API relies on various functions that need to be provided by
*     the 'host' operating system.  In order to make porting the API to a different
*     'host' easier to achieve an abstraction layer has been used to provide
*     the required functionality.
*
******************************************************************************/

#ifndef _ZL_OS_H_
#define _ZL_OS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/

#include "zl303xx_DataTypes.h"
#include "zl303xx_LibcAbstractions.h"

#include <stdlib.h>



#define OS_MISC_TYPE UnativeT
#define OS_SMISC_TYPE SnativeT

#define OS_SEM_ID    OS_MISC_TYPE
#define OS_MUTEX_ID  OS_MISC_TYPE
#define OS_MSG_Q_ID  OS_MISC_TYPE
#define OS_TASK_ID   OS_SMISC_TYPE


typedef Sint32T osStatusT;
typedef osStatusT OS_STATUS;

typedef OS_MISC_TYPE osMiscT;
typedef OS_SMISC_TYPE osSMiscT;
typedef Sint32T (*OS_FUNC_PTR) (void);
typedef void (*OS_VOID_FUNC_PTR) (void);
typedef Sint32T (*OS_ARG1_FUNC_PTR) (Uint32T);

#ifdef OS_LINUX
   typedef Sint32T (*OS_TASK_FUNC_PTR) (Sint32T);
#else
   typedef Sint32T (*OS_TASK_FUNC_PTR) (void);
#endif

/* include the function definitions */
#include "zl303xx_OsFunctions.h"

/*****************   General Constants   **************************************/

extern const osStatusT OsStatusOk;
extern const osStatusT OsStatusError;
extern const Uint32T OsRandMax;

extern const osSMiscT OsWaitForever;
extern const osSMiscT OsTimeout;
extern const osSMiscT OsNoWait;

extern const osMiscT OsSemEmpty;
extern const osMiscT OsSemFull;

/*****************   OS Dependent Endian Definitions   ************************/

/* Fixup differing endianess macro styles */

#if defined(OS_FREERTOS)
#define __BIG_ENDIAN _BIG_ENDIAN
#define __LITTLE_ENDIAN _LITTLE_ENDIAN
#define __BYTE_ORDER _BYTE_ORDER
#endif

#if defined OS_VXWORKS
   #include <types/vxArch.h>
   #define __BIG_ENDIAN _BIG_ENDIAN
   #define __LITTLE_ENDIAN _LITTLE_ENDIAN
   #define __BYTE_ORDER _BYTE_ORDER
#endif


#ifndef __BYTE_ORDER
#define __BYTE_ORDER _BYTE_ORDER
#endif
#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN _BIG_ENDIAN
#endif
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN _LITTLE_ENDIAN
#endif
#if !defined(__BYTE_ORDER)
   #error Endian constants not defined
#endif


#if defined OS_FREERTOS
#include "freeRTOS/include/FreeRTOS.h"
#endif

/*****************   OS Definitions   *****************************************/

#define OS_OK           OsStatusOk
#define OS_ERROR        OsStatusError
#define OS_WAIT_FOREVER OsWaitForever
#define OS_TIMEOUT      OsTimeout
#define OS_NO_WAIT      OsNoWait

/* define minimum -> maximum range for the data types */
#define OS_UINT32T_MAX     (Uint32T)4294967295
#define OS_SINT32T_MAX     (Sint32T)2147483647
#define OS_SINT32T_MIN     (-OS_SINT32T_MAX - 1)

#define OS_UINT16T_MAX     (Uint16T)65535
#define OS_SINT16T_MAX     (Sint16T)32767
#define OS_SINT16T_MIN     (-OS_SINT16T_MAX - 1)

#define OS_UINT8T_MAX      (Uint8T)255
#define OS_SINT8T_MAX      (Sint8T)127
#define OS_SINT8T_MIN      (-OS_SINT8T_MAX - 1)

#define OS_RAND_MAX        OsRandMax;

/*****************   Mutex / Sema4 related constants   ************************/

/* The following definitions are for binary semaphores/mutexes.
   OS_MUTEX_ID is an OS specific type returned by OS_MUTEX_CREATE
   which uniquely identifies a particular mutex.
   See OS_MUTEX_CREATE for description of OS_SEM_EMPTY and OS_SEM_FULL.
   */
#define OS_SEM_EMPTY       0
#define OS_SEM_FULL        1
#define OS_SEM_INVALID     (OS_SEM_ID)NULL
#define OS_MUTEX_INVALID   (OS_MUTEX_ID)NULL

/*****************   Message queue related constants   ************************/
#define OS_MSG_Q_INVALID   (OS_MSG_Q_ID)NULL

/*****************   Task Control related constants   *************************/
#define OS_TASK_INVALID   (OS_TASK_ID)NULL

/*****************   Task Control   *******************************************/
/* Max and min priority values. */
#if defined OS_VXWORKS
   #define OS_TASK_MAX_PRIORITY  0   /* Highest task priority */
   #define OS_TASK_MIN_PRIORITY  255
#elif defined OS_LINUX
   #define OS_TASK_MAX_PRIORITY  99  /* Highest task priority */
   #define OS_TASK_MIN_PRIORITY  0
#elif defined OS_FREERTOS           /* SOCPG_PORTING */
   #define OS_TASK_MAX_PRIORITY  31  /* Highest task priority */
   #define OS_TASK_MIN_PRIORITY  0
#elif !defined OS_TASK_MAX_PRIORITY || !defined OS_TASK_MIN_PRIORITY
   #error NO MAX or MIN_PRIORITY FOR YOUR OS DEFINED!
#endif

#define OS_MAX_TASK_NAME 32

/* Create and activate a new task */
#define OS_TASK_SPAWN         osTaskSpawn

/* Delete a task */
#define OS_TASK_DELETE        osTaskDelete

#define OS_TASK_LOCK          osTaskLock
#define OS_TASK_UNLOCK        osTaskUnLock

#define OS_NET_TASK_PRIORITY  osNetTaskPriority

#define OS_TASK_SELF_ID_GET   taskIdSelf

/* Work out the number of system ticks for given number of milliseconds */
#define OS_TICKS(milliseconds) ((OS_TICK_RATE_GET() * (milliseconds))/1000)

/* Delay a task from executing */
/* Note: The delay period should be specified in milliseconds */
#ifndef OS_LINUX
    #define OS_TASK_DELAY(milliseconds) OS_TICK_DELAY(OS_TICKS(milliseconds)+1)
    #define OS_TICK_DELAY         osTaskDelay
#endif
#ifdef OS_LINUX
    #define OS_TASK_DELAY(millis) osTaskDelay(millis)   /* Everything is in millisecs */
    #define OS_TICK_DELAY(millis) osTaskDelay(millis)
#endif

/*****************   Time   ***************************************************/

/* Get the value of the kernel's tick counter */
#define OS_TICK_GET           osTickGet

/* Get the OS tick rate */
#define OS_TICK_RATE_GET      osTickRateGet

#define OS_TICK_RATE_SET      osTickRateSet

/* Get the value of the kernel's absolute time (CLOCK_MONOTONIC) */
#define OS_TIME_GET           osTimeGet
/*****************   Interrupts   *********************************************/

#define OS_INTERRUPT_CONNECT  osInterruptConnect
#define OS_INTERRUPT_LOCK     osInterruptLock
#define OS_INTERRUPT_UNLOCK   osInterruptUnlock
#define OS_INTERRUPT_ENABLE   osInterruptEnable
#define OS_INTERRUPT_DISABLE  osInterruptDisable

/*****************   Semaphores   *********************************************/

/* Create and initialise a counting semaphore */
#define OS_SEMA4_CREATE          osSema4Create
#define OS_SEMA4_CREATE_BINARY   osSema4CreateBinary
#define OS_SEMA4_GIVE            osSema4Give
#define OS_SEMA4_TAKE            osSema4Take
#define OS_SEMA4_DELETE          osSema4Delete

/*****************   Mutex Semaphores   ***************************************/

/* Create and initialise a mutex (binary) semaphore */
#define OS_MUTEX_CREATE       osMutexCreate
#define OS_MUTEX_TAKE         osMutexTake
#define OS_MUTEX_TAKE_T       osMutexTakeT
#define OS_MUTEX_GIVE         osMutexGive
#define OS_MUTEX_DELETE       osMutexDelete

/*****************   Message Queues   *****************************************/

/* Create and initialise a message queue */
#define OS_MSG_Q_CREATE       osMsgQCreate
#define OS_MSG_Q_SEND         osMsgQSend
#define OS_MSG_Q_RECEIVE      osMsgQReceive
#define OS_MSG_Q_DELETE       osMsgQDelete
#define OS_MSG_Q_NUM_MSG      osMsgQNumMsgs

/*****************   logging functions   **************************************/

#define OS_LOG_INIT           osLogInit
#define OS_LOG_CLOSE          osLogClose
#define OS_LOG_MSG            osLogMsg

/*****************   Memory allocation   **************************************/

#ifndef OS_LINUX
#ifdef OS_FREERTOS           /* SOCPG_PORTING */
#define OS_CALLOC             pvPortCalloc	//fixme- - malachy we should make these FreeRTOS thread safe
#define OS_FREE               vPortFree		//fixme- - we should make theses FreeRTOS thread safe
#else
#define OS_DPRAM_MALLOC       osDpramMalloc
#define OS_DPRAM_FREE         osDpramFree
#define OS_CALLOC             calloc
#define OS_FREE               free
#endif
#endif

#ifdef OS_LINUX
#define OS_CALLOC             osCalloc
#define OS_FREE               osFree
#endif  /* OS_LINUX */

/*****************    Timer functions    **************************************/

#define OS_SYS_TIMESTAMP_ENABLE     osSysTimestampEnable
#define OS_SYS_TIMESTAMP_DISABLE    osSysTimestampDisable
#define OS_SYS_TIMESTAMP_FREQ       osSysTimestampFreq
#define OS_SYS_TIMESTAMP            osSysTimestamp

/*****************    Misc functions     **************************************/

#define OS_IMMR_GET                 osImmrGet

#define OS_SETUP                    osSetup
#define OS_CLEANUP                  osCleanup

/******************  LIBC FUNCTION abstractions *******************************/

#if defined REPLACE_OS_PRINTF
#define fprintf         zl303xx_OsFprintf
#define printf          zl303xx_OsPrintf
#endif

#define OS_ABS          zl303xx_OsAbs
#define OS_MEMCPY       zl303xx_OsMemcpy
#define OS_MEMSET       zl303xx_OsMemset
#define OS_SPRINTF      zl303xx_OsSprintf
#define OS_SNPRINTF     zl303xx_OsSnprintf
#define OS_STRLEN       zl303xx_OsStrlen
#define OS_STRNCAT      zl303xx_OsStrncat
#define OS_ERRNO        zl303xx_OsErrno


#ifdef __cplusplus
}
#endif

#endif
