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

#ifndef __MAC_UTILS_HXX__
#define __MAC_UTILS_HXX__

#include <microchip/ethernet/switch/api.h>
#include <vtss/basics/stream.hxx>
#include <vtss/basics/print_fmt.hxx>

extern mesa_mac_t mac_broadcast;

bool operator<( const mesa_mac_t     &a, const mesa_mac_t &b);
bool operator!=(const mesa_mac_t     &a, const mesa_mac_t &b);
bool operator==(const mesa_mac_t     &a, const mesa_mac_t &b);
bool operator<( const mesa_vid_mac_t &a, const mesa_vid_mac_t &b);
mesa_mac_t &operator&=(mesa_mac_t &a, const mesa_mac_t &b);

bool mac_is_unicast(  const mesa_mac_t &m);
bool mac_is_multicast(const mesa_mac_t &m);
bool mac_is_broadcast(const mesa_mac_t &m);
bool mac_is_zero(     const mesa_mac_t &m);

// These are for outputting structures to a stream (used e.g. by VTSS_TRACE())
vtss::ostream &operator<<(vtss::ostream &o, const mesa_vid_mac_t &i);

// These are for outputting to a stream (used e.g. by T_D())
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_vid_mac_t *r);

#endif // __MAC_UTILS_HXX__

