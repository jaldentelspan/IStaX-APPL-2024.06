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

#include "vtss_xxrp_types.hxx"
#include "vtss_xxrp_util.h"
#include "../platform/xxrp_trace.h"
#include "vtss_xxrp_api.h"
#include "vtss_xxrp_callout.h"
#include "vtss_xxrp_mvrp.h"

#define VTSS_MVRP_LA_BIT_OFFSET 5
/* 13 bits: NumOfVals - 1 bit: LA */
#define VTSS_MVRP_EXTRACT_LA_EVENT(bytes) ((bytes[0] >> VTSS_MVRP_LA_BIT_OFFSET) & 1)

#define VTSS_MVRP_NUMOFVALS_HIGHER_BYTE_MASK  0x1F
#define VTSS_MVRP_VECTOR_HDR_SIZE             4
#define VTSS_MVRP_VECTOR_MAX_EVENTS           3
#define VTSS_XXRP_MIN_PDU_SIZE                25
#define VTSS_MVRP_VERSION                     0
#define VTSS_MRP_ENDMARK                      0
#define VTSS_MVRP_ATTR_TYPE_VLAN              1
#define VTSS_XXRP_MIN_MSG_SIZE                6
#define VTSS_XXRP_VEC_ATTR_HDR_SIZE           4

static const u8 mvrp_multicast_macaddr[] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x21};
static const u8 mvrp_eth_type[]          = {0x88, 0xF5};
#define MVRP_MULTICAST_MACADDR mvrp_multicast_macaddr

namespace vtss {
namespace mrp {

/* MVRP message */
typedef struct {
    u8 attr_type;
    u8 attr_len;
} XXRP_ATTRIBUTE_PACKED mvrp_pdu_msg_t;

/* Fixed fields in MVRP PDU */
typedef struct {
    /* Ethernet Header */
    u8 dst_mac[VTSS_XXRP_MAC_ADDR_LEN];
    u8 src_mac[VTSS_XXRP_MAC_ADDR_LEN];
    u8 eth_type[VTSS_XXRP_ETH_TYPE_LEN];

    /* MVRP PDU start */
    u8 version;
} XXRP_ATTRIBUTE_PACKED mvrp_pdu_fixed_flds_t;

static const char *vtss_mrp_event2txt(u8 event)
{
    switch (event) {
    case MRP_EVENT_NEW:
        return "New";
    case MRP_EVENT_JOININ:
        return "JoinIn";
    case MRP_EVENT_IN:
        return "In";
    case MRP_EVENT_JOINMT:
        return "JoinMt";
    case MRP_EVENT_MT:
        return "Mt";
    case MRP_EVENT_LV:
        return "Leave";
    case MRP_EVENT_INVALID:
        return "Invalid";
    case MRP_EVENT_LA:
        return "LeaveAll";
    default:
        return "Unrecognized";
    }
}

mesa_rc MrpAppl::process_vector(u32 port_no, u16 first_value, u16 num_of_valid_vlans, u8 vector)
{
    mesa_rc rc = VTSS_RC_OK;
    u32 temp;
    u16 vid = first_value;
    u8  event, divider = 36;
    //vtss_mrp_stat_type_t type;
    const VlanList &v = vlan_list();

    /* Three-packed event parsing */
    for (temp = 0; temp < num_of_valid_vlans; temp++) {
        //type = VTSS_MRP_STAT_MAX;

        if (divider == 0) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, ERROR) << "Divider is 0";
            return VTSS_RC_ERROR;
        }

        event = vector / divider;
        /* Do not process any events for non-managed VLANs */
        if (!v.get(vid)) {
            goto next_vid;
        }
        if (event == MRP_EVENT_MT) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, RACKET) << "Received MVRP MT event "
                    << "for VID " << vid;
        } else {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "Received MVRP " << vtss_mrp_event2txt(event)
                    << " event for VID " << vid;
        }
        /* Until now we are unlocked in order to ensure protocol operation
           during the MRPDU processing. But now is the time to take the lock
           since we are about to process the event. */
        /* We only take the lock while the current event is processed,
           hence the scope {} */
        {
            switch (event) {
            case MRP_EVENT_NEW:
                // rNew!
                if (mad[port_no]->regApp.registrar_admin(vid) == XXRP_NORMAL) {
                    (void)mad[port_no]->regApp.handle_event_reg(XXRP_EV_REG_RNEW,
                            port_no,
                            vid);
                }
                (void)mad[port_no]->regApp.handle_event_app(XXRP_EV_APP_RNEW, vid);
                break;
            case MRP_EVENT_JOININ:
                // rJoinIn!
                if (mad[port_no]->regApp.registrar_admin(vid) == XXRP_NORMAL) {
                    (void)mad[port_no]->regApp.handle_event_reg(XXRP_EV_REG_RJOININ,
                            port_no,
                            vid);
                }
                (void)mad[port_no]->regApp.handle_event_app(XXRP_EV_APP_RJOININ, vid);
                break;
            case MRP_EVENT_IN:
                // rIn!
                (void)mad[port_no]->regApp.handle_event_app(XXRP_EV_APP_RIN, vid);
                break;
            case MRP_EVENT_JOINMT:
                // rJoinMt!
                if (mad[port_no]->regApp.registrar_admin(vid) == XXRP_NORMAL) {
                    (void)mad[port_no]->regApp.handle_event_reg(XXRP_EV_REG_RJOINMT,
                            port_no,
                            vid);
                }
                (void)mad[port_no]->regApp.handle_event_app(XXRP_EV_APP_RJOINMT, vid);
                break;
            case MRP_EVENT_MT:
                // rMt!
                (void)mad[port_no]->regApp.handle_event_app(XXRP_EV_APP_RMT, vid);
                break;
            case MRP_EVENT_LV:
                // rLv!
                if (mad[port_no]->regApp.registrar_admin(vid) == XXRP_NORMAL) {
                    (void)mad[port_no]->regApp.handle_event_reg(XXRP_EV_REG_RLEAVE,
                            port_no,
                            vid);
                }
                (void)mad[port_no]->regApp.handle_event_app(XXRP_EV_APP_RLEAVE, vid);
                break;
            case MRP_EVENT_LA:
                // rLA!
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, WARNING) << "LA inside process vector";
                if (mad[port_no]->regApp.registrar_admin(vid) == XXRP_NORMAL) {
                    (void)mad[port_no]->regApp.handle_event_reg(XXRP_EV_REG_RLEAVEALL,
                            port_no,
                            vid);
                }
                (void)mad[port_no]->regApp.handle_event_app(XXRP_EV_APP_RLEAVEALL, vid);
                break;
            }
        }
        //vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, type);
next_vid:
        vector -= (divider * event);
        divider = divider / 6;
        vid++;
    }

    return rc;
}

static inline u16 vtss_mvrp_get_num_of_vals(u8 * bytes)
{
    bytes[0] &= VTSS_MVRP_NUMOFVALS_HIGHER_BYTE_MASK;
    return xxrp_ntohs(bytes);
}

mesa_rc MrpAppl::process_vector_attribute(u32 port_no, mvrp_pdu_vector_attr_t *vec_attr, u16 * size)
{
    i16     number_of_values = 0;
    u16     first_value, temp = 0, num_of_vec_events;
    mesa_rc rc = VTSS_RC_OK;
    bool    leaveall_event;

    leaveall_event = VTSS_MVRP_EXTRACT_LA_EVENT(vec_attr->la_and_num_of_vals);
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "Processing VectorAttribute: LeaveAll and NumberOfValues = 0x "
            << vtss::hex_fixed<2>(vec_attr->la_and_num_of_vals[0])
            << " " << vtss::hex_fixed<2>(vec_attr->la_and_num_of_vals[1]);
    if (leaveall_event) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, DEBUG) << "LeaveAll event received on port " << port_no;
        //vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_LA);
        /* 1: Handle event on LeaveAll STM */
        (void)mad[port_no]->handle_event_leaveall(XXRP_EV_LA_RX);
        /* 2: Handle event on Applicant STMs */
        mad[port_no]->regApp.for_all_machines([&] (mesa_vid_t v, MrpMadState & s) {
            (void)mad[port_no]->regApp.handle_event_app(XXRP_EV_APP_RLEAVEALL, v);
        });
        /* 3: handle event on Registrar STMs */
        mad[port_no]->regApp.for_all_machines([&] (mesa_vid_t v, MrpMadState & s) {
            if (mad[port_no]->regApp.registrar_admin(v) == XXRP_NORMAL) {
                (void)mad[port_no]->regApp.handle_event_reg(XXRP_EV_REG_RLEAVEALL,
                        port_no, v);
            }
        });
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
        rc += process_vector(port_no, first_value, num_of_vec_events, vec_attr->vectors[temp++]);
        number_of_values -= VTSS_MVRP_VECTOR_MAX_EVENTS;
        first_value += VTSS_MVRP_VECTOR_MAX_EVENTS;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "Exit VectorAttribute processing";
    return rc;
}

mesa_rc MrpAppl::receive_frame(u32 port_no, const u8 * pdu, size_t length)
{
    mvrp_pdu_fixed_flds_t  *mrpdu;
    mvrp_pdu_msg_t         *msg;
    mvrp_pdu_vector_attr_t *vec_attr;
    mesa_rc                rc = VTSS_RC_OK;
    u16                    msg_endmark, vec_attr_endmark, vec_attr_size;
    u16                    number_of_values = 0, number_of_vecs = 0;
    u8                     src_mac[VTSS_XXRP_MAC_ADDR_LEN];

    //vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_PKTS);
    if (length < VTSS_XXRP_MIN_PDU_SIZE) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, WARNING) << "Incorrect length of the MVRPDU";
        //vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
        return XXRP_ERROR_FRAME;
    }

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "Initial checks of MVRPDU are done, length of pdu = " << length;
    mrpdu = (mvrp_pdu_fixed_flds_t *)pdu;
    memcpy(src_mac, mrpdu->src_mac, VTSS_XXRP_MAC_ADDR_LEN);
    /* Checking ProtocolVersion field of MVRPDU */
    if (mrpdu->version != VTSS_MVRP_VERSION) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, WARNING) << "MVRP protocol version mismatch, got " << mrpdu->version;
        //vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
        return XXRP_ERROR_FRAME;
    }
    /* Move pdu pointer past the fixed fields */
    pdu += sizeof(mvrp_pdu_fixed_flds_t);
    length -= sizeof(mvrp_pdu_fixed_flds_t);
    msg = (mvrp_pdu_msg_t *)pdu;
    msg_endmark = xxrp_ntohs((u8 *)msg);
    //ScopeLock<Lock> locked(lock_);
    /* Parse the messages till end mark is reached */
    while (msg_endmark != VTSS_MRP_ENDMARK) {
        if (length < VTSS_XXRP_MIN_MSG_SIZE) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, WARNING) << "Incorrect length of the MVRPDU";
            //vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
            return XXRP_ERROR_FRAME;
        }
        /* Checking AttributeType field of MVRPDU */
        if (msg->attr_type != VTSS_MVRP_ATTR_TYPE_VLAN) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, WARNING) << "MVRP attribute type mismatch, got " << msg->attr_type;
            //vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
            return XXRP_ERROR_FRAME;
        }
        /* Checking AttributeLength field of MVRPDU */
        if (msg->attr_len != VTSS_MVRP_ATTR_LEN_VLAN) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, WARNING) << "MVRP attribute length mismatch, got " << msg->attr_len;
            //vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
            return XXRP_ERROR_FRAME;
        }

        /* Move pdu pointer to vector attribute */
        pdu += sizeof(mvrp_pdu_msg_t);
        length -= sizeof(mvrp_pdu_msg_t);
        vec_attr_endmark = xxrp_ntohs(pdu);
        /* Process all the vector attributes until endmark is reached */
        while (vec_attr_endmark != VTSS_MRP_ENDMARK) {
            vec_attr = (mvrp_pdu_vector_attr_t *)pdu;
            if (length < VTSS_XXRP_VEC_ATTR_HDR_SIZE) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, WARNING) << "Incorrect length of the mvrpdu";
                //vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
                return XXRP_ERROR_FRAME;
            }
            number_of_values = (i16)vtss_mvrp_get_num_of_vals(vec_attr->la_and_num_of_vals);
            number_of_vecs = ((number_of_values + (VTSS_MVRP_VECTOR_MAX_EVENTS - 1)) / VTSS_MVRP_VECTOR_MAX_EVENTS);
            if (length < (number_of_vecs + VTSS_XXRP_VEC_ATTR_HDR_SIZE)) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, WARNING) << "Incorrect length of the mvrpdu";
                //vtss_xxrp_update_rx_stats(VTSS_MRP_APPL_MVRP, port_no, VTSS_MRP_RX_DROPPED_PKTS);
                return XXRP_ERROR_FRAME;
            }
            length -= (number_of_vecs + VTSS_XXRP_VEC_ATTR_HDR_SIZE);
            rc = process_vector_attribute(port_no, vec_attr, &vec_attr_size);
            /* Move pdu pointer to next vector attribute */
            pdu += vec_attr_size;
            vec_attr_endmark = xxrp_ntohs(pdu);
        }
        pdu += sizeof(vec_attr_endmark);
        msg = (mvrp_pdu_msg_t *)pdu;
        msg_endmark = xxrp_ntohs((u8 *)msg);
    } /* while (msg->attr_type != VTSS_MRP_ENDMARK) */

    mad[port_no]->update_peer_mac(src_mac);

    return rc;
}

mesa_rc MrpAppl::vtss_mvrp_tx(u32 l2port, u8 * all_attr_events, u32 total_events, BOOL la_flag)
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
    memcpy(bufptr, MVRP_MULTICAST_MACADDR, VTSS_XXRP_MAC_ADDR_LEN);
    bufptr += VTSS_XXRP_MAC_ADDR_LEN;
    /* skip source MAC */
    bufptr += VTSS_XXRP_MAC_ADDR_LEN;
    /* fill MVRP ether_type - 0x88f5 */
    memcpy(bufptr, mvrp_eth_type, VTSS_XXRP_ETH_TYPE_LEN);
    bufptr += VTSS_XXRP_ETH_TYPE_LEN;
    /* fill MVRP protocol version - 0x00 */
    *bufptr++ = VTSS_MVRP_VERSION;
    /* Message */
    *bufptr++ = VTSS_MVRP_ATTR_TYPE_VLAN;
    *bufptr++ = VTSS_MVRP_ATTR_LEN_VLAN;
    /* check for at least one valid event to encode */
    const VlanList &v = vlan_list();
    if (total_events) {
        for (indx = 0, temp = 0; indx < XXRP_MAX_ATTRS; indx++) {
            if (v.get(indx)) {
                event = MRP_GET_EVENT(all_attr_events, indx);
            } else {
                event = MRP_EVENT_INVALID;
            }
            /* Check for continuous vectors */
            if (event == MRP_EVENT_INVALID) {
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
                        //vtss_xxrp_update_tx_stats(VTSS_MRP_APPL_MVRP, l2port, VTSS_XXRP_APPL_EVENT_LA);
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
            }
            if (event != MRP_EVENT_INVALID && event != MRP_EVENT_MT) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, DEBUG) << "Encoding " << vtss_mrp_event2txt(event)
                        << " event as part of the current MVRPDU";
            }
            //vtss_xxrp_update_tx_stats(VTSS_MRP_APPL_MVRP, l2port, event);
            num_of_vals++;
            temp++;
            num_of_events++;
            if (num_of_vals == 1) {
                start_vid = indx;
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
                //vtss_xxrp_update_tx_stats(VTSS_MRP_APPL_MVRP, l2port, VTSS_XXRP_APPL_EVENT_LA);
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
        //vtss_xxrp_update_tx_stats(VTSS_MRP_APPL_MVRP, l2port, VTSS_XXRP_APPL_EVENT_LA);
    }
    /* Add 4 bytes for the two endmarks */
    pdu_size += 4;
    // xxrp_packet_dump(l2port, mvrp_pdu, TRUE);
    if ((vtss_mrp_mrpdu_tx(l2port, mvrp_pdu, pdu_size)) != VTSS_COMMON_CC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, DEBUG) << "MVRPDU tx failed";
        return XXRP_ERROR_UNKNOWN;
    } else {
        //vtss_xxrp_update_tx_stats(VTSS_MRP_APPL_MVRP, l2port, VTSS_XXRP_APPL_EVENT_TX_PKTS);
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, DEBUG) << "MVRPDU was created and sent successfully";
    return VTSS_RC_OK;
}

} // namespace mrp
} // namespace vtss
