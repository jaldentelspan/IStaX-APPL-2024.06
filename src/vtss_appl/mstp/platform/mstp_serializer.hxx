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

#ifndef _MSTP_SERIALIZER_HXX_
#define _MSTP_SERIALIZER_HXX_

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/mstp.h"

struct vtss_appl_mstp_msti_config_name_and_rev_t {
    char configname[VTSS_APPL_MSTP_CONFIG_NAME_MAXLEN];  /*!< The Configuration Name */
    u16 revision;                                   /*!< The Configuration Revision */
};

struct vtss_appl_mstp_vlan_msti_config {
    vtss_appl_mstp_mstid_t msti;    /*!< The MSTID of this VLAN */
};

mesa_rc mstp_msti_itr(const vtss_appl_mstp_msti_t *prev_msti, vtss_appl_mstp_msti_t *next_msti);
mesa_rc mstp_msti_port_itr(const vtss_ifindex_t  *prev_ifindex, vtss_ifindex_t *next_ifindex, const vtss_appl_mstp_msti_t *prev_msti,   vtss_appl_mstp_msti_t *next_msti);
mesa_rc mstp_msti_port_exist_itr(const vtss_ifindex_t  *prev_ifindex, vtss_ifindex_t *next_ifindex, const vtss_appl_mstp_msti_t *prev_msti,   vtss_appl_mstp_msti_t *next_msti);
mesa_rc vtss_appl_mstp_msti_config_name_and_rev_get(vtss_appl_mstp_msti_config_name_and_rev_t *conf);
mesa_rc vtss_appl_mstp_msti_config_name_and_rev_set(const vtss_appl_mstp_msti_config_name_and_rev_t *conf);
mesa_rc vtss_appl_mstp_msti_config_get_table(mesa_vid_t vid, vtss_appl_mstp_vlan_msti_config *conf);
mesa_rc vtss_appl_mstp_msti_config_table_itr(const mesa_vid_t *prev_ifindex, mesa_vid_t *next_ifindex);
mesa_rc vtss_appl_mstp_msti_config_set_table(mesa_vid_t vid, const vtss_appl_mstp_vlan_msti_config *conf);

extern vtss_enum_descriptor_t mstp_forceVersion_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_mstp_forceversion_t,
                         "MSTPForceVersion",
                         mstp_forceVersion_txt,
                         "This enumeration control the STP protocol variant to run.");

extern vtss_enum_descriptor_t mstp_portstate_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_mstp_portstate_t,
                         "MstpPortState",
                         mstp_portstate_txt,
                         "This enumeration describe the forwarding state of an interface.");

extern vtss_enum_descriptor_t mstp_p2p_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_mstp_p2p_t,
                         "MstpPoint2Point",
                         mstp_p2p_txt,
                         "This enumeration describe the values of adminPointToPointMAC and operPointToPointMAC parameters. (Full duplex port administrative and operational status.) See 6.4.3 of IEEE Std 802.1D.");

struct MSTP_msti_index_1 {
    MSTP_msti_index_1(vtss_appl_mstp_msti_t &x) : inner(x) { }
    vtss_appl_mstp_msti_t &inner;
};

struct MSTP_msti_index_2 {
    MSTP_msti_index_2(vtss_appl_mstp_msti_t &x) : inner(x) { }
    vtss_appl_mstp_msti_t &inner;
};

struct MSTP_ifindex_index {
    MSTP_ifindex_index(vtss_ifindex_t &x) : inner(x) { }
    vtss_ifindex_t &inner;
};

struct MSTP_msti_value_index {
    MSTP_msti_value_index(vtss_appl_mstp_mstid_t &x) : inner(x) {}
    vtss_appl_mstp_mstid_t &inner;
};

template<typename T>
void serialize(T &a, MSTP_msti_value_index s)
{
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("MstiValue"),
               vtss::expose::snmp::RangeSpec<vtss_appl_mstp_mstid_t>(0, VTSS_MSTID_TE),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("MSTI value."));
}

template<typename T>
void serialize(T &a, MSTP_ifindex_index s)
{
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("InterfaceNo"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Logical interface number."));
}

template<typename T>
void serialize(T &a, MSTP_msti_index_1 s)
{
    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("Instance"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 255),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Bridge instance number. The CIST = 0, MSTI1 = 1, etc"));
}

template<typename T>
void serialize(T &a, MSTP_msti_index_2 s)
{
    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("Instance"),
               vtss::expose::snmp::RangeSpec<uint32_t>(0, 255),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("Bridge instance number. The CIST = 0, MSTI1 = 1, etc"));
}

template<typename T>
void serialize(T &a, vtss_appl_mstp_bridge_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mstp_bridge_status_t"));
    int ix = 2;  // After instance index

    m.add_leaf(vtss::AsOctetString(s.bridgeId, sizeof(s.bridgeId)), vtss::tag::Name("BridgeId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Bridge Identifier of this bridge"));

    m.add_leaf(s.timeSinceTopologyChange, vtss::tag::Name("TimeSinceTopologyChange"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The count in seconds of the time elapsed since the Topology Change flag was last True"));

    m.add_leaf(s.topologyChangeCount, vtss::tag::Name("TopologyChangeCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The count of the times the Topology Change flag parameter for the Bridge has been set since the Bridge was powered on or initialized"));

    m.add_leaf(vtss::AsBool(s.topologyChange), vtss::tag::Name("TopologyChange"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Topology Change Flag current status"));

    m.add_leaf(vtss::AsOctetString(s.designatedRoot, sizeof(s.designatedRoot)), vtss::tag::Name("DesignatedRoot"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Designated Root Bridge"));

    m.add_leaf(s.rootPathCost, vtss::tag::Name("RootPathCost"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Root Path Cost"));

    m.add_leaf(s.rootPort, vtss::tag::Name("RootPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Root Port"));

    m.add_leaf(s.maxAge, vtss::tag::Name("MaxAge"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Max Age, IEEE-802.1D-2004 sect 13.23.7"));

    m.add_leaf(s.forwardDelay, vtss::tag::Name("ForwardDelay"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Forward Delay, IEEE-802.1D-2004 sect 13.23.7"));

    m.add_leaf(s.bridgeMaxAge, vtss::tag::Name("BridgeMaxAge"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Bridge Max Age, IEEE-802.1D-2004 sect 13.23.4"));
    m.add_leaf(s.bridgeHelloTime, vtss::tag::Name("BridgeHelloTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Bridge Hello Time, IEEE-802.1D-2004 sect 13.23.4"));

    m.add_leaf(s.bridgeForwardDelay, vtss::tag::Name("BridgeForwardDelay"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Bridge Forward Delay, IEEE-802.1D-2004 sect 13.23.4"));

    m.add_leaf(s.txHoldCount, vtss::tag::Name("TxHoldCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Hold Time / Transmission Limit, IEEE-802.1D-2004 sect 13.22"));

    m.add_leaf(s.forceVersion, vtss::tag::Name("ForceVersion"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Value of the Force Protocol Version parameter - IEEE-802.1D-2004 sect 17.16.1"));

    m.add_leaf(vtss::AsOctetString(s.cistRegionalRoot, sizeof(s.cistRegionalRoot)), vtss::tag::Name("CistRegionalRoot"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("CIST Regional Root Identifier (13.16.4)"));

    m.add_leaf(s.cistInternalPathCost, vtss::tag::Name("CistInternalPathCost"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("CIST Path Cost"));

    m.add_leaf(s.maxHops, vtss::tag::Name("MaxHops"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("MaxHops (13.22.1)"));
}

template<typename T>
void serialize(T &a, vtss_appl_mstp_bridge_param_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mstp_bridge_param_t"));
    int ix = 1;
    m.add_leaf(s.bridgeMaxAge, vtss::tag::Name("BridgeMaxAge"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Bridge Max Age, IEEE-802.1D-2004 sect 13.23.4"));

    m.add_leaf(s.bridgeHelloTime, vtss::tag::Name("BridgeHelloTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::expose::snmp::RangeSpec<u32>(1, 2),
               vtss::tag::Description("Bridge Hello Time, 13.25.7 of IEEE-802.1Q-2005. Fixed value of two seconds by the standard, but this implementation allow a compatibility range from 1 to 2 seconds, in stipulated in 802.1Q-2005"));

    m.add_leaf(s.bridgeForwardDelay, vtss::tag::Name("BridgeForwardDelay"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::expose::snmp::RangeSpec<u32>(4, 30),
               vtss::tag::Description("Bridge Forward Delay, IEEE-802.1D-2004 sect 17.20"));

    m.add_leaf(s.forceVersion, vtss::tag::Name("ForceVersion"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("value of the Force Protocol Version parameter - 13.6.2 of IEEE-802.1Q-2005"));

    m.add_leaf(s.txHoldCount, vtss::tag::Name("TxHoldCount"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::expose::snmp::RangeSpec<u32>(1, 10),
               vtss::tag::Description("TxHoldCount - 17.13.12 of IEEE Std 802.1D"));

    m.add_leaf(s.MaxHops, vtss::tag::Name("MaxHops"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::expose::snmp::RangeSpec<u32>(6, 40),
               vtss::tag::Description("MaxHops - 13.22.1 of IEEE-802.1Q-2005"));

    m.add_leaf(vtss::AsBool(s.bpduFiltering), vtss::tag::Name("BpduFiltering"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("BPDU filtering for edge ports. Control whether a port explicitly configured as Edge will transmit and receive BPDUs"));

    m.add_leaf(vtss::AsBool(s.bpduGuard), vtss::tag::Name("BpduGuard"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("BPDU guard for edge ports. Control whether a port explicitly configured as Edge will disable itself upon reception of a BPDU. The port will enter the error-disabled state, and will be removed from the active topology. "));

    m.add_leaf(s.errorRecoveryDelay, vtss::tag::Name("ErrorRecoveryDelay"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of seconds until a STP inconsistent port is recovered. Valid values are zero (recovery disabled) or between 30 and 86400 (24 hours)"));
}

template<typename T>
void serialize(T &a, vtss_appl_mstp_msti_config_name_and_rev_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mstp_msti_config_name_and_rev_t"));
    m.add_leaf(vtss::AsDisplayString(s.configname, sizeof(s.configname)),
               vtss::tag::Name("Name"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The configuration name"));
    m.add_leaf(s.revision, vtss::tag::Name("Revision"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("The configuration revision"));
}

struct MSTP_vid_index {
    MSTP_vid_index(mesa_vid_t &x) : inner(x) { }
    mesa_vid_t &inner;
};

template<typename T>
void serialize(T &a, MSTP_vid_index s)
{
    a.add_leaf(vtss::AsInt(s.inner), vtss::tag::Name("Vid"),
               vtss::expose::snmp::RangeSpec<uint32_t>(1, 4095),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Vlan id"));
}

template<typename T>
void serialize(T &a, vtss_appl_mstp_vlan_msti_config &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mstp_vlan_msti_config"));
    m.add_leaf(s.msti, vtss::tag::Name("Mstid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The MSTID value associated with the vlan id"));
}

template<typename T>
void serialize(T &a, vtss_appl_mstp_msti_param_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mstp_msti_param_t"));
    int ix = 2;  // After MSTI index
    m.add_leaf(s.priority, vtss::tag::Name("Priority"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Bridge Priority"));
}

template<typename T>
void serialize(T &a, vtss_appl_mstp_port_config_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mstp_port_config_t"));
    int ix = 1;  // After interface index

    m.add_leaf(vtss::AsBool(s.enable), vtss::tag::Name("Enable"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Control whether port is controlled by xSTP. If disabled, the port forwarding state follow the MAC state"));

    m.add_leaf(vtss::AsBool(s.param.adminEdgePort), vtss::tag::Name("AdminEdgePort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("adminEdgePort parameter - 18.3.3 of IEEE Std 802.1D"));

    m.add_leaf(vtss::AsBool(s.param.adminAutoEdgePort), vtss::tag::Name("AdminAutoEdgePort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("adminAutoEdgePort parameter - 17.13.3 of IEEE Std 802.1D"));

    m.add_leaf(s.param.adminPointToPointMAC,  vtss::tag::Name("adminPointToPointMAC"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("adminPointToPointMAC parameter - 6.4.3 of IEEE Std 802.1D"));

    m.add_leaf(vtss::AsBool(s.param.restrictedRole), vtss::tag::Name("RestrictedRole"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("restrictedRole parameter - 13.25.14 of IEEE Std 802.1Q-2005"));

    m.add_leaf(vtss::AsBool(s.param.restrictedTcn), vtss::tag::Name("RestrictedTcn"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("restrictedRole parameter - 13.25.15 of IEEE Std 802.1Q-2005"));

    m.add_leaf(vtss::AsBool(s.param.bpduGuard), vtss::tag::Name("BpduGuard"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If enabled, causes the port to disable itself upon receiving valid BPDU's. Contrary to the similar bridge setting, the port Edge status does not effect this setting. A port entering error-disabled state due to this setting is subject to the bridge ErrorRecoveryDelay setting as well"));
}

template<typename T>
void serialize(T &a, vtss_appl_mstp_msti_port_param_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mstp_msti_port_param_t"));
    int ix = 3;  // After interface index, msti index

    m.add_leaf(s.adminPathCost, vtss::tag::Name("AdminPathCost"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Path Cost - 13.37.1 of 802.1Q-2005"));

    m.add_leaf(s.adminPortPriority, vtss::tag::Name("AdminPortPriority"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("priority field for the Port Identifier - 13.24.12 of 802.1Q-2005"));
}

template<typename T>
void serialize(T &a, vtss_appl_mstp_port_mgmt_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mstp_port_mgmt_status_t"));
    int ix = 3;  // After interface index, msti

    m.add_leaf(vtss::AsBool(s.enabled), vtss::tag::Name("Enabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Whether port is controlled by xSTP"));

    m.add_leaf(vtss::AsBool(s.active), vtss::tag::Name("Active"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Whether port is active"));

    m.add_leaf(s.parent, vtss::tag::Name("ParentPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Parent port if physical port is aggregated. (Otherwise 0xffff)"));

    m.add_leaf(s.core.uptime, vtss::tag::Name("UpTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("seconds of the time elapsed since the Port was last reset or initialized"));

    m.add_leaf(s.core.state, vtss::tag::Name("PortState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Current state of the Port"));

    m.add_leaf(vtss::AsOctetString(s.core.portId, sizeof(s.core.portId)), vtss::tag::Name("PortId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Unique Port identifier comprising two parts, the Port Number and the Port Priority field"));

    m.add_leaf(s.core.pathCost, vtss::tag::Name("PathCost"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Path Cost (17.16.5 of IEEE Std 802.1D)"));

    m.add_leaf(vtss::AsOctetString(s.core.designatedRoot, sizeof(s.core.designatedRoot)), vtss::tag::Name("DesignatedRoot"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Designated Root"));

    m.add_leaf(s.core.designatedCost, vtss::tag::Name("DesignatedCost"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Designated Cost"));

    m.add_leaf(vtss::AsOctetString(s.core.designatedBridge, sizeof(s.core.designatedBridge)), vtss::tag::Name("DesignatedBridge"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Designated Bridge"));

    m.add_leaf(vtss::AsOctetString(s.core.designatedPort, sizeof(s.core.designatedPort)), vtss::tag::Name("DesignatedPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Designated Port"));

    m.add_leaf(vtss::AsBool(s.core.tcAck), vtss::tag::Name("TcAck"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Topology Change Acknowledge"));

    m.add_leaf(s.core.helloTime, vtss::tag::Name("HelloTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Hello Time"));

    m.add_leaf(vtss::AsBool(s.core.adminEdgePort), vtss::tag::Name("AdminEdgePort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("adminEdgePort (18.3.3 of IEEE Std 802.1D)"));

    m.add_leaf(vtss::AsBool(s.core.operEdgePort), vtss::tag::Name("OperEdgePort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("operEdgePort (18.3.4 of IEEE Std 802.1D)"));

    m.add_leaf(vtss::AsBool(s.core.autoEdgePort), vtss::tag::Name("AutoEdgePort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("autoEdgePort (17.13.3 of IEEE Std 802.1D)"));

    m.add_leaf(vtss::AsBool(s.core.macOperational), vtss::tag::Name("MacOperational"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Current state of the MAC Operational parameter (6.4.2 of IEEE Std 802.1D,)"));

    m.add_leaf(s.core.adminPointToPointMAC, vtss::tag::Name("AdminPointToPointMAC"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Current state of the adminPointToPointMAC parameter (6.4.3 of IEEE Std 802.1D)"));

    m.add_leaf(vtss::AsBool(s.core.operPointToPointMAC), vtss::tag::Name("OperPointToPointMAC"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Current state of the operPointToPointMAC parameter (6.4.3 of IEEE Std 802.1D)"));

    m.add_leaf(vtss::AsBool(s.core.restrictedRole), vtss::tag::Name("RestrictedRole"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Current state of the restrictedRole parameter for the Port (13.25.14)"));

    m.add_leaf(vtss::AsBool(s.core.restrictedTcn), vtss::tag::Name("RestrictedTcn"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Current state of the restrictedTcn parameter for the Port (13.25.15)"));

    m.add_leaf(vtss::AsDisplayString(s.core.rolestr, sizeof(s.core.rolestr)), vtss::tag::Name("PortRole"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Current Port Role"));

    m.add_leaf(vtss::AsBool(s.core.disputed), vtss::tag::Name("Disputed"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Current value of the disputed variable for the CIST Port"));
}

template<typename T>
void serialize(T &a, vtss_appl_mstp_port_statistics_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mstp_port_statistics_t"));
    int ix = 2;  // After interface index

    m.add_leaf(s.stp_frame_xmits, vtss::tag::Name("StpFrameXmits"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of STP frames transmitted"));

    m.add_leaf(s.stp_frame_recvs, vtss::tag::Name("StpFrameReceived"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of STP frames received"));

    m.add_leaf(s.rstp_frame_xmits, vtss::tag::Name("RstpFrameXmits"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of RSTP frames transmitted"));

    m.add_leaf(s.rstp_frame_recvs, vtss::tag::Name("RstpFrameReceived"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of RSTP frames received"));

    m.add_leaf(s.mstp_frame_xmits, vtss::tag::Name("MstpFrameXmits"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of MSTP frames transmitted"));

    m.add_leaf(s.mstp_frame_recvs, vtss::tag::Name("MstpFrameReceived"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of MSTP frames received"));

    m.add_leaf(s.unknown_frame_recvs, vtss::tag::Name("UnknownFramesReceived"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of unknown frames received and discarded in error"));

    m.add_leaf(s.illegal_frame_recvs, vtss::tag::Name("IllegalFrameReceived"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of illegal frames received and discarded in error"));

    m.add_leaf(s.tcn_frame_xmits, vtss::tag::Name("TcnFrameXmits"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of TCN frames transmitted"));

    m.add_leaf(s.tcn_frame_recvs, vtss::tag::Name("TcnFrameReceived"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of TCN frames received"));
}

template<typename T>
void serialize(T &a, vtss_appl_mstp_vlan_bitmap_t &s)
{
    using namespace vtss;
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_mstp_vlan_bitmap_t"));

    int idx = 1;  // after msti

    m.add_rpc_leaf(vtss::AsVlanList(s.vlan_bitmap, 512),
                   vtss::tag::Name("VlanBitmap"),
                   vtss::tag::Description("Vlans set with a specific MSTI value."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.vlan_bitmap +   0, 128),
                    vtss::tag::Name("AccessVlans0To1K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("First quarter of bit-array indicating the enabled access VLANs."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.vlan_bitmap + 128, 128),
                    vtss::tag::Name("AccessVlans1KTo2K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Second quarter of bit-array indicating the enabled access VLANs."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.vlan_bitmap + 256, 128),
                    vtss::tag::Name("AccessVlans2KTo3K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Third quarter of bit-array indicating the enabled access VLANs."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.vlan_bitmap + 384, 128),
                    vtss::tag::Name("AccessVlans3KTo4K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Last quarter of bit-array indicating the enabled access VLANs."));
}

namespace vtss
{
namespace appl
{
namespace mstp
{
namespace interfaces
{
struct MstpBridgeLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_mstp_bridge_param_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mstp_bridge_param_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_system_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mstp_system_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP);
};

struct MstpMstiParamsTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_mstp_msti_t>,
         vtss::expose::ParamVal<vtss_appl_mstp_msti_param_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table of the bridge instance (MSTIs) parameters";

    static constexpr const char *index_description =
        "Each MSTI has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mstp_msti_t &i)
    {
        serialize(h, MSTP_msti_index_1(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mstp_msti_param_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_msti_param_get);
    VTSS_EXPOSE_ITR_PTR(mstp_msti_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mstp_msti_param_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP);
};

struct MstpMstiConfigLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_mstp_msti_config_name_and_rev_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mstp_msti_config_name_and_rev_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_msti_config_name_and_rev_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mstp_msti_config_name_and_rev_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP);
};

struct MstpMstiConfigTableEntries {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<mesa_vid_t>,
         vtss::expose::ParamVal<vtss_appl_mstp_vlan_msti_config *>
         > P;

    static constexpr const char *table_description =
        "This is the 802.1Q - 8.9 MST Configuration table\n          For the purposes of calculating the Configuration Digest, the MST          Configuration Table is considered to contain 4096 consecutive          single octet elements, where each element of the table (with the          exception of the first and last) contains an MSTID value.";

    static constexpr const char *index_description =
        "The first element of the table contains the value 0, the second element          the MSTID value corresponding to VID 1, the third element the MSTID          value corresponding to VID 2, and so on, with the next to last          element of the table containing the MSTID value corresponding to          VID 4094, and the last element containing the value 0.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_vid_t &i)
    {
        serialize(h, MSTP_vid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mstp_vlan_msti_config &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_msti_config_get_table);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_mstp_msti_config_table_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mstp_msti_config_set_table);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP);
};

struct MstpMstiConfigVlanBitmapTableEntries {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_mstp_mstid_t>,
         vtss::expose::ParamVal<vtss_appl_mstp_vlan_bitmap_t *>
         > P;

    static constexpr const char *table_description =
        "This is the 802.1Q - 8.9 MST Configuration table\n          For the purposes of calculating the Configuration Digest, the MST Configuration Table is considered to contain N elements, one for each MSTID, where each element of the table contains a VLAN bitmap (vlans that have the MSTID value specified are set to one).";

    static constexpr const char *index_description =
        "The first element of the table contains a VLAN bitmap corresponding to MSTID 0, the second element contains a VLAN bitmap corresponding to MSTID 1 until the Nth element. MSTID range is 0-7, 4094";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mstp_mstid_t &i)
    {
        serialize(h, MSTP_msti_value_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mstp_vlan_bitmap_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_msti_table_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_mstp_msti_table_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mstp_msti_table_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP);
};

struct MstpCistportParamsTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_mstp_port_config_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table of the CIST physical interface parameters";

    static constexpr const char *index_description =
        "Each CIST physical interface has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        serialize(h, MSTP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mstp_port_config_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_interface_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mstp_interface_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP);
};

struct MstpAggrParamLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_mstp_port_config_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mstp_port_config_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_aggregation_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mstp_aggregation_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP);
};

struct MstpMstiportParamTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamKey<vtss_appl_mstp_msti_t>,
         vtss::expose::ParamVal<vtss_appl_mstp_msti_port_param_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table of the MSTI interface parameters";

    static constexpr const char *index_description =
        "Each MSTI interface has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        serialize(h, MSTP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mstp_msti_t &i)
    {
        serialize(h, MSTP_msti_index_2(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_mstp_msti_port_param_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_interface_mstiport_config_get);
    VTSS_EXPOSE_ITR_PTR(mstp_msti_port_exist_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mstp_interface_mstiport_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP);
};

struct MstpMstiportAggrparamTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_mstp_msti_t>,
         vtss::expose::ParamVal<vtss_appl_mstp_msti_port_param_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table of the MSTI aggregations parameters";

    static constexpr const char *index_description =
        "The aggregations for each MSTI has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mstp_msti_t &i)
    {
        serialize(h, MSTP_msti_index_1(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mstp_msti_port_param_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_aggregation_mstiport_config_get);
    VTSS_EXPOSE_ITR_PTR(mstp_msti_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mstp_aggregation_mstiport_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP);
};

struct MstpBridgeStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_mstp_msti_t>,
         vtss::expose::ParamVal<vtss_appl_mstp_bridge_status_t *>
         > P;

    static constexpr const char *table_description =
        "This table represent the status of the bridge instances";

    static constexpr const char *index_description =
        "A MSTP Bridge instance set of status objects";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mstp_msti_t &i)
    {
        serialize(h, MSTP_msti_index_1(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mstp_bridge_status_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_bridge_status_get);
    VTSS_EXPOSE_ITR_PTR(mstp_msti_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_RSTP);
};

struct MstpPortStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamKey<vtss_appl_mstp_msti_t>,
         vtss::expose::ParamVal<vtss_appl_mstp_port_mgmt_status_t *>
         > P;

    static constexpr const char *table_description =
        "This table represent the status of the interface instances";

    static constexpr const char *index_description =
        "A MSTP interface instance set of status objects";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        serialize(h, MSTP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mstp_msti_t &i)
    {
        serialize(h, MSTP_msti_index_2(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_mstp_port_mgmt_status_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_interface_status_get);
    VTSS_EXPOSE_ITR_PTR(mstp_msti_port_exist_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_RSTP);
};

struct MstpPortStatsTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_mstp_port_statistics_t *>
         > P;

    static constexpr const char *table_description =
        "This table represent the statistics of the CIST interfaces";

    static constexpr const char *index_description =
        "A CIST interface set of statistics";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        serialize(h, MSTP_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mstp_port_statistics_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mstp_interface_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_RSTP);
};
}  // namespace interfaces
}  // namespace mstp
}  // namespace appl
}  // namespace vtss

#endif // _MSTP_SERIALIZER_HXX_

