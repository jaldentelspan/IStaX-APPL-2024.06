

/*******************************************************************************
*
*  $Id: 5829f97c957ab0b7024bbe9785d3bfb3bdfb63de
*
*  Copyright 2006-2019 Microchip/Microsemi Semiconductor Limited.
*  All rights reserved.
*
*  Module Description:
*     Debug functions for the ZL3036x DPLL.
*
*******************************************************************************/

#ifndef ZL303XX_DEBUG_DPLL_36X_H_
#define ZL303XX_DEBUG_DPLL_36X_H_

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
zlStatusE zl303xx_DebugPllStatus36x(zl303xx_ParamsS *zl303xx_Params);
zlStatusE zl303xx_DebugHwRefStatus36x(zl303xx_ParamsS *zl303xx_Params, Uint32T refId);
zlStatusE zl303xx_DebugHwRefCfg36x(zl303xx_ParamsS *zl303xx_Params, Uint32T refId);
zlStatusE zl303xx_DebugDpllStatus36x(zl303xx_ParamsS *zl303xx_Params);
zlStatusE zl303xx_DebugDpllConfig36x(zl303xx_ParamsS *zl303xx_Params);
zlStatusE zl303xx_DumpWritableRegs(zl303xx_ParamsS *zl303xx_Params);

#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */
