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

#include "ip_expose.hxx"
#include "ip_utils.hxx" // For operator<(vtss_appl_ip_route_status_key_t)
#include <vtss/basics/enum-descriptor.h>

const vtss_enum_descriptor_t ip_expose_dhcp4c_state_txt[] = {
    {VTSS_APPL_IP_DHCP4C_STATE_STOPPED,         "stopped"},
    {VTSS_APPL_IP_DHCP4C_STATE_INIT,            "init"},
    {VTSS_APPL_IP_DHCP4C_STATE_SELECTING,       "selecting"},
    {VTSS_APPL_IP_DHCP4C_STATE_REQUESTING,      "requesting"},
    {VTSS_APPL_IP_DHCP4C_STATE_REBINDING,       "rebinding"},
    {VTSS_APPL_IP_DHCP4C_STATE_BOUND,           "bound"},
    {VTSS_APPL_IP_DHCP4C_STATE_RENEWING,        "renewing"},
    {VTSS_APPL_IP_DHCP4C_STATE_FALLBACK,        "fallback"},
    {VTSS_APPL_IP_DHCP4C_STATE_BOUND_ARP_CHECK, "arpCheck"},
    {}
};

extern const vtss_enum_descriptor_t ip_expose_dhcp4c_id_type_txt[] = {
    {VTSS_APPL_IP_DHCP4C_ID_TYPE_AUTO,   "auto"},
    {VTSS_APPL_IP_DHCP4C_ID_TYPE_IF_MAC, "ifmac"},
    {VTSS_APPL_IP_DHCP4C_ID_TYPE_ASCII,  "ascii"},
    {VTSS_APPL_IP_DHCP4C_ID_TYPE_HEX,    "hex"},
    {}
};

const vtss_enum_descriptor_t ip_expose_route_protocol_txt[] {
    {VTSS_APPL_IP_ROUTE_PROTOCOL_KERNEL,    "protoKernel"},
    {VTSS_APPL_IP_ROUTE_PROTOCOL_DHCP,      "protoDhcp"},
    {VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED, "protoConnected"},
    {VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC,    "protoStatic"},
    {VTSS_APPL_IP_ROUTE_PROTOCOL_OSPF,      "protoOspf"},
    {VTSS_APPL_IP_ROUTE_PROTOCOL_RIP,       "protoRip"},
    {VTSS_APPL_IP_ROUTE_PROTOCOL_ANY,       "protoUnknown"},
    {}
};

