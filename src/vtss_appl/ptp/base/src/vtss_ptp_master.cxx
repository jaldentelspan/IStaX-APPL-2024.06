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
#include "vtss_ptp_master.h"
#include "vtss_ptp_sys_timer.h"
#include "vtss_ptp_pack_unpack.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_ptp_packet_callout.h"
#include "vtss_tod_api.h"
#include "vtss/appl/ptp.h"
#include "vtss_ptp_802_1as.hxx"
#include "vtss_ptp_path_trace.hxx"
#include "ptp_api.h"
#include "misc_api.h"

/* state defined as random numbers for consistency check */
#define MASTER_STATE_PTP_OBJCT_ACTIVE       0x47835672
#define MASTER_STATE_PTP_OBJCT_WAIT_TX_DONE 0x5a3be982
#define MASTER_STATE_PTP_OBJCT_PASSIVE      0x58370465

/* state defined as random numbers for consistency check */
#define ANNOUNCE_STATE_PTP_OBJCT_ACTIVE  0x47835672
#define ANNOUNCE_STATE_PTP_OBJCT_PASSIVE 0x58370465

/*
 * Forward declarations
 */
static void vtss_ptp_master_sync_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time);
static void vtss_ptp_master_sync_timer(vtss_ptp_sys_timer_t *timer, void *m);
static void vtss_ptp_announce_sync_timer(vtss_ptp_sys_timer_t *timer, void *a);

/*
 * private functions
 */
static const char *state2name(u32 state) {
    switch (state) {
        case MASTER_STATE_PTP_OBJCT_ACTIVE:         return "ACTIVE";
        case MASTER_STATE_PTP_OBJCT_WAIT_TX_DONE:   return "WAIT_TX_DONE";
        case MASTER_STATE_PTP_OBJCT_PASSIVE:        return "PASSIVE";
        default:                                    return "invalid state";
    }
}

static void vtss_ptp_announce_ds_init(AnnounceDS *announced)
{
    memset(announced, 0, sizeof(AnnounceDS));
}

static bool vtss_ptp_announce_ds_refresh(const ptp_clock_t *ptp_clock, AnnounceDS *announced)
{
    u8 flags[2];
    vtss_appl_ptp_clock_timeproperties_ds_t time_prop = ptp_clock->timepropertiesDS;
    bool changed = false;
    if (ptp_clock->ssm.slave_port != NULL && (ptp_clock->ssm.slave_port->portDS.status.portState == VTSS_APPL_PTP_SLAVE || ptp_clock->ssm.slave_port->portDS.status.portState == VTSS_APPL_PTP_UNCALIBRATED )) {
        // Setup clock_data from ptp_clock->parentDS and parentStepsRemoved field of ptp_clock
        if (memcmp(announced->grandmaster_identity, ptp_clock->parentDS.grandmasterIdentity, sizeof(vtss_appl_clock_identity))) {
            memcpy(announced->grandmaster_identity, ptp_clock->parentDS.grandmasterIdentity, sizeof(vtss_appl_clock_identity));
            changed = true;
        }
        if (announced->grandmaster_priority1 != ptp_clock->parentDS.grandmasterPriority1) {
            announced->grandmaster_priority1 = ptp_clock->parentDS.grandmasterPriority1;
            changed = true;
        }
        if (memcmp(&announced->grandmaster_clockQuality, &ptp_clock->parentDS.grandmasterClockQuality, sizeof(ptp_clock->parentDS.grandmasterClockQuality))) {
            announced->grandmaster_clockQuality = ptp_clock->parentDS.grandmasterClockQuality;
            changed = true;
        }
        if (announced->grandmaster_priority2 != ptp_clock->parentDS.grandmasterPriority2) {
            announced->grandmaster_priority2 = ptp_clock->parentDS.grandmasterPriority2;
            changed = true;
        }
        if (announced->steps_removed != ptp_clock->currentDS.stepsRemoved) {
            announced->steps_removed = ptp_clock->currentDS.stepsRemoved;
            changed = true;
        }
    } else {

        // Setup clock_data from ptp_clock->defaultDS and stepsRemoved field of ptp_clock->currentDS
        if (memcmp(announced->grandmaster_identity, ptp_clock->defaultDS.status.clockIdentity, sizeof(vtss_appl_clock_identity))) {
            memcpy(announced->grandmaster_identity, ptp_clock->defaultDS.status.clockIdentity, sizeof(vtss_appl_clock_identity));
            changed = true;
        }
        if (announced->grandmaster_priority1 != ptp_clock->clock_init->cfg.priority1) {
            announced->grandmaster_priority1 = ptp_clock->clock_init->cfg.priority1;
            changed = true;
        }
        if (memcmp(&announced->grandmaster_clockQuality, &ptp_clock->announced_clock_quality, sizeof(ptp_clock->parentDS.grandmasterClockQuality))) {
            announced->grandmaster_clockQuality = ptp_clock->announced_clock_quality;
            changed = true;
        }
        if (announced->grandmaster_priority2 != ptp_clock->clock_init->cfg.priority2) {
            announced->grandmaster_priority2 = ptp_clock->clock_init->cfg.priority2;
            changed = true;
        }
        if (announced->steps_removed != 0) {
            announced->steps_removed = 0;
            changed = true;
        }

    }
    if (announced->time_source != time_prop.timeSource) {
        announced->time_source = time_prop.timeSource;
        changed = true;
    }

    if (announced->currentUtcOffset != time_prop.currentUtcOffset) {
        announced->currentUtcOffset = time_prop.currentUtcOffset;
        changed = true;
    }

    /* header part only variable part is packed */
    /* flag field */
    flags[1] =
        (time_prop.leap61 ? PTP_LI_61 : 0) |
        (time_prop.leap59 ? PTP_LI_59 : 0) |
        (time_prop.currentUtcOffsetValid ? PTP_CURRENT_UTC_OFFSET_VALID :0) |
        (time_prop.ptpTimescale ? PTP_PTP_TIMESCALE : 0) |
        (time_prop.timeTraceable ? PTP_TIME_TRACEABLE : 0) |
        (time_prop.frequencyTraceable ? PTP_FREQUENCY_TRACEABLE : 0);
    if (announced->flags[1] != flags[1]) {
        announced->flags[1] = flags[1];
        changed = true;
    }
    return changed;
}

/*
 * create a PTP masterclock instance
 * Allocate packet buffer
 * Initialize encapsulation protocol
 * Initialize PTP header
 * Initialize PTP sync data.
 * save pointer to sequence number
 * start sync timer
 */
void vtss_ptp_master_create(ptp_master_t *master, vtss_appl_ptp_protocol_adr_t *ptp_dest, ptp_tag_conf_t *tag_conf)
{
    mesa_timestamp_t originTimestamp = {0,0,0};
    MsgHeader header;
    bool port_uses_two_step;
    size_t alloc_size;
    if ((master->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) || (master->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_AED_GM) || (master->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY)) {
        port_uses_two_step = (master->clock->clock_init->cfg.twoStepFlag && !(master->ptp_port->port_config->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE)) ||
                             (master->ptp_port->port_config->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE);
    }
    else {
        port_uses_two_step = master->clock->clock_init->cfg.twoStepFlag;
    }
    header.versionPTP = master->ptp_port->port_config->versionNumber;
    header.minorVersionPTP = master->ptp_port->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
    header.messageType = PTP_MESSAGE_TYPE_SYNC;
    header.transportSpecific = master->clock->majorSdoId;
    header.messageLength = SYNC_PACKET_LENGTH;
    header.domainNumber = master->clock->clock_init->cfg.domainNumber;
    header.reserved1 = MINOR_SDO_ID;
    header.flagField[0] = port_uses_two_step ? PTP_TWO_STEP_FLAG : 0;
    header.flagField[0] |= master->clock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI ? PTP_UNICAST_FLAG : 0;
    header.flagField[1] = (master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS && !master->ptp_port->port_config->c_802_1as.as2020) ? PTP_PTP_TIMESCALE : 0;
    header.correctionField = 0LL;
    memcpy(&header.sourcePortIdentity, &master->ptp_port->portDS.status.portIdentity, sizeof(vtss_appl_ptp_port_identity));
    T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"create master for port %d, inst %p", tag_conf->port, master);
    // Giving each port their unique counter id. Adding VTSS_PORTS to avoid conflict with sequenceId on announce messages.
    header.sequenceId = master->ptp_port->portDS.status.portIdentity.portNumber + VTSS_PORTS;
    header.controlField = (master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) ? 0 : PTP_SYNC_MESSAGE;
    header.logMessageInterval = getFlag(header.flagField[0],PTP_UNICAST_FLAG) ? 0x7f : master->sync_log_msg_period;
    master->ptp_port->portDS.status.s_802_1as.syncLocked = TRUE;
    if (master->state != MASTER_STATE_PTP_OBJCT_ACTIVE && master->state != MASTER_STATE_PTP_OBJCT_WAIT_TX_DONE) {
        master->last_sync_event_sequence_number = 0;
        master->state = MASTER_STATE_PTP_OBJCT_ACTIVE;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"state %x", master->state);
        master->sync_buf.size = vtss_1588_packet_tx_alloc(&master->sync_buf.handle, &master->sync_buf.frame, ENCAPSULATION_SIZE + PACKET_SIZE);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"handle %p", master->sync_buf.handle);
        master->sync_buf.hw_time = 0;

        T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"frame %p", master->sync_buf.frame);
        vtss_ptp_timer_init(&master->sync_timer, "master_sync", -1, vtss_ptp_master_sync_timer, master);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"timer log_msg_period: %d", master->sync_log_msg_period);
        if (port_uses_two_step) {
            master->sync_buf.msg_type = VTSS_PTP_MSG_TYPE_2_STEP;
            master->sync_ts_context.cb_ts = vtss_ptp_master_sync_event_transmitted;
            master->sync_ts_context.context = master;
            master->sync_buf.ts_done = &master->sync_ts_context;
            /* prepare followup packet*/
            alloc_size = ENCAPSULATION_SIZE + PACKET_SIZE;
            if (master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                alloc_size += FOLLOW_UP_TLV_LENGTH; // allocate space for Follow_up TLV
            }
            master->follow_up_allocated_size = vtss_1588_packet_tx_alloc(&master->follow_buf.handle, &master->follow_buf.frame, alloc_size);
            T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"follow up handle %p, frame %p", master->follow_buf.handle, master->follow_buf.frame);
            master->follow_buf.header_length = vtss_1588_pack_encap_header(master->follow_buf.frame, NULL, ptp_dest, FOLLOW_UP_PACKET_LENGTH, false, master->clock->localClockId);
            uint32_t header_size;
            if (ptp_dest->ip != 0) {
                master->follow_buf.inj_encap.type = MESA_PACKET_ENCAP_TYPE_IP4;
                header_size = 42;
            } else {
                master->follow_buf.inj_encap.type = MESA_PACKET_ENCAP_TYPE_ETHER;
                header_size = 14;
            }

            if (master->follow_buf.header_length > header_size) {
                master->follow_buf.inj_encap.tag_count = (master->follow_buf.header_length - header_size) / 4;
            } else {
                master->follow_buf.inj_encap.tag_count = 0;
            }
            master->follow_buf.size = master->follow_buf.header_length + FOLLOW_UP_PACKET_LENGTH;
            master->follow_buf.hw_time = 0;
            master->follow_buf.ts_done = NULL;
        } else {
            master->sync_buf.ts_done = NULL;
            master->follow_buf.handle = 0;
        }
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Master already active");
    }

    memcpy(&master->mac, &ptp_dest->mac, sizeof(mesa_mac_t)); /* used to refresh the mac header before each transmission */
    master->tag_conf = *tag_conf;
    master->sync_buf.header_length = vtss_1588_pack_encap_header(master->sync_buf.frame, NULL, ptp_dest, SYNC_PACKET_LENGTH, true, master->clock->localClockId);
    uint32_t header_size;
    if (ptp_dest->ip != 0) {
        master->sync_buf.inj_encap.type = MESA_PACKET_ENCAP_TYPE_IP4;
        header_size = 42;
    } else {
        master->sync_buf.inj_encap.type = MESA_PACKET_ENCAP_TYPE_ETHER;
        header_size = 14;
    }

    if (master->sync_buf.header_length > header_size) {
        master->sync_buf.inj_encap.tag_count = (master->sync_buf.header_length - header_size) / 4;
    } else {
        master->sync_buf.inj_encap.tag_count = 0;
    }

    vtss_1588_tag_get(tag_conf, master->clock->localClockId, &master->sync_buf.tag);
    master->sync_buf.size = master->sync_buf.header_length + SYNC_PACKET_LENGTH;
    vtss_ptp_pack_msg44(master->sync_buf.frame + master->sync_buf.header_length, &header, &originTimestamp);
    if (port_uses_two_step) {
        header.messageType = PTP_MESSAGE_TYPE_FOLLOWUP;
        clearFlag(header.flagField[0], PTP_TWO_STEP_FLAG);
        header.controlField = (master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) ? 0 : PTP_FOLLOWUP_MESSAGE;
        if (master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            header.messageLength += FOLLOW_UP_TLV_LENGTH; // adjust length for Follow_up TLV
        }
        vtss_1588_tag_get(tag_conf, master->clock->localClockId, &master->follow_buf.tag);
        vtss_ptp_pack_msg44(master->follow_buf.frame + master->follow_buf.header_length, &header, &originTimestamp);
    }
    if (!port_uses_two_step && master->clock->clock_init->afi_sync_enable && master->sync_log_msg_period <= 0 && 
    (master->clock->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_IEEE_802_1AS && master->clock->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS)) {
        // use automatic frame injection
        if (master->sync_log_msg_period != 127) {
            master->sync_buf.msg_type = VTSS_PTP_MSG_TYPE_ORG_TIME; /* org timestamp sync packet */
            if (vtss_1588_afi(&master->afi, master->ptp_port->port_mask,
                              &master->sync_buf, master->sync_log_msg_period, TRUE) != VTSS_RC_OK) {
                T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"sync message transmission failed");
            } else {
                master->afi_in_use = true;
                // start timer with 1 sec interval to test if afi_sync_enable has changed
                vtss_ptp_timer_start(&master->sync_timer, PTP_LOG_TIMEOUT(0), true);
                T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"enabled afi sync message");
            }
        }
    } else {
        master->afi_in_use = false;
        if (master->sync_log_msg_period != 127) {
            if ((master->clock->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_IEEE_802_1AS && master->clock->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS) || 
            ((master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS || master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) && 
            (master->clock->slavePort == 0 || (uport2iport(master->clock->slavePort) == ptp_get_virtual_port_number())))) {
                vtss_ptp_timer_start(&master->sync_timer, PTP_LOG_TIMEOUT(master->sync_log_msg_period), true);
                T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"time %d", PTP_LOG_TIMEOUT(master->sync_log_msg_period));
            }
        } else {
            vtss_ptp_timer_start(&master->sync_timer, PTP_LOG_TIMEOUT(0), true);
        }
    }
}
/*
 * delete a PTP masterclock instance
 * free packet buffer(s)
 */
void vtss_ptp_master_delete(ptp_master_t *master)
{
    if (master->state == MASTER_STATE_PTP_OBJCT_ACTIVE ||
        master->state == MASTER_STATE_PTP_OBJCT_WAIT_TX_DONE) {
        if (master->sync_buf.handle) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"buf.handle %p", master->sync_buf.handle);
            vtss_1588_packet_tx_free(&master->sync_buf.handle);
        }

        if (master->follow_buf.handle) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"buf.handle %p", master->follow_buf.handle);
            vtss_1588_packet_tx_free(&master->follow_buf.handle);
        }

        if (master->afi_in_use) {
            // stop automatic frame injection
            if (vtss_1588_afi(&master->afi, master->ptp_port->port_mask,
                              &master->sync_buf, master->sync_log_msg_period, FALSE) != VTSS_RC_OK) {
                T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"sync message afi failed");
            } else {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"stopped afi sync message");
            }

            master->afi_in_use = false;
        }

        vtss_ptp_timer_stop(&master->sync_timer);
        master->state = MASTER_STATE_PTP_OBJCT_PASSIVE;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"delete master");
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Master not active");
    }
}

/*
 * PTP masterclock sync timer
 * update sequence number
 * send packet
 * mark packet buffer as in use
 * increment sequence number.
 * start timer
 */
/*lint -esym(459, vtss_ptp_master_sync_timer) */
void vtss_ptp_master_sync_timer(vtss_ptp_sys_timer_t *timer, void *m)
{
    mesa_timestamp_t origin_time;
    mesa_timeinterval_t corr = 0;
    ptp_master_t *master = (ptp_master_t *)m;
    size_t tlv_size = 0;
    T_NG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Port mask = " VPRI64x ", state %s",master->ptp_port->port_mask, state2name(master->state));
    bool port_uses_two_step;
    if ((master->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) || (master->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_AED_GM) || (master->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY)) {
        port_uses_two_step = (master->clock->clock_init->cfg.twoStepFlag && !(master->ptp_port->port_config->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE)) ||
                             (master->ptp_port->port_config->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE);
    }
    else {
        port_uses_two_step = master->clock->clock_init->cfg.twoStepFlag;
    }
    switch (master->state) {
        case MASTER_STATE_PTP_OBJCT_WAIT_TX_DONE:
            T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Missed Event transmitted event, sequence no = %d, inst %p",master->last_sync_event_sequence_number, master);
            master->state = MASTER_STATE_PTP_OBJCT_ACTIVE;
            /* fall through is needed for not missing successive sync packet transmission. Otherwise, If one followup packet is not sent, then the Sync packet
               to be sent at next interval is missing. On the whole, two sync packets are considered not sent. To avoid this, fall through is needed. */
        case MASTER_STATE_PTP_OBJCT_ACTIVE:
            if (master->ptp_port->linkState) {
                if (master->afi_in_use) {
                    if (!master->clock->clock_init->afi_sync_enable) {
                        // stop AFI and start SW packet injection
                        if (vtss_1588_afi(&master->afi, master->ptp_port->port_mask,
                                          &master->sync_buf, master->sync_log_msg_period, FALSE) != VTSS_RC_OK) {
                            T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"sync message afi failed");
                        } else {
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"stopped afi sync message");
                        }
                        master->afi_in_use = false;
                        // restart timer with the configured packet rate
                        vtss_ptp_timer_start(timer, PTP_LOG_TIMEOUT(master->sync_log_msg_period), true);
                    } else {
                        u32 tx_count;
                        vtss_1588_afi_packet_tx(&master->afi, &tx_count);
                        master->ptp_port->port_statistics.txSyncCount += tx_count;
                    }
                } else {
                    if (master->sync_log_msg_period != 127 && !port_uses_two_step && master->clock->clock_init->afi_sync_enable && 
                    (master->clock->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_IEEE_802_1AS && master->clock->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS)) {
                        // use automatic frame injection
                        master->sync_buf.msg_type = VTSS_PTP_MSG_TYPE_ORG_TIME; /* org timestamp sync packet */
                        if (vtss_1588_afi(&master->afi, master->ptp_port->port_mask,
                                          &master->sync_buf, master->sync_log_msg_period, TRUE) != VTSS_RC_OK) {
                            T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"sync message transmission failed");
                        } else {
                            master->afi_in_use = true;
                            // start timer with 1 sec interval to test if afi_sync_enable has changed
                            vtss_ptp_timer_start(timer, PTP_LOG_TIMEOUT(0), true);
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"enabled afi sync message");
                        }
                    } else {
                        if (master->clock->slavePort == 0  && master->clock->defaultDS.status.s_802_1as.gmCapable) {
                            if (master->syncSlowdown)
                            {
                                if (master->numberSyncTransmissions >= master->ptp_port->port_config->c_802_1as.syncReceiptTimeout)
                                {
                                    master->sync_log_msg_period = master->ptp_port->portDS.status.s_802_1as.currentLogSyncInterval;
                                    master->numberSyncTransmissions = 0;
                                    master->syncSlowdown = FALSE;
                                }
                                else
                                {
                                    master->numberSyncTransmissions++;
                                }
                             }
                             else
                             {
                                 master->numberSyncTransmissions = 0;
                                 master->sync_log_msg_period = master->ptp_port->portDS.status.s_802_1as.currentLogSyncInterval;
                             }
                        }                            
                        if (master->sync_log_msg_period != 127 && (master->clock->defaultDS.status.s_802_1as.gmCapable || 
                        (master->clock->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_IEEE_802_1AS && master->clock->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS))) {
                            uint32_t ts_id;
                            vtss_tod_pack16(++master->last_sync_event_sequence_number,master->sync_buf.frame + master->sync_buf.header_length + PTP_MESSAGE_SEQUENCE_ID_OFFSET);
                            vtss_ptp_pack_correctionfield(master->sync_buf.frame + master->sync_buf.header_length, &corr);
                            // domain number update
                            master->sync_buf.frame[master->sync_buf.header_length + PTP_MESSAGE_DOMAIN_OFFSET] = master->clock->clock_init->cfg.domainNumber;
                            if ((master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) && !port_uses_two_step) {
                                master->sync_buf.size = master->sync_buf.header_length + SYNC_PACKET_LENGTH;
                                //section 11.3.3, 8.4.4 - 802.1AS frames will not have any tag;
                                master->sync_buf.tag.vid = 0;
                                // Append Follow_Up information TLV to sync frame
                                tlv_size = vtss_ptp_tlv_follow_up_tlv_insert(master->sync_buf.frame + master->sync_buf.size,
                                        master->follow_up_allocated_size - (master->sync_buf.header_length + FOLLOW_UP_PACKET_LENGTH),
                                        &master->clock->follow_up_info);
                                if (tlv_size == 0) T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"failed to insert Follow_Up TLV to sync frame");
                                master->sync_buf.size += tlv_size;
                            }
                            if (port_uses_two_step) {
                                master->state = MASTER_STATE_PTP_OBJCT_WAIT_TX_DONE;
                            } else {
                                vtss_1588_org_time_option_get(master->clock->localClockId, master->ptp_port->portDS.status.portIdentity.portNumber, &master->org_time);
                                if (master->org_time) {
                                    master->sync_buf.msg_type = VTSS_PTP_MSG_TYPE_ORG_TIME; /* org timestamp sync packet */
                                } else {
                                    master->sync_buf.msg_type = VTSS_PTP_MSG_TYPE_CORR_FIELD; /* one-step sync packet using correction field update */
                                    vtss_local_clock_time_get(&origin_time, master->clock->localClockId, &master->sync_buf.hw_time);
                                    vtss_ptp_pack_timestamp(master->sync_buf.frame + master->sync_buf.header_length, &origin_time);
                                }
                            }
                            master->sync_buf.frame[master->sync_buf.header_length + PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] = master->sync_log_msg_period;
                            if (master->syncSlowdown){
                                master->sync_buf.frame[master->sync_buf.header_length + PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] =
                                             master->ptp_port->portDS.status.s_802_1as.currentLogSyncInterval;
                            }
                            vtss_1588_pack_eth_header(master->sync_buf.frame, master->mac);
                            if (master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                                //section 11.3.3, 8.4.4 - 802.1AS frames will not have any tag;
                                master->sync_buf.tag.vid = 0;
                            }
                            if (!vtss_1588_tx_msg(master->ptp_port->port_mask, &master->sync_buf, master->clock->localClockId, false, &ts_id)) {
                                T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"sync message transmission failed");
                            } else {
                                T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"sent sync message seq %d", master->last_sync_event_sequence_number);
                                master->ptp_port->port_statistics.txSyncCount++;
                            }
                            vtss_ptp_timer_start(&master->sync_timer, PTP_LOG_TIMEOUT(master->sync_log_msg_period), true);
                        } else {
                            // check once pr. sec if sync_log_msg_period has changed
                            vtss_ptp_timer_start(&master->sync_timer, PTP_LOG_TIMEOUT(0), true);
                        }
                    }
                }
            } else {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"link went down");
            }
            break;
        default:
            T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"invalid state");
            break;
    }
}

/*
 * 2-step sync packet event transmitted. Send followup
 *
 */
/*lint -esym(459, vtss_ptp_master_sync_event_transmitted) */
static void vtss_ptp_master_sync_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time)
{
    char str[50];
    ptp_master_t *master = (ptp_master_t *) context;
    mesa_timestamp_t origin_time;
    size_t tlv_size = 0;
    T_NG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"context %p, portnum %d, tx_time %u", context, portnum, tx_time);
    if (master->state == MASTER_STATE_PTP_OBJCT_WAIT_TX_DONE) {
        /*send followup */
        master->state = MASTER_STATE_PTP_OBJCT_ACTIVE;
        if (master->ptp_port->linkState) {
            uint32_t ts_id;
            vtss_tod_pack16(master->last_sync_event_sequence_number,master->follow_buf.frame + master->follow_buf.header_length + PTP_MESSAGE_SEQUENCE_ID_OFFSET);
            vtss_local_clock_convert_to_time( tx_time, &origin_time, master->clock->localClockId);
            vtss_ptp_pack_timestamp(master->follow_buf.frame + master->follow_buf.header_length, &origin_time);
            master->follow_buf.frame[master->follow_buf.header_length + PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] = master->sync_log_msg_period;
            master->follow_buf.size = master->follow_buf.header_length + FOLLOW_UP_PACKET_LENGTH;
            if (master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || master->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                //section 11.3.3, 8.4.4 - 802.1AS frames will not have any tag;
                master->follow_buf.tag.vid = 0;
                // Append Follow_Up information TLV
                tlv_size = vtss_ptp_tlv_follow_up_tlv_insert(master->follow_buf.frame + master->follow_buf.size,
                           master->follow_up_allocated_size - (master->follow_buf.header_length + FOLLOW_UP_PACKET_LENGTH),
                           &master->clock->follow_up_info);
                if (tlv_size == 0) T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"failed to insert Follow_Up TLV");
                master->follow_buf.size += tlv_size;
            }

            vtss_1588_pack_eth_header(master->follow_buf.frame, master->mac);
            if (!vtss_1588_tx_msg(master->ptp_port->port_mask, &master->follow_buf, master->clock->localClockId, false, &ts_id)) {
                T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Follow_Up message transmission failed");
            } else {
                T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"sent follow_Up seq:%d message, origin_time: %s", master->last_sync_event_sequence_number, TimeStampToString(&origin_time,str));
                master->ptp_port->port_statistics.txFollowUpCount++;
            }
        }
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"unexpected timestamp event, inst %p", master);
    }
}

/*
 * Handle delay request from a slave
 *
 */
bool vtss_ptp_master_delay_req(ptp_master_t *master, ptp_tx_buffer_handle_t *tx_buf)
{
    mesa_timestamp_t receive_timestamp;
    bool forwarded = false;
    u8 *buf;
    char str[50];

    T_NG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Port mask = " VPRI64x ", state %s",master->ptp_port->port_mask, state2name(master->state));
    if (master->ptp_port->port_config->delayMechanism != VTSS_APPL_PTP_DELAY_MECH_P2P) {
        if (master->state == MASTER_STATE_PTP_OBJCT_WAIT_TX_DONE || master->state ==  MASTER_STATE_PTP_OBJCT_ACTIVE) {
            if (master->ptp_port->linkState) {
                uint32_t ts_id;
                /*send delay resp */
                vtss_1588_prepare_tx_buffer(tx_buf, DELAY_RESP_PACKET_LENGTH, false);
                buf = tx_buf->frame + tx_buf->header_length;
                /* update encapsulation header */
                vtss_1588_update_encap_header(tx_buf->frame, ((buf[PTP_MESSAGE_FLAG_FIELD_OFFSET] & PTP_UNICAST_FLAG) != 0), false, DELAY_RESP_PACKET_LENGTH, master->clock->localClockId);
                /* update messageType and length */
                vtss_local_clock_convert_to_time(tx_buf->hw_time,&receive_timestamp, master->clock->localClockId);
                buf[PTP_MESSAGE_MESSAGE_TYPE_OFFSET] &= 0xf0;
                buf[PTP_MESSAGE_MESSAGE_TYPE_OFFSET] |= (PTP_MESSAGE_TYPE_DELAY_RESP & 0x0f);
                vtss_tod_pack16(DELAY_RESP_PACKET_LENGTH, buf + PTP_MESSAGE_MESSAGE_LENGTH_OFFSET); /* messageLength */
                vtss_tod_pack32(0,buf + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET);   /* clear reserved field */
                buf[PTP_MESSAGE_CONTROL_FIELD_OFFSET] = PTP_DELAY_RESP_MESSAGE;  /* control */
                buf[PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] = master->clock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI ? 0x7f : master->ptp_port->port_config->logMinPdelayReqInterval;  /* logMessageInterval */
                vtss_ptp_pack_timestamp(buf, &receive_timestamp);
                /* requestingPortIdentity = received sourcePortIdentity */
                memcpy(&buf[PTP_MESSAGE_REQ_PORT_ID_OFFSET], &buf[PTP_MESSAGE_SOURCE_PORT_ID_OFFSET],
                       sizeof(vtss_appl_ptp_port_identity));
                /* update sourcePortIdentity from static allocated sync buffer*/
                memcpy(&buf[PTP_MESSAGE_SOURCE_PORT_ID_OFFSET],
                       master->sync_buf.frame + tx_buf->header_length + PTP_MESSAGE_SOURCE_PORT_ID_OFFSET,
                       sizeof(vtss_appl_ptp_port_identity));
                tx_buf->size += sizeof(vtss_appl_ptp_port_identity);
                forwarded = true;
                /* insert correct tag info */
                vtss_1588_tag_get(&master->tag_conf, master->clock->localClockId, &tx_buf->tag);
                if (!vtss_1588_tx_msg(master->ptp_port->port_mask, tx_buf, master->clock->localClockId, false, &ts_id)) {
                    T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Delay_Resp message transmission failed");
                } else {
                    ++master->ptp_port->port_statistics.txDelayResponseCount;
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"sent Delay_Resp message, receive_timestamp: %s", TimeStampToString(&receive_timestamp,str));
                }
            }
        } else {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"unexpected delay_Req");
        }
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"delay_Req not expected in P2P mode");
    }
    return forwarded;

}

static const char *ann_state2name(u32 state) {
    switch (state) {
        case ANNOUNCE_STATE_PTP_OBJCT_ACTIVE:  return "ACTIVE";
        case ANNOUNCE_STATE_PTP_OBJCT_PASSIVE: return "PASSIVE";
        default:                               return "invalid state";
    }
}

/*
 * create a PTP master announce transmitter instance
 * Allocate packet buffer
 * Initialize encapsulation protocol
 * Initialize PTP header
 * save pointer to sequence number
 * start announce timer
 */
void vtss_ptp_announce_create(ptp_announce_t *announce, vtss_appl_ptp_protocol_adr_t *ptp_dest, ptp_tag_conf_t *tag_conf)
{
    MsgHeader header;
    size_t alloc_size;
    header.versionPTP = announce->ptp_port->port_config->versionNumber;
    header.minorVersionPTP = announce->ptp_port->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
    header.transportSpecific = announce->clock->majorSdoId;
    header.messageType = PTP_MESSAGE_TYPE_ANNOUNCE;
    header.messageLength = ANNOUNCE_PACKET_LENGTH;
    header.domainNumber = announce->clock->clock_init->cfg.domainNumber;
    header.reserved1 = MINOR_SDO_ID;
    header.flagField[0] = announce->clock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI ? PTP_UNICAST_FLAG : 0;
    header.flagField[1] = 0;
    header.correctionField = 0LL;
    memcpy(&header.sourcePortIdentity, &announce->ptp_port->portDS.status.portIdentity, sizeof(vtss_appl_ptp_port_identity));
    T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"create announce sm for port %d", tag_conf->port);
    header.sequenceId = 0;
    header.controlField = announce->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS ? 0 : PTP_ALL_OTHERS;
    header.logMessageInterval = announce->ann_log_msg_period;

    if (announce->state != ANNOUNCE_STATE_PTP_OBJCT_ACTIVE) {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"state %x", announce->state);
        alloc_size = ENCAPSULATION_SIZE + PACKET_SIZE + 4 + 8 * PTP_PATH_TRACE_MAX_SIZE; // allocate space for Path Trace TLV
        announce->allocated_size = vtss_1588_packet_tx_alloc(&announce->ann_buf.handle, &announce->ann_buf.frame, alloc_size);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"handle %p, allocated_size " VPRIz"", announce->ann_buf.handle, announce->allocated_size);
        announce->ann_buf.hw_time = 0;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"frame %p", announce->ann_buf.frame);

        vtss_ptp_timer_init(&announce->ann_timer, "ann", announce->ptp_port->portDS.status.portIdentity.portNumber, vtss_ptp_announce_sync_timer, announce);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"timer");
        T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"time %d", PTP_LOG_TIMEOUT(header.logMessageInterval));
        announce->ann_buf.ts_done = NULL;
        announce->last_announce_event_sequence_number = 0;
        vtss_ptp_announce_ds_init(&announce->announced);
        announce->afi_in_use = false;
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Master already active");
    }
    memcpy(&announce->mac, &ptp_dest->mac, sizeof(mesa_mac_t)); /* used to refresh the mac header before each transmission */
    announce->ann_buf.header_length = vtss_1588_pack_encap_header(announce->ann_buf.frame, NULL, ptp_dest, ANNOUNCE_PACKET_LENGTH, false, announce->clock->localClockId);
    uint32_t header_size;
    if (ptp_dest->ip != 0) {
        announce->ann_buf.inj_encap.type = MESA_PACKET_ENCAP_TYPE_IP4;
        header_size = 42;
    } else {
        announce->ann_buf.inj_encap.type = MESA_PACKET_ENCAP_TYPE_ETHER;
        header_size = 14;
    }

    if (announce->ann_buf.header_length > header_size) {
        announce->ann_buf.inj_encap.tag_count = (announce->ann_buf.header_length - header_size) / 4;
    } else {
        announce->ann_buf.inj_encap.tag_count = 0;
    }

    vtss_1588_tag_get(tag_conf, announce->clock->localClockId, &announce->ann_buf.tag);
    announce->ann_buf.size = announce->ann_buf.header_length + ANNOUNCE_PACKET_LENGTH;
    vtss_ptp_pack_header(announce->ann_buf.frame + announce->ann_buf.header_length, &header);
    announce->state = ANNOUNCE_STATE_PTP_OBJCT_ACTIVE;
    T_IG_HEX(VTSS_TRACE_GRP_PTP_BASE_MASTER, announce->ann_buf.frame, announce->ann_buf.size);
    vtss_ptp_timer_start(&announce->ann_timer, 1, true);
    announce->afi_refresh = true;
}
/*
 * delete a PTP masterclock instance
 * free packet buffer(s)
 */
void vtss_ptp_announce_delete(ptp_announce_t *announce)
{
    if (announce->state == ANNOUNCE_STATE_PTP_OBJCT_ACTIVE) {
        if (announce->ann_buf.handle) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"buf.handle %p", announce->ann_buf.handle);
            vtss_1588_packet_tx_free(&announce->ann_buf.handle);
        }

        if (announce->afi_in_use) {
            // stop automatic frame injection
            if (vtss_1588_afi(&announce->afi, announce->ptp_port->port_mask, &announce->ann_buf, announce->ann_log_msg_period, FALSE) != VTSS_RC_OK) {
                T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"announce message afi failed");
            } else {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"stopped afi announce message");
            }

            announce->afi_in_use = false;
        }

        vtss_ptp_timer_stop(&announce->ann_timer);
        announce->state = ANNOUNCE_STATE_PTP_OBJCT_PASSIVE;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"delete master");
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Announce SM not active");
    }
}

/*
 * PTP anounce timer
 * update sequence number
 * send packet
 * mark packet buffer as in use
 * increment sequence number.
 * start timer
 */
/*lint -esym(459, vtss_ptp_announce_sync_timer) */
static void vtss_ptp_announce_sync_timer(vtss_ptp_sys_timer_t *timer, void *a)
{
    mesa_timestamp_t origin_time = {0,0,0};
    u64 hw_time;
    size_t tlv_size = 0;
    bool pt_update = false;
    bool log_interval_update = false,ann_interval_update = false;
    ptp_announce_t *announce = (ptp_announce_t *)a;
    int16_t cur_ann_intvl = (announce->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) ? announce->ptp_port->portDS.status.s_802_1as.currentLogAnnounceInterval : announce->ptp_port->port_config->logAnnounceInterval;
    uint32_t ts_id;
    T_NG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Port mask = " VPRI64x ", state %s",announce->ptp_port->port_mask, ann_state2name(announce->state));
    switch (announce->state) {
        case ANNOUNCE_STATE_PTP_OBJCT_ACTIVE:
            (void)clock_class_update(&announce->clock->ssm);
            if (announce->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
                //section 11.3.3, 8.4.4 - 802.1AS frames will not have any tag;
                announce->ann_buf.tag.vid = 0;
            }
            if (announce->ptp_port->linkState) {
                if (announce->announceSlowdown){
                    if (announce->numberAnnounceTransmissions >= announce->ptp_port->port_config->announceReceiptTimeout)
                    {
                        announce->ann_log_msg_period = announce->ptp_port->portDS.status.s_802_1as.currentLogAnnounceInterval;
                        announce->numberAnnounceTransmissions = 0;
                        announce->announceSlowdown = false;
                        ann_interval_update = true;
                    }
                    else
                    {
                        announce->numberAnnounceTransmissions++;
                    }
                }
                else
                {
                    announce->numberAnnounceTransmissions = 0;
                    announce->ann_log_msg_period = cur_ann_intvl;
                }
                log_interval_update = (((int8_t)announce->ann_buf.frame[announce->ann_buf.header_length+PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET]) != (int8_t)cur_ann_intvl);
                if (announce->clock->clock_init->afi_announce_enable) {
                    // use automatic frame injection
                    T_NG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"AFI announce message transmission enabled, path trace enable %d", announce->clock->clock_init->cfg.path_trace_enable);
                    if (announce->clock->clock_init->cfg.path_trace_enable) {
                        pt_update = vtss_ptp_tlv_path_trace_check(&announce->clock->path_trace, &announce->announced_path_trace);
                        if (pt_update) {
                            // path trace has changed
                            announce->announced_path_trace = announce->clock->path_trace;
                        }

                        T_NG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"path trace changed ? %d", pt_update);
                    }

                    if (announce->clock->clock_init->cfg.path_trace_enable != announce->announced_path_trace_enabled) {
                        // configuration has changed
                        pt_update = true;
                        announce->announced_path_trace_enabled = announce->clock->clock_init->cfg.path_trace_enable;
                    }
                    bool af_update = vtss_ptp_announce_ds_refresh(announce->clock, &announce->announced);
                    if (pt_update || af_update || !announce->afi_in_use || announce->afi_refresh || log_interval_update || ann_interval_update) {
                        if (announce->afi_in_use) {
                            // stop automatic frame injection
                            if (vtss_1588_afi(&announce->afi, announce->ptp_port->port_mask,
                                              &announce->ann_buf, announce->ann_log_msg_period, FALSE) != VTSS_RC_OK) {
                                T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"announce message afi failed");
                            } else {
                                T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"stopped afi announce message, pt_update %d, afi_in_use %d, afi_refresh %d", pt_update, announce->afi_in_use, announce->afi_refresh);
                            }

                            announce->afi_in_use = false;
                        }

                        announce->afi_refresh = false;
                        // Giving each port their unique sequence counter id.
                        announce->last_announce_event_sequence_number = announce->ptp_port->portDS.status.portIdentity.portNumber;
                        vtss_ptp_pack_announce(announce->ann_buf.frame + announce->ann_buf.header_length, &origin_time, announce->clock, announce->last_announce_event_sequence_number, &announce->announced);
                        announce->ann_buf.frame[announce->ann_buf.header_length+PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] = (int8_t)cur_ann_intvl;
                        log_interval_update = false;
                        ann_interval_update = false;
                        announce->ann_buf.size = announce->ann_buf.header_length + ANNOUNCE_PACKET_LENGTH;
                        vtss_1588_pack_eth_header(announce->ann_buf.frame, announce->mac);
                        if (announce->clock->clock_init->cfg.path_trace_enable) {
                            tlv_size = sizeof(vtss_appl_clock_identity) * (announce->clock->path_trace.size + 1) + 4;
                            // If frame size exceeds full-duplex IEEE 802.3 media standards do not append Path trace TLV
                            if (tlv_size > IEEE802_VALID_FRAME_LENGTH && announce->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
                                    T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Path trace TLV is not appended due to frame size %lu exceeds IEEE 802.3 valid length of %d",announce->ann_buf.size + tlv_size,IEEE802_VALID_FRAME_LENGTH);
                                    tlv_size = 0;
                            } else {
                                tlv_size = vtss_ptp_tlv_path_trace_insert(announce->ann_buf.frame + announce->ann_buf.header_length + ANNOUNCE_PACKET_LENGTH,
                                                            announce->allocated_size - (announce->ann_buf.header_length + ANNOUNCE_PACKET_LENGTH),
                                                            &announce->clock->path_trace, announce->clock->defaultDS.status.clockIdentity); // insert Path Trace TLV
                                if (tlv_size == 0) T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"failed to insert Path trace TLV");
                            }
                        }
                        vtss_1588_update_encap_header(announce->ann_buf.frame, false, false, ANNOUNCE_PACKET_LENGTH + tlv_size, announce->clock->localClockId);
                        announce->ann_buf.size += tlv_size;
                        // update message length in PTP header
                        vtss_tod_pack16(ANNOUNCE_PACKET_LENGTH + tlv_size, announce->ann_buf.frame + announce->ann_buf.header_length + PTP_MESSAGE_MESSAGE_LENGTH_OFFSET);
                        announce->ann_buf.msg_type = VTSS_PTP_MSG_TYPE_GENERAL; /* general message */
                        if (announce->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
                            if (!vtss_1588_tx_msg(announce->ptp_port->port_mask, &announce->ann_buf, announce->clock->localClockId, false, &ts_id)) {
                                T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"announce message transmission failed");
                            } else {
                                T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"sent announce message");
                                announce->ptp_port->port_statistics.txAnnounceCount++;
                            }
                        }
                        // start automatic frame injection
                        if (vtss_1588_afi(&announce->afi, announce->ptp_port->port_mask,
                                          &announce->ann_buf, announce->ann_log_msg_period, TRUE) != VTSS_RC_OK) {
                            T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"announce message transmission failed");
                        } else {
                            announce->afi_in_use = true;
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"enabled afi announce message");
                        }
                    }

                    if (announce->afi_in_use) {
                        u32 tx_count;
                        vtss_1588_afi_packet_tx(&announce->afi, &tx_count);
                        announce->ptp_port->port_statistics.txAnnounceCount += tx_count;
                    }
                    // check at most once pr sec if the announce contents has changed
                    vtss_ptp_timer_start(&announce->ann_timer, PTP_LOG_TIMEOUT((announce->ann_log_msg_period > 0) ? announce->ann_log_msg_period : 0), true);
                } else {
                    if (announce->afi_in_use) {
                        // stop automatic frame injection
                        if (vtss_1588_afi(&announce->afi, announce->ptp_port->port_mask,
                                          &announce->ann_buf, announce->ann_log_msg_period, FALSE) != VTSS_RC_OK) {
                            T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"announce message afi failed");
                        } else {
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"stopped afi announce message");
                        }
                        announce->afi_in_use = false;
                    }
                    // if announce->ann_log_msg_period == 127 announce packets are not sent (802.1AS requirement)
                    if (announce->ann_log_msg_period != 127) {
                        vtss_local_clock_time_get(&origin_time, announce->clock->localClockId, &hw_time);

                        vtss_ptp_announce_ds_refresh(announce->clock, &announce->announced);
                        vtss_ptp_pack_announce(announce->ann_buf.frame + announce->ann_buf.header_length, &origin_time, announce->clock, ++announce->last_announce_event_sequence_number, &announce->announced);
                        announce->ann_buf.frame[announce->ann_buf.header_length + PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] = announce->ann_log_msg_period;
                        if (announce->announceSlowdown){
                            announce->ann_buf.frame[announce->ann_buf.header_length + PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] = announce->ptp_port->portDS.status.s_802_1as.currentLogAnnounceInterval;
                        }
                        announce->ann_buf.size = announce->ann_buf.header_length + ANNOUNCE_PACKET_LENGTH;
                        vtss_1588_pack_eth_header(announce->ann_buf.frame, announce->mac);
                        if (announce->clock->clock_init->cfg.path_trace_enable) {
                            tlv_size = sizeof(vtss_appl_clock_identity) * (announce->clock->path_trace.size + 1) + 4;
                            // If frame size exceeds full-duplex IEEE 802.3 media standards do not append Path trace TLV
                            if (tlv_size > IEEE802_VALID_FRAME_LENGTH && announce->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
                                    T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"Path trace TLV is not appended due to frame size %lu exceeds IEEE 802.3 valid length of %d",announce->ann_buf.size + tlv_size,IEEE802_VALID_FRAME_LENGTH);
                                    tlv_size = 0;
                            } else {
                                tlv_size = vtss_ptp_tlv_path_trace_insert(announce->ann_buf.frame + announce->ann_buf.header_length + ANNOUNCE_PACKET_LENGTH,
                                                            announce->allocated_size - (announce->ann_buf.header_length + ANNOUNCE_PACKET_LENGTH),
                                                            &announce->clock->path_trace, announce->clock->defaultDS.status.clockIdentity); // insert Path Trace TLV
                                if (tlv_size == 0) T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"failed to insert Path trace TLV");
                            }
                            vtss_1588_update_encap_header(announce->ann_buf.frame, false, false, ANNOUNCE_PACKET_LENGTH + tlv_size, announce->clock->localClockId);
                            announce->ann_buf.size += tlv_size;
                            // update message length in PTP header
                            vtss_tod_pack16(ANNOUNCE_PACKET_LENGTH + tlv_size, announce->ann_buf.frame + announce->ann_buf.header_length + PTP_MESSAGE_MESSAGE_LENGTH_OFFSET);
                        }

                        if (!vtss_1588_tx_msg(announce->ptp_port->port_mask, &announce->ann_buf, announce->clock->localClockId, false, &ts_id)) {
                            T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"announce message transmission failed");
                        } else {
                            T_DG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"sent announce message");
                            announce->ptp_port->port_statistics.txAnnounceCount++;
                        }

                        vtss_ptp_timer_start(&announce->ann_timer, PTP_LOG_TIMEOUT(announce->ann_log_msg_period), true);
                    } else {
                        // check once pr sec if the announce->ann_log_msg_period has changed
                        vtss_ptp_timer_start(&announce->ann_timer, PTP_LOG_TIMEOUT(0), true);
                    }
                }
            }
            break;

        default:
            T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"invalid state");
            break;
    }
}

