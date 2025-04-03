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

#include "dhcp6_snooping_serializer.hxx"
#include "vtss/appl/dhcp6_snooping.h"
#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#include "web_api.h"
#endif

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("dhcp6SnoopingMib", "DHCP6-SNOOPING", dhcp6_snooping_mib_init,
                VTSS_MODULE_ID_DHCP6_SNOOPING, root, h) {
    h.add_history_element("201805250000Z", "Initial version");
    h.description("This is a private version of the DHCPv6 Snooping MIB");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(dhcp6_snooping_mib_objects, root, 1, "dhcp6SnoopingMibObjects");;
NS(dhcp6_snooping_config, dhcp6_snooping_mib_objects, 2, "dhcp6SnoopingConfig");;
NS(dhcp6_snooping_status, dhcp6_snooping_mib_objects, 3, "dhcp6SnoopingStatus");;
NS(dhcp6_snooping_statistics, dhcp6_snooping_mib_objects, 5, "dhcp6SnoopingStatistics");;
NS(dhcp6_snooping_control, dhcp6_snooping_mib_objects, 4, "dhcp6SnoopingControl");;

namespace vtss {
namespace appl {
namespace dhcp6_snooping {
namespace interfaces {
static StructRW2<Dhcp6SnoopingParamsLeaf> dhcp6_snooping_params_leaf(
        &dhcp6_snooping_config, vtss::expose::snmp::OidElement(1, "dhcp6SnoopingConfigGlobals"));
static TableReadWrite2<Dhcp6SnoopingInterfaceEntry> dhcp6_snooping_interface_entry(
        &dhcp6_snooping_config, vtss::expose::snmp::OidElement(2, "dhcp6SnoopingConfigInterface"));
static StructRO2<Dhcp6SnoopingGlobalStatusImpl> dhcp6_snooping_global_status_impl(
        &dhcp6_snooping_status, vtss::expose::snmp::OidElement(1, "dhcp6SnoopingStatusGlobals"));
static TableReadOnly2<Dhcp6SnoopingClientInfoEntry> dhcp6_snooping_client_table_entry(
        &dhcp6_snooping_status, vtss::expose::snmp::OidElement(2, "dhcp6SnoopingStatusClientTable"));
static TableReadOnly2<Dhcp6SnoopingAddressInfoEntry> dhcp6_snooping_address_table_entry(
        &dhcp6_snooping_status, vtss::expose::snmp::OidElement(3, "dhcp6SnoopingStatusAddressTable"));
static TableReadWrite2<Dhcp6SnoopingInterfaceStatisticsEntry> dhcp6_snooping_interface_statistics_entry(
        &dhcp6_snooping_statistics, vtss::expose::snmp::OidElement(1, "dhcp6SnoopingStatisticsInterfaceTable"));
}  // namespace interfaces
}  // namespace dhcp6_snooping
}  // namespace appl
}  // namespace vtss
