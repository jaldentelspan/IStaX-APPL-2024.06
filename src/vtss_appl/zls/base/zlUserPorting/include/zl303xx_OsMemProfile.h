/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Simple memory profiler. Include this file at the bottom of zl303xx_Os.h
*     to redirect calls to OS_CALLOC() and OS_FREE(), so they can be tracked.
*
*     OS_MEM_PROFILE needs to be defined at compile time. Either add it to
*     the CFLAGS, or uncomment its #define below. SHOW_CALLOCS can also
*     be defined to print out info when a call to OS_CALLOC() or OS_FREE()
*     is made (will only work if OS_MEM_PROFILE is also defined).
*
*     If OS_MEM_PROFILE is defined, it will override the DEBUG_MEMORY_TRAMPLER
*     stuff in zl303xx_LinuxOs.c. So, don't use both at the same time!
*
*     This isn't thread-safe, but it should be OK. Our API generally doesn't
*     allocate/free memory outside of the thread that executes the example code.
*
*******************************************************************************/

#ifndef ZL303XX_OS_MEM_PROFILE_H_
#define ZL303XX_OS_MEM_PROFILE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"

/*****************   DEFINES   ************************************************/
/*#define OS_MEM_PROFILE*/
/*#define SHOW_CALLOCS*/

#ifdef OS_MEM_PROFILE
#undef  OS_CALLOC
#undef  OS_FREE

#define OS_CALLOC(num, size) osCallocProfile(num, size, #size, __FILE__, __LINE__)
#define OS_FREE(ptr)         osFreeProfile(ptr, __FILE__, __LINE__)
#endif

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
void *osCallocProfile(size_t num, size_t size, const char *ptrName,
                      const char *fileName, Uint32T line);
void osFreeProfile(void *ptr, const char *file, Uint32T line);
void osMemStats(void);
void osMemStatsReset(void);

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
