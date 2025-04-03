/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __VTSS_NAS_SERIALIZER_HXX__
#define __VTSS_NAS_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/nas.h"

//****************************************
// Enum  and types definitions
//****************************************
extern vtss_enum_descriptor_t vtss_nas_port_status_txt[];
extern vtss_enum_descriptor_t vtss_nas_vlan_type_txt[];
extern vtss_enum_descriptor_t vtss_nas_port_control_txt[];

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_nas_port_status_t,
                         "nasPortStatus",
                         vtss_nas_port_status_txt,
                         "This enumerates the NAS interface status.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_nas_vlan_type_t,
                         "nasVlanType",
                         vtss_nas_vlan_type_txt,
                         "This enumeration the NAS VLAN type.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_nas_port_control_t,
                         "nasPortControl",
                         vtss_nas_port_control_txt,
                         "This enumeration the NAS admin state.\n");

//****************************************
// Shared descriptions
//****************************************

#define ADMIN_MODES_DESCRIP "TypeNone       : Forces an interface to be disabled.\n"                                           \
                            "forceAuthorized : Forces an interface to grant access to all clients, 802.1X-aware or not.\n"     \
                            "auto           : Requires an 802.1X-aware client to be authorized by the authentication "         \
                                             "server. Clients that are not 802.1X-aware will be denied access.\n"              \
                            "unauthorized   : Forces an interface to deny access to all clients, 802.1X-aware or not.\n"       \
                            "macBased       : The switch authenticates on behalf of the client, using the client "             \
                                             "MAC-address as the username and password and MD5 EAP method.\n"                  \
                            "dot1xSingle    : At most one supplicant is allowed to authenticate, and it authenticates "        \
                                             "using normal 802.1X frames.\n"                                                   \
                            "dot1xmulti     : One or more supplicants are allowed to authenticate individually using "         \
                                             "an 802.1X variant, where EAPOL frames sent from the switch are directed towards "\
                                             "the supplicants MAC address instead of using the multi-cast BPDU MAC address. "  \
                                             "Unauthenticated supplicants will not get access.\n"                              \
                            "\n"                                                                                               \
                            "Note: The 802.1X Admin State must be set to Authorized for interfaces that are enabled for"       \
                            "Spanning Tree"

//******************************************************************************
// /* Global Struct serializer(s) */ {
//******************************************************************************
template<typename T>
void serialize(T &a, vtss_appl_glbl_cfg_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_glbl_cfg_t"));
    int idx = 1;

    m.add_leaf(vtss::AsBool(s.enabled),
               vtss::tag::Name("nasMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Set to TRUE to globally enabled or "
               "disabled NAS for the switch. If globally disabled, all physical interfaces are "
               "allowed forwarding of frames."));

    m.add_leaf(vtss::AsBool(s.reauth_enabled),
               vtss::tag::Name("reauthenticationMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("If set to TRUE, successfully authenticated "
               "supplicants/clients are re-authenticated after the interval specified "
               "by the Reauthentication Period. Re-authentication for 802.1X-enabled "
               "interfaces can be used to detect if a new device is plugged into a switch "
               "port or if a supplicant is no longer attached. For MAC-based ports, "
               "re-authentication is only useful, if the RADIUS server configuration has "
               "changed. It does not involve communication between the switch and the "
               "client, and therefore does not imply that a client is still present on "
               "a port (see Aging Period)."));

    m.add_leaf(s.reauth_period_secs,
               vtss::tag::Name("reauthenticationPeriod"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::expose::snmp::RangeSpec<uint32_t>(VTSS_APPL_NAS_REAUTH_PERIOD_SECS_MIN, VTSS_APPL_NAS_REAUTH_PERIOD_SECS_MAX),
               vtss::tag::Description("Sets the period in seconds, after which a "
                                      "connected client must be re-authenticated. This is only active if the "
                                      "ReauthenticationMode is set to TRUE."));

    m.add_leaf(s.eapol_timeout_secs,
               vtss::tag::Name("eapolTimeout"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::expose::snmp::RangeSpec<uint16_t>(VTSS_APPL_NAS_EAPOL_TIMEOUT_SECS_MIN, VTSS_APPL_NAS_EAPOL_TIMEOUT_SECS_MAX),
               vtss::tag::Description("Determines the time for re-transmission of Request Identity EAPOL frames. "
                                      "This has no effect for MAC-based ports."));

    m.add_leaf(s.psec_aging_period_secs,
               vtss::tag::Name("agingPeriod"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::RangeSpec<uint32_t>(VTSS_APPL_NAS_PSEC_AGING_PERIOD_SECS_MIN, VTSS_APPL_NAS_PSEC_AGING_PERIOD_SECS_MAX),
               vtss::expose::snmp::OidElementValue(idx++),
	       vtss::tag::Description("Specific the PSEC aging period in seconds. In the period the CPU starts listening to "
                                      "frames from the given MAC address, and if none arrives before period end, the entry "
                                      "will be removed."));
    m.add_leaf(s.psec_hold_time_secs,
               vtss::tag::Name("authFailureHoldTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::RangeSpec<uint32_t>(VTSS_APPL_NAS_PSEC_HOLD_TIME_SECS_MIN, VTSS_APPL_NAS_PSEC_HOLD_TIME_SECS_MAX),
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Time in seconds to wait before attempting to "
                                      "re-authenticate if re-authentication failed for a given client."));

    m.add_leaf(vtss::AsBool(s.qos_backend_assignment_enabled),
               vtss::tag::Name("radiusAssignedQosMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Set to true to enable RADIUS assigned QoS."));

    m.add_leaf(vtss::AsBool(s.vlan_backend_assignment_enabled),
               vtss::tag::Name("radiusAssignedVlanMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Set to true to enable RADIUS assigned VLAN."));

    m.add_leaf(vtss::AsBool(s.guest_vlan_enabled),
               vtss::tag::Name("guestVlanMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Set to true to enable Guest VLAN Mode."));

    m.add_leaf(vtss::AsVlan(s.guest_vid),
               vtss::tag::Name("guestVlanId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Guest VLAN ID to get assigned to an interface moved "
                                      "to Guest VLAN mode."));

    m.add_leaf(s.reauth_max,
               vtss::tag::Name("maxReauthrequestsCount"),
               vtss::expose::snmp::RangeSpec<uint32_t>(VTSS_APPL_NAS_REAUTH_MIN, VTSS_APPL_NAS_REAUTH_MAX),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Maximum re-authentication request count."));

    m.add_leaf(vtss::AsBool(s.guest_vlan_allow_eapols),
               vtss::tag::Name("guestVlanAllowEapols"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Set to TRUE to allow an interface to move to Guest VLAN even "
                                      "when EAPOL packets has been received at an interface."));
}



template<typename T>
void serialize(T &a, vtss_appl_nas_eapol_counters_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_nas_eapol_counters_t"));
    int idx = 5;

    m.add_leaf(s.dot1xAuthEapolFramesRx,
               vtss::tag::Name("dot1xAuthEapolFramesRx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of dot1x Auth Eapol Frames Received."));

    m.add_leaf(s.dot1xAuthEapolFramesTx,
               vtss::tag::Name("dot1xAuthEapolFramesTx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of dot1x Auth Eapol Frames Transmitted."));

   m.add_leaf(s.dot1xAuthEapolStartFramesRx,
               vtss::tag::Name("dot1xAuthEapolStartFramesRx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of dot1x Auth Eapol Start Frames Received."));

   m.add_leaf(s.dot1xAuthEapolLogoffFramesRx,
               vtss::tag::Name("dot1xAuthEapolLogoffFramesRx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of dot1x Auth Eapol Logoff Frames Received."));

   m.add_leaf(s.dot1xAuthEapolRespIdFramesRx,
               vtss::tag::Name("dot1xAuthEapolRespIdFramesRx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of dot1x Auth Eapol RespId Frames Received."));

   m.add_leaf(s.dot1xAuthEapolRespFramesRx,
               vtss::tag::Name("dot1xAuthEapolRespFramesRx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of dot1x Auth Eapol Resp Frames Received."));

   m.add_leaf(s.dot1xAuthEapolReqIdFramesTx,
               vtss::tag::Name("dot1xAuthEapolReqIdFramesTx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of dot1x Auth Eapol Req Id Frames Transmitted."));

   m.add_leaf(s.dot1xAuthEapolReqFramesTx,
               vtss::tag::Name("dot1xAuthEapolReqFramesTx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of dot1x Auth Eapol Req Frames Transmitted."));

   m.add_leaf(s.dot1xAuthInvalidEapolFramesRx,
               vtss::tag::Name("dot1xAuthInvalidEapolFramesRx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of dot1x Auth Invalid Eapol Frames Received."));

   m.add_leaf(s.dot1xAuthEapLengthErrorFramesRx,
               vtss::tag::Name("dot1xAuthEapLengthErrorFramesRx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of dot1x Auth Eap Length Error Frames Received."));
}

template<typename T>
void serialize(T &a, vtss_appl_nas_backend_counters_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_nas_backend_counters_t"));
    int idx = 5;

   m.add_leaf(s.backendResponses,
               vtss::tag::Name("backendResponses"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of backend Responses."));

   m.add_leaf(s.backendAccessChallenges,
               vtss::tag::Name("backendAccessChallenges"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of backend Access Challenges."));

   m.add_leaf(s.backendOtherRequestsToSupplicant,
               vtss::tag::Name("backendOtherRequestsToSupplicant"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of backend Other Requests To Supplicant."));

   m.add_leaf(s.backendAuthSuccesses,
               vtss::tag::Name("backendAuthSuccesses"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of backend Auth Successes."));

   m.add_leaf(s.backendAuthFails,
               vtss::tag::Name("backendAuthFails"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Numbers of backend Auth Fails."));
}

template<typename T>
void serialize(T &a, vtss_appl_nas_interface_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_nas_interface_status_t"));
    int idx = 2;

    m.add_leaf(s.status,
               vtss::tag::Name("status"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("dot1x port status."));

    m.add_leaf(s.qos_class,
               vtss::tag::Name("qosClass"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("QoS class for this port. If value is 4294967295"
                " it means that the QoS is not overridden."));

    m.add_leaf(s.vlan_type,
               vtss::tag::Name("vlanType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("VLAN Type for this port."));

    m.add_leaf(vtss::AsVlan(s.vid),
               vtss::tag::Name("vlanId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("VLAN Id for this port. 0 if not overridden."));

    m.add_leaf(s.auth_cnt,
               vtss::tag::Name("authCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("In multi-client modes, number of authenticated clients on this port."));

    m.add_leaf(s.unauth_cnt,
               vtss::tag::Name("unauthCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("In multi-client modes, number of unauthenticated clients on this port."));
}

template<typename T>
void serialize(T &a, vtss_appl_nas_client_info_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_nas_client_info_t"));
    int idx = 2;

    m.add_leaf(vtss::AsVlan(s.vid_mac.vid),
               vtss::tag::Name("VlanId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("VLAN ID."));

    m.add_leaf(s.vid_mac.mac,
               vtss::tag::Name("Mac"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Binary version of MacAddrStr."));

    m.add_leaf(vtss::AsDisplayString(s.identity, sizeof(s.identity)),
               vtss::tag::Name("Identity"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Identity string."));
}

template<typename T>
void serialize(T &a, vtss_appl_nas_port_cfg_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_nas_port_cfg_t"));
    int idx = 2;
    m.add_leaf(s.admin_state,
               vtss::tag::Name("adminState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Administrative State.\n"
                                       ADMIN_MODES_DESCRIP));

    m.add_leaf(vtss::AsBool(s.qos_backend_assignment_enabled),
               vtss::tag::Name("RadiusAssignedQosState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Set to TRUE to enable RADIUS-assigned QoS for this interface."));


    m.add_leaf(vtss::AsBool(s.vlan_backend_assignment_enabled),
               vtss::tag::Name("RadiusAssignedVlanState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Set to TRUE to enable RADIUS-assigned VLAN for this interface."));

    m.add_leaf(vtss::AsBool(s.guest_vlan_enabled),
               vtss::tag::Name("GuestVlanState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Set to TRUE to enable Guest-VLAN for this interface."));

}

//******************************************************************************
// /* Global Struct serializers */ }
//******************************************************************************
struct NAS_ifindex_index {
    NAS_ifindex_index(vtss_ifindex_t &x) : inner(x) { }
    vtss_ifindex_t &inner;
};

template<typename T>
void serialize(T &a, NAS_ifindex_index s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("NAS_ifindex_index"));

    m.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("InterfaceNo"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Logical interface number."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_nas_reauth_now_bool_t, BOOL, a, s) {
    a.add_leaf(vtss::AsBool(s.inner),
               vtss::tag::Name("Now"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("TRUE to force re-authentication immediately. FALSE to refresh (restart) 802.1X authentication process."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_nas_clr_statistics_bool_t, BOOL, a, s) {
    a.add_leaf(vtss::AsBool(s.inner),
               vtss::tag::Name("Clear"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("TRUE to clear NAS statistics."));
}


/****************************************************************************
 * Get and get declarations  -- See topo_expose.cxx
 ****************************************************************************/
mesa_rc vtss_nas_ser_reauth_dummy_get(vtss_ifindex_t ifindex, BOOL *const now);

mesa_rc vtss_nas_ser_clr_statistics_set(vtss_ifindex_t ifindex, const BOOL *clear);

mesa_rc vtss_nas_ser_clr_statistics_dummy_get(vtss_ifindex_t ifindex, BOOL *const clear);

mesa_rc vtss_nas_ser_port_status_get(vtss_ifindex_t ifindex, vtss_appl_nas_interface_status_t *const status);

mesa_rc vtss_nas_ser_last_supplicant_info_get(vtss_ifindex_t ifindex, vtss_appl_nas_client_info_t *last_supplicant_info);
/****************************************************************************
 * Serializes
 ****************************************************************************/


namespace vtss {
namespace appl {
namespace nas {
namespace interfaces {

struct NasLastSupplicantInfo {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_nas_client_info_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to get NAS last supplicant";

    static constexpr const char *index_description =
        "Each physical interface has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, NAS_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_nas_client_info_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_nas_ser_last_supplicant_info_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};


struct NasReauthTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<BOOL *>
    > P;

    static constexpr const char *table_description =
        "This is a table to start NAS re-authorization";

    static constexpr const char *index_description =
        "Each physical interface has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, NAS_ifindex_index(i));
   }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i) {
        serialize(h, vtss_nas_reauth_now_bool_t(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_nas_ser_reauth_dummy_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_SET_PTR(vtss_appl_nas_port_reinitialize);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct NasPortStatsDot1xEapolTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_nas_eapol_counters_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to get NAS EAPOL statistics.";

    static constexpr const char *index_description =
        "Each physical interface has a set of EAPOL counters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, NAS_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_nas_eapol_counters_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_GET_PTR(vtss_appl_nas_eapol_statistics_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct NasPortStatsRadiusServerTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_nas_backend_counters_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to get NAS EAPOL statistics.";

    static constexpr const char *index_description =
        "Each physical interface has a set of EAPOL counters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, NAS_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_nas_backend_counters_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_GET_PTR(vtss_appl_nas_radius_statistics_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct NasStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_nas_interface_status_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to get NAS per-port status table.";

    static constexpr const char *index_description =
        "Each physical interface has a set of per-port status parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, NAS_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_nas_interface_status_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_GET_PTR(vtss_nas_ser_port_status_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct NasGlblConfigGlobals {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_glbl_cfg_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_glbl_cfg_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_nas_glbl_cfg_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_nas_glbl_cfg_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct NasStatClrLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<BOOL *>
    > P;

    static constexpr const char *table_description =
        "This is a table to clear NAS statistics for a specific interface.";

    static constexpr const char *index_description =
        "Each interface has a set of statistics counters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, NAS_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_nas_clr_statistics_bool_t(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_nas_ser_clr_statistics_dummy_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_SET_PTR(vtss_nas_ser_clr_statistics_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct NasPortConfig {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_nas_port_cfg_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table for port configuration";

    static constexpr const char *index_description =
        "Each physical interface has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, NAS_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_nas_port_cfg_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_nas_port_cfg_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_nas_port_cfg_set);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

}  // namespace interfaces
}  // namespace nas
}  // namespace appl
}  // namespace vtss

#endif
