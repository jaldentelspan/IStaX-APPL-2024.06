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
#include "vtss_ptp_internal_types.h"
#include "vtss_ptp_os.h"
#include "vtss_ptp_packet_callout.h"
#include "vtss_ptp_slave.h"
#include "vtss_ptp_tc.h"
#include "vtss_ptp_peer_delay.h"
#include "vtss_ptp_unicast.hxx"
#include "vtss/appl/ptp.h"
#include "ptp_api.h"
#include "vtss_ptp_802_1as_site_sync.h"
#include "vtss_ptp_802_1as_bmca.h"

/**
 * \file vtss_ptp_api.c
 * \brief PTP main API implementation file
 *
 * This file contain the implementation of API functions
 *
 */

/* When defaultDS is changed, and this clock is grandmaster, then the parentDS shall also be updated */
void defaultDSChanged(ptp_clock_t *ptpClock)
{
    /* Parent data set */
    if (memcmp(ptpClock->parentDS.grandmasterIdentity, ptpClock->defaultDS.status.clockIdentity, CLOCK_IDENTITY_LENGTH) == 0) {
        ptpClock->parentDS.grandmasterPriority1 = ptpClock->clock_init->cfg.priority1;
        ptpClock->parentDS.grandmasterPriority2 = ptpClock->clock_init->cfg.priority2;
        ptpClock->parentDS.grandmasterClockQuality = ptpClock->defaultDS.status.clockQuality;
    }
#if defined (VTSS_SW_OPTION_P802_1_AS)
    if (ptpClock->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
        vtss_ptp_802_1as_bmca_new_info(ptpClock);
    }
#endif //(VTSS_SW_OPTION_P802_1_AS)
}

static void timepropertiesDSChanged(ptp_clock_t *ptpClock)
{
    /* TimeProperties data set */
    if (memcmp(ptpClock->parentDS.grandmasterIdentity, ptpClock->defaultDS.status.clockIdentity, CLOCK_IDENTITY_LENGTH) == 0) {
        memcpy(&ptpClock->timepropertiesDS, ptpClock->time_prop, sizeof(vtss_appl_ptp_clock_timeproperties_ds_t));
    }
}

static bool cmldsEnabledOnPort(PtpPort_t *ptpPort)
{
    bool ret = false;

    for (int i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        if (ptpPort->pDelay.port_cmlds->cmlds_in_use[i]) {
            ret = true;
            break;
        }
    }
    return ret;
}

void vtss_ptp_clock_remove(ptp_clock_t *ptpClock)
{
    int i;
    for (i=0; i < ptpClock->defaultDS.status.numberPorts; i++) {
        vtss_ptp_free(ptpClock->ptpPort[i].foreignMasterDS);
    }
    vtss_ptp_free(ptpClock->ptpPort);
    vtss_ptp_tc_delete(&ptpClock->tcsm);
    vtss_ptp_free(ptpClock);
}

/*lint -esym(429, vtss_ptp_tc_create) */
ptp_clock_t * vtss_ptp_clock_add(ptp_init_clock_ds_t *clock_init,
                                 vtss_appl_ptp_clock_timeproperties_ds_t *time_prop,
                                 vtss_appl_ptp_config_port_ds_t *port_config,
                                 ptp_servo *servo,
                                 int localClockId)
{
    ptp_clock_t *ptpClock;
    int i;
    uint8_t domain = clock_init->cfg.domainNumber;
    ptpClock = VTSS_PTP_CREATE(ptp_clock_t);
    if (!ptpClock) {
        T_E("failed to allocate memory for protocol engine data");
        return 0;
    } else {
        vtss_clear(*ptpClock);
        T_N("allocated %d bytes for clock data\n", (int)sizeof(ptp_clock_t)); // NOLINT
        ptpClock->clock_init = clock_init;
        ptpClock->time_prop = time_prop;
        //ptpClock->owd_filt = delayFilterGetInstance(servo, 0);
        ptpClock->localClockId = localClockId;
        ptpClock->ptpPort = (PtpPort_t *) vtss_ptp_calloc(clock_init->numberPorts, sizeof(PtpPort_t));
        if (!ptpClock->ptpPort) {
            T_E("failed to allocate memory for port data");
            vtss_ptp_free(ptpClock);
            return 0;
        }

        for (i = 0; i < clock_init->numberPorts; i++) {
            vtss_clear(ptpClock->ptpPort[i]);
        }

        T_N("allocated %d bytes for %d ports data\n", (int) sizeof(PtpPort_t)*clock_init->numberPorts, clock_init->numberPorts);
        ptpClock->tcsm.outstanding_delay_req_list.list = (DelayReqListEntry*)vtss_ptp_calloc(clock_init->max_outstanding_records, sizeof(DelayReqListEntry));
        ptpClock->tcsm.outstanding_delay_req_list.listSize = clock_init->max_outstanding_records;
        ptpClock->tcsm.sync_outstanding_list.list = (SyncOutstandingListEntry*)vtss_ptp_calloc(clock_init->max_outstanding_records, sizeof(SyncOutstandingListEntry));
        ptpClock->tcsm.sync_outstanding_list.listSize = clock_init->max_outstanding_records;
        if (!ptpClock->tcsm.outstanding_delay_req_list.list || !ptpClock->tcsm.sync_outstanding_list.list) {
            T_EG(VTSS_TRACE_GRP_PTP_BASE_TC,"failed to allocate " VPRIz " bytes of memory for forwarding", clock_init->max_outstanding_records*sizeof(DelayReqListEntry));
            if (ptpClock->tcsm.outstanding_delay_req_list.list) vtss_ptp_free(ptpClock->tcsm.outstanding_delay_req_list.list);
            if (ptpClock->tcsm.sync_outstanding_list.list) vtss_ptp_free(ptpClock->tcsm.sync_outstanding_list.list);
            vtss_ptp_free(ptpClock->ptpPort);
            vtss_ptp_free(ptpClock);
            return 0;
        }
        memset(ptpClock->tcsm.outstanding_delay_req_list.list, 0, clock_init->max_outstanding_records * sizeof(DelayReqListEntry));
        memset(ptpClock->tcsm.sync_outstanding_list.list, 0, clock_init->max_outstanding_records * sizeof(SyncOutstandingListEntry));
        vtss_ptp_tc_create(&ptpClock->tcsm, clock_init->max_outstanding_records, ptpClock);
#if defined (VTSS_SW_OPTION_P802_1_AS)
        vtss_ptp_802_1as_site_synccreate(ptpClock);
#endif //(VTSS_SW_OPTION_P802_1_AS)
        ptpClock->ptsf_state = SYNCE_PTSF_LOSS_OF_ANNOUNCE;
        ptpClock->synce_clock_class = 255;
        vtss_ptp_slave_create(&ptpClock->ssm);
        (void)vtss_ptp_clock_slave_statistics_enable(ptpClock, false);
        ptpClock->ssm.servo = servo;
        ptpClock->ssm.clock = ptpClock;
        for (i=0; i < clock_init->numberPorts; i++) {
            ptpClock->ptpPort[i].portDS.status.portIdentity.portNumber = i+1;
            ptpClock->ptpPort[i].port_mask = 1LL<<i;        /* set port mask used by tx functions */
            ptpClock->ptpPort[i].foreignMasterDS = (ForeignMasterDS*)vtss_ptp_calloc(clock_init->max_foreign_records, sizeof(ForeignMasterDS));
            ptpClock->ptpPort[i].port_config = &port_config[i];
            ptpClock->ptpPort[i].parent = ptpClock;
            ptpClock->ptpPort[i].pDelay.clock = ptpClock;
            ptpClock->ptpPort[i].portDS.status.peer_delay_ok = true;
            ptpClock->ptpPort[i].portDS.status.s_802_1as.portRole = VTSS_APPL_PTP_PORT_ROLE_DISABLED_PORT;
            ptpClock->ptpPort[i].portDS.status.s_802_1as.peer_d.isMeasuringDelay = FALSE;
            ptpClock->ptpPort[i].portDS.status.s_802_1as.asCapable = FALSE;
            ptpClock->ptpPort[i].portDS.status.s_802_1as.peer_d.neighborRateRatio = 0;
            ptpClock->ptpPort[i].portDS.status.s_802_1as.syncReceiptTimeInterval = 0LL;
            ptpClock->ptpPort[i].portDS.status.s_802_1as.acceptableMasterTableEnabled = FALSE;
            ptpClock->ptpPort[i].portDS.status.s_802_1as.peer_d.versionNumber = VERSION_PTP;
            ptpClock->ptpPort[i].portDS.status.s_802_1as.peer_d.minorVersionNumber = ptpClock->ptpPort[i].port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
            ptpClock->ptpPort[i].neighborGptpCapable = FALSE;

            if (i >= clock_init->numberEtherPorts) {
                ptpClock->ptpPort[i].virtual_port = true;
            }

            if (domain == 0) {
                ptpClock->ptpPort[i].port_config->c_802_1as.useMgtSettableLogAnnounceInterval = FALSE;
                ptpClock->ptpPort[i].port_config->c_802_1as.useMgtSettableLogSyncInterval = FALSE;
            } else {
                ptpClock->ptpPort[i].port_config->c_802_1as.useMgtSettableLogAnnounceInterval = TRUE;
                ptpClock->ptpPort[i].port_config->c_802_1as.useMgtSettableLogSyncInterval = TRUE;
            }
            ptpClock->ptpPort[i].port_config->c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval = FALSE;
            ptpClock->ptpPort[i].port_config->c_802_1as.useMgtSettableLogGptpCapableMessageInterval = FALSE;
            ptpClock->ptpPort[i].portDS.status.s_802_1as.syncLocked = TRUE;
            ptpClock->ptpPort[i].ansm.numberAnnounceTransmissions = 0;
            ptpClock->ptpPort[i].ansm.announceSlowdown = FALSE;
            ptpClock->ptpPort[i].msm.numberSyncTransmissions = 0;
            ptpClock->ptpPort[i].msm.syncSlowdown = FALSE;
            ptpClock->ptpPort[i].gptpsm.gPtpCapableMessageSlowdown = FALSE;
            ptpClock->ptpPort[i].gptpsm.numberGptpCapableMessageTransmissions = 0;
            ptpClock->ptpPort[i].port_config->c_802_1as.mgtSettableLogSyncInterval = -3;
            ptpClock->ptpPort[i].port_config->c_802_1as.mgtSettableLogAnnounceInterval = 0;
            ptpClock->ptpPort[i].port_config->c_802_1as.peer_d.mgtSettableLogPdelayReqInterval = 0;
            ptpClock->ptpPort[i].port_config->c_802_1as.mgtSettableLogGptpCapableMessageInterval = 3;
            ptpClock->ptpPort[i].port_signaling_message_sequence_number = 0;
            //ptpClock->ptpPort[i].owd_filt = delayFilterGetInstance(servo, i+1);
            if (!ptpClock->ptpPort[i].foreignMasterDS) {
                T_E("failed to allocate memory for foreign master data");
                while (i>0) {
                    vtss_ptp_free(ptpClock->ptpPort[--i].foreignMasterDS);
                }
                vtss_ptp_free(ptpClock->ptpPort);
                vtss_ptp_tc_delete(&ptpClock->tcsm);
                vtss_ptp_free(ptpClock);
                return 0;
            } else {
                T_N("allocated %d bytes for foreign master data\n", (int)(clock_init->max_foreign_records*sizeof(ForeignMasterDS)));
            }
            vtss_ptp_peer_delay_state_init(&ptpClock->ptpPort[i].pDelay);
        }
        //vtss_ptp_bmca_create(ptpClock);

    }
    return ptpClock;
}

bool vtss_ptp_clock_servo_set(ptp_clock_t *ptp, ptp_servo *servo)
{
    ptp->ssm.servo = servo;
    return true;
}

mesa_rc vtss_ptp_port_ena(ptp_clock_t *ptp, uint portnum)
{
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        if (ptpPort->designatedEnabled != true) {
            ptpPort->designatedEnabled = true;
            if (vtss_ptp_p2p_state(ptp, portnum, ptpPort->p2p_state) != VTSS_RC_OK) return VTSS_RC_ERROR;
            if (ptpPort->portDS.status.portState == VTSS_APPL_PTP_DISABLED && ptpPort->linkState) {
                vtss_ptp_state_set(VTSS_APPL_PTP_INITIALIZING, ptp, ptpPort);
            }
            if (ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED && !ptpPort->linkState) {
                T_I("port disabled %d", portnum);
                vtss_ptp_state_set(VTSS_APPL_PTP_DISABLED,ptp, ptpPort);
            }
        }
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_ptp_port_ena_virtual(ptp_clock_t *ptp, uint portnum)
{
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        if (ptpPort->virtual_port != true) {
            ptpPort->virtual_port = true;
        }
    }
    return vtss_ptp_port_ena(ptp, portnum);
}

mesa_rc vtss_ptp_port_dis(ptp_clock_t *ptp, uint portnum)
{
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        if (ptpPort->designatedEnabled != false) {
            ptpPort->designatedEnabled = false;
            if (vtss_ptp_p2p_state(ptp, portnum, ptpPort->p2p_state) != VTSS_RC_OK) return VTSS_RC_ERROR;
            if (ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED) {
                T_I("port disabled %d", portnum);
                vtss_ptp_state_set(VTSS_APPL_PTP_DISABLED, ptp, ptpPort);
            }
        }
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_ptp_port_linkstate(ptp_clock_t *ptp, uint portnum, bool enable)
{
    vtss_ptp_clock_mode_t clock_mode;
    bool internal_link_down = false;
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        ptpPort->linkState = enable;
        T_I("port %d,link_state = %d", portnum, enable);
        vtss_local_clock_mode_get(&clock_mode);
        internal_link_down = (clock_mode == VTSS_PTP_CLOCK_LOCKING) && ptpPort->port_config->portInternal;
        if (ptpPort->linkState && ptpPort->designatedEnabled && !internal_link_down) {
            vtss_ptp_state_set(VTSS_APPL_PTP_INITIALIZING, ptp, ptpPort);
            T_I("port %d, ptp_state = VTSS_APPL_PTP_INITIALIZING", portnum);
        }
        if (ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED && (!(ptpPort->linkState && ptpPort->designatedEnabled) ||
                internal_link_down)) {
            vtss_ptp_state_set(VTSS_APPL_PTP_DISABLED, ptp, ptpPort);
            T_I("port %d, ptp_state = VTSS_APPL_PTP_DISABLED, internal_link_down %d, initPortInternal %d", portnum, internal_link_down, ptpPort->port_config->portInternal);
        }
        return VTSS_RC_OK;
    }
    T_W("port %d, numberPorts %d", portnum, ptp->defaultDS.status.numberPorts);
    return VTSS_RC_ERROR;
}

mesa_rc vtss_ptp_port_internal_linkstate(ptp_clock_t *ptp, uint portnum)
{
    vtss_ptp_clock_mode_t clock_mode;
    bool internal_link_down;
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        T_I("port %d,internal_link_state", portnum);
        if (ptp->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT ||
                ptp->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT) {
            vtss_local_clock_mode_get(&clock_mode);
            internal_link_down = (clock_mode == VTSS_PTP_CLOCK_LOCKING) && ptpPort->port_config->portInternal;
        } else {
            internal_link_down = false;
        }
        if (ptpPort->portDS.status.portState == VTSS_APPL_PTP_DISABLED && ptpPort->linkState &&
                ptpPort->designatedEnabled && !internal_link_down) {
            vtss_ptp_state_set(VTSS_APPL_PTP_INITIALIZING, ptp, ptpPort);
            T_I("port %d, ptp_state = VTSS_APPL_PTP_INITIALIZING", portnum);
        }
        if (ptp->clock_init->cfg.protocol != VTSS_APPL_PTP_PROTOCOL_ONE_PPS && ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED && (!(ptpPort->linkState && ptpPort->designatedEnabled) ||
                internal_link_down)) {
            vtss_ptp_state_set(VTSS_APPL_PTP_DISABLED, ptp, ptpPort);
            T_I("port %d, ptp_state = VTSS_APPL_PTP_DISABLED, internal_link_down %d", portnum, internal_link_down);
        }
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_ptp_p2p_state(ptp_clock_t *ptp, uint portnum, bool enable)
{
    if (0 < portnum && portnum <= ptp->clock_init->numberEtherPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        ptpPort->p2p_state = enable;
        T_I("port %d,p2p_state = %d designated enabled=%d", portnum, enable, ptpPort->designatedEnabled);
        if (ptp->clock_init->cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE && ptpPort->p2p_state && ptpPort->designatedEnabled &&
           (ptpPort->port_config->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_P2P || ptpPort->port_config->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_COMMON_P2P)) {
            if (ptpPort->port_config->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_P2P && !cmldsEnabledOnPort(ptpPort)) {
                /* start P2P delay measurement */
                ptpPort->pDelay.clock = ptp;
                ptpPort->pDelay.ptp_port = ptpPort;
                if (ptp->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptp->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                    ptpPort->pDelay.pdelay_mech = PTP_PDELAY_MECH_802_1AS_INST;
                } else {
                    ptpPort->pDelay.pdelay_mech = PTP_PDELAY_MECH_NON_802_1AS;
                }
#if defined (VTSS_SW_OPTION_P802_1_AS)
                if ((portnum-1) < ptp->clock_init->numberEtherPorts) {
                    ptpPort->pDelay.port_cmlds->cmlds_in_use[ptp->localClockId] = false;
                }
#endif
                vtss_ptp_peer_delay_create(&ptpPort->pDelay, &ptp->ptp_pdelay, get_tag_conf(ptp, ptpPort));
                T_I("port %d, enable P2P", portnum);
            } else if (ptpPort->port_config->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_COMMON_P2P){
#if defined (VTSS_SW_OPTION_P802_1_AS)
                if ((portnum-1) < ptp->clock_init->numberEtherPorts) {
                    ptpPort->pDelay.port_cmlds->pDelay.pdelay_mech = PTP_PDELAY_MECH_802_1AS_CMLDS;
                    ptpPort->pDelay.port_cmlds->cmlds_in_use[ptp->localClockId] = true;
                    /* stop P2P delay measurements for this instance */
                    vtss_ptp_peer_delay_request_stop(&ptpPort->pDelay);
                }
#endif
            }
#if defined (VTSS_SW_OPTION_P802_1_AS)
            if (ptp->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptp->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                u32 to;
                ptpPort->gptpsm.numberGptpCapableMessageTransmissions = 0;
                ptpPort->gptpsm.gPtpCapableMessageSlowdown = FALSE;
                to = PTP_LOG_TIMEOUT(ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval);
                vtss_ptp_timer_start(&ptpPort->gptpsm.bmca_802_1as_gptp_tx_timer, to, true);
                T_D("port %d, start gptp timer", portnum);
            }
#endif
        } else {
#if defined (VTSS_SW_OPTION_P802_1_AS)
            if ((portnum-1) < ptp->clock_init->numberEtherPorts) {
                /* Common Link Delay Service usage must be unset for this clock instance */
                ptpPort->pDelay.port_cmlds->cmlds_in_use[ptp->localClockId] = false;
            }
            if (ptp->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || ptp->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                vtss_ptp_timer_stop(&ptpPort->gptpsm.bmca_802_1as_gptp_tx_timer);
                T_D("port %d, disable gptp timer", portnum);
            }
#endif
            /* stop P2P delay measurement */
            vtss_ptp_peer_delay_delete(&ptpPort->pDelay);
            T_I("port %d, disable P2P", portnum);
        }

        return VTSS_RC_OK;
    } else if (portnum <= ptp->defaultDS.status.numberPorts) {
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_ptp_get_clock_default_ds_cfg(const ptp_clock_t *ptp, vtss_appl_ptp_clock_config_default_ds_t *default_ds_cfg)
{
    *default_ds_cfg = ptp->clock_init->cfg;

    return VTSS_RC_OK;
}

mesa_rc vtss_ptp_set_clock_default_ds_cfg(ptp_clock_t *ptp, const vtss_appl_ptp_clock_config_default_ds_t *default_ds_cfg)
{
    ptp->clock_init->cfg = *default_ds_cfg;
    ptp->defaultDS.status.s_802_1as.gmCapable = (ptp->clock_init->cfg.priority1 == 255) ? FALSE : TRUE;
    defaultDSChanged(ptp);

    return VTSS_RC_OK;
}

mesa_rc vtss_ptp_get_clock_default_ds_status(const ptp_clock_t *ptp, vtss_appl_ptp_clock_status_default_ds_t *default_ds_status)
{
    *default_ds_status = ptp->defaultDS.status;
    default_ds_status->s_802_1as.sdoId = (ptp->majorSdoId<<8) | MINOR_SDO_ID;

    return VTSS_RC_OK;
}


mesa_rc vtss_ptp_set_clock_quality(ptp_clock_t *ptp, const vtss_appl_clock_quality *quality)
{
    ptp->defaultDS.status.clockQuality = *quality;
    defaultDSChanged(ptp);

    return VTSS_RC_OK;
}

/**
 * Read Clock Current Data Set
 *
 * Purpose: To obtain information regarding the Clock's Current data
 * Protocol Entity.
 *
 * \param ptp The PTP instance data.
 *
 * \param current_ds The Current Data Set
 */
void vtss_ptp_get_clock_current_ds(const ptp_clock_t *ptp, vtss_appl_ptp_clock_current_ds_t *current_ds)
{
    current_ds->stepsRemoved = ptp->currentDS.stepsRemoved;
    current_ds->offsetFromMaster = ptp->currentDS.offsetFromMaster;
    current_ds->meanPathDelay = ptp->currentDS.meanPathDelay;
#if defined (VTSS_SW_OPTION_P802_1_AS)
    current_ds->cur_802_1as = ptp->current_802_1as_ds;
#endif //(VTSS_SW_OPTION_P802_1_AS)
}

void vtss_ptp_get_clock_parent_ds(const ptp_clock_t *ptp, vtss_appl_ptp_clock_parent_ds_t *parent_ds)
{
    *parent_ds = ptp->parentDS;
}

void vtss_ptp_get_clock_timeproperties_ds(const ptp_clock_t *ptp, vtss_appl_ptp_clock_timeproperties_ds_t *timeproperties_ds)
{
    memcpy(timeproperties_ds, &ptp->timepropertiesDS, sizeof(vtss_appl_ptp_clock_timeproperties_ds_t));
}

void vtss_ptp_set_clock_timeproperties_ds(ptp_clock_t *ptp, const vtss_appl_ptp_clock_timeproperties_ds_t *timeproperties_ds)
{
    *ptp->time_prop = *timeproperties_ds;
    timepropertiesDSChanged(ptp);

}

mesa_rc vtss_ptp_get_port_status(const ptp_clock_t *ptp, uint portnum, vtss_appl_ptp_status_port_ds_t *port_ds)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        *port_ds = ptpPort->portDS.status;
        rc = VTSS_RC_OK;
    }
    return rc;
}

static mesa_rc vtss_ptp_clear_port_statistics(vtss_appl_ptp_status_port_statistics_t *port_statistics)
{
    mesa_rc rc = VTSS_RC_OK;
    memset(port_statistics, 0, sizeof(vtss_appl_ptp_status_port_statistics_t));
    return rc;
}
mesa_rc vtss_ptp_get_port_statistics(const ptp_clock_t *ptp, uint portnum, vtss_appl_ptp_status_port_statistics_t *port_statistics, BOOL clear)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        *port_statistics = ptpPort->port_statistics;
        rc = VTSS_RC_OK;
        if (clear) {
            rc = vtss_ptp_clear_port_statistics(&ptpPort->port_statistics);
        }
    }
    return rc;
}

mesa_rc vtss_ptp_set_port_cfg(ptp_clock_t *ptp, uint portnum, const vtss_appl_ptp_config_port_ds_t *port_ds)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        (void) vtss_ptp_p2p_state(ptp, portnum, ptpPort->p2p_state);
        if (port_ds->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_E2E) {
            ptpPort->portDS.status.peerMeanPathDelay = 0;
            if (!ptp->clock_init->cfg.twoStepFlag) {
                /* peerMeanPathDelay is handled in HW in onestep mode */
                vtss_1588_p2p_delay_set(ptpPort->portDS.status.portIdentity.portNumber, ptpPort->portDS.status.peerMeanPathDelay);
            } else {
                vtss_1588_p2p_delay_set(ptpPort->portDS.status.portIdentity.portNumber, 0LL);
            }
        }
        rc = VTSS_RC_OK;
        if (ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED) {
            vtss_ptp_state_set(VTSS_APPL_PTP_INITIALIZING, ptp, ptpPort);
        }
    }
    return rc;
}

bool vtss_ptp_get_port_foreign_ds(ptp_clock_t *ptp, uint portnum, i16 ix, ptp_foreign_ds_t *f_ds)
{
    bool ok = false;
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        if (ptpPort->number_foreign_records > ix) {
            f_ds->foreignmasterIdentity = ptpPort->foreignMasterDS[ix].ds.sourcePortIdentity;
            f_ds->foreignmasterClockQuality = ptpPort->foreignMasterDS[ix].ds.clockQuality;
            f_ds->foreignmasterPriority1 = ptpPort->foreignMasterDS[ix].ds.priority1;
            f_ds->foreignmasterPriority2 = ptpPort->foreignMasterDS[ix].ds.priority2;
            f_ds->foreignmasterLocalPriority = ptpPort->foreignMasterDS[ix].ds.localPriority;
            f_ds->best = (ix == ptpPort->foreign_record_best);
            f_ds->qualified = ptpPort->foreignMasterDS[ix].qualified;
            ok = true;
        }
    }
    return ok;
}

/**
 * \brief Set unicast slave configuration parameters.
 *
 */
void vtss_ptp_uni_slave_conf_set(ptp_clock_t *ptp, int ix, const vtss_appl_ptp_unicast_slave_config_t *c)
{
    ptp->slave[ix].conf_master_ip = c->ip_addr;
    ptp->slave[ix].duration = c->duration;
    ptp->slave[ix].log_msg_period = 0;
    vtss_ptp_unicast_slave_conf_upd(ptp, ix);
}

/**
 * \brief Get unicast slave configuration parameters.
 *
 */
void vtss_ptp_uni_slave_conf_get(ptp_clock_t *ptp, int ix, vtss_appl_ptp_unicast_slave_config_t *c)
{
    c->ip_addr = ptp->slave[ix].conf_master_ip;
    c->duration = ptp->slave[ix].duration;
}

/*
 * \brief Read clock slave-master communication table
 */
mesa_rc vtss_ptp_clock_unicast_table_get(const ptp_clock_t *ptp, vtss_appl_ptp_unicast_slave_table_t *uni_slave_table, int ix)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (ix < MAX_UNICAST_MASTERS_PR_SLAVE) {
        uni_slave_table->master.ip = ptp->slave[ix].master.ip;
        memcpy(&uni_slave_table->master.mac, &ptp->slave[ix].master.mac, 6);
        memcpy(uni_slave_table->sourcePortIdentity.clockIdentity, ptp->slave[ix].sourcePortIdentity.clockIdentity, sizeof(vtss_appl_clock_identity));
        uni_slave_table->sourcePortIdentity.portNumber = ptp->slave[ix].sourcePortIdentity.portNumber;
        uni_slave_table->port = ptp->slave[ix].port;
        uni_slave_table->log_msg_period = ptp->slave[ix].log_msg_period;
        uni_slave_table->comm_state = ptp->slave[ix].comm_state;
        uni_slave_table->conf_master_ip = ptp->slave[ix].conf_master_ip;
        rc = VTSS_RC_OK;
    }
    return rc;
}

mesa_rc vtss_ptp_clock_status_unicast_master_table_get(vtss_ptp_master_table_key_t key, vtss_appl_ptp_unicast_master_table_t *uni_master_table)
{
    UnicastMasterTable_t *internal_master_table;
    mesa_rc rc = VTSS_RC_ERROR;
    rc = vtss_ptp_clock_unicast_master_table_get(key, &internal_master_table);
    if (rc == VTSS_RC_OK) {
        // update uni_master_table
        memcpy(uni_master_table->mac.addr, internal_master_table->slave.mac.addr, 6);
        uni_master_table->port = internal_master_table->port;
        uni_master_table->ann_log_msg_period = internal_master_table->ansm.ann_log_msg_period;    /**< the granted Announce interval */
        // uni_master_table->ann_log_msg_period = internal_master_table->ann_req_timer.period;    /**< the granted Announce interval */
        uni_master_table->ann = (internal_master_table->ann_timer_cnt != 0);                      /**< true if sending announce messages */
        uni_master_table->log_msg_period = internal_master_table->msm.sync_log_msg_period;        /**< the granted sync interval */
        // uni_master_table->log_msg_period = internal_master_table->sync_req_timer.period;       /**< the granted sync interval */
        uni_master_table->sync = true;
    }
    return rc;
}

/* Set debug_mode. */
bool vtss_ptp_debug_mode_set(ptp_clock_t *ptp, int debug_mode, BOOL has_log_to_file, BOOL has_control, u32 log_time)
{
    if (ptp->ssm.logFile != NULL) {  // If logFile descriptor is != NULL a log file is already open. Close it before doing anything else.
        fclose(ptp->ssm.logFile);
    }
    ptp->ssm.debugMode = debug_mode;
    if (debug_mode) {
        // start a new log and save the start time
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Begin logging for %d seconds", log_time);
        vtss_ptp_timer_start(&ptp->ssm.log_timer, PTP_LOG_TIMEOUT(0) * log_time, FALSE);

        if (has_log_to_file) {
            char log_file_name[20];
            snprintf(log_file_name, sizeof(log_file_name), "/tmp/ptp_log_%d.tpk", ptp->localClockId);

            ptp->ssm.logFile = fopen(log_file_name, "w");
        } else {
            ptp->ssm.logFile = fopen("/proc/self/fd/1", "a");   // Note: There is no device file for stdout on the target. Assuming handle 1 is stdout.
        }
        ptp->ssm.keep_control = has_control;
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Stop logging");
        ptp->ssm.logFile = NULL;
        ptp->ssm.activeDebug = false;
        vtss_ptp_timer_stop(&ptp->ssm.log_timer);
    }
    return true;
}

/* Get debug_mode. */
bool vtss_ptp_debug_mode_get(ptp_clock_t *ptp, vtss_ptp_logmode_t *log_mode)
{
    vtss_ptp_sys_timer_t *t     = &ptp->ssm.log_timer;
    u64                  now_us = vtss::uptime_microseconds();
    u32                  time_left_secs;

    if (now_us > t->timeout_us) {
        time_left_secs = 0;
    } else {
        time_left_secs = (t->timeout_us - now_us) / 1000000LLU;
    }

    log_mode->debug_mode   = ptp->ssm.debugMode;
    log_mode->file_open    = (ptp->ssm.logFile != NULL) ? TRUE : FALSE;
    log_mode->keep_control = ptp->ssm.keep_control;
    log_mode->log_time     = t->period_us / 1000000LLU; // log_time is measured in seconds
    log_mode->time_left    = time_left_secs;
    return true;
}

bool vtss_ptp_log_delete(ptp_clock_t *ptp)
{
    if (ptp->ssm.logFile != NULL) {  // If logFile descriptor is != NULL a log file is already open. Close it before doing anything else.
        fclose(ptp->ssm.logFile);
        ptp->ssm.logFile = NULL;
    }
    ptp->ssm.debugMode = 0;

    char log_file_name[20];
    snprintf(log_file_name, sizeof(log_file_name), "/tmp/ptp_log_%d.tpk", ptp->localClockId);

    return unlink(log_file_name) ? 0 : 1;
}

/*
 * Enable/disable the wireless variable tx delay feature for a port.
 */
bool vtss_ptp_port_wireless_delay_mode_set(ptp_clock_t *ptp, bool enable, int portnum)
{
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        ptpPort->wd.enable = enable;
        return true;
    }
    return false;
}

/*
 * Get Enable/disable mode for the wireless variable tx delay feature for a port.
 */
bool vtss_ptp_port_wireless_delay_mode_get(ptp_clock_t *ptp, bool *enable, int portnum)
{
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        *enable = ptpPort->wd.enable;
        return true;
    }
    return false;
}

/*
 * Pre notification sent from the wireless modem transmitter before the delay is changed.
 */
bool vtss_ptp_port_wireless_delay_pre_notif(ptp_clock_t *ptp, int portnum)
{
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        ptpPort->wd.delay_pre = 1;
        return true;
    }
    return false;
}

/*
 * Set the delay configuration, sent from the wireless modem transmitter whenever the delay is changed.
 */
bool vtss_ptp_port_wireless_delay_set(ptp_clock_t *ptp, const vtss_ptp_delay_cfg_t *delay_cfg, int portnum)
{
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        ptpPort->wd.base_delay = delay_cfg->base_delay;
        ptpPort->wd.incr_delay = delay_cfg->incr_delay;
        ptpPort->wd.delay_pre = 2;
        return true;
    }
    return false;
}

/*
 * Get the delay configuration.
 */
bool vtss_ptp_port_wireless_delay_get(ptp_clock_t *ptp, vtss_ptp_delay_cfg_t *delay_cfg, int portnum)
{
    if (0 < portnum && portnum <= ptp->defaultDS.status.numberPorts) {
        PtpPort_t *ptpPort = &ptp->ptpPort[portnum-1];
        delay_cfg->base_delay = ptpPort->wd.base_delay;
        delay_cfg->incr_delay = ptpPort->wd.incr_delay;
        return true;
    }
    return false;
}

static vtss_appl_ptp_slave_clock_state_t internal_state_to_slave_state(vtss_slave_clock_state_t clock_state, bool holdover_ok)
{
    switch (clock_state) {
        case     VTSS_PTP_SLAVE_CLOCK_FREERUN:        return holdover_ok ? VTSS_APPL_PTP_SLAVE_CLOCK_STATE_HOLDOVER : VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREERUN;
        case     VTSS_PTP_SLAVE_CLOCK_F_SETTLING:
        case     VTSS_PTP_SLAVE_CLOCK_FREQ_LOCK_INIT:
        case     VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING:   return VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREQ_LOCKING;
        case     VTSS_PTP_SLAVE_CLOCK_P_SETTLING:
        case     VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING:  return VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKING;
        case     VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED:    return VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREQ_LOCKED;
        case     VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED:   return VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKED;
        case     VTSS_PTP_SLAVE_CLOCK_HOLDOVER:       return VTSS_APPL_PTP_SLAVE_CLOCK_STATE_HOLDOVER;
        case     VTSS_PTP_SLAVE_CLOCK_RECOVERING:     return VTSS_APPL_PTP_SLAVE_CLOCK_STATE_HOLDOVER;
        default:                                      return VTSS_APPL_PTP_SLAVE_CLOCK_STATE_INVALID;
    }
}

void vtss_ptp_get_clock_slave_ds(const ptp_clock_t *ptp, vtss_appl_ptp_clock_slave_ds_t *slave_ds)
{
    vtss_ptp_servo_status_t s;
    ptp->ssm.servo->clock_servo_status(ptp->clock_init->cfg.clock_domain, &s);

    slave_ds->port_number = ptp->slavePort;
    if (ptp->slavePort || ptp->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {
        slave_ds->slave_state = internal_state_to_slave_state(ptp->ssm.clock_state, s.holdover_ok);
    } else  {
        slave_ds->slave_state = internal_state_to_slave_state(VTSS_PTP_SLAVE_CLOCK_FREERUN, s.holdover_ok);
    }
    slave_ds->holdover_stable = s.holdover_ok;
    slave_ds->holdover_adj = s.holdover_adj;
}

const char *vtss_ptp_slave_state_2_text(vtss_appl_ptp_slave_clock_state_t s)
{
    switch (s) {
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREERUN:        return "FREERUN";
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREQ_LOCKING:   return "FREQ_LOCKING";
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREQ_LOCKED:    return "FREQ_LOCKED";
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKING:  return "PHASE_LOCKING";
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKED:   return "PHASE_LOCKED";
        case VTSS_APPL_PTP_SLAVE_CLOCK_STATE_HOLDOVER:       return "HOLDOVER";
        default:                                             return "UNKNOWN";
    }
}

char *vtss_ptp_ho_state_2_text(bool stable, i64 adj, char *str, bool details)
{
    if (stable && details) {
        sprintf(str,"%d.%03ld", (i32)adj / 1000, labs((i32)adj % 1000));
    } else if (stable) {
        sprintf(str,"%d.%ld", (i32)adj / 1000, labs(((i32)adj % 1000))/100);
    } else {
        sprintf(str,"N.A.");
    }
    return str;
}
