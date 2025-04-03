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

#ifndef _IP_UTILS_HXX_
#define _IP_UTILS_HXX_

#include <vtss/appl/ip.h>
#include <vtss/basics/stream.hxx>
#include <vtss/basics/memcmp-operator.hxx>
#include "misc_api.h"

#define VTSS_IPV4_FORMAT "%u.%u.%u.%u"
#define VTSS_IPV4_ARGS(X) (((X) >> 24) & 0xff), (((X) >> 16) & 0xff), (((X) >> 8) & 0xff), ((X)&0xff)

#define VTSS_IPV4N_FORMAT VTSS_IPV4_FORMAT "/%d"
#define VTSS_IPV4N_ARG(X) VTSS_IPV4_ARGS((X).address), (X).prefix_size

extern mesa_ipv4_t vtss_ipv4_blackhole_route;
extern mesa_ipv6_t vtss_ipv6_blackhole_route;

typedef enum {
    VTSS_IP_CHECKSUM_TYPE_NORMAL, /**< Normal IPv4 checksum                                                                  */
    VTSS_IP_CHECKSUM_TYPE_UDLD,   /**< IP-like checksum with a possible last lonely byte added to the LSByte of the checksum */
    VTSS_IP_CHECKSUM_TYPE_CDP,    /**< IP-like checksum with signed add of last lonely byte                                  */
} vtss_ip_checksum_type_t;

// Calculate an IPv4 header checksum across a number of bytes.
//
// frm points to the beginning of the IPv4 header and len is the size of the
// IPv4 header. frm is expected to be in network order. The return value is in
// host order.
//
// Some protocols use IP-like checksums, with the exception that if the length
// is odd, the last byte is used in different ways.
//
// In UDLD, for instance, the last byte is used as the lower 8 bits in the
// computation instead of the higher eight bits (as in IP header).
//
// In CDP, for instance, the last byte must be added as a *signed* number and
// used as the lower 8 bits instead of the higher 8 bits in the 16-bit
// computation.
//
// The behavior of this function is controlled with the last argument.
//
// When the function is used to verify a checksum and the checksum to verify is
// included in frm, the result will be 0xffff only if the checksum is OK.
//
// This function may be used to calculate other checksums, e.g. IGMP checksums.
uint16_t vtss_ip_checksum(const uint8_t *frm, uint16_t len, vtss_ip_checksum_type_t checksum_type = VTSS_IP_CHECKSUM_TYPE_NORMAL);

// Calculate the checksum of an IPv4 or IPv6 pseudo header and a possible
// accompanying L4 datagram. This is typically used by UDP or TCP checksum
// calculations, but may also be used by other protocols like MLD.
//
// frm points to the beginning of the L4 data (e.g. UDP or TCP header).
// Note that the checksum inside the UDP/TCP header must be cleared before
// calling this function if you are generating the checksum. If you are checking
// the checkum, leave it as it was received. In that case, the returned checksum
// is 0xffff if the checksum is correct.
// frm may be nullptr if you don't want to include any L4 data in the
// calculations (very unusual).
//
// len is the length of the L4 datagram in bytes, including any L4 header.
//
// sip and dip are the source and destination IP addresses (used to computed the
// pseudo header checksum).
//
// protocol could be e.g. IP_PROTO_TCP (6) or IP_PROTO_UDP (17), but any value
// is accepted (e.g. 56 for ICMPv6 next-header, which is used in MLD).
//
// The function returns the checksum folded to 16 bits in host order. The caller
// is responsible for converting this to network order (with htons()).
uint16_t vtss_ip_pseudo_header_checksum(const uint8_t *frm, uint16_t len, const mesa_ip_addr_t &sip, const mesa_ip_addr_t &dip, uint8_t protocol);

/* Returns TRUE if IPv4/v6 is a zero-address. FALSE otherwise. */
BOOL vtss_ip_addr_is_zero(          const mesa_ip_addr_t *addr);
BOOL vtss_ipv4_addr_is_unicast(     const mesa_ipv4_t    *addr);
BOOL vtss_ipv4_addr_is_multicast(   const mesa_ipv4_t    *addr);
BOOL vtss_ipv4_addr_is_zero(        const mesa_ipv4_t    *addr);
BOOL vtss_ipv4_addr_is_loopback(    const mesa_ipv4_t    *addr);
BOOL vtss_ipv6_addr_is_multicast(   const mesa_ipv6_t    *addr);
bool vtss_ipv4_addr_is_routable(    const mesa_ipv4_t    *addr);
BOOL vtss_ipv6_addr_is_zero(        const mesa_ipv6_t    *addr);
BOOL vtss_ipv6_addr_is_loopback(    const mesa_ipv6_t    *addr);
BOOL vtss_ipv6_addr_is_link_local(  const mesa_ipv6_t    *addr);
BOOL vtss_ipv6_addr_is_mgmt_support(const mesa_ipv6_t    *addr);
bool vtss_ipv6_addr_is_routable(    const mesa_ipv6_t    *addr);
void vtss_ipv6_addr_link_local_get(mesa_ipv6_t *ipv6, const mesa_mac_t *mac = nullptr);

// using the class system to derive a prefix
int vtss_ipv4_addr_to_prefix(mesa_ipv4_t ip);

/* Type conversion --------------------------------------------------------- */

mesa_rc     vtss_prefix_cnt(const u8 *data, const u32 length, u32 *prefix);
mesa_rc     vtss_conv_ipv4mask_to_prefix(const mesa_ipv4_t ipv4, u32 *const prefix);
mesa_ipv4_t vtss_ipv4_prefix_to_mask(uint32_t prefix);
mesa_rc     vtss_conv_ipv6mask_to_prefix(const mesa_ipv6_t *ipv6, u32 *prefix);
mesa_rc     vtss_conv_prefix_to_ipv6mask(const u32 prefix, mesa_ipv6_t *const ipv6);
mesa_rc     vtss_build_ipv4_network(mesa_ipv4_network_t *network, const mesa_ipv4_t address, const mesa_ipv4_t mask);
mesa_rc     vtss_appl_ip_if_status_to_ip(const vtss_appl_ip_if_status_t *const status, mesa_ip_addr_t *const ip);

BOOL vtss_ip_ipv4_ifaddr_valid(const mesa_ipv4_network_t *const net);
BOOL vtss_ip_ipv6_ifaddr_valid(const mesa_ipv6_network_t *const net);
BOOL vtss_ip_ifaddr_valid(const mesa_ip_network_t *const net);

/* Various type equal checks ----------------------------------------------- */

bool vtss_ipv4_network_equal(const mesa_ipv4_network_t *a, const mesa_ipv4_network_t *b);
bool vtss_ipv6_network_equal(const mesa_ipv6_network_t *a, const mesa_ipv6_network_t *b);
bool vtss_ipv4_net_equal(    const mesa_ipv4_network_t *a, const mesa_ipv4_network_t *b);
bool vtss_ipv6_net_equal(    const mesa_ipv6_network_t *a, const mesa_ipv6_network_t *b);
bool vtss_ip_net_equal(      const mesa_ip_network_t   *a, const mesa_ip_network_t   *b);

mesa_ipv4_network_t vtss_ipv4_net_mask_out(const mesa_ipv4_network_t *n);
mesa_ipv6_network_t vtss_ipv6_net_mask_out(const mesa_ipv6_network_t *n);
mesa_ip_network_t   vtss_ip_net_mask_out(  const mesa_ip_network_t   *n);

bool vtss_ipv4_net_overlap(const mesa_ipv4_network_t *a,   const mesa_ipv4_network_t *b);
bool vtss_ipv6_net_overlap(const mesa_ipv6_network_t *a,   const mesa_ipv6_network_t *b);
bool vtss_ip_net_overlap(  const mesa_ip_network_t   *a,   const mesa_ip_network_t   *b);
bool vtss_ipv4_net_include(const mesa_ipv4_network_t *net, const mesa_ipv4_t         *addr);
bool vtss_ipv6_net_include(const mesa_ipv6_network_t *net, const mesa_ipv6_t         *addr);

bool operator<(const mesa_routing_entry_t &a, const mesa_routing_entry_t &b);

/* Printers */
int   vtss_ip_if_status_to_txt(         char *buf, int size, const vtss_appl_ip_if_status_t    *st, u32 length);
int   vtss_ip_ip_addr_to_txt(           char *buf, int size, const mesa_ip_addr_t              *ip);
int   vtss_if_link_flag_to_txt(         char *buf, int size, vtss_appl_ip_if_link_flags_t      f);
int   vtss_if_ipv6_flag_to_txt(         char *buf, int size, vtss_appl_ip_if_ipv6_flags_t      f);
char *vtss_ip_route_status_flags_to_txt(char *buf, int size, vtss_appl_ip_route_status_flags_t f);

// These are for outputting structures to a stream (used e.g. by VTSS_TRACE())
vtss::ostream &operator<<(vtss::ostream &o, const mesa_ipv4_uc_t                    &r);
vtss::ostream &operator<<(vtss::ostream &o, const mesa_ipv6_uc_t                    &r);
vtss::ostream &operator<<(vtss::ostream &o, const mesa_routing_entry_t              &r);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_route_key_t          &r);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_route_conf_t         &conf);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_route_status_flags_t &flags);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_route_status_key_t   &k);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_route_status_t       &st);
vtss::ostream &operator<<(vtss::ostream &o,       vtss_appl_ip_if_link_flags_t      f);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_status_link_t     &s);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_status_ipv4_t     &s);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_status_ipv6_t     &s);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_status_dhcp4c_t   &s);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_status_t          &s);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_neighbor_key_t       &k);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_neighbor_status_t    &st);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_conf_t            &conf);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_conf_ipv4_t       &conf);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_conf_ipv6_t       &conf);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_key_ipv4_t        &key);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_info_ipv4_t       &info);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_key_ipv6_t        &key);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_if_info_ipv6_t       &info);

// These are for outputting to a stream (used e.g. by T_D())
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_route_key_t           *rt);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_route_conf_t          *conf);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_route_status_key_t    *k);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_route_status_t        *st);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_ipv4_uc_t                     *r);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_ipv6_uc_t                     *r);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_routing_entry_t               *r);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss::Ipv4Network                  *r);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_neighbor_key_t        *key);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_neighbor_status_t     *st);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_acd_status_ipv4_key_t *key);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_conf_t             *conf);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_conf_ipv4_t        *conf);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_conf_ipv6_t        *conf);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_key_ipv4_t         *key);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_info_ipv4_t        *info);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_key_ipv6_t         *key);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_info_ipv6_t        *info);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_if_status_link_t      *st);

VTSS_BASICS_MEMCMP_OPERATOR(mesa_ipv6_t);
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_ip_if_status_link_t);
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_ip_if_info_ipv4_t);
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_ip_if_status_dhcp4c_t);

bool operator<( const vtss_appl_ip_if_key_ipv4_t  &a, const vtss_appl_ip_if_key_ipv4_t  &b);
bool operator<( const vtss_appl_ip_if_key_ipv6_t  &a, const vtss_appl_ip_if_key_ipv6_t  &b);
bool operator<( const vtss_appl_ip_neighbor_key_t &a, const vtss_appl_ip_neighbor_key_t &b);

bool operator!=(const vtss_appl_ip_if_info_ipv6_t &a, const vtss_appl_ip_if_info_ipv6_t &b);

bool operator<( const mesa_ipv4_network_t &a, const mesa_ipv4_network_t &b);
bool operator==(const mesa_ipv4_network_t &a, const mesa_ipv4_network_t &b);
bool operator!=(const mesa_ipv4_network_t &a, const mesa_ipv4_network_t &b);

bool operator<( const mesa_ipv4_uc_t &a, const mesa_ipv4_uc_t &b);
bool operator!=(const mesa_ipv4_uc_t &a, const mesa_ipv4_uc_t &b);
bool operator==(const mesa_ipv4_uc_t &a, const mesa_ipv4_uc_t &b);

bool operator<( const mesa_ipv6_network_t &a, const mesa_ipv6_network_t &b);
bool operator!=(const mesa_ipv6_network_t &a, const mesa_ipv6_network_t &b);
bool operator==(const mesa_ipv6_network_t &a, const mesa_ipv6_network_t &b);

bool operator<(const mesa_ipv6_uc_t &a, const mesa_ipv6_uc_t &b);
bool operator!=(const mesa_ipv6_uc_t &a, const mesa_ipv6_uc_t &b);
bool operator==(const mesa_ipv6_uc_t &a, const mesa_ipv6_uc_t &b);

bool operator<( const vtss_appl_ip_acd_status_ipv4_key_t &a,  const vtss_appl_ip_acd_status_ipv4_key_t &b);
bool operator!=(const vtss_appl_ip_acd_status_ipv4_key_t &a, const vtss_appl_ip_acd_status_ipv4_key_t &b);
bool operator==(const vtss_appl_ip_acd_status_ipv4_key_t &a, const vtss_appl_ip_acd_status_ipv4_key_t &b);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_acd_status_ipv4_key_t &k);

bool operator<( const vtss_appl_ip_acd_status_ipv4_t &a, const vtss_appl_ip_acd_status_ipv4_t &b);
bool operator!=(const vtss_appl_ip_acd_status_ipv4_t &a, const vtss_appl_ip_acd_status_ipv4_t &b);
bool operator==(const vtss_appl_ip_acd_status_ipv4_t &a, const vtss_appl_ip_acd_status_ipv4_t &b);
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ip_acd_status_ipv4_t &s);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ip_acd_status_ipv4_t *st);

VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_ip_statistics_t);
vtss_appl_ip_statistics_t operator-(
    const vtss_appl_ip_statistics_t &a,
    const vtss_appl_ip_statistics_t &b);

vtss_appl_ip_if_statistics_t operator-(const vtss_appl_ip_if_statistics_t &a, const vtss_appl_ip_if_statistics_t &b);

bool operator<(const vtss_appl_ip_route_key_t        &a, const vtss_appl_ip_route_key_t        &b);
bool operator<(const vtss_appl_ip_route_status_key_t &a, const vtss_appl_ip_route_status_key_t &b);

typedef int vtss_ip_cli_pr(const char *fmt, ...);
const char *ip_util_route_protocol_to_letter(vtss_appl_ip_route_protocol_t protocol);
const char *ip_util_route_protocol_to_str(vtss_appl_ip_route_protocol_t protocol, bool capital_first_letter);
mesa_rc ip_util_route_warning_print(vtss_ip_cli_pr *pr);
mesa_rc ip_util_route_print(vtss_appl_ip_route_type_t type, vtss_ip_cli_pr *pr);
int ip_util_if_print(vtss_ip_cli_pr *pr, vtss_appl_ip_if_status_type_t type, bool vlan_only);
int ip_util_if_brief_print(mesa_ip_type_t type, vtss_ip_cli_pr *pr);
int ip_util_nb_print(mesa_ip_type_t type, vtss_ip_cli_pr *pr);

char *vtss_ipv4_to_txt(char *buf, mesa_ipv4_t ip);

/**
 * This structure facilitates computations and calculations on IPv6 addresses.
 * It is a kind of uint128_t.
 * For code that supports both IPv4 and IPv6, this structure may be used on IPv4
 * addresses as well.
 */
struct ipv6_calc_t {
    ipv6_calc_t(mesa_ipv6_t v)
    {
        hi = (uint64_t)v.addr[ 0] << 56 |
             (uint64_t)v.addr[ 1] << 48 |
             (uint64_t)v.addr[ 2] << 40 |
             (uint64_t)v.addr[ 3] << 32 |
             (uint64_t)v.addr[ 4] << 24 |
             (uint64_t)v.addr[ 5] << 16 |
             (uint64_t)v.addr[ 6] <<  8 |
             (uint64_t)v.addr[ 7] <<  0;

        lo = (uint64_t)v.addr[ 8] << 56 |
             (uint64_t)v.addr[ 9] << 48 |
             (uint64_t)v.addr[10] << 40 |
             (uint64_t)v.addr[11] << 32 |
             (uint64_t)v.addr[12] << 24 |
             (uint64_t)v.addr[13] << 16 |
             (uint64_t)v.addr[14] <<  8 |
             (uint64_t)v.addr[15] <<  0;
    }

    ipv6_calc_t(mesa_ipv4_t v)        : hi(0),    lo(v) {}
    ipv6_calc_t()                     : hi(0),    lo(0) {}
    ipv6_calc_t(uint64_t v)           : hi(0),    lo(v) {}
    ipv6_calc_t(const ipv6_calc_t &v) : hi(v.hi), lo(v.lo) {}

    mesa_ipv4_t to_mesa_ipv4()
    {
        return (mesa_ipv4_t)lo;
    }

    mesa_ipv6_t to_mesa_ipv6()
    {
        mesa_ipv6_t v;

        v.addr[ 0] = hi >> 56;
        v.addr[ 1] = hi >> 48;
        v.addr[ 2] = hi >> 40;
        v.addr[ 3] = hi >> 32;
        v.addr[ 4] = hi >> 24;
        v.addr[ 5] = hi >> 16;
        v.addr[ 6] = hi >>  8;
        v.addr[ 7] = hi >>  0;
        v.addr[ 8] = lo >> 56;
        v.addr[ 9] = lo >> 48;
        v.addr[10] = lo >> 40;
        v.addr[11] = lo >> 32;
        v.addr[12] = lo >> 24;
        v.addr[13] = lo >> 16;
        v.addr[14] = lo >>  8;
        v.addr[15] = lo >>  0;
        return v;
    }

    char *print(char *buf, bool as_ipv4 = false)
    {
        // Buf must be at least IPV6_ADDR_IBUF_MAX_LEN bytes long.
        mesa_ipv6_t v;

        if (as_ipv4) {
            return misc_ipv4_txt(this->to_mesa_ipv4(), buf);
        }

        v = to_mesa_ipv6();
        return misc_ipv6_txt(&v, buf);
    }

    bool operator==(const ipv6_calc_t &other) const
    {
        return hi == other.hi && lo == other.lo;
    }

    bool operator<(const ipv6_calc_t &other) const
    {
        return hi == other.hi ? lo < other.lo : hi < other.hi;
    }

    bool operator>(const ipv6_calc_t &other) const
    {
        return hi == other.hi ? lo > other.lo : hi > other.hi;
    }

    ipv6_calc_t operator~() const
    {
        ipv6_calc_t v(*this);
        v.lo = ~v.lo;
        v.hi = ~v.hi;
        return v;
    }

    // Prefix operator
    ipv6_calc_t &operator++()
    {
        if (++lo == 0) {
            hi++;
        }

        return *this;
    }

    // Postfix operator
    ipv6_calc_t operator++(int)
    {
        ipv6_calc_t v(*this);
        ++(*this);

        return v;
    }

    // Prefix operator
    ipv6_calc_t &operator--()
    {
        if (lo-- == 0) {
            hi--;
        }

        return *this;
    }

    // Postfix operator
    ipv6_calc_t operator--(int)
    {
        ipv6_calc_t v(*this);
        --(*this);

        return v;
    }

    ipv6_calc_t &operator+=(const ipv6_calc_t &v)
    {
        const uint64_t old_lo = lo;

        lo += v.lo;
        hi += v.hi;

        if (lo < old_lo) {
            hi++;
        }

        return *this;
    }

    ipv6_calc_t &operator-=(const ipv6_calc_t &b)
    {
        *this += -b;
        return *this;
    }

    ipv6_calc_t operator-() const
    {
        ipv6_calc_t t = ~(*this);

        t += (uint64_t)1;
        return t;
    }

    ipv6_calc_t operator+(uint64_t v)
    {
        ipv6_calc_t res(*this);
        res += v;
        return res;
    }

    ipv6_calc_t operator-(uint64_t v)
    {
        ipv6_calc_t res(*this);
        bool        carry = v > lo;

        res.lo -= v;
        res.hi -= carry ? 1 : 0;
        return res;
    }

    ipv6_calc_t operator-(ipv6_calc_t v)
    {
        ipv6_calc_t res(*this);
        bool        carry = v.lo > lo;

        res.lo -= v.lo;
        res.hi -= carry ? 1 : 0;
        res.hi -= v.hi;
        return res;
    }

private:
    uint64_t hi;
    uint64_t lo;
};

#endif /* _IP_UTILS_HXX_ */

