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

#ifndef _FRR_ACCESS_IP_ROUTE_HXX_
#define _FRR_ACCESS_IP_ROUTE_HXX_

#include <vtss/appl/ip.h>
#include <vtss/basics/map.hxx>

mesa_rc frr_ip_route_conf_set(const vtss_appl_ip_route_key_t &key, const vtss_appl_ip_route_conf_t &conf);
mesa_rc frr_ip_route_conf_del(const vtss_appl_ip_route_key_t &key);
mesa_rc frr_ip_route_status_get(vtss_appl_ip_route_status_map_t &routes, vtss_appl_ip_route_type_t route_type, vtss_appl_ip_route_protocol_t protocol);

#endif  // _FRR_ACCESS_IP_ROUTE_HXX_

