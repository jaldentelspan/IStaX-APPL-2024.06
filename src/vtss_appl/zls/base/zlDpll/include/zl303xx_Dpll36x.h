

/*******************************************************************************
*
*  $Id: 5829f97c957ab0b7024bbe9785d3bfb3bdfb63de
*
*  Copyright 2006-2019 Microchip/Microsemi Semiconductor Limited.
*  All rights reserved.
*
*  Module Description:
*     ZL3036x DPLL function and function bindings for APR.
*
*******************************************************************************/

#ifndef ZL303XX_NEW_DPLL_36X_H_
#define ZL303XX_NEW_DPLL_36X_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Error.h"
#include "zl303xx_AddressMap36x.h"
#include "zl303xx_DeviceSpec.h"
#include "zl303xx_Ptsf.h"

/*****************   DEFINES   ************************************************/
/* Scaled dco count per ppm for 36x. Each 1 count of the 36x register is:
      x           1
   ------- = -----------    => x = 1099511.627776
     2^40    1000000(ppm)
*/
#define ZLS3036X_DCO_COUNT_PER_PPM 1099512

#define ZLS3036X_NCO_REFSW_CTRL_REG(dpllId)   ZL303XX_MAKE_MEM_ADDR_36X(0x2D8 + (dpllId), ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_NCO_REFSW_CTRL_MASK          0x01


/*****************   DATA TYPES   *********************************************/
/* Phase step control bitfield options */
typedef enum
{
   ZLS3036X_PHASE_STEP_CTRL_READY = 0x00,
   ZLS3036X_PHASE_STEP_CTRL_RESET = 0x04,
   ZLS3036X_PHASE_STEP_CTRL_READ  = 0x08,
   ZLS3036X_PHASE_STEP_CTRL_WRITE = 0x0C,

   ZLS3036X_PHASE_STEP_CTRL_MASK  = 0x0C
} ZLS3036X_PhaseStepCtrlE;



/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/
extern Uint8T isStatusReg[0x300];

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
zlStatusE zl303xx_Dpll36xSetClearSyncFailFlag(zl303xx_ParamsS *zl303xx_Params, Uint32T synthId, zl303xx_BooleanE val);
Sint32T zl303xx_Dpll36xGetHwLockStatus(void *hwParams, Sint32T *lockStatus);
Sint32T zl303xx_Dpll36xGetHwManualHoldoverStatus(void *hwParams, Sint32T *status);
zlStatusE zl303xx_Dpll36xMitigationEnabledSet(void *hwParams, zl303xx_BooleanE mitigationEnabled);
Sint32T zl303xx_Dpll36xGetHwManualFreerunStatus(void *hwParams, Sint32T *status);
Sint32T zl303xx_Dpll36xGetHwSyncInputEnStatus(void *hwParams, Sint32T *en);
Sint32T zl303xx_Dpll36xGetHwOutOfRangeStatus(void *hwParams, Sint32T *oor);
zlStatusE zl303xx_Dpll36xStickyLockSet(void *zl303xx_Params,
                                       ZLS3036X_StickyLockE lock);
zlStatusE zl303xx_Dpll36xModeGet(void *hwParams, ZLS3036X_DpllModeE *mode);
zlStatusE zl303xx_Dpll36xModeSet(void *hwParams, ZLS3036X_DpllModeE mode);
zlStatusE zl303xx_Dpll36xHWModeSet(void *hwParams, ZLS3036X_DpllModeE mode, Uint8T refId, Sint32T freqOffsetToSetInppt);


zlStatusE zl303xx_Dpll36xParamsInit(void *hwParams);

zlStatusE zl303xx_Dpll36xTieClearGet(void *hwParams, zl303xx_BooleanE *tieClear);
zlStatusE zl303xx_Dpll36xTieClearSet(void *hwParams, zl303xx_BooleanE tieClear);

zlStatusE zl303xx_Dpll36xCheckRefSyncPair(void *hwParams, Uint32T refId, Uint32T *syncId, zl303xx_BooleanE *bRefSyncPair);

zlStatusE zl303xx_Dpll36xGetUserHitlessCompensationParams(void *hwParams,Uint32T *PhaseSlopeLimit,Uint32T *Bandwidth,
                                                    Uint32T *CustomBandwidth,zl303xx_BooleanE *FastLock,zl303xx_BooleanE *TieClear);
zlStatusE zl303xx_Dpll36xGetTransientHitlessCompensationParams(void *hwParams,Uint32T *PhaseSlopeLimit,Uint32T *Bandwidth,
                                                    Uint32T *CustomBandwidth,zl303xx_BooleanE *FastLock,Uint32T *DelayPeriod,Uint32T *RefSyncDelayPeriod);
zlStatusE zl303xx_Dpll36xSetUserHitlessCompensationParams(void *hwParams,Uint32T PhaseSlopeLimit,Uint32T Bandwidth,
                                                    Uint32T CustomBandwidth,zl303xx_BooleanE FastLock,zl303xx_BooleanE TieClear);
zlStatusE zl303xx_Dpll36xSetTransientHitlessCompensationParams(void *hwParams,Uint32T PhaseSlopeLimit,Uint32T Bandwidth,
                                                    Uint32T CustomBandwidth,zl303xx_BooleanE FastLock, Uint32T DelayPeriod, Uint32T RefSyncDelayPeriod);

zlStatusE zl303xx_Dpll36xFastLockSet(void *hwParams, zl303xx_BooleanE fastLock);
zlStatusE zl303xx_Dpll36xFastLockGet(void *hwParams, zl303xx_BooleanE *fastLock);
zlStatusE zl303xx_Dpll36xModeSwitchFastLockDisableSet(void *hwParams, zl303xx_BooleanE fastLock);

zlStatusE zl303xx_Dpll36xPhaseSlopeLimitSet(void *hwParams, Uint32T PSL);
zlStatusE zl303xx_Dpll36xPhaseSlopeLimitGet(void *hwParams, Uint32T *PSL);

zlStatusE zl303xx_Dpll36xBandwidthSet(void *hwParams, Uint32T PSL);
zlStatusE zl303xx_Dpll36xBandwidthGet(void *hwParams, Uint32T *PSL);

zlStatusE zl303xx_Dpll36xBandwidthCustomSet(void *hwParams, Uint32T PSL);
zlStatusE zl303xx_Dpll36xBandwidthCustomGet(void *hwParams, Uint32T *PSL);

zlStatusE  zl303xx_Dpll36xAccuInputPhaseErrorSet(void *hwParams, Uint32T pllId, Sint32T tieNs);
zlStatusE  zl303xx_Dpll36xAccuInputPhaseErrorGet(void *hwParams, Uint32T pllId, Sint32T *tieNs);

zlStatusE zl303xx_Dpll36xCurrRefSet(void *hwParams,  Uint8T refId);
zlStatusE zl303xx_Dpll36xCurrRefGet(void *hwParams, Uint32T *refId);

zlStatusE zl303xx_Dpll36xPullInRangeSet(void *hwParams, Uint32T PullInRange);

zlStatusE zl303xx_Ref36xFreqDetectedGet(void *zl303xx_Params,
                                        Uint32T refId, Uint32T *freqHz);
zlStatusE zl303xx_Ref36xMonBitsClear(void *hwParams, Uint32T refId);
zlStatusE zl303xx_Ref36xPhaseMemLimitSet(void *hwParams, Uint32T refId,
                                         Uint32T limitUs);

zlStatusE zl303xx_Dpll36xPhaseStepMaskGet(void *hwParams, Uint32T synthId,
                                          Uint32T *mask);
zlStatusE zl303xx_Dpll36xPhaseStepMaskSet(void *hwParams, Uint32T synthId,
                                          Uint32T mask);

zlStatusE zl303xx_Dpll36xGetClockFreq(void *hwParams,
                                    Uint32T synthId,
                                    ZLS3036X_PostDivE postDiv,
                                    Uint32T *freqHz);
zlStatusE zl303xx_Dpll36xGetClockPeriodLcm(void *hwParams, Uint32T postDivBitmap,
                                           Uint32T *lcmNs);

zlStatusE zl303xx_Dpll36xTieCtrlReady(void *hwParams, zl303xx_BooleanE *ready);
zlStatusE zl303xx_Dpll36xMtieSnap(void *hwParams);
zlStatusE zl303xx_Dpll36xTieRead(void *hwParams, Uint32T timeoutMs, Sint32T *tieNs);
zlStatusE zl303xx_Dpll36xTieCtrlSet(void *hwParams, ZLS3036X_DpllTieCtrlE oper);

zlStatusE zl303xx_Dpll36xOverridePllPriorMode(void *hwParams, ZLS3036X_DpllModeE overrridePllPriorMode, zl303xx_RefIdE refId);

zlStatusE zl303xx_Dpll36xConvertVcoCyclesToNs(void *hwParams, Uint32T synthId, Sint32T vcoCycles, Sint32T *resultInNs);
zlStatusE zl303xx_Dpll36xRequestFreqRead(void *hwParams, Uint32T pllId, ZLS3036X_MemPartE memPart);
zl303xx_BooleanE zl303xx_Dpll36xInNCOMode(void *hwParams);

zlStatusE zl303xx_Dpll36xOutputPhaseStepStatusClear(void *hwParams, Uint32T postDivBitmap);
zlStatusE zl303xx_Dpll36xOutputPhaseStepStatusGet(void *hwParams, Uint32T postDivBitmap,
                                                Uint32T *completeBitmap);
Sint32T zl303xx_Dpll36xOutputPhaseStepWrite(void *hwParams, Uint32T pllId, Sint32T deltaTime, zl303xx_BooleanE inCycles,
                                          Uint32T clockFreq);
zlStatusE zl303xx_Dpll36xInputPhaseErrorWrite(void *hwParams, Sint32T tieNs, zl303xx_BooleanE snapAlign);
zlStatusE zl303xx_Dpll36xInputPhaseErrorRead(void *hwParams, Uint32T timeoutMs, Sint32T* tieNs);
zlStatusE zl303xx_Dpll36xMaxPhaseStepMargin(void *hwParams, Uint32T margin);
Sint32T zl303xx_Dpll36xRequestOutputPhasePostDivs(void *hwParams, Uint32T pllId, Uint32T *synthMask,
                                                Uint32T *postDivHold, Uint32T *synthPostDiv);
Sint32T zl303xx_Dpll36xGetSynthMask(void *hwParams, Uint32T pllId, Uint32T *synthMask);
zlStatusE zl303xx_Dpll36xWaitForPhaseCtrlReady(void *hwParams);

zlStatusE zl303xx_Dpll36xOutputPhaseErrorClear(void *hwParams, Uint32T pllId, Sint32T phaseToRemove,
                                                Uint32T clockFreq);
zlStatusE zl303xx_Dpll36xOutputPhaseStepRead(void *hwParams, Uint32T synthId, Uint32T clockFreq,
                                            Sint32T *phaseInNs, Sint32T *phaseInFreqClockCycles, Sint32T *accumPhase);
Sint32T zl303xx_Dpll36xjumpActiveCGU(void* hwParams, Uint32T pllId, Uint64S seconds,
                                 Uint32T nanoSeconds, zl303xx_BooleanE bBackwardAdjust);

Sint32T zl303xx_Dpll36xSetFreq(void *hwParams, Sint32T freqOffsetUppm);
Uint64S zl303xx_Dpll36xSpursSuppress(Uint16T oscType, Uint8T spursReg, Uint64S dfOffsetReg);

zlStatusE zl303xx_GetRefSyncPairFailStatus(void *hwParams, Uint32T inputRefId, zl303xx_UpDownPtsfE *refStatus);

zlStatusE zl303xx_Dpll36xSetRefSyncPair(
        void *hwParams,
        Uint32T refId,
        Uint32T syncId
        );
zlStatusE zl303xx_Dpll36xAccuStepTimePhase1588HostRegGet(
        void *hwParams,
        Uint32T pllId,
        Sint32T *phaseNs
        );
zlStatusE zl303xx_Dpll36xAccuStepTimePhase1588HostRegSet(
        void *hwParams,
        Uint32T pllId,
        Sint32T phaseNs
        );
zlStatusE zl303xx_Dpll36xReset1588HostRegisters(
        void *hwParams
        );
Sint32T zl303xx_Dpll36xStickyReadMon(void *hwParams, Uint32T refId, Uint32T encodedReg, Uint32T mask, Uint32T *data);
zlStatusE zl303xx_Dpll36xGetAlignedPhaseStep(void *hwParams,
                                                   Uint32T synthId,
                                                   Sint32T stepNs,
                                                   Uint32T *stepReg,
                                                   Sint64T *stepVco);


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */

