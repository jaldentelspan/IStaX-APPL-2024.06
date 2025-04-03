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

#ifndef _VTSS_PTP_PACKET_CALLOUT_H_
#define _VTSS_PTP_PACKET_CALLOUT_H_

#include "vtss_ptp_types.h"

#ifdef __cplusplus
        extern "C" {
#endif
typedef struct {
    mesa_vid_t vid;     /* tag */
    mesa_tagprio_t pcp;
    u16 port;           /* port number */
} ptp_tag_conf_t;

typedef struct {
    mesa_etype_t tpid;
    mesa_vid_t vid;
    mesa_tagprio_t pcp;
} vtss_ptp_tag_t;

/**
 * \file vtss_packet_callout.h
 * \brief PTP main API header file
 *
 * This file contain the definitions of API packet functions and associated
 * types for the 1588 packet interface
 * The functions are called from the ptp base implementation, and expected to be
 * implemented in the pladform specific part.
 *
 */

/**
 * Allocate a buffer and prepare for packet transmission.
 *
 * \param frame pointer to a frame buffer.
 * \param receiver pointer to a receiver structure (mac and IP address).
 * \param size length of data to transmit, excluding encapsulation.
 * \param header_size pointer to where the encapsulation header is stored.
 * \param tag_conf pointer to vlan tag configuration.
 * \return size if buffer allocated, 0 if no buffer alocated
 */
size_t vtss_1588_prepare_general_packet(u8 **frame, vtss_appl_ptp_protocol_adr_t *receiver, size_t size, size_t *header_size, int instance);

size_t vtss_1588_prepare_general_packet_2(u8 **frame, vtss_appl_ptp_protocol_adr_t *sender, vtss_appl_ptp_protocol_adr_t *receiver, size_t size, size_t *header_size, int instance);

void vtss_1588_release_general_packet(u8 **handle);

/**
 * Allocate a buffer for packet transmission.
 *
 * \param handle pointer to a buffer handle.
 * \param size length of data to transmit, including encapsulation.
 * \param frame pointer to where the frame pointer is stored.
 * \return size if buffer allocated, 0 if no buffer alocated
 */
size_t
    vtss_1588_packet_tx_alloc(void **handle, u8 **frame, size_t size);

/**
 * Free a buffer for packet transmission.
 *
 * \param handle pointer to a buffer handle.
 *
 */
void vtss_1588_packet_tx_free(void **handle);


/**
 * Allocate a buffer for packet transmission.
 *
 * \param buf pointer to frame buffer.
 * \param sender pointer to sender protocol adr (=0 if my local address is used).
 * \param receiver pointer to receiver protocol adr.
 * \param size of application data.
 * \param event true if UDP PTP event port(319) is used, false if UDP general port (320) is used
 * \param tag_conf pointer to vlan tag configuration.
 * \return size of encapsulation header
 */
size_t vtss_1588_pack_encap_header(u8 * buf, vtss_appl_ptp_protocol_adr_t *sender, vtss_appl_ptp_protocol_adr_t *receiver, u16 data_size, bool event, int instance);

void vtss_1588_pack_eth_header(u8 * buf, mesa_mac_t receiver);

/**
 * Update encapsulation header, when responding to a request.
 *
 * \param buf   pointer to frame buffer.
 * \param uni   True indicates unicast operation, i.e the src and dst are swapped .
 * \param event True indicates event message type, i.e if UDP port 319 or 320 is used in IP/UDP encapsulation.
 * \param len   Length of application data
 * \return nothing
 */
void vtss_1588_update_encap_header(u8 *buf, bool uni, bool event, u16 len, int instance);

/**
 * Send a general PTP message.
 *
 * \param port_mask switch port mask.
 * \param frame  pointer to data to transmit, including encapsulation.
 * \param size length of data to transmit, including encapsulation.
 * \param context user defined context, user in tx_done callback.
 *
 */
size_t vtss_1588_tx_general(u64 port_mask, u8 *frame, size_t size, vtss_ptp_tag_t *tag);

typedef void (*tx_timestamp_cb_t)(void *context, uint portnum, uint32_t ts_id, u64 tx_time);

typedef struct {
    tx_timestamp_cb_t cb_ts;            /* pointer to function called when tx timestamp is read from the hw */
    void *context;                      /* user defined context used as parameter to the timestamp callback */
} ptp_tx_timestamp_context_t;

void vtss_1588_tag_get(ptp_tag_conf_t *tag_conf, int instance, vtss_ptp_tag_t *tag);

typedef enum {
    VTSS_PTP_MSG_TYPE_GENERAL,
    VTSS_PTP_MSG_TYPE_2_STEP,
    VTSS_PTP_MSG_TYPE_CORR_FIELD,
    VTSS_PTP_MSG_TYPE_ORG_TIME
} vtss_ptp_msg_type_t;

typedef struct ptp_tx_buffer_handle_s { /* tx buffer handle for tx messages*/
    void *handle;                       /* handle for allocated tx buffer, this handle may be 0, indicating that the
                                           application does not need to care about free'ing the buffer */
    u8 *frame;                          /* pointer to allocated tx frame buffer */
    size_t size;                        /* length of data to transmit, including encapsulation. */
    u32 header_length;                  /* length of encapsulation header */
    mesa_packet_inj_encap_t inj_encap;  /* specify type of encapsulation: NotUsed/Ethernet/IPv4/Ipv6 */
    vtss_ptp_msg_type_t msg_type;       /* 0 = general, 1 = 2-step, 2 = correctionfield update, 3 = org_time */
    u64 hw_time;                        /* internal HW time value used for correction field update. */
    ptp_tx_timestamp_context_t *ts_done;/* user defined context used as parameter to the timestamp callback */
    vtss_ptp_tag_t tag;
} ptp_tx_buffer_handle_t;

void vtss_1588_tx_handle_init(ptp_tx_buffer_handle_t *ptp_buf_handle);

size_t vtss_1588_prepare_tx_buffer(ptp_tx_buffer_handle_t *tx_buf, u32 length, bool tc);


/**
 * Send PTP messages using automatic frame injection.
 *
 * \param port_no switch port.
 * \param ptp_buf_handle ptp transmit buffer handle
 * \param log_sync_interval message interval (-7..0)
 *
 */
mesa_rc vtss_1588_afi(void **afi, u64 port_mask,
                     ptp_tx_buffer_handle_t *ptp_buf_handle, i8 log_sync_interval, BOOL resume);

/**
 * read number of messages transmitted using automatic frame injection.
 *
 * \param afi  pointer to afi handle.
 * \param OUT the number of packets transmitted since latest call.
 */
mesa_rc vtss_1588_afi_packet_tx(void ** afi, u32* cnt_delta);

bool vtss_1588_check_transmit_resources(int instance);

/**
 * Send a PTP message.
 *
 * \param port_mask switch port mask.
 * \param ptp_buf_handle ptp transmit buffer handle
 * \param instance number
 * \param cmlds => true if this message is sent by Common mean link delay service but not any specific instance.
 * \param ts_id returns timestamp id used for switch timestamping.
 *
 */
size_t vtss_1588_tx_msg(u64 port_mask,
                        ptp_tx_buffer_handle_t *ptp_buf_handle,
                        int instance, bool cmlds,
                        uint32_t *ts_id);
/**
 * Send a Signalling PTP message, to an IP address, where the port and MAC
 * address is unknown.
 *
 * \param dest_ip  Destination IP address.
 *
 * \param buffer pointer to data to send.
 *
 * \param size length of data to transmit.
 *
 */
size_t vtss_1588_tx_unicast_request(u32 dest_ip, const void *buffer, size_t size, int instance);

/**
 * \brief Set the ingress latency
 * \param portnum port number
 * \param ingress_latency ingress latency
 *
 */
mesa_rc vtss_1588_ingress_latency_set(u16 portnum, mesa_timeinterval_t ingress_latency);

/**
 * \brief Get the ingress latency
 * \param portnum port number
 * \param ingress_latency ingress latency
 *
 */
void vtss_1588_ingress_latency_get(u16 portnum, mesa_timeinterval_t *ingress_latency);

/**
 * \brief Set the egress latency
 * \param portnum port number
 * \param egress_latency egress latency
 *
 */
mesa_rc vtss_1588_egress_latency_set(u16 portnum, mesa_timeinterval_t egress_latency);

/**
 * \brief Get the egress latency
 * \param portnum port number
 * \param egress_latency egress latency
 *
 */
void vtss_1588_egress_latency_get(u16 portnum, mesa_timeinterval_t *egress_latency);

/**
 * \brief Set the P2P path delay
 * \param portnum port number
 * \param p2p_delay P2P path delay
 *
 */
void vtss_1588_p2p_delay_set(u16 portnum, mesa_timeinterval_t p2p_delay);

/**
 * \brief Set the link asymmetry
 * \param portnum port number
 * \param asymmetry link asymmetry
 *
 */
mesa_rc vtss_1588_asymmetry_set(u16 portnum, mesa_timeinterval_t asymmetry);

/**
 * \brief calculate difference between two tc counters.
 * \param r result = x-y
 * \param x tc counter
 * \param y tc counter
 */
void vtss_1588_ts_cnt_sub(u64 *r, u64 x, u64 y);

/**
 * \brief calculate sum of two tc counters.
 * \param r result = x+y
 * \param x tc counter
 * \param y tc counter
 */
void vtss_1588_ts_cnt_add(u64 *r, u64 x, u64 y);

void vtss_1588_timeinterval_to_ts_cnt(u64 *r, mesa_timeinterval_t x);

void vtss_1588_ts_cnt_to_timeinterval(mesa_timeinterval_t *r, u64 x);

/**
 * \brief Set/Get the clock mode for the HW clock
 *
 * \param mode FreeRun, Locking or Locked.
 *
 */
void vtss_local_clock_mode_set(vtss_ptp_clock_mode_t mode);
void vtss_local_clock_mode_get(vtss_ptp_clock_mode_t *mode);

bool vtss_1588_external_pdv(u32 clock_id);

/**
 * Get the actual org_time_option for a PTP instance.
 *
 * \param instance   PTP instance number
 * \param portnum    PTP port number
 * \param org_time_option True indicates originTimestamp is inserted into Sync messages.
 * \return nothing
 */
void vtss_1588_org_time_option_get(int instance, u16 portnum, bool *org_time_option);

void vtss_ptp_ptsf_state_set(const int instance);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_PTP_PACKET_CALLOUT_H_ */
