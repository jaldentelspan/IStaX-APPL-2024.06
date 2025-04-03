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

#include "rmon_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::rmon::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_rmon("rmon");
extern "C" void vtss_appl_rmon_json_init() { json_node_add(&ns_rmon); }

NS(ns_conf,     ns_rmon, "config");;
NS(ns_status,   ns_rmon, "status");;
NS(ns_stats,    ns_rmon, "statistics");;
NS(ns_control,  ns_rmon, "control");;

namespace vtss {
namespace appl {
namespace rmon {
static TableReadWriteAddDelete<RmonstatisticsEntryAddDeleteTable>   rmon_statistics_entry_table(&ns_conf,       "etherStats");
static TableReadWriteAddDelete<RmonHistoryEntryAddDeleteTable>      rmon_history_entry_table(&ns_conf,          "history");
static TableReadWriteAddDelete<RmonAlarmEntryAddDeleteTable>        rmon_alarm_entry_table(&ns_conf,            "alarm");
static TableReadWriteAddDelete<RmonEventEntryAddDeleteTable>        rmon_event_entry_table(&ns_conf,            "event");
static TableReadOnly<RmonStatisticsDataTable>                       rmon_statistics_data_get_table(&ns_stats,   "etherStats");
static TableReadOnly<RmonHistoryStatusTable>                        rmon_history_status_get_table(&ns_status,   "history");
static TableReadOnly<RmonHistoryStatisticsTable>                    rmon_history_stats_get_table(&ns_stats,     "history");
static TableReadOnly<RmonEventStatusTable>                          rmon_event_data_get_table(&ns_status,       "event");
static TableReadOnly<RmonEventLastSentStatusTable>                  rmon_event_lastsent_get_table(&ns_status,   "eventLastSent");
static TableReadOnly<RmonAlarmStatusTable>                          rmon_alarm_data_get_table(&ns_status,       "alarm");
}  // namespace rmon
}  // namespace appl
}  // namespace vtss

