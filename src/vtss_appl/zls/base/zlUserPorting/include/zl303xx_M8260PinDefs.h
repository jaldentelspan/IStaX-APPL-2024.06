

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Header file for several C modules that use Ports of the M8260.
*
*******************************************************************************/

#ifndef _ZL303XX_M8260PINDEFS_H_
#define _ZL303XX_M8260PINDEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************   INCLUDE FILES   ******************************************/

/*****************   DEFINES   ************************************************/

/*** IO Port definitions ***/
/* Port Pin Masks for the M8260 */
/* The same pin value applies regardless of the Port (A,B,C or D). */
#define M8260_P0    ((Uint32T)0x80000000)
#define M8260_P1    ((Uint32T)0x40000000)
#define M8260_P2    ((Uint32T)0x20000000)
#define M8260_P3    ((Uint32T)0x10000000)
#define M8260_P4    ((Uint32T)0x08000000)
#define M8260_P5    ((Uint32T)0x04000000)
#define M8260_P6    ((Uint32T)0x02000000)
#define M8260_P7    ((Uint32T)0x01000000)
#define M8260_P8    ((Uint32T)0x00800000)
#define M8260_P9    ((Uint32T)0x00400000)
#define M8260_P10   ((Uint32T)0x00200000)
#define M8260_P11   ((Uint32T)0x00100000)
#define M8260_P12   ((Uint32T)0x00080000)
#define M8260_P13   ((Uint32T)0x00040000)
#define M8260_P14   ((Uint32T)0x00020000)
#define M8260_P15   ((Uint32T)0x00010000)
#define M8260_P16   ((Uint32T)0x00008000)
#define M8260_P17   ((Uint32T)0x00004000)
#define M8260_P18   ((Uint32T)0x00002000)
#define M8260_P19   ((Uint32T)0x00001000)
#define M8260_P20   ((Uint32T)0x00000800)
#define M8260_P21   ((Uint32T)0x00000400)
#define M8260_P22   ((Uint32T)0x00000200)
#define M8260_P23   ((Uint32T)0x00000100)
#define M8260_P24   ((Uint32T)0x00000080)
#define M8260_P25   ((Uint32T)0x00000040)
#define M8260_P26   ((Uint32T)0x00000020)
#define M8260_P27   ((Uint32T)0x00000010)
#define M8260_P28   ((Uint32T)0x00000008)
#define M8260_P29   ((Uint32T)0x00000004)
#define M8260_P30   ((Uint32T)0x00000002)
#define M8260_P31   ((Uint32T)0x00000001)

#define M8260_PIN_MAX   31    /* Values range from 0 to 31 */

/* Macro for computing the pin definitions. */
#define M8260_PIN(pinNum)  ((Uint32T)(1 << (M8260_PIN_MAX - pinNum)))

/* Macros for computing the Address of each of the Port Registers. */
/* Port B Registers */
#define M8260_PBDDR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D20))
#define M8260_PBPAR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D24))
#define M8260_PBSOR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D28))
#define M8260_PBODR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D2C))
#define M8260_PBDTR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D30))

/* Port C Registers */
#define M8260_PCDDR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D40))
#define M8260_PCPAR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D44))
#define M8260_PCSOR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D48))
#define M8260_PCODR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D4C))
#define M8260_PCDTR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D50))

/* Port D Registers */
#define M8260_PDDDR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D60))
#define M8260_PDPAR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D64))
#define M8260_PDSOR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D68))
#define M8260_PDODR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D6C))
#define M8260_PDDTR(base)  ((volatile Sint32T *)((base) + (Uint32T)0x010D70))

/*****************   DATA TYPES   *********************************************/

/*****************   DATA STRUCTURES   ****************************************/

/*****************   EXPORTED GLOBAL VARIABLE DECLARATIONS   ******************/

/*****************   EXTERNAL FUNCTION DECLARATIONS   *************************/

#ifdef __cplusplus
}
#endif

#endif   /* _ZL303XX_M8260PINDEFS_H_ */



















