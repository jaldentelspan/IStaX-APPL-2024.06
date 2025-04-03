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
#ifndef __VTSS_DHCP_RELAY_SERIALIZER_HXX__
#define __VTSS_DHCP_RELAY_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/dhcp_relay.h"

/*****************************************************************************
    Enum serializer
*****************************************************************************/
extern vtss_enum_descriptor_t dhcp_relay_information_policy_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_dhcp_relay_information_policy_t,
                         "DhcpRelayInformationPolicyType",
                         dhcp_relay_information_policy_txt,
                         "This enumeration indicates the DHCP relay information policy type.");

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_dhcp_relay_param_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_relay_param_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Global mode of DHCP relay. true is to "
            "enable DHCP relay and false is to disable it.")
    );

    m.add_leaf(
        vtss::AsIpv4(s.serverIpAddr),
        vtss::tag::Name("ServerIpAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Server IP address. This IP address is for "
            "DHCP server where the DHCP relay will relay DHCP packets to.")
    );

    m.add_leaf(
        vtss::AsBool(s.informationMode),
        vtss::tag::Name("InformationMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates the DHCP relay information mode option operation. "
            "Possible modes are - Enabled: Enable DHCP relay information mode "
            "operation. When DHCP relay information mode operation is enabled, "
            "the agent inserts specific information (option 82) into a DHCP message "
            "when forwarding to DHCP server and removes it from a DHCP message "
            "when transferring to DHCP client. It only works when DHCP relay "
            "operation mode is enabled. Disabled: Disable DHCP relay information "
            "mode operation.")
    );

    m.add_leaf(
        s.informationPolicy,
        vtss::tag::Name("InformationPolicy"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates the DHCP relay information option policy. When DHCP relay "
            "information mode operation is enabled, if the agent receives a DHCP "
            "message that already contains relay agent information it will enforce "
            "the policy. The 'Replace' policy is invalid when relay information "
            "mode is disabled.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_relay_statistics_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_relay_statistics_t"));
    int ix = 0;

    m.add_leaf(
        s.server_packets_relayed,
        vtss::tag::Name("ServerPacketsRelayed"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Packets relayed from server to client.")
    );

    m.add_leaf(
        s.server_packet_errors,
        vtss::tag::Name("ServerPacketErrors"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Errors sending packets to servers.")
    );

    m.add_leaf(
        s.client_packets_relayed,
        vtss::tag::Name("ClientPacketsRelayed"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Packets relayed from client to server.")
    );

    m.add_leaf(
        s.client_packet_errors,
        vtss::tag::Name("ClientPacketErrors"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Errors sending packets to clients.")
    );

    m.add_leaf(
        s.agent_option_errors,
        vtss::tag::Name("AgentOptionErrors"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of packets forwarded without agent "
            "options because there was no room.")
    );

    m.add_leaf(
        s.missing_agent_option,
        vtss::tag::Name("MissingAgentOption"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Number of packets dropped because no RAI "
            "option matching our ID was found.")
    );

    m.add_leaf(
        s.bad_circuit_id,
        vtss::tag::Name("BadCircuitId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Circuit ID option in matching RAI option "
            "did not match any known circuit ID.")
    );

    m.add_leaf(
        s.missing_circuit_id,
        vtss::tag::Name("MissingCircuitId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Circuit ID option in matching RAI option "
            "was missing.")
    );

    m.add_leaf(
        s.bad_remote_id,
        vtss::tag::Name("BadRemoteId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Remote ID option in matching RAI option "
            "did not match any known remote ID.")
    );

    m.add_leaf(
        s.missing_remote_id,
        vtss::tag::Name("MissingRemoteId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Remote ID option in matching RAI option "
            "was missing.")
    );

    m.add_leaf(
        s.receive_server_packets,
        vtss::tag::Name("ReceiveServerPackets"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Receive DHCP message from server.")
    );

    m.add_leaf(
        s.receive_client_packets,
        vtss::tag::Name("ReceiveClientPackets"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Receive DHCP message from client.")
    );

    m.add_leaf(
        s.receive_client_agent_option,
        vtss::tag::Name("ReceiveClientAgentOption"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Receive relay agent information option from client.")
    );

    m.add_leaf(
        s.replace_agent_option,
        vtss::tag::Name("ReplaceAgentOption"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Replace relay agent information option.")
    );

    m.add_leaf(
        s.keep_agent_option,
        vtss::tag::Name("KeepAgentOption"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Keep relay agent information option.")
    );

    m.add_leaf(
        s.drop_agent_option,
        vtss::tag::Name("DropAgentOption"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Drop relay agent information option.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_dhcp_relay_control_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_dhcp_relay_control_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.clearStatistics),
        vtss::tag::Name("ClearStatistics"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The action to clear statistics. true is to "
            "clear the statistics data. false, then, does nothing.")
    );
}

namespace vtss {
namespace appl {
namespace dhcp_relay {
namespace interfaces {

struct DhcpRelayParamsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcp_relay_param_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp_relay_param_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_relay_system_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_relay_system_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpRelayStatisticsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcp_relay_statistics_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp_relay_statistics_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_relay_statistics_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_DHCP);
};

struct DhcpRelayControlLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_dhcp_relay_control_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_dhcp_relay_control_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_dhcp_relay_control_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_dhcp_relay_control_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_DHCP);
};

}  // namespace interfaces
}  // namespace dhcp_relay
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_DHCP_RELAY_SERIALIZER_HXX__ */
