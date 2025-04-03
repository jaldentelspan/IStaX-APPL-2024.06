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
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::erps::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_erps("erps");
extern "C" void vtss_appl_erps_json_init()
{
    json_node_add(&ns_erps);
}

NS(ns_control,       ns_erps,  "control");

static StructReadOnly<ErpsCapabilities> erps_capabilities(&ns_erps, "capabilities");
static TableReadWriteAddDelete<ErpsConfTable> erps_conf_table(&ns_erps, "config");
static TableReadOnly<ErpsStatusTable> erps_status_table(&ns_erps, "status");

static TableReadWrite<ErpsControlCommandTable> erps_control_command_table(
    &ns_control, "command");

static TableReadWrite<ErpsControlStatisticsClearTable> erps_control_statistics_clear_table(
    &ns_control, "statisticsClear");
