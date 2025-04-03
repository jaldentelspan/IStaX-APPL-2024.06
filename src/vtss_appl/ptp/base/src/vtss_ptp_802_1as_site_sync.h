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

#ifndef VTSS_PTP_802_1AS_SITE_SYNC_H
#define VTSS_PTP_802_1AS_SITE_SYNC_H

//#include "vtss_ptp_types.h"
//#include "vtss_ptp_api.h"
//#include "vtss_ptp_internal_types.h"
#ifdef __cplusplus
        extern "C" {
#endif

#if defined (VTSS_SW_OPTION_P802_1_AS)
void vtss_ptp_802_1as_site_synccreate(ptp_clock_t *ptpClock);
bool vtss_ptp_802_1as_site_syncsync(ptp_clock_t *ptpClock, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header,vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort);
bool vtss_ptp_802_1as_site_syncfollow_up(ptp_clock_t *ptpClock, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header,vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort);
void vtss_ptp_802_1as_site_sync_update(ptp_clock_t *ptpClock, bool slvCreate);
#endif //(VTSS_SW_OPTION_P802_1_AS)

#ifdef __cplusplus
}
#endif

#endif
