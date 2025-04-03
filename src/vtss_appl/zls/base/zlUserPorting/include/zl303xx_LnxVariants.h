

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Includes for various Linux variants
*
*******************************************************************************/

#ifndef _ZL303XX_LNX_VRNTS_H_
#define _ZL303XX_LNX_VRNTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef OS_LINUX

/*
    This file deals with header files required for POSIX time and signals for a Linux OS.
    See feature_test_macros (See man 7 feature_test_macros) or compiler features.h.

    Each compiler/toolchain needs to be able to specify the correct header files and combinations of header
    files to resolve all the required symbols.
    
    The following linux variant definitions work for Microchip-specific set of cross compilers.
    
    The zl303xx_HWTimer.c, zl303xx_LinuxOs.c and zl303xx_LinuxOsTimerEtc.c files set one or more of these defines:
    NEED_POSIX_TIME, NEED_LINUX_TIME, and NEED_SIGINFO to get the headers they need.

    Use the following example (ZL_LNX_MIPS_CODESOURCERY was the latest (and cleanest)) as a starting point for your 
    particular toolchain and specify the files needed for the combined and uncombined header requirements.
    
    POSIX: timers, pthreads and real time signals AND Linux: time are usually required within the Linux BSP.
        e.g., CFLAGS_DEF += -D_POSIX_C_SOURCE=200112L

    For BSD-style signal() and socket functionality define _BSD_SOURCE (defines USE_MISC)
    Note: _BSD_SOURCE deprecated in newer compilers use _DEFAULT_SOURCE
        e.g., CFLAGS_DEF += -D_BSD_SOURCE

*/

#if !defined _POSIX_C_SOURCE || (_POSIX_C_SOURCE < 200112L) /* Just in case this is included outside of our 3 SDK files */
    #define _POSIX_C_SOURCE 200112L
#endif

/* Note: it is also recommended to define _BSD_SOURCE to get the BSD-style signal and sockets */

#ifdef ZL_LNX_MIPS_CODESOURCERY
    /* Also, see zl303xx_Os.h for this toolchain */

    #ifdef NEED_POSIX_TIME
        #include <time.h>
    #endif
    #ifdef NEED_LINUX_TIME
        #include <linux/time.h>
    #endif
    #ifdef NEED_SIGINFO
        #include <signal.h>

        #if !defined SA_RESTART
        #warning MSCC: SA_RESTART is MISSING in this compiler: ZL_LNX_MIPS_CODESOURCERY
            #define SA_RESTART   0x10000000 /* Would have to define __USE_UNIX98?  */
        #endif
    #endif

#endif


#ifdef ZL_LNX_CODESOURCERY
    #ifdef NEED_POSIX_TIME
        #include <time.h>
    #endif

    #ifdef NEED_SIGINFO
        #include <signal.h>

        #if !defined SA_RESTART
        #warning MSCC: SA_RESTART is MISSING in this compiler: ZL_LNX_CODESOURCERY
            #define SA_RESTART   0x10000000
        #endif
    #endif

    #ifdef NEED_LINUX_TIME
        #define _STRUCT_TIMESPEC
        typedef long suseconds_t;
        #define __need_timeval
        #include <linux/time.h>
    #endif

#endif  /* ZL_LNX_CODESOURCERY */


#ifdef ZL_LNX_DENX
    #ifdef NEED_SIGINFO
        #include <signal.h>

        #if !defined SA_RESTART
        #warning MSCC: SA_RESTART is MISSING in this compiler: ZL_LNX_DENX
            #define SA_RESTART   0x10000000
        #endif
    #endif

    #ifdef NEED_POSIX_TIME
        #include <time.h>
    #endif

    #ifdef NEED_LINUX_TIME
        #define _STRUCT_TIMESPEC
        #define __need_timeval
        #define __need_timespec
        typedef long suseconds_t;

        #include <linux/time.h>
    #endif

#endif  /* ZL_LNX_DENX */


#if defined ZL_LNX_INTEL
    #ifdef NEED_LINUX_TIME
        #define _STRUCT_TIMESPEC
        typedef long suseconds_t;

        #include <linux/time.h>
    #endif

    #ifdef NEED_POSIX_TIME
        #include <time.h>

        #ifndef CLOCK_MONOTONIC
        #warning MSCC: CLOCK_MONOTONIC is MISSING in this compiler: ZL_LNX_INTEL
            #define CLOCK_REALTIME  0
            #define CLOCK_MONOTONIC 1
        #endif
    #endif

    #ifdef NEED_SIGINFO
        #include <signal.h>

        #if !defined SA_RESTART
        #warning MSCC: SA_RESTART is MISSING in this compiler: ZL_LNX_INTEL
            #define SA_RESTART   0x10000000
        #endif
    #endif

#endif  /* ZL_LNX_INTEL */


#include <signal.h>

/* see SIGRTMAX in bits/local_lim.h or bits/signum.h or ... */
/* SIGRTZLBLOCK must align within your kernel SIGRTMAX */
#ifdef ZL_UNG_MODIFIED
//#warning MSCC: SIGRTZLBLOCK should align with your kernel SIGRTMAX
#if defined (__MIPSEL__)
    #define SIGRTZLBLOCK      (128)          /* We need a block of real-time signals - counting down from here */
#else
    #define SIGRTZLBLOCK      (64)
#endif //__MIPSEL__

#endif //ZL_UNG_MODIFIED

#if defined ZL_INTR_USES_SIGACTION
   #define SIGZL0HIGH     (SIGRTZLBLOCK -1)   /* Must match with kernel driver code! */
   #define SIGZL0LOW      (SIGRTZLBLOCK -2)   /* Must match with kernel driver code! */
   #define ZLWDTIMERSIG   (SIGRTZLBLOCK -3)   /* S/W WatchDog for high interrupts */
   #define ZLTICKTIMERSIG (SIGRTZLBLOCK -4)   /* PTP Timer */
   #define ZLAPRTIMERSIG  (SIGRTZLBLOCK -5)   /* Apr Timer */
#else
   #define ZLTICKTIMERSIG (SIGRTZLBLOCK -1)   /* PTP Timer */
   #define ZLAPRTIMERSIG  (SIGRTZLBLOCK -2)   /* Apr Timer */
   #define ZLWDTIMERSIG   (SIGRTZLBLOCK -3)   /* S/W WatchDog for high interrupts */
  #if defined _ZL303XX_ZLE1588_BOARD || defined POLL_FOR_ZL303XX_INTR 
   #define SIGZL0HIGH     (SIGRTZLBLOCK -7)   /* Fake interrupt with timer */
   #define SIGZL0LOW      (SIGRTZLBLOCK -8)   /* Unused but required by handler */
  #endif
   #define ZLCSTTIMERSIG  (SIGRTZLBLOCK -10)   /* Clock settling timer - Need ZL303XX_PTP_NUM_CLOCKS_MAX (now 4) so 10 to 14 used */
#endif


#endif /* OS_LINUX */

#ifdef __cplusplus
}
#endif

#endif  /* _ZL303XX_LNX_VRNTS_H_ */

