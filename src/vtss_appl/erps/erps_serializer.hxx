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

#ifndef __VTSS_ERPS_SERIALIZER_HXX__
#define __VTSS_ERPS_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/erps.h"
#include "vtss_appl_formatting_tags.hxx" // for AsStdDisplayString


mesa_rc vtss_erps_create_conf_default(uint32_t *instance, vtss_appl_erps_conf_t *s);
mesa_rc vtss_erps_statistics_clear(uint32_t instance, const BOOL *clear);
mesa_rc vtss_erps_statistics_clear_dummy(uint32_t instance,  BOOL *clear);


//----------------------------------------------------------------------------
// SNMP tagging for basic types
//----------------------------------------------------------------------------

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_erps_u32_dsc, u32, a, s )
{
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("groupIndex"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("ERPS group index number. Valid range is (1..max groups). The maximum group number is platform-specific and can be retrieved from the ERPS capabilities."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_erps_ctrl_bool_t, BOOL, a, s)
{
    a.add_leaf(vtss::AsBool(s.inner), vtss::tag::Name("Clear"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Set to TRUE to clear the counters of an ERPS instance."));
}

//----------------------------------------------------------------------------
// SNMP enums; become textual conventions
//----------------------------------------------------------------------------

extern vtss_enum_descriptor_t vtss_appl_erps_ring_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_erps_ring_type_t,
                         "ErpsRingType",
                         vtss_appl_erps_ring_type_txt,
                         "Specifies the ERPS ring type.");

extern vtss_enum_descriptor_t vtss_appl_erps_command_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_erps_command_t,
                         "ErpsCommand",
                         vtss_appl_erps_command_txt,
                         "Specifies the ERPS command.");

extern vtss_enum_descriptor_t vtss_appl_erps_version_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_erps_version_t,
                         "ErpsVersion",
                         vtss_appl_erps_version_txt,
                         "Specifies the ERPS protocol version.");

extern vtss_enum_descriptor_t vtss_appl_erps_rpl_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_erps_rpl_mode_t,
                         "ErpsRplMode",
                         vtss_appl_erps_rpl_mode_txt,
                         "Specifies the Ring Protection Link mode. Use 'none' if port is neither RPL owner nor neighbor.");

extern vtss_enum_descriptor_t vtss_appl_erps_sf_trigger_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_erps_sf_trigger_t,
                         "ErpsSfTrigger",
                         vtss_appl_erps_sf_trigger_txt,
                         "Signal fail can either come from the physical link on a given port or from a Down-MEP.");

extern vtss_enum_descriptor_t vtss_appl_erps_request_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_erps_request_t,
                         "ErpsRequestState",
                         vtss_appl_erps_request_txt,
                         "Specifies a request state.");

extern vtss_enum_descriptor_t vtss_appl_erps_ring_port_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_erps_ring_port_t,
                         "ErpsRingPort",
                         vtss_appl_erps_ring_port_txt,
                         "Specifies a particular logical ring port.");

extern vtss_enum_descriptor_t vtss_appl_erps_oper_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_erps_oper_state_t,
                         "ErpsOperState",
                         vtss_appl_erps_oper_state_txt,
                         "Specifies an operational state.");

extern vtss_enum_descriptor_t vtss_appl_erps_node_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_erps_node_state_t,
                         "ErpsNodeState",
                         vtss_appl_erps_node_state_txt,
                         "Specifies a node state.");

extern vtss_enum_descriptor_t vtss_appl_erps_oper_warning_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_erps_oper_warning_t,
                         "ErpsOperWarning",
                         vtss_appl_erps_oper_warning_txt,
                         "Operational warnings of an ERPS instance.");


//----------------------------------------------------------------------------
// Capabilities
//----------------------------------------------------------------------------

template<typename T>
void serialize(T &a, vtss_appl_erps_capabilities_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_erps_capabilities_t"));
    int ix = 1;

    m.add_leaf(s.inst_cnt_max,
               vtss::tag::Name("InstanceMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of created ERPS instances."));

    m.add_leaf(s.wtr_secs_max,
               vtss::tag::Name("WtrSecsMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum WTR timer value in secs."));

    m.add_leaf(s.guard_time_msecs_max,
               vtss::tag::Name("GuardTimeMsecsMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum Guard timer value in msec."));

    m.add_leaf(s.hold_off_msecs_max,
               vtss::tag::Name("HoldOffMsecsMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum Hold Off timer value in msec."));

}

//----------------------------------------------------------------------------
// Configuration
//----------------------------------------------------------------------------

template<typename T>
void serialize(T &a, vtss_appl_erps_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_erps_conf_t"));
    int ix = 2;

    m.add_leaf(vtss::AsBool(s.admin_active),
               vtss::tag::Name("AdminActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The administrative state of this ERPS instance. Set to true to make it function normally "
                                      "and false to make it cease functioning."));

    m.add_leaf(s.version,
               vtss::tag::Name("Version"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("ERPS protocol version."));

    m.add_leaf(s.ring_type,
               vtss::tag::Name("RingType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Type of ring."));

    m.add_leaf(vtss::AsBool(s.virtual_channel),
               vtss::tag::Name("VirtualChannel"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Whether to use a virtual channel. Controls whether to use a virtual channel with a sub-ring."));

    m.add_leaf(s.interconnect_conf.connected_ring_inst,
               vtss::tag::Name("ConnectedRingId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("For a sub-ring on an interconnection node, this must reference the instance ID of the ring to which this sub-ring is connected."));

    m.add_leaf(vtss::AsBool(s.interconnect_conf.tc_propagate),
               vtss::tag::Name("ConnectedRingPropagate"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls whether the ring referenced by connected_ring_inst shall propagate R-APS flush PDUs whenever this sub-ring's topology changes."));

    m.add_leaf(s.ring_id,
               vtss::tag::Name("RingId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The Ring ID is used - along with the control VLAN - to identify R-APS PDUs as belonging to a particular ring."));

    m.add_leaf(s.node_id,
               vtss::tag::Name("NodeId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The Node ID is used inside the R-APS specific PDU to uniquely identify this node (switch) on the ring."));

    m.add_leaf(s.level,
               vtss::tag::Name("Level"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("MD/MEG Level of R-APS PDUs we transmit."));

    m.add_leaf(s.control_vlan,
               vtss::tag::Name("ControlVlan"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The VLAN on which R-APS PDUs are transmitted and received on the ring ports."));

    m.add_leaf(s.pcp,
               vtss::tag::Name("Pcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The PCP value used in the VLAN tag of the R-APS PDUs."));

    m.add_leaf(s.ring_port_conf[0].sf_trigger,
               vtss::tag::Name("Port0SfTrigger"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Selects whether Signal Fail (SF) comes from the link state of a given interface, or from a Down-MEP."));

    m.add_leaf(vtss::AsInterfaceIndex(s.ring_port_conf[0].ifindex),
               vtss::tag::Name("Port0If"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Interface index of ring protection port 0."));

    m.add_leaf(vtss::AsStdDisplayString(s.ring_port_conf[0].mep.md, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("Port0MEPDomain"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Domain name name of Port0 MEP."));

    m.add_leaf(vtss::AsStdDisplayString(s.ring_port_conf[0].mep.ma, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("Port0MEPService"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Service name of Port0 MEP."));

    m.add_leaf(s.ring_port_conf[0].mep.mepid,
               vtss::tag::Name("Port0MEPId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("MEPID of Port0 MEP."));

    m.add_leaf(s.ring_port_conf[0].smac,
               vtss::tag::Name("Port0Smac"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Source MAC address (must be unicast) used in R-APS PDUs sent on this ring-port."));

    m.add_leaf(s.ring_port_conf[1].sf_trigger,
               vtss::tag::Name("Port1SfTrigger"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Selects whether Signal Fail (SF) comes from the link state of a given interface, or from a Down-MEP."));

    m.add_leaf(vtss::AsInterfaceIndex(s.ring_port_conf[1].ifindex),
               vtss::tag::Name("Port1If"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Interface index of ring protection port 1."));

    m.add_leaf(vtss::AsStdDisplayString(s.ring_port_conf[1].mep.md, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("Port1MEPDomain"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Domain name name of Port1 MEP."));

    m.add_leaf(vtss::AsStdDisplayString(s.ring_port_conf[1].mep.ma, VTSS_APPL_CFM_KEY_LEN_MAX),
               vtss::tag::Name("Port1MEPService"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Service name of Port1 MEP."));

    m.add_leaf(s.ring_port_conf[1].mep.mepid,
               vtss::tag::Name("Port1MEPId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("MEPID of Port1 MEP."));

    m.add_leaf(s.ring_port_conf[1].smac,
               vtss::tag::Name("Port1Smac"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Source MAC address (must be unicast) used in R-APS PDUs sent on this ring-port."));

    m.add_leaf(vtss::AsBool(s.revertive),
               vtss::tag::Name("Revertive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Revertive (true) or Non-revertive (false) mode."));

    m.add_leaf(s.wtr_secs,
               vtss::tag::Name("WaitToRestoreTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Wait-to-Restore time in seconds. Valid range is 1-720."));

    m.add_leaf(s.guard_time_msecs,
               vtss::tag::Name("GuardTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Guard time in ms. Valid range is 10-2000 ms."));

    m.add_leaf(s.hold_off_msecs,
               vtss::tag::Name("HoldOffTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Hold off time in ms. Value is rounded down to 100ms precision. Valid range is 0-10000 ms"));

    m.add_leaf(s.rpl_mode,
               vtss::tag::Name("RplMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Ring Protection Link mode."));

    m.add_leaf(s.rpl_port,
               vtss::tag::Name("RplPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether it is port0 or port1 that is the RPL."));

    m.add_rpc_leaf(vtss::AsVlanList(s.protected_vlans, 512),
                   vtss::tag::Name("ProtectedVlans"),
                   vtss::tag::Description("VLANs which are protected by this ring instance"));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.protected_vlans + 0 * 128, 128),
                    vtss::tag::Name("ProtectedVlans0Kto1K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(ix++),
                    vtss::tag::Description("First quarter of bit-array indicating whether a VLAN is protected by this ring instance ('1') or not ('0')."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.protected_vlans + 1 * 128, 128),
                    vtss::tag::Name("ProtectedVlans1Kto2K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(ix++),
                    vtss::tag::Description("Second quarter of bit-array indicating whether a VLAN is protected by this ring instance ('1') or not ('0')."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.protected_vlans + 2 * 128, 128),
                    vtss::tag::Name("ProtectedVlans2Kto3K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(ix++),
                    vtss::tag::Description("Third quarter of bit-array indicating whether a VLAN is protected by this ring instance ('1') or not ('0')."));

    a.add_snmp_leaf(vtss::AsVlanListQuarter(s.protected_vlans + 3 * 128, 128),
                    vtss::tag::Name("ProtectedVlans3Kto4K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(ix++),
                    vtss::tag::Description("Fourth quarter of bit-array indicating whether a VLAN is protected by this ring instance ('1') or not ('0')."));

}

//----------------------------------------------------------------------------
// Status
//----------------------------------------------------------------------------



template<typename T>
void serialize(T &a, vtss_appl_erps_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_erps_status_t"));
    int ix = 2;
    int j = 0;
    char namebuf[128];
    char descbuf[512];

#define _PORT_NAME( _name_, _desc_)    \
    memset(namebuf, 0, sizeof(namebuf)); \
    memset(descbuf, 0, sizeof(descbuf)); \
    strcpy(namebuf, j ? "Port1" : "Port0"); \
    strncat(namebuf, _name_, sizeof(namebuf) - 6); \
    strcpy(descbuf, j ? "Port1:" : "Port0:"); \
    strncat(descbuf, _desc_, sizeof(descbuf) - 7); \

    m.add_leaf(s.oper_state,
               vtss::tag::Name("OperState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The operational state of ERPS instance."));

    m.add_leaf(s.oper_warning,
               vtss::tag::Name("OperWarning"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Operational warnings of ERPS instance."));

    m.add_leaf(s.node_state,
               vtss::tag::Name("NodeState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Specifies protection/node state of ERPS."));

    m.add_leaf(vtss::AsBool(s.tx_raps_active),
               vtss::tag::Name("TxRapsActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Specifies whether we are currently supposed to be transmitting R-APS PDUs on our ring ports."));

    m.add_leaf(s.tx_raps_info.update_time_secs,
               vtss::tag::Name("TxInfoUpdateTimeSecs"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Time in seconds since boot that this structure was last updated."));

    m.add_leaf(s.tx_raps_info.request,
               vtss::tag::Name("TxInfoRequest"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Request/state according to G.8032, table 10-3."));

    m.add_leaf(s.tx_raps_info.version,
               vtss::tag::Name("TxInfoVersion"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Version of received/used R-APS Protocol. 0 means v1, 1 means v2, etc."));

    m.add_leaf(vtss::AsBool(s.tx_raps_info.rb),
               vtss::tag::Name("TxInfoRb"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("RB (RPL blocked) bit of R-APS info. See Figure 10-3 of G.8032."));

    m.add_leaf(vtss::AsBool(s.tx_raps_info.dnf),
               vtss::tag::Name("TxInfoDnf"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("DNF (Do Not Flush) bit of R-APS info. See Figure 10-3 of G.8032."));

    m.add_leaf(s.tx_raps_info.bpr,
               vtss::tag::Name("TxInfoBpr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("BPR (Blocked Port Reference) of R-APS info. See Figure 10-3 of G.8032."));

    m.add_leaf(s.tx_raps_info.node_id,
               vtss::tag::Name("TxInfoNodeId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Node ID of this request."));

    m.add_leaf(s.tx_raps_info.smac,
               vtss::tag::Name("TxInfoSmac"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The Source MAC address used in the request/state."));

    m.add_leaf(vtss::AsBool(s.cFOP_TO),
               vtss::tag::Name("cFOPTo"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Failure of Protocol - R-APS Rx Time Out."));

    for (j = 0; j <= 1; j++) {
        _PORT_NAME("StatusBlocked", "Specifies whether ring port is blocked or not.")
        m.add_leaf(vtss::AsBool(s.ring_port_status[j].blocked),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("StatusSf", "Specifies the Signal Fail state of ring port after hold-off timer has expired.")
        m.add_leaf(vtss::AsBool(s.ring_port_status[j].sf),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("StatusFopPm", "This boolean indicates whether there are two RPL owners on the ring.")
        m.add_leaf(vtss::AsBool(s.ring_port_status[j].cFOP_PM),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("StatusUpdateTimeSecs", "Time in seconds since boot that this structure was last updated.")
        m.add_leaf(s.ring_port_status[j].rx_raps_info.update_time_secs,
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("StatusRequest", "Request/state according to G.8032, table 10-3.")
        m.add_leaf(s.ring_port_status[j].rx_raps_info.request,
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("StatusVersion", "Version of received/used R-APS Protocol. 0 means v1, 1 means v2, etc. ")
        m.add_leaf(s.ring_port_status[j].rx_raps_info.version,
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("StatusRb", "RB (RPL blocked) bit of R-APS info.")
        m.add_leaf(vtss::AsBool(s.ring_port_status[j].rx_raps_info.rb),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("StatusDnf", "DNF (Do Not Flush) bit of R-APS info.")
        m.add_leaf(vtss::AsBool(s.ring_port_status[j].rx_raps_info.dnf),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("StatusBpr", "BPR (Blocked Port Reference) of R-APS info.")
        m.add_leaf(s.ring_port_status[j].rx_raps_info.bpr,
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("StatusNodeId", "Node ID of this request.")
        m.add_leaf(s.ring_port_status[j].rx_raps_info.node_id,
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("StatusSmac", "The Source MAC address used in the request/state.")
        m.add_leaf(s.ring_port_status[j].rx_raps_info.smac,
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("RxErrorCnt", "Number of received erroneous R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.rx_error_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("RxOwnCnt", "Number of received R-APS PDUs with our own node ID.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.rx_own_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("RxGuardCnt", "Number of received R-APS PDUs during guard timer.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.rx_guard_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("RxFOPPmCnt", "Number of received R-APS PDUs causing FOP-PM.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.rx_fop_pm_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("RxNrCnt", "Number of received NR R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.rx_nr_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("RxNrRbCnt", "Number of received NR, RB R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.rx_nr_rb_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("RxSfCnt", "Number of received SF R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.rx_sf_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("RxFxCnt", "Number of received FS R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.rx_fs_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("RxMsCnt", "Number of received MS R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.rx_ms_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("RxEventCnt", "Number of received Event R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.rx_event_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("TxNrCnt", "Number of transmitted NR R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.tx_nr_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("TxNrRbCnt", "Number of transmitted NR, RB R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.tx_nr_rb_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("TxSfCnt", "Number of transmitted SF R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.tx_sf_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("TxFsCnt", "Number of transmitted FS R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.tx_fs_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("TxMsCnt", "Number of transmitted MS R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.tx_ms_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("TxEventCnt", "Number of transmitted Event R-APS PDUs.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.tx_event_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("SfCn", "Number of local signal fails.")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.sf_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));

        _PORT_NAME("FlushCnt", "Number of FDB flushes (same for both rings).")
        m.add_leaf(vtss::AsCounter(s.ring_port_status[j].statistics.flush_cnt),
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(descbuf));
    }
}


//----------------------------------------------------------------------------
// Control
//----------------------------------------------------------------------------

template<typename T>
void serialize(T &a, vtss_appl_erps_control_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_erps_control_t"));
    int ix = 2;

    m.add_leaf(s.command,
               vtss::tag::Name("Command"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Control command to execute. Always returns none when read."));
}


namespace vtss
{
namespace appl
{
namespace erps
{
namespace interfaces
{

struct ErpsCapabilities {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_erps_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_erps_capabilities_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_erps_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ERPS);
};

struct ErpsConfTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<vtss_appl_erps_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is the ERPS group configuration table.";

    static constexpr const char *index_description =
        "Each entry in this table represents an ERPS group.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        serialize(h, vtss_appl_erps_u32_dsc(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_erps_conf_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_erps_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_erps_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_erps_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_erps_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_erps_conf_del);
    VTSS_EXPOSE_DEF_PTR(vtss_erps_create_conf_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_ERPS);
};

struct ErpsStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<vtss_appl_erps_status_t *>
         > P;

    static constexpr const char *table_description =
        "This table contains status per EPRS group.";

    static constexpr const char *index_description =
        "Status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        serialize(h, vtss_appl_erps_u32_dsc(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_erps_status_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_erps_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_erps_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ERPS);
};

struct ErpsControlCommandTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<vtss_appl_erps_control_t *>
         > P;

    static constexpr const char *table_description =
        "This is the ERPS group control table.";

    static constexpr const char *index_description =
        "Each entry in this table represents dynamic control elements an ERPS group.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        serialize(h, vtss_appl_erps_u32_dsc(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_erps_control_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_erps_control_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_erps_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_erps_control_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_ERPS);
};


struct ErpsControlStatisticsClearTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<u32>,
         vtss::expose::ParamVal<BOOL *>
         > P;

    static constexpr const char *table_description =
        "This is a table of created ERPS clear commands.";

    static constexpr const char *index_description =
        "This is a created ERPS clear command.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i)
    {
        serialize(h, vtss_appl_erps_u32_dsc(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(tag::Name("clear"));
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, vtss_erps_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_erps_itr);
    VTSS_EXPOSE_SET_PTR(vtss_erps_statistics_clear);
    VTSS_EXPOSE_GET_PTR(vtss_erps_statistics_clear_dummy);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ERPS);
};


}  // namespace interfaces
}  // namespace erps
}  // namespace appl
}  // namespace vtss
#endif  /* __VTSS_ERPS_SERIALIZER_HXX__ */
