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

#include "vtss_ptp_api.h"
#include "vtss_ptp_os.h"
#include "vtss_ptp_802_1as.hxx"
#include "vtss_ptp_pack_unpack.h"
#include "vtss_tod_api.h"
#include "vtss_ptp_peer_delay.h"

static const u32 ORGANIZATION_ID = 0x0080C2;
static const u32 ORGANIZATION_SUBTYPE = 0x02;
static const u32 ORGANIZATION_SUBTYPE_FOLLOW_UP = 0x01;
static const u32 ORGANIZATION_SUBTYPE_GPTP_CAPABLE = 0x04;
static const u32 ORGANIZATION_SUBTYPE_GPTP_MSG_INTVL = 0x05;
static const mesa_mac_t ptp_802_1as_mcast_adr  = {{0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E}};

/**
 * pack and send signalling message: Message Interval Request.
 * This function can be called by ptpPort or CMLDS port. So, either ptpPort or port_cmlds contains a valid pointer during function call but not both.
 */
void issue_message_interval_request(PtpPort_t *ptpPort, i8 txAnv, i8 txSyv, i8 txMpr, u8 flags, ptp_cmlds_port_ds_t *port_cmlds, bool cmlds_msg)
{
    ptp_clock_t *ptpClock;
    TLV tlv;
    mesa_rc rc = VTSS_RC_ERROR;
    u8 tlv_value[12];
    u16 packetLength = SIGNALLING_MIN_PACKET_LENGTH + MESSAGE_INTERVAL_REQUEST_TLV_LENGTH;
    u8 *frame;
    vtss_ptp_tag_t tag = {0, 0, 0}; //section 11.3.3, 8.4.4 - 802.1AS frames will not have any tag;
    size_t buffer_size;
    size_t header_size;
    vtss_appl_ptp_port_identity targetPortIdentity, *portId;
    vtss_appl_ptp_protocol_adr_t addr;
    u8 domainNumber, sdoId;
    u16 version, minorVersion;
    int inst;
    u64 port_mask;
    u16 seq_id;

    addr.ip = 0;
    if (cmlds_msg) {
        addr.mac = ptp_802_1as_mcast_adr;
        portId = &port_cmlds->status.portIdentity;
        domainNumber = inst = CMLDS_DOMAIN;
        sdoId = MAJOR_SDOID_CMLDS_802_1AS;
        version = VERSION_PTP;
        minorVersion = MINOR_VERSION_PTP;
        port_mask = (((u64)1) << port_cmlds->uport);
        seq_id = port_cmlds->port_signaling_message_sequence_number++;
    } else {
        ptpClock = ptpPort->parent;
        addr = ptpClock->ptp_pdelay;
        portId = &ptpPort->portDS.status.portIdentity;
        domainNumber = ptpClock->clock_init->cfg.domainNumber;
        sdoId = ptpClock->majorSdoId;
        version = ptpPort->port_config->versionNumber;
        minorVersion = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
        inst = ptpClock->localClockId;
        port_mask = ptpPort->port_mask;
        seq_id = ptpPort->port_signaling_message_sequence_number++;
    }
    buffer_size = vtss_1588_prepare_general_packet(&frame, &addr, packetLength, &header_size, inst);
    if (buffer_size) {
        memset(&targetPortIdentity, 0xff, sizeof(targetPortIdentity));
        vtss_ptp_pack_signalling(frame + header_size,&targetPortIdentity, portId, domainNumber, sdoId, version, minorVersion, seq_id, packetLength, 0, 0);
        /* Insert TLV field */
        tlv.tlvType = TLV_ORGANIZATION_EXTENSION;
        tlv.lengthField = 12;
        tlv.valueField = tlv_value;
        vtss_tod_pack24(ORGANIZATION_ID, &tlv_value[0]); // organizationid is 00-80-C2
        vtss_tod_pack24(ORGANIZATION_SUBTYPE, &tlv_value[3]); // organization subtype
        tlv_value[6] = (u8)txMpr;   //linkDelayInterval;
        tlv_value[7] = (u8)txSyv;   //timeSyncInterval;
        tlv_value[8] = (u8)txAnv;   //announceInterval;
        tlv_value[9] = flags;       // bit 0 = computeNeighborRateRatio, bit 1 = computeNeighborRateRatio, bit 2 = oneStepReceiveCapable. (Table 10-15 has not been changed according to the change in bit numbering from 1..8 to 0..7)
        vtss_tod_pack16(0, &tlv_value[10]); // reserved

        rc = vtss_ptp_pack_tlv(frame+header_size+SIGNALLING_MIN_PACKET_LENGTH, (u16)buffer_size-SIGNALLING_MIN_PACKET_LENGTH, &tlv);
        if (VTSS_RC_OK == rc) {
            if (!vtss_1588_tx_general(port_mask, frame, header_size + packetLength, &tag))
                T_WG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "sending MessageIntervalRequest failed");
            else
                T_DG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "sent MessageIntervalRequest message");
        } else {
            T_EG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "Transmit buffer too small");
            vtss_1588_release_general_packet(&frame);
        }
    }
}

/**
 * pack and send signalling message: Gptp Capable/Message Interval Request.
 */
void issue_message_gptp_tlv(PtpPort_t *ptpPort, i8 txGptp, u32 org_subtype)
{
    ptp_clock_t *ptpClock = ptpPort->parent;
    TLV tlv;
    mesa_rc rc = VTSS_RC_ERROR;
    u8 tlv_value[12];
    u16 packetLength, minorVersion;
    if (org_subtype == ORGANIZATION_SUBTYPE_GPTP_CAPABLE){
        packetLength = SIGNALLING_MIN_PACKET_LENGTH + GPTP_CAPABLE_TLV_LENGTH ;
    } else {
         packetLength = SIGNALLING_MIN_PACKET_LENGTH + GPTP_CAPABLE_MESSAGE_INTERVAL_REQUEST_TLV_LENGTH ;
    }
    minorVersion = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
    u8 *frame;
    vtss_ptp_tag_t tag = {0, 0, 0}; //section 11.3.3, 8.4.4 - 802.1AS frames will not have any tag;
    size_t buffer_size;
    size_t header_size;
    vtss_appl_ptp_port_identity targetPortIdentity;
    buffer_size = vtss_1588_prepare_general_packet(&frame, &ptpClock->ptp_pdelay, packetLength, &header_size, ptpClock->localClockId);
    if (buffer_size) {
        memset(&targetPortIdentity, 0xff, sizeof(targetPortIdentity));
        vtss_ptp_pack_signalling(frame + header_size,&targetPortIdentity, &ptpPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.domainNumber, ptpClock->majorSdoId, ptpPort->port_config->versionNumber, minorVersion, ptpPort->port_signaling_message_sequence_number++, packetLength, 0, 0);

        /* Insert TLV field */
        if (org_subtype == ORGANIZATION_SUBTYPE_GPTP_CAPABLE){
            tlv.lengthField = 12;
            tlv.tlvType = TLV_ORGANIZXATION_EXTENSION_DO_NOT_PROPAGATE;
        } else {
            tlv.lengthField = 10;
            tlv.tlvType = TLV_ORGANIZATION_EXTENSION;
        }
        tlv.valueField = tlv_value;
        vtss_tod_pack24(ORGANIZATION_ID, &tlv_value[0]); // organizationid is 00-80-C2
        vtss_tod_pack24(org_subtype, &tlv_value[3]); // organization subtype
        tlv_value[6] = (u8)txGptp;   //gptp message interval;
        if (org_subtype == ORGANIZATION_SUBTYPE_GPTP_CAPABLE){
            tlv_value[7] = 0x0;    //flags
            vtss_tod_pack32(0, &tlv_value[8]); // reserved
        }else {
            vtss_tod_pack24(0, &tlv_value[7]); // reserved
        }

        rc = vtss_ptp_pack_tlv(frame+header_size+SIGNALLING_MIN_PACKET_LENGTH, (u16)buffer_size-SIGNALLING_MIN_PACKET_LENGTH, &tlv);
        if (VTSS_RC_OK == rc) {
            if (!vtss_1588_tx_general(ptpPort->port_mask,frame, header_size + packetLength, &tag)){
                T_WG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "sending Gptp tlv org_subtype = %d failed",org_subtype);
            }else{
                T_DG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "sent Gptp tlv org_subtype = %d message",org_subtype);
            }
        } else {
            T_EG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "Transmit buffer too small");
            vtss_1588_release_general_packet(&frame);
        }
    }
}
/**
 * unpack the Message Interval Request, and update the current message intervals.
 * This function can be called by ptpPort or CMLDS port. So, either ptpPort or port_cmlds contains a valid pointer during function call but not both at sametime.
 */
void vtss_ptp_tlv_organization_extension_process(TLV *tlv, PtpPort_t *ptpPort, ptp_cmlds_port_ds_t *port_cmlds, bool cmlds_msg)
{
    u32 organizationId;
    u32 organizationSubType;
    switch (tlv->tlvType) {
        case TLV_ORGANIZATION_EXTENSION:
        case TLV_ORGANIZXATION_EXTENSION_DO_NOT_PROPAGATE:
            organizationId = vtss_tod_unpack24(&tlv->valueField[0]);
            organizationSubType = vtss_tod_unpack24(&tlv->valueField[3]);
            if (organizationId == ORGANIZATION_ID && (organizationSubType == ORGANIZATION_SUBTYPE)) {
                if (!cmlds_msg) {
                    // set interval parameters
                    ptp_802_1as_set_current_message_interval(ptpPort, tlv->valueField[8], tlv->valueField[7], tlv->valueField[6]);
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "Setting message intervals:  Anv %d, Syv %d, Mpr %d",
                         (i8)tlv->valueField[8], (i8)tlv->valueField[7], (i8)tlv->valueField[6]);
                } else {
#if defined (VTSS_SW_OPTION_P802_1_AS)
                    vtss_ptp_cmlds_set_pdelay_interval(port_cmlds, tlv->valueField[6], true);
#endif
                }
                // Process flags
                vtss_ptp_set_comp_ratio_pdelay(ptpPort, (u8)tlv->valueField[9], true, port_cmlds, cmlds_msg);
            } else if (organizationId == ORGANIZATION_ID && organizationSubType == ORGANIZATION_SUBTYPE_GPTP_CAPABLE){
                i32 gPtpCapableReceiptTimeoutInterval;
                gPtpCapableReceiptTimeoutInterval = PTP_LOG_TIMEOUT(ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval) * ptpPort->port_config->c_802_1as.gPtpCapableReceiptTimeout;
                ptpPort->neighborGptpCapable = TRUE;
                vtss_ptp_timer_start(&ptpPort->gptpsm.bmca_802_1as_gptp_rx_timeout_timer, gPtpCapableReceiptTimeoutInterval, false);
                T_IG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "Received gptp capable tlv");

            } else if (organizationId == ORGANIZATION_ID && organizationSubType == ORGANIZATION_SUBTYPE_GPTP_MSG_INTVL){
                ptp_802_1as_set_gptp_current_message_interval(ptpPort, tlv->valueField[6]);
                T_IG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "Setting gptp message interval: %d",
                                                     (i8)tlv->valueField[6]);
            } else {
                T_WG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "Unknown organizationId 0x%x or organizationSubType 0x%x",
                     organizationId, organizationSubType);
            }
            break;
        default:
            T_WG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "Unknown signalling TLV type received: tlvType %d", tlv->tlvType);
            break;
    }
}

size_t vtss_ptp_tlv_follow_up_tlv_insert(u8 *tx_buf, size_t buflen, ptp_follow_up_tlv_info_t *follow_up_info)
{
    TLV tlv;
    u8 tlv_value[FOLLOW_UP_TLV_LENGTH];
    /* Insert TLV field */
    if (buflen >= FOLLOW_UP_TLV_LENGTH) {
        tlv.tlvType = TLV_ORGANIZATION_EXTENSION;
        tlv.lengthField = FOLLOW_UP_TLV_LENGTH -4;
        tlv.valueField = tlv_value;
        vtss_tod_pack24(ORGANIZATION_ID, &tlv_value[0]); // organizationid is 00-80-C2
        vtss_tod_pack24(ORGANIZATION_SUBTYPE_FOLLOW_UP, &tlv_value[3]); // organization subtype
        T_DG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "buflen " VPRIz ", rateOffset %d, gmTimeBaseIndicator %d", buflen, follow_up_info->cumulativeScaledRateOffset, follow_up_info->gmTimeBaseIndicator);
        vtss_tod_pack32(follow_up_info->cumulativeScaledRateOffset, &tlv_value[6]);
        vtss_tod_pack16(follow_up_info->gmTimeBaseIndicator, &tlv_value[10]);
        vtss_tod_pack32(follow_up_info->lastGmPhaseChange.scaled_ns_high, &tlv_value[12]);
        vtss_tod_pack64(follow_up_info->lastGmPhaseChange.scaled_ns_low, &tlv_value[16]);
        vtss_tod_pack32(follow_up_info->scaledLastGmFreqChange, &tlv_value[24]);
        if (VTSS_RC_OK == vtss_ptp_pack_tlv(tx_buf, buflen, &tlv)) {
            return FOLLOW_UP_TLV_LENGTH;
        }
    }
    return 0;
}

mesa_rc vtss_ptp_tlv_follow_up_tlv_process(u8 *tx_buf, size_t buflen, ptp_follow_up_tlv_info_t *follow_up_info)
{
    TLV tlv;
    const u8 *tlv_value;
    u32 org_id, sub_type;
    mesa_rc rc = VTSS_RC_ERROR;
    // Decode Follow_Up information TLV
    if (buflen >= FOLLOW_UP_TLV_LENGTH) {
        if (VTSS_RC_OK == vtss_ptp_unpack_tlv(tx_buf, buflen, &tlv)) {
            tlv_value = tlv.valueField;
            T_D("process Follow_up Tlv extension with type %d and length %d", tlv.tlvType, tlv.lengthField);
            if (tlv.tlvType == TLV_ORGANIZATION_EXTENSION && tlv.lengthField == FOLLOW_UP_TLV_LENGTH -4) {
                org_id = vtss_tod_unpack24(&tlv_value[0]); // organizationid is 00-80-C2
                sub_type = vtss_tod_unpack24(&tlv_value[3]); // organization subtype
                if (org_id == ORGANIZATION_ID && sub_type == ORGANIZATION_SUBTYPE_FOLLOW_UP) {
                    follow_up_info->cumulativeScaledRateOffset = vtss_tod_unpack32(&tlv_value[6]);
                    follow_up_info->gmTimeBaseIndicator = vtss_tod_unpack16(&tlv_value[10]);
                    follow_up_info->lastGmPhaseChange.scaled_ns_high = vtss_tod_unpack32(&tlv_value[12]);
                    follow_up_info->lastGmPhaseChange.scaled_ns_low = vtss_tod_unpack64(&tlv_value[16]);
                    follow_up_info->scaledLastGmFreqChange = vtss_tod_unpack32(&tlv_value[24]);
                    rc = VTSS_RC_OK;
                    T_D("Follow_Up Tlv: cumulativeScaledRateOffset %d, gmTimeBaseIndicator %d, lastGmPhaseChange high %d, low %" PRIu64 ", scaledLastGmFreqChange % d", follow_up_info->cumulativeScaledRateOffset,
                              follow_up_info->gmTimeBaseIndicator,
                              follow_up_info->lastGmPhaseChange.scaled_ns_high,
                              follow_up_info->lastGmPhaseChange.scaled_ns_low,
                              follow_up_info->scaledLastGmFreqChange);
                } else {
                    T_W("Unsupported Follow_Up Tlv subtype");
                }
            } else {
                T_W("Unsupported Follow_Up Tlv extension");
            }
        }
    } else {
        T_W("Follow_Up Tlv extension size error");
    }
    return rc;
}

/*
 * gptp transmit timer
 */

void vtss_ptp_bmca_gptp_transmit_timer(vtss_ptp_sys_timer_t *timer, void *t)
{
    PtpPort_t *ptpPort = (PtpPort_t *)t;
    if(ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS){
        u32 to;
        i8 txGptp = ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval;
        if (ptpPort->gptpsm.gPtpCapableMessageSlowdown)
        {
            if ((ptpPort->gptpsm.numberGptpCapableMessageTransmissions+1) >= ptpPort->port_config->c_802_1as.gPtpCapableReceiptTimeout)
            {
                ptpPort->gptpsm.gptp_log_msg_period = txGptp;
                ptpPort->gptpsm.numberGptpCapableMessageTransmissions = 0;
                ptpPort->gptpsm.gPtpCapableMessageSlowdown = FALSE;
            }
            else
            {
                ptpPort->gptpsm.numberGptpCapableMessageTransmissions++;
            }
        }
        else
        {
            ptpPort->gptpsm.numberGptpCapableMessageTransmissions = 0;
            ptpPort->gptpsm.gptp_log_msg_period = txGptp;
        }
        if (txGptp != 127) {
            issue_message_gptp_tlv(ptpPort, txGptp, ORGANIZATION_SUBTYPE_GPTP_CAPABLE);
            T_NG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "Issued gptp capable tlv");
            to = PTP_LOG_TIMEOUT(ptpPort->gptpsm.gptp_log_msg_period);
            vtss_ptp_timer_start(&ptpPort->gptpsm.bmca_802_1as_gptp_tx_timer, to, true);
        }
    }
}
