/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "netdb.h"
#include <arpa/inet.h>

#include "vtss_dns.h"
#include "vtss_dns_oswrapper.h"

#define VTSS_TRACE_MODULE_ID    VTSS_MODULE_ID_IP_DNS

void vtss_dns_init(void)
{
    vtss_dns_lib_initialize();
}

BOOL vtss_dns_ipa_valid(const mesa_ip_addr_t *const ipa)
{
    u8  idx;

    if (!ipa) {
        return FALSE;
    }

    switch ( ipa->type ) {
    case MESA_IP_TYPE_NONE:
        for (idx = 0; idx < 16; idx++) {
            if (ipa->addr.ipv6.addr[idx]) {
                return FALSE;
            }
        }

        break;
    case MESA_IP_TYPE_IPV4:
        if (ipa->addr.ipv4 == 0) {
            break;
        }

        idx = (u8)((ipa->addr.ipv4 >> 24) & 0xFF);
        if ((idx == 127) ||
            ((idx > 223) && (idx < 240))) {
            return FALSE;
        }

        break;
    case MESA_IP_TYPE_IPV6:
        idx = ipa->addr.ipv6.addr[0];
        if (idx == 0xFF) {
            return FALSE;
        }

        for (idx = 0; idx < 16; idx++) {
            if (ipa->addr.ipv6.addr[idx]) {
                break;
            } else {
                if ((idx == 15) && (ipa->addr.ipv6.addr[idx] == 1)) {
                    return FALSE;
                }
            }
        }

        break;
    default:
        return FALSE;
    }

    return TRUE;
}

mesa_rc vtss_dns_tick_cache(vtss_tick_count_t ts)
{
    u32 tick = (u32)(ts & 0xFFFFFFFF);

    return vtss_dns_lib_tick_cache(tick);
}

u32 vtss_dns_frame_parse_question_section(const u8 *const hdr, u8 *const qname, u16 *const qtype, u16 *const qclass)
{
    u8  *ptr, *dnsq, length;
    u32 offset = 0;

    if (!hdr || !qname || !qtype || !qclass) {
        return offset;
    }

    dnsq = (u8 *)(hdr + sizeof(dns_dns_hdr));
    ptr = (u8 *)(dnsq + offset);
    while (*ptr != 0x0) {
        length = *ptr;
        memcpy((u8 *)(qname + offset), (u8 *)(dnsq + offset + 1), length);
        qname[offset + length] = '.';
        offset += (length + 1);
        ptr = (u8 *)(dnsq + offset);
    }
    if (qname[offset - 1] == '.') {
        qname[offset - 1] = 0x0;
    } else {
        qname[offset] = 0x0;
    }
    offset++;

    ptr = (u8 *)(dnsq + offset);
    memcpy((u8 *)qtype, ptr, sizeof(u16));
    *qtype = ntohs(*qtype);
    offset += sizeof(u16);

    ptr = (u8 *)(dnsq + offset);
    memcpy((u8 *)qclass, ptr, sizeof(u16));
    *qclass = ntohs(*qclass);
    offset += sizeof(u16);

    return offset;
}

int vtss_dns_frame_build_question_name(u8 *const ptr, const char *const hostname)
{
    return vtss_dns_lib_build_qname(ptr, hostname);
}

mesa_rc vtss_dns_current_server_get(mesa_ip_addr_t *const srv, u8 *const idx, i32 *const ecnt)
{
    return vtss_dns_lib_current_server_get(srv, idx, ecnt);
}

mesa_rc vtss_dns_current_server_set(mesa_vid_t vidx, const mesa_ip_addr_t *const srv, u8 idx)
{
    return vtss_dns_lib_current_server_set(vidx, srv, idx);
}

mesa_rc vtss_dns_current_server_rst(u8 idx)
{
    return vtss_dns_lib_current_server_rst(idx);
}

mesa_rc vtss_dns_default_domainname_get(char *const ns)
{
    return vtss_dns_lib_default_domainname_get(ns);
}

mesa_rc vtss_dns_default_domainname_set(const char *const ns)
{
    return vtss_dns_lib_default_domainname_set(ns);
}
