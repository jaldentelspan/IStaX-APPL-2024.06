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

#ifndef VTSS_PTP_PEER_DELAY_H
#define VTSS_PTP_PEER_DELAY_H

#include "vtss_ptp_packet_callout.h"
#include "vtss_ptp_internal_types.h"

/* Flags used in the Message Interval Request messages of 802.1AS 2020*/
#define PTP_PDELAY_COMP_NBR_RRATIO 0x1
#define PTP_PDELAY_COMP_MEAN_DELAY 0x2
/*Common Mean Delay Service uses clock from domain 0 as reference for calculations. */
#define CMLDS_DOMAIN 0
#define PEER_DOMAIN_NUMBER 0

/**
 * ptp peer delay mechanism implementation
 */

void vtss_ptp_peer_delay_create(PeerDelayData *peer_delay, vtss_appl_ptp_protocol_adr_t *ptp_dest, ptp_tag_conf_t *tag_conf);
void vtss_ptp_peer_delay_delete(PeerDelayData *peer_delay);
void vtss_ptp_peer_delay_request_stop(PeerDelayData *peer_delay);

bool vtss_ptp_peer_delay_resp(PeerDelayData *peer_delay, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header);
bool vtss_ptp_peer_delay_resp_follow_up(PeerDelayData *peer_delay, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header);
bool vtss_ptp_peer_delay_req(PeerDelayData *peer_delay, ptp_tx_buffer_handle_t *tx_buf);

void vtss_ptp_peer_delay_state_init(PeerDelayData *peer_delay);
void vtss_ptp_set_comp_ratio_pdelay(PtpPort_t *ptpPort, u8 flags, bool signalling, ptp_cmlds_port_ds_t *port_cmlds, bool cmlds);

void vtss_ptp_pdelay_vid_update(PeerDelayData *peer_delay, ptp_tag_conf_t *tag_conf);
#endif
