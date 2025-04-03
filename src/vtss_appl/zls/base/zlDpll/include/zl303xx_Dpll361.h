/*******************************************************************************
*
*  $Id: 5829f97c957ab0b7024bbe9785d3bfb3bdfb63de
*
*  Copyright 2006-2019 Microchip/Microsemi Semiconductor Limited.
*  All rights reserved.
*
*  Module Description:
*     ZL30361 DPLL phase step (jump) function protos and registers.
*
*******************************************************************************/

#ifndef ZL303XX_DPLL_361_H_
#define ZL303XX_DPLL_361_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_AddressMap36x.h"

/*****************   TYPES   ************************************************/

/*****************   DEFINES   ************************************************/
/* DPLL registers */
#define ZLS3036X_DPLL_CONFIG_REG        ZL303XX_MAKE_MEM_ADDR_36X(0x182, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_DPLL_CONFIG_TWC_MASK   0x0F
#define ZLS3036X_DPLL_CONFIG_TWC_SHIFT  4

/* TIE control registers */
#define ZLS3036X_TIE_WR_THRESH_REG   ZL303XX_MAKE_MEM_ADDR_36X(0x2D0, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_TIE_WR_STATUS_REG   ZL303XX_MAKE_MEM_ADDR_36X(0x2D1, ZL303XX_MEM_SIZE_1_BYTE)
#define ZLS3036X_TIE_WR_STATUS_MASK  0x0F


/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

Sint32T zl303xx_Dpll36xGetFreq(void *hwParams, Sint32T *freqOffsetUppm, ZLS3036X_MemPartE memPart);
zlStatusE zl303xx_Dpll36xInputPhaseErrorThresholdSet(void *hwParams, Uint32T threshNs);
zlStatusE zl303xx_Dpll36xInputPhaseErrorStatusGet(void *hwParams, zl303xx_BooleanE *complete);
zlStatusE zl303xx_Dpll36xInputPhaseErrorClearSet(void *hwParams, zl303xx_BooleanE tieWrClear);

Sint32T zl303xx_Dpll36xTakeHwNcoControl(void *hwParams);
Sint32T zl303xx_Dpll36xReturnHwNcoControl(void *hwParams);

zlStatusE zl303xx_Dpll36xTieWrite(void *hwParams, Uint32T timeoutMs, Sint32T tieNs);

zlStatusE zl303xx_Dpll36xAdjustPhaseUsingHitlessCompensation(void *hwParams);

Sint32T zl303xx_Dpll36xAdjustIfHitlessCompensationBeingUsed(void *hwParams,
                                                          zl303xx_HitlessCompE hitlessType,
                                                          zl303xx_BooleanE *apply);
zlStatusE zl303xx_Dpll36xAccuInputPhaseError1588HostRegSet(void *hwParams,
                                                          Uint32T pllId,
                                                          Sint32T tieNs);
zlStatusE zl303xx_Dpll36xAccuInputPhaseError1588HostRegGet( void *hwParams,
                                                          Uint32T pllId,
                                                          Sint32T *tieNs);

/* Callback functions */
Sint32T zl303xx_Dpll36xGetDeviceInfo(void *hwParams, zl303xx_DevTypeE *devType, zl303xx_DeviceIdE *devId, Uint32T *devRev);
Sint32T zl303xx_Dpll36xBCHybridActionPhaseLock(void *hwParams);
Sint32T zl303xx_Dpll36xBCHybridActionOutOfLock(void *hwParams);
Sint32T zl303xx_Dpll36xSetActiveElecActionsInitial(void *hwParams, Sint32T refId, Sint32T syncId);
Sint32T zl303xx_Dpll36xSetActiveElecActionsTransientStage1(void *hwParams, Uint32T refId);
Sint32T zl303xx_Dpll36xSetActiveElecActionsTransientStage2(void *hwParams, Uint32T refId);

/* CGU Message Router callback function */
Sint32T zl303xx_Dpll36xMsgRouter(void *hwParams, void *inData, void *outData);

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
