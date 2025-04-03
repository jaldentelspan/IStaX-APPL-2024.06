

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     This file contains functions that are bound into PSLFCL and APR to provide the
*     user with the ability to use their own methods to handle certain situations.
*
*******************************************************************************/

#ifndef _ZL303XX_EXAMPLE_APRBINDING_H_
#define _ZL303XX_EXAMPLE_APRBINDING_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_DataTypes.h"

#include "zl303xx_Apr.h"

/*****************   DEFINES   ************************************************/

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
void exampleAprCguNotify(zl303xx_AprCGUNotifyS *msg);
void exampleAprElecNotify(zl303xx_AprElecNotifyS *msg);
void exampleAprServerNotify(zl303xx_AprServerNotifyS *msg);
void exampleAprOneHzNotify(zl303xx_Apr1HzNotifyS *msg);
zlStatusE exampleUserDelayFunc(Uint32T requiredDelayMs, Uint64S startOfRun, Uint64S endOfRun, Uint64S *lastStartTime);
void zl303xx_CalcTimeDiff(Uint64S startTime, Uint64S stopTime, Uint64S *diffTime);
void zl303xx_CalcNsDelay(Uint64S runTimeNs, Uint64S usualDelayNs, Uint64S *newDelayNs);


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
