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

#include "erps_serializer.hxx"

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("erpsMib", "ERPS", erps_mib_init, VTSS_MODULE_ID_ERPS, root, h)
{
    h.add_history_element("201911010000Z", "Initial version");
    h.description("Private MIB for Ethernet Ring Protection Switching, ERPS.");
}
#define NSN(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

namespace vtss
{
namespace appl
{
namespace erps
{
namespace interfaces
{

// Parent: vtss
NSN(ns_erps,         root,    1, "erpsMibObjects");

// Parent: vtss/erps
NSN(ns_config,       ns_erps, 2, "erpsConfig");
NSN(ns_status,       ns_erps, 3, "erpsStatus");
NSN(ns_control,      ns_erps, 4, "erpsControl");

static StructRO2<ErpsCapabilities> erps_capabilities(
    &ns_erps,
    vtss::expose::snmp::OidElement(1, "erpsCapabilities"));

static TableReadWriteAddDelete2<ErpsConfTable> erps_conf_table(
    &ns_config,
    vtss::expose::snmp::OidElement(1, "erpsConfigTable"),
    vtss::expose::snmp::OidElement(2, "erpsConfigRowEditor"));

static TableReadOnly2<ErpsStatusTable> erps_status_table(
    &ns_status,
    vtss::expose::snmp::OidElement(1, "erpsStatusTable"));

static TableReadWrite2<ErpsControlCommandTable> erps_control_command_table(
    &ns_control,
    vtss::expose::snmp::OidElement(1, "erpsControlCommandTable"));

static TableReadWrite2<ErpsControlStatisticsClearTable> aps_control_statistics_clear_table(
    &ns_control,
    vtss::expose::snmp::OidElement(2, "erpsControlStatisticsClearTable"));

}  // namespace interfaces
}  // namespace erps
}  // namespace appl
}  // namespace vtss
