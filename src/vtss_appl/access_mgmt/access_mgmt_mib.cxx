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

#include "main.h" 
#include "vtss_appl_serialize.hxx"
#include "access_mgmt_serializer.hxx"
#include "vtss/basics/preprocessor.h"

using namespace vtss;
using namespace vtss::expose;
using namespace expose::snmp;

VTSS_MIB_MODULE("accessManagementMib", "ACCESS-MANAGEMENT",
                access_mgmt_mib_init, VTSS_MODULE_ID_ACCESS_MGMT, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201705220000Z", "Remove definition of unused OID ...AccessManagementStatus");
    h.description("This is a private version of the Access Management MIB");
}


namespace vtss {
namespace appl {
namespace access_mgmt {
namespace interfaces {

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(access_mgmt_objects, root, 1, "accessManagementMibObjects");
NS(access_mgmt_config, access_mgmt_objects, 2, "accessManagementConfig");
// NS(access_mgmt_status, access_mgmt_objects, 3, "accessManagementStatus"); // Not used, see Bz#22933
NS(access_mgmt_control, access_mgmt_objects, 4, "accessManagementControl");

static StructRW2<AccessMgmtParamsLeaf> access_mgmt_params_leaf(
        &access_mgmt_config,
        vtss::expose::snmp::OidElement(1, "accessManagementConfigGlobals"));

static TableReadWriteAddDelete2<AccessMgmtIpv4ConfigTable> access_mgmt_ipv4_config_table(
        &access_mgmt_config,
        vtss::expose::snmp::OidElement(2, "accessManagementConfigIpv4Table"),
        vtss::expose::snmp::OidElement(3, "accessManagementConfigIpv4TableRowEditor"));

#ifdef VTSS_SW_OPTION_IPV6
static TableReadWriteAddDelete2<AccessMgmtIpv6ConfigTable> access_mgmt_ipv6_config_table(
        &access_mgmt_config,
        vtss::expose::snmp::OidElement(4, "accessManagementConfigIpv6Table"),
        vtss::expose::snmp::OidElement(5, "accessManagementConfigIpv6TableRowEditor"));
#endif /* VTSS_SW_OPTION_IPV6 */

static StructRO2<AccessMgmtStatisticsLeaf> access_mgmt_statistics_leaf(
        &access_mgmt_objects,
        vtss::expose::snmp::OidElement(5, "accessManagementStatistics"));

static StructRW2<AccessMgmtCtrlStatsLeaf> access_mgmt_ctrl_stats_leaf(
        &access_mgmt_control,
        vtss::expose::snmp::OidElement(1, "accessManagementControlStatistics"));

}  // namespace interfaces
}  // namespace access_mgmt
}  // namespace appl
}  // namespace vtss
