

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Types and prototypes needed by the APR debug routines
*
*******************************************************************************/

#ifndef _ZL303XX_DEBUG_APR_H_
#define _ZL303XX_DEBUG_APR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Int64.h"
#include "zl303xx_Error.h"
#include "zl303xx_DeviceSpec.h"
/*****************   DEFINES   ************************************************/

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

zlStatusE zl303xx_GetAprDeviceConfigInfo(void *hwParams);
zlStatusE zl303xx_GetAprDeviceStatus(void *hwParams);
zlStatusE zl303xx_GetAprServerConfigInfo(void *hwParams, Uint16T serverId);
zlStatusE zl303xx_GetAprServerStatus(void *hwParams, Uint16T serverId);
void zl303xx_DebugAprBuildInfo(void);

zlStatusE zl303xx_DebugPrintAprByStreamNum(void *hwParams, Uint32T streamNumber, Uint32T algorithmNumber);
zlStatusE zl303xx_DebugPrintAprByReferenceId(void *hwParams, Uint32T referenceID);


zlStatusE zl303xx_DebugGetAprPathStatistics(void * hwParams);
zlStatusE zl303xx_AprGetDeviceCurrentPathDelays(void * hwParams);
zlStatusE zl303xx_AprGetServerCurrentPathDelays(void * hwParams, Uint16T serverId);

zlStatusE zl303xx_DebugGetAllAprStatistics(void * hwParams);
zlStatusE zl303xx_DebugAprPrint1HzData(void *hwParams, Uint32T streamNumber, Uint32T algorithmNumber);
zlStatusE zl303xx_DebugAprGet1HzData(void *hwParams, Uint32T streamNumber, Uint32T algorithmNumber);
zlStatusE zl303xx_DebugAprPrintPSLFCLData(void *hwParams);

zlStatusE zl303xx_DebugHybridState(void *hwParams);
zlStatusE zl303xx_DebugGetServerStatus(void *hwParams, Uint16T serverId);
zlStatusE zl303xx_DebugCGULockState(void *hwParams);

zlStatusE zl303xx_DebugAprHoldoverQualityParams(void *hwParams);

zlStatusE zl303xx_DebugAprGetSWVersion(void);


zlStatusE zl303xx_DebugAprPrintStructAprInit(void *pointerAprInitS, const char *prefixStr);
zlStatusE zl303xx_DebugAprPrintStructAprAddDevice(void *hwParams, void *pointerAprAddDeviceS, const char *prefixStr);
zlStatusE zl303xx_DebugAprPrintStructAprAddServer(void *hwParams, void *pointerAprAddServerS, const char *prefixStr);

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */

