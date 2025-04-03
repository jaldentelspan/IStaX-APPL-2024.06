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

#include "mvr_serializer.hxx"

VTSS_MIB_MODULE("ipmcMvrMib", "IPMC-MVR", mvr_mib_init, VTSS_MODULE_ID_MVR,
                root, h)
{
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201708240000Z", "Added Querier election cofigured option");
    h.add_history_element("202204190000Z", "Added capability table and updated various text");
    h.add_history_element("202210210000Z", "Restructured entire MIB");
    h.description("This is a private version of the IPMC MVR MIB");
}

using namespace vtss;
using namespace expose::snmp;

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(mvr_objects, root, 1, "ipmcMvrMibObjects");
NS(mvr_config,  mvr_objects, 2, "ipmcMvrConfig");
NS(mvr_status,  mvr_objects, 3, "ipmcMvrStatus");
NS(mvr_control, mvr_objects, 4, "ipmcMvrControl");

namespace vtss
{
namespace appl
{
namespace mvr
{
namespace interfaces
{
static StructRW2<MvrGlobalsConfig> mvr_globals_config(
    &mvr_config, vtss::expose::snmp::OidElement(1, "ipmcMvrConfigGlobals"));
static TableReadWrite2<MvrPortConfigTable> mvr_interface_config_table(
    &mvr_config, vtss::expose::snmp::OidElement(2, "ipmcMvrConfigPortTable"));
static TableReadWriteAddDelete2<MvrVlanConfigTable> mvr_igmp_vlan_config_table(
    &mvr_config, vtss::expose::snmp::OidElement(3, "ipmcMvrConfigInterfaceTable"), vtss::expose::snmp::OidElement(4, "ipmcMvrConfigInterfaceTableRowEditor"));
static TableReadWrite2<MvrVlanPortConfigTable> mvr_vlan_port_config_table(
    &mvr_config, vtss::expose::snmp::OidElement(5, "ipmcMvrConfigVlanPortTable"));
static TableReadOnly2<MvrIgmpVlanStatusTable> mvr_igmp_vlan_status_table(
    &mvr_status, vtss::expose::snmp::OidElement(2, "ipmcMvrStatusIgmpVlanTable"));
static TableReadOnly2<MvrIgmpGrpTable> mvr_igmp_grpadrs_table(
    &mvr_status, vtss::expose::snmp::OidElement(3, "ipmcMvrStatusIgmpGroupAddressTable"));
static TableReadOnly2<MvrIgmpSrcTable> mvr_igmp_srclist_table(
    &mvr_status, vtss::expose::snmp::OidElement(4, "ipmcMvrStatusIgmpGroupSrcListTable"));
static TableReadOnly2<MvrMldVlanStatusTable> mvr_mld_vlan_status_table(
    &mvr_status, vtss::expose::snmp::OidElement(5, "ipmcMvrStatusMldVlanTable"));
static TableReadOnly2<MvrMldGrpTable> mvr_mld_grpadrs_table(
    &mvr_status, vtss::expose::snmp::OidElement(6, "ipmcMvrStatusMldGroupAddressTable"));
static TableReadOnly2<MvrMldSrcTable> mvr_mld_srclist_table(
    &mvr_status, vtss::expose::snmp::OidElement(7, "ipmcMvrStatusMldGroupSrcListTable"));
static StructRW2<MvrStatisticsClear> mvr_control_clear_statistics(
    &mvr_control, vtss::expose::snmp::OidElement(1, "ipmcMvrControlStatisticsClear"));
}  // namespace interfaces
}  // namespace mvr
}  // namespace appl
}  // namespace vtss

