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

#include "snmp_serializer.hxx"
VTSS_MIB_MODULE("snmpMib", "SNMP", snmp_mib_init, VTSS_MODULE_ID_SNMP , root, h) {
    h.add_history_element("201407180000Z", "Initial version");
    h.add_history_element("201507240000Z",
            "Correct the valid length of AuthPassword and PrivPassword, and correct "
            "the description of vtssSnmpConfigViewViewType");
    h.add_history_element("201512110000Z",
            "Remove version and communities from vtssSnmpConfigGlobals, and update "
            "SnmpConfigCommunityTable with security_name");
    h.add_history_element("201602110000Z",
            "Add SourceIpType as well as SourceIPv6 / SourceIPv6PrefixSize to "
            "SnmpConfigCommunityTable.");
    h.add_history_element("201602230000Z",
            "Move IPv6 enabled communities to seperate SnmpConfigCommunity6Table. "
            "Rename community fields for backward compatibility.");
    h.add_history_element("201603070000Z",
            "Add trap receiver and source tables.");
    h.add_history_element("201604060000Z",
            "Improve descriptions for use of security_model. Change trap source table.");
    h.description("This is a private version of SNMP");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

using namespace vtss;
using namespace expose::snmp;

namespace vtss {
namespace appl {
namespace snmp {
namespace interfaces {
NS(snmp_mib_objects, root, 1, "snmpMibObjects");;
NS(snmp_config, snmp_mib_objects, 2, "snmpConfig");;

static StructRW2<SnmpParamsLeaf> snmp_params_leaf(
        &snmp_config, vtss::expose::snmp::OidElement(1, "SnmpConfigGlobals"));

static TableReadWriteAddDelete2<SnmpCommunityEntry> snmp_community_entry(
        &snmp_config, vtss::expose::snmp::OidElement(2, "SnmpConfigCommunityTable"), vtss::expose::snmp::OidElement(3, "SnmpConfigCommunityTableRowEditor"));

static TableReadWriteAddDelete2<SnmpUserEntry> snmp_user_entry(
        &snmp_config, vtss::expose::snmp::OidElement(4, "SnmpConfigUserTable"), vtss::expose::snmp::OidElement(5, "SnmpConfigUserTableRowEditor"));

static TableReadWriteAddDelete2<SnmpGroupEntry> snmp_group_entry(
        &snmp_config, vtss::expose::snmp::OidElement(6, "SnmpConfigUserToAccessGroupTable"), vtss::expose::snmp::OidElement(7, "SnmpConfigUserToAccessGroupTableRowEditor"));

static TableReadWriteAddDelete2<SnmpAccessEntry> snmp_access_entry(
        &snmp_config, vtss::expose::snmp::OidElement(8, "SnmpConfigAccessGroupTable"), vtss::expose::snmp::OidElement(9, "SnmpConfigAccessGroupTableRowEditor"));

static TableReadWriteAddDelete2<SnmpViewEntry> snmp_view_entry(
        &snmp_config, vtss::expose::snmp::OidElement(10, "SnmpConfigViewTable"), vtss::expose::snmp::OidElement(11, "SnmpConfigViewTableRowEditor"));

static TableReadWriteAddDelete2<SnmpCommunity6Entry> snmp_community6_entry(
        &snmp_config, vtss::expose::snmp::OidElement(12, "SnmpConfigCommunity6Table"), vtss::expose::snmp::OidElement(13, "SnmpConfigCommunity6TableRowEditor"));

static TableReadWriteAddDelete2<SnmpTrapReceiverEntry> snmp_trap_receiver_entry(
        &snmp_config, vtss::expose::snmp::OidElement(20, "SnmpConfigTrapReceiverTable"), vtss::expose::snmp::OidElement(21, "SnmpConfigTrapReceiverTableRowEditor"));

static TableReadWriteAddDelete2<SnmpTrapSourceEntry> snmp_trap_source_entry(
        &snmp_config, vtss::expose::snmp::OidElement(22, "SnmpConfigTrapSourceTable"), vtss::expose::snmp::OidElement(23, "SnmpConfigTrapSourceTableRowEditor"));

}  // namespace interfaces
}  // namespace snmp
}  // namespace appl
}  // namespace vtss
