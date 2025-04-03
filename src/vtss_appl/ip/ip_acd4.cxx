/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "dhcp_client_api.h"
#include "ip_acd4.hxx"
#include "ip_expose.hxx"
#include "ip_lock.hxx"
#include "ip_os.hxx"
#include "ip_trace.h"
#include "ip_utils.hxx"
#include "mac_utils.hxx"
#include "icli_api.h"
#include "packet_api.h"
#include <vtss/basics/types.hxx>

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

// RFC 5227 Address Conflict Detection, all delays in seconds
#define IP_ACD4_PROBE_NUM         3 // Number of probes
#define IP_ACD4_PROBE_WAIT        1 // Delay before sending first probe
#define IP_ACD4_PROBE_MIN         1 // Minimum delay between probes
#define IP_ACD4_PROBE_MAX         2 // Maximum delay between probes
#define IP_ACD4_ANNOUNCE_NUM      2 // Number of announcements
#define IP_ACD4_ANNOUNCE_WAIT     2 // Delay before first announcement
#define IP_ACD4_ANNOUNCE_INTERVAL 2 // Delay between announcements

StatusAcdIpv4             status_acd_ipv4("status_acd_ipv4", VTSS_MODULE_ID_IP);
static packet_rx_filter_t IP_ACD4_arp_rx_filter;

// MAC/ARP header offsets and values
#define IP_ACD4_O_MAC_DMAC   0  // DMAC
#define IP_ACD4_O_MAC_SMAC   6  // SMAC
#define IP_ACD4_O_MAC_ETYPE  12 // Type/length
#define IP_ACD4_O_ARP_HTYPE  14 // Hardware type
#define IP_ACD4_O_ARP_PTYPE  16 // Protocol type
#define IP_ACD4_O_ARP_HLEN   18 // Hardware address length
#define IP_ACD4_O_ARP_PLEN   19 // Protocol address length
#define IP_ACD4_O_ARP_OPER   20 // Opcode
#define IP_ACD4_O_ARP_SHA    22 // Sender hardware address
#define IP_ACD4_O_ARP_SPA    28 // Sender protocol address
#define IP_ACD4_O_ARP_THA    32 // Target hardware address
#define IP_ACD4_O_ARP_TPA    38 // Target protocol address

#define IP_ACD4_ARP_LEN      42     // ARP Ethernet/IPv4 message length
#define IP_ACD4_ARP_ETYPE    0x0806 // ARP Ethernet type
#define IP_ACD4_ARP_HTYPE    1      // Hardware address type Ethernet
#define IP_ACD4_ARP_PTYPE    0x0800 // Protocol address type IPv4
#define IP_ACD4_ARP_HLEN     6      // Ethernet address length
#define IP_ACD4_ARP_PLEN     4      // IPv4 address length
#define IP_ACD4_ARP_OPER_REQ 1      // ARP request
#define IP_ACD4_ARP_OPER_REP 2      // ARP response

#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
/******************************************************************************/
// IP_ACD4_sm_state_to_str()
/******************************************************************************/
static const char *IP_ACD4_sm_state_to_str(ip_acd4_sm_state_t sm_state)
{
    switch (sm_state) {
    case IP_ACD4_SM_STATE_IDLE:
        return "IDLE";

    case IP_ACD4_SM_STATE_PROBING:
        return "PROBING";

    case IP_ACD4_SM_STATE_BOUND:
        return "BOUND";

    default:
        T_EG(IP_TRACE_GRP_ACD, "Unknown sm_state (%d)", sm_state);
        return "<Unknown>";
    }
}
#endif

/******************************************************************************/
// getb16()
/******************************************************************************/
static uint16_t getb16(const uint8_t *p, int offs)
{
    return ((p[offs] << 8) | p[offs + 1]);
}

/******************************************************************************/
// getb32()
/******************************************************************************/
static uint32_t getb32(const uint8_t *p, int offs)
{
    return ((p[offs] << 24) | (p[offs + 1] << 16) | (p[offs + 2] << 8) | p[offs + 3]);
}

/******************************************************************************/
// putb16()
/******************************************************************************/
static void putb16(uint8_t *p, int offs, uint16_t val)
{
    p[offs + 0] = (val >> 8);
    p[offs + 1] = (val >> 0);
}

/******************************************************************************/
// putb32()
/******************************************************************************/
static void putb32(uint8_t *p, int offs, uint32_t val)
{
    p[offs + 0] = (val >> 24);
    p[offs + 1] = (val >> 16);
    p[offs + 2] = (val >> 8);
    p[offs + 3] = (val >> 0);
}

/******************************************************************************/
// arp_tx()
/******************************************************************************/
void arp_tx(ip_if_itr_t if_itr, bool announce)
{
    ip_acd4_state_t   *acd_state = &if_itr->second.acd_state;
    packet_tx_props_t tx_props;
    uint8_t           *frm, i, *smac = if_itr->second.mac_address.addr;
    mesa_vid_t        vid;

    if ((vid = vtss_ifindex_as_vlan(if_itr->first)) == 0) {
        return;
    }

    T_IG(IP_TRACE_GRP_ACD, "Tx ARP %s, interface %u", announce ? "Announce" : "Probe", if_itr->first);
    if ((frm = packet_tx_alloc(IP_ACD4_ARP_LEN)) == nullptr) {
        T_WG(IP_TRACE_GRP_ACD, "packet_tx_alloc(%u) failed", IP_ACD4_ARP_LEN);
        return;
    }

    memset(frm, 0, IP_ACD4_ARP_LEN);
    for (i = 0; i < 6; i++) {
        frm[IP_ACD4_O_MAC_DMAC + i] = 0xff;
        frm[IP_ACD4_O_MAC_SMAC + i] = smac[i];
        frm[IP_ACD4_O_ARP_SHA  + i] = smac[i];
    }

    putb16(frm, IP_ACD4_O_MAC_ETYPE,  IP_ACD4_ARP_ETYPE);
    putb16(frm, IP_ACD4_O_ARP_HTYPE,  IP_ACD4_ARP_HTYPE);
    putb16(frm, IP_ACD4_O_ARP_PTYPE,  IP_ACD4_ARP_PTYPE);
    frm[        IP_ACD4_O_ARP_HLEN] = IP_ACD4_ARP_HLEN;
    frm[        IP_ACD4_O_ARP_PLEN] = IP_ACD4_ARP_PLEN;
    putb16(frm, IP_ACD4_O_ARP_OPER,   IP_ACD4_ARP_OPER_REQ);
    putb32(frm, IP_ACD4_O_ARP_SPA, announce ? acd_state->net.address : 0);
    putb32(frm, IP_ACD4_O_ARP_TPA, acd_state->net.address);

    packet_tx_props_init(&tx_props);
    tx_props.packet_info.modid  = VTSS_MODULE_ID_IP;
    tx_props.tx_info.switch_frm = TRUE;
    tx_props.tx_info.tag.vid    = vid;
    tx_props.packet_info.frm    = frm;
    tx_props.packet_info.len    = IP_ACD4_ARP_LEN;
    if (packet_tx(&tx_props) != VTSS_RC_OK) {
        T_WG(IP_TRACE_GRP_ACD, "packet_tx() failed");
    }
}

/******************************************************************************/
// IP_ACD4_timer_start()
/******************************************************************************/
static void IP_ACD4_timer_start(ip_if_itr_t if_itr, uint32_t scale, uint32_t offset_sec)
{
    uint64_t ms;
    mesa_rc  rc;

    if (scale) {
        ms = rand();
        ms = ((ms * 1000) / RAND_MAX); // 0-1000 ms
        ms *= scale;
    } else {
        ms = 0;
    }

    ms += 1000 * offset_sec;

    T_DG(IP_TRACE_GRP_ACD, "%u: Starting ACD timer with timeout in " VPRI64u " ms", VTSS_IFINDEX_PRINTF_ARG(if_itr->first), ms);

    if ((rc = vtss_timer_cancel(&if_itr->second.acd_state.timer)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_ACD, "%u: Unable to stop timer: %s", VTSS_IFINDEX_PRINTF_ARG(if_itr->first), error_txt(rc));
    }

    if_itr->second.acd_state.timer.set_period(vtss::milliseconds(ms));

    if ((rc = vtss_timer_start(&if_itr->second.acd_state.timer)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_ACD, "%u: Unable to start timer (" VPRI64u " ms): %s", VTSS_IFINDEX_PRINTF_ARG(if_itr->first), ms, error_txt(rc));
    }
}

/******************************************************************************/
// new_state()
/******************************************************************************/
static void new_state(ip_if_itr_t if_itr, ip_acd4_sm_state_t s)
{
    ip_acd4_state_t    *acd_state = &if_itr->second.acd_state;
    ip_acd4_sm_state_t &sm_state  = acd_state->sm_state;
    uint32_t           &count     = acd_state->count;

    if (sm_state != s) {
        T_IG(IP_TRACE_GRP_ACD, "ACD SM state: %s->%s", IP_ACD4_sm_state_to_str(sm_state), IP_ACD4_sm_state_to_str(s));
        count    = 0;
        sm_state = s;
        (void)vtss_timer_cancel(&acd_state->timer);
    }

    switch (s) {
    case IP_ACD4_SM_STATE_PROBING:
        if (count > IP_ACD4_PROBE_NUM) {
            // Probing done. Go to BOUND state.
            new_state(if_itr, IP_ACD4_SM_STATE_BOUND);
            if (if_itr->second.if_config.ipv4.dhcpc_enable) {
                vtss::dhcp::client_bind(if_itr->first);
            }
        } else {
            if (count) {
                arp_tx(if_itr, false);
            }

            if (count == IP_ACD4_PROBE_NUM) {
                IP_ACD4_timer_start(if_itr, 0, IP_ACD4_ANNOUNCE_WAIT);
            } else if (count) {
                IP_ACD4_timer_start(if_itr, IP_ACD4_PROBE_MAX - IP_ACD4_PROBE_MIN, IP_ACD4_PROBE_MIN);
            } else {
                IP_ACD4_timer_start(if_itr, IP_ACD4_PROBE_WAIT, 0);
            }

            count++;
        }

        break;

    case IP_ACD4_SM_STATE_BOUND:
        if (count < IP_ACD4_ANNOUNCE_NUM) {
            arp_tx(if_itr, true);

            if (count == 0) {
                if (ip_os_ipv4_add(if_itr->first, &acd_state->net) == VTSS_RC_OK) {
                    if_itr->second.ipv4_active = true;
                    ip_if_signal(if_itr->first);
                } else {
                    T_IG(IP_TRACE_GRP_ACD, "Failed to set IPv4 address %s on interface %u", acd_state->net, if_itr->first);
                }

                IP_ACD4_timer_start(if_itr, 0, IP_ACD4_ANNOUNCE_INTERVAL);
            }

            count++;
        }

        break;

    default:
        break;
    }
}

/******************************************************************************/
// IP_ACD4_arp_rx()
/******************************************************************************/
static void IP_ACD4_arp_rx(ip_if_itr_t if_itr, const uchar *frm, const mesa_packet_rx_info_t *rx_info)
{
    ip_acd4_state_t                    *acd_state = &if_itr->second.acd_state;
    uint16_t                           oper, i;
    vtss_appl_ip_acd_status_ipv4_key_t key;
    vtss_appl_ip_acd_status_ipv4_t     status;
    vtss::StringStream                 log_str;

    // Ignore ARP messages in IDLE state
    if (acd_state->sm_state == IP_ACD4_SM_STATE_IDLE) {
        return;
    }

    T_NG(IP_TRACE_GRP_ACD, "vid: %u, port_no: %u, len: %u", rx_info->tag.vid, rx_info->port_no, rx_info->length);

    // Only process valid Ethernet/IPv4 ARP request/response messages
    oper = getb16(frm, IP_ACD4_O_ARP_OPER);
    if (oper != IP_ACD4_ARP_OPER_REQ && oper != IP_ACD4_ARP_OPER_REP) {
        return;
    }

    if (rx_info->length                  < IP_ACD4_ARP_LEN    ||
        getb16(frm, IP_ACD4_O_ARP_HTYPE) != IP_ACD4_ARP_HTYPE ||
        getb16(frm, IP_ACD4_O_ARP_PTYPE) != IP_ACD4_ARP_PTYPE ||
        frm[        IP_ACD4_O_ARP_HLEN]  != IP_ACD4_ARP_HLEN  ||
        frm[        IP_ACD4_O_ARP_PLEN]  != IP_ACD4_ARP_PLEN) {
        return;
    }

    // It is a conflict if the sender IP matches and the sender MAC doesn't
    for (i = 0; i < 6; i++) {
        key.smac.addr[i] = frm[IP_ACD4_O_ARP_SHA + i];
    }

    key.sip = getb32(frm, IP_ACD4_O_ARP_SPA);

    if (key.sip != acd_state->net.address || key.smac == if_itr->second.mac_address) {
        // Not our Source IP or our own MAC address. Skip.
        return;
    }

    status.ifindex = if_itr->first;

    if (vtss_ifindex_from_port(VTSS_ISID_START, rx_info->port_no, &status.ifindex_port) == VTSS_RC_OK) {
        T_IG(IP_TRACE_GRP_ACD, "ACD add, key: %s, status: %s", key, status);

        if (status_acd_ipv4.set(&key, &status) != VTSS_RC_OK) {
            T_EG(IP_TRACE_GRP_ACD, "%u: status_acd_ipv4.set() failed", VTSS_IFINDEX_PRINTF_ARG(if_itr->first));
        }
    }

    if (acd_state->sm_state == IP_ACD4_SM_STATE_PROBING) {
        // Avoid using address
        new_state(if_itr, IP_ACD4_SM_STATE_IDLE);

        if (if_itr->second.if_config.ipv4.dhcpc_enable) {
            vtss::dhcp::client_decline(if_itr->first);
        }

        log_str << "Duplicate address " << vtss::AsIpv4(key.sip)
                << " on "                       << status.ifindex_port
                << ", sourced by "              << key.smac
                << ". Vlan "                    << rx_info->tag.vid << " did not receive an address"
                << ". DHCP client will retry to obtain an address in a few seconds.";
        T_IG(IP_TRACE_GRP_ACD, "%s", log_str.cstring());

        // We log the message on syslog and all active ICLI sessions,
        // since the address conflict detection is an asynchronous
        // process and it came from the user manual address assignment.

#ifdef VTSS_SW_OPTION_SYSLOG
        S_I("%%IP-4-DUPADDR: %s", log_str.cstring());
#endif /* VTSS_SW_OPTION_SYSLOG */

        // Alert message on all ICLI sessions
        (void)icli_session_printf_to_all("%%IP-4-DUPADDR: %s\n", log_str.cstring());
    }
}

/******************************************************************************/
// IP_ACD4_arp_rx_callback()
/******************************************************************************/
static BOOL IP_ACD4_arp_rx_callback(void *contxt, const unsigned char *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    ip_if_itr_t    if_itr;
    mesa_vid_t     vid     = rx_info->tag.vid;
    vtss_ifindex_t ifindex = vtss_ifindex_cast_from_u32(vid, VTSS_IFINDEX_TYPE_VLAN);

    IP_LOCK_SCOPE();

    if (ip_if_exists(ifindex, &if_itr) != VTSS_RC_OK) {
        return FALSE;
    }

    IP_ACD4_arp_rx(if_itr, frm, rx_info);

    return FALSE; // Allow other subscribers to receive ARP frames
}

/******************************************************************************/
// IP_ACD4_timeout()
/******************************************************************************/
static void IP_ACD4_timeout(vtss::Timer *timer)
{
    ip_if_itr_t    if_itr;
    vtss_ifindex_t ifindex;

    if (timer == nullptr || timer->user_data == nullptr) {
        T_EG(IP_TRACE_GRP_ACD, "Invalid timer");
        return;
    }

    ifindex.private_ifindex_data_do_not_use_directly = (uint32_t)(uintptr_t)timer->user_data;

    IP_LOCK_SCOPE();

    if (ip_if_exists(ifindex, &if_itr) != VTSS_RC_OK) {
        return;
    }

    T_DG(IP_TRACE_GRP_ACD, "ACD timer event on I/F %u", VTSS_IFINDEX_PRINTF_ARG(if_itr->first));
    new_state(if_itr, if_itr->second.acd_state.sm_state);
}

/******************************************************************************/
// ip_acd4_sm_start()
/******************************************************************************/
void ip_acd4_sm_start(ip_if_itr_t if_itr, const mesa_ipv4_network_t *new_net)
{
    if_itr->second.acd_state.net = *new_net;
    T_IG(IP_TRACE_GRP_ACD, "%u: Start", VTSS_IFINDEX_PRINTF_ARG(if_itr->first));
    new_state(if_itr, IP_ACD4_SM_STATE_PROBING);
}

/******************************************************************************/
// ip_acd4_sm_stop()
/******************************************************************************/
void ip_acd4_sm_stop(ip_if_itr_t if_itr)
{
    vtss_appl_ip_acd_status_ipv4_key_t key;
    vtss_appl_ip_acd_status_ipv4_t     status;

    T_IG(IP_TRACE_GRP_ACD, "%u: Stop", VTSS_IFINDEX_PRINTF_ARG(if_itr->first));

    new_state(if_itr, IP_ACD4_SM_STATE_IDLE);

    // Delete previous conflict entries on interface
    memset(&key, 0, sizeof(key));

    while (vtss_appl_ip_acd_status_ipv4_itr(&key, &key) == VTSS_RC_OK) {
        if (vtss_appl_ip_acd_status_ipv4_get(&key, &status) != VTSS_RC_OK) {
            continue;
        }

        if (status.ifindex != if_itr->first) {
            continue;
        }

        T_IG(IP_TRACE_GRP_ACD, "%u: ACD delete. Key = %s", VTSS_IFINDEX_PRINTF_ARG(if_itr->first), key);

        if (status_acd_ipv4.del(&key) != VTSS_RC_OK) {
            T_EG(IP_TRACE_GRP_ACD, "%u: status_acd_ipv4.del(%s) failed", VTSS_IFINDEX_PRINTF_ARG(if_itr->first), key);
        }
    }
}

/******************************************************************************/
// ip_acd4_sm_init()
/******************************************************************************/
void ip_acd4_sm_init(ip_if_itr_t if_itr)
{
    ip_acd4_state_t *acd_state = &if_itr->second.acd_state;

    acd_state->sm_state        = IP_ACD4_SM_STATE_IDLE;
    acd_state->timer.callback  = IP_ACD4_timeout;
    acd_state->timer.user_data = (void *)(uintptr_t)if_itr->first.private_ifindex_data_do_not_use_directly;
    acd_state->timer.modid     = VTSS_MODULE_ID_IP;
}

/******************************************************************************/
// ip_acd4_init()
/******************************************************************************/
void ip_acd4_init(void)
{
    void    *filter_id;
    mesa_rc rc;

    // Register for ARP frames
    packet_rx_filter_init(&IP_ACD4_arp_rx_filter);
    IP_ACD4_arp_rx_filter.modid = VTSS_MODULE_ID_IP;
    IP_ACD4_arp_rx_filter.match = PACKET_RX_FILTER_MATCH_ETYPE;
    IP_ACD4_arp_rx_filter.etype = 0x0806;
    IP_ACD4_arp_rx_filter.prio  = PACKET_RX_FILTER_PRIO_NORMAL;
    IP_ACD4_arp_rx_filter.cb    = IP_ACD4_arp_rx_callback;
    if ((rc = packet_rx_filter_register(&IP_ACD4_arp_rx_filter, &filter_id)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_ACD, "packet_rx_filter_register() failed: %s", error_txt(rc));
    }
}

