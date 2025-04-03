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

#include "pvlan_serializer.hxx"

VTSS_MIB_MODULE("pvlanMib", "PVLAN", vtss_appl_pvlan_mib_init, VTSS_MODULE_ID_PVLAN, root, h)
{
    h.add_history_element("201407160000Z", "Initial version");
    h.description("This is a private version of the Private VLAN MIB");
}
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

using namespace vtss;
using namespace expose::snmp;

namespace vtss
{
namespace appl
{
namespace pvlan
{
namespace interfaces
{

/* Construct the MIB Objects hierarchy */
/*
    xxxPvlanMIBObjects    +-  xxxPvlanCapabilities
                                +-  xxxPvlanConfig    -> ...
                                +-  xxxPvlanStatus    -> ...
                                +-  xxxPvlanControl   -> ...

    xxxPvlanMIBConformance will be generated automatically
*/
/* root: parent node (See VTSS_MIB_MODULE) */
NS(pvlan_mib_objects, root, 1,  "pvlanMibObjects");

/* xxxPvlanCapabilities */
static StructRO2<PvlanCapabilitiesLeaf> pvlan_capabilities_leaf(
    &pvlan_mib_objects,
    vtss::expose::snmp::OidElement(1, "pvlanCapabilities"));

/* xxxPvlanConfig */
NS(pvlan_config,                pvlan_mib_objects,  2,  "pvlanConfig");
/* xxxPvlanConfig:xxxPvlanConfigInterface */
NS(pvlan_config_interface,      pvlan_config,       1,  "pvlanConfigInterface");

/* xxxPvlanConfigInterfaceVlanMembershipTable */
static TableReadWriteAddDelete2<PvlanVlanMembershipTable> pvlan_vlan_membership_table(
    &pvlan_config_interface,
    vtss::expose::snmp::OidElement(1, "pvlanConfigInterfaceVlanMembershipTable"),
    vtss::expose::snmp::OidElement(2, "pvlanConfigInterfaceVlanMembershipTableRowEditor"));

/* xxxPvlanConfigInterfacePortIsolatationTable */
static TableReadWrite2<PvlanPortIsolationTable> pvlan_port_isolation_table(
    &pvlan_config_interface,
    vtss::expose::snmp::OidElement(3, "pvlanConfigInterfacePortIsolatationTable"));

}  // namespace interfaces
}  // namespace pvlan
}  // namespace appl
}  // namespace vtss
