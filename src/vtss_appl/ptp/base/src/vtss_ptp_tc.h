/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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
/**
 * \file
 * \brief PTP Transparent Clock statemachine function.
 * \details Define the interface for an PTP tc protocol handler.
 */

#ifndef VTSS_PTP_TC_H
#define VTSS_PTP_TC_H

#include "vtss_ptp_types.h"
#include "vtss_ptp_internal_types.h"

void vtss_ptp_tc_create(ptp_tc_t *tc, u32 max_outstanding_records, ptp_clock_t *clock);
void vtss_ptp_tc_delete(ptp_tc_t *tc);
void vtss_ptp_tc_enable(ptp_tc_t *tc);
void vtss_ptp_tc_disable(ptp_tc_t *tc);

bool vtss_ptp_tc_sync(ptp_tc_t *tc, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort);
bool vtss_ptp_tc_follow_up(ptp_tc_t *tc, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort);
bool vtss_ptp_tc_delay_req(ptp_tc_t *tc, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort);
bool vtss_ptp_tc_delay_resp(ptp_tc_t *tc, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort);
bool vtss_ptp_tc_general(ptp_tc_t *tc, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort);

#endif
