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
#ifndef __ETH_LINK_OAM_SERIALIZER_HXX__
#define __ETH_LINK_OAM_SERIALIZER_HXX__

#include "vtss/basics/snmp.hxx"
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/eth_link_oam.h"
#include "vtss/appl/interface.h"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss_common_iterator.hxx"

mesa_rc eth_link_oam_stats_dummy_get(vtss_ifindex_t ifindex, BOOL *const clear);
mesa_rc eth_link_oam_stats_clr_set(vtss_ifindex_t ifindex, const BOOL *const clear);

extern vtss::expose::TableStatus <vtss::expose::ParamKey<vtss_ifindex_t>,
       vtss::expose::ParamVal<vtss_appl_eth_link_oam_crit_link_event_statistics_t *>>
       eth_link_oam_crit_event_update;

extern const vtss_enum_descriptor_t vtss_appl_eth_link_oam_control_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_eth_link_oam_control_t, "ethLinkOamAdminStateType",
                         vtss_appl_eth_link_oam_control_txt,
                         "This enumeration defines the types of Ethernet Link OAM Administrative State.");

extern const vtss_enum_descriptor_t vtss_appl_eth_link_oam_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_eth_link_oam_mode_t, "ethLinkOamModeType",
                         vtss_appl_eth_link_oam_mode_txt,
                         "This enumeration defines the types of Ethernet Link OAM mode.");

extern const vtss_enum_descriptor_t vtss_appl_eth_link_oam_discovery_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_eth_link_oam_discovery_state_t, "ethLinkOamDiscoveryStateType",
                         vtss_appl_eth_link_oam_discovery_state_txt,
                         "This enumeration defines the types of Ethernet Link OAM discovery state.");

extern const vtss_enum_descriptor_t vtss_appl_eth_link_oam_pdu_control_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_eth_link_oam_pdu_control_t, "ethLinkOamPduControlType",
                         vtss_appl_eth_link_oam_pdu_control_txt,
                         "This enumeration defines the types of the Ethernet Link OAM PDU permission.");

extern const vtss_enum_descriptor_t vtss_appl_eth_link_oam_mux_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_eth_link_oam_mux_state_t, "ethLinkOamMuxStateType",
                         vtss_appl_eth_link_oam_mux_state_txt,
                         "This enumeration defines the types of the Ethernet Link OAM Multiplexer state.");

extern const vtss_enum_descriptor_t vtss_appl_eth_link_oam_parser_state_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_eth_link_oam_parser_state_t, "ethLinkOamParserStateType",
                         vtss_appl_eth_link_oam_parser_state_txt,
                         "This enumeration defines the types of the Ethernet Link OAM Parser state.");

/*****************************************************************************
  - MIB row/table indexes serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(LOAM_ifindex_index, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface number."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_appl_eth_link_oam_ctrl_bool_t, BOOL, a, s) {
    a.add_leaf(vtss::AsBool(s.inner), vtss::tag::Name("StatisticsClear"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Set to TRUE to clear the Link OAM statistics of an interface."));
}

template <typename T>
void serialize(T &a, vtss_appl_eth_link_oam_port_conf_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eth_link_oam_port_conf_t"));
    int ix = 0;
    m.add_leaf(s.admin_state, vtss::tag::Name("AdminState"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Set administrative state for this interface. "
                   "Enabling AdminState indicates that "
                   "the Ethernet Link OAM will attempt to operate over this interface."));
    
    m.add_leaf(s.mode, vtss::tag::Name("Mode"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Configure the mode of OAM operation for this interface. "
                   "OAM on Ethernet interfaces may be Active mode or Passive mode. "
                   "These two modes differ in that active mode provides additional capabilities to initiate "
                   "monitoring activities with the remote OAM peer entity, while "
                   "passive mode generally waits for the peer to initiate OA "
                   "actions with it.  As an example, an active OAM entity can put "
                   "the remote OAM entity in a loopback state, where a passive OAM entity cannot."));
    
    m.add_leaf(vtss::AsBool(s.mib_retrieval_support), vtss::tag::Name("MibRetrievalSupport"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Controls the MIB retrieval support for this interface. "
                   "Enabling MIB retrieval support indicates that the OAM entity supports polling of "
                   "various Link OAM based MIB variables contents."));
    
    m.add_leaf(vtss::AsBool(s.remote_loopback_support), vtss::tag::Name("RemoteLoopbackSupport"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Controls the remote loopback support for this interface. "
                   "Enabling loopback support indicates that the OAM entity can initiate "
                   "and respond to loopback commands"));
    
    m.add_leaf(vtss::AsBool(s.link_monitoring_support), vtss::tag::Name("LinkMonitorSupport"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Controls the link monitor support for this interface. "
                   "Enabling link monitor support indicates that "
                   "the OAM entity can initiate event notification " 
                   "that permits the inclusion of diagnostic information."));
}

template <typename T>
void serialize(T &a, vtss_appl_eth_link_oam_port_event_conf_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eth_link_oam_port_event_conf_t"));
    int ix = 0;
    m.add_leaf(s.error_frame_window, vtss::tag::Name("ErrFrameWindow"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Error frame event window indicates the "
                   "duration of the monitoring period in terms of seconds."));
    
    m.add_leaf(s.error_frame_threshold, vtss::tag::Name("ErrFrameThreshold"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Error frame event threshold indicates the "
                   "number of permissible errors frames in the period defined by error frame window. "
                   "If the threshold value is zero, then an Event Notification OAMPDU is "
                   "sent periodically (at the end of every window). "
                   "This can be used as an asynchronous notification to the peer OAM entity of "
                   "the statistics related to this threshold crossing alarm"));
    
    m.add_leaf(s.symbol_period_error_window, vtss::tag::Name("ErrSymPeriodWindow"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Symbol period error event threshold indicates "
                   "the number of permissible symbol errors frames in the period defined "
                   "by symbol period error window."));
    
    m.add_leaf(s.symbol_period_error_threshold, vtss::tag::Name("ErrSymPeriodThreshold"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Symbol period error event threshold indicates "
                   "the number of permissible symbol errors frames in the period "
                   "defined by symbol period error window."
                   "If ErrSymPeriodThreshold symbol errors occur within a "
                   "window of ErrSymPeriodWindow symbols, an Event Notification OAMPDU "
                   "should be generated with an Errored Symbol Period Event TLV "
                   "indicating that the threshold has been crossed in this window. "
                   "If the threshold value is zero, "
                   "then an Event Notification OAMPDU is sent periodically (at the end of every window). "
                   "This can be used as an asynchronous notification to "
                   "the peer OAM entity of the statistics related to this threshold crossing alarm."));
    
    m.add_leaf(s.error_frame_second_summary_window, vtss::tag::Name("ErrFrameSecsSummaryWindow"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Error frame second summary window indicates "
                   "the duration of the monitoring period in terms of seconds."));
    
    m.add_leaf(s.error_frame_second_summary_threshold, vtss::tag::Name("ErrFrameSecsSummaryThreshold"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Error frame second summary threshold indicates "
                   "the number of permissible error frame seconds in the period "
                   "defined by error frame second summary window. "
                   "If ErrFrameSecsSummaryThreshold frame errors occur within a window of "
                   "ErrFrameSecsSummaryWindow , an Event Notification OAMPDU should be generated "
                   "with an Errored Frame Seconds Summary Event TLV "
                   "indicating that the threshold has been crossed in this window."));
}

template <typename T>
void serialize(T &a, vtss_appl_eth_link_oam_remote_loopback_test_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eth_link_oam_remote_loopback_test_t"));
    int ix = 0;
    m.add_leaf(vtss::AsBool(s.loopback_test), vtss::tag::Name("LoopbackTest"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "During loopback testing local device will send test PDUs to peer device, "
                   "Peer device looped back test PDUs to local device "
                   "without altering any field of the frames."));
}

template <typename T>
void serialize(T &a, vtss_appl_eth_link_oam_port_status_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eth_link_oam_port_status_t"));
    int ix = 0;

    m.add_leaf(s.pdu_control, vtss::tag::Name("PduControl"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the local PDU control as specified in table 57.7(IEEE Std 802.3ah). "
                   "Four type of PDU controls are supported. "
                   "RX_INFO: No OAM PDU transmission allowed, only receive OAM PDUs "
                   "LF_INFO: Information OAM PDUs with Link Fault bit of Flags field and "
                   "without Information TLVs can be transmitted "
                   "INFO: Only Information OAM PDU's can be transmitted "
                   "ANY: Transmission of Information OAM PDUs with appropriate bits of Flags field set."));
    
    m.add_leaf(s.discovery_state, vtss::tag::Name("DiscoveryState"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Link OAM discovery state indicates the current state of the discovery process, "
                   "as defined with 57.5(IEEE Std 802.3ah) state machine."));
    
    m.add_leaf(s.multiplexer_state, vtss::tag::Name("MultiplexerState"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the OAM MUX states as specified in table 57.7 (IEEE Std 802.3ah)"
                   "When in forwarding state, the Device is forwarding non-OAMPDUs to the lower sublayer. "
                   "Incase of discarding, the device discards all the non-OAMPDUs."));
    
    m.add_leaf(s.parser_state, vtss::tag::Name("ParserState"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the OAM parser states as specified in table 57.7(IEEE Std 802.3ah). "
                   "In forwarding state, device is forwarding non-OAMPDUs to higher sublayer. "
                   "When in loopback, device is looping back non-OAMPDUs to the lower sublayer. "
                   "In discarding state, device is discarding non-OAMPDUs"));
    m.add_leaf(s.revision, vtss::tag::Name("PduRevision"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the current revision of the Information TLV. "
                   "The value of this field shall start at zero and be incremented "
                   "each time something in the Information TLV changes. "
                   "Upon reception of an Information TLV from a peer, an OAM client may use this field to "
                   "decide if it needs to be processed."));
    
    m.add_leaf(vtss::AsDisplayString(&s.oui[0], VTSS_APPL_ETH_LINK_OAM_OUI_LEN), vtss::tag::Name("Oui"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Vendor Organizationally Unique Identifier."));
 
    m.add_leaf(s.mtu_size, vtss::tag::Name("Mtu"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the largest OAMPDU, in octets, supported by the device. "
                   "This value is compared to the remotes maximum PDU size "
                   "and the smaller of the two is used."));
    
    m.add_leaf(vtss::AsBool(s.uni_dir_support), vtss::tag::Name("UniDirSupport"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether local device supports uni directional capability."));
    
}

template <typename T>
void serialize(T &a, vtss_appl_eth_link_oam_port_peer_status_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eth_link_oam_port_peer_status_t"));
    int ix = 0;
    m.add_leaf(s.peer_oam_mode, vtss::tag::Name("Mode"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the remote OAM mode."));
    
    m.add_leaf(s.peer_mac, vtss::tag::Name("MacAddress"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the remote MAC address."));

    m.add_leaf(s.peer_multiplexer_state, vtss::tag::Name("MultiplexerState"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the remote OAM multiplexer states. "
                   "When in forwarding state, the Device is forwarding non-OAMPDUs to the lower sublayer. "
                   "Incase of discarding, the device discards all the non-OAMPDUs."));
    
    m.add_leaf(s.peer_parser_state, vtss::tag::Name("ParserState"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the remote OAM parser states. "
                   "In forwarding state, forwarding non-OAMPDUs to higher sublayer. "
                   "When in loopback, looping back non-OAMPDUs to the lower sublayer. "
                   "In discarding state, discarding non-OAMPDUs"));
    
    m.add_leaf(s.peer_pdu_revision, vtss::tag::Name("PduRevision"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the remote PDU revision. "
                   "The value of this field shall start at zero and be incremented "
                   "each time something in the Information TLV changes. "
                   "Upon reception of an Information TLV from a peer, an OAM client may use this field to "
                   "decide if it needs to be processed."));
    
    m.add_leaf(vtss::AsDisplayString(&s.peer_oui[0], VTSS_APPL_ETH_LINK_OAM_OUI_LEN), vtss::tag::Name("Oui"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the remote vendor organizationally unique identifier."));
    
    m.add_leaf(s.peer_mtu_size, vtss::tag::Name("Mtu"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the largest OAMPDU, in octets, supported by the remote device."));
    
    m.add_leaf(vtss::AsBool(s.peer_uni_dir_support), vtss::tag::Name("UniDirSupport"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether remote device supports uni directional capability."));
    
    m.add_leaf(vtss::AsBool(s.peer_mib_retrieval_support), vtss::tag::Name("MibRetrievalSupport"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether remote device supports MIB retrieval functionality."));
    
    m.add_leaf(vtss::AsBool(s.peer_loopback_support), vtss::tag::Name("RemoteLoopbackSupport"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether remote device supports remote loopback functionality."));
    
    m.add_leaf(vtss::AsBool(s.peer_link_monitoring_support), vtss::tag::Name("LinkMonitorSupport"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates whether remote device supports link monitor functionality. "));
}

template <typename T>
void serialize(T &a, vtss_appl_eth_link_oam_statistics_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eth_link_oam_statistics_t"));
    int ix = 0;
    m.add_leaf(s.unsupported_codes_tx, vtss::tag::Name("UnsupportedCodesTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the the number of OAMPDUs transmitted on this "
                   "interface with an unsupported op-code."));
    
    m.add_leaf(s.unsupported_codes_rx, vtss::tag::Name("UnsupportedCodesRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the the number of OAMPDUs received on this "
                   "interface with an unsupported op-code."));
    
    m.add_leaf(s.information_tx, vtss::tag::Name("InformationTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of Information OAMPDUs transmitted on "
                   "this interface."));
    
    m.add_leaf(s.information_rx, vtss::tag::Name("InformationRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of Information OAMPDUs received on "
                   "this interface."));
    
    m.add_leaf(s.unique_event_notification_tx, vtss::tag::Name("UniqueEventNotificationTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of unique Event OAMPDUs transmitted on "
                   "this interface."));
    
    m.add_leaf(s.unique_event_notification_rx, vtss::tag::Name("UniqueEventNotificationRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of unique Event OAMPDUs received on "
                   "this interface."));
    
    m.add_leaf(s.duplicate_event_notification_tx, vtss::tag::Name("DuplicateEventNotificationTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of duplicate Event OAMPDUs transmitted on "
                   "this interface."));
    
    m.add_leaf(s.duplicate_event_notification_rx, vtss::tag::Name("DuplicateEventNotificationRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of duplicate Event OAMPDUs received on "
                   "this interface."));
    
    m.add_leaf(s.loopback_control_tx, vtss::tag::Name("LoopbackControlTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of Loopback Control OAMPDUs transmitted on "
                   "this interface."));
    
    m.add_leaf(s.loopback_control_rx, vtss::tag::Name("LoopbackControlRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of Loopback Control OAMPDUs received on "
                   "this interface."));
    
    m.add_leaf(s.variable_request_tx, vtss::tag::Name("VariableRequestTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of Variable Request OAMPDUs transmitted "
                   "on this interface."));
    
    m.add_leaf(s.variable_request_rx, vtss::tag::Name("VariableRequestRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of Variable Request OAMPDUs received "
                   "on this interface."));
    
    m.add_leaf(s.variable_response_tx, vtss::tag::Name("VariableReponseTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of Variable Response OAMPDUs transmitted "
                   "on this interface."));
    
    m.add_leaf(s.variable_response_rx, vtss::tag::Name("VariableReponseRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of Variable Response OAMPDUs received "
                   "on this interface."));
    
    m.add_leaf(s.org_specific_tx, vtss::tag::Name("OrgSpecificTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of Organization Specific OAMPDUs transmitted "
                   "on this interface."));
    
    m.add_leaf(s.org_specific_rx, vtss::tag::Name("OrgSpecificRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the number of Organization Specific OAMPDUs received "
                   "on this interface."));
}

template <typename T>
void serialize(T &a, vtss_appl_eth_link_oam_crit_link_event_statistics_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eth_link_oam_crit_link_event_statistics_t"));
    int ix = 0;
    m.add_leaf(s.link_fault_tx, vtss::tag::Name("LinkFaultTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the the number of Link Fault OAMPDUs transmitted "
                   "on this interface."));
    
    m.add_leaf(s.link_fault_rx, vtss::tag::Name("LinkFaultRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the the number of Link Fault OAMPDUs received "
                   "on this interface."));
    
    m.add_leaf(s.critical_event_tx, vtss::tag::Name("CriticalEventTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the the number of Critical Event OAMPDUs transmitted "
                   "on this interface."));
    
    m.add_leaf(s.critical_event_rx, vtss::tag::Name("CriticalEventRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the the number of Critical Event OAMPDUs received "
                   "on this interface."));
    
    m.add_leaf(s.dying_gasp_tx, vtss::tag::Name("DyingGaspTx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the the number of Dying Gasp OAMPDUs transmitted "
                   "on this interface."));
    
    m.add_leaf(s.dying_gasp_rx, vtss::tag::Name("DyingGaspRx"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the the number of Dying Gasp OAMPDUs received "
                   "on this interface."));
}

template <typename T>
void serialize(T &a, vtss_appl_eth_link_oam_port_link_event_status_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_eth_link_oam_port_link_event_status_t"));
    int ix = 0;
    m.add_leaf(s.sequence_number, vtss::tag::Name("SequenceNumber"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Indicates the the total number of events occurred. "));
    
    m.add_leaf(s.symbol_period_error_event_timestamp, vtss::tag::Name("SymPeriodErrTimestamp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Symbol period error event timestamp indicates the "
                   "time reference when the event was generated, in terms of 100 ms intervals. "));
    
    m.add_leaf(s.symbol_period_error_event_window, vtss::tag::Name("SymPeriodErrEventWindow"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Symbol period error event window indicates "
                   "the number of symbols in the period."));
    
    m.add_leaf(s.symbol_period_error_event_threshold, vtss::tag::Name("SymPeriodErrEventThreshold"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Symbol period error event threshold indicates "
                   "the number of symbol errors in the SymbolPeriodErrEventWindow."));
    
    m.add_leaf(s.symbol_period_error, vtss::tag::Name("SymPeriodErr"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Symbol period error indicates "
                   "the number of symbol errors in the period."));
    
    m.add_leaf(s.total_symbol_period_error, vtss::tag::Name("TotalSymPeriodErr"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Total symbol period errors indicates "
                   "the sum of symbol errors since the OAM sublayer was reset."));
    
    m.add_leaf(s.total_symbol_period_error_events, vtss::tag::Name("TotalSymPeriodErrEvents"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Total symbol period error events indicates "
                   "the number of Errored Symbol Period Event TLVs that have been generated "
                   "since the OAM sublayer was reset."));
    
    m.add_leaf(s.frame_error_event_timestamp, vtss::tag::Name("FrameErrEventTimestamp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Frame error event timestamp indicates the time reference when "
                   "the event was generated, in terms of 100 ms intervals."));
    
    m.add_leaf(s.frame_error_event_window, vtss::tag::Name("FrameErrEventWindow"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Frame error event window indicates "
                   "the duration of the period in terms of 100 ms intervals."));
        
    m.add_leaf(s.frame_error_event_threshold, vtss::tag::Name("FrameErrEventThreshold"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Frame error event threshold indicates the number of detected errored frames "
                   "in the period is required to be equal to or "
                   "greater than in order for the event to be generated."));

    m.add_leaf(s.frame_error, vtss::tag::Name("FrameErr"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Frame errors indicates "
                   "the number of detected errored frames in the period."));

    m.add_leaf(s.total_frame_errors, vtss::tag::Name("TotalFrameError"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Total frame errors indicates the sum of errored frames that "
                   "have been detected since the OAM sublayer was reset."));

    m.add_leaf(s.total_frame_errors_events, vtss::tag::Name("TotalFrameErrorEvents"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Total frame error events indicates the number of Errored Frame Event TLVs that "
                   "have been generated since the OAM sublayer was reset."));

    m.add_leaf(s.frame_period_error_event_timestamp, vtss::tag::Name("FramePeriodErrTimestamp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Frame period error event timestamp indicates the time reference when "
                   "the event was generated, in terms of 100 ms intervals."));

    m.add_leaf(s.frame_period_error_event_window, vtss::tag::Name("FramePeriodErrWindow"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Frame period error event window indicates "
                   "the duration of period in terms of frames."));

    m.add_leaf(s.frame_period_error_event_threshold, vtss::tag::Name("FramePeriodErrThreshold"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Frame period error event threshold indicates "
                   "the number of errored frames in the period is required to be equal to "
                   "or greater than in order for the event to be generated."));
            
    m.add_leaf(s.frame_period_errors, vtss::tag::Name("FramePeriodErr"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Frame period errors indicates the number of frame errors in the period."));

    m.add_leaf(s.total_frame_period_errors, vtss::tag::Name("TotalFramePeriodErr"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Total frame period errors indicates the sum of frame errors that "
                   "have been detected since the OAM sublayer was reset."));

    m.add_leaf(s.total_frame_period_error_event, vtss::tag::Name("TotalFramePeriodErrEvent"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Total frame period error events indicates "
                   "the number of Errored Frame Period Event TLVs "
                   "that have been generated since the OAM sublayer was reset."));

    m.add_leaf(s.error_frame_seconds_summary_event_timestamp, vtss::tag::Name("ErrFrameSecSummaryTimestamp"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Error frame seconds summary event timestamp indicates "
                   "the time reference when the event was generated, in terms of 100 ms intervals."));
    
    m.add_leaf(s.error_frame_seconds_summary_event_window, vtss::tag::Name("ErrFrameSecSummaryWindow"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Error frame seconds summary event window indicates "
                   "the duration of the period in terms of 100 ms intervals."));

    m.add_leaf(s.error_frame_seconds_summary_event_threshold, vtss::tag::Name("ErrFrameSecSummaryThreshold"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Error frame seconds summary event threshold indicates "
                   "the number of errored frame seconds in the period is required to be equal to "
                   "or greater than in order for the event to be generated."));

    m.add_leaf(s.error_frame_seconds_summary_errors, vtss::tag::Name("ErrFrameSecSummaryErrors"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Error frame seconds summary errors indicates "
                   "the number of errored frame seconds in the period."));

    m.add_leaf(s.total_error_frame_seconds_summary_errors, vtss::tag::Name("TotalErrFrameSecSummaryErrors"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Total error frame seconds summary errors indicates "
                   "the sum of errored frame seconds "
                   "that have been detected since the OAM sublayer was reset."));

    m.add_leaf(s.total_error_frame_seconds_summary_events, vtss::tag::Name("TotalErrFrameSecSummaryEvents"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(
                   "Total error frame seconds summary events indicates "
                   "the number of Errored Frame Seconds Summary Event TLVs that "
                   "have been generated since the OAM sublayer was reset."));
}

template <typename T>
void serialize(T &a, vtss_appl_eth_link_oam_port_capabilities_t &s) {
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_eth_link_oam_port_capabilities_t"));
    int ix = 0;
    m.add_leaf(vtss::AsBool(s.eth_link_oam_capable), vtss::tag::Name("Capability"),
            vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
            vtss::tag::Description(
                "Indicates whether interface is Ethernet Link OAM capable or not."));
}


namespace vtss {
namespace appl {
namespace eth_link_oam {
namespace interfaces {
struct ethLinkOamInterfaceConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eth_link_oam_port_conf_t *>> P;

    static constexpr const char *table_description =
            "This is a table to configure Ethernet Link OAM configurations "
            "for a specific interface.";

    static constexpr const char *index_description =
            "Each interface has a set of Ethernet Link OAM configurable parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, LOAM_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eth_link_oam_port_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eth_link_oam_port_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_SET_PTR(vtss_appl_eth_link_oam_port_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_ETH_LINK_OAM);
};

struct ethLinkOamInterfaceEventConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eth_link_oam_port_event_conf_t *>> P;

    static constexpr const char *table_description =
            "This is a table to configure Ethernet Link OAM Event configurations "
            "for a specific interface.";

    static constexpr const char *index_description =
            "Each interface has a set of Ethernet Link OAM Event configurable parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, LOAM_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eth_link_oam_port_event_conf_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eth_link_oam_port_event_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_SET_PTR(vtss_appl_eth_link_oam_port_event_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_ETH_LINK_OAM);
};

struct ethLinkOamInterfaceLoopbackTestConfigEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eth_link_oam_remote_loopback_test_t *>> P;

    static constexpr const char *table_description =
            "This is a table to test remote loopback feature for a specific interface. "
            "OAM remote loopback can be used for fault localization and link performance testing. "
            "During loopback testing local device will send test PDUs to peer device, "
            "Peer device looped back test PDUs to local device without altering any field of the frames.";

    static constexpr const char *index_description =
            "Each interface has a remote loopback test parameter";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, LOAM_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eth_link_oam_remote_loopback_test_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eth_link_oam_remote_loopback_test_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_SET_PTR(vtss_appl_eth_link_oam_remote_loopback_test_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_ETH_LINK_OAM);
};

struct ethLinkOamInterfaceStatisticsEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eth_link_oam_statistics_t *>> P;

    static constexpr const char *table_description =
            "This table represents the Ethernet Link OAM  interface PDU counters";

    static constexpr const char *index_description =
            "Each interface has a set of Ethernet Link OAM PDU statistics counters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, LOAM_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eth_link_oam_statistics_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eth_link_oam_port_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ETH_LINK_OAM);
};

struct ethLinkOamInterfaceCriticalLinkEventStatisticsEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eth_link_oam_crit_link_event_statistics_t *>> P;

    static constexpr const char *table_description =
            "This table represents the Ethernet Link OAM  interface Critical Link Event counters";

    static constexpr const char *index_description =
            "Each interface has a set of Critical Link Event statistics counters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, LOAM_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eth_link_oam_crit_link_event_statistics_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eth_link_oam_port_critical_link_event_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ETH_LINK_OAM);
};


struct ethLinkOamInterfaceStatusEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eth_link_oam_port_status_t *>> P;

    static constexpr const char *table_description =
            "This is a table to Ethernet Link OAM interface status";

    static constexpr const char *index_description =
            "Each interface has a set of status parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, LOAM_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eth_link_oam_port_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eth_link_oam_port_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ETH_LINK_OAM);
};

struct ethLinkOamInterfacePeerStatusEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eth_link_oam_port_peer_status_t *>> P;

    static constexpr const char *table_description =
            "This is a table to Ethernet Link OAM Peer interface status";

    static constexpr const char *index_description =
            "Each interface has a set of Peer interface status parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, LOAM_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eth_link_oam_port_peer_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eth_link_oam_port_peer_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ETH_LINK_OAM);
};

struct ethLinkOamInterfaceLinkEventStatusEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eth_link_oam_port_link_event_status_t *>> P;

    static constexpr const char *table_description =
            "This is a table to Ethernet Link OAM interface link event status";

    static constexpr const char *index_description =
            "Each interface has a set of interface status link event parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, LOAM_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eth_link_oam_port_link_event_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eth_link_oam_port_link_event_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ETH_LINK_OAM);
};

struct ethLinkOamInterfacePeerLinkEventStatusEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eth_link_oam_port_link_event_status_t *>> P;

    static constexpr const char *table_description =
            "This is a table to Ethernet Link OAM Peer interface link event status";

    static constexpr const char *index_description =
            "Each interface has a set of Peer interface status link event parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, LOAM_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eth_link_oam_port_link_event_status_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eth_link_oam_port_peer_link_event_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ETH_LINK_OAM);
};

struct ethLinkOamInterfaceCapabilitiesEntry {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamKey<vtss_ifindex_t>,
            vtss::expose::ParamVal<vtss_appl_eth_link_oam_port_capabilities_t *>> P;

    static constexpr const char *table_description =
            "This is a table to interface capabilities";

    static constexpr const char *index_description =
            "Each interface has a set of capability parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, LOAM_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_eth_link_oam_port_capabilities_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_eth_link_oam_port_capabilities_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ETH_LINK_OAM);
};

struct ethLinkOamInterfaceStatsClearEntry {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<BOOL *>> P;

    static constexpr const char *table_description =
        "This is a table to clear Link OAM statistics for a specific interface.";

    static constexpr const char *index_description =
        "Each interface has a set of statistics counters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, LOAM_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i) {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, vtss_appl_eth_link_oam_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_GET_PTR(eth_link_oam_stats_dummy_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_SET_PTR(eth_link_oam_stats_clr_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_ETH_LINK_OAM);
};

}  // namespace interfaces
}  // namespace eth_link_oam
}  // namespace appl
}  // namespace vtss
#endif
