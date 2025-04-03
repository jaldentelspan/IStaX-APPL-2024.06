

/******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     When the zl303xx API is ported to a particular processor architecture it
*     may need some device specific knowledge. This file contains some constants
*     for an MPC8313 device.
*
******************************************************************************/

#ifndef _ZL303XX_M8313_H_
#define _ZL303XX_M8313_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"

/*****************   DEFINES   ************************************************/
#ifdef OS_VXWORKS
    /* Interrupt Registers */
    #define M8313_SEPNR(base)    ((volatile Sint32T *)((base) + (Uint32T)0x00072C))
    #define M8313_SEMSR(base)    ((volatile Sint32T *)((base) + (Uint32T)0x000738))
    #define M8313_SECNR(base)    ((volatile Sint32T *)((base) + (Uint32T)0x00073C))

    /* GPIO */
    #define M8313_GP1DIR(base)   ((volatile Sint32T *)((base) + (Uint32T)0x000C00))
    #define M8313_GP1ODR(base)   ((volatile Sint32T *)((base) + (Uint32T)0x000C04))
    #define M8313_GP1DAT(base)   ((volatile Sint32T *)((base) + (Uint32T)0x000C08))

    #define IRQ1_SECNR_EDGE_MASK   (0x80000000 >> 17)
    #define IRQ2_SECNR_EDGE_MASK   (0x80000000 >> 18)

    #define HI_PRI_IRQ_PIN_MASK    (0x80000000 >> 1)  /* IRQ1# pin */
    #define LO_PRI_IRQ_PIN_MASK    (0x80000000 >> 2)  /* IRQ2# pin */

    #define GPIO_RESET_MASK        (0x80000000 >> 4)  /* GPIO pin 4 */
#endif

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/
#ifdef __cplusplus
}
#endif

#endif
