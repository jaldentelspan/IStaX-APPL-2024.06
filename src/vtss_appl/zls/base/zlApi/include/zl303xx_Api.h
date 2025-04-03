

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Top level header file that includes all the other header files for the API
*
*******************************************************************************/

#ifndef ZL303XX_API_TOP_H
#define ZL303XX_API_TOP_H

/*****************   INCLUDE FILES                *****************************/

#include "zl303xx_Global.h"  /* This should always be the first file included */

/* Now include the porting library since most other components depend on it */
#include "zl303xx_Porting.h"

#if defined ZLS30361_INCLUDED || defined ZLS30721_INCLUDED || defined ZLS30701_INCLUDED || defined ZLS30731_INCLUDED || defined ZLS30751_INCLUDED || defined ZLS30771_INCLUDED
/* Devices need to include other header files from this directory */
#include "zl303xx.h"
#include "zl303xx_Init.h"
#include "zl303xx_Spi.h"
#include "zl303xx_RdWr.h"
#if defined ZLS30341_INCLUDED
#include "zl303xx_ApiInterrupt.h"
#include "zl303xx_Interrupt.h"
#include "zl303xx_Dpll34xDco.h"
#include "zl303xx_TsEng.h"
#endif
#endif
/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/
/* API description strings */
extern const char zl303xx_ApiBuildDate[];
extern const char zl303xx_ApiBuildTime[];
extern const char zl303xx_ApiReleaseDate[];
extern const char zl303xx_ApiReleaseTime[];
extern const char zl303xx_ApiReleaseType[];
extern const char zl303xx_ApiReleaseVersion[];
extern const char zl303xx_ApiPatchLevel[];
extern const char zl303xx_ApiReleaseSwId[];

#endif   /* MULTIPLE INCLUDE BARRIER */
