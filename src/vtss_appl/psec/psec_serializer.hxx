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

#ifndef __VTSS_PSEC_SERIALIZER_HXX__
#define __VTSS_PSEC_SERIALIZER_HXX__

#include <vtss/appl/psec.h>
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/interface.h"
#include "psec_expose.hxx"

using namespace vtss;

//******************************************************************************
// Enum serializer
//******************************************************************************
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_psec_violation_mode_t,
                         "PsecViolationMode",
                         psec_expose_violation_mode_txt,
                         "The violation mode determines what should happen when a limit is exceeded.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_psec_mac_type_t,
                         "PsecMacType",
                         psec_expose_mac_type_txt,
                         "The MAC type determines how the entry is learned.");

//******************************************************************************
// Index serializers
//******************************************************************************
VTSS_SNMP_TAG_SERIALIZE(PSEC_SERIALIZER_ifindex_t, vtss_ifindex_t, a, s) {
    a.add_leaf(AsInterfaceIndex(s.inner),
               tag::Name("IfIndex"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("Logical interface index."));
}

VTSS_SNMP_TAG_SERIALIZE(PSEC_SERIALIZER_ifindex_port_vid_t, vtss_ifindex_t, a, s) {
    a.add_leaf(AsInterfaceIndex(s.inner),
               tag::Name("IfIndex"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("Logical interface index."));
}

VTSS_SNMP_TAG_SERIALIZE(PSEC_SERIALIZER_vid_t, mesa_vid_t, a, s) {
    a.add_leaf(AsVlan(s.inner),
               tag::Name("VlanId"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("VLAN ID used for indexing."));
}

VTSS_SNMP_TAG_SERIALIZE(PSEC_SERIALIZER_mac_t, mesa_mac_t, a, s) {
    a.add_leaf(s.inner,
               tag::Name("MacAddress"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("The MAC address for which this entry applies."));
}

//******************************************************************************
// Struct serializers
//******************************************************************************

template<typename T>
void serialize(T &a, vtss_appl_psec_capabilities_t &cap)
{
    typename T::Map_t m = a.as_map(tag::Typename("vtss_appl_psec_capabilities_t"));

    m.add_leaf(AsPsecUserBitmaskType(cap.users),
               tag::Name("users"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("A bitmask indicating the internal modules (a.k.a. users) included in this flavor of the application."));

    m.add_leaf(cap.pool_size,
               tag::Name("poolSize"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(2),
               tag::Description("Total number of Port Security-controlled <VLAN, MAC> entries."));

    m.add_leaf(cap.limit_min,
               tag::Name("limitMin"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(3),
               tag::Description("The minimum limit on number of <VLAN, MAC> entries a given interface can be configured to."));

    m.add_leaf(cap.limit_max,
               tag::Name("limitMax"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(4),
               tag::Description("The maximum limit on number of <VLAN, MAC> entries a given interface can be configured to."));

    m.add_leaf(cap.violate_limit_min,
               tag::Name("violateLimitMin"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(5),
               tag::Description("The minimum number of violating <VLAN, MAC> entries a given interface can be configured to."));

    m.add_leaf(cap.violate_limit_max,
               tag::Name("violateLimitMax"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(6),
               tag::Description("The maximum number of violating <VLAN, MAC> entries a given interface can be configured to."));

    m.add_leaf(cap.age_time_min,
               tag::Name("ageTimeMin"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(7),
               tag::Description("The minimum allowed age time value."));

    m.add_leaf(cap.age_time_max,
               tag::Name("ageTimeMax"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(8),
               tag::Description("The maximum allowed age time value."));

    m.add_leaf(cap.hold_time_min,
               tag::Name("holdTimeMin"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(9),
               tag::Description("The minimum allowed hold time value."));

    m.add_leaf(cap.hold_time_max,
               tag::Name("holdTimeMax"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(10),
               tag::Description("The maximum allowed hold time value."));
}

template<typename T>
void serialize(T &a, vtss_appl_psec_global_conf_t &s)
{
    typename T::Map_t m = a.as_map(tag::Typename("vtss_appl_psec_global_conf_t"));

    m.add_leaf(AsBool(s.enable_aging),
               tag::Name("enableAging"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("When TRUE, learned, forwarding entries are subject to aging - FALSE if not."));

    m.add_leaf(s.aging_period_secs,
               tag::Name("agingPeriodSecs"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(2),
               tag::Description("If aging is enabled (with enableAging), this is the aging period in seconds. "
                                "Valid range is defined by capabilities' ageTimeMin and ageTimeMax members."));

    m.add_leaf(s.hold_time_secs,
               tag::Name("holdTimeSecs"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(3),
               tag::Description("Denies entries are held in a blocking state for this amount of time, measured in seconds. "
                                "Valid range is defined by capabilities' holdTimeMin and holdTimeMax members."));
}

template<typename T>
void serialize(T &a, vtss_appl_psec_interface_conf_t &s)
{
    typename T::Map_t m = a.as_map(tag::Typename("vtss_appl_psec_interface_conf_t"));

    m.add_leaf(AsBool(s.enabled),
               tag::Name("enabled"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("Controls whether port-security is enabled for this interface. "));

    m.add_leaf(s.limit,
               tag::Name("limit"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(2),
               tag::Description("Maximum number of MAC addresses allowed on this interface. "
                                "Valid range is given by capabilities' limitMin and limitMax members."));

    m.add_leaf(s.violate_limit,
               tag::Name("violateLimit"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(3),
               tag::Description("Maximum number of violating MAC addresses allowed on this interface. "
                                "This is only used when violationMode is 'restrict'. "
                                "Valid range is given by capabilities' violateLimitMin and violateLimitMax members."));

    m.add_leaf(s.violation_mode,
               tag::Name("violationMode"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(4),
               tag::Description("Action to take if number of MAC addresses exceeds the limit. "
                                "protect(0) Do nothing, except disallowing further clients. "
                                "restrict(1) Keep recording the violating MAC addresses, which are kept until holdTimeSecs expires. "
                                "shutdown(2) Shut-down the interface."));

    m.add_leaf(AsBool(s.sticky),
               tag::Name("sticky"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(5),
               tag::Description("Set to TRUE to automatically convert dynamic entries to sticky, which will survive link changes."));
}

//******************************************************************************
// Serializer for vtss_appl_psec_mac_conf_t.
//******************************************************************************
template<typename T>
void serialize(T &a, vtss_appl_psec_mac_conf_t &s)
{
    typename T::Map_t m = a.as_map(tag::Typename("vtss_appl_psec_mac_conf_t"));

    m.add_leaf(s.mac_type,
               tag::Name("macType"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("Indicates how to add entry. "
                                "dynamic(0) It is strongly recommended not to use this interface to add dynamic entries. "
                                "static(1) Add statically to MAC table (forwarding database). "
                                "sticky(2) Like dynamic, but kept across link-changes. It is strongly recommended not to use this interface to add sticky entries."));
}

template<typename T>
void serialize(T &a, vtss_appl_psec_global_status_t &global_status)
{
    typename T::Map_t m = a.as_map(tag::Typename("vtss_appl_psec_global_status_t"));

    m.add_leaf(global_status.total_mac_cnt,
               tag::Name("totalMacCnt"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("Total number of MAC addresses in the system managed by Port Security."));

    m.add_leaf(global_status.cur_mac_cnt,
               tag::Name("curMacCnt"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(2),
               tag::Description("Number of currently unused MAC addresses in the system managed by Port Security. "
                                "The number of used MAC addresses can then be found by subtracting curMacCnt from totalMacCnt."));
}

template<typename T>
void serialize(T &a, vtss_appl_psec_global_notification_status_t &global_notif_status)
{
    typename T::Map_t m = a.as_map(tag::Typename("vtss_appl_psec_global_notification_status_t"));

    m.add_leaf(AsBool(global_notif_status.pool_depleted),
               tag::Name("poolDepleted"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("TRUE if Port Security has run out of free <VLAN, MAC>-entries, FALSE if at least one is yet free."));
}

//******************************************************************************
// Serializer for psec_semi_public_interface_status_t.
// psec_semi_public_interface_status_t is an internal structure, which contains
// a public structure, vtss_appl_psec_interface_status_t, which is the one we
// are actually serializing.
//******************************************************************************
template<typename T>
void serialize(T &a, psec_semi_public_interface_status_t &s)
{
    typename T::Map_t m = a.as_map(tag::Typename("vtss_appl_psec_interface_status_t"));

    m.add_leaf(AsPsecUserBitmaskType(s.status.users),
               tag::Name("users"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("Bitmask indicating the port-security users that are currently "
                                "enabled on this interface."));

    m.add_leaf(s.status.mac_cnt,
               tag::Name("macCount"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(2),
               tag::Description("Number of MAC addresses currently assigned to this interface. This includes violateCount."));

    m.add_leaf(s.status.cur_violate_cnt,
               tag::Name("violateCount"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(3),
               tag::Description("Current number of violating MAC addresses. Only counts when violationMode is 'restrict'. "
                                "This keeps track of the current number of violating MAC addresses in the MAC table. "
                                "These MAC addresses time out (according to holdTimeSecs)"));

    m.add_leaf(AsBool(s.status.limit_reached),
               tag::Name("limitReached"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(4),
               tag::Description("TRUE if the limit is reached on the interface, FALSE otherwise."));

    m.add_leaf(AsBool(s.status.sec_learning),
               tag::Name("secLearning"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(5),
               tag::Description("TRUE if secure learning is enabled on the interface, FALSE otherwise. Mainly for debugging."));

    m.add_leaf(AsBool(s.status.cpu_copying),
               tag::Name("cpuCopying"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(6),
               tag::Description("TRUE if CPU copying is enabled on the interface, FALSE otherwise. Mainly for debugging."));

    m.add_leaf(AsBool(s.status.link_is_up),
               tag::Name("linkIsUp"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(7),
               tag::Description("TRUE if interface link is up, FALSE otherwise. Mainly for debugging."));

    m.add_leaf(AsBool(s.status.stp_discarding),
               tag::Name("stpDiscarding"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(8),
               tag::Description("TRUE if at least one STP MSTI instance is discarding on the interface, FALSE otherwise. Mainly for debugging."));

    m.add_leaf(AsBool(s.status.hw_add_failed),
               tag::Name("hwAddFailed"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(9),
               tag::Description("TRUE if H/W add of a MAC address failed on this interface, FALSE otherwise. Mainly for debugging."));

    m.add_leaf(AsBool(s.status.sw_add_failed),
               tag::Name("swAddFailed"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(10),
               tag::Description("TRUE if S/W add of a MAC address failed on this interface, FALSE otherwise. Mainly for debugging."));

    m.add_leaf(AsBool(s.status.sticky),
               tag::Name("sticky"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(11),
               tag::Description("TRUE if interface is configured as sticky, that is, learned MAC addresses will survive link changes."));
}

//******************************************************************************
// Serializer for vtss_appl_psec_interface_notification_status_t.
//******************************************************************************
template<typename T>
void serialize(T &a, vtss_appl_psec_interface_notification_status_t &s)
{
    typename T::Map_t m = a.as_map(tag::Typename("vtss_appl_psec_interface_notification_status_t"));

    m.add_leaf(s.total_violate_cnt,
               tag::Name("totalViolateCount"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("Total number of violating MAC addresses. Only counts when violationMode is 'restrict'. "
                                "This keeps on counting up, whereas violateCount counts the actual number of violating MAC addresses"));

    m.add_leaf(AsBool(s.shut_down),
               tag::Name("shutDown"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(2),
               tag::Description("TRUE if the interface is shut down as a result of a violation when violationMode is 'shutdown', FALSE otherwise. "
                                "Do a 'shutdown/no shutdown' or a port-security configuration change on the interface to re-open it."));

    m.add_leaf(AsVlanOrZero(s.latest_violating_vlan),
               tag::Name("latestViolatingVlan"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(3),
               tag::Description("Holds the VLAN ID of the latest violating host. Used when violationMode is 'restrict' or 'shutdown'. "
                                "This field and latestViolatingMac are only valid if this field is non-zero"));

    m.add_leaf(s.latest_violating_mac,
               tag::Name("latestViolatingMac"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(4),
               tag::Description("Holds the MAC address of the latest violating host. Used when violationMode is 'restrict' or 'shutdown'."));
}

//******************************************************************************
// Serializer for vtss_appl_psec_mac_status_t.
//******************************************************************************
template<typename T>
void serialize(T &a, vtss_appl_psec_mac_status_t &s)
{
    typename T::Map_t m = a.as_map(tag::Typename("vtss_appl_psec_mac_status_t"));

    m.add_leaf(s.ifindex,
               tag::Name("port"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("Port on which this MAC address is learned."));

    m.add_leaf(AsDisplayString(s.creation_time, sizeof(s.creation_time) - 1),
               tag::Name("creationTime"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(2),
               tag::Description("MAC address was originally added at this time."));

    m.add_leaf(AsDisplayString(s.changed_time, sizeof(s.changed_time) - 1),
               tag::Name("changedTime"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(3),
               tag::Description("MAC address was last changed at this time (due to e.g. aging)."));

    m.add_leaf(s.age_or_hold_time_secs,
               tag::Name("ageHoldTime"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(4),
               tag::Description("Down-counter indicating time left of aging "
                                "period or hold time measured in seconds. 0 "
                                "means that aging is disabled."));

    m.add_leaf(s.violating,
               tag::Name("violating"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(5),
               tag::Description("TRUE if this MAC address is violating the limit and therefore is blocked. "
                                "This can only be set when violationMode is set to 'restrict'. "
                                "Non-violating MAC addresses may be blocked or kept blocked if other user modules "
                                "are enabled on this interface. MAC addresses may be blocked for other reasons."));
    m.add_leaf(s.blocked,
               tag::Name("blocked"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(6),
               tag::Description("TRUE if this MAC address is blocked from forwarding. "
                                "Such MAC addresses are subject to hold-time timeout."));

    m.add_leaf(s.kept_blocked,
               tag::Name("keptBlocked"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(7),
               tag::Description("TRUE if this MAC address is kept blocked from forwarding. "
                                "Only internal user modules can set a MAC address to be kept "
                                "blocking until further notice. MAC addresses in this state are "
                                "not subject to hold-time expiration."));

    m.add_leaf(s.cpu_copying,
               tag::Name("cpuCopying"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(8),
               tag::Description("TRUE if CPU copying is enabled for this MAC address due to aging. "
                                "Mainly used for debugging."));

    m.add_leaf(s.age_frame_seen,
               tag::Name("ageFrameSeen"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(9),
               tag::Description("TRUE if an age frame was seen during aging of this MAC address. "
                                "Mainly used for debugging."));

    m.add_leaf(AsPsecUserBitmaskType(s.users_forward),
               tag::Name("usersForward"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(10),
               tag::Description("Bitmask indicating the user modules that have marked this MAC address as forwarding."));

    m.add_leaf(AsPsecUserBitmaskType(s.users_block),
               tag::Name("usersBlock"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(11),
               tag::Description("Bitmask indicating the user modules that have marked this MAC address as blocking (wins over forward)."));

    m.add_leaf(AsPsecUserBitmaskType(s.users_keep_blocked),
               tag::Name("usersKeepBlocked"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(12),
               tag::Description("Bitmask indicating the user modules that wish this MAC address to be kept blocked (wins over block)."));

    m.add_leaf(s.mac_type,
               tag::Name("macType"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(13),
               tag::Description("Indicates how entry is learned. "
                                "dynamic(0) Learned when packets are received on the secure port. "
                                "static(1) Configured by the user. "
                                "sticky(2) Learned like dynamic addresses, but persist through switch reboots and link changes."));
}

template<typename T>
void serialize(T &a, vtss_appl_psec_global_control_mac_clear_t &s)
{
    typename T::Map_t m = a.as_map(tag::Typename("vtss_appl_psec_global_control_mac_clear_t"));

    m.add_leaf(AsBool(s.specific_ifindex),
               tag::Name("specificIfindex"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(1),
               tag::Description("When TRUE, this structure's ifindex field is searched for matching MAC addresses to remove. "
                                "When FALSE, all ports are searched for MAC addresses to remove."));

    m.add_leaf(s.ifindex,
               tag::Name("ifindex"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(2),
               tag::Description("When specificIfindex is TRUE, this member holds the interface (must be of type port) "
                                "on which to search for MAC addresses to remove."));

    m.add_leaf(AsBool(s.specific_vlan),
               tag::Name("specificVlan"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(3),
               tag::Description("When TRUE, this structure's vlanId field is searched for MAC addresses to remove. "
                                "When FALSE, all VLANs are searched for MAC addresses to remove."));

    m.add_leaf(AsVlanOrZero(s.vlan),
               tag::Name("vlanId"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(4),
               tag::Description("When specificVlan is TRUE, this member holds the VLAN ID on which to search "
                                "for MAC addresses to remove."));

    m.add_leaf(AsBool(s.specific_mac),
               tag::Name("specificMac"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(5),
               tag::Description("When TRUE, this structure's macAddress field holds the MAC address to remove. "
                                "When FALSE, all MAC addresses are searched for MAC addresses to remove."));

    m.add_leaf(s.mac,
               tag::Name("macAddress"),
               expose::snmp::Status::Current,
               expose::snmp::OidElementValue(6),
               tag::Description("When specificMac is TRUE, this member holds the MAC address to remove."));
}

namespace vtss {
namespace appl {
namespace psec {

struct Capabilities {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_psec_capabilities_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psec_capabilities_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_psec_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct ConfigGlobal {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_psec_global_conf_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psec_global_conf_t &i) {
        h.argument_properties(expose::snmp::OidOffset(0));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_psec_global_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_psec_global_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct ConfigInterface {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>, expose::ParamVal<vtss_appl_psec_interface_conf_t *>> P;
    static constexpr const char *table_description = R"(Table of per-interface configuration.)";
    static constexpr const char *index_description = R"(Each row contains the port-security configuration for one interface.)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(0));
        serialize(h, PSEC_SERIALIZER_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_psec_interface_conf_t &i) {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_psec_interface_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_SET_PTR(vtss_appl_psec_interface_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct ConfigInterfaceMac {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>, expose::ParamKey<mesa_vid_t>, expose::ParamKey<mesa_mac_t>, expose::ParamVal<vtss_appl_psec_mac_conf_t *>> P;

    static constexpr uint32_t   snmpRowEditorOid   = 1000;
    static constexpr const char *table_description = R"(Table of static and sticky MAC addresses to be applied to MAC table (forwarding database))";
    static constexpr const char *index_description = R"(Each row contains the configurating for one MAC address)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(0));
        serialize(h, PSEC_SERIALIZER_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_vid_t &i) {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PSEC_SERIALIZER_vid_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_mac_t &i) {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, PSEC_SERIALIZER_mac_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_psec_mac_conf_t &i) {
        h.argument_properties(expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_DEF_PTR(vtss_appl_psec_interface_conf_mac_default_get);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_psec_interface_conf_mac_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_psec_interface_conf_mac_del);
    VTSS_EXPOSE_GET_PTR(vtss_appl_psec_interface_conf_mac_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_psec_interface_conf_mac_add); // Updates existing
    VTSS_EXPOSE_ITR_PTR(vtss_appl_psec_interface_conf_mac_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct StatusGlobal {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_psec_global_status_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psec_global_status_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_psec_global_status_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct StatusGlobalNotification {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_psec_global_notification_status_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psec_global_notification_status_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_psec_global_notification_status_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct StatusInterface {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>, expose::ParamVal<psec_semi_public_interface_status_t *>> P;
    static constexpr const char *table_description = R"(Table of per-interface status.)";
    static constexpr const char *index_description  = R"(Each row contains the port-security status for one interface.)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(0));
        serialize(h, PSEC_SERIALIZER_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(psec_semi_public_interface_status_t &i) {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(psec_expose_interface_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct StatusInterfaceNotification {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>, expose::ParamVal<vtss_appl_psec_interface_notification_status_t *>> P;
    static constexpr const char *table_description = R"(Table of per-interface notification status.)";
    static constexpr const char *index_description = R"(Each row contains the port-security notification status for one interface.)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(0));
        serialize(h, PSEC_SERIALIZER_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_psec_interface_notification_status_t &i) {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_psec_interface_notification_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

// Predeclare an otherwise hidden function
mesa_rc psec_interface_status_mac_get_all_json(const expose::json::Request *req, ostreamBuf *os);

struct StatusInterfaceMac {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>, expose::ParamKey<mesa_vid_t>, expose::ParamKey<mesa_mac_t>, expose::ParamVal<vtss_appl_psec_mac_status_t *>> P;

    static constexpr const char *table_description = R"(Table of port-security controlled entries in the MAC table (forwarding database))";
    static constexpr const char *index_description = R"(Each row contains the status for one MAC address)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(0));
        serialize(h, PSEC_SERIALIZER_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_vid_t &i) {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PSEC_SERIALIZER_vid_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(mesa_mac_t &i) {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, PSEC_SERIALIZER_mac_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(vtss_appl_psec_mac_status_t &i) {
        h.argument_properties(expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_psec_interface_status_mac_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_psec_interface_status_mac_itr);
    VTSS_JSON_GET_ALL_PTR(        psec_interface_status_mac_get_all_json);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct ControlGlobalMacClear {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_psec_global_control_mac_clear_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_psec_global_control_mac_clear_t &i) {
        h.argument_properties(expose::snmp::OidOffset(0));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(psec_expose_global_control_mac_clear_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_psec_global_control_mac_clear);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

} // namespace vtss
} // namespace appl
} // namespance psec
#endif /* __VTSS_PSEC_SERIALIZER_HXX__ */

