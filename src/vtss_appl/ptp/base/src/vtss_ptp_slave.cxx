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
#include "vtss_ptp_slave.h"
#include "vtss_ptp_sys_timer.h"
#include "vtss_ptp_pack_unpack.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_ptp_packet_callout.h"
#include "vtss_tod_api.h"
#include "vtss_ptp_internal_types.h"
#include "vtss/appl/ptp.h"
#include "ptp_api.h"
#include "vtss_ptp_unicast.hxx"
#include "vtss_ptp_802_1as.hxx"
#include <vtss/appl/port.h>
#include "misc_api.h"
#include "conf_api.h"
#include "vtss_ptp_slave.h"
#include "vtss_ptp_802_1as_site_sync.h"

#if defined(VTSS_SW_OPTION_ZLS30387)
#define ZL303XX_OS_LINUX
#include "zl303xx_HoldoverUtils.h"
#include "zl303xx_DeviceSpec.h"
#endif
#if defined(VTSS_SW_OPTION_SYNCE)
#include "synce_ptp_if.h"
#endif

#define RATE_TO_U32_CONVERT_FACTOR (1LL<<41)

/* state defined as random numbers for consistency check */
#define SLAVE_STATE_PTP_OBJCT_ACTIVE  0x47835672
#define SLAVE_STATE_PTP_OBJCT_PASSIVE 0x58370465

/*
 * Forward declarations
 */
static void vtss_ptp_slave_sync_timer(vtss_ptp_sys_timer_t *timer, void *s);
static void vtss_ptp_slave_delay_req_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time);
static void vtss_ptp_settle_sync_timer(vtss_ptp_sys_timer_t *timer, void *s);
static void transmit_delay_req(vtss_ptp_sys_timer_t *timer, void *s);
static void vtss_ptp_slave_log_timeout(vtss_ptp_sys_timer_t *timer, void *s);

/*
 * private functions
 */
static const char *state2name(u32 state) {
    switch (state) {
        case SLAVE_STATE_PTP_OBJCT_ACTIVE:         return "ACTIVE";
        case SLAVE_STATE_PTP_OBJCT_PASSIVE:        return "PASSIVE";
        default:                                   return "invalid state";
    }
}

const char *clock_state_to_string(vtss_slave_clock_state_t s)
{
    switch (s) {
        case VTSS_PTP_SLAVE_CLOCK_FREERUN:        return "FREERUN";
        case VTSS_PTP_SLAVE_CLOCK_F_SETTLING:     return "F_SETTLING";
        case VTSS_PTP_SLAVE_CLOCK_FREQ_LOCK_INIT: return "FREQ_LOCK_INIT";
        case VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING:   return "FREQ_LOCKING";
        case VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED:    return "FREQ_LOCKED";
        case VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING:  return "PHASE_LOCKING";
        case VTSS_PTP_SLAVE_CLOCK_P_SETTLING:     return "P_SETTLING";
        case VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED:   return "PHASE_LOCKED";
        case VTSS_PTP_SLAVE_CLOCK_RECOVERING:     return "RECOVERING";
        default:                                  return "UNKNOWN";
    }
}

/*
 * functions for slave statistics management
 */
void master_to_slave_delay_stati(ptp_slave_t *slave)
{
    if (slave->master_to_slave_delay > slave->statistics.master_to_slave_max) {
        slave->statistics.master_to_slave_max = slave->master_to_slave_delay;
    }
    if (slave->master_to_slave_delay < slave->statistics.master_to_slave_min) {
        slave->statistics.master_to_slave_min = slave->master_to_slave_delay;
    }
    slave->statistics.master_to_slave_mean += slave->master_to_slave_delay;
    slave->statistics.master_to_slave_mean_count++;
    slave->statistics.master_to_slave_cur = slave->master_to_slave_delay;
}

void slave_to_master_delay_stati(ptp_slave_t *slave)
{
    if (slave->slave_to_master_delay > slave->statistics.slave_to_master_max) {
        slave->statistics.slave_to_master_max = slave->slave_to_master_delay;
    }
    if (slave->slave_to_master_delay < slave->statistics.slave_to_master_min) {
        slave->statistics.slave_to_master_min = slave->slave_to_master_delay;
    }
    slave->statistics.slave_to_master_mean += slave->slave_to_master_delay;
    slave->statistics.slave_to_master_mean_count++;
    slave->statistics.slave_to_master_cur = slave->slave_to_master_delay;
    
}

static void slave_statistics_clear(ptp_slave_t *slave)
{
    slave->statistics.master_to_slave_max       = -VTSS_MAX_TIMEINTERVAL;
    slave->statistics.master_to_slave_min       =  VTSS_MAX_TIMEINTERVAL;
    slave->statistics.master_to_slave_mean      = 0LL;
    slave->statistics.master_to_slave_mean_count = 0;
    slave->statistics.master_to_slave_cur       = 0LL;
    slave->statistics.slave_to_master_max       = -VTSS_MAX_TIMEINTERVAL;
    slave->statistics.slave_to_master_min       =  VTSS_MAX_TIMEINTERVAL;
    slave->statistics.slave_to_master_mean      = 0LL;
    slave->statistics.slave_to_master_mean_count = 0;
    slave->statistics.slave_to_master_cur       = 0LL;
    slave->statistics.sync_pack_rx_cnt          = 0;
    slave->statistics.sync_pack_timeout_cnt     = 0;
    slave->statistics.delay_req_pack_tx_cnt     = 0;
    slave->statistics.delay_resp_pack_rx_cnt    = 0;
    slave->statistics.sync_pack_seq_err_cnt     = 0;
    slave->statistics.follow_up_pack_loss_cnt   = 0;
    slave->statistics.delay_resp_seq_err_cnt    = 0;
    slave->statistics.delay_req_not_saved_cnt   = 0;
    slave->statistics.delay_req_intr_not_rcvd_cnt = 0;
}

mesa_rc vtss_ptp_clock_slave_statistics_enable(ptp_clock_t *ptp, bool enable)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (ptp != NULL) {
        ptp_slave_t *slave = &ptp->ssm;
        if (slave != NULL) {
            slave->statistics.enable = enable;
            slave_statistics_clear(slave);
            rc = VTSS_RC_OK;
        }
    }
    return rc;
}

mesa_rc vtss_ptp_clock_slave_statistics_get(ptp_clock_t *ptp, vtss_ptp_slave_statistics_t *statistics, bool clear)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (ptp != NULL) {
        ptp_slave_t *slave = &ptp->ssm;
        if (slave != NULL) {
            *statistics = slave->statistics;
            if (clear) {
                slave_statistics_clear(slave);
            }
            rc = VTSS_RC_OK;
        }
    }
    return rc;
}

/*
 * create a PTP slaveclock instance:
 * clear data and allocate buffer for delayReq transmission
 */
void vtss_ptp_slave_create(ptp_slave_t *slave)
{
    vtss_clear(*slave);
    slave->delay_req_buf.size = vtss_1588_packet_tx_alloc(&slave->delay_req_buf.handle, &slave->delay_req_buf.frame, ENCAPSULATION_SIZE + PACKET_SIZE);
    slave->dly_q = new delay_req_ts_q;
    if (slave->dly_q == NULL) {
        T_EG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "not able to create delay request Q");
    }
    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"handle %p delay_q-handle %p", slave->delay_req_buf.handle, slave->dly_q);
    vtss_ptp_timer_init(&slave->sync_timer, "slave_sync", -1, vtss_ptp_slave_sync_timer, slave);
    slave->clock_settle_time = 2;  /* 8 sec settle time after clock setting */
    slave->in_settling_period = false;
    vtss_ptp_timer_init(&slave->settle_timer,        "slave_settle",        -1, vtss_ptp_settle_sync_timer, slave);
    vtss_ptp_timer_init(&slave->delay_req_sys_timer, "slave_delay_req_sys", -1, transmit_delay_req, slave);
    slave_statistics_clear(slave);
    slave->master_to_slave_delay = 0;
    slave->master_to_slave_delay_valid = false;    
    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"timer");
    slave->ptsf_loss_of_announce = true;
    slave->ptsf_loss_of_sync = true;
    slave->ptsf_unusable = true;
    vtss_ptp_timer_init(&slave->log_timer, "slave_log", -1, vtss_ptp_slave_log_timeout, slave);
    T_D("*** SLAVE CREATE ***");
}

/*
 * create a PTP slaveclock instance
 */
void vtss_ptp_slave_init(ptp_slave_t *slave, vtss_appl_ptp_protocol_adr_t *ptp_dest, ptp_tag_conf_t *tag_conf)
{
    mesa_timestamp_t originTimestamp = {0,0,0};
    MsgHeader header;
    vtss_appl_ptp_config_port_ds_t *slave_config;
    // T_E("*** vtss_ptp_slave_init ***");
    if (slave->slave_port) {
        bool port_uses_two_step;

        slave_config = slave->slave_port->port_config;
        slave->servo->delay_filter_reset(0);  /* clears one-way E2E delay filter */
        slave->clock->currentDS.meanPathDelay = 0;
        slave->last_delay_req_event_sequence_number = 0;
        slave->random_relay_request_timer = (slave->protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET || slave->protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI || slave->protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED || slave->protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED) ? true : false;
        /* DelayReq timer is chosen to ensure delay request after the first Sync packet */
        slave->delay_req_timer = 1;
        slave->first_sync_packet = true;
        slave->ptsf_loss_of_announce = false;  // Note: At this point a master is already nominated and selected.
        slave->ptsf_loss_of_sync = true;
        slave->ptsf_unusable = true;
        slave->prev_psettling = false;
        if (slave->dly_q == NULL) {
            // Should not come here.
            T_EG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "no delay request Q allocated");
            slave->dly_q = new delay_req_ts_q;
            if (slave->dly_q == NULL) {
                T_EG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "could not create delay request Q");
            }
        }
        T_D("*** SLAVE INIT ***");
#if defined(VTSS_SW_OPTION_SYNCE)
        vtss_ptp_ptsf_state_set(slave->localClockId);
#endif
        T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"DelayReq initial timer = %d", slave->delay_req_timer);

        header.versionPTP = slave->versionNumber;
        header.minorVersionPTP = slave_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
        header.messageType = PTP_MESSAGE_TYPE_DELAY_REQ;;
        header.transportSpecific = slave->clock->majorSdoId;
        header.messageLength = DELAY_REQ_PACKET_LENGTH;
        header.domainNumber = slave->domainNumber;
        header.reserved1 = MINOR_SDO_ID;
        header.flagField[0] = (slave->protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI || slave->protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED || slave->protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED) ? PTP_UNICAST_FLAG : 0;
        header.flagField[1] = 0;
        header.correctionField = 0LL;
        memcpy(&header.sourcePortIdentity, slave->portIdentity_p, sizeof(vtss_appl_ptp_port_identity));
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"create slave for port %d", tag_conf->port);
        header.sequenceId = slave->last_delay_req_event_sequence_number;
        header.controlField = (slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) ? 0 : PTP_DELAY_REQ_MESSAGE;
        header.logMessageInterval = 0x7f;

        slave->state = SLAVE_STATE_PTP_OBJCT_ACTIVE;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"state %x", slave->state);
        slave->delay_req_buf.header_length = vtss_1588_pack_encap_header(slave->delay_req_buf.frame, NULL, ptp_dest, DELAY_REQ_PACKET_LENGTH, true, slave->localClockId);
        uint32_t header_size;
        if (ptp_dest->ip != 0) {
            slave->delay_req_buf.inj_encap.type = MESA_PACKET_ENCAP_TYPE_IP4;
            header_size = 42;
        } else {
            slave->delay_req_buf.inj_encap.type = MESA_PACKET_ENCAP_TYPE_ETHER;
            header_size = 14;
        }
        
        if (slave->delay_req_buf.header_length > header_size) {
            slave->delay_req_buf.inj_encap.tag_count = (slave->delay_req_buf.header_length - header_size) / 4;
        } else {
            slave->delay_req_buf.inj_encap.tag_count = 0;
        }
        
        vtss_1588_tag_get(tag_conf, slave->localClockId, &slave->delay_req_buf.tag);
        slave->delay_req_buf.size = slave->delay_req_buf.header_length + DELAY_REQ_PACKET_LENGTH;
        vtss_ptp_pack_msg44(slave->delay_req_buf.frame + slave->delay_req_buf.header_length, &header, &originTimestamp);
        slave->delay_req_buf.hw_time = 0;

        T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"frame %p", slave->delay_req_buf.frame);
        slave->logSyncInterval = slave_config->logSyncInterval;
        slave->sy_to = (slave->logSyncInterval > -2) ? 3*PTP_LOG_TIMEOUT(slave->logSyncInterval) : PTP_LOG_TIMEOUT(0);
        vtss_ptp_timer_start(&slave->sync_timer, slave->sy_to, false);
        slave->logSyncInterval = slave_config->logSyncInterval;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"time %d", slave->sy_to);
        if ((slave->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND)) {
            if ((slave->twoStepFlag && !(slave_config->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE)) ||
                    (slave_config->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE)) {
                port_uses_two_step = true;
            } else {
                port_uses_two_step = false;
            }
        } else {
            port_uses_two_step = slave->twoStepFlag;
        }

        //  Always use 2-step method if clock_domain != 0 since correction field update is not working currently on sparx-v for clk domain 1.
        if (port_uses_two_step || slave->clock->clock_init->cfg.clock_domain) {
            slave->delay_req_buf.msg_type = VTSS_PTP_MSG_TYPE_2_STEP;
            slave->del_req_ts_context.cb_ts = vtss_ptp_slave_delay_req_event_transmitted;
            slave->del_req_ts_context.context = slave;
            slave->delay_req_buf.ts_done = &slave->del_req_ts_context;
        } else {
            slave->delay_req_buf.msg_type = VTSS_PTP_MSG_TYPE_CORR_FIELD; /* one-step sync packet using correction field update */
            slave->delay_req_buf.ts_done = NULL;
        }

        memcpy(&slave->mac, &ptp_dest->mac, sizeof(mesa_mac_addr_t)); /* used to refresh the mac header before each transmission */
        slave->parent_last_sync_sequence_number = 0;
        slave->wait_follow_up  = false;
        slave->statistics.follow_up_pack_loss_cnt = 0;      /* count number of lost followup messages */
        slave->sync_correctionField = 0LL;
        slave->sync_tx_time.seconds = 0; /* used for debugging */
        slave->sync_tx_time.nanoseconds = 0; /* used for debugging */
        slave->sync_tx_time.nanosecondsfrac = 0;
        slave->state = SLAVE_STATE_PTP_OBJCT_ACTIVE;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Instance %d, Slave active", slave->localClockId);
        if (slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED || slave->clock_state == VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED) {
            slave->clock_state = VTSS_PTP_SLAVE_CLOCK_RECOVERING;
        } else if (slave->clock_state != VTSS_PTP_SLAVE_CLOCK_RECOVERING) {
            slave->clock_state = VTSS_PTP_SLAVE_CLOCK_FREERUN;
        }
        slave->old_clock_state = slave->clock_state;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Instance %d, clock_state %s. Reason: Slave initialized", slave->localClockId, clock_state_to_string(slave->clock_state));
        // Update 802.1as siteSync related structures.
        if (slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            vtss_ptp_802_1as_site_sync_update(slave->clock, (uport2iport(slave->clock->slavePort) == ptp_get_virtual_port_number()) ? false : true);
        }

        // Clear the contents of delay request Queue.
        slave->dly_q->clear();
    } else {
        slave->state = SLAVE_STATE_PTP_OBJCT_PASSIVE;
        vtss_ptp_timer_stop(&slave->sync_timer);
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Instance %d, Slave passive", slave->localClockId);
        slave->servo->clock_servo_reset(SET_VCXO_FREQ);
        // Update 802.1as siteSync related structures.
        if (slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            vtss_ptp_802_1as_site_sync_update(slave->clock, false);
        }
    }
}

/*
 * delete a PTP slaveclock instance
 * free packet buffer(s)
 */
void vtss_ptp_slave_delete(ptp_slave_t *slave)
{
    if (slave->state == SLAVE_STATE_PTP_OBJCT_ACTIVE) {
        if (slave->delay_req_buf.handle) {
            T_WG(VTSS_TRACE_GRP_PTP_BASE_MASTER,"buf.handle %p", slave->delay_req_buf.handle);
            vtss_1588_packet_tx_free(&slave->delay_req_buf.handle);
        }
        // Delete delay request Queue.
        if (slave->dly_q) {
            delete(slave->dly_q);
        }

        vtss_ptp_timer_stop(&slave->sync_timer);
        slave->state = SLAVE_STATE_PTP_OBJCT_PASSIVE;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"delete slave");

        slave->ptsf_loss_of_announce = true;
        slave->ptsf_loss_of_sync = true;
        slave->ptsf_unusable = true;
        T_D("*** SLAVE DELETE ***");
#if defined(VTSS_SW_OPTION_SYNCE)
        vtss_ptp_ptsf_state_set(slave->localClockId);
#endif
    } else {
        T_WG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Slave not active");
    }
}


/*
 * PTP slaveclock sync timout
 */
/*lint -esym(459, vtss_ptp_slave_sync_timer) */
static void vtss_ptp_slave_sync_timer(vtss_ptp_sys_timer_t *timer, void *s)
{
    ptp_slave_t *slave = (ptp_slave_t *)s;
    vtss_ptp_servo_status_t servo_status;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"state %s, localClockId %d", state2name(slave->state), slave->localClockId);
    if (slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Timeout is handled elswhere in the 802.1AS profile");
        return;
    }
    switch (slave->state) {
        case SLAVE_STATE_PTP_OBJCT_ACTIVE:
            ++slave->statistics.sync_pack_timeout_cnt;
            if (slave->slave_port != NULL) {
                vtss_ptp_timer_start(timer, slave->sy_to, false);
                slave->servo->clock_servo_status(slave->clock->clock_init->cfg.clock_domain, &servo_status);
                if ((slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED || slave->clock_state == VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED) && servo_status.holdover_ok) {
                    slave->clock_state = VTSS_PTP_SLAVE_CLOCK_RECOVERING;
                } else if (slave->clock_state != VTSS_PTP_SLAVE_CLOCK_RECOVERING){
                    slave->clock_state = VTSS_PTP_SLAVE_CLOCK_FREERUN;
                }
                slave->servo->clock_servo_reset(SET_VCXO_FREQ);
            } else {
                vtss_ptp_timer_start(timer, PTP_LOG_TIMEOUT(0), false);
                T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. holdover_trace_count %d", clock_state_to_string (slave->clock_state), slave->g8275_holdover_trace_count);

            }
            (void)clock_class_update(slave);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: Sync timeout", clock_state_to_string (slave->clock_state));
            break;
        default:
            if (clock_class_update(slave)) {
                vtss_ptp_timer_start(timer, PTP_LOG_TIMEOUT(0), false);
            }
            T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Sync timeout in a Passive slave");
            break;
    }
    slave->ptsf_loss_of_sync = true;
    T_D("*** SLAVE SYNC TIMER ***");
#if defined(VTSS_SW_OPTION_SYNCE)
    vtss_ptp_ptsf_state_set(slave->localClockId);
#endif
}


/*
 * 2-step delayReq packet event transmitted. Save tx time
 *
 */
/*lint -esym(459, vtss_ptp_slave_delay_req_event_transmitted) */
// PHY API provides seq-id as signature but switch API does not provide seq-id as signature.
// So, the FIFO slot id(ts-id) in switch is used for matching timestamp entries.
// Same FIFO slot id would be used by multiple packets.So, matching the timestamp ids
// should start with most recent packet transmitted.
static void vtss_ptp_slave_delay_req_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time)
{
    ptp_slave_t *slave = (ptp_slave_t *)context;
    delay_req_ts_q_itr_t itr = slave->dly_q->end();
    delay_req_ts_q_itr_t itr_begin = slave->dly_q->begin();
    bool found = false;
    ptp_ts_entry_t ts;

    if (!slave->dly_q->empty()) {
        do {
            --itr;
            T_RG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Q seq-id %u ts_id %d intr ts_id %d", (*itr).seq_id, (*itr).ts_id, ts_id);
            if ((*itr).ts_id == ts_id) {
                vtss_local_clock_convert_to_time(tx_time, &(*itr).tx_time, slave->clock->localClockId);
                found = true;
                (*itr).tx_valid = true;
                break;
            }
        } while (itr != itr_begin);
    }

    if (found) {
        if ((*itr).rx_valid) {
            if (slave->clock_state != VTSS_PTP_SLAVE_CLOCK_F_SETTLING &&
                slave->clock_state != VTSS_PTP_SLAVE_CLOCK_P_SETTLING) {
                T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "delay calculation after receiving intr");
                ptp_delay_calc(*slave->clockBase, slave->clock->localClockId, (*itr).tx_time, (*itr).rx_time, (*itr).corr, slave->logDelayReqInterval);
            }
            // Pop entry since both tx, rx timestamps are valid.
            while (!slave->dly_q->empty()) {
                slave->dly_q->pop(ts);
                if (ts.ts_id == ts_id) {
                    break;
                } else {
                    if (!ts.tx_valid) {
                        slave->statistics.delay_req_intr_not_rcvd_cnt++;
                    } else if (!ts.rx_valid) {
                        slave->statistics.delay_resp_seq_err_cnt++;
                    }
                }
            }
        }
    }
}

// This function emulates ingress timestamping for correction field update in phys using phy TC mode A or TC mode C in delay request message.
// pkt buffer is updated using inputs.
void delay_req_phy_corr_field_update(ptp_tx_buffer_handle_t *buf, mesa_timestamp_t *tx_time, vtss_ptp_phy_corr_type_t phy_corr)
{
    int64_t cor_upd;
    mesa_timeinterval_t corr = 0;
    char str[50];

    switch (phy_corr) {
        case PHY_CORR_GEN_2A:
            // phy TC mode A equivalent to switch tc mode 0.
            vtss_tod_pack32(tx_time->nanoseconds, buf->frame + buf->header_length + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET);
            vtss_ptp_pack_correctionfield(buf->frame + buf->header_length, &corr);
            break;
        case PHY_CORR_GEN_3:
            // In Lan8814 Rev B due to chip bug, TC mode C is in modified format as calculated below.
            cor_upd = ((int64_t)(tx_time->seconds & 0x3FFFF) * 1000000000LL) + tx_time->nanoseconds;
            corr = -(cor_upd << 16);
            vtss_ptp_pack_correctionfield(buf->frame + buf->header_length, &corr);
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, " delay_req correction %s sec %u nano-sec %u", vtss_tod_TimeInterval_To_String (&corr, str,0), tx_time->seconds, tx_time->nanoseconds);
            break;
        case PHY_CORR_GEN_3C:
            // phy TC mode C equivalent to switch tc mode 3.
            cor_upd = ((int64_t)(tx_time->seconds * 1000000000LL)) + tx_time->nanoseconds;
            corr = -(cor_upd << 16);
            vtss_ptp_pack_correctionfield(buf->frame + buf->header_length, &corr);
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, " delay_req correction %s", vtss_tod_TimeInterval_To_String (&corr, str,0));
            break;
        default:
            break;
    }
}

/*
 * Transmit delay_Req
 *
 */
/*lint -esym(459, transmit_delay_req) */
static void transmit_delay_req(vtss_ptp_sys_timer_t *timer, void *s)
{
    ptp_slave_t *slave = (ptp_slave_t *)s;
    i32 delay_req_period;
    i8 shifts;
    vtss_ptp_phy_corr_type_t phy_corr = {};
    ptp_ts_entry_t dly_req = {};
    bool two_step = false;
    uint32_t clk_domain = slave->clock->clock_init->cfg.clock_domain;
    
    T_RG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"transmit_delay_req");
    if ((slave->slave_port != NULL) &&
        (slave->clock->slavePort <= slave->clock->clock_init->numberEtherPorts)) {
        delay_req_ts_q_itr_t itr;

        // Prepare the delay request entry details for sending.
        // sequence-id
        dly_req.seq_id = dly_req.ts_id = ++slave->last_delay_req_event_sequence_number;
        vtss_tod_pack16(dly_req.seq_id, slave->delay_req_buf.frame + slave->delay_req_buf.header_length + PTP_MESSAGE_SEQUENCE_ID_OFFSET);

        // correction field update is done only for 1-step clock using tx time from LTC.
        // For 2-step clock, tx time is filled from interrupt handler.
        two_step = ((slave->twoStepFlag && (slave->slave_port->port_config->twoStepOverride != VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE)) ||
                   (slave->slave_port->port_config->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE));
        // Note: correction field update using switch timestamping does not work with clock domain 1. Hence, delay request
        //       timestamp is saved in FIFO for 1-step clock also in clock domain 1.
        if (!two_step && !clk_domain) {
            // 1-step clock.
            vtss_local_clock_time_get(&dly_req.tx_time, slave->localClockId, &slave->delay_req_buf.hw_time);
            vtss_ptp_pack_timestamp(slave->delay_req_buf.frame + slave->delay_req_buf.header_length, &dly_req.tx_time);
            dly_req.tx_valid = true;
            // phy timestamping works only in clk domain 0
            if (!clk_domain) {
                phy_corr = vtss_port_phy_delay_corr_upd(slave->slave_port->portDS.status.portIdentity.portNumber);
            }
            if (phy_corr) {
                // Update buffer for using phy correction field update.
                delay_req_phy_corr_field_update(&slave->delay_req_buf, &dly_req.tx_time, phy_corr);
            } else {
                // Clear correction field because the buffer is reused.
                // TBD: remove by moving this to slave_init.
                vtss_ptp_pack_correctionfield(slave->delay_req_buf.frame + slave->delay_req_buf.header_length, &dly_req.corr);
            }
        }

        // Configure encapsulation header.
        if (slave->protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED) {
            vtss_1588_pack_eth_header(slave->delay_req_buf.frame, slave->sender.mac);
        } else if (slave->protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI || slave->protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED) {
            (void)vtss_1588_pack_encap_header(slave->delay_req_buf.frame, NULL, &slave->sender, DELAY_REQ_PACKET_LENGTH, true, slave->localClockId);
        } else {
            vtss_1588_pack_eth_header(slave->delay_req_buf.frame, slave->mac);
        }

        // Push the delay request entry on to Q for acknowledgement(delay response).
        // When corresponding delay response message is received, this delay request entry is removed from Q for delay calculation.
        if (!slave->dly_q->push(dly_req)) {
            // Buffer overwritten.
            slave->statistics.delay_req_not_saved_cnt++;
        }
        itr = slave->dly_q->end();
        --itr;

        T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Sending Delay_Req message with seq_id %d ts-id %d", dly_req.seq_id, dly_req.ts_id);
        if (!vtss_1588_tx_msg(slave->port_mask, &slave->delay_req_buf, slave->clock->localClockId, false, &(*itr).ts_id)) {
            T_WG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Delay_Req message transmission failed");
        } else {
            ++slave->slave_port->port_statistics.txDelayRequestCount;
            ++slave->statistics.delay_req_pack_tx_cnt;
        }

        T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"logMinDelayReqInterval %d, logSyncInterval %d", slave->slave_port->portDS.status.logMinDelayReqInterval, slave->logSyncInterval);
        // Configure timer for next delay request message interval.
        if (!slave->random_relay_request_timer) {
            // Used for IPv4 unicast
            // Idea here is send a delay request message immediately after receiving sync message. But,
            // timer decides for which sync message delay request message need to be sent. Ref - section 9.5.1.2 c.2 in 1588-2019.
            if (slave->slave_port->portDS.status.logMinDelayReqInterval < slave->logSyncInterval) {
                slave->slave_port->portDS.status.logMinDelayReqInterval = slave->logSyncInterval;
            }
            shifts = slave->slave_port->portDS.status.logMinDelayReqInterval - slave->logSyncInterval;
            slave->delay_req_timer = vtss_ptp_get_rand(&slave->random_seed)%((1<<(shifts+1))-1) + 1;
            T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"delay_req_timer = %d, shifts = %d", slave->delay_req_timer, shifts);
        } else {
            if (slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_1) {
                // G.8275.1 profile is a randomization: see G.8275.1 section 6.2.8 Message rates
                // Delay request period should be within +/-30% interval of logMinDelayReqInterval for 8275.1
                // => range of random number is 60%(-30 to 30).
                // => Find a random number in range of (0,60%) and subtract 30% of logMinDelayReqInterval for +-30%.
                delay_req_period = PTP_LOG_TIMEOUT(slave->slave_port->portDS.status.logMinDelayReqInterval) +
                                   vtss_ptp_get_rand(&slave->random_seed)%(6 * PTP_LOG_TIMEOUT(slave->slave_port->portDS.status.logMinDelayReqInterval) / 10) -
                                   (3 * PTP_LOG_TIMEOUT(slave->slave_port->portDS.status.logMinDelayReqInterval) / 10);
            } else {
                delay_req_period = vtss_ptp_get_rand(&slave->random_seed)%(PTP_LOG_TIMEOUT(slave->slave_port->portDS.status.logMinDelayReqInterval + 1));
            }
            vtss_ptp_timer_start(timer, delay_req_period, false);
            T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"delay_req_period = %d", delay_req_period);
        }
    }
}

static char *vtss_ptp_clock_identity_to_str(const vtss_appl_clock_identity& clk_id)
{
    static char buf[3 * CLOCK_IDENTITY_LENGTH];

    for (int n = 0; n < CLOCK_IDENTITY_LENGTH; n++) {
       sprintf(&buf[n * 3], "%02X", clk_id[n]);
       buf[n * 3 + 2] = ':';
    }
    buf[3 * CLOCK_IDENTITY_LENGTH - 1] = 0;

    return buf;
}

mesa_rc vtss_ptp_port_speed_to_txt(mesa_port_speed_t speed, const char **speed_txt)
{
    switch (speed) {
        case MESA_SPEED_10M:    *speed_txt = "10M";   break;
        case MESA_SPEED_100M:   *speed_txt = "100M";  break;
        case MESA_SPEED_1G:     *speed_txt = "1G";    break;
        case MESA_SPEED_2500M:  *speed_txt = "2G5";   break;
        case MESA_SPEED_5G:     *speed_txt = "5G";    break;
        case MESA_SPEED_10G:    *speed_txt = "10G";   break;
        case MESA_SPEED_25G:    *speed_txt = "25G";   break;
        default:                return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

ptp_port_calibration_s ptp_port_calibration;
bool calib_t_plane_enabled = false;
bool calib_p2p_enabled = false;
bool calib_port_enabled = false;
bool calib_initiate = false;
i32  calib_cable_latency = 0;
i32  calib_pps_offset = 0;

/*
 * Handle sync request from a master
 *
 */
bool vtss_ptp_slave_sync(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender)
{
    ptp_slave_t *slave = &ptpClock[clock_inst]->ssm;    
    bool forwarded = false;
    u8 *buf = tx_buf->frame + tx_buf->header_length;
    mesa_timestamp_t  originTimestamp;
    char str[50];
    char str1[50];

    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"received sync, current parent is %s,%d",
         ClockIdentityToString(slave->parent_portIdentity_p->clockIdentity, str), slave->parent_portIdentity_p->portNumber);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"received sync, expected seqNo %d", slave->parent_last_sync_sequence_number);
    if (slave->state == SLAVE_STATE_PTP_OBJCT_ACTIVE && slave->slave_port != NULL &&
            (!PortIdentitycmp(&header->sourcePortIdentity, slave->parent_portIdentity_p) || slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS)) {
        ++slave->statistics.sync_pack_rx_cnt;
        if (slave->first_sync_packet || SEQUENCE_ID_CHECK(header->sequenceId, slave->parent_last_sync_sequence_number)) {
            if (header->domainNumber == slave->domainNumber) { /* if transparent clock only syntonize to a master with the default domain number*/
                slave->parent_last_sync_sequence_number = header->sequenceId;
                slave->ptsf_loss_of_sync = false;
                T_D("*** SLAVE SYNC ***");
#if defined(VTSS_SW_OPTION_SYNCE)
                vtss_ptp_ptsf_state_set(slave->localClockId);
#endif
//                *slave->record_update = true;
                vtss_ptp_unpack_timestamp(buf, &originTimestamp);
                T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"received sync, originTimestamp: %s corr %s", TimeStampToString(&originTimestamp,str), 
                    vtss_tod_TimeInterval_To_String(&header->correctionField, str1, '.'));
                // Send message interval request to update sync interval to operLogSyncInterval if synchronized (AED only)
                if (slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS && header->logMessageInterval != slave->slave_port->port_config->c_802_1as.operLogSyncInterval
                    && slave->clock_state >= VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING && slave->clock_state <= VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED) {
                        issue_message_interval_request(slave->slave_port, -128, slave->slave_port->port_config->c_802_1as.operLogSyncInterval, -128, 0, NULL, false);
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Request update of sync interval %d to operLogSyncInterval of %d", header->logMessageInterval, slave->slave_port->port_config->c_802_1as.operLogSyncInterval);
                    }
                if (header->logMessageInterval != slave->logSyncInterval && header->logMessageInterval != 0x7F) {
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"message's sync interval is %d, but clock's is %d", header->logMessageInterval, slave->logSyncInterval);
                    /* spec recommends handling a sync interval discrepancy as a fault, but we accept the interval from the master */
                    slave->slave_port->portDS.status.syncIntervalError = true;
                } else {
                    slave->slave_port->portDS.status.syncIntervalError = false;
                }
                if (header->logMessageInterval != 0x7F) {
                    slave->logSyncInterval = header->logMessageInterval;
                } else {
                    /* in the case where master sends 0x7f (unicast), we use the configured value in the slave */
                    slave->logSyncInterval = slave->slave_port->port_config->logSyncInterval;
                }
                vtss_local_clock_convert_to_time(tx_buf->hw_time,&slave->sync_receive_time,slave->localClockId);
                if (!getFlag(header->flagField[0], PTP_TWO_STEP_FLAG)) {
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"received 1-step sync");
                    slave->wait_follow_up  = false;
                    slave->sync_tx_time = originTimestamp;

                    i8 logMsgIntv;
                    if (getFlag(header->flagField[0], PTP_UNICAST_FLAG)) {
                        int master_index = slaveTableEntryFind(ptpClock[clock_inst]->slave, sender->ip);
                        if (master_index >=0) {
                            logMsgIntv = ptpClock[clock_inst]->slave[master_index].log_msg_period;
                        }
                        else {
                            logMsgIntv = slave->slave_port->port_config->logSyncInterval;
                        }
                    }
                    else {
                        logMsgIntv = header->logMessageInterval;
                    }

                    if (calib_t_plane_enabled || calib_p2p_enabled || calib_port_enabled) {
                        mesa_timestamp_t t1 = originTimestamp;
                        mesa_timestamp_t t2 = slave->sync_receive_time;
                        mesa_timeinterval_t diff_t1_t2;
                        static int calib_iteration = 0;
                        static mesa_timeinterval_t diff_t1_t2_accumulated = 0;
                        static mesa_timeinterval_t meanPathDelay_accumulated = 0;

                        if (calib_initiate) {
                           calib_iteration = 0;
                           diff_t1_t2_accumulated = 0;
                           meanPathDelay_accumulated = 0;

                           calib_initiate = false;
                        }
                        calib_iteration++;

                        vtss_tod_sub_TimeInterval(&diff_t1_t2, &t2, &t1);
                        diff_t1_t2 -= header->correctionField;
                        diff_t1_t2_accumulated += diff_t1_t2;
                        meanPathDelay_accumulated += slave->clock->currentDS.meanPathDelay;

                        T_I("Difference from t1 to t2 : %d.%03d ns", diff_t1_t2 >> 16, ((diff_t1_t2 & 0xFFFF) * 1000) >> 16);
                       
                        T_I("Slave port is: %s %u", vtss_ptp_clock_identity_to_str(slave->portIdentity_p->clockIdentity), slave->portIdentity_p->portNumber);
                        T_I("Master port is: %s %u", vtss_ptp_clock_identity_to_str(slave->parent_portIdentity_p->clockIdentity), slave->parent_portIdentity_p->portNumber);

                        if (calib_iteration == 100) {
                            mesa_timeinterval_t diff_t1_t2_avg = diff_t1_t2_accumulated / calib_iteration - ((mesa_timeinterval_t) calib_cable_latency << 16);
                            mesa_timeinterval_t meanPathDelay_avg = meanPathDelay_accumulated / calib_iteration - ((mesa_timeinterval_t) calib_cable_latency << 16);

                            // Check that clock identity of master matches that of the slave (except for the last bit) and the port is the same (otherwise the port has not been
                            // properly looped back).
                            bool loopback_ok = true;
                            if (calib_t_plane_enabled || calib_p2p_enabled) {
                                for (int n = 0; n < CLOCK_IDENTITY_LENGTH - 1; n++) {
                                   if (slave->portIdentity_p->clockIdentity[n] != slave->parent_portIdentity_p->clockIdentity[n]) loopback_ok = false;
                                }
                                if ((slave->portIdentity_p->clockIdentity[CLOCK_IDENTITY_LENGTH - 1] & 0xFE) != (slave->parent_portIdentity_p->clockIdentity[CLOCK_IDENTITY_LENGTH - 1] & 0xFE)) loopback_ok = false;
                            }
                            if (calib_t_plane_enabled) {
                                if (slave->portIdentity_p->portNumber != slave->parent_portIdentity_p->portNumber) loopback_ok = false;
                            } else if (calib_p2p_enabled) {
                                // FIXME: Make a check here that the correct port from the master has been looped to the slave port 
                            } else if (calib_port_enabled) {  
                                // FIXME: In the case of calib_port_enabled just check that the slave ported matches the configured port
                            } else {
                                T_W("Unexpected case in calibration procedure.");
                            }
     
                            if (loopback_ok) {
                                T_I("Loopback is OK");
     
                                // Add 50% of diff_t1_t2 to ports egress latency and 50% of it to ports ingress latency in order to make t1 and t2 equal going forward
                                // This involves a number of steps below:
     
                                // Determine port mode
                                vtss_appl_port_status_t port_status;
                                vtss_ifindex_t ifindex;
                                (void) vtss_ifindex_from_port(0, uport2iport(slave->portIdentity_p->portNumber), &ifindex);
                                vtss_appl_port_status_get(ifindex, &port_status);
     
                                const char *speed_txt;
                                vtss_ptp_port_speed_to_txt(port_status.speed, &speed_txt);
                                T_I("Port speed is: %s, duplex state is: %s", speed_txt, port_status.fdx ? "fdx" : "hdx");
          
                                // Read previous ingress and egress calibration values
                                mesa_timeinterval_t ingress_calib_value;
                                mesa_timeinterval_t egress_calib_value;

                                BOOL fiber = port_status.fiber;

                                switch (port_status.speed) {

                                    case MESA_SPEED_10M:   {
                                                               ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_10m_cu.ingress_latency;
                                                               egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_10m_cu.egress_latency;
                                                               break;
                                                           }  
                                    case MESA_SPEED_100M:  {
                                                               ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_100m_cu.ingress_latency;
                                                               egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_100m_cu.egress_latency;
                                                               break;
                                                           }  
                                    case MESA_SPEED_1G:    {
                                                               if (fiber) {
                                                                   ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_1g.ingress_latency;
                                                                   egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_1g.egress_latency;
                                                               } else {
                                                                   ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_1g_cu.ingress_latency;
                                                                   egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_1g_cu.egress_latency;
                                                               }
                                                               break;
                                                           }  
                                    case MESA_SPEED_2500M: {
                                                               ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_2g5.ingress_latency;
                                                               egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_2g5.egress_latency;
                                                               break;
                                                           }
                                    case MESA_SPEED_5G:    {
                                                               ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_5g.ingress_latency;
                                                               egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_5g.egress_latency;
                                                               break;
                                                           }
                                    case MESA_SPEED_10G:   {
                                                               ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_10g.ingress_latency;
                                                               egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_10g.egress_latency;
                                                               break;
                                                           }
                                    case MESA_SPEED_25G:   if ((port_status.fec_mode == VTSS_APPL_PORT_FEC_MODE_RS_FEC) || (port_status.fec_mode == VTSS_APPL_PORT_FEC_MODE_AUTO))
                                                           {
                                                               ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_25g_rsfec.ingress_latency;
                                                               egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_25g_rsfec.egress_latency;
                                                           } else {
                                                               ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_25g_nofec.ingress_latency;
                                                               egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_25g_nofec.egress_latency;
                                                           }
                                                           break;
                                    default: {
                                                 ingress_calib_value = 0;
                                                 egress_calib_value = 0;
                                             }
                                }
                                T_I("Previous egress calibration value was: %d", egress_calib_value);
                                T_I("Previous ingress calibration value was: %d", ingress_calib_value);
     
                                // Calculate new calibration values
                                if (calib_t_plane_enabled) {
                                    ingress_calib_value += diff_t1_t2_avg / 2;
                                } else if (calib_p2p_enabled) {
                                    ingress_calib_value += diff_t1_t2_avg;
                                } else if (calib_port_enabled) {
                                    ingress_calib_value += meanPathDelay_avg - ((mesa_timeinterval_t) calib_pps_offset << 16);
                                } else {
                                    T_W("Unexpected case in calibration procedure.");
                                }
                                if (ingress_calib_value >= 0) {
                                    T_I("New ingress calibration value: %lld.%03lld", ingress_calib_value, ((ingress_calib_value % 0xffff) * 1000) >> 16);
                                } else {
                                    T_I("New ingress calibration value: %lld.%03lld", ingress_calib_value, ((-ingress_calib_value % 0xffff) * 1000) >> 16);
                                }

                                if (calib_t_plane_enabled) {
                                    egress_calib_value += diff_t1_t2_avg / 2;
                                } else if (calib_p2p_enabled) {
                                    egress_calib_value -= diff_t1_t2_avg;
                                } else if (calib_port_enabled) {
                                    egress_calib_value += meanPathDelay_avg + ((mesa_timeinterval_t) calib_pps_offset << 16);
                                } else {
                                    T_W("Unexpected case in calibration procedure.");
                                }
                                if (egress_calib_value >= 0) {
                                    T_I("New egress calibration value: %lld.%03lld", egress_calib_value, ((egress_calib_value % 0xffff) * 1000) >> 16);
                                } else {
                                    T_I("New egress calibration value: %lld.%03lld", egress_calib_value, ((-egress_calib_value % 0xffff) * 1000) >> 16);
                                }

                                // Store the updated calibration values
                                switch (port_status.speed) {

                                    case MESA_SPEED_10M:   {
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_10m_cu.ingress_latency = ingress_calib_value;
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_10m_cu.egress_latency = egress_calib_value;
                                                               break;
                                                           }
                                    case MESA_SPEED_100M:  {
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_100m_cu.ingress_latency = ingress_calib_value;
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_100m_cu.egress_latency = egress_calib_value;
                                                               break;
                                                           }
                                    case MESA_SPEED_1G:    {
                                                               if (fiber) {
                                                                   ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_1g.ingress_latency = ingress_calib_value;
                                                                   ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_1g.egress_latency = egress_calib_value;
                                                               } else {
                                                                   ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_1g_cu.ingress_latency = ingress_calib_value;
                                                                   ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_1g_cu.egress_latency = egress_calib_value;
                                                               }
                                                               break;
                                                           }  
                                    case MESA_SPEED_2500M: {
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_2g5.ingress_latency = ingress_calib_value;
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_2g5.egress_latency = egress_calib_value;
                                                               break;
                                                           }
                                    case MESA_SPEED_5G:    {
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_5g.ingress_latency = ingress_calib_value;
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_5g.egress_latency = egress_calib_value;
                                                               break;
                                                           }
                                    case MESA_SPEED_10G:   {
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_10g.ingress_latency = ingress_calib_value;
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_10g.egress_latency = egress_calib_value;
                                                               break;
                                                           }
                                    case MESA_SPEED_25G:   if ((port_status.fec_mode == VTSS_APPL_PORT_FEC_MODE_RS_FEC) || (port_status.fec_mode == VTSS_APPL_PORT_FEC_MODE_AUTO))
                                                           {
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_25g_rsfec.ingress_latency = ingress_calib_value;
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_25g_rsfec.egress_latency = egress_calib_value;
                                                           } else {
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_25g_nofec.ingress_latency = ingress_calib_value;
                                                               ptp_port_calibration.port_calibrations[uport2iport(slave->portIdentity_p->portNumber)].port_latencies_25g_nofec.egress_latency = egress_calib_value;
                                                           }
                                                           break;
                                    default:;
                                }

                                // Write PTP port calibration to file on Linux filesystem
                                {
                                    int ptp_port_calib_file = open(PTP_CALIB_FILE_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
                                    if (ptp_port_calib_file != -1) {
                                        ptp_port_calibration.version = PTP_CURRENT_CALIB_FILE_VER;
                                        u32 crc32 = vtss_crc32((const unsigned char*)&ptp_port_calibration.version, sizeof(u32));
                                        crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)&ptp_port_calibration.rs422_pps_delay, sizeof(u32));
                                        crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)&ptp_port_calibration.sma_pps_delay, sizeof(u32));
                                        crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)ptp_port_calibration.port_calibrations.data(), fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * sizeof(port_calibrations_s));
                                        ptp_port_calibration.crc32 = crc32;
                        
                                        ssize_t numwritten = write(ptp_port_calib_file, &ptp_port_calibration.version, sizeof(u32));
                                        numwritten += write(ptp_port_calib_file, &ptp_port_calibration.crc32, sizeof(u32));
                                        numwritten += write(ptp_port_calib_file, &ptp_port_calibration.rs422_pps_delay, sizeof(u32));
                                        numwritten += write(ptp_port_calib_file, &ptp_port_calibration.sma_pps_delay, sizeof(u32));
                                        numwritten += write(ptp_port_calib_file, ptp_port_calibration.port_calibrations.data(), fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * sizeof(port_calibrations_s));
                        
                                        if (numwritten != 4 * sizeof(u32) + fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * sizeof(port_calibrations_s)) {
                                            T_W("Problem writing PTP port calibration data file.");
                                        }
                        
                                        if (close(ptp_port_calib_file) == -1) {
                                            T_W("Could not close PTP port calibration data file.");
                                        }
                                    } else {
                                        T_W("Could not create/open PTP port calibration data file");
                                    }
                                }

                                // Update ingrees and egress latencies in the hardware
                                {
                                    mesa_timeinterval_t egress_latency;
     
                                    vtss_1588_egress_latency_get(slave->portIdentity_p->portNumber, &egress_latency);
                                    vtss_1588_egress_latency_set(slave->portIdentity_p->portNumber, egress_latency);
                                }
                                {
                                    mesa_timeinterval_t ingress_latency;
     
                                    vtss_1588_ingress_latency_get(slave->portIdentity_p->portNumber, &ingress_latency);
                                    vtss_1588_ingress_latency_set(slave->portIdentity_p->portNumber, ingress_latency);
                                }
     
                            } else {
                                T_I("Loopback is NOT OK");
                            }
                            calib_t_plane_enabled = false;
                            calib_p2p_enabled = false;
                            calib_port_enabled = false;
                        }
                    }

                    if (ptp_offset_calc(ptpClock, clock_inst, originTimestamp, slave->sync_receive_time, header->correctionField, logMsgIntv, header->sequenceId, false)) {
                        // Advanced servo algorithm has asked for adjustment of time. Do not send delay request as response will be useless anyway                       
                        // and most importantly: do not retrigger sync_timer as servo algorithm has already programmed a timeout value of 3s.
                        return forwarded;
                    }
                    (void)clock_class_update(slave);
                } else {
                    if (slave->wait_follow_up  == true) {
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Follow_up message lost: cur.seq. %d", header->sequenceId);
                        slave->statistics.follow_up_pack_loss_cnt++;
                    }
                    slave->wait_follow_up  = true;
                    slave->sync_correctionField = header->correctionField;
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"received 2-step sync");
                }
                if (slave->slave_port != NULL) {
                    slave->sender = *sender;
                    if (slave->two_way) {
                        if (slave->slave_port->port_config->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_E2E) {
                            if (slave->first_sync_packet) {
                                slave->first_sync_packet = false;
                                if (slave->random_relay_request_timer &&
                                   ((slave->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) ||
                                    (slave->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY))) {
                                    vtss_ptp_timer_start(&slave->delay_req_sys_timer, PTP_LOG_TIMEOUT(slave->logDelayReqInterval), false);
                                }
                            }
                            if (!slave->random_relay_request_timer) {
                                if (((slave->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) || (slave->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY)) &&
                                        !(--slave->delay_req_timer)) {
                                    transmit_delay_req(&slave->delay_req_sys_timer, slave);
                                }
                            }
                        } else {
                            slave->clock->currentDS.delayOk = slave->slave_port->portDS.status.peer_delay_ok;
                            T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"delayOk %d", slave->clock->currentDS.delayOk);
                        }
                    }
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"SYNC_RECEIPT_TIMER reset");
                    if (slave->in_settling_period) {
                        slave->sy_to = (slave->clock_settle_time ) * PTP_LOG_TIMEOUT(0);
                    } else {
                        slave->sy_to = (slave->logSyncInterval > -2) ? 3*PTP_LOG_TIMEOUT(slave->logSyncInterval) : PTP_LOG_TIMEOUT(0);
                    }
                    vtss_ptp_timer_start(&slave->sync_timer, slave->sy_to, false);
                } else {
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"port went disabled in offset calc");
                }
            }
        } else {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"sync sequence error: last %d, this %d", slave->parent_last_sync_sequence_number, header->sequenceId);
            slave->parent_last_sync_sequence_number = header->sequenceId;
            ++slave->statistics.sync_pack_seq_err_cnt;
        }
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"received Sync: slave not active");
    }
    return forwarded;
}

/*
 * Handle follow_up request from a master
 *
 */
bool vtss_ptp_slave_follow_up(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender)
{
    ptp_slave_t *slave = &ptpClock[clock_inst]->ssm;
    bool forwarded = false;
    u8 *buf = tx_buf->frame + tx_buf->header_length;
    mesa_timestamp_t  preciseOriginTimestamp;
    bool addSeedFreq = false;
    

    if (slave->slave_port != NULL) {
        vtss_ptp_unpack_timestamp(buf, &preciseOriginTimestamp);

        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"wait_follow_up %d", slave->wait_follow_up);
        if ( slave->wait_follow_up
                && header->sequenceId == slave->parent_last_sync_sequence_number
                && (!PortIdentitycmp(&header->sourcePortIdentity, slave->parent_portIdentity_p) || slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS
                   )) {
            slave->wait_follow_up  = false;
            slave->sync_tx_time = preciseOriginTimestamp;
            // check if Follow_Up TLV is wanted and present.
#if defined (VTSS_SW_OPTION_P802_1_AS)
            if (slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || slave->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                ptpClock[clock_inst]->localGMUpdate = false;
                if (VTSS_RC_OK !=  vtss_ptp_tlv_follow_up_tlv_process(tx_buf->frame + tx_buf->header_length + FOLLOW_UP_PACKET_LENGTH, tx_buf->size - (tx_buf->header_length + FOLLOW_UP_PACKET_LENGTH), &slave->clock->follow_up_info)) {
                    T_W("process Follow_Up Tlv extension failed");
                } else {
                    double fcumul; // received cumulativeScaledRateOffset converted to rate offset
                    double fnei;   // calculated neighborRateRatio converted to rate offset
                    // calculate cumulativeRateRatio
                    fcumul = ((double)slave->clock->follow_up_info.cumulativeScaledRateOffset/(double)RATE_TO_U32_CONVERT_FACTOR) + 1.0;
                    fnei = ((double)slave->slave_port->portDS.status.s_802_1as.peer_d.neighborRateRatio/(double)RATE_TO_U32_CONVERT_FACTOR) + 1.0;
                    ptpClock[clock_inst]->parentDS.par_802_1as.cumulativeRateRatio = (i64)((fcumul * fnei - 1.0) * (double)RATE_TO_U32_CONVERT_FACTOR);
                    ptpClock[clock_inst]->port_802_1as_sync_sync.rateRatio = fcumul * fnei;
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"received cumul %d, cumulative rate (fp) %E, scaled %d", slave->clock->follow_up_info.cumulativeScaledRateOffset, fcumul * fnei - 1.0, ptpClock[clock_inst]->parentDS.par_802_1as.cumulativeRateRatio);
                    ptpClock[clock_inst]->current_802_1as_ds.lastGMPhaseChange = slave->clock->follow_up_info.lastGmPhaseChange;
                    if (ptpClock[clock_inst]->current_802_1as_ds.gmTimeBaseIndicator != slave->clock->follow_up_info.gmTimeBaseIndicator) {
                        addSeedFreq = true;
                        ptpClock[clock_inst]->current_802_1as_ds.lastGMFreqChange = ((double)slave->slave_port->portDS.status.s_802_1as.peer_d.neighborRateRatio/(double)RATE_TO_U32_CONVERT_FACTOR) + 1.0;
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"TimebaseIndicator changed from %u to %u", ptpClock[clock_inst]->current_802_1as_ds.gmTimeBaseIndicator, slave->clock->follow_up_info.gmTimeBaseIndicator);
                        ptpClock[clock_inst]->current_802_1as_ds.gmTimeBaseIndicator = slave->clock->follow_up_info.gmTimeBaseIndicator;
                        if (slave->clock->follow_up_info.lastGmPhaseChange.scaled_ns_low != 0) {
                            ptpClock[clock_inst]->current_802_1as_ds.timeOfLastGMPhaseChangeEvent = u32(vtss_current_time()/10);
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"new timeOfLastGMPhaseChangeEvent %u", ptpClock[clock_inst]->current_802_1as_ds.timeOfLastGMPhaseChangeEvent);
                        }
                        if (slave->clock->follow_up_info.scaledLastGmFreqChange != 0) {
                            ptpClock[clock_inst]->current_802_1as_ds.timeOfLastGMFreqChangeEvent = u32(vtss_current_time()/10);
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"new timeOfLastGMFreqChangeEvent %u", ptpClock[clock_inst]->current_802_1as_ds.timeOfLastGMFreqChangeEvent);
                        }
                    }

                    if (slave->first_sync_packet) {
                        addSeedFreq = true;
                    }
                }
            }
#endif //(VTSS_SW_OPTION_P802_1_AS)
            slave->first_sync_packet = false;
            if (addSeedFreq) {
                ptpClock[clock_inst]->ssm.servo->seedFreqSet(ptpClock[clock_inst], &preciseOriginTimestamp, &slave->sync_receive_time,
                        header->correctionField + slave->sync_correctionField, ptpClock[clock_inst]->port_802_1as_sync_sync.rateRatio-1,
                        &slave->clock->follow_up_info, &ptpClock[clock_inst]->current_802_1as_ds);
                return forwarded;
            }

            i8 logMsgIntv;
            if (getFlag(header->flagField[0], PTP_UNICAST_FLAG)) {
                int master_index = slaveTableEntryFind(ptpClock[clock_inst]->slave, sender->ip);
                if (master_index >=0) {
                    logMsgIntv = ptpClock[clock_inst]->slave[master_index].log_msg_period;
                }
                else {
                    logMsgIntv = slave->slave_port->port_config->logSyncInterval;
                }
            }
            else {
                logMsgIntv = header->logMessageInterval;
            }

            if (ptp_offset_calc(ptpClock, clock_inst, preciseOriginTimestamp, slave->sync_receive_time, header->correctionField + slave->sync_correctionField, logMsgIntv, header->sequenceId, false)) {
                // Advanced servo algorithm has asked for adjustment of time. Do not send delay request as response will be useless anyway                       
                // and most importantly: do not retrigger sync_timer as servo algorithm has already programmed a timeout value of 3s.
                return forwarded;
            }
            (void)clock_class_update(slave);
            if (slave->in_settling_period) {
                vtss_ptp_timer_start(&slave->sync_timer, (slave->clock_settle_time ) * PTP_LOG_TIMEOUT(0), false);
            }
        } else {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"handleFollowUp: unwanted, wff = %d, header.seq = %d, sync.seq = %d",
                slave->wait_follow_up ,
                header->sequenceId,
                slave->parent_last_sync_sequence_number);
        }
    }
    return forwarded;
}

/*
 * Handle delayResp request from a master
 *
 */
bool vtss_ptp_slave_delay_resp(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender)
{
    ptp_slave_t *slave = &ptpClock[clock_inst]->ssm;
    bool forwarded = false;
    u8 *buf = tx_buf->frame + tx_buf->header_length;
    MsgDelayResp resp;
    ptp_ts_entry_t ts = {.seq_id = 0};
    bool found = false;
    delay_req_ts_q_itr_t itr = slave->dly_q->begin();
    delay_req_ts_q_itr_t itr_end = slave->dly_q->end();

    if (slave->slave_port != NULL) {
        vtss_ptp_unpack_delay_resp(buf, &resp);

        // Iterate delay request pkts in Q. If required, pop and process them.
        while (itr != itr_end) {
            T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Q seq-id %u hdr seq-id %u", (*itr).seq_id, header->sequenceId);
            if (header->sequenceId == (*itr).seq_id) {
                found = true;
                slave->statistics.delay_resp_pack_rx_cnt++;
                // found entry with sequence-id. Hence break the loop.
                break;
            }
            itr++;
        }

        if (found) {
            // Delay request packet acknowledged.
            if (!PortIdentitycmp(&resp.requestingPortIdentity, slave->portIdentity_p) &&
                !PortIdentitycmp(&header->sourcePortIdentity, slave->parent_portIdentity_p)) {
                if (!getFlag(header->flagField[0], PTP_UNICAST_FLAG)) {
                    slave->slave_port->portDS.status.logMinDelayReqInterval = header->logMessageInterval;
                } else {
                    slave->slave_port->portDS.status.logMinDelayReqInterval = slave->slave_port->port_config->logMinPdelayReqInterval;
                }
                T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "logMinDelayReqInterval %d", slave->slave_port->portDS.status.logMinDelayReqInterval);
                (*itr).rx_time = resp.receiveTimestamp;
                (*itr).corr    = header->correctionField;
                (*itr).rx_valid = true;

                // logDelayReqInterval used by basic servo.
                if (getFlag(header->flagField[0], PTP_UNICAST_FLAG)) {
                    int master_index = slaveTableEntryFind(ptpClock[clock_inst]->slave, sender->ip);
                    if (master_index >=0) {
                        slave->logDelayReqInterval = ptpClock[clock_inst]->slave[master_index].log_msg_period;
                    }
                    else {
                        slave->logDelayReqInterval = slave->slave_port->port_config->logSyncInterval;
                    }
                } else {
                    slave->logDelayReqInterval = header->logMessageInterval;
                }

                // If both Tx, Rx timestamps are valid, then calculate delay.
                // Both *itr and ts will point to same entry.
                if ((*itr).tx_valid) {
                    if (slave->clock_state != VTSS_PTP_SLAVE_CLOCK_F_SETTLING &&
                        slave->clock_state != VTSS_PTP_SLAVE_CLOCK_P_SETTLING) {
                        ptp_delay_calc(ptpClock, clock_inst, (*itr).tx_time, (*itr).rx_time, (*itr).corr, slave->logDelayReqInterval);
                        T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"DelayResp received for seq_id %d.", (*itr).seq_id);
                    }

                    // Pop entry used delay calculation
                    while (!slave->dly_q->empty()) {
                        slave->dly_q->pop(ts);
                        if (ts.seq_id == header->sequenceId) {
                            T_RG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Removing ts seq_id %d from Q", ts.seq_id);
                            break;
                        } else {
                            if (!ts.tx_valid) {
                                slave->statistics.delay_req_intr_not_rcvd_cnt++;
                            } else if (!ts.rx_valid) {
                                slave->statistics.delay_resp_seq_err_cnt++;
                            }
                        }
                    }
                } else if (!(*itr).tx_valid) {
                    // received delay response ahead of interrupt.
                    slave->clockBase = &ptpClock;
                } else {
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Clock not settled.Delay response ignored.");
                }
            } else {
                T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"response not intended for me");
            }
        }
    }
    return forwarded;
}

/*
 * PTP clock settling timer
 */
/*lint -esym(459, vtss_ptp_settle_sync_timer) */
static void vtss_ptp_settle_sync_timer(vtss_ptp_sys_timer_t *timer, void *s)
{
    ptp_slave_t *slave = (ptp_slave_t *)s;
    if (slave->clock_state == VTSS_PTP_SLAVE_CLOCK_F_SETTLING) {
        slave->clock_state = VTSS_PTP_SLAVE_CLOCK_FREERUN;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: Settling time expired", clock_state_to_string (slave->clock_state));
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Settle timeout in a Free run state");
    } else if (slave->clock_state == VTSS_PTP_SLAVE_CLOCK_P_SETTLING) {
        slave->clock_state = VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING;
        slave->prev_psettling = true;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: Settling time expired", clock_state_to_string (slave->clock_state));
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Settle timeout in a Phase locking state");
    } else if (slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCK_INIT) {
        slave->clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: Settling time expired", clock_state_to_string (slave->clock_state));
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Settle timeout in a FreqLockInit state");
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"Settle timeout in an unexpected state");
    }

    slave->in_settling_period = false;
}

void debug_log_header_2_print(FILE *logFile)
{
    fprintf(logFile, "#MasterUUID 00:01:C1:FF:FE:xx:xx:xx\n");
    fprintf(logFile, "#MasterIP n.a.\n");
    fprintf(logFile, "#ProbeUUID 00:01:c1:FF:FE:xx:xx:xx\n");
    fprintf(logFile, "#ProbeIP n.a.\n");
    fprintf(logFile, "#Title: Microchip Test Probe/1588 Timestamp Data/Transmit and receive Timestamp\n");
}

int ptp_offset_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, mesa_timeinterval_t correction, i8 logMsgIntv, u16 sequenceId, bool virtual_port)
{
    int r;
    bool peer_delay;
    // The Servo does not support oneway, unless we tell it that we use P2P delay mechanism, therefore the peer_delay parameter is also set in the oneWay case and in the virtual port case
    if (ptpClock[clock_inst]->ssm.slave_port == 0) {
        peer_delay = false;
    } else {
        peer_delay = ptpClock[clock_inst]->ssm.slave_port->port_config->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_P2P || ptpClock[clock_inst]->clock_init->cfg.oneWay ||
                     virtual_port;
    }
    r = ptpClock[clock_inst]->ssm.servo->offset_calc(ptpClock, clock_inst, send_time, recv_time, correction, logMsgIntv, sequenceId,
            peer_delay, virtual_port);
    if (ptpClock[clock_inst]->ssm.clock_state != ptpClock[clock_inst]->ssm.old_clock_state) {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s", clock_state_to_string(ptpClock[clock_inst]->ssm.clock_state));
        ptpClock[clock_inst]->ssm.old_clock_state = ptpClock[clock_inst]->ssm.clock_state;
    }
    return r;
}

void ptp_delay_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, mesa_timeinterval_t correction, i8 logMsgIntv)
{
    ptpClock[clock_inst]->ssm.servo->delay_calc(ptpClock, clock_inst, send_time, recv_time, correction, logMsgIntv);
}

bool clock_class_update(ptp_slave_t *ssm)
{
    bool rc = true;
    vtss_appl_ptp_clock_slave_ds_t slave_ds;

    vtss_ptp_get_clock_slave_ds(ssm->clock, &slave_ds);

    T_D("slave_state %d",slave_ds.slave_state);
    switch (slave_ds.slave_state) {
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREERUN:
            ssm->clock->announced_clock_quality = ssm->clock->defaultDS.status.clockQuality;
            ssm->clock->time_prop->frequencyTraceable = false;
            break;
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREQ_LOCKING:
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKING:
            break; // unchanged clock class
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREQ_LOCKED:
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKED:
            if (ssm->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_1 || ssm->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_2) {
                if (ssm->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {
                    ssm->clock->announced_clock_quality.clockClass = G8275PRTC_GM_CLOCK_CLASS;
                    ssm->clock->announced_clock_quality.clockAccuracy = 0x21;
                    ssm->clock->announced_clock_quality.offsetScaledLogVariance = 0x4E5D;
                    ssm->clock->timepropertiesDS.frequencyTraceable = true;
                } else { // otherwise the announced clock class from the master is used
                    ssm->clock->announced_clock_quality.clockClass = ssm->clock->parentDS.grandmasterClockQuality.clockClass;
                    ssm->clock->announced_timeTraceable = ssm->clock->timepropertiesDS.timeTraceable;
                    ssm->clock->announced_currentUtcOffsetValid = ssm->clock->timepropertiesDS.currentUtcOffsetValid;
                }
                ssm->g8275_holdover_trace_count = 0;
            } else {
                ssm->clock->announced_clock_quality = ssm->clock->parentDS.grandmasterClockQuality;
            }
            break;
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_HOLDOVER:
            if (ssm->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_1 || ssm->clock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_2) {
                #if defined(VTSS_SW_OPTION_ZLS30387)
                    zl303xx_HoldoverQualityParamsS holdoverQualityParamsP;
                    extern zl303xx_ParamsS *zl303xx_Params_dpll;

                    zl303xx_GetHoldoverQuality(zl303xx_Params_dpll, &holdoverQualityParamsP);
                    if (holdoverQualityParamsP.holdoverQuality == HOLDOVER_QUALITY_IN_SPEC){
                        if (ssm->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {
                            ssm->clock->announced_clock_quality.clockClass = G8275PRTC_GM_OUT_OF_HO_CLOCK_CLASS_CAT1;
                        } else {
                            // BC in holdover: only change the clockClass if it has  been locked to one with better class
                            if (ssm->clock->announced_clock_quality.clockClass < G8275PRTC_BC_HO_CLOCK_CLASS) {
                                ssm->clock->announced_clock_quality.clockClass = G8275PRTC_BC_HO_CLOCK_CLASS;
                                }
                            ssm->clock->timepropertiesDS.timeTraceable = ssm->clock->announced_timeTraceable;
                            ssm->clock->timepropertiesDS.currentUtcOffsetValid = ssm->clock->announced_currentUtcOffsetValid;
                        }
                    } else {
                        if (ssm->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {
                            ssm->clock->announced_clock_quality.clockClass = ssm->clock->local_clock_class;
                        } else {
                            // BC in holdover: only change the clockClass if it has  been locked to one with better class
                            if (ssm->clock->announced_clock_quality.clockClass < G8275PRTC_BC_OUT_OF_HO_CLOCK_CLASS) {
                                ssm->clock->announced_clock_quality.clockClass = G8275PRTC_BC_OUT_OF_HO_CLOCK_CLASS;
                            }
                            ssm->clock->timepropertiesDS.timeTraceable = false;
                            ssm->clock->timepropertiesDS.currentUtcOffsetValid = false;
                        }
                    rc = false;
                    }
                #else
                    if (ssm->g8275_holdover_trace_count < ssm->clock->holdover_timeout_spec) {
                        ssm->g8275_holdover_trace_count++;
                        T_I("g8275_holdover_trace_count %d, holdover time %d",ssm->g8275_holdover_trace_count, ssm->clock->holdover_timeout_spec);
                        if (ssm->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {
                            ssm->clock->announced_clock_quality.clockClass = G8275PRTC_GM_OUT_OF_HO_CLOCK_CLASS_CAT1;
                        } else {
                            // BC in holdover: only change the clockClass if it has  been locked to one with better class
                            if (ssm->clock->announced_clock_quality.clockClass < G8275PRTC_BC_HO_CLOCK_CLASS) {
                                ssm->clock->announced_clock_quality.clockClass = G8275PRTC_BC_HO_CLOCK_CLASS;
                            }
                            ssm->clock->timepropertiesDS.timeTraceable = ssm->clock->announced_timeTraceable;
                            ssm->clock->timepropertiesDS.currentUtcOffsetValid = ssm->clock->announced_currentUtcOffsetValid;
                        }
                    } else {
                        if (ssm->clock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {
                            ssm->clock->announced_clock_quality.clockClass = ssm->clock->local_clock_class;
                        } else {
                            // BC in holdover: only change the clockClass if it has  been locked to one with better class
                            if (ssm->clock->announced_clock_quality.clockClass < G8275PRTC_BC_OUT_OF_HO_CLOCK_CLASS) {
                                ssm->clock->announced_clock_quality.clockClass = G8275PRTC_BC_OUT_OF_HO_CLOCK_CLASS;
                            }
                            ssm->clock->timepropertiesDS.timeTraceable = false;
                            ssm->clock->timepropertiesDS.currentUtcOffsetValid = false;
                        }
                        rc = false;
                    }
                    if (ssm->clock->local_clock_class == G8275PRTC_GM_OUT_OF_HO_CLOCK_CLASS_CAT1 ||
                            ssm->clock->local_clock_class == G8275PRTC_BC_HO_CLOCK_CLASS) {
                        ssm->clock->timepropertiesDS.frequencyTraceable = true;
                    } else {
                        ssm->clock->timepropertiesDS.frequencyTraceable = false;
                    }
                    ssm->clock->announced_clock_quality.clockAccuracy = 0xFE;
                    ssm->clock->announced_clock_quality.offsetScaledLogVariance = 0xFFFF;
                #endif
            } else {
                ssm->clock->announced_clock_quality = ssm->clock->defaultDS.status.clockQuality;
            }
            break;
        default:
            ssm->clock->announced_clock_quality = ssm->clock->defaultDS.status.clockQuality;
            ssm->clock->time_prop->frequencyTraceable = false;
            break;
    }
    return rc;
}

/*
 * Stop logging
 *
 */
/*lint -esym(459, vtss_ptp_slave_log_timeout) */
static void vtss_ptp_slave_log_timeout(vtss_ptp_sys_timer_t *timer, void *s)
{
    ptp_slave_t *slave = (ptp_slave_t *)s;
    ptp_clock_t *clock = slave->clock;
    T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"logging timed out");
    if (!vtss_ptp_debug_mode_set(clock, 0, FALSE, FALSE, 0)) {
        T_WG(VTSS_TRACE_GRP_PTP_BASE_SLAVE,"stop logging failed");
    }
}

