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
#include "vtss_ptp_tc.h"
#include "vtss_ptp_sys_timer.h"
#include "vtss_ptp_pack_unpack.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_ptp_packet_callout.h"
#include "vtss_tod_api.h"

/*
 * Forward declarations
 */
//#define RESIDENCETIME_TEST
#if defined (RESIDENCETIME_TEST)
static u64 tc_hw_rx;
static u64 tc_sw_tx;  // after tx
static u64 tc_sw_bx; // before tx
static u64 tc_sw_rx;
static mesa_timestamp_t ts_rx;
#endif

/*
 * private functions
 */

static void delayReqListInit(DelayReqList *list)
{
    int i;

    for (i = 0; i < list->listSize; i++) {
        list->list[i].inUse = IN_USE_FREE;
    }
}

static i16 delayReqListEntryFind(const DelayReqList *list, const vtss_appl_ptp_port_identity *portId, u16 seq_id)
{
    int i;
    for (i = 0; i < list->listSize; i++) {
        if (list->list[i].inUse &&
            PortIdentitycmp(portId, &list->list[i].sourcePortIdentity) == 0 &&
            seq_id == list->list[i].sequenceId) {
            return i;
        }

    }
    return -1;
}

static i16 delayReqListEntryGetFree(DelayReqList *list)
{
    int i;
    for (i = 0; i < list->listSize; i++) {
        if (!list->list[i].inUse) {
            list->list[i].inUse = IN_USE_WAIT_RESP;
            return i;
        }
    }
    return -1;
}

static void delayReqListEntryFree(DelayReqListEntry *dEntry)
{
    dEntry->inUse = IN_USE_FREE;
}

#if 0
static void delayReqListEntryDump(DelayReqList *list)
{
    int i;
    char str1 [25];
    for (i = 0; i < list->listSize; i++) {
        if (list->list[i].inUse) {
            printf("Entry %d,InUse %d, age %d, PortNo %d, SourceId %s %d\n", i, list->list[i].inUse,
                   list->list[i].age,
                   list->list[i].originPtpPort->portDS.status.portIdentity.portNumber,
                   ClockIdentityToString(list->list[i].sourcePortIdentity.clockIdentity,str1),
                   list->list[i].sourcePortIdentity.portNumber);
        }
    }
}
#endif
static void delayReqListEntryAgeing(DelayReqList *list)
{
    int i;
    for (i = 0; i < list->listSize; i++) {
        if (list->list[i].inUse) {
            if (++list->list[i].age > DELAY_REQ_E2E_MAX_OUTSTANDING_TIME) {
                list->list[i].inUse = IN_USE_FREE;
            }
        }
    }
    //delayReqListEntryDump(list);
}
/*
 * SyncOutstanding list interface
 */
static void syncListInit(SyncOutstandingList *list)
{
    int i;

    for (i = 0; i < list->listSize; i++) {
        list->list[i].sync_followup_action = FOLLOW_UP_NO_ACTION;
    }
}

static i16 syncListEntryFind(const SyncOutstandingList *list, const MsgHeader *header)
{
    int i;
    for (i = 0; i < list->listSize; i++) {
        if ((list->list[i].sync_followup_action != FOLLOW_UP_NO_ACTION) &&
                PortIdentitycmp(&header->sourcePortIdentity, &list->list[i].syncForwardingHeader.sourcePortIdentity) == 0 &&
                header->sequenceId == list->list[i].syncForwardingHeader.sequenceId)
            return i;

    }
    return -1;
}

static i16 syncListEntryGetFree(SyncOutstandingList *list)
{
    int i;
    for (i = 0; i < list->listSize; i++) {
        if (FOLLOW_UP_NO_ACTION == list->list[i].sync_followup_action) {
            list->list[i].sync_followup_action = FOLLOW_UP_CREATE;
            list->list[i].age = 0;
            return i;
        }
    }
    return -1;
}

static void syncListEntryFree(SyncOutstandingListEntry *sEntry)
{
    sEntry->sync_followup_action = FOLLOW_UP_NO_ACTION;
}

#if 0
static void syncListEntryDump(SyncOutstandingList *list)
{
    int i;
    char str1 [25];
    for (i = 0; i < list->listSize; i++) {
        if (list->list[i].sync_followup_action != FOLLOW_UP_NO_ACTION) {
            printf("Entry %d, age %d, Action %d, SourceId %s %d\n", i,
                   list->list[i].age,
                   list->list[i].sync_followup_action,
                   ClockIdentityToString(list->list[i].syncForwardingHeader.sourcePortIdentity.clockIdentity,str1),
                   list->list[i].syncForwardingHeader.sourcePortIdentity.portNumber);
        }
    }
}
#endif

static void syncListEntryAgeing(SyncOutstandingList *list)
{
    int i;
    for (i = 0; i < list->listSize; i++) {
        if (list->list[i].sync_followup_action != FOLLOW_UP_NO_ACTION) {
            if (++list->list[i].age > SYNC_2STEP_MAX_OUTSTANDING_TIME) {
                list->list[i].sync_followup_action = FOLLOW_UP_NO_ACTION;
            }
        }
    }
    //syncListEntryDump(list);
}


/*
 * TC age timer
 * do ageing of the outstanding message list in a TC
 */
/*lint -esym(459, vtss_ptp_tc_age_timer) */
static void vtss_ptp_tc_age_timer(vtss_ptp_sys_timer_t *timer, void *t)
{
    ptp_tc_t *tc = (ptp_tc_t *)t;
    delayReqListEntryAgeing(&tc->outstanding_delay_req_list);
    syncListEntryAgeing(&tc->sync_outstanding_list);
    T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"age processing");
}

/*
 * create a PTP transparent clock instance
 */
void vtss_ptp_tc_create(ptp_tc_t *tc, u32 max_outstanding_records, ptp_clock_t *ptp_clock)
{
    u32 i;
    tc->clock = ptp_clock;
    T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"allocated " VPRIz " bytes for transparent clock outstanding data", sizeof(DelayReqListEntry)*max_outstanding_records);
    for (i=0; i < max_outstanding_records; i++) {
        tc->outstanding_delay_req_list.list[i].parent = ptp_clock;
        tc->sync_outstanding_list.list[i].parent = ptp_clock;
    }
    delayReqListInit(&tc->outstanding_delay_req_list);
    syncListInit(&tc->sync_outstanding_list);
    vtss_ptp_timer_init(&tc->age_timer, "tc_age", tc->clock->localClockId, vtss_ptp_tc_age_timer, tc);
}

/*
 * delete a PTP transparent clock instance
 * free allocated buffer(s)
 */
void vtss_ptp_tc_delete(ptp_tc_t *tc)
{
    vtss_ptp_timer_stop(&tc->age_timer);
    if (tc->outstanding_delay_req_list.list) vtss_ptp_free(tc->outstanding_delay_req_list.list);
    if (tc->sync_outstanding_list.list) vtss_ptp_free(tc->sync_outstanding_list.list);
}

/*
 * enable a PTP transparent clock ageing function
 * only uset in a 2-step TC
 */
void vtss_ptp_tc_enable(ptp_tc_t *tc)
{
    T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"start timer");
    vtss_ptp_timer_start(&tc->age_timer, PTP_LOG_TIMEOUT(0), true);
}

/*
 * disable a PTP transparent clock ageing function
 * only uset in a 2-step TC
 */
void vtss_ptp_tc_disable(ptp_tc_t *tc)
{
    T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"stop timer");
    vtss_ptp_timer_stop(&tc->age_timer);
}


static void createTransparentFollowup(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, SyncOutstandingListEntry *entry, mesa_timeinterval_t *syncResidenceTime)
{
    u8 *frame;
    size_t buffer_size;
    size_t header_size;
    vtss_ptp_tag_t tag;

    buffer_size = vtss_1588_prepare_general_packet_2(&frame, &entry->sender, &ptpClock->ptp_primary, FOLLOW_UP_PACKET_LENGTH, &header_size, ptpClock->localClockId);
    vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
    if (buffer_size) {
        if (entry->header_length != header_size) {
            T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"Size mismatch header_length %d, header size " VPRIz, entry->header_length, header_size);
        }
        memcpy(frame, entry->msgFbuf, header_size);
        vtss_ptp_pack_transparent_follow_up(frame+header_size, &entry->syncForwardingHeader,
                                   &entry->syncForwardingcontent.originTimestamp, syncResidenceTime);
        if (vtss_1588_tx_general(ptpPort->port_mask,frame, buffer_size, &tag))
            T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"create and send transparent followup message\n");
    }
}

/*
 * 2-step sync packet event transmitted.
 *
 */
/*lint -esym(459, vtss_ptp_tc_sync_event_transmitted) */
static void vtss_ptp_tc_sync_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time)
{
    char str1[50];
    mesa_timeinterval_t correctionValue;
    u8 *frame;
    vtss_ptp_tag_t tag;
    size_t buffer_size;
    size_t header_size;
    SyncOutstandingListEntry *sEntry = (SyncOutstandingListEntry *)context;
    ptp_clock_t *ptpClock = NULL;
    PtpPort_t *ptpPort = NULL;
    mesa_timestamp_t egress_time;

    T_D("vtss_ptp_tc_sync_event_transmitted (%d)", portnum);

    if (sEntry != NULL) {
        ptpClock = sEntry->parent;
        ptpPort = &ptpClock->ptpPort[portnum-1];
        // it is a multicast event
        if ((ptpPort->portDS.status.portState == VTSS_APPL_PTP_P2P_TRANSPARENT  || ptpPort->portDS.status.portState == VTSS_APPL_PTP_E2E_TRANSPARENT)
                && ptpClock->clock_init->cfg.twoStepFlag) {  /* sync forwarded in a transparent clock */
            /* find entry in outstanding list */
            /* find free entry in outstanding list for the port */
            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"sync eventTransmitted, txTime = %u, portnum %d, action %d seq-id %d",
                tx_time, portnum, sEntry->sync_followup_action, sEntry->syncForwardingHeader.sequenceId);
            vtss_local_clock_convert_to_time(tx_time, &egress_time, ptpClock->localClockId);
            vtss_tod_sub_TimeInterval(&sEntry->syncResidenceTime[portnum-1], &egress_time, &sEntry->sync_ingress_time);
            T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"residence time %s", vtss_tod_TimeInterval_To_String (&sEntry->syncResidenceTime[portnum-1], str1,0));
            T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"tx time: %s", TimeStampToString(&egress_time, str1));
            T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"ingress_time: %s", TimeStampToString(&sEntry->sync_ingress_time, str1));
#if defined (RESIDENCETIME_TEST)
                u32 tc_sw_tx_done;
                vtss_tod_gettimeofday(ptpClock->localClockId, &ts_rx,&tc_sw_tx_done);
                T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"TX_port %u, rx_latency %u, handling_latency %u, tx_latency %u, tx_done_latency %u, txfunction_latency %u",
                     portnum,
                     tc_sw_rx-tc_hw_rx,
                     tc_sw_bx-tc_sw_rx,
                     tx_time-tc_sw_bx,
                     tc_sw_tx_done-tx_time,
                     tc_sw_tx-tc_sw_bx);
#endif

            if (sEntry->sync_followup_action == FOLLOW_UP_CREATE) {
                correctionValue = sEntry->syncResidenceTime[portnum-1] + sEntry->rx_delay_asy;
                createTransparentFollowup(ptpClock, ptpPort, sEntry, &correctionValue);
                sEntry->port_mask &= ~(1LL<<(portnum-1));
            } else if (sEntry->sync_followup_action == FOLLOW_UP_WAIT_TX_READY) {
                /* followup has already been received, and a copy exists in the msgFbuf */
                correctionValue = sEntry->syncResidenceTime[portnum-1] + sEntry->rx_delay_asy;
                sEntry->port_mask &= ~(1LL<<(portnum-1));

                buffer_size = vtss_1588_prepare_general_packet_2(&frame, &sEntry->sender, &ptpClock->ptp_primary, FOLLOW_UP_PACKET_LENGTH, &header_size, ptpClock->localClockId);
                vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
                T_DG(VTSS_TRACE_GRP_PTP_BASE_TC, "WAIT_TX_READY (%d)", (uint32_t)buffer_size);
                if (buffer_size) {
                    memcpy(frame+ header_size, sEntry->msgFbuf, FOLLOW_UP_PACKET_LENGTH);
                    vtss_ptp_update_correctionfield(frame+ header_size, &correctionValue);
                    if (vtss_1588_tx_general(ptpPort->port_mask,frame, buffer_size, &tag))
                        T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded Follow Up message to port: %d", ptpPort->portDS.status.portIdentity.portNumber);
                }
            }
            if (sEntry->port_mask == 0) syncListEntryFree(sEntry);
        }
    } else {
        T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"unexpected context: %p", context);
    }
}


/*
 * Forward sync request from a master
 *
 * forwardsync messages
 * A sync message is forwarded to all active PTP ports.
 * The ingress timestamp has already been saved.
 * The header is saved for later use.
 * if 1step sync message & 2step clock then the sync is converted to 2 step.
 * if 2step is forwarded then the egress timestamp is saved for residence time calculation
 * if 1step is forwarded, the correction field is updated
 * Forwarding to an internal port requires special action, i.e. the rx timestamp is inserted in the reserved field,
 * and the message is forwarded as an general message.
 * Forwarding from an internal port requires special action, i.e. the rx timestamp is read from the reserved field,
 * and the message is forwarded as as messages from an external port.
 */
bool vtss_ptp_tc_sync(ptp_tc_t *tc, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header,vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort)
{
    bool forwarded = false;
    MsgSync sync;
    vtss_ptp_unpack_timestamp(buf_handle->frame + buf_handle->header_length, &sync.originTimestamp);
    /* forward sync message to other ports */
    T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"seq %d, rxtime " VPRI64d " correctionField  " VPRI64d, header->sequenceId, buf_handle->hw_time, header->correctionField>>16);
#if defined (RESIDENCETIME_TEST)
    vtss_tod_gettimeofday(tc->clock->localClockId, &ts_rx,&tc_sw_rx);
    tc_hw_rx = buf_handle->hw_time;
#endif
    u16 portIdx;
    SyncOutstandingListEntry *entry;
    i16 entryIdx;
    u8 flag_field[2];
    u64  port_mask = 0;
    u64  internal_port_mask = 0;
    u8 *frame = 0;
    vtss_ptp_tag_t tag;
    size_t buffer_size = 0;
    size_t header_size = 0;
    mesa_timestamp_t receive_time;
    PtpPort_t * ptpPort_internal = 0;
    bool one_step_origin = false;
    uint32_t ts_id;

    /* if received from an internal port, the rxtime should be found in the reserved field, otherwise ignore the packet */
    T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"internal port: %d", rxptpPort->port_config->portInternal);
    if (rxptpPort->port_config->portInternal) {
        if (getFlag(header->flagField[0], PTP_RESERVED_0_7_FLAG)) {
            clearFlag(header->flagField[0], PTP_RESERVED_0_7_FLAG);
            vtss_ptp_update_flags(buf_handle->frame + buf_handle->header_length, header->flagField); /* also update the original message */
            clearFlag(header->flagField[0], PTP_RESERVED_0_7_FLAG);
            vtss_local_clock_convert_to_hw_tc( vtss_tod_unpack32(buf_handle->frame + buf_handle->header_length + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET), &buf_handle->hw_time);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"reservedField: %d, buf_handle->hw_time %d", vtss_tod_unpack32(buf_handle->frame + buf_handle->header_length + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET), buf_handle->hw_time);
            vtss_tod_pack32(0, buf_handle->frame + buf_handle->header_length + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET);
        } else {
            return forwarded;
        }
    }
    T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"two-step mode: %d", tc->clock->clock_init->cfg.twoStepFlag);
    if (tc->clock->clock_init->cfg.twoStepFlag) {
        /* forward twostep */
        /* find free entry in outstanding list for the port */
        entryIdx = syncListEntryFind(&tc->sync_outstanding_list, header);
        if (entryIdx == -1) { /* the previous request has been responded. I.e. find a free entry */
            entryIdx = syncListEntryGetFree(&tc->sync_outstanding_list);
        } else { /* the entry was found. i.e. the previous request has not been responded */
            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"missed followup or previous sync packet has not been forwarded from master to slave on port: %d", rxptpPort->portDS.status.portIdentity.portNumber);
            entryIdx = -1;
        }
        T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"found entryIdx: %d", entryIdx);
        if (entryIdx != -1) {
            entry = &tc->sync_outstanding_list.list[entryIdx];
            T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"found entry ptr: %p", entry);
            memcpy(&entry->sender, sender, sizeof(entry->sender));/*save sender address */
            /* if the received Sync event is a onestep type, then a follow up is generated */
            memcpy(flag_field,header->flagField, sizeof(flag_field));
            if (!getFlag(header->flagField[0], PTP_TWO_STEP_FLAG)) {
                setFlag(flag_field[0], PTP_TWO_STEP_FLAG);
                one_step_origin = true;
                entry->sync_followup_action = FOLLOW_UP_CREATE;
                T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"Create FOLLOW_UP_CREATE");
                /* save encapsulation header for the Followup packet */
                if (buf_handle->header_length <= sizeof(entry->msgFbuf)) {
                    memcpy(entry->msgFbuf, buf_handle->frame, buf_handle->header_length);
                    entry->header_length = buf_handle->header_length;
                } else {
                    T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"header length mismatch %d", buf_handle->header_length);
                }
            } else {
                entry->sync_followup_action = FOLLOW_UP_WAIT_TX;
            }
            T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"saving data for follow_up");
            entry->syncForwardingHeader = *header; /* store the header information to be used when the followup message shall be handled. */
            entry->rx_delay_asy = rxptpPort->port_config->delayAsymmetry;  /* the delay asymmetry is later added to the correction field of the followup packet*/
            entry->rx_delay_asy += rxptpPort->portDS.status.peerMeanPathDelay;  /* the peer mean path delay is later added to the correction field of the followup packet (the value is 0 if E2E configuration) */
            entry->syncForwardingcontent = sync; /* store the sync information to be used when the followup message shall be created. */
            T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"saved data for follow_up entry-id %d seq-id %d", entryIdx, header->sequenceId);

            vtss_ptp_update_flags(buf_handle->frame + buf_handle->header_length, flag_field);
            vtss_local_clock_convert_to_time(buf_handle->hw_time, &entry->sync_ingress_time, tc->clock->localClockId);
            for (portIdx = 0; portIdx < tc->clock->defaultDS.status.numberPorts; portIdx++) {
                PtpPort_t *ptpPort = &tc->clock->ptpPort[portIdx];
                if (ptpPort != rxptpPort && ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED) {
                    /* forwarding to internal ports are handled separately */
                    if (ptpPort->port_config->portInternal) {
                        internal_port_mask |= (1LL<<portIdx);
                        ptpPort_internal = &tc->clock->ptpPort[portIdx];
                    } else {
                        port_mask |= (1LL<<portIdx);
                        entry->syncResidenceTime[portIdx] = 0;
                        vtss_1588_tag_get(get_tag_conf(tc->clock, ptpPort), tc->clock->localClockId, &buf_handle->tag);
                    }
                }
            }
            entry->port_mask = port_mask;
            if (internal_port_mask) {
                /* create buffer for internal forwarding before forwarding the original buffer */
                buffer_size = vtss_1588_prepare_general_packet(&frame, &tc->clock->ptp_primary, SYNC_PACKET_LENGTH, &header_size, tc->clock->localClockId);
                vtss_1588_tag_get(get_tag_conf(tc->clock, ptpPort_internal), tc->clock->localClockId, &tag);
                if (buffer_size) {
                    memcpy(frame, buf_handle->frame, buf_handle->size);
                    vtss_1588_tag_get(get_tag_conf(tc->clock, ptpPort_internal), tc->clock->localClockId, &buf_handle->tag);
                }
            }
            if (port_mask) {
                T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"create new buffer: current_size %u, prew size %u", (u32)buf_handle->size, buf_handle->header_length+SYNC_PACKET_LENGTH );
                vtss_1588_prepare_tx_buffer(buf_handle, SYNC_PACKET_LENGTH, true);
                buf_handle->msg_type = VTSS_PTP_MSG_TYPE_2_STEP;
                entry->sync_ts_context.cb_ts = vtss_ptp_tc_sync_event_transmitted;
                entry->sync_ts_context.context = entry;
                buf_handle->ts_done = &entry->sync_ts_context;

#if defined (RESIDENCETIME_TEST)
                vtss_tod_gettimeofday(tc->clock->localClockId, &ts_rx,&tc_sw_bx);
#endif
                if (!vtss_1588_tx_msg(port_mask, buf_handle, tc->clock->localClockId, false, &ts_id)) {
                    /* could not forward: therefore free the sync entry in the list */
                    syncListEntryFree(entry);
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"could not forward two-step Sync message to ports: " VPRI64x, port_mask);
                } else {
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded two-step Sync message to ports: " VPRI64x, port_mask);
                }
#if defined (RESIDENCETIME_TEST)
                vtss_tod_gettimeofday(tc->clock->localClockId, &ts_rx,&tc_sw_tx);
#endif
                forwarded = true;
            } else {
                syncListEntryFree(entry);
            }
        } else {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"Waiting for previous followup or Sync forwarding list empty");
        }
    } else {
        /* forward onestep (applies to both one-step and 2 step sync events) */
        for (portIdx = 0; portIdx < tc->clock->defaultDS.status.numberPorts; portIdx++) {
            PtpPort_t *ptpPort = &tc->clock->ptpPort[portIdx];
            if (ptpPort != rxptpPort && ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED) {
                if (ptpPort->port_config->portInternal) {
                    internal_port_mask |= (1LL<<portIdx);
                    ptpPort_internal = ptpPort;
                } else {
                    port_mask |= (1LL<<portIdx);
                }
            }
        }
        if (internal_port_mask) {
            /* create buffer for internal forwarding before forwarding the original buffer */
            buffer_size = vtss_1588_prepare_general_packet(&frame, &tc->clock->ptp_primary, SYNC_PACKET_LENGTH, &header_size, tc->clock->localClockId);
            vtss_1588_tag_get(get_tag_conf(tc->clock, ptpPort_internal), tc->clock->localClockId, &tag);
            if (buffer_size) {
                memcpy(frame, buf_handle->frame, buf_handle->size);
            }
        }
        if (port_mask) {
            vtss_1588_prepare_tx_buffer(buf_handle, SYNC_PACKET_LENGTH, true);
            buf_handle->msg_type = VTSS_PTP_MSG_TYPE_CORR_FIELD;
#if defined (RESIDENCETIME_TEST)
            vtss_tod_gettimeofday(tc->clock->localClockId, &ts_rx,&tc_sw_bx);
#endif
            if (vtss_1588_tx_msg(port_mask, buf_handle, tc->clock->localClockId, false, &ts_id))
                T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded one-step Sync message to ports: " VPRI64x, port_mask);
#if defined (RESIDENCETIME_TEST)
            vtss_tod_gettimeofday(tc->clock->localClockId, &ts_rx,&tc_sw_tx);
#endif
            forwarded = true;
        }
    }
    if (internal_port_mask) {
        /* internal forwarding handled here */
        if (buffer_size != 0 && frame != 0) {
            if (one_step_origin) { /* if the original sync packet was a onestep, then it is forwarded internally as onestep */
                clearFlag(frame[header_size + PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_TWO_STEP_FLAG);
            }
            /* update reserved Field and flag bit (octet 0, bit 4) */
            vtss_local_clock_convert_to_time(buf_handle->hw_time,&receive_time, tc->clock->localClockId);
            vtss_tod_pack32(receive_time.nanoseconds, frame + header_size + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET);
            setFlag(frame[header_size + PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_RESERVED_0_7_FLAG);
            if (!vtss_1588_tx_general(internal_port_mask,frame, buffer_size, &tag)) {
                T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"could not forward two-step Sync message to internal ports: " VPRI64x, internal_port_mask);
            } else {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded Sync message to internal ports: " VPRI64x, internal_port_mask);
            }
        }
        /* end of internal forwarding */
    }
    return forwarded;
}

/*
 * Forward follow_up request from a master
 * A follow up message is forwarded to all active PTP ports.
 *
 */
bool vtss_ptp_tc_follow_up(ptp_tc_t *tc, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header,vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort)
{
    bool forwarded = false;
    u16 portIdx;
    mesa_timeinterval_t correctionValue;
    SyncOutstandingListEntry *entry;
    i16 entryIdx;
    u64 port_mask = 0;
    u64  internal_port_mask = 0;
    u8 *frame;
    vtss_ptp_tag_t tag;
    size_t buffer_size;
    size_t header_size;
    PtpPort_t * internal_ptpPort = 0;
    uint32_t ts_id;

    /* if received on an internal port, only forward if flag is set */
    if (rxptpPort->port_config->portInternal)  {
        if (getFlag(buf_handle->frame[buf_handle->header_length + PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_RESERVED_0_7_FLAG)) {
            clearFlag(buf_handle->frame[buf_handle->header_length + PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_RESERVED_0_7_FLAG);
        } else {
            return forwarded;
        }
    }
    if (tc->clock->clock_init->cfg.twoStepFlag) {
        entryIdx = syncListEntryFind(&tc->sync_outstanding_list, header);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_TC, "TwoStepFlag enabled (%d) (%d)", entryIdx, tc->clock->defaultDS.status.numberPorts);
        if (entryIdx != -1) {
            entry = &tc->sync_outstanding_list.list[entryIdx];
            for (portIdx = 0; portIdx < tc->clock->defaultDS.status.numberPorts; portIdx++) {
                PtpPort_t *ptpPort = &tc->clock->ptpPort[portIdx];
                if (ptpPort != rxptpPort && ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED) {
                    if (!ptpPort->port_config->portInternal) {
                        if (entry->syncResidenceTime[portIdx] &&
                                header->sequenceId == entry->syncForwardingHeader.sequenceId) {
                            correctionValue = entry->syncResidenceTime[portIdx] + entry->rx_delay_asy;
                            entry->port_mask &= ~(1LL<<portIdx);

                            buffer_size = vtss_1588_prepare_general_packet_2(&frame, sender, &tc->clock->ptp_primary, FOLLOW_UP_PACKET_LENGTH, &header_size, tc->clock->localClockId);
                            vtss_1588_tag_get(get_tag_conf(tc->clock, ptpPort), tc->clock->localClockId, &tag);
                            if (buffer_size) {
                                if (buf_handle->header_length != header_size) {
                                    T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"Size mismatch header_length %d, header size " VPRIz, buf_handle->header_length, header_size);
                                }
                                memcpy(frame, buf_handle->frame, FOLLOW_UP_PACKET_LENGTH + header_size);
                                vtss_ptp_update_correctionfield(frame + header_size, &correctionValue);
                                if (vtss_1588_tx_general(ptpPort->port_mask,frame, buffer_size, &tag))
                                    T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded Follow Up message to port: %d (%d)", ptpPort->portDS.status.portIdentity.portNumber, ptpPort->port_mask);
                            }
                            if (entry->port_mask == 0) syncListEntryFree(entry);
                        } else if (entry->sync_followup_action == FOLLOW_UP_WAIT_TX &&
                                   (entry->syncResidenceTime[portIdx]== 0) &&
                                   header->sequenceId == entry->syncForwardingHeader.sequenceId) {
                            /* Unfortunately the followup message is received before the forwarded sync event has been sent */
                            /* save the followup message and wait until sync transmitted */
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"Follow up saved, waiting for sync tx timestamp seq-id %d", header->sequenceId);
                            memcpy(entry->msgFbuf, buf_handle->frame + buf_handle->header_length, FOLLOW_UP_PACKET_LENGTH);
                            entry->sync_followup_action = FOLLOW_UP_WAIT_TX_READY;
                        } else {
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"not waiting for followup: port: %d, action: %d, header.seq = %d, exp.seq = %d",
                                ptpPort->portDS.status.portIdentity.portNumber,
                                entry->sync_followup_action,
                                header->sequenceId,
                                entry->syncForwardingHeader.sequenceId);

                        }
                    }
                }
            }
        }
        /* forwarding to internal ports is done without saving a syncListEntry */
        for (portIdx = 0; portIdx < tc->clock->defaultDS.status.numberPorts; portIdx++) {
            PtpPort_t *ptpPort = &tc->clock->ptpPort[portIdx];
            if (ptpPort != rxptpPort && ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED && ptpPort->port_config->portInternal) {
                internal_port_mask |= (1LL<<portIdx);
            }
        }
        if (internal_port_mask) {
            setFlag(buf_handle->frame[buf_handle->header_length + PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_RESERVED_0_7_FLAG);
            vtss_1588_prepare_tx_buffer(buf_handle, FOLLOW_UP_PACKET_LENGTH, true);
            buf_handle->msg_type = VTSS_PTP_MSG_TYPE_GENERAL;
            if (vtss_1588_tx_msg(internal_port_mask, buf_handle, tc->clock->localClockId, false, &ts_id)) {
                T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded Follow Up message to internal ports: " VPRI64x, internal_port_mask);
            } else {
                T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded Follow Up message to internal ports: " VPRI64x " failed", internal_port_mask);
            }
            forwarded = true;
        }
    } else {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"TwoStepFlag not enabled (%d)", tc->clock->defaultDS.status.numberPorts);
        for (portIdx = 0; portIdx < tc->clock->defaultDS.status.numberPorts; portIdx++) {
            PtpPort_t *ptpPort = &tc->clock->ptpPort[portIdx];
            if (ptpPort != rxptpPort && ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED) {
                if (ptpPort->port_config->portInternal) {
                    internal_port_mask |= (1LL<<portIdx);
                    internal_ptpPort = ptpPort;
                } else {
                    port_mask |= 1LL<<portIdx;
                }
            }
        }
        /* one step: forwarding unchanged */
        if (internal_port_mask) {
            buffer_size = vtss_1588_prepare_general_packet(&frame, &tc->clock->ptp_primary, FOLLOW_UP_PACKET_LENGTH, &header_size, tc->clock->localClockId);
            vtss_1588_tag_get(get_tag_conf(tc->clock, internal_ptpPort), tc->clock->localClockId, &tag);
            if (buffer_size) {
                memcpy(frame, buf_handle->frame, buf_handle->size);
                setFlag(frame[header_size + PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_RESERVED_0_7_FLAG);
                if (vtss_1588_tx_general(internal_port_mask,frame, buffer_size, &tag))
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded Follow Up message to internal ports: " VPRI64x, internal_port_mask);
            }
        }
        if (port_mask) {
            vtss_1588_prepare_tx_buffer(buf_handle, FOLLOW_UP_PACKET_LENGTH, true);
            buf_handle->msg_type = VTSS_PTP_MSG_TYPE_GENERAL;
            if (vtss_1588_tx_msg(port_mask, buf_handle, tc->clock->localClockId, false, &ts_id))
                T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded Follow Up message to external ports: " VPRI64x, port_mask);
            forwarded = true;
        }
    }
    return forwarded;
}


/*
 * 2-step delayReq packet event transmitted. Save tx time
 *
 */
/*lint -esym(459, vtss_ptp_tc_delay_req_event_transmitted) */
static void vtss_ptp_tc_delay_req_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time)
{
    DelayReqListEntry *dEntry = (DelayReqListEntry *)context;
    ptp_clock_t *ptpClock = dEntry->parent;
    PtpPort_t *ptpPort = &ptpClock->ptpPort[portnum-1];
    mesa_timestamp_t txTime;
    mesa_timeinterval_t correctionValue;
    u8 *frame;
    vtss_ptp_tag_t tag;
    size_t buffer_size;
    size_t header_size;
    u64 forw_mask;

    if (ptpPort->portDS.status.portState == VTSS_APPL_PTP_E2E_TRANSPARENT) {
        vtss_local_clock_convert_to_time( tx_time, &txTime, ptpClock->localClockId);

        dEntry->delayReqTxTime[portnum-1] = txTime;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"Txtime: %d,%09d, Rxtime: %d,%09d", txTime.seconds, txTime.nanoseconds, dEntry->delayReqRxTime.seconds, dEntry->delayReqRxTime.nanoseconds);
        if (dEntry->inUse == IN_USE_WAIT_TX && dEntry->masterPort == portnum) {
            vtss_tod_sub_TimeInterval(&correctionValue,&txTime,&dEntry->delayReqRxTime);
            correctionValue -= ptpPort->port_config->delayAsymmetry; /* in 2 step the asymmetry for the DelayReq egress port is added to DelayResp */
            vtss_ptp_update_correctionfield(dEntry->msgFbuf, &correctionValue);
            buffer_size = vtss_1588_prepare_general_packet_2(&frame, &dEntry->rsp_sender, &ptpClock->ptp_primary, DELAY_RESP_PACKET_LENGTH, &header_size, ptpClock->localClockId);
            vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
            if (buffer_size) {
                memcpy(frame + header_size, dEntry->msgFbuf, DELAY_RESP_PACKET_LENGTH);
                forw_mask = 1LL<<(dEntry->originPtpPort->portDS.status.portIdentity.portNumber-1);
                if (vtss_1588_tx_general(forw_mask,frame, buffer_size, &tag))
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"sent delay response message\n");
            }
            delayReqListEntryFree(dEntry);
        } else {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"inUse %d, masterPort %d", dEntry->inUse, dEntry->masterPort);
        }
    }
}



/*
 * Forward delayReq request from a tc
 * An entry in the outstandingDelayReqList is found.
 * The sourcePortIdentity, sequenceId and ingress timestamp are saved in this entry.
 * The delayReq message is forwarded to the other ports.
 * if 2step clock:
 *   When the event is sent, the egress timestamp is saved for residence time calculation
 * if 1step clock:
 *   the correction field is updated with the residence time
 * Forwarding to an internal port requires special action, i.e. the rx timestamp is inserted in the reserved field,
 * and the message is forwarded as an general message.
 * Forwarding from an internal port requires special action, i.e. the rx timestamp is read from the reserved field,
 * and the message is forwarded as as messages from an external port.
 */
bool vtss_ptp_tc_delay_req(ptp_tc_t *tc, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort)
{
    DelayReqListEntry *entry;
    i16 entryIdx;
    int portIdx;
    u64 port_mask = 0;
    u64  internal_port_mask = 0;
    bool forwarded = false;
    PtpPort_t * ptpPort_internal = 0;
    u8 *frame;
    vtss_ptp_tag_t tag;
    size_t buffer_size;
    size_t header_size;
    mesa_timestamp_t receive_time;
    uint32_t ts_id;

    T_NG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarding DelayReq message");
    /* if received from an internal port, the rxtime should be found in the reserved field, otherwise ignore the packet */
    if (rxptpPort->port_config->portInternal) {
        if (getFlag(header->flagField[0], PTP_RESERVED_0_7_FLAG)) {
            clearFlag(buf_handle->frame[buf_handle->header_length + PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_RESERVED_0_7_FLAG);
            vtss_local_clock_convert_to_hw_tc(vtss_tod_unpack32(buf_handle->frame + buf_handle->header_length + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET), &buf_handle->hw_time);
            T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"reservedField: %d, buf_handle->hw_time " VPRI64d, vtss_tod_unpack32(buf_handle->frame + buf_handle->header_length + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET), buf_handle->hw_time);
            vtss_tod_pack32(0, buf_handle->frame + buf_handle->header_length + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET);
        } else {
            return forwarded;
        }
    }

    entryIdx = delayReqListEntryFind(&tc->outstanding_delay_req_list, &header->sourcePortIdentity, header->sequenceId);
    if (entryIdx == -1) { /* the previous request has been responded. I.e. find a free entry */
        entryIdx = delayReqListEntryGetFree(&tc->outstanding_delay_req_list);
        if (entryIdx == -1) { /* no free entry found */
            T_EG(VTSS_TRACE_GRP_PTP_BASE_TC,"No free entry for DelayReq forwarding found");
        }
    } else { /* the entry was found. i.e. the previous request has not been responded */
        T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"missed response from master to slave on port: %d", rxptpPort->portDS.status.portIdentity.portNumber);
        entryIdx = -1;
    }
    if (entryIdx != -1) {
        entry = &tc->outstanding_delay_req_list.list[entryIdx];
        entry->sourcePortIdentity = header->sourcePortIdentity;
        entry->sequenceId = header->sequenceId;
        entry->originPtpPort = rxptpPort;
        entry->age = 0;
        entry->masterPort = 0;
        memcpy(&entry->sender, sender, sizeof(entry->sender));/*save sender address */
        T_IG(VTSS_TRACE_GRP_PTP_BASE_TC, "seq-id %d", header->sequenceId);
        for (portIdx = 0; portIdx < tc->clock->defaultDS.status.numberPorts; portIdx++) {
            PtpPort_t *ptpPort = &tc->clock->ptpPort[portIdx];
            if (ptpPort != rxptpPort && ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED) {
                /* forwarding to internal ports are handled separately */
                if (ptpPort->port_config->portInternal) {
                    internal_port_mask |= (1LL<<portIdx);
                    ptpPort_internal = ptpPort;
                } else {
                    port_mask |= (1LL<<portIdx);
                    entry->delayReqTxTime[portIdx].seconds = 0;
                }
            }
        }
        if (port_mask) {
            vtss_1588_tag_get(get_tag_conf(tc->clock, rxptpPort), tc->clock->localClockId, &buf_handle->tag);
            vtss_1588_prepare_tx_buffer(buf_handle, DELAY_REQ_PACKET_LENGTH, true);
            if (tc->clock->clock_init->cfg.twoStepFlag) {
                vtss_local_clock_convert_to_time(buf_handle->hw_time,&entry->delayReqRxTime,tc->clock->localClockId);
                buf_handle->msg_type = VTSS_PTP_MSG_TYPE_2_STEP;
                entry->del_req_ts_context.cb_ts = vtss_ptp_tc_delay_req_event_transmitted;
                entry->del_req_ts_context.context = entry;
                buf_handle->ts_done = &entry->del_req_ts_context;

                if (!vtss_1588_tx_msg(port_mask, buf_handle, tc->clock->localClockId, false, &ts_id)) {
                    /* could not forward: therefore free the entry in the list */
                    delayReqListEntryFree(entry);
                    T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"could not forward two-step DelayReq message to ports: " VPRI64x, port_mask);
                } else
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded two-step DelayReq message to ports: " VPRI64x, port_mask);
            } else {
                buf_handle->msg_type = VTSS_PTP_MSG_TYPE_CORR_FIELD;
                if (!vtss_1588_tx_msg(port_mask, buf_handle, tc->clock->localClockId, false, &ts_id)) {
                    /* could not forward: therefore free the entry in the list */
                    delayReqListEntryFree(entry);
                    T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"could not forward two-step DelayReq message to ports: " VPRI64x, port_mask);
                } else
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded one-step DelayReq message to port: " VPRI64x, port_mask);
            }
            forwarded = true;
        }
        if (internal_port_mask) {
            /* internal port forwarding handled here */
            buffer_size = vtss_1588_prepare_general_packet(&frame, &tc->clock->ptp_primary, SYNC_PACKET_LENGTH, &header_size, tc->clock->localClockId);
            vtss_1588_tag_get(get_tag_conf(tc->clock, ptpPort_internal), tc->clock->localClockId, &tag);
            if (buffer_size) {
                memcpy(frame, buf_handle->frame, buf_handle->size);
                /* update reserved Field and flag bit (octet 0, bit 4) */
                vtss_local_clock_convert_to_time(buf_handle->hw_time,&receive_time,tc->clock->localClockId);
                vtss_tod_pack32(receive_time.nanoseconds, frame + header_size + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET);
                setFlag(frame[header_size + PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_RESERVED_0_7_FLAG);
                if (!vtss_1588_tx_general(internal_port_mask,frame, buffer_size, &tag)) {
                    T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"could not forward DelayReq message to internal ports: " VPRI64x, internal_port_mask);
                } else {
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded DelayReq message to internal ports: " VPRI64x, internal_port_mask);
                }
            }
            /* end of internal forwarding */
        }
        if (port_mask == 0 && internal_port_mask == 0) {
            delayReqListEntryFree(entry);
        }
    } else {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"Waiting for response or no free entry for DelayReq forwarding found");
    }
    return forwarded;
}

/*
 * Forward delayResp request from a master
 *
 */
bool vtss_ptp_tc_delay_resp(ptp_tc_t *tc, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort)
{
    bool forwarded = false;
    MsgDelayResp resp;
    i16 entryIdx;
    DelayReqListEntry *entry;
    u64 forw_mask;
    uint32_t ts_id;

    vtss_ptp_unpack_delay_resp(buf_handle->frame + buf_handle->header_length, &resp);
    entryIdx = delayReqListEntryFind(&tc->outstanding_delay_req_list, &resp.requestingPortIdentity, header->sequenceId);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_TC, "seq-id %d entry-id %d", header->sequenceId, entryIdx);
    if (entryIdx == -1) { /* no request is outstanding for this response */
        T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"unexpected delay response");
        return false;
    }
    if (rxptpPort->port_config->portInternal)  {
        if (getFlag(buf_handle->frame[buf_handle->header_length + PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_RESERVED_0_7_FLAG)) {
            clearFlag(buf_handle->frame[buf_handle->header_length + PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_RESERVED_0_7_FLAG);
        } else {
            return forwarded;
        }
    }
    entry = &tc->outstanding_delay_req_list.list[entryIdx];
    mesa_timeinterval_t correctionValue;
    forw_mask = 1LL<<(entry->originPtpPort->portDS.status.portIdentity.portNumber-1);
    vtss_1588_tag_get(get_tag_conf(tc->clock, entry->originPtpPort), tc->clock->localClockId, &buf_handle->tag);
    if (entry->originPtpPort->port_config->portInternal) {
        setFlag(buf_handle->frame[buf_handle->header_length + PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_RESERVED_0_7_FLAG);
    }
    if (rxptpPort->port_config->portInternal)  {
        forwarded = true;
    } else if ((entry->delayReqTxTime[rxptpPort->portDS.status.portIdentity.portNumber-1].seconds != 0) && (tc->clock->clock_init->cfg.twoStepFlag)) {
        /* only modify correction field if two-step mode */
        vtss_tod_sub_TimeInterval(&correctionValue,&entry->delayReqTxTime[rxptpPort->portDS.status.portIdentity.portNumber-1],&entry->delayReqRxTime);
        correctionValue -= rxptpPort->port_config->delayAsymmetry; /* in 2 step the asymmetry for the DelayReq egress port is added to DelayResp */
        vtss_ptp_update_correctionfield(buf_handle->frame + buf_handle->header_length, &correctionValue);
        forwarded = true;
    } else if ((entry->delayReqTxTime[rxptpPort->portDS.status.portIdentity.portNumber-1].seconds == 0) && (tc->clock->clock_init->cfg.twoStepFlag)) {
        /* Delay response came before the timestamp from DelayReq transmitted */
        T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"Received delay response message before DelayReq transmitted");
        memcpy(entry->msgFbuf, buf_handle->frame + buf_handle->header_length, DELAY_RESP_PACKET_LENGTH);
        entry->rsp_sender = *sender;
        entry->inUse = IN_USE_WAIT_TX;
        entry->masterPort = rxptpPort->portDS.status.portIdentity.portNumber;
    }
    if (!tc->clock->clock_init->cfg.twoStepFlag) {
        forwarded = true;
    }
    if (forwarded) {
        delayReqListEntryFree(entry);
        vtss_1588_prepare_tx_buffer(buf_handle, DELAY_RESP_PACKET_LENGTH, true);
        buf_handle->msg_type = VTSS_PTP_MSG_TYPE_GENERAL;
        if (vtss_1588_tx_msg(forw_mask, buf_handle, tc->clock->localClockId, false, &ts_id))
            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded delay response message\n");
    }
    return forwarded;
}

/*
 * Forward general message
 *
 */
bool vtss_ptp_tc_general(ptp_tc_t *tc, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender, PtpPort_t *rxptpPort)
{
    /* The general packet forwarding is done in the switch */
    return false;
}
