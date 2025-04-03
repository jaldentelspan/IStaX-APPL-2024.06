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

#include "arp_inspection_serializer.hxx"
#include "vtss/basics/preprocessor.h"
#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#include "web_api.h"
#endif


using namespace vtss;
using namespace expose::snmp;


VTSS_MIB_MODULE("arpInspectionMib", "ARP-INSPECTION", arp_inspection_mib_init,
                VTSS_MODULE_ID_ARP_INSPECTION, root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.description("This is a private version of the ARP Inspection MIB");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(arp_inspection_objects, root, 1, "arpInspectionMibObjects");
NS(arp_inspection_config, arp_inspection_objects, 2, "arpInspectionConfig");
NS(arp_inspection_status, arp_inspection_objects, 3, "arpInspectionStatus");
NS(arp_inspection_control, arp_inspection_objects, 4, "arpInspectionControl");

namespace vtss {
namespace appl {
namespace arp_inspection {
namespace interfaces {
static StructRW2<ArpInspectionParamsLeaf> arp_inspection_params_leaf(
        &arp_inspection_config,
        vtss::expose::snmp::OidElement(1, "arpInspectionConfigGlobals"));
static TableReadWrite2<ArpInspectionPortConfigTable>
        arp_inspection_port_config_table(
                &arp_inspection_config,
                vtss::expose::snmp::OidElement(2, "arpInspectionConfigPortTable"));
static TableReadWriteAddDelete2<ArpInspectionVlanConfigTable>
        arp_inspection_vlan_config_table(
                &arp_inspection_config,
                vtss::expose::snmp::OidElement(3, "arpInspectionConfigVlanTable"),
                vtss::expose::snmp::OidElement(
                        4, "arpInspectionConfigVlanTableRowEditor"));
static TableReadWriteAddDelete2<ArpInspectionStaticConfigTable>
        arp_inspection_static_config_table(
                &arp_inspection_config,
                vtss::expose::snmp::OidElement(5, "arpInspectionConfigStaticTable"),
                vtss::expose::snmp::OidElement(
                        6, "arpInspectionConfigStaticTableRowEditor"));
static TableReadOnly2<ArpInspectionDynamicStatusTable>
        arp_inspection_dynamic_status_table(
                &arp_inspection_status,
                vtss::expose::snmp::OidElement(
                        1, "arpInspectionStatusDynamicAddressTable"));
static StructRW2<ArpInspectionActionLeaf> arp_inspection_action_leaf(
        &arp_inspection_control,
        vtss::expose::snmp::OidElement(1, "arpInspectionControlGlobals"));
}  // namespace interfaces
}  // namespace arp_inspection
}  // namespace appl
}  // namespace vtss

