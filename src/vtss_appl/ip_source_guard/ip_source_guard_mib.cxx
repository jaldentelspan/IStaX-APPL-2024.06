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

#include "ip_source_guard_serializer.hxx"
#include "vtss/appl/ip_source_guard.h"
#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#include "web_api.h"
#endif

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("ipSourceGuardMib", "IP-SOURCE-GUARD", ip_source_guard_mib_init, VTSS_MODULE_ID_IP_SOURCE_GUARD , root, h) {
    h.add_history_element("201412080000Z", "Initial version");
    h.description("This is a private version of the IP source guard MIB");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(ip_source_guard_mib_objects, root,                           1, "ipSourceGuardMibObjects");

NS(ip_source_guard_config,  ip_source_guard_mib_objects,        2, "ipSourceGuardConfig");
NS(ip_source_guard_status,  ip_source_guard_mib_objects,        3, "ipSourceGuardStatus");
NS(ip_source_guard_control, ip_source_guard_mib_objects,        4, "ipSourceGuardControl");

NS(ip_source_guard_config_interface, ip_source_guard_config,    2, "ipSourceGuardConfigInterface");
NS(ip_source_guard_config_static,    ip_source_guard_config,    3, "ipSourceGuardConfigStatic");




namespace vtss {
namespace appl {
namespace ip_source_guard {
namespace interfaces {
static StructRO2<IpSourceGuardCapabilitiesLeaf> ip_source_guard_capabilities_leaf(
        &ip_source_guard_mib_objects, vtss::expose::snmp::OidElement(1, "ipSourceGuardCapabilities"));
static StructRW2<IpSourceGuardParamsLeaf> ip_source_guard_params_leaf(
        &ip_source_guard_config, vtss::expose::snmp::OidElement(1, "ipSourceGuardConfigGlobals"));
static TableReadWrite2<IpSourceGuardInterfaceEntry> ip_source_guard_interface_entry(
        &ip_source_guard_config_interface, vtss::expose::snmp::OidElement(1, "ipSourceGuardConfigInterface"));
static TableReadWriteAddDelete2<IpSourceGuardStaticConfigEntry> ip_source_guard_static_config_entry(
        &ip_source_guard_config_static, vtss::expose::snmp::OidElement(1, "ipSourceGuardConfigStaticTable"), vtss::expose::snmp::OidElement(2, "ipSourceGuardConfigStaticTableRowEditor"));
static TableReadOnly2<IpSourceGuardDynamicStatusEntry> ip_source_guard_dynamic_status_entry(
        &ip_source_guard_status, vtss::expose::snmp::OidElement(1, "ipSourceGuardStatusDynamic"));
static StructRW2<IpSourceGuardControlLeaf> ip_source_guard_control_leaf(
        &ip_source_guard_control, vtss::expose::snmp::OidElement(1, "ipSourceGuardControlTranslate"));
}  // namespace interfaces
}  // namespace ip_source_guard
}  // namespace appl
}  // namespace vtss

