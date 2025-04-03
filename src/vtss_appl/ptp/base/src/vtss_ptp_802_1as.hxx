/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef VTSS_PTP_802_1AS_H
#define VTSS_PTP_802_1AS_H

//#include "vtss_ptp_types.h"
//#include "vtss_ptp_packet_callout.h"
//#include "vtss_ptp_sys_timer.h"
#include "vtss_ptp_internal_types.h"
//#include "vtss_ptp_clock.h"
//#include "vtss_ptp_unicast_master_table.h"
//#include "vtss_ptp_unicast_slave_table.h"

void issue_message_interval_request(PtpPort_t *ptpPort, i8 txAnv, i8 txSyv, i8 txMpr, u8 flags, ptp_cmlds_port_ds_t *port_cmlds, bool cmlds_msg);
void issue_message_gptp_tlv(PtpPort_t *ptpPort, i8 txGptp, u32 org_subtype);
void vtss_ptp_tlv_organization_extension_process(TLV *tlv, PtpPort_t *ptpPort, ptp_cmlds_port_ds_t *port_cmlds, bool cmlds_msg);

size_t vtss_ptp_tlv_follow_up_tlv_insert(u8 *tx_buf, size_t buflen, ptp_follow_up_tlv_info_t *follow_up_info);

mesa_rc vtss_ptp_tlv_follow_up_tlv_process(u8 *tx_buf, size_t buflen, ptp_follow_up_tlv_info_t *follow_up_info);

#endif
