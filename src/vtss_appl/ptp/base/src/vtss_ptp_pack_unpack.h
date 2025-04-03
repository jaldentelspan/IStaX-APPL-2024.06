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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef VTSS_PTP_PACK_UNPACK_H
#define VTSS_PTP_PACK_UNPACK_H

#include "vtss_ptp_types.h"
#include "vtss_ptp_internal_types.h"

void vtss_ptp_pack_header(u8 *buf, const MsgHeader *header);
void vtss_ptp_unpack_header(const u8 *buf, MsgHeader *header);
void vtss_ptp_pack_timestamp(u8 *buf, const mesa_timestamp_t *ptp_time);
void vtss_ptp_pack_correctionfield(u8 *buf, const mesa_timeinterval_t *corr);
void vtss_ptp_update_correctionfield(u8 *buf, mesa_timeinterval_t *corr);
void vtss_ptp_pack_sourceportidentity(u8 *buf, const vtss_appl_ptp_port_identity *sourcePortIdentity);

void vtss_ptp_pack_msg44(u8 *buf, const MsgHeader *header, const mesa_timestamp_t *originTimestamp);

void vtss_ptp_unpack_timestamp(const u8 *buf, mesa_timestamp_t *ptp_time);
void vtss_ptp_unpack_delay_resp(const u8 *buf, MsgDelayResp *resp);

void vtss_ptp_pack_announce(u8 *buf, const mesa_timestamp_t *originTimestamp, ptp_clock_t *clock, u16 sequenceNo, AnnounceDS * ann_ds);
void vtss_ptp_unpack_announce(const u8 *buf, ForeignMasterDS *announce);
void vtss_ptp_unpack_announce_msg(const u8 *buf, MsgAnnounce *announce);
void vtss_ptp_unpack_steps_removed(const u8 *buf, u16 *steps_removed);

void vtss_ptp_pack_msg_pdelay_xx(u8 *buf, MsgHeader *header, mesa_timestamp_t *timestamp, vtss_appl_ptp_port_identity *port_id);
void vtss_ptp_unpack_msg_pdelay_xx(u8 *buf, mesa_timestamp_t *timestamp, vtss_appl_ptp_port_identity *port_id);
void vtss_ptp_pack_transparent_follow_up(u8 *buf, MsgHeader *header,
                                        mesa_timestamp_t *preciseOriginTimestamp, mesa_timeinterval_t *correctionValue);

void vtss_ptp_update_flags(u8 *buf, u8 *flags);

void vtss_ptp_pack_signalling(u8 *buf, const vtss_appl_ptp_port_identity *targetPortIdentity, const vtss_appl_ptp_port_identity *sourcePortIdentity,
                              u8 domain, u8 transport, u16 version, u16 minorVersion, u16 sequenceId, u16 packetLength, u8 flag, u8 controlField);
void vtss_ptp_unpack_signalling(const u8 *buf, MsgSignalling *resp);
int vtss_ptp_pack_tlv(u8 *buf, u16 length, const TLV *tlv);
int vtss_ptp_unpack_tlv(const u8 *buf, u16 length, TLV *tlv);

#endif

