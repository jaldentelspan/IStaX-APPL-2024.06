

/******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Header for remote SPI read / write operations
*
******************************************************************************/

#ifndef _ZL303XX_SPI_VXW_SOCKET_H_
#define _ZL303XX_SPI_VXW_SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"
#include "zl303xx_Os.h"
#include "zl303xx_Error.h"

/*****************   DEFINES   ************************************************/

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/
extern OS_SEM_ID spiIsrSemId;

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
/* Provide OS Exclusion for the interface */
zlStatusE spiLock(void);
zlStatusE spiUnlock(void);

zlStatusE zl303xx_SpiInitInterrupts(Uint32T spi_irq);
zlStatusE zl303xx_SpiDisableInterrupts(Uint32T spi_irq);

zlStatusE zl303xx_SpiSocketCreate(void);
zlStatusE zl303xx_SpiSocketClose(void);

zlStatusE zl303xx_SpiSocketRead(void *par, Uint32T regAddr, Uint32T *data, Uint16T bufLen);
zlStatusE zl303xx_SpiSocketWrite(void *par, Uint32T regAddr, Uint32T *data, Uint16T bufLen);

#ifdef __cplusplus
}
#endif

#endif
