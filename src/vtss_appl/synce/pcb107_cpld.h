/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef _VTSS_PCB107_CPLD_H_
#define _VTSS_PCB107_CPLD_H_

#include "main_types.h"

void pcb107_cpld_mux_set(u32 mux, u32 input);
void pcb107_cpld_init(void);

mesa_rc vtss_ts_spi_fpga_read(vtss_phy_timestamp_t    *const ts,
                              vtss_phy_ts_fifo_sig_t  *const signature,
                              int                     *const status,
                              u16                     *const channel_no,
                              const u16               port_no);

void pcb107_fifo_clear(void);
void pcb107_cpld_read(uchar address, uchar *data);
void pcb107_cpld_write(uchar address, uchar data);

void pcb135_cpld_mux_set(u32 mux, u32 input);
void pcb135_cpld_init(void);
void pcb135_cpld_read(uchar address, uchar *data);
void pcb135_cpld_write(uchar address, uchar data);
#endif /* _VTSS_PCB107_CPLD_H_ */
