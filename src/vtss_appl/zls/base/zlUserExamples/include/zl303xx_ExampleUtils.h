

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Header file for example files
*
*******************************************************************************/

#ifndef ZL303XX_EXAMPLE_UTILS_H_
#define ZL303XX_EXAMPLE_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"

#if defined _ZL303XX_ZLE30360_BOARD || defined _ZL303XX_ZLE1588_BOARD || defined _ZL303XX_FMC_BOARD || defined _ZL303XX_NTM_BOARD 
#include "zl303xx_ApiConfig.h"
#endif

#include "zl303xx_ExampleMain.h"

#if defined ZLS30390_INCLUDED
#include "zl303xx_PtpApiTypes.h" 
#include "zl303xx_PtpConstants.h" 
#endif

#include "zl303xx_Ptsf.h" 
#include "zl303xx_TSDebug.h"

#if defined APR_INCLUDED
/* for 380+361 combination, 361 does not include this file but we need it when
   combining the 2 loads. */
#include "zl303xx_Apr1Hz.h"
#endif
/*****************   DEFINES   ************************************************/

/*****************   DATA TYPES   *********************************************/

#if defined APR_INCLUDED
typedef enum {
   ACI_DEFAULT  = -1,
   ACI_FREQ_XO=16,
   ACI_PHASE_XO=17,
   ACI_FREQ_TCXO=0,
   ACI_PHASE_TCXO=1,
   ACI_PHASE_TCXO_Q=21,
   ACI_FREQ_OCXO_S3E=2,
   ACI_FREQ_OCXO_S3E_C=18,
   ACI_PHASE_OCXO_S3E=3,

   ACI_BC_PARTIAL_ON_PATH_FREQ=4,
   ACI_BC_PARTIAL_ON_PATH_PHASE=5,
   ACI_BC_PARTIAL_ON_PATH_PHASE_SYNCE=6,

   ACI_BC_FULL_ON_PATH_FREQ=7,
   ACI_BC_FULL_ON_PATH_PHASE=8,
   ACI_BC_FULL_ON_PATH_PHASE_SYNCE=9,

   ACI_FREQ_ACCURACY_FDD=10,
   ACI_FREQ_ACCURACY_XDSL=11,

   ACI_ELEC_FREQ=12,
   ACI_ELEC_REF_FREQ = ACI_ELEC_FREQ,

   ACI_ELEC_PHASE=13,
   ACI_ELEC_1PPS_PHASE = ACI_ELEC_PHASE,
   ACI_ELEC_REF_SYNC_PHASE = ACI_ELEC_PHASE,

   ACI_PHASE_RELAXED_C60W=102,
   ACI_PHASE_RELAXED_C150=103,
   ACI_PHASE_RELAXED_C180=104,
   ACI_PHASE_RELAXED_C240=105,


   ACI_BASIC_PHASE=110,                      /* different bw options */
   ACI_BASIC_PHASE_SYNCE=111,                /* different bw options */
   ACI_BASIC_PHASE_LOW = 113,
   ACI_BASIC_PHASE_LOW_SYNCE = 114,
   
   ACI_BC_PARTIAL_ON_PATH_PHASE_GROUP_4_ENG = 116,
   ACI_BC_PARTIAL_ON_PATH_PHASE_SYNCE_GROUP_4_ENG = 117,
   ACI_BC_PARTIAL_ON_PATH_PHASE_GROUP_4_G8261_ENG = 216,
   ACI_BC_PARTIAL_ON_PATH_PHASE_SYNCE_GROUP_4_G8261_ENG = 217,
   
   ACI_BC_FULL_ON_PATH_FREQ_HIGH_BW = 122,
   

   ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK = 125,
   ACI_BC_FULL_ON_PATH_PHASE_SYNCE_FASTER_LOCK = 126,

   ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE = 127,
   ACI_BC_FULL_ON_PATH_PHASE_SYNCE_FASTER_LOCK_LOW_PKT_RATE = 128,

   ACI_BC_FULL_ON_PATH_PHASE_HIGH_BW = 129,

   ACI_BC_FULL_ON_PATH_PHASE_TSN = 130,
   ACI_BC_FULL_ON_PATH_PHASE_SYNCE_TSN = 131,

   ACI_LAST = 500
} exampleAprConfigIdentifiersE;

/*****************   DATA STRUCTURES   ****************************************/

typedef struct
{
    zl303xx_BooleanE active;
    Uint32T param;
    Uint32T value;
} usrOvrXParamArryS;

typedef struct
{
    zl303xx_BooleanE active;
    Uint32T value;
} usrOvr1HzArryS;

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
zlStatusE exampleAprOverrideXParam(zl303xx_BooleanE fwdPath, Uint32T param, Uint32T value);
zlStatusE exampleAprOverrideXParamReset(zl303xx_BooleanE fwdPath, Uint32T param);
zlStatusE exampleAprOverrideXParamResetAll(zl303xx_BooleanE fwdPath);
zlStatusE exampleAprOverrideXParamApply(zl303xx_AprAddServerS *aprAddServer);

zlStatusE exampleAprOverride1HzXParam(zl303xx_BooleanE fwdPath, Uint32T param, Uint32T value);
zlStatusE exampleAprOverride1HzXParamReset(zl303xx_BooleanE fwdPath, Uint32T param);
zlStatusE exampleAprOverride1HzXParamResetAll(zl303xx_BooleanE fwdPath);
zlStatusE exampleAprOverride1HzXParamApply(zl303xx_BooleanE fwdPath, zl303xx_AprConfigure1HzS *config1Hz);
zlStatusE exampleAprSet1HzEnabled(void *hwParams, zl303xx_BooleanE bEnable1Hz);
#endif

#if defined _ZL303XX_ZLE30360_BOARD || defined _ZL303XX_ZLE1588_BOARD || defined _ZL303XX_FMC_BOARD || defined _ZL303XX_NTM_BOARD 
zlStatusE exampleSetup(zl303xx_DeviceModeE deviceMode, zl303xx_PtpVersionE ptpVersion);
zlStatusE exampleConfig(zl303xx_BooleanE isUnicast, zl303xx_BooleanE slaveCapable, zl303xx_PtpVersionE ptpVersion);
zlStatusE examplePtpShutdown(void);

void setExampleIpSrc(const char *srcIp);
void addExampleIpSrc(const char *srcIp);
char *exampleIpv4ToIpv6(const char *ipv4, char *ipv6, Uint16T bufLen);
void setExampleIPv6Src(Uint32T streamHandle, Uint8T const *srcAddr);
void setExampleMulticastIpDest(char const *destIP);
void setExampleUnicastIpDest(char const *destIP);
void setExampleUniIpSlaveDest(char const *destIP);
void setExampleUniIpMasterDest(char const *destIP);
void setExampleMulticastIPv6Dest(Uint32T streamHandle, Uint8T const *destAddr);
void setExampleUnicastIPv6Dest(Uint32T streamHandle, Uint8T *destAddr);
zlStatusE examplePtpPortAdd(Uint16T portNumber, Uint8T const *localAddr);
void setExampleIpSrcDest2(char const *srcIP, char const *destIP);
void getExampleIpSrcDest2(char *srcIP, Uint32T srcIpSize, char *destIP, Uint32T destIpSize);
#endif

#if defined APR_INCLUDED
zlStatusE exampleConfigure1Hz(
            void *hwParams,
            zl303xx_BooleanE enabled,
            Uint32T serverID,
            zl303xx_Apr1HzRealignmentTypeE realignmentType,
            Uint32T realignmentInterval,
            zl303xx_BooleanE bForwardPath,
            Sint32T packetRate,
            zl303xx_BooleanE startupModifier);

zlStatusE exampleConfigure1HzWithGlobals(void *hwParams, Uint16T serverId);
zlStatusE exampleAprModeSwitching(void *hwParams, Uint16T serverId, exampleAprConfigIdentifiersE configId);
zlStatusE exampleReConfigure1HzSettings(void *hwParams, Uint16T serverId, exampleAprConfigIdentifiersE configId);

zlStatusE exampleAprForceHoldover (void *hwParams, zl303xx_BooleanE bHoldover);
zlStatusE exampleAprForceActiveRef (void *hwParams, zl303xx_AprDeviceRefModeE userMode, Uint16T serverOrRefId);
zlStatusE exampleAprReConfigurePullInRange (void *hwParams,
                                   Uint32T serverId,
                                   zl303xx_AprPullInRangeE pullInRange);
zlStatusE exampleAprReConfigurePSLFCL (void *hwParams,
                                  Sint32T StaticOffset,
                                  Uint32T APRFrequencyLockedPhaseSlopeLimit,
                                  Uint32T APRFrequencyNotLockedPhaseSlopeLimit,
                                  Uint32T APRFrequencyFastPhaseSlopeLimit,
                                  Uint32T APRFrequencyLockedFrequencyChangeLimit,
                                  Uint32T APRFrequencyNotLockedFrequencyChangeLimit,
                                  Uint32T APRFrequencyFastFrequencyChangeLimit,
                                  Uint32T adjSize1HzPSL[ZL303XX_MAX_NUM_PSL_LIMITS],
                                  Uint32T PSL_1Hz[ZL303XX_MAX_NUM_PSL_LIMITS],
                                  Uint32T adjSize1HzFCL[ZL303XX_MAX_NUM_FCL_LIMITS],
                                  Uint32T FCL_1Hz[ZL303XX_MAX_NUM_FCL_LIMITS],
                                  Uint32T adjSize1HzAdjScaling[ZL303XX_MAX_NUM_ADJ_SCALING_LIMITS],
                                  Uint32T adjScaling_1Hz[ZL303XX_MAX_NUM_ADJ_SCALING_LIMITS]);
zlStatusE exampleAprGetPSLFCLStateData (void *hwParams);
zlStatusE exampleAprGetPSLFCLConfigData (void *hwParams);


zlStatusE exampleAprTsLoggingCtrl(void *hwParams, Uint32T serverId,
                                  Uint32T secondsToLog,
                                  swFuncPtrTSLogging userCalloutFn);

exampleAprConfigIdentifiersE exampleAprGetConfigParameters(void);
zlStatusE exampleAprSetConfigParameters(exampleAprConfigIdentifiersE configId);
zlStatusE exampleAprSetConfigDefaults(void);

#endif

#if defined ZLS30390_INCLUDED
zlStatusE exampleSetPtpPLLSettlingTimeOverride(zl303xx_PtpClockHandleT clockHandle, zl303xx_BooleanE clockSettlingTimeActiveB);
zl303xx_BooleanE exampleGetPtpPLLSettlingTimeOverride(zl303xx_PtpClockHandleT clockHandle);
zlStatusE examplePtpSettlingTimerSet(zl303xx_PtpClockHandleT clockHandle, Uint32T timeInSecs, zl303xx_BooleanE timerActiveB);
zlStatusE examplePtpTsLoggingCtrl(Uint32T serverId, Uint32T secondsToLog,
                                  swFuncPtrTSLogging userCalloutFn);
#endif

#ifdef ZL_UNG_MODIFIED
zlStatusE exampleGetHoldoverAndWarmStartServer(void *hw_params, Uint16T serverId);
#endif //ZL_UNG_MODIFIED

zlStatusE exampleStoreWarmStartInFile(void);
zlStatusE exampleGetWarmStartFromFileAndGo(void);

/* PTSF example */
void examplePtsfDefaultCallback(zl303xx_PtsfEventS *pEvent);

zlStatusE exampleAppStart1Hz(exampleAppS *pApp, Uint32T indx);

zlStatusE exampleAprOverrideXParam(zl303xx_BooleanE fwdPath, Uint32T param, Uint32T value);
zlStatusE exampleAprOverride1HzXParam(zl303xx_BooleanE fwdPath, Uint32T param, Uint32T value);

zlStatusE exampleAprReConfigurePSLFCLWithGlobals (void *hwParams);

/** zl303xx_AprSelectTypeIIPLL

*   The function selects and configures the type II PLL.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSelectType2BPLL(void);


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */

