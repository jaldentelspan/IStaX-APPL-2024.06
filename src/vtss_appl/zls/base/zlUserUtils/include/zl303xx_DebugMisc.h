

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     This is the header file for the accessing routines in zl303xx_DebugMisc.c.
*
*******************************************************************************/

#ifndef _ZL303XX_DEBUGMISC_H
#define _ZL303XX_DEBUGMISC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zl303xx_Global.h"
#if defined ZLS30361_INCLUDED || defined ZLS30721_INCLUDED || defined ZLS30701_INCLUDED || defined ZLS30731_INCLUDED || defined ZLS30751_INCLUDED || defined ZLS30771_INCLUDED
#include "zl303xx.h"
#include "zl303xx_DeviceSpec.h"
#include "zl303xx_Params.h"
#endif

zlStatusE zl303xx_DebugHwRefStatus(zl303xx_ParamsS *zl303xx_Params, Uint32T refId);
zlStatusE zl303xx_DebugDpllStatus(zl303xx_ParamsS *zl303xx_Params, Uint32T dpllId);
#ifdef ZL_UNG_MODIFIED
zlStatusE zl303xx_DebugPllStatus(zl303xx_ParamsS *zl303xx_Params);
zlStatusE zl303xx_DebugHwRefCfg(zl303xx_ParamsS *zl303xx_Params, Uint32T refId);
zlStatusE zl303xx_DebugDpllConfig(zl303xx_ParamsS *zl303xx_Params, Uint32T dpllId);
#endif //ZL_UNG_MODIFIED


void zl303xx_DebugApiBuildInfo(void);

#if defined ZLS30361_INCLUDED || defined ZLS30721_INCLUDED || defined ZLS30701_INCLUDED || defined ZLS30731_INCLUDED || defined ZLS30751_INCLUDED || defined ZLS30771_INCLUDED
zlStatusE zl303xx_DisplayMsgRouterCallStatistics(void*, void*);
zlStatusE zl303xx_DisplayMsgRouterCallAverageFor100Secs(void*, void*);
zlStatusE zl303xx_ClearRdWrStats(void *hwParams);
zlStatusE zl303xx_ClearMsgRouterCallStats(void *hwParams);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ZL303XX_DEBUGMISC_H */
