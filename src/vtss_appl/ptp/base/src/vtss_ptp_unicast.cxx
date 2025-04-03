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

#include "vtss_ptp_api.h"
#include "vtss_ptp_types.h"
#include "vtss_ptp_os.h"
#include "vtss_ptp_master.h"
#include "vtss_ptp_unicast.hxx"
#include "vtss_ptp_sys_timer.h"
#include "vtss_ptp_pack_unpack.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_ptp_packet_callout.h"
#include "vtss_tod_api.h"
#include "vtss_ptp_internal_types.h"
#include "misc_api.h"
#include "vtss/appl/ptp.h"
#include "ptp_api.h"
#include "vtss/basics/map.hxx"

bool operator < (const vtss_ptp_master_table_key_t& a, const vtss_ptp_master_table_key_t& b) {
    return (a.inst < b.inst || (a.inst == b.inst && a.ip < b.ip));
}

static vtss::Map <vtss_ptp_master_table_key_t, UnicastMasterTable_t *> master_table;
 
// Master table traverse
void vtss_ptp_master_table_traverse(void)
{
    char str1[20];
    char str2[30];
    UnicastMasterTable_t *uni_master_table;
    vtss_ptp_master_table_key_t next_key;
    mesa_rc rc = VTSS_RC_OK;
    T_N("Traverse master table\n");
    rc = vtss_appl_ptp_clock_slave_itr_get(0, &next_key);
    while (rc == VTSS_RC_OK) {
        if (rc == VTSS_RC_OK) {
            rc = vtss_ptp_clock_unicast_master_table_get(next_key, &uni_master_table);
            if (rc == VTSS_RC_OK) {
                T_N("inst %d, ip %s, data port %d mac %s\n",next_key.inst, misc_ipv4_txt(next_key.ip, str1), uni_master_table->port,
                     misc_mac_txt(uni_master_table->slave.mac.addr, str2));
            }
        }
        rc = vtss_appl_ptp_clock_slave_itr_get(&next_key, &next_key);
    }
}

mesa_rc vtss_appl_ptp_clock_slave_itr_get(const vtss_ptp_master_table_key_t *const prev, vtss_ptp_master_table_key_t *const next)
{
    mesa_rc rc = VTSS_RC_ERROR;
    auto i = master_table.begin();
    if (i == master_table.end()) {
        return rc; // empty table
    }
    if (prev == 0) {
        *next = i->first;
        return VTSS_RC_OK;
    }
    i = master_table.greater_than(*prev);
    if (i != master_table.end()) {
        *next = i->first;
        rc = VTSS_RC_OK;
    }
    return rc;
}

mesa_rc vtss_ptp_clock_unicast_master_table_get(vtss_ptp_master_table_key_t key, UnicastMasterTable_t **uni_master_table)
{
    mesa_rc rc = VTSS_RC_ERROR;
    auto i = master_table.find(key);
    if (i != master_table.end()) {
        *uni_master_table = i->second;
        rc = VTSS_RC_OK;
    }
    
    return rc;
}  
     
/*
 * Forward declarations
 */
static void announce_request_to(vtss_ptp_sys_timer_t *timer, void *m);
static void sync_request_to(vtss_ptp_sys_timer_t *timer, void *m);
static void request_timer_to(vtss_ptp_sys_timer_t *timer, void *s);
static void request_sync_timer_to(vtss_ptp_sys_timer_t *timer, void *s);
static void ann_grant_timer_to(vtss_ptp_sys_timer_t *timer, void *s);
static void sync_grant_timer_to(vtss_ptp_sys_timer_t *timer, void *s);

typedef struct grant_t {
    u8   messageType;    // the granted message type
    i8    log_msg_period; // the granted  interval
    u32  duration;       // number of seconds for which the messages are to sent to slave
} grant_t;

/*
 * private functions
 */
void masterTableInit(UnicastMasterTable_t *list, ptp_clock_t *parent)
{
    int i;
    for (i = 0; i < MAX_UNICAST_SLAVES_PR_MASTER; i++) {
        list[i].slave.ip = 0;
        list[i].port = 0;
        list[i].parent = parent;
        vtss_ptp_timer_init(&list[i].ann_req_timer,  "ann_req",  i, announce_request_to, &list[i]);
        vtss_ptp_timer_init(&list[i].sync_req_timer, "sync_req", i, sync_request_to,     &list[i]);
        list[i].ann_timer_cnt = 0;

        /* This function is called then a clock instance is deleted, therefore any active master instances are stopped */
        vtss_ptp_announce_delete(&list[i].ansm);
        vtss_ptp_master_delete(&list[i].msm);
        list[i].master_active = false;
    }
}

i16 masterTableEntryFind(UnicastMasterTable_t *list, u32 ip)
{
    int i;
    for (i = 0; i < MAX_UNICAST_SLAVES_PR_MASTER; i++) {
        if (list[i].slave.ip == ip) {
            return i; // the ip addres is already in the list
        }
    }
    for (i = 0; i < MAX_UNICAST_SLAVES_PR_MASTER; i++) {
        if (list[i].slave.ip == 0) {
            return i; // return an empty entry
        }
    }
    return -1; // bad luck
}

void slaveTableInit(UnicastSlaveTable_t *list, ptp_clock_t *parent)
{
    int i;
    for (i = 0; i < MAX_UNICAST_MASTERS_PR_SLAVE; i++) {
        list[i].master.ip = 0;
        memset(&list[i].sourcePortIdentity, 0xff, sizeof(list[i].sourcePortIdentity));
        list[i].conf_master_ip = 0;
        list[i].duration = 100;
        list[i].log_msg_period = 0;
        list[i].port = 0;
        list[i].comm_state = VTSS_APPL_PTP_COMM_STATE_IDLE;
        list[i].parent = parent;
        vtss_ptp_timer_init(&list[i].unicast_slave_request_timer,      "unicast_slave_request",      i, request_timer_to,      &list[i]);
        vtss_ptp_timer_init(&list[i].unicast_slave_request_sync_timer, "unicast_slave_request_sync", i, request_sync_timer_to, &list[i]);
        vtss_ptp_timer_init(&list[i].unicast_slave_ann_grant_timer,    "unicast_slave_ann_grant",    i, ann_grant_timer_to,    &list[i]);
        vtss_ptp_timer_init(&list[i].unicast_slave_sync_grant_timer,   "unicast_slave_sync_grant",   i, sync_grant_timer_to,   &list[i]);
    }
}
i16 slaveTableEntryFind(UnicastSlaveTable_t *list, u32 ip)
{
    int i;
    for (i = 0; i < MAX_UNICAST_MASTERS_PR_SLAVE; i++) {
        if (list[i].conf_master_ip == ip) {
            return i; // the ip addres is already in the list
        }
    }
    return -1; // bad luck
}

i16 slaveTableEntryFindClockId(UnicastSlaveTable_t *list, vtss_appl_ptp_port_identity * id)
{
    int i;
    for (i = 0; i < MAX_UNICAST_MASTERS_PR_SLAVE; i++) {
        if (0 == memcmp(list[i].sourcePortIdentity.clockIdentity, id->clockIdentity, sizeof(vtss_appl_clock_identity)) &&
                list[i].sourcePortIdentity.portNumber == id->portNumber) {
            return i; // the ip addres is already in the list
        }
    }
    return -1; // bad luck
}

/* pack and send signalling messages */
/**
 * Request unicastAnnounce is special as the destination port and MAC address is not known.
 */
void issueRequestUnicastAnnounce(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, UnicastSlaveTable_t *slave)
{
    vtss_appl_ptp_port_identity targetPortIdentity;
    TLV tlv;
    u8 tlv_value[6];
    ++slave->last_unicast_request_sequence_number;
    int txbytes;
    u8 tx_buf [SIGNALLING_MIN_PACKET_LENGTH + REQUEST_UNICAST_TRANSMITTION_TLV_LENGTH];

    u16 packetLength = SIGNALLING_MIN_PACKET_LENGTH + REQUEST_UNICAST_TRANSMITTION_TLV_LENGTH;
    u16 minorVersion = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
    memset(&targetPortIdentity, 0xff, sizeof(targetPortIdentity));
    vtss_ptp_pack_signalling(tx_buf, &targetPortIdentity, &ptpPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.domainNumber, ptpClock->majorSdoId, ptpPort->port_config->versionNumber, minorVersion, slave->last_unicast_request_sequence_number, packetLength, PTP_UNICAST_FLAG, PTP_ALL_OTHERS);

    /* Insert TLV field */
    tlv.tlvType = REQUEST_UNICAST_TRANSMISSION;
    tlv.lengthField = 6;
    tlv.valueField = tlv_value;
    tlv_value[0] = (PTP_MESSAGE_TYPE_ANNOUNCE<<4);
    tlv_value[1] = ptpPort->port_config->logAnnounceInterval;
    vtss_tod_pack32(slave->duration,&tlv_value[2]);

    if (VTSS_RC_OK == vtss_ptp_pack_tlv(tx_buf+SIGNALLING_MIN_PACKET_LENGTH, packetLength-SIGNALLING_MIN_PACKET_LENGTH, &tlv)) {
        T_N_HEX(tx_buf, packetLength);
        if (!(txbytes = vtss_1588_tx_unicast_request(slave->master.ip,
                        tx_buf, packetLength, ptpClock->localClockId))) {
            if (ptpPort->linkState) {
                vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock, ptpPort);
            }
        } else {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "sent RequestUnicast Announce message, txbytes %d", txbytes);
        }
    } else
        T_EG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Transmit buffer too small");
}

/* pack and send signalling messages */
/**
 * Request unicast Sync.
 */
//#define TELCO_VARIANT
void issueRequestUnicastSync(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, UnicastSlaveTable_t *slave)
{
    TLV tlv;
    mesa_rc rc;
    u8 tlv_value[6];
    u16 packetLength = SIGNALLING_MIN_PACKET_LENGTH;
    u16 minorVersion = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
    u16 tlvs;
    u8 *frame;
    vtss_ptp_tag_t tag;
    size_t buffer_size;
    size_t header_size;
#ifdef TELCO_VARIANT
    buffer_size = vtss_1588_prepare_general_packet(&frame, &slave->master, packetLength + REQUEST_UNICAST_TRANSMITTION_TLV_LENGTH, &header_size, ptpClock->localClockId);
    vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
    if (buffer_size) {
        /* Insert TLV field */
        tlv.tlvType = REQUEST_UNICAST_TRANSMISSION;
        tlv.lengthField = 6;
        tlv.valueField = tlv_value;
        tlv_value[0] = (PTP_MESSAGE_TYPE_SYNC<<4);
        tlv_value[1] = ptpPort->port_config->logSyncInterval;
        vtss_tod_pack32(slave->duration,&tlv_value[2]);

        rc = vtss_ptp_pack_tlv(frame+header_size+packetLength, (u16)buffer_size-packetLength, &tlv);
        if (VTSS_RC_OK == rc) {
            packetLength += REQUEST_UNICAST_TRANSMITTION_TLV_LENGTH;
            ++slave->last_unicast_request_sequence_number;
            vtss_ptp_pack_signalling(frame + header_size,&slave->sourcePortIdentity, &ptpClock->defaultDS.d0.portIdentity, ptpClock->clock_init->cfg.domainNumber, ptpClock->majorSdoId, ptpPort->port_config->versionNumber, minorVersion, slave->last_unicast_request_sequence_number, packetLength, PTP_UNICAST_FLAG, PTP_ALL_OTHERS);
            if (!vtss_1588_tx_general(ptpPort->port_mask,frame, header_size + packetLength, &tag))
                vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock, ptpPort);
            else
                T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "sent RequestUnicast Sync message");
        } else
            T_EG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Transmit buffer too small");


        if (ptpClock->clock_init->cfg.oneWay == FALSE) {

            packetLength = SIGNALLING_MIN_PACKET_LENGTH;
            buffer_size = vtss_1588_prepare_general_packet(&frame, &slave->master, packetLength + REQUEST_UNICAST_TRANSMITTION_TLV_LENGTH, &header_size, ptpClock->localClockId);
            vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
            if (buffer_size) {
                /* also request Del_Resp */
                tlv.tlvType = REQUEST_UNICAST_TRANSMISSION;
                tlv.lengthField = 6;
                tlv.valueField = tlv_value;
                tlv_value[0] = (PTP_MESSAGE_TYPE_DELAY_RESP<<4);
                tlv_value[1] = ptpPort->port_config->logSyncInterval;
                vtss_tod_pack32(slave->duration,&tlv_value[2]);
                rc = vtss_ptp_pack_tlv(frame+header_size+packetLength, (u16)buffer_size-packetLength, &tlv);
                if (VTSS_RC_OK == rc) {
                    packetLength += REQUEST_UNICAST_TRANSMITTION_TLV_LENGTH;
                    ++slave->last_unicast_request_sequence_number;
                    vtss_ptp_pack_signalling(frame + header_size,&slave->sourcePortIdentity, &ptpClock->defaultDS.d0.portIdentity, ptpClock->clock_init->cfg.domainNumber, ptpClock->majorSdoId, ptpPort->port_config->versionNumber, minorVersion, slave->last_unicast_request_sequence_number, packetLength, PTP_UNICAST_FLAG, PTP_ALL_OTHERS);
                    if (!vtss_1588_tx_general(ptpPort->port_mask,frame, header_size + packetLength, &tag))
                        vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock, ptpPort);
                    else
                        T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "sent RequestUnicast DelayReq message");
                } else
                    T_EG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Transmit buffer too small");
            }
        }
    }
#else
    if (ptpClock->clock_init->cfg.oneWay == FALSE) {
        tlvs = 2;
    } else {
        tlvs = 1;
    }
    buffer_size = vtss_1588_prepare_general_packet(&frame, &slave->master, packetLength + tlvs*REQUEST_UNICAST_TRANSMITTION_TLV_LENGTH, &header_size, ptpClock->localClockId);
    vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
    if (buffer_size) {
        /* Insert TLV field */
        tlv.tlvType = REQUEST_UNICAST_TRANSMISSION;
        tlv.lengthField = 6;
        tlv.valueField = tlv_value;
        tlv_value[0] = (PTP_MESSAGE_TYPE_SYNC<<4);
        tlv_value[1] = ptpPort->port_config->logSyncInterval;
        vtss_tod_pack32(slave->duration,&tlv_value[2]);

        rc = vtss_ptp_pack_tlv(frame+header_size+packetLength, (u16)buffer_size-packetLength, &tlv);
        if (VTSS_RC_OK == rc) {
            packetLength += REQUEST_UNICAST_TRANSMITTION_TLV_LENGTH;
            if (ptpClock->clock_init->cfg.oneWay == FALSE) {
                /* also request Del_Resp */
                tlv.tlvType = REQUEST_UNICAST_TRANSMISSION;
                tlv.lengthField = 6;
                tlv.valueField = tlv_value;
                tlv_value[0] = (PTP_MESSAGE_TYPE_DELAY_RESP<<4);
                tlv_value[1] = ptpPort->port_config->logSyncInterval;
                vtss_tod_pack32(slave->duration,&tlv_value[2]);
                rc = vtss_ptp_pack_tlv(frame+header_size+packetLength, (u16)buffer_size-packetLength, &tlv);
                if (VTSS_RC_OK == rc) {
                    packetLength += REQUEST_UNICAST_TRANSMITTION_TLV_LENGTH;

                }
            }
        }
        if (VTSS_RC_OK == rc) {
            ++slave->last_unicast_request_sequence_number;
            vtss_ptp_pack_signalling(frame + header_size,&slave->sourcePortIdentity, &ptpPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.domainNumber, ptpClock->majorSdoId, ptpPort->port_config->versionNumber, minorVersion, slave->last_unicast_request_sequence_number, packetLength, PTP_UNICAST_FLAG, PTP_ALL_OTHERS);

            if (!vtss_1588_tx_general(ptpPort->port_mask,frame, header_size + packetLength, &tag))
                vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock, ptpPort);
            else
                T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "sent RequestUnicast Sync message");
        } else {
            T_EG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Transmit buffer too small");
            vtss_1588_release_general_packet(&frame);
        }
    }
#endif
}

/**
 * Grant unicast .
 */
void issueGrantUnicast(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, u16 sequenceId, vtss_appl_ptp_port_identity *targetPortIdentity, grant_t *grant, vtss_appl_ptp_protocol_adr_t *receiver)
{
    TLV tlv;
    u8 tlv_value[8];
    u16 packetLength = SIGNALLING_MIN_PACKET_LENGTH + GRANT_UNICAST_TRANSMITTION_TLV_LENGTH;
    u16 minorVersion = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
    u8 *frame;
    vtss_ptp_tag_t tag;
    size_t buffer_size;
    size_t header_size;

    buffer_size = vtss_1588_prepare_general_packet(&frame, receiver, packetLength, &header_size, ptpClock->localClockId);
    vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
    if (buffer_size) {

        vtss_ptp_pack_signalling(frame + header_size,targetPortIdentity, &ptpPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.domainNumber, ptpClock->majorSdoId, ptpPort->port_config->versionNumber, minorVersion, sequenceId, packetLength, PTP_UNICAST_FLAG, PTP_ALL_OTHERS);
        /* Insert TLV field */
        tlv.tlvType = GRANT_UNICAST_TRANSMISSION;
        tlv.lengthField = 8;
        tlv.valueField = tlv_value;
        tlv_value[0] = (grant->messageType<<4);
        tlv_value[1] = grant->log_msg_period;
        vtss_tod_pack32(grant->duration,&tlv_value[2]);
        tlv_value[6] = 0;
        tlv_value[7] = 1; // 'R'bit (Renewal invited)
        if (VTSS_RC_OK == vtss_ptp_pack_tlv(frame + header_size+SIGNALLING_MIN_PACKET_LENGTH, packetLength-SIGNALLING_MIN_PACKET_LENGTH, &tlv)) {
            if (!vtss_1588_tx_general(ptpPort->port_mask,frame, header_size + packetLength, &tag))
                vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock, ptpPort);
            else
                T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "sent GrantUnicast message");
        } else {
            T_EG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Transmit buffer too small");
            vtss_1588_release_general_packet(&frame);
        }
    }
}

/**
 * Cancel unicast .
 */
void issueCancelUnicast(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, UnicastSlaveTable_t *slave, u8 message_type)
{
    TLV tlv;
    u8 tlv_value[6];
    u16 packetLength = SIGNALLING_MIN_PACKET_LENGTH + CANCEL_UNICAST_TRANSMITTION_TLV_LENGTH;
    u16 minorVersion = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
    u8 *frame;
    vtss_ptp_tag_t tag;
    size_t buffer_size;
    size_t header_size;

    buffer_size = vtss_1588_prepare_general_packet(&frame, &slave->master, packetLength, &header_size, ptpClock->localClockId);
    vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
    if (buffer_size) {
        ++slave->last_unicast_request_sequence_number;

        vtss_ptp_pack_signalling(frame + header_size,&slave->sourcePortIdentity, &ptpPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.domainNumber, ptpClock->majorSdoId, ptpPort->port_config->versionNumber, minorVersion, slave->last_unicast_request_sequence_number, packetLength, PTP_UNICAST_FLAG, PTP_ALL_OTHERS);
        /* Insert TLV field */
        tlv.tlvType = CANCEL_UNICAST_TRANSMISSION;
        tlv.lengthField = 2;
        tlv.valueField = tlv_value;
        tlv_value[0] = (message_type<<4);
        tlv_value[1] = 0;
        if (VTSS_RC_OK == vtss_ptp_pack_tlv(frame + header_size+SIGNALLING_MIN_PACKET_LENGTH, packetLength-SIGNALLING_MIN_PACKET_LENGTH, &tlv)) {
            if (!vtss_1588_tx_general(ptpPort->port_mask,frame, header_size + packetLength, &tag))
                vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock, ptpPort);
            else
                T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "sent CancelUnicast message");
        } else {
            T_EG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Transmit buffer too small");
            vtss_1588_release_general_packet(&frame);
        }
    }
}

/**
 * Acknowledge Cancel unicast .
 */
void issueAcknowledgeCancelUnicast(ptp_clock_t *ptpClock, PtpPort_t *ptpPort, u16 sequenceId, vtss_appl_ptp_port_identity *targetPortIdentity, u8 messageType, vtss_appl_ptp_protocol_adr_t *receiver)
{
    TLV tlv;
    u8 tlv_value[2];
    u16 packetLength = SIGNALLING_MIN_PACKET_LENGTH + ACKNOWLEDGE_CANCEL_UNICAST_TRANSMITTION_TLV_LENGTH;
    u16 minorVersion = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
    u8 *frame;
    vtss_ptp_tag_t tag;
    size_t buffer_size;
    size_t header_size;

    buffer_size = vtss_1588_prepare_general_packet(&frame, receiver, packetLength, &header_size, ptpClock->localClockId);
    vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
    if (buffer_size) {

        vtss_ptp_pack_signalling(frame + header_size,targetPortIdentity, &ptpPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.domainNumber, ptpClock->majorSdoId, ptpPort->port_config->versionNumber, minorVersion, sequenceId, packetLength, PTP_UNICAST_FLAG, PTP_ALL_OTHERS);
        /* Insert TLV field */
        tlv.tlvType = ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION;
        tlv.lengthField = 2;
        tlv.valueField = tlv_value;
        tlv_value[0] = (messageType<<4);
        tlv_value[1] = 0;
        if (VTSS_RC_OK == vtss_ptp_pack_tlv(frame + header_size+SIGNALLING_MIN_PACKET_LENGTH, packetLength-SIGNALLING_MIN_PACKET_LENGTH, &tlv)) {
            if (!vtss_1588_tx_general(ptpPort->port_mask,frame, header_size + packetLength, &tag))
                vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock, ptpPort);
            else
                T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "sent Ack Cancel Unicast message");
        } else {
            T_EG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Transmit buffer too small");
            vtss_1588_release_general_packet(&frame);
        }
    }
}

/**
 * Cancel unicast .
 */
void debugIssueCancelUnicast(ptp_clock_t *ptpClock, uint slave_index, u8 message_type)
{
    UnicastMasterTable_t *master = &ptpClock->master[slave_index];
    PtpPort_t *ptpPort;
    TLV tlv;
    u8 tlv_value[6];
    u16 packetLength = SIGNALLING_MIN_PACKET_LENGTH + CANCEL_UNICAST_TRANSMITTION_TLV_LENGTH;
    u16 minorVersion;
    u8 *frame;
    vtss_ptp_tag_t tag;
    size_t buffer_size;
    size_t header_size;
    char buf1[20];

    if (master->slave.ip != 0 && master->port > 0) {
        ptpPort = &master->parent->ptpPort[master->port-1];
        minorVersion = ptpPort->port_config->c_802_1as.as2020 ? MINOR_VERSION_PTP : MINOR_VERSION_PTP_2011;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "slave.ip %s, master port %d", misc_ipv4_txt(master->slave.ip, buf1), master->port);
        buffer_size = vtss_1588_prepare_general_packet(&frame, &master->slave, packetLength, &header_size, ptpClock->localClockId);
        vtss_1588_tag_get(get_tag_conf(ptpClock, ptpPort), ptpClock->localClockId, &tag);
        if (buffer_size) {

            vtss_ptp_pack_signalling(frame + header_size,&master->targetPortIdentity, &ptpPort->portDS.status.portIdentity, ptpClock->clock_init->cfg.domainNumber, ptpClock->majorSdoId, ptpPort->port_config->versionNumber, minorVersion, 0, packetLength, PTP_UNICAST_FLAG, PTP_ALL_OTHERS);
            /* Insert TLV field */
            tlv.tlvType = CANCEL_UNICAST_TRANSMISSION;
            tlv.lengthField = 2;
            tlv.valueField = tlv_value;
            tlv_value[0] = (message_type<<4);
            tlv_value[1] = 0;
            if (VTSS_RC_OK == vtss_ptp_pack_tlv(frame + header_size+SIGNALLING_MIN_PACKET_LENGTH, packetLength-SIGNALLING_MIN_PACKET_LENGTH, &tlv)) {
                if (!vtss_1588_tx_general(ptpPort->port_mask,frame, header_size + packetLength, &tag))
                    vtss_ptp_state_set(VTSS_APPL_PTP_FAULTY, ptpClock, ptpPort);
                else
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "sent CancelUnicast message");
            } else {
                T_EG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Transmit buffer too small");
                vtss_1588_release_general_packet(&frame);
            }
        } else {
            T_WG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "No Transmit buffer available");
        }
    } else {
        T_WG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "unknown slave");
    }
}

void vtss_ptp_tlv_process(MsgHeader *header, TLV *tlv, ptp_clock_t *ptpClock, PtpPort_t *ptpPort, vtss_appl_ptp_protocol_adr_t *sender)
{
    u8 messageType;
    i8 logInterMessagePeriod;
    u32 durationField;
    u8 renewal;
    char buf1[40];
    char buf2[20];
    UnicastMasterTable_t *master;
    UnicastSlaveTable_t *slave;
    int slave_index;
    int master_index;
    grant_t grant;  /* grant unicast parameters */
    bool deny_grant = false;
    switch (tlv->tlvType) {
        case REQUEST_UNICAST_TRANSMISSION:
            grant.messageType = (tlv->valueField[0]>>4);
            grant.log_msg_period = tlv->valueField[1];
            grant.duration = vtss_tod_unpack32(tlv->valueField+2);
            T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "REQUEST_UNICAST_TRANSMISSION: length %d, msgType %d, period %d, duration %d",
                tlv->lengthField, grant.messageType, grant.log_msg_period, grant.duration);
            T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Requester: ip %s, mac %s",
                misc_ipv4_txt(sender->ip, buf1), misc_mac_txt((const u8 *) &sender->mac, buf2));
            if ((ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY ||
                    ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) && ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
                /* only master functions accept unicast request */
                slave_index = masterTableEntryFind(ptpClock->master, sender->ip);
                 // Do not act as master through port on which 'notMaster' option is enabled.
                master_index = slaveTableEntryFind(ptpClock->slave, sender->ip);
                if ((master_index >= 0 && ptpPort->portDS.status.portIdentity.portNumber == ptpClock->slave[master_index].port &&
                    (ptpPort->portDS.status.portState == VTSS_APPL_PTP_UNCALIBRATED ||
                     ptpPort->portDS.status.portState == VTSS_APPL_PTP_SLAVE)) ||
                     ptpPort->port_config->notMaster) {
                    deny_grant = true;
                }
                if (slave_index >=0 && !deny_grant) {
                    master = &ptpClock->master[slave_index];
                    vtss_ptp_master_table_key_t key;
                    key.ip = sender->ip;
                    key.inst = ptpClock->localClockId;
                    master_table.set(key, master);

                    if (grant.messageType == PTP_MESSAGE_TYPE_ANNOUNCE) {
                        if (grant.log_msg_period < -3) grant.log_msg_period = -3;
                        if (grant.log_msg_period > 4) grant.log_msg_period = 4;
                        bool new_master = false;
                        bool resource_available = true;
                        if (master->slave.ip != sender->ip || master->port != ptpPort->portDS.status.portIdentity.portNumber ||
                                master->ansm.ann_log_msg_period != grant.log_msg_period) {
                            new_master = true;
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "request from new slave. IP addr %s, port %d, period %d ", misc_ipv4_txt(sender->ip, buf1), ptpPort->portDS.status.portIdentity.portNumber, grant.log_msg_period);
                            resource_available = vtss_1588_check_transmit_resources(ptpClock->localClockId);
                        }
                        if (!resource_available) {
                            grant.duration = 0; // the request has been denied
                        }
                        /* request Announce: save sender mac and ip */
                        master->slave = *sender;//
                        master->port = ptpPort->portDS.status.portIdentity.portNumber;
                        //issue grant message
                        master->ansm.ann_log_msg_period = grant.log_msg_period; // the granted announce interval
                        master->targetPortIdentity = header->sourcePortIdentity;
                        issueGrantUnicast(ptpClock, ptpPort, header->sequenceId, &header->sourcePortIdentity, &grant, &master->slave);
                        master->ansm.clock = ptpClock;
                        master->ansm.ptp_port = ptpPort;
                        if (new_master && resource_available) {
                            vtss_ptp_announce_create(&master->ansm, sender, get_tag_conf(ptpClock, ptpPort));
                            master->msm.sync_log_msg_period = -128; // this is to force new_master when the first sync request is received
                        }
                        if (resource_available) {
                            vtss_ptp_timer_start(&master->ann_req_timer, PTP_LOG_TIMEOUT(0), FALSE);
                            master->ann_timer_cnt = grant.duration;
                        }
                        if (grant.duration == 0) {
                            master->slave.ip = 0;   // indicate that the entry is not used
                            master->port = 0;
                        }

                    } else if (grant.messageType == PTP_MESSAGE_TYPE_SYNC) {
                        if (grant.log_msg_period < -7) grant.log_msg_period = -7;
                        if (grant.log_msg_period > 4) grant.log_msg_period = 4;
                        bool resource_available = true;
                        bool new_master = false;
                        if (master->slave.ip != sender->ip || master->port != ptpPort->portDS.status.portIdentity.portNumber ||
                           master->msm.sync_log_msg_period != grant.log_msg_period) {
                            new_master = true;
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "request from new slave. IP addr %s, port %d, period %d ", misc_ipv4_txt(sender->ip, buf1), ptpPort->portDS.status.portIdentity.portNumber, grant.log_msg_period);
                        }
                        master->msm.sync_log_msg_period = grant.log_msg_period; // the granted sync interval
                        if (master->slave.ip == 0) { /* missing Request Announce, therefore update the slave address */
                            master->slave = *sender;//
                            master->port = ptpPort->portDS.status.portIdentity.portNumber;
                            master->ann_timer_cnt = 0;
                            resource_available = vtss_1588_check_transmit_resources(ptpClock->localClockId);
                        }
                        if (!resource_available) {
                            grant.duration = 0; // the request has been denied
                        }
                        issueGrantUnicast(ptpClock, ptpPort, header->sequenceId, &header->sourcePortIdentity, &grant, &master->slave);
                        master->msm.clock = ptpClock;
                        master->msm.ptp_port = ptpPort;
                        if ((new_master && resource_available) || !master->master_active) {
                            vtss_ptp_master_create(&master->msm, sender, get_tag_conf(ptpClock, ptpPort));
                            master->master_active = true;
                        }
                        if (resource_available) {
                            vtss_ptp_timer_start(&master->sync_req_timer, grant.duration*PTP_LOG_TIMEOUT(0), FALSE);
                        }
                        if (grant.duration == 0) {
                            master->slave.ip = 0;   // indicate that the entry is not used
                            master->port = 0;
                        }
                    } else if (grant.messageType == PTP_MESSAGE_TYPE_DELAY_RESP) {
                        if (grant.log_msg_period < -7) grant.log_msg_period = -7;
                        if (grant.log_msg_period > 4) grant.log_msg_period = 4;
                        if (master->slave.ip == 0) { /* missing Request Announce, therefore update the slave address */
                            master->slave = *sender;//
                            master->port = ptpPort->portDS.status.portIdentity.portNumber;
                        }
                        issueGrantUnicast(ptpClock, ptpPort, header->sequenceId, &header->sourcePortIdentity, &grant, &master->slave);
                    }
                } else {
                    /* deny request */
                    grant.duration = 0;
                    issueGrantUnicast(ptpClock, ptpPort, header->sequenceId, &header->sourcePortIdentity, &grant, sender);
                }
            } else {
                /* deny request */
                grant.duration = 0;
                issueGrantUnicast(ptpClock, ptpPort, header->sequenceId, &header->sourcePortIdentity, &grant, sender);
            }
            break;
        case GRANT_UNICAST_TRANSMISSION:
            messageType = (tlv->valueField[0]>>4);
            logInterMessagePeriod = tlv->valueField[1];
            durationField = vtss_tod_unpack32(tlv->valueField+2);
            renewal = tlv->valueField[7];
            T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "GRANT_UNICAST_TRANSMISSION: length %d, msgType %d, period %d, duration %d, r %d",
                tlv->lengthField, messageType, logInterMessagePeriod, durationField, renewal);
            master_index = slaveTableEntryFind(ptpClock->slave, sender->ip);
            if (master_index >=0) {
                slave = &ptpClock->slave[master_index];
                if (durationField == 0) {
                    /* request is denied */
                    slave->comm_state = VTSS_APPL_PTP_COMM_STATE_INIT;
                } else {
                    if (messageType == PTP_MESSAGE_TYPE_ANNOUNCE) {
                        /* grant Announce: update master's clock id */
                        slave->sourcePortIdentity = header->sourcePortIdentity;
                        slave->master = *sender;//save master's mac and ip
                        T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Master source id: %s, %d",ClockIdentityToString (slave->sourcePortIdentity.clockIdentity, buf1), slave->sourcePortIdentity.portNumber);
                        //save the port number connected to the master
                        slave->port = ptpPort->portDS.status.portIdentity.portNumber;
                        // master has accepted
                        if (slave->comm_state == VTSS_APPL_PTP_COMM_STATE_INIT) {
                            slave->comm_state = VTSS_APPL_PTP_COMM_STATE_CONN;
                        }
                        vtss_ptp_timer_stop(&slave->unicast_slave_ann_grant_timer);
                    } else if (messageType == PTP_MESSAGE_TYPE_SYNC) {
                        /* grant Sync: update clock id */
                        slave->log_msg_period = logInterMessagePeriod; // the granted sync interval
                        // master has accepted
                        if (slave->comm_state == VTSS_APPL_PTP_COMM_STATE_SELL) {
                            slave->comm_state = VTSS_APPL_PTP_COMM_STATE_SYNC;
                        }
                        vtss_ptp_timer_stop(&slave->unicast_slave_sync_grant_timer);
                    }
                    slave->canceled_by_master = FALSE;
                }
                vtss_ptp_unicast_slave_conf_upd(ptpClock, master_index);
            } else {
                T_WG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Unknown master IP address %s",misc_ipv4_txt(sender->ip,buf1));
            }
            break;
        case CANCEL_UNICAST_TRANSMISSION:
            messageType = (tlv->valueField[0]>>4);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "CANCEL_UNICAST_TRANSMISSION: length %d, msgType %d",
                tlv->lengthField, messageType);
            if ((ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY ||
                    ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) && ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
                slave_index = masterTableEntryFind(ptpClock->master, sender->ip);
                if (slave_index >=0) {
                    master = &ptpClock->master[slave_index];
                    vtss_ptp_master_table_key_t key;
                    key.ip = sender->ip;
                    key.inst = ptpClock->localClockId;
                    
                    // stop sending messages to slave
                    if (master->slave.ip != 0) {
                        issueAcknowledgeCancelUnicast(ptpClock, ptpPort, header->sequenceId, &header->sourcePortIdentity, messageType, &master->slave);
                        if (messageType == PTP_MESSAGE_TYPE_ANNOUNCE) {
                            vtss_ptp_announce_delete(&master->ansm);
                            vtss_ptp_master_delete(&master->msm);
                            master->slave.ip = 0;
                            master->port = 0;
                            master->ann_timer_cnt = 0;
                            master_table.erase(key);
                        } else if (messageType == PTP_MESSAGE_TYPE_SYNC) {
                            vtss_ptp_master_delete(&master->msm);
                            if(master->ann_timer_cnt == 0) {
                                master->slave.ip = 0;
                                master->port = 0;
                                master_table.erase(key);
                            }
                        }
                    } else {
                        T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "CANCEL msgType %d from unknown slave %s, slave_index = %d", messageType, misc_ipv4_txt(sender->ip, buf1), slave_index);
                    }
                }
            }
            if ((ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY ||
                    ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) && ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
                master_index = slaveTableEntryFind(ptpClock->slave, sender->ip);
                if (master_index >=0) {
                    slave = &ptpClock->slave[master_index];
                    issueAcknowledgeCancelUnicast(ptpClock, ptpPort, header->sequenceId, &header->sourcePortIdentity, messageType, &slave->master);
                    slave->canceled_by_master = TRUE;
                    vtss_ptp_unicast_slave_conf_upd(ptpClock, master_index);
                }
            }
            break;
        case ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION:
            messageType = (tlv->valueField[0]>>4);
            T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION: length %d, msgType %d",
                tlv->lengthField, messageType);
            break;
        default:
            T_WG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "Unknown signalling TLV type received: tlvType %d", tlv->tlvType);
            break;

    }
}

#if 0
// Master only unicast clock
if ((ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY ||
        ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) && ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
    for (i = 0; i < MAX_UNICAST_SLAVES_PR_MASTER; i++) {
        UnicastMasterTable_t *master = &ptpClock->master[i];
        if (master->port < 1) {
            T_WG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "port no is not allowed to be < 1");
            master->port = 1;
        }
        ptpPort =&ptpClock->ptpPort[master->port-1];
        // If the master IP is defined, then send RequestUnicast(Announce) transmission regularly.
        if (ptpPort->portDS.portState == VTSS_APPL_PTP_MASTER &&
                master->slave.ip != 0 && master->ann_count > 0) {
            if (timerExpired(UNICAST_MASTER_ANNOUNCE_INTERVAL_TIMER, master->itimer)) {
                issueAnnounceUni(ptpClock,ptpPort,master);
                timerStart(UNICAST_MASTER_ANNOUNCE_INTERVAL_TIMER, PTP_LOG_TIMEOUT(master->ann_log_msg_period), master->itimer);
                --master->ann_count;
                if (master->ann_count == 0) {
                    vtss_ptp_master_table_key_t key;
                    key.ip = master->slave->p;
                    key.inst = ptpClock->localClockId;
                    master->slave.ip = 0;
                    master_table.erase(key);
                }
            }
        }
        // If the master IP is defined, then send RequestUnicast (Sync) transmission regularly.
        if (ptpPort->portDS.portState == VTSS_APPL_PTP_MASTER &&
                master->slave.ip != 0 && master->sync_count > 0) {
            if (timerExpired(UNICAST_MASTER_SYNC_INTERVAL_TIMER, master->itimer)) {
                issueSyncUni(ptpClock,ptpPort,i);
                timerStart(UNICAST_MASTER_SYNC_INTERVAL_TIMER, PTP_LOG_TIMEOUT(master->log_msg_period), master->itimer);
                --master->sync_count;
            }
        }
        /* release tx buffer if not used any more */
        if (master->sync_count == 0 && NULL != master->buf_handle) {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "free master tx buffer %p", master->buf_handle);
            vtss_1588_packet_tx_free(&master->buf_handle);
        }
    }
}
#endif

// Delete unicast master.
void vtss_ptp_unicast_master_delete(UnicastMasterTable_t *master)
{
    vtss_ptp_announce_delete(&master->ansm);
    master->ann_timer_cnt = 0;
    T_IG(VTSS_TRACE_GRP_PTP_BASE_MASTER, "Deleting unicast master on port %d", master->port);
    vtss_ptp_master_delete(&master->msm);
    master->master_active = false;
    vtss_ptp_master_table_key_t key;
    key.ip = master->slave.ip;
    key.inst = master->parent->localClockId;
    master->slave.ip = 0;
    master->port = 0;
    master_table.erase(key);
}
/*
 * Announce request timeout
 */
/*lint -esym(459, announce_request_to) */
static void announce_request_to(vtss_ptp_sys_timer_t *timer, void *m)
{
    UnicastMasterTable_t *master = (UnicastMasterTable_t *)m;
    PtpPort_t *ptpPort;

    if (master->port < 1) {
        //Invalid master port.
        return;
    }
    ptpPort = &master->parent->ptpPort[uport2iport(master->port)];
    T_NG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "announce request timer");
    if (0 >= --master->ann_timer_cnt || !ptpPort->linkState) {
        vtss_ptp_unicast_master_delete(master);
        // stop sending announce.
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "announce request timeout");
    } else {
        vtss_ptp_timer_start(&master->ann_req_timer, PTP_LOG_TIMEOUT(0), FALSE);
    }
}

/*
 * Sync request timeout
 */
static void sync_request_to(vtss_ptp_sys_timer_t *timer, void *m)
{
    UnicastMasterTable_t *master = (UnicastMasterTable_t *)m;
    PtpPort_t *ptpPort;

    if (master->port < 1) {
        //Invalid master port.
        return;
    }
    ptpPort = &master->parent->ptpPort[uport2iport(master->port)];
    T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "sync request timeout");
    // If the master IP is defined, then stop sending sync.
    if (ptpPort->portDS.status.portState == VTSS_APPL_PTP_MASTER && master->slave.ip != 0) {
        vtss_ptp_master_delete(&master->msm);
        master->master_active = false;
        if(master->ann_timer_cnt == 0) {
            vtss_ptp_master_table_key_t key;
            key.ip = master->slave.ip;
            key.inst = master->parent->localClockId;
            master->slave.ip = 0;
            master->port = 0;
            master_table.erase(key);
        }
    }
}


// Slave only unicast clock
void vtss_ptp_unicast_slave_conf_upd(ptp_clock_t *ptpClock, u32 slaveIndex)
{
    char str1[40];
    int one_sec = 0;
    UnicastSlaveTable_t *slave;
    PtpPort_t *ptpPort;
    uint32_t port = 0;

    if ((ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY ||
            ptpClock->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) &&
        ptpClock->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
        /* announce request */
        slave = &ptpClock->slave[slaveIndex];
        // If the master IP is defined, then send RequestUnicast(announce) transmission regularly.
        if (slave->comm_state == VTSS_APPL_PTP_COMM_STATE_IDLE) {
            if (slave->conf_master_ip) {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "now request announce %s", misc_ipv4_txt(slave->conf_master_ip,str1));
                slave->master.ip = slave->conf_master_ip;
                memset(&slave->sourcePortIdentity, 0xff, sizeof(slave->sourcePortIdentity));
                slave->comm_state = VTSS_APPL_PTP_COMM_STATE_INIT;
                if (slave->port < 1) {
                    // Request announce on all ptp enabled ports.
                    for (port = 0; port < ptpClock->clock_init->numberEtherPorts; port++) {
                        ptpPort = &ptpClock->ptpPort[port];
                        if (!ptpPort->port_config->masterOnly &&
                             ptpPort->linkState && ptpPort->designatedEnabled) {
                            issueRequestUnicastAnnounce(ptpClock, ptpPort, slave); // issue unicast request
                        }
                    }
                } else {
                    ptpPort =&ptpClock->ptpPort[uport2iport(slave->port)];
                    issueRequestUnicastAnnounce(ptpClock, ptpPort, slave); // issue unicast request
                }
                vtss_ptp_timer_start(&slave->unicast_slave_request_timer, (PTP_LOG_TIMEOUT(one_sec)*slave->duration)/4, FALSE);
            }
        } else {
            if (slave->conf_master_ip != slave->master.ip || slave->canceled_by_master) {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "now cancel request announce %s", misc_ipv4_txt(slave->master.ip,str1));
                vtss_ptp_timer_stop(&slave->unicast_slave_request_timer);
                if (slaveIndex == ptpClock->selected_master) {
                    ptpClock->selected_master = 0xffff;
                    T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "now cancel request sync %s", misc_ipv4_txt(slave->master.ip,str1));
                    if (slave->port > 0) {
                        ptpPort =&ptpClock->ptpPort[uport2iport(slave->port)];
                        issueCancelUnicast(ptpClock, ptpPort, slave, PTP_MESSAGE_TYPE_ANNOUNCE); // issue cancel request
                        issueCancelUnicast(ptpClock, ptpPort, slave, PTP_MESSAGE_TYPE_SYNC); // issue cancel request
                    }
                    vtss_ptp_timer_stop(&slave->unicast_slave_request_sync_timer);
                }
                slave->comm_state = VTSS_APPL_PTP_COMM_STATE_IDLE;
                slave->canceled_by_master = FALSE;
                slave->master.ip = 0;
                slave->port = 0;
                /* current connection has been cancelled, check if an other ip address has been requested instead */
                vtss_ptp_unicast_slave_conf_upd(ptpClock, slaveIndex);
            }
        }
        if (slave->port < 1) {
            // No valid port
            return;
        }
        ptpPort =&ptpClock->ptpPort[uport2iport(slave->port)];
        if (slave->comm_state == VTSS_APPL_PTP_COMM_STATE_CONN) {
            if (slaveIndex == ptpClock->selected_master) {
                issueRequestUnicastSync(ptpClock, ptpPort, slave); // issue unicast request
                vtss_ptp_timer_start(&slave->unicast_slave_request_sync_timer, (PTP_LOG_TIMEOUT(one_sec)*slave->duration)/4, FALSE);
                vtss_ptp_timer_start(&slave->unicast_slave_sync_grant_timer, PTP_LOG_TIMEOUT(one_sec)*5, FALSE);
                slave->comm_state = VTSS_APPL_PTP_COMM_STATE_SELL;
            }
        }
        if (slave->comm_state == VTSS_APPL_PTP_COMM_STATE_SELL || slave->comm_state == VTSS_APPL_PTP_COMM_STATE_SYNC) {
            if (slaveIndex != ptpClock->selected_master) {
                T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "now cancel request sync %s", misc_ipv4_txt(slave->master.ip,str1));
                issueCancelUnicast(ptpClock, ptpPort, slave, PTP_MESSAGE_TYPE_SYNC); // issue cancel request
                vtss_ptp_timer_stop(&slave->unicast_slave_request_sync_timer);
                vtss_ptp_timer_stop(&slave->unicast_slave_sync_grant_timer);
                slave->comm_state = VTSS_APPL_PTP_COMM_STATE_CONN;
            }
        }
    }
}

/*
 * Transmit Announce request timer
 */
/*lint -esym(459, request_timer_to) */
static void request_timer_to(vtss_ptp_sys_timer_t *timer, void *s)
{
    char str1[40];
    int one_sec = 0;
    UnicastSlaveTable_t *slave = (UnicastSlaveTable_t *)s;
    ptp_clock_t *ptpClock = slave->parent;
    PtpPort_t *ptpPort;
    if ((slave->parent->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY ||
            slave->parent->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) &&
            slave->parent->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
        if (slave->comm_state >= VTSS_APPL_PTP_COMM_STATE_INIT) {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "now request announce %s", misc_ipv4_txt(slave->conf_master_ip,str1));
            if (slave->port < 1) {
                T_DG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "request announce on all possible ports");
                // Request announce on all ptp enabled ports.
                for (auto port = 0; port < ptpClock->clock_init->numberEtherPorts; port++) {
                    ptpPort = &ptpClock->ptpPort[port];
                    if (!ptpPort->port_config->masterOnly &&
                         ptpPort->linkState && ptpPort->designatedEnabled) {
                        issueRequestUnicastAnnounce(ptpClock, ptpPort, slave); // issue unicast request
                    }
                }
            } else {
                ptpPort =&ptpClock->ptpPort[uport2iport(slave->port)];
                issueRequestUnicastAnnounce(ptpClock, ptpPort, slave); // issue unicast request
            }
            vtss_ptp_timer_start(&slave->unicast_slave_request_timer, (PTP_LOG_TIMEOUT(one_sec)*slave->duration)/4, FALSE);
            vtss_ptp_timer_start(&slave->unicast_slave_ann_grant_timer, PTP_LOG_TIMEOUT(one_sec)*5, FALSE);
        }
    } else {
        slave->comm_state = VTSS_APPL_PTP_COMM_STATE_IDLE;
        slave->master.ip = 0;
        slave->port = 0;
    }
}

/*
 * Transmit Sync request timer
 */
/*lint -esym(459, request_sync_timer_to) */
static void request_sync_timer_to(vtss_ptp_sys_timer_t *timer, void *s)
{
    UnicastSlaveTable_t *slave = (UnicastSlaveTable_t *)s;
    PtpPort_t *ptpPort;
    int one_sec = 0;
    if ((slave->parent->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY ||
            slave->parent->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) &&
            slave->parent->clock_init->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
        if (slave->port < 1) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "port no is not allowed to be < 1");
            vtss_ptp_timer_start(&slave->unicast_slave_request_sync_timer, (PTP_LOG_TIMEOUT(one_sec)*slave->duration)/4, FALSE);
            vtss_ptp_timer_start(&slave->unicast_slave_sync_grant_timer, PTP_LOG_TIMEOUT(one_sec)*5, FALSE);
            // No valid port.
            return;
        }
        ptpPort = &slave->parent->ptpPort[slave->port-1];
        if (slave->comm_state == VTSS_APPL_PTP_COMM_STATE_SELL || slave->comm_state == VTSS_APPL_PTP_COMM_STATE_SYNC) {
            issueRequestUnicastSync(slave->parent, ptpPort, slave); // issue unicast request
            vtss_ptp_timer_start(&slave->unicast_slave_request_sync_timer, (PTP_LOG_TIMEOUT(one_sec)*slave->duration)/4, FALSE);
            vtss_ptp_timer_start(&slave->unicast_slave_sync_grant_timer, PTP_LOG_TIMEOUT(one_sec)*5, FALSE);
        }
    } else {
        slave->comm_state = VTSS_APPL_PTP_COMM_STATE_IDLE;
        slave->master.ip = 0;
        slave->port = 0;
    }
}

/*
 * Announce grant timout
 */
/*lint -esym(459, ann_grant_timer_to) */
static void ann_grant_timer_to(vtss_ptp_sys_timer_t *timer, void *s)
{
    char str1[40];
    UnicastSlaveTable_t *slave = (UnicastSlaveTable_t *)s;
    if (slave->comm_state >= VTSS_APPL_PTP_COMM_STATE_INIT) {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "missed announce grant from %s", misc_ipv4_txt(slave->conf_master_ip,str1));
        slave->comm_state = VTSS_APPL_PTP_COMM_STATE_INIT;
    }
}

/*
 * Sync grant timout
 */
/*lint -esym(459, sync_grant_timer_to) */
static void sync_grant_timer_to(vtss_ptp_sys_timer_t *timer, void *s)
{
    char str1[40];
    UnicastSlaveTable_t *slave = (UnicastSlaveTable_t *)s;
    if (slave->comm_state == VTSS_APPL_PTP_COMM_STATE_SELL || slave->comm_state == VTSS_APPL_PTP_COMM_STATE_SYNC) {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_SLAVE, "missed sync grant from %s", misc_ipv4_txt(slave->conf_master_ip,str1));
        slave->comm_state = VTSS_APPL_PTP_COMM_STATE_CONN;
    }
}

// 1588-2008 standard does not contain G,R flags in cancel unicast message. Due to this, if a cancel unicast message is sent while leaving master state,
// neighbor device acting as master is also deleted or restarted. This function need to be called while leaving master state only after G,R flags are
// implemented in cancel unicast messages.
void vtss_ptp_cancel_unicast_master(ptp_clock_t *ptpClock, PtpPort_t *ptpPort)
{
    UnicastMasterTable_t *master;

    for (int i = 0; i < MAX_UNICAST_SLAVES_PR_MASTER; i++) {
        master = &ptpClock->master[i];
        if (master->slave.ip &&
            master->port == ptpPort->portDS.status.portIdentity.portNumber) {
            debugIssueCancelUnicast(ptpClock, i, PTP_MESSAGE_TYPE_ANNOUNCE); // issue cancel request
        }
    }
}
