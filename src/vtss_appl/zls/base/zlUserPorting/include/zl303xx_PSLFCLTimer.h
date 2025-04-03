

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  This files holds deprecated function that will no longer be used after V4.
*  Please use the new functions provided in zl303xx_HWTimer.c/.h
*
*  This provides a deprecated implementation of the timer needed by the phase slope 
*   limit and frequency change limit software components.*
*
*******************************************************************************/

#ifndef _ZL303XX_VXW_PSLFCL_TIMER_H_
#define _ZL303XX_VXW_PSLFCL_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
void zl303xx_PFSetHWTimer(Sint32T PFOsTimeDelay, void (*callout)(timer_t, Sint32T));
void zl303xx_PFDeleteHWTimer(void);

/*****************   DEFINES   ************************************************/

#ifdef __cplusplus
}
#endif

#endif

