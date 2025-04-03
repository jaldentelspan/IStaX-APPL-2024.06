

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Header file for APR example files
*
*******************************************************************************/

#ifndef _ZL303XX_EXAMPLE_APR_H_
#define _ZL303XX_EXAMPLE_APR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Error.h"
#include "zl303xx_Apr.h"
#include "zl303xx_DeviceSpec.h"

/*****************   DEFINES   ************************************************/

#define ZL303XX_HYBRID_TRANSIENT_ON   (1)
#define ZL303XX_HYBRID_TRANSIENT_OFF  (0)

/*****************   DATA TYPES   *********************************************/

typedef struct
{
   zl303xx_ParamsS *cguId;
   zl303xx_AprAddServerS server;
} exampleAprStreamCreateS;

typedef struct
{
   zl303xx_ParamsS *cguId;
   zl303xx_AprAddDeviceS device;

} exampleAprClockCreateS;



/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

#if defined APR_INCLUDED
zlStatusE exampleAprColdSwitchGroup(void);
zlStatusE exampleAprEnvInit(void);
zlStatusE exampleAprEnvClose(void);
zlStatusE exampleAprEnvDebug(void);

zlStatusE exampleAprClockCreateStructInit(exampleAprClockCreateS *pClock);
zlStatusE exampleAprClockCreate(exampleAprClockCreateS *pClock);
zlStatusE exampleAprClockRemove(exampleAprClockCreateS *pClock);
zlStatusE exampleAprClockCreateDebug(exampleAprClockCreateS *pClock, const char *prefixStr);

zlStatusE exampleAprStreamCreateStructInit(exampleAprStreamCreateS *pStream);
zlStatusE exampleAprStreamCreate(exampleAprStreamCreateS *pStream);
zlStatusE exampleAprStreamRemove(exampleAprStreamCreateS *pStream);
zlStatusE exampleAprStreamCreateDebug(exampleAprStreamCreateS *pStream, const char *prefixStr);

Sint32T exampleAprDcoSetFreq(void *clkGenId, Sint32T freqOffsetInPartsPerTrillion);
Sint32T exampleAprDcoGetFreq(void *clkGenId, Sint32T *freqOffsetInPartsPerTrillion);
Sint32T exampleAprGetHwManualFreerunStatus(void *hwParams, Sint32T *manStatus);
Sint32T exampleAprGetHwManualHoldoverStatus(void *hwParams, Sint32T *manStatus);
Sint32T exampleAprGetHwLockStatus(void *hwParams, Sint32T *manStatus);
Sint32T exampleAprDefaultgetHwSyncInputEnStatus(void *hwParams, Sint32T *manStatus);
Sint32T exampleAprDefaultgetHwOutOfRangeStatus(void *hwParams, Sint32T *manStatus);
Sint32T exampleAprDefaultSwitchToPacketRef(void *hwParams);
Sint32T exampleAprDefaultSwitchToElectricalRef(void *hwParams);

Sint32T exampleAprSetTimeTsu(void *clkGenId, 
                             Uint64S deltaTimeSec, 
                             Uint32T deltaTimeNanoSec,
                             zl303xx_BooleanE negative);
Sint32T exampleAprStepTimeTsu(void *clkGenId, 
                             Sint32T deltaTimeNs);
Sint32T exampleAprJumpTimeTsu(void *clkGenId,
                             Uint64S deltaTimeSec,
                             Uint32T deltaTimeNanoSec,
                             zl303xx_BooleanE negative);

zlStatusE exampleAprHandleHybridTransient(void *hwParams, zl303xx_BCHybridTransientType transient);
zlStatusE exampleAprHandleHybridTransientType2B(void *hwParams, zl303xx_BCHybridTransientType transient);
zlStatusE exampleAprHandleHybridTransientType2BDeviceActions(void *hwParams, zl303xx_BCHybridTransientType transient);
zlStatusE exampleAprHandleHybridTransientType2BSynceDPLL(void *hwParams, zl303xx_BCHybridTransientType transient);

zlStatusE exampleAprRecalculateStepTimeResolution(void *hwParams, Uint32T *newStepTimeResolutionNs);
#endif

/* Message router example for custom device */
Sint32T zl303xx_DpllCustomMsgRouter(void *hwParams, void *inData, void *outData);

/* Example code message router */
Sint32T zl303xx_UserMsgRouter(void *hwParams, void *inData, void *outData);

/* Group Switching based on CID */
zlStatusE exampleAprReconfigureDeviceAndServer(Uint32T newACI, zl303xx_BooleanE bRecreateAllServers);


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
