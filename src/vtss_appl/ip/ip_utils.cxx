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

#include "dhcp_client_api.h"
#include "conf_api.h"    /* For conf_mgmt_mac_addr_get() */
#include "ip_utils.hxx"
#include "ip_api.h"
#include "ip_trace.h"
#include "mac_utils.hxx" /* For mesa_mac_t::operator<() */
#include "misc_api.h"
#include "frr_daemon.hxx"
#ifdef VTSS_SW_OPTION_CPUPORT
#include "cpuport_api.hxx"
#endif /* VTSS_SW_OPTION_CPUPORT */
#include "vtss/appl/vlan.h"
#include "vtss/appl/ospf.h"
#include "vlan_api.h"
#include "vtss/appl/interface.h"
#include "vtss/basics/memory.hxx"
#include "vtss/basics/map.hxx"
#include "vtss/basics/trace.hxx"

using namespace vtss;

mesa_ipv4_t vtss_ipv4_blackhole_route = 0xffffffff;
mesa_ipv6_t vtss_ipv6_blackhole_route = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_IP

#define PRINTF(...)                                         \
    if (size - s > 0) {                                     \
        int res = snprintf(buf + s, size - s, __VA_ARGS__); \
        if (res > 0) {                                      \
            s += res;                                       \
        }                                                   \
    }

#define PRINTFUNC(F, ...)                       \
    if (size - s > 0) {                         \
        s += F(buf + s, size - s, __VA_ARGS__); \
    }

/******************************************************************************/
// IP_UTILS_chksum_fold()
// This one folds the 32-bit checksum into a 16-bit checksum.
/******************************************************************************/
static uint16_t IP_UTILS_chksum_fold(uint32_t sum)
{
    // Fold the sum to 16 bits
    sum = (sum >> 16) + (sum & 0xffff);

    // This could give rise to yet another carry, that we need to take into
    // account:
    sum += sum >> 16;

    // One's complement
    sum = ~sum & 0xffff;

    if (sum == 0) {
        sum = 0xffff;
    }

    return sum;
}

/******************************************************************************/
// IP_UTILS_hton_08()
/******************************************************************************/
static void IP_UTILS_hton_08(uint8_t **p, uint8_t val)
{
    uint8_t *q = *p;

    q[0] = val;
    *p += 1;
}

/******************************************************************************/
// IP_UTILS_hton_16()
/******************************************************************************/
static void IP_UTILS_hton_16(uint8_t **p, uint16_t val)
{
    uint8_t *q = *p;

    q[0] = val >> 8;
    q[1] = val >> 0;
    *p += 2;
}

/****************************************************************************/
// IP_UTILS_hton_24()
/****************************************************************************/
static void IP_UTILS_hton_24(uint8_t **p, uint32_t val)
{
    uint8_t *q = *p;

    q[0] = val >> 16;
    q[1] = val >>  8;
    q[2] = val >>  0;
    *p += 3;
}

/******************************************************************************/
// IP_UTILS_hton_32()
/******************************************************************************/
static void IP_UTILS_hton_32(uint8_t **p, uint32_t val)
{
    uint8_t *q = *p;

    q[0] = val >> 24;
    q[1] = val >> 16;
    q[2] = val >>  8;
    q[3] = val >>  0;
    *p += 4;
}

/******************************************************************************/
// IP_UTILS_hton_cp()
// "cp" stands for "copy".
/******************************************************************************/
static void IP_UTILS_hton_cp(uint8_t **p, const uint8_t *src, size_t len)
{
    memcpy(*p, src, len);
    *p += len;
}

/******************************************************************************/
// IP_UTILS_checksum_no_fold()
// This one simply sums up what it gets in frm and adds to the sum argument.
/******************************************************************************/
static void IP_UTILS_checksum_no_fold(const uint8_t *frm, uint16_t len, uint32_t &sum, vtss_ip_checksum_type_t checksum_type)
{
    const uint8_t *ptr = frm;
    int32_t       i;

    for (i = 0; i < len - 1; i += 2) {
        sum += (ptr[0] << 8) | ptr[1];
        ptr += 2;
    }

    // Take care of an odd number of bytes by padding with a zero
    if (len % 2) {
        switch (checksum_type) {
        case VTSS_IP_CHECKSUM_TYPE_NORMAL:
            // IPv4 uses this one. Makes sense to use the last byte as the
            // MSByte in the computation.
            sum += (ptr[0] << 8) | 0;
            break;

        case VTSS_IP_CHECKSUM_TYPE_UDLD:
            // UDLD uses this form, which doesn't really makes sense.
            // See RFC5171.
            sum += ptr[0];
            break;

        case VTSS_IP_CHECKSUM_TYPE_CDP:
            // CDP uses this form, which makes even less sense.
            // See dissector for CDP in Wireshark.
            if (ptr[0] >= 0x80) {
                sum -= 256 - ptr[0];
            } else {
                sum += ptr[0];
            }

            break;

        default:
            T_E("Unknown checksum type (%d)", checksum_type);
            break;
        }
    }
}

/******************************************************************************/
// IP_UTILS_checksum()
/******************************************************************************/
static uint16_t IP_UTILS_checksum(const uint8_t *frm, uint16_t len, uint32_t start_sum, vtss_ip_checksum_type_t checksum_type = VTSS_IP_CHECKSUM_TYPE_NORMAL)
{
    IP_UTILS_checksum_no_fold(frm, len, start_sum, checksum_type);
    return IP_UTILS_chksum_fold(start_sum);
}

/******************************************************************************/
// vtss_ip_checksum()
/******************************************************************************/
uint16_t vtss_ip_checksum(const uint8_t *frm, uint16_t len, vtss_ip_checksum_type_t checksum_type)
{
    return IP_UTILS_checksum(frm, len, 0, checksum_type);
}

/******************************************************************************/
// vtss_ip_pseudo_header_checksum()
/******************************************************************************/
uint16_t vtss_ip_pseudo_header_checksum(const uint8_t *frm, uint16_t len, const mesa_ip_addr_t &sip, const mesa_ip_addr_t &dip, uint8_t protocol)
{
    uint8_t  *p;
    uint8_t  pseudo_hdr[40]; // IPv4: 12, IPv6: 40 bytes.
    uint16_t pseudo_hdr_len;
    uint32_t sum;

    if (sip.type != dip.type) {
        T_E("sip (type = %u) and dip (type = %u) must be of the same type", sip.type, dip.type);
        return 0;
    }

    // Fill in pseudo-header.
    p = pseudo_hdr;

    if (sip.type == MESA_IP_TYPE_IPV4) {
        IP_UTILS_hton_32(&p, sip.addr.ipv4);
        IP_UTILS_hton_32(&p, dip.addr.ipv4);
        IP_UTILS_hton_08(&p, 0);
        IP_UTILS_hton_08(&p, protocol);
        IP_UTILS_hton_16(&p, len);
        pseudo_hdr_len = 12;
    } else if (sip.type == MESA_IP_TYPE_IPV6) {
        IP_UTILS_hton_cp(&p, sip.addr.ipv6.addr, sizeof(sip.addr.ipv6.addr));
        IP_UTILS_hton_cp(&p, dip.addr.ipv6.addr, sizeof(dip.addr.ipv6.addr));
        IP_UTILS_hton_32(&p, len);
        IP_UTILS_hton_24(&p, 0);
        IP_UTILS_hton_08(&p, protocol);
        pseudo_hdr_len = 40;
    } else {
        T_E("Unknown IP type (%u)", sip.type);
        return 0;
    }

    if (frm) {
        if (!len) {
            T_E("L4 data is specified, but the length of it is 0");
            return 0;
        }

        // Compute the checksum of the pseudo header
        sum = 0;
        IP_UTILS_checksum_no_fold(pseudo_hdr, pseudo_hdr_len, sum, VTSS_IP_CHECKSUM_TYPE_NORMAL);

        // And add the UDP/TCP data checksum and fold the result to 16 bits
        return IP_UTILS_checksum(frm, len, sum);
    }

    // User just wants the checksum of the pseudo header.
    return IP_UTILS_checksum(pseudo_hdr, pseudo_hdr_len, 0);
}

int vtss_ip_ifindex_to_txt(char *buf, int size, vtss_ifindex_t const i)
{
    int s = 0;
    vtss_ifindex_elm_t e;

    if (vtss_ifindex_decompose(i, &e) != VTSS_RC_OK) {
        PRINTF("IFINDEX %u", VTSS_IFINDEX_PRINTF_ARG(i));
        return s;
    }

    switch (e.iftype) {
    case VTSS_IFINDEX_TYPE_PORT:
        PRINTF("PORT %u", e.ordinal);
        return s;
    case VTSS_IFINDEX_TYPE_LLAG:
        PRINTF("LLAG %u", e.ordinal);
        return s;
    case VTSS_IFINDEX_TYPE_GLAG:
        PRINTF("GLAG %u", e.ordinal);
        return s;
    case VTSS_IFINDEX_TYPE_VLAN:
        PRINTF("VLAN %u", e.ordinal);
        return s;
    case VTSS_IFINDEX_TYPE_CPU:
        PRINTF("CPU %d/%d", e.usid, iport2uport(e.ordinal));
        return s;
    default:
        PRINTF("IFINDEX %u", VTSS_IFINDEX_PRINTF_ARG(i));
        return s;
    }
}

bool operator<(const mesa_routing_entry_t &a, const mesa_routing_entry_t &b)
{
#define CMP(A, B) \
    if ((A) != (B)) return (A) < (B);

#define CMP_AB(X) CMP(a.X, b.X)

#define CMP_AB_16(X) \
    CMP_AB(X[0]);    \
    CMP_AB(X[1]);    \
    CMP_AB(X[2]);    \
    CMP_AB(X[3]);    \
    CMP_AB(X[4]);    \
    CMP_AB(X[5]);    \
    CMP_AB(X[6]);    \
    CMP_AB(X[7]);    \
    CMP_AB(X[8]);    \
    CMP_AB(X[9]);    \
    CMP_AB(X[10]);   \
    CMP_AB(X[11]);   \
    CMP_AB(X[12]);   \
    CMP_AB(X[13]);   \
    CMP_AB(X[14]);   \
    CMP_AB(X[15]);

    CMP_AB(type);

    switch (a.type) {
    case MESA_ROUTING_ENTRY_TYPE_IPV4_UC:
        CMP_AB(route.ipv4_uc.network.address);
        CMP_AB(route.ipv4_uc.network.prefix_size);
        CMP_AB(route.ipv4_uc.destination);
        break;

    case MESA_ROUTING_ENTRY_TYPE_IPV6_UC:
        CMP_AB_16(route.ipv6_uc.network.address.addr);
        CMP_AB(route.ipv6_uc.network.prefix_size);
        CMP_AB_16(route.ipv6_uc.destination.addr);
        CMP_AB(vlan);
        break;
    case MESA_ROUTING_ENTRY_TYPE_IPV4_MC:
    default:
        return false;
    }

    return false;
#undef CMP_AB_16
#undef CMP_AB
#undef CMP
}

BOOL vtss_ipv4_addr_is_zero(const mesa_ipv4_t *addr)
{
    if (!addr) {
        return FALSE;
    }

    return *addr == 0;
}

BOOL vtss_ipv6_addr_is_zero(const mesa_ipv6_t *addr)
{
    static const mesa_ipv6_t zero_ipv6 = {};
    return memcmp(addr->addr, zero_ipv6.addr, sizeof(zero_ipv6.addr)) == 0;
}

// Pure function, no side effects
/* Returns TRUE if IPv4/v6 is a zero-address. FALSE otherwise. */
BOOL vtss_ip_addr_is_zero(const mesa_ip_addr_t *ip_addr)
{
    switch (ip_addr->type) {
    case MESA_IP_TYPE_IPV4:
        return vtss_ipv4_addr_is_zero(&ip_addr->addr.ipv4);
    case MESA_IP_TYPE_IPV6:
        return vtss_ipv6_addr_is_zero(&ip_addr->addr.ipv6);
    case MESA_IP_TYPE_NONE:
        return TRUE; /* Consider MESA_IP_TYPE_NONE as zero */
    }

    return FALSE; /* Undefined */
}

// Pure function, no side effects
BOOL vtss_ipv4_addr_is_unicast(const mesa_ipv4_t *addr)
{
    if (!addr) {
        return FALSE;
    }

    /* IN_CLASSD */
    if (((*addr >> 24) & 0xF0) == 0xE0) {
        return FALSE;
    }

    /* IN_CLASSE */
    if (((*addr >> 24) & 0xF0) == 0xF0) {
        return FALSE;
    }

    return TRUE;
}

// Pure function, no side effects
BOOL vtss_ipv4_addr_is_multicast(const mesa_ipv4_t *addr)
{
    if (!addr) {
        return FALSE;
    }

    /* IN_CLASSD */
    return (((*addr >> 24) & 0xF0) == 0xE0);
}

// Pure function, no side effects
BOOL vtss_ipv4_addr_is_loopback(const mesa_ipv4_t *addr)
{
    if (!addr) {
        return FALSE;
    }

    /* IN_CLASSD */
    return (((*addr >> 24) & 0xFF) == 0x7F);
}

// Pure function, no side effects
BOOL vtss_ipv6_addr_is_link_local(const mesa_ipv6_t *addr)
{
    if (addr->addr[0] == 0xfe && (addr->addr[1] >> 6) == 0x2) {
        return TRUE;
    }

    return FALSE;
}

// Pure function, no side effects
bool vtss_ipv4_addr_is_routable(const mesa_ipv4_t *addr)
{
    // We cannot route 169.254.0.0/16 (see RFC3927, section 7)
    if (((*addr >> 24) & 0xFF) != 169) {
        return true;
    }

    if (((*addr >> 16) & 0xFF) != 254) {
        return true;
    }

    return false;
}

// Pure function, no side effects
bool vtss_ipv6_addr_is_routable(const mesa_ipv6_t *addr)
{
    // We cannot route fe80::/10
    return !vtss_ipv6_addr_is_link_local(addr);
}

void vtss_ipv6_addr_link_local_get(mesa_ipv6_t *ipv6, const mesa_mac_t *mac)
{
    const mesa_ipv6_t link_local_skeleton = {0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x00, 0x00};

    mesa_mac_t    system_mac;
    const uint8_t *p;

    if (mac) {
        p = mac->addr;
    } else {
        (void)conf_mgmt_mac_addr_get(system_mac.addr, 0);
        p = system_mac.addr;
    }

    *ipv6 = link_local_skeleton;
    memcpy(&ipv6->addr[ 8], &p[0], 3);
    memcpy(&ipv6->addr[13], &p[3], 3);

    ipv6->addr[8] &= 0xFE; // G/I (Group/Individual) Bit
    ipv6->addr[8] |= 0x02; // U/L (Universal/Local) Bit
}

// Pure function, no side effects
BOOL vtss_ipv6_addr_is_multicast(const mesa_ipv6_t *addr)
{
    if (addr->addr[0] == 0xff) {
        return TRUE;
    }

    return FALSE;
}

// Pure function, no side effects
BOOL vtss_ipv6_addr_is_loopback(const mesa_ipv6_t *const addr)
{
    u8 idx;

    if (!addr) {
        return FALSE;
    }

    for (idx = 0; idx < 16; ++idx) {
        if (idx < 15) {
            if (addr->addr[idx]) {
                return FALSE;
            }
        } else {
            if (addr->addr[idx] != 0x1) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL _vtss_ipv6_addr_is_v4_form(const mesa_ipv6_t *const addr)
{
    u8 idx;

    if (!addr) {
        return FALSE;
    }

    for (idx = 0; idx < 16; ++idx) {
        if (idx < 10) {
            if (addr->addr[idx]) {
                return FALSE;
            }
        } else {
            if (idx < 12) {
                if (addr->addr[idx] && (addr->addr[idx] != 0xFF)) {
                    return FALSE;
                }
            } else {
                if (vtss_ipv6_addr_is_zero(addr)) {
                    return FALSE;
                }

                break;
            }
        }
    }

    return TRUE;
}

// Pure function, no side effects
BOOL vtss_ipv6_addr_is_mgmt_support(const mesa_ipv6_t *addr)
{
    if (vtss_ipv6_addr_is_loopback(addr) || _vtss_ipv6_addr_is_v4_form(addr)) {
        return FALSE;
    }

    return TRUE;
}

/*
This function follows the private IP internets defined in RFC 1918 .
--
3. Private Address Space

   The Internet Assigned Numbers Authority (IANA) has reserved the
   following three blocks of the IP address space for private internets:

     10.0.0.0        -   10.255.255.255  (10/8 prefix)
     172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
     192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
*/
// Pure function, no side effects
int vtss_ipv4_addr_to_prefix(mesa_ipv4_t ip)
{
    if ((ip & 0xff000000) == 0x0a000000) {  // 10.0.0.0/8
        return 8;
    } else if ((ip & 0xfff00000) == 0xac100000) {  // 172.16.0.0/12
        return 12;
    } else if ((ip & 0xffff0000) == 0xc0a80000) {  // 192.168.0.0/16
        return 16;
    } else if ((ip & 0x80000000) == 0x00000000) {  // class A
        return 8;
    } else if ((ip & 0xC0000000) == 0x80000000) {  // class B
        return 16;
    } else if ((ip & 0xE0000000) == 0xC0000000) {  // class C
        return 24;
    }

    return 0;
}

// Pure function, no side effects
mesa_rc vtss_prefix_cnt(const u8 *data, const u32 length, u32 *prefix)
{
    u32 i;
    u32 cnt = 0;
    BOOL prefix_ended = FALSE;

    for (i = 0; i < length; ++i) {
        if (prefix_ended && data[i] != 0) {
            return VTSS_RC_ERROR;
        }

        /*lint --e{616} ... Fallthrough intended */
        switch (data[i]) {
        case 0xff:
            cnt += 8;
            break;
        case 0xfe:
            cnt += 1;
        case 0xfc:
            cnt += 1;
        case 0xf8:
            cnt += 1;
        case 0xf0:
            cnt += 1;
        case 0xe0:
            cnt += 1;
        case 0xc0:
            cnt += 1;
        case 0x80:
            cnt += 1;
        case 0x00:
            prefix_ended = TRUE;
            break;
        default:
            return VTSS_RC_ERROR;
        }
    }

    *prefix = cnt;
    return VTSS_RC_OK;
}

// Pure function, no side effects
mesa_rc vtss_conv_ipv4mask_to_prefix(const mesa_ipv4_t ipv4, u32 *const prefix)
{
    u32 data = htonl(ipv4);
    return vtss_prefix_cnt((u8 *)&data, 4, prefix);
}

// Pure function, no side effects
mesa_ipv4_t vtss_ipv4_prefix_to_mask(uint32_t prefix)
{
    mesa_ipv4_t result;

    if (prefix > 32) {
        T_E("Prefix = %d", prefix);
        prefix = 32;
    }

    if (prefix) {
        result = ~((1u << (32 - prefix)) - 1);
    } else {
        result = 0;
    }

    return result;
}

// Pure function, no side effects
mesa_rc vtss_conv_ipv6mask_to_prefix(const mesa_ipv6_t *ipv6, u32 *prefix)
{
    return vtss_prefix_cnt(ipv6->addr, 16, prefix);
}

// Pure function, no side effects
mesa_rc vtss_conv_prefix_to_ipv6mask(const u32 prefix,
                                     mesa_ipv6_t *const mask)
{
    u8 v = 0;
    u32 i = 0;
    u32 next_bit = 0;

    if (prefix > 128) {
        return VTSS_RC_ERROR;
    }

    /* byte-wise update or clear */
    for (i = 0; i < 16; ++i) {
        u32 b = (i + 1) * 8;
        if (b <= prefix) {
            mask->addr[i] = 0xff;
            next_bit = b;
        } else {
            mask->addr[i] = 0x0;
        }
    }

    switch (prefix % 8) {
    case 1:
        v = 0x80;
        break;
    case 2:
        v = 0xc0;
        break;
    case 3:
        v = 0xe0;
        break;
    case 4:
        v = 0xf0;
        break;
    case 5:
        v = 0xf8;
        break;
    case 6:
        v = 0xfc;
        break;
    case 7:
        v = 0xfe;
        break;
    }

    mask->addr[(next_bit / 8)] = v;
    return VTSS_RC_OK;
}

// Pure function, no side effects
mesa_rc vtss_build_ipv4_network(mesa_ipv4_network_t *network,
                                const mesa_ipv4_t address,
                                const mesa_ipv4_t mask)
{
    if (!network) {
        return VTSS_RC_ERROR;
    }

    network->address = address;
    VTSS_RC(vtss_conv_ipv4mask_to_prefix(mask, &network->prefix_size));

    return VTSS_RC_OK;
}

// Pure function, no side effects
int vtss_if_link_flag_to_txt(char *buf, int size, vtss_appl_ip_if_link_flags_t f)
{
    int s = 0;
    BOOL first = TRUE;

    /*lint --e{438} */
#define F(X)                                 \
    if (f & VTSS_APPL_IP_IF_LINK_FLAG_##X) { \
        if (first) {                         \
            first = FALSE;                   \
            PRINTF(#X);                      \
        } else {                             \
            PRINTF(" " #X);                  \
        }                                    \
    }

    *buf = 0;
    F(UP);
    F(BROADCAST);
    F(LOOPBACK);
    F(RUNNING);
    F(NOARP);
    F(PROMISC);
    F(MULTICAST);
#if 0
    F(IPV6_RA_MANAGED);
    F(IPV6_RA_OTHER);
#endif
#undef F

    buf[MIN(size - 1, s)] = 0;
    return s;
}

// Pure function, no side effects
int vtss_if_ipv6_flag_to_txt(char *buf, int size, vtss_appl_ip_if_ipv6_flags_t f)
{
    int s = 0;
    BOOL first = TRUE;

    /*lint --e{438} */
#define F(X)                                 \
    if (f & VTSS_APPL_IP_IF_IPV6_FLAG_##X) { \
        if (first) {                         \
            first = FALSE;                   \
            PRINTF(#X);                      \
        } else {                             \
            PRINTF(" " #X);                  \
        }                                    \
    }

    *buf = 0;
    F(UP);
    F(RUNNING);
    F(STATIC);
    F(AUTOCONF);
    F(DHCP);
    F(ANYCAST);
    F(TENTATIVE);
    F(TEMPORARY);
    F(NODAD);
    F(DUPLICATED);
    F(DETACHED);
    F(DEPRECATED);
#undef F

    buf[MIN(size - 1, s)] = 0;
    return s;
}

// Pure function, no side effects
char *vtss_ip_route_status_flags_to_txt(char *buf, int size, vtss_appl_ip_route_status_flags_t f)
{
    int  s     = 0;
    bool first = true;

    /*lint --e{438} */
#define F(X)                                      \
    if (f & VTSS_APPL_IP_ROUTE_STATUS_FLAG_##X) { \
        if (first) {                              \
            PRINTF(#X);                           \
            first = false;                        \
        } else {                                  \
            PRINTF(" " #X);                       \
        }                                         \
    }

    buf[0] = '\0';
    F(SELECTED);
    F(ACTIVE);
    F(FIB);
    F(DIRECTLY_CONNECTED);
    F(ONLINK);
    F(DUPLICATE);
    F(RECURSIVE);
    F(UNREACHABLE);
    F(REJECT);
    F(ADMIN_PROHIBITED);
    F(BLACKHOLE);
#undef F

    buf[MIN(size - 1, s)] = 0;
    return buf;
}

/******************************************************************************/
// vtss_ipv4_network_equal()
/******************************************************************************/
bool vtss_ipv4_network_equal(const mesa_ipv4_network_t *a, const mesa_ipv4_network_t *b)
{
    if (a->prefix_size != b->prefix_size) {
        return false;
    }

    return a->address == b->address;
}

/******************************************************************************/
// vtss_ipv6_network_equal()
/******************************************************************************/
bool vtss_ipv6_network_equal(const mesa_ipv6_network_t *a, const mesa_ipv6_network_t *b)
{
    if (a->prefix_size != b->prefix_size) {
        return false;
    }

    return memcmp(a->address.addr, b->address.addr, sizeof(a->address.addr)) == 0;
}

/******************************************************************************/
// vtss_ipv4_net_equal()
/******************************************************************************/
bool vtss_ipv4_net_equal(const mesa_ipv4_network_t *a, const mesa_ipv4_network_t *b)
{
    mesa_ipv4_t mask;

    if (a->prefix_size != b->prefix_size) {
        return false;
    }

    mask = vtss_ipv4_prefix_to_mask(a->prefix_size);

    return (a->address & mask) == (b->address & mask);
}

/******************************************************************************/
// vtss_ipv6_net_equal()
/******************************************************************************/
bool vtss_ipv6_net_equal(const mesa_ipv6_network_t *a, const mesa_ipv6_network_t *b)
{
    mesa_ipv6_t mask;
    if (a->prefix_size == b->prefix_size &&
        vtss_conv_prefix_to_ipv6mask(a->prefix_size, &mask) == VTSS_RC_OK) {
        size_t i;
        for (i = 0; i < sizeof(mask.addr); i++) {
            u8 maskb = mask.addr[i];
            // Compare network (masked) only
            if ((a->address.addr[i] & maskb) != (b->address.addr[i] & maskb)) {
                return false;
            }
        }

        return true;
    }

    return false;
}

/******************************************************************************/
// vtss_ip_net_equal()
/******************************************************************************/
bool vtss_ip_net_equal(const mesa_ip_network_t *a, const mesa_ip_network_t *b)
{
    if (a->address.type != b->address.type) {
        return false;
    }

    switch (a->address.type) {
    case MESA_IP_TYPE_IPV4: {
        if (a->prefix_size == b->prefix_size) {
            mesa_ipv4_t mask = vtss_ipv4_prefix_to_mask(a->prefix_size);
            return (a->address.addr.ipv4 & mask) == (b->address.addr.ipv4 & mask);
        }

        return false;
    }

    case MESA_IP_TYPE_IPV6: {
        mesa_ipv6_t mask;
        if (a->prefix_size == b->prefix_size && vtss_conv_prefix_to_ipv6mask(a->prefix_size, &mask) == VTSS_RC_OK) {
            size_t i;
            for (i = 0; i < sizeof(mask.addr); i++) {
                u8 maskb = mask.addr[i];

                // Compare network (masked) only
                if ((a->address.addr.ipv6.addr[i] & maskb) != (b->address.addr.ipv6.addr[i] & maskb)) {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    default:
        break;
    }

    return false;
}

mesa_ipv4_network_t vtss_ipv4_net_mask_out(const mesa_ipv4_network_t *n)
{
    mesa_ipv4_network_t res = *n;

    res.address = n->address & vtss_ipv4_prefix_to_mask(n->prefix_size);
    return res;
}

mesa_ipv6_network_t vtss_ipv6_net_mask_out(const mesa_ipv6_network_t *n)
{
    mesa_ipv6_t mask;
    mesa_ipv6_network_t res = *n;
    (void)vtss_conv_prefix_to_ipv6mask(n->prefix_size, &mask);
    for (size_t i = 0; i < sizeof(mask.addr); i++) {
        res.address.addr[i] = n->address.addr[i] & mask.addr[i];
    }

    return res;
}

mesa_ip_network_t vtss_ip_net_mask_out(const mesa_ip_network_t *n)
{
    mesa_ip_network_t res = *n;

    switch (n->address.type) {
    case MESA_IP_TYPE_IPV4: {
        res.address.addr.ipv4 = n->address.addr.ipv4 & vtss_ipv4_prefix_to_mask(n->prefix_size);
        return res;
    }

    case MESA_IP_TYPE_IPV6: {
        mesa_ipv6_t mask;
        (void)vtss_conv_prefix_to_ipv6mask(n->prefix_size, &mask);
        for (size_t i = 0; i < sizeof(mask.addr); i++)
            res.address.addr.ipv6.addr[i] =
                n->address.addr.ipv6.addr[i] & mask.addr[i];
        return res;
    }

    default:
        return res;
    }
}

static void operator_ipv6_and(const mesa_ipv6_t *const a,
                              const mesa_ipv6_t *const b,
                              mesa_ipv6_t *const res)
{
    int i;
    for (i = 0; i < 16; ++i) {
        res->addr[i] = a->addr[i] & b->addr[i];
    }
}

static BOOL operator_ipv6_equal(const mesa_ipv6_t *const a,
                                const mesa_ipv6_t *const b)
{
    int i;
    for (i = 0; i < 16; ++i) {
        if (a->addr[i] != b->addr[i]) {
            return FALSE;
        }
    }

    return TRUE;
}

/******************************************************************************/
// vtss_ipv4_net_overlap()
/******************************************************************************/
bool vtss_ipv4_net_overlap(const mesa_ipv4_network_t *a, const mesa_ipv4_network_t *b)
{
    mesa_ipv4_t mask_a = 0, mask_b = 0;

    mask_a = vtss_ipv4_prefix_to_mask(a->prefix_size);
    mask_b = vtss_ipv4_prefix_to_mask(b->prefix_size);

    return (a->address & mask_a) == (b->address & mask_a) ||
           (b->address & mask_b) == (a->address & mask_b);
}

/******************************************************************************/
// vtss_ipv6_net_overlap()
/******************************************************************************/
bool vtss_ipv6_net_overlap(const mesa_ipv6_network_t *a, const mesa_ipv6_network_t *b)
{
    mesa_ipv6_t mask_a, mask_b;
    mesa_ipv6_t a_mask_a;
    mesa_ipv6_t a_mask_b;
    mesa_ipv6_t b_mask_a;
    mesa_ipv6_t b_mask_b;

    (void)vtss_conv_prefix_to_ipv6mask(a->prefix_size, &mask_a);
    (void)vtss_conv_prefix_to_ipv6mask(b->prefix_size, &mask_b);

    operator_ipv6_and(&a->address, &mask_a, &a_mask_a);
    operator_ipv6_and(&a->address, &mask_b, &a_mask_b);
    operator_ipv6_and(&b->address, &mask_a, &b_mask_a);
    operator_ipv6_and(&b->address, &mask_b, &b_mask_b);

    return operator_ipv6_equal(&a_mask_a, &b_mask_a) ||
           operator_ipv6_equal(&b_mask_b, &a_mask_b);
}

bool vtss_ip_net_overlap(const mesa_ip_network_t *a, const mesa_ip_network_t *b)
{
    if (a->address.type != b->address.type) {
        return false;
    }

    switch (a->address.type) {
    case MESA_IP_TYPE_IPV4: {
        mesa_ipv4_network_t _a, _b;
        _a.prefix_size = a->prefix_size;
        _a.address = a->address.addr.ipv4;
        _b.prefix_size = b->prefix_size;
        _b.address = b->address.addr.ipv4;
        return vtss_ipv4_net_overlap(&_a, &_b);
    }

    case MESA_IP_TYPE_IPV6: {
        mesa_ipv6_network_t _a, _b;
        _a.prefix_size = a->prefix_size;
        _a.address = a->address.addr.ipv6;
        _b.prefix_size = b->prefix_size;
        _b.address = b->address.addr.ipv6;
        return vtss_ipv6_net_overlap(&_a, &_b);
    }

    default:
        return false;
    }
}

/******************************************************************************/
// vtss_ipv4_net_include()
/******************************************************************************/
bool vtss_ipv4_net_include(const mesa_ipv4_network_t *net, const mesa_ipv4_t *addr)
{
    mesa_ipv4_t mask = vtss_ipv4_prefix_to_mask(net->prefix_size);

    return (net->address & mask) == (*addr & mask);
}

/******************************************************************************/
// vtss_ipv6_net_include()
/******************************************************************************/
bool vtss_ipv6_net_include(const mesa_ipv6_network_t *net, const mesa_ipv6_t *addr)
{
    mesa_ipv6_t mask;
    mesa_ipv6_t a;
    mesa_ipv6_t b;

    (void)vtss_conv_prefix_to_ipv6mask(net->prefix_size, &mask);

    operator_ipv6_and(&net->address, &mask, &a);
    operator_ipv6_and(addr, &mask, &b);

    return operator_ipv6_equal(&a, &b);
}

mesa_rc vtss_appl_ip_if_status_to_ip(
    const vtss_appl_ip_if_status_t *const status,
    mesa_ip_addr_t *const ip)
{
    if (status->type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV4) {
        ip->type = MESA_IP_TYPE_IPV4;
        ip->addr.ipv4 = status->u.ipv4.net.address;
        return VTSS_RC_OK;
    }

    if (status->type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV6) {
        ip->type = MESA_IP_TYPE_IPV6;
        memcpy(ip->addr.ipv6.addr, status->u.ipv6.net.address.addr, 16);
        return VTSS_RC_OK;
    }

    return VTSS_RC_ERROR;
}

BOOL vtss_ip_ipv4_ifaddr_valid(const mesa_ipv4_network_t *const net)
{
    mesa_ipv4_t mask;

    /* IPMC/BCast check */
    if (((net->address >> 24) & 0xff) >= 224) {
        return FALSE;
    }

    // Do not allow address in the 0.0.0.0/8 network
    if (((net->address >> 24) & 0xff) == 0) {
        return FALSE;
    }

    /* Prefix check */
    if (net->prefix_size == 31) {
        return TRUE; // RFC3021
    }
    if (net->prefix_size <= 30) {
        /* Host part cheks */
        mask = vtss_ipv4_prefix_to_mask(net->prefix_size);

        if ((net->address & ~mask) != (((uint32_t) - 1) & ~mask) &&
            (net->address & ~mask) != 0) {
            return TRUE; /* Not using subnet bcast/zero host part */
        }
    }

    return FALSE;
}

BOOL vtss_ip_ipv6_ifaddr_valid(const mesa_ipv6_network_t *const net)
{
    if (net->prefix_size <= 128 &&
        vtss_ipv6_addr_is_mgmt_support(&net->address) &&
        !vtss_ipv6_addr_is_zero(&net->address) &&
        !vtss_ipv6_addr_is_multicast(&net->address)) {
        return TRUE; /* Not using subnet bcast/zero host part */
    }

    return FALSE;
}

BOOL vtss_ip_ifaddr_valid(const mesa_ip_network_t *const net)
{
    if (net->address.type == MESA_IP_TYPE_IPV4) {
        mesa_ipv4_network_t n;
        n.address = net->address.addr.ipv4;
        n.prefix_size = net->prefix_size;
        return vtss_ip_ipv4_ifaddr_valid(&n);

    } else if (net->address.type == MESA_IP_TYPE_IPV6) {
        mesa_ipv6_network_t n;
        n.address = net->address.addr.ipv6;
        n.prefix_size = net->prefix_size;
        return vtss_ip_ipv6_ifaddr_valid(&n);
    }

    return FALSE; /* Bad address */
}

/******************************************************************************/
// vtss_ip_ip_addr_to_txt()
/******************************************************************************/
int vtss_ip_ip_addr_to_txt(char *buf, int size, const mesa_ip_addr_t *ip)
{
    int          s = 0;
    char        _buf[41];
    mesa_ipv6_t adrs6;

    switch (ip->type) {
    case MESA_IP_TYPE_NONE:
        PRINTF("NONE");
        break;
    case MESA_IP_TYPE_IPV4:
        PRINTF(VTSS_IPV4_FORMAT, VTSS_IPV4_ARGS(ip->addr.ipv4));
        break;
    case MESA_IP_TYPE_IPV6:
        /* Ensure the linklocal IPv6 address to get rid of the scope id for
         * display */
        memcpy(&adrs6, &(ip->addr.ipv6), sizeof(mesa_ipv6_t));
        if (vtss_ipv6_addr_is_link_local(&adrs6)) {
            adrs6.addr[2] = adrs6.addr[3] = 0x0;
        }
        (void)misc_ipv6_txt(&adrs6, _buf);
        _buf[40] = 0;
        PRINTF("%s", _buf);
        break;
    default:
        PRINTF("Unknown-type:%u", ip->type);
    }

    return s;
}

/******************************************************************************/
// vtss_ip_if_status_to_txt()
/******************************************************************************/
int vtss_ip_if_status_to_txt(char *buf, int size, const vtss_appl_ip_if_status_t *st, u32 length)
{
    u32         i;
    int         s = 0;
    char        mac_str[18];
#if defined(VTSS_SW_OPTION_IPV6)
    char        _buf[128];
    mesa_ipv6_t adrs6;
#endif

    if (length == 0) {
        return 0;
    }

    PRINTFUNC(vtss_ip_ifindex_to_txt, (st->ifindex));
    PRINTF("\n");

    for (i = 0; i < length; ++i, ++st) {
        switch (st->type) {
        case VTSS_APPL_IP_IF_STATUS_TYPE_LINK:
            PRINTF("  LINK: %s Mtu:%u <", misc_mac_txt(st->u.link.mac.addr, mac_str), st->u.link.mtu);
            PRINTFUNC(vtss_if_link_flag_to_txt, st->u.link.flags);
            PRINTF(">\n");
            break;

        case VTSS_APPL_IP_IF_STATUS_TYPE_IPV4:
            PRINTF("  IPv4: " VTSS_IPV4N_FORMAT,
                   VTSS_IPV4N_ARG(st->u.ipv4.net));
            PRINTF(" " VTSS_IPV4_FORMAT "\n",
                   VTSS_IPV4_ARGS(st->u.ipv4.info.broadcast));
            break;

        case VTSS_APPL_IP_IF_STATUS_TYPE_IPV6:
#if defined(VTSS_SW_OPTION_IPV6)
            /* Ensure the linklocal IPv6 address to get rid of the scope id for
             * display */
            memcpy(&adrs6, &(st->u.ipv6.net.address), sizeof(mesa_ipv6_t));
            if (vtss_ipv6_addr_is_link_local(&adrs6)) {
                adrs6.addr[2] = adrs6.addr[3] = 0x0;
            }
            (void)misc_ipv6_txt(&adrs6, _buf);
            PRINTF("  IPv6: %s/%d", _buf, st->u.ipv6.net.prefix_size);
            PRINTF(" <");
            PRINTFUNC(vtss_if_ipv6_flag_to_txt, st->u.ipv6.info.flags);
            PRINTF(">\n");
#endif /* VTSS_SW_OPTION_IPV6 */
            break;

        case VTSS_APPL_IP_IF_STATUS_TYPE_DHCP4C:
            PRINTF("  DHCP: ");
            PRINTFUNC(vtss::dhcp::to_txt, &st->u.dhcp4c);
            PRINTF("\n");
            break;

        default:
            PRINTF("  UNKNOWN\n");
        }
    }

    buf[MIN(size - 1, s)] = 0;
    return s;
}

/******************************************************************************/
// IP_UTILS_neighbor_status_to_txt()
/******************************************************************************/
static int IP_UTILS_neighbor_status_to_txt(char *buf, int size, const vtss_appl_ip_neighbor_key_t &key, const vtss_appl_ip_neighbor_status_t &status)
{
    char mac_str[18];
    int  s = 0;
    bool first = true;

    PRINTFUNC(vtss_ip_ip_addr_to_txt, &key.dip);

    if (status.flags & VTSS_APPL_IP_NEIGHBOR_FLAG_VALID) {
        PRINTF(" via ");
        PRINTFUNC(vtss_ip_ifindex_to_txt, key.ifindex);

        if (!vtss_ifindex_is_cpu(key.ifindex)) {
            PRINTF(":%s", misc_mac_txt(status.dmac.addr, mac_str));
        }
    } else {
        PRINTF(" (Incomplete)");
    }

    PRINTF(" <")

    if (status.flags & VTSS_APPL_IP_NEIGHBOR_FLAG_PERMANENT) {
        PRINTF("%sPermanent", first ? "" : " ");
        first = false;
    }

    if (status.flags & VTSS_APPL_IP_NEIGHBOR_FLAG_ROUTER) {
        PRINTF("%sRouter", first ? "" : " ");
        first = false;
    }

    if (status.flags & VTSS_APPL_IP_NEIGHBOR_FLAG_HARDWARE) {
        PRINTF("%sHardware", first ? "" : " ");
        first = false;
    }

    PRINTF(">");

    return s;
}

bool operator<(const vtss_appl_ip_if_key_ipv4_t  &a, const vtss_appl_ip_if_key_ipv4_t  &b)
{
    // First sort by ifindex
    if (a.ifindex != b.ifindex) {
        return a.ifindex < b.ifindex;
    }

    // Then by address
    return a.addr < b.addr;
}

bool operator<(const vtss_appl_ip_if_key_ipv6_t  &a, const vtss_appl_ip_if_key_ipv6_t  &b)
{
    // First sort by ifindex
    if (a.ifindex != b.ifindex) {
        return a.ifindex < b.ifindex;
    }

    // Then by address
    return a.addr < b.addr;
}

/******************************************************************************/
// ip_util_route_protocol_to_letter()
/******************************************************************************/
const char *ip_util_route_protocol_to_letter(vtss_appl_ip_route_protocol_t protocol)
{
    switch (protocol) {
    case VTSS_APPL_IP_ROUTE_PROTOCOL_KERNEL:
        return "K";

    case VTSS_APPL_IP_ROUTE_PROTOCOL_DHCP:
        return "D";

    case VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED:
        return "C";

    case VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC:
        return "S";

    case VTSS_APPL_IP_ROUTE_PROTOCOL_OSPF:
        return "O";

    case VTSS_APPL_IP_ROUTE_PROTOCOL_RIP:
        return "R";

    default:
        T_E("Unknown route protocol (%d)", protocol);
        break;
    }

    return "?";
}

/******************************************************************************/
// ip_util_route_protocol_to_str()
/******************************************************************************/
const char *ip_util_route_protocol_to_str(vtss_appl_ip_route_protocol_t protocol, bool capital_first_letter)
{
    switch (protocol) {
    case VTSS_APPL_IP_ROUTE_PROTOCOL_KERNEL:
        return capital_first_letter ? "Kernel"    : "kernel";

    case VTSS_APPL_IP_ROUTE_PROTOCOL_DHCP:
        return capital_first_letter ? "DHCP"      : "dhcp";

    case VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED:
        return capital_first_letter ? "Connected" : "connected";

    case VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC:
        return capital_first_letter ? "Static"    : "static";

    case VTSS_APPL_IP_ROUTE_PROTOCOL_OSPF:
        return capital_first_letter ? "OSPF"      : "ospf";

    case VTSS_APPL_IP_ROUTE_PROTOCOL_RIP:
        return capital_first_letter ? "RIP"       : "rip";

    default:
        T_E("Unknown route protocol (%d)", protocol);
        break;
    }

    return "?";
}

/******************************************************************************/
// vtss_appl_ip_route_key_t::operator<
// A.o. for insertion into vtss::Map.
/******************************************************************************/
bool operator<(const vtss_appl_ip_route_key_t &a, const vtss_appl_ip_route_key_t &b)
{
    // First sort by type (IPv4/IPv6)
    if (a.type != b.type) {
        return a.type < b.type;
    }

    switch (a.type) {
    case VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC:
        return a.route.ipv4_uc < b.route.ipv4_uc;

    case VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC:
        if (a.route.ipv6_uc != b.route.ipv6_uc) {
            return a.route.ipv6_uc < b.route.ipv6_uc;
        }

        return a.vlan_ifindex < b.vlan_ifindex;

    default:
        T_E("Invalid type (%d)", a.type);
        return false;
    }
}

/******************************************************************************/
// vtss_appl_ip_route_key_t::operator==
/******************************************************************************/
bool operator==(const vtss_appl_ip_route_key_t &a, const vtss_appl_ip_route_key_t &b)
{
    // First sort by type (IPv4/IPv6)
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
    case VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC:
        return a.route.ipv4_uc == b.route.ipv4_uc;

    case VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC:
        if (a.route.ipv6_uc != b.route.ipv6_uc) {
            return false;
        }

        return a.vlan_ifindex != b.vlan_ifindex;

    default:
        T_E("Invalid type (%d)", a.type);
        return false;
    }
}

/******************************************************************************/
// IP_UTIL_route_entry_print()
/******************************************************************************/
static void IP_UTIL_route_entry_print(vtss_ip_cli_pr *pr, const vtss_appl_ip_route_status_key_t &key, const vtss_appl_ip_route_status_t &status, bool same_net_and_proto_as_prev)
{
    StringStream bs, network;
    bool         dest_is_zero, dest_is_blackhole, is_ipv4 = key.route.type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC;

    if (same_net_and_proto_as_prev) {
        // Don't print protocol letter
        bs << " ";
    } else {
        bs << ip_util_route_protocol_to_letter(key.protocol);
    }

#define F(X) ((status.flags & VTSS_APPL_IP_ROUTE_STATUS_FLAG_##X) != 0)

    if (F(FIB) && !F(DUPLICATE)) {
        // Route in kernel's FIB (Forwarding Information Base) database and not
        // a duplicate.
        bs << "* ";
    } else {
        bs << "  ";
    }

    // If the network information needs to be skipped, like the second line in
    // this example:
    //   S  25.0.0.0/8 [1/0] via 60.0.0.25, Vlan60
    //    *            [9/0] via 50.0.0.100, Vlan50
    // we need to know the length start of network.
    if (is_ipv4) {
        network << key.route.route.ipv4_uc.network;
    } else {
        network << key.route.route.ipv6_uc.network;
    }

    if (same_net_and_proto_as_prev) {
        bs << WhiteSpace(network.buf.size());
    } else {
        bs << network.buf;
    }

    // All routes have distance/metric except connected routes.
    if (key.protocol != VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED) {
        bs << " [" << status.distance << "/" << status.metric << "]";
    }

    if (is_ipv4) {
        dest_is_zero      = key.route.route.ipv4_uc.destination == 0;
        dest_is_blackhole = key.route.route.ipv4_uc.destination == vtss_ipv4_blackhole_route;
    } else {
        dest_is_zero = vtss_ipv6_addr_is_zero(&key.route.route.ipv6_uc.destination);
        dest_is_blackhole = key.route.route.ipv6_uc.destination == vtss_ipv6_blackhole_route;
    }

    if (dest_is_zero || dest_is_blackhole) {
        if (F(DIRECTLY_CONNECTED)) {
            bs << " is directly connected, " << status.nexthop_ifindex;
        } else if (F(UNREACHABLE)) {
            bs << " unreachable";
            if (F(REJECT)) {
                bs << " (ICMP unreachable)";
            } else if (F(ADMIN_PROHIBITED)) {
                bs << " (ICMP admin-prohibited)";
            } else if (F(BLACKHOLE)) {
                bs << " (blackhole)";
            }
        }
    } else {
        if (is_ipv4) {
            bs << " via " << Ipv4Address(key.route.route.ipv4_uc.destination);
        } else {
            bs << " via " << key.route.route.ipv6_uc.destination;
        }

        if (status.nexthop_ifindex != VTSS_IFINDEX_NONE) {
            bs << ", " << status.nexthop_ifindex;
        }
    }

    if (!F(ACTIVE)) {
        bs << " inactive";
    }

    if (F(ONLINK)) {
        bs << " onlink";
    }

    if (F(RECURSIVE)) {
        bs << " (recursive)";
    }

    bs << ", " << misc_time_txt(status.uptime);

#undef F
    pr("%s\n", bs.buf.c_str());
}

/******************************************************************************/
// ip_util_route_warning_print()
/******************************************************************************/
mesa_rc ip_util_route_warning_print(vtss_ip_cli_pr *pr)
{
    vtss_appl_ip_global_notification_status_t notif;

    VTSS_RC(vtss_appl_ip_global_notification_status_get(&notif));

    if (notif.hw_routing_table_depleted) {
        pr("\n"
           "WARNING: The hardware routing table is depleted and is now out of sync with the\n"
           "         kernel's FIB database. This may cause routing to fail.\n"
           "         It is strongly advised to lower the number of routes by restructuring\n"
           "         the network or summarizing routes.\n"
           "         A reload of the switch is required to leave this state.\n\n");
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// ip_util_route_print()
/******************************************************************************/
mesa_rc ip_util_route_print(vtss_appl_ip_route_type_t type, vtss_ip_cli_pr *pr)
{
    vtss_appl_ip_route_status_map_t     routes;
    vtss_appl_ip_route_status_map_itr_t itr;
    vtss_appl_ip_route_status_key_t     prev_rt = {};
    bool                                same_net_and_proto_as_prev, is_ipv4 = type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC;

    VTSS_RC(ip_util_route_warning_print(pr));

    pr("Codes: C - connected, S - static");

    if (frr_has_ospfd() || frr_has_ospf6d()) {
        pr(", O - OSPF");
    }

    if (is_ipv4 && frr_has_ripd()) {
        pr(", R - RIP");
    }

    if (is_ipv4) {
        pr("\n       * - FIB route, D - DHCP installed route\n\n");
    } else {
        // Detection of DHCP-installed route not yet supported for IPv6.
        pr("\n       * - FIB route\n\n");
    }

    VTSS_RC(vtss_appl_ip_route_status_get_all(routes, type));

    for (itr = routes.begin(); itr != routes.end(); ++itr) {
        if (is_ipv4) {
            same_net_and_proto_as_prev = prev_rt.route.route.ipv4_uc.network == itr->first.route.route.ipv4_uc.network && prev_rt.protocol == itr->first.protocol;
        } else {
            same_net_and_proto_as_prev = prev_rt.route.route.ipv6_uc.network == itr->first.route.route.ipv6_uc.network && prev_rt.protocol == itr->first.protocol;
        }

        prev_rt = itr->first;

        IP_UTIL_route_entry_print(pr, itr->first, itr->second, same_net_and_proto_as_prev);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// ip_util_if_print()
/******************************************************************************/
int ip_util_if_print(vtss_ip_cli_pr *pr, vtss_appl_ip_if_status_type_t status_type, bool vlan_only)
{
    const uint32_t           obj_cnt_max  = 1024;
    int                      res          = 0;
    bool                     first        = true;
    vtss_ifindex_t           prev_ifindex = VTSS_IFINDEX_NONE, ifindex;
    vtss_appl_ip_if_status_t *status      = (vtss_appl_ip_if_status_t *)VTSS_CALLOC(obj_cnt_max, sizeof(vtss_appl_ip_if_status_t));
    char                     buf[1024];
    uint32_t                 if_st_cnt;

    if (status == nullptr) {
        return -1;
    }

    while (vtss_appl_ip_if_itr(&prev_ifindex, &ifindex, vlan_only) == VTSS_RC_OK) {
        prev_ifindex = ifindex;

        if (!first) {
            (void)(*pr)("\n");
        } else {
            first = false;
        }

        if (vtss_appl_ip_if_status_get(ifindex, status_type, obj_cnt_max, &if_st_cnt, status) != VTSS_RC_OK) {
            break;
        }

        res += vtss_ip_if_status_to_txt(buf, sizeof(buf), status, if_st_cnt);
        (void)(*pr)("%s", buf);
    }

    if (status) {
        VTSS_FREE(status);
    }

    return res;
}

/******************************************************************************/
// ip_util_if_brief_print()
/******************************************************************************/
int ip_util_if_brief_print(mesa_ip_type_t type, vtss_ip_cli_pr *pr)
{
    vtss_appl_ip_if_key_ipv4_t    ipv4_key = {};
    vtss_appl_ip_if_key_ipv6_t    ipv6_key = {};
    vtss_appl_ip_if_status_link_t link_status;
    vtss_appl_ip_if_conf_ipv4_t   conf;
    vtss::StringStream            bs;
    char                          buf[64];
    bool                          first = true;
    mesa_rc                       rc;

    if (type == MESA_IP_TYPE_IPV4) {
        while (vtss_appl_ip_if_status_ipv4_itr(&ipv4_key, &ipv4_key) == VTSS_RC_OK) {
            if ((rc = vtss_appl_ip_if_conf_ipv4_get(ipv4_key.ifindex, &conf)) != VTSS_RC_OK) {
                T_E("vtss_appl_ip_if_conf_ipv4_get(%s) failed: %s", ipv4_key.ifindex, error_txt(rc));
                continue;
            }

            if ((rc = vtss_appl_ip_if_status_link_get(ipv4_key.ifindex, &link_status)) != VTSS_RC_OK) {
                T_E("vtss_appl_ip_if_status_link_get(%s) failed: %s", ipv4_key.ifindex, error_txt(rc));
                continue;
            }

            if (first) {
                first = false;
                (void)(*pr)("Interface Address            Method Status\n");
                (void)(*pr)("--------- ------------------ ------ ------\n");
            }

            (void)vtss_ip_ifindex_to_txt(buf, sizeof(buf), ipv4_key.ifindex);
            (void)(*pr)("%-9s ", buf);
            (void)snprintf(buf, sizeof(buf), VTSS_IPV4N_FORMAT, VTSS_IPV4N_ARG(ipv4_key.addr));
            (void)(*pr)("%-18s %-6s %s\n", buf, conf.dhcpc_enable ? "DHCP" : "Manual", link_status.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP ? "UP" : "DOWN");
        }
    } else {
        while (vtss_appl_ip_if_status_ipv6_itr(&ipv6_key, &ipv6_key) == VTSS_RC_OK) {
            if ((rc = vtss_appl_ip_if_status_link_get(ipv6_key.ifindex, &link_status)) != VTSS_RC_OK) {
                T_E("vtss_appl_ip_if_status_link_get(%s) failed: %s", ipv6_key.ifindex, error_txt(rc));
                continue;
            }

            if (first) {
                first = false;
                (void)(*pr)("Interface Address                                     Status\n");
                (void)(*pr)("--------- ------------------------------------------- ------\n");
            }

            (void)vtss_ip_ifindex_to_txt(buf, sizeof(buf), ipv6_key.ifindex);
            // Use operator<< to get the IP address abbreviated
            bs.clear();
            bs << ipv6_key.addr;
            (void)(*pr)("%-9s %-43s %s\n", buf, bs.buf.c_str(), link_status.flags & VTSS_APPL_IP_IF_LINK_FLAG_UP ? "UP" : "DOWN");
        }
    }

    return 0;
}

/******************************************************************************/
// ip_util_nb_print()
/******************************************************************************/
int ip_util_nb_print(mesa_ip_type_t type, vtss_ip_cli_pr *pr)
{
    vtss_appl_ip_neighbor_key_t    prev_key, key;
    vtss_appl_ip_neighbor_status_t status;
    char                           buf[256];
    int                            cnt   = 0;
    bool                           first = true;

    while (vtss_appl_ip_neighbor_itr(first ? nullptr : &prev_key, &key, type) == VTSS_RC_OK) {
        first    = false;
        prev_key = key;

        if (vtss_appl_ip_neighbor_status_get(&key, &status) != VTSS_RC_OK) {
            cnt += (*pr)("Failed to get neighbor cache\n");
            return cnt;
        }

        if (IP_UTILS_neighbor_status_to_txt(buf, sizeof(buf), key, status)) {
            cnt += (*pr)("%s\n", buf);
        }
    }

    return cnt;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_conf_t &conf)
{
    o << "MTU = " << conf.mtu;

    return o;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_ipv4_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_conf_ipv4_t &conf)
{
    o << "{enable = "            << conf.enable
      << ", dhcpc_enable = "     << conf.dhcpc_enable
      << ", network = "          << conf.network
      << ", fallback enable = "  << conf.fallback_enable
      << ", fallback timeout = " << conf.fallback_timeout_secs
      << "secs}";

    return o;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_ipv4_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_conf_ipv4_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_ipv6_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_conf_ipv6_t &conf)
{
    o << "{enable = "   << conf.enable
      << ", network = " << conf.network
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ip_if_conf_ipv6_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_conf_ipv6_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ip_if_key_ipv4_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_key_ipv4_t &key)
{
    o << "{ifindex = "  << key.ifindex
      << ", network = " << key.addr
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ip_if_info_ipv4_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_info_ipv4_t &info)
{
    o << "{bcast = "                << vtss::Ipv4Address(info.broadcast)
      << ", reasm_max_size = "      << info.reasm_max_size
      << ", arp_retransmit_time = " << info.arp_retransmit_time
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ip_if_info_ipv4_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_info_ipv4_t *info)
{
    o << *info;
    return 0;
}

/******************************************************************************/
// vtss_appl_ip_if_key_ipv4_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_key_ipv4_t *key)
{
    o << *key;
    return 0;
}

/******************************************************************************/
// vtss_appl_ip_if_key_ipv6_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_key_ipv6_t &key)
{
    o << "{ifindex = "  << key.ifindex
      << ", network = " << key.addr
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ip_if_key_ipv6_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_key_ipv6_t *key)
{
    o << *key;
    return 0;
}

/******************************************************************************/
// vtss_appl_ip_if_info_ipv6_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_info_ipv6_t &info)
{
    o << "{flags = "       << info.flags
      << ", os_ifindex = " << info.os_ifindex
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ip_if_info_ipv6_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_info_ipv6_t *info)
{
    o << *info;
    return 0;
}

/******************************************************************************/
// vtss_appl_ip_route_key_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_route_key_t &rt)
{
    mesa_vid_t vlan;

    // The following uses vtss_basics' formatters for network and destination
    switch (rt.type) {
    case VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC:
        o << rt.route.ipv4_uc.network << " via " << vtss::Ipv4Address(rt.route.ipv4_uc.destination);
        break;

    case VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC:
        o << rt.route.ipv6_uc.network << " via " << rt.route.ipv6_uc.destination;

        if ((vlan = vtss_ifindex_as_vlan(rt.vlan_ifindex)) != 0) {
            o << " on VLAN " << vlan;
        }

        break;

    default:
        o << "<Unknown route entry type: " << (uint32_t)rt.type << ">";
    }

    return o;
}

/******************************************************************************/
// vtss_appl_ip_route_key_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_route_key_t *rt)
{
    o << *rt;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ip_route_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_route_conf_t &conf)
{
    o << "distance = " << conf.distance;

    return o;
}

/******************************************************************************/
// vtss_appl_ip_route_conf_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_route_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ip_route_status_flags_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_route_status_flags_t &flags)
{
    bool first = true;

#define F(X)                                          \
    if (flags & VTSS_APPL_IP_ROUTE_STATUS_FLAG_##X) { \
        if (!first) {                                 \
            o << " ";                                 \
        }                                             \
        first = false;                                \
        o << #X;                                      \
    }

    o << "<";
    F(SELECTED);
    F(ACTIVE);
    F(FIB);
    F(DIRECTLY_CONNECTED);
    F(ONLINK);
    F(DUPLICATE);
    F(RECURSIVE);
    F(UNREACHABLE);
    F(REJECT);
    F(ADMIN_PROHIBITED);
    F(BLACKHOLE);
    o << ">";
#undef F

    return o;
}

/******************************************************************************/
// mesa_ipv4_uc_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_ipv4_uc_t *r)
{
    o << *r;
    return 0;
}

/******************************************************************************/
// mesa_ipv6_uc_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_ipv6_uc_t *r)
{
    o << *r;
    return 0;
}

/******************************************************************************/
// vtss::Ipv4Network::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss::Ipv4Network *r)
{
    o << *r;
    return 0;
}

vtss::ostream &operator<<(vtss::ostream &o, const mesa_ipv6_uc_t &i)
{
    o << i.network << " via " << i.destination;
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const mesa_ipv4_uc_t &r)
{
    o << r.network << " via " << vtss::Ipv4Address(r.destination);
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const mesa_routing_entry_t &r)
{
    switch (r.type) {
    case MESA_ROUTING_ENTRY_TYPE_IPV4_UC:
        o << r.route.ipv4_uc.network << " via "
          << vtss::Ipv4Address(r.route.ipv4_uc.destination);
        break;

    case MESA_ROUTING_ENTRY_TYPE_IPV6_UC:
        o << r.route.ipv6_uc.network << " via " << r.route.ipv6_uc.destination;

        if (r.vlan != 0) {
            o << " VLAN " << r.vlan;
        }

        break;

    default:
        o << "<Unknown route entry type: " << (uint32_t)r.type << ">";
    }

    return o;
}

/******************************************************************************/
// vtss_appl_ip_route_status_key_t::operator<()
// For get from and set into a vtss::Map.
/******************************************************************************/
bool operator<(const vtss_appl_ip_route_status_key_t &a, const vtss_appl_ip_route_status_key_t &b)
{
    // First sort by type
    if (a.route.type != b.route.type) {
        return a.route.type < b.route.type;
    }

    // Then by network
    if (a.route.type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
        if (a.route.route.ipv4_uc.network != b.route.route.ipv4_uc.network) {
            return a.route.route.ipv4_uc.network < b.route.route.ipv4_uc.network;
        }
    } else {
        if (a.route.route.ipv6_uc.network != b.route.route.ipv6_uc.network) {
            return a.route.route.ipv6_uc.network < b.route.route.ipv6_uc.network;
        }
    }

    // Then by protocol
    if (a.protocol != b.protocol) {
        return a.protocol < b.protocol;
    }

    // Then by destination (nexthop)
    if (a.route.type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
        if (a.route.route.ipv4_uc.destination != b.route.route.ipv4_uc.destination) {
            return a.route.route.ipv4_uc.destination < b.route.route.ipv4_uc.destination;
        }
    } else {
        if (a.route.route.ipv6_uc.destination != b.route.route.ipv6_uc.destination) {
            return a.route.route.ipv6_uc.destination < b.route.route.ipv6_uc.destination;
        }
    }

    // And finally by nexthop interface (otherwise, we can't distinguish IPv6
    // link-local addresses from each other).
    return a.route.vlan_ifindex < b.route.vlan_ifindex;
}

/******************************************************************************/
// vtss_appl_ip_route_status_key_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_route_status_key_t &k)
{
    o << "{route = "     << k.route
      << ", protocol = " << k.protocol
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ip_route_status_key_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_route_status_key_t *k)
{
    o << *k;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ip_route_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_route_status_t &st)
{
    o << "{nexthop I/F = " << st.nexthop_ifindex
      << ", distance = "   << st.distance
      << ", metric = "     << st.metric
      << ", uptime = "     << st.uptime << " secs"
      << ", flags = "      << st.flags
      << ", os_ifindex = " << st.os_ifindex
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ip_route_status_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_route_status_t *st)
{
    o << *st;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_routing_entry_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_routing_entry_t *r)
{
    o << *r;
    return 0;
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_neighbor_key_t &k)
{
    o << "{" << k.dip << " via " << k.ifindex << "}";
    return o;
}

size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_neighbor_key_t *key)
{
    o << *key;
    return 0;
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_neighbor_status_t &st)
{
    o << "{OS I/F: " << st.os_ifindex << ", dmac: " << st.dmac << ", flags: " << st.flags << ", state: " << st.state << "}";
    return o;
}

size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_neighbor_status_t *st)
{
    o << *st;
    return 0;
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_status_link_t &s)
{
    o << "{os-ifidx: " << s.os_ifindex << " mtu: " << s.mtu
      << " mac: " << s.mac << " flags: " << s.flags << "}";
    return o;
}

size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_status_link_t *st)
{
    o << *st;
    return 0;
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_status_ipv4_t &s)
{
    o << "{" << s.info << ", " << s.net << "}";
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_status_ipv6_t &s)
{
    o << "{info: " << s.info
      << "  net: " << s.net
      << "}";

    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, vtss_appl_ip_if_link_flags_t f)
{
    BOOL first = TRUE;

    o << "[";
    /*lint --e{438} */
#define F(X)                                 \
    if (f & VTSS_APPL_IP_IF_LINK_FLAG_##X) { \
        if (!first) o << " ";                \
        first = FALSE;                       \
        o << #X;                             \
    }

    F(UP);
    F(BROADCAST);
    F(LOOPBACK);
    F(RUNNING);
    F(NOARP);
    F(PROMISC);
    F(MULTICAST);
    F(IPV6_RA_MANAGED);
    F(IPV6_RA_OTHER);
#undef F
    o << "]";

    return o;
}

vtss::ostream &operator<<(vtss::ostream &o,
                          const vtss_appl_ip_if_status_dhcp4c_t &s)
{
    o << "{state: " << s.state << " server: " << s.server_ip << "}";
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_status_t &s)
{
    o << s.ifindex;

    switch (s.type) {
    case VTSS_APPL_IP_IF_STATUS_TYPE_LINK:
        o << " LINK: " << s.u.link;
        break;

    case VTSS_APPL_IP_IF_STATUS_TYPE_IPV4:
        o << " IPV4: " << s.u.ipv4;
        break;

    case VTSS_APPL_IP_IF_STATUS_TYPE_IPV6:
#if defined(VTSS_SW_OPTION_IPV6)
        o << " IPV6: " << s.u.ipv6;
#endif /* VTSS_SW_OPTION_IPV6 */
        break;

    case VTSS_APPL_IP_IF_STATUS_TYPE_DHCP4C:
        o << " DHCP: " << s.u.dhcp4c;
        break;

    default:
        o << "  UNKNOWN";
        break;
    }

    return o;
}

bool operator<(const mesa_ipv4_network_t &a, const mesa_ipv4_network_t &b)
{
    if (a.address != b.address) {
        return a.address < b.address;
    }

    return a.prefix_size < b.prefix_size;
}

bool operator==(const mesa_ipv4_network_t &a, const mesa_ipv4_network_t &b)
{
    return vtss_ipv4_network_equal(&a, &b);
}

bool operator!=(const mesa_ipv4_network_t &a, const mesa_ipv4_network_t &b)
{
    return !vtss_ipv4_network_equal(&a, &b);
}

bool operator<(const mesa_ipv4_uc_t &a, const mesa_ipv4_uc_t &b)
{
    if (a.network != b.network) {
        return a.network < b.network;
    }

    return a.destination < b.destination;
}

bool operator!=(const mesa_ipv4_uc_t &a, const mesa_ipv4_uc_t &b)
{
    if (a.network != b.network) {
        return true;
    }

    return a.destination != b.destination;
}

bool operator==(const mesa_ipv4_uc_t &a, const mesa_ipv4_uc_t &b)
{
    if (a.network != b.network) {
        return false;
    }

    return a.destination == b.destination;
}

bool operator<(const vtss_appl_ip_acd_status_ipv4_key_t &a, const vtss_appl_ip_acd_status_ipv4_key_t &b)
{
    return (a.sip != b.sip ? (a.sip < b.sip) : (a.smac < b.smac));
}

bool operator!=(const vtss_appl_ip_acd_status_ipv4_key_t &a, const vtss_appl_ip_acd_status_ipv4_key_t &b)
{
    return (a.sip != b.sip || a.smac != b.smac);
}

bool operator==(const vtss_appl_ip_acd_status_ipv4_key_t &a, const vtss_appl_ip_acd_status_ipv4_key_t &b)
{
    return (a.sip == b.sip && a.smac == b.smac);
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_acd_status_ipv4_key_t &k)
{
    o << "{sip: " << vtss::Ipv4Address(k.sip) << ", smac: " << k.smac << "}";
    return o;
}

size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_acd_status_ipv4_key_t *key)
{
    o << *key;
    return 0;
}

bool operator<(const vtss_appl_ip_acd_status_ipv4_t &a, const vtss_appl_ip_acd_status_ipv4_t &b)
{
    return (a.ifindex != b.ifindex ? (a.ifindex < b.ifindex) : (a.ifindex_port < b.ifindex_port));
}

bool operator!=(const vtss_appl_ip_acd_status_ipv4_t &a, const vtss_appl_ip_acd_status_ipv4_t &b)
{
    return (a.ifindex != b.ifindex || a.ifindex_port != b.ifindex_port);
}

bool operator==(const vtss_appl_ip_acd_status_ipv4_t &a, const vtss_appl_ip_acd_status_ipv4_t &b)
{
    return (a.ifindex == b.ifindex && a.ifindex_port == b.ifindex_port);
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_acd_status_ipv4_t &s)
{
    vtss_ifindex_elm_t e;

    o << "{";
    if (vtss_ifindex_decompose(s.ifindex, &e) == VTSS_RC_OK &&
        e.iftype == VTSS_IFINDEX_TYPE_VLAN) {
        o << "vid: " << e.ordinal << ", ";
    }

    if (vtss_ifindex_decompose(s.ifindex_port, &e) == VTSS_RC_OK &&
        e.iftype == VTSS_IFINDEX_TYPE_PORT) {
        o << "iport: " << e.ordinal;
    }

    o << "}";
    return o;
}

size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_acd_status_ipv4_t *st)
{
    o << *st;
    return 0;
}

/******************************************************************************/
// vtss_appl_ip_neighbor_key_t::operator<
// For insertion into status_nb_ipv4 and status_nb_ipv6
/******************************************************************************/
bool operator<(const vtss_appl_ip_neighbor_key_t &a, const vtss_appl_ip_neighbor_key_t &b)
{
    // To support RFC 4293, we first order by ifindex, then by IP type, and
    // finally by IP address.
    if (a.ifindex != b.ifindex) {
        return a.ifindex < b.ifindex;
    }

    if (a.dip.type != b.dip.type) {
        return a.dip.type < b.dip.type;
    }

    if (a.dip.type == MESA_IP_TYPE_IPV4) {
        return a.dip.addr.ipv4 < b.dip.addr.ipv4;
    }

    return a.dip.addr.ipv6 < b.dip.addr.ipv6;
}

bool operator<(const mesa_ipv6_network_t &a, const mesa_ipv6_network_t &b)
{
    if (a.address != b.address) {
        return a.address < b.address;
    }

    return a.prefix_size < b.prefix_size;
}

bool operator!=(const mesa_ipv6_network_t &a, const mesa_ipv6_network_t &b)
{
    return !vtss_ipv6_network_equal(&a, &b);
}

bool operator==(const mesa_ipv6_network_t &a, const mesa_ipv6_network_t &b)
{
    return vtss_ipv6_network_equal(&a, &b);
}

bool operator<(const mesa_ipv6_uc_t &a, const mesa_ipv6_uc_t &b)
{
    if (a.network != b.network) {
        return a.network < b.network;
    }

    return a.destination < b.destination;
}

bool operator!=(const mesa_ipv6_uc_t &a, const mesa_ipv6_uc_t &b)
{
    if (a.network != b.network) {
        return true;
    }

    return a.destination != b.destination;
}

bool operator==(const mesa_ipv6_uc_t &a, const mesa_ipv6_uc_t &b)
{
    if (a.network != b.network) {
        return false;
    }

    return a.destination == b.destination;
}

bool operator!=(const vtss_appl_ip_if_info_ipv6_t &a, const vtss_appl_ip_if_info_ipv6_t &b)
{
    // Only public part of this struct should be included in the compairson
    unsigned a_flag = a.flags & (VTSS_APPL_IP_IF_IPV6_FLAG_TENTATIVE |
                                 VTSS_APPL_IP_IF_IPV6_FLAG_DUPLICATED |
                                 VTSS_APPL_IP_IF_IPV6_FLAG_DETACHED |
                                 VTSS_APPL_IP_IF_IPV6_FLAG_NODAD |
                                 VTSS_APPL_IP_IF_IPV6_FLAG_AUTOCONF);
    unsigned b_flag = b.flags & (VTSS_APPL_IP_IF_IPV6_FLAG_TENTATIVE |
                                 VTSS_APPL_IP_IF_IPV6_FLAG_DUPLICATED |
                                 VTSS_APPL_IP_IF_IPV6_FLAG_DETACHED |
                                 VTSS_APPL_IP_IF_IPV6_FLAG_NODAD |
                                 VTSS_APPL_IP_IF_IPV6_FLAG_AUTOCONF);

    return a_flag != b_flag;
}

vtss_appl_ip_statistics_t operator-(const vtss_appl_ip_statistics_t &a, const vtss_appl_ip_statistics_t &b)
{
    vtss_appl_ip_statistics_t res = {};

#define UPDATE(name) res.name = a.name - b.name; res.name##Valid = a.name##Valid
    UPDATE(InReceives);
    UPDATE(HCInReceives);
    UPDATE(InOctets);
    UPDATE(HCInOctets);
    UPDATE(InHdrErrors);
    UPDATE(InNoRoutes);
    UPDATE(InAddrErrors);
    UPDATE(InUnknownProtos);
    UPDATE(InTruncatedPkts);
    UPDATE(InForwDatagrams);
    UPDATE(HCInForwDatagrams);
    UPDATE(ReasmReqds);
    UPDATE(ReasmOKs);
    UPDATE(ReasmFails);
    UPDATE(InDiscards);
    UPDATE(InDelivers);
    UPDATE(HCInDelivers);
    UPDATE(OutRequests);
    UPDATE(HCOutRequests);
    UPDATE(OutNoRoutes);
    UPDATE(OutForwDatagrams);
    UPDATE(HCOutForwDatagrams);
    UPDATE(OutDiscards);
    UPDATE(OutFragReqds);
    UPDATE(OutFragOKs);
    UPDATE(OutFragFails);
    UPDATE(OutFragCreates);
    UPDATE(OutTransmits);
    UPDATE(HCOutTransmits);
    UPDATE(OutOctets);
    UPDATE(HCOutOctets);
    UPDATE(InMcastPkts);
    UPDATE(HCInMcastPkts);
    UPDATE(InMcastOctets);
    UPDATE(HCInMcastOctets);
    UPDATE(OutMcastPkts);
    UPDATE(HCOutMcastPkts);
    UPDATE(OutMcastOctets);
    UPDATE(HCOutMcastOctets);
    UPDATE(InBcastPkts);
    UPDATE(HCInBcastPkts);
    UPDATE(OutBcastPkts);
    UPDATE(HCOutBcastPkts);

    res.DiscontinuityTime = a.DiscontinuityTime;
    res.RefreshRate = b.RefreshRate;
#undef UPDATE

    return res;
}

vtss_appl_ip_if_statistics_t operator-(const vtss_appl_ip_if_statistics_t &a, const vtss_appl_ip_if_statistics_t &b)
{
    vtss_appl_ip_if_statistics_t res = {};

#define UPDATE(name) res.name = a.name - b.name
    UPDATE(in_packets);
    UPDATE(out_packets);
    UPDATE(in_bytes);
    UPDATE(out_bytes);
    UPDATE(in_multicasts);
    UPDATE(out_multicasts);
    UPDATE(in_broadcasts);
    UPDATE(out_broadcasts);
#undef UPDATE

    return res;
}

char *vtss_ipv4_to_txt(char *buf, mesa_ipv4_t ip)
{
    return misc_ipv4_txt(ip, buf);
}

