/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "ipmc_serializer.hxx"
VTSS_MIB_MODULE("ipmcSnoopingMib", "IPMC-SNOOPING", ipmc_mib_init, VTSS_MODULE_ID_IPMC, root, h)
{
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201612130000Z", "Removed the add/delete methods for VLAN interfaces from IGMP/MLD");
    h.add_history_element("202210210000Z", "Restructured entire MIB");
    h.description("This is a private version of the IPMC Snooping MIB");
}

using namespace vtss;
using namespace expose::snmp;

namespace vtss
{
namespace appl
{
namespace ipmc
{
namespace interfaces
{

/* Construct the MIB Objects hierarchy */
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(ipmc_snooping_objects, root, 1, "ipmcSnoopingMibObjects");
NS(ipmc_snooping_config, ipmc_snooping_objects, 2, "ipmcSnoopingConfig");
NS(ipmc_snooping_status, ipmc_snooping_objects, 3, "ipmcSnoopingStatus");
NS(ipmc_snooping_control, ipmc_snooping_objects, 4, "ipmcSnoopingControl");
NS(ipmc_snooping_control_statistics, ipmc_snooping_control, 1, "ipmcSnoopingControlStatistics");

static StructRW2<IpmcIgmpGlobalsConfig> ipmc_snooping_igmp_globals_config(
    &ipmc_snooping_config,
    vtss::expose::snmp::OidElement(1, "ipmcSnoopingConfigIgmpGlobals"));

static TableReadWrite2<IpmcIgmpPortConfigTable> ipmc_snooping_igmp_port_config_table(
    &ipmc_snooping_config,
    vtss::expose::snmp::OidElement(2, "ipmcSnoopingConfigIgmpPortTable"));

static TableReadWrite2<IpmcIgmpVlanConfigTable> ipmc_snooping_igmp_vlan_config_table(
    &ipmc_snooping_config,
    vtss::expose::snmp::OidElement(3, "ipmcSnoopingConfigIgmpInterfaceTable"));

#ifdef VTSS_SW_OPTION_SMB_IPMC
static StructRW2<IpmcMldGlobalsConfig> ipmc_snooping_mld_globals_config(
    &ipmc_snooping_config,
    vtss::expose::snmp::OidElement(5, "ipmcSnoopingConfigMldGlobals"));

static TableReadWrite2<IpmcMldPortConfigTable> ipmc_snooping_mld_port_config_table(
    &ipmc_snooping_config,
    vtss::expose::snmp::OidElement(6, "ipmcSnoopingConfigMldPortTable"));

static TableReadWrite2<IpmcMldVlanConfigTable> ipmc_snooping_mld_vlan_config_table(
    &ipmc_snooping_config,
    vtss::expose::snmp::OidElement(7, "ipmcSnoopingConfigMldInterfaceTable"));
#endif /* VTSS_SW_OPTION_SMB_IPMC */

static TableReadOnly2<IpmcIgmpPortStatusTable> ipmc_snooping_igmp_port_status_table(
    &ipmc_snooping_status,
    vtss::expose::snmp::OidElement(2, "ipmcSnoopingStatusIgmpRouterPortTable"));

static TableReadOnly2<IpmcIgmpVlanStatusTable> ipmc_snooping_igmp_vlan_status_table(
    &ipmc_snooping_status,
    vtss::expose::snmp::OidElement(3, "ipmcSnoopingStatusIgmpVlanTable"));

static TableReadOnly2<IpmcIgmpGrpTable> ipmc_snooping_igmp_grpadrs_table(
    &ipmc_snooping_status,
    vtss::expose::snmp::OidElement(4, "ipmcSnoopingStatusIgmpGroupAddressTable"));

#ifdef VTSS_SW_OPTION_SMB_IPMC
static TableReadOnly2<IpmcIgmpSrcTable> ipmc_snooping_igmp_srclist_table(
    &ipmc_snooping_status,
    vtss::expose::snmp::OidElement(5, "ipmcSnoopingStatusIgmpGroupSrcListTable"));

static TableReadOnly2<IpmcMldPortStatusTable> ipmc_snooping_mld_port_status_table(
    &ipmc_snooping_status,
    vtss::expose::snmp::OidElement(6, "ipmcSnoopingStatusMldRouterPortTable"));

static TableReadOnly2<IpmcMldVlanStatusTable> ipmc_snooping_mld_vlan_status_table(
    &ipmc_snooping_status,
    vtss::expose::snmp::OidElement(7, "ipmcSnoopingStatusMldVlanTable"));

static TableReadOnly2<IpmcMldGrpTable> ipmc_snooping_mld_grpadrs_table(
    &ipmc_snooping_status,
    vtss::expose::snmp::OidElement(8, "ipmcSnoopingStatusMldGroupAddressTable"));

static TableReadOnly2<IpmcMldSrcTable> ipmc_snooping_mld_srclist_table(
    &ipmc_snooping_status,
    vtss::expose::snmp::OidElement(9, "ipmcSnoopingStatusMldGroupSrcListTable"));
#endif /* VTSS_SW_OPTION_SMB_IPMC */

static StructRW2<IpmcIgmpStatisticsClear> ipmc_snooping_control_igmp_statistics(
    &ipmc_snooping_control_statistics,
    vtss::expose::snmp::OidElement(1, "ipmcSnoopingControlStatisticsIgmpClearByIfIndex"));

#ifdef VTSS_SW_OPTION_SMB_IPMC
static StructRW2<IpmcMldStatisticsClear> ipmc_snooping_control_mld_statistics(
    &ipmc_snooping_control_statistics,
    vtss::expose::snmp::OidElement(2, "ipmcSnoopingControlStatisticsMldClearByIfIndex"));
#endif /* VTSS_SW_OPTION_SMB_IPMC */

}  // namespace interfaces
}  // namespace ipmc
}  // namespace appl
}  // namespace vtss

