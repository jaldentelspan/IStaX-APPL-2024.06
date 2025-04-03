

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Various macros for use anywhere in the API.
*
*******************************************************************************/

#ifndef ZL303XX_MACROS_H_
#define ZL303XX_MACROS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Error.h"

/* For offsetof used in ZL303XX_CONTAINER_GET */
/* See other options at https://en.wikipedia.org/wiki/Offsetof */
#include <stddef.h>


/*****************   DEFINES   ************************************************/

/* Some Operating Systems may not have these simple function defined.*/
#ifndef min
    #define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

#ifndef max
    #define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

/* Gets the number of entries in a statically declared array. */
#define ZL303XX_ARRAY_SIZE(arr)  (sizeof(arr) / sizeof(*(arr)))

/* Macro to return a pointer to a structure given a pointer to a member within
 * that structure. Example usage (with a linked list):
 *
 * struct
 * {
 *    Uint32T data1;
 *    Uint32T data2;
 *    zl303xx_ListS listEntry;
 * } zl303xx_SomeStructS;
 *
 * Uint32T doWork(zl303xx_ListS *node)
 * {
 *    zl303xx_SomeStructS *worker = ZL303XX_CONTAINER_GET(node, zl303xx_SomeStructS, listEntry);
 *
 *    return worker->data1 + worker->data2;
 * }
 * 
 * Wrapping this macro in another define can make code more readable:
 *    #define LIST_TO_WORKER(ptr)  ZL303XX_CONTAINER_GET(ptr, zl303xx_SomeStructS, listEntry)
 */
#define ZL303XX_CONTAINER_GET(ptr, type, member) \
   ((type *)(void *)((char *)ptr - offsetof(type, member)))

/* Bit manipulation macros */
#define ZL303XX_BIT_SET(var, bit)     ((var) |= (1 << (bit)))
#define ZL303XX_BIT_CLEAR(var, bit)   ((var) &= ~(1 << (bit)))
#define ZL303XX_BIT_TEST(var, bit)    ((var) & (1 << (bit)))

/***** Macros for checking parameters *****/
/* Macro to check for a non-null pointer. Use of the comma operator is
   intentional here */
#ifndef ZL303XX_CHECK_POINTER
#define ZL303XX_CHECK_POINTER(ptr) \
   ( ((ptr) == NULL) ?  \
     (ZL303XX_ERROR_NOTIFY("Invalid pointer: "#ptr),ZL303XX_INVALID_POINTER) : \
     ZL303XX_OK    \
   )
#endif
#ifndef ZL303XX_CHECK_POINTERS
#define ZL303XX_CHECK_POINTERS(ptr1, ptr2) \
   ( ((ptr1 == NULL) || (ptr2 == NULL)) ?  \
     (ZL303XX_ERROR_NOTIFY("Invalid pointers"),ZL303XX_INVALID_POINTER) : \
     ZL303XX_OK    \
   )
#endif

/* Macro to check that a value is within an acceptable range (MIN & MAX are OK).
   Use of the comma operator is intentional here */
#ifndef ZL303XX_CHECK_RANGE
#define ZL303XX_CHECK_RANGE(val, min, max)    \
   ( ((val < min) || (val > max)) ?    \
     (ZL303XX_ERROR_NOTIFY("Value out of range:" #val),ZL303XX_PARAMETER_INVALID) : \
     ZL303XX_OK    \
   )
#endif

#if defined OS_VXWORKS || defined OS_LINUX

    extern void *taskMonConfig;

    extern zlStatusE OS_TASKMON_CONFIG(SINT_T, void *);
    extern zlStatusE OS_TASKMON_DELETE(SINT_T, void *);

    extern zlStatusE OS_TASKMON_CHECK_IN(SINT_T, void *);
    extern zlStatusE OS_TASKMON_CHECK_OUT(SINT_T, void *);


    /* The taskId of -1 below will force the decision into the porting layer. See zl303xx_OsTaskMon.c */
    #define OS_TASKMON_INIT(taskId) \
    OS_TASKMON_CONFIG(taskId, taskMonConfig)

    #define OS_TASKMON_FUNC_START() \
    OS_TASKMON_CHECK_IN(-1, taskMonConfig)

    #define OS_TASKMON_FUNC_END() \
    OS_TASKMON_CHECK_OUT(-1, taskMonConfig)

    #define OS_TASKMON_END(taskId) \
    OS_TASKMON_DELETE(taskId, taskMonConfig) 
#else
#ifdef SOCPG_PORTING
#ifdef OS_FREERTOS

#if !defined OS_TASKMON_UTILS
   /* These EMPTY macros are called at the start and end of every task loop.
   Override these empty macros by defining OS_TASKMON_UTILS and adding code in zl303xx_OsTaskMon.c */
   #define OS_TASKMON_CONFIG(taskId, taskMonConfig)
   #define OS_TASKMON_DELETE(taskId, taskMonConfig)

   #define OS_TASKMON_CHECK_IN(taskId, taskMonConfig)
   #define OS_TASKMON_CHECK_OUT(taskId, taskMonConfig)

 #else
   extern zlStatusE OS_TASKMON_CONFIG(SINT_T, void *);
   extern zlStatusE OS_TASKMON_DELETE(SINT_T, void *);

   extern zlStatusE OS_TASKMON_CHECK_IN(SINT_T, void *);
   extern zlStatusE OS_TASKMON_CHECK_OUT(SINT_T, void *);
 #endif

//fixme - ml fill in below
/* The taskId of -1 below will force the decision into the porting layer. See zl303xx_OsTaskMon.c */
#define OS_TASKMON_INIT(taskId) \
OS_TASKMON_CONFIG(taskId, taskMonConfig)   /* See zl303xx_OsTaskMon.c */

#define OS_TASKMON_FUNC_START() \
OS_TASKMON_CHECK_IN(-1, taskMonConfig)

#define OS_TASKMON_FUNC_END() \
OS_TASKMON_CHECK_OUT(-1, taskMonConfig)

#define OS_TASKMON_END(taskId) \
OS_TASKMON_DELETE(taskId, taskMonConfig)

#endif
#else

    #define OS_TASKMON_INIT(taskId) \
    #warning OS_TASKMON_INIT. Unknown OS. 

    #define OS_TASKMON_CHECK_IN() \
    #warning OS_TASKMON_CHECK_IN. Unknown OS. 

    #define OS_TASKMON_CHECK_OUT() \
    #warning OS_TASKMON_CHECK_OUT. Unknown OS.

    #define OS_TASKMON_END(taskId) \
    #warning OS_TASKMON_END. Unknown OS.
#endif
#endif


/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
