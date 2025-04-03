/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "mstp_serializer.hxx"
#include "vtss/appl/mstp.h"

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("mstpMib", "MSTP", mstp_mib_init, VTSS_MODULE_ID_RSTP, root, h)
{
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201509290000Z", "Added range information for bridge parameters");
    h.add_history_element("201511040000Z", "Added msti config table with msti value as the iteration key");
    h.add_history_element("201511130000Z", "Add TCN counters");
    h.add_history_element("202202280000Z", "vtssMstpConfigMstiConfigMstid extends supported range to 16 bit for Traffic Engineering (TE) MSTID 0xFFE");
    h.description("This is a private version of the 802.1Q-2005 MSTP MIB");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(mstp, root, 1, "mstpMibObjects");;
NS(mstp_config, mstp, 2, "mstpConfig");;
NS(mstp_msti_config, mstp_config, 3, "mstpConfigMstiConfig");;
NS(mstp_status, mstp, 3, "mstpStatus");;

namespace vtss
{
namespace appl
{
namespace mstp
{
namespace interfaces
{
static StructRW2<MstpBridgeLeaf> mstp_bridge_leaf(&mstp_config, vtss::expose::snmp::OidElement(1, "mstpConfigBridgeParams"));
static TableReadWrite2<MstpMstiParamsTable> mstp_msti_params_table(&mstp_config, vtss::expose::snmp::OidElement(2, "mstpConfigMstiParamTable"));
static StructRW2<MstpMstiConfigLeaf> mstp_msti_config_leaf(&mstp_msti_config, vtss::expose::snmp::OidElement(1, "mstpConfigMstiConfigId"));
static TableReadWrite2<MstpMstiConfigTableEntries> mstp_msti_config_table_entries(&mstp_msti_config, vtss::expose::snmp::OidElement(2, "mstpConfigMstiConfigTable"));
static TableReadWrite2<MstpMstiConfigVlanBitmapTableEntries> mstp_msti_config_vlan_bitmap_table_entries(&mstp_msti_config, vtss::expose::snmp::OidElement(3, "mstpConfigMstiConfigVlanBitmapTable"));
static TableReadWrite2<MstpCistportParamsTable> mstp_cistport_params_table(&mstp_config, vtss::expose::snmp::OidElement(4, "mstpConfigCistInterfaceParamTable"));
static StructRW2<MstpAggrParamLeaf> mstp_aggr_param_leaf(&mstp_config, vtss::expose::snmp::OidElement(5, "mstpConfigAggrParams"));
static TableReadWrite2<MstpMstiportParamTable> mstp_mstiport_param_table(&mstp_config, vtss::expose::snmp::OidElement(6, "mstpConfigMstiInterfaceParamTable"));
static TableReadWrite2<MstpMstiportAggrparamTable> mstp_mstiport_aggrparam_table(&mstp_config, vtss::expose::snmp::OidElement(7, "mstpConfigMstiAggrParamTable"));
static TableReadOnly2<MstpBridgeStatusTable> mstp_bridge_status_table(&mstp_status, vtss::expose::snmp::OidElement(1, "mstpStatusBridgeTable"));
static TableReadOnly2<MstpPortStatusTable> mstp_port_status_table(&mstp_status, vtss::expose::snmp::OidElement(2, "mstpStatusInterfaceTable"));
static TableReadOnly2<MstpPortStatsTable> mstp_port_stats_table(&mstp_status, vtss::expose::snmp::OidElement(3, "mstpStatusInterfaceStatisticsTable"));
}  // namespace interfaces
}  // namespace mstp
}  // namespace appl
}  // namespace vtss

