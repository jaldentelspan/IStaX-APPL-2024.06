

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
*     ZL3073X DPLL Configuration prototypes.
*
*******************************************************************************/

#ifndef ZL303XX_DPLL_73X_CONFIG_H_
#define ZL303XX_DPLL_73X_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Dpll73xGlobal.h"

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/


Sint32T zl303xx_Dpll73xGetHwManualHoldoverStatus(void *hwParams, Sint32T *status);
Sint32T zl303xx_Dpll73xGetHwManualFreerunStatus(void *hwParams, Sint32T *status);
Sint32T zl303xx_Dpll73xGetHwSyncInputEnStatus(void *hwParams, Sint32T *en);
zlStatusE zl303xx_Dpll73xModeSet(void *hwParams, ZLS3073X_DpllHWModeE mode);
zlStatusE zl303xx_Dpll73xModeGet(void *hwParams, ZLS3073X_DpllHWModeE *mode);
zlStatusE zl303xx_Dpll73xHWModeSet(void *hwParams, ZLS3073X_DpllHWModeE mode,
                                 Uint8T refId, Sint32T freqOffsetToSetInppt);
zlStatusE zl303xx_Dpll73xCurrRefSet(void *hwParams,  Uint8T refId);
zlStatusE zl303xx_Dpll73xCurrRefGet(void *hwParams, Uint32T *refId);
zlStatusE zl303xx_Dpll73xTieClearGet(void *hwParams, zl303xx_BooleanE *tieClear);
zlStatusE zl303xx_Dpll73xTieClearSet(void *hwParams, zl303xx_BooleanE tieClear);
zlStatusE zl303xx_Dpll73xPhaseSlopeLimitSet(void *hwParams, Uint32T PSL);
zlStatusE zl303xx_Dpll73xPhaseSlopeLimitGet(void *hwParams, Uint32T *PSL);
zlStatusE zl303xx_Dpll73xBandwidthSet(void *hwParams, ZLS3073X_BandwidthE Bandwidth);
zlStatusE zl303xx_Dpll73xBandwidthGet(void *hwParams, ZLS3073X_BandwidthE *Bandwidth);
zlStatusE zl303xx_Dpll73xGetOutputClockConfig(void *hwParams,
                                            zl303xx_Dpll73xOutputConfS *conf,
                                            zl303xx_BooleanE localMutexProtect);
zlStatusE zl303xx_Dpll73xGetOutputClockConfigCached(void *hwParams,
                                                  zl303xx_Dpll73xOutputConfS *conf);
zlStatusE zl303xx_Dpll73xGetOutputClockConfigCachedWithProtection(void *hwParams,
                                                                zl303xx_Dpll73xOutputConfS *conf,
                                                                zl303xx_BooleanE mutexProtect);
zlStatusE zl303xx_Dpll73xPhaseStepMaskSet(void *hwParams,
                                        ZLS3073X_OutputsE outputNum,
                                        Uint32T v);
zlStatusE zl303xx_Dpll73xOutputThatDrivesTimeStamperSet(void *hwParams,
                                                      ZLS3073X_OutputsE outputNum);
zlStatusE zl303xx_Dpll73xPhaseStepMaskGet(void *hwParams,
                                        Uint32T *phaseStepMask,
                                        ZLS3073X_OutputsE *outputNumThatDrivesTimeStamper);
zlStatusE zl303xx_Dpll73xPhaseStepMaskGetWithProtection(void *hwParams,
                                                      Uint32T *phaseStepMask,
                                                      ZLS3073X_OutputsE *outputNumThatDrivesTimeStamper,
                                                      zl303xx_BooleanE mutexProtect);
zlStatusE zl303xx_Dpll73xModeSwitchFastLockEnableSet(void *hwParams,
                                                   zl303xx_BooleanE nco, zl303xx_BooleanE force, zl303xx_BooleanE en);
zlStatusE zl303xx_Dpll73xNCOAutoReadSet(void *hwParams,
                                      zl303xx_BooleanE enabled);
zlStatusE zl303xx_Dpll73xNCOAutoReadGetData(void *hwParams,
                                          Sint32T *freqOffsetInppt);
zlStatusE zl303xx_Dpll73xPhaseStepMaxSet(void *hwParams, Uint32T psm);
zlStatusE zl303xx_Dpll73xGetDeviceStepTimeResolutionsFreq(void *hwParams,
                                                        zl303xx_ClockOutputFreqArrayS *clockFreqArray);
zlStatusE zl303xx_Dpll73xGetDeviceClockOutputsFreq(void *hwParams,
                                                 zl303xx_ClockOutputFreqArrayS *clockFreqArray);
zl303xx_BooleanE zl303xx_Dpll73xInNCOMode(void *hwParams);
zlStatusE zl303xx_Dpll73xGetRefSyncPairMode(void *hwParams, Uint32T refId, Uint32T *syncId,
                                          zl303xx_BooleanE *bRefSyncPair, zl303xx_BooleanE *bEPPS );
zlStatusE zl303xx_Dpll73xSetRefSyncPair(void *hwParams, Uint32T refId, Uint32T syncId);
zlStatusE zl303xx_Dpll73xSetNCOAssistParamsSAssociation(zl303xx_ParamsS *APR,
                                                      zl303xx_ParamsS *NCOAssist);
zlStatusE zl303xx_Dpll73xGetNCOAssistParamsSAssociation(zl303xx_ParamsS *APR,
                                                      zl303xx_ParamsS **NCOAssist);
zlStatusE zl303xx_Dpll73xInputPhaseErrorClearSet(void *hwParams, zl303xx_BooleanE tieWrClear);
zlStatusE zl303xx_Dpll73xTieCtrlSet(void *hwParams, ZLS3073X_DpllTieCtrlE oper);
zlStatusE zl303xx_Dpll73xTieCtrlReady(void *hwParams, zl303xx_BooleanE *ready);
zlStatusE zl303xx_Dpll73xSetNCOAssistEnable(void *hwParams, zl303xx_BooleanE enable);
zlStatusE zl303xx_Dpll73xGetNCOAssistEnable(void *hwParams, zl303xx_BooleanE *enable);
zlStatusE zl303xx_Dpll73xSetNCOHoldoverEnable(void *hwParams, Uint32T dpllId, zl303xx_BooleanE enableHOReady);

zlStatusE zl303xx_Dpll73xWaitForTieDoneSticky(void *hwParams);
zlStatusE zl303xx_Dpll73xWaitForTieCtrlReady(void *hwParams);

Sint32T zl303xx_Dpll73xSetModeHoldoverInfo(void *hwParams,
                                         Uint16T lastServerId,
                                         Sint32T lastDfValue,
                                         Sint32T algHOFreq,
                                         zl303xx_BooleanE algHOValid);
zlStatusE zl303xx_Dpll73xSetModeHoldoverFreqOffset(void *hwParams,
                                                 Sint32T holdoverValue);
zlStatusE zl303xx_Dpll73xBandwidthCustomSet(void *hwParams, Uint32T bandwidthCustom);
zlStatusE zl303xx_Dpll73xBandwidthCustomGet(void *hwParams, Uint32T *bandwidthCustom);


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
