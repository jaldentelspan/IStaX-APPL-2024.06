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

#include <vtss/basics/expose/json.hxx>
#include "psfp_serializer.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::psfp::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}

static NamespaceNode ns_psfp("psfp");
extern "C" void psfp_json_init(void)
{
    json_node_add(&ns_psfp);
}

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
NS(ns_conf,       ns_psfp, "config");
NS(ns_status,     ns_psfp, "status");
NS(ns_control,    ns_psfp, "control");
NS(ns_statistics, ns_psfp, "statistics");

namespace vtss
{
namespace appl
{
namespace psfp
{
namespace interfaces
{

static StructReadOnly<Capabilities>           capabilities(           &ns_psfp,       "capabilities");
static StructReadOnly<FlowMeterDefaultConf>   flow_meter_default_conf(&ns_conf,       "flow_meter_default");
static TableReadWriteAddDelete<FlowMeterConf> flow_meter_conf(        &ns_conf,       "flow_meter");
static StructReadOnly<GateDefaultConf>        gate_default_conf(      &ns_conf,       "gate_default");
static TableReadWriteAddDelete<GateConf>      gate_conf(              &ns_conf,       "gate");
static StructReadOnly<FilterDefaultConf>      filer_default_conf(     &ns_conf,       "filter_default");
static TableReadWriteAddDelete<FilterConf>    filter_conf(            &ns_conf,       "filter");
static TableReadOnly<FlowMeterStatus>         flow_meter_status(      &ns_status,     "flow_meter");
static TableReadOnly<GateStatus>              psfp_gate_status(       &ns_status,     "gate");
static TableReadOnly<FilterStatus>            filter_status(          &ns_status,     "filter");
static TableWriteOnly<FlowMeterControl>       flow_meter_control(     &ns_control,    "flow_meter_clear");
static TableWriteOnly<GateControl>            gate_control(           &ns_control,    "gate_clear");
static TableWriteOnly<FilterControl>          filter_control(         &ns_control,    "filter_clear");
static TableWriteOnly<FilterStatisticsClear>  filter_statistics_clear(&ns_control,    "filter_statistics_clear");
static TableReadOnly<FilterStatistics>        filter_statistics(      &ns_statistics, "filter");

}  // namespace interfaces
}  // namespace psfp
}  // namespace appl
}  // namespace vtss

