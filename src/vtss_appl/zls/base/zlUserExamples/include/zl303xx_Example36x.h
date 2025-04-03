

/*******************************************************************************
 *
 *  $Id: 5829f97c957ab0b7024bbe9785d3bfb3bdfb63de
 *
 *  Copyright 2006-2019 Microchip/Microsemi Semiconductor Limited.
 *  All rights reserved.
 *
 *  Module Description:
 *     Supporting interfaces for the 36x examples
 *
 ******************************************************************************/

#ifndef _ZL303XX_EXAMPLE_36X_H_
#define _ZL303XX_EXAMPLE_36X_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Error.h"
#include "zl303xx_DeviceSpec.h"

/*****************   DEFINES   ************************************************/

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

typedef struct
{
   zl303xx_DeviceModeE deviceMode;
   Uint32T pllId;
   zl303xx_ParamsS *zl303xx_Params;

} example36xClockCreateS;



/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

zlStatusE example36xEnvInit(void);
zlStatusE example36xEnvClose(void);
zlStatusE example36xSlaveWarmStart(void);

zlStatusE example36xClockCreateStructInit(example36xClockCreateS *pClock);
zlStatusE example36xClockCreate(example36xClockCreateS *pClock);
zlStatusE example36xClockRemove(example36xClockCreateS *pClock);

zlStatusE example36xLoadConfigFile(zl303xx_ParamsS *zl303xx_Params, const char *filename);
zlStatusE example36xLoadConfigDefaults(zl303xx_ParamsS *zl303xx_Params);

zlStatusE example36xStickyLockCallout(void *hwParams, zl303xx_DpllIdE pllId, zl303xx_BooleanE lockFlag);

#if defined _ZL303XX_ZLE30360_BOARD || defined _ZL303XX_ZLE1588_BOARD
zlStatusE example36xSlave(void);
zlStatusE example36xMaster(void);
#else
zlStatusE example36x(void);
#endif

#if defined _ZL303XX_ZLE30360_BOARD || defined _ZL303XX_ZLE1588_BOARD
zlStatusE example36xCheckPhyTSClockFreq(zl303xx_ParamsS *zl303xx_Params);
#endif

/* Callback functions */
Sint32T example36xSetActiveElecActionsSyncEand1588DpllStage1to4(void *hwParams, Sint32T refId, Sint32T syncId);
Sint32T example36xSetActiveElecActionsSyncEand1588DpllStage6(void *hwParams, Sint32T refId);
Sint32T example36xSetActiveElecActionsSyncEand1588DpllStage8(void *hwParams, Sint32T refId);
zlStatusE example36xBalanceDpllInputAndOutputOffsetsUsingOutputAdj(void *hwParams);
zlStatusE example36xBalanceDpllInputAndOutputOffsetsUsingInputAdj(void *hwParams);

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
