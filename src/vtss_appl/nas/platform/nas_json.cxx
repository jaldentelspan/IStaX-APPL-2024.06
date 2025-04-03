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

#include "nas_serializer.hxx"
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::nas::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_nas("nas");
extern "C" void vtss_appl_nas_json_init(void) { json_node_add(&ns_nas); }

NS(ns_conf,          ns_nas,     "config");
NS(ns_status,        ns_nas,     "status");
NS(ns_stats,         ns_nas,     "statistics");
NS(ns_control,       ns_nas,     "control");
NS(ns_control_stats, ns_control, "statistics");

namespace vtss {
namespace appl {
namespace nas {
static StructReadWrite<NasGlblConfigGlobals>            nas_global_params_leaf(&ns_conf, "global");
static TableReadOnly<NasLastSupplicantInfo>             nas_last_supplicant_info_lead(&ns_status, "lastSupplicant");
static TableWriteOnly<NasReauthTable>                   nas_prt_re_authenticate_tbl(&ns_control, "reinitOrReauthen");
static TableReadWrite<NasPortConfig>                    nas_port_config_tbl(&ns_conf, "interface");
static TableReadOnly<NasStatusTable>                    nas_port_status_tbl(&ns_status, "interface");
static TableReadOnly<NasPortStatsDot1xEapolTable>       nas_statistics_eapol_stats_table(&ns_stats, "eapol");
static TableReadOnly<NasPortStatsRadiusServerTable>     nas_statistics_radius_stats_table(&ns_stats, "radius");
static TableWriteOnly<NasStatClrLeaf>                   nas_clear_statistics_table(&ns_control_stats, "clear");
}  // namespace nas
}  // namespace appl
}  // namespace vtss
