

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
*     ZL3073X DPLL Dynamic Operations prototypes.
*
*******************************************************************************/

#ifndef ZL303XX_DPLL_73X_DYNAMIC_OP_H_
#define ZL303XX_DPLL_73X_DYNAMIC_OP_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Dpll73xGlobal.h"

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

/* Freq. */
Sint32T zl303xx_Dpll73xSetFreq(void *hwParams, Sint32T freqOffsetUppm);
Sint32T zl303xx_Dpll73xGetFreq(void *hwParams, Sint32T *freqOffsetInppt,
                             ZLS3073X_MemPartE memPart);
Sint32T zl303xx_Dpll73xGetDeviceFreq(void *hwParams, Sint32T *freqOffsetInppt,
                                   ZLS3073X_MemPartE memPart);
zlStatusE zl303xx_Dpll73xRequestFreqRead(void *hwParams, ZLS3073X_MemPartE memPart);
zlStatusE zl303xx_Dpll73xWaitForDfCtrlReadSem(void *hwParams);


/* Phase */
zlStatusE zl303xx_Dpll73xOutputPhaseStepStatusGet(void *hwParams, Uint32T *complete);
zlStatusE zl303xx_Dpll73xWaitForPhaseCtrlReady(void *hwParams);
zlStatusE zl303xx_Dpll73xModifyStepTimeNs(void *hwParams, Sint32T deltaTime,
                                        Sint32T *modDeltaTime);
zlStatusE zl303xx_Dpll73xOutputPhaseStepRead(void *hwParams, Uint32T clockFreq,
                                           Sint32T *phaseInFreqClockCycles, Sint32T *accumPhase);
Sint32T zl303xx_Dpll73xOutputPhaseStepWrite(void *hwParams, Sint32T deltaTime,
                                          zl303xx_BooleanE inCycles, Uint32T clockFreq);
Sint32T zl303xx_Dpll73xGetStepTimeActive(void *hwParams,  zl303xx_BooleanE *stepActive);
zlStatusE zl303xx_Dpll73xStepTimePhaseGet(void *hwParams, Sint32T *tieNs);
zlStatusE zl303xx_Dpll73xAccuStepTimePhase1588HostRegSet(void *hwParams, Sint32T tieNs);
zlStatusE zl303xx_Dpll73xAccuStepTimePhase1588HostRegGet(void *hwParams, Sint32T *tieNs);
zlStatusE zl303xx_Dpll73xAccuInputPhaseError1588Get(void *hwParams, Sint32T *tieNs);
zlStatusE zl303xx_Dpll73xAccuInputPhaseError1588HostRegSet(void *hwParams, Sint32T tieNs);
zlStatusE zl303xx_Dpll73xAccuInputPhaseError1588HostRegGet(void *hwParams, Sint32T *tieNs);
/* Redundancy */
Sint32T zl303xx_Dpll73xJumpActiveCGU(void* hwParams, Uint64S seconds,
                                   Uint32T nanoSeconds, zl303xx_BooleanE bBackwardAdjust);

/* Time-Of-Day (ToD) */
zlStatusE zl303xx_Dpll73xReset1588HostRegisters(void *hwParams);
zlStatusE zl303xx_Dpll73xInputPhaseErrorWrite(void *hwParams, Sint32T tieNs,
                                            zl303xx_BooleanE snapAlign);
zlStatusE zl303xx_Dpll73xRegisterStepDoneFunc(void *hwParams,
                                            hwFuncPtrTODDone stepDoneFuncPtr);
zlStatusE zl303xx_Dpll73xToDWaitForSpecificRollover(void *hwParams, Uint32T timeoutMs,
                                                  ZLS3073X_TODReadTypeE readType, Sint32T initNs,
                                                  Uint64S *sec, Uint32T *ns);
zlStatusE zl303xx_Dpll73xToDWaitForRollover(void *hwParams, Uint32T timeoutMs,
                                          ZLS3073X_TODReadTypeE readType, Uint64S *sec, Uint32T *ns);
zlStatusE zl303xx_Dpll73xLatchedToDReadSetup(void *hwParams, ZLS3073X_TODReadTypeE readType);
zlStatusE zl303xx_Dpll73xToDRead(void *hwParams, ZLS3073X_TODReadTypeE readType,
                               Uint64S *sec, Uint32T *ns);
zlStatusE zl303xx_Dpll73xToDWrite(void *hwParams, ZLS3073X_ToDWriteTypeE writeType,
                                zl303xx_BooleanE relativeAdjust,
                                Uint64S sec, Uint32T ns, zl303xx_BooleanE bBackwardAdjust);

/* TIE */
zlStatusE zl303xx_Dpll73xInputPhaseErrorRead(void *hwParams, Uint32T timeoutMs,
                                           Sint32T *tieNs);
/* Hitless compensation via Negative TIE Write */
Sint32T zl303xx_Dpll73xAdjustIfNegativeTieWriteBeingUsed(void *hwParams,
                                                       zl303xx_HitlessCompE hitlessType,
                                                       zl303xx_BooleanE *apply);
zlStatusE zl303xx_Dpll73xAdjustPhaseUsingNegativeTieWrite(void *hwParams);



/* Transition functions */
zlStatusE zl303xx_Dpll73xMitigationEnabledSet(void *hwParams, zl303xx_BooleanE mitigationEnabled);
zlStatusE zl303xx_Dpll73xOverridePllPriorMode(void *hwParams,
                                            ZLS3073X_DpllHWModeE overridePllPriorMode, zl303xx_RefIdE refId);
zlStatusE zl303xx_Dpll73xGetSWHybridTransientState(void *hwParams,zl303xx_BCHybridTransientType *BCHybridTransientType);
zlStatusE zl303xx_Dpll73xSetSWHybridTransientState(void *hwParams,zl303xx_BCHybridTransientType BCHybridTransientType);
Sint32T zl303xx_Dpll73xTakeHwNcoControl(void *hwParams);
Sint32T zl303xx_Dpll73xReturnHwNcoControl(void *hwParams);
Sint32T zl303xx_Dpll73xBCHybridActionPhaseLock(void *hwParams);
Sint32T zl303xx_Dpll73xBCHybridActionOutOfLock(void *hwParams);
Sint32T zl303xx_Dpll73xSetActiveElecActionsInitial(void *hwParams, Sint32T refId, Sint32T syncId);
Sint32T zl303xx_Dpll73xSetActiveElecActionsTransientStage1(void *hwParams, Uint32T refId);
Sint32T zl303xx_Dpll73xSetActiveElecActionsTransientStage2(void *hwParams, Uint32T refId);


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
