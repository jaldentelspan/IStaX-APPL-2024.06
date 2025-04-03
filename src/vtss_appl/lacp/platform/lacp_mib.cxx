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

#include "lacp_serializer.hxx"
#include "vtss/appl/lacp.h"
#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#include "web_api.h"
#endif

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("lacpMib", "LACP", lacp_mib_init, VTSS_MODULE_ID_LACP, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201411140000Z", "Added a new leaf for LACP system priority");
    h.add_history_element("201704060000Z", "Changed lacpPortConfigTable: 1) Made dot3adAggrActorAdminMode read-only "
                          "as this parameter is now controlled by the AGGR-MIB. 2) Made "
                          "dot3adAggrRole object read-only as this is not a port-level but "
                          "group-level property. Added new objects to lacpPortStatusTable.");
    h.add_history_element("201707310000Z", "Removed members (AdminMode, AdminKey and AdminKey) from vtssLacpConfigPortEntry "
                          "as these parameters are now controlled by the AGGR-MIB.");
    h.add_history_element("201807030000Z", "Added LacpConfigGroupTable.");
    h.description("This is a private version of the IEEE802.3ad LAG MIB");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(objects, root, 1, "lacpMibObjects");
NS(lag_config, objects, 2, "lacpConfig");
NS(lag_status, objects, 3, "lacpStatus");
NS(lacp_control, objects, 4, "lacpControl");
NS(lag_statistics, objects, 5, "lacpStatistics");

namespace vtss {
namespace appl {
namespace lacp {
namespace interfaces {
static StructRW2<LacpGlobalParamsLeaf> lacp_global_params_leaf(
        &lag_config, vtss::expose::snmp::OidElement(2, "lacpConfigGlobals"));
static TableReadWrite2<LacpPortConfTable> lacp_port_conf_table(
        &lag_config, vtss::expose::snmp::OidElement(1, "lacpConfigPortTable"));
static TableReadWrite2<LacpGroupConfTable> lacp_group_conf_table(
        &lag_config, vtss::expose::snmp::OidElement(3, "lacpConfigGroupTable"));
static TableReadOnly2<LacpSystemStatusTable> lacp_system_status_table(
        &lag_status, vtss::expose::snmp::OidElement(1, "lacpStatusSystemTable"));
static TableReadOnly2<LacpPortStatusTable> lacp_oper_port_status_table(
        &lag_status, vtss::expose::snmp::OidElement(2, "lacpStatusPortTable"));
static TableReadOnly2<LacpPortStatsTable> lacp_port_stats_table(
        &lag_statistics, vtss::expose::snmp::OidElement(3, "lacpStatisticsPortTable"));
static TableReadWrite2<LacpPortStatsClearTable> lacp_port_stats_clear_table(
        &lacp_control, vtss::expose::snmp::OidElement(1, "lacpControlPortStatsClearTable"));
}  // namespace interfaces
}  // namespace lacp
}  // namespace appl
}  // namespace vtss

