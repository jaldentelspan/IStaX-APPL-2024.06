

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Types and prototypes needed by the timer routines
*
*******************************************************************************/

#ifndef _ZL303XX_HW_TIMER_H_
#define _ZL303XX_HW_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Error.h"
#include "zl303xx_Os.h"
#include "zl303xx_DataTypesEx.h"

#include <time.h>

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
zlStatusE zl303xx_SetHWTimer(Uint32T rtSignal, timer_t *timerId, Sint32T osTimeDelayMs, void (*callout)(Sint32T, Sint32T), zl303xx_BooleanE);
zlStatusE zl303xx_DeleteHWTimer(Uint32T rtSignal, timer_t *timerId);

/*****************   DEFINES   ************************************************/
#if !defined TEN_e3
#define TEN_e3 1000
#endif
#if !defined TEN_e6
#define TEN_e6 1000000
#endif
#if !defined TEN_e9
    #define TEN_e9 1000000000
#endif


#if defined OS_LINUX
#include "zl303xx_LnxVariants.h"  /* Linux SIGRTZLBLOCK*/
#endif
 
#ifdef OS_VXWORKS
    /* Real one is defined/used only in Linux but are used so define it here */
    #define SIGRTZLBLOCK   (100)
    #define ZLAPRTIMERSIG   (SIGRTZLBLOCK -1)       /* Apr Timer */
   #define ZLCSTTIMERSIG  (SIGRTZLBLOCK -10)   /* Clock settling timer - Need ZL303XX_PTP_NUM_CLOCKS_MAX (now 4) */
#endif

#ifdef SOCPG_PORTING
    #define SIGRTZLBLOCK   (100)
    #define ZLAPRTIMERSIG   (SIGRTZLBLOCK -1)       /* Apr Timer */
    #define ZLCSTTIMERSIG  (SIGRTZLBLOCK -10)   /* Clock settling timer - Need ZL303XX_PTP_NUM_CLOCKS_MAX (now 4) */
#endif
#define NOT_ABSTIME 0   /* relative to now */


#ifdef __cplusplus
}
#endif

#endif

