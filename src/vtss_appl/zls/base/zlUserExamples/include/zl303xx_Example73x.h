

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
 *     Supporting interfaces for the 73X examples
 *
 ******************************************************************************/

#ifndef _ZL303XX_EXAMPLE_73X_H_
#define _ZL303XX_EXAMPLE_73X_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Error.h"
#include "zl303xx_DeviceSpec.h"
#include "zl303xx_AddressMap73x.h"

/*****************   DEFINES   ************************************************/

#define ZLS3073X_MAX_NUM_CHARS_PER_LINE_IN_CONFIG_FILE (128)
#define ZLS3073X_MAX_NUM_LINES_IN_CONFIG_FILE          (1900)   /* > 1819 */

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

typedef struct
{
   zl303xx_DeviceModeE deviceMode;
   Uint32T pllId;
   zl303xx_ParamsS *zl303xx_Params;
} example73xClockCreateS;

typedef struct {
    const char *line;
} example73xStructConfigLine_t;

typedef struct {
    example73xStructConfigLine_t lines[1];  /* Minimum */
/*  If device is to be programmed via the struct, change the max. number of lines here: */
/*
    example73xStructConfigLine_t lines[ZLS3073X_MAX_NUM_LINES_IN_CONFIG_FILE];
*/
} example73xStructConfigData_t;

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

zlStatusE example73xEnvInit(void);
zlStatusE example73xEnvClose(void);
zlStatusE example73xSlaveWarmStart(void);

zlStatusE example73xClockCreateStructInit(example73xClockCreateS *pClock);
zlStatusE example73xClockCreate(example73xClockCreateS *pClock);
zlStatusE example73xClockRemove(example73xClockCreateS *pClock);

zlStatusE example73xLoadConfigFile(zl303xx_ParamsS *zl303xx_Params, const char *filename);

zlStatusE example73xStickyLockCallout(void *hwParams, zl303xx_DpllIdE pllId, zl303xx_BooleanE lockFlag);

#if defined _ZL303XX_ZLE30360_BOARD || defined _ZL303XX_ZLE1588_BOARD
zlStatusE example73xGetTsuDpllTimeError(zl303xx_ParamsS *zl303xx_Params,
      Uint64S *pErrorSec,
      Uint32T *pErrorNsec,
      zl303xx_BooleanE *pErrorNegative,
      zl303xx_BooleanE bDpllIsSource,
      Uint32T timeoutMs);
zlStatusE example73xAlignTsuDpllToD(zl303xx_ParamsS *zl303xx_Params, zl303xx_BooleanE bDpllIsSource);
zlStatusE example73xCheckPhyTSClockFreq(zl303xx_ParamsS *zl303xx_Params);
#endif

#if defined ZLS30731_INCLUDED && !(defined _ZL303XX_ZLE30360_BOARD || defined _ZL303XX_ZLE1588_BOARD)
zlStatusE example73x(void);
#else
zlStatusE example73xSlave(void);
zlStatusE example73xMaster(void);
#endif


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
