

/*******************************************************************************
*
*  $Id: 6517a3b4707c071ac26ad8fcef236bd9d8d99889
*  Copyright (c) 2006-2022 Microchip Technology Inc. and its subsidiaries, all rights reserved.
*  Subject to the terms of the license that accompanies the software and controls as it relates to the software and any conflicting terms herein, you may use this Microchip software and any derivatives exclusively with Microchip products.
*  You are responsible for complying with third party license terms applicable to your use of third party software (including open source software) that may accompany this Microchip software.
*  SOFTWARE IS 'AS IS'. NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
*  IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.
*  TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
*  Module Description:
*     A generic DPLL module API.
*
*******************************************************************************/

#ifndef ZL303XX_DPLL_GENERIC_H
#define ZL303XX_DPLL_GENERIC_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Error.h"
#include "zl303xx_DeviceIf.h"

/*****************   DEFINES     **********************************************/
#define ZL303XX_DPLL_CLOCK_MODE_STR_LEN 48


zlStatusE zl303xx_DpllSetDeviceMode(void *hwParams, zl303xx_DpllClockModeE dpllClockModeToSet, zl303xx_RefIdE genericDpllInputRef);
zlStatusE zl303xx_DpllGetDeviceMode(void *hwParams, zl303xx_DpllClockModeE *dpllClockModeToGetP, zl303xx_RefIdE *dpllInputRefIfForcedP);
zlStatusE zl303xx_DpllMapClockModeToDeviceMode(void *hwParams, zl303xx_DpllClockModeE dpllClockMode, Uint32T *mappedMode);
zlStatusE zl303xx_DpllMapDeviceModeToClockMode(void *hwParams, Uint32T deviceDpllMode, zl303xx_DpllClockModeE *mappedClockMode);
zlStatusE zl303xx_DpllClockModeToStr(zl303xx_DpllClockModeE dpllClockMode, char *clockModeStrP, Uint32T strLen);


#if defined __cplusplus
}
#endif

#endif   /* MULTIPLE INCLUDE BARRIER */
