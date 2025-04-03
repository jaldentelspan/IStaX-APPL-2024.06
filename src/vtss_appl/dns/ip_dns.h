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

/* debug msg can be enabled by cmd "debug trace module level qos default debug" */

#ifndef _VTSS_IP_DNS_H_
#define _VTSS_IP_DNS_H_

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include "vtss_dns.h"

#define VTSS_TRACE_GRP_DEFAULT  0

#include <vtss_trace_api.h>

#define DNS_EVENT_ANY           0xFFFFFFFF  /* Any possible bit... */
#define DNS_EVENT_WAKEUP        0x00000001
#define DNS_EVENT_PROBE         0x00000010
#define DNS_EVENT_QRTV          0x00000100
#define DNS_EVENT_RESUME        0x00100000
#define DNS_EVENT_SUSPEND       0x01000000
#define DNS_EVENT_EXIT          0x10000000

#define DNS_BIP_FIFO_GROW       3
#define DNS_BIP_FIFO_SZ         (0x10 * DNS_BIP_FIFO_GROW)
#define DNS_BIP_BUF_SZ_B        (DNS_BIP_FIFO_SZ * sizeof(dns_proxy_query_item_t))

/* DNS TYPEs */
#define DNS_TYPE_A              1           /* Host address */
#define DNS_TYPE_NS             2           /* Authoritative name server */
#define DNS_TYPE_CNAME          5           /* Canonical name for an alias */
#define DNS_TYPE_PTR            12          /* Domain name pointer */
#define DNS_TYPE_AAAA           28          /* IPv6 host address */

/* DNS CLASSs */
#define DNS_CLASS_IN            1           /* Internet */

typedef enum {
    DNS_TXT_CASE_CAPITAL = 0,
    DNS_TXT_CASE_LOWER,
    DNS_TXT_CASE_UPPER,
    DNS_TXT_CASE_FREE
} dns_text_cap_t;

/* ================================================================= *
 *  DNS global structure
 * ================================================================= */

/* DNS global structure */
typedef struct {
    critd_t                     dns_crit;   /* Critical section for DNS */
    critd_t                     dns_pkt;    /* Critical section for PKT */
    void                        *filter_id; /* Packet filter ID */

    dns_info_t                  info;       /* Module information */
    dns_conf_t                  conf;       /* Module configuration */
} dns_global_record_t;

typedef struct dns_proxy_query_item {
    mesa_ipv4_t                 querier_ip;
    mesa_ipv4_t                 querier_dns_dip;
    u16                         querier_udp_port;
    u16                         querier_dns_transaction_id;
    u16                         querier_question_type;
    u16                         querier_question_class;
    char                        querier_question[256];
    u32                         querier_incoming_port;
    mesa_vid_t                  querier_vid;
    u8                          querier_mac[6];
} dns_proxy_query_item_t;

#endif /* _VTSS_IP_DNS_H_ */

