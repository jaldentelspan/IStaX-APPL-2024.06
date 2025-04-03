

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
*     This file contains headers for ZL303xx variant support.
*
*******************************************************************************/

#ifndef ZL303XX_VAR_H_
#define ZL303XX_VAR_H_

#ifdef __cplusplus
extern "C" {
#endif


/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_RdWr.h"

/*****************   DEFINES   ************************************************/

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
zlStatusE zl303xx_InitDeviceIdAndRev(zl303xx_ParamsS *zl303xx_Params);

zlStatusE zl303xx_GetDeviceId(zl303xx_ParamsS *zl303xx_Params, Uint8T *deviceId); /* DEPRECATED - use zl303xx_GetDeviceId16Bits() */
zlStatusE zl303xx_GetDeviceRev(zl303xx_ParamsS *zl303xx_Params, Uint8T *revision);
zlStatusE zl303xx_GetDeviceId16Bits(zl303xx_ParamsS *zl303xx_Params, zl303xx_DeviceIdE *deviceId);
zlStatusE zl303xx_GetDeviceFWId16Bits(zl303xx_ParamsS *zl303xx_Params, Uint32T *firmwareRev);


void zl303xx_PrintDeviceId(zl303xx_ParamsS *zl303xx_Params);
void zl303xx_PrintDeviceRev(zl303xx_ParamsS *zl303xx_Params);

/* These functions read values directly from the hardware registers rather
   than zl303xx_Params */
zlStatusE zl303xx_ReadDeviceId(zl303xx_ParamsS *zl303xx_Params, Uint8T *deviceId);
zlStatusE zl303xx_ReadDeviceRevision(zl303xx_ParamsS *zl303xx_Params, Uint8T *revision);

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */

