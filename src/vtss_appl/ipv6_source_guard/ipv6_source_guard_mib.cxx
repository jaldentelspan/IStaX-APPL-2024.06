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

#include "ipv6_source_guard_serializer.hxx"
#include "vtss/appl/ipv6_source_guard.h"

#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#include "web_api.h"
#endif

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("ipv6SourceGuardMib", "IPV6-SOURCE-GUARD", ipv6_source_guard_mib_init,
                VTSS_MODULE_ID_IPV6_SOURCE_GUARD, root, h) {
    h.add_history_element("201805230000Z", "Initial version");
    h.description("This is a private version of the IPv6 Source Guard MIB");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(ipv6_source_guard_mib_objects, root, 1, "ipv6SourceGuardMibObjects");;
NS(ipv6_source_guard_config, ipv6_source_guard_mib_objects, 2, "ipv6SourceGuardConfig");;
NS(ipv6_source_guard_status, ipv6_source_guard_mib_objects, 3, "ipv6SourceGuardStatus");;
NS(ipv6_source_guard_control, ipv6_source_guard_mib_objects, 4, "ipv6SourceGuardControl");;
NS(ipv6_source_guard_statistics, ipv6_source_guard_mib_objects, 5, "ipv6SourceGuardStatistics");;

NS(ipv6_source_guard_config_interface, ipv6_source_guard_config, 2, "ipv6SourceGuardConfigInterface");;
NS(ipv6_source_guard_config_static, ipv6_source_guard_config, 3, "ipv6SourceGuardConfigStatic");;

namespace vtss {
namespace appl {
namespace ipv6_source_guard {
namespace interfaces {

static StructRW2<IPv6SourceGuardGlobalConfigLeaf> ipv6_source_guard_global_conf_leaf(
        &ipv6_source_guard_config, vtss::expose::snmp::OidElement(1, "ipv6SourceGuardConfigGlobals"));

static TableReadWrite2<IPv6SourceGuardPortConfigEntry> ipv6_source_guard_port_conf_entry(
        &ipv6_source_guard_config_interface, vtss::expose::snmp::OidElement(1, "ipv6SourceGuardConfigInterface"));

static TableReadWriteAddDelete2<Ipv6SourceGuardStaticEntry> ipv6_source_guard_static_entry(
        &ipv6_source_guard_config_static, vtss::expose::snmp::OidElement(1, "ipv6SourceGuardConfigStaticTable"), vtss::expose::snmp::OidElement(5, "ipv6SourceGuardConfigStaticTableRowEditor"));

static TableReadOnly2<Ipv6SourceGuardDynamicEntry> ipv6_source_guard_port_dynamic_entry(
        &ipv6_source_guard_status, vtss::expose::snmp::OidElement(1, "ipv6SourceGuardStatusDynamic"));

static StructRW2<Ipv6SourceGuardControlTranslateLeaf> ipv6_source_guard_control_translate_leaf(
        &ipv6_source_guard_control, vtss::expose::snmp::OidElement(1, "ipv6SourceGuardControlTranslate"));

}  // namespace interfaces
}  // namespace ipv6_source_guard
}  // namespace appl
}  // namespace vtss

