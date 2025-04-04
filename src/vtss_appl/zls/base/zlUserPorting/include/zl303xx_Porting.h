

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Top level header file that includes all the other header files for this library
*
*******************************************************************************/

#ifndef _ZL303XX_PORTING_H
#define _ZL303XX_PORTING_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES                *****************************/

/* Include all the other files in this directory */
#include "zl303xx_Global.h"
#include "zl303xx_DataTypes.h"
#include "zl303xx_Trace.h"
#include "zl303xx_ErrTrace.h"

/*****************   General Constants   **************************************/

extern const Uint32T ZL303XX_DEVICE_HI_PRI_CPU_IRQ;
extern const Uint32T ZL303XX_DEVICE_LO_PRI_CPU_IRQ;

extern const Uint32T ZL303XX_DEVICE_HI_PRI_CPU_VECTOR;
extern const Uint32T ZL303XX_DEVICE_LO_PRI_CPU_VECTOR;

extern const Uint32T ZL303XX_CHIP_SELECT_MASK;
#if defined _ZL303XX_ZLE1588_BOARD
extern const Uint32T CSB2_CHIP_SELECT_MASK;
extern const Uint32T CSB3_CHIP_SELECT_MASK;
#endif
extern const Uint32T APLL_CHIP_SELECT_MASK;

/*****************   DEFINES     **********************************************/

#define CPU_ERROR_BASE       2100

/*****************   DATA TYPES   *********************************************/

/* Error codes used by the API */
typedef enum
{
   /* General OK and ERROR codes */
   CPU_OK = 0,
   CPU_ERROR = CPU_ERROR_BASE,

   /* Specific Error scenerios */
   CPU_SPI_MULTIPLE_INIT,      /* SPI already initialized */
   CPU_SPI_INIT_ERROR,         /* SPI error during initialization */
   CPU_SPI_MALLOC_ERROR,       /* SPI memory allocation error */
   CPU_SPI_CMD_CP_BUSY,        /* SPI init command time-out due to CP being busy */
   CPU_SPI_SEM_CREATE_FAIL,    /* SPI failed to create its Mutex */
   CPU_SPI_SEM_DELETE_FAIL,    /* SPI failed to delete its Mutex */
   CPU_SPI_SEM_TAKE_FAIL,      /* SPI failed to take its Mutex */
   CPU_SPI_ISR_CONNECT_FAIL,   /* SPI failed to connect IS-Routine to HW int */
   CPU_SPI_ISR_ENABLE_FAIL,    /* SPI failed to enable its HW int */
   CPU_SPI_ISR_DISABLE_FAIL,   /* SPI failed to disable its HW int */

   CPU_ERROR_CODE_END
} cpuStatusE;

/*****************   DATA STRUCTURES   ****************************************/

/* include the function definitions */
#include "zl303xx_PortingFunctions.h"


#ifdef __cplusplus
}
#endif

#endif   /* MULTIPLE INCLUDE BARRIER */
