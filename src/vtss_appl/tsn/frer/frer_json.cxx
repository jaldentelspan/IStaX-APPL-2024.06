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

#include "frer_serializer.hxx"
#include "frer_expose.hxx" // For frer_notification_status

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::frer::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_frer("frer");
extern "C" void frer_json_init(void)
{
    json_node_add(&ns_frer);
}

NS(ns_conf,       ns_frer, "config");
NS(ns_status,     ns_frer, "status");
NS(ns_control,    ns_frer, "control");
NS(ns_statistics, ns_frer, "statistics");

namespace vtss
{
namespace appl
{
namespace frer
{
namespace interfaces
{

static StructReadOnly<FrerCapabilities>           frer_capabilities(    &ns_frer,    "capabilities");
static StructReadOnly<FrerDefaultConf>            frer_default_conf(    &ns_conf,    "defaultConf");
static TableReadWriteAddDeleteNoNS<FrerConf>      frer_conf(            &ns_conf);
static TableReadOnlyNoNS<FrerStatus>              frer_status(          &ns_status);
static TableReadOnlyNotification<FrerNotifStatus> frer_notif_status(    &ns_status,  "notification", &frer_notification_status);
static TableReadOnlyNoNS<FrerStatistics>          frer_statistics(      &ns_statistics);
static TableReadWrite<FrerStatisticsClear>        frer_statistics_clear(&ns_control, "statisticsClear");
static TableReadWrite<FrerControl>                frer_control(         &ns_control, "clear");

}  // namespace interfaces
}  // namespace frer
}  // namespace appl
}  // namespace vtss

