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

#include "dhcp6_relay_serializer.hxx"
#include "vtss/appl/dhcp6_relay.h"

#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif
#endif

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("dhcp6RelayMib", "DHCP6-RELAY", dhcp6_relay_mib_init, VTSS_MODULE_ID_DHCP6_RELAY, root, h) {
    h.add_history_element("201804200000Z", "Initial version");
    h.description("This is a private mib for dhcp6_relays");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(objects, root, 1, "dhcp6RelayMibObjects");
NS(dhcp6_relay_config, objects, 2, "dhcp6RelayConfig");
NS(dhcp6_relay_status, objects, 3, "dhcp6RelayStatus");
NS(dhcp6_relay_control, objects, 4, "dhcp6RelayControl");
//NS(ns_traps, objects, 6, "dhcp6_relayTrap");


namespace vtss {
namespace appl {
namespace dhcp6_relay {

static TableReadWrite2<Dhcp6RelayVlanConfigEntry> dhcp6_relay_interface_conf_table(
    &dhcp6_relay_config, vtss::expose::snmp::OidElement(1, "dhcp6RelayConfigTable"));

static TableReadWrite2<Dhcp6RelayVlanStatisticsEntry> dhcp6_relay_interface_statistics_table(
    &dhcp6_relay_status, vtss::expose::snmp::OidElement(1, "dhcp6RelayStatusStatisticsTable"));

static StructRO2<Dhcp6RelayInterfaceMissinglLeaf> dhcp6_relay_statistics_leaf(
        &dhcp6_relay_status, vtss::expose::snmp::OidElement(2, "dhcp6RelayStatusStatisticsInterfaceMissingLeaf"));

static StructRW2<Dhcp6RelayControlLeaf> dhcp6_relay_conf_leaf(
        &dhcp6_relay_control, vtss::expose::snmp::OidElement(1, "dhcp6RelayControlLeaf"));

}  // namespace dhcp6_relay
}  // namespace appl
}  // namespace vtss
