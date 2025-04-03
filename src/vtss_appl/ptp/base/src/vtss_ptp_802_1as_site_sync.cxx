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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "vtss_ptp_api.h"
#include "vtss_ptp_os.h"
#include "vtss_ptp_sys_timer.h"
#include "vtss_ptp_pack_unpack.h"
#include "vtss_ptp_packet_callout.h"
#include "vtss_tod_api.h"
#include "vtss_ptp_802_1as_site_sync.h"
#include "vtss_ptp_local_clock.h"
#include "misc_api.h"
#include "vtss_ptp_802_1as.hxx"
/*
 * Forward declarations
 */

/*
 * private functions
 */

#if defined (VTSS_SW_OPTION_P802_1_AS)
/*
 * create a PTP site sync clock instance
 */
void vtss_ptp_802_1as_site_synccreate(ptp_clock_t *ptpClock)
{
    ptpClock->port_802_1as_sync_sync.port_mask = 0;
    ptpClock->port_802_1as_sync_sync.rateRatio = 1;
}

// When Site sync structures are activated, sync messages from master device
// will be forwarded on all master ports. Hence, existing sync timers in master
// port should be stopped.
void vtss_ptp_802_1as_site_sync_update(ptp_clock_t *ptpClock, bool slvCreate)
{
    u16 portIdx;
    for (portIdx = 0; portIdx < ptpClock->clock_init->numberEtherPorts; portIdx++) {
        if (portIdx == uport2iport(ptpClock->slavePort)) {
            continue;
        }
        PtpPort_t *ptpPort = &ptpClock->ptpPort[portIdx];
        if (slvCreate) {
            if (ptpPort->portDS.status.portState == VTSS_APPL_PTP_MASTER) {
                vtss_ptp_timer_stop(&ptpPort->msm.sync_timer);
            }
        } else {
            if (ptpPort->portDS.status.portState == VTSS_APPL_PTP_MASTER) {
                if (ptpPort->msm.sync_log_msg_period != 127) {
                    vtss_ptp_timer_start(&ptpPort->msm.sync_timer, PTP_LOG_TIMEOUT(ptpPort->msm.sync_log_msg_period), true);
                }
            }
        }
    }
}

/*
 * 2-step sync packet event transmitted.
 *
 */
/*lint -esym(459, vtss_ptp_tc_sync_event_transmitted) */
static void vtss_ptp_802_1as_site_syncsync_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time)
{
    u64 res_time;
    char str1[30];
    vtss_timeinterval_t correctionValue;
    u8 *frame;
    vtss_ptp_tag_t tag;
    size_t buffer_size;
    size_t header_size;
    ptp_clock_t *ptpClock = (ptp_clock_t *)context;
    PtpPort_t *ptpPort = NULL;

    if (ptpClock != NULL) {
        uint32_t cumulativeScaledRateOffset = htonl(ptpClock->parentDS.par_802_1as.cumulativeRateRatio);
        ptpPort = &ptpClock->ptpPort[portnum-1];
        // it is a multicast event
        if ((ptpPort->portDS.status.portState == VTSS_APPL_PTP_MASTER)
                && ptpClock->clock_init->cfg.twoStepFlag) {  /* sync forwarded in a 802.1as node */
            T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"sync eventTransmitted, txTime = " VPRI64x ", receive_time = " VPRI64x ", portnum %d rateRatio %g",
                tx_time, ptpClock->port_802_1as_sync_sync.sync_ingress_time, portnum, ptpClock->port_802_1as_sync_sync.rateRatio);
            vtss_1588_ts_cnt_sub(&res_time,tx_time,ptpClock->port_802_1as_sync_sync.sync_ingress_time);
            vtss_local_clock_convert_to_time(ptpClock->port_802_1as_sync_sync.sync_ingress_time,&ptpClock->port_802_1as_sync_sync.last_sync_ingress_time, ptpClock->localClockId);
            vtss_1588_ts_cnt_to_timeinterval(&ptpClock->port_802_1as_sync_sync.syncResidenceTime[portnum-1], res_time);
            if (ptpClock->clock_init->cfg.clock_domain >= fast_cap(MESA_CAP_TS_DOMAIN_CNT)) {
                //convert res_time into grandmaster time base.
                ptpClock->port_802_1as_sync_sync.syncResidenceTime[portnum-1] *= ptpClock->port_802_1as_sync_sync.rateRatio;
            }
            /* log residence times > 50 ms */
            if (llabs(ptpClock->port_802_1as_sync_sync.syncResidenceTime[portnum-1]) > (((vtss_timeinterval_t)VTSS_ONE_MIA)<<16)/20) {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"Great residence time %s", vtss_tod_TimeInterval_To_String (&ptpClock->port_802_1as_sync_sync.syncResidenceTime[portnum-1], str1,0));
                T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"txTime: %u ingress_time: %u", tx_time, ptpClock->port_802_1as_sync_sync.sync_ingress_time);
            }

            if (ptpClock->port_802_1as_sync_sync.sync_followup_action == FOLLOW_UP_WAIT_TX_READY) {
                /* followup has already been received, and a copy exists in the msgFbuf */
                correctionValue = ptpClock->port_802_1as_sync_sync.syncResidenceTime[portnum-1] + ptpClock->port_802_1as_sync_sync.rx_delay_asy;
                ptpClock->port_802_1as_sync_sync.port_mask &= ~(1LL<<(portnum-1));

                vtss_appl_ptp_protocol_adr_t *ptp_dest;
                ptp_dest = (ptpPort->port_config->dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT) ? &ptpClock->ptp_primary : &ptpClock->ptp_pdelay;
                buffer_size = vtss_1588_prepare_general_packet(&frame, ptp_dest, ptpClock->port_802_1as_sync_sync.msgFbuf_size,
                                         &header_size, ptpClock->localClockId);
                vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
                if (buffer_size) {
                    memcpy(frame+ header_size, ptpClock->port_802_1as_sync_sync.msgFbuf, ptpClock->port_802_1as_sync_sync.msgFbuf_size);
                    vtss_ptp_pack_sourceportidentity(frame+ header_size, &ptpPort->portDS.status.portIdentity);
                    vtss_ptp_update_correctionfield(frame+ header_size, &correctionValue);
                    // Send to downstream devices about phase change and frequency change.
                    if (ptpClock->localGMUpdate) {
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_TC, "Update local GM properties change in seq-id %d", ptpClock->port_802_1as_sync_sync.syncForwardingHeader.sequenceId);
                        // Update Follow-up TLV lastGmPhaseChange, scaledLastGmFreqChange.
                        vtss_tod_pack32(ptpClock->follow_up_info.lastGmPhaseChange.scaled_ns_high, frame + header_size + PTP_MESSAGE_FOLLOWUP_GM_PHASE_CHANGE);
                        vtss_tod_pack64(ptpClock->follow_up_info.lastGmPhaseChange.scaled_ns_low, frame + header_size + PTP_MESSAGE_FOLLOWUP_GM_PHASE_CHANGE + 4);
                        vtss_tod_pack32(ptpClock->follow_up_info.scaledLastGmFreqChange, frame + header_size + PTP_MESSAGE_FOLLOWUP_GM_FREQ_CHANGE);
                    }
                    // Update cumulativeScaledRateOffset of this node
                    memcpy(frame + header_size + PTP_MESSAGE_FOLLOWUP_CUM_RATE_OFFSET, &cumulativeScaledRateOffset, sizeof(uint32_t));
                    // Ensure controlField is correct
                    ptpClock->port_802_1as_sync_sync.syncForwardingHeader.controlField = ptpPort->port_config->c_802_1as.as2020 ? 0x00 : PTP_FOLLOWUP_MESSAGE;
                    memcpy(frame + header_size + PTP_MESSAGE_CONTROL_FIELD_OFFSET, &ptpClock->port_802_1as_sync_sync.syncForwardingHeader.controlField, sizeof(uint8_t));

                    if (vtss_1588_tx_general(ptpPort->port_mask,frame, buffer_size, &tag)) {
                        T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded Follow Up message to port: %d", ptpPort->portDS.status.portIdentity.portNumber);
                        ptpPort->port_statistics.txFollowUpCount++;
                    }
                }
            }
        } else {
            T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"packet transmitted on port %d that is not master.", portnum);
        }
    } else {
        T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"unexpected context: %p", context);
    }
}

/*
 * Forward sync request from a master
 *
 * forwardsync messages
 * A sync message is forwarded to all PTP ports with portRole = Master.
 * The ingress timestamp has already been saved.
 * The header is saved for later use.
 * if 2step is forwarded then the egress timestamp is saved for residence time calculation
 */
bool vtss_ptp_802_1as_site_syncsync(ptp_clock_t *ptpClock, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header,vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort)
{
    bool forwarded = false;
    MsgSync sync;
    vtss_ptp_unpack_timestamp(buf_handle->frame + buf_handle->header_length, &sync.originTimestamp);
    ptpClock->port_802_1as_sync_sync.last_sync_tx_time = sync.originTimestamp;
    /* forward sync message to other ports */
    T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"seq %d, rxtime " VPRI64d "  correctionField  " VPRI64d, header->sequenceId, buf_handle->hw_time, header->correctionField>>16);
    u16 portIdx;
    u64  port_mask = 0;
    i32 syncReceiptTimeoutInterval;
    size_t header_size;
    ptp_tx_buffer_handle_t tx_handle;
    i32 syncReceiptInterval;
    mesa_bool_t prev_syncLocked;

    T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"two-step mode: %d", ptpClock->clock_init->cfg.twoStepFlag);
    if (ptpClock->clock_init->cfg.twoStepFlag) {
        // refresh sync timer
        ptpClock->parentDS.parentLogSyncInterval = header->logMessageInterval;
        syncReceiptTimeoutInterval = PTP_LOG_TIMEOUT(header->logMessageInterval) * rxptpPort->port_config->c_802_1as.syncReceiptTimeout;
        rxptpPort->portDS.status.s_802_1as.syncReceiptTimeInterval = ((mesa_timeinterval_t)syncReceiptTimeoutInterval * TickTimeNs) << 16;
        if (ptpClock->port_802_1as_sync_sync.sync_followup_action != FOLLOW_UP_WAIT_TX) {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"SYNC timer restarted, timeout = %d", syncReceiptTimeoutInterval);
            vtss_ptp_timer_start(&rxptpPort->bmca_802_1as_sync_timer, syncReceiptTimeoutInterval, false); // Restart SyncReceiptTimeoutTimer
        }
        /* forward twostep */
        if (ptpClock->port_802_1as_sync_sync.port_mask != 0LL) { /* the previous request has not been responded. */
            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"missed followup or previous sync packet has not been forwarded from master to slave on port: %d", rxptpPort->portDS.status.portIdentity.portNumber);
            ptpClock->port_802_1as_sync_sync.port_mask = 0LL;
            if (ptpClock->port_802_1as_sync_sync.sync_followup_action == FOLLOW_UP_WAIT_READY || ptpClock->port_802_1as_sync_sync.sync_followup_action == FOLLOW_UP_WAIT_TX) {
                rxptpPort->port_statistics.rxPTPPacketDiscardCount++;
            }
        }
        /* if the received Sync event is a onestep type, then it is not forwarded in two-step configuration */
        if (!getFlag(header->flagField[0], PTP_TWO_STEP_FLAG)) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"Recieved one-step sync on two-step configured port %d",rxptpPort->portDS.status.portIdentity.portNumber);
        } else {
            ptpClock->port_802_1as_sync_sync.sync_followup_action = FOLLOW_UP_WAIT_TX;
        }
        T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"saving data for follow_up");
        ptpClock->port_802_1as_sync_sync.syncForwardingHeader = *header; /* store the header information to be used when the followup message shall be handled. */
        ptpClock->port_802_1as_sync_sync.rx_delay_asy = rxptpPort->port_config->delayAsymmetry;  /* the delay asymmetry is later added to the correction field of the followup packet*/
        T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"saved data for follow_up");

        ptpClock->port_802_1as_sync_sync.sync_ingress_time = buf_handle->hw_time;
        for (portIdx = 0; portIdx < ptpClock->clock_init->numberEtherPorts; portIdx++) {
            PtpPort_t *ptpPort = &ptpClock->ptpPort[portIdx];
            if (ptpPort != rxptpPort && ptpPort->portDS.status.portState == VTSS_APPL_PTP_MASTER) {
                prev_syncLocked = ptpPort->portDS.status.s_802_1as.syncLocked;
                ptpPort->portDS.status.s_802_1as.syncLocked = 
                                (ptpPort->portDS.status.s_802_1as.currentLogSyncInterval == ptpClock->parentDS.parentLogSyncInterval);
                u64 old_port_mask = ptpClock->port_802_1as_sync_sync.port_mask;
                port_mask = (1LL<<portIdx);
                ptpClock->port_802_1as_sync_sync.syncResidenceTime[portIdx] = 0;
                // forward pr port
                header->sourcePortIdentity = ptpPort->portDS.status.portIdentity;
                vtss_appl_ptp_protocol_adr_t *ptp_dest;
                ptp_dest = (ptpPort->port_config->dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT) ? &ptpClock->ptp_primary : &ptpClock->ptp_pdelay;
                tx_handle.size = vtss_1588_prepare_general_packet(&tx_handle.frame, ptp_dest, buf_handle->size - buf_handle->header_length, &header_size, ptpClock->localClockId);
                ptpClock->port_802_1as_sync_sync.last_ptp_sync_msg_size = (buf_handle->size - buf_handle->header_length);
                if (tx_handle.size) {
                    if (ptpPort->portDS.status.s_802_1as.syncLocked || (prev_syncLocked && !ptpPort->portDS.status.s_802_1as.syncLocked )) {
                        uint32_t ts_id;
                        if(!ptpPort->portDS.status.s_802_1as.syncLocked) {
                            header->logMessageInterval = ptpPort->portDS.status.s_802_1as.currentLogSyncInterval;
                            ptpPort->msm.numberSyncTransmissions++;
                        }else {
                            header->logMessageInterval = ptpClock->parentDS.parentLogSyncInterval;
                        }
                        tx_handle.handle = 0; // Release buffer automatically
                        tx_handle.header_length = header_size;
                        tx_handle.hw_time = buf_handle->hw_time;
                        header->minorVersionPTP = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
                        vtss_ptp_pack_msg44(tx_handle.frame + tx_handle.header_length, header, &sync.originTimestamp);
                        tx_handle.msg_type = VTSS_PTP_MSG_TYPE_2_STEP;
                        ptpClock->port_802_1as_sync_sync.sync_ts_context.cb_ts = vtss_ptp_802_1as_site_syncsync_event_transmitted;
                        ptpClock->port_802_1as_sync_sync.sync_ts_context.context = ptpClock;
                        tx_handle.ts_done = &ptpClock->port_802_1as_sync_sync.sync_ts_context;
                        vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tx_handle.tag);
                        ptpClock->port_802_1as_sync_sync.port_mask |= port_mask;
                        if (!vtss_1588_tx_msg(port_mask, &tx_handle, ptpClock->localClockId, false, &ts_id)) {
                           /* could not forward: therefore free the sync entry in the list */
                            ptpClock->port_802_1as_sync_sync.port_mask = old_port_mask;
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"could not forward two-step Sync message to ports: " VPRI64x, port_mask);
                        } else {
                            T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded two-step Sync message to ports: " VPRI64x, port_mask);
                        }
                        ptpPort->port_statistics.txSyncCount++;
                    }
                } else {
                    T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"cound not allocate buffer for Sync message forwarding to portmask: " VPRI64x, port_mask);
                }
                if (prev_syncLocked && !ptpPort->portDS.status.s_802_1as.syncLocked){
                    if (ptpPort->msm.syncSlowdown){
                        syncReceiptInterval = header->logMessageInterval;
                    } else {
                        syncReceiptInterval = PTP_LOG_TIMEOUT(ptpPort->portDS.status.s_802_1as.currentLogSyncInterval);
                    }
                    vtss_ptp_timer_start(&ptpPort->bmca_802_1as_in_timeout_sync_timer,(i32)(syncReceiptInterval), false);
                }

            }

        }
    } else {
        u32 cumulativeScaledRateOffset = htonl(ptpClock->parentDS.par_802_1as.cumulativeRateRatio);
        // refresh sync timer
        ptpClock->parentDS.parentLogSyncInterval = header->logMessageInterval;
        syncReceiptTimeoutInterval = PTP_LOG_TIMEOUT(header->logMessageInterval) * rxptpPort->port_config->c_802_1as.syncReceiptTimeout;
        rxptpPort->portDS.status.s_802_1as.syncReceiptTimeInterval = ((mesa_timeinterval_t)syncReceiptTimeoutInterval * TickTimeNs) << 16;
        vtss_ptp_timer_start(&rxptpPort->bmca_802_1as_sync_timer, syncReceiptTimeoutInterval, false); // Restart SyncReceiptTimeoutTimer
        ptpClock->port_802_1as_sync_sync.sync_ingress_time = buf_handle->hw_time;

        for (portIdx = 0; portIdx < ptpClock->clock_init->numberEtherPorts; portIdx++) {
            PtpPort_t *ptpPort = &ptpClock->ptpPort[portIdx];
            if (ptpPort != rxptpPort && ptpPort->portDS.status.portState == VTSS_APPL_PTP_MASTER) {
                uint32_t ts_id;
                memcpy(ptpClock->port_802_1as_sync_sync.msgFbuf, buf_handle->frame + buf_handle->header_length, buf_handle->size - buf_handle->header_length);
                ptpClock->port_802_1as_sync_sync.msgFbuf_size = buf_handle->size - buf_handle->header_length;
                ptpPort->portDS.status.s_802_1as.syncLocked =
                                (ptpPort->portDS.status.s_802_1as.currentLogSyncInterval == ptpClock->parentDS.parentLogSyncInterval);
                u64 old_port_mask = ptpClock->port_802_1as_sync_sync.port_mask;
                port_mask = (1LL<<portIdx);
                ptpClock->port_802_1as_sync_sync.syncResidenceTime[portIdx] = 0;
                // forward pr port
                header->sourcePortIdentity = ptpPort->portDS.status.portIdentity;
                vtss_appl_ptp_protocol_adr_t *ptp_dest;
                ptp_dest = (ptpPort->port_config->dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT) ? &ptpClock->ptp_primary : &ptpClock->ptp_pdelay;
                tx_handle.size = vtss_1588_prepare_general_packet(&tx_handle.frame, ptp_dest, buf_handle->size - buf_handle->header_length, &header_size, ptpClock->localClockId);
                ptpClock->port_802_1as_sync_sync.last_ptp_sync_msg_size = (buf_handle->size - buf_handle->header_length);

                if (tx_handle.size) {
                    if(!ptpPort->portDS.status.s_802_1as.syncLocked) {
                        header->logMessageInterval = ptpPort->portDS.status.s_802_1as.currentLogSyncInterval;
                        ptpPort->msm.numberSyncTransmissions++;
                    } else {
                        header->logMessageInterval = ptpClock->parentDS.parentLogSyncInterval;
                    }
                    tx_handle.handle = 0; // Release buffer automatically
                    tx_handle.header_length = header_size;
                    // copy preciseOriginTimestamp and followUp TLV
                    memcpy(tx_handle.frame + header_size + HEADER_LENGTH, buf_handle->frame + header_size + HEADER_LENGTH, buf_handle->size - header_size - HEADER_LENGTH);
                    // Update cumulativeScaledRateOffset of this node
                    memcpy(tx_handle.frame + header_size + PTP_MESSAGE_FOLLOWUP_CUM_RATE_OFFSET, &cumulativeScaledRateOffset, sizeof(int32_t));
                    tx_handle.hw_time = buf_handle->hw_time;
                    header->minorVersionPTP = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
                    vtss_ptp_pack_msg44(tx_handle.frame + tx_handle.header_length, header, &sync.originTimestamp);
                    tx_handle.msg_type = VTSS_PTP_MSG_TYPE_ORG_TIME;
                    tx_handle.ts_done = NULL;
                    vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tx_handle.tag);
                    ptpClock->port_802_1as_sync_sync.port_mask |= port_mask;
                    if (!vtss_1588_tx_msg(port_mask, &tx_handle, ptpClock->localClockId, false, &ts_id)) {
                        /* could not forward: therefore free the sync entry in the list */
                        ptpClock->port_802_1as_sync_sync.port_mask = old_port_mask;
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"could not forward one-step Sync message to ports: " VPRI64x, port_mask);
                    } else {
                        T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded one-step Sync message to ports: " VPRI64x, port_mask);
                    }
                    ptpPort->port_statistics.txSyncCount++;
                } else {
                    T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"cound not allocate buffer for Sync message forwarding to portmask: " VPRI64x, port_mask);
                }
            }

        }
    }
    return forwarded;
}

/*
 * Forward follow_up request from a master
 * A follow up message is forwarded to all active PTP ports.
 *
 */
bool vtss_ptp_802_1as_site_syncfollow_up(ptp_clock_t *ptpClock, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header,vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort)
{
    bool forwarded = false;
    u16 portIdx;
    vtss_timeinterval_t correctionValue;
    u8 *frame;
    vtss_ptp_tag_t tag;
    size_t buffer_size;
    size_t header_size;
    char str1[30];
    // 802.1as standard till d7.0 allows storing of cumulativeRateRatio till this node in to ParentDS. Instead of calculating again, we use it here.
    u32 cumulativeScaledRateOffset = htonl(ptpClock->parentDS.par_802_1as.cumulativeRateRatio);

    if (ptpClock->clock_init->cfg.twoStepFlag) {
        if (ptpClock->port_802_1as_sync_sync.port_mask != 0LL) {
            for (portIdx = 0; portIdx < ptpClock->clock_init->numberEtherPorts; portIdx++) {
                PtpPort_t *ptpPort = &ptpClock->ptpPort[portIdx];
                if (ptpPort != rxptpPort && ptpPort->portDS.status.portState == VTSS_APPL_PTP_MASTER) {
                    if (ptpClock->port_802_1as_sync_sync.syncResidenceTime[portIdx] &&
                            header->sequenceId == ptpClock->port_802_1as_sync_sync.syncForwardingHeader.sequenceId) {
                        correctionValue = ptpClock->port_802_1as_sync_sync.syncResidenceTime[portIdx] + ptpClock->port_802_1as_sync_sync.rx_delay_asy;                       
                        /* Keep track of below  parameters  in this case also to transmit repeated followup 
                           during sync-time out period
                        */
                        if (PACKET_SIZE < buf_handle->size - buf_handle->header_length) {
                        } else {
                        memcpy(ptpClock->port_802_1as_sync_sync.msgFbuf, buf_handle->frame + buf_handle->header_length, buf_handle->size - buf_handle->header_length);
                        ptpClock->port_802_1as_sync_sync.msgFbuf_size = buf_handle->size - buf_handle->header_length;
                        }
                        ptpClock->port_802_1as_sync_sync.port_mask &= ~(1LL<<portIdx);
                        T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"corr %s portIdx %u, port_mask " VPRI64d, 
                             vtss_tod_TimeInterval_To_String (&correctionValue, str1,0), portIdx,
                             ptpClock->port_802_1as_sync_sync.port_mask);
                        if (ptpClock->port_802_1as_sync_sync.port_mask == 0LL) {
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"Forwarding the followup message. No further ports remaining ");
                            ptpClock->port_802_1as_sync_sync.sync_followup_action = FOLLOW_UP_NO_ACTION;
                        }
                        vtss_appl_ptp_protocol_adr_t *ptp_dest;
                        ptp_dest = (ptpPort->port_config->dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT) ? &ptpClock->ptp_primary : &ptpClock->ptp_pdelay;
                        buffer_size = vtss_1588_prepare_general_packet(&frame, ptp_dest, buf_handle->size - buf_handle->header_length, &header_size, ptpClock->localClockId);
                        vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
                        if (buffer_size) {
                            if (buf_handle->header_length != header_size) {
                                T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"Size mismatch header_length %d, header size " VPRIz, buf_handle->header_length, header_size);
                            }
                            header->sourcePortIdentity = ptpPort->portDS.status.portIdentity;
                            header->correctionField += correctionValue;
                            header->minorVersionPTP = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
                            header->controlField = ptpPort->port_config->c_802_1as.as2020 ? 0x00 : PTP_FOLLOWUP_MESSAGE;
                            vtss_ptp_pack_header(frame + header_size, header);
                            // copy preciseOriginTimestamp and followUp TLV
                            memcpy(frame + header_size + HEADER_LENGTH, buf_handle->frame + header_size + HEADER_LENGTH, buf_handle->size - header_size - HEADER_LENGTH);
                            // Update cumulativeScaledRateOffset of this node
                            memcpy(frame + header_size + PTP_MESSAGE_FOLLOWUP_CUM_RATE_OFFSET, &cumulativeScaledRateOffset, sizeof(int32_t));

                            // Send to downstream devices about phase change and frequency change.
                            if (ptpClock->localGMUpdate) {
                                T_IG(VTSS_TRACE_GRP_PTP_BASE_TC, "sending local GM Update port mask 0x" VPRI64x " with sequenceId %d\n", ptpPort->port_mask, header->sequenceId);
                                vtss_tod_pack32(ptpClock->follow_up_info.lastGmPhaseChange.scaled_ns_high, frame + header_size + PTP_MESSAGE_FOLLOWUP_GM_PHASE_CHANGE);
                                vtss_tod_pack64(ptpClock->follow_up_info.lastGmPhaseChange.scaled_ns_low, frame + header_size + PTP_MESSAGE_FOLLOWUP_GM_PHASE_CHANGE + 4);
                                vtss_tod_pack32(ptpClock->follow_up_info.scaledLastGmFreqChange, frame + header_size + PTP_MESSAGE_FOLLOWUP_GM_FREQ_CHANGE);
                            }

                            if (vtss_1588_tx_general(ptpPort->port_mask,frame, buffer_size, &tag)) {
                                T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded Follow Up message to port: %d", ptpPort->portDS.status.portIdentity.portNumber);
                                ptpPort->port_statistics.txFollowUpCount++;
                            }
                            header->correctionField -= correctionValue;
                        }
                    } else if (ptpClock->port_802_1as_sync_sync.sync_followup_action == FOLLOW_UP_WAIT_TX &&
                               (ptpClock->port_802_1as_sync_sync.syncResidenceTime[portIdx]== 0) &&
                               header->sequenceId == ptpClock->port_802_1as_sync_sync.syncForwardingHeader.sequenceId) {
                        /* Unfortunately the followup message is received before the forwarded sync event has been sent */
                        /* save the followup message and wait until sync transmitted */
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"Follow up saved, waiting for sync tx timestamp");
                        if (PACKET_SIZE < buf_handle->size - buf_handle->header_length) {
                        } else {
                            memcpy(ptpClock->port_802_1as_sync_sync.msgFbuf, buf_handle->frame + buf_handle->header_length, buf_handle->size - buf_handle->header_length);
                            ptpClock->port_802_1as_sync_sync.sync_followup_action = FOLLOW_UP_WAIT_TX_READY;
                            ptpClock->port_802_1as_sync_sync.msgFbuf_size = buf_handle->size - buf_handle->header_length;
                        }
                    } else {
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"not waiting for followup: port: %d, action: %d, header.seq = %d, exp.seq = %d",
                            ptpPort->portDS.status.portIdentity.portNumber,
                            ptpClock->port_802_1as_sync_sync.sync_followup_action,
                            header->sequenceId,
                            ptpClock->port_802_1as_sync_sync.syncForwardingHeader.sequenceId);
                    }
                }
            }
        } else {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC, "No forwarding of sync and followup messages");
            ptpClock->port_802_1as_sync_sync.sync_followup_action = FOLLOW_UP_NO_ACTION;
        }
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"No forwarding of followup messages in one-step mode");
    }
    return forwarded;
}
#endif //(VTSS_SW_OPTION_P802_1_AS)
