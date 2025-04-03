/*******************************************************************************
*
*  $Id: 4899e900e59c66d2345501933f3c6287f7ad4f29
*
*  Copyright 2006-2020 Microchip/Microsemi Semiconductor Limited.
*  All rights reserved.
*
*  Module Description:
*     Functions and registers for flash porting when not included with full
*     API package.
*
*******************************************************************************/

#ifndef EXAMPLE_FLASH_PORTING_H_
#define EXAMPLE_FLASH_PORTING_H_

#ifdef __cplusplus
extern "C" {
#endif


/* define a minumum set for porting */
#include <stdint.h>
#include <stdlib.h>  /* calloc/free */

/*****************   Types porting       **********************************************/
//
// Removed definitions (typedef+defines) below.
// They are already defined via zl303xx_DeviceSpec.h, and including them
// in this file will cause re-defintions of symbols and subsequent compile error.
// The remaining symbols/definitions in this file are mandatory in order to support
// flashing of firmware wihtin DPLL and thus not removed.
//

//typedef uint8_t Uint8T;
//typedef int8_t Sint8T;
//
//typedef uint16_t Uint16T;
//typedef int16_t Sint16T;
//
//typedef uint32_t Uint32T;
//typedef int32_t Sint32T;
//
//typedef Uint8T zl303xx_BooleanE;
//#define ZL303XX_FALSE 0
//#define ZL303XX_TRUE 1


/*****************   OS porting           **********************************************/
//#define OS_SETUP()      /* Nothing required */
//#define OS_CLEANUP()    /* Nothing required */
//
//#define OS_CALLOC(num, sz) calloc(num, sz)
//#define OS_FREE(ptr) free(ptr)
//#define OS_MEMSET(ptr, val, sz) memset(ptr, val, sz)
//
//#define OS_TASK_DELAY(msec) osTaskDelay(msec)
//#define OS_TICK_GET() osTickGet()
//#define OS_TICK_RATE_GET() osTickRateGet()

/**
    Returns the number of ticks contained in one second as returned by osTickGet().
    See osTickGet() for example.
*/
Uint32T osTickRateGet(void);

/**
   Returns the number of ticks elapsed in the system from an arbitrary start point (epoch).
   Used with osTickRateGet() to calculate elapsed time between two points.
   Must be a monotonically increasing number (no backwards jumps) but it is allowed to
   rollover the Uint32T boundary (i.e. go from 2^32-1 to 0)

Example usage:
   Uint32T startTick = osTickGet();
   ...
   Uint32T endTick = osTickGet();
   Uint32T elapsedTick = (endTick - startTick);
   Uint32T elapsedSec = elapsedTick / osTickRateGet();
*/
Uint32T osTickGet(void);

/**
    Blocks the caller for at least given `msec` millisec amount of time
    (e.g. with a sleep).

  Parameters:
   [in] msec     Number of milliseconds (10^-3 sec) to sleep.

\returns
    0 for success,
    other for error
*/
Sint32T osTaskDelay(Sint32T msec);


/*****************   Trace porting        **********************************************/
#define ZL303XX_NEWLINE "\n"

/* Identifier for modules used for trace statements */
typedef enum {
    ZL303XX_MOD_ID_RDWR          = 0,  /* low level registers */
    ZL303XX_MOD_ID_PLL           = 1,  /* high level driver */
    ZL303XX_MOD_ID_SYSINT        = 2,  /* system and OS */
    ZL303XX_NUM_API_MODULE_IDS   /* Must be the last element */
} zl303xx_ModuleIdE;

/* A macro wrapper to output module trace output at various levels (1-5 where 5 is more information) */
/* Example below outputs to stderr */
#define ZL303XX_TRACE(modId, level, fmt, ...) do { \
    if (level <= zl303xx_TraceGetLevel(modId)) { \
        printf("[%09u, %u:%u] " fmt ZL303XX_NEWLINE, OS_TICK_GET(), modId, level, ##__VA_ARGS__); \
    } } while(0)

/* A macro wrapper to output trace output that cannot be disabled (for important messages) */
/* Example below outputs to stdout */
//#define ZL303XX_TRACE_ALWAYS(fmt, a0, a1, a2, a3, a4, a5) fprintf(stdout, "[%09u] " fmt ZL303XX_NEWLINE, OS_TICK_GET(), a0, a1, a2, a3, a4, a5)
#define ZL303XX_TRACE_ALWAYS(fmt, ...) printf("[%09u] " fmt ZL303XX_NEWLINE, OS_TICK_GET(), ##__VA_ARGS__)

/* Routines to control module trace output level (1-5 where 5 is more information) */
void zl303xx_TraceSetLevel(Uint16T modId, Uint8T level);
Uint8T zl303xx_TraceGetLevel(Uint16T modId);

/* Routines to init and close trace */
/* void zl303xx_TraceInit(FILE *logFd); */
/* void zl303xx_TraceClose(FILE *logFd); */
#define zl303xx_TraceInit(logFd)     /* Nothing required, could use for configuring trace to a file */
#define zl303xx_TraceClose(logFd)    /* Nothing required */

/*****************   zlStatusE porting    **********************************************/
#include "zl303xx_Error.h"

//#define ZL303XX_CHECK_POINTER(ptr) (((ptr) == NULL) ? ZL303XX_TRACE_ALWAYS("Invalid pointer %p", (void*)(ptr),0,0,0,0,0), ZL303XX_INVALID_POINTER : ZL303XX_OK)
#define ZL303XX_CHECK_POINTERS(ptr1, ptr2) (((ptr1) == NULL || (ptr2) == NULL) ? ZL303XX_TRACE_ALWAYS("Invalid pointer %p or %p", (void*)(ptr1), (void*)(ptr2),0,0,0,0),ZL303XX_INVALID_POINTER : ZL303XX_OK)


/*****************   Device porting       **********************************************/
#define DEVICE_PAGE_SEL_REG_ADDR     0x7F
#define DEVICE_PAGE_SEL_REG_ADDR_73X 0x7F
#define DEVICE_PAGE_SEL_REG_ADDR_77X 0x7F


/*****************   Device interfaces    **********************************************/
/**
    Device register read interface.

  Parameters:
   [in]  hwparams    Pointer to user specified device.
   [in]  arg         Pointer to user specified options.
   [in]  address     Virtual register address to access (i.e. combined page & offset for debugging)
   [in]  page        Page containing register to access
   [in]  offset      Offset within page containing register to access
   [in]  size        Size of register to access (1, 2, or 4)
   [out] value       Pointer to 32-bit integer to store read value (in host endian order).

  Return Value:
    0 for success,
    other for error
*/
Sint32T device_readFn(void *hwparams, void *arg, Uint32T address, Uint16T page, Uint16T offset, Uint8T size, Uint32T *value);

/**
    Device register write interface.

  Parameters:
   [in]  hwparams    Pointer to user specified device.
   [in]  arg         Pointer to user specified options.
   [in]  address     Virtual register address to access (i.e. combined page & offset for debugging)
   [in]  page        Page containing register to access
   [in]  offset      Offset within page containing register to access
   [in]  size        Size of register to access (1, 2, or 4)
   [in]  value       Register value to write (host endian order).

  Return Value:
    0 for success,
    other for error
*/
Sint32T device_writeFn(void *hwparams, void *arg, Uint32T address, Uint16T page, Uint16T offset, Uint8T size, Uint32T value);


#if defined _ZL303XX_ZLE1588_BOARD
/* Device interfaces for ZLE1588 EVB as examples and test */

typedef struct {
    /* Pathname to the device driver (e.g. /dev/zl_spi0) */
    char devicePath[64];
} ZLE1588_HwParamsT;

Sint32T ZLE1588_readFn(void *hwparams, void *arg, Uint32T address, Uint16T page, Uint16T offset, Uint8T size, Uint32T *value);
Sint32T ZLE1588_writeFn(void *hwparams, void *arg, Uint32T address, Uint16T page, Uint16T offset, Uint8T size, Uint32T value);
Sint32T ZLE1588_GPIOSet(void *hwparams, Uint32T gpio, Uint8T value);
#endif


/*****************   Other               **********************************************/
/**
    Porting unit tests.

  Return Value:
    0 for success,
    other for error
*/
Sint32T osTestAll(void);


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
