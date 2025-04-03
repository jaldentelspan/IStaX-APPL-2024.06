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
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::redbox::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}

static NamespaceNode ns_redbox("redbox");
extern "C" void redbox_json_init()
{
    json_node_add(&ns_redbox);
}

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
NS(ns_capabilities, ns_redbox, "capabilities");
NS(ns_conf,         ns_redbox, "config");
NS(ns_status,       ns_redbox, "status");
NS(ns_statistics,   ns_redbox, "statistics");
NS(ns_control,      ns_redbox, "control");

namespace vtss
{
namespace appl
{
namespace redbox
{
namespace interfaces
{

static StructReadOnly<RedBoxCapabilities>           redbox_capabilities(           &ns_capabilities, "capabilities");
static TableReadOnly<RedBoxCapabilitiesInterfaces>  redbox_capabilities_interfaces(&ns_capabilities, "interfaces");
static StructReadOnly<RedBoxDefaultConf>            redbox_default_conf(           &ns_conf,         "defaultConf");
static TableReadWriteAddDeleteNoNS<RedBoxConf>      redbox_conf(                   &ns_conf);
static TableReadOnlyNoNS<RedBoxStatus>              redbox_status(                 &ns_status);
static TableReadOnly<RedBoxNodesTableStatus>        redbox_nt_status(              &ns_status,       "nt");
static TableReadOnly<RedBoxNodesTableMacStatus>     redbox_nt_mac_status(          &ns_status,       "nt_mac");
static TableReadOnly<RedBoxProxyNodeTableStatus>    redbox_pnt_status(             &ns_status,       "pnt");
static TableReadOnly<RedBoxProxyNodeTableMacStatus> redbox_pnt_mac_status(         &ns_status,       "pnt_mac");
static TableReadOnlyNotification<RedBoxNotifStatus> redbox_notif_status(           &ns_status,       "notification", &redbox_notification_status);
static TableReadOnlyNoNS<RedBoxStatistics>          redbox_statistics(             &ns_statistics);
static TableReadWrite<RedBoxNtClear>                redbox_nt_clear(               &ns_control,      "ntClear");
static TableReadWrite<RedBoxPntClear>               redbox_pnt_clear(              &ns_control,      "pntClear");
static TableReadWrite<RedBoxStatisticsClear>        redbox_statistics_clear(       &ns_control,      "statisticsClear");

}  // namespace interfaces
}  // namespace redbox
}  // namespace appl
}  // namespace vtss

