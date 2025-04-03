

/*******************************************************************************
*
*  $Id: 6517a3b4707c071ac26ad8fcef236bd9d8d99889
*
*  Copyright (c) 2006-2022 Microchip Technology Inc. and its subsidiaries, all rights reserved.
*  Subject to the terms of the license that accompanies the software and controls as it relates to the software and any conflicting terms herein, you may use this Microchip software and any derivatives exclusively with Microchip products.
*  You are responsible for complying with third party license terms applicable to your use of third party software (including open source software) that may accompany this Microchip software.
*  SOFTWARE IS 'AS IS'. NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
*  IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.
*  TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
*  Module Description:
*     This file contains functions to configure, start, and stop APR
*
*******************************************************************************/

#ifndef ZL303XX_EXAMPLE_APR_GLOBALS_H_
#define ZL303XX_EXAMPLE_APR_GLOBALS_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "zl303xx_Global.h"  /* This should always be the first include file */
#include "zl303xx_Error.h"

#if defined APR_INCLUDED
#include "zl303xx_Apr.h"
#include "zl303xx_Apr1Hz.h"
#include "zl303xx_ExampleUtils.h"
#endif

#if defined APR_INCLUDED
zlStatusE zl303xx_ExampleResetAprGlobalCfg(void);
zlStatusE zl303xx_ExampleResetAprDeviceGlobalCfg(void);
zlStatusE zl303xx_ExampleResetAprServerGlobalCfg(void);
zlStatusE zl303xx_ExampleResetAprInitGlobalCfg(void);
zlStatusE zl303xx_ExampleResetAprMiscGlobalCfg(void);

zlStatusE  exampleAprSetConfigId(Uint32T configId);
exampleAprConfigIdentifiersE exampleAprGetConfigParameters(void);

/******************************************************************************
                              AddDevice() start
*******************************************************************************/


/** zl303xx_AprSetDeviceOptMode

   The function sets the default device reference mode for APR.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetDeviceOptMode(zl303xx_AprDeviceRefModeE devOptMode);

/** zl303xx_AprGetDeviceOptMode

   The function returns the default device reference mode for APR.

  Return Value: zl303xx_AprDeviceRefModeE       Device Reference Type
****************************************************************************/
zl303xx_AprDeviceRefModeE zl303xx_AprGetDeviceOptMode(void);


/** zl303xx_AprSetDeviceHybridAdjMode

The function sets the default device reference mode for APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetDeviceHybridAdjMode(zl303xx_AprDeviceHybridAdjModeE deviceHybridAdjMode);

/** zl303xx_AprGetDeviceHybridAdjMode

The function returns the default device reference mode for APR.

  Return Value: zl303xx_AprDeviceHybridAdjModeE       Device Hybrid Adjustment Type

****************************************************************************/
zl303xx_AprDeviceHybridAdjModeE zl303xx_AprGetDeviceHybridAdjMode(void);


/** zl303xx_AprSetDcs

   The function is for future use.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetDcs(zl303xx_BooleanE dcs);

/** zl303xx_AprGetDcs

   The function is for future use.

  Return Value: zlStatusE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetDcs(void);


/** zl303xx_AprSetHwDcoResolution

   The function sets the default device hardware DCO resolution for APR.
   Unit: ppt

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetHwDcoResolution(Uint32T hwDcoResolution);
/** zl303xx_AprGetHwDcoResolution

   The function returns the default device hardware DCO resolution for APR.

  Return Value: Uint32T   Hardware DCO resolution in ppt
****************************************************************************/
Uint32T zl303xx_AprGetHwDcoResolution(void);


/** zl303xx_AprSetEnterPhaseLockStatusThreshold

   The function sets the default enter phase lock status threshold for APR.
   Unit: ns

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetEnterPhaseLockStatusThreshold(Uint32T threshold);

/** zl303xx_AprGetEnterPhaseLockStatusThreshold

   The function returns the default enter phase lock status threshold for APR.

  Return Value: Uint32T   Enter phase lock status threshold in ns
****************************************************************************/
Uint32T zl303xx_AprGetEnterPhaseLockStatusThreshold(void);


/** zl303xx_AprSetExitPhaseLockStatusThreshold

   The function sets the default exit phase lock status threshold for APR.
   Unit: ns

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetExitPhaseLockStatusThreshold(Uint32T threshold);

/** zl303xx_AprGetExitPhaseLockStatusThreshold

   The function returns the default exit phase lock status threshold for APR.

  Return Value: Uint32T   Exit phase lock status threshold in ns
****************************************************************************/
Uint32T zl303xx_AprGetExitPhaseLockStatusThreshold(void);


/** zl303xx_AprSetWarmStart

   The function sets the warm start flag for APR.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetWarmStart(zl303xx_BooleanE warmStart);
/** zl303xx_AprGetWarmStart

   The function gets the warm start flag for APR.

  Return Value: zlStatusE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetWarmStart(void);


/** zl303xx_AprSetWsInitialOffset

   The function sets the warm start initial offset for APR.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetWsInitialOffset(Sint32T initOffset);

/** zl303xx_AprGetWsInitialOffset

   The function gets the warm start initial offset for APR.

  Return Value: zlStatusE
****************************************************************************/
Sint32T zl303xx_AprGetWsInitialOffset(void);

/** zl303xx_AprSetAPRFrequencyLockedPhaseSlopeLimit

   The function sets the PSL/FCL's phase slope limit for locked APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetAPRFrequencyLockedPhaseSlopeLimit(zl303xx_AprPhaseSlopeLimitE psl);
/** zl303xx_AprGetPFPhaseSlopeLimit

   The function gets the PSL/FCL's phase slope limit for locked APR.

  Return Value: zlStatusE

****************************************************************************/
zl303xx_AprPhaseSlopeLimitE zl303xx_AprGetAPRFrequencyLockedPhaseSlopeLimit(void);

/** zl303xx_AprSetAPRFrequencyNotLockedPhaseSlopeLimit

   The function sets the PSL/FCL's phase slope limit for not-locked APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetAPRFrequencyNotLockedPhaseSlopeLimit(zl303xx_AprPhaseSlopeLimitE psl);

/** zl303xx_AprGetPFPhaseSlopeLimit

   The function gets the PSL/FCL's phase slope limit for not-locked APR.

  Return Value: zlStatusE

****************************************************************************/
zl303xx_AprPhaseSlopeLimitE zl303xx_AprGetAPRFrequencyNotLockedPhaseSlopeLimit(void);

/** zl303xx_AprSetAPRPhaseSlopeLimitTimeoutPeriodSec

   The function sets the PSL limited timeout period (seconds)

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetAPRPhaseSlopeLimitTimeoutPeriodSec(Uint32T pslTimeoutPeriodSec);

/** zl303xx_AprGetAPRPhaseSlopeLimitTimeoutPeriodSec

   The function gets the PSL limited timeout period (seconds)

  Return Value: zl303xx_AprPhaseSlopeLimitE

****************************************************************************/
Uint32T zl303xx_AprGetAPRPhaseSlopeLimitTimeoutPeriodSec(void);

/** zl303xx_AprSetAPRFrequencyFastPhaseSlopeLimit

   The function sets the PSL/FCL's phase slope limit for fast lock APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetAPRFrequencyFastPhaseSlopeLimit(zl303xx_AprPhaseSlopeLimitE psl);

/** zl303xx_AprGetAPRFrequencyFastPhaseSlopeLimit

   The function gets the PSL/FCL's phase slope limit for fast lock APR.

  Return Value: zlStatusE
****************************************************************************/
zl303xx_AprPhaseSlopeLimitE zl303xx_AprGetAPRFrequencyFastPhaseSlopeLimit(void);

/** zl303xx_AprSetAPRFrequencyFastFrequencyChangeLimit

The function sets the PSL/FCL's phase slope limit for fast lock APR.

  Return Value: zlStatusE

 * ****************************************************************************/
zlStatusE zl303xx_AprSetAPRFrequencyFastFrequencyChangeLimit(zl303xx_AprFrequencyChangeLimitE fcl);
/** zl303xx_AprGetAPRFrequencyFastFrequencyChangeLimit

The function gets the PSL/FCL's phase slope limit for fast lock APR.

  Return Value: zl303xx_AprFrequencyChangeLimitE

****************************************************************************/
zl303xx_AprFrequencyChangeLimitE zl303xx_AprGetAPRFrequencyFastFrequencyChangeLimit(void);

/** zl303xx_AprSetAPRFrequencyLockedFrequencyChangeLimit

   The function sets the PSL/FCL's phase slope limit for locked APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetAPRFrequencyLockedFrequencyChangeLimit(zl303xx_AprFrequencyChangeLimitE fcl);
/** zl303xx_AprGetAPRFrequencyLockedFrequencyChangeLimit

   The function gets the PSL/FCL's phase slope limit for locked APR.

  Return Value: zl303xx_AprFrequencyChangeLimitE

****************************************************************************/
zl303xx_AprFrequencyChangeLimitE zl303xx_AprGetAPRFrequencyLockedFrequencyChangeLimit(void);

/** zl303xx_AprSetAPRFrequencyNotLockedFrequencyChangeLimit

   The function sets the PSL/FCL's phase slope limit for not-locked APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetAPRFrequencyNotLockedFrequencyChangeLimit(zl303xx_AprFrequencyChangeLimitE fcl);
/** zl303xx_AprGetAPRFrequencyNotLockedFrequencyChangeLimit

   The function gets the PSL/FCL's phase slope limit for not-locked APR.

  Return Value: zl303xx_AprFrequencyChangeLimitE

****************************************************************************/
zl303xx_AprFrequencyChangeLimitE zl303xx_AprGetAPRFrequencyNotLockedFrequencyChangeLimit(void);


/** zl303xx_AprSetPSL

   The function sets an element in the PSL size ranges for 1Hz.

   When 1Hz makes an adjustment, the adjustment size is compared against each
   element in defaultAdjSize1HzPSL[]. When the smallest element that is still
   larger than the adjustment is found, the corresponding PSL value in
   defaultPSL_1Hz[] is used.

   The defaults:
      defaultAdjSize1HzPSL[0] = 1000,   defaultPSL_1Hz[0] = 4
      defaultAdjSize1HzPSL[1] = 4000,   defaultPSL_1Hz[1] = APR_PSL_885_NS
      defaultAdjSize1HzPSL[2] = 10000,  defaultPSL_1Hz[2] = 4000
      defaultAdjSize1HzPSL[3] = 0,      defaultPSL_1Hz[3] = 0
      defaultAdjSize1HzPSL[4] = 0,      defaultPSL_1Hz[4] = 0

   eg. If an adjustment is 1200ns, then element [1] will be chosen
       (1000 < 1200 < 4000) and the PSL used will be 885ns/s.

   With these defaults, adjustments where adjustments < 1000 are corrected
   very slowly to comply with performance requirements, adjustment where
   4000 < adjustments < 1000 are corrected a more aggressive rate, and
   very large adjustments where 10000 < adjustments are considered fast
   lock and are adjusted very aggressively.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetPSL(Uint32T idx, Uint32T adjSize, Uint32T psl);

/** zl303xx_AprGetPSL

   The function gets the PSL size ranges for 1Hz.

  Return Value: zlStatusE

****************************************************************************/
 zlStatusE zl303xx_AprGetPSL(Uint32T idx, Uint32T *adjSize, Uint32T *psl );

/** zl303xx_AprSetPFLockInThreshold

   The function sets the PSL/FCL's lockin threshold for APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetPFLockInThreshold(Uint32T lit);

/** zl303xx_AprGetPFLockInThreshold

   The function gets the PSL/FCL's lockin threshold for APR.

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGetPFLockInThreshold(void);

/** zl303xx_AprSetPFLockInCount

   The function sets the PSL/FCL's lockin count for APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetPFLockInCount(Uint32T lic);

/** zl303xx_AprGetPFLockInCount

   The function gets the PSL/FCL's lockin count for APR.

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGetPFLockInCount(void);


/** zl303xx_AprSetPFLockOutThreshold

   The function sets the PSL/FCL's lockout threshold for APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetPFLockOutThreshold(Uint32T lot);

/** zl303xx_AprGetPFLockOutThreshold

   The function gets the PSL/FCL's lockout threshold for APR.

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGetPFLockOutThreshold(void);

/** zl303xx_AprSetPFAdjustFreqMinPhase

   The function sets the PSL/FCL's minimum phase at which it will make an
   adjustment.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetPFAdjustFreqMinPhase(Sint32T afmp);
/** zl303xx_AprGetPFAdjustFreqMinPhase

   The function gets the PSL/FCL's minimum phase at which it will make an
   adjustment.

  Return Value: zlStatusE
****************************************************************************/
Sint32T zl303xx_AprGetPFAdjustFreqMinPhase(void);


/** zl303xx_AprSetCustomerThresholdForFlagV

   The function sets the customer threshold of V flag for APR.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetCustomerThresholdForFlagV(Sint32T thresholdForFlagV);

/** zl303xx_AprGetCustomerThresholdForFlagV

   The function returns the customer threshold of V flag for APR.

  Return Value: Sint32T
****************************************************************************/
Sint32T zl303xx_AprGetCustomerThresholdForFlagV(void);


/** zl303xx_AprSetSetTimePacketTreatment

   The function sets the packet treatment when setTime() is called.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetSetTimePacketTreatment(zl303xx_PacketTreatmentE setTimePacketTreatment);

/** zl303xx_AprGetSetTimePacketTreatment

   The function returns the packet treatment.

  Return Value: Sint32T
****************************************************************************/
zl303xx_PacketTreatmentE zl303xx_AprGetSetTimePacketTreatment(void);


/** zl303xx_AprSetStepTimePacketTreatment

   The function sets the packet treatment when stepTime() is called.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetStepTimePacketTreatment(zl303xx_PacketTreatmentE stepTimePacketTreatment);

/** zl303xx_AprGetStepTimePacketTreatment

   The function returns the packet stepTime() treatment.

  Return Value: zl303xx_PacketTreatmentE
****************************************************************************/
zl303xx_PacketTreatmentE zl303xx_AprGetStepTimePacketTreatment(void);


/** zl303xx_AprSetAdjustTimePacketTreatment

   The function sets the packet treatment when adjustTime() is called.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetAdjustTimePacketTreatment(zl303xx_PacketTreatmentE adjustTimePacketTreatment);

/** zl303xx_AprGetAdjustTimePacketTreatment

   The function returns the packet adjustTime() treatment.

  Return Value: zl303xx_PacketTreatmentE
****************************************************************************/
zl303xx_PacketTreatmentE zl303xx_AprGetAdjustTimePacketTreatment(void);

/** zl303xx_AprSetSetTimeExecutionTime

   The function sets the setTime() execution time.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetSetTimeExecutionTime(Uint32T value);

/** zl303xx_AprGetSetTimeExecutionTime

   The function returns the setTime() execution time.

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetSetTimeExecutionTime(void);

/** zl303xx_AprSetStepTimeExecutionTime

   The function sets the stepTime() execution time.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetStepTimeExecutionTime(Uint32T value);

/** zl303xx_AprGetStepTimeExecutionTime

   The function returns the stepTime() execution time.

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetStepTimeExecutionTime(void);

/** zl303xx_AprSetAdjustTimeExecutionTime

   The function sets the adjustTime() execution time.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetAdjustTimeExecutionTime(Uint32T value);

/** zl303xx_AprGetAdjustTimeExecutionTime

   The function returns the adjustTime() execution time.

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetAdjustTimeExecutionTime(void);

/** zl303xx_AprSetLegacyTreatment

   The function sets the legacy treatment.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetLegacyTreatment(zl303xx_BooleanE legacyTreatment);

/** zl303xx_AprGetLegacyTreatment

   The function returns the legacy treatment.

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetLegacyTreatment(void);

/** zl303xx_AprSetPFStepTimeResolution

   The function sets the PSL/FCL's step time resolution

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetPFStepTimeResolution(Uint32T stepTimeResolution);

/** zl303xx_AprGetPFStepTimeResolution

   The function gets the PSL/FCL's step time resolution

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGetPFStepTimeResolution(void);

/** zl303xx_AprSetPFLockedPhaseOutlierThreshold

   The function sets the PSL/FCL's locked phase outlier threshold

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetPFLockedPhaseOutlierThreshold(Uint32T lockedPhaseOutlierThreshold);

/** zl303xx_AprGetPFLockedPhaseOutlierThreshold

   The function gets the PSL/FCL's locked phase outlier threshold

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGetPFLockedPhaseOutlierThreshold(void);

/** zl303xx_AprSetPFInitialFrequencyOffset

    The function sets the initial DCO frequency offset in ppt

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetPFInitialFrequencyOffset(Sint32T initialFrequencyOffset);

/** zl303xx_AprGetPFInitialFrequencyOffset

    The function gets the initial DCO frequency offset at startup in ppt

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGetPFInitialFrequencyOffset(void);

/** zl303xx_AprSetPFHwPollingIntervalTimer2Cycles

    The function sets the CGU read access rate in interrupts.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetPFHwPollingIntervalTimer2Cycles(Uint32T hwPollingIntervalTimer2Cycles);

/** zl303xx_AprGetPFHwPollingIntervalTimer2Cycles

    The function gets the CGU read access rate in interrupts.

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGetPFHwPollingIntervalTimer2Cycles(void);


/******************************************************************************
                              AddServer() start
*******************************************************************************/



/** zl303xx_AprSetAlgTypeMode

   The function sets the default algorithm type mode for APR.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetAlgTypeMode(zl303xx_AprAlgTypeModeE algTypeMode);

/** zl303xx_AprGetAlgTypeMode

   The function returns the default algorithm type mode for APR.

  Return Value: zl303xx_AprOscillatorTypesE
****************************************************************************/
zl303xx_AprAlgTypeModeE zl303xx_AprGetAlgTypeMode(void);

/** zl303xx_AprSetUseType2BPLL

*  The function sets the Boolean for using Type 2 PLL default to
*  FALSE.

  Return Value: zlStatusE*

****************************************************************************/
zlStatusE zl303xx_AprSetUseType2BPLL(zl303xx_BooleanE bUseT2PLL);

/** zl303xx_AprGetUseType2BPLL

   The function returns the Boolean for using Type 2 PLL.

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetUseType2BPLL(void);

/** zl303xx_AprSetUseGroup4Alg

    The function sets the Boolean for using Group4 algorithm.
    Default to FALSE.

  Return Value: zlStatusE*

****************************************************************************/
zlStatusE zl303xx_AprSetUseGroup4Alg(zl303xx_BooleanE bUseGroup4Alg);

/** zl303xx_AprGetUseGroup4Alg

   The function returns the Boolean for using Group4 algorithm.

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetUseGroup4Alg(void);

/** zl303xx_AprSetEnableXOCompensation

  The function sets the Boolean enableXOComp default to
  FALSE.

  Return Value: zlStatusE*

****************************************************************************/
zlStatusE zl303xx_AprSetEnableXOCompensation(zl303xx_BooleanE enableXOComp);

/** zl303xx_AprGetEnableXOCompensation

   The function returns the Boolean for enableXOComp.

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetEnableXOCompensation(void);

/** zl303xx_AprGetL2phase_varLimitValue

*   The function gets L2phase_varLimit

  Return Value: Uint32T
****************************************************************************/
Sint32T zl303xx_AprGetL2phase_varLimitValue(void);

/** zl303xx_AprSetL2phase_varLimitValue

The function sets L2phase_varLimit

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetL2phase_varLimitValue(Sint32T VarLimit);

/** zl303xx_AprSetUseType2BPLLFastLock

*  The function sets the Boolean for enabling T2PLL Fast Lock.

  Return Value: zlStatusE*

****************************************************************************/
zlStatusE zl303xx_AprSetUseType2BPLLFastLock(zl303xx_BooleanE bUseT2PLLFLE);

/** zl303xx_AprGetUseType2BPLLFastLock

*  The function returns the Boolean for enabling T2PLL Fast
*  Lock.

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetUseType2BPLLFastLock(void);

/** zl303xx_AprSetEnableLowBWFastLock

 * The function sets the Boolean for enabling T2PLL low BW Fast
 * Lock.

  Return Value: zlStatusE*

****************************************************************************/
zlStatusE zl303xx_AprSetEnableLowBWFastLock(zl303xx_BooleanE LowBWEnable);
/** zl303xx_AprGetEnableLowBWFastLock

 * The function returns the Boolean for enabling T2PLL low BW
 * Fast Lock.

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetEnableLowBWFastLock(void);


/** zl303xx_AprSetType2bFastLockMinPhase

*  The function sets the value for Type2bFastLockMinPhase.

  Return Value: zlStatusE*

****************************************************************************/
zlStatusE zl303xx_AprSetType2bFastLockMinPhase(Uint32T MinPhase);

/** zl303xx_AprGetUseType2BPLLFastLock

*  The function returns the value for Type2bFastLockMinPhase
*  Lock.

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGetType2bFastLockMinPhase(void);



/** zl303xx_AprSetOscillatorFilterType

   The function sets the default oscillator filter type for APR.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetOscillatorFilterType(zl303xx_AprOscillatorFilterTypesE oscillatorFilter);

/** zl303xx_AprGetOscillatorFilterType

   The function returns the default oscillator filter type for APR.

  Return Value: zl303xx_AprOscillatorFilterTypesE
****************************************************************************/
zl303xx_AprOscillatorFilterTypesE zl303xx_AprGetOscillatorFilterType(void);


/** zl303xx_AprSetHoldoverStability

   The function sets the oscillator holdover stability for APR X-Params.

  Parameters:
   [in] Uint32T       Oscillator Holdover Stability
****************************************************************************/
zlStatusE zl303xx_AprSetHoldoverStability(Uint32T holdoverStabilityPpt);

/** zl303xx_AprGetHoldoverStability

   The function returns the oscillator setting for APR X-Params.

  Return Value: Uint32T       Oscillator Holdover Stability
****************************************************************************/
Uint32T zl303xx_AprGetHoldoverStability(void);


/** zl303xx_AprSetSModeTimeout

   The function sets the SMode timeout for APR X-Params.

  Parameters:
   [in] sModeTimeoutSecs       Timeout (in secs)

****************************************************************************/
zlStatusE zl303xx_AprSetSModeTimeout(Uint32T sModeTimeoutSecs);

/** zl303xx_AprGetSModeTimeout

   The function returns the SMode timeout setting for APR X-Params.

  Return Value: Uint32T       sModeTimeoutSecs

****************************************************************************/
Uint32T zl303xx_AprGetSModeTimeout(void);

/** zl303xx_AprSetSModeAgeOut

*  The function sets the SMode age out for APR X-Params.

  Parameters:
   [in] sModeAgeOutSecs       Timeout (in 10s of secs)

****************************************************************************/
zlStatusE zl303xx_AprSetSModeAgeOut(Uint32T sModeAgeOutSecs);

/** zl303xx_AprGetSModeAgeOut

*  The function returns the SMode age out setting for APR
*  X-Params.

  Return Value: Uint32T       Oscillator Holdover Stability

****************************************************************************/
Uint32T zl303xx_AprGetSModeAgeOut(void);


/** zl303xx_AprSetXdslHpFlag

   The function sets the default G8261 flag of XDSL algorithm type mode for APR.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetXdslHpFlag(zl303xx_BooleanE bXdslHpFlag);

/** zl303xx_AprGetXdslHpFlag

   The function returns the default G8261 flag of XDSL algorithm type mode for APR.

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetXdslHpFlag(void);

/** zl303xx_AprSetFilterType

   The function sets the default filter type for APR.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetFilterType(zl303xx_AprFilterTypesE filter);

/** zl303xx_AprGetFilterType

   The function returns the default filter type for APR.

  Return Value: zl303xx_AprFilterTypesE
****************************************************************************/
zl303xx_AprFilterTypesE zl303xx_AprGetFilterType(void);

/** zl303xx_AprSetPktRate

   The function sets the Packet rate type for APR X-Params.

  Parameters:
   [in] zl303xx_AprPktRateE        Packet rate Type
   [in] bFwdPath              forward path or not
****************************************************************************/
zlStatusE zl303xx_AprSetPktRate(zl303xx_AprPktRateE pps, zl303xx_BooleanE bFwdPath);

/** zl303xx_AprGetPktRate

   The function returns the default packet rate setting for APR X-Params.

  Return Value: zl303xx_AprPktRateE       Packet Rate Type
****************************************************************************/
zl303xx_AprPktRateE zl303xx_AprGetPktRate(zl303xx_BooleanE bFwdPath);


/** zl303xx_AprSetTsFormat

   The function sets the default timestamp format for APR.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetTsFormat(zl303xx_AprTsFormatE tsFormat);

/** zl303xx_AprGetTsFormat

   The function returns the default timestamp format for APR.

  Return Value: zl303xx_AprTsFormatE
****************************************************************************/
zl303xx_AprTsFormatE zl303xx_AprGetTsFormat(void);


/** zl303xx_AprSet32BitTsFlag

   The function sets the default 32-bit timestamp flag for APR.

  Parameters:
   [in] zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSet32BitTsFlag(zl303xx_BooleanE b32BitTs);

/** zl303xx_AprGet32BitTsFlag

   The function returns the default value of 32-bit timestamp flag in APR.

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGet32BitTsFlag(void);

/** zl303xxexampleAprSetCorrFieldThreshold

   The function sets the default correction field threshold.

  Parameters:
   [in] zlStatusE

****************************************************************************/
zlStatusE zl303xxexampleAprSetCorrFieldThreshold(Uint64T thresholdValue);

/** zl303xxexampleAprGetCorrFieldThreshold

 * The function returns the default value of correction field
 * threshold.

  Return Value: Uint64T

****************************************************************************/
Uint64T zl303xxexampleAprGetCorrFieldThreshold(void);

/** zl303xx_AprSetType2BEnabled

   The function sets the default type2B enabled flag for APR.

  Parameters:
   [in] type2Benabled -1: use default settings, 0: force disable, 1: force enable

****************************************************************************/
Sint8T zl303xx_AprSetType2BEnabled(Sint8T type2Benabled);

/** zl303xx_AprGetType2BEnabled

   The function returns the default value of type2B enabled flag for APR.

  Return Value: Sint32T

****************************************************************************/
Sint32T zl303xx_AprGetType2BEnabled(void);

/**
  Function Name:
   zl303xx_AprSetUseReversePath

  Details:
   Set the flag to indicate if using reverse path or not

  Parameters:
   [in]    bUserRevPath

  Return Value:
   zlStatusE

*******************************************************************************/
zlStatusE zl303xx_AprSetUseReversePath(zl303xx_BooleanE bUseRevPath);

/**
  Function Name:
   zl303xx_AprGetUseReversePath

  Details:
   Get the flag of using reverse path or not

  Parameters:
   [in]    None

  Return Value:
   zl303xx_BooleanE
*******************************************************************************/
zl303xx_BooleanE zl303xx_AprGetUseReversePath(void);

/** zl303xx_AprSetHybridServer

   The function sets the default hybrid server flag for APR.

  Parameters:
   [in] zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetHybridServerFlag(zl303xx_BooleanE bHybrid);

/** zl303xx_AprGetHybridServerFlag

   The function returns the default settings of hybrid server flag in APR.

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetHybridServerFlag(void);

/** zl303xx_AprSetPacketDiscardDurationInSecFlag

   The function sets the default settings of the packet dicard duration
   in seconds flag in APR.

  Parameters:
   [in] zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetPacketDiscardDurationInSecFlag(Uint32T durationIsSec);

/** zl303xx_AprGetPacketDiscardDurationInSecFlag

   The function returns the default settings of the packet dicard duration
   in seconds flag in APR.

  Return Value: zl303xx_BooleanE
****************************************************************************/
Uint32T zl303xx_AprGetPacketDiscardDurationInSecFlag(void);

/** zl303xx_AprGetoorPeriodResetThr
   This function sets the default defaultCustomOORCounter for APR.

  Parameters:
   [in]   none

  Return Value:  ZL303XX_OK  Success.

***************************************************************************/
Sint32T zl303xx_AprGetoorPeriodResetThr(void);

/** zl303xx_AprSetoorPeriodResetThr
   This function sets the default CustomOORCounter for APR.

  Parameters:
   [in]   CustomOORCounter   Default CustomOORCounter to use for all created APR
                             streams.

  Return Value:  ZL303XX_OK  Success.

***************************************************************************/
Sint32T zl303xx_AprSetoorPeriodResetThr(Sint32T OORCounter);

/** zl303xx_AprGetPathReselectTimerLimitIns
   This function gets the default PathReselectTimerLimitIns for APR.

  Parameters:
   [in]   none

  Return Value: Sint32T default Path Reselect Timer Limit in seconds.

***************************************************************************/
Sint32T zl303xx_AprGetPathReselectTimerLimitIns(void);

/** zl303xx_AprSetPathReselectTimerLimitIns
   This function sets the default PathReselectTimerLimitIns for APR.

  Parameters:
   [in]   TimerLimit   Default PathReselectTimerLimitIns

  Return Value:  ZL303XX_OK  Success.

***************************************************************************/
Sint32T zl303xx_AprSetPathReselectTimerLimitIns(Sint32T TimerLimit);

/** zl303xx_AprSetPullInRange

   This function sets the default pull-in range for APR.

  Parameters:
   [in]   pullInRange   Default pull-in range to use for all created APR
                             devices.

  Return Value:  ZL303XX_OK  Success.
****************************************************************************/
zlStatusE zl303xx_AprSetPullInRange(zl303xx_AprPullInRangeE pullInRange);

/** zl303xx_AprGetPullInRange

   This function returns the default pull-in range for APR.

  Parameters:
      None.

  Return Value:  #zl303xx_AprPullInRangeE  Default pull-in range.
****************************************************************************/
zl303xx_AprPullInRangeE zl303xx_AprGetPullInRange(void);

/** zl303xx_AprSetEnterHoldoverGST

   The function sets the default guard soak timer to enter holdover status flag
   in APR.
   Unit: ten second

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetEnterHoldoverGST(Uint32T gstTime);

/** zl303xx_AprGetEnterHoldoverGST

   The function returns the default guard soak timer to enter holdover status flag
   in APR.

  Return Value: Uint32T   time in ten second
****************************************************************************/
Uint32T zl303xx_AprGetEnterHoldoverGST(void);

/** zl303xx_AprSetExitValidGST

   The function sets the default guard soak timer to exit Valid status flag
   (V from 1 to 0) in APR.
   Unit: ten second

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetExitValidGST(Uint32T gstTime);

/** zl303xx_AprGetExitValidGST

   The function returns the default guard soak timer to exit Valid status flag
   (V from 1 to 0) in APR.

  Return Value: Uint32T   time in ten second
****************************************************************************/
Uint32T zl303xx_AprGetExitValidGST(void);

/** zl303xx_AprSetExitLockGST

   The function sets the default guard soak timer to exit Lock status flag
   (L from 1 to 0) in APR.
   Unit: ten second

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetExitLockGST(Uint32T gstTime);

/** zl303xx_AprGetExitLockGST

   The function returns the default guard soak timer to exit Lock status flag
   (L from 1 to 0) in APR.

  Return Value: Uint32T   time in ten second
****************************************************************************/
Uint32T zl303xx_AprGetExitLockGST(void);

/** zl303xx_AprSetExitPhaseAlignGST

   The function sets the default guard soak timer to exit PhaseAlign status flag
   (PA from 1 to 0) in APR.
   Unit: ten second

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetExitPhaseAlignGST(Uint32T gstTime);

/** zl303xx_AprGetExitPhaseAlignGST

   The function returns the default guard soak timer to exit PhaseAlign status flag
   (PA from 1 to 0) in APR.

  Return Value: Uint32T   time in ten second
****************************************************************************/
Uint32T zl303xx_AprGetExitPhaseAlignGST(void);

/** zl303xx_AprSetLockMasks

*  The function sets the default masks for L2, L3 and L4 flags
*  in APR.
*
*
*           0 - unmask L2, L3 and L4 flags (use all of them);
            1 - mask L2 flag (not use L2);
            2 - mask L3 flag (not use L3);
            3 - mask both L2 and L3 flags (not use both of them)
            4 - mask only L4 (not use L4); (This is the default of Struct_init for backward compatibility).
            5 - unmask only L3 (Not use L2, L4).
            6 - unmask only L2 (Not use L3, L4)
            7 - mask all flages (Not use L2, L3 and L4 flags).


  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetLockMasks(Uint8T maskVal);

/** zl303xx_AprGetLockMasks

*  The function returns the default masks for L2, L3 and L4
*  flags in APR.
*
*
            0 - unmask L2, L3 and L4 flags (use all of them);
            1 - mask L2 flag (not use L2);
            2 - mask L3 flag (not use L3);
            3 - mask both L2 and L3 flags (not use both of them)
            4 - mask only L4 (not use L4); (This is the default of Struct_init for backward compatibility).
            5 - unmask only L3 (Not use L2, L4).
            6 - unmask only L2 (Not use L3, L4)
            7 - mask all flages (Not use L2, L3 and L4 flags).




  Return Value: Uint8T
****************************************************************************/
Uint8T zl303xx_AprGetLockMasks(void);




/** zl303xx_SetAdjustTimeMinThreshold

   The function sets the adjustTime() minimum threshold.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_SetAdjustTimeMinThreshold(Uint32T adjustTimeMinThreshold);

/** zl303xx_GetAdjustTimeMinThreshold

   The function returns the adjustTime() minimum threshold.

  Return Value: zl303xx_BooleanE
****************************************************************************/
Uint32T zl303xx_GetAdjustTimeMinThreshold(void);


/** zl303xx_SetUseAdjustTimeHybrid

   The function sets the useAdjustTimeHybrid boolean.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_SetUseAdjustTimeHybrid(Uint32T useAdjustTimeHybrid);

/** zl303xx_GetUseAdjustTimeHybrid

   The function returns the useAdjustTimeHybrid boolean.

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_GetUseAdjustTimeHybrid(void);


/** zl303xx_SetUseAdjustTimePacket

   The function sets the useAdjustTimePacket boolean.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_SetUseAdjustTimePacket(Uint32T useAdjustTimePacket);

/** zl303xx_GetUseAdjustTimePacket

   The function returns the useAdjustTimePacket boolean.

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_GetUseAdjustTimePacket(void);


/** zl303xx_SetStepTimeDetectableThr

   The function sets the stepTimeDetectableThr value.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_SetStepTimeDetectableThr(Uint32T stepTimeDetectableThr);

/** zl303xx_GetStepTimeDetectableThr

   The function returns the stepTimeDetectableThr value.

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_GetStepTimeDetectableThr(void);

/** zl303xx_SetAprDynamicPhaseCorrectionEnabled

   The function sets the AprDynamicPhaseCorrectionEnabled value.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_SetAprDynamicPhaseCorrectionEnabled(zl303xx_BooleanE AprDynamicPhaseCorrectionEnabled);

/** zl303xx_GetAprDynamicPhaseCorrectionEnabled

   The function returns the AprDynamicPhaseCorrectionEnabled value.

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_GetAprDynamicPhaseCorrectionEnabled(void);

/** zl303xx_AprSetAdjFreqDynamicAdjustmentEnable

   The function sets the AdjFreqDynamicAdjustmentEnable value.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetAdjFreqDynamicAdjustmentEnable(zl303xx_BooleanE AdjFreqDynamicAdjustmentEnable);

/** zl303xx_AprGetAdjFreqDynamicAdjustmentEnable

   The function returns the AdjFreqDynamicAdjustmentEnable value.

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetAdjFreqDynamicAdjustmentEnable(void);

/** zl303xx_AprSetHybridLockTimeTarget

   The function sets the hybridLockTimeTarget value.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetHybridLockTimeTarget(zl303xx_BooleanE hybridLockTimeTarget);

/** zl303xx_AprGetHybridLockTimeTarget

   The function returns the hybridLockTimeTarget value.

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetHybridLockTimeTarget(void);

/** zl303xx_SetAprDynamicPhaseCorrectionThr

   The function sets the defaultAprDynamicPhaseCorrectionThr value.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_SetAprDynamicPhaseCorrectionThr(Uint32T AprDynamicPhaseCorrectionThr);

/** zl303xx_GetAprDynamicPhaseCorrectionThr

   The function returns the defaultAprDynamicPhaseCorrectionThr value.

  Return Value: zl303xx_BooleanE

****************************************************************************/
Uint32T zl303xx_GetAprDynamicPhaseCorrectionThr(void);

/** zl303xx_AprSetUseNCOAssist

   The function sets the use-NCO-assist-dpll boolean

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetUseNCOAssist(zl303xx_BooleanE UseNCOAssistDpllValue);

/** zl303xx_AprGetUseNCOAssist

   The function gets the the use-NCO-assist-dpll boolean

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetUseNCOAssist(void);


/******************************************************************************
                               APR Misc start
*******************************************************************************/

/** zl303xx_AprGetClkStabDelayIterations

  The function returns the default value (0) or the
  customer-provided value of number of iterations to pass prior
  to starting to calculate the Clock Stability value.

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetClkStabDelayIterations(void);

/** zl303xx_AprSetClkStabDelayIterations

  The function overwrites the default value (0) with the
  customer-provided value of number of iterations to pass prior
  to starting to calculate the Clock Stability value. Setting
  this to non-zero allows calulation of non-active servers.
  Think of this value as seconds*.8

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprSetClkStabDelayIterations(Uint32T);

/** zl303xx_AprGetFastLockPktSelWindowSize

    The function gets the fast lock packet rate select window size

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetFastLockPktSelWindowSize(void);

/** zl303xx_AprSetFastLockPktSelWindowSize

The function sets the fast lock packet rate select window size

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetFastLockPktSelWindowSize(Uint32T PktSelWindowSize);

/** zl303xx_AprSetL4Threshold

The function sets the L4 lock indicator threshold****

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetL4Threshold(Uint32T L4ThresholdValue);

/** zl303xx_AprGetL4Threshold

*   The function gets the L4 lock indicator threshold

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetL4Threshold(void);

/** zl303xx_AprSetUseOFM

The function sets the Use OFM logic value*
*

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetUseOFM(zl303xx_BooleanE UseOFMValue);

/** zl303xx_AprGetUseOFM

*   The function gets the the Use OFM logic value

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetUseOFM(void);

/** zl303xx_AprGetFastLockTotalTimeInSecs

    The function gets the fast lock total time in seconds
*
  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetFastLockTotalTimeInSecs(void);

/** zl303xx_AprSetFastLockTotalTimeInSecs

The function sets the fast lock total time in seconds

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetFastLockTotalTimeInSecs(Uint32T TotalTimeInSecs);

/** zl303xx_AprGetHoldoverType

*   The function gets the value of HoldoverType control

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetHoldoverType(void);

/** zl303xx_AprSetHoldoverType

*   The function sets the value of HoldoverType control

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetHoldoverType(Uint32T ControlValue);

/** zl303xx_AprGetHoldoverUserValue

    This function gets the value of HoldoverUserValue.

  Return Value: Uint32T
****************************************************************************/
Sint32T zl303xx_AprGetHoldoverUserValue(void);

/** zl303xx_AprSetHoldoverUserValue

*   The function sets the value of HoldoverUserValue.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetHoldoverUserValue(Sint32T val);

/** zl303xx_AprGetAprTaskBasePeriodMs

*   The function gets the value of AprTaskBasePeriodMs

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetAprTaskBasePeriodMs(void);

/** zl303xx_AprSetAprTaskBasePeriodMs

*   The function sets the value of AprTaskBasePeriodMs

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetAprTaskBasePeriodMs(Uint32T period);

/** zl303xx_ExampleGetAprClockGroupType

*   The function gets the value of aprClockGroupType

  Return Value: zl303xx_AprClockGroupE
****************************************************************************/
zl303xx_AprClockGroupE zl303xx_ExampleGetAprClockGroupType(void);

/** zl303xx_ExampleSetAprClockGroupType

*   The function sets the value of aprClockGroupType

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_ExampleSetAprClockGroupType(zl303xx_AprClockGroupE groupType);

/** zl303xx_AprGetPslFclTaskBasePeriodMs

*   The function gets the value of PslFclTaskBasePeriodMs

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetPslFclTaskBasePeriodMs(void);

/** zl303xx_AprSetPslFclTaskBasePeriodMs

*   The function sets the value of PslFclTaskBasePeriodMs

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetPslFclTaskBasePeriodMs(Uint32T period);

/** zl303xx_AprSetDFSeedValue

*   The function sets the value of DFSeed control

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetDFSeedValue(Sint32T SValue);

/** zl303xx_AprGetDFSeedValue

*   The function gets the value of DFSeed control

  Return Value: Sint32T**
****************************************************************************/
Sint32T zl303xx_AprGetDFSeedValue(void);


/** zl303xx_AprGetCableFastLock

*   The function gets the value of bCableFastLock

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetCableFastLock(void);

/** zl303xx_AprSetCableFastLock

*   The function sets the value of bCableFastLock

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetCableFastLock(zl303xx_BooleanE bValue);

/** zl303xx_AprGetCableFastLockTimer

*   The function gets the value of CableFastLockTimer

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetCableFastLockTimer(void);

/** zl303xx_AprSetCableFastLockTimer

*   The function sets the value of CableFastLockTimer

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetCableFastLockTimer(Uint32T Value);


/** zl303xx_AprSetType2BFastlockStartupIt

*   The function sets the value of Type2BFastlockStartupIt

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BFastlockStartupIt(Uint32T Value);

/** zl303xx_AprGetType2BFastlockStartupIt

*   The function gets the value of Type2BFastlockStartupIt

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetType2BFastlockStartupIt(void);

/** zl303xx_AprSetType2BFastlockThreshold

*   The function sets the value of Type2BFastlockThreshold

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BFastlockThreshold(Uint32T Value);

/** zl303xx_AprGetType2BFastlockThreshold

*   The function gets the value of Type2BFastlockThreshold

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetType2BFastlockThreshold(void);

/** zl303xx_AprSetType2BFastlockSecondaryTriggerPhaseThreshold

*   The function sets the value of Type2BFastlockSecondaryTriggerPhaseThreshold

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BFastlockSecondaryTriggerPhaseThreshold(Uint32T Value);

/** zl303xx_AprGetType2BFastlockSecondaryTriggerPhaseThreshold

*   The function gets the value of Type2BFastlockSecondaryTriggerPhaseThreshold

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetType2BFastlockSecondaryTriggerPhaseThreshold(void);

/** zl303xx_AprSetType2BFastlockSecondaryTriggerLimitSec

*   The function sets the value of Type2BFastlockSecondaryTriggerLimitSec

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BFastlockSecondaryTriggerLimitSec(Uint32T Value);

/** zl303xx_AprGetType2BFastlockSecondaryTriggerLimitSec

*   The function gets the value of Type2BFastlockSecondaryTriggerLimitSec

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetType2BFastlockSecondaryTriggerLimitSec(void);

/** zl303xx_AprGetType2BFastlockTypeRatio

*   The function gets the value of Type2BFastlockTypeRatio

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetType2BFastlockTypeRatio(void);

/** zl303xx_AprSetType2BFastlockTypeRatio

*   The function sets the value of Type2BFastlockTypeRatio

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BFastlockTypeRatio(Uint32T Value);

/** zl303xx_AprGetType2BFastLockSimpleCoef

*   The function gets the value of Type2BFastLockSimpleCoef

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetType2BFastLockSimpleCoef(void);

/** zl303xx_AprSetType2BFastLockSimpleCoef

*   The function sets the value of Type2BFastLockSimpleCoef

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BFastLockSimpleCoef(Uint32T Value);

/** zl303xx_AprGetType2BFastLockPSL

*   The function gets the value of Type2BFastLockPSL

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetType2BFastLockPSL(void);

/** zl303xx_AprSetType2BFastLockPSL

*   The function sets the value of Type2BFastLockPSL

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BFastLockPSL(Uint32T Value);

/** zl303xx_AprGetType2BFastLockFreqEstInterval

*   The function gets the value of Type2BType2BFastLockFreqEstInterval

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetType2BFastLockFreqEstInterval(void);

/** zl303xx_AprSetType2BFastLockFreqEstInterval

*   The function sets the value of Type2BFastLockFreqEstInterval

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BFastLockFreqEstInterval(Uint32T Value);

/** zl303xx_AprGetType2BMaxFastLockTargetTime

*   The function gets the value of Type2BMaxFastLockTargetTime

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetType2BMaxFastLockTargetTime(void);

/** zl303xx_AprSetType2BMaxFastLockTargetTime

*   The function sets the value of Type2BMaxFastLockTargetTime

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BMaxFastLockTargetTime(Uint32T Value);

/** zl303xx_AprGetType2BLockedPhaseSlopeLimit

*   The function gets the value of Type2BLockedPhaseSlopeLimit

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetType2BLockedPhaseSlopeLimit(void);

/** zl303xx_AprSetType2BLockedPhaseSlopeLimit

*   The function sets the value of Type2BLockedPhaseSlopeLimit

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BLockedPhaseSlopeLimit(Uint32T Value);

/** zl303xx_AprGetType2BMinFastLockTargetTime

*   The function gets the value of Type2BMinFastLockTargetTime

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetType2BMinFastLockTargetTime(void);

/** zl303xx_AprSetType2BMinFastLockTargetTime

*   The function sets the value of Type2BMinFastLockTargetTime

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BMinFastLockTargetTime(Uint32T Value);

/** zl303xx_AprGetType2BfastLockBypassPSLFCL

The function GETS the Type2BfastLockBypassPSLFCL boolean

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetType2BfastLockBypassPSLFCL(void);

/** zl303xx_AprSetType2BfastLockBypassPSLFCL

The function SETS the Type2BfastLockBypassPSLFCL boolean

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BfastLockBypassPSLFCL(zl303xx_BooleanE Type2BfastLockBypassPSLFCL);

/** zl303xx_AprGetType2BfastLockOnMonitorToActive

The function GETS the Type2BfastLockOnMonitorToActive boolean

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetType2BfastLockOnMonitorToActive(void);

/** zl303xx_AprSetType2BfastLockOnMonitorToActive

The function SETS the Type2BfastLockOnMonitorToActive boolean

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetType2BfastLockOnMonitorToActive(zl303xx_BooleanE Type2BfastLockOnMonitorToActive);

/** zl303xx_AprSetOutlierTimerValue

*   The function sets the value of OutlierTimer

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetOutlierTimerValue(Sint32T OValue);

/** zl303xx_AprGetOutlierTimerValue

*   The function gets the value of OutlierTimer control

  Return Value: Sint32T*
****************************************************************************/
Sint32T zl303xx_AprGetOutlierTimerValue(void);


/** zl303xx_AprGetClkInvalidCntr

*   The function gets the value of ClkInvalidCntr

  Return Value: Sint32T
****************************************************************************/
Sint32T zl303xx_AprGetClkInvalidCntr(void);

/** zl303xx_AprSetClkInvalidCntr

*   The function sets the value of ClkInvalidCntr

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetClkInvalidCntr(Sint32T CValue);

/** zl303xxexampleAprGetfastClkInvalidCounts

*   The function gets the value of fastClkInvalidCounts

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xxexampleAprGetfastClkInvalidCounts(void);

/** zl303xxexampleAprSetfastClkInvalidCounts

*   The function sets the value of fastClkInvalidCounts

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xxexampleAprSetfastClkInvalidCounts(Uint32T CValue);

/** zl303xx_AprGetEnableFastNetOutageDetection

 *  The function gets the value of
 *  enableFastNetOutageDetectionValue

  Return Value: zl303xx_BooleanE*
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetEnableFastNetOutageDetection(void);

/** zl303xx_AprSetEnableFastNetOutageDetection

 *The function sets the value of
 *enableFastNetOutageDetectionValue

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetEnableFastNetOutageDetection(zl303xx_BooleanE EValue);

/** zl303xx_AprGetLegacyMonitoringModeControl

 *  The function gets the value of
 *  legacyMonitoringModeControl

  Return Value: zl303xx_BooleanE*
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetLegacyMonitoringModeControl(void);

/** zl303xx_AprSetLegacyMonitoringModeControl

 *The function sets the value of
 *legacyMonitoringModeControl

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetLegacyMonitoringModeControl(zl303xx_BooleanE EValue);

/** zl303xx_AprGetFastLockBW

The function gets the fast lock BW

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetFastLockBW(void);

/** zl303xx_AprSetFastLockBW

The function sets  the fast lock BW

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetFastLockBW(zl303xx_AprFilterTypesE filter_type);


/** zl303xx_AprGetpllStepDetectCalcValue

*   The function gets the value of pllStepDetectCalcValue

  Return Value: zl303xx_AprStepDetectCalcE
****************************************************************************/
zl303xx_AprStepDetectCalcE zl303xx_AprGetpllStepDetectCalcValue(void);

/** zl303xx_AprSetpllStepDetectCalcValue

*   The function sets the value of pllStepDetectCalcValue

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetpllStepDetectCalcValue(zl303xx_AprStepDetectCalcE SValue);


/******************************************************************************
                                 1Hz start
*******************************************************************************/

extern zl303xx_BooleanE default1HzEnabledFlag;
extern zl303xx_BooleanE startupTimeInit;


/** zl303xx_AprSet1HzEnabled

   The function sets the default 1Hz enabled flag for APR.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSet1HzEnabled(zl303xx_BooleanE enable1Hz);

/** zl303xx_AprGet1HzEnabled

   The function returns the default 1Hz enabled flag for APR.

  Return Value: zlStatusE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGet1HzEnabled(void);


/** zl303xx_AprSet1HzRealignmentType

   The function sets the default 1Hz realignment type for APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSet1HzRealignmentType(zl303xx_Apr1HzRealignmentTypeE realignmentType);

/** zl303xx_AprGet1HzRealignmentType

   The function returns the default 1Hz realignment type for APR.

  Return Value: zl303xx_Apr1HzRealignmentTypeE

****************************************************************************/
zl303xx_Apr1HzRealignmentTypeE zl303xx_AprGet1HzRealignmentType(void);


/** zl303xx_AprSetPerPacketAdjType

   The function sets the default per packet realignment adjustment type.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetPerPacketAdjType(zl303xx_AprPerPacketAdjTypeE perPacketAdjustmentType);

/** zl303xx_AprGetPerPacketAdjType

   The function returns the default per packet realignment adjustment method.

  Return Value: zl303xx_AprPerPacketAdjTypeE

****************************************************************************/
zl303xx_AprPerPacketAdjTypeE zl303xx_AprGetPerPacketAdjType(void);


/** zl303xx_AprSet1HzRealignmentInterval

   The function sets the default 1Hz realignment interval for APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSet1HzRealignmentInterval(Uint32T realignmentInterval);

/** zl303xx_AprGet1HzRealignmentInterval

   The function returns the default 1Hz realignment interval for APR.

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGet1HzRealignmentInterval(void);

/** zl303xx_AprSet1HzFilterBandwidth

   The function sets the default 1Hz filter bandwidth for APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSet1HzFilterBandwidth(Uint32T filterBandwidth);


/** zl303xx_AprSetPCLFCLStaticOffset

   The function sets the default defaultStaticOffset for APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetPCLFCLStaticOffset(Sint32T StaticOffset);

/** zl303xx_AprGet1HzRealignmentInterval

   The function returns the default defaultStaticOffset for APR.

  Return Value: Sint32T*

****************************************************************************/
Sint32T zl303xx_AprGetPCLFCLStaticOffset(void);

/** zl303xx_AprGet1HzFilterBandwidth

   The function returns the default 1Hz filter bandwidth for APR.

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGet1HzFilterBandwidth(void);

/** zl303xx_AprSet1HzFilterWriteInterval

   The function sets the default 1Hz filter write interval for APR.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSet1HzFilterWriteInterval(Uint32T filterWriteInterval);

/** zl303xx_AprGet1HzFilterWriteInterval

   The function returns the default 1Hz filter write interval for APR.

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGet1HzFilterWriteInterval(void);

/** zl303xx_AprSet1HzStartupTimeInit

   The function sets the default 1Hz startupTime flag for APR. The default is normally
   ZL303XX_TRUE which means there is startup timer which affects among other things, the
   realignment interval.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSet1HzStartupTimeInit(zl303xx_BooleanE init1HzStartupTime);

/** zl303xx_AprGet1HzStartupTimeInit

   The function gets the default 1Hz startupTime flag for APR.

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGet1HzStartupTimeInit(void);

/** zl303xx_AprSetUseLegacyStreamStartUp

    The function sets the global for old startup behaviour settings flag

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetUseLegacyStreamStartUp(zl303xx_BooleanE enabled);

/** zl303xx_AprGetUseLegacyStreamStartUp

    The function gets the global for old startup behaviour settings flag

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetUseLegacyStreamStartUp(void);
/** zl303xx_AprSetAllow1HzAdjustmentsInHoldover

    The function sets the global for allowing 1Hz adjustments to take place
    while in holdover

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetAllow1HzAdjustmentsInHoldover(zl303xx_BooleanE enabled);
/** zl303xx_AprGetAllow1HzAdjustmentsInHoldover

    The function gets the global for allowing 1Hz adjustments to take place
    while in holdover

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetAllow1HzAdjustmentsInHoldover(void);


/** zl303xx_AprSetOnlyAllowSteps

    The function sets the global for allowing 1Hz adjustments to only apply
    setTime() and stepTime() operations

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetOnlyAllowSteps(zl303xx_BooleanE enabled);

/** zl303xx_AprGetOnlyAllowSteps

    The function gets the global for allowing 1Hz adjustments to only apply
    setTime() and stepTime() operations

  Return Value: zl303xx_BooleanE

****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetOnlyAllowSteps(void);


/** zl303xx_AprSetStepTimePollingIntervalMs

    The function sets the global that enables polling for the end of step time.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetStepTimePollingIntervalMs(zl303xx_BooleanE enabled);

/** zl303xx_AprGetStepTimePollingIntervalMs

    The function gets the global that enables polling for the end of step time.

  Return Value: Uint32T

****************************************************************************/
Uint32T zl303xx_AprGetStepTimePollingIntervalMs(void);


/** zl303xx_AprSetAdjSize1HzFCL

    The function sets an element in the FCL size ranges for 1Hz.

    When 1Hz makes an adjustment, the adjustment size is compared against each
    element in defaultAdjSize1HzFCL[]. When the smallest element that is still
    larger than the adjustment is found, the corresponding FCL value in
    defaultFCL_1Hz[] is used.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetFCL(Uint32T idx, Uint32T adjSize, Uint32T psl);

/** zl303xx_AprGetFCL

    The function gets the FCL size ranges for 1Hz.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprGetFCL(Uint32T idx, Uint32T *adjSize, Uint32T *psl );

/** zl303xx_AprSetAdjSize1HzAdjScaling

    The function sets an element in the adjustment scaling factor size ranges for 1Hz.

    When 1Hz makes an adjustment, the adjustment size is compared against each
    element in defaultAdjSize1HzAdjScaling[]. When the smallest element that is still
    larger than the adjustment is found, the corresponding adjustment scaling factor value in
    defaultAdjScaling_1Hz[] is used.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprSetAdjScaling(Uint32T idx, Uint32T adjSize, Uint32T adjScaling);

/** zl303xx_AprGetAdjScaling

    The function gets the adjustment scaling factor size for 1Hz.

  Return Value: zlStatusE

****************************************************************************/
zlStatusE zl303xx_AprGetAdjScaling(Uint32T idx, Uint32T *adjSize, Uint32T *adjScaling );

/** zl303xx_AprSetFastLock1HzEnable

The function sets the 1 Hz fast lock enable flag

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetFastLock1HzEnable(zl303xx_BooleanE set);

/** zl303xx_AprGetFastLock1HzEnable

The function gets the 1 Hz fast lock enable flag

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetFastLock1HzEnable(void);

/** zl303xx_AprSetFastLock1HzInterval

The function sets the 1 Hz fast lock adjustment interval in number of adjustments

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetFastLock1HzInterval(Uint32T set);

/** zl303xx_AprGetFastLock1HzInterval

The function gets the 1 Hz fast lock adjustment interval in number of adjustments

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetFastLock1HzInterval(void);

/** zl303xx_AprSetFastLock1HzTotalPeriod

The function sets the 1 Hz fast lock total period in seconds

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetFastLock1HzTotalPeriod(Uint32T set);

/** zl303xx_AprGetFastLock1HzTotalPeriod

The function gets the 1 Hz fast lock total period in seconds

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetFastLock1HzTotalPeriod(void);

/** zl303xx_AprSetFastLock1HzDelay

The function sets the 1 Hz fast lock delay interval in seconds

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetFastLock1HzDelay(Uint32T set);

/** zl303xx_AprGetFastLock1HzDelay

The function gets the 1 Hz fast lock delay interval in seconds

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetFastLock1HzDelay(void);

/** zl303xx_AprSetStepTimeAdjustTimeThreshold

The function sets the Step Time Adjust Time Threshold

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetStepTimeAdjustTimeThreshold(Uint32T set);

/** zl303xx_AprGetStepTimeAdjustTimeThreshold

The function gets the Step Time Adjust Time Threshold

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetStepTimeAdjustTimeThreshold(void);

/** zl303xx_AprSetStepTimeAdjustFreqThreshold

The function sets the Step Time Adjust Frequency Threshold

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetStepTimeAdjustFreqThreshold(Uint32T set);

/** zl303xx_AprGetStepTimeAdjustFreqThreshold

The function gets the Step Time Adjust Frequency Threshold

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetStepTimeAdjustFreqThreshold(void);

/** zl303xx_AprSetMaxAdjFreqTime

The function sets the maximum adjust frequency adjustment period in seconds.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetMaxAdjFreqTime(Uint32T maxAdjFreqTime);

/** zl303xx_AprGetMaxAdjFreqTime

The function gets the maximum adjust frequency adjustment period in seconds.

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetMaxAdjFreqTime(void);

/** zl303xx_AprSetStepTimeMaxTimePerAdjust

The function sets the maximum adjust step time adjustment period in seconds.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetStepTimeMaxTimePerAdjust(Uint32T stepTimeMaxAdjust);

/** zl303xx_AprGetStepTimeMaxTimePerAdjust

The function gets the maximum adjust step time adjustment period in seconds.

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetStepTimeMaxTimePerAdjust(void);

/** zl303xx_AprGetPrintStructDebug
 *
 * The function gets the APR structure debug printing option enable value.
 *
 *  Return Value: zl303xx_BooleanE
 * ****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetPrintStructDebug(void);

/** zl303xx_AprSetPrintStructDebug
 *
 * The function sets the APR structure debug printing option enable value.
 *
 *  Return Value: zlStatusE
 * ****************************************************************************/
zlStatusE zl303xx_AprSetPrintStructDebug(zl303xx_BooleanE bEnable);

/** zl303xx_AprGetFirstPacketTreatment

The function gets first packet treatment for Set/Step time operations/

  Return Value: zl303xxfirstPacketTreatmentE
****************************************************************************/
zl303xxfirstPacketTreatmentE zl303xx_AprGetFirstPacketTreatment(void);

/** zl303xx_AprSetFirstPacketTreatment

The function sets first packet treatment for Set/Step time operations.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetFirstPacketTreatment(zl303xxfirstPacketTreatmentE firstPacketTreatmentVal);

/** zl303xx_AprGetAprStepTimeOptions

The function gets an ENUM for additional StepTime options.

  Return Value: zl303xx_AprStepTimeOptionsE
****************************************************************************/
zl303xx_AprStepTimeOptionsE zl303xx_AprGetAprStepTimeOptions(void);

/** zl303xx_AprSetAprStepTimeOptions

The function sets an ENUM for additional StepTime options.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetAprStepTimeOptions(zl303xx_AprStepTimeOptionsE additionalStepTimeOptions);

/** zl303xx_AprGetUseLegacyJumpTime

The function gets global APR useLegacyJumpTime configuration

  Return Value: Uint32T
****************************************************************************/
zl303xx_BooleanE zl303xx_AprGetUseLegacyJumpTime(void);

/** zl303xx_AprSetUseLegacyJumpTime

The function sets global APR useLegacyJumpTime configuration

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetUseLegacyJumpTime(zl303xx_BooleanE useLegacyJumpTime);

/** exampleAprGetJumpLoggingThresholdNs

The function gets global APR jumpLoggingThresholdNs configuration

  Return Value: Uint32T
****************************************************************************/
Uint32T exampleAprGetJumpLoggingThresholdNs(void);

/** exampleAprSetJumpLoggingThresholdNs

The function sets global APR jumpLoggingThresholdNs configuration

  Return Value: zlStatusE
****************************************************************************/
zlStatusE exampleAprSetJumpLoggingThresholdNs(Uint32T thresholdNs);

/** zl303xx_AprGetCguStateMachineReportingPeriodMs

The function gets global APR state machine long interrupt interval.

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetCguStateMachineReportingPeriodMs(void);

/** zl303xx_AprSetCguStateMachineReportingPeriodMs

The function sets global APR state machine long interrupt interval.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetCguStateMachineReportingPeriodMs(Uint32T periodMs);

/** zl303xx_AprGetAprStateMachinePeriodMs

The function gets global APR algorithm long interrupt interval.

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_AprGetAprStateMachinePeriodMs (void);

/** zl303xx_AprSetAprStateMachinePeriodMs

The function sets global APR algorithm long interrupt interval.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_AprSetAprStateMachinePeriodMs (Uint32T periodMs);

/** zl303xx_ExampleAprGetLogLevel

The function gets global APR log level configuration

  Return Value: Uint8T
****************************************************************************/
Uint8T zl303xx_ExampleAprGetLogLevel(void);

/** zl303xx_ExampleAprSetLogLevel

The function sets global APR log level configuration

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_ExampleAprSetLogLevel(Uint8T logLevel);

/** zl303xx_ExampleAprGetQIFTaskPriority

The function GETS the global APR QIF task priority

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_ExampleAprGetQIFTaskPriority(void);
/** zl303xx_ExampleAprSetQIFTaskPriority

The function SETS the global APR QIF task priority

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_ExampleAprSetQIFTaskPriority(Uint32T priorityLevel);
/** zl303xx_ExampleAprGetNotifyTaskPriority

The function GETS the global APR Notify task priority

  Return Value: Uint32T
****************************************************************************/
Uint32T zl303xx_ExampleAprGetNotifyTaskPriority(void);
/** zl303xx_ExampleAprSetQIFTaskPriority

The function SETS the global APR Notify task priority

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_ExampleAprSetNotifyTaskPriority(Uint32T priorityLevel);



/** zl303xx_ExampleAprGetAdjModifierPositiveStepTimeOnly

The function GETS the global APR adjustment modifier positive stepTime only override.

  Return Value: zl303xx_BooleanE
****************************************************************************/
zl303xx_BooleanE zl303xx_ExampleAprGetAdjModifierPositiveStepTimeOnly(void);

/** zl303xx_ExampleAprSetAdjModifierPositiveStepTimeOnly

The function SETS the global APR adjustment modifier positive stepTime only override.

  Return Value: zlStatusE
****************************************************************************/
zlStatusE zl303xx_ExampleAprSetAdjModifierPositiveStepTimeOnly(zl303xx_BooleanE bPositiveStepTimeOnly);

#endif


#ifdef __cplusplus
}
#endif

#endif
