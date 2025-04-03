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

#include "types.hxx"
#include "vtss_dhcp6_core.hxx"
#include "dhcp6c_priv.hxx"
#include "ip_utils.hxx"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_DHCP6C
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_DHCP6C

#define INVALID_CHECKSUM 0

namespace vtss
{
static uint16_t tcpudp6_chksum_calc(const mesa_ipv6_t *src, const mesa_ipv6_t *dst, uint16_t len, uint8_t *data)
{
    uint8_t  *ptr;
    uint32_t sum = 0;
    int32_t  i;
    struct {
        mesa_ipv6_t src;     // Source Address
        mesa_ipv6_t dst;     // Destination Address
        uint32_t    len;     // Upper-Layer Packet Length
        uint8_t     zero[3]; // Must be zero
        uint8_t     nxt_hdr; // Next Header
    } VTSS_IPV6_PACK_STRUCT pseudo_hdr;

    vtss_clear(pseudo_hdr);

    pseudo_hdr.src     = *src; // Already in network order
    pseudo_hdr.dst     = *dst; // Ditto
    pseudo_hdr.len     = htonl(len);
    pseudo_hdr.nxt_hdr = VTSS_IPV6_HEADER_NXTHDR_UDP;

    if (sizeof(pseudo_hdr) != 40) {
        T_E("What? sizeof(Packed IPv6 pseudo header) is not 40, but %zu", sizeof(pseudo_hdr));
        return INVALID_CHECKSUM;
    }

    ptr = (uint8_t *)&pseudo_hdr;
    for (i = 0; i < sizeof(pseudo_hdr); i += 2) {
        sum += (ptr[0] << 8) | ptr[1];
        ptr += 2;
    }

    ptr = data;
    for (i = 0; i < len; i += 2) {
        sum += (ptr[0] << 8) | ptr[1];
        ptr += 2;
    }

    // Take care of an odd number of bytes by padding with a zero.
    if (len % 2) {
        sum += (ptr[0] << 8) | 0;
    }

    // Fold the sum to 16 bits.
    sum = (sum >> 16) + (sum & 0xffff);

    // This could give rise to yet another carry, that we need to take into
    // account:
    sum += sum >> 16;

    // One's complement
    sum = ~sum & 0xffff;

    if (sum == 0) {
        sum = 0xffff;
    }

    T_I("sum = 0x%04x after one's complement", sum);
    return sum;
}

namespace dhcp6
{
static BOOL DHCP6_rx_sanity(const u8 *const frm, u32 len)
{
    Ip6Header *frm_ip;
    UdpHeader *frm_udp;
    uint16_t  chksum;

    if (!frm) {
        return FALSE;
    }

    frm_ip = (Ip6Header *)frm;
    if (IPV6_IP_HEADER_VERSION(frm_ip) != VTSS_IPV6_HEADER_VERSION) {
        T_D("Version:%u", IPV6_IP_HEADER_VERSION(frm_ip));
        return FALSE;
    }

    if (IPV6_IP_HEADER_NXTHDR(frm_ip) != VTSS_IPV6_HEADER_NXTHDR_UDP) {
        T_D("NextHeader:%u", IPV6_IP_HEADER_NXTHDR(frm_ip));
        return FALSE;
    }

    if (IPV6_IP_HEADER_PAYLOAD_LEN(frm_ip) + sizeof(Ip6Header) > len) {
        T_D("PayloadLen:%u + IPv6Header:%zu > LEN:%u", IPV6_IP_HEADER_PAYLOAD_LEN(frm_ip), sizeof(Ip6Header), len);
        return FALSE;
    }

    frm_udp = (UdpHeader *)((u8 *)frm_ip + sizeof(Ip6Header));
    if (IPV6_UDP_HEADER_CHECKSUM(frm_udp) == INVALID_CHECKSUM) {
        T_D("Checksum inside frame is invalid. It must not be 0x0000");
        return FALSE;
    }

    if (IPV6_UDP_HEADER_SRC_PORT(frm_udp) != VTSS_DHCP6_CLIENT_UDP_PORT &&
        IPV6_UDP_HEADER_SRC_PORT(frm_udp) != VTSS_DHCP6_SERVER_UDP_PORT &&
        IPV6_UDP_HEADER_DST_PORT(frm_udp) != VTSS_DHCP6_CLIENT_UDP_PORT &&
        IPV6_UDP_HEADER_DST_PORT(frm_udp) != VTSS_DHCP6_SERVER_UDP_PORT) {
        T_D("UDP-SRC-PORT:%u / UDP-DST-PORT:%u",
            IPV6_UDP_HEADER_SRC_PORT(frm_udp),
            IPV6_UDP_HEADER_DST_PORT(frm_udp));
        return FALSE;
    }

    if (IPV6_UDP_HEADER_LENGTH(frm_udp) != IPV6_IP_HEADER_PAYLOAD_LEN(frm_ip)) {
        T_D("UdpHdrLen(%u) != IpHdrLen(%u)",
            IPV6_UDP_HEADER_LENGTH(frm_udp),
            IPV6_IP_HEADER_PAYLOAD_LEN(frm_ip));
        return FALSE;
    }

    // Upon Rx, the computed checksum must be equal to 0xffff, because the
    // checksum calculation includes the received checksum itself.
    chksum = tcpudp6_chksum_calc(&frm_ip->src, &frm_ip->dst, IPV6_UDP_HEADER_LENGTH(frm_udp), (uint8_t *)frm_ip + sizeof(Ip6Header));
    if (chksum != 0xffff) {
        T_I("Invalid checksum. Checksum in frame = 0x%04x, calculated = 0x%04x, expected 0xffff", IPV6_UDP_HEADER_CHECKSUM(frm_udp), chksum);
        return FALSE;
    }

    return TRUE;
}

static BOOL DHCP6_ipstk_address_get(mesa_vid_t vdx, const mesa_ipv6_t *const dip)
{
    vtss_ifindex_t             ifindex;
    vtss_appl_ip_if_key_ipv6_t key = {};

    if (vtss_ifindex_from_vlan(vdx, &ifindex) != VTSS_RC_OK) {
        return FALSE;
    }

    key.ifindex = ifindex;
    while (vtss_appl_ip_if_status_ipv6_itr(&key, &key) == VTSS_RC_OK && key.ifindex == ifindex) {
        if (key.addr.address == *dip) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL client_support_msg_type(const Dhcp6Message *const dhcp6_msg)
{
    switch ( DHCP6_MSG_MSG_TYPE(dhcp6_msg) ) {
    case DHCP6SOLICIT:
    case DHCP6ADVERTISE:
    case DHCP6REQUEST:
    case DHCP6CONFIRM:
    case DHCP6RENEW:
    case DHCP6REBIND:
    case DHCP6REPLY:
    case DHCP6RELEASE:
    case DHCP6DECLINE:
    case DHCP6RECONFIGURE:
    case DHCP6INFORMATION_REQUEST:
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

BOOL client_validate_opt_code(const Dhcp6Message *const dhcp6_msg, u32 len)
{
    Dhcp6Option *p = DHCP6_MSG_OPTIONS_PTR(dhcp6_msg);
    u32         opt_len, chk_len = 0;
    MessageType msg_type = (MessageType)DHCP6_MSG_MSG_TYPE(dhcp6_msg);
    BOOL        opt_s, opt_c, opt_a;

    opt_s = opt_c = opt_a = FALSE;
    opt_len = len - sizeof(dhcp6_msg->msg_xid);

    while (chk_len + DHCP6_MSG_OPT_SHIFT_LENGTH(p) <= opt_len) {
        switch ( DHCP6_MSG_OPT_CODE(p) ) {
        case OPT_CLIENTID:
            opt_c = TRUE;
            break;
        case OPT_SERVERID:
            opt_s = TRUE;
            break;
        case OPT_AUTH:
            opt_a = TRUE;
            break;
        case OPT_IA_NA:
        case OPT_IA_TA:
        case OPT_IAADDR:
        case OPT_ORO:
        case OPT_PREFERENCE:
        case OPT_ELAPSED_TIME:
        case OPT_UNICAST:
            break;
        case OPT_STATUS_CODE:
            /* RFC-3315: 17.1.3 */
            if (msg_type == DHCP6ADVERTISE) {
                OptStatusCode   *s = (OptStatusCode *)p;

                if (ntohs(s->status_code) == STATUS_NO_ADDRS_AVAIL) {
                    return FALSE;
                }
            }
            break;
        case OPT_RAPID_COMMIT:
            /* opt_r = TRUE; */
            break;
        case OPT_RECONF_MSG:
        case OPT_RECONF_ACCEPT:
        case OPT_DNS_SERVERS:
        case OPT_DOMAIN_LIST:
        case OPT_IA_PD:
        case OPT_IAPREFIX:
            break;
        default:
            T_D("Unsupported DHCPv6 Option-Code: %u", DHCP6_MSG_OPT_CODE(p));
            break;
        }

        chk_len += DHCP6_MSG_OPT_SHIFT_LENGTH(p);
        if (chk_len < opt_len) {
            p = DHCP6_MSG_OPT_NEXT(p);
        } else {
            if (chk_len > opt_len) {
                return FALSE;
            }
        }
    }

    /* RFC-3315: 15.3, 15.10, 15.11 */
    if (msg_type == DHCP6ADVERTISE) {
        if (!opt_s || !opt_c) {
            return FALSE;
        }
    }
    if (msg_type == DHCP6REPLY) {
        if (!opt_s) {
            return FALSE;
        }
    }
    if (msg_type == DHCP6RECONFIGURE) {
        if (!opt_s || !opt_a) {
            return FALSE;
        }
    }

    return TRUE;
}

namespace client
{
static BOOL CLIENT_rx_sanity(
    const u8    *const frm,
    u32         len,
    mesa_vid_t  ifx,
    BOOL        *const erf,
    BOOL        *const drp)
{
    Ip6Header       *frm_ip;
    UdpHeader       *frm_udp;
    Dhcp6Message    *frm_msg;

    if (!frm || !erf || !drp) {
        T_D("Invalid Input!");
        return FALSE;
    }

    if (!DHCP6_rx_sanity(frm, len)) {
        T_D("Failed in DHCP6_rx_sanity");
        *drp = *erf = TRUE;
        return FALSE;
    }

    frm_ip = (Ip6Header *)frm;
    if (!DHCP6_ipstk_address_get(ifx, &frm_ip->dst)) {
        T_D("Failed in DHCP6_ipstk_address_get");
        *drp = TRUE;
        return FALSE;
    }

    frm_udp = (UdpHeader *)((u8 *)frm_ip + sizeof(Ip6Header));
    if (IPV6_UDP_HEADER_DST_PORT(frm_udp) != VTSS_DHCP6_CLIENT_UDP_PORT) {
        T_D("Invalid UDP-DST-PORT:%u", IPV6_UDP_HEADER_DST_PORT(frm_udp));
        *drp = TRUE;
        return FALSE;
    }

    frm_msg = (Dhcp6Message *)((u8 *)frm_udp + sizeof(UdpHeader));
    if (!vtss::dhcp6::client_support_msg_type(frm_msg)) {
        T_D("Unsupported DHCPv6 Msg-Type");
        *drp = TRUE;
        return FALSE;
    }

    if (!vtss::dhcp6::client_validate_opt_code(frm_msg, len - sizeof(Ip6Header) - sizeof(UdpHeader))) {
        T_D("Invalid DHCPv6 Option-Code");
        *drp = TRUE;
        return FALSE;
    }

    return TRUE;
}

static mesa_rc CLIENT_process_message(u8 *const frm, vtss_tick_count_t ts, u32 len, mesa_vid_t ifx)
{
    mesa_rc     rc = VTSS_RC_ERROR;
    MessageType msg_type = DHCP6RESERVED;

    if (frm) {
        Dhcp6Message    *msg = (Dhcp6Message *)(frm + sizeof(Ip6Header) + sizeof(UdpHeader));
        u32             msg_len = len - sizeof(Ip6Header) - sizeof(UdpHeader);
        mesa_ipv6_t     *sip = &(((Ip6Header *)frm)->src);

        msg_type = (MessageType)DHCP6_MSG_MSG_TYPE(msg);
        switch ( msg_type ) {
        case DHCP6SOLICIT:
            rc = rx_solicit(ROLE_CLIENT, sip, msg, ts, msg_len, ifx);
            break;
        case DHCP6ADVERTISE:
            rc = rx_advertise(ROLE_CLIENT, sip, msg, ts, msg_len, ifx);
            break;
        case DHCP6REQUEST:
            rc = rx_request(ROLE_CLIENT, sip, msg, ts, msg_len, ifx);
            break;
        case DHCP6CONFIRM:
            rc = rx_confirm(ROLE_CLIENT, sip, msg, ts, msg_len, ifx);
            break;
        case DHCP6RENEW:
            rc = rx_renew(ROLE_CLIENT, sip, msg, ts, msg_len, ifx);
            break;
        case DHCP6REBIND:
            rc = rx_rebind(ROLE_CLIENT, sip, msg, ts, msg_len, ifx);
            break;
        case DHCP6REPLY:
            rc = rx_reply(ROLE_CLIENT, sip, msg, ts, msg_len, ifx);
            break;
        case DHCP6RELEASE:
            rc = rx_release(ROLE_CLIENT, sip, msg, ts, msg_len, ifx);
            break;
        case DHCP6DECLINE:
            rc = rx_decline(ROLE_CLIENT, sip, msg, ts, msg_len, ifx);
            break;
        case DHCP6RECONFIGURE:
            rc = rx_reconfigure(ROLE_CLIENT, sip, msg, ts, msg_len, ifx);
            break;
        case DHCP6INFORMATION_REQUEST:
            rc = rx_information_request(ROLE_CLIENT, sip, msg, ts, msg_len, ifx);
            break;
        default:
            break;
        }
    }

    (void) interface_rx_cntr_inc(ifx, msg_type, frm == NULL, rc != VTSS_RC_OK);
    return rc;
}

mesa_rc transmit(
    mesa_vid_t          ifx,
    const mesa_ipv6_t   *const sip,
    const mesa_ipv6_t   *const dip,
    MessageType         msg_type,
    u32                 xid,
    u32                 opt_len,
    const u8            *const opts
)
{
    u32             len;
    u8              *frm;
    Ip6Header       *p;
    UdpHeader       *q;
    Dhcp6Message    *r;
    MessageType     mtype = DHCP6UNASSIGNED;
    mesa_rc         snd_rc = VTSS_RC_ERROR;

    if (!dip || !opts ||
        VTSS_CALLOC_CAST(frm, 1, DHCP6_PKT_SZ_VAL) == NULL) {
        (void) interface_tx_cntr_inc(ifx, mtype, TRUE, FALSE);
        return snd_rc;
    }

    len = VTSS_IPV6_ETHER_LENGTH;
    p = (Ip6Header *)(frm + len);
    p->ver_tc_fl = VTSS_IPV6_HEADER_VERSION;
    p->ver_tc_fl = htonl(p->ver_tc_fl << 28 | 0xe000000);
    p->next_header = VTSS_IPV6_HEADER_NXTHDR_UDP;
    p->hop_limit = 0xFF;
    if (sip) {
        memcpy(&p->src, sip, sizeof(mesa_ipv6_t));
    } else {
        (void) vtss::dhcp6c::utils::eui64_linklocal_addr_get(&p->src);
    }
    memcpy(&p->dst, dip, sizeof(mesa_ipv6_t));

    len += sizeof(Ip6Header);
    q = (UdpHeader *)(frm + len);
    q->src_port = htons(VTSS_DHCP6_CLIENT_UDP_PORT);
    q->dst_port = htons(VTSS_DHCP6_SERVER_UDP_PORT);
    q->check_sum = 0;

    len += sizeof(UdpHeader);
    r = (Dhcp6Message *)(frm + len);
    if (len + sizeof(r->msg_xid) + opt_len > DHCP6_PKT_SZ_VAL) {
        T_D("Cannot Do Fragment (Len:%u + Opt:%zu > %u)", len, sizeof(r->msg_xid) + opt_len, DHCP6_PKT_SZ_VAL);
        VTSS_FREE(frm);
        (void) interface_tx_cntr_inc(ifx, mtype, FALSE, TRUE);
        return snd_rc;
    }
    q->len = p->payload_len = htons(sizeof(UdpHeader) + sizeof(r->msg_xid) + opt_len);
    r->msg_xid = msg_type;
    r->msg_xid = htonl((r->msg_xid << 24) | (xid & 0xFFFFFF));
    memcpy(&r->options, opts, opt_len);

    q->check_sum = htons(tcpudp6_chksum_calc(&p->src, &p->dst, IPV6_UDP_HEADER_LENGTH(q), (u8 *)q));
    if (q->check_sum != INVALID_CHECKSUM) {
        mtype = msg_type;
        snd_rc = vtss::dhcp6c::frame_snd(ifx, len + sizeof(r->msg_xid) + opt_len, frm);
    }
    VTSS_FREE(frm);

    (void) interface_tx_cntr_inc(ifx, mtype, msg_type == DHCP6UNASSIGNED, snd_rc != VTSS_RC_OK);
    return snd_rc;
}

mesa_rc receive(
    u8                  *const frm,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    BOOL    err_pkt, drp_pkt;

    err_pkt = drp_pkt = FALSE;
    if (!CLIENT_rx_sanity(frm, len, ifx, &err_pkt, &drp_pkt)) {
        MessageType msg_type = DHCP6RESERVED;

        if (frm) {
            Dhcp6Message    *msg = (Dhcp6Message *)(frm + sizeof(Ip6Header) + sizeof(UdpHeader));

            msg_type = (MessageType)DHCP6_MSG_MSG_TYPE(msg);
        }
        (void) interface_rx_cntr_inc(ifx, msg_type, err_pkt, drp_pkt);

        T_D("Failed in Sanity-Check");
        return VTSS_RC_ERROR;
    }

    T_D("TimeStamp:" VPRI64u" / Length:%u / IngressVid:%u", ts, len, ifx);
    T_D_HEX(frm, len);
    return CLIENT_process_message(frm, ts, len, ifx);
}
} /* client */

#define DO_CLIENT_RX(v, w, x, y, z, ...)            do {                \
    Dhcp6Option *p = DHCP6_MSG_OPTIONS_PTR((y));                        \
    u32         chk_len = 0, opt_len = (z) - sizeof((y)->msg_xid);      \
                                                                        \
    while (chk_len + DHCP6_MSG_OPT_SHIFT_LENGTH(p) <= opt_len) {        \
        (w) = vtss::dhcp6::client::__VA_ARGS__(FALSE, (v), (x),         \
                DHCP6_MSG_TRANSACTION_ID((y)),                          \
                DHCP6_MSG_OPT_CODE(p),                                  \
                DHCP6_MSG_OPT_LENGTH(p),                                \
                DHCP6_MSG_OPT_DATA_PTR_CAST(p, u8));                    \
        if ((w) != VTSS_RC_OK) {                                        \
            break;                                                      \
        }                                                               \
                                                                        \
        chk_len += DHCP6_MSG_OPT_SHIFT_LENGTH(p);                       \
        if (chk_len < opt_len) {                                        \
            p = DHCP6_MSG_OPT_NEXT(p);                                  \
        } else {                                                        \
            if (chk_len > opt_len) {                                    \
                (w) = VTSS_RC_ERROR;                                    \
                break;                                                  \
            }                                                           \
        }                                                               \
    }                                                                   \
    if ((w) == VTSS_RC_OK) {                                            \
        (w) = vtss::dhcp6::client::__VA_ARGS__(TRUE, (v), (x),          \
                DHCP6_MSG_TRANSACTION_ID((y)), 0, 0, 0);                \
    }                                                                   \
} while (0)

#define CLIENT_RX_SOLICIT(v, w, x, y, z)            DO_CLIENT_RX(v, w, x, y, z, do_rx_solicit)
#define SERVER_RX_SOLICIT(v, w, x, y, z)
#define RELAY_RX_SOLICIT(v, w, x, y, z)
#define CLIENT_RX_ADVERTISE(v, w, x, y, z)          DO_CLIENT_RX(v, w, x, y, z, do_rx_advertise)
#define SERVER_RX_ADVERTISE(v, w, x, y, z)
#define RELAY_RX_ADVERTISE(v, w, x, y, z)
#define CLIENT_RX_REQUEST(v, w, x, y, z)            DO_CLIENT_RX(v, w, x, y, z, do_rx_request)
#define SERVER_RX_REQUEST(v, w, x, y, z)
#define RELAY_RX_REQUEST(v, w, x, y, z)
#define CLIENT_RX_CONFIRM(v, w, x, y, z)            DO_CLIENT_RX(v, w, x, y, z, do_rx_confirm)
#define SERVER_RX_CONFIRM(v, w, x, y, z)
#define RELAY_RX_CONFIRM(v, w, x, y, z)
#define CLIENT_RX_RENEW(v, w, x, y, z)              DO_CLIENT_RX(v, w, x, y, z, do_rx_renew)
#define SERVER_RX_RENEW(v, w, x, y, z)
#define RELAY_RX_RENEW(v, w, x, y, z)
#define CLIENT_RX_REBIND(v, w, x, y, z)             DO_CLIENT_RX(v, w, x, y, z, do_rx_rebind)
#define SERVER_RX_REBIND(v, w, x, y, z)
#define RELAY_RX_REBIND(v, w, x, y, z)
#define CLIENT_RX_REPLY(v, w, x, y, z)              DO_CLIENT_RX(v, w, x, y, z, do_rx_reply)
#define SERVER_RX_REPLY(v, w, x, y, z)
#define RELAY_RX_REPLY(v, w, x, y, z)
#define CLIENT_RX_RELEASE(v, w, x, y, z)            DO_CLIENT_RX(v, w, x, y, z, do_rx_release)
#define SERVER_RX_RELEASE(v, w, x, y, z)
#define RELAY_RX_RELEASE(v, w, x, y, z)
#define CLIENT_RX_DECLINE(v, w, x, y, z)            DO_CLIENT_RX(v, w, x, y, z, do_rx_decline)
#define SERVER_RX_DECLINE(v, w, x, y, z)
#define RELAY_RX_DECLINE(v, w, x, y, z)
#define CLIENT_RX_RECONFIGURE(v, w, x, y, z)        DO_CLIENT_RX(v, w, x, y, z, do_rx_reconfigure)
#define SERVER_RX_RECONFIGURE(v, w, x, y, z)
#define RELAY_RX_RECONFIGURE(v, w, x, y, z)
#define CLIENT_RX_INFORMATION_REQ(v, w, x, y, z)    DO_CLIENT_RX(v, w, x, y, z, do_rx_information_request)
#define SERVER_RX_INFORMATION_REQ(v, w, x, y, z)
#define RELAY_RX_INFORMATION_REQ(v, w, x, y, z)

#define RXOP(u, v, w, x, y, z, ...)                 do {                \
    if ((u) == ROLE_CLIENT) CLIENT_##__VA_ARGS__(v, w, x, y, z);        \
    else if ((u) == ROLE_SERVER) SERVER_##__VA_ARGS__(v, w, x, y, z);   \
    else if ((u) == ROLE_RAGENT) RELAY_##__VA_ARGS__(v, w, x, y, z);    \
    else (w) = VTSS_RC_ERROR;                                           \
} while (0)

mesa_rc rx_solicit(
    ServiceRole         r,
    const mesa_ipv6_t   *const sip,
    const Dhcp6Message  *const msg,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("enter");
    RXOP(r, sip, rc, ifx, msg, len, RX_SOLICIT);
    T_D("exit");

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc rx_advertise(
    ServiceRole         r,
    const mesa_ipv6_t   *const sip,
    const Dhcp6Message  *const msg,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("enter");
    RXOP(r, sip, rc, ifx, msg, len, RX_ADVERTISE);
    T_D("exit");

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc rx_request(
    ServiceRole         r,
    const mesa_ipv6_t   *const sip,
    const Dhcp6Message  *const msg,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("enter");
    RXOP(r, sip, rc, ifx, msg, len, RX_REQUEST);
    T_D("exit");

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc rx_confirm(
    ServiceRole         r,
    const mesa_ipv6_t   *const sip,
    const Dhcp6Message  *const msg,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("enter");
    RXOP(r, sip, rc, ifx, msg, len, RX_CONFIRM);
    T_D("exit");

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc rx_renew(
    ServiceRole         r,
    const mesa_ipv6_t   *const sip,
    const Dhcp6Message  *const msg,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("enter");
    RXOP(r, sip, rc, ifx, msg, len, RX_RENEW);
    T_D("exit");

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc rx_rebind(
    ServiceRole         r,
    const mesa_ipv6_t   *const sip,
    const Dhcp6Message  *const msg,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("enter");
    RXOP(r, sip, rc, ifx, msg, len, RX_REBIND);
    T_D("exit");

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc rx_reply(
    ServiceRole         r,
    const mesa_ipv6_t   *const sip,
    const Dhcp6Message  *const msg,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("enter, serviceRole is %d", r);
    RXOP(r, sip, rc, ifx, msg, len, RX_REPLY);
    T_D("exit");

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc rx_release(
    ServiceRole         r,
    const mesa_ipv6_t   *const sip,
    const Dhcp6Message  *const msg,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("enter");
    RXOP(r, sip, rc, ifx, msg, len, RX_RELEASE);
    T_D("exit");

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc rx_decline(
    ServiceRole         r,
    const mesa_ipv6_t   *const sip,
    const Dhcp6Message  *const msg,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("enter");
    RXOP(r, sip, rc, ifx, msg, len, RX_DECLINE);
    T_D("exit");

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc rx_reconfigure(
    ServiceRole         r,
    const mesa_ipv6_t   *const sip,
    const Dhcp6Message  *const msg,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("enter");
    RXOP(r, sip, rc, ifx, msg, len, RX_RECONFIGURE);
    T_D("exit");

    DHCP6C_PRIV_TD_RETURN(rc);
}

mesa_rc rx_information_request(
    ServiceRole         r,
    const mesa_ipv6_t   *const sip,
    const Dhcp6Message  *const msg,
    vtss_tick_count_t   ts,
    u32                 len,
    mesa_vid_t          ifx
)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("enter");
    RXOP(r, sip, rc, ifx, msg, len, RX_INFORMATION_REQ);
    T_D("exit");

    DHCP6C_PRIV_TD_RETURN(rc);
}

} /* dhcp6 */
} /* vtss */
