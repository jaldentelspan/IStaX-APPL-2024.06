

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
*     for an MPC8260 device.
*
******************************************************************************/

#ifndef _ZL303XX_MPC8260_H_
#define _ZL303XX_MPC8260_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/
#include "zl303xx_Global.h"

/*****************   DEFINES   ************************************************/

/* Interrupt Registers */

/* interrupt edge control register */
#define M8260_SIEXR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010C24))

/* interrupt pending register */
#define M8260_SIPNR_H(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010C08))

/* various CPM registers and bit definitions required for SPI operation */
#define M8260_CPCR(base)      ((volatile Uint32T *)((base) + (Uint32T)0x119C0))
#define M8260_CPCR_PAGE_SPI   0x09
#define M8260_CPCR_SBC_SPI    0x0a
#define M8260_CPCR_RT_INIT    0x0         /* Init rx and tx */
#define M8260_CPCR_FLG        0x00010000  /* flag - command executing */

#define M8260_CPCR_PAGE_MSK   0x7c000000  /* RAM page number */
#define M8260_CPCR_SBC_MSK    0x03e00000  /* sub-block code */
#define M8260_CPCR_MCN_MSK    0x00003fc0  /* MCC channel number */
#define M8260_CPCR_OP_MSK     0x0000000f  /* command opcode */
#define M8260_CPCR_PAGE_SHIFT 0x1a        /* get to the page field */
#define M8260_CPCR_SBC_SHIFT  0x15        /* get to the SBC field */
#define M8260_CPCR_MCN_SHIFT  0x6         /* get to the MCC field */
#define M8260_CPCR_OP_SHIFT   0x0         /* get to the opcode field */

#define M8260_CPCR_OP(x)      (((x) << M8260_CPCR_OP_SHIFT) & M8260_CPCR_OP_MSK)
#define M8260_CPCR_SBC(x)     (((x) << M8260_CPCR_SBC_SHIFT) & M8260_CPCR_SBC_MSK)
#define M8260_CPCR_PAGE(x)    (((x) << M8260_CPCR_PAGE_SHIFT) & M8260_CPCR_PAGE_MSK)
#define M8260_CPCR_MCN(x)     (((x) << M8260_CPCR_MCN_SHIFT) & M8260_CPCR_MCN_MSK)

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

#ifdef __cplusplus
}
#endif

#endif
