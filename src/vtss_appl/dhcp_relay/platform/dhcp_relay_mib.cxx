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
#include "dhcp_relay_serializer.hxx"
#include "vtss/appl/dhcp_relay.h"

VTSS_MIB_MODULE("dhcpRelayMib", "DHCP-RELAY", dhcp_relay_mib_init,
                VTSS_MODULE_ID_DHCP_RELAY, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201410100000Z", "Editorial changes");
    h.description("This is a private version of the DHCP Relay MIB");
}


using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace dhcp_relay {
namespace interfaces {

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(dhcp_relay_mib_objects, root, 1, "dhcpRelayMibObjects");
NS(dhcp_relay_config, dhcp_relay_mib_objects, 2, "dhcpRelayConfig");
NS(dhcp_relay_status, dhcp_relay_mib_objects, 3, "dhcpRelayStatus");

static StructRW2<DhcpRelayParamsLeaf> dhcp_relay_params_leaf(
        &dhcp_relay_config,
        vtss::expose::snmp::OidElement(1, "dhcpRelayConfigGlobals"));

static StructRO2<DhcpRelayStatisticsLeaf> dhcp_relay_statistics_leaf(
        &dhcp_relay_status,
        vtss::expose::snmp::OidElement(1, "dhcpRelayStatusStatistics"));

static StructRW2<DhcpRelayControlLeaf> dhcp_relay_control_leaf(
        &dhcp_relay_mib_objects, vtss::expose::snmp::OidElement(4, "dhcpRelayControl"));

}  // namespace interfaces
}  // namespace dhcp_relay
}  // namespace appl
}  // namespace vtss

