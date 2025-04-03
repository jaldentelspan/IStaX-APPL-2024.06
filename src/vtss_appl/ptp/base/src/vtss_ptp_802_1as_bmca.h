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

#ifndef VTSS_PTP_802_1AS_BMCA_H
#define VTSS_PTP_802_1AS_BMCA_H

//#include "vtss_ptp_types.h"
//#include "vtss_ptp_api.h"
//#include "vtss_ptp_internal_types.h"
#ifdef __cplusplus
        extern "C" {
#endif

#if defined (VTSS_SW_OPTION_P802_1_AS)
void vtss_ptp_802_1as_bmca_init(ptp_clock_t *ptpClock);
void vtss_ptp_802_1as_bmca_state(PtpPort_t *ptpPort, bool enable);
void vtss_ptp_802_1as_bmca_ascapable(PtpPort_t *ptpPort);
void vtss_ptp_802_1as_bmca_new_info(ptp_clock_t *ptpClock);

bool vtss_ptp_port_announce_information(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, ptp_path_trace_t *path_sequence);
void vtss_ptp_801_1as_bmca_announce_virtual(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, ClockDataSet *clock_ds, vtss_appl_ptp_clock_timeproperties_ds_t *time_prop);
#endif //(VTSS_SW_OPTION_P802_1_AS)

#ifdef __cplusplus
}
#endif

#endif
