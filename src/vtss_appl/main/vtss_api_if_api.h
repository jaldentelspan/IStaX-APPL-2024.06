/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.
*/

#ifndef _VTSS_API_IF_API_H_
#define _VTSS_API_IF_API_H_

#include "board_if.h"
#include "main_types.h" /* For vtss_init_data_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize module */
mesa_rc vtss_api_if_init(vtss_init_data_t *data);

/* Get number of chips making up this target */
u32 vtss_api_if_chip_count(void);

// Set SPI interface for register access rather than using the normal UIO
// device. This is called very early in the startup-process - before board
// initialization.
void vtss_api_if_spi_reg_io_set(const char *spi_dev, int spi_pad, int spi_freq);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_API_IF_API_H_ */
