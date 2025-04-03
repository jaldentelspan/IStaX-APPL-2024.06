/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "dhcp_server_serializer.hxx"

const vtss_enum_descriptor_t dhcp_server_pool_type_txt[] {
    {VTSS_APPL_DHCP_SERVER_POOL_TYPE_NONE,      "none"},
    {VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK,   "network"},
    {VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST,      "host"},
    {0, 0},
};

const vtss_enum_descriptor_t dhcp_server_netbios_node_type_txt[] {
    {VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_NONE,  "nodeNone"},
    {VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_B,     "nodeB"},
    {VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_P,     "nodeP"},
    {VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_M,     "nodeM"},
    {VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_H,     "nodeH"},
    {0, 0},
};

const vtss_enum_descriptor_t dhcp_server_client_identifier_type_txt[] {
    {VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE, "none"},
    {VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NAME, "name"},
    {VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_MAC,  "mac"},
    {0, 0},
};

const vtss_enum_descriptor_t dhcp_server_binding_type_txt[] {
    {VTSS_APPL_DHCP_SERVER_BINDING_TYPE_NONE,       "none"},
    {VTSS_APPL_DHCP_SERVER_BINDING_TYPE_AUTOMATIC,  "automatic"},
    {VTSS_APPL_DHCP_SERVER_BINDING_TYPE_MANUAL,     "manual"},
    {VTSS_APPL_DHCP_SERVER_BINDING_TYPE_EXPIRED,    "expired"},
    {0, 0},
};

const vtss_enum_descriptor_t dhcp_server_binding_state_txt[] {
    {VTSS_APPL_DHCP_SERVER_BINDING_STATE_NONE,      "none"},
    {VTSS_APPL_DHCP_SERVER_BINDING_STATE_ALLOCATED, "allocated"},
    {VTSS_APPL_DHCP_SERVER_BINDING_STATE_COMMITTED, "committed"},
    {VTSS_APPL_DHCP_SERVER_BINDING_STATE_EXPIRED,   "expired"},
    {0, 0},
};

