

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     This file is included into every other API file and should be used for defining
*     truly global properties
*
*******************************************************************************/

#ifndef _ZL303XX_GLOBAL_H_
#define _ZL303XX_GLOBAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   BEGIN DEFINES     **********************************************/

/* Company name e.g. used by PTP node description */
#define ZL303XX_MICROCHIP_NAME  "Microchip Technology Inc"

/*****************   END DEFINES     **********************************************/


/*****************   BEGIN COMPILER CONTROL FLAGS   **********************************/

/* The following define enables a check every time the timestamps are sampled
   to ensure they are changing and will output an error message if not. This
   facility can be disabled by undefining this value */
#define ZL303XX_CHECK_TIMESTAMPS_CHANGING  1

   #define _ZL303XX_LOCAL    static

#ifdef OS_LINUX
   #define NORETURN __attribute__ ((noreturn))
#else
   #define NORETURN
#endif

/* task settings for APR */
#ifdef OS_VXWORKS
#define ZL303XX_APR_AD_TASK_PRIORITY              (Uint32T)80         /* Task is DEPRECATED in 5.0.0 */
#endif
#ifdef OS_LINUX
#define ZL303XX_APR_AD_TASK_PRIORITY              (Uint32T)88         /* Task is DEPRECATED in 5.0.0 */
#endif
#ifdef SOCPG_PORTING
#ifdef OS_FREERTOS
#define ZL303XX_APR_AD_TASK_PRIORITY              (Uint32T)17         /* Task is DEPRECATED in 5.0.0 */
#endif
#endif
#define ZL303XX_APR_AD_TASK_STACK_SIZE            (Uint32T)20000      /* Task is DEPRECATED in 5.0.0 */

#ifdef OS_VXWORKS
#define ZL303XX_PF_TASK_PRIORITY              (Uint32T)33             /* Task is DEPRECATED in 5.0.0 */
#endif
#ifdef OS_LINUX
#define ZL303XX_PF_TASK_PRIORITY              (Uint32T)88             /* Task is DEPRECATED in 5.0.0 */
#endif
#ifdef SOCPG_PORTING
#ifdef OS_FREERTOS
#define ZL303XX_PF_TASK_PRIORITY              (Uint32T)17             /* Task is DEPRECATED in 5.0.0 */
#endif
#endif
#define ZL303XX_PF_TASK_STACK_SIZE            (Uint32T)20000          /* Task is DEPRECATED in 5.0.0 */

#ifdef OS_VXWORKS
#define ZL303XX_APR_Sample_TASK_PRIORITY              (Uint32T)34     /* Task is DEPRECATED in 5.0.0 */
#endif
#ifdef OS_LINUX
#define ZL303XX_APR_Sample_TASK_PRIORITY              (Uint32T)98     /* Task is DEPRECATED in 5.0.0 */
#endif
#ifdef SOCPG_PORTING
#ifdef OS_FREERTOS
#define ZL303XX_APR_Sample_TASK_PRIORITY              (Uint32T)22     /* Task is DEPRECATED in 5.0.0 */
#endif
#endif
#define ZL303XX_APR_Sample_TASK_STACK_SIZE            (Uint32T)20000  /* Task is DEPRECATED in 5.0.0 */

/* Define system log file location */
#ifdef OS_VXWORKS
#define LOG_FILE_NAME "/tgtsvr/"
#endif
#ifdef OS_LINUX
#define LOG_FILE_NAME "/tmp/"
#endif


/*****************   END COMPILER CONTROL FLAGS   **********************************/


/*****************   BEGIN INCLUDE FILES   ******************************************/

#include "zl303xx_DataTypes.h"     /* Basic ZL datatypes */
#include "zl303xx_DataTypesEx.h"   /* Extended datatypes specific to this project */

#include <ctype.h>
#include "zl303xx_Os.h"

/*****************   END INCLUDE FILES   ******************************************/

#ifdef __cplusplus
}
#endif

#endif   /* MULTIPLE INCLUDE BARRIER */
