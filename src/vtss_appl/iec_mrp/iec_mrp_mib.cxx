/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "iec_mrp_serializer.hxx"

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("iecmrpMib", "IECMRP", iec_mrp_mib_init, VTSS_MODULE_ID_IEC_MRP, root, h)
{
    h.add_history_element("202104010000Z", "Initial version");
    h.add_history_element("202309140000Z", "Changed vtssIecmrpCapabilitiesInfoGroup::vtssIecmrpCapabilitiesHtTxTestPdus to vtssIecmrpCapabilitiesHwTxTestPdus,");
    h.description("Private MIB for Ethernet Ring Protection Switching, MRP.");
}
#define NSN(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

namespace vtss
{
namespace appl
{
namespace iec_mrp
{
namespace interfaces
{

// Parent: vtss
NSN(ns_iec_mrp,         root,    1, "iecmrpMibObjects");

// Parent: vtss/iec_mrp
NSN(ns_config,       ns_iec_mrp, 2, "iecmrpConfig");
NSN(ns_status,       ns_iec_mrp, 3, "iecmrpStatus");
NSN(ns_control,      ns_iec_mrp, 4, "iecmrpControl");

static StructRO2<IecMrpCapabilities> iec_iecmrpcapabilities(
    &ns_iec_mrp,
    vtss::expose::snmp::OidElement(1, "iecmrpCapabilities"));

static TableReadWriteAddDelete2<IecMrpConfTable> iec_mrp_conf_table(
    &ns_config,
    vtss::expose::snmp::OidElement(1, "iecmrpConfigTable"),
    vtss::expose::snmp::OidElement(2, "iecmrpConfigRowEditor"));

static TableReadOnly2<IecMrpStatusTable> iec_mrp_status_table(
    &ns_status,
    vtss::expose::snmp::OidElement(1, "iecmrpStatusTable"));

static TableReadWrite2<IecMrpControlStatisticsClearTable> iec_mrp_control_statistics_clear_table(
    &ns_control,
    vtss::expose::snmp::OidElement(2, "iecmrpControlStatisticsClearTable"));

}  // namespace interfaces
}  // namespace iec_mrp
}  // namespace appl
}  // namespace vtss
