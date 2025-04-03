/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _PTP_AFI_
#define _PTP_AFI_
/******************************************************************************/
//
// This header file contains various definitions and functions for
// implementing the 1pps synchronization feature supported bu the Gen2
// PHY's.
//
//
/******************************************************************************/

#include "vtss_ptp_packet_callout.h"

typedef struct {
    i8 log_sync_interval;
    BOOL switch_port;
    uint32_t clk_domain;
} ptp_afi_setup_t;

/*
 * 1PPS synchronization configuration.
 */
typedef struct ptp_afi_conf_s {
public:
    mesa_rc ptp_afi_sync_conf_start(mesa_port_no_t port_no, ptp_tx_buffer_handle_t *ptp_buf_handle, const ptp_afi_setup_t *conf);
    mesa_rc ptp_afi_sync_conf_stop(void);
    mesa_rc ptp_afi_packet_tx(u32 *cnt_delta);
    ptp_afi_conf_s(void);
    ptp_afi_conf_s(u32 idx, u32 port_no);
    i32 my_seq_cntr;  // sequence counter index. Allocated dynamically
    i32 my_port_no;
private:
    u32 single_id;  // id allocated for the AFI instance
    BOOL active;    // indicates if this object's AFI is active
    u16 my_tx_cnt;     // latest counter read from the AFI

} ptp_afi_conf_t;

mesa_rc ptp_afi_alloc(ptp_afi_conf_s **afi, mesa_port_no_t port_no);
mesa_rc ptp_afi_free(ptp_afi_conf_s **afi);
mesa_rc ptp_afi_available(u32 afi_request, bool *available);

/******************************************************************************/
// Interface functions.
/******************************************************************************/

#endif /* _PTP_AFI_ */

