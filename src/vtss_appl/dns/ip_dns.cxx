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

#include "main.h"
#include "conf_api.h"
#include "msg_api.h"
#include "critd_api.h"
#include "packet_api.h"
#include "misc_api.h"
#include "vtss_bip_buffer_api.h"
#include "vtss_timer_api.h"
#include "vtss/basics/string.hxx"

#if defined(VTSS_SW_OPTION_IP)
#include "ip_api.h"
#include "ip_utils.hxx"
#endif /* defined(VTSS_SW_OPTION_IP) */
#if defined(VTSS_SW_OPTION_DHCP_CLIENT)
#include "dhcp_client_api.h"
#endif /* defined(VTSS_SW_OPTION_DHCP_CLIENT) */
#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)
#include "dhcp6_client_api.h"
#endif /* defined(VTSS_SW_OPTION_DHCP6_CLIENT) */
#if defined(VTSS_SW_OPTION_ACCESS_MGMT)
#include "access_mgmt_api.h"
#endif /* defined(VTSS_SW_OPTION_ACCESS_MGMT) */

#include "netdb.h"
#include <arpa/inet.h>

#include "ip_dns.h"
#include "ip_dns_api.h"
#include "vtss_dns_oswrapper.h"

#ifdef VTSS_SW_OPTION_ICFG
#include "ip_dns_icfg.h"
#endif /* VTSS_SW_OPTION_ICFG */

#define VTSS_TRACE_MODULE_ID    VTSS_MODULE_ID_IP_DNS
#define VTSS_ALLOC_MODULE_ID    VTSS_MODULE_ID_IP_DNS

/* Global structure */
static dns_global_record_t      vtss_dns_database;
static BOOL                     vtss_dns_thread_ready = FALSE;

#define VTSS_DNS_READY          (_vtss_dns_thread_status_get() == TRUE)
#define VTSS_DNS_BREAK_WAIT(x)  do {    \
    ++(x);                              \
    VTSS_OS_MSLEEP(1973);               \
    if ((x) > 5) break;                 \
} while (0)

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "IP_DNS", "IP DNS Module"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define DNS_CRIT_ENTER() critd_enter(&vtss_dns_database.dns_crit, __FILE__, __LINE__)
#define DNS_CRIT_EXIT()  critd_exit( &vtss_dns_database.dns_crit, __FILE__, __LINE__)
#define DNS_PKT_ENTER()  critd_enter(&vtss_dns_database.dns_pkt,  __FILE__, __LINE__)
#define DNS_PKT_EXIT()   critd_exit( &vtss_dns_database.dns_pkt,  __FILE__, __LINE__)

/* Thread variables */
/* DNS Client updating Thread */
static vtss_handle_t            vtss_dns_thread_handle;
static vtss_thread_t            vtss_dns_thread_block;
static vtss_flag_t              vtss_dns_thread_flag;
static vtss::Timer              vtss_dns_thread_timer;
static vtss_bip_buffer_t        vtss_dns_bip;

const mesa_ipv6_t               dns_unspecified_address = DNS_UNSPECIFIED_ADDRESS_INIT;

static BOOL _vtss_dns_thread_status_get(void)
{
    BOOL    status;

    DNS_CRIT_ENTER();
    status = vtss_dns_thread_ready;
    DNS_CRIT_EXIT();

    return status;
}

static void _vtss_dns_thread_status_set(BOOL status)
{
    DNS_CRIT_ENTER();
    vtss_dns_thread_ready = status;
    DNS_CRIT_EXIT();
}

void dns_pkt_tx(u32 out_port, mesa_vid_t vid, u8 *frame, size_t len)
{
    u8  *pkt_buf;

    if ((pkt_buf = packet_tx_alloc(len))) {
        packet_tx_props_t   tx_props;

        memcpy(pkt_buf, frame, len);
        packet_tx_props_init(&tx_props);

        tx_props.packet_info.modid  = VTSS_MODULE_ID_IP_DNS;
        tx_props.packet_info.frm    = pkt_buf;
        tx_props.packet_info.len    = len;
        tx_props.tx_info.tag.vid    = vid;
        tx_props.tx_info.switch_frm = TRUE;
//        tx_props.tx_info.dst_port_mask = VTSS_BIT64(out_port);

        if (packet_tx(&tx_props) != VTSS_RC_OK) {
            T_D("Failed in transmitting DNS to VID-%u/Port-%u", vid, out_port);
        }
    }
}

static void transmit_dns_response(dns_proxy_query_item_t *dns_proxy_query_msg, ulong qip)
{
#define PKT_BUFSIZE     1514
#define PKT_LLH_LEN     14
#define PKT_IP_LEN      20
#define PKT_UDP_LEN     sizeof(dns_udp_hdr)
#define PKT_DNS_LEN     sizeof(dns_dns_hdr)
#define PKT_ETHTYPE_IP  0x0800
#define PKT_PROTO_UDP   0x11
    static uchar            t = 0;
    uchar                   pkt_buf[PKT_BUFSIZE + 2];
    /* point to pkt buffer */
    dns_eth_hdr             *eth_hdr;
    dns_ip_hdr              *ip_hdr;
    dns_udp_hdr             *udp_hdr;
    dns_dns_hdr             *dns_hdr;
    dns_dns_answer          *dns_answer_record;

    size_t                  pkt_len;
    uchar                   mac_addr[6];
    int                     len, dns_question_len;
    uchar                   *ptr;
    int                     dns_type_a = DNS_TYPE_A;
    int                     dns_class_in = DNS_CLASS_IN;
    mesa_ip_addr_t          sip, dip;
    uint16_t                udp_len;

    eth_hdr = (dns_eth_hdr *)pkt_buf;
    ip_hdr = (dns_ip_hdr *)&pkt_buf[PKT_LLH_LEN];

    /* set IP Hdr */
    memcpy(eth_hdr->dest.addr, dns_proxy_query_msg->querier_mac, sizeof(uchar) * 6);
    (void) conf_mgmt_mac_addr_get(mac_addr, 0);
    memcpy(eth_hdr->src.addr, mac_addr, sizeof(uchar) * 6);
    eth_hdr->type = htons(PKT_ETHTYPE_IP);

    sip.type      = MESA_IP_TYPE_IPV4;
    sip.addr.ipv4 = dns_proxy_query_msg->querier_dns_dip;
    dip.type      = MESA_IP_TYPE_IPV4;
    dip.addr.ipv4 = dns_proxy_query_msg->querier_ip;

    ip_hdr->vhl = 0x45;
    ip_hdr->tos = 0x00;
    ip_hdr->ipid[0] = 0x00;
    ip_hdr->ipid[1] = ++t;
    ip_hdr->ipoffset[0] = 0x00;
    ip_hdr->ipoffset[1] = 0x00;
    ip_hdr->ttl = 128;
    ip_hdr->proto = PKT_PROTO_UDP;

    /* src address */
    ip_hdr->srcipaddr = htonl(sip.addr.ipv4);

    /* dst address */
    ip_hdr->destipaddr = htonl(dip.addr.ipv4);

    /* UDP Header */
    udp_hdr = (dns_udp_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN];
    udp_hdr->sport = htons(53);
    udp_hdr->dport = htons(dns_proxy_query_msg->querier_udp_port);

    /* DNS Header */
    dns_hdr = (dns_dns_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN];
    dns_hdr->id = (dns_proxy_query_msg->querier_dns_transaction_id);
    dns_hdr->flags = htons(0x8180);
    dns_hdr->qdcount = htons(1);    /* number of question entries */
    dns_hdr->ancount = htons(1);    /* number of answer entries */
    dns_hdr->nscount = 0;           /* number of authority entries */
    dns_hdr->arcount = 0;           /* number of resource entries */

    /* DNS Question */
    ptr = &pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN];
    if ((len = vtss_dns_frame_build_question_name(ptr, dns_proxy_query_msg->querier_question)) < 0) {
        T_W("Failed to build QNname");
        return;
    }

    ptr += len;
    /* Set the type and class fields */
    *ptr++ = (dns_type_a >> 8) & 0xff;
    *ptr++ = dns_type_a & 0xff;
    *ptr++ = (dns_class_in >> 8) & 0xff;
    *ptr++ = dns_class_in & 0xff;
    dns_question_len = len + 4;

    /* DNS Answer */
    dns_answer_record = (dns_dns_answer *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len];
    dns_answer_record->domain = htons(0xc00c);
    dns_answer_record->rr_type = htons(DNS_TYPE_A);
    dns_answer_record->cls = htons(DNS_CLASS_IN);
    dns_answer_record->ttl = htonl(60);
    dns_answer_record->rdlength = htons(4);
    dns_answer_record->rdata[3] = (qip >> 24) & 0xff;
    dns_answer_record->rdata[2] = (qip >> 16) & 0xff;
    dns_answer_record->rdata[1] = (qip >> 8) & 0xff;
    dns_answer_record->rdata[0] = (qip >> 0) & 0xff;

    ip_hdr->len = htons(PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + sizeof(dns_dns_answer));
    ip_hdr->ipchksum = 0;
    ip_hdr->ipchksum = htons(vtss_ip_checksum(&pkt_buf[PKT_LLH_LEN], PKT_IP_LEN));

    /* UDP checksum related  */
    udp_len = PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + sizeof(dns_dns_answer);
    udp_hdr->ulen = htons(udp_len);
    udp_hdr->csum = 0;
    udp_hdr->csum = htons(vtss_ip_pseudo_header_checksum(&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN], udp_len, sip, dip, IP_PROTO_UDP));

    /* length = eth_len [14] + ip_hdr [20] + udp_hdr [8] = total [42] */
    pkt_len = PKT_LLH_LEN + PKT_IP_LEN + udp_len;

    T_D_HEX(pkt_buf, pkt_len);

    dns_pkt_tx(dns_proxy_query_msg->querier_incoming_port, dns_proxy_query_msg->querier_vid, pkt_buf, pkt_len);
}

static void transmit_dns_response_rev_lookup(dns_proxy_query_item_t *dns_proxy_query_msg, char *q_host)
{
#define PKT_BUFSIZE     1514
#define PKT_LLH_LEN     14
#define PKT_IP_LEN      20
#define PKT_UDP_LEN     sizeof(dns_udp_hdr)
#define PKT_DNS_LEN     sizeof(dns_dns_hdr)
#define PKT_ETHTYPE_IP  0x0800
#define PKT_PROTO_UDP   0x11
    static uchar            t = 0;
    uchar                   pkt_buf[PKT_BUFSIZE + 2];
    /* point to pkt buffer */
    dns_eth_hdr             *eth_hdr;
    dns_ip_hdr              *ip_hdr;
    dns_udp_hdr             *udp_hdr;
    dns_dns_hdr             *dns_hdr;

    size_t                  pkt_len;
    uchar                   mac_addr[6];
    int                     len, dns_question_len, dns_answer_len;
    uchar                   *ptr;
    int                     dns_type_a = DNS_TYPE_A;
    int                     dns_class_in = DNS_CLASS_IN;
    uchar                   encoded_name[64];
    mesa_ip_addr_t          sip, dip;
    uint16_t                udp_len;

    eth_hdr = (dns_eth_hdr *)pkt_buf;
    ip_hdr = (dns_ip_hdr *)&pkt_buf[PKT_LLH_LEN];

    /* set IP Hdr */
    memcpy(eth_hdr->dest.addr, dns_proxy_query_msg->querier_mac, sizeof(uchar) * 6);
    (void) conf_mgmt_mac_addr_get(mac_addr, 0);
    memcpy(eth_hdr->src.addr, mac_addr, sizeof(uchar) * 6);
    eth_hdr->type = htons(PKT_ETHTYPE_IP);

    sip.type      = MESA_IP_TYPE_IPV4;
    sip.addr.ipv4 = dns_proxy_query_msg->querier_dns_dip;
    dip.type      = MESA_IP_TYPE_IPV4;
    dip.addr.ipv4 = dns_proxy_query_msg->querier_ip;

    ip_hdr->vhl = 0x45;
    ip_hdr->tos = 0x00;
    ip_hdr->ipid[0] = 0x00;
    ip_hdr->ipid[1] = ++t;
    ip_hdr->ipoffset[0] = 0x00;
    ip_hdr->ipoffset[1] = 0x00;
    ip_hdr->ttl = 128;
    ip_hdr->proto = PKT_PROTO_UDP;

    /* src address */
    ip_hdr->srcipaddr = htonl(sip.addr.ipv4);

    /* dst address */
    ip_hdr->destipaddr = htonl(dip.addr.ipv4);

    /* UDP Header */
    udp_hdr = (dns_udp_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN];
    udp_hdr->sport = htons(53);
    udp_hdr->dport = htons(dns_proxy_query_msg->querier_udp_port);

    /* DNS Header */
    dns_hdr = (dns_dns_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN];
    dns_hdr->id = (dns_proxy_query_msg->querier_dns_transaction_id);
    dns_hdr->flags = htons(0x8080);
    dns_hdr->qdcount = htons(1);    /* number of question entries */
    dns_hdr->ancount = htons(1);    /* number of answer entries */
    dns_hdr->nscount = 0;           /* number of authority entries */
    dns_hdr->arcount = 0;           /* number of resource entries */

    /* DNS Question */
    ptr = &pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN];
    if ((len = vtss_dns_frame_build_question_name(ptr, dns_proxy_query_msg->querier_question)) < 0) {
        T_W("Failed to build QNname");
        return;
    }
    ptr += len;
    /* Set the type and class fields */
    *ptr++ = (dns_type_a >> 8) & 0xff;
    *ptr++ = dns_type_a & 0xff;
    *ptr++ = (dns_class_in >> 8) & 0xff;
    *ptr++ = dns_class_in & 0xff;
    dns_question_len = len + 4;

    /* DNS Answer */
    *ptr++ = 0xc0;
    *ptr++ = 0x0c;
    *ptr++ = 0x00;
    *ptr++ = 0x0c;
    *ptr++ = 0x00;
    *ptr++ = 0x01;
    *ptr++ = 0;
    *ptr++ = 0;
    *ptr++ = 0;
    *ptr++ = 60;
    if ((len = vtss_dns_frame_build_question_name(encoded_name, q_host)) < 0) {
        T_W("Failed to build QNname");
        return;
    }
    *ptr++ = (len >> 8) & 0xff;
    *ptr++ = len & 0xff;
    memcpy(ptr, encoded_name, len);
    dns_answer_len = len + 12;

    ip_hdr->len = htons(PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_answer_len);
    ip_hdr->ipchksum = 0;
    ip_hdr->ipchksum = htons(vtss_ip_checksum(&pkt_buf[PKT_LLH_LEN], PKT_IP_LEN));

    /* UDP checksum related  */
    udp_len = PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_answer_len;
    udp_hdr->ulen = htons(udp_len);
    udp_hdr->csum = 0;
    udp_hdr->csum = htons(vtss_ip_pseudo_header_checksum(&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN], udp_len, sip, dip, IP_PROTO_UDP));

    /* length = eth_len [14] + ip_hdr [20] + igmp_hdr [8] = total [42] */
    pkt_len = PKT_LLH_LEN + PKT_IP_LEN + udp_len;

    T_D_HEX(pkt_buf, pkt_len);

    dns_pkt_tx(dns_proxy_query_msg->querier_incoming_port, dns_proxy_query_msg->querier_vid, pkt_buf, pkt_len);
}

static void transmit_dns_no_such_name_response(dns_proxy_query_item_t *dns_proxy_query_msg)
{
#define PKT_BUFSIZE     1514
#define PKT_LLH_LEN     14
#define PKT_IP_LEN      20
#define PKT_UDP_LEN     sizeof(dns_udp_hdr)
#define PKT_DNS_LEN     sizeof(dns_dns_hdr)
#define PKT_ETHTYPE_IP  0x0800
#define PKT_PROTO_UDP   0x11
    static uchar            t = 0;
    uchar                   pkt_buf[PKT_BUFSIZE + 2];
    /* point to pkt buffer */
    dns_eth_hdr             *eth_hdr;
    dns_ip_hdr              *ip_hdr;
    dns_udp_hdr             *udp_hdr;
    dns_dns_hdr             *dns_hdr;

    size_t                  pkt_len;
    uchar                   mac_addr[6];
    int                     len, dns_question_len, dns_authority_len;
    uchar                   *ptr;
    int                     dns_type_a = DNS_TYPE_A;
    int                     dns_class_in = DNS_CLASS_IN;
    mesa_ip_addr_t          sip, dip;
    uint16_t                udp_len;

    eth_hdr = (dns_eth_hdr *)pkt_buf;
    ip_hdr = (dns_ip_hdr *)&pkt_buf[PKT_LLH_LEN];

    /* set IP Hdr */
    memcpy(eth_hdr->dest.addr, dns_proxy_query_msg->querier_mac, sizeof(uchar) * 6);
    (void) conf_mgmt_mac_addr_get(mac_addr, 0);
    memcpy(eth_hdr->src.addr, mac_addr, sizeof(uchar) * 6);
    eth_hdr->type = htons(PKT_ETHTYPE_IP);

    sip.type      = MESA_IP_TYPE_IPV4;
    sip.addr.ipv4 = dns_proxy_query_msg->querier_dns_dip;
    dip.type      = MESA_IP_TYPE_IPV4;
    dip.addr.ipv4 = dns_proxy_query_msg->querier_ip;

    ip_hdr->vhl = 0x45;
    ip_hdr->tos = 0x00;
    ip_hdr->ipid[0] = 0x00;
    ip_hdr->ipid[1] = ++t;
    ip_hdr->ipoffset[0] = 0x00;
    ip_hdr->ipoffset[1] = 0x00;
    ip_hdr->ttl = 128;
    ip_hdr->proto = PKT_PROTO_UDP;

    /* src address */
    ip_hdr->srcipaddr = htonl(sip.addr.ipv4);

    /* dst address */
    ip_hdr->destipaddr = htonl(dip.addr.ipv4);

    /* UDP Header */
    udp_hdr = (dns_udp_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN];
    udp_hdr->sport = htons(53);
    udp_hdr->dport = htons(dns_proxy_query_msg->querier_udp_port);

    /* DNS Header */
    dns_hdr = (dns_dns_hdr *)&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN];
    dns_hdr->id = (dns_proxy_query_msg->querier_dns_transaction_id);
    dns_hdr->flags = htons(0x8183);
    dns_hdr->qdcount = htons(1);    /* number of question entries */
    dns_hdr->ancount = 0;           /* number of answer entries */
    if (dns_proxy_query_msg->querier_question_type == DNS_TYPE_PTR) {
        dns_hdr->nscount = htons(1);    /* number of authority entries */
    } else {
        dns_hdr->nscount = 0;    /* number of authority entries */
    }
    dns_hdr->arcount = 0;           /* number of resource entries */

    /* DNS Question */
    ptr = &pkt_buf[PKT_LLH_LEN + PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN];
    if ((len = vtss_dns_frame_build_question_name(ptr, dns_proxy_query_msg->querier_question)) < 0) {
        T_W("Failed to build QNname");
        return;
    }
    ptr += len;
    /* Set the type and class fields */
    if (dns_proxy_query_msg->querier_question_type == DNS_TYPE_PTR) {
        dns_type_a = DNS_TYPE_PTR;
        *ptr++ = (dns_type_a >> 8) & 0xff;
        *ptr++ = dns_type_a & 0xff;
    } else {
        *ptr++ = (dns_type_a >> 8) & 0xff;
        *ptr++ = dns_type_a & 0xff;
    }

    *ptr++ = (dns_class_in >> 8) & 0xff;
    *ptr++ = dns_class_in & 0xff;
    dns_question_len = len + 4;

    /* authority */
    dns_authority_len = 0;
    ip_hdr->len = htons(PKT_IP_LEN + PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_authority_len);
    ip_hdr->ipchksum = 0;
    ip_hdr->ipchksum = htons(vtss_ip_checksum(&pkt_buf[PKT_LLH_LEN], PKT_IP_LEN));

    /* UDP checksum related  */
    udp_len = PKT_UDP_LEN + PKT_DNS_LEN + dns_question_len + dns_authority_len;
    udp_hdr->ulen = htons(udp_len);
    udp_hdr->csum = 0;
    udp_hdr->csum = htons(vtss_ip_pseudo_header_checksum(&pkt_buf[PKT_LLH_LEN + PKT_IP_LEN], udp_len, sip, dip, IP_PROTO_UDP));

    /* length = eth_len [14] + ip_hdr [20] + udp_hdr [8] = total [42] */
    pkt_len = PKT_LLH_LEN + PKT_IP_LEN + udp_len;

    T_D_HEX(pkt_buf, pkt_len);

    dns_pkt_tx(dns_proxy_query_msg->querier_incoming_port, dns_proxy_query_msg->querier_vid, pkt_buf, pkt_len);
}

void vtss_dns_signal(void)
{
    BOOL    dns_ready = VTSS_DNS_READY;

    T_D("vtss_dns_signal(%sReady)", dns_ready ? "" : "Not");
    if (dns_ready) {
        vtss_flag_setbits(&vtss_dns_thread_flag, DNS_EVENT_PROBE);
    }
}

static BOOL DNS_addr6_is_zero(const mesa_ipv6_t *const addr)
{
    return memcmp(addr, &dns_unspecified_address, sizeof(mesa_ipv6_t)) == 0;
}

static BOOL DNS_proxy_query_queue(dns_proxy_query_item_t *query_item)
{
    u8      *rcv_buf;
    u32     aligned_frm_len_bytes;
    BOOL    go_for_it;

    if (!query_item) {
        return FALSE;
    }

    DNS_CRIT_ENTER();
    go_for_it = vtss_dns_thread_ready & vtss_dns_database.conf.dns_proxy_status;
    DNS_CRIT_EXIT();

    if (!go_for_it) {
        return TRUE;
    }

    T_D_HEX((u8 *)query_item, sizeof(dns_proxy_query_item_t));

    aligned_frm_len_bytes = sizeof(dns_proxy_query_item_t);
    aligned_frm_len_bytes = sizeof(int) * ((aligned_frm_len_bytes + 3) / sizeof(int));

    DNS_PKT_ENTER();
    rcv_buf = vtss_bip_buffer_reserve(&vtss_dns_bip, aligned_frm_len_bytes);
    if (!rcv_buf) {
        T_I("Failure in reserving BIP(BUF_SZ:%d/CMT_SZ:%d)",
            vtss_bip_buffer_get_buffer_size(&vtss_dns_bip),
            vtss_bip_buffer_get_committed_size(&vtss_dns_bip));
        DNS_PKT_EXIT();
        return FALSE;
    }
    if ((u64)rcv_buf & 0x3) {
        T_D("BIP buffer not correctly aligned");
    }

    memcpy(&rcv_buf[0], query_item, sizeof(dns_proxy_query_item_t));

    vtss_bip_buffer_commit(&vtss_dns_bip);
    DNS_PKT_EXIT();

    vtss_flag_setbits(&vtss_dns_thread_flag, DNS_EVENT_QRTV);

    return TRUE;
}

const char *DNS_frm_tagtype_txt(mesa_tag_type_t type, dns_text_cap_t cap)
{
    const char    *txt;

    switch ( type ) {
    case MESA_TAG_TYPE_UNTAGGED:
        if (cap == DNS_TXT_CASE_LOWER) {
            txt = "untagged";
        } else if (cap == DNS_TXT_CASE_UPPER) {
            txt = "UNTAGGED";
        } else {
            txt = "Untagged";
        }
        break;
    case MESA_TAG_TYPE_C_TAGGED:
        if (cap == DNS_TXT_CASE_LOWER) {
            txt = "c-tagged";
        } else if (cap == DNS_TXT_CASE_UPPER) {
            txt = "C-TAGGED";
        } else {
            txt = "C-Tagged";
        }
        break;
    case MESA_TAG_TYPE_S_TAGGED:
        if (cap == DNS_TXT_CASE_LOWER) {
            txt = "s-tagged";
        } else if (cap == DNS_TXT_CASE_UPPER) {
            txt = "S-TAGGED";
        } else {
            txt = "S-Tagged";
        }
        break;
    case MESA_TAG_TYPE_S_CUSTOM_TAGGED:
        if (cap == DNS_TXT_CASE_LOWER) {
            txt = "custom-s-tagged";
        } else if (cap == DNS_TXT_CASE_UPPER) {
            txt = "CUSTOM-S-TAGGED";
        } else {
            txt = "Custom-S-Tagged";
        }
        break;
    default:
        if (cap == DNS_TXT_CASE_LOWER) {
            txt = "unknown";
        } else if (cap == DNS_TXT_CASE_UPPER) {
            txt = "UNKNOWN";
        } else {
            txt = "Unknown";
        }
        break;
    }

    return txt;
}

/******************************************************************************/
// RX_dns()
// Receive a DNS Protocol frame.
/******************************************************************************/
static BOOL DNS_rx_callback(void *contxt, const u8 *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    dns_eth_hdr                   *etherHdr;
    dns_ip_hdr                    *ipHdr;
    dns_udp_hdr                   *udpHdr;
    dns_dns_hdr                   *dnsHdr;
    u8                            frm_offset;
    BOOL                          dns2me;
    char                          adrBuf[40];
    mesa_ip_addr_t                iteradr = {};
    vtss_appl_ip_if_status_t      ipv4_status;
    vtss_appl_ip_if_status_link_t link_status;
    mesa_rc                       chk_rc;
    dns_proxy_query_item_t        dns_proxy_query_msg;

    T_D("%s Frame with length %d received on port %d with vid %d",
        DNS_frm_tagtype_txt(rx_info->tag_type, DNS_TXT_CASE_CAPITAL),
        rx_info->length, rx_info->port_no, rx_info->tag.vid);
    T_D_HEX(frm, rx_info->length);

    frm_offset = 0;
    etherHdr = (dns_eth_hdr *) (frm + frm_offset);
    if (rx_info->tag_type != MESA_TAG_TYPE_UNTAGGED) {
        if (ntohs(etherHdr->type) != 0x0800) {
            if ((frm[16] == 0x08) && (frm[17] == 0x00)) {
                frm_offset += 16;
            } else {
                if ((frm[20] == 0x08) && (frm[21] == 0x00)) {
                    frm_offset += 20;
                } else {
                    return FALSE;
                }
            }
        } else {
            frm_offset += 12;
        }
    } else {
        if (ntohs(etherHdr->type) != 0x0800) {
            return FALSE;
        }
        frm_offset += 12;
    }
    frm_offset += sizeof(etherHdr->type);

    ipHdr = (dns_ip_hdr *) (frm + frm_offset);
    if ((ipHdr->vhl >> 4) != 0x4) {     /* 4 for IPv4 */
        T_D("Incorrect IP version %u", ipHdr->vhl >> 4);
        return FALSE;
    }
    if (ntohs(ipHdr->len) > (rx_info->length - frm_offset)) {
        T_D("IP header length %u GT packet length %u", ntohs(ipHdr->len), rx_info->length - frm_offset);
        return FALSE;
    }
    if (ipHdr->proto != 0x11) {         /* 17 for UDP */
        T_D("Unsupported IP protocol %u", ipHdr->proto);
        return FALSE;
    }
    frm_offset += ((ipHdr->vhl & 0xF) * sizeof(u32));

    udpHdr = (dns_udp_hdr *) (frm + frm_offset);
    if (ntohs(udpHdr->dport) != 0x35) { /* 53 for DNS */
        T_D("Incorrect UDP destination port %u", ntohs(udpHdr->dport));
        return FALSE;
    }
    frm_offset += sizeof(dns_udp_hdr);

    dnsHdr = (dns_dns_hdr *) (frm + frm_offset);
    if (ntohs(dnsHdr->flags) >> 15) {   /* First bit for Query */
        T_D("Unsupported DNS flag %u", ntohs(dnsHdr->flags));
        return FALSE;
    }

    dns2me = FALSE;

    iteradr.type = MESA_IP_TYPE_IPV4;
    iteradr.addr.ipv4 = ntohl(ipHdr->destipaddr);

    if ((chk_rc = vtss_appl_ip_if_status_find(&iteradr, &ipv4_status))                == VTSS_RC_OK &&
        (chk_rc = vtss_appl_ip_if_status_link_get(ipv4_status.ifindex, &link_status)) == VTSS_RC_OK) {
        char  adrBuf1[40];

        // We are up if we could look up the IPv4 address (which we could, since
        // vtss_appl_ip_if_status_find() succeeded) and if the link is up.
        bool up = (link_status.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP) != 0;

        memset(adrBuf,  0x0, sizeof(adrBuf));
        memset(adrBuf1, 0x0, sizeof(adrBuf1));

        T_D("INTF-ADR-%s is %s (DST=%s)", misc_ipv4_txt(ipv4_status.u.ipv4.net.address, adrBuf), up ? "Up" : "Down", misc_ipv4_txt(ntohl(ipHdr->destipaddr), adrBuf1));

        if (up) {
            dns2me = TRUE;
        }
    }

    if (!dns2me) {
        memset(adrBuf, 0x0, sizeof(adrBuf));
        T_D("Frame destined to %s is not for me!", misc_ipv4_txt(ntohl(ipHdr->destipaddr), adrBuf));
        return FALSE;
    }

    memset(&dns_proxy_query_msg, 0x0, sizeof(dns_proxy_query_item_t));
    if (vtss_dns_frame_parse_question_section((u8 *)dnsHdr, (u8 *)dns_proxy_query_msg.querier_question, &dns_proxy_query_msg.querier_question_type, &dns_proxy_query_msg.querier_question_class) == 0) {
        T_D("Cannot parse DNS question section");
        return FALSE;
    }
    dns_proxy_query_msg.querier_incoming_port = rx_info->port_no;
    dns_proxy_query_msg.querier_vid = rx_info->tag.vid;
    memcpy(dns_proxy_query_msg.querier_mac, etherHdr->src.addr, sizeof(dns_eth_addr));
    dns_proxy_query_msg.querier_ip = ntohl(ipHdr->srcipaddr);
    dns_proxy_query_msg.querier_dns_dip = ntohl(ipHdr->destipaddr);
    dns_proxy_query_msg.querier_udp_port = ntohs(udpHdr->sport);
    dns_proxy_query_msg.querier_dns_transaction_id = dnsHdr->id;

    return DNS_proxy_query_queue(&dns_proxy_query_msg);
}

static mesa_rc DNS_packet_register(void)
{
    mesa_rc             rc;
    packet_rx_filter_t  filter;

    /* Register for UDP frames via packet API */
    packet_rx_filter_init(&filter);
    filter.modid    = VTSS_MODULE_ID_IP_DNS;
    filter.match    = PACKET_RX_FILTER_MATCH_IP_PROTO | PACKET_RX_FILTER_MATCH_ETYPE;
    filter.prio     = PACKET_RX_FILTER_PRIO_NORMAL;
    filter.ip_proto = 17;
    filter.etype    = ETYPE_IPV4;
    filter.cb       = DNS_rx_callback;

    rc = VTSS_RC_OK;
    DNS_PKT_ENTER();
    if (vtss_dns_database.filter_id != NULL) {
        rc = packet_rx_filter_change(&filter, &vtss_dns_database.filter_id);
    } else {
        rc = packet_rx_filter_register(&filter, &vtss_dns_database.filter_id);
    }
    DNS_PKT_EXIT();

#if defined(VTSS_SW_OPTION_ACCESS_MGMT)
#define IP_DNS_ACCESS_MGMT_SETUP(x)         do {    \
    (x)->source = ACCESS_MGMT_INTERNAL_TYPE_DNS;    \
    (x)->protocol = ACCESS_MGMT_PROTOCOL_TCP_UDP;   \
    (x)->start_src.type = MESA_IP_TYPE_NONE;        \
    (x)->end_src.type = MESA_IP_TYPE_NONE;          \
    (x)->start_dst.type = MESA_IP_TYPE_NONE;        \
    (x)->end_dst.type = MESA_IP_TYPE_NONE;          \
    (x)->start_sport = 0;                           \
    (x)->end_sport = 0;                             \
    (x)->start_dport = 53;                          \
    (x)->end_dport = 0;                             \
    (x)->vidx = VTSS_VID_NULL;                      \
    (x)->operation = FALSE;                         \
    (x)->priority = 0;                              \
} while (0)

    if (rc == VTSS_RC_OK) {
        access_mgmt_inter_module_entry_t    access_mgmt;

        access_mgmt.source = ACCESS_MGMT_INTERNAL_TYPE_DNS;
        if (access_mgmt_internal_entry_get(&access_mgmt) == VTSS_RC_OK) {
            IP_DNS_ACCESS_MGMT_SETUP(&access_mgmt);
            rc = access_mgmt_internal_entry_upd(&access_mgmt);
        } else {
            memset(&access_mgmt, 0x0, sizeof(access_mgmt_inter_module_entry_t));
            IP_DNS_ACCESS_MGMT_SETUP(&access_mgmt);
            rc = access_mgmt_internal_entry_add(&access_mgmt);
        }
    }

#undef IP_DNS_ACCESS_MGMT_SETUP
#endif /* defined(VTSS_SW_OPTION_ACCESS_MGMT) */

    return rc;
}

static mesa_rc DNS_packet_unregister(void)
{
    mesa_rc rc = VTSS_RC_OK;

    DNS_PKT_ENTER();
    if (vtss_dns_database.filter_id != NULL) {
        if ((rc = packet_rx_filter_unregister(vtss_dns_database.filter_id)) == VTSS_RC_OK) {
            vtss_dns_database.filter_id = NULL;
        }
    }
    DNS_PKT_EXIT();

#if defined(VTSS_SW_OPTION_ACCESS_MGMT)
    if (rc == VTSS_RC_OK) {
        access_mgmt_inter_module_entry_t    access_mgmt;

        access_mgmt.source = ACCESS_MGMT_INTERNAL_TYPE_DNS;
        if (access_mgmt_internal_entry_get(&access_mgmt) == VTSS_RC_OK) {
            rc = access_mgmt_internal_entry_del(&access_mgmt);
        }
    }
#endif /* defined(VTSS_SW_OPTION_ACCESS_MGMT) */

    return rc;
}

static BOOL DNS_do_proxy(dns_proxy_query_item_t *const dns_proxy_query_msg)
{
    vtss_hostent_t      hp;
    struct in_addr      ip;
    char                adrBuf[40];
    mesa_ip_addr_t      dns_ipa;

    if (!dns_proxy_query_msg) {
        return FALSE;
    }
    if (!msg_switch_is_primary()) {
        return TRUE;
    }

    T_D("Query[XID:%u] for %s (Type:%u/Class:%u)",
        dns_proxy_query_msg->querier_dns_transaction_id,
        dns_proxy_query_msg->querier_question,
        dns_proxy_query_msg->querier_question_type,
        dns_proxy_query_msg->querier_question_class);
    if (vtss_gethostbyname(dns_proxy_query_msg->querier_question, AF_INET, &hp) == VTSS_RC_OK) {
        memset(adrBuf, 0x0, sizeof(adrBuf));
        T_D("IP of %s is %s", dns_proxy_query_msg->querier_question, misc_ipv4_txt(hp.h_address.ipv4.address, adrBuf));

        transmit_dns_response(dns_proxy_query_msg, hp.h_address.ipv4.address);
    } else {
        if (dns_proxy_query_msg->querier_question_type == DNS_TYPE_PTR) {
            u8  idx;

            memset(adrBuf, 0x0, sizeof(adrBuf));
            if (vtss_dns_mgmt_active_server_get(&idx, &dns_ipa) == VTSS_RC_OK && idx < DNS_MAX_SRV_CNT) {
                if (dns_ipa.type == MESA_IP_TYPE_IPV4) {
                    (void) misc_ipv4_txt(dns_ipa.addr.ipv4, adrBuf);
                }
            }

            if (inet_aton(adrBuf, &ip) && vtss_gethostbyaddr(&ip, AF_INET, &hp) == VTSS_RC_OK) {
                T_D("Reverse lookup for %s from %s", hp.h_name, adrBuf);
                transmit_dns_response_rev_lookup(dns_proxy_query_msg, hp.h_name);
            } else {
                T_D("DNS_TYPE_PTR: No such name for %s from %s", dns_proxy_query_msg->querier_question, adrBuf);
                transmit_dns_no_such_name_response(dns_proxy_query_msg);
            }
        } else {
            T_D("No such name for %s", dns_proxy_query_msg->querier_question);
            transmit_dns_no_such_name_response(dns_proxy_query_msg);
        }
    }

    return TRUE;
}

static void DNS_queue_rtv(void)
{
    u8      *rcv_buf;
    int     buf_size;
    size_t  sz_val;

    if (!msg_switch_is_primary()) {
        DNS_PKT_ENTER();
        vtss_bip_buffer_clear(&vtss_dns_bip);
        DNS_PKT_EXIT();

        return;
    }

    buf_size = 0;
    DNS_PKT_ENTER();
    while ((rcv_buf = vtss_bip_buffer_get_contiguous_block(&vtss_dns_bip, &buf_size)) != NULL) {
        if ((u64)rcv_buf & 0x3) {
            T_D("BIP buffer not correctly aligned");
        }
        DNS_PKT_EXIT();

        sz_val = buf_size;
        if (!DNS_do_proxy((dns_proxy_query_item_t *)rcv_buf)) {
            T_D("DNS_do_proxy failed!");
        }
        sz_val = sizeof(int) * ((sz_val + 3) / sizeof(int));

        DNS_PKT_ENTER();
        vtss_bip_buffer_decommit_block(&vtss_dns_bip, sz_val);
    }
    DNS_PKT_EXIT();
}

static mesa_rc DNS_server_apply(u8 idx)
{
    mesa_rc                         rc;
    mesa_ip_addr_t                  ip, cur_dns;
    char                            buf[40];
    vtss_dns_srv_conf_t             *dns_conf;
    vtss_dns_srv_info_t             *dns_info;
    vtss_appl_dns_config_type_t     dns_type;
    mesa_ipv4_t                     dns_ipa4;
    vtss_ifindex_t                  dns_ifidx;
    mesa_ipv6_t                     *dns_ipa6;
    mesa_vid_t                      dns_vid;
    vtss::dhcp::ConfPacket          dhcp_fields;
#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)
    Dhcp6cInterface                 *dhcp6c_intf;

    if (VTSS_MALLOC_CAST(dhcp6c_intf, sizeof(Dhcp6cInterface)) == NULL) {
        T_D("Not enough memory!");
    }
#endif /* defined(VTSS_SW_OPTION_DHCP6_CLIENT) */

    T_D("DNS_server_apply(%u)", idx);

    DNS_CRIT_ENTER();
    dns_info = &vtss_dns_database.info.dns_info[idx];
    dns_conf = &vtss_dns_database.conf.dns_conf[idx];
    dns_type = VTSS_DNS_INFO_CONF_TYPE(dns_conf);
    dns_vid = VTSS_DNS_INFO_CONF_VLAN(dns_conf);

    if (vtss_dns_current_server_get(&cur_dns, (u8 *)&buf[0], (i32 *)&buf[4]) != VTSS_RC_OK || buf[0] >= DNS_MAX_SRV_CNT) {
        memset(&cur_dns, 0x0, sizeof(mesa_ip_addr_t));
    }

    DNS_CRIT_EXIT();

    memset(&ip, 0, sizeof(ip));
    switch ( dns_type ) {
    case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
        DNS_CRIT_ENTER();
        if (dns_conf->static_conf_addr.type == MESA_IP_TYPE_IPV4) {
            T_D("DNS: VTSS_APPL_DNS_CONFIG_TYPE_STATIC for IPv4");
            ip.type = MESA_IP_TYPE_IPV4;
            ip.addr.ipv4 = VTSS_DNS_INFO_CONF_ADDR4(dns_conf);
        } else if (dns_conf->static_conf_addr.type == MESA_IP_TYPE_IPV6) {
            T_D("DNS: VTSS_APPL_DNS_CONFIG_TYPE_STATIC for IPv6");
            dns_ipa6 = VTSS_DNS_INFO_CONF_ADDR6(dns_conf);
#if defined(VTSS_SW_OPTION_IP)
            if (dns_ipa6 && !DNS_addr6_is_zero(dns_ipa6) && vtss_ipv6_addr_is_mgmt_support(dns_ipa6)) {
#else
            if (dns_ipa6 && !DNS_addr6_is_zero(dns_ipa6) && dns_ipa6->addr[0] != 0xFF) {
#endif /* defined(VTSS_SW_OPTION_IP) */
                dns_vid = dns_conf->egress_vlan;
                ip.type = MESA_IP_TYPE_IPV6;
                memcpy(&ip.addr.ipv6, dns_ipa6, sizeof(mesa_ipv6_t));
            } else {
                ip.type = MESA_IP_TYPE_NONE;
            }
        }
        DNS_CRIT_EXIT();
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY:
        T_D("DNS: VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY");
        rc = vtss::dhcp::client_dns_option_ip_any_get(cur_dns.addr.ipv4, &dns_ipa4);

        if (rc == VTSS_RC_OK) {
            T_D("Got dns address from dhcp");
            ip.type = MESA_IP_TYPE_IPV4;
            ip.addr.ipv4 = dns_ipa4;
        } else {
            T_D("Failed to get dns address from dhcp");
            ip.type = MESA_IP_TYPE_NONE;
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
        T_D("DNS: VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN");
        rc = vtss_ifindex_from_vlan(dns_vid, &dns_ifidx);
        if (rc == VTSS_RC_OK) {
            rc = vtss::dhcp::client_fields_get(dns_ifidx, &dhcp_fields);
        }

        if (rc == VTSS_RC_OK && dhcp_fields.domain_name_server.valid()) {
            T_D("Got dns address from dhcp on VLAN: %u", dns_vid);
            ip.type = MESA_IP_TYPE_IPV4;
            ip.addr.ipv4 = dhcp_fields.domain_name_server.get();
        } else {
            T_D("Failed to get dns address from dhcp on VLAN: %u", dns_vid);
            ip.type = MESA_IP_TYPE_NONE;
        }
        break;
#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY:
        T_D("DNS: VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY");

        ip.type = MESA_IP_TYPE_NONE;
        dns_vid = VTSS_VID_NULL;

        if (dhcp6c_intf == NULL) {
            T_D("Invalid DHCP6C memory!");
            break;
        }

        while (vtss::dhcp6c::dhcp6_client_interface_itr(dns_vid, dhcp6c_intf) == VTSS_RC_OK) {
            dns_vid = dhcp6c_intf->ifidx;

#if defined(VTSS_SW_OPTION_IP)
            if (!DNS_addr6_is_zero(&dhcp6c_intf->dns_srv_addr) && vtss_ipv6_addr_is_mgmt_support(&dhcp6c_intf->dns_srv_addr)) {
#else
            if (!DNS_addr6_is_zero(&dhcp6c_intf->dns_srv_addr) && dhcp6c_intf->dns_srv_addr.addr[0] != 0xFF) {
#endif /* defined(VTSS_SW_OPTION_IP) */
                T_D("Got dns address from dhcp6c on VLAN: %u", dns_vid);
                ip.type = MESA_IP_TYPE_IPV6;
                memcpy(&ip.addr.ipv6, &dhcp6c_intf->dns_srv_addr, sizeof(mesa_ipv6_t));

                break;
            }
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
        T_D("DNS: VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN");

        if (dhcp6c_intf == NULL) {
            ip.type = MESA_IP_TYPE_NONE;
            T_D("Invalid DHCP6C memory!");
            break;
        }

        rc = vtss::dhcp6c::dhcp6_client_interface_get(dns_vid, dhcp6c_intf);

#if defined(VTSS_SW_OPTION_IP)
        if (rc == VTSS_RC_OK && !DNS_addr6_is_zero(&dhcp6c_intf->dns_srv_addr) && vtss_ipv6_addr_is_mgmt_support(&dhcp6c_intf->dns_srv_addr)) {
#else
        if (rc == VTSS_RC_OK && !DNS_addr6_is_zero(&dhcp6c_intf->dns_srv_addr) && dhcp6c_intf->dns_srv_addr.addr[0] != 0xFF) {
#endif /* defined(VTSS_SW_OPTION_IP) */
            T_D("Got dns address from dhcp6c on VLAN: %u", dns_vid);
            ip.type = MESA_IP_TYPE_IPV6;
            memcpy(&ip.addr.ipv6, &dhcp6c_intf->dns_srv_addr, sizeof(mesa_ipv6_t));
        } else {
            T_D("Failed to get dns address from dhcp6c on VLAN: %u", dns_vid);
            ip.type = MESA_IP_TYPE_NONE;
        }
        break;
#endif /* defined(VTSS_SW_OPTION_DHCP6_CLIENT) */
    case VTSS_APPL_DNS_CONFIG_TYPE_NONE:
        T_D("DNS: VTSS_APPL_DNS_CONFIG_TYPE_NONE");
        ip.type = MESA_IP_TYPE_NONE;
        break;
    default:
        T_D("DNS: default");
        ip.type = MESA_IP_TYPE_NONE;
        break;
    }

#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)
    if (dhcp6c_intf) {
        VTSS_FREE(dhcp6c_intf);
    }
#endif /* defined(VTSS_SW_OPTION_DHCP6_CLIENT) */

    rc = VTSS_RC_ERROR;
    DNS_CRIT_ENTER();
    switch ( ip.type ) {
    case MESA_IP_TYPE_NONE:
        rc = vtss_dns_current_server_set(VTSS_VID_NULL, 0, idx);

        break;
    case MESA_IP_TYPE_IPV4:
    case MESA_IP_TYPE_IPV6:
        rc = vtss_dns_current_server_set(dns_vid, &ip, idx);

        break;
    default:
        break;
    }
    if (rc == VTSS_RC_OK) {
        memset(dns_info, 0x0, sizeof(vtss_dns_srv_info_t));
        dns_info->srv_type = dns_type;
        if (ip.type != MESA_IP_TYPE_NONE) {
            dns_info->srv_addr = ip;
        } else {
            dns_info->srv_addr.type = MESA_IP_TYPE_NONE;
        }
        dns_info->srv_vlan = dns_vid;
    }
    DNS_CRIT_EXIT();

    if (rc == VTSS_RC_OK) {
        T_D("Updated DNS-%u information to: %s", idx,
            ip.type == MESA_IP_TYPE_IPV4 ? misc_ipv4_txt(ip.addr.ipv4, buf) :
            (ip.type == MESA_IP_TYPE_IPV6 ? misc_ipv6_txt(&ip.addr.ipv6, buf) : "NONE"));
    } else {
        T_D("Failed to updated DNS-%u information to: %s", idx,
            ip.type == MESA_IP_TYPE_IPV4 ? misc_ipv4_txt(ip.addr.ipv4, buf) :
            (ip.type == MESA_IP_TYPE_IPV6 ? misc_ipv6_txt(&ip.addr.ipv6, buf) : "NONE"));
    }

    return rc;
}

static mesa_rc DNS_domainname_apply(void)
{
    mesa_rc                     rc;
    vtss_dns_domainname_conf_t  *dnp;
    vtss_appl_dns_config_type_t dnt;
    mesa_vid_t                  dnv;
    vtss_ifindex_t              dnifidx;
    char                        *np, dnn[DNS_MAX_NAME_LEN + 1];
    vtss::Buffer                b;
    vtss::dhcp::ConfPacket      dhcp_fields;
#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)
    mesa_vid_t                  vidx;
    Dhcp6cInterface             *dhcp6c_intf = NULL;
#endif /* defined(VTSS_SW_OPTION_DHCP6_CLIENT) */

    T_D("DNS_domainname_apply");

    DNS_CRIT_ENTER();
    dnp = &vtss_dns_database.conf.default_domain_name;
    dnt = dnp->domain_name_type;
    dnv = dnp->domain_name_vlan;
    memcpy(dnn, dnp->domain_name_char, DNS_MAX_NAME_LEN + 1);
    np = &dnn[0];
    DNS_CRIT_EXIT();

    rc = vtss_ifindex_from_vlan(dnv, &dnifidx);
    switch ( dnt ) {
    case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
        T_D("DOMAINNAME Type: VTSS_APPL_DNS_CONFIG_TYPE_STATIC");
        if (strlen(dnn) < 1) {
            np = NULL;
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY:
        T_D("DOMAINNAME Type: VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY");
        if ( vtss::dhcp::client_dns_option_domain_any_get(&b) == VTSS_RC_OK && b.size() <= DNS_MAX_NAME_LEN) {
            strncpy(dnn, b.begin(), DNS_MAX_NAME_LEN);
            dnn[DNS_MAX_NAME_LEN] = 0;
            np = &dnn[0];
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
        T_D("DOMAINNAME Type: VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN");
        if ( vtss::dhcp::client_fields_get(dnifidx, &dhcp_fields) == VTSS_RC_OK && dhcp_fields.domain_name.size() && dhcp_fields.domain_name.size() <= DNS_MAX_NAME_LEN) {
            strncpy(dnn, dhcp_fields.domain_name.begin(), DNS_MAX_NAME_LEN);
            dnn[DNS_MAX_NAME_LEN] = 0;
            np = &dnn[0];
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY:
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
        T_D("DOMAINNAME Type: %s",
            dnt == VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN
            ? "VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN"
            : "VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY");
        np = NULL;
#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)
        if (VTSS_MALLOC_CAST(dhcp6c_intf, sizeof(Dhcp6cInterface)) == NULL) {
            T_D("Not enough memory!");
            rc = VTSS_APPL_DHCP6C_ERROR_MEMORY_NG;
            break;
        }

        vidx = VTSS_VID_NULL;
        while (vtss::dhcp6c::dhcp6_client_interface_itr(vidx, dhcp6c_intf) == VTSS_RC_OK) {
            vidx = dhcp6c_intf->ifidx;
            if (dnt == VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN &&
                vidx != dnv) {
                continue;
            }

            if (strlen(dhcp6c_intf->dns_domain_name) > 0 &&
                strlen(dhcp6c_intf->dns_domain_name) < DNS_MAX_NAME_LEN + 1) {
                strncpy(dnn, dhcp6c_intf->dns_domain_name, strlen(dhcp6c_intf->dns_domain_name));
                dnn[strlen(dhcp6c_intf->dns_domain_name)] = 0;
                np = &dnn[0];
                break;
            }
        }
        VTSS_FREE(dhcp6c_intf);
#endif /* defined(VTSS_SW_OPTION_DHCP6_CLIENT) */
        break;
    default:
        T_D("DOMAINNAME Type: %d", dnt);
        np = NULL;
        break;
    }

    if (rc == VTSS_RC_OK) {
        DNS_CRIT_ENTER();
        rc = vtss_dns_default_domainname_set(np);
        DNS_CRIT_EXIT();
    }

    T_D("Update domain name information %s to: %s",
        rc == VTSS_RC_OK ? "OK" : "NG", np ? np : "!NULL!");

    return rc;
}

static void DNS_server_probe(void)
{
    u8  idx;

    if (DNS_domainname_apply() != VTSS_RC_OK) {
        T_D("Failed in DNS_domainname_apply()");
    }

    for (idx = 0; idx< DNS_MAX_SRV_CNT; ++idx) {
        if (DNS_server_apply(idx) != VTSS_RC_OK) {
            T_D("Failed in DNS_server_apply(%u)", idx);
        }
    }
}

static void DNS_timer_isr(struct vtss::Timer *timer)
{
    vtss_flag_setbits(&vtss_dns_thread_flag, DNS_EVENT_WAKEUP);
}

static void DNS_conf_default(void)
{
    u8          idx;
    dns_conf_t  *conf;
    dns_info_t  *info;

    DNS_CRIT_ENTER();

    info = &vtss_dns_database.info;
    memset(info, 0x0, sizeof(dns_info_t));

    conf = &vtss_dns_database.conf;

    /* Use default configuration */
    memset(conf, 0x0, sizeof(dns_conf_t));
    conf->default_domain_name.domain_name_type = VTSS_DNS_DOMAINAME_DEF_TYPE;
    conf->dns_proxy_status = VTSS_DNS_PROXY_DEF_STATE;
    for (idx = 0; idx < DNS_MAX_SRV_CNT; ++idx) {
        VTSS_DNS_TYPE_SET(&conf->dns_conf[idx], VTSS_DNS_SERVER_DEF_TYPE);
    }

    DNS_CRIT_EXIT();

    vtss_dns_signal();
}

void vtss_dns_thread(vtss_addrword_t data)
{
    u32                 skip_cnt;
    BOOL                dns_proxy_status;
    vtss_flag_value_t   events;

    /* Initialize EVENT groups */
    vtss_flag_init(&vtss_dns_thread_flag);

    skip_cnt = 0;
    while (!VTSS_DNS_READY) {
        VTSS_DNS_BREAK_WAIT(skip_cnt);
    }

    /* Initialize Periodical Wakeup Timer  */
    vtss_dns_thread_timer.set_repeat(true);
    vtss_dns_thread_timer.set_period(vtss::milliseconds(1000));
    vtss_dns_thread_timer.callback = DNS_timer_isr;
    vtss_dns_thread_timer.modid = VTSS_MODULE_ID_IP_DNS;

    DNS_CRIT_ENTER();
    dns_proxy_status = vtss_dns_database.conf.dns_proxy_status;
    DNS_CRIT_EXIT();
    if (dns_proxy_status == VTSS_DNS_PROXY_ENABLE) {
        if (DNS_packet_register() != VTSS_RC_OK) {
            T_D("Failed to do packet_register");
        }
    } else {
        if (DNS_packet_unregister() != VTSS_RC_OK) {
            T_D("Failed to do packet_unregister");
        }
    }

    _vtss_dns_thread_status_set(TRUE);
    if (vtss_timer_start(&vtss_dns_thread_timer) != VTSS_RC_OK) {
        T_D("vtss_timer_start failed");
    }
    DNS_server_probe();

    T_I("VTSS_DNS_READY");
    while (VTSS_DNS_READY) {
        events = vtss_flag_wait(&vtss_dns_thread_flag, DNS_EVENT_ANY, VTSS_FLAG_WAITMODE_OR_CLR);

        if (events & DNS_EVENT_PROBE) {
            T_D("DNS_EVENT_PROBE");
            DNS_server_probe();
        }

        if (events & DNS_EVENT_QRTV) {
            T_D("DNS_EVENT_QRTV");
            DNS_queue_rtv();
        }

        if (events & DNS_EVENT_WAKEUP) {
            u8                  idx, rnd;
            i32                 cnt;
            mesa_ip_addr_t      dns_ipa;
            mesa_rc             rc;
            vtss_tick_count_t   ts = vtss_current_time();

            T_N("DNS_EVENT_WAKEUP(" VPRI64u")", ts);

            DNS_CRIT_ENTER();
            if (vtss_dns_current_server_get(&dns_ipa, &idx, &cnt) == VTSS_RC_OK &&
                idx < DNS_MAX_SRV_CNT && cnt > DNS_MAX_SRV_ERR) {
                rc = VTSS_RC_ERROR;
                for (rnd = idx + 1; rnd < DNS_MAX_SRV_CNT; ++rnd) {
                    if (VTSS_DNS_INFO_CONF_TYPE(&vtss_dns_database.conf.dns_conf[rnd]) != VTSS_APPL_DNS_CONFIG_TYPE_NONE) {
                        DNS_CRIT_EXIT();
                        rc = DNS_server_apply(rnd);
                        DNS_CRIT_ENTER();

                        if (rc == VTSS_RC_OK) {
                            if ((rc = vtss_dns_current_server_rst(rnd)) == VTSS_RC_OK) {
                                break;
                            }
                        }
                    }
                }
                for (rnd = 0; rc != VTSS_RC_OK && rnd < idx; ++rnd) {
                    if (VTSS_DNS_INFO_CONF_TYPE(&vtss_dns_database.conf.dns_conf[rnd]) != VTSS_APPL_DNS_CONFIG_TYPE_NONE) {
                        DNS_CRIT_EXIT();
                        rc = DNS_server_apply(rnd);
                        DNS_CRIT_ENTER();

                        if (rc == VTSS_RC_OK) {
                            if ((rc = vtss_dns_current_server_rst(rnd)) == VTSS_RC_OK) {
                                break;
                            }
                        }
                    }
                }

                T_D("Activate DNS server %s:(%u->%u)", rc == VTSS_RC_OK ? "OK" : "NG", idx, rnd);
            }

            if (vtss_dns_tick_cache(ts) != VTSS_RC_OK) {
                T_D("Failed to process DNS cache at TICK " VPRI64u"", ts);
            }
            DNS_CRIT_EXIT();
        }
    }
    T_I("VTSS_DNS_EXIT");

    _vtss_dns_thread_status_set(FALSE);
}

#ifdef VTSS_SW_OPTION_IPV6
mesa_rc vtss_dns_mgmt_get_server6(mesa_ipv6_t *dns_srv)
{
    mesa_rc             rc;
    u8                  idx;
    i32                 cnt;
    vtss_dns_srv_conf_t *dns_conf;
    mesa_ip_addr_t      dns_ipa;
    mesa_ipv6_t         *ipa6;
    char                ipbuf1[45], ipbuf2[45];

    if (!dns_srv) {
        return VTSS_RC_ERROR;
    }

    rc = VTSS_RC_OK;
    memset(ipbuf1, 0x0, sizeof(ipbuf1));
    memset(ipbuf2, 0x0, sizeof(ipbuf2));
    DNS_CRIT_ENTER();
    if (vtss_dns_current_server_get(&dns_ipa, &idx, &cnt) != VTSS_RC_OK || idx >= DNS_MAX_SRV_CNT) {
        DNS_CRIT_EXIT();
        return VTSS_RC_ERROR;
    }
    dns_conf = &vtss_dns_database.conf.dns_conf[idx];
    switch ( VTSS_DNS_INFO_CONF_TYPE(dns_conf) ) {
    case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
        if (dns_conf->static_conf_addr.type != MESA_IP_TYPE_IPV6) {
            rc = IP_DNS_ERROR_GEN;
            break;
        }

        ipa6 = VTSS_DNS_INFO_CONF_ADDR6(dns_conf);
        if (!ipa6 || memcmp(&dns_ipa.addr.ipv6, ipa6, sizeof(mesa_ipv6_t))) {
            T_D("Running: %s != Config: %s",
                misc_ipv6_txt(&dns_ipa.addr.ipv6, ipbuf1),
                ipa6 ? misc_ipv6_txt(ipa6, ipbuf2) : "NULL");
            rc = IP_DNS_ERROR_GEN;
        } else {
            memcpy(dns_srv, &dns_ipa.addr.ipv6, sizeof(mesa_ipv6_t));
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY:
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
        memcpy(dns_srv, &dns_ipa.addr.ipv6, sizeof(mesa_ipv6_t));
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_NONE:
    default:
        if (!DNS_addr6_is_zero(&dns_ipa.addr.ipv6)) {
            T_D("Running DNS is not empty: %s", misc_ipv6_txt(&dns_ipa.addr.ipv6, ipbuf1));
            rc = IP_DNS_ERROR_GEN;
        } else {
            memset(dns_srv, 0x0, sizeof(mesa_ipv6_t));
        }

        break;
    }
    DNS_CRIT_EXIT();

    return rc;
}
#endif /* VTSS_SW_OPTION_IPV6 */

mesa_rc vtss_dns_mgmt_get_server4(mesa_ipv4_t *dns_srv)
{
    mesa_rc             rc;
    u8                  idx;
    i32                 cnt;
    vtss_dns_srv_conf_t *dns_conf;
    mesa_ip_addr_t      dns_ipa;
    char                ipbuf[45];

    if (!dns_srv) {
        return VTSS_RC_ERROR;
    }

    rc = VTSS_RC_OK;
    DNS_CRIT_ENTER();
    if (vtss_dns_current_server_get(&dns_ipa, &idx, &cnt) != VTSS_RC_OK || idx >= DNS_MAX_SRV_CNT) {
        DNS_CRIT_EXIT();
        return VTSS_RC_ERROR;
    }
    dns_conf = &vtss_dns_database.conf.dns_conf[idx];
    switch ( VTSS_DNS_INFO_CONF_TYPE(dns_conf) ) {
    case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
        if (dns_conf->static_conf_addr.type != MESA_IP_TYPE_IPV4) {
            rc = IP_DNS_ERROR_GEN;
            break;
        }

        if (dns_ipa.addr.ipv4 != VTSS_DNS_INFO_CONF_ADDR4(dns_conf)) {
            T_D("Running: %s != Config: %u",
                misc_ipv4_txt(dns_ipa.addr.ipv4, ipbuf),
                VTSS_DNS_INFO_CONF_ADDR4(dns_conf));
            rc = IP_DNS_ERROR_GEN;
        } else {
            *dns_srv = dns_ipa.addr.ipv4;
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY:
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
        *dns_srv = dns_ipa.addr.ipv4;
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_NONE:
    default:
        if (dns_ipa.addr.ipv4) {
            T_D("Running DNS is not empty: %s", misc_ipv4_txt(dns_ipa.addr.ipv4, ipbuf));
            rc = IP_DNS_ERROR_GEN;
        } else {
            *dns_srv = 0;
        }
        break;
    }
    DNS_CRIT_EXIT();

    return rc;
}

BOOL vtss_dns_mgmt_support_multicast(void)
{
    /*
        TODO:
        If multicast server is not support, we don't
        allow IPv6 linklocal as well.
    */
    return VTSS_DNS_SUPPORT_MULTICAST;
}

mesa_rc vtss_dns_mgmt_active_server_get(u8 *const idx, mesa_ip_addr_t *const srv)
{
    i32     cnt;
    mesa_rc rc;

    if (!idx || !srv) {
        return VTSS_RC_ERROR;
    }

    DNS_CRIT_ENTER();
    rc = vtss_dns_current_server_get(srv, idx, &cnt);
    DNS_CRIT_EXIT();

    return rc;
}

mesa_rc vtss_dns_mgmt_get_server(u8 idx, vtss_dns_srv_conf_t *dns_srv)
{
    if (!dns_srv || !(idx < DNS_MAX_SRV_CNT)) {
        return VTSS_RC_ERROR;
    }

    DNS_CRIT_ENTER();
    memcpy(dns_srv, &vtss_dns_database.conf.dns_conf[idx], sizeof(vtss_dns_srv_conf_t));
    DNS_CRIT_EXIT();

    return VTSS_RC_OK;
}

mesa_rc vtss_dns_mgmt_set_server(u8 idx, vtss_dns_srv_conf_t *dns_srv)
{
    vtss_dns_srv_conf_t *dns_conf, dns_setting;
    mesa_ipv6_t         *ip6a;

    if (!dns_srv || !(idx < DNS_MAX_SRV_CNT)) {
        return VTSS_RC_ERROR;
    }

    T_D("enter(%u)", idx);

    /* Sanity */
    memset(&dns_setting, 0x0, sizeof(vtss_dns_srv_conf_t));
    switch ( VTSS_DNS_INFO_CONF_TYPE(dns_srv) ) {
    case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
        T_D("VTSS_APPL_DNS_CONFIG_TYPE_STATIC");
        if (dns_srv->static_conf_addr.type == MESA_IP_TYPE_IPV6) {
            ip6a = VTSS_DNS_INFO_CONF_ADDR6(dns_srv);
            if (!ip6a) {
                return IP_DNS_ERROR_GEN;
            }
            if (ip6a->addr[0] == 0xfe && (ip6a->addr[1] >> 6) == 0x2) {
                if (dns_srv->egress_vlan >= VTSS_APPL_VLAN_ID_MIN &&
                    dns_srv->egress_vlan <= VTSS_APPL_VLAN_ID_MAX) {
                    dns_setting.egress_vlan = dns_srv->egress_vlan;
                } else {
                    return IP_DNS_ERROR_GEN;
                }
            }
        }
        if (VTSS_DNS_ADDR_VALID(dns_srv)) {
            VTSS_DNS_ADDR_SET(&dns_setting, VTSS_DNS_ADDR_PTR(dns_srv));
        } else {
            return IP_DNS_ERROR_GEN;
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
        T_D("VTSS_APPL_DNS_CONFIG_TYPE_DHCP_VLAN");
        if (VTSS_DNS_VLAN_VALID(dns_srv)) {
            VTSS_DNS_VLAN_SET(&dns_setting, VTSS_DNS_VLAN_GET(dns_srv));
        } else {
            return IP_DNS_ERROR_GEN;
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY:
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY:
        T_D("VTSS_APPL_DNS_CONFIG_TYPE_DHCP");
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_NONE:
        T_D("VTSS_APPL_DNS_CONFIG_TYPE_DHCP");
        break;
    default:
        return IP_DNS_ERROR_GEN;
    }
    VTSS_DNS_TYPE_SET(&dns_setting, VTSS_DNS_INFO_CONF_TYPE(dns_srv));

    DNS_CRIT_ENTER();
    dns_conf = &vtss_dns_database.conf.dns_conf[idx];
    if (!memcmp(&dns_setting, dns_conf, sizeof(vtss_dns_srv_conf_t))) {
        DNS_CRIT_EXIT();
        T_D("exit(SAME)");
        return VTSS_RC_OK;
    }

    memcpy(dns_conf, &dns_setting, sizeof(vtss_dns_srv_conf_t));
    DNS_CRIT_EXIT();

    vtss_dns_signal();

    T_D("exit(DIFF)");

    return VTSS_RC_OK;
}

mesa_rc vtss_dns_mgmt_get_proxy_status(BOOL *status)
{
    DNS_CRIT_ENTER();
    *status = vtss_dns_database.conf.dns_proxy_status;
    DNS_CRIT_EXIT();

    return VTSS_RC_OK;
}

mesa_rc vtss_dns_mgmt_set_proxy_status(BOOL *status)
{

    if (!status) {
        return VTSS_RC_ERROR;
    }

    DNS_CRIT_ENTER();
    if (vtss_dns_database.conf.dns_proxy_status == *status) {
        DNS_CRIT_EXIT();
        return VTSS_RC_OK;
    }
    vtss_dns_database.conf.dns_proxy_status = *status;
    DNS_CRIT_EXIT();

    if (VTSS_DNS_READY) {
        if (*status == VTSS_DNS_PROXY_ENABLE) {
            if (DNS_packet_register() != VTSS_RC_OK) {
                T_D("Failed to do packet_register");
            }
        } else {
            if (DNS_packet_unregister() != VTSS_RC_OK) {
                T_D("Failed to do packet_unregister");
            }
        }
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_dns_mgmt_default_domainname_get(vtss_dns_domainname_conf_t *const domain_name)
{
    mesa_rc                     rc;
    char                        dn[DNS_MAX_NAME_LEN + 1];
    vtss_dns_domainname_conf_t  *defdn;

    if (!domain_name) {
        return VTSS_RC_ERROR;
    }

    memset(dn, 0x0, sizeof(dn));
    DNS_CRIT_ENTER();
    rc = vtss_dns_default_domainname_get(dn);
    defdn = &vtss_dns_database.conf.default_domain_name;
    if (defdn->domain_name_type == VTSS_APPL_DNS_CONFIG_TYPE_STATIC) {
        if (strncmp(defdn->domain_name_char, dn, strlen(dn))) {
            strncpy(domain_name->domain_name_char, defdn->domain_name_char, strlen(defdn->domain_name_char));
            domain_name->domain_name_char[strlen(defdn->domain_name_char)] = 0;
        } else {
            strncpy(domain_name->domain_name_char, dn, strlen(dn));
            domain_name->domain_name_char[strlen(dn)] = 0;
        }
        rc = VTSS_RC_OK;
    } else {
        if (defdn->domain_name_type == VTSS_APPL_DNS_CONFIG_TYPE_NONE) {
            if (rc == VTSS_RC_OK && strlen(dn)) {
                T_D("No domain name setting but existing domain name %s", dn);
            }
            memset(domain_name->domain_name_char, 0x0, sizeof(domain_name->domain_name_char));
        } else {
            if (rc == VTSS_RC_OK && strlen(dn)) {
                T_I("Retrieved domain name is %s", dn);
                strncpy(domain_name->domain_name_char, dn, strlen(dn));
                domain_name->domain_name_char[strlen(dn)] = 0;
            } else {
                T_I("No domain name is retrieved");
                memset(domain_name->domain_name_char, 0x0, sizeof(domain_name->domain_name_char));
            }
        }
    }
    domain_name->domain_name_vlan = defdn->domain_name_vlan;
    domain_name->domain_name_type = defdn->domain_name_type;
    DNS_CRIT_EXIT();

    return rc;
}

mesa_rc vtss_dns_mgmt_default_domainname_set(const vtss_dns_domainname_conf_t *const domain_name)
{
    mesa_rc                     rc;
    char                        dn[DNS_MAX_NAME_LEN + 1];
    vtss_dns_domainname_conf_t  *defdn;
    BOOL                        do_signal;

    if (!domain_name) {
        return VTSS_RC_ERROR;
    }

    do_signal = FALSE;
    DNS_CRIT_ENTER();
    defdn = &vtss_dns_database.conf.default_domain_name;
    memcpy(dn, defdn->domain_name_char, DNS_MAX_NAME_LEN + 1);
    memset(defdn->domain_name_char, 0x0, DNS_MAX_NAME_LEN + 1);
    if (domain_name->domain_name_type == VTSS_APPL_DNS_CONFIG_TYPE_STATIC &&
        strlen(domain_name->domain_name_char) > 0) {
        strncpy(defdn->domain_name_char, domain_name->domain_name_char, strlen(domain_name->domain_name_char));
    }

    if ((rc = vtss_dns_default_domainname_set(NULL)) != VTSS_RC_OK) {
        T_D("Failed to set domainname(RC=%d)", rc);
        memcpy(defdn->domain_name_char, dn, DNS_MAX_NAME_LEN + 1);
    }

    defdn->domain_name_vlan = domain_name->domain_name_vlan;
    if ((defdn->domain_name_type = domain_name->domain_name_type) != VTSS_APPL_DNS_CONFIG_TYPE_NONE) {
        do_signal = TRUE;
    }
    DNS_CRIT_EXIT();

    if (do_signal) {
        vtss_dns_signal();
    }

    return rc;
}

/* IP DNS error text */
const char *ip_dns_error_txt(ip_dns_error_t rc)
{
    const char *txt;

    switch (rc) {
    case IP_DNS_ERROR_GEN:
        txt = "IP DNS generic error";
        break;
    default:
        txt = "IP DNS unknown error";
        break;
    }
    return txt;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void vtss_appl_dns_mib_init(void);
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_dns_json_init(void);
#endif /* VTSS_SW_OPTION_JSON_RPC */
extern "C" int ip_dns_icli_cmd_register();

/* Initialize module */
mesa_rc ip_dns_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");

        memset(&vtss_dns_database, 0x0, sizeof(dns_global_record_t));
        vtss_dns_init();

        if (!vtss_bip_buffer_init(&vtss_dns_bip, DNS_BIP_BUF_SZ_B)) {
            T_E("vtss_bip_buffer_init failed!");
        }

        /* Create semaphore for critical regions */
        critd_init(&vtss_dns_database.dns_crit, "ip_dns_crit", VTSS_MODULE_ID_IP_DNS, CRITD_TYPE_MUTEX);
        critd_init(&vtss_dns_database.dns_pkt,  "ip_dns_pkt",  VTSS_MODULE_ID_IP_DNS, CRITD_TYPE_MUTEX);

#ifdef VTSS_SW_OPTION_ICFG
        if (ip_dns_icfg_init() != VTSS_RC_OK) {
            T_E("ip_dns_icfg_init failed!");
        }
#endif /* VTSS_SW_OPTION_ICFG */

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        vtss_appl_dns_mib_init();   /* Register DNS Private-MIB */
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_dns_json_init();  /* Register DNS JSON-RPC */
#endif /* VTSS_SW_OPTION_JSON_RPC */
        ip_dns_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_D("START");

        DNS_CRIT_ENTER();
        (void) vtss_dns_current_server_set(VTSS_VID_NULL, 0, DNS_MAX_SRV_CNT);
        DNS_CRIT_EXIT();

        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           vtss_dns_thread,
                           0,
                           "DNS_Handler",
                           nullptr,
                           0,
                           &vtss_dns_thread_handle,
                           &vtss_dns_thread_block);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        DNS_conf_default();
        DNS_PKT_ENTER();
        vtss_bip_buffer_clear(&vtss_dns_bip);
        DNS_PKT_EXIT();
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        DNS_conf_default();
        DNS_PKT_ENTER();
        vtss_bip_buffer_clear(&vtss_dns_bip);
        DNS_PKT_EXIT();
        _vtss_dns_thread_status_set(TRUE);
        DNS_server_probe();
        break;

    default:
        break;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

/*****************************************************************************
    Public API section for DNS
    from vtss_appl/include/vtss/appl/dns.h
*****************************************************************************/
#include "vtss/basics/expose/snmp/iterator-compose-static-range.hxx"

static BOOL DNS_domainname_config_sanity(
    const vtss_appl_dns_name_conf_t     *const conf,
    mesa_vid_t                          *const vidx
)
{
    mesa_vid_t                      chk_vid;
    vtss_ifindex_elm_t              ife;
    vtss_appl_dns_capabilities_t    cap;
    BOOL                            rc = FALSE;

    if (!conf || !vidx ||
        vtss_appl_dns_capabilities_get(&cap) != VTSS_RC_OK ||
        !cap.support_default_domain_name) {
        T_D("Invalid Input!");
        return rc;
    }

    *vidx = VTSS_VID_NULL;
    switch ( conf->domainname_type ) {
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY:
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
        if (!cap.support_dhcp4_domain_name) {
            T_D("Not Support!");
            break;
        }

        if (conf->domainname_type == VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY ||
            conf->dhcp_ifindex == VTSS_IFINDEX_NONE) {
            rc = TRUE;
            break;
        }

        if (vtss_ifindex_decompose(conf->dhcp_ifindex, &ife) == VTSS_RC_OK &&
            ife.iftype == VTSS_IFINDEX_TYPE_VLAN) {
            if ((chk_vid = (mesa_vid_t)ife.ordinal) != VTSS_VID_NULL) {
                if (chk_vid >= VTSS_APPL_VLAN_ID_MIN &&
                    chk_vid <= VTSS_APPL_VLAN_ID_MAX) {
                    *vidx = chk_vid;
                    rc = TRUE;
                }
            } else {
                rc = TRUE;
            }
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY:
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
        if (!cap.support_dhcp6_domain_name) {
            T_D("Not Support!");
            break;
        }

        if (conf->domainname_type == VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY ||
            conf->dhcp_ifindex == VTSS_IFINDEX_NONE) {
            rc = TRUE;
            break;
        }

        if (vtss_ifindex_decompose(conf->dhcp_ifindex, &ife) == VTSS_RC_OK &&
            ife.iftype == VTSS_IFINDEX_TYPE_VLAN) {
            if ((chk_vid = (mesa_vid_t)ife.ordinal) != VTSS_VID_NULL) {
                if (chk_vid >= VTSS_APPL_VLAN_ID_MIN &&
                    chk_vid <= VTSS_APPL_VLAN_ID_MAX) {
                    *vidx = chk_vid;
                    rc = TRUE;
                }
            } else {
                rc = TRUE;
            }
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
        if (strlen(conf->static_domain_name) > 0 &&
            misc_str_is_domainname(conf->static_domain_name) == VTSS_RC_OK) {
            rc = TRUE;
        }
        break;
    default:
        T_D("Skip checking");
        rc = TRUE;
        break;
    }

    T_D("CHK-Done(%s)", rc ? "OK" : "NG");
    return rc;
}

static BOOL DNS_server_config_sanity(
    const vtss_appl_dns_server_conf_t   *const conf,
    mesa_vid_t                          *const vidx
)
{
    u8                              chk_pfx;
    mesa_ipv6_t                     ipa6;
    mesa_vid_t                      chk_vid;
    vtss_ifindex_elm_t              ife;
    vtss_appl_dns_capabilities_t    cap;
    BOOL                            rc = FALSE;

    if (!conf || !vidx ||
        vtss_appl_dns_capabilities_get(&cap) != VTSS_RC_OK) {
        T_D("Invalid Input!");
        return rc;
    }

    *vidx = VTSS_VID_NULL;
    switch ( conf->server_type ) {
    case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
        if (conf->static_ip_address.type == MESA_IP_TYPE_IPV4) {
            chk_pfx = (u8)((conf->static_ip_address.addr.ipv4 >> 24) & 0xFF);
            if (chk_pfx && chk_pfx != 127 && chk_pfx < 224) {
                rc = TRUE;
            } else {
                if (conf->static_ip_address.addr.ipv4 == 0) {
                    rc = TRUE;
                }
            }
        } else if (conf->static_ip_address.type == MESA_IP_TYPE_IPV6) {
            ipa6 = conf->static_ip_address.addr.ipv6;
#if defined(VTSS_SW_OPTION_IP)
            if (vtss_ipv6_addr_is_mgmt_support(&ipa6)) {
#else
            if (ipa6.addr[0] != 0xFF) {
#endif /* defined(VTSS_SW_OPTION_IP) */
                if (vtss_ipv6_addr_is_zero(&ipa6)) {
                    rc = TRUE;
                } else {
                    if (ipa6.addr[0] == 0xfe && (ipa6.addr[1] >> 6) == 0x2) {
                        if (vtss_dns_mgmt_support_multicast() &&
                            vtss_ifindex_decompose(conf->static_ifindex, &ife) == VTSS_RC_OK &&
                            ife.iftype == VTSS_IFINDEX_TYPE_VLAN) {
                            if ((chk_vid = (mesa_vid_t)ife.ordinal) != VTSS_VID_NULL) {
                                if (chk_vid >= VTSS_APPL_VLAN_ID_MIN &&
                                    chk_vid <= VTSS_APPL_VLAN_ID_MAX) {
                                    *vidx = chk_vid;
                                    rc = TRUE;
                                }
                            }
                        }
                    } else {
                        rc = TRUE;
                    }
                }
            }
        } else if (conf->static_ip_address.type == MESA_IP_TYPE_NONE) {
            rc = TRUE;
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY:
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
        if (!cap.support_dhcp4_config_server) {
            T_D("Not Support!");
            break;
        }

        if (conf->server_type == VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY ||
            conf->static_ifindex == VTSS_IFINDEX_NONE) {
            rc = TRUE;
            break;
        }

        if (vtss_ifindex_decompose(conf->static_ifindex, &ife) == VTSS_RC_OK &&
            ife.iftype == VTSS_IFINDEX_TYPE_VLAN) {
            if ((chk_vid = (mesa_vid_t)ife.ordinal) != VTSS_VID_NULL) {
                if (chk_vid >= VTSS_APPL_VLAN_ID_MIN &&
                    chk_vid <= VTSS_APPL_VLAN_ID_MAX) {
                    *vidx = chk_vid;
                    rc = TRUE;
                }
            } else {
                rc = TRUE;
            }
        }
        break;
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY:
    case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
        if (!cap.support_dhcp6_config_server) {
            T_D("Not Support!");
            break;
        }

        if (conf->server_type == VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY ||
            conf->static_ifindex == VTSS_IFINDEX_NONE) {
            rc = TRUE;
            break;
        }

        if (vtss_ifindex_decompose(conf->static_ifindex, &ife) == VTSS_RC_OK &&
            ife.iftype == VTSS_IFINDEX_TYPE_VLAN) {
            if ((chk_vid = (mesa_vid_t)ife.ordinal) != VTSS_VID_NULL) {
                if (chk_vid >= VTSS_APPL_VLAN_ID_MIN &&
                    chk_vid <= VTSS_APPL_VLAN_ID_MAX) {
                    *vidx = chk_vid;
                    rc = TRUE;
                }
            } else {
                rc = TRUE;
            }
        }
        break;
    default:
        T_D("Skip checking");
        rc = TRUE;
        break;
    }

    T_D("CHK-Done(%s)", rc ? "OK" : "NG");
    return rc;
}

/**
 * \brief Get the capabilities of DNS.
 *
 * \param cap       [OUT]   The capability properties of the DNS module.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_capabilities_get(
    vtss_appl_dns_capabilities_t        *const cap
)
{
    T_D("enter");

    if (!cap) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(cap, 0x0, sizeof(vtss_appl_dns_capabilities_t));
    cap->support_dhcp4_config_server = TRUE;
    cap->support_dhcp6_config_server = vtss_ip_hasdhcpv6();
    cap->support_default_domain_name = TRUE;
    if (cap->support_default_domain_name) {
        cap->support_dhcp4_domain_name = TRUE;
        cap->support_dhcp6_domain_name = vtss_ip_hasdhcpv6();
    }
    cap->support_mcast_anycast_ll_dns = VTSS_DNS_SUPPORT_MULTICAST;
    cap->ns_cnt_max = DNS_MAX_SRV_CNT;

    T_D("exit");
    return VTSS_RC_OK;
}

/**
 * \brief Get DNS proxy default configuration.
 *
 * Get default configuration of the DNS proxy settings.
 *
 * \param entry     [OUT]   The default configuration of DNS proxy settings.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_proxy_config_default(
    vtss_appl_dns_proxy_conf_t          *const entry
)
{
    T_D("enter");

    if (!entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(entry, 0x0, sizeof(vtss_appl_dns_proxy_conf_t));
    entry->proxy_admin_state = VTSS_DNS_PROXY_DEF_STATE;

    T_D("exit");
    return VTSS_RC_OK;
}

/**
 * \brief Get DNS proxy configuration.
 *
 * To read current DNS proxy settings.
 *
 * \param entry     [OUT]   The current DNS proxy configuration data.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_proxy_config_get(
    vtss_appl_dns_proxy_conf_t          *const entry
)
{
    mesa_rc rc;
    BOOL    dns_proxy;

    T_D("enter");

    if (!entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    if ((rc = vtss_dns_mgmt_get_proxy_status(&dns_proxy)) == VTSS_RC_OK) {
        T_D("Get DNS-Proxy(%s)", dns_proxy ? "Enabled" : "Disabled");
        entry->proxy_admin_state = dns_proxy;
    }

    T_D("exit(%s)", error_txt(rc));
    return rc;
}

/**
 * \brief Set DNS proxy configuration.
 *
 * To modify current DNS proxy settings.
 *
 * \param entry     [IN]    The revised DNS proxy configuration data.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_proxy_config_set(
    const vtss_appl_dns_proxy_conf_t    *const entry
)
{
    mesa_rc rc;
    BOOL    dns_proxy;

    T_D("enter");

    if (!entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    if ((rc = vtss_dns_mgmt_get_proxy_status(&dns_proxy)) == VTSS_RC_OK) {
        dns_proxy = entry->proxy_admin_state;
        rc = vtss_dns_mgmt_set_proxy_status(&dns_proxy);
    }

    T_D("exit(%s)", error_txt(rc));
    return rc;
}

/**
 * \brief Get DNS default domain name default configuration.
 *
 * Get default configuration of the DNS default domain name settings.
 *
 * \param entry     [OUT]   The default configuration of DNS default domain name settings.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_domain_name_config_default(
    vtss_appl_dns_name_conf_t           *const entry
)
{
    T_D("enter");

    if (!entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    memset(entry, 0x0, sizeof(vtss_appl_dns_name_conf_t));
    entry->domainname_type = VTSS_DNS_DOMAINAME_DEF_TYPE;
    entry->dhcp_ifindex = VTSS_IFINDEX_VLAN_OFFSET;

    T_D("exit");
    return VTSS_RC_OK;
}

/**
 * \brief Get DNS default domain name configuration.
 *
 * To read current DNS default domain name settings.
 *
 * \param entry     [OUT]   The current DNS default domain name configuration data.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_domain_name_config_get(
    vtss_appl_dns_name_conf_t           *const entry
)
{
    mesa_rc                     rc;
    vtss_dns_domainname_conf_t  my_dn;
    vtss_appl_dns_config_type_t dnt;
    vtss_ifindex_t              dnv;

    T_D("enter");

    if (!entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    if ((rc = vtss_dns_mgmt_default_domainname_get(&my_dn)) == VTSS_RC_OK) {
        dnv = VTSS_IFINDEX_VLAN_OFFSET;
        dnt = my_dn.domain_name_type;
        if (dnt == VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN ||
            dnt == VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN) {
            rc = vtss_ifindex_from_vlan(my_dn.domain_name_vlan, &dnv);
        }

        if (rc == VTSS_RC_OK) {
            T_D("Get domain_name_type(%d)", dnt);
            memset(entry, 0x0, sizeof(vtss_appl_dns_name_conf_t));
            entry->domainname_type = dnt;
            switch ( dnt ) {
            case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
                if (strlen(my_dn.domain_name_char) > 0) {
                    strncpy(entry->static_domain_name, my_dn.domain_name_char, strlen(my_dn.domain_name_char));
                }
                break;
            case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
            case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
                entry->dhcp_ifindex = dnv;
                break;
            default:
                break;
            }
        }
    }

    T_D("exit(%s)", error_txt(rc));
    return rc;
}

/**
 * \brief Set DNS default domain name configuration.
 *
 * To modify current DNS default domain name settings.
 *
 * \param entry     [IN]    The revised DNS default domain name configuration data.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_domain_name_config_set(
    const vtss_appl_dns_name_conf_t     *const entry
)
{
    mesa_rc                     rc;
    mesa_vid_t                  vidx;
    vtss_dns_domainname_conf_t  my_dn;

    T_D("enter");

    vidx = VTSS_VID_NULL;
    if (!DNS_domainname_config_sanity(entry, &vidx)) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    if ((rc = vtss_dns_mgmt_default_domainname_get(&my_dn)) == VTSS_RC_OK) {
        memset(&my_dn, 0x0, sizeof(vtss_dns_domainname_conf_t));
        my_dn.domain_name_type = entry->domainname_type;
        my_dn.domain_name_vlan = vidx;
        if (my_dn.domain_name_type == VTSS_APPL_DNS_CONFIG_TYPE_STATIC &&
            strlen(entry->static_domain_name) > 0) {
            strncpy(my_dn.domain_name_char, entry->static_domain_name, strlen(entry->static_domain_name));
        }

        rc = vtss_dns_mgmt_default_domainname_set(&my_dn);
    }

    T_D("exit(%s)", error_txt(rc));
    return rc;
}

/**
 * \brief Iterator for retrieving DNS server table index.
 *
 * Retrieve the 'next' configuration index of the DNS server table according to the given 'prev'.
 * Table index also represents the precedence in selecting target DNS server: less index value means
 * higher priority in round-robin selection.
 * Only one server is working at a time, that is when the chosen server is active, system marks the
 * designated server as target and stops selection.
 * When the active server becomes inactive, system starts another round of selection starting from
 * the next available server setting.
 * At maximum four server settings could be configured.
 *
 * \param prev      [IN]    Porinter of precedence index to be used for index determination.
 *
 * \param next      [OUT]   The next index should be used for the table entry.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Error code.      VTSS_RC_OK for success operation and the value in 'next' is valid,
 *                          other error code means that no "next" index and its corresponding
 *                          entry exists, and the end has been reached.
 */
mesa_rc vtss_appl_dns_server_config_itr(
    const u32                           *const prev,
    u32                                 *const next
)
{
    if (!next) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    return vtss::expose::snmp::IteratorComposeStaticRange<u32, DNS_DEF_SRV_IDX, DNS_MAX_SRV_CNT - 1>(prev, next);
}

/**
 * \brief Get DNS server default configuration.
 *
 * Get default configuration of the DNS server settings.
 *
 * \param priority  [OUT]   The default precedence in DNS server configurations.
 * \param entry     [OUT]   The default DNS server configuration settings.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_server_config_default(
    u32                                 *const priority,
    vtss_appl_dns_server_conf_t         *const entry
)
{
    T_D("enter");

    if (!priority || !entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    *priority = DNS_DEF_SRV_IDX;
    memset(entry, 0x0, sizeof(vtss_appl_dns_server_conf_t));
    entry->server_type = VTSS_DNS_SERVER_DEF_TYPE;
    entry->static_ifindex = VTSS_IFINDEX_VLAN_OFFSET;

    T_D("exit");
    return VTSS_RC_OK;
}

/**
 * \brief Get specific index DNS server configuration.
 *
 * Get DNS server configuration of the specific index.
 *
 * \param priority  [IN]    (key) Index - the precedence in DNS server configurations.
 *
 * \param entry     [OUT]   The current DNS server configuration with the specific index.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_server_config_get(
    const u32                           *const priority,
    vtss_appl_dns_server_conf_t         *const entry
)
{
    mesa_rc                     rc;
    vtss_dns_srv_conf_t         dns_srv;
    vtss_appl_dns_config_type_t dnt;
    vtss_ifindex_t              dnv;
    u8                          idx;

    T_D("enter");

    if (!priority || !entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    idx = (u8) (*priority & 0xFF);
    if ((rc = vtss_dns_mgmt_get_server(idx, &dns_srv)) == VTSS_RC_OK) {
        dnv = VTSS_IFINDEX_VLAN_OFFSET;
        dnt = VTSS_DNS_TYPE_GET(&dns_srv);
        if (dnt == VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN ||
            dnt == VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN) {
            rc = vtss_ifindex_from_vlan(VTSS_DNS_VLAN_GET(&dns_srv), &dnv);
        }

        if (rc == VTSS_RC_OK) {
            T_D("Get server_type: %d", dnt);
            memset(entry, 0x0, sizeof(vtss_appl_dns_server_conf_t));
            entry->server_type = dnt;
            switch ( dnt ) {
            case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
                entry->static_ip_address.type = VTSS_DNS_ADDR_TYPE_GET(&dns_srv);
                if (entry->static_ip_address.type == MESA_IP_TYPE_IPV4) {
                    entry->static_ip_address.addr.ipv4 = VTSS_DNS_ADDR_IPA4_GET(&dns_srv);
                } else if (entry->static_ip_address.type == MESA_IP_TYPE_IPV6) {
                    entry->static_ip_address.addr.ipv6 = VTSS_DNS_ADDR_IPA6_GET(&dns_srv);
                }
                break;
            case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
            case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
                entry->static_ifindex = dnv;
                break;
            default:
                break;
            }
        }
    }

    T_D("exit(%s)", error_txt(rc));
    return rc;
}

/**
 * \brief Set/Update specific index DNS server configuration.
 *
 * Modify DNS server configuration of the specific index.
 *
 * \param priority  [IN]    (key) Index - the precedence in DNS server configurations.
 *
 * \param entry     [OUT]   The revised DNS server configuration with the specific index.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_server_config_set(
    const u32                           *const priority,
    const vtss_appl_dns_server_conf_t   *const entry
)
{
    mesa_rc                     rc;
    mesa_vid_t                  vidx;
    vtss_dns_srv_conf_t         dns_srv;
    vtss_appl_dns_config_type_t dnt;
    u8                          idx;

    T_D("enter");

    vidx = VTSS_VID_NULL;
    if (!priority || !DNS_server_config_sanity(entry, &vidx)) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    idx = (u8) (*priority & 0xFF);
    if ((rc = vtss_dns_mgmt_get_server(idx, &dns_srv)) == VTSS_RC_OK) {
        T_D("Set server_type as %d", entry->server_type);
        memset(&dns_srv, 0x0, sizeof(vtss_dns_srv_conf_t));
        dnt = entry->server_type;
        switch ( entry->server_type ) {
        case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
            if (entry->static_ip_address.type == MESA_IP_TYPE_IPV4) {
                VTSS_DNS_ADDR4_SET(&dns_srv, entry->static_ip_address.addr.ipv4);
            } else if (entry->static_ip_address.type == MESA_IP_TYPE_IPV6) {
                VTSS_DNS_ADDR6_SET(&dns_srv, &entry->static_ip_address.addr.ipv6);
            }
            break;
        case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY:
        case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY:
        case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
        case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
            VTSS_DNS_VLAN_SET(&dns_srv, vidx);
            break;
        default:
            dnt = VTSS_APPL_DNS_CONFIG_TYPE_NONE;
            break;
        }
        VTSS_DNS_TYPE_SET(&dns_srv, dnt);

        rc = vtss_dns_mgmt_set_server(idx, &dns_srv);
    }

    T_D("exit(%s)", error_txt(rc));
    return rc;
}

/**
 * \brief Retrieve active default domain name.
 *
 * To read system default domain name; empty name presents default domain name is not available.
 *
 * \param entry     [OUT]   The current DNS default domain name.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_domainname_status_get(
    vtss_appl_dns_domainname_status_t   *const entry
)
{
    mesa_rc                     rc;
    vtss_dns_domainname_conf_t  my_dn;

    T_D("enter");

    if (!entry) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    if ((rc = vtss_dns_mgmt_default_domainname_get(&my_dn)) == VTSS_RC_OK) {
        memset(entry->default_domain_name, 0x0, sizeof(entry->default_domain_name));
        if (strlen(my_dn.domain_name_char) > 0) {
            strncpy(entry->default_domain_name, my_dn.domain_name_char, strlen(my_dn.domain_name_char));
        }
    }

    T_D("exit(%s)", error_txt(rc));
    return rc;
}

/**
 * \brief Iterator for retrieving DNS server table index.
 *
 * Retrieve the 'next' configuration index of the DNS server table according to the given 'prev'.
 *
 * \param prev      [IN]    Porinter of precedence index to be used for index determination.
 *
 * \param next      [OUT]   The next index should be used for the table entry.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Error code.      VTSS_RC_OK for success operation and the value in 'next' is valid,
 *                          other error code means that no "next" index and its corresponding
 *                          entry exists, and the end has been reached.
 */
mesa_rc vtss_appl_dns_server_status_itr(
    const u32                           *const prev,
    u32                                 *const next
)
{
    return vtss_appl_dns_server_config_itr(prev, next);
}

/**
 * \brief Retrieve configured DNS server information.
 *
 * To read the information of configured DNS server.
 *
 * \param priority  [IN]    (key) Index - the precedence in DNS server configurations.
 *
 * \param entry     [OUT]   The current DNS server information with the specific index.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_server_status_get(
    const u32                           *const priority,
    vtss_appl_dns_server_status_t       *const entry
)
{
    u8                  idx;
    vtss_dns_srv_info_t *info, dns_info;
    vtss_ifindex_t      dnv;
    mesa_rc             rc;

    T_D("enter");

    if (!priority || !entry || *priority >= DNS_MAX_SRV_CNT) {
        T_D("Invalid Input!");
        return VTSS_RC_ERROR;
    }

    idx = (u8) (*priority & 0xFF);
    DNS_CRIT_ENTER();
    info = &vtss_dns_database.info.dns_info[idx];
    dns_info.srv_type = info->srv_type;
    dns_info.srv_addr = info->srv_addr;
    dns_info.srv_vlan = info->srv_vlan;
    DNS_CRIT_EXIT();

    rc = VTSS_RC_OK;
    dnv = VTSS_IFINDEX_VLAN_OFFSET;
    if (dns_info.srv_vlan != VTSS_VID_NULL) {
        rc = vtss_ifindex_from_vlan(dns_info.srv_vlan, &dnv);
    }

    if (rc == VTSS_RC_OK) {
        entry->configured_type = dns_info.srv_type;
        entry->ip_address = dns_info.srv_addr;
        entry->reference_ifindex = dnv;
    }

    T_D("exit(%s)", error_txt(rc));
    return rc;
}

