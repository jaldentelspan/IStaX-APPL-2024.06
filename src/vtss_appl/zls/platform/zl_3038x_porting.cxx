/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.
*/

#include "zl303xx_Os.h"
#include "zl303xx_Trace.h"
#include "zl303xx_LogToMsgQ.h"
#include "zl_3038x_api_pdv.h"
#include "vtss_tod_api.h"
#include "vtss_os_wrapper.h"
#include "critd_api.h"

#include "vtss_timer_api.h"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_ZL_3034X_API
#ifdef VTSS_SW_OPTION_ZLS30387
    #define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_ZL_3034X_PDV
#else
    #define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_ZL_3034X_API
#endif //VTSS_SW_OPTION_ZLS30387
#define OK      0
#define NO_WAIT                         0
#define WAIT_FOREVER                    -1
#define WAIT_TIMEOUT                    -2
#define ERROR   (-1)
#define VTSS_FLAG_BASED_SEMA4

/* TBD: Replace with appropriate locks */
#define TASK_LOCK()   {vtss_global_lock(__FILE__, __LINE__);}
#define TASK_UNLOCK() {vtss_global_unlock(__FILE__, __LINE__);}

// #define MQ_LOCK()   {vtss_global_lock(__FILE__, __LINE__);}
// #define MQ_UNLOCK() {vtss_global_unlock(__FILE__, __LINE__);}

#define ZL303XX_ABORT_FN exit(-1)
const osStatusT OsStatusOk = (osStatusT)OK;
const osSMiscT OsNoWait = (osSMiscT)NO_WAIT;
const osSMiscT OsWaitForever = (osSMiscT)WAIT_FOREVER;
const osSMiscT OsTimeout = (osSMiscT)WAIT_TIMEOUT;

const osStatusT OsStatusError = (osStatusT)ERROR;
// const char      err_msg_parm[64] = "Error(%d) in file: %s at line:%d ";

/***********************************************************************************
 * Task information block:-                                                        *
 * t_handler: Task handler                                                         *
 * t_info:    Task information                                                     *
 ***********************************************************************************/
typedef struct task {
    vtss_handle_t t_handler;
    vtss_thread_t t_info;
    struct task   *next;
} task_t;

static task_t *g_task_list;

/* Message queue information structure */
typedef struct
{
   vtss_mutex_t  lock;
   vtss_cond_t   condv;
   Uint32T            inPtr;
   Uint32T            outPtr;
   Uint32T            numPendingMsgs;
   Uint32T            maxMsgSize;
   Uint32T            numBufs;
   zl303xx_BooleanE   inShutdown;
   char    *msgBufs;
} msgQInfoS;

/* Sema4 information structure */
typedef struct
{
   vtss_flag_t  flag;
   zl303xx_BooleanE   inShutdown;
} sema4InfoS;

/* Mutex information structure */
typedef struct
{
   vtss_recursive_mutex_t mutex;
   zl303xx_BooleanE   inShutdown;
} mutexInfoS;

// #ifdef TASK_MGMT
// static mesa_rc task_list_init(task_t *task_list)
// {
//     task_list = NULL;
//
//     return VTSS_RC_OK;
// }
//
// static mesa_rc task_find(task_t *task_list, vtss_handle_t t_handler, task_t **task)
// {
//     task_t *tmp_task = task_list;
//
//     while (tmp_task) {
//         if (tmp_task->t_handler == t_handler) {
//             *task = tmp_task;
//             break;
//         }
//         tmp_task = tmp_task->next;
//     }
//     if (tmp_task == NULL) {
//         *task = NULL;
//         T_EG(TRACE_GRP_OS_PORT, "Specified Task not found in the Task list");
//         return VTSS_RC_ERROR;
//     } else {
//         return VTSS_RC_OK;
//     }
// }
// #endif /* end of TASK_MGMT. These functions are not used yet */

static mesa_rc task_insert(task_t **task_list,  task_t *task)
{
    if (!task) {
        return VTSS_RC_ERROR;
    }
    task->next = *task_list;
    *task_list = task;

    return VTSS_RC_OK;
}

static mesa_rc _task_delete(task_t **task_list, vtss_handle_t t_handler)
{
    task_t *cur_task = *task_list;
    task_t *prev_task = NULL;
    mesa_rc  rc = VTSS_RC_ERROR;

    while(cur_task) {
        if (cur_task->t_handler == t_handler) {
            if (prev_task == NULL) { /* Delete the first task */
                *task_list = (*task_list)->next;
            } else {
                prev_task->next = cur_task->next;
            }
            VTSS_FREE(cur_task);
            rc = VTSS_RC_OK;
            break;
        }
        prev_task = cur_task;
        cur_task = cur_task->next;
    }

    return rc;
}

static mesa_rc task_create(char               *name,
                           vtss_thread_prio_t priority,
                           Sint32T            options,
                           Sint32T            stackSize,
                           OS_TASK_FUNC_PTR   entryPt,
                           Sint32T            taskArg,
                           i32                *tcb_id,
                           OS_TASK_ID         *task_id)

{
    mesa_rc rc = VTSS_RC_OK;
    task_t  *new_task = NULL;

    do {
        new_task = (task_t *) VTSS_MALLOC(sizeof (task_t));
        if (new_task == NULL) {
            T_EG (TRACE_GRP_OS_PORT, "Unable to allocate memory for task: %s", name);
            rc = VTSS_RC_ERROR;
            break;
        } else {
            new_task->next = NULL;
            vtss_thread_create(priority,
                    (vtss_thread_entry_f *)entryPt,
                    (vtss_addrword_t)((intptr_t)taskArg),
                    name,
                    nullptr,
                    0,
                    &new_task->t_handler,
                    &new_task->t_info);

            *task_id = (OS_TASK_ID)new_task->t_handler;
            TASK_LOCK();
            if (task_insert(&g_task_list, new_task) != VTSS_RC_OK) {
                rc = VTSS_RC_ERROR;
            }
            TASK_UNLOCK();
            if (rc == VTSS_RC_ERROR) {
                T_EG (TRACE_GRP_OS_PORT, "Unable to insert task(%s) in task list", name);
            }
        }
    } while(0); /* end of do-while */


    return rc;
}

static mesa_rc task_delete(vtss_handle_t t_handler)
{
    mesa_rc rc = VTSS_RC_OK;

    vtss_thread_prio_set(t_handler, VTSS_THREAD_PRIO_HIGHEST);  // Boost thread's priority to highest level to enable it to terminate
    if (vtss_thread_delete(t_handler) == TRUE) {
        TASK_LOCK();
        if (_task_delete(&g_task_list, t_handler) != VTSS_RC_OK) {
            rc = VTSS_RC_ERROR;
        }
        TASK_UNLOCK();

        if (rc == VTSS_RC_ERROR) {
            T_EG (TRACE_GRP_OS_PORT, "Unable to delete the specified task from task list");
        }
    } else {
            T_EG (TRACE_GRP_OS_PORT, "Unable to delete the specified task from Kernel");
    }

    return rc;
}

// /******************************************************************************
//  * Trace function: Filters the trace messages based on debug flag             *
//  *                                                                            *
//  *****************************************************************************/
//
// void zl303xx_TraceFnFiltered(Uint32T modId, Uint32T level, const char *str,
//       UnativeT arg0, UnativeT arg1, UnativeT arg2, UnativeT arg3, UnativeT arg4, UnativeT arg5)
// {
// #if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_DEBUG)
//     char buf[MAX_FMT_LEN] = {0};
//     i32 temp_len;
//
//     temp_len = snprintf(buf, MAX_FMT_LEN, str, arg0, arg1, arg2, arg3, arg4, arg5);
//     if ((temp_len < 0) || (temp_len > MAX_FMT_LEN)) {
//         T_EG(TRACE_GRP_ZL_TRACE, "Buffer Size too Small to fit the supplied message");
//         return;
//     }
//     if (modId == ZL303XX_MOD_ID_NOTIFY) {
//         level = 0;
//     }  // rise level for notifications
//     switch(level) {
//         case 0:
//             T_IG(TRACE_GRP_ZL_TRACE, "modId %d,%s", modId, buf);
//             break;
//         case 1:
//             T_DG(TRACE_GRP_ZL_TRACE, "modId %d,%s", modId, buf);
//             break;
//         case 2:
//             T_NG(TRACE_GRP_ZL_TRACE, "modId %d,%s", modId, buf);
//             break;
//         default:
//             T_RG(TRACE_GRP_ZL_TRACE, "modId %d,%s", modId, buf);
//             break;
//     }
// #endif
//
// }
//
// /******************************************************************************
//  * Trace function:                                                            *
//  *                                                                            *
//  *****************************************************************************/
// void zl303xx_TraceFnNoFilter(const char * str, UnativeT arg0, UnativeT arg1, UnativeT arg2, UnativeT arg3, UnativeT arg4, UnativeT arg5)
// {
//     char buf[MAX_FMT_LEN] = {0};
//     i32 tmp_len;
//     tmp_len = snprintf(buf, MAX_FMT_LEN, str, arg0, arg1, arg2, arg3, arg4, arg5);
//     if (tmp_len < 0 || tmp_len > MAX_FMT_LEN) {
//         T_EG(TRACE_GRP_ZL_TRACE, "Buffer Size too Small to fit the supplied message");
//         return;
//     }
//     T_WG(TRACE_GRP_OS_PORT, "%s", buf);
// }

void zl303xx_ErrorTrapFn(const Uint32T bStopOnError, const char *errCodeString,
                       const char *fileName, const char * const lineNum)
{
   if (errCodeString)
   {
      /* Build up the error message. We cannot assume that the error message will
         be displayed immediately so some rules apply:
            1. Any items identified by pointers must be constants
            2. Therefore we can't use any items (particularly) strings on the stack
         However, in our favour is the fact that all the parameters to this function
         are compiler generated constants so we can satisfy these rules if we are careful. */

      if (bStopOnError == ZL303XX_TRUE)
      {
         if (fileName)
         {
            if (lineNum)
            {
               ZL303XX_TRACE_ERROR("Fatal Error: \"%s\", in %s, line %s" ZL303XX_NEWLINE,
                     errCodeString,
                     fileName,
                     lineNum,
                     0, 0, 0);
               (void)OS_TASK_DELAY(1000);
            }
            else /* no linenumber given */
            {
               ZL303XX_TRACE_ERROR("Fatal Error: \"%s\", in %s" ZL303XX_NEWLINE,
                     errCodeString,
                     fileName,
                     0, 0, 0, 0);
               (void) OS_TASK_DELAY(1000);
            }
         }
         else
         {
            ZL303XX_TRACE_ERROR("Fatal Error: \"%s\"\n",
                  errCodeString, 0, 0, 0, 0, 0);
            (void) OS_TASK_DELAY(1000);
         }

         /* We should stop on error so abort the current thread */
         ZL303XX_ABORT_FN;

      }
      else  /* A "less than severe" error */
      {
         if (fileName)
         {
            if (lineNum)
            {
               ZL303XX_TRACE_ERROR("Error: \"%s\", in %s, line %s",
                     errCodeString, fileName, lineNum, 0, 0, 0);
            }
            else /* no linenumber given */
            {
               ZL303XX_TRACE_ERROR("Error: \"%s\", in %s",
                     errCodeString, fileName, 0, 0, 0, 0);
            }
         }
         else
         {
            ZL303XX_TRACE_ERROR("Error: \"%s\"\n",
                  errCodeString, 0, 0, 0, 0, 0);
         }
      }
   }
}

#if 0

// ScaledNs32T ClockPeriod_ScaledNs(Uint32T freqHz)
// {
// /* Not finished */
//     return(0);
// }
//
// /********************************************************************************
//    Function to divide a Uint64S value by a Uint32T value to produce a Uint64S
//       result->hi = the 32-bit whole portion of the result
//       result->lo = the 32-bit remainder (modulus)
// *********************************************************************************/
// Uint64S Div_U64S_U32(Uint64S num, Uint32T den, Uint32T *mod)
// {
//     Uint64S result;
//     u64 num64, result64;
//
//     num64 = (num.hi*0x100000000LL) + num.lo;
//     result64 = num64/den;
//     result.hi = (result64/0x100000000LL) & 0xFFFFFFFF;
//     result.lo = result64 & 0xFFFFFFFF;
//
//     if (mod)    *mod = (num64%den) & 0xFFFFFFFF;
//
//     return(result);
// }
//
// /********************************************************************************
//    Function to shift a Uint64S value to the left by lshift number of bits.
// *********************************************************************************/
// Uint64S LShift_U64S(Uint64S inVal, Uint8T lshift)
// {
//     Uint64S result;
//     u64 inVal64, result64;
//
//     inVal64 = (inVal.hi*0x100000000LL) + inVal.lo;
//     result64 = inVal64<<lshift;
//     result.hi = (result64/0x100000000LL) & 0xFFFFFFFF;
//     result.lo = result64 & 0xFFFFFFFF;
//
//     return(result);
// }
//
// /********************************************************************************
//    Function to add 2 Uint64S values.
// *********************************************************************************/
// Uint64S Add_U64S(Uint64S val1, Uint64S val2, Uint8T *carry)
// {
//     Uint64S result;
//     u64 val164, val264, result64;
//
//     val164 = (val1.hi*0x100000000LL) + val1.lo;
//     val264 = (val2.hi*0x100000000LL) + val2.lo;
//     result64 = val164 + val264;
//     result.hi = (result64/0x100000000LL) & 0xFFFFFFFF;
//     result.lo = result64 & 0xFFFFFFFF;
//
//     if (carry)   *carry = ((result.hi < val1.hi) ? (1) : (0));
//
//     return(result);
// }
//
// /********************************************************************************
//    Function to find the difference between 2 Uint64S values.
// *********************************************************************************/
// Uint64S Diff_U64S(Uint64S val1, Uint64S val2, Uint8T *isNegative)
// {
//     Uint64S result;
//     u64 val164, val264, result64;
//
//     val164 = (val1.hi*0x100000000LL) + val1.lo;
//     val264 = (val2.hi*0x100000000LL) + val2.lo;
//     result64 = val164 - val264;
//     result.hi = (result64/0x100000000LL) & 0xFFFFFFFF;
//     result.lo = result64 & 0xFFFFFFFF;
//
//     if (isNegative) *isNegative = ((result64 > val164) ? (1) : (0));
//
//     return(result);
// }
//
// /********************************************************************************
//    Function to shift a Uint64S value to the right by rshift number of bits.
// *********************************************************************************/
// Uint64S RShift_U64S(Uint64S inVal, Uint8T rshift)
// {
//     Uint64S result;
//     u64 inVal64, result64;
//
//     inVal64 = (inVal.hi*0x100000000LL) + inVal.lo;
//     result64 = inVal64>>rshift;
//     result.hi = (result64/0x100000000LL) & 0xFFFFFFFF;
//     result.lo = result64 & 0xFFFFFFFF;
//
//     return(result);
// }
//
// /********************************************************************************
//    Function to multiply 2 Uint32T values to produce a Uint64S.
// *********************************************************************************/
// Uint64S Mult_U32_U32(Uint32T val1, Uint32T val2)
// {
//     Uint64S result;
//     u64 result64;
//
//     result64 = val1 * val2;
//     result.hi = (result64/0x100000000LL) & 0xFFFFFFFF;
//     result.lo = result64 & 0xFFFFFFFF;
//
//     return(result);
// }
//
// /********************************************************************************
//    Function used to implement the following ratio formula:
//
//
//         n1     n2                n1 * d2
//        ---- = ----   >>>   n2 = ---------
//         d1     d2                  d1
//
//
//    Proves useful when (n1 * d2) is larger than a Uint32T value.
//    Often used to convert tick counts from one frequency to another.
// *********************************************************************************/
// Uint32T RatioConvert_U32(Uint32T n1, Uint32T d1, Uint32T d2)
// {
//     u64 result64;
//
//     result64 = (n1 * d2)/d1;
//
//     return(result64 & 0xFFFFFFFF);
// }
//
// /********************************************************************************
//    Function used to implement the following formula:
//
//
//                  n1 * n2
//        result = ---------
//                    d1
//
//
//        round       Flag indicating if the decimal remainder as a result of the
//                   division is >= 0.5 ( >= 0.5 then round = 1; otherwise = 0).
//                   (If round == NULL, this value is ignored).
//        overflow    For results that require more than 32-bits, this contains the
//                   upper 32-bits of the result.
//                   (If overflow == NULL, this value is ignored).
//
//    Proves useful when the final result should be less than a 32-bit value.
//    Otherwise, the overflow output can be used to return the 64-bit extension.
// *********************************************************************************/
// Uint32T Mult_Mult_Div_U32(Uint32T n1, Uint32T n2, Uint32T d1,
//                           Uint8T *round, Uint32T *overflow)
// {
//     u64 mul64, result64, reminder64;
//
//     mul64 = n1 * n2;
//     result64 = mul64/d1;
//     reminder64 = mul64%d1;
//
//     if (round)      *round = (reminder64*2 >= d1) ? 1 : 0;
//     if (overflow)   *overflow = (result64/0x100000000LL) & 0xFFFFFFFF;
//
//     return(result64 & 0xFFFFFFFF);
// }
//
// /********************************************************************************
//    Used to determine the ratio of 2 Uint32T numbers and express them as a 32-bit
//    fraction. Any whole portion is also available:
//
//
//         n1         n2                     n1:0
//        ---- = -------------   >>>   n2 = -------
//         d1     0x100000000                 d1
//
//
//    Proves useful when finding the ratio of 2 frequencies to create a frequency
//    conversion ratio.
//
//   Return Value:
//    Uint32T  ratio of num/denom as a 32-bit fraction
// *********************************************************************************/
// Uint32T Ratio_U32_U32(Uint32T n1, Uint32T d1, Uint32T *whole32)
// {
//     u64 n164, result64;
//
//     n164 = (n1*0x100000000LL) + d1/2;  /* n1 is multiplied with 32 bit - d1/2 for eventual rounding */
//     result64 = n164/d1;
//     if (whole32)   *whole32 = (result64/0x100000000LL) & 0xFFFFFFFF;
//
//     return(result64 & 0xFFFFFFFF);
// }

#endif

// #ifdef CRITD_BASED_SEMA4
// static critd_t *create_critd(critd_type_t  type)
// {
//     critd_t        *mutex;
//     struct timeval tv;
//     static int     mutex_count = 0;
//     char           buffer[40];
//
//     T_DG(TRACE_GRP_OS_PORT, "os mapper called");
//     mutex = (critd_t *)VTSS_MALLOC(sizeof(critd_t));
//     if (mutex == NULL) {
//         return (NULL);
//     }
//     if (gettimeofday(&tv, NULL)) {
//         T_EG(TRACE_GRP_OS_PORT, "gettimeofday failed with errno: %d", errno);
//         VTSS_FREE(mutex);
//         mutex = NULL;
//         return mutex;
//     } else {
//         sprintf(buffer, "%ld:%ld:%d", tv.tv_sec, tv.tv_usec, (mutex_count++)%1000);
//         critd_init(mutex, buffer, VTSS_MODULE_ID_ZL_3034X, VTSS_TRACE_MODULE_ID, type);
//     }
//
//     T_DG(TRACE_GRP_OS_PORT, "os mapper(%s) finished", __FUNCTION__);
//     return(mutex);
// }
// #endif

osStatusT osMutexGive(OS_MUTEX_ID mutex)
{
    mutexInfoS* my_mutex = (mutexInfoS*) mutex;
    T_IG(TRACE_GRP_OS_PORT, "os mapper(%s) called, mutex %lx", __FUNCTION__, mutex);
    if (my_mutex == NULL) {
        T_EG(TRACE_GRP_OS_PORT, "Invalid mutex");
        return OS_ERROR;
    }
    if (my_mutex->inShutdown) {
        T_WG(TRACE_GRP_OS_PORT, "mutex: %p is shutdown", my_mutex);
        osTaskDelay(250);  /* This gives some time for shutdown to work correctly */
        return OS_ERROR;    /* Do not call any pthread functions if being deleted */
    }
    (void)vtss_recursive_mutex_unlock(&my_mutex->mutex);

    return OS_OK;
}

osStatusT osMutexTake(OS_MUTEX_ID mutex)
{
    mutexInfoS* my_mutex = (mutexInfoS*) mutex;
    T_IG(TRACE_GRP_OS_PORT, "os mapper(%s) called, mutex %lx", __FUNCTION__, mutex);
    if (my_mutex == NULL) {
        T_EG(TRACE_GRP_OS_PORT, "Invalid mutex");
        return OS_ERROR;
    }
    if (my_mutex->inShutdown) {
        T_WG(TRACE_GRP_OS_PORT, "mutex: %p is shutdown", my_mutex);
        osTaskDelay(250);  /* This gives some time for shutdown to work correctly */
        return OS_ERROR;    /* Do not call any pthread functions if being deleted */
    }
    vtss_recursive_mutex_lock(&my_mutex->mutex);

    return OS_OK;
}

osStatusT osMutexTakeT(OS_MUTEX_ID mutex, Sint32T timeout)
{
    mutexInfoS* my_mutex = (mutexInfoS*) mutex;
    T_IG(TRACE_GRP_OS_PORT, "take mutex %lx", mutex);
    if (timeout) {
        T_EG(TRACE_GRP_OS_PORT, "timeout %d", timeout);
    }
    if (my_mutex == NULL) {
        T_EG(TRACE_GRP_OS_PORT, "Invalid mutex");
        return OS_ERROR;
    }
    if (my_mutex->inShutdown) {
        T_WG(TRACE_GRP_OS_PORT, "mutex: %p is shutdown", my_mutex);
        osTaskDelay(250);  /* This gives some time for shutdown to work correctly */
        return OS_ERROR;    /* Do not call any pthread functions if being deleted */
    }
    vtss_recursive_mutex_lock(&my_mutex->mutex);
    return OS_OK;
}

osStatusT osMutexDelete(OS_MUTEX_ID mutex)
{
    mutexInfoS* my_mutex = (mutexInfoS*) mutex;
    T_IG(TRACE_GRP_OS_PORT, "delete mutex %p", my_mutex);
    if (my_mutex == NULL) {
        T_EG(TRACE_GRP_OS_PORT, "Invalid mutex");
        return OS_ERROR;
    }
    if (my_mutex->inShutdown) {
        T_WG(TRACE_GRP_OS_PORT, "mutex: %p is ALREADY shutdown", my_mutex);
        osTaskDelay(250);  /* This gives some time for shutdown to work correctly */
        return OS_ERROR;    /* Do not call any pthread functions if being deleted */
    }
    my_mutex->inShutdown = ZL303XX_TRUE;
    (void)vtss_recursive_mutex_unlock(&my_mutex->mutex);
    osTaskDelay(10);  /* This gives some time for waiting tasks to get the flag */
    T_IG(TRACE_GRP_OS_PORT, "os mapper(%s) called, mutex %lx", __FUNCTION__, mutex);
    VTSS_FREE((void *) my_mutex);
    return OS_OK;
}

OS_SEM_ID osMutexCreate(void)
{
    mutexInfoS *mutex;

    mutex = (mutexInfoS *)VTSS_MALLOC(sizeof(mutexInfoS));
    T_IG(TRACE_GRP_OS_PORT, "os mapper(%s) called, mutex %lx", __FUNCTION__, (OS_MUTEX_ID)mutex);

    if (mutex == NULL) {
        return (OS_MUTEX_INVALID);
    }
    vtss_recursive_mutex_init(&mutex->mutex);
    mutex->inShutdown = ZL303XX_FALSE;

    return (OS_SEM_ID)mutex;
}

// #if 0
// OS_SEM_ID osMutexCreate(void)
// {
//     critd_t*  mutex;
// /* Not finished - critd do not support this */
// /*
//           - Multiple calls to take the mutex may be nested within the same
//              calling task
// */
//
//     mutex = create_critd(CRITD_TYPE_MUTEX);
//     osMutexGive((OS_SEM_ID)mutex);
//     return (OS_SEM_ID)mutex;
// }
// #endif

osStatusT osSema4Take(OS_SEM_ID semId, Sint32T timeout)
{
    T_IG(TRACE_GRP_OS_PORT, "take semId %lx, timeout %d", semId, timeout);

#if defined VTSS_FLAG_BASED_SEMA4
    sema4InfoS* sem = (sema4InfoS*) semId;
    vtss_tick_count_t wakeup;
    osStatusT rc = OS_OK;
    if ((sema4InfoS*)semId == NULL) {
        T_EG(TRACE_GRP_OS_PORT, "Sema4 NULL error");
        return OS_ERROR;
    }
    if (sem->inShutdown) {
        T_WG(TRACE_GRP_OS_PORT, "sema4: %p is shutting down", sem);
        osTaskDelay(250);  /* This gives some time for shutdown to work correctly */
        return OS_ERROR;    /* Do not call any pthread functions if being deleted */
    }
    if (timeout == OsWaitForever) { // wait forever
        //wakeup = vtss_current_time() + VTSS_OS_MSEC2TICK(100000);
        //if (vtss_flag_timed_wait((vtss_flag_t *)semId, 0x1, VTSS_FLAG_WAITMODE_OR_CLR, wakeup) == 0) {
        if (vtss_flag_wait(&sem->flag, 0x1, VTSS_FLAG_WAITMODE_OR_CLR) == 0) {
            T_EG(TRACE_GRP_OS_PORT, "semaphore error");
            rc = OS_ERROR;
        }
    } else {
        wakeup = vtss_current_time() + VTSS_OS_MSEC2TICK(timeout);
        if (vtss_flag_timed_wait(&sem->flag, 0x1, VTSS_FLAG_WAITMODE_OR_CLR, wakeup) == 0) {
            rc = OS_ERROR;
        }
    }
    return rc;
#else
    vtss_sem_wait((vtss_sem_t *)semId);
    return OS_OK;
#endif /* CRITD_BASED_SEMA4 */
}

osStatusT osSema4Give(OS_SEM_ID semId)
{
#if defined VTSS_FLAG_BASED_SEMA4
    sema4InfoS* sem = (sema4InfoS*) semId;
    if ((sema4InfoS*)semId == NULL) {
        T_EG(TRACE_GRP_OS_PORT, "Sema4 NULL error");
        return OS_ERROR;
    }
    if (sem->inShutdown) {
        T_WG(TRACE_GRP_OS_PORT, "sema4: %p is shutting down", sem);
        osTaskDelay(250);  /* This gives some time for shutdown to work correctly */
        return OS_ERROR;    /* Do not call any pthread functions if being deleted */
    }
    T_IG(TRACE_GRP_OS_PORT, "give semId %p", sem);
    vtss_flag_setbits(&sem->flag, 0x1);
#else
    vtss_sem_post((vtss_sem_t *)semId);
#endif /* CRITD_BASED_SEMA4 */
    return OS_OK;
}

osStatusT osSema4Delete(OS_SEM_ID semId)
{
#ifdef VTSS_FLAG_BASED_SEMA4
    sema4InfoS* sem = (sema4InfoS*) semId;
    T_IG(TRACE_GRP_OS_PORT, "delete semId %p", sem);
    if (sem == NULL) {
        T_EG(TRACE_GRP_OS_PORT, "Invalid sema4 ID");
        return OS_ERROR;
    }
    if (sem->inShutdown) {
        T_WG(TRACE_GRP_OS_PORT, "sema4: %p is ALREADY shutdown", sem);
        osTaskDelay(250);  /* This gives some time for shutdown to work correctly */
        return OS_ERROR;    /* Do not call any pthread functions if being deleted */
    }
    sem->inShutdown = ZL303XX_TRUE;
    // if any threads are waiting for this semaphore then give a signal to these
    vtss_flag_setbits(&sem->flag, 0x1);
    osTaskDelay(10);  /* This gives some time for waiting tasks to get the flag */
    vtss_flag_destroy(&sem->flag);
    T_IG(TRACE_GRP_OS_PORT, "after flag_destroy");
    VTSS_FREE(sem);
    T_IG(TRACE_GRP_OS_PORT, "after free");
#elif defined(CRITD_BASED_SEMA4)
/* Not finished - critd do not support this */
    VTSS_FREE((critd_t *)semId);
#else
    vtss_sem_destroy((vtss_sem_t *)semId);
    VTSS_FREE((vtss_sem_t *)semId);
#endif /* VTSS_FLAG_BASED_SEMA4 */
    return OS_OK;
}

OS_SEM_ID osSema4CreateBinary(osMiscT initialState)
{
#ifdef CRITD_BASED_SEMA4
    critd_t* semId;

    semId = create_critd(CRITD_TYPE_SEMAPHORE);
    T_IG(TRACE_GRP_OS_PORT, "critd created, semId %p, initialState %lu", semId, initialState);
    osSema4Give((OS_SEM_ID)semId);
    if (!initialState)  /* Initially a critd semaphore counter is set to '1', it is cleared by osSema4Take() */
        osSema4Take((OS_SEM_ID)semId, 0);

    T_IG(TRACE_GRP_OS_PORT, "semId %p", semId);
#elif defined VTSS_FLAG_BASED_SEMA4
    sema4InfoS* semId;

    semId = (sema4InfoS *)VTSS_MALLOC(sizeof(*semId));
    T_IG(TRACE_GRP_OS_PORT, "flag based semaphore created, semId %p, initialState %lu", semId, initialState);
    if (semId == NULL) {
        return OS_SEM_INVALID;
    }
    vtss_flag_init(&semId->flag);
    semId->inShutdown = ZL303XX_FALSE;
    if (initialState == OS_SEM_FULL) {
        vtss_flag_setbits(&semId->flag, 0x1);
    }
#else
    vtss_sem_t *semId;

    T_IG(TRACE_GRP_OS_PORT, "os mapper called");
    semId = (vtss_sem_t *)VTSS_MALLOC(sizeof(*semId));
    if (semId == NULL) {
        T_EG(TRACE_GRP_OS_PORT, "Unable to create Semaphore");
        return OS_SEM_INVALID;
    }

    vtss_sem_init(semId, initialState);

#endif
    return (OS_SEM_ID)semId;
}


#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************************
 * osInterruptLock: Disables all interrupts                                     *
 *                                                                              *
 ********************************************************************************/
Uint32T osInterruptLock(void)
{
    T_IG(TRACE_GRP_OS_PORT, "os mapper called");
    return(0);
}

/********************************************************************************
 * osInterruptUnlock: Enables all interrupts                                    *
 *                                                                              *
 ********************************************************************************/
void osInterruptUnlock(Uint32T lockKey)
{
    T_IG(TRACE_GRP_OS_PORT, "os mapper called");
}

/*******************************************************************************
 * osSysTimestamp:                                                             *
 *                                                                             *
 *******************************************************************************/
Uint32T osSysTimestamp(void)
{
    T_IG(TRACE_GRP_OS_PORT, "os mapper called");
    return vtss_current_time();
}
#ifdef __cplusplus
}
#endif


/*******************************************************************************
 * osTickGet:                                                                  *
 *                                                                             *
 *******************************************************************************/
Uint32T osTickGet(void)
{
    T_IG(TRACE_GRP_OS_PORT, "os mapper called");
    return vtss_current_time();
}

osStatusT osTimeGet(Uint64S *pTime, Uint32T *pEpoch)
{
    mesa_timestamp_t ts;
    u64 tc;

    (void) pEpoch; /* reserved for future use */

    T_IG(TRACE_GRP_OS_PORT, "os mapper called");
    vtss_tod_gettimeofday(0, &ts, &tc);
    pTime->lo = ts.nanoseconds;
    pTime->hi = ts.seconds;

    return OS_OK;
}

typedef struct {
    vtss::Timer my_timer;
    void (*callout) (Sint32T, Sint32T);
    Sint32T p1;
    Sint32T p2;
} os_hw_timer_t;

/*
 * timer callout
 */
static void zl30380_os_time_my_timer_expired(vtss::Timer *timer)
{
    os_hw_timer_t *my_timer = (os_hw_timer_t *)timer->user_data;
    T_DG(TRACE_GRP_OS_PORT, "timer expired: callout %p, p1 0x%x, p2 %d", my_timer->callout, my_timer->p1, my_timer->p2);
    my_timer->callout(my_timer->p1, my_timer->p2);
}

#ifdef __cplusplus
extern "C" {
#endif

zlStatusE zl303xx_SetHWTimer(Uint32T rtSignal, timer_t *timerId, Sint32T osTimeDelayMs, void (*callout)(Sint32T, Sint32T), zl303xx_BooleanE periodic)
{
#if defined(VTSS_SW_OPTION_ZLS30387)
    os_hw_timer_t *my_hw_timer;
    T_IG(TRACE_GRP_OS_PORT, "os mapper called, rtSignal 0x%x, *timerId %p, delay %d, callout %p, periodic %d", rtSignal, *timerId, osTimeDelayMs, callout, periodic);

    if (!callout) {
        T_WG(TRACE_GRP_OS_PORT, "NULL callout" );
        return ZL303XX_ERROR;
    }
    if (!timerId) {
        T_WG(TRACE_GRP_OS_PORT, "NULL timerId" );
        return ZL303XX_ERROR;
    }
    if (*timerId != 0) {
        T_EG(TRACE_GRP_OS_PORT, "Trying to add an already existing timer" );
        return ZL303XX_ERROR;
    }
    my_hw_timer = VTSS_CREATE(os_hw_timer_t);
    if (my_hw_timer == NULL) {
        T_WG(TRACE_GRP_OS_PORT, "timer allocation error" );
        return ZL303XX_ERROR;
    }
    T_IG(TRACE_GRP_OS_PORT, "allocated my_hw_timer %p, size %d", my_hw_timer, (u32)sizeof(os_hw_timer_t));  // NOLINT

    // timer used for calling back, call the constructor after allocating the timer on the heap
    my_hw_timer->my_timer.set_repeat(periodic ? TRUE : FALSE);
    my_hw_timer->my_timer.set_period(vtss::milliseconds(osTimeDelayMs));     /* period */
    my_hw_timer->my_timer.callback    = zl30380_os_time_my_timer_expired;
    my_hw_timer->my_timer.modid       = VTSS_MODULE_ID_ZL_3034X_PDV;
    my_hw_timer->my_timer.user_data   = my_hw_timer;
    my_hw_timer->callout = callout;
    my_hw_timer->p1 = (intptr_t)my_hw_timer;
    my_hw_timer->p2 = (Sint32T) getpid();
    if (VTSS_RC_OK != vtss_timer_start(&my_hw_timer->my_timer)) {
        T_WG(TRACE_GRP_OS_PORT, "timer start error" );
    }
    *timerId = (timer_t) my_hw_timer;
    /* Warning clean-up. */
    (void) rtSignal;

    T_IG(TRACE_GRP_OS_PORT, "rtSignal 0x%x, *timerId %p, delay %d, callout %p, periodic %d", rtSignal, *timerId, osTimeDelayMs, callout, periodic);
    return ZL303XX_OK;
#else
    T_EG(TRACE_GRP_OS_PORT, "Not supported in this configuration" );
    return ZL303XX_ERROR;
#endif // defined(VTSS_SW_OPTION_ZLS30387)
}

zlStatusE zl303xx_DeleteHWTimer(Uint32T rtSignal, timer_t *timerId)
{
#if defined(VTSS_SW_OPTION_ZLS30387)
    T_IG(TRACE_GRP_OS_PORT, "rtSignal 0x%x, *timerId %p", rtSignal, *timerId);
    os_hw_timer_t *t = (os_hw_timer_t *)*timerId;
    if (t) {
        // make the last callout before deleting the timer
        t->callout(t->p1, t->p1);
        if (VTSS_RC_OK != vtss_timer_cancel(&t->my_timer)) {
            T_WG(TRACE_GRP_OS_PORT, "timer cancel error" );
        }
        vtss_destroy(t);
        *timerId = NULL;
    } else {
        T_EG(TRACE_GRP_OS_PORT, "Trying to delete a non existent timer" );
    }
    return ZL303XX_OK;
#else
    T_EG(TRACE_GRP_OS_PORT, "Not supported in this configuration" );
    return ZL303XX_ERROR;
#endif // defined(VTSS_SW_OPTION_ZLS30387)
}

#ifdef __cplusplus
}
#endif

/*******************************************************************************
 * osMsgQCreate: Creates a new message queue                                   *
 *                                                                             *
 *******************************************************************************/
OS_MSG_Q_ID osMsgQCreate(Sint32T maxMsgs, Sint32T maxMsgLength)
{
    msgQInfoS *msgQInfo = NULL;

    msgQInfo = (msgQInfoS *)VTSS_CALLOC(1, sizeof(msgQInfoS));
    if (msgQInfo == NULL)
    {
        T_EG(TRACE_GRP_OS_PORT, "osMsgQCreate: Message queue structure memory allocation error");
        return OS_MSG_Q_INVALID;
    }

    msgQInfo->msgBufs = (char  *)VTSS_CALLOC(maxMsgs, maxMsgLength);
    if (msgQInfo->msgBufs == NULL)
    {
        T_EG(TRACE_GRP_OS_PORT, "osMsgQCreate: Message queue memory allocation error");
        VTSS_FREE(msgQInfo);
        return OS_MSG_Q_INVALID;
    }

    vtss_mutex_init(&msgQInfo->lock);
    vtss_cond_init(&msgQInfo->condv, &msgQInfo->lock); // TBD if it should be an other mutex

    msgQInfo->inPtr = 0;
    msgQInfo->outPtr = 0;
    msgQInfo->numPendingMsgs = 0;
    msgQInfo->maxMsgSize = maxMsgLength;
    msgQInfo->numBufs = maxMsgs;
    msgQInfo->inShutdown = ZL303XX_FALSE;
    //T_WG(TRACE_GRP_OS_PORT, "Message queue msgQInfo %p, msgBufs %p, maxMsgSize %d, numBufs %d",  msgQInfo, msgQInfo->msgBufs, msgQInfo->maxMsgSize, msgQInfo->numBufs);

    return (OS_MSG_Q_ID)msgQInfo;
}

/*******************************************************************************
 * osMsgQDelete: Delete a message queue                                        *
 *                                                                             *
 *******************************************************************************/
osStatusT osMsgQDelete(OS_MSG_Q_ID msgQId)
{
    msgQInfoS *msgQInfo = (msgQInfoS *)msgQId;

    if (msgQInfo == NULL) {
        T_EG(TRACE_GRP_OS_PORT, "osMsgQDelete: Invalid message queue ID");
        return OS_ERROR;
    }

    if (msgQInfo->inShutdown) {
        T_EG(TRACE_GRP_OS_PORT, "msgQue: %p is ALREADY shutdown", msgQInfo);
        osTaskDelay(250);  /* This gives some time for shutdown to work correctly */
        return OS_ERROR;    /* Do not call any pthread functions if being deleted */
    }
    msgQInfo->inShutdown = ZL303XX_TRUE;
    //T_WG(TRACE_GRP_OS_PORT, "Message queue msgBufs %p, maxMsgSize %d, numBufs %d",  msgQInfo->msgBufs, msgQInfo->maxMsgSize, msgQInfo->numBufs);
    vtss_cond_destroy(&msgQInfo->condv);
    vtss_mutex_destroy(&msgQInfo->lock);

    VTSS_FREE(msgQInfo->msgBufs);
    VTSS_FREE(msgQInfo);

    return OS_OK;
}

/* Static helper function */
static void copyMsgToQ(msgQInfoS *msgQInfo, Sint8T *buffer, Uint32T nBytes)
{
   /* Copy nBytes from buffer to the message queue */
   char *msgBufToUse = msgQInfo->msgBufs + (msgQInfo->maxMsgSize * msgQInfo->inPtr);

   memcpy(msgBufToUse, buffer, nBytes);
   msgQInfo->numPendingMsgs++;
   msgQInfo->inPtr++;
    if (msgQInfo->inPtr >= msgQInfo->numBufs)
   {  /* Wrap around the end of the queue */
        if (msgQInfo->inPtr > msgQInfo->numBufs)
            T_EG(TRACE_GRP_OS_PORT, "copyMsgToQ: msgQInfo->inPtr=%d > msgQInfo->numBufs=%d ; msgQInfo->numPendingMsgs=%d. Possible OS mutex issue?\n",
                     msgQInfo->inPtr, msgQInfo->numBufs, msgQInfo->numPendingMsgs);
      msgQInfo->inPtr = 0;
   }
}

/********************************************************************************
 * osMsgQSend: Enqueues the messages                                            *
 *                                                                              *
 ********************************************************************************/
osStatusT osMsgQSend(OS_MSG_Q_ID msgQId, Sint8T *buffer, Uint32T nBytes, Sint32T timeout)
{
    msgQInfoS *msgQInfo = (msgQInfoS *)msgQId;
    int msgSent = 0;
    vtss_bool_t retVal = true;

    if (msgQInfo == NULL)
    {
        T_EG(TRACE_GRP_OS_PORT, "osMsgQSend: Mutex NULL error");
        return OS_ERROR;
    }

    if (msgQInfo->inShutdown) {
        T_EG(TRACE_GRP_OS_PORT, "msqQueue: %p is shutting down", msgQInfo);
        osTaskDelay(250);  /* This gives some time for shutdown to work correctly */
        return OS_ERROR;    /* Do not call any pthread functions if being deleted */
    }
    /* Lock access to the message queue variables */
    if (!vtss_mutex_lock(&msgQInfo->lock))
    {
        T_EG(TRACE_GRP_OS_PORT, "osMsgQSend: Error locking mutex");
        return OS_ERROR;
    }
    /* Check for valid size */
    if (nBytes > msgQInfo->maxMsgSize)
    {
        T_EG(TRACE_GRP_OS_PORT, "osMsgQSend: Invalid message size=%d exceeded max=%d", nBytes, msgQInfo->maxMsgSize);
        vtss_mutex_unlock(&msgQInfo->lock);
        return OS_ERROR;
    }

    if (timeout == (Sint32T)OS_WAIT_FOREVER)
    {
        while (msgQInfo->numPendingMsgs == msgQInfo->numBufs)
        {  /* Buffers are all full. Wait until something changes */
            retVal = vtss_cond_wait(&msgQInfo->condv);
        }
        if (retVal && msgQInfo->numPendingMsgs < msgQInfo->numBufs)
        {
            /* There is now space so copy the message */
            copyMsgToQ(msgQInfo, buffer, nBytes);
            msgSent = 1;
        }
    }
    else if (timeout == (Sint32T)OS_NO_WAIT)
    {
        if (msgQInfo->numPendingMsgs < msgQInfo->numBufs)
        {
            /* There is space so copy the message */
            copyMsgToQ(msgQInfo, buffer, nBytes);
            msgSent = 1;
        }
        else
        {
            /* No space, but continue anyway */
            msgSent = 0;
        }
    }
    else  /* Specified timeout value */
    {
        if (msgQInfo->numPendingMsgs < msgQInfo->numBufs)
        {
            /* There is space so copy the message straightaway */
            copyMsgToQ(msgQInfo, buffer, nBytes);
            msgSent = 1;
        }
        else
        {
            /* No space, wait for the timeout to see if it becomes available */
            vtss_tick_count_t endtime;

            /* Get absolute time now */
            endtime = vtss_current_time() + timeout;

            /* repeat this operation until either there is space or the timeout occurs */
            while ((msgQInfo->numPendingMsgs >= msgQInfo->numBufs) && (retVal))
            {
                retVal = vtss_cond_timed_wait(&msgQInfo->condv, endtime);
            }
            if (retVal && msgQInfo->numPendingMsgs < msgQInfo->numBufs)
            {
                /* There is now space so copy the message */
                copyMsgToQ(msgQInfo, buffer, nBytes);
                msgSent = 1;
            }
            else
            {  /* Could not send */
                msgSent = 0;
            }
        }
    }

    if (msgSent)
    {  /* Inform pending receivers that there is now a message */
        (void)vtss_cond_broadcast(&msgQInfo->condv);
    }

    /* Unlock access to the queue */
    vtss_mutex_unlock(&msgQInfo->lock);


    T_IG(TRACE_GRP_OS_PORT, "Message queue msgQInfo %p, nBytes %d, msgSent %d, timeout %d",  msgQInfo, nBytes, msgSent, timeout);
    if (msgSent == 1)
    {
        /* Inform pending receivers that there is now a message */
        (void)vtss_cond_broadcast(&msgQInfo->condv);

        return OS_OK;
    }
    else
    {
        return OS_ERROR;
        /* Note that in the case where NO_WAIT was specified or the send timed out then
           if the send did not succeed there is no way of determining whether this is an
           error or not. */
    }
}

/* Static helper function */
static void copyMsgFromQ(msgQInfoS *msgQInfo, Sint8T *buffer, Uint32T nBytes)
{
   /* Copy nBytes from message queue to the buffer */
   char *msgBufToUse = msgQInfo->msgBufs + (msgQInfo->maxMsgSize * msgQInfo->outPtr);

   memcpy(buffer, msgBufToUse, nBytes);
   msgQInfo->numPendingMsgs--;
   msgQInfo->outPtr++;
    if (msgQInfo->outPtr >= msgQInfo->numBufs)
   {  /* Wrap around the end of the queue */
        if (msgQInfo->outPtr > msgQInfo->numBufs)
            T_EG(TRACE_GRP_OS_PORT, "copyMsgFromQ: msgQInfo->outPtr=%d > msgQInfo->numBufs=%d ; msgQInfo->numPendingMsgs=%d. Possible OS mutex issue?",
                     msgQInfo->outPtr, msgQInfo->numBufs, msgQInfo->numPendingMsgs);
      msgQInfo->outPtr = 0;
   }
}

/*******************************************************************************
 * osMsgQReceive: Dequeues message                                             *
 *                                                                             *
 *******************************************************************************/
osStatusT osMsgQReceive(OS_MSG_Q_ID msgQId, Sint8T *buffer, Uint32T maxNBytes, Sint32T timeout)
{
    msgQInfoS *msgQInfo = (msgQInfoS *)msgQId;
    int nBytesToCopy;
    int msgRecvd = 0;
    vtss_bool_t retVal = true;

    //T_WG(TRACE_GRP_OS_PORT, "msgQId %lx, maxNBytes %d, timeout %d", msgQId, maxNBytes, timeout);
    if (msgQInfo == NULL)
    {
        T_EG(TRACE_GRP_OS_PORT, "osMsgQReceive: Mutex NULL error");
        return OS_ERROR;
    }

    if (msgQInfo->inShutdown) {
        T_EG(TRACE_GRP_OS_PORT, "msqQueue: %p is shutting down", msgQInfo);
        osTaskDelay(250);  /* This gives some time for shutdown to work correctly */
        return OS_ERROR;    /* Do not call any pthread functions if being deleted */
    }
    /* Lock access to the message queue variables */
    if (!vtss_mutex_lock(&msgQInfo->lock))
    {
        T_EG(TRACE_GRP_OS_PORT, "osMsgQReceive: Error locking mutex");
        return OS_ERROR;
    }

    /* Calculate size of message to copy */
    if (maxNBytes > msgQInfo->maxMsgSize)
    {
        nBytesToCopy = msgQInfo->maxMsgSize;
    }
    else
    {
        nBytesToCopy = maxNBytes;
    }

    if (timeout == (Sint32T)OS_WAIT_FOREVER)
    {
        while (msgQInfo->numPendingMsgs == 0)
        {  /* Queue is empty. Wait until something changes */
            retVal = vtss_cond_wait(&msgQInfo->condv);
            T_DG(TRACE_GRP_OS_PORT, "retVal %d", retVal);
        }
        if (retVal && msgQInfo->numPendingMsgs > 0)
        {
            /* A message is now available so copy it */
            copyMsgFromQ(msgQInfo, buffer, (Uint32T)nBytesToCopy);
            msgRecvd = 1;
        }
    }
    else if (timeout == (Sint32T)OS_NO_WAIT)
    {
        if (msgQInfo->numPendingMsgs > 0)
        {
            /* A message is  available so copy it */
            copyMsgFromQ(msgQInfo, buffer, (Uint32T)nBytesToCopy);
            msgRecvd = 1;
        }
        else
        {
            /* No message, but continue anyway */
            msgRecvd = 0;
        }
    }
    else  /* Specified timeout value */
    {
        if (msgQInfo->numPendingMsgs > 0)
        {
            /* A message is available so copy it straightaway */
            copyMsgFromQ(msgQInfo, buffer, (Uint32T)nBytesToCopy);
            msgRecvd = 1;
        }
        else
        {
            /* No message, wait for the timeout to see if one becomes available */
            vtss_tick_count_t endtime;

            /* Get absolute time now */
            endtime = vtss_current_time() + timeout;

            /* repeat this operation until either there is a message or the timeout occurs */
            while ((msgQInfo->numPendingMsgs == 0) && (retVal))
            {
                retVal = vtss_cond_timed_wait(&msgQInfo->condv, endtime);
            }
            if (retVal && msgQInfo->numPendingMsgs > 0)
            {
                /* A message is available so copy it */
                copyMsgFromQ(msgQInfo, buffer, (Uint32T)nBytesToCopy);
                msgRecvd = 1;
            }
            else
            {  /* No message */
                if (!msgQInfo->numPendingMsgs) {
                    retVal = true;
                }
                msgRecvd = 0;
            }
        }
    }

    /* Unlock access to the queue */
    vtss_mutex_unlock(&msgQInfo->lock);

    if (msgRecvd)
    {  /* Inform pending senders that there is now space in the queue */
        (void)vtss_cond_broadcast(&msgQInfo->condv);
    }


    T_IG(TRACE_GRP_OS_PORT, "Message queue msgQInfo %p, msgRecvd %d, nBytesToCopy %d, timeout %d",  msgQInfo, msgRecvd, nBytesToCopy, timeout);
    if (msgRecvd == 1)
    {
        return nBytesToCopy;

    }
    else if (retVal && !msgRecvd)
    {
        /* No message received. */
        return 0;
    } else {
        return OS_ERROR;
        /* Note that in the case where NO_WAIT was specified or the receive timed out then
           if the receive did not succeed there is no way of determining whether this is an
           error or not. */
    }
}

/*
  Function Name: osMsgQNumMsgs
^M
  Details:
   Returns the number of messages currently queued to a specified message queue

  Parameters:
        msgQId - message queue to examine****
^M
^M
^M
  Return Value:  The number of messages queued, or ERROR.^M
 */

Sint32T osMsgQNumMsgs(OS_MSG_Q_ID msgQId)
{
    Sint32T numPendingMsgs;
    msgQInfoS *msgQInfo = (msgQInfoS *)msgQId;

    if (NULL == msgQInfo)
    {
        T_EG(TRACE_GRP_OS_PORT, "osMsgQNumMsgs: NULL message queue ID\n");
        return OS_ERROR;
    }
    if (msgQInfo->inShutdown) {
        return OS_ERROR;
    }
    if (!vtss_mutex_lock(&msgQInfo->lock))
    {
        T_EG(TRACE_GRP_OS_PORT, "osMsgQNumMsgs: Could not lock mutex\n");
        return OS_ERROR;
    }
    numPendingMsgs = msgQInfo->numPendingMsgs;

    vtss_mutex_unlock(&msgQInfo->lock);

    return numPendingMsgs;
}


/*******************************************************************************
*  The function sends "trace" msgs to the msgQ for customer logging.
 *******************************************************************************/

Sint32T osLogMsg(const char *fmt, UnativeT arg1, UnativeT arg2, UnativeT arg3, UnativeT arg4, UnativeT arg5, UnativeT arg6)
{
    T_DG(TRACE_GRP_OS_PORT, "os mapper called");
    return zl303xx_LogToMsgQ(LOG_FMT_STR, fmt, (UnativeT)arg1, (UnativeT)arg2, (UnativeT)arg3, (UnativeT)arg4, (UnativeT)arg5, (UnativeT)arg6);
}

/*******************************************************************************
 * osTickRateGet: Rate get                                                     *
 * Return number of ticks in one second                                        *
 *******************************************************************************/
Uint32T osTickRateGet(void)
{
    Uint32T val = VTSS_OS_MSEC2TICK(1000);
    T_DG(TRACE_GRP_OS_PORT, "os mapper, rate %u", val);
    return val;
}

/*
 * Convert task priorities to WebStaX priorities, the priority numbers used in the call from ZL code are Linux priorities,
 *   i.e. 99 is highest, 0 is lowest
 */
static vtss_thread_prio_t task_priority_map(Sint32T priority)
{

    if (priority >= 98) return VTSS_THREAD_PRIO_HIGHEST;
    if (priority >= 96) return VTSS_THREAD_PRIO_HIGHER;
    if (priority >= 89) return VTSS_THREAD_PRIO_HIGH;
    if (priority >= 88) return VTSS_THREAD_PRIO_ABOVE_NORMAL;
    if (priority >= 80) return VTSS_THREAD_PRIO_DEFAULT;
    return VTSS_THREAD_PRIO_BELOW_NORMAL;
}

/*******************************************************************************
 * osTaskSpawn: Creates a new task                                             *
 *                                                                             *
 *******************************************************************************/
OS_TASK_ID osTaskSpawn(const char *name, Sint32T priority, Sint32T options, Sint32T stackSize, OS_TASK_FUNC_PTR entryPt, Sint32T taskArg)
{
    OS_TASK_ID task_id;
    i32        tmp_id;

    //T_WG(TRACE_GRP_OS_PORT, "os mapper(%s) called, name %s, pri %d", __FUNCTION__, name, priority);
    //T_WG(TRACE_GRP_OS_PORT, "WebStaX pri %d", priority);
    if (task_create((char *)name,
                    task_priority_map(priority),
                    options,
                    stackSize,
                    entryPt,
                    taskArg,
                    &tmp_id,
                    &task_id) != VTSS_RC_OK) {
        task_id = OS_TASK_INVALID;
    }
    T_IG(TRACE_GRP_OS_PORT, "created thread, taskId %lx", task_id);
    return task_id;
}

/*******************************************************************************
 * osTaskDelay:                                                                *
 *                                                                             *
 *******************************************************************************/
osStatusT osTaskDelay(Sint32T millis)
{
    T_DG(TRACE_GRP_OS_PORT, "os mapper called %d", millis);
    VTSS_OS_MSLEEP(millis + 1);
    return OS_OK;
}

/*******************************************************************************
 * osTaskDelete:                                                                *
 *                                                                             *
 *******************************************************************************/
osStatusT osTaskDelete (OS_TASK_ID tid)
{
    vtss_thread_info_t info;

    if (!vtss_thread_info_get((vtss_handle_t)tid, &info)) {
        return OS_ERROR;
    }

    T_IG(TRACE_GRP_OS_PORT, "os mapper called, tid = %d", info.tid);

    if (task_delete((vtss_handle_t)tid) != VTSS_RC_OK)
        return OS_ERROR;
    else
        return OS_OK;
}

#ifdef __cplusplus
extern "C" {
#endif
void *osCalloc(size_t NumberOfElements, size_t size)
{
    void *p;
    T_IG(TRACE_GRP_OS_PORT, "os mapper called");
    p = VTSS_CALLOC(NumberOfElements, size);
    //T_WG(TRACE_GRP_OS_PORT, "NumberOfElements %d, size %d, p %p", NumberOfElements, size, p);
    return p;
}

void osFree(void *ptrToMem)
{
    T_IG(TRACE_GRP_OS_PORT, "os mapper called");
    //T_WG(TRACE_GRP_OS_PORT, "ptrToMem %p", ptrToMem);
    VTSS_FREE(ptrToMem);
}
#ifdef __cplusplus
}
#endif

// cpuStatusE cpuSpiWrite(void *par, Uint32T regAddr, Uint8T *dataBuf, Uint16T bufLen)
// {
//     T_DG(TRACE_GRP_OS_PORT, "cpuSpiWrite  regAddr %X  bufLen %u  data %X-%X\n", regAddr, bufLen, dataBuf[0], dataBuf[1]);
// //printf("cpuSpiWrite  regAddr %X  bufLen %u  data %X-%X\n", regAddr, bufLen, dataBuf[0], dataBuf[1]);
//     zl_3036x_spi_write(regAddr, dataBuf, bufLen);
//
//     return(CPU_OK);
// }
//
// cpuStatusE cpuSpiRead(void *par, Uint32T regAddr, Uint8T *dataBuf, Uint16T bufLen)
// {
//     zl_3036x_spi_read(regAddr, dataBuf, bufLen);
//     T_DG(TRACE_GRP_OS_PORT, "cpuSpiRead  regAddr %X  bufLen %u  data %X-%X\n", regAddr, bufLen, dataBuf[0], dataBuf[1]);
// //printf("cpuSpiRead  regAddr %X  bufLen %u  data %X-%X\n", regAddr, bufLen, dataBuf[0], dataBuf[1]);
//
//     return(CPU_OK);
// }

