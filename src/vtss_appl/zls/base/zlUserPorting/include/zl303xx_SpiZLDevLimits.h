

/*******************************************************************************
*
*  $Id: 8410f584bd768792a15e60f6157c94bf8acedd47
*
*  Copyright 2006-2022 Microchip Technology Inc.
*  All rights reserved.
*
*  Module Description:
*     Defines the ZL device limits on the SPI bus interface.
*
*******************************************************************************/

#ifndef _ZL303XX_ZLDEV_LIMITS_H
#define _ZL303XX_ZLDEV_LIMITS_H

/*****************   INCLUDE FILES                *****************************/

#ifdef __cplusplus
extern "C" {
#endif



/***********************   DEFINES   ******************************************/

/* Microsemi ZL30310/20/4X/6X - Device-Specific addresses and limits */
#define ISR_PAGE_ADDR       0x0F
#define TOP_ISR0_MASK_ADDR  0x7E
#define TOP_ISR1_MASK_ADDR  0x7F
#define DEVICE_PAGE_SEL_REG_ADDR 0x64
#define DEVICE_CONF_INDX_REG_ADDR 0x65
#define DEVICE_CONF_DATA_REG_ADDR 0x67
#define DEVICE_MEM_CMD_REG_ADDR 0x73
#define DEVICE_MEM_CMD_STATUS_REG_ADDR 0x77
#define DEVICE_MEM_READ_REG_ADDR 0x6B
#define DEVICE_MEM_WRITE_REG_ADDR 0x6F
#define MIN_PAGED_ADDR 0x65
#define MAX_PAGED_ADDR 0x7F
#define MAX_PAGES      0x0F

#if defined ZLS30361_INCLUDED
/* Microsemi ZL3036X Device-Specific addresses and limits */
#define DEVICE_PAGE_SEL_REG_ADDR_36X    0x7F
#define MIN_PAGED_ADDR_36X              0x00
#define MAX_PAGED_ADDR_36X              0x7F
#define MAX_PAGES_36X                   0x08
#endif

#if defined ZLS30721_INCLUDED
/* Microsemi ZL3072X Device-Specific addresses and limits */
#define SPI_W_BYTE_72X                  0x02
#define SPI_R_BYTE_72X                  0x03
#endif

#if defined ZLS30701_INCLUDED
/* Microsemi ZL3070X Device-Specific addresses and limits */
#define DEVICE_PAGE_SEL_REG_ADDR_70X    0x7F
#define MIN_PAGED_ADDR_70X              0x00
#define MAX_PAGED_ADDR_70X              0x7F
#define MAX_PAGES_70X                   0x0E
#endif

#if defined ZLS30751_INCLUDED
/* Microsemi ZL3075X Device-Specific addresses and limits */
#define DEVICE_PAGE_SEL_REG_ADDR_75X    0x7F
#define MIN_PAGED_ADDR_75X              0x00
#define MAX_PAGED_ADDR_75X              0x7F
#define MAX_PAGES_75X                   0x0E
#endif

#if defined ZLS30731_INCLUDED
/* ZL3070X Device-Specific addresses are in zl303xx_AddressMap73x.h (where they belong) */
#endif
#if defined ZLS30771_INCLUDED
/* ZL3070X Device-Specific addresses and limits moved to zl303xx_AddressMap77x.h (where they belong) */
#endif

#define SPI_W_BIT       0x7F
#define SPI_R_BIT       0x80

#ifndef CONFIG_CPM_SPI_BDSIZE                   /* Buffer desc. (in bytes) */
   #define CONFIG_CPM_SPI_BDSIZE 25
#endif
#define DRV_LIMIT (CONFIG_CPM_SPI_BDSIZE -1)    /* Limit set in the Linux SPI driver(25) (-1 for address) */


#ifdef __cplusplus
}
#endif

#endif /* MULTIPLE INCLUDE BARRIER */

