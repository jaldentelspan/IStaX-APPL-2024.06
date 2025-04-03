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

#include "snmp.hxx"  // For CYG_HTTPD_STATE
#include "vtss/appl/mac.h"
#include "mac_serializer.hxx"
#include "vtss/appl/vlan.h" /* For VTSS_APPL_VLAN_ID_DEFAULT */

VTSS_MIB_MODULE("macMib", "MAC", mac_mib_init, VTSS_MODULE_ID_MAC, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201408200000Z", "Updated types");
    h.add_history_element("201702230000Z", "Added support for SR and PSFP.");
    h.add_history_element("201905290000Z", "Removed support for SR and PSFP.");
    h.description("This is a private version of the MAC MIB");
}

using namespace vtss;
using namespace expose::snmp;

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(objects, root, 1, "macMibObjects");
NS(mac_config, objects, 2, "macConfig");
NS(mac_status, objects, 3, "macStatus");

namespace vtss {
namespace appl {
namespace mac {
namespace interfaces {
static StructRO2<MacCapLeaf> mac_cap_leaf(
        &objects, vtss::expose::snmp::OidElement(1, "macCapabilities"));
static StructRW2<MacAgeLeaf> mac_age_leaf(
        &mac_config, vtss::expose::snmp::OidElement(1, "macConfigFdbGlobal"));
static TableReadWriteAddDelete2<MacAddressConfigTableImpl>
        mac_address_config_table_impl(
                &mac_config, vtss::expose::snmp::OidElement(2, "macConfigFdbTable"),
                vtss::expose::snmp::OidElement(3, "macConfigFdbTableRowEditor"));
static TableReadWrite2<MacAddressLearnTableImpl> mac_address_learn_table_impl(
        &mac_config, vtss::expose::snmp::OidElement(4, "macConfigPortLearn"));
static TableReadWrite2<MacAddressVlanLearnTableImpl>
        mac_address_vlan_learn_table_impl(
                &mac_config, vtss::expose::snmp::OidElement(5, "macConfigVlanLearn"));
static TableReadOnly2<MacFdbTableImpl> mac_fdb_table_impl(
        &mac_status, vtss::expose::snmp::OidElement(1, "macStatusFdbTable"));
static TableReadOnly2<MacFdbTableStaticImpl> mac_fdb_table_static_impl(
        &mac_status, vtss::expose::snmp::OidElement(2, "macStatusFdbStaticTable"));
static TableReadOnly2<MacAddressStatsTableImpl> mac_address_stats_table_impl(
        &mac_status, vtss::expose::snmp::OidElement(3, "macStatusFdbPortStatistics"));
static StructRO2<MacStatisLeaf> mac_statis_leaf(
        &mac_status, vtss::expose::snmp::OidElement(4, "macStatusFdbStatistics"));
static StructRW2<MacFlushLeaf> mac_flush_leaf(
        &objects, vtss::expose::snmp::OidElement(4, "macControl"));
}  // namespace interfaces
}  // namespace mac
}  // namespace appl
}  // namespace vtss

