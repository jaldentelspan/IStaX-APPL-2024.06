

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
*     ZL3073X DPLL Reporting prototypes.
*
*******************************************************************************/

#ifndef ZL303XX_DPLL_73X_REPORT_H_
#define ZL303XX_DPLL_73X_REPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Dpll73xGlobal.h"

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/


/* Input */
zlStatusE zl303xx_Ref73xFreqDetectedGet(void *zl303xx_Params,
                                      Uint32T refId, Uint32T *freqHz);
zlStatusE zl303xx_Ref73xMonBitsClear(void *hwParams, Uint32T refId);
Sint32T zl303xx_Dpll73xStickyReadMon(void *hwParams, Uint32T refId, Uint32T *data);


/* DPLL */
Sint32T zl303xx_Dpll73xGetHwLockStatus(void *hwParams, Sint32T *lockStatus);
Sint32T zl303xx_Dpll73xGetNCOAssistHwLockStatus(void *hwParams, Sint32T *lockStatus);
Sint32T zl303xx_Dpll73xGetHwOutOfRangeStatus(void *hwParams, Sint32T *oor);
zlStatusE zl303xx_Dpll73xGetHoldoverReady(void *zl303xx_Params, Uint32T dpllId, zl303xx_AprHOReadyE *HOR);


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
