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

#include "vtss_ptp_types.h"
#include "vtss_ptp_os.h"
#include "vtss_ptp_packet_callout.h"
#include "vtss_ptp_master.h"
#include "vtss_ptp_slave.h"
#include "vtss_ptp_peer_delay.h"
#include "vtss_ptp_unicast.hxx"
#include "vtss_ptp_tc.h"
#include "vtss_tod_api.h"
#include "vtss_ptp_bmca.h"
#include "vtss_ptp_802_1as_bmca.h"
#include "vtss_ptp_internal_types.h"
#include "vtss_ptp_pack_unpack.h"
#include "vtss/appl/ptp.h"
#include "vtss_ptp_802_1as.hxx"
#include "vtss_ptp_802_1as_site_sync.h"
#include "vtss_ptp_path_trace.hxx"
#include "vtss_ptp_local_clock.h"
#include "ptp_api.h"
#if defined(VTSS_SW_OPTION_SYNCE)
#include "synce_ptp_if.h"
#endif

const u16 Ticktable[TICK_SIZE] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
const u32 TickTimeNs = 7812500;  // timer tick period in nanosec

static const mesa_mac_t ptp_primary_mcast_adr = {{0x01, 0x1b, 0x19, 0x00, 0x00, 0x00}};
static const mesa_mac_t ptp_pdelay_mcast_adr  = {{0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E}};

static const mesa_mac_t ptp_primary_ip_mcast_adr = {{0x01, 0x00, 0x5e, 0, 1, 129}};
static const mesa_mac_t ptp_pdelay_ip_mcast_adr  = {{0x01, 0x00, 0x5e, 0, 0, 107}};

static void ptp_port_initialize(ptp_clock_t *, PtpPort_t *);

static void clock_data_init(ptp_clock_t *ptpClock, const ptp_init_clock_ds_t *clock_init, const vtss_appl_ptp_clock_timeproperties_ds_t *time_prop)
{
    T_N("clock_data_init");
    /* Default data set */
    if (clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY) {
        ptpClock->defaultDS.status.clockQuality.clockClass = 255;
    } else if (clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {
        // If clock class is below 127, then in bmca, device will become master or passive.
        // To avoid passive state during bmca decision with virtual port, class is set to - 140 for 8275 profile and 187 for default ieee1588 profile.
        // Jira Ref: APPL-3625 & APPL-4788
        ptpClock->defaultDS.status.clockQuality.clockClass = (clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_1 || clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_2) ? G8275PRTC_GM_OUT_OF_HO_CLOCK_CLASS_CAT1 : DEFAULT_GM_CLOCK_CLASS;
    } else {
        ptpClock->defaultDS.status.clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
    }
    ptpClock->defaultDS.status.clockQuality.clockAccuracy = 0xfe; /* = Unknown */
    memcpy(ptpClock->defaultDS.status.clockIdentity, clock_init->clockIdentity, CLOCK_IDENTITY_LENGTH);

    ptpClock->defaultDS.status.clockQuality.offsetScaledLogVariance = (clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS ||
                                                                       clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) ?
                                                                       DEFAULT_802_1_AS_CLOCK_VARIANCE : DEFAULT_CLOCK_VARIANCE;  /* see spec 7.6.3.5 */
    ptpClock->announced_clock_quality = ptpClock->defaultDS.status.clockQuality;

    ptpClock->currentDS.stepsRemoved = 0;
#if defined (VTSS_SW_OPTION_P802_1_AS)
    ptpClock->current_802_1as_ds.lastGMPhaseChange.scaled_ns_high = 0;    /**< The value is the phase change that occurred on the most recent change in either grandmaster or gmTimeBaseIndicator. */
    ptpClock->current_802_1as_ds.lastGMPhaseChange.scaled_ns_low = 0LL;   /**< The value is the phase change that occurred on the most recent change in either grandmaster or gmTimeBaseIndicator. */
    ptpClock->current_802_1as_ds.lastGMFreqChange = 0.0;    /**< The value is the frequency change that occurred on the most recent change in either grandmaster or gmTimeBaseIndicator. */
    ptpClock->current_802_1as_ds.gmTimeBaseIndicator = 0;            /**< timeBaseIndicator of the current grandmaster */
    ptpClock->current_802_1as_ds.gmChangeCount = 0;                  /**< The number of times the grandmaster has changed in a gPTP domain */
    ptpClock->current_802_1as_ds.timeOfLastGMChangeEvent = 0;        /**< The system time when the most recent grandmaster change occurred. */
    ptpClock->current_802_1as_ds.timeOfLastGMPhaseChangeEvent = 0;   /**< The system time when the most recent change in grandmaster phase occurred due to a change of either the grandmaster or grandmaster time base. */
    ptpClock->current_802_1as_ds.timeOfLastGMFreqChangeEvent = 0;    /**< The system time when the most recent change in grandmaster frequency occurred due to a change of either the grandmaster or grandmaster time base. */
    ptpClock->defaultDS.status.s_802_1as.gmCapable = (ptpClock->clock_init->cfg.priority1 == 255) ? FALSE : TRUE;

#endif //(VTSS_SW_OPTION_P802_1_AS)

    ptpClock->defaultDS.status.numberPorts = clock_init->numberPorts;

    /* Global time properties data set */
    ptpClock->timepropertiesDS.currentUtcOffset = time_prop->currentUtcOffset;
    ptpClock->timepropertiesDS.currentUtcOffsetValid = time_prop->currentUtcOffsetValid;
    ptpClock->timepropertiesDS.leap59 = time_prop->leap59;
    ptpClock->timepropertiesDS.leap61 = time_prop->leap61;
    ptpClock->timepropertiesDS.timeTraceable = time_prop->timeTraceable;
    ptpClock->timepropertiesDS.frequencyTraceable = time_prop->frequencyTraceable;
    ptpClock->timepropertiesDS.ptpTimescale = time_prop->ptpTimescale;
    ptpClock->timepropertiesDS.timeSource = time_prop->timeSource;
    ptpClock->ssm.servo->delay_filter_reset(0);  /* clears one-way E2E delay filter */

    ptpClock->slavePort = 0;
    ptpClock->selected_master = 0xffff; // not selected

    memset(&ptpClock->path_trace, 0, sizeof(ptpClock->path_trace)); // empty path trace
    // init Follow_Up info for 802.1AS profile
#if defined (VTSS_SW_OPTION_P802_1_AS)
    ptpClock->follow_up_info.cumulativeScaledRateOffset = 0;
    ptpClock->follow_up_info.gmTimeBaseIndicator =  ptpClock->current_802_1as_ds.gmTimeBaseIndicator;
    ptpClock->follow_up_info.lastGmPhaseChange = ptpClock->current_802_1as_ds.lastGMPhaseChange;
    ptpClock->follow_up_info.scaledLastGmFreqChange =  0;
#endif //(VTSS_SW_OPTION_P802_1_AS)

    /* multicast destination addresses */
    if ( (clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET) || (clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED) ) {
        ptpClock->ptp_primary.ip = 0;
        memcpy(&ptpClock->ptp_primary.mac, &ptp_primary_mcast_adr, sizeof(mesa_mac_t));
        ptpClock->ptp_pdelay.ip = 0;
        memcpy(&ptpClock->ptp_pdelay.mac, &ptp_pdelay_mcast_adr, sizeof(mesa_mac_t));
    } else {
        ptpClock->ptp_primary.ip = PTP_PRIMARY_DEST_IP;
        memcpy(&ptpClock->ptp_primary.mac, &ptp_primary_ip_mcast_adr, sizeof(mesa_mac_t));
        ptpClock->ptp_pdelay.ip = PTP_PDELAY_DEST_IP;
        memcpy(&ptpClock->ptp_pdelay.mac, &ptp_pdelay_ip_mcast_adr, sizeof(mesa_mac_t));
    }
    ptpClock->ssm.debugMode = 0;
}

static void port_data_init(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, vtss_appl_clock_identity c, i16 maxForeign)
{
    T_N("port_data_init");

    /* Port configuration data set */
    ptpPort->portDS.status.logMinDelayReqInterval = ptpPort->port_config->logMinPdelayReqInterval;
    memcpy(ptpPort->portDS.status.portIdentity.clockIdentity, c, CLOCK_IDENTITY_LENGTH);
    ptpPort->number_foreign_records = 0;
    ptpPort->foreign_record_i = 0;
    ptpPort->max_foreign_records = maxForeign;
    /* other stuff */
    ptpClock->ssm.servo->delay_filter_reset(ptpPort->portDS.status.portIdentity.portNumber);  /* clears one-way P2P delay filter */

}

static void ptp_802_1as_send_message_interval_request(PtpPort_t *ptpPort, bool twoStep)
{
    i8 txAnv;
    i8 txSyv;
    i8 txMpr;
    u8 flags=0;

    if (ptpPort->first_message_interval_request) {
        //send as is
        txAnv = ptpPort->port_config->logAnnounceInterval;
        txSyv = ptpPort->port_config->logSyncInterval;
        txMpr = ptpPort->port_config->logMinPdelayReqInterval;
    } else {
        //send changes
        txAnv = ptpPort->port_config->logAnnounceInterval == ptpPort->transmittedLogAnnounceInterval ? -128 : ptpPort->port_config->logAnnounceInterval;
        txSyv = ptpPort->port_config->logSyncInterval == ptpPort->transmittedLogSyncInterval ? -128 : ptpPort->port_config->logSyncInterval;
        txMpr = ptpPort->port_config->logMinPdelayReqInterval == ptpPort->transmittedLogPDelayReqInterval ? -128 : ptpPort->port_config->logMinPdelayReqInterval;

    }
    if (ptpPort->port_config->delayMechanism != VTSS_APPL_PTP_DELAY_MECH_COMMON_P2P) {
        // bit 0 = computeNeighborRateRatio, bit 1 = computeNeighborRateRatio, bit 2 = oneStepReceiveCapable.
        if (ptpPort->port_config->c_802_1as.as2020 == TRUE) {
            flags = (!twoStep ? (2 << 1) : 0) | (ptpPort->portDS.status.s_802_1as.peer_d.currentComputeMeanLinkDelay ? (1 << 1) : 0) | (ptpPort->portDS.status.s_802_1as.peer_d.currentComputeNeighborRateRatio? 1 : 0);
        } else {
            flags = (ptpPort->portDS.status.s_802_1as.peer_d.currentComputeMeanLinkDelay ? (1 << 2) : 0) | (ptpPort->portDS.status.s_802_1as.peer_d.currentComputeNeighborRateRatio? (1 << 1) : 0);
        }
        issue_message_interval_request(ptpPort, txAnv, txSyv, txMpr, flags, NULL, false);
        ptpPort->transmittedLogAnnounceInterval = ptpPort->port_config->logAnnounceInterval;
        ptpPort->transmittedLogSyncInterval = ptpPort->port_config->logSyncInterval;
        ptpPort->transmittedLogPDelayReqInterval = ptpPort->port_config->logMinPdelayReqInterval;
        ptpPort->first_message_interval_request = false;
    }
}

//update current intervals
static void ptp_802_1as_update_current_message_interval(PtpPort_t *ptpPort)
{
    vtss_appl_ptp_config_port_ds_t port_ds;
    vtss_ptp_apply_profile_defaults_to_port_ds(&port_ds, ptpPort->parent->clock_init->cfg.profile);

    if (ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
        //set initial Message Interval Request values
        ptpPort->first_message_interval_request = true;
    }
    // get the initial values from profile defaults
    if (ptpPort->port_config->c_802_1as.useMgtSettableLogAnnounceInterval == FALSE) {
        ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval = (ptpPort->port_config->logAnnounceInterval == 126 || ptpPort->port_config->logAnnounceInterval == 127) ? port_ds.logAnnounceInterval : ptpPort->port_config->logAnnounceInterval;
    } else {
        ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval = ptpPort->port_config->c_802_1as.mgtSettableLogAnnounceInterval;
    }
    if (ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
        ptpPort->portDS.status.s_802_1as.currentLogSyncInterval = ptpPort->port_config->c_802_1as.initialLogSyncInterval;
    } else if (ptpPort->port_config->c_802_1as.useMgtSettableLogSyncInterval == FALSE) {
        ptpPort->portDS.status.s_802_1as.currentLogSyncInterval = (ptpPort->port_config->logSyncInterval == 126 || ptpPort->port_config->logSyncInterval == 127) ? port_ds.logSyncInterval : ptpPort->port_config->logSyncInterval;
    } else {
        ptpPort->portDS.status.s_802_1as.currentLogSyncInterval = ptpPort->port_config->c_802_1as.mgtSettableLogSyncInterval;
    }
    if (ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
        ptpPort->portDS.status.s_802_1as.peer_d.currentLogPDelayReqInterval = ptpPort->port_config->c_802_1as.peer_d.initialLogPdelayReqInterval;
    } else if (ptpPort->port_config->c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval == FALSE) {
        ptpPort->portDS.status.s_802_1as.peer_d.currentLogPDelayReqInterval = (ptpPort->port_config->logMinPdelayReqInterval == 126 || ptpPort->port_config->logMinPdelayReqInterval == 127) ? port_ds.logMinPdelayReqInterval : ptpPort->port_config->logMinPdelayReqInterval;
    } else {
        ptpPort->portDS.status.s_802_1as.peer_d.currentLogPDelayReqInterval = ptpPort->port_config->c_802_1as.peer_d.mgtSettableLogPdelayReqInterval;
    }
    if (ptpPort->port_config->c_802_1as.useMgtSettableLogGptpCapableMessageInterval == FALSE) {
        ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval = (ptpPort->port_config->c_802_1as.initialLogGptpCapableMessageInterval == 126 || ptpPort->port_config->c_802_1as.initialLogGptpCapableMessageInterval == 127) ? port_ds.c_802_1as.initialLogGptpCapableMessageInterval : ptpPort->port_config->c_802_1as.initialLogGptpCapableMessageInterval;
    } else {
        ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval = ptpPort->port_config->c_802_1as.mgtSettableLogGptpCapableMessageInterval;
    }
}


//set current intervals according to received Message Interval Request
void ptp_802_1as_set_current_message_interval(PtpPort_t *ptpPort, i8 rxAnv, i8 rxSyv, i8 rxMpr)
{
    i8 old_intvl;

    if (ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {

        if (rxAnv != -128 && (ptpPort->port_config->c_802_1as.useMgtSettableLogAnnounceInterval == FALSE)) {
            old_intvl = ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval;
            ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval = rxAnv == 126 ? ptpPort->port_config->logAnnounceInterval : rxAnv;
            if(ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval > old_intvl){
                ptpPort->ansm.announceSlowdown = TRUE;
            } else {
                ptpPort->ansm.announceSlowdown = FALSE;
                ptpPort->ansm.ann_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval;
            }

        }
        if (rxSyv != -128 && (ptpPort->port_config->c_802_1as.useMgtSettableLogSyncInterval == FALSE)) {
            old_intvl = ptpPort->portDS.status.s_802_1as.currentLogSyncInterval;
            ptpPort->portDS.status.s_802_1as.currentLogSyncInterval = rxSyv == 126 ? ptpPort->port_config->logSyncInterval : rxSyv;
            if(ptpPort->portDS.status.s_802_1as.currentLogSyncInterval > old_intvl){
                ptpPort->msm.syncSlowdown = TRUE;
            } else {
                ptpPort->msm.syncSlowdown = FALSE;
                ptpPort->msm.sync_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogSyncInterval;
            }
        }
        if (rxMpr != -128 && (ptpPort->port_config->c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval == FALSE)) {
            ptpPort->portDS.status.s_802_1as.peer_d.currentLogPDelayReqInterval = rxMpr == 126 ? ptpPort->port_config->logMinPdelayReqInterval : rxMpr;
        }
    }
}

void ptp_802_1as_set_gptp_current_message_interval(PtpPort_t *ptpPort, i8 rxGptp)
{
    i8 old_intvl;

    if (ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {

        if (rxGptp != -128 && (ptpPort->port_config->c_802_1as.useMgtSettableLogGptpCapableMessageInterval == FALSE)) {
            old_intvl = ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval;
            ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval = rxGptp == 126 ? ptpPort->port_config->c_802_1as.initialLogGptpCapableMessageInterval : rxGptp;
            if(ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval  > old_intvl){
                ptpPort->gptpsm.gPtpCapableMessageSlowdown = TRUE;
            } else {
                ptpPort->gptpsm.gPtpCapableMessageSlowdown = FALSE;
                ptpPort->gptpsm.gptp_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval;
            }

        }
    }
}

ptp_tag_conf_t *get_tag_conf(ptp_clock_t *ptpClock, PtpPort_t *ptpPort)
{
    static ptp_tag_conf_t tag_conf;
    tag_conf.vid = ptpClock->clock_init->cfg.configured_vid;
    tag_conf.pcp = ptpClock->clock_init->cfg.configured_pcp;
    tag_conf.port = ptpPort->portDS.status.portIdentity.portNumber;
    return &tag_conf;
}

/* Pointer for call back function used by vtss_ptp_clock_create to update the UDP rx filter. */
static vtss_ptp_udp_rx_filter_update_cb_t vtss_ptp_udp_rx_filter_update_cb;

/* Function for installing the pointer to the rx filter callback function */
void vtss_ptp_install_udp_rx_filter_update_cb(const vtss_ptp_udp_rx_filter_update_cb_t cb)
{
    vtss_ptp_udp_rx_filter_update_cb = cb;
}

#if defined (VTSS_SW_OPTION_P802_1_AS)
static void ptp_cmlds_reset_status_statistics(ptp_cmlds_port_ds_s *ptp_cmlds)
{
    /* Initialise all status parameters */
    ptp_cmlds->status.peer_d.isMeasuringDelay = false;
    ptp_cmlds->status.asCapableAcrossDomains = false;
    ptp_cmlds->status.meanLinkDelay = 0;
    ptp_cmlds->status.peer_d.neighborRateRatio = 0;
    ptp_cmlds->status.peer_d.currentLogPDelayReqInterval = 0;
    ptp_cmlds->status.peer_d.currentComputeNeighborRateRatio = true;
    ptp_cmlds->status.peer_d.currentComputeMeanLinkDelay = true;
    ptp_cmlds->status.peer_d.versionNumber = VERSION_PTP;
    ptp_cmlds->status.peer_d.minorVersionNumber = MINOR_VERSION_PTP;

    /* Reset all statistics */
    ptp_cmlds->statistics.peer_d.rxPdelayRequestCount = 0;
    ptp_cmlds->statistics.peer_d.rxPdelayResponseCount = 0;
    ptp_cmlds->statistics.peer_d.rxPdelayResponseFollowUpCount = 0;
    ptp_cmlds->statistics.rxPTPPacketDiscardCount = 0;
    ptp_cmlds->statistics.peer_d.pdelayAllowedLostResponsesExceededCount = 0;
    ptp_cmlds->statistics.peer_d.txPdelayRequestCount = 0;
    ptp_cmlds->statistics.peer_d.txPdelayResponseCount = 0;
    ptp_cmlds->statistics.peer_d.txPdelayResponseFollowUpCount = 0;
}
/*
 * Create and initialise PTP CMLDS instance
 */
ptp_cmlds_port_ds_t * vtss_ptp_cmlds_port_inst_add(uint port_num)
{
    ptp_cmlds_port_ds_t *ptp_cmlds = (ptp_cmlds_port_ds_t *)vtss_ptp_calloc(1, sizeof(ptp_cmlds_port_ds_t));

    ptp_cmlds->uport = port_num;

    /* Initialise all status parameters */
    ptp_cmlds_reset_status_statistics(ptp_cmlds);

    ptp_cmlds->status.cmldsLinkPortEnabled = false;
    /* Initialise all instance usage as false */
    for (int i=0; i < PTP_CLOCK_INSTANCES; i++) {
        ptp_cmlds->cmlds_in_use[i] = false;
    }
    char str[30];
    snprintf(str, sizeof(str), "CMLDS delay filter %d", port_num);
    new (&ptp_cmlds->delay_filter) vtss_ptp_filters::vtss_lowpass_filter_t(str, 4);
    ptp_cmlds->peer_delay_ok = false;
    ptp_cmlds->pDelay.pdelay_mech = PTP_PDELAY_MECH_802_1AS_CMLDS;
    ptp_cmlds->port_signaling_message_sequence_number = 0;
    vtss_ptp_peer_delay_state_init(&ptp_cmlds->pDelay);

    return ptp_cmlds;
}
void vtss_ptp_cmlds_set_pdelay_interval(ptp_cmlds_port_ds_t *port_cmlds, i8 rxMpr, bool signalling)
{
    vtss_appl_ptp_802_1as_pdelay_conf_port_ds_t *conf = &port_cmlds->conf->peer_d;
    vtss_appl_ptp_802_1as_pdelay_status_ds_t *status = &port_cmlds->status.peer_d;

    if (conf->useMgtSettableLogPdelayReqInterval) {
        status->currentLogPDelayReqInterval = conf->mgtSettableLogPdelayReqInterval;
    } else if (signalling && (rxMpr != 126)) {
        if (rxMpr != -128) {
            status->currentLogPDelayReqInterval =  rxMpr;
        }
    } else {
        status->currentLogPDelayReqInterval = conf->initialLogPdelayReqInterval;
    }
    T_D("rxMpr=%d signalling %d status %d", rxMpr, signalling, status->currentLogPDelayReqInterval);
}
/* Enable/Disable the CMLDS service on a port */
void vtss_ptp_cmlds_peer_delay_update(ptp_cmlds_port_ds_s *port_cmlds, vtss_appl_clock_identity clock_id, const vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t *conf, uint portnum, bool enable, bool conf_modified)
{
    bool compMeanDelay = port_cmlds->status.peer_d.currentComputeMeanLinkDelay;
    bool compNRR = port_cmlds->status.peer_d.currentComputeNeighborRateRatio;
    i8   curPRI  = port_cmlds->status.peer_d.currentLogPDelayReqInterval;
    u8   flags = 0;
    ptp_tag_conf_t tag_conf = {0, 0, 0};
    vtss_appl_ptp_protocol_adr_t dest_addr;

    tag_conf.port = (u16)portnum;
    dest_addr.ip = 0;
    dest_addr.mac = ptp_pdelay_mcast_adr;
    if (conf_modified) {
        vtss_ptp_cmlds_set_pdelay_interval(port_cmlds, 0, false);
        vtss_ptp_set_comp_ratio_pdelay(NULL, 0, false, port_cmlds, true);
        if (curPRI != port_cmlds->status.peer_d.currentLogPDelayReqInterval) {
            /* Restart the Peer delay service when P_delay Req interval is changed */
            ptp_cmlds_reset_status_statistics(port_cmlds);
            vtss_ptp_peer_delay_create(&port_cmlds->pDelay, &dest_addr, &tag_conf);
            /* Load current configuration after creation. */
            vtss_ptp_cmlds_set_pdelay_interval(port_cmlds, 0, false);
            vtss_ptp_set_comp_ratio_pdelay(NULL, 0, false, port_cmlds, true);
            flags = (port_cmlds->status.peer_d.currentComputeMeanLinkDelay?(1 << 1):0)|(port_cmlds->status.peer_d.currentComputeNeighborRateRatio?1:0);    // bit 0 = computeNeighborRateRatio, bit 1 = computeNeighborPropDelay.
            /* Issue message interval request TLV */
            issue_message_interval_request(NULL, 0, 0, port_cmlds->status.peer_d.currentLogPDelayReqInterval, flags, port_cmlds, true);
        } else if ((compMeanDelay != port_cmlds->status.peer_d.currentComputeMeanLinkDelay) ||
                   (compNRR != port_cmlds->status.peer_d.currentComputeNeighborRateRatio)) {
            flags = (port_cmlds->status.peer_d.currentComputeMeanLinkDelay?(1 << 1):0)|(port_cmlds->status.peer_d.currentComputeNeighborRateRatio?1:0);    // bit 0 = computeNeighborRateRatio, bit 1 = computeNeighborPropDelay.
            curPRI = (curPRI == port_cmlds->status.peer_d.currentLogPDelayReqInterval) ? -128 : port_cmlds->status.peer_d.currentLogPDelayReqInterval;
            /* Issue message interval request TLV */
            issue_message_interval_request(NULL, 0, 0, curPRI, flags, port_cmlds, true);
        }
        port_cmlds->conf_modified = false;
        T_D("Peer Delay measurement Configuration modified port %d", portnum);
    } else if (enable) {
        port_cmlds->status.cmldsLinkPortEnabled = enable;
        ptp_cmlds_reset_status_statistics(port_cmlds);
        memcpy(port_cmlds->status.portIdentity.clockIdentity, clock_id, CLOCK_IDENTITY_LENGTH);
        T_I("Peer Delay measurement initialisation port %d", portnum);
        vtss_ptp_peer_delay_create(&port_cmlds->pDelay, &dest_addr, &tag_conf);
        /* Load current configuration after creation. */
        vtss_ptp_cmlds_set_pdelay_interval(port_cmlds, 0, false);
        vtss_ptp_set_comp_ratio_pdelay(NULL, 0, false, port_cmlds, true);
        /* Issue message interval request with initial configuration. */
        flags = (port_cmlds->status.peer_d.currentComputeMeanLinkDelay?(1 << 1):0)|(port_cmlds->status.peer_d.currentComputeNeighborRateRatio?1:0);    // bit 0 = computeNeighborRateRatio, bit 1 = computeNeighborPropDelay.
        curPRI = port_cmlds->status.peer_d.currentLogPDelayReqInterval;
        issue_message_interval_request(NULL, 0, 0, curPRI, flags, port_cmlds, true);
        /* After issuing message interval request, if peer delay request interval is having 126, set current interval to initial interval. */
        if (port_cmlds->status.peer_d.currentLogPDelayReqInterval == 126) {
            port_cmlds->status.peer_d.currentLogPDelayReqInterval = port_cmlds->conf->peer_d.initialLogPdelayReqInterval;
        }
    } else {
        T_I("Peer Delay measurement Deletion port %d", portnum);
        port_cmlds->status.cmldsLinkPortEnabled = enable;
        vtss_ptp_peer_delay_delete(&port_cmlds->pDelay);
    }
}

/* Update CMLDS status on clock instances */
void vtss_ptp_cmlds_clock_inst_status_notify(ptp_clock_t *ptpClock, uint portnum, const vtss_appl_ptp_802_1as_cmlds_status_port_ds_t *status)
{
    if (ptpClock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS &&
        ptpClock->ptpPort[portnum].port_config->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_COMMON_P2P) {
        ptpClock->ptpPort[portnum].portDS.status.s_802_1as.peer_d.isMeasuringDelay = status->peer_d.isMeasuringDelay;
        ptpClock->ptpPort[portnum].portDS.status.s_802_1as.peer_d.neighborRateRatio = status->peer_d.neighborRateRatio;
        ptpClock->ptpPort[portnum].portDS.status.peerMeanPathDelay = status->meanLinkDelay;
        if (ptpClock->ptpPort[portnum].portDS.status.s_802_1as.asCapable != status->asCapableAcrossDomains &&
            ptpClock->ptpPort[portnum].neighborGptpCapable) {
            /* NeighborGPtpCapable status must also be used to determine asCapable state for individual ports */
            ptpClock->ptpPort[portnum].portDS.status.s_802_1as.asCapable = status->asCapableAcrossDomains;
            vtss_ptp_802_1as_bmca_ascapable(&ptpClock->ptpPort[portnum]);
        }
    }
}
#endif

/* set the protocol in initial state */
void vtss_ptp_clock_create(ptp_clock_t *ptpClock)
{
    char str [40];
    u16 portidx;
    mesa_rc rc;
    PtpPort_t *slave_port;
    T_I("create");

    if (ptpClock->slavePort != 0) {
        slave_port = ptpClock->ssm.slave_port;
        ptpClock->ssm.slave_port = NULL;
        vtss_ptp_slave_init(&ptpClock->ssm, &ptpClock->ptp_primary, get_tag_conf(ptpClock, slave_port));
    }
    /* in a BC frontend, two-step must be false and oneWay must be true, and are therefore overwritten */
    if (ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_BC_FRONTEND) {
        ptpClock->clock_init->cfg.oneWay = true;
        ptpClock->clock_init->cfg.twoStepFlag = false;
    }

    clock_data_init(ptpClock, ptpClock->clock_init, ptpClock->time_prop);
    masterTableInit(ptpClock->master, ptpClock);
    slaveTableInit(ptpClock->slave, ptpClock);
    ptpClock->selected_master = 0xffff; // not selected
    if ((ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT ||
            ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT) && ptpClock->clock_init->cfg.twoStepFlag) {
        vtss_ptp_tc_enable(&ptpClock->tcsm);
    } else {
        vtss_ptp_tc_disable(&ptpClock->tcsm);
    }
    if (vtss_ptp_udp_rx_filter_update_cb != NULL) {
        if ((ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI) ||
            (ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED) ||
            (ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI))
        {
            rc = vtss_ptp_udp_rx_filter_update_cb(ptpClock->localClockId, 1);
        }
        else {
            rc = vtss_ptp_udp_rx_filter_update_cb(ptpClock->localClockId, 0);
        }
        if (rc != VTSS_RC_OK) T_E("Problems updating UDP rx filter");
    }
    for (portidx = 0; portidx < ptpClock->defaultDS.status.numberPorts; portidx++) {
        if (ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT)
            ptpClock->ptpPort[portidx].port_config->delayMechanism = VTSS_APPL_PTP_DELAY_MECH_P2P;
        if (ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT)
            ptpClock->ptpPort[portidx].port_config->delayMechanism = VTSS_APPL_PTP_DELAY_MECH_E2E;

        /* initialize  */
        port_data_init(ptpClock, &ptpClock->ptpPort[portidx], ptpClock->defaultDS.status.clockIdentity, ptpClock->clock_init->max_foreign_records);
        rc = vtss_ptp_set_port_cfg(ptpClock, portidx+1, ptpClock->ptpPort[portidx].port_config);

        if (rc == VTSS_RC_OK) {
            if (ptpClock->ptpPort[portidx].port_config->enabled)
                rc = vtss_ptp_port_ena(ptpClock, portidx+1);
            else
                rc = vtss_ptp_port_dis(ptpClock, portidx+1);
        }

        vtss_ptp_port_crc_update(ptpClock->localClockId, portidx);

        if (rc == VTSS_RC_ERROR) {
            T_E("Port init failed, port no: %d", portidx+1);

        }
    }

    ptpClock->ssm.servo->offset_filter_reset();
    vtss_ptp_bmca_m1(ptpClock);
    T_N("clock identity: %s", ClockIdentityToString(ptpClock->defaultDS.status.clockIdentity, str));
    T_N("256*log2(clock variance): %d", ptpClock->defaultDS.status.clockQuality.offsetScaledLogVariance);
    T_N("clock class: %d", ptpClock->defaultDS.status.clockQuality.clockClass);
    T_N("clock priority1: %d", ptpClock->clock_init->cfg.priority1);
    T_N("PTP domain number: %d", ptpClock->clock_init->cfg.domainNumber);

#if defined (VTSS_SW_OPTION_P802_1_AS)
    if ((ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND || ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_AED_GM) &&
    (ptpClock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpClock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS)) {
        ptpClock->majorSdoId = MAJOR_SDOID_802_1AS;
        vtss_ptp_802_1as_bmca_init(ptpClock);
    } else {
#else
    {
#endif //(VTSS_SW_OPTION_P802_1_AS)
        ptpClock->majorSdoId = MAJOR_SDOID_OTHER;
        vtss_ptp_bmca_init(ptpClock);
    }
}

void ptp_port_initialize(ptp_clock_t *ptpClock, PtpPort_t *ptpPort)
{
    char str [40];
    T_I("manufacturerIdentity: %s", "2.0.0");


    /* initialize  */
    port_data_init(ptpClock, ptpPort, ptpClock->defaultDS.status.clockIdentity, ptpClock->clock_init->max_foreign_records);

    T_N("sync message interval: %d", PTP_LOG_TIMEOUT(ptpPort->port_config->logSyncInterval));
    T_N("portIdentity: %s, %d",
        ClockIdentityToString(ptpPort->portDS.status.portIdentity.clockIdentity, str),ptpPort->portDS.status.portIdentity.portNumber);

    if (ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT)
        vtss_ptp_state_set(VTSS_APPL_PTP_P2P_TRANSPARENT, ptpClock, ptpPort);
    else if (ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT)
        vtss_ptp_state_set(VTSS_APPL_PTP_E2E_TRANSPARENT, ptpClock, ptpPort);
    else if (ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_BC_FRONTEND && !ptpPort->port_config->portInternal)
        vtss_ptp_state_set(VTSS_APPL_PTP_FRONTEND, ptpClock, ptpPort);
    else {
        /* initialize encapsulation */
        vtss_ptp_state_set(VTSS_APPL_PTP_LISTENING, ptpClock, ptpPort);
    }
}

/* handle actions and events for 'port_state' */
void vtss_ptp_tick(ptp_clock_t *ptpClock)
{
    int i;
    PtpPort_t *ptpPort;
    if (ptpClock->clock_init->cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
        for (i = 0; i < ptpClock->defaultDS.status.numberPorts; i++) {
            ptpPort =&ptpClock->ptpPort[i];
            if (ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED) {
                T_N("vtss_ptp_tick: port %d",i+1);
                T_N("doState: state = %d",ptpPort->portDS.status.portState);

                switch (ptpPort->portDS.status.portState) {
                case VTSS_APPL_PTP_LISTENING:
                    if (ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_BC_FRONTEND && ptpPort->port_config->portInternal) {
                        /* the internal port in the BC frontend always act as a 'slave' port */
                        vtss_ptp_state_set(VTSS_APPL_PTP_UNCALIBRATED, ptpClock, ptpPort);
                    }
                    break;
                case VTSS_APPL_PTP_PASSIVE:
                case VTSS_APPL_PTP_UNCALIBRATED:
                case VTSS_APPL_PTP_SLAVE:
                    break;
                case VTSS_APPL_PTP_INITIALIZING:
                    ptp_port_initialize(ptpClock, ptpPort);
                    break;
                case VTSS_APPL_PTP_FAULTY:
                    /* troubleshooting tbd. */
                    T_N("event FAULT_CLEARED");
                    vtss_ptp_state_set(VTSS_APPL_PTP_INITIALIZING, ptpClock, ptpPort);
                    break;
                case VTSS_APPL_PTP_MASTER:
                    if ((ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY) || ptpClock->defaultDS.status.clockQuality.clockClass == 255)
                        vtss_ptp_state_set(VTSS_APPL_PTP_LISTENING, ptpClock, ptpPort);
                    break;
                default:
                    T_N("unrecognized state");
                    break;
                }
            }
        }
    }
}

/* handle bmc recommended state from the BMCA */
void vtss_ptp_recommended_state(vtss_ptp_bmc_recommended_state_t rec_state, ptp_clock_t *ptpClock, PtpPort_t *ptpPort)
{
    int slaveIndex = -1;
    int old_master;
    char str1[40];
    u8 new_state = ptpPort->portDS.status.portState;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_STATE,"Port %d current state %s, recommended state %d", ptpPort->portDS.status.portIdentity.portNumber,
         PortStateToString(ptpPort->portDS.status.portState), rec_state);
    switch (rec_state) {
        case VTSS_PTP_BMC_UNCHANGED:
            /* no change */
            break;
        case VTSS_PTP_BMC_MASTER_M1:
        case VTSS_PTP_BMC_MASTER_M2:
        case VTSS_PTP_BMC_MASTER_M3:
            if (ptpPort->virtual_port) {
                    if (ptpPort->portDS.status.portState == VTSS_APPL_PTP_DISABLED) {
                        new_state = VTSS_APPL_PTP_DISABLED;
                    } else {
                        new_state = ptpPort->number_foreign_records ? VTSS_APPL_PTP_PASSIVE : VTSS_APPL_PTP_LISTENING;
                    }
            } else if (ptpPort->portDS.status.portState != VTSS_APPL_PTP_INITIALIZING &&  ptpPort->portDS.status.portState != VTSS_APPL_PTP_FAULTY &&
                    ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED) {
                if ((ptpClock->clock_init->cfg.deviceType != VTSS_APPL_PTP_DEVICE_SLAVE_ONLY) && ptpClock->defaultDS.status.clockQuality.clockClass != 255) {
                    new_state = VTSS_APPL_PTP_MASTER;
                } else {
                    new_state = VTSS_APPL_PTP_LISTENING;
                }
            }
            break;
        case VTSS_PTP_BMC_UNCALIBRATED:
            new_state = VTSS_APPL_PTP_UNCALIBRATED;
            break;
        case VTSS_PTP_BMC_SLAVE:
            new_state = VTSS_APPL_PTP_UNCALIBRATED;
//            new_state = VTSS_APPL_PTP_SLAVE;
            break;
        case VTSS_PTP_BMC_PASSIVE:
            new_state = VTSS_APPL_PTP_PASSIVE;
            break;
    }
    if (ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI && !ptpPort->virtual_port) {
        if (new_state == VTSS_APPL_PTP_UNCALIBRATED) {
            slaveIndex = slaveTableEntryFindClockId(ptpClock->slave, &ptpClock->parentDS.parentPortIdentity);
        } else if (new_state == VTSS_PTP_BMC_PASSIVE && ptpClock->ssm.virtual_port_select) {
            if (ptpPort->number_foreign_records) {
                slaveIndex = slaveTableEntryFindClockId(ptpClock->slave, &ptpPort->foreignMasterDS[ptpPort->foreign_record_best].ds.sourcePortIdentity);
            }
        }
        /* Find new master in slavetable */
        if (slaveIndex < 0) {
            if (rec_state != VTSS_PTP_BMC_UNCHANGED) {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_STATE, "Master not found in Unicast slave table %s, %d",
                    ClockIdentityToString(ptpClock->parentDS.parentPortIdentity.clockIdentity, str1), ptpClock->parentDS.parentPortIdentity.portNumber);
            }
        } else {
            old_master = ptpClock->selected_master;
            ptpClock->selected_master = slaveIndex;
            if (old_master != slaveIndex && old_master != 0xffff) {
                vtss_ptp_unicast_slave_conf_upd(ptpClock, old_master);
            }
            T_DG(VTSS_TRACE_GRP_PTP_BASE_STATE, "New master selection: slavetableindex = %d, selected Master index %d", slaveIndex, ptpClock->selected_master);
            vtss_ptp_unicast_slave_conf_upd(ptpClock, slaveIndex);
        }
    }
    if (new_state != ptpPort->portDS.status.portState)
        vtss_ptp_state_set(new_state, ptpClock, ptpPort);
}

/* perform actions required when leaving 'port_state' and entering 'state' */
void vtss_ptp_state_set(u8 state, ptp_clock_t *ptpClock, PtpPort_t *ptpPort)
{
    vtss_appl_ptp_protocol_adr_t *ptp_dest;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_STATE,"Port %d current state %s, new state %s", ptpPort->portDS.status.portIdentity.portNumber,
        PortStateToString(ptpPort->portDS.status.portState), PortStateToString(state));

    /* leaving state tasks */
    switch (ptpPort->portDS.status.portState) {
    case VTSS_APPL_PTP_MASTER:
        vtss_ptp_master_delete(&ptpPort->msm);
        vtss_ptp_announce_delete(&ptpPort->ansm);
        if (ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
            for (int i = 0; i < MAX_UNICAST_SLAVES_PR_MASTER; i++) {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_STATE, "mstr port %d portPort %d", ptpClock->master[i].port, ptpPort->portDS.status.portIdentity.portNumber);
                if (ptpClock->master[i].port == ptpPort->portDS.status.portIdentity.portNumber) {
                    vtss_ptp_unicast_master_delete(&ptpClock->master[i]);
                }
            }
        }
        break;

    case VTSS_APPL_PTP_DISABLED:
        // update current message intervals with configured intervals
        T_IG(VTSS_TRACE_GRP_PTP_BASE_STATE,"port %d leaving DISABLED state", ptpPort->portDS.status.portIdentity.portNumber);
        ptp_802_1as_update_current_message_interval(ptpPort);
        break;

    case VTSS_APPL_PTP_LISTENING:
        if (state == VTSS_APPL_PTP_MASTER || state == VTSS_APPL_PTP_UNCALIBRATED || state == VTSS_APPL_PTP_PASSIVE) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_STATE,"leaving LISTENING and entering MASTER or UNCALIBRATED or PASSIVE");
            if (!ptpPort->virtual_port) {
                if (ptpClock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpClock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                    // Configure computeNeighborRateRatio, computeMeanLinkDelay before sending them in message interval request
                    if (ptpPort->port_config->delayMechanism != VTSS_APPL_PTP_DELAY_MECH_COMMON_P2P) {
                        vtss_ptp_set_comp_ratio_pdelay(ptpPort, 0, false, NULL, false);
                    }
                    //send Message Interval Request
                    ptp_802_1as_send_message_interval_request(ptpPort, ptpClock->clock_init->cfg.twoStepFlag);
                } else {
                    //update current message intervals
                    ptp_802_1as_update_current_message_interval(ptpPort);
                }
            }
            ptpPort->portDS.status.portState = VTSS_APPL_PTP_LISTENING;
            vtss_ptp_bmca_state(ptpPort, true);
        }
        break;
    case VTSS_APPL_PTP_SLAVE:
    case VTSS_APPL_PTP_UNCALIBRATED:
        if (state != VTSS_APPL_PTP_SLAVE && state != VTSS_APPL_PTP_UNCALIBRATED && state != VTSS_APPL_PTP_PASSIVE) {
            if (ptpClock->slavePort == ptpPort->portDS.status.portIdentity.portNumber || !ptpClock->slavePort) {
                ptpClock->ssm.slave_port = NULL;
                vtss_ptp_slave_init(&ptpClock->ssm, &ptpClock->ptp_primary, get_tag_conf(ptpClock, ptpPort));
                T_DG(VTSS_TRACE_GRP_PTP_BASE_STATE,"Instance %d, ptpClock %p, ssm.clock %p, slave_port %p", ptpClock->localClockId, ptpClock, ptpClock->ssm.clock, ptpClock->ssm.slave_port);
                vtss_ptp_bmca_m1(ptpClock);
                ptpClock->slavePort = 0;

                if (ptpClock->selected_master != 0xffff) {
                    ptpClock->selected_master = 0xffff;
                    T_I("Slave port (%d) lost connection to master", ptpPort->portDS.status.portIdentity.portNumber);
                }
                T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"Port %d switch from slave mode", ptpPort->portDS.status.portIdentity.portNumber);

                ptpClock->ssm.ptsf_loss_of_announce = true;
                T_D("*** vtss_ptp_state_set ***");
#if defined(VTSS_SW_OPTION_SYNCE)
                vtss_ptp_ptsf_state_set(ptpClock->ssm.localClockId);
#endif
            }
        }
        // update clock class with holdover state
        (void)clock_class_update(&ptpClock->ssm);
        break;

    default:
        break;
    }

    /* entering state tasks */
    switch (state) {
    case VTSS_APPL_PTP_INITIALIZING:
        T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"state VTSS_APPL_PTP_INITIALIZING");
#if defined (VTSS_SW_OPTION_P802_1_AS)
        if(ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            vtss_ptp_802_1as_bmca_state(ptpPort, false);
        }
#endif //(VTSS_SW_OPTION_P802_1_AS)
        vtss_ptp_bmca_state(ptpPort, false);
        ptpPort->portDS.status.portState = VTSS_APPL_PTP_INITIALIZING;
        break;
    case VTSS_APPL_PTP_FAULTY:
        T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"state VTSS_APPL_PTP_FAULTY");
        ptpPort->portDS.status.portState = VTSS_APPL_PTP_FAULTY;
        break;
    case VTSS_APPL_PTP_DISABLED:
        T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"state change to VTSS_APPL_PTP_DISABLED foreign record");
#if defined (VTSS_SW_OPTION_P802_1_AS)
        if(ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            vtss_ptp_802_1as_bmca_state(ptpPort, false);
        }
#endif //(VTSS_SW_OPTION_P802_1_AS)
        vtss_ptp_bmca_state(ptpPort, false);
        ptpPort->number_foreign_records = 0;
        ptpPort->foreign_record_i = 0;
        ptpPort->portDS.status.portState = VTSS_APPL_PTP_DISABLED;
        break;
    case VTSS_APPL_PTP_LISTENING:
        T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"state VTSS_APPL_PTP_LISTENING");
        ptpPort->portDS.status.portState = VTSS_APPL_PTP_LISTENING;
#if defined (VTSS_SW_OPTION_P802_1_AS)
        if(ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            vtss_ptp_802_1as_bmca_state(ptpPort, true);
        }
#endif //(VTSS_SW_OPTION_P802_1_AS)
        vtss_ptp_bmca_state(ptpPort, true);
        break;
    case VTSS_APPL_PTP_MASTER:
        T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"state VTSS_APPL_PTP_MASTER");
        ptpPort->msm.clock = ptpClock;
        ptpPort->msm.ptp_port = ptpPort;
        /* use the link-local (pdelay) dest mac address if configured for link-local */
        ptp_dest = (ptpPort->port_config->dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT) ? &ptpClock->ptp_primary : &ptpClock->ptp_pdelay;
        if (ptpClock->clock_init->cfg.protocol != VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
            ptpPort->msm.sync_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogSyncInterval;
            if (!ptpPort->virtual_port) {
                vtss_ptp_master_create(&ptpPort->msm, ptp_dest, get_tag_conf(ptpClock, ptpPort));
            }
            ptpPort->ansm.clock = ptpClock;
            ptpPort->ansm.ptp_port = ptpPort;
            ptpPort->ansm.ann_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval;
            if (!ptpPort->virtual_port && ptpPort->parent->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                vtss_ptp_announce_create(&ptpPort->ansm, ptp_dest, get_tag_conf(ptpClock, ptpPort));
            }
            vtss_ptp_bmca_state(ptpPort, false);
        } else {
            // For unicast transmission, until sync TLV request is received, master configuration should not be created.
            // When sync TLV request is received, based on change in log interval, master state is created.
            ptpPort->msm.sync_log_msg_period = 127;
        }
        ptpPort->number_foreign_records = 0; /* disqualify foreign masters */
        ptpPort->foreign_record_i = 0;
        ptpPort->portDS.status.portState = VTSS_APPL_PTP_MASTER;
        break;
    case VTSS_APPL_PTP_PASSIVE:
        T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"state VTSS_APPL_PTP_PASSIVE");
        ptpPort->portDS.status.portState = VTSS_APPL_PTP_PASSIVE;
        break;
    case VTSS_APPL_PTP_UNCALIBRATED:
        T_IG(VTSS_TRACE_GRP_PTP_BASE_STATE,"instance %d, state VTSS_APPL_PTP_UNCALIBRATED", ptpClock->localClockId);
        ptpClock->ssm.twoStepFlag = ptpClock->clock_init->cfg.twoStepFlag;
        ptpClock->ssm.protocol = ptpClock->clock_init->cfg.protocol;
        ptpClock->ssm.domainNumber = ptpClock->clock_init->cfg.domainNumber;
        ptpClock->ssm.portIdentity_p = &ptpPort->portDS.status.portIdentity;
        ptpClock->ssm.parent_portIdentity_p = &ptpClock->parentDS.parentPortIdentity;
        ptpClock->ssm.localClockId = ptpClock->localClockId;
        ptpClock->ssm.versionNumber = ptpPort->port_config->versionNumber;
        ptpClock->ssm.port_mask = ptpPort->port_mask;
        ptpClock->ssm.clock = ptpClock;
        ptpClock->ssm.slave_port = ptpPort;
        ptpClock->ssm.two_way = !ptpClock->clock_init->cfg.oneWay;
        /* use the link-local (pdelay) dest mac address if configured for link-local */
        ptp_dest = (ptpPort->port_config->dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT) ? &ptpClock->ptp_primary : &ptpClock->ptp_pdelay;
        if ((ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) && (ptpClock->selected_master < MAX_UNICAST_MASTERS_PR_SLAVE)) {
            ptp_dest = &ptpClock->slave[ptpClock->selected_master].master;
        }
        vtss_ptp_slave_init(&ptpClock->ssm, ptp_dest, get_tag_conf(ptpClock, ptpPort));

        ptpClock->slavePort = ptpPort->portDS.status.portIdentity.portNumber;
        ptpClock->currentDS.delayOk = false;
        vtss_ptp_bmca_state(ptpPort, true);
        T_IG(VTSS_TRACE_GRP_PTP_BASE_STATE, "current port state %d\n", ptpPort->portDS.status.portState);

        if (ptpPort->virtual_port) { // switch to virtual port
            ptpClock->ssm.virtual_port_select = true;
            vtss_ptp_set_1pps_virtual_reference(ptpClock->localClockId, true,
                    ptpPort->portDS.status.portState != VTSS_APPL_PTP_PASSIVE);
        } else if (ptpClock->ssm.virtual_port_select) { // switch to ethernet port from virtual port
            ptpClock->ssm.virtual_port_select = false;
            vtss_ptp_set_1pps_virtual_reference(ptpClock->localClockId, false,
                    ptpPort->portDS.status.portState != VTSS_APPL_PTP_PASSIVE);
        }
        // With unicast encapsulation, there is a chance of port entering uncl without entering master state. Possible in case of not-master option.
        // So, need to clean up announce grants.
        if (ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
            for (int i = 0; i < MAX_UNICAST_SLAVES_PR_MASTER; i++) {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_STATE, "mstr port %d portPort %d", ptpClock->master[i].port, ptpPort->portDS.status.portIdentity.portNumber);
                if (ptpClock->master[i].port == ptpPort->portDS.status.portIdentity.portNumber) {
                    vtss_ptp_unicast_master_delete(&ptpClock->master[i]);
                }
            }
        }
        ptpPort->portDS.status.portState = VTSS_APPL_PTP_UNCALIBRATED;
        break;
    case VTSS_APPL_PTP_SLAVE:
        T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"state PTP_PTP_SLAVE");
        ptpPort->portDS.status.portState = VTSS_APPL_PTP_SLAVE;
        break;
    case VTSS_APPL_PTP_P2P_TRANSPARENT:
        T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"state VTSS_APPL_PTP_P2P_TRANSPARENT");
        ptpPort->portDS.status.portState = VTSS_APPL_PTP_P2P_TRANSPARENT;
        break;
    case VTSS_APPL_PTP_E2E_TRANSPARENT:
        T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"state VTSS_APPL_PTP_E2E_TRANSPARENT");
        ptpPort->portDS.status.portState = VTSS_APPL_PTP_E2E_TRANSPARENT;
        break;
    case VTSS_APPL_PTP_FRONTEND:
        T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"state VTSS_APPL_PTP_FRONTEND");
        ptpPort->portDS.status.portState = VTSS_APPL_PTP_FRONTEND;
        break;
    default:
        T_NG(VTSS_TRACE_GRP_PTP_BASE_STATE,"to unrecognized state");
        break;
    }
    T_IG(VTSS_TRACE_GRP_PTP_BASE_STATE,"Port: %d, state : %s", ptpPort->portDS.status.portIdentity.portNumber,
        PortStateToString(ptpPort->portDS.status.portState));
}

static void ptp_process_virtual_alternate_timestamp(int clock_inst, ptp_clock_t *ptp_clk, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header, bool two_step)
{
    uint32_t domain = ptp_clk->clock_init->cfg.clock_domain;
    mesa_timestamp_t send_time, recv_time;
    u8 *buf = tx_buf->frame + tx_buf->header_length;

    vtss_local_clock_convert_to_time(tx_buf->hw_time, &recv_time, clock_inst);
    if (two_step) {
        ptp_clk->ssm.servo->alt_rcv_ts = recv_time;
    } else {
        vtss_ptp_unpack_timestamp(buf, &send_time);
        ptp_clk->ssm.servo->process_alternate_timestamp(ptp_clk, domain, clock_inst, &send_time, &recv_time, header->correctionField, header->logMessageInterval, false);
    }
}

static void ptp_process_two_step_virtual_alt_timestamp(int clock_inst, ptp_clock_t *ptp_clk, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header)
{
    uint32_t domain = ptp_clk->clock_init->cfg.clock_domain;
    mesa_timestamp_t send_time;
    u8 *buf = tx_buf->frame + tx_buf->header_length;

    vtss_ptp_unpack_timestamp(buf, &send_time);
    ptp_clk->ssm.servo->process_alternate_timestamp(ptp_clk, domain, clock_inst, &send_time, &ptp_clk->ssm.servo->alt_rcv_ts, header->correctionField, header->logMessageInterval, false);
}

static const size_t msg_len[] = {
    /*[PTP_MESSAGE_TYPE_SYNC] = 0*/                   SYNC_PACKET_LENGTH,
    /*[PTP_MESSAGE_TYPE_DELAY_REQ] = 0*/              DELAY_REQ_PACKET_LENGTH,
    /*[PTP_MESSAGE_TYPE_P_DELAY_REQ] = 0*/            P_DELAY_REQ_PACKET_LENGTH,
    /*[PTP_MESSAGE_TYPE_P_DELAY_RESP] = 0*/           P_DELAY_RESP_PACKET_LENGTH,
    /*4 */                                            0,
    /*5 */                                            0,
    /*6 */                                            0,
    /*7 */                                            0,
    /*[PTP_MESSAGE_TYPE_FOLLOWUP] = 0*/               FOLLOW_UP_PACKET_LENGTH,
    /*[PTP_MESSAGE_TYPE_DELAY_RESP] = 0*/             DELAY_RESP_PACKET_LENGTH,
    /*[PTP_MESSAGE_TYPE_P_DELAY_RESP_FOLLOWUP] = 0*/  P_DELAY_RESP_FOLLOW_UP_PACKET_LENGTH,
    /*[PTP_MESSAGE_TYPE_ANNOUNCE] = 0*/               ANNOUNCE_PACKET_LENGTH,
    /*[PTP_MESSAGE_TYPE_SIGNALLING] = 0*/             SIGNALLING_MIN_PACKET_LENGTH,
    /*[PTP_MESSAGE_TYPE_MANAGEMENT] = 0*/             255, /* not supported */
    /*[PTP_MESSAGE_TYPE_ALL_OTHERS] = 0*/             255
};

/* handle received event messages */
bool vtss_ptp_event_rx(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, uint portnum, ptp_tx_buffer_handle_t *buf_handle, vtss_appl_ptp_protocol_adr_t *sender)
{
    MsgHeader header;
    bool forwarded = false;

    if (0 >= portnum || portnum > ptpClock[clock_inst]->defaultDS.status.numberPorts) {
        T_E("invalid portnum");
        return forwarded;
    }
    PtpPort_t *ptpPort = &ptpClock[clock_inst]->ptpPort[portnum-1];

    if (buf_handle->size < HEADER_LENGTH) {
        T_E("message shorter than header length");
        vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock[clock_inst], ptpPort);
        return forwarded;
    }

    vtss_ptp_unpack_header(buf_handle->frame + buf_handle->header_length, &header);

    if (buf_handle->size < msg_len[header.messageType]) {
        T_E("too short message");
        vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock[clock_inst], ptpPort);
        return forwarded;
    }
    T_I("Receipt of event Message\n"
        "   type %d\n"
        "   hw_time " VPRI64d,
        header.messageType,
        buf_handle->hw_time);

    if (header.versionPTP != ptpPort->port_config->versionNumber) {
        T_I("ignore version %d message", header.versionPTP);
        return forwarded;
    }
    if ((header.transportSpecific != MAJOR_SDOID_802_1AS || header.reserved1 != MINOR_SDO_ID) && ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
        T_I("ignore transportSpecific %d message for domain %d", header.transportSpecific, header.domainNumber);
        return forwarded;
    }
    // Announce messages are not used in AED profile, therefore set parentPortIdentity from sync message
    if (header.messageType == PTP_MESSAGE_TYPE_SYNC && ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS && ptpPort->portDS.status.portState == VTSS_APPL_PTP_SLAVE) {
        memcpy(&ptpClock[clock_inst]->parentDS.parentPortIdentity, &header.sourcePortIdentity.clockIdentity, sizeof(vtss_appl_clock_identity));
    }
    // We shall not ignore multicast packets, because Peer delay mackats are always sent as multicast
    //if (ptpClock[clock_inst]->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI && !getFlag(header.flagField[0], PTP_UNICAST_FLAG)) {
    //    T_N("ignore multicast");
    //    return forwarded;
    //}

    if (!PortIdentitycmp(&header.sourcePortIdentity, &ptpPort->portDS.status.portIdentity)) {
        T_D("Received packet from myself");
        return  forwarded;
    }
    if (getFlag(header.flagField[0],PTP_ALTERNATE_MASTER_FLAG)) {
        T_D("Alternate master not implemented");
        return forwarded;
    }

    /* subtract the inbound latency adjustment is done in the platform part */
    bool port_uses_two_step;

    if ((ptpClock[clock_inst]->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) || (ptpClock[clock_inst]->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY)) {
        port_uses_two_step = (ptpClock[clock_inst]->clock_init->cfg.twoStepFlag && !(ptpPort->port_config->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE)) ||
                             (ptpPort->port_config->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE);
    }
    else {
        port_uses_two_step = ptpClock[clock_inst]->clock_init->cfg.twoStepFlag;
    }

    if (ptpClock[clock_inst]->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
        if (header.messageType == PTP_MESSAGE_TYPE_P_DELAY_REQ || header.messageType == PTP_MESSAGE_TYPE_P_DELAY_RESP) {
            //If unicast use the destination mac address instead of the multicast mac
            memcpy(&ptpPort->pDelay.mac, &sender->mac, sizeof(mesa_mac_t));
        }
    }
    switch (header.messageType) {
    case PTP_MESSAGE_TYPE_SYNC:
        ptpPort->port_statistics.rxSyncCount++;
        ptpPort->parent_last_sync_sequence_number = header.sequenceId;
        ptpPort->awaitingFollowUp = true;
        if (port_uses_two_step) {
            /* in 2-step mode the peerMeanPathDelay is always handled in SW */
            if (ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS &&
                ptpClock[clock_inst]->clock_init->cfg.clock_domain >= fast_cap(MESA_CAP_TS_DOMAIN_CNT)) {
                header.correctionField += ptpPort->portDS.status.peerMeanPathDelay * ptpClock[clock_inst]->port_802_1as_sync_sync.rateRatio;
            } else {
                header.correctionField += ptpPort->portDS.status.peerMeanPathDelay;
            }
        }
        switch (ptpPort->portDS.status.portState) {
            case VTSS_APPL_PTP_FAULTY:
            case VTSS_APPL_PTP_INITIALIZING:
            case VTSS_APPL_PTP_DISABLED:
            case VTSS_APPL_PTP_MASTER:
                T_I("ignore");
                return forwarded;
            case VTSS_APPL_PTP_PASSIVE:
                /* TODO: */
                if (!forwarded && ptpClock[clock_inst]->slavePort && ptpClock[clock_inst]->ssm.virtual_port_select) {
                    ptp_process_virtual_alternate_timestamp(clock_inst, ptpClock[clock_inst], buf_handle, &header, port_uses_two_step);
                }
                return forwarded;
            case VTSS_APPL_PTP_UNCALIBRATED:
            case VTSS_APPL_PTP_SLAVE:
                forwarded = vtss_ptp_slave_sync(ptpClock, clock_inst, buf_handle, &header, sender);
#if defined (VTSS_SW_OPTION_P802_1_AS)
                if (!forwarded && (ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS)) {
                    forwarded = vtss_ptp_802_1as_site_syncsync(ptpClock[clock_inst], buf_handle, &header, sender, ptpPort);
                }
#endif //(VTSS_SW_OPTION_P802_1_AS)
                break;
            case VTSS_APPL_PTP_P2P_TRANSPARENT:
            case VTSS_APPL_PTP_E2E_TRANSPARENT:
                forwarded = vtss_ptp_tc_sync(&ptpClock[clock_inst]->tcsm, buf_handle, &header, sender, ptpPort);
                break;
            default:
                break;
        }
        return forwarded;

    case PTP_MESSAGE_TYPE_DELAY_REQ:
        ptpPort->port_statistics.rxDelayRequestCount++;
        switch (ptpPort->portDS.status.portState) {
            case VTSS_APPL_PTP_MASTER:
                if (ptpClock[clock_inst]->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
                    auto slave_index = masterTableEntryFind(ptpClock[clock_inst]->master, sender->ip);
                    if (slave_index >= 0) {
                        forwarded = vtss_ptp_master_delay_req(&ptpClock[clock_inst]->master[slave_index].msm, buf_handle);
                    } else {
                        T_D("Not able to find unicast master");
                    }
                } else {
                    forwarded = vtss_ptp_master_delay_req(&ptpPort->msm, buf_handle);
                }
                break;
            case VTSS_APPL_PTP_E2E_TRANSPARENT:
                forwarded = vtss_ptp_tc_delay_req(&ptpClock[clock_inst]->tcsm, buf_handle, &header, sender, ptpPort);
                break;
            case VTSS_APPL_PTP_SLAVE:
            default:
                T_I("ignore");
                break;
        }
        return forwarded;

    case PTP_MESSAGE_TYPE_P_DELAY_REQ:
        ptpPort->port_statistics.peer_d.rxPdelayRequestCount++;
        return vtss_ptp_peer_delay_req(&ptpPort->pDelay, buf_handle);

    case PTP_MESSAGE_TYPE_P_DELAY_RESP:
        ptpPort->port_statistics.peer_d.rxPdelayResponseCount++;
        return vtss_ptp_peer_delay_resp(&ptpPort->pDelay, buf_handle, &header);
    default:
        T_N("handle: unrecognized message");
        break;
    }
    return  forwarded;
}

/* handle received general messages */
bool vtss_ptp_general_rx(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, uint portnum, ptp_tx_buffer_handle_t *buf_handle, vtss_appl_ptp_protocol_adr_t *sender)
{
    MsgHeader header;
    bool forwarded = false;
    MsgSignalling signalling;
    TLV tlv;
    ssize_t offset;
    ptp_path_trace_t path_sequence;

    if (0 >= portnum || portnum > ptpClock[clock_inst]->defaultDS.status.numberPorts) {
        T_E("invalid portnum");
        return forwarded;
    }
    PtpPort_t *ptpPort = &ptpClock[clock_inst]->ptpPort[portnum-1];

    if (buf_handle->size < HEADER_LENGTH) {
        T_E("message shorter than header length");
        vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock[clock_inst], ptpPort);
        return forwarded;
    }

    vtss_ptp_unpack_header(buf_handle->frame + buf_handle->header_length, &header);

    if (buf_handle->size < msg_len[header.messageType]) {
        T_E("too short message");
        vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock[clock_inst], ptpPort);
        return forwarded;
    }

	if ((ptpClock[clock_inst]->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) ||
        (ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS && header.messageType != PTP_MESSAGE_TYPE_P_DELAY_RESP_FOLLOWUP)) {
		if (!memcmp(&header.sourcePortIdentity.clockIdentity, &ptpPort->portDS.status.portIdentity.clockIdentity, CLOCK_IDENTITY_LENGTH)) {
			T_D("Received packet from myself");
			if (header.messageType == PTP_MESSAGE_TYPE_ANNOUNCE) {
				ptpPort->port_statistics.rxPTPPacketDiscardCount++;
			}
			return  forwarded;
		}
	}

    T_D("Receipt of general Message\n"
        "   type %d",
        header.messageType);

    if (header.versionPTP != ptpPort->port_config->versionNumber) {
        T_W("ignore version %d message", header.versionPTP);
        return forwarded;
    }
    if ((header.transportSpecific != MAJOR_SDOID_802_1AS || header.reserved1 != MINOR_SDO_ID) && ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
        T_I("ignore transportSpecific %d message", header.transportSpecific);
        return forwarded;
    }

    if ((header.domainNumber != ptpClock[clock_inst]->clock_init->cfg.domainNumber && (header.messageType != PTP_MESSAGE_TYPE_P_DELAY_REQ && header.messageType != PTP_MESSAGE_TYPE_P_DELAY_RESP
        && header.messageType != PTP_MESSAGE_TYPE_P_DELAY_RESP_FOLLOWUP)) && (ptpClock[clock_inst]->clock_init->cfg.deviceType != VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT
        && ptpClock[clock_inst]->clock_init->cfg.deviceType != VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT)) {
        T_I("ignore message from subdomain %d", header.domainNumber);
        return forwarded;
    }

    if (header.messageType == PTP_MESSAGE_TYPE_ANNOUNCE && ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
        T_I("ignore announce messages received on device with 802.1as-aed profile");
        return forwarded;
    }

    switch (header.messageType) {
    case PTP_MESSAGE_TYPE_ANNOUNCE:
        ptpPort->port_statistics.rxAnnounceCount++;
        // implement announce receive in all profiles
        // If 802.1AS profile is selected: Implement 802.1AS PortAnnounceReceive state machine as described in [802.1AS rev/D3.0] clause 10.3.10
        // i.e. sender == this node => not qualified (done above for all PTP messages).
        //      stepsRemoved >= 255 => not qualified
        //      this node included in path trace => not qualified.
        memset(&path_sequence, 0, sizeof(path_sequence));
        if (ptpClock[clock_inst]->clock_init->cfg.path_trace_enable) {
            T_D("Check Announce Tlv extension, buffer size = " VPRIz"", buf_handle->size);
            if (buf_handle->size > buf_handle->header_length + ANNOUNCE_PACKET_LENGTH) {
                offset = buf_handle->header_length + ANNOUNCE_PACKET_LENGTH;
                while (VTSS_RC_OK == vtss_ptp_unpack_tlv(buf_handle->frame + offset, buf_handle->size-offset, &tlv)) {
                    T_D("process Announce Tlv extension with type %d and length %d", tlv.tlvType, tlv.lengthField);
                    if (tlv.tlvType == TLV_PATH_TRACE) {
                        if (VTSS_RC_OK != vtss_ptp_tlv_path_trace_process(&tlv, &path_sequence)) {
                            T_W("process Announce Tlv extension failed");
                        }
                    } else {
                        T_W("Unsupported Announce Tlv extension");
                    }
                    offset += TLV_HEADER_SIZE + tlv.lengthField;
                }
                if (vtss_ptp_path_trace_loop_check(ptpClock[clock_inst]->defaultDS.status.clockIdentity, &path_sequence)) {
                    ptpPort->port_statistics.rxPTPPacketDiscardCount++;
                    T_W("Loop detected in the path trace");
                    return forwarded;
                }
            }
        }
        // ignore Announce if stepsRemoved is >=255
        u16 steps_removed;
        vtss_ptp_unpack_steps_removed(buf_handle->frame + buf_handle->header_length, &steps_removed);
        if (steps_removed >= 255) {
            ptpPort->port_statistics.rxPTPPacketDiscardCount++;
            T_I("ignore Announce message with stepsRemoved >= 255");
            return forwarded;
        }
#if defined (VTSS_SW_OPTION_P802_1_AS)
        if (ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
            forwarded = vtss_ptp_port_announce_information(ptpClock[clock_inst], ptpPort, buf_handle, &header, &path_sequence);
        } else {
#else
        {
#endif //(VTSS_SW_OPTION_P802_1_AS)
            switch (ptpPort->portDS.status.portState) {
                case VTSS_APPL_PTP_FAULTY:
                case VTSS_APPL_PTP_INITIALIZING:
                case VTSS_APPL_PTP_DISABLED:
                    T_I("ignore");
                    return forwarded;

                case VTSS_APPL_PTP_SLAVE:
                case VTSS_APPL_PTP_UNCALIBRATED:
                case VTSS_APPL_PTP_PASSIVE:     /* in the PASSIVE state, the parentDS holds the data for the clock that caused the PASSIVE state */
                    forwarded = vtss_ptp_bmca_announce_slave(ptpClock[clock_inst], ptpPort, buf_handle, &header, &path_sequence);

                    break;
                case VTSS_APPL_PTP_P2P_TRANSPARENT:
                case VTSS_APPL_PTP_E2E_TRANSPARENT:
                    forwarded = vtss_ptp_tc_general(&ptpClock[clock_inst]->tcsm, buf_handle, &header, sender, ptpPort);
                    break;
                case VTSS_APPL_PTP_MASTER:
                default:
                    if (ptpClock[clock_inst]->clock_init->cfg.protocol != VTSS_APPL_PTP_PROTOCOL_IP4UNI ||
                            slaveTableEntryFindClockId(ptpClock[clock_inst]->slave, &header.sourcePortIdentity) >= 0) {
                        forwarded = vtss_ptp_bmca_announce_master(ptpClock[clock_inst], ptpPort, buf_handle, &header);
    #ifdef SW_OPTION_IPCLOCK_MODE
                        /* special hancling of IP-CLOCK Announce message */
                        /* IPCLOCK don't support UNICAST_GRANT messages, therefore this code sets the parameters normally done by the GRANT */
                        if (ptpClock[clock_inst]->defaultDS.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY) {
                            master_index = slaveTableEntryFind(ptpClock[clock_inst]->slave, sender->ip);
                            if (master_index >=0) {
                                slave = &ptpClock[clock_inst]->slave[master_index];
                                /* grant Announce: update master's clock id */
                                slave->sourcePortIdentity = header->sourcePortIdentity;
                                slave->master = *sender;//save master's mac and ip
                                T_I("Master source id: %s, %d",ClockIdentityToString (slave->sourcePortIdentity.clockIdentity, buf1), slave->sourcePortIdentity.portNumber);
                                //save the port number connected to the master
                                slave->port = ptpPort->portDS.status.portIdentity.portNumber;
                                // master has accepted
                                if (slave->comm_state == PTP_COMM_STATE_INIT) {
                                    timerStop(UNICAST_SLAVE_REQUEST_TIMER, slave->itimer); /* In the IP-CLOCK mode, request announce are not issued */
                                    slave->comm_state = PTP_COMM_STATE_CONN;
                                }

                            }
                        }
    #endif
                    }
                    break;
            }
        }
        break;
    case PTP_MESSAGE_TYPE_FOLLOWUP:
        ptpPort->port_statistics.rxFollowUpCount++;
        ptpPort->parent_last_follow_up_sequence_number = header.sequenceId;
        ptpPort->awaitingFollowUp = false;
        switch (ptpPort->portDS.status.portState) {
            case VTSS_APPL_PTP_UNCALIBRATED:
            case VTSS_APPL_PTP_SLAVE:
#if defined (VTSS_SW_OPTION_P802_1_AS)
                if (ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS &&
                    ptpPort->parent_last_follow_up_sequence_number != ptpPort->parent_last_sync_sequence_number) {
                    vtss_ptp_state_set(VTSS_APPL_PTP_MASTER, ptpClock[clock_inst], ptpPort);
                }
#endif
                forwarded = vtss_ptp_slave_follow_up(ptpClock, clock_inst, buf_handle, &header, sender);
#if defined (VTSS_SW_OPTION_P802_1_AS)
                if (!forwarded && (ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS)) {
                    forwarded = vtss_ptp_802_1as_site_syncfollow_up(ptpClock[clock_inst], buf_handle, &header, sender, ptpPort);
                }
#endif //(VTSS_SW_OPTION_P802_1_AS)
                break;

            case VTSS_APPL_PTP_P2P_TRANSPARENT:
            case VTSS_APPL_PTP_E2E_TRANSPARENT:
                /* forward follow up message to other ports */
                forwarded = vtss_ptp_tc_follow_up(&ptpClock[clock_inst]->tcsm, buf_handle, &header, sender, ptpPort);
                //forwarded = forwardFollowUp(header, msgIbuf, length, ptpClock,  ptpPort, sender, frm, buffers);
                T_I("FollowUp: forwarding (%d %d)", forwarded, ptpPort->port_statistics.rxFollowUpCount);
                break;
            case VTSS_APPL_PTP_PASSIVE:
                // needed when ptp port is used with virtual port.
                if (ptpPort->parent_last_follow_up_sequence_number == ptpPort->parent_last_sync_sequence_number) {
                    ptp_process_two_step_virtual_alt_timestamp(clock_inst, ptpClock[clock_inst], buf_handle, &header);
                }
                break;
            default:
                T_I("FollowUp: ignore");
                break;
        }
        return forwarded;

    case PTP_MESSAGE_TYPE_DELAY_RESP:
        ptpPort->port_statistics.rxDelayResponseCount++;
        switch (ptpPort->portDS.status.portState) {
            case VTSS_APPL_PTP_UNCALIBRATED:
            case VTSS_APPL_PTP_SLAVE:
                forwarded = vtss_ptp_slave_delay_resp(ptpClock, clock_inst, buf_handle, &header, sender);
                break;
            case VTSS_APPL_PTP_E2E_TRANSPARENT:
                forwarded = vtss_ptp_tc_delay_resp(&ptpClock[clock_inst]->tcsm, buf_handle, &header, sender, ptpPort);
                break;
            default:
                T_I("ignore");
                break;
        }
        return forwarded;

    case PTP_MESSAGE_TYPE_P_DELAY_RESP_FOLLOWUP:
        ptpPort->port_statistics.peer_d.rxPdelayResponseFollowUpCount++;
        if (ptpClock[clock_inst]->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
            //If unicast use the destination mac address instead of the multicast mac
            memcpy(&ptpPort->pDelay.mac, &sender->mac, sizeof(mesa_mac_t));
        }
        return vtss_ptp_peer_delay_resp_follow_up(&ptpPort->pDelay, buf_handle, &header);

    case PTP_MESSAGE_TYPE_MANAGEMENT:
        T_N("management message not supported");
        break;
    case PTP_MESSAGE_TYPE_SIGNALLING:

        switch (ptpPort->portDS.status.portState) {
            case VTSS_APPL_PTP_FAULTY:
            case VTSS_APPL_PTP_INITIALIZING:
            case VTSS_APPL_PTP_DISABLED:
            case VTSS_APPL_PTP_P2P_TRANSPARENT:
            case VTSS_APPL_PTP_E2E_TRANSPARENT:
                T_I("ignore");
                break;
            default:
                vtss_ptp_unpack_signalling(buf_handle->frame + buf_handle->header_length, &signalling);
                offset = 44;
                while (VTSS_RC_OK == vtss_ptp_unpack_tlv(buf_handle->frame + buf_handle->header_length+offset, buf_handle->size-buf_handle->header_length-offset, &tlv)) {
                    if (tlv.tlvType == TLV_ORGANIZATION_EXTENSION || tlv.tlvType == TLV_ORGANIZXATION_EXTENSION_DO_NOT_PROPAGATE) {
                        if (ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                            //process Message Interval Request
                            vtss_ptp_tlv_organization_extension_process(&tlv, ptpPort, NULL, false);
                        } else {
                            T_W("Tlv Organization extension is only supported in profile IEEE_802_1AS");
                        }
                    } else {
                        vtss_ptp_tlv_process(&header, &tlv, ptpClock[clock_inst], ptpPort, sender);
                    }
                    offset += TLV_HEADER_SIZE + tlv.lengthField;
                }
                break;
        }
        break;

    default:
        T_N("unrecognized message");
        break;
    }
    return forwarded;
}

bool vtss_ptp_cmlds_messages(ptp_cmlds_port_ds_s *port_cmlds, ptp_tx_buffer_handle_t *buf_handle)
{
    MsgHeader header;
    bool forwarded = false;
    ssize_t offset;
    MsgSignalling signalling;
    TLV tlv;

    if (buf_handle->size < HEADER_LENGTH) {
        T_E("message shorter than header length");
        return forwarded;
    }

    vtss_ptp_unpack_header(buf_handle->frame + buf_handle->header_length, &header);

    if (buf_handle->size < msg_len[header.messageType]) {
        T_E("too short message");
        return forwarded;
    }
    if (header.versionPTP != port_cmlds->status.peer_d.versionNumber) {
        T_I("Ignore Non CMLDS messages ");
        return forwarded;
    }
    if (!PortIdentitycmp(&header.sourcePortIdentity, &port_cmlds->status.portIdentity)) {
        T_D("Received packet from myself");
        port_cmlds->statistics.rxPTPPacketDiscardCount++;
        return  forwarded;
    }
    switch(header.messageType) {
    case PTP_MESSAGE_TYPE_P_DELAY_REQ:
        port_cmlds->statistics.peer_d.rxPdelayRequestCount++;
        return vtss_ptp_peer_delay_req(&port_cmlds->pDelay, buf_handle);

    case PTP_MESSAGE_TYPE_P_DELAY_RESP:
        port_cmlds->statistics.peer_d.rxPdelayResponseCount++;
        return vtss_ptp_peer_delay_resp(&port_cmlds->pDelay, buf_handle, &header);

    case PTP_MESSAGE_TYPE_P_DELAY_RESP_FOLLOWUP:
        port_cmlds->statistics.peer_d.rxPdelayResponseFollowUpCount++;
        return vtss_ptp_peer_delay_resp_follow_up(&port_cmlds->pDelay, buf_handle, &header);
    case PTP_MESSAGE_TYPE_SIGNALLING:
        vtss_ptp_unpack_signalling(buf_handle->frame + buf_handle->header_length, &signalling);
        offset = 44;
        while (VTSS_RC_OK == vtss_ptp_unpack_tlv(buf_handle->frame + buf_handle->header_length+offset, buf_handle->size-buf_handle->header_length-offset, &tlv)) {
            if (tlv.tlvType == TLV_ORGANIZATION_EXTENSION || tlv.tlvType == TLV_ORGANIZXATION_EXTENSION_DO_NOT_PROPAGATE) {
                //process Message Interval Request
                vtss_ptp_tlv_organization_extension_process(&tlv, NULL, port_cmlds, true);
            }
            offset += TLV_HEADER_SIZE + tlv.lengthField;
        }
        break;
    default:
        break;
    }
    return forwarded;
}

/* handle received T1,T2 from an other protocol: (OAM, 1PPS or other) */
void vtss_non_ptp_slave_t1_t2_rx(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, vtss_ptp_timestamps_t *ts, u8 clock_class, u8 log_repeat_interval, bool virt_port)
{
    ptpClock[clock_inst]->ssm.clock->parentDS.grandmasterClockQuality.clockClass = clock_class;
    ptpClock[clock_inst]->ssm.timeout_cnt = 0;
    (void) ptp_offset_calc(ptpClock, clock_inst, ts->tx_ts, ts->rx_ts, ts->corr, 0, 0, virt_port);
    T_D("SYNC_RECEIPT_TIMER reset");
    vtss_ptp_timer_start(&ptpClock[clock_inst]->ssm.sync_timer, 3*PTP_LOG_TIMEOUT(log_repeat_interval), false);
    // update clock class and frequency traceable parameters
    (void)clock_class_update(&ptpClock[clock_inst]->ssm);
    if (virt_port) {
        ptpClock[clock_inst]->currentDS.delayOk = true;
    }
}

/* handle received T3,T4 from an other protocol: (OAM) */
void vtss_non_ptp_slave_t3_t4_rx(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, vtss_ptp_timestamps_t *ts)
{
    ptp_delay_calc(ptpClock, clock_inst, ts->tx_ts, ts->rx_ts, ts->corr, 0);  // Note: The last parameter to ptp_delay_calc (logMsgIntv) is only relevant
}                                                                       //       with the advanced filter/servo algorithm and has been set to 0.

/* handle simulated Announce from a virtual port */
void vtss_virtual_ptp_announce_rx(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, uint portnum, ClockDataSet *clock_ds, vtss_appl_ptp_clock_timeproperties_ds_t *time_prop)
{
    PtpPort_t *ptpPort = &ptpClock[clock_inst]->ptpPort[portnum-1];

    T_D("Virtual port announce rx, clock_inst %d, portnum %d", clock_inst, portnum);
    if (ptpClock[clock_inst]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
        vtss_ptp_801_1as_bmca_announce_virtual(ptpClock[clock_inst], ptpPort, clock_ds, time_prop);
    } else {
        vtss_ptp_bmca_announce_virtual(ptpClock[clock_inst], ptpPort, clock_ds, time_prop);
    }
}

/* handle timeout from an other protocol: (OAM) */
void vtss_non_ptp_slave_timeout_rx(ptp_clock_t *ptpClock)
{
    T_I("OAM/1_PPS slave timeout");
    (void)clock_class_update(&ptpClock->ssm);
    if (++ptpClock->ssm.timeout_cnt >= 3) {
        T_I("OAM/1_PPS slave timeout");
        ptpClock->ssm.timeout_cnt = 0;
        if (ptpClock->slavePort != 0) {
            vtss_ptp_state_set(VTSS_APPL_PTP_DISABLED, ptpClock, ptpClock->ssm.slave_port);
        } else {
            T_I("No active slave port");
        }
    }
}

/* check slave_port an other protocol: (OAM) */
void *vtss_non_ptp_slave_check_port(ptp_clock_t *ptpClock)
{
    if (ptpClock->slavePort != 0) {
        return ptpClock->ssm.slave_port;
    } else {
        return NULL;
    }
}

/* check slave_port_no for an other protocol: (1PPS) */
u32 vtss_non_ptp_slave_check_port_no(ptp_clock_t *ptpClock)
{
    return ptpClock->slavePort;
}

/* initialize slave for an other protocol: (OAM) */
void vtss_non_ptp_slave_init(ptp_clock_t *ptpClock, int portnum)
{
    if (0 >= portnum || portnum > ptpClock->defaultDS.status.numberPorts) {
        T_E("invalid portnum");
        return;
    }
    PtpPort_t *ptpPort = &ptpClock->ptpPort[portnum-1];

    if (ptpClock->slavePort != portnum) {
        T_I("Enter uncalibrated state for port %d", portnum);
        vtss_ptp_state_set(VTSS_APPL_PTP_UNCALIBRATED, ptpClock, ptpPort);
    }
}

void vtss_ptp_apply_profile_defaults_to_port_ds(vtss_appl_ptp_config_port_ds_t *port_ds, vtss_appl_ptp_profile_t profile)
{
    if (profile == VTSS_APPL_PTP_PROFILE_1588 || profile == VTSS_APPL_PTP_PROFILE_NO_PROFILE) {
        port_ds->logAnnounceInterval = 1;
        port_ds->announceReceiptTimeout = 3;
        port_ds->logSyncInterval = 0;
        port_ds->logMinPdelayReqInterval = 0;
        port_ds->c_802_1as.as2020 = FALSE;
    }
    else if (profile == VTSS_APPL_PTP_PROFILE_G_8265_1) {
        // port_ds->logAnnounceInterval = ?;
        port_ds->announceReceiptTimeout = 2;
        port_ds->logSyncInterval = -6;
        port_ds->logMinPdelayReqInterval = -6;
        port_ds->c_802_1as.as2020 = FALSE;
    }
    else if (profile == VTSS_APPL_PTP_PROFILE_G_8275_1) {
        port_ds->logAnnounceInterval = -3;
        port_ds->announceReceiptTimeout = 3;
        port_ds->logSyncInterval = -4;
        port_ds->logMinPdelayReqInterval = -4;
        port_ds->localPriority = 128;
        port_ds->dest_adr_type = VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT;
        port_ds->masterOnly = FALSE;
        port_ds->notMaster = FALSE;
        port_ds->c_802_1as.as2020 = FALSE;
    }
    else if (profile == VTSS_APPL_PTP_PROFILE_G_8275_2) {
        port_ds->logAnnounceInterval = 0;
        port_ds->announceReceiptTimeout = 3;
        port_ds->logSyncInterval = -3;        //standard allows 1 to 128 pkts/sec
        port_ds->logMinPdelayReqInterval = -3;//standard allows 1 to 128 pkts/sec
        port_ds->localPriority = 128;
        port_ds->c_802_1as.as2020 = FALSE;
    }
    else if (profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
        port_ds->logAnnounceInterval = 0;
        port_ds->announceReceiptTimeout = 3;
        port_ds->logSyncInterval = -3;
        port_ds->logMinPdelayReqInterval = 0;
        port_ds->c_802_1as.initialLogGptpCapableMessageInterval = 3;
        port_ds->localPriority = 128;
        port_ds->dest_adr_type = VTSS_APPL_PTP_PROTOCOL_SELECT_LINK_LOCAL;
        port_ds->delayMechanism = VTSS_APPL_PTP_DELAY_MECH_P2P;
        port_ds->c_802_1as.peer_d.meanLinkDelayThresh = 800LL<<16;
        port_ds->c_802_1as.syncReceiptTimeout = 3;
        port_ds->c_802_1as.as2020 = TRUE;
        port_ds->c_802_1as.gPtpCapableReceiptTimeout = DEFAULT_GPTP_CAPABLE_RECEIPT_TIMEOUT;
        port_ds->c_802_1as.peer_d.allowedLostResponses = DEFAULT_MAX_CONTIGOUS_MISSED_PDELAY_RESPONSE;
        port_ds->c_802_1as.peer_d.allowedFaults = DEFAULT_MAX_PDELAY_ALLOWED_FAULTS;
    }
    else if (profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
        port_ds->aedPortState = VTSS_APPL_PTP_PORT_STATE_AED_MASTER;
        port_ds->c_802_1as.peer_d.operLogPdelayReqInterval = 0;
        port_ds->c_802_1as.initialLogSyncInterval = -3;
        port_ds->c_802_1as.operLogSyncInterval = -3;
    }
}

// Initialize virtual port master state
void vtss_ptp_virtual_port_master_state_init(ptp_clock_t *ptpClock, uint portnum)
{
    if (portnum >= ptpClock->defaultDS.status.numberPorts) {
        T_E("invalid portnum");
        return;
    }
    PtpPort_t *ptpPort = &ptpClock->ptpPort[portnum];
    if (ptpPort->virtual_port) {
        vtss_ptp_state_set(VTSS_APPL_PTP_MASTER, ptpClock, ptpPort);
    }
}

void vtss_ptp_update_vid_pkt_buf(ptp_clock_t *ptpClock, uint32_t port)
{
    PtpPort_t *ptpPort = &ptpClock->ptpPort[port];

    if (ptpPort->p2p_state) {
        vtss_ptp_pdelay_vid_update(&ptpPort->pDelay, get_tag_conf(ptpClock, ptpPort));
    }

}
#if 0
void protocolTest(void)
{
    u16 x;
    u16 y;
    u16 z;
    uint i;
    const u16 X[] = {65535, 0,     1,     32768};
    const u16 Y[] = {65534, 65535, 0,     1    };
    const u16 Z[] = {32767, 1,     32768, 32769};

    for (i = 0; i < sizeof(X)/sizeof(u16); i++) {
        x = X[i];
        y = Y[i];
        z = Z[i];
        if (!SEQUENCE_ID_CHECK(x, y))
            T_W("SequenceIdcheck failed : %d, %d, %d", x,y,SEQUENCE_ID_CHECK(x, y));
        if (SEQUENCE_ID_CHECK(x, z))
            T_W("SequenceIdcheck failed : %d, %d, %d", x,z,SEQUENCE_ID_CHECK(x, z));
    }
}
#endif
