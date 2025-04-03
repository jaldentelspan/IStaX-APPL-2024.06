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

#include "vtss_xxrp_mvrp.h"
#include "vtss_xxrp_types.h"
#include "vtss_xxrp_api.h"
#include <xxrp_api.h>
#include "vtss_xxrp_os.h"
#include "vtss_xxrp_mad.h"
#include "vtss_xxrp_util.h"
#include "vtss_xxrp.h"
#include "vtss/appl/vlan.h"
#include "../platform/xxrp_trace.h"
#include "misc_api.h"

/* vtss_mvrp_tx is called only after LOCK has been taken. So, ignoring this lint error */
/*lint -esym(459, vtss_mvrp_tx) */
static const u8 mrp_mvrp_multicast_macaddr[] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x21};
static const u8 mrp_mvrp_eth_type[]          = {0x88, 0xF5};
#define MRP_MVRP_MULTICAST_MACADDR mrp_mvrp_multicast_macaddr
#define VTSS_MVRP_ETH_TYPE         mrp_mvrp_eth_type

void vtss_mrp_get_msti_from_mad_fsm_index(u16 mad_fsm_index, u8 *msti)
{
    /* Get the MSTI information from VLAN attribute */
    (void)mrp_mstp_index_msti_mapping_get(mad_fsm_index, msti);
}

void mvrp_mad_fsm_index_to_vid(u16 mad_fsm_index, u16 *vid)
{
    *vid = mad_fsm_index + 1;
}

BOOL vtss_mvrp_is_vlan_registered(u32 l2port, u16 vid)
{
    return XXRP_mvrp_is_vlan_registered(l2port, vid);
}

static mesa_rc vtss_mvrp_process_vector(u32 port_no, u16 first_value,
                                        u16 num_of_valid_vlans, u8 vector, vlan_registration_type_t reg[VTSS_APPL_VLAN_ID_MAX + 1])
{
    u32 temp, rc = VTSS_RC_OK;
    u16 vid = first_value, mad_fsm_indx;
    u8  event, divider = 36;
    vtss_mrp_stat_type_t type;

    /* Three-packed event parsing */
    for (temp = 0; temp < num_of_valid_vlans; temp++) {
        type = VTSS_MRP_STAT_MAX;
        if (divider) {
            event = vector / divider;
        }
        if (reg[vid] == VLAN_REGISTRATION_TYPE_NORMAL) {
            mvrp_vid_to_mad_fsm_index(vid, &mad_fsm_indx);
            if (event == VTSS_XXRP_APPL_EVENT_MT) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, RACKET) << "Received MVRP MT event "
                                                           << "for VID " << vid;
            } else {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "Received MVRP " << vtss_mrp_event2txt(event)
                                                          << " event for VID " << vid;
            }
            switch (event) {
            case VTSS_XXRP_APPL_EVENT_NEW:
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "New event";
                type = VTSS_MRP_RX_NEW;
                rc = vtss_mrp_process_new_event(VTSS_MRP_APPL_MVRP, port_no, mad_fsm_indx);
                break;
            case VTSS_XXRP_APPL_EVENT_JOININ:
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "JoinIn event";
                type = VTSS_MRP_RX_JOININ;
                rc = vtss_mrp_process_joinin_event(VTSS_MRP_APPL_MVRP, port_no, mad_fsm_indx);
                break;
            case VTSS_XXRP_APPL_EVENT_IN:
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "In event";
                type = VTSS_MRP_RX_IN;
                rc = vtss_mrp_process_in_event(VTSS_MRP_APPL_MVRP, port_no, mad_fsm_indx);
                break;
            case VTSS_XXRP_APPL_EVENT_JOINMT:
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "JoinMt event";
                type = VTSS_MRP_RX_JOINMT;
                rc = vtss_mrp_process_joinmt_event(VTSS_MRP_APPL_MVRP, port_no, mad_fsm_indx);
                break;
            case VTSS_XXRP_APPL_EVENT_MT:
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, RACKET) << "Mt event";
                type = VTSS_MRP_RX_MT;
                rc = vtss_mrp_process_mt_event(VTSS_MRP_APPL_MVRP, port_no, mad_fsm_indx);
                break;
            case VTSS_XXRP_APPL_EVENT_LV:
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "Lv event";
                type = VTSS_MRP_RX_LV;
                rc = vtss_mrp_process_lv_event(VTSS_MRP_APPL_MVRP, port_no, mad_fsm_indx);
                break;
            }
            vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, type);
        } else {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, DEBUG) << "Cannot process event for port "
                                                      << l2port2str(port_no) << " and VID "
                                                      << vid << ". Registrar is not in Normal state";
        }
        vector -= (divider * event);
        divider = divider / 6;
        vid++;
    }

    return rc;
}

static inline u16 vtss_mvrp_get_num_of_vals(u8 *bytes)
{
    bytes[0] &= VTSS_MVRP_NUMOFVALS_HIGHER_BYTE_MASK;
    return xxrp_ntohs(bytes);
}

static mesa_rc vtss_mvrp_process_vector_attribute(u32 port_no, mvrp_pdu_vector_attr_t *vec_attr, u16 *size,
                                                  vlan_registration_type_t reg[VTSS_APPL_VLAN_ID_MAX + 1])
{
    i16                       number_of_values = 0;
    u16                       first_value, temp = 0, num_of_vec_events;
    u32                       rc = VTSS_RC_OK;
    BOOL                      leaveall_event;

    leaveall_event = VTSS_MVRP_EXTRACT_LA_EVENT(vec_attr->la_and_num_of_vals);
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "Processing VectorAttribute: LeaveAll and NumberOfValues = 0x "
                                              << vtss::hex_fixed<2>(vec_attr->la_and_num_of_vals[0])
                                              << " " << vtss::hex_fixed<2>(vec_attr->la_and_num_of_vals[1]);
    if (leaveall_event == TRUE) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, DEBUG) << "LeaveAll event received on port " << l2port2str(port_no);
        vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_LA);
        rc = vtss_mrp_process_leaveall(VTSS_MRP_APPL_MVRP, port_no);
    }
    number_of_values = (i16)vtss_mvrp_get_num_of_vals(vec_attr->la_and_num_of_vals);
    *size = (VTSS_MVRP_VECTOR_HDR_SIZE + ((number_of_values + 2) / 3));
    first_value = xxrp_ntohs(vec_attr->first_value);
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "NumberOfValues = " << number_of_values
                                              << ", FirstValue = " << first_value;
    while (number_of_values > 0) {
        if (number_of_values >= VTSS_MVRP_VECTOR_MAX_EVENTS) {
            num_of_vec_events = VTSS_MVRP_VECTOR_MAX_EVENTS;
        } else {
            num_of_vec_events = number_of_values;
        }
        rc += vtss_mvrp_process_vector(port_no, first_value, num_of_vec_events, vec_attr->vectors[temp++], reg);
        number_of_values -= VTSS_MVRP_VECTOR_MAX_EVENTS;
        first_value += VTSS_MVRP_VECTOR_MAX_EVENTS;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "Exit VectorAttribute processing";
    return rc;
}

BOOL vtss_mvrp_pdu_rx(u32 port_no, const u8 *pdu, u32 length)
{
    mvrp_pdu_fixed_flds_t  *mrpdu;
    mvrp_pdu_msg_t         *msg;
    mvrp_pdu_vector_attr_t *vec_attr;
    BOOL                   status = FALSE;
    mesa_rc                rc = VTSS_RC_OK, brc = VTSS_RC_OK;
    u16                    msg_endmark, vec_attr_endmark, vec_attr_size, number_of_values = 0;
    u16                    number_of_vecs = 0;
    u8                     src_mac[VTSS_XXRP_MAC_ADDR_LEN];
    static vlan_registration_type_t reg[VTSS_APPL_VLAN_ID_MAX + 1];

    if ((rc = vtss_mrp_global_control_conf_get(VTSS_MRP_APPL_MVRP, &status)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, ERROR) << "MVRP global configuration get failed - " << error_txt(rc);
        return FALSE;
    }
    if (!status) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "MVRP is disabled globally";
        return FALSE;
    }
    if ((rc = vtss_mrp_port_control_conf_get(VTSS_MRP_APPL_MVRP, port_no, &status)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, ERROR) << "MVRP port configuration get failed - " << error_txt(rc);
        return FALSE;
    }
    if (!status) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "MVRP is disabled on this port";
        return FALSE;
    }

    vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_PKTS);
    if (length < VTSS_XXRP_MIN_PDU_SIZE) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, ERROR) << "Incorrect length of the MVRPDU";
        vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
        return FALSE;
    }

    /* TODO: Need to check NAS status etc */
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "Initial checks of MVRPDU are done, length of pdu = " << length;
    mrpdu = (mvrp_pdu_fixed_flds_t *) pdu;
    memcpy(src_mac, mrpdu->src_mac, VTSS_XXRP_MAC_ADDR_LEN);
    /* Checking ProtocolVersion field of MVRPDU */
    if (mrpdu->version != VTSS_MVRP_VERSION) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, ERROR) << "MVRP protocol version mismatch, got " << mrpdu->version;
        vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
        return FALSE;
    }
    /* Move pdu pointer past the fixed fields */
    pdu += sizeof(mvrp_pdu_fixed_flds_t);
    length -= sizeof(mvrp_pdu_fixed_flds_t);
    msg = (mvrp_pdu_msg_t *)pdu;

    (void)xxrp_mgmt_vlan_state(port_no, reg);

    msg_endmark = xxrp_ntohs((u8 *)msg);
    /* Parse the messages till end mark is reached */
    while (msg_endmark != VTSS_MRP_ENDMARK) {
        if (length < VTSS_XXRP_MIN_MSG_SIZE) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, ERROR) << "Incorrect length of the MVRPDU";
            vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
            return FALSE;
        }
        /* Checking AttributeType field of MVRPDU */
        if (msg->attr_type != VTSS_MVRP_ATTR_TYPE_VLAN) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, ERROR) << "MVRP attribute type mismatch, got " << msg->attr_type;
            vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
            return FALSE;
        }
        /* Checking AttributeLength field of MVRPDU */
        if (msg->attr_len != VTSS_MVRP_ATTR_LEN_VLAN) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, ERROR) << "MVRP attribute length mismatch, got " << msg->attr_len;
            vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
            return FALSE;
        }

        /* Move pdu pointer to vector attribute */
        pdu += sizeof(mvrp_pdu_msg_t);
        length -= sizeof(mvrp_pdu_msg_t);
        vec_attr_endmark = xxrp_ntohs(pdu);
        /* Process all the vector attributes until endmark is reached */
        while (vec_attr_endmark != VTSS_MRP_ENDMARK) {
            vec_attr = (mvrp_pdu_vector_attr_t *)pdu;
            if (length < VTSS_XXRP_VEC_ATTR_HDR_SIZE) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, ERROR) << "Incorrect length of the mvrpdu";
                vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
                return FALSE;
            }
            number_of_values = (i16)vtss_mvrp_get_num_of_vals(vec_attr->la_and_num_of_vals);
            number_of_vecs = ((number_of_values + (VTSS_MVRP_VECTOR_MAX_EVENTS - 1)) / VTSS_MVRP_VECTOR_MAX_EVENTS);
            if (length < (number_of_vecs + VTSS_XXRP_VEC_ATTR_HDR_SIZE)) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, ERROR) << "Incorrect length of the mvrpdu";
                vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
                return FALSE;
            }
            length -= (number_of_vecs + VTSS_XXRP_VEC_ATTR_HDR_SIZE);
            rc = vtss_mvrp_process_vector_attribute(port_no, vec_attr, &vec_attr_size, reg);
            /* Move pdu pointer to next vector attribute */
            pdu += vec_attr_size;
            vec_attr_endmark = xxrp_ntohs(pdu);
        }
        pdu += sizeof(vec_attr_endmark);
        msg = (mvrp_pdu_msg_t *)pdu;
        msg_endmark = xxrp_ntohs((u8 *)msg);
    } /* while (msg->attr_type != VTSS_MRP_ENDMARK) */

    if ((brc = vtss_mrp_port_update_peer_mac_addr(VTSS_MRP_APPL_MVRP, port_no, src_mac)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << "MRP failed to update peer mac address - " << error_txt(brc);
    }
    rc += brc;
    return ((rc == VTSS_RC_OK) ? TRUE : FALSE);
}

void vtss_mvrp_join_indication(u32 port_no, u16 joining_mad_index, BOOL new_flag)
{
    u16 vid;

    mvrp_mad_fsm_index_to_vid(joining_mad_index, &vid);
    (void)XXRP_mvrp_vlan_port_membership_add(port_no, vid);
    if (new_flag) {
        //Flush L2 fdb for this vid
    }
}

void vtss_mvrp_leave_indication(u32 port_no, u32 leaving_mad_index)
{
    u16 vid;

    mvrp_mad_fsm_index_to_vid(leaving_mad_index, &vid);
    (void)XXRP_mvrp_vlan_port_membership_del(port_no, vid);
}

void vtss_mvrp_join_propagated(void *mvrp, void *my_port, u32 mad_index, BOOL new_flag)
{
}

void vtss_mvrp_leave_propagated(void *mvrp, void *my_port, u32 mad_index)
{
}

u32 vtss_mvrp_tx(u32 l2port, u8 *all_attr_events, u32 total_events, BOOL la_flag)
{
    u8  temp;
    u8  event, first_event = 0, second_event = 0, third_event = 0;
    u8  *bufptr, *mvrp_pdu;
    u16 indx, start_vid = 1, num_of_vals = 0, vec_offset = VTSS_MVRP_VECTOR_HDR_SIZE;
    u32 num_of_events = 0;
    u16 pdu_size = VTSS_MVRP_PDU_SIZE_OF_FIXED_FLDS, tmp_len = 0;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, DEBUG) << "Creating MVRPDU for " << total_events << " events on port " << l2port2str(l2port);
    mvrp_pdu = (u8 *)vtss_mrp_mrpdu_tx_alloc(l2port, VTSS_XXRP_MAX_PDU_SIZE);
    if (!mvrp_pdu) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, DEBUG) << "No memory for tx buf allocation";
        return XXRP_ERROR_NO_MEMORY;
    }

    bufptr = mvrp_pdu;
    // xxrp_packet_dump(l2port, all_attr_events, TRUE);
    memset(bufptr, 0, VTSS_XXRP_MAX_PDU_SIZE);
    /* fill MVRP destination MAC */
    memcpy(bufptr, MRP_MVRP_MULTICAST_MACADDR, VTSS_XXRP_MAC_ADDR_LEN);
    bufptr += VTSS_XXRP_MAC_ADDR_LEN;
    /* skip source MAC */
    bufptr += VTSS_XXRP_MAC_ADDR_LEN;
    /* fill MVRP ether_type - 0x88f5 */
    memcpy(bufptr, mrp_mvrp_eth_type, VTSS_XXRP_ETH_TYPE_LEN);
    bufptr += VTSS_XXRP_ETH_TYPE_LEN;
    /* fill MVRP protocol version - 0x00 */
    *bufptr++ = VTSS_MVRP_VERSION;
    /* Message */
    *bufptr++ = VTSS_MVRP_ATTR_TYPE_VLAN;
    *bufptr++ = VTSS_MVRP_ATTR_LEN_VLAN;
    /* check for at least one valid event to encode */
    if (total_events) {
        for (indx = 0, temp = 0; indx < XXRP_MAX_ATTRS - 2; indx++) {
            /* Check for continuous vectors */
            if ((event = VTSS_XXRP_GET_EVENT(all_attr_events, indx)) == VTSS_XXRP_APPL_EVENT_INVALID) {
                if ((temp == 1) || (temp == 2)) {
                    /* found partial vector to encode */
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, RACKET) << "Encoding Partial Vector: "
                                                               << first_event << " " << second_event << " " << third_event;
                    /* encode three packed event vector */
                    bufptr[vec_offset++] = ((((first_event * 6) + second_event) * 6) + third_event);
                    tmp_len++;
                    temp = 0;
                    first_event = 0;
                    second_event = 0;
                    third_event = 0;
                } /* if ((temp == 1) || (temp == 2)) */
                if (num_of_vals) { /* At least one event */
                    /* Encode VectorHeader field */
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, NOISE) << "VectorAttribute has "
                                                              << num_of_vals << " attributes that start from " << start_vid;
                    if (la_flag) {
                        /* Erase LA flag, so that it is incoded only once in a MVRPDU */
                        la_flag = FALSE;
                        *bufptr++ = ((1 << VTSS_MVRP_LA_BIT_OFFSET) | ((num_of_vals >> 8) & VTSS_MVRP_NUMOFVALS_HIGHER_BYTE_MASK));
                        vtss_xxrp_update_tx_stats(VTSS_MRP_APPL_MVRP, l2port, VTSS_XXRP_APPL_EVENT_LA);
                    } else {
                        *bufptr++ = ((num_of_vals >> 8) & VTSS_MVRP_NUMOFVALS_HIGHER_BYTE_MASK);
                    } /* if (la_flag) */
                    *bufptr++ = (num_of_vals & 0xFF);
                    /* Encode FirstValue field */
                    *bufptr++ = ((start_vid >> 8) & 0xFF);
                    *bufptr++ = (start_vid & 0xFF);
                    /* Move the pointer beyond vectors */
                    bufptr += (vec_offset - VTSS_MVRP_VECTOR_HDR_SIZE);
                    /* Initialize for next iteration */
                    vec_offset = VTSS_MVRP_VECTOR_HDR_SIZE;
                    num_of_vals = 0;
                    tmp_len += VTSS_MVRP_VECTOR_HDR_SIZE;
                } /* if (num_of_vals) */
                if (num_of_events == total_events) {
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, DEBUG) << "All events are now processed";
                    break;
                }
                start_vid = indx;
                continue;
            } /* if ((event = VTSS_XXRP_GET_EVENT(all_attr_events, indx)) == VTSS_XXRP_APPL_EVENT_INVALID) */
            if (event != VTSS_XXRP_APPL_EVENT_INVALID && event != VTSS_XXRP_APPL_EVENT_MT) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, DEBUG) << "Encoding " << vtss_mrp_event2txt(event)
                                                          << " event as part of the current MVRPDU";
            }
            vtss_xxrp_update_tx_stats(VTSS_MRP_APPL_MVRP, l2port, event);
            num_of_vals++;
            temp++;
            num_of_events++;
            if (num_of_vals == 1) {
                start_vid = indx + 1;
            } /* if (num_of_vals == 1) */
            if (temp == 1) {
                first_event = event;
            } /* if (temp == 1) */
            if (temp == 2) {
                second_event = event;
            } /* if (temp == 2) */
            if (temp == 3) {
                third_event = event;
                /* We now have 3 consecutive events, so we encode them in a vector and updating counters */
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, RACKET) << "Encoding Full Vector: "
                                                           << first_event << " " << second_event << " " << third_event;
                bufptr[vec_offset++] = ((((first_event * 6) + second_event) * 6) + third_event);
                tmp_len++;
                temp = 0;
                first_event = 0;
                second_event = 0;
                third_event = 0;
            } /* if (temp == 3) */
        } /* for (indx = 0, temp = 0; indx < XXRP_MAX_ATTRS; indx++) */
        if (num_of_vals) { /* If we reach max attributes with valid events */
            if ((temp == 1) || (temp == 2)) {
                /* found partial vector to encode */
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, RACKET) << "Encoding Partial Vector: "
                                                           << first_event << " " << second_event << " " << third_event;
                /* encode three packed event vector */
                bufptr[vec_offset++] = ((((first_event * 6) + second_event) * 6) + third_event);
                tmp_len++;
                temp = 0;
                first_event = 0;
                second_event = 0;
                third_event = 0;
            } /* if ((temp == 1) || (temp == 2)) */
            if (la_flag) {
                la_flag = FALSE; /* Encode LA only once */
                *bufptr++ = ((1 << VTSS_MVRP_LA_BIT_OFFSET) | ((num_of_vals >> 8) & VTSS_MVRP_NUMOFVALS_HIGHER_BYTE_MASK));
                vtss_xxrp_update_tx_stats(VTSS_MRP_APPL_MVRP, l2port, VTSS_XXRP_APPL_EVENT_LA);
            } else {
                *bufptr++ = ((num_of_vals >> 8) & VTSS_MVRP_NUMOFVALS_HIGHER_BYTE_MASK);
            } /* if (la_flag) */
            *bufptr++ = (num_of_vals & 0xFF);
            *bufptr++ = ((start_vid >> 8) & 0xFF);
            *bufptr++ = (start_vid & 0xFF);
            tmp_len += VTSS_MVRP_VECTOR_HDR_SIZE;
        }
    } /* if (total_events) */
    pdu_size += tmp_len; /* Total frame size */
    if (la_flag) { /* This means there are no events to transmit except LA */
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, NOISE) << "MVRPDU is a leave all transmission - no events are encoded";
        /* fill the leave all bit in the LeaveAllEvent field */
        mvrp_pdu[VTSS_MVRP_LA_BYTE_OFFSET - 1] |= (1 << VTSS_MVRP_LA_BIT_OFFSET);
        pdu_size += 4;
        vtss_xxrp_update_tx_stats(VTSS_MRP_APPL_MVRP, l2port, VTSS_XXRP_APPL_EVENT_LA);
    }
    /* Add 4 bytes for the two endmarks */
    pdu_size += 4;
    // xxrp_packet_dump(l2port, mvrp_pdu, TRUE);
    if ((vtss_mrp_mrpdu_tx(l2port, mvrp_pdu, pdu_size)) != VTSS_COMMON_CC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, DEBUG) << "MVRPDU tx failed";
        return XXRP_ERROR_UNKNOWN;
    } else {
        vtss_xxrp_update_tx_stats(VTSS_MRP_APPL_MVRP, l2port, VTSS_XXRP_APPL_EVENT_TX_PKTS);
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, DEBUG) << "MVRPDU was created and sent successfully";
    return VTSS_RC_OK;
}

void vtss_mvrp_added_port(void *mvrp, u32 port_no)
{
}

void vtss_mvrp_removed_port(void *mvrp, u32 port_no)
{
}

mesa_rc vtss_mvrp_global_control_conf_create(vtss_mrp_t **mrp_app)
{
    vtss_mrp_t *mvrp_app;

    /* Create mrp application structure and fill it. This will be freed in MRP on
       global disable of the MRP application  */
    mvrp_app = (vtss_mrp_t *)XXRP_SYS_MALLOC(sizeof(vtss_mrp_t));
    mvrp_app->appl_type = VTSS_MRP_APPL_MVRP;
    mvrp_app->mad = NULL;
    mvrp_app->map = NULL;
    mvrp_app->max_mad_index = MVRP_VLAN_ID_MAX - 1;
    mvrp_app->join_indication_fn = vtss_mvrp_join_indication;
    mvrp_app->leave_indication_fn = vtss_mvrp_leave_indication;
    mvrp_app->join_propagated_fn = vtss_mvrp_join_propagated;
    mvrp_app->leave_propagated_fn = vtss_mvrp_leave_propagated;
    mvrp_app->transmit_fn = vtss_mvrp_tx;
    mvrp_app->receive_fn = vtss_mvrp_pdu_rx;
    mvrp_app->added_port_fn = vtss_mvrp_added_port;
    mvrp_app->removed_port_fn = vtss_mvrp_removed_port;
    *mrp_app = mvrp_app;

    return VTSS_RC_OK;
}
