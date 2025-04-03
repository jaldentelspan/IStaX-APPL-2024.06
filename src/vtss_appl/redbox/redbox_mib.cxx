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

#include "redbox_serializer.hxx"
#include "redbox_expose.hxx" // For redbox_notification_status

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("redboxMib", "REDBOX", redbox_mib_init, VTSS_MODULE_ID_REDBOX, root, h)
{
    h.add_history_element("202309150000Z", "Initial version");
    h.description("Private MIB for IEC 62439-3 PRP/HSR RedBox.");
}
#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

namespace vtss
{
namespace appl
{
namespace redbox
{
namespace interfaces
{
// Parent: vtss
NS(ns_redbox,     root,      1, "redboxMibObjects");

// Parent: vtss/redbox
NS(ns_capabilities, ns_redbox, 1, "redboxCapabilities");
NS(ns_conf,         ns_redbox, 2, "redboxConfig");
NS(ns_status,       ns_redbox, 3, "redboxStatus");
NS(ns_control,      ns_redbox, 4, "redboxControl");
NS(ns_statistics,   ns_redbox, 5, "redboxStatistics");
NS(ns_traps,        ns_redbox, 6, "redboxTrap");

static StructRO2<RedBoxCapabilities>                 redbox_capabilities(           &ns_capabilities, OidElement(1, "redboxCapabilitiesList")); // Apparently, a structure name cannot contain the word "Table". If it does, "Table" gets removed, and then we get a duplicate redboxCapabilities :-(
static TableReadOnly2<RedBoxCapabilitiesInterfaces>  redbox_capabilities_interfaces(&ns_capabilities, OidElement(2, "redboxCapabilitiesInterfaces"));
static TableReadWriteAddDelete2<RedBoxConf>          redbox_conf(                   &ns_conf,         OidElement(1, "redboxConfigTable"), OidElement(2, "redboxConfigRowEditor"));
static TableReadOnly2<RedBoxStatus>                  redbox_status(                 &ns_status,       OidElement(1, "redboxStatusTable"));
static TableReadOnly2<RedBoxNodesTableStatus>        redbox_nt_status(              &ns_status,       OidElement(2, "redboxStatusNtTable"));
static TableReadOnly2<RedBoxNodesTableMacStatus>     redbox_nt_mac_status(          &ns_status,       OidElement(3, "redboxStatusNtMacTable"));
static TableReadOnly2<RedBoxProxyNodeTableStatus>    redbox_pnt_status(             &ns_status,       OidElement(4, "redboxStatusPntTable"));
static TableReadOnly2<RedBoxProxyNodeTableMacStatus> redbox_pnt_mac_status(         &ns_status,       OidElement(5, "redboxStatusPntMacTable"));
static TableReadOnlyTrap<RedBoxNotifStatus>          redbox_notif_status(           &ns_status,       OidElement(6, "redboxStatusNotificationTable"), &redbox_notification_status, &ns_traps, "redboxTrap", OidElement(1, "redboxTrapAdd"), OidElement(2, "redboxTrapMod"), OidElement(3, "redboxTrapDel"));
static TableReadWrite2<RedBoxNtClear>                redbox_nt_clear(               &ns_control,      OidElement(1, "redboxControlNtClear"));
static TableReadWrite2<RedBoxPntClear>               redbox_pnt_clear(              &ns_control,      OidElement(2, "redboxControlPntClear"));
static TableReadWrite2<RedBoxStatisticsClear>        redbox_statistics_clear(       &ns_control,      OidElement(3, "redboxControlStatisticsClear"));
static TableReadOnly2<RedBoxStatistics>              redbox_statistics(             &ns_statistics,   OidElement(1, "redboxStatisticsTable"));

}  // namespace interfaces
}  // namespace redbox
}  // namespace appl
}  // namespace vtss
