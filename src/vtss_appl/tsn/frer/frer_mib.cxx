/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "frer_serializer.hxx"
#include "frer_expose.hxx" // For frer_notification_status

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("frerMib", "FRER", frer_mib_init, VTSS_MODULE_ID_FRER, root, h)
{
    h.add_history_element("202009020000Z", "Initial version");
    h.add_history_element("202202230000Z", "PSFP and FRER can now use same stream IDs, causing one less operational warning");
    h.add_history_element("202212190000Z", "Added one more capability, indicating the maximum supported number of egress interfaces");
    h.add_history_element("202308290000Z", "Added number of ingress stream matches in generation mode");
    h.add_history_element("202311100000Z", "Added StreamIdMax to capabilities");
    h.add_history_element("202402010000Z", "Added support for Stream Collections and pop of outer VLAN tag in generator mode");
    h.description("Private MIB for Frame Replication and Elimination for Reliability, FRER.");
}
#define NSN(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

namespace vtss
{
namespace appl
{
namespace frer
{
namespace interfaces
{
NSN(ns_frer,    root,    1, "frerMibObjects");
NSN(ns_config,  ns_frer, 2, "frerConfig");
NSN(ns_status,  ns_frer, 3, "frerStatus");
NSN(ns_control, ns_frer, 4, "frerControl");
NSN(ns_traps,   ns_frer, 6, "frerTrap");

static StructRO2<FrerCapabilities>          frer_capabilities(    &ns_frer,    OidElement(1, "frerCapabilities"));
static TableReadWriteAddDelete2<FrerConf>   frer_conf(            &ns_config,  OidElement(1, "frerConfigTable"), vtss::expose::snmp::OidElement(2, "frerConfigRowEditor"));
static TableReadOnly2<FrerStatus>           frer_status(          &ns_status,  OidElement(1, "frerStatusTable"));
static TableReadOnly2<FrerStatistics>       frer_statistics(      &ns_status,  OidElement(3, "frerStatusStatisticsTable"));
static TableReadOnlyTrap<FrerNotifStatus>   frer_notif_status(    &ns_status,  OidElement(2, "frerStatusNotificationTable"), &frer_notification_status, &ns_traps, "frerTrap", OidElement(1, "frerTrapAdd"), OidElement(2, "frerTrapMod"), OidElement(3, "frerTrapDel"));
static TableReadWrite2<FrerStatisticsClear> frer_statistics_clear(&ns_control, OidElement(1, "frerControlStatisticsClearTable"));
static TableReadWrite2<FrerControl>         frer_control(         &ns_control, OidElement(2, "frerControlClearTable"));

}  // namespace interfaces
}  // namespace frer
}  // namespace appl
}  // namespace vtss
