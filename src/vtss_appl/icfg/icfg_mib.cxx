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

#include "icfg_serializer.hxx"

using namespace vtss;
using namespace expose::snmp;
VTSS_MIB_MODULE("icfgMib", "ICFG", icfg_mib_init, VTSS_MODULE_ID_ICFG , root, h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201410100000Z", "Editorial changes");
    h.add_history_element("201605090000Z", "Add support for allocated/free flash size");
    h.description("This is a private version of ICFG");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(icfg_mib_objects, root, 1, "icfgMibObjects");
NS(icfg_status, icfg_mib_objects, 3, "icfgStatus");
NS(icfg_control, icfg_mib_objects, 4, "icfgControl");

namespace vtss {
namespace appl {
namespace icfg{
namespace interfaces {
static StructRO2<IcfgStatusFileStatisticsLeaf> icfg_status_file_statistics_leaf(
        &icfg_status, vtss::expose::snmp::OidElement(1, "icfgStatusFileStatistics"));
static TableReadOnly2<IcfgStatusFileEntry> icfg_status_file_entry(
        &icfg_status, vtss::expose::snmp::OidElement(2, "icfgStatusFileTable"));
static StructRO2<IcfgStatusCopyConfigLeaf> icfg_status_copy_config_leaf(
        &icfg_status, vtss::expose::snmp::OidElement(3, "icfgStatusCopyConfig"));
static StructRW2<IcfgControlGlobalsLeaf> icfg_control_globals_leaf(
        &icfg_control, vtss::expose::snmp::OidElement(1, "icfgControlGlobals"));
static StructRW2<IcfgControlCopyConfigLeaf> icfg_control_copy_config_leaf(
        &icfg_control, vtss::expose::snmp::OidElement(2, "icfgControlCopyConfig"));
}  // namespace interfaces
}  // namespace aggr
}  // namespace appl
}  // namespace vtss

