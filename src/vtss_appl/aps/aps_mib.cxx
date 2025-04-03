/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "aps_serializer.hxx"
#include <vtss/appl/aps.h>
VTSS_MIB_MODULE("ApsMib", "APS", aps_mib_init, VTSS_MODULE_ID_APS, root, h)
{
    h.add_history_element("201908270000Z", "Initial version");
    h.add_history_element("202003050000Z", "Added Signal Fail triggering options");
    h.description("This is a private Linear APS (G.8031) MIB");
}

using namespace vtss;
using namespace expose::snmp;

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
namespace vtss
{
namespace appl
{
namespace aps
{
namespace interfaces
{
NS(aps, root, 1, "apsMibObjects");
NS(aps_config, aps, 2, "apsConfig");
NS(aps_status, aps, 3, "apsStatus");
NS(aps_control, aps, 4, "apsControl");

static StructRO2<ApsCapabilitiesImpl> aps_capabilities_impl(
    &aps, vtss::expose::snmp::OidElement(1, "apsCapabilities"));

static TableReadWriteAddDelete2<ApsConfigTableImpl> aps_config_table_impl(
    &aps_config, vtss::expose::snmp::OidElement(1, "apsConfigTable"), vtss::expose::snmp::OidElement(2, "apsConfigRowEditor"));

static TableReadOnly2<ApsStatusTableImpl> aps_status_table_impl(
    &aps_status, vtss::expose::snmp::OidElement(1, "apsStatusTable"));

static TableReadWrite2<ApsControlCommandTableImpl> aps_control_command_table_impl(
    &aps_control, vtss::expose::snmp::OidElement(1, "apsControlCommandTable"));

static TableReadWrite2<ApsControlStatisticsClearTableImpl> aps_control_statistics_clear_table_impl(
    &aps_control, vtss::expose::snmp::OidElement(2, "apsControlStatisticsClearTable"));


}  // namespace interfaces
}  // namespace aps
}  // namespace appl
}  // namespace vtss

