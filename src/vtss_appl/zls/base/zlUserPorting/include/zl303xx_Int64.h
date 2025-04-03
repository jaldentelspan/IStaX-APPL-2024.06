

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     This file contains the type definitions for 64 bit integers.
*
*******************************************************************************/

#ifdef __cplusplus
   extern "C" {
#endif

/* make some generic 64 bit type definitions if they haven't already been made */
#ifndef _ZL_INT64_DEFINED

   #define _ZL_INT64_DEFINED

   #include "zl303xx_Os.h"

#if !defined OS_LINUX
    #define OS_UINT64    unsigned long long int     /* VxWorks uses long long */
    #define OS_SINT64    signed long long int
#else
    #define OS_UINT64    u_int64_t                  /* Linux uses a 64 type */
    #define OS_SINT64    int64_t
#endif

      typedef OS_UINT64 Uint64T;
      typedef OS_SINT64 Sint64T;

#endif

/* if the device specific macros haven't been defined then do so */
#ifndef _ZL303XX_INT64_DEFINED
   #define _ZL303XX_INT64_DEFINED

      #define ZL303XX_CONVERT_TO_64(val, structVal)           ((val) = ((Uint64T)((structVal).hi) << 32) | (Uint64T)((structVal).lo))
      #define ZL303XX_CONVERT_TO_64_SIGNED(val, structVal)    ((val) = (Sint64T)(((Uint64T)((structVal).hi) << 32) | (Uint64T)((structVal).lo)))

      /* do the shift in 2 steps below, since the compiler implements the >> 32 correctly but
         produces a warning. If optimisations are on then this will default to a single 32 bit
         op which probably won't even be a shift, so just inelegant */
      #define ZL303XX_CONVERT_FROM_64(structVal, val)   (structVal).hi = (Uint32T)(((val) >> 16) >> 16); \
                                                      (structVal).lo = (Uint32T)(val);
      #define ZL303XX_SHIFT_RIGHT_32(val)               (((val) >> 16) >> 16)

#endif

#ifdef __cplusplus
}
#endif
