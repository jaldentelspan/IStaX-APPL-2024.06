

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Types and prototypes needed by the SPI routines
*
*******************************************************************************/

#ifndef _ZL303XX_LNX_SPI_INTERRUPTS_H_
#define _ZL303XX_LNX_SPI_INTERRUPTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "zl303xx_Global.h"  /* This should always be the first include file */
#include "zl303xx_Error.h"    /* For zlStatusE */
#include "zl303xx.h"

#define ZL303XX_LNX_INTR_TASK_STACK_SIZE 2000
#define ZL303XX_LNX_INTR_TASK_PRIORITY   99   /* Highest priority */

#ifdef OS_LINUX
    void zl303xx_Intr0Handler(Sint32T sigNum, Sint32T unUsed);
    zlStatusE zl303xx_InitHighIntr0Task(Uint8T cpuIntNum);
    zlStatusE zl303xx_DestroyHighIntr0Task(Uint8T cpuIntNum);
    void zl303xx_LinuxHighIntr0Task(void);
    osStatusT zl303xx_ResetDev(const char * devBaseName, Uint8T devCSToReset);

    #ifdef ZL_INTR_USES_SIGACTION
        zlStatusE zl303xx_Init0Intr(zl303xx_ParamsS *zl303xx_Params, Uint16T sigNum);
        zlStatusE zl303xx_Destroy0Intr(zl303xx_ParamsS *zl303xx_Params, Uint16T sigNum);
    #endif
#endif

#ifdef __cplusplus
}
#endif

#endif  /* _ZL303XX_LNX_SPI_INTERRUPTS_H_ */


