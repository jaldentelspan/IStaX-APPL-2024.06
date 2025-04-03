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

#include "psfp_serializer.hxx"

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("psfpMib", "PSFP", psfp_mib_init, VTSS_MODULE_ID_PSFP, root, h)
{
    h.add_history_element("202402010000Z", "Initial version");
    h.description("Private MIB for Per-Stream Filtering  and Policing, PSFP.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

namespace vtss
{
namespace appl
{
namespace psfp
{
namespace interfaces
{
NS(ns_psfp,       root,    1, "psfpMibObjects");
NS(ns_config,     ns_psfp, 2, "psfpConfig");
NS(ns_status,     ns_psfp, 3, "psfpStatus");
NS(ns_control,    ns_psfp, 4, "psfpControl");
NS(ns_statistics, ns_psfp, 5, "psfpStatistics");

static StructRO2<Capabilities>                 capabilities(           &ns_psfp,       OidElement(1, "psfpCapabilities"));
static TableReadWriteAddDelete2<FlowMeterConf> flow_meter_conf(        &ns_config,     OidElement(1, "psfpConfigFlowMeterTable"),  vtss::expose::snmp::OidElement(2, "psfpConfigFlowMeterRowEditor"));
static TableReadWriteAddDelete2<GateConf>      gate_conf(              &ns_config,     OidElement(3, "psfpConfigGateTable"),       vtss::expose::snmp::OidElement(4, "psfpConfigGateRowEditor"));
static TableReadWriteAddDelete2<FilterConf>    filter_conf(            &ns_config,     OidElement(5, "psfpConfigFilterTable"),     vtss::expose::snmp::OidElement(6, "psfpConfigFilterRowEditor"));
static TableReadOnly2<FlowMeterStatus>         flow_meter_status(      &ns_status,     OidElement(1, "psfpStatusFlowMeterTable"));
static TableReadOnly2<GateStatus>              gate_status(            &ns_status,     OidElement(2, "psfpStatusGateTable"));
static TableReadOnly2<FilterStatus>            filter_status(          &ns_status,     OidElement(3, "psfpStatusFilterTable"));
static TableReadWrite2<FlowMeterControl>       flow_meter_control(     &ns_control,    OidElement(1, "psfpControlFlowMeterClearTable"));
static TableReadWrite2<GateControl>            gate_control(           &ns_control,    OidElement(2, "psfpControlGateClearTable"));
static TableReadWrite2<FilterControl>          filter_control(         &ns_control,    OidElement(3, "psfpControlFilterClearTable"));
static TableReadWrite2<FilterStatisticsClear>  filter_statistics_clear(&ns_control,    OidElement(4, "psfpControlFilterStatisticsClear"));
static TableReadOnly2<FilterStatistics>        filter_statistics(      &ns_statistics, OidElement(1, "psfpStatisticsFilterTable"));

}  // namespace interfaces
}  // namespace psfp
}  // namespace appl
}  // namespace vtss
