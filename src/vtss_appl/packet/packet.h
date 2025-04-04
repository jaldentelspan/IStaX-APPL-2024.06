/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_PACKET_H_
#define _VTSS_PACKET_H_

#include <vtss_module_id.h>

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_PACKET

#define VTSS_TRACE_GRP_DEFAULT 0
#define TRACE_GRP_RX           1
#define TRACE_GRP_TX           2
#define TRACE_GRP_NETLINK      3

#include <vtss_trace_api.h>
#include "critd_api.h"

// This macro must *not* evaluate to an empty macro, since it's expected to do some useful stuff.
#define PACKET_CHECK(x, y) do {if (!(x)) {T_E("Assertion failed: " #x); y}} while (0)

typedef struct tag_packet_rx_filter_item {
    packet_rx_filter_t               filter;
    struct tag_packet_rx_filter_item *next;
} packet_rx_filter_item_t;

typedef struct tag_packet_port_counters {
    // The counters for received packets per port.
    // Frames received on GLAGs are counted in the last
    // VTSS_GLAGS bins.
    // The priority is taken from the frame's IFH, but if the
    // packet is received with a VStaX header and the header's
    // Super Priority bit is set, it's counted in the VTSS_PRIO + 1
    // bucket.
    // Frames received on unknown ports (e.g. sFlow frames received on
    // interconnect ports in JR-48 solutions) are counted in the
    // very last bucket.
    CapArray<u32, VTSS_APPL_CAP_PACKET_RX_PORT_CNT, VTSS_APPL_CAP_PACKET_RX_PRIO_CNT> rx_pkts;

    // The counters for transmitted packets are counted per the expected
    // priority on the Rx side.
    // The first VTSS_PORTS entries are for the logical ports.
    // The next entry (VTSS_PORTS + 1) is for switched frames.
    // The last entry (VTSS_PORTS + 2) is for multicast frames.
    CapArray<u32, VTSS_APPL_CAP_PACKET_TX_PORT_CNT> tx_pkts;
} packet_port_counters_t;

typedef struct tag_packet_module_counters {
    u32              rx_pkts;
    u64              rx_bytes;
    u32              tx_pkts;
    u64              tx_bytes;
    vtss_tick_count_t longest_rx_callback_ticks;
} packet_module_counters_t;

#define BITMASK(x) ((1U << (x)) - 1U)

#define DMAC_POS                    0
#define SMAC_POS                    (VTSS_MAC_ADDR_SZ_BYTES)
#define ETYPE_POS                   (2 * (VTSS_MAC_ADDR_SZ_BYTES))
#define ETYPE_INNER_POS             (ETYPE_POS + 4)
#define PROT_HDR_POS                (ETYPE_POS + 2)
#define EPID_POS                    PROT_HDR_POS
#define SSPID_POS                   (EPID_POS + 2)
#define IP_VER_POS                  (PROT_HDR_POS)
#define IPV4_HLEN_POS               (PROT_HDR_POS)
#define IPV4_PROTO_POS              (PROT_HDR_POS + 9)
#define IPV6_PROTO_POS              (PROT_HDR_POS + 6) /* Only first next header is checked */
#define UDP_HDR_POS(_ip_hlen_)      (PROT_HDR_POS + (_ip_hlen_))
#define TCP_HDR_POS(_ip_hlen_)      (PROT_HDR_POS + (_ip_hlen_))
#define UDP_SRC_PORT_POS(_ip_hlen_) (UDP_HDR_POS(_ip_hlen_) + 0)
#define UDP_DST_PORT_POS(_ip_hlen_) (UDP_HDR_POS(_ip_hlen_) + 2)
#define TCP_SRC_PORT_POS(_ip_hlen_) (TCP_HDR_POS(_ip_hlen_) + 0)
#define TCP_DST_PORT_POS(_ip_hlen_) (TCP_HDR_POS(_ip_hlen_) + 2)

#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))

#endif /* _VTSS_PACKET_H_ */

