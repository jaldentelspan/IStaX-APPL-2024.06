/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef VTSS_PTP_BMCA_H
#define VTSS_PTP_BMCA_H

#include "vtss_ptp_types.h"
#include "vtss_ptp_api.h"
#include "vtss_ptp_internal_types.h"

void vtss_ptp_bmca_init(ptp_clock_t *ptpClock);
void vtss_ptp_bmca_state(PtpPort_t *ptpPort, bool enable);

bool vtss_ptp_bmca_announce_slave(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, ptp_path_trace_t *path_sequence);
bool vtss_ptp_bmca_announce_master(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header);
void vtss_ptp_bmca_m1(ptp_clock_t *);
void vtss_ptp_bmca_announce_virtual(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, ClockDataSet *clock_ds, vtss_appl_ptp_clock_timeproperties_ds_t *time_prop);

#endif
