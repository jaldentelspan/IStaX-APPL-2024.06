

/*******************************************************************************
*
*  $Id: 5829f97c957ab0b7024bbe9785d3bfb3bdfb63de
*  Copyright 2006-2019 Microchip/Microsemi Semiconductor Limited.
*  All rights reserved.
*
*  Module Description:
*     Device initialisation functions
*
*******************************************************************************/

#ifndef ZL303XX_INIT_H_
#define ZL303XX_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/

#include "zl303xx_Global.h"
#include "zl303xx_DeviceIf.h"
#if defined ZLS30341_INCLUDED
#include "zl303xx.h"        /* Required for zl303xx_ParamS Definition   */
#else
#include "zl303xx_DeviceSpec.h"
#endif
#if defined ZLS30341_INCLUDED
#include "zl303xx_TsEng.h"
#endif

/*****************   DEFINES   ************************************************/

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/
typedef struct
{
   Uint32T sysClockFreqHz; /* The system clock rate */
   Uint32T dcoClockFreqHz; /* The DCO clock rate */
#if defined ZLS30341_INCLUDED
   zl303xx_TsEngineInitS tsEngInit;
#endif
   zl303xx_PllInitS pllInit;
} zl303xx_InitDeviceS;

typedef struct
{
#if defined ZLS30341_INCLUDED
    zl303xx_TsEngineCloseS tsEngClose;
#endif
    Uint32T Unused; /* unused member to avoid warning */
} zl303xx_CloseDeviceS;

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/
#if defined _ZL303XX_ZLE30360_BOARD || defined _ZL303XX_ZLE1588_BOARD
/* Only PLL1 has a valid NCO clock to control the TSU on the ZLE30360 and ZLE1588 boards */
extern Uint8T TARGET_DPLL;
#endif

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
zlStatusE zl303xx_InitApi(void);
zlStatusE zl303xx_CloseApi(void);

zlStatusE zl303xx_InitDeviceStructInit(zl303xx_ParamsS *zl303xx_Params, zl303xx_InitDeviceS *par, zl303xx_DeviceModeE deviceMode);
zlStatusE zl303xx_InitDevice(zl303xx_ParamsS *zl303xx_Params,
                hwFuncPtrDriverMsgRouter driverMsgRouter, zl303xx_InitDeviceS *par);

zlStatusE zl303xx_CloseDeviceStructInit(zl303xx_ParamsS *zl303xx_Params, zl303xx_CloseDeviceS *par);
zlStatusE zl303xx_CloseDevice(zl303xx_ParamsS *zl303xx_Params, zl303xx_CloseDeviceS *par);

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */

