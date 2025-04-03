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

/**
 * \file dhcp6_snooping_frame.h
 * \brief This file contains DHCP frame definitions for the DHCPv6 Snooping module.
 */

#ifndef _DHCP6_SNOOPING_FRAME_H_
#define _DHCP6_SNOOPING_FRAME_H_

#include <vtss/basics/vector.hxx>

// Pull in useful frame definitions from the DHCPv6 client
#include "../dhcp6_client/base/include/vtss_dhcp6_type.hxx"
#include "../dhcp6_client/base/include/vtss_dhcp6_frame.hxx"

namespace dhcp6_snooping
{

struct EthHeader {
    mesa_mac_t              dmac;
    mesa_mac_t              smac;
    u16                     ether_type;
} VTSS_IPV6_PACK_STRUCT;

struct Ipv6ExtHeaderStart {
    u8      next_header;
    u8      length;
    u8      pl_start;
} VTSS_IPV6_PACK_STRUCT;

#define ETH_HEADER_TYPE(x)           (ntohs((x)->ether_type))

// known upper-level protocols
#define VTSS_IPV6_HEADER_NXTHDR_TCP     6
#define VTSS_IPV6_HEADER_NXTHDR_EGP     8
#define VTSS_IPV6_HEADER_NXTHDR_IGP     9
#define VTSS_IPV6_HEADER_NXTHDR_RDP     27
#define VTSS_IPV6_HEADER_NXTHDR_ISOTP4  29
#define VTSS_IPV6_HEADER_NXTHDR_IPv6    41
#define VTSS_IPV6_HEADER_NXTHDR_RSVP    46
#define VTSS_IPV6_HEADER_NXTHDR_GRE     47
#define VTSS_IPV6_HEADER_NXTHDR_ICMP    58

// Extension header types
#define IPV6_EXT_HEADER_HOP_BY_HOP      0
#define IPV6_EXT_HEADER_ROUTING         43
#define IPV6_EXT_HEADER_FRAGMENT        44
#define IPV6_EXT_HEADER_ESP             50
#define IPV6_EXT_HEADER_AUTH            51
#define IPV6_EXT_HEADER_NO_NEXT         59
#define IPV6_EXT_HEADER_DEST_OPT        60
#define IPV6_EXT_HEADER_MOBILITY        135

#define IPV6_MAX_EXT_HEADER_DEPTH       10

} // namespace dhcp6_snooping

#endif /* _DHCP6_SNOOPING_FRAME_H_ */

