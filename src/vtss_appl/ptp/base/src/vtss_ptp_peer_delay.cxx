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
#include "vtss_ptp_types.h"
#include "vtss_ptp_os.h"
#include "vtss_ptp_peer_delay.h"
#include "vtss_ptp_sys_timer.h"
#include "vtss_ptp_pack_unpack.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_ptp_packet_callout.h"
#include "vtss_tod_api.h"
#include "vtss_ptp_internal_types.h"
#if defined (VTSS_SW_OPTION_P802_1_AS)
#include "vtss_ptp_802_1as_bmca.h"
#endif //(VTSS_SW_OPTION_P802_1_AS)
#include "ptp_api.h"

/* state defined as random numbers for consistency check */
#define PEER_DELAY_PTP_OBJCT_ACTIVE         0x47835672
#define PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE   0x5a3be982
#define PEER_DELAY_PTP_OBJCT_PASSIVE        0x58370465

#if defined (VTSS_SW_OPTION_P802_1_AS)
#define MAX_PPM_OFFSET 0.002 // allow till 2000ppm offset as valid
#define RATE_TO_U32_CONVERT_FACTOR (1LL<<41)
#define RATE_LP_FILTER_CONST 4
// minimum delta T in the rate calculation function
#define VTSS_TIMEINTERVAL_1US (1000LL<<16)
#define AVNU_MULT_RESP_CEASE_TIME 40400 // 5 minutes
#endif //defined (VTSS_SW_OPTION_P802_1_AS)

// Define Peer delay error status
#define PEER_DELAY_MORE_THAN_ONE_T1_ERROR 0x1
#define PEER_DELAY_MORE_THAN_ONE_RESPONDER_ERROR 0x2
#define PEER_DELAY_MORE_THAN_ONE_FOLLOWUP_ERROR 0x4
#define PEER_DELAY_MISSED_RESPONSE_ERROR 0x8
#define PEER_DELAY_DELTA_T3_ERROR 0x10
#define PEER_DELAY_DELTA_T4_ERROR 0x20
#define PEER_DELAY_INVALID_RATE_RATIO_ERROR 0x40
#define PEER_DELAY_ONESTEP_RESPONDER_ERROR 0x80
#define PEER_DELAY_MAX_ERROR 0x100
// 802.1as-2020 AVNU case 15.3d - 10ms is no longer a requirement in 2020
#define PEER_DELAY_RESPONSE_TIMEOUT_2020 500 // 500 milli seconds
#define PEER_DELAY_RESPONSE_TIMEOUT 10 // 10 milli seconds

#define PEER_DELAY_DELAY_VARIATION 1000 // 1000 nano seconds.

extern u64 tick_duration_us;
/*
 * Forward declarations
 */
static void vtss_ptp_peer_delay_req_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time);
static void vtss_ptp_peer_delay_req_timer(vtss_ptp_sys_timer_t *timer, void *m);
static void vtss_ptp_peer_delay_resp_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time);
#if defined (VTSS_SW_OPTION_P802_1_AS)
static void vtss_ptp_peer_delay_resp_timer(vtss_ptp_sys_timer_t *timer, void *m);
#endif //VTSS_SW_OPTION_P802_1_AS

/*
 * private functions
 */
static void show_measure_status(u16 measure_status)
{
    const char * peer_delay_error_txt[] = {
        "more_than_one_t1",
        "more_than_one_responder",
        "more_than_one_followup or missing pDelayResp",
        "missed_responses",
        "delta_t3",
        "delta_t4",
        "invalid_rate_ratio",
        "onestep_responder",
    };
    u16 mask = 1;
    u16 i = 0;
    while (mask <= PEER_DELAY_MAX_ERROR && i < (sizeof(peer_delay_error_txt) / sizeof(char *))) {
        if (measure_status & mask) T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Peer delay failed: %s", peer_delay_error_txt[i]);
        mask = mask<<1;
        i++;
    }
}

static const char *state2name(u32 state) {
    switch (state) {
        case PEER_DELAY_PTP_OBJCT_ACTIVE:         return "ACTIVE";
        case PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE:   return "WAIT_TX_DONE";
        case PEER_DELAY_PTP_OBJCT_PASSIVE:        return "PASSIVE";
        default:                                  return "invalid state";
    }
}

static void init_pdelay_measurements(PeerDelayData *peer_delay)
{
    peer_delay->last_pdelay_req_event_sequence_number = 0;
    peer_delay->pdelay_event_sequence_errors = 0;
#if defined (VTSS_SW_OPTION_P802_1_AS)
    vtss_appl_ptp_802_1as_pdelay_conf_port_ds_t *conf;
    vtss_appl_ptp_802_1as_pdelay_status_ds_t *status;
    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
        conf = &peer_delay->port_cmlds->conf->peer_d;
        status = &peer_delay->port_cmlds->status.peer_d;
    } else {
        conf = &peer_delay->ptp_port->port_config->c_802_1as.peer_d;
        status = &peer_delay->ptp_port->portDS.status.s_802_1as.peer_d;
    }
    peer_delay->pdelay_missed_response_cnt = conf->allowedLostResponses + 1;  // initialy we assume lost response until the first response is received
    status->currentComputeMeanLinkDelay = conf->initialComputeMeanLinkDelay;
    status->currentComputeNeighborRateRatio = conf->initialComputeNeighborRateRatio;
#endif
    /* these fields are filled in when the message is transmitted and the response is received */
    peer_delay->t1.seconds = 0;
    peer_delay->t1.nanoseconds = 0;
    peer_delay->t4.seconds = 0;
    peer_delay->t4.nanoseconds = 0;
    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
        peer_delay->port_cmlds->delay_filter.reset();
    } else {
        peer_delay->clock->ssm.servo->delay_filter_reset(peer_delay->ptp_port->portDS.status.portIdentity.portNumber);  /* clears one-way P2P delay filter */
    }
    peer_delay->t2_t3_valid = 0;
    peer_delay->t4_valid = 0;
#if defined (VTSS_SW_OPTION_P802_1_AS)
    peer_delay->detectedFaults = 0;
#endif
}

static bool port_uses_two_step(PeerDelayData *peer_delay)
{
    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
        /* Currently only two step is supported for cmlds */
        return true;
    } else if ((peer_delay->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) || (peer_delay->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_AED_GM) || (peer_delay->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY)) {
        return (peer_delay->clock->clock_init->cfg.twoStepFlag && !(peer_delay->ptp_port->port_config->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE)) ||
                             (peer_delay->ptp_port->port_config->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE) ||
                             (peer_delay->clock->clock_init->cfg.clock_domain != 0);
    }
    else {
        return peer_delay->clock->clock_init->cfg.twoStepFlag || peer_delay->clock->clock_init->cfg.clock_domain != 0;
    }
}

// Whether peer delay needs to be reset or not is verified here.
static bool ptp_peer_delay_filter_needed(PeerDelayData *peer_delay, uint16_t port, mesa_timeinterval_t *link_delay)
{
    bool ret = true;
    int64_t del_var;
    char str[50], str1[50];

    T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY, "prev delay %s current delay %s ", vtss_tod_TimeInterval_To_String(&peer_delay->prev_delay, str,','),
                                             vtss_tod_TimeInterval_To_String(link_delay, str1,','));

    // With hardware clocks, initial frequency or 'adj' is zero. After it
    // becomes 'slave' state, 'adj' starts with new value and there will be
    // large variations in peer mean path delay on both sides. To discard history,
    // delay filter must be reset.
    del_var = labs((peer_delay->prev_delay>>16) - (*link_delay>>16));
    if (del_var > PEER_DELAY_DELAY_VARIATION) {
        peer_delay->clock->ssm.servo->delay_filter_reset(port);
        ret = false;
    }
    peer_delay->prev_delay = *link_delay;

    return ret;
}

static void ptp_peer_delay_calc(PeerDelayData *peer_delay)
{
    char str [40];
    mesa_timeinterval_t t4minust1;
    vtss_ptp_offset_filter_param_t delay;
    u32 cur_set_time_count;
    T_RG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"ptp_peer_delay_calc");

    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
        cur_set_time_count = vtss_domain_clock_set_time_count_get(CMLDS_DOMAIN);
    } else {
        cur_set_time_count = vtss_local_clock_set_time_count_get(peer_delay->clock->localClockId);
    }

    if (cur_set_time_count != peer_delay->requester_set_time_count) {
        peer_delay->requester_set_time_count = cur_set_time_count;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"set_time_count changed to %d", cur_set_time_count);
        return; // delay calculation is ignored if time has been changed since last pdelay_req, because this wil result in a wrong t4minust1
    }
    /* update 'one_way_delay' */
    if (peer_delay->t1_valid && peer_delay->t2_t3_valid && peer_delay->t4_valid && !peer_delay->peerDelayNotMeasured) {
        uint16_t portNumber;
        mesa_timeinterval_t *peerMeanPathDelay;
        vtss_appl_ptp_802_1as_pdelay_status_ds_t *status;
        if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
            status = &peer_delay->port_cmlds->status.peer_d;
            portNumber = peer_delay->port_cmlds->status.portIdentity.portNumber;
            peerMeanPathDelay = &peer_delay->port_cmlds->status.meanLinkDelay;
        } else {
            status = &peer_delay->ptp_port->portDS.status.s_802_1as.peer_d;
            portNumber = peer_delay->ptp_port->portDS.status.portIdentity.portNumber;
            peerMeanPathDelay = &peer_delay->ptp_port->portDS.status.peerMeanPathDelay;
        }
        vtss_tod_sub_TimeInterval(&t4minust1, &peer_delay->t4, &peer_delay->t1);
#if defined (VTSS_SW_OPTION_P802_1_AS)
        if (peer_delay->neighbor_rate_ratio_valid) {
            //802.1as - 2020
            //10.2.5.8 meanLinkDelay: The measured mean propagation delay (see 8.3) on the link attached to this port,
            //relative to the LocalClock entity of the time-aware system at the other end of the link (i.e., expressed in the
            //time base of the time-aware system at the other end of the link).
            T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Port %d, t4minust1 %s", portNumber,
                 vtss_tod_TimeInterval_To_String(&t4minust1, str,','));
            t4minust1 = t4minust1 * peer_delay->fratio;
            T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Port %d, delta peer delay %s", portNumber,
                 vtss_tod_TimeInterval_To_String(&t4minust1, str,','));
        }
#endif // defined (VTSS_SW_OPTION_P802_1_AS)
        // if responder is 1 step, t3-t2 is included in the correction field in the PdelayResp
        // if responder is 2 step, t3-t2 is added to the correction field in the PdelayRespFollpw_Up
        delay.offsetFromMaster = (t4minust1 - peer_delay->correctionField)/2;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Port %d, peer delay before filtering %s fratio %E", portNumber,
             vtss_tod_TimeInterval_To_String(&delay.offsetFromMaster, str,','), peer_delay->fratio);

        //Convert to current device timescale so that we do not need to do this again before any 802.1as calculations.
        if (peer_delay->neighbor_rate_ratio_valid) {
            delay.offsetFromMaster /= peer_delay->fratio;
        }
        // Setup data for this latency calculation
        delay.rcvTime = {0, 0};  // Not used when doing peer delay calculation
        if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
            peer_delay->port_cmlds->delay_filter.filter(&delay.offsetFromMaster);
        } else {
            //TODO: logMsgInterval must be passed instead of '0' to 'delay_filter'.
            if (ptp_peer_delay_filter_needed(peer_delay, portNumber, &delay.offsetFromMaster)) {
                peer_delay->clock->ssm.servo->delay_filter(&delay, 0, portNumber);
            }
        }
        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"peer delay after filtering %s compMeandelay=%d", vtss_tod_TimeInterval_To_String(&delay.offsetFromMaster, str,','), status->currentComputeMeanLinkDelay);

        if (status->currentComputeMeanLinkDelay) {
            /* We computed delay till here to avoid incrementing missed response count */
            *peerMeanPathDelay = delay.offsetFromMaster;
            if (!port_uses_two_step(peer_delay)) {
                vtss_1588_p2p_delay_set(portNumber, *peerMeanPathDelay);
            }
        }
    }
    if (!peer_delay->t1_valid) {
        T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"t1(%d) not measured yet", peer_delay->t1.seconds);
    }
}

void vtss_ptp_peer_delay_state_init(PeerDelayData *peer_delay)
{
    peer_delay->requester_state = PEER_DELAY_PTP_OBJCT_PASSIVE;
    peer_delay->responder_state = PEER_DELAY_PTP_OBJCT_PASSIVE;
    peer_delay->pdel_req_buf.handle = NULL;
    peer_delay->follow_buf.handle = NULL;
}

/* Update Current compute Mean link delay and Neighbor rate Ratio */
void vtss_ptp_set_comp_ratio_pdelay(PtpPort_t *ptpPort, u8 flags, bool signalling, ptp_cmlds_port_ds_t *port_cmlds, bool cmlds)
{
    vtss_appl_ptp_802_1as_pdelay_conf_port_ds_t *conf;
    vtss_appl_ptp_802_1as_pdelay_status_ds_t *status;
    uint32_t delayConst, nrrConst;

    if (!cmlds && ptpPort->port_config->c_802_1as.as2020 == FALSE) {
        delayConst = PTP_PDELAY_COMP_MEAN_DELAY << 1;
        nrrConst   = PTP_PDELAY_COMP_NBR_RRATIO << 1;
    } else {
        delayConst = PTP_PDELAY_COMP_MEAN_DELAY;
        nrrConst   = PTP_PDELAY_COMP_NBR_RRATIO;
    }

    if (cmlds) {
        conf = &port_cmlds->conf->peer_d;
        status = &port_cmlds->status.peer_d;
    } else {
        conf = &ptpPort->port_config->c_802_1as.peer_d;
        status = &ptpPort->portDS.status.s_802_1as.peer_d;
    }
    if (conf->useMgtSettableComputeMeanLinkDelay) {
        status->currentComputeMeanLinkDelay = conf->mgtSettableComputeMeanLinkDelay;
    } else if (signalling) {
        status->currentComputeMeanLinkDelay = (flags & delayConst) ? true : false;
    } else {
        status->currentComputeMeanLinkDelay = conf->initialComputeMeanLinkDelay;
    }
    if (conf->useMgtSettableComputeNeighborRateRatio) {
        status->currentComputeNeighborRateRatio = conf->mgtSettableComputeNeighborRateRatio;
    } else if (signalling) {
        status->currentComputeNeighborRateRatio = (flags & nrrConst) ? true : false;
    } else {
        status->currentComputeNeighborRateRatio = conf->initialComputeNeighborRateRatio;
    }
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY, "received TLV flags 0x%x status compute delay %d nr-ratio %d", flags, status->currentComputeMeanLinkDelay, status->currentComputeNeighborRateRatio);
}

/*
 * create a PTP Peer Delay instance
 * Allocate packet buffer for PDelay_Req and if TwoStep: PDelay_Resp_FollowUp
 * Initialize encapsulation protocol
 * Initialize PTP header
 * Initialize PTP PDelayReq data.
 * start PDelay_Req timer
 */
void vtss_ptp_peer_delay_create(PeerDelayData *peer_delay, vtss_appl_ptp_protocol_adr_t *ptp_dest, ptp_tag_conf_t *tag_conf)
{
    mesa_timestamp_t originTimestamp = {0,0,0};
    vtss_appl_ptp_port_identity reserved = {{0,0,0,0,0,0,0,0},0};
    MsgHeader header;
    peer_delay->last_pdelay_req_event_sequence_number = 0;
    peer_delay->ratio_clear_time = 0;
    mesa_bool_t *peer_delay_ok;
    vtss_appl_ptp_802_1as_pdelay_status_ds_t *status;
    int instance, portNumber;

    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
        header.versionPTP = VERSION_PTP;
        header.minorVersionPTP = MINOR_VERSION_PTP;
        header.transportSpecific = MAJOR_SDOID_CMLDS_802_1AS;
        header.domainNumber = CMLDS_DOMAIN;
        header.flagField[1] = 0;
        header.controlField = 0;
        memcpy(&header.sourcePortIdentity, &peer_delay->port_cmlds->status.portIdentity, sizeof(vtss_appl_ptp_port_identity));
        status = &peer_delay->port_cmlds->status.peer_d;
        instance = CMLDS_DOMAIN;
        peer_delay_ok = &peer_delay->port_cmlds->peer_delay_ok;
        portNumber = peer_delay->port_cmlds->status.portIdentity.portNumber;
    } else {
        header.versionPTP = peer_delay->ptp_port->port_config->versionNumber;
        header.minorVersionPTP = peer_delay->ptp_port->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
        header.transportSpecific = peer_delay->clock->majorSdoId;
        header.domainNumber = PEER_DOMAIN_NUMBER;//peer_delay->clock->clock_init->cfg.domainNumber;
        header.flagField[1] = (peer_delay->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS && !peer_delay->ptp_port->port_config->c_802_1as.as2020) ? PTP_PTP_TIMESCALE : 0;
        header.controlField = (peer_delay->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || peer_delay->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) ? 0 : PTP_ALL_OTHERS;
        memcpy(&header.sourcePortIdentity, &peer_delay->ptp_port->portDS.status.portIdentity, sizeof(vtss_appl_ptp_port_identity));
        status = &peer_delay->ptp_port->portDS.status.s_802_1as.peer_d;
        instance = peer_delay->clock->localClockId;
        peer_delay_ok = &peer_delay->ptp_port->portDS.status.peer_delay_ok;
        portNumber = peer_delay->ptp_port->portDS.status.portIdentity.portNumber;
    }
    header.messageLength = P_DELAY_REQ_PACKET_LENGTH;
    header.messageType = PTP_MESSAGE_TYPE_P_DELAY_REQ;
    header.reserved1 = MINOR_SDO_ID;
    header.flagField[0] = 0;
    

    T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"create peer_delay for port %d peer_delay mech=%d", tag_conf->port, peer_delay->pdelay_mech);
    header.sequenceId = peer_delay->last_pdelay_req_event_sequence_number;
    header.logMessageInterval = 0x7f;

    if (peer_delay->requester_state == PEER_DELAY_PTP_OBJCT_ACTIVE || peer_delay->requester_state == PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE ||
        peer_delay->responder_state != PEER_DELAY_PTP_OBJCT_PASSIVE) {
        vtss_ptp_peer_delay_delete(peer_delay);
    }

    if (peer_delay->requester_state != PEER_DELAY_PTP_OBJCT_ACTIVE && peer_delay->requester_state != PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE) {
        peer_delay->requester_state = PEER_DELAY_PTP_OBJCT_ACTIVE;
        peer_delay->responder_state = PEER_DELAY_PTP_OBJCT_ACTIVE;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"state %s", state2name(peer_delay->requester_state));
        memcpy(&peer_delay->mac, &ptp_dest->mac, sizeof(mesa_mac_addr_t)); /* used to refresh the mac header before each transmission */
        peer_delay->pdel_req_buf.size =vtss_1588_packet_tx_alloc(&peer_delay->pdel_req_buf.handle, &peer_delay->pdel_req_buf.frame, ENCAPSULATION_SIZE + PACKET_SIZE);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"handle %p", peer_delay->pdel_req_buf.handle);
        peer_delay->pdel_req_buf.header_length = vtss_1588_pack_encap_header(peer_delay->pdel_req_buf.frame, NULL, ptp_dest, P_DELAY_REQ_PACKET_LENGTH, true, instance);
        if (ptp_dest->ip != 0) {
            peer_delay->pdel_req_buf.inj_encap.type = MESA_PACKET_ENCAP_TYPE_IP4;
        } else {
            peer_delay->pdel_req_buf.inj_encap.type = MESA_PACKET_ENCAP_TYPE_ETHER;
        }
        peer_delay->pdel_req_buf.inj_encap.tag_count = 0;
        if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_NON_802_1AS) {
            vtss_1588_tag_get(tag_conf, peer_delay->clock->localClockId, &peer_delay->pdel_req_buf.tag);
        } else {
            /* no tagging on 802.1as Peer Delay messages */
            peer_delay->pdel_req_buf.tag.tpid = 0;
            peer_delay->pdel_req_buf.tag.vid = 0;
            peer_delay->pdel_req_buf.tag.pcp = 0;
        }
        peer_delay->pdel_req_buf.size = peer_delay->pdel_req_buf.header_length + P_DELAY_REQ_PACKET_LENGTH;
        vtss_ptp_pack_msg_pdelay_xx(peer_delay->pdel_req_buf.frame + peer_delay->pdel_req_buf.header_length, &header, &originTimestamp, &reserved);
        peer_delay->pdel_req_buf.hw_time = 0;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"frame %p", peer_delay->pdel_req_buf.frame);
        vtss_ptp_timer_init(&peer_delay->pdelay_req_timer, "pdelay_req", -1, vtss_ptp_peer_delay_req_timer, peer_delay);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"timer");
        // Start the peer delay mesaurements immediately at the next tick.
        vtss_ptp_timer_start(&peer_delay->pdelay_req_timer, 1, false);
#if defined (VTSS_SW_OPTION_P802_1_AS)
        vtss_ptp_timer_init(&peer_delay->pdelay_resp_timer, "pdelay_resp_timeout", -1, vtss_ptp_peer_delay_resp_timer, peer_delay);
#endif //VTSS_SW_OPTION_P802_1_AS
        if (port_uses_two_step(peer_delay)) {
            peer_delay->pdel_req_buf.msg_type = VTSS_PTP_MSG_TYPE_2_STEP;
            peer_delay->pdel_req_ts_context.cb_ts = vtss_ptp_peer_delay_req_event_transmitted;
            peer_delay->pdel_req_ts_context.context = peer_delay;
            peer_delay->pdel_req_buf.ts_done = &peer_delay->pdel_req_ts_context;
            /* prepare followup packet*/
            peer_delay->follow_buf.size = vtss_1588_packet_tx_alloc(&peer_delay->follow_buf.handle, &peer_delay->follow_buf.frame, ENCAPSULATION_SIZE + PACKET_SIZE);
            peer_delay->follow_buf.header_length = vtss_1588_pack_encap_header(peer_delay->follow_buf.frame, NULL, ptp_dest, P_DELAY_RESP_FOLLOW_UP_PACKET_LENGTH, false, instance);
            if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_NON_802_1AS) {
                vtss_1588_tag_get(tag_conf, peer_delay->clock->localClockId, &peer_delay->follow_buf.tag);
            } else {
                /* no tagging on 802.1as Peer Delay messages */
                peer_delay->follow_buf.tag.tpid = 0;
                peer_delay->follow_buf.tag.vid = 0;
                peer_delay->follow_buf.tag.pcp = 0;
            }
            peer_delay->follow_buf.size = peer_delay->follow_buf.header_length + P_DELAY_RESP_FOLLOW_UP_PACKET_LENGTH;
            header.messageType = PTP_MESSAGE_TYPE_P_DELAY_RESP_FOLLOWUP;
            clearFlag(header.flagField[0], PTP_TWO_STEP_FLAG);
            vtss_ptp_pack_msg_pdelay_xx(peer_delay->follow_buf.frame + peer_delay->follow_buf.header_length, &header, &originTimestamp, &reserved);
            peer_delay->follow_buf.hw_time = 0;
            peer_delay->follow_buf.ts_done = NULL;

        } else { /* one step */
            peer_delay->pdel_req_buf.msg_type = VTSS_PTP_MSG_TYPE_CORR_FIELD; /* one-step PDelay_Req packet using correction field update */
            peer_delay->pdel_req_buf.ts_done = NULL;
            peer_delay->follow_buf.handle = 0;
        }
    } else {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"state %s", state2name(peer_delay->requester_state));
        peer_delay->requester_state = PEER_DELAY_PTP_OBJCT_ACTIVE;
        peer_delay->responder_state = PEER_DELAY_PTP_OBJCT_ACTIVE;
        T_WG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Could not re-create Peer delay process");
    }
    init_pdelay_measurements(peer_delay);
    *peer_delay_ok = false;
    peer_delay->peerDelayNotMeasured = true;
#if defined (VTSS_SW_OPTION_P802_1_AS)
    char str1[40];
    // reset 802.1AS specific data
    peer_delay->old_t3_t4_ok = false;
    peer_delay->neighbor_rate_ratio_valid = false;
    status->neighborRateRatio = 0;
    sprintf(str1,"RateRatio filter port %d", portNumber);
    new (&peer_delay->neighbor_rate_filter) vtss_ptp_filters::vtss_lowpass_filter_t (str1, RATE_LP_FILTER_CONST);
    peer_delay->current_measure_status = 0;
#endif // defined (VTSS_SW_OPTION_P802_1_AS)
}
/*
 * delete a PTP Peer Delay instance
 * free packet buffer(s)
 */
void vtss_ptp_peer_delay_delete(PeerDelayData *peer_delay)
{
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"state %s", state2name(peer_delay->requester_state));
    if (peer_delay->requester_state == PEER_DELAY_PTP_OBJCT_ACTIVE || peer_delay->requester_state == PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE ||
        peer_delay->responder_state != PEER_DELAY_PTP_OBJCT_PASSIVE) {
        if (peer_delay->pdel_req_buf.handle) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"buf.handle %p", peer_delay->pdel_req_buf.handle);
            vtss_1588_packet_tx_free(&peer_delay->pdel_req_buf.handle);
        }
        if (peer_delay->follow_buf.handle) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"buf.handle %p", peer_delay->follow_buf.handle);
            vtss_1588_packet_tx_free(&peer_delay->follow_buf.handle);
        }

        vtss_ptp_timer_stop(&peer_delay->pdelay_req_timer);
#if defined (VTSS_SW_OPTION_P802_1_AS)
        vtss_ptp_timer_stop(&peer_delay->pdelay_resp_timer);
#endif
        peer_delay->requester_state = PEER_DELAY_PTP_OBJCT_PASSIVE;
        peer_delay->responder_state = PEER_DELAY_PTP_OBJCT_PASSIVE;
        if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_802_1AS_CMLDS) {
            peer_delay->ptp_port->portDS.status.peer_delay_ok = true;
            peer_delay->ptp_port->portDS.status.s_802_1as.peer_d.isMeasuringDelay =  FALSE;
#if defined (VTSS_SW_OPTION_P802_1_AS)
            peer_delay->ptp_port->portDS.status.s_802_1as.peer_d.neighborRateRatio = 0;
            peer_delay->ptp_port->portDS.status.s_802_1as.asCapable = FALSE;
#endif // defined (VTSS_SW_OPTION_P802_1_AS)
        } else {
#if defined (VTSS_SW_OPTION_P802_1_AS)
            peer_delay->port_cmlds->peer_delay_ok = true;
            peer_delay->port_cmlds->status.peer_d.neighborRateRatio = 0;
            peer_delay->port_cmlds->status.asCapableAcrossDomains = FALSE;
            peer_delay->port_cmlds->status.peer_d.isMeasuringDelay =  FALSE;
#endif
        }
        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"delete peer delay");
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"state %s", state2name(peer_delay->requester_state));
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Peer Delay not active");
    }
}

void vtss_ptp_pdelay_vid_update(PeerDelayData *peer_delay, ptp_tag_conf_t *tag_conf)
{
    T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"tag update vid %d port %d", tag_conf->vid, peer_delay->ptp_port->portDS.status.portIdentity.portNumber);
    // update peer request, peer delay response follow-up messages.
    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_NON_802_1AS) {
        vtss_1588_tag_get(tag_conf, peer_delay->clock->localClockId, &peer_delay->pdel_req_buf.tag);
        vtss_1588_tag_get(tag_conf, peer_delay->clock->localClockId, &peer_delay->follow_buf.tag);
    }
}

/*
 * Stop requesting Peer delay responses. But, the responder state would be still active.
 * This is needed When Common Mean Link Delay Service is enabled but the instance specific requests from the Peer need to be accepted.
 */
void vtss_ptp_peer_delay_request_stop(PeerDelayData *peer_delay)
{
    if (peer_delay->requester_state == PEER_DELAY_PTP_OBJCT_ACTIVE || peer_delay->requester_state == PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE) {
        if (peer_delay->pdel_req_buf.handle) {
            vtss_1588_packet_tx_free(&peer_delay->pdel_req_buf.handle);
        }
        vtss_ptp_timer_stop(&peer_delay->pdelay_req_timer);
#if defined (VTSS_SW_OPTION_P802_1_AS)
        vtss_ptp_timer_stop(&peer_delay->pdelay_resp_timer);
#endif
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY, "Peer Delay Requests stopped");
        peer_delay->requester_state = PEER_DELAY_PTP_OBJCT_PASSIVE;
        if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_802_1AS_CMLDS) {
            peer_delay->ptp_port->portDS.status.peer_delay_ok = true;
#if defined (VTSS_SW_OPTION_P802_1_AS)
            peer_delay->ptp_port->portDS.status.s_802_1as.peer_d.isMeasuringDelay =  FALSE;
            peer_delay->ptp_port->portDS.status.s_802_1as.peer_d.neighborRateRatio = 0;
            peer_delay->ptp_port->portDS.status.s_802_1as.asCapable = FALSE;
            peer_delay->ptp_port->portDS.status.s_802_1as.peer_d.currentComputeNeighborRateRatio = FALSE;
            peer_delay->ptp_port->portDS.status.s_802_1as.peer_d.currentComputeMeanLinkDelay = FALSE;
#endif // defined (VTSS_SW_OPTION_P802_1_AS)
        } else {
            peer_delay->port_cmlds->peer_delay_ok = true;
            peer_delay->port_cmlds->status.peer_d.isMeasuringDelay =  FALSE;
            peer_delay->port_cmlds->status.peer_d.neighborRateRatio = 0;
            peer_delay->port_cmlds->status.asCapableAcrossDomains = FALSE;
            peer_delay->port_cmlds->status.peer_d.currentComputeNeighborRateRatio = FALSE;
            peer_delay->port_cmlds->status.peer_d.currentComputeMeanLinkDelay = FALSE;
        }
    }
}
/*
 * Peer Delay Requester part
 ***************************
 *
 * delayCalc
 *      if (t1_valid && t2_t3_valid && t4_valid && !peerDelayNotMeasured)
 *          t4minust1 = t4 - t1
 *          meanpathdelay = t4 - t1 -CF (t3-t2 is held in the CF)
 *          filter
 *          if (valid delay) peer_delay_ok
 *          else peerDelayNotMeasured.
 *
 *  peerDelayNotMeasured usage:
 *      Set to false when a PdelayReq is issued, set to false during the process if an error is detected.
 *     peerDelayNotMeasured is set if no peer node answers, or if more than one node answers
 *                            or the calculated mean path delay is > meanLinkDelayThresh
 * Requester sequence flow:
 * event        action
 * init         peerDelayNotMeasured
 * timer        if (missed responses) peerDelayNotMeasured
 *              if (peerDelayNotMeasured) ^asCapable
 *              else asCapable
 *              ^t1_valid, ^t2_t3_valid, ^t4_valid
 *              ^peerDelayNotMeasured
 *              1-step: read t1;send Pdelay_Req, t1_valid
 *              2-step: send Pdelay_Req
 *
 * event_trans  if (^peerDelayNotMeasured)
 *                  if (t1_valid) peerDelayNotMeasured (more than 1 timesstamps received)
 *                  else save t1; t1_valid; delayCalc
 *
 * delayResp    if (^peerDelayNotMeasured)
 *                  if (t4_valid) peerDelayNotMeasured (more than 1 PdelayResp received)
 *                  else save t2 and t4; t4_valid
 *                      if (response is 1-step) t3_t2 is included in CF; t3_t2_valid; delayCalc
 *                      else wait_follow
 *
 * followup     if (^peerDelayNotMeasured)
 *                  if (wait_follow) save t3; t3_t2 = t3 - t2; delayCalc
 *                  else peerDelayNotMeasured (more than 1 followup received)
 */

#if defined (VTSS_SW_OPTION_P802_1_AS)
/*
 * Helper function that calculates the neighbor_rate_ratio, according to 802.1AS
 * The ratio is calculated as ratio = (t3(n) - t3(0)) / (t4(n) - t4(0)) =>
 * Mratio = ((t3(n) - t3(0)) / (t4(n) - t4(0)) - 1)(2**41) =
 * (((t3(n) - t3(0)) * (2**41))/(t4(n) - t4(0)) - (2**41)
 * We use the floating point data type.

 */
static void ptp_peer_rate_ratio_calc(PeerDelayData *peer_delay)
{
    mesa_timeinterval_t deltat3;
    mesa_timeinterval_t deltat4;
    char str1 [50];
    char str2 [50];
    i64 mratio;
    vtss_appl_ptp_802_1as_pdelay_status_ds_t *status;

    if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_802_1AS_CMLDS) {
        status = &peer_delay->ptp_port->portDS.status.s_802_1as.peer_d;
    } else {
        status = &peer_delay->port_cmlds->status.peer_d;
    }
    if (status->currentComputeNeighborRateRatio) {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY," nbrRateRatio %d, ratio_valid %s old_t3_t4_ok %d t2_t3_valid %d t4_valid %d", status->neighborRateRatio, peer_delay->neighbor_rate_ratio_valid ? "true" : "false", peer_delay->old_t3_t4_ok, peer_delay->t2_t3_valid, peer_delay->t4_valid);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"t3 %s, oldt3 %s", TimeStampToString(&peer_delay->t3,str1), TimeStampToString(&peer_delay->oldt3,str2));
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"t4 %s, oldt4 %s", TimeStampToString(&peer_delay->t4,str1), TimeStampToString(&peer_delay->oldt4,str2));
        if (peer_delay->old_t3_t4_ok) {
            if (!peer_delay->pdelay_missed_response_cnt && peer_delay->t2_t3_valid  && peer_delay->t4_valid) {
                /* Wait for atleast 1 set of peer delay message exchange before computing neighbor rate ratio. */
                if (peer_delay->ratio_settle_time >= 1) {
                    vtss_tod_sub_TimeInterval(&deltat3, &peer_delay->t3, &peer_delay->oldt3);
                    vtss_tod_sub_TimeInterval(&deltat4, &peer_delay->t4, &peer_delay->oldt4);
                    T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"deltat3 %s deltat4 %s", vtss_tod_TimeInterval_To_String(&deltat3, str1,','), vtss_tod_TimeInterval_To_String(&deltat4, str2,','));
                    if (llabs(deltat3) < VTSS_TIMEINTERVAL_1US) peer_delay->current_measure_status |= PEER_DELAY_DELTA_T3_ERROR;
                    if (llabs(deltat4) < VTSS_TIMEINTERVAL_1US) {
                        peer_delay->current_measure_status |= PEER_DELAY_DELTA_T4_ERROR;
                        T_WG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"deltat4 %s", vtss_tod_TimeInterval_To_String(&deltat4, str2,','));
                        peer_delay->fratio = 1.0;
                    } else {
                        peer_delay->fratio = (double)deltat3/(double)deltat4;
                    }
                    if (peer_delay->fratio >= (1.0 + MAX_PPM_OFFSET) || peer_delay->fratio <= (1.0 - MAX_PPM_OFFSET)) {
                        /* Wait for atleast 1 instances of rate ratio before declaring nbr rate ratio as invalid. */
                        if (peer_delay->ratio_clear_time++ > 1) {
                            peer_delay->ratio_clear_time = 0;
                            peer_delay->neighbor_rate_ratio_valid = false;
                            peer_delay->neighbor_rate_filter.reset();
                            status->neighborRateRatio = 0;
                            peer_delay->current_measure_status |= PEER_DELAY_INVALID_RATE_RATIO_ERROR;
                            T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"deltat3 %s deltat4 %s", vtss_tod_TimeInterval_To_String(&deltat3, str1,','), vtss_tod_TimeInterval_To_String(&deltat4, str2,','));
                        }
                    } else {
                        peer_delay->ratio_clear_time = 0;
                        mratio = (i64)((peer_delay->fratio-1.0) * (double)RATE_TO_U32_CONVERT_FACTOR);
                        peer_delay->neighbor_rate_ratio_valid = true;
                        (void)peer_delay->neighbor_rate_filter.filter(&mratio);
                        //peer_delay->neighbor_rate_ratio_valid = peer_delay->neighbor_rate_filter.filter(&mratio);
                        status->neighborRateRatio = (i32)mratio;
                        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"rate_ratio %g, converted ratio %d, ratio_valid %s", peer_delay->fratio-1.0, status->neighborRateRatio, peer_delay->neighbor_rate_ratio_valid ? "true" : "false");
                    }
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"rate_ratio %g, ratio_valid %s", peer_delay->fratio-1.0, peer_delay->neighbor_rate_ratio_valid ? "true" : "false");
                } else {
                    peer_delay->ratio_settle_time++;
                    peer_delay->ratio_clear_time = 0;
                }
            }
        } else {
            peer_delay->ratio_settle_time = 0;
            /* Once the nbr rate ratio is true and the successive old_t3_t4_ok are false, still the rate ratio holds valid value indefinitely. To avoid this, rate ratio is cleared after sometime. */
            if (peer_delay->ratio_clear_time++ > 1 && peer_delay->neighbor_rate_ratio_valid) {
                peer_delay->neighbor_rate_ratio_valid = false;
                peer_delay->neighbor_rate_filter.reset();
                status->neighborRateRatio = 0;
                peer_delay->current_measure_status |= PEER_DELAY_INVALID_RATE_RATIO_ERROR;
                T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Rate ratio reset");
            }
        }
    }
    if (!peer_delay->pdelay_missed_response_cnt) {
        if (peer_delay->t2_t3_valid  && peer_delay->t4_valid) {
            peer_delay->old_t3_t4_ok = true;
            peer_delay->oldt3 = peer_delay->t3;
            peer_delay->oldt4 = peer_delay->t4;
        } else {
            peer_delay->old_t3_t4_ok = false;
        }
    }
}

static void ptp_peer_rate_ratio_clear(PeerDelayData *peer_delay)
{
    if (peer_delay->neighbor_rate_ratio_valid) {
        peer_delay->neighbor_rate_ratio_valid = false;
        peer_delay->neighbor_rate_filter.reset();
        if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_802_1AS_CMLDS) {
            peer_delay->ptp_port->portDS.status.s_802_1as.peer_d.neighborRateRatio = 0;
        } else {
            peer_delay->port_cmlds->status.peer_d.neighborRateRatio = 0;
        }
        peer_delay->old_t3_t4_ok = false;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"rate_ratio cleared");
    }
    peer_delay->current_measure_status |= PEER_DELAY_ONESTEP_RESPONDER_ERROR;
}
#endif //defined (VTSS_SW_OPTION_P802_1_AS)


/*
 * PTP peer delay request timer
 * update sequence number
 * send packet
 * mark packet buffer as in use
 * increment sequence number.
 * start timer
 */
/*lint -esym(459, vtss_ptp_peer_delay_req_timer) */
void vtss_ptp_peer_delay_req_timer(vtss_ptp_sys_timer_t *timer, void *m)
{
    PeerDelayData *peer_delay = (PeerDelayData *)m;
    mesa_timeinterval_t new_cf = 0LL;
    u32 to;
    uint16_t portNumber;
    mesa_timeinterval_t peerMeanPathDelay;
    vtss_appl_ptp_802_1as_port_peer_delay_statistics_t *stats;
    vtss_appl_ptp_802_1as_pdelay_conf_port_ds_t *conf;
    vtss_appl_ptp_802_1as_pdelay_status_ds_t *status;
    uint32_t *rxPTPPacketDiscardCount;
    mesa_bool_t *peer_delay_ok;
    u32 cur_set_time_count;
    u64 port_mask;
    bool p2p_state;
    u8 domain = 0, clk_domain;
    mesa_timeinterval_t delayAsymmetry;
#if defined (VTSS_SW_OPTION_P802_1_AS)
    BOOL newAscapable;
#endif
    vtss_ptp_phy_corr_type_t phy_corr;
    uint32_t ts_id;
    bool aedProfile = false;
    bool cmlds = false;
    uint16_t pdelayTimeout;

    if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_802_1AS_CMLDS) {
        conf = &peer_delay->ptp_port->port_config->c_802_1as.peer_d;
        stats = &peer_delay->ptp_port->port_statistics.peer_d;
        status = &peer_delay->ptp_port->portDS.status.s_802_1as.peer_d;
        port_mask = peer_delay->ptp_port->port_mask;
        rxPTPPacketDiscardCount = &peer_delay->ptp_port->port_statistics.rxPTPPacketDiscardCount;
        peerMeanPathDelay = peer_delay->ptp_port->portDS.status.peerMeanPathDelay;
        portNumber = peer_delay->ptp_port->portDS.status.portIdentity.portNumber;
        peer_delay_ok = (mesa_bool_t *)&peer_delay->ptp_port->portDS.status.peer_delay_ok;
#if defined (VTSS_SW_OPTION_P802_1_AS)
        newAscapable = peer_delay->ptp_port->portDS.status.s_802_1as.asCapable;
#endif
        p2p_state = peer_delay->ptp_port->p2p_state;
        clk_domain = peer_delay->clock->clock_init->cfg.clock_domain;
        domain = PEER_DOMAIN_NUMBER;//peer_delay->clock->clock_init->cfg.domainNumber;
        delayAsymmetry = peer_delay->ptp_port->port_config->delayAsymmetry;
        if (peer_delay->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            aedProfile = true;
        }
        pdelayTimeout = peer_delay->ptp_port->port_config->c_802_1as.as2020 ? PEER_DELAY_RESPONSE_TIMEOUT_2020 : PEER_DELAY_RESPONSE_TIMEOUT;
    } else {
        conf = &peer_delay->port_cmlds->conf->peer_d;
        stats = &peer_delay->port_cmlds->statistics.peer_d;
        status = &peer_delay->port_cmlds->status.peer_d;
        port_mask = ((u64)1) << peer_delay->port_cmlds->uport;
        rxPTPPacketDiscardCount = &peer_delay->port_cmlds->statistics.rxPTPPacketDiscardCount;
        clk_domain = CMLDS_DOMAIN;
        peer_delay_ok = (mesa_bool_t *)&peer_delay->port_cmlds->peer_delay_ok;
#if defined (VTSS_SW_OPTION_P802_1_AS)
        newAscapable = peer_delay->port_cmlds->status.asCapableAcrossDomains;
        peerMeanPathDelay = peer_delay->port_cmlds->status.meanLinkDelay;
        portNumber = peer_delay->port_cmlds->uport;
#endif
        p2p_state = peer_delay->port_cmlds->p2p_state;
        delayAsymmetry = peer_delay->port_cmlds->conf->delayAsymmetry;
        cmlds = true;
        pdelayTimeout = PEER_DELAY_RESPONSE_TIMEOUT_2020;
    }
    T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY," state %s", state2name(peer_delay->requester_state));
#if defined (VTSS_SW_OPTION_P802_1_AS)
    if (port_uses_two_step(peer_delay)) {
        // two-step is needed for rate ratio calculations
        ptp_peer_rate_ratio_calc(peer_delay);
    }
#endif //defined (VTSS_SW_OPTION_P802_1_AS)

    cur_set_time_count = vtss_domain_clock_set_time_count_get(clk_domain);
    if (cur_set_time_count != peer_delay->requester_set_time_count) {
        peer_delay->requester_set_time_count = cur_set_time_count;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"set_time_count changed to %d", cur_set_time_count);
#if defined (VTSS_SW_OPTION_P802_1_AS)
        if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_NON_802_1AS) {
            peer_delay->old_t3_t4_ok = false; // old t4 is ignored if time has been changed since last pdelay_req, because this wil result in a wrong rateRatio
        }
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
    }
#if defined (VTSS_SW_OPTION_P802_1_AS)
    if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_NON_802_1AS) {
        status->isMeasuringDelay =  peer_delay->t2_t3_valid ? TRUE : FALSE;
        if (!peer_delay->peerDelayNotMeasured && peer_delay->t1_valid && peer_delay->t2_t3_valid && peer_delay->t4_valid) {
            // linkDelayThreshold should not trigger asCapable for 802.1AS-AED profile (AED standard)
            if ((!peer_delay->neighbor_rate_ratio_valid && port_uses_two_step(peer_delay)) || (llabs(peerMeanPathDelay) > conf->meanLinkDelayThresh)) {
                if (peer_delay->detectedFaults <= conf->allowedFaults) {
                    peer_delay->detectedFaults++;
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"detected faults = %d", peer_delay->detectedFaults);
                } else {
                    newAscapable = FALSE;
                    status->isMeasuringDelay = FALSE;
                    peer_delay->detectedFaults = 0;
                }
                // Set the current PdelayReqInterval to the intial value until stable (AED only)
                if (aedProfile) {
                    status->currentLogPDelayReqInterval = conf->initialLogPdelayReqInterval;
                }
            } else {
                // Set the current PdelayReqInterval to the operational value (AED only)
                if (aedProfile) {
                    status->currentLogPDelayReqInterval = conf->operLogPdelayReqInterval;
                }
                T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"newAscapable = true");
                newAscapable = TRUE;
                peer_delay->detectedFaults = 0;
            }
        }
    }
#endif
    bool new_delay_ok = peer_delay->peerDelayNotMeasured ? false : true;
    if (*peer_delay_ok != new_delay_ok) {
        *peer_delay_ok = new_delay_ok;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"port %d: peer_delay_ok changed to %s", portNumber, new_delay_ok ? "true" : "false");
    }
#if defined (VTSS_SW_OPTION_P802_1_AS)
    newAscapable = newAscapable && !peer_delay->peerDelayNotMeasured;
    if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_802_1AS_CMLDS) {
        if (newAscapable != peer_delay->ptp_port->portDS.status.s_802_1as.asCapable) {
            peer_delay->ptp_port->portDS.status.s_802_1as.asCapable = newAscapable;
            T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Port %d: AsCapable %d, notMeasured %d, ratio_valid %d", peer_delay->ptp_port->portDS.status.portIdentity.portNumber, newAscapable, peer_delay->peerDelayNotMeasured, peer_delay->neighbor_rate_ratio_valid);
            // Tell BMCA if the port is asCapable
            vtss_ptp_802_1as_bmca_ascapable(peer_delay->ptp_port);
        }
    } else {
        if (newAscapable != peer_delay->port_cmlds->status.asCapableAcrossDomains) {
            peer_delay->port_cmlds->status.asCapableAcrossDomains = newAscapable;
            T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Port %d: AsCapableAcrossDomains %d, notMeasured %d, ratio_valid %d", peer_delay->port_cmlds->uport, newAscapable, peer_delay->peerDelayNotMeasured, peer_delay->neighbor_rate_ratio_valid);
        }
    }
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
    if (peer_delay->current_measure_status != 0) {
        show_measure_status(peer_delay->current_measure_status);
        if (!peer_delay->t2_t3_valid || peer_delay->waitingForPdelayRespFollow) {
            *rxPTPPacketDiscardCount += 1;
        }
    }
    peer_delay->current_measure_status = 0;
    T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"pdelay_missed_response_cnt %d", peer_delay->pdelay_missed_response_cnt);
    switch (peer_delay->requester_state) {
        case PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE:
            T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Missed Event transmitted event, sequence no = %d",peer_delay->last_pdelay_req_event_sequence_number);
            /* fall through is intended */
        case PEER_DELAY_PTP_OBJCT_ACTIVE:
            to = PTP_LOG_TIMEOUT(0);
            if (p2p_state) {  /* transmit PdelayReq if link is up */
                if (status->currentLogPDelayReqInterval != 127) {
                    if (status->currentLogPDelayReqInterval == 126) {
                        T_EG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"currentLogPDelayReqInterval == 126 should never occur");
                    }

                    to = PTP_LOG_TIMEOUT(status->currentLogPDelayReqInterval) + (vtss_ptp_get_rand(&peer_delay->random_seed)%16);
    T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"state %s cur %d to=%d", state2name(peer_delay->requester_state), status->currentLogPDelayReqInterval, to);
                    peer_delay->t1.sec_msb = 0;
                    peer_delay->t1.seconds = 0;
                    peer_delay->t1.nanoseconds = 0;
                    peer_delay->t4.sec_msb = 0;
                    peer_delay->t4.seconds = 0;
                    peer_delay->t4.nanoseconds = 0;
                    peer_delay->t2.sec_msb = 0;
                    peer_delay->t2.seconds = 0;
                    peer_delay->t2.nanoseconds = 0;
                    peer_delay->t3.sec_msb = 0;
                    peer_delay->t3.seconds = 0;
                    peer_delay->t3.nanoseconds = 0;
                    peer_delay->t1_valid = false;
                    peer_delay->t2_t3_valid = false;
                    peer_delay->t4_valid = false;
                    peer_delay->peerDelayNotMeasured = false;
                    vtss_tod_pack16(++peer_delay->last_pdelay_req_event_sequence_number,peer_delay->pdel_req_buf.frame + peer_delay->pdel_req_buf.header_length + PTP_MESSAGE_SEQUENCE_ID_OFFSET);
                    // domain number update
                    peer_delay->pdel_req_buf.frame[peer_delay->pdel_req_buf.header_length + PTP_MESSAGE_DOMAIN_OFFSET]= domain;
                    peer_delay->waitingForPdelayRespFollow = false;
#if defined (VTSS_SW_OPTION_P802_1_AS)
                    /* AVNU 12.5a IEEE 802.1as rev-d4-2 section 11.4.2.9 - Update logMessageInterval */
                    if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_NON_802_1AS) {
                        peer_delay->pdel_req_buf.frame[peer_delay->pdel_req_buf.header_length + PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] = status->currentLogPDelayReqInterval;
                    }
#endif

                    if (port_uses_two_step(peer_delay)) {
                        peer_delay->requester_state = PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE;
                        /* in 2 step always do asymmetry in SW */
                        vtss_ptp_pack_correctionfield(peer_delay->pdel_req_buf.frame + peer_delay->pdel_req_buf.header_length, &delayAsymmetry);
                    } else {
                        vtss_local_clock_time_get(&peer_delay->t1, peer_delay->clock->localClockId, &peer_delay->pdel_req_buf.hw_time);
                        vtss_ptp_pack_timestamp(peer_delay->pdel_req_buf.frame + peer_delay->pdel_req_buf.header_length, &peer_delay->t1);
                        peer_delay->t1_valid = true;
                        // Update correction field for phy using TC mode C or TC mode A without involving switch timestamping.
                        phy_corr = vtss_port_phy_delay_corr_upd(portNumber);
                        int64_t cor_upd;
                        switch (phy_corr) {
                            case PHY_CORR_GEN_2A:
                                vtss_tod_pack32(peer_delay->t1.nanoseconds, peer_delay->pdel_req_buf.frame + peer_delay->pdel_req_buf.header_length + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET);
                                break;
                            case PHY_CORR_GEN_3:
                                cor_upd = ((int64_t)(peer_delay->t1.seconds & 0x3FFFF) * 1000000000LL) + peer_delay->t1.nanoseconds;
                                new_cf = -(cor_upd << 16);
                                break;
                            case PHY_CORR_GEN_3C:
                                cor_upd = ((int64_t)(peer_delay->t1.seconds * 1000000000LL)) + peer_delay->t1.nanoseconds;
                                new_cf = -(cor_upd << 16);
                                break;
                            default:
                                break;
                        }
                        vtss_ptp_pack_correctionfield(peer_delay->pdel_req_buf.frame + peer_delay->pdel_req_buf.header_length, &new_cf);
                    }

                    vtss_1588_pack_eth_header(peer_delay->pdel_req_buf.frame, peer_delay->mac);
                    if (!vtss_1588_tx_msg(port_mask, &peer_delay->pdel_req_buf,
                                          cmlds ? 0 : peer_delay->clock->localClockId,
                                          cmlds, &ts_id)) {
                        T_WG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"PDelay_Req message transmission failed");
                    } else {
                        T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"sent PDelay_Req message");
                        stats->txPdelayRequestCount++;
                    }
                } else {
                    // requested to stop sending PdelayRequests
                    peer_delay->t2_t3_valid = false;
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"peerDelayNotMeasured = true");
                    peer_delay->peerDelayNotMeasured = true;
                }
                vtss_ptp_timer_start(timer, to, false);
                // start timer to timeout peer delay response
#if defined(VTSS_SW_OPTION_P802_1_AS)
                if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_NON_802_1AS) {
                    u32 ticks = (pdelayTimeout * 1000/tick_duration_us) + 1;
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY, "pdelay resp timer started");
                    vtss_ptp_timer_start(&peer_delay->pdelay_resp_timer, ticks, false);
                }
#endif
            } else {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"peerDelayNotMeasured = true");
                peer_delay->peerDelayNotMeasured = true;
                if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_802_1AS_CMLDS) {
                    peer_delay->clock->ssm.servo->delay_filter_reset(peer_delay->ptp_port->portDS.status.portIdentity.portNumber);  /* clears one-way P2P delay filter */
                } else {
                    peer_delay->port_cmlds->delay_filter.reset();
                }
            }
            break;
        default:
            T_WG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"invalid state");
            break;
    }
}

/*
 * 2-step Pdelay_Req packet event transmitted. Save tx_time
 *
 */
/*lint -esym(459, vtss_ptp_peer_delay_req_event_transmitted) */
static void vtss_ptp_peer_delay_req_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time)
{
    PeerDelayData *peer_delay = (PeerDelayData *)context;
    uint32_t clkDomain = 0;

    if (peer_delay != 0) {
        if (peer_delay->t1_valid) {  // more than one t1 on the same Pdelay_Req
            T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"peerDelayNotMeasured = true");
            peer_delay->peerDelayNotMeasured = true;
            peer_delay->current_measure_status |= PEER_DELAY_MORE_THAN_ONE_T1_ERROR;
        }
        if (!peer_delay->peerDelayNotMeasured) {
            /*save tx time */
            peer_delay->requester_state = PEER_DELAY_PTP_OBJCT_ACTIVE;
            peer_delay->t1_valid = true;
            if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_802_1AS_CMLDS) {
                clkDomain = peer_delay->clock->clock_init->cfg.clock_domain;
                if (clkDomain >= fast_cap(MESA_CAP_TS_DOMAIN_CNT)) {
                    clkDomain = SOFTWARE_CLK_DOMAIN;
                }
                T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Port %d: tx_time %u", peer_delay->ptp_port->portDS.status.portIdentity.portNumber, tx_time);
            } else {
                clkDomain = CMLDS_DOMAIN;
            }
            vtss_domain_clock_convert_to_time( tx_time, &peer_delay->t1, clkDomain);
            ptp_peer_delay_calc(peer_delay);
        }
    } else {
        T_WG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"unexpected timestamp event");
    }
}

#if defined (VTSS_SW_OPTION_P802_1_AS)
static void vtss_ptp_peer_delay_resp_timer(vtss_ptp_sys_timer_t *timer, void *m)
{
    PeerDelayData *peer_delay = (PeerDelayData *)m;
    vtss_appl_ptp_802_1as_pdelay_conf_port_ds_t *conf;

    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
        conf = &peer_delay->port_cmlds->conf->peer_d;
    } else {
        conf = &peer_delay->ptp_port->port_config->c_802_1as.peer_d;
    }
    if (!peer_delay->t1_valid || !peer_delay->t4_valid || !peer_delay->t2_t3_valid) {
        if (++peer_delay->pdelay_missed_response_cnt >= conf->allowedLostResponses) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"missed response exceeded allowed count");
            peer_delay->peerDelayNotMeasured = true;
            if (peer_delay->pdelay_mech != PTP_PDELAY_MECH_802_1AS_CMLDS) {
                peer_delay->clock->ssm.servo->delay_filter_reset(peer_delay->ptp_port->portDS.status.portIdentity.portNumber);  /* clears one-way P2P delay filter */
                peer_delay->ptp_port->port_statistics.peer_d.pdelayAllowedLostResponsesExceededCount++;
            } else {
                peer_delay->port_cmlds->delay_filter.reset();
                peer_delay->port_cmlds->statistics.peer_d.pdelayAllowedLostResponsesExceededCount++;
            }
            peer_delay->current_measure_status |= PEER_DELAY_MISSED_RESPONSE_ERROR;
            ptp_peer_rate_ratio_clear(peer_delay);
        }
        T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY, "missed response %d t1_valid %d", peer_delay->pdelay_missed_response_cnt, peer_delay->t1_valid);
    } else {
        peer_delay->pdelay_missed_response_cnt = 0;
    }
}
#endif //VTSS_SW_OPTION_P802_1_AS
/*
 * Handle PDelay_Resp from a responder
 *
 */
bool vtss_ptp_peer_delay_resp(PeerDelayData *peer_delay, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header)
{
    bool forwarded = false;
    u8 *buf = tx_buf->frame + tx_buf->header_length;
    vtss_appl_ptp_port_identity port_id, my_port_id;
    char str1[50];
    char str2[50];
    char str3[50];
    bool p2p_state = false;
    uint32_t clkDomain = 0;

    VTSS_ASSERT(peer_delay != NULL);
    VTSS_ASSERT(tx_buf != NULL);
    VTSS_ASSERT(header != NULL);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"state %s pdelay-mech=%d", state2name(peer_delay->requester_state), peer_delay->pdelay_mech);
    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
        memcpy(&my_port_id, &peer_delay->port_cmlds->status.portIdentity, sizeof(vtss_appl_ptp_port_identity)); 
        p2p_state = peer_delay->port_cmlds->p2p_state;
        clkDomain = CMLDS_DOMAIN;
    } else if (peer_delay->ptp_port != NULL) {
        memcpy(&my_port_id, &peer_delay->ptp_port->portDS.status.portIdentity, sizeof(vtss_appl_ptp_port_identity));
        p2p_state = peer_delay->ptp_port?peer_delay->ptp_port->p2p_state:false;
        clkDomain = peer_delay->clock->clock_init->cfg.clock_domain;
        if (clkDomain >= fast_cap(MESA_CAP_TS_DOMAIN_CNT)) {
            clkDomain = SOFTWARE_CLK_DOMAIN;
        }
    }
    vtss_domain_clock_convert_to_time(tx_buf->hw_time, &peer_delay->t4, clkDomain);

    if ((peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) || (peer_delay->ptp_port != NULL && peer_delay->ptp_port->port_config->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_P2P)) {
        if ((peer_delay->requester_state == PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE || peer_delay->requester_state == PEER_DELAY_PTP_OBJCT_ACTIVE)) {
            if (peer_delay->t4_valid) {  // more than one PDelay_Resp on the same Pdelay_Req
                T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"peerDelayNotMeasured = true");
                peer_delay->current_measure_status |= PEER_DELAY_MORE_THAN_ONE_RESPONDER_ERROR;
#if defined (VTSS_SW_OPTION_P802_1_AS)
                peer_delay->pdelay_multiple_responses = true;
                ++peer_delay->pdelay_multiple_response_cnt;
                peer_delay->peerDelayNotMeasured = true;
                peer_delay->ptp_port->portDS.status.s_802_1as.asCapable = false;
                T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"%u x multiple responses to the same Pdelay_Req",peer_delay->pdelay_multiple_response_cnt);
                if (peer_delay->pdelay_multiple_response_cnt > 3) {
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Cease pdelay_req transmission");
                    // Multiple continuous Pdelay_resp cease Pdelay_req transmitting
                    peer_delay->pdelay_multiple_response_cnt = 0;
                    vtss_ptp_802_1as_bmca_ascapable(peer_delay->ptp_port);
                    vtss_ptp_timer_start(&peer_delay->pdelay_req_timer, AVNU_MULT_RESP_CEASE_TIME, false); // Cease for 5 minutes as per 802.1as standard
                }
#endif
            }
            if(!peer_delay->peerDelayNotMeasured) {
                T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"state %s", state2name(peer_delay->requester_state));
                if (p2p_state) {
                    vtss_ptp_unpack_msg_pdelay_xx(buf, &peer_delay->t2, &port_id);
                    if ( header->sequenceId == peer_delay->last_pdelay_req_event_sequence_number
                            && (!PortIdentitycmp(&port_id, &my_port_id)) && (memcmp(&header->sourcePortIdentity.clockIdentity, &my_port_id.clockIdentity, sizeof(vtss_appl_clock_identity))
                               )) {
                        peer_delay->t4_valid = true;
                        peer_delay->correctionField  = header->correctionField;
                        if (!getFlag(header->flagField[0], PTP_TWO_STEP_FLAG)) {
                            peer_delay->waitingForPdelayRespFollow = false;
                            peer_delay->t2_t3_valid = true;

#if defined (VTSS_SW_OPTION_P802_1_AS)
                            ptp_peer_rate_ratio_clear(peer_delay);
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                            ptp_peer_delay_calc(peer_delay);
                            T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"t4 = %s, t1 = %s, corr = %s", TimeStampToString(&peer_delay->t4,str1),
                                TimeStampToString(&peer_delay->t1,str2),
                                vtss_tod_TimeInterval_To_String(&peer_delay->correctionField,str3,'.'));
                        } else {
                            memcpy(&peer_delay->peer_port_id, &header->sourcePortIdentity, sizeof(header->sourcePortIdentity));
                            peer_delay->waitingForPdelayRespFollow = true;
                            T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"t4 = %s, t1 = %s, corr = %s", TimeStampToString(&peer_delay->t4,str1),
                                TimeStampToString(&peer_delay->t1,str2),
                                vtss_tod_TimeInterval_To_String(&peer_delay->correctionField,str3,'.'));
                        }
                    } else {
                        ++peer_delay->pdelay_event_sequence_errors;
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"handlePdelayResp: unexpected sequence no or requesting port id");
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"received sequence no %d, expected sequence no %d", header->sequenceId, peer_delay->last_pdelay_req_event_sequence_number);
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"requesting port id %s, my port id %s",ClockIdentityToString (port_id.clockIdentity, str1),
                            ClockIdentityToString (my_port_id.clockIdentity, str2));
                        return forwarded;
                    }
                }
            }
        } else {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"unexpected PdelayResp, state = %s", state2name(peer_delay->requester_state));
        }
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"unexpected PdelayResp");
    }
    return forwarded;
}

/*
 * Handle PDelay_Resp_Follow_Up from a responder
 *
 */
bool vtss_ptp_peer_delay_resp_follow_up(PeerDelayData *peer_delay, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header)
{
    bool forwarded = false;
    mesa_timeinterval_t t3minust2;
    u8 *buf = tx_buf->frame + tx_buf->header_length;
    vtss_appl_ptp_port_identity port_id, my_port_id;
    char str1[50];
    char str2[50];
    char str3[40];
    char str4[40];

    T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"state %s delay-mech=%d", state2name(peer_delay->requester_state), peer_delay->pdelay_mech);
    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
        memcpy(&my_port_id, &peer_delay->port_cmlds->status.portIdentity, sizeof(vtss_appl_ptp_port_identity)); 
    } else {
        memcpy(&my_port_id, &peer_delay->ptp_port->portDS.status.portIdentity, sizeof(vtss_appl_ptp_port_identity));
    }
    vtss_ptp_unpack_msg_pdelay_xx(buf, &peer_delay->t3, &port_id);
    if ((peer_delay->requester_state == PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE || peer_delay->requester_state == PEER_DELAY_PTP_OBJCT_ACTIVE) &&
        !peer_delay->peerDelayNotMeasured)  {
        if (( header->sequenceId == peer_delay->last_pdelay_req_event_sequence_number) &&
              !PortIdentitycmp(&port_id, &my_port_id) &&
              !PortIdentitycmp(&header->sourcePortIdentity, &peer_delay->peer_port_id)) {
            bool fault_clock_id = !memcmp(&header->sourcePortIdentity.clockIdentity, &my_port_id.clockIdentity, sizeof(vtss_appl_clock_identity));
            if (peer_delay->waitingForPdelayRespFollow && !fault_clock_id) {
#if defined (VTSS_SW_OPTION_P802_1_AS)
                if (!peer_delay->pdelay_multiple_responses) {
                    peer_delay->pdelay_multiple_response_cnt = 0;
                }
                peer_delay->pdelay_multiple_responses = false;
#endif
                peer_delay->waitingForPdelayRespFollow = false;
                // add t3-t2 to CF
                vtss_tod_sub_TimeInterval(&t3minust2, &peer_delay->t3, &peer_delay->t2);
                            T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"t2 = %s, t3 = %s, corr = %s t3-t2=%s", TimeStampToString(&peer_delay->t2,str1),
                                TimeStampToString(&peer_delay->t3,str2),
                                vtss_tod_TimeInterval_To_String(&header->correctionField,str3,'.'), vtss_tod_TimeInterval_To_String(&t3minust2,str4, '.'));
                peer_delay->correctionField += header->correctionField;
                peer_delay->correctionField += t3minust2;
                peer_delay->t2_t3_valid = true;
                ptp_peer_delay_calc(peer_delay);
            } else {
                if (peer_delay->t2_t3_valid) {
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"peerDelayNotMeasured = true");
                    peer_delay->peerDelayNotMeasured = true;
                    peer_delay->current_measure_status |= PEER_DELAY_MORE_THAN_ONE_FOLLOWUP_ERROR;
                } else if (fault_clock_id) {
                    /* Same clock id not considered as missed response in 802.1AS figure 11-8 but asCapable=isMeasuringDelay=false */
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Same clock id peerDelayNotMeasured = true");
                    peer_delay->waitingForPdelayRespFollow = false;
                    peer_delay->peerDelayNotMeasured = true;
#if defined (VTSS_SW_OPTION_P802_1_AS)
                    peer_delay->detectedFaults = 0;
#endif
                }
            }
        } else {
            ++peer_delay->pdelay_event_sequence_errors;
        }
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"unexpected PdelayRespFollowUp, state = %s", state2name(peer_delay->requester_state));
    }

    return forwarded;
}

/*
 * Peer Delay Responder part
 ***************************
 */
/*
 * Handle delay request from a requester
 */
bool vtss_ptp_peer_delay_req(PeerDelayData *peer_delay, ptp_tx_buffer_handle_t *tx_buf)
{
    mesa_timestamp_t receive_timestamp = {0,0,0};
    bool forwarded = false;
    u8 *buf = tx_buf->frame + tx_buf->header_length;
    bool p2p_state=false;
    u64 port_mask = 0;
    u32 cur_set_time_count, inst;
    vtss_appl_ptp_802_1as_port_peer_delay_statistics_t *stats;
    uint32_t ts_id;
    uint32_t clkDomain = 0;
    bool cmlds = false;

    T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"responder state %s pdelay-mech=%d", state2name(peer_delay->responder_state), peer_delay->pdelay_mech);
    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
        stats = &peer_delay->port_cmlds->statistics.peer_d;
        p2p_state = peer_delay->port_cmlds->p2p_state;
        cur_set_time_count = vtss_domain_clock_set_time_count_get(CMLDS_DOMAIN);
        port_mask = ((u64)1) << (peer_delay->port_cmlds->uport);
        inst = CMLDS_DOMAIN;
        clkDomain = CMLDS_DOMAIN;
        cmlds = true;
    } else if (peer_delay->ptp_port != NULL) {
        stats = &peer_delay->ptp_port->port_statistics.peer_d;
        p2p_state = peer_delay->ptp_port?peer_delay->ptp_port->p2p_state:false;
        port_mask = peer_delay->ptp_port->port_mask;
        cur_set_time_count = vtss_local_clock_set_time_count_get(peer_delay->clock->localClockId);
        inst = peer_delay->clock->localClockId;
        clkDomain = peer_delay->clock->clock_init->cfg.clock_domain;
        if (clkDomain >= fast_cap(MESA_CAP_TS_DOMAIN_CNT)) {
            clkDomain = SOFTWARE_CLK_DOMAIN;
        }
    }
    vtss_domain_clock_convert_to_time(tx_buf->hw_time, &receive_timestamp, clkDomain);
    tx_buf->inj_encap.type = peer_delay->pdel_req_buf.inj_encap.type;
    tx_buf->inj_encap.tag_count = 0;

    if (peer_delay->responder_state ==  PEER_DELAY_PTP_OBJCT_ACTIVE) {
        if (p2p_state) {
            T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY," state %s", state2name(peer_delay->responder_state));
            /*send pdelay resp */
            if (vtss_1588_prepare_tx_buffer(tx_buf, P_DELAY_RESP_PACKET_LENGTH, false)) {
                buf = tx_buf->frame + tx_buf->header_length;
                if (cur_set_time_count != peer_delay->responder_set_time_count) {
                    peer_delay->responder_set_time_count = cur_set_time_count;
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"set_time_count changed to %d", cur_set_time_count);
                    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS || (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_INST)) {
                        return forwarded; // dont respond if time has been changed since last pdelay_req, because this wil result in a wrong rateRatio
                    }
                }
                if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_NON_802_1AS) {
                    peer_delay->t2_cnt = tx_buf->hw_time;
                }
                /* update encapsulation header */
                /* CMLDS_DOMAIN 0 is passed as instance because currently instance id is not used in vtss_1588_update_encap_header for 802.1AS Peer delay messages */
                vtss_1588_update_encap_header(tx_buf->frame, ((buf[PTP_MESSAGE_FLAG_FIELD_OFFSET] & PTP_UNICAST_FLAG) != 0), true, P_DELAY_RESP_PACKET_LENGTH, inst);

                /* update messageType and length */
                buf[PTP_MESSAGE_MESSAGE_TYPE_OFFSET] &= 0xf0;
                buf[PTP_MESSAGE_MESSAGE_TYPE_OFFSET] |= (PTP_MESSAGE_TYPE_P_DELAY_RESP & 0x0f);
                buf[PTP_MESSAGE_CONTROL_FIELD_OFFSET] = peer_delay->pdelay_mech != PTP_PDELAY_MECH_NON_802_1AS ? 0 : PTP_ALL_OTHERS;  /* control */
                buf[PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] = 0x7f;  /* logMessageInterval */
                vtss_tod_pack16(P_DELAY_RESP_PACKET_LENGTH, buf + PTP_MESSAGE_MESSAGE_LENGTH_OFFSET); /* messageLength */
                vtss_tod_pack32(0,buf + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET); /* clear reserved field */
                vtss_ptp_pack_timestamp(buf, &receive_timestamp);
                /* requestingPortIdentity = received sourcePortIdentity */
                memcpy(&buf[PTP_MESSAGE_REQ_PORT_ID_OFFSET], &buf[PTP_MESSAGE_SOURCE_PORT_ID_OFFSET],
                       sizeof(vtss_appl_ptp_port_identity));
                /* update sourcePortIdentity from static allocated PDelay_Req buffer*/
                memcpy(&buf[PTP_MESSAGE_SOURCE_PORT_ID_OFFSET],
                       peer_delay->pdel_req_buf.frame + peer_delay->pdel_req_buf.header_length + PTP_MESSAGE_SOURCE_PORT_ID_OFFSET,
                       sizeof(vtss_appl_ptp_port_identity));

                forwarded = true;

                if (port_uses_two_step(peer_delay)) {
                    uint8_t ptpTimeScale = 0;
                    setFlag(buf[PTP_MESSAGE_FLAG_FIELD_OFFSET], PTP_TWO_STEP_FLAG);
                    //802.1as 2020 table 10-9: ptp_timescale flag required only in announce message. 802.1as 2011 standard requires ptp timescale flag.
                    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_INST && !peer_delay->ptp_port->port_config->c_802_1as.as2020) {
                        ptpTimeScale = PTP_PTP_TIMESCALE;
                    }
                    setFlag(buf[PTP_MESSAGE_FLAG_FIELD_OFFSET+1], ptpTimeScale);
                    /* requestingPortIdentity = received sourcePortIdentity, also used in followup */
                    memcpy(peer_delay->follow_buf.frame + peer_delay->follow_buf.header_length + PTP_MESSAGE_REQ_PORT_ID_OFFSET,
                           &buf[PTP_MESSAGE_REQ_PORT_ID_OFFSET], sizeof(vtss_appl_ptp_port_identity));
                    /* save sequence number in the followup buffer */
                    memcpy(peer_delay->follow_buf.frame + peer_delay->follow_buf.header_length + PTP_MESSAGE_SEQUENCE_ID_OFFSET,
                           &buf[PTP_MESSAGE_SEQUENCE_ID_OFFSET], sizeof(u16));
                    /* save correction field in the followup buffer */
                    memcpy(peer_delay->follow_buf.frame + peer_delay->follow_buf.header_length + PTP_MESSAGE_CORRECTION_FIELD_OFFSET,
                           &buf[PTP_MESSAGE_CORRECTION_FIELD_OFFSET], sizeof(mesa_timeinterval_t));
                    /* clear correction field in the Pdelay_Resp buffer */
                    memset(&buf[PTP_MESSAGE_CORRECTION_FIELD_OFFSET], 0, sizeof(mesa_timeinterval_t));
                    T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"sequenceId %d", (buf[PTP_MESSAGE_SEQUENCE_ID_OFFSET]<<8) + buf[PTP_MESSAGE_SEQUENCE_ID_OFFSET+1]);
                    peer_delay->pdel_resp_ts_context.cb_ts = vtss_ptp_peer_delay_resp_event_transmitted;
                    peer_delay->pdel_resp_ts_context.context = peer_delay;
                    tx_buf->ts_done = &peer_delay->pdel_resp_ts_context;

                    tx_buf->msg_type = VTSS_PTP_MSG_TYPE_2_STEP;
                    peer_delay->responder_state = PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE;
                    T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"2-step, context %p", peer_delay);
                } else {
                    tx_buf->msg_type = VTSS_PTP_MSG_TYPE_CORR_FIELD;
                    T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"1-step");
                }

                /* no tagging on Peer Delay messages */
                if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_NON_802_1AS) {
                    vtss_1588_tag_get(get_tag_conf(peer_delay->clock, peer_delay->ptp_port), peer_delay->clock->localClockId, &tx_buf->tag);
                } else {
                    tx_buf->tag.tpid = 0;
                    tx_buf->tag.vid = 0;
                    tx_buf->tag.pcp = 0;
                }
                tx_buf->size = tx_buf->header_length + P_DELAY_RESP_PACKET_LENGTH;
                vtss_1588_pack_eth_header(tx_buf->frame, peer_delay->mac);
                if (!vtss_1588_tx_msg(port_mask, tx_buf,
                                      cmlds ? 0 : peer_delay->clock->localClockId,
                                      cmlds, &ts_id)) {
                    T_WG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Delay_Resp message transmission failed");
                } else {
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"sent PDelay_Resp message");
                    stats->txPdelayResponseCount++;
                }
            }
        }
    } else {
        if (peer_delay->responder_state == PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE) {
            peer_delay->responder_state = PEER_DELAY_PTP_OBJCT_ACTIVE;
        }
        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"unexpected Pdelay_Req");
    }
    return forwarded;
}

/*
 * 2-step PDelay_Resp packet event transmitted. Send followup
 *
 */
/*lint -esym(459, vtss_ptp_peer_delay_resp_event_transmitted) */
static void vtss_ptp_peer_delay_resp_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time)
{
    PeerDelayData *peer_delay = (PeerDelayData *)context;
#if defined (VTSS_SW_OPTION_P802_1_AS)
    mesa_timestamp_t tx_timestamp = {0,0,0};
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
    u32 cur_set_time_count;
    u64 port_mask;
    vtss_appl_ptp_802_1as_port_peer_delay_statistics_t *stats;
    uint32_t clkDomain = 0;
    bool cmlds = false;

    T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"context %p, state %x, port %u", peer_delay, peer_delay->responder_state, portnum);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"responder state %s peer delay-mech=%d", state2name(peer_delay->responder_state), peer_delay->pdelay_mech);
    if (peer_delay->pdelay_mech == PTP_PDELAY_MECH_802_1AS_CMLDS) {
        stats = &peer_delay->port_cmlds->statistics.peer_d;
        cur_set_time_count = vtss_domain_clock_set_time_count_get(CMLDS_DOMAIN);
        port_mask = ((u64) 1) << (peer_delay->port_cmlds->uport);
        clkDomain = CMLDS_DOMAIN;
        cmlds = true;
    } else {
        stats = &peer_delay->ptp_port->port_statistics.peer_d;
        cur_set_time_count = vtss_local_clock_set_time_count_get(peer_delay->clock->localClockId);
        port_mask = peer_delay->ptp_port->port_mask;
        clkDomain = peer_delay->clock->clock_init->cfg.clock_domain;
        if (clkDomain >= fast_cap(MESA_CAP_TS_DOMAIN_CNT)) {
            clkDomain = SOFTWARE_CLK_DOMAIN;
        }
    }
    vtss_domain_clock_convert_to_time(tx_time, &tx_timestamp, clkDomain);

    if (peer_delay->responder_state == PEER_DELAY_PTP_OBJCT_WAIT_TX_DONE) {
        /*send followup */
        peer_delay->responder_state = PEER_DELAY_PTP_OBJCT_ACTIVE;
        if (cur_set_time_count != peer_delay->responder_set_time_count) {
            peer_delay->responder_set_time_count = cur_set_time_count;
            T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"set_time_count changed to %d", cur_set_time_count);
            return; // don't send followup if time has been set since the pdelay_resp was transmitted
        }
#if defined (VTSS_SW_OPTION_P802_1_AS)
        vtss_ptp_pack_timestamp(peer_delay->follow_buf.frame + peer_delay->follow_buf.header_length, &tx_timestamp);
#else
        u64 residenceTime;
        mesa_timeinterval_t r;
        vtss_1588_ts_cnt_sub(&residenceTime, tx_time, peer_delay->t2_cnt);
        vtss_1588_ts_cnt_to_timeinterval(&r,residenceTime);
        // add residence time to CF
        vtss_ptp_update_correctionfield(peer_delay->follow_buf.frame + peer_delay->follow_buf.header_length, &r);
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
        vtss_1588_pack_eth_header(peer_delay->follow_buf.frame, peer_delay->mac);
        if (!vtss_1588_tx_msg(port_mask, &peer_delay->follow_buf,
                              cmlds ? 0 : peer_delay->clock->localClockId,
                              cmlds, &ts_id)) {
            T_WG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"Follow_Up message transmission failed");
        } else {
            T_NG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"sent follow_Up message");
            stats->txPdelayResponseFollowUpCount++;
        }
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY,"unexpected timestamp event");
    }
}
