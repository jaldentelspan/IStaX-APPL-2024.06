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

#include "vtss_ptp_os.h"
#include "vtss_ptp_api.h"
#include "vtss_ptp_internal_types.h"
#include "vtss_ptp_pack_unpack.h"
#include "vtss/appl/ptp.h"
#include "vtss_ptp_802_1as_bmca.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_tod_api.h"

#if defined (VTSS_SW_OPTION_P802_1_AS)

#define RATE_TO_U32_CONVERT_FACTOR (1LL<<41)
#if 0
static void dump_ClockDataSet(const ClockDataSet *ds, const vtss_appl_ptp_port_identity *rec)
{
    char str1 [40];
    char str2 [40];

    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"ClockDataSet: %s %d %d %d %s %d %d\n",
           ClockIdentityToString(ds->grandmasterIdentity, str1),
           ds->priority1, ds->priority2,
           ds->stepsRemoved,
           ClockIdentityToString(ds->sourcePortIdentity.clockIdentity, str2),
           ds->sourcePortIdentity.portNumber,
           ds->localPriority);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"Receive port: %s %d\n",
           ClockIdentityToString(rec->clockIdentity, str1),
           rec->portNumber);
}
#endif

/**
 * compare two priority vectors's
 * \return value as memcmp i.e.
 * a < b => <0, a == b => 0, a > b => >0
 */
extern void vtss_ptp_bmca_gptp_transmit_timer(vtss_ptp_sys_timer_t *timer, void *t);

static int system_identity_cmp(const vtss_ptp_system_identity_t* a, const vtss_ptp_system_identity_t* b)
{
    int cmp;

    if (a->priority1 > b->priority1)
        return 1;
    else if (a->priority1 < b->priority1)
        return -1;

    if (a->clockQuality.clockClass > b->clockQuality.clockClass)
        return 1;
    else if (a->clockQuality.clockClass < b->clockQuality.clockClass)
        return -1;

    if (a->clockQuality.clockAccuracy > b->clockQuality.clockAccuracy)
        return 1;
    else if (a->clockQuality.clockAccuracy < b->clockQuality.clockAccuracy)
        return -1;

    if (a->clockQuality.offsetScaledLogVariance > b->clockQuality.offsetScaledLogVariance)
        return 1;
    else if (a->clockQuality.offsetScaledLogVariance < b->clockQuality.offsetScaledLogVariance)
        return -1;

    if (a->priority2 > b->priority2)
        return 1;
    else if (a->priority2 < b->priority2)
        return -1;

    cmp = memcmp(a->clockIdentity, b->clockIdentity, CLOCK_IDENTITY_LENGTH);
    if (cmp > 0)
        return 1;
    else if (cmp < 0)
        return -1;
    else
        return 0;
}

/**
 * compare two priority vectors's
 * \return value as memcmp i.e.
 * a < b => <0, a == b => 0, a > b => >0
 */
static int priority_vector_cmp(const vtss_ptp_priority_vector_t* a, const vtss_ptp_priority_vector_t* b)
{
    int cmp;
    if ((cmp = system_identity_cmp(&a->systemIdentity, &b->systemIdentity))) {
        return cmp;
    }
    if (a->stepsRemoved != b->stepsRemoved) {
        return a->stepsRemoved > b->stepsRemoved ? 1 : -1;
    }
    if ((cmp = PortIdentitycmp(&a->sourcePortIdentity, &b->sourcePortIdentity))) {
        return cmp;
    }
    if (a->portNumber != b->portNumber) {
        return a->portNumber > b->portNumber ? 1 : -1;
    }
    return 0;
}

static void vtss_ptp_port_bmca_updtinfo(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, bool updateDownStream)
{
    ptpPort->bmca_802_1as.portPriority = ptpPort->bmca_802_1as.masterPriority;
    
    ptpPort->bmca_802_1as.infoIs = VTSS_PTP_INFOIS_MINE;
    // Update Announce info to downstream neighbors.
    if (updateDownStream) {
        vtss_ptp_timer_start(&ptpPort->ansm.ann_timer, 1, true);
    }
}

static void update_802_1as_current_ds(ptp_clock_t *ptpClock)
{
    u32 system_time = u32(vtss_current_time()/10); //system time in units of 0,01 sec, not sure what this can be used for
    ptpClock->current_802_1as_ds.gmChangeCount++;
    T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"system time %d", system_time);

    ptpClock->current_802_1as_ds.gmTimeBaseIndicator++;
    ptpClock->current_802_1as_ds.timeOfLastGMChangeEvent = system_time;
    ptpClock->current_802_1as_ds.timeOfLastGMPhaseChangeEvent = system_time;
    ptpClock->current_802_1as_ds.timeOfLastGMFreqChangeEvent = system_time;
}

/**
 * Common variables pr clock  (kept in ptp_clock_t):
 *      timeProperties: The Time properties to be sent out in announce messages.
 *      sysTimeProperties: The time properties configured for this node.
 *      systemPriorityVector: Ny own priorityVector.
 *      gmPriorityVector: Received priorityVector from current grandmaster.
 * Common variables pr port (kept in PtpPort_t):
 *      masterPriority: priority vector that holds the data used in announce when the port is in master state.
 *      portPriority: latest received announce information or masterPriority in case of timeout.
 *      annTimeProperties:latest received announce information.
 *      infoIs: an enum that takes the values Received, Mine, Aged, or Disabled, to indicate the origin and state of the port's time-synchronization spanning tree information
 *      portStepsRemoved: the value of stepsRemoved for the port.
 *      portDS.status.s_802_1as.portRole: Must be the same as selectedRole in the port_role_Selection state machine
 *
 * Implement PortAnnounceTransmit state machine as defined in P802.1AS-Rev/D3.0 clause 10.3.13.
 * param ptpClock [IN/OUT]  The number of the PTP clock instance for which the configuration shall be get.
 * param ptpPort [IN/OUT]   The number of the port for which to get the portDS.
 * param buf_handle [IN]    A pointer to a structure in which the configuration shall be returned.
 * param header [IN]        Decoded message header
 * return forwarded         Return false if the message is not forwarded.
 *
 * trigger: announceSentTimeExpired or newInfo (i.e. announce info changed)
 * 
 * if (portRole == MasterPort)
 *      messagePriorityVector = masterPriority[port]
 *      stepsRemoved = masterStepsRemoved
 *      set messageTimeProperties to TimeProperties
 *      update SequenceId
 *      add PathTrace TLV
 *      transmit announce packet
 *
 */

/**
 * Implement PortRoleSelection state machine as defined in P802.1AS-Rev/D3.0 clause 10.3.12.
 * param ptpClock [IN/OUT]  The number of the PTP clock instance for which the configuration shall be get.
 * param ptpPort [IN/OUT]   The number of the port for which to get the portDS.
 * param buf_handle [IN]    A pointer to a structure in which the configuration shall be returned.
 * param header [IN]        Decoded message header
 * return forwarded         Return false if the message is not forwarded.
 *
 * Local variables:
 *      gmPathPriority
 *      lastGmPriority ?
 *      gmPriority
 * trigger: systemIdentityChange or reselect
 * 
 * For each port:
 *      gmPathPriority = portPriority (stepRemoved += 1)
 * lastGmPriority = gmPriority
 * gmPriority = the best of gmPathPriority for all ports and the systemPriorityVector
 * if (gmPriority is a port) 
 *      TimeProperties = annTimeProperties[port]
 *      masterStepsRemoved = messageStepsRemoved
 * else 
 *      TimeProperties = sysTimeProperties
 *      masterStepsRemoved = 0
 * For each port:
 *      masterPriority[port] = {gmPriority.rootsystemIdentity : gmPriority.stepsRemoved : {my clockIdentity:port} : port} (P802.1AS-Rev/D3.0 clause 10.3.5)
 * Update port roles for all ports:
 *      Disabled ports are unchanged
 *      if (infoIs == Aged || infoIs == Mine) selectedRole = MasterPort; updtInfo = true
 *      if (infoIs == Received && gmPriority = portPriority) selectedRole = SlavePort; updtInfo = false
 *      if (infoIs == Received && gmPriority != portPriority && gmPriority is not better than portPriority) selectedRole = PassivePort; updtInfo = false
 *      if (infoIs == Received && gmPriority != portPriority && gmPriority is better than portPriority) selectedRole = MasterPort; updtInfo = true
 * gmPresent = gmPriorityVector.priority1 < 255
 * selectedRole[0] = (any other slave port) ? PassivePort : SlavePort (Slaveport means this node is GM)
 *      
 */
// With virtual port feature inclusion, BMCA changes done to include virtual port in BMCA election.
// Depending on notMaster option, ptpPort moves to passive state.
// 'updateInfo' input indicates updates to be forwarded downstream.

static void vtss_ptp_port_role_selection(ptp_clock_t *ptpClock, bool updateInfo)
{
    vtss_ptp_priority_vector_t gmPathPriority;
    vtss_ptp_priority_vector_t gmPriority;
    u32 portidx, slave_port = 0;
    PtpPort_t *ptpSlavePort = nullptr;
    PtpPort_t *ptpPort; 
    int cmp, cmp1;
    char str1 [300];

    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"vtss_ptp_port_role_selection");
    gmPriority = ptpClock->bmca_802_1as.systemPriorityVector;
    for (portidx = 0; portidx < ptpClock->clock_init->numberPorts; portidx++) {
        ptpPort = &ptpClock->ptpPort[portidx];
        if (ptpPort->bmca_802_1as.infoIs == VTSS_PTP_INFOIS_RECEIVED) {
            gmPathPriority = ptpPort->bmca_802_1as.portPriority;
            gmPathPriority.stepsRemoved++;
            T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"gmPriority %s", vtss_ptp_PriorityVectorToString(&gmPriority, str1, sizeof(str1)));
            T_NG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"gmPathPriority %s", vtss_ptp_PriorityVectorToString(&gmPathPriority, str1, sizeof(str1)));
            if (memcmp(gmPathPriority.sourcePortIdentity.clockIdentity, ptpClock->bmca_802_1as.systemPriorityVector.sourcePortIdentity.clockIdentity, sizeof(gmPathPriority.sourcePortIdentity.clockIdentity))) {
                cmp = priority_vector_cmp(&gmPriority, &gmPathPriority);
                if (cmp > 0) { // gmPathPriority better than gmPriority
                    gmPriority = gmPathPriority;
                    ptpSlavePort = ptpPort;
                    slave_port = ptpPort->portDS.status.portIdentity.portNumber;
                }
            }
        }
    }
    ptpClock->slavePort = slave_port;
    ptpClock->bmca_802_1as.gmPriorityVector = gmPriority;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"slavePort %d", slave_port);
    if (slave_port != 0 && ptpSlavePort != 0) {
        ptpClock->timepropertiesDS = ptpSlavePort->bmca_802_1as.annTimeProperties;
        //masterStepsRemoved = messageStepsRemoved;
        // Update Parent data set in original data structure */
        memcpy(&ptpClock->parentDS.parentPortIdentity, &ptpClock->bmca_802_1as.gmPriorityVector.sourcePortIdentity, sizeof(vtss_appl_ptp_port_identity));
        memcpy(ptpClock->parentDS.grandmasterIdentity, ptpClock->bmca_802_1as.gmPriorityVector.systemIdentity.clockIdentity, sizeof(vtss_appl_clock_identity));
        ptpClock->parentDS.grandmasterClockQuality = ptpClock->bmca_802_1as.gmPriorityVector.systemIdentity.clockQuality;
        ptpClock->parentDS.grandmasterPriority1 = ptpClock->bmca_802_1as.gmPriorityVector.systemIdentity.priority1;
        ptpClock->parentDS.grandmasterPriority2 = ptpClock->bmca_802_1as.gmPriorityVector.systemIdentity.priority2;
        ptpClock->currentDS.stepsRemoved = ptpClock->bmca_802_1as.gmPriorityVector.stepsRemoved;
        ptpClock->parentRecPort = ptpSlavePort;
    } else {
        // me grandmaster
        ptpClock->timepropertiesDS = ptpClock->bmca_802_1as.sysTimeProperties;
        // Update Parent data set in original data structure */
        memcpy(ptpClock->parentDS.parentPortIdentity.clockIdentity, ptpClock->defaultDS.status.clockIdentity, CLOCK_IDENTITY_LENGTH);
        ptpClock->parentDS.parentPortIdentity.portNumber = 0;
        memcpy(ptpClock->parentDS.grandmasterIdentity, ptpClock->defaultDS.status.clockIdentity, CLOCK_IDENTITY_LENGTH);
        ptpClock->parentDS.grandmasterClockQuality = ptpClock->defaultDS.status.clockQuality;
        ptpClock->announced_clock_quality = ptpClock->defaultDS.status.clockQuality;
        ptpClock->parentDS.grandmasterPriority1 = ptpClock->clock_init->cfg.priority1;
        ptpClock->parentDS.grandmasterPriority2 = ptpClock->clock_init->cfg.priority2;
        ptpClock->parentRecPort = NULL;
    }
    ptpClock->bmca_802_1as.timeProperties = ptpClock->timepropertiesDS;

    for (portidx = 0; portidx < ptpClock->clock_init->numberPorts; portidx++) {
        ptpPort = &ptpClock->ptpPort[portidx];
        // update master priority
        ptpPort->bmca_802_1as.masterPriority.systemIdentity = gmPriority.systemIdentity;
        ptpPort->bmca_802_1as.masterPriority.stepsRemoved = gmPriority.stepsRemoved;
        ptpPort->bmca_802_1as.masterPriority.sourcePortIdentity = ptpPort->portDS.status.portIdentity;
        ptpPort->bmca_802_1as.masterPriority.portNumber = ptpPort->portDS.status.portIdentity.portNumber;
        
        switch (ptpPort->bmca_802_1as.infoIs) {
            case VTSS_PTP_INFOIS_AGED:
            case VTSS_PTP_INFOIS_MINE:
                if ((ptpPort->virtual_port || ptpPort->port_config->notMaster) && ptpPort->portDS.status.portState != VTSS_APPL_PTP_PASSIVE) {
                    ptpPort->portDS.status.s_802_1as.portRole = VTSS_APPL_PTP_PORT_ROLE_PASSIVE_PORT;
                    vtss_ptp_state_set(VTSS_APPL_PTP_PASSIVE, ptpClock, ptpPort);
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"port %d enters passive role", ptpPort->portDS.status.portIdentity.portNumber);
                } else if (ptpPort->portDS.status.portState != VTSS_APPL_PTP_MASTER) {
                    ptpPort->portDS.status.s_802_1as.portRole = VTSS_APPL_PTP_PORT_ROLE_MASTER_PORT;
                    vtss_ptp_state_set(VTSS_APPL_PTP_MASTER, ptpClock, ptpPort);
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"port %d enters master role", ptpPort->portDS.status.portIdentity.portNumber);
                }
                vtss_ptp_port_bmca_updtinfo(ptpClock, ptpPort, updateInfo);
                break;
            case VTSS_PTP_INFOIS_RECEIVED:
                if (ptpPort == ptpSlavePort) {
                    if (ptpPort->portDS.status.portState != VTSS_APPL_PTP_UNCALIBRATED &&
                        ptpPort->portDS.status.portState != VTSS_APPL_PTP_SLAVE) {
                        /* Set uncalibrated state and wait till the rate ratio is 0.1 ppm to set slave state. */
                        vtss_ptp_state_set(VTSS_APPL_PTP_UNCALIBRATED, ptpClock, ptpPort);
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"port %d enters slave role in uncalib state", ptpPort->portDS.status.portIdentity.portNumber);
                    }
                    ptpPort->portDS.status.s_802_1as.portRole = VTSS_APPL_PTP_PORT_ROLE_SLAVE_PORT;
                } else {
                    cmp = priority_vector_cmp(&gmPriority, &ptpPort->bmca_802_1as.portPriority);
                    cmp1 = priority_vector_cmp(&ptpPort->bmca_802_1as.masterPriority, &ptpPort->bmca_802_1as.portPriority);
                    if ((cmp != 0 && cmp1 >= 0) || (ptpPort->virtual_port || ptpPort->port_config->notMaster)) {
                        if (ptpPort->portDS.status.portState != VTSS_APPL_PTP_PASSIVE) {
                            ptpPort->portDS.status.s_802_1as.portRole = VTSS_APPL_PTP_PORT_ROLE_PASSIVE_PORT;
                            vtss_ptp_state_set(VTSS_APPL_PTP_PASSIVE, ptpClock, ptpPort);
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"port %d enters pasive role (cmp = %d, cmp1 = %d)", ptpPort->portDS.status.portIdentity.portNumber, cmp, cmp1);
                        }
                    } else {
                        if (ptpPort->portDS.status.portState != VTSS_APPL_PTP_MASTER) {
                            ptpPort->portDS.status.s_802_1as.portRole = VTSS_APPL_PTP_PORT_ROLE_MASTER_PORT;
                            vtss_ptp_state_set(VTSS_APPL_PTP_MASTER, ptpClock, ptpPort);
                            ptpPort->bmca_802_1as.infoIs = VTSS_PTP_INFOIS_MINE;
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"port %d enters master role (cmp = %d, cmp1 = %d)", ptpPort->portDS.status.portIdentity.portNumber, cmp, cmp1);
                        }
                        vtss_ptp_port_bmca_updtinfo(ptpClock, ptpPort, updateInfo);
                    }
                }
                break;
            case VTSS_PTP_INFOIS_DISABLED:
                T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"port is disabled");
                break;
        }
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"port_no %d, infoIs %s, portRole %s", ptpPort->portDS.status.portIdentity.portNumber, vtss_ptp_InfoIsToString(ptpPort->bmca_802_1as.infoIs), vtss_ptp_PortRoleToString(ptpPort->portDS.status.s_802_1as.portRole));
    }    
    ptpClock->bmca_802_1as.gmPresent = ptpClock->bmca_802_1as.gmPriorityVector.systemIdentity.priority1 < 255 ? true : false;
    //TBD: selectedRole[0] = (any other slave port) ? PassivePort : SlavePort (Slaveport means this node is GM)
    // if this node is grandmaster, the path trace is set to this node only.
}

/**
 * Implement PortAnnounceInformation state machine as defined in P802.1AS-Rev/D3.0 clause 10.3.11.
 * param ptpClock [IN/OUT]  The number of the PTP clock instance for which the announce message is received.
 * param ptpPort [IN/OUT]   The number of the port for which the announce message is received.
 * param buf_handle [IN]    A pointer to a handle to the received message.
 * param header [IN]        Decoded message header
 * return forwarded         Return false if the message is not forwarded.
 *
 * messagePriority, messageStepsRemoved and messageTimeProperties are received in the Announce message
 * 
 * Compare messagePriority and portPriority
 *      case    Repeated master port (messagePriority equal to portPriority)
 *          no action
 *      case    Superior master port (messagePriority better than portPriority)
 *          announceReceiptTimeoutInterval = announceReceiptTimeout*2**logMessageInterval sec
 *          syncReceiptTimeoutInterval = syncReceiptTimeout*2**initialLogSyncInterval sec
 *          Restart SyncReceiptTimeoutTimer(syncReceiptTimeoutInterval)
 *          infoIs = Received;
 *          reselect (trigger PortRoleSelection)
 *      case    Inferior master or other port (otherwise)
 *          reselect (trigger PortRoleSelection)
 * portPriority = messagePriority
 * portStepsRemoved = messageStepsRemoved
 * annTimeProperties = messageTimeProperties
 * Restart AnnounceReceiptTimeoutTimer(announceReceiptTimeoutInterval)
 * 
 *
 * announceReceiptTimeout || (syncReceiptTimeout && gmPresent):
 *      if(infoIs == Received)
 *          reselect (trigger PortRoleSelection)
 *          infoIs = Aged
 *
 * selected (always set when the port roles are updated)  && updtInfo (trigger from PortRoleSelection):
 *      portPriority = masterPriority
 *      portStepsRemoved = masterStepsRemoved
 *      infoIs = Mine
 *      vtss_ptp_port_announce_transmit (~newInfo)
 *
 */

bool vtss_ptp_port_announce_information(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, ptp_tx_buffer_handle_t *buf_handle, MsgHeader *header, ptp_path_trace_t *path_sequence)
{
    MsgAnnounce announce;
    vtss_ptp_priority_vector_t messagePriority;
    i32 announceReceiptTimeoutInterval;
    i32 syncReceiptTimeoutInterval;
    int cmp;
    char str1 [300];

    if (ptpPort->portDS.status.s_802_1as.portRole == VTSS_APPL_PTP_PORT_ROLE_DISABLED_PORT) {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"announce ignored while portRole is Disabled");
    // Only do this check in two-step mode as follow_up does not exist in one-step
    } else if (ptpClock->clock_init->cfg.twoStepFlag && ((ptpPort->parent_last_follow_up_sequence_number != ptpPort->parent_last_sync_sequence_number && ptpPort->awaitingFollowUp == false)
        || (ptpPort->parent_last_follow_up_sequence_number+1 != ptpPort->parent_last_sync_sequence_number && ptpPort->awaitingFollowUp == true))) {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"announce ignored since sync %d and follow_up %d do not match", ptpPort->parent_last_sync_sequence_number,ptpPort->parent_last_follow_up_sequence_number);
    } else {
        vtss_ptp_unpack_announce_msg(buf_handle->frame + buf_handle->header_length, &announce);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"announce: gmPri1: %d, gmPri2 %d", announce.grandmasterPriority1, announce.grandmasterPriority2);

        // compose message priority
        messagePriority.systemIdentity.priority1 = announce.grandmasterPriority1;
        messagePriority.systemIdentity.clockQuality = announce.grandmasterClockQuality;
        messagePriority.systemIdentity.priority2 = announce.grandmasterPriority2;
        memcpy(messagePriority.systemIdentity.clockIdentity, announce.grandmasterIdentity, sizeof(messagePriority.systemIdentity.clockIdentity));
        messagePriority.stepsRemoved = announce.stepsRemoved;
        messagePriority.sourcePortIdentity = header->sourcePortIdentity;
        messagePriority.portNumber = ptpPort->portDS.status.portIdentity.portNumber;
        // compare received priority to current saved portPriority vector
        cmp = priority_vector_cmp(&messagePriority, &ptpPort->bmca_802_1as.portPriority);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"messagePriority %s", vtss_ptp_PriorityVectorToString(&messagePriority, str1, sizeof(str1)));
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"portPriority %s", vtss_ptp_PriorityVectorToString(&ptpPort->bmca_802_1as.portPriority, str1, sizeof(str1)));
        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"cmp: %d", cmp);
        //ptpPort->bmca_802_1as.portPriority = messagePriority;
        //ptpPort->bmca_802_1as.portPriority.stepsRemoved++;
        // portStepsRemoved = messageStepsRemoved is included in port_priority (portPriority.stepsRemoved)
        // update announced timeproperties
        ptpPort->bmca_802_1as.annTimeProperties.currentUtcOffset = announce.currentUtcOffset;
        ptpPort->bmca_802_1as.annTimeProperties.currentUtcOffsetValid = getFlag(header->flagField[1], PTP_CURRENT_UTC_OFFSET_VALID);
        ptpPort->bmca_802_1as.annTimeProperties.leap59 = getFlag(header->flagField[1], PTP_LI_59);
        ptpPort->bmca_802_1as.annTimeProperties.leap61 = getFlag(header->flagField[1], PTP_LI_61);
        ptpPort->bmca_802_1as.annTimeProperties.timeTraceable = getFlag(header->flagField[1], PTP_TIME_TRACEABLE);
        ptpPort->bmca_802_1as.annTimeProperties.frequencyTraceable = getFlag(header->flagField[1], PTP_FREQUENCY_TRACEABLE);
        ptpPort->bmca_802_1as.annTimeProperties.ptpTimescale = getFlag(header->flagField[1], PTP_PTP_TIMESCALE);
        ptpPort->bmca_802_1as.annTimeProperties.timeSource = announce.timeSource;


        ptpPort->bmca_802_1as.infoIs = VTSS_PTP_INFOIS_RECEIVED;
        ptpPort->bmca_802_1as.portPriority = messagePriority;
        if (cmp == 0) { /* Repeated master port */
            if ( !SEQUENCE_ID_CHECK(header->sequenceId, ptpPort->parent_last_announce_sequence_number)) {
                T_WG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"portNo %d, sequence number mismatch header seq-id %d expected %d", ptpPort->portDS.status.portIdentity.portNumber, header->sequenceId, ptpPort->parent_last_announce_sequence_number);
            }
            vtss_ptp_port_role_selection(ptpClock, false);
            T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"portNo %d, Repeated master port", ptpPort->portDS.status.portIdentity.portNumber);
        } else if (cmp <  0) { /* Superior master port */
            ptpClock->ssm.first_sync_packet = true;
            update_802_1as_current_ds(ptpClock);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"portNo %d, Superior master port", ptpPort->portDS.status.portIdentity.portNumber);
            syncReceiptTimeoutInterval = PTP_LOG_TIMEOUT(ptpPort->portDS.status.s_802_1as.currentLogSyncInterval) * ptpPort->port_config->c_802_1as.syncReceiptTimeout;
            T_IG(VTSS_TRACE_GRP_PTP_BASE_TC,"SYNC timer restarted, timeout = %d", syncReceiptTimeoutInterval);
            vtss_ptp_timer_start(&ptpPort->bmca_802_1as_sync_timer, syncReceiptTimeoutInterval, false); // Restart SyncReceiptTimeoutTimer

            vtss_ptp_port_role_selection(ptpClock, true);
        } else { /* Inferior master or other port */
            T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"portNo %d, Inferior master port", ptpPort->portDS.status.portIdentity.portNumber);
            vtss_ptp_port_role_selection(ptpClock, false);
        }

        ptpPort->parent_last_announce_sequence_number = header->sequenceId;
        announceReceiptTimeoutInterval = PTP_LOG_TIMEOUT(header->logMessageInterval) * ptpPort->port_config->announceReceiptTimeout;
        vtss_ptp_timer_start(&ptpPort->announce_to_timer, announceReceiptTimeoutInterval, false);

        if (ptpClock->slavePort == 0) {
            ptpClock->path_trace.size = 0; // note: this node's path trace is appended in the path trace insertion function
            ptpClock->parentDS.par_802_1as.cumulativeRateRatio = 0;
        } else if (ptpClock->slavePort == ptpPort->portDS.status.portIdentity.portNumber) {
            ptpClock->path_trace = *path_sequence;
        }
    }
    return false;
}

/*
 * Announce receipt timeout timer
 */
/*lint -esym(459, vtss_ptp_bmca_announce_to_timer) */
static void vtss_ptp_bmca_announce_to_timer(vtss_ptp_sys_timer_t *timer, void *t)
{
    PtpPort_t *ptpPort = (PtpPort_t *)t;
    T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"announce timeout, port %d", ptpPort->portDS.status.portIdentity.portNumber);
    if (ptpPort->bmca_802_1as.infoIs == VTSS_PTP_INFOIS_RECEIVED) {
        ptpPort->bmca_802_1as.infoIs = VTSS_PTP_INFOIS_AGED;
        ptpPort->port_statistics.announceReceiptTimeoutCount++;
        vtss_ptp_port_role_selection(ptpPort->parent, false);
        // clear path trace if this node enters GM mode
        if (ptpPort->parent->slavePort == 0) {
            ptpPort->parent->path_trace.size = 0; // note: this node's path trace is appended in the path trace insertion function
            ptpPort->parent->parentDS.par_802_1as.cumulativeRateRatio = 0;
        }
    }
}

/*
 * Sync receipt timeout timer
 */
/*lint -esym(459, vtss_ptp_bmca_sync_to_timer) */
static void vtss_ptp_bmca_sync_to_timer(vtss_ptp_sys_timer_t *timer, void *t)
{
    PtpPort_t *ptpPort = (PtpPort_t *)t;
    T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"sync timeout, port %d", ptpPort->portDS.status.portIdentity.portNumber);

    // sync packets are only expected in slave state
    if (ptpPort->portDS.status.s_802_1as.portRole == VTSS_APPL_PTP_PORT_ROLE_SLAVE_PORT && ptpPort->parent->bmca_802_1as.gmPresent && ptpPort->bmca_802_1as.infoIs == VTSS_PTP_INFOIS_RECEIVED) {
        ptpPort->bmca_802_1as.infoIs = VTSS_PTP_INFOIS_AGED;
        ptpPort->port_statistics.syncReceiptTimeoutCount++;
        vtss_ptp_port_role_selection(ptpPort->parent, false);
        // clear path trace if this node enters GM mode
        if (ptpPort->parent->slavePort == 0) {
            ptpPort->parent->path_trace.size = 0; // note: this node's path trace is appended in the path trace insertion function
            ptpPort->parent->parentDS.par_802_1as.cumulativeRateRatio = 0;
        }
    }
}


/*
 * gptp rx timeout timer
 */
/*lint -esym(459, vtss_ptp_bmca_sync_to_timer) */
static void vtss_ptp_bmca_gptp_timeout_timer(vtss_ptp_sys_timer_t *timer, void *t)
{
    PtpPort_t *ptpPort = (PtpPort_t *)t;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA," Set ascapable to FALSE as gptp capable tlv rx timeout on  port %d ", ptpPort->portDS.status.portIdentity.portNumber);
    ptpPort->neighborGptpCapable = FALSE;
}
static void vtss_ptp_802_1as_site_repeated_syncsync_event_transmitted(void *context, uint portnum, uint32_t ts_id, u64 tx_time)
{
    ptp_clock_t *ptpClock = (ptp_clock_t *)context;
    PtpPort_t *ptpPort = NULL;
    mesa_timeinterval_t correctionValue;
    mesa_timestamp_t    mesa_tx_time;
    size_t buffer_size;
    size_t header_size;
    u8 *frame;
    u8 *buf;
    vtss_ptp_tag_t tag;
    
    if (ptpClock != NULL) {
        ptpPort = &ptpClock->ptpPort[portnum-1];
        // it is a multicast event
        if ((ptpPort->portDS.status.portState == VTSS_APPL_PTP_MASTER)
                && ptpClock->clock_init->cfg.twoStepFlag) {  /* sync forwarded in a 802.1as node */
                T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"sync eventTransmitted, txTime = %u, receive_time = %u, portnum %d",
                tx_time, ptpClock->port_802_1as_sync_sync.sync_ingress_time, portnum);
                vtss_local_clock_convert_to_time(tx_time, &mesa_tx_time, ptpClock->localClockId);
                vtss_tod_sub_TimeInterval(&correctionValue, &mesa_tx_time, &ptpClock->port_802_1as_sync_sync.last_sync_ingress_time);
                ptpClock->port_802_1as_sync_sync.port_mask &= ~(1LL<<(portnum-1));

                vtss_appl_ptp_protocol_adr_t *ptp_dest;
                ptp_dest = (ptpPort->port_config->dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT) ? &ptpClock->ptp_primary : &ptpClock->ptp_pdelay;
                buffer_size = vtss_1588_prepare_general_packet(&frame, ptp_dest, ptpClock->port_802_1as_sync_sync.msgFbuf_size, &header_size, ptpClock->localClockId);
                vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
                if (buffer_size) {
                    memcpy(frame + header_size, ptpClock->port_802_1as_sync_sync.msgFbuf,
                                        ptpClock->port_802_1as_sync_sync.msgFbuf_size);
                    buf = frame + header_size;
                    buf[PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] = ptpPort->portDS.status.s_802_1as.currentLogSyncInterval;
                    memcpy(&buf[PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET],
                       frame+ header_size + PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET,
                       sizeof(ptpPort->portDS.status.s_802_1as.currentLogSyncInterval));
                    vtss_tod_pack16(ptpClock->port_802_1as_sync_sync.syncForwardingHeader.sequenceId, frame+ header_size+ PTP_MESSAGE_SEQUENCE_ID_OFFSET);
                    vtss_ptp_pack_sourceportidentity(frame+ header_size, &ptpPort->portDS.status.portIdentity);
                    vtss_ptp_update_correctionfield(frame+ header_size, &correctionValue);
                    if (vtss_1588_tx_general(ptpPort->port_mask,frame, buffer_size, &tag))
                        T_DG(VTSS_TRACE_GRP_PTP_BASE_TC,"forwarded Follow Up message to port: %d", ptpPort->portDS.status.portIdentity.portNumber);
                }
        }
    }
}
/*
 * Sync send  timer when syncLocked=FALSE
 */

static void  vtss_ptp_bmca_802_1as_in_timeout_sync_timer(vtss_ptp_sys_timer_t *timer, void *t)
{
    PtpPort_t *ptpPort = (PtpPort_t *)t;
    ptp_clock_t *ptpClock = ptpPort->parent;
    u64 port_mask = 0;
    MsgHeader *header;
    size_t header_size;
    ptp_tx_buffer_handle_t tx_handle;
    header = &ptpClock->port_802_1as_sync_sync.syncForwardingHeader;

    i32 syncReceiptInterval;
      
    ptpPort->portDS.status.s_802_1as.syncLocked = (ptpPort->portDS.status.s_802_1as.currentLogSyncInterval ==
                                                               ptpClock->parentDS.parentLogSyncInterval);
    if (!ptpPort->portDS.status.s_802_1as.syncLocked && ptpPort->parent->slavePort != 0){
        if (ptpPort->portDS.status.portState == VTSS_APPL_PTP_MASTER) {
            u64 old_port_mask = ptpClock->port_802_1as_sync_sync.port_mask;
            port_mask = ptpPort->port_mask; 
            header->sourcePortIdentity = ptpPort->portDS.status.portIdentity;
            vtss_appl_ptp_protocol_adr_t *ptp_dest;
            ptp_dest = (ptpPort->port_config->dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT) ? &ptpClock->ptp_primary : &ptpClock->ptp_pdelay;
            tx_handle.size = vtss_1588_prepare_general_packet(&tx_handle.frame, ptp_dest, ptpClock->port_802_1as_sync_sync.last_ptp_sync_msg_size, &header_size, ptpClock->localClockId);
            if (tx_handle.size) {
                u8 *buf;
                uint32_t ts_id;
                tx_handle.handle = 0; // Release buffer automatically
                tx_handle.header_length = header_size;
                header->minorVersionPTP = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
                vtss_ptp_pack_msg44(tx_handle.frame + tx_handle.header_length, header, 
                                               &ptpClock->port_802_1as_sync_sync.last_sync_tx_time);
                buf = tx_handle.frame + tx_handle.header_length;
                buf[PTP_MESSAGE_LOGMSG_INTERVAL_OFFSET] = ptpPort->portDS.status.s_802_1as.currentLogSyncInterval;
                tx_handle.msg_type = VTSS_PTP_MSG_TYPE_2_STEP;
                ptpClock->port_802_1as_sync_sync.sync_ts_context.cb_ts = vtss_ptp_802_1as_site_repeated_syncsync_event_transmitted;
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
                    ptpPort->port_statistics.txSyncCount++;
                }
            } else {
                T_WG(VTSS_TRACE_GRP_PTP_BASE_TC,"cound not allocate buffer for Sync message forwarding to portmask: " VPRI64x, port_mask);
            }
        }
        if (ptpPort->msm.syncSlowdown)
        {
            if (ptpPort->msm.numberSyncTransmissions >= ptpPort->port_config->c_802_1as.syncReceiptTimeout)
            {
                ptpPort->msm.sync_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogSyncInterval;
                ptpPort->msm.numberSyncTransmissions = 0;
                ptpPort->msm.syncSlowdown = FALSE;
            }
            else
            {
                ptpPort->msm.numberSyncTransmissions++;
            }
        }
        else
        {
            ptpPort->msm.numberSyncTransmissions = 0;
            ptpPort->msm.sync_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogSyncInterval;
        }
        syncReceiptInterval = PTP_LOG_TIMEOUT(ptpPort->msm.sync_log_msg_period);
        vtss_ptp_timer_start(&ptpPort->bmca_802_1as_in_timeout_sync_timer,(i32)(syncReceiptInterval), false);
    }
}

void vtss_ptp_802_1as_bmca_init(ptp_clock_t *ptpClock)
{
    u32 portidx;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"no of ethernet ports %d", ptpClock->clock_init->numberEtherPorts);
    // initialize clock BMCA data
    ptpClock->bmca_802_1as.timeProperties = *ptpClock->time_prop;
    ptpClock->bmca_802_1as.sysTimeProperties = *ptpClock->time_prop;
    ptpClock->bmca_802_1as.systemPriorityVector.systemIdentity.priority1 = ptpClock->clock_init->cfg.priority1;
    ptpClock->bmca_802_1as.systemPriorityVector.systemIdentity.clockQuality = ptpClock->defaultDS.status.clockQuality;
    ptpClock->bmca_802_1as.systemPriorityVector.systemIdentity.priority2 = ptpClock->clock_init->cfg.priority2;
    memcpy(ptpClock->bmca_802_1as.systemPriorityVector.systemIdentity.clockIdentity, ptpClock->clock_init->clockIdentity, sizeof(vtss_appl_clock_identity));
    ptpClock->bmca_802_1as.systemPriorityVector.stepsRemoved = 0;
    memcpy(ptpClock->bmca_802_1as.systemPriorityVector.sourcePortIdentity.clockIdentity, ptpClock->clock_init->clockIdentity, sizeof(vtss_appl_clock_identity));
    ptpClock->bmca_802_1as.systemPriorityVector.sourcePortIdentity.portNumber = 0;
    ptpClock->bmca_802_1as.systemPriorityVector.portNumber = 0;
    ptpClock->parentDS.par_802_1as.cumulativeRateRatio = 0;
    
    ptpClock->bmca_802_1as.gmPriorityVector = ptpClock->bmca_802_1as.systemPriorityVector;

    for (portidx = 0; portidx < ptpClock->clock_init->numberPorts; portidx++) {
        PtpPort_t *ptpPort = &ptpClock->ptpPort[portidx];
        if (ptpClock->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            vtss_ptp_timer_init(&ptpPort->announce_to_timer,                         "announce_to",             portidx, vtss_ptp_bmca_announce_to_timer,             ptpPort);
        }
        vtss_ptp_timer_init(&ptpPort->bmca_802_1as_sync_timer,                   "bmca_802_1as_sync",       portidx, vtss_ptp_bmca_sync_to_timer,                 ptpPort);
        vtss_ptp_timer_init(&ptpPort->gptpsm.bmca_802_1as_gptp_rx_timeout_timer, "bmca_802_1as_gptp_rx_to", portidx, vtss_ptp_bmca_gptp_timeout_timer,            ptpPort);
        vtss_ptp_timer_init(&ptpPort->gptpsm.bmca_802_1as_gptp_tx_timer,         "bmca_802_1as_gptp_tx",    portidx, vtss_ptp_bmca_gptp_transmit_timer,           ptpPort);
        vtss_ptp_timer_init(&ptpPort->bmca_802_1as_in_timeout_sync_timer,        "bmca_802_1as_in_to_sync", portidx, vtss_ptp_bmca_802_1as_in_timeout_sync_timer, ptpPort);
        ptpPort->bmca_802_1as.infoIs = VTSS_PTP_INFOIS_DISABLED;
        // initial portPriority vector
        ptpPort->bmca_802_1as.portPriority.systemIdentity.priority1 = ptpClock->clock_init->cfg.priority1;
        ptpPort->bmca_802_1as.portPriority.systemIdentity.clockQuality = ptpClock->defaultDS.status.clockQuality;
        ptpPort->bmca_802_1as.portPriority.systemIdentity.priority2 = ptpClock->clock_init->cfg.priority2;
        memcpy(ptpPort->bmca_802_1as.portPriority.systemIdentity.clockIdentity, ptpClock->clock_init->clockIdentity, sizeof(ptpPort->bmca_802_1as.portPriority.systemIdentity.clockIdentity));
        ptpPort->bmca_802_1as.portPriority.stepsRemoved = 0;
        ptpPort->bmca_802_1as.portPriority.sourcePortIdentity = ptpPort->portDS.status.portIdentity;
        ptpPort->bmca_802_1as.portPriority.portNumber = ptpPort->portDS.status.portIdentity.portNumber;

        T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"timer");
    }
}

void vtss_ptp_802_1as_bmca_state(PtpPort_t *ptpPort, bool enable)
{
    i32 announceReceiptTimeoutInterval;
    ptp_clock_t *ptpClock;
    T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"port %d: enable %d, infoIs %s", ptpPort->portDS.status.portIdentity.portNumber, enable, vtss_ptp_InfoIsToString(ptpPort->bmca_802_1as.infoIs));
    if (enable && ptpPort->portDS.status.s_802_1as.asCapable && ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
        if (ptpPort->bmca_802_1as.infoIs == VTSS_PTP_INFOIS_DISABLED) {
            ptpPort->bmca_802_1as.infoIs = VTSS_PTP_INFOIS_RECEIVED; // to trig the port role selection if a timeout occurs
            ptpPort->portDS.status.s_802_1as.portRole = VTSS_APPL_PTP_PORT_ROLE_PASSIVE_PORT; // portRole must be != DISABLED, so that announce messages are accepted
            announceReceiptTimeoutInterval = PTP_LOG_TIMEOUT(ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval) * ptpPort->port_config->announceReceiptTimeout;
            vtss_ptp_timer_start(&ptpPort->announce_to_timer, announceReceiptTimeoutInterval, false);
        }
    } else if (enable && ptpPort->portDS.status.s_802_1as.asCapable && ptpPort->parent->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS){
        ptpClock = ptpPort->parent;
        if (ptpPort->port_config->aedPortState == VTSS_APPL_PTP_PORT_STATE_AED_MASTER) {
            ptpPort->portDS.status.s_802_1as.portRole = VTSS_APPL_PTP_PORT_ROLE_MASTER_PORT;
            vtss_ptp_state_set(VTSS_APPL_PTP_MASTER, ptpClock, ptpPort);
        } else if (ptpPort->port_config->aedPortState == VTSS_APPL_PTP_PORT_STATE_AED_SLAVE) {
            ptpPort->portDS.status.s_802_1as.portRole = VTSS_APPL_PTP_PORT_ROLE_SLAVE_PORT;
            ptpPort->portDS.status.portState = VTSS_APPL_PTP_PASSIVE;
            vtss_ptp_state_set(VTSS_APPL_PTP_UNCALIBRATED, ptpClock, ptpPort);
        }
    } else {
        ptpPort->bmca_802_1as.infoIs = VTSS_PTP_INFOIS_DISABLED;
        ptpPort->portDS.status.s_802_1as.portRole = VTSS_APPL_PTP_PORT_ROLE_DISABLED_PORT;
        // initial portPriority vector
        ptpClock = ptpPort->parent;
        ptpPort->bmca_802_1as.portPriority.systemIdentity.priority1 = ptpClock->clock_init->cfg.priority1;
        ptpPort->bmca_802_1as.portPriority.systemIdentity.clockQuality = ptpClock->defaultDS.status.clockQuality;
        ptpPort->bmca_802_1as.portPriority.systemIdentity.priority2 = ptpClock->clock_init->cfg.priority2;
        memcpy(ptpPort->bmca_802_1as.portPriority.systemIdentity.clockIdentity, ptpClock->clock_init->clockIdentity, sizeof(ptpPort->bmca_802_1as.portPriority.systemIdentity.clockIdentity));
        ptpPort->bmca_802_1as.portPriority.stepsRemoved = 0;
        ptpPort->bmca_802_1as.portPriority.sourcePortIdentity = ptpPort->portDS.status.portIdentity;
        ptpPort->bmca_802_1as.portPriority.portNumber = ptpPort->portDS.status.portIdentity.portNumber;
    }
    ptpPort->bmca_802_1as.bmca_enabled = enable;
}

void vtss_ptp_802_1as_bmca_ascapable(PtpPort_t *ptpPort)
{
    T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"port %d new asCapable %d", ptpPort->portDS.status.portIdentity.portNumber, ptpPort->portDS.status.s_802_1as.asCapable);
    vtss_ptp_802_1as_bmca_state(ptpPort, ptpPort->bmca_802_1as.bmca_enabled);
    if (!ptpPort->portDS.status.s_802_1as.asCapable) {
        vtss_ptp_state_set(VTSS_APPL_PTP_LISTENING, ptpPort->parent, ptpPort);
    }
}

void vtss_ptp_802_1as_bmca_new_info(ptp_clock_t *ptpClock)
{
    T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"new_info");
    // update clock BMCA data
    ptpClock->bmca_802_1as.systemPriorityVector.systemIdentity.priority1 = ptpClock->clock_init->cfg.priority1;
    ptpClock->bmca_802_1as.systemPriorityVector.systemIdentity.clockQuality = ptpClock->defaultDS.status.clockQuality;
    ptpClock->bmca_802_1as.systemPriorityVector.systemIdentity.priority2 = ptpClock->clock_init->cfg.priority2;
    memcpy(ptpClock->bmca_802_1as.systemPriorityVector.systemIdentity.clockIdentity, ptpClock->clock_init->clockIdentity, sizeof(vtss_appl_clock_identity));
    ptpClock->bmca_802_1as.systemPriorityVector.stepsRemoved = 0;
    memcpy(ptpClock->bmca_802_1as.systemPriorityVector.sourcePortIdentity.clockIdentity, ptpClock->clock_init->clockIdentity, sizeof(vtss_appl_clock_identity));
    ptpClock->bmca_802_1as.systemPriorityVector.sourcePortIdentity.portNumber = 0;
    ptpClock->bmca_802_1as.systemPriorityVector.portNumber = 0;

    ptpClock->bmca_802_1as.gmPriorityVector = ptpClock->bmca_802_1as.systemPriorityVector;
    vtss_ptp_port_role_selection(ptpClock, false);
}

void vtss_ptp_801_1as_bmca_announce_virtual(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, ClockDataSet *clock_ds, vtss_appl_ptp_clock_timeproperties_ds_t *time_prop)
{
    vtss_ptp_priority_vector_t messagePriority;
    i32 announceReceiptTimeoutInterval;
    int cmp;
    char str1 [300];

    // compose message priority
    messagePriority.systemIdentity.priority1 = clock_ds->priority1;
    messagePriority.systemIdentity.clockQuality = clock_ds->clockQuality;
    messagePriority.systemIdentity.priority2 = clock_ds->priority2;
    memcpy(messagePriority.systemIdentity.clockIdentity, clock_ds->grandmasterIdentity, sizeof(messagePriority.systemIdentity.clockIdentity));
    messagePriority.stepsRemoved = 1;
    messagePriority.sourcePortIdentity = clock_ds->sourcePortIdentity;
    messagePriority.portNumber = ptpPort->portDS.status.portIdentity.portNumber;
    // compare received priority to current saved portPriority vector
    cmp = priority_vector_cmp(&messagePriority, &ptpPort->bmca_802_1as.portPriority);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"messagePriority %s", vtss_ptp_PriorityVectorToString(&messagePriority, str1, sizeof(str1)));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"portPriority %s", vtss_ptp_PriorityVectorToString(&ptpPort->bmca_802_1as.portPriority, str1, sizeof(str1)));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_BMCA,"cmp: %d", cmp);

    // update announced timeproperties
    ptpPort->bmca_802_1as.annTimeProperties.currentUtcOffset = time_prop->currentUtcOffset;
    ptpPort->bmca_802_1as.annTimeProperties.currentUtcOffsetValid = time_prop->currentUtcOffsetValid;
    ptpPort->bmca_802_1as.annTimeProperties.leap59 = time_prop->leap59;
    ptpPort->bmca_802_1as.annTimeProperties.leap61 = time_prop->leap61;
    ptpPort->bmca_802_1as.annTimeProperties.timeTraceable = time_prop->timeTraceable;
    ptpPort->bmca_802_1as.annTimeProperties.frequencyTraceable = time_prop->frequencyTraceable;
    ptpPort->bmca_802_1as.annTimeProperties.ptpTimescale = time_prop->ptpTimescale;
    ptpPort->bmca_802_1as.annTimeProperties.timeSource = time_prop->timeSource;


    ptpPort->bmca_802_1as.infoIs = VTSS_PTP_INFOIS_RECEIVED;
    ptpPort->bmca_802_1as.portPriority = messagePriority;

    vtss_ptp_port_role_selection(ptpClock, cmp < 0 ? true : false);
    announceReceiptTimeoutInterval = PTP_LOG_TIMEOUT(0) * ptpPort->port_config->announceReceiptTimeout;
    vtss_ptp_timer_start(&ptpPort->announce_to_timer, announceReceiptTimeoutInterval, false);

    if (ptpClock->slavePort == 0) {
        ptpClock->path_trace.size = 0; // note: this node's path trace is appended in the path trace insertion function
        ptpClock->parentDS.par_802_1as.cumulativeRateRatio = 0;
    } else if (ptpClock->slavePort == ptpPort->portDS.status.portIdentity.portNumber) {
        ptpClock->path_trace.size = 1;
        memcpy(&ptpClock->path_trace.pathSequence[0], &clock_ds->grandmasterIdentity, sizeof(ptpClock->path_trace.pathSequence[0]));
    }
}

#endif //(VTSS_SW_OPTION_P802_1_AS)
