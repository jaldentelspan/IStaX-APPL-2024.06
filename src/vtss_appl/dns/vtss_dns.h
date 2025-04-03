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
 * This file contains the API definitions used between the DNS protocol
 * module and the operating environment.
 */
#ifndef _VTSS_DNS_H_
#define _VTSS_DNS_H_

#include "vlan_api.h"
#include "vtss/appl/dns.h"
#include "resolv.h" /* FOR MAXNS */

#define DNS_MAX_SRV_CNT             MAXNS /* glibc only reads this many entries from etc/resolv.conf */
#define DNS_DEF_SRV_IDX             0
#define DNS_MAX_SRV_ERR             5
#define DNS_MAX_NAME_LEN            VTSS_APPL_DNS_MAX_STRING_LEN

typedef struct {
    vtss_appl_dns_config_type_t     dns_type;

    mesa_ip_addr_t                  static_conf_addr;
    mesa_vid_t                      dynamic_conf_vlan;
    mesa_vid_t                      egress_vlan;
} vtss_dns_srv_conf_t;

typedef struct {
    vtss_appl_dns_config_type_t     srv_type;
    mesa_ip_addr_t                  srv_addr;
    mesa_vid_t                      srv_vlan;
} vtss_dns_srv_info_t;

typedef struct {
    vtss_appl_dns_config_type_t     domain_name_type;
    char                            domain_name_char[DNS_MAX_NAME_LEN + 1];
    mesa_vid_t                      domain_name_vlan;
} vtss_dns_domainname_conf_t;

#define VTSS_DNS_INFO_CONF_TYPE(x)  ((x) ? (x)->dns_type : VTSS_APPL_DNS_CONFIG_TYPE_NONE)

#define VTSS_DNS_INFO_CONF_ADDR4(x)                                             \
    ((VTSS_DNS_INFO_CONF_TYPE((x)) == VTSS_APPL_DNS_CONFIG_TYPE_STATIC)         \
             ? (((x)->static_conf_addr.type == MESA_IP_TYPE_IPV4)               \
                        ? (x)->static_conf_addr.addr.ipv4                       \
                        : 0)                                                    \
             : 0)

#define VTSS_DNS_INFO_CONF_ADDR6(x)                                             \
    ((VTSS_DNS_INFO_CONF_TYPE((x)) == VTSS_APPL_DNS_CONFIG_TYPE_STATIC)         \
             ? (((x)->static_conf_addr.type == MESA_IP_TYPE_IPV6)               \
                        ? &((x)->static_conf_addr.addr.ipv6)                    \
                        : NULL)                                                 \
             : NULL)

#define VTSS_DNS_INFO_CONF_VLAN(x)                                              \
    (((VTSS_DNS_INFO_CONF_TYPE((x)) == VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY) ||  \
      (VTSS_DNS_INFO_CONF_TYPE((x)) == VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN) || \
      (VTSS_DNS_INFO_CONF_TYPE((x)) == VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN) || \
      (VTSS_DNS_INFO_CONF_TYPE((x)) == VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY))    \
             ? (x)->dynamic_conf_vlan                                           \
             : VTSS_VID_NULL)

#define VTSS_DNS_ADDR_VALID(x)      (vtss_dns_ipa_valid(&((x)->static_conf_addr)))

#define VTSS_DNS_VLAN_VALID(x)                                                  \
    ((x)->dynamic_conf_vlan                                                     \
             ? ((x)->dynamic_conf_vlan >= VTSS_APPL_VLAN_ID_MIN) &&             \
                       (x)->dynamic_conf_vlan <= VTSS_APPL_VLAN_ID_MAX          \
             : TRUE)

#define DNS_UNSPECIFIED_ADDRESS_INIT                                            \
    {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                          \
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}

/* DNS General configuration */
typedef struct {
    vtss_dns_srv_conf_t             dns_conf[DNS_MAX_SRV_CNT];

    /* status of DNS Proxy (1:enable 0:disable) */
    BOOL                            dns_proxy_status;

    /* default domain name */
    vtss_dns_domainname_conf_t      default_domain_name;

    /* domain name list */
//    vtss_dns_domainname_conf_t      domain_name_list[DNS_MAX_NAME_LIST];
} dns_conf_t;

/* DNS Information Cache */
typedef struct {
    vtss_dns_srv_info_t             dns_info[DNS_MAX_SRV_CNT];
} dns_info_t;

/**
 * Representation of a 48-bit Ethernet address.
 */
typedef struct {
    uchar           addr[6];
} __attribute__((packed)) dns_eth_addr;

/**
 * The Ethernet header.
 */
typedef struct {
    dns_eth_addr    dest;
    dns_eth_addr    src;
    ushort          type;
} __attribute__((packed)) dns_eth_hdr;

/* The IP headers. */
typedef struct {
    /* IP header. */
    uchar           vhl;
    uchar           tos;
    ushort          len;
    uchar           ipid[2];
    uchar           ipoffset[2];
    uchar           ttl;
    uchar           proto;
    ushort          ipchksum;
    uint32_t        srcipaddr,
                    destipaddr;
} __attribute__((packed)) dns_ip_hdr;

/* The UDP headers. */
typedef struct {
    ushort          sport;      /* source port */
    ushort          dport;      /* destination port */
    ushort          ulen;       /* udp length */
    ushort          csum;       /* udp checksum */
}  __attribute__((packed)) dns_udp_hdr;

typedef struct {
    ushort          id;         /* query identification number */
    ushort          flags;
    ushort          qdcount;    /* number of question entries */
    ushort          ancount;    /* number of answer entries */
    ushort          nscount;    /* number of authority entries */
    ushort          arcount;    /* number of resource entries */
} __attribute__((packed)) dns_dns_hdr;

typedef struct {
    ushort          domain;
    ushort          rr_type;    /* Type of resourse */
    ushort          cls;        /* Class of resource */
    uint32_t        ttl;        /* Time to live of this record */
    ushort          rdlength;   /* Length of data to follow */
    uchar           rdata [4];  /* Resource DATA */
} __attribute__((packed)) dns_dns_answer;

void vtss_dns_init(void);

BOOL vtss_dns_ipa_valid(const mesa_ip_addr_t *const ipa);

u32 vtss_dns_frame_parse_question_section(const u8 *const hdr, u8 *const qname, u16 *const qtype, u16 *const qclass);

int vtss_dns_frame_build_question_name(u8 *const ptr, const char *const hostname);

mesa_rc vtss_dns_tick_cache(vtss_tick_count_t ts);

mesa_rc vtss_dns_current_server_get(mesa_ip_addr_t *const srv, u8 *const idx, i32 *const ecnt);

mesa_rc vtss_dns_current_server_set(mesa_vid_t vidx, const mesa_ip_addr_t *const srv, u8 idx);

mesa_rc vtss_dns_current_server_rst(u8 idx);

mesa_rc vtss_dns_default_domainname_get(char *const ns);

mesa_rc vtss_dns_default_domainname_set(const char *const ns);
#endif
