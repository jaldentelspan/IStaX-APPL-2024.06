

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
*     Debug functions for the ZL3074X DPLL.
*
*******************************************************************************/

#ifndef ZL303XX_DEBUG_DPLL_73X_H_
#define ZL303XX_DEBUG_DPLL_73X_H_

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

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
zlStatusE zl303xx_DebugPllStatus73x(zl303xx_ParamsS *zl303xx_Params);
zlStatusE zl303xx_DebugHwRefStatus73x(zl303xx_ParamsS *zl303xx_Params, Uint32T refId);
zlStatusE zl303xx_DebugHwRefCfg73x(zl303xx_ParamsS *zl303xx_Params, Uint32T refId);
zlStatusE zl303xx_DebugDpllConfig73x(zl303xx_ParamsS *zl303xx_Params);
zlStatusE zl303xx_DebugPrintMailbox73x(zl303xx_ParamsS *zl303xx_Params,
                                     ZLS3073X_MailboxE mb, Uint32T mbNum);
#if defined(ZL_UNG_MODIFIED)
zlStatusE zl303xx_DebugPrintAllRegs73x(zl303xx_ParamsS *zl303xx_Params);
#endif //ZL_UNG_MODIFIED

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
