

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Prototypes for structure initialisation and locking.
*
*******************************************************************************/

#ifndef _ZL303XX_PARAMS_H_
#define _ZL303XX_PARAMS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "zl303xx_DeviceSpec.h"

zlStatusE zl303xx_CreateDeviceInstance(zl303xx_ParamsS **zl303xx_Params);
zlStatusE zl303xx_FreeDeviceInstance(zl303xx_ParamsS **zl303xx_Params);

zlStatusE zl303xx_LockDevParams(zl303xx_ParamsS *zl303xx_Params);
zlStatusE zl303xx_UnlockDevParams(zl303xx_ParamsS *zl303xx_Params);

#ifdef __cplusplus
}
#endif

#endif


