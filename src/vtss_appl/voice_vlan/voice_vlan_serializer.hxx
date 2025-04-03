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
#ifndef __VTSS_VOICE_VLAN_SERIALIZER_HXX__
#define __VTSS_VOICE_VLAN_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/voice_vlan.h"
#include "vtss/appl/types.hxx"

/*****************************************************************************
    Data type serializer
*****************************************************************************/

/*****************************************************************************
    Enumerator serializer
*****************************************************************************/
extern vtss_enum_descriptor_t   voice_vlan_portManagement_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_voice_vlan_management_t,
                         "VoiceVlanPortManagementType",
                         voice_vlan_portManagement_txt,
                         "This enumeration indicates per port Voice VLAN function administrative type.");

extern vtss_enum_descriptor_t   voice_vlan_portDiscovery_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_voice_vlan_discovery_t,
                         "VoiceVlanPortDiscoveryProtocol",
                         voice_vlan_portDiscovery_txt,
                         "This enumeration indicates per port Voice VLAN discovery protocol.");

/*****************************************************************************
    Index serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_voice_vlan_oui_index_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_voice_vlan_oui_index_t"));
    int                 ix = 0;
    u32                 idx_len = VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN;

    m.add_leaf(
        vtss::BinaryLen(s.prefix, VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN, VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN, idx_len),
        vtss::tag::Name("Prefix"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Address prefix of the telephony OUI. A leading 3 bytes index used to "
            "denote whether specific MAC address is presenting a voice device.")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_voice_vlan_capabilities_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_voice_vlan_capabilities_t"));
    int                 ix = 0;

    m.add_leaf(
        vtss::AsBool(s.support_lldp_discovery_notification),
        vtss::tag::Name("SupportLldpDiscovery"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The capability to support voice device discovery from LLDP notification.")
    );

    m.add_leaf(
        s.min_oui_learning_aging_time,
        vtss::tag::Name("MinAgeTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The minimum time value in second for aging telephony OUI sources in voice VLAN.")
    );

    m.add_leaf(
        s.max_oui_learning_aging_time,
        vtss::tag::Name("MaxAgeTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum time value in second for aging telephony OUI sources in voice VLAN.")
    );

    m.add_leaf(
        s.max_forwarding_traffic_class,
        vtss::tag::Name("MaxTrafficClass"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum allowed CoS (Class of Service) value to be used in forwarding voice VLAN traffic.")
    );

    m.add_leaf(
        s.max_oui_registration_entry_count,
        vtss::tag::Name("MaxOuiEntryCount"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The maximum number of telephony OUI entry registration.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_voice_vlan_global_conf_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_voice_vlan_global_conf_t"));
    int                 ix = 0;

    m.add_leaf(
        vtss::AsBool(s.admin_state),
        vtss::tag::Name("AdminState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Administrative control for system wide voice VLAN function, "
            "TRUE is to enable the voice VLAN function and FALSE is to disable it.")
    );

    m.add_leaf(
        vtss::AsVlan(s.voice_vlan_id),
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "VLAN ID, which should be unique in the system, for voice VLAN.")
    );

    m.add_leaf(
        s.aging_time,
        vtss::tag::Name("AgingTime"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "MAC address aging time (T) for telephony OUI source registrated "
            "by voice VLAN. The actual timing in purging the specific entry ranges from T to 2T.")
    );

    m.add_leaf(
        s.traffic_class,
        vtss::tag::Name("TrafficClass"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Traffic class value used in frame CoS queuing insides voice VLAN. "
            "All kinds of traffic on voice VLAN apply this traffic class.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_voice_vlan_port_conf_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_voice_vlan_port_conf_t"));
    int                 ix = 0;

    m.add_leaf(
        s.management,
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Management mode of the specific port in voice VLAN. "
            "'disabled' will disjoin the port from voice VLAN. "
            "'forced' will force the port to join voice VLAN. "
            "'automatic' will join the port in voice VLAN upon detecting attached VoIP "
            "devices by using DiscoveryProtocol parameter.")
    );

    m.add_leaf(
        s.protocol,
        vtss::tag::Name("DiscoveryProtocol"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Specify the protocol for detecting attached VoIP devices. "
            "It only works when 'automatic' is set in Mode parameter, and voice VLAN will "
            "restart automatic detecting process upon changing the protocol. "
            "When 'oui' is given, voice VLAN performs VoIP device detection based on checking "
            "telephony OUI settings via new MAC address notification. "
            "When 'lldp' is given, voice VLAN performs VoIP device detection based on LLDP notifications."
            "When 'both' is given, voice VLAN performs VoIP device detection based on either "
            "new MAC address notification or LLDP notifications.\n"
            "In addition, the first come notification will be first served.")
    );

    m.add_leaf(
        vtss::AsBool(s.secured),
        vtss::tag::Name("Secured"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Manage the security control of this port interface in voice VLAN, "
            "TRUE is to enable the security control and FALSE is to disable it. "
            "When it is disabled, all the traffic in voice VLAN will be permit. "
            "When it is enabled, all non-telephonic MAC addresses in the voice VLAN will be blocked "
            "for 10 seconds and thus the traffic from these senders will be deny.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_voice_vlan_telephony_oui_conf_t &s)
{
    typename T::Map_t   m = a.as_map(vtss::tag::Typename("vtss_appl_voice_vlan_telephony_oui_conf_t"));
    int                 ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.description, sizeof(s.description)),
        vtss::tag::Name("Description"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The description for the specific telephony OUI.")
    );
}

/*****************************************************************************
    Serializer interface
*****************************************************************************/
namespace vtss
{
namespace appl
{
namespace voice_vlan
{
namespace interfaces
{

struct VoiceVlanCapabilitiesLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_voice_vlan_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_voice_vlan_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1), vtss::tag::Name("capability"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_voice_vlan_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_VOICE_VLAN);
};

struct VoiceVlanGlobalsMgmt {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_voice_vlan_global_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_voice_vlan_global_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1), vtss::tag::Name("global_conf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_voice_vlan_global_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_voice_vlan_global_config_set);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_voice_vlan_global_config_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VOICE_VLAN);
};

struct VoiceVlanPortTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t *>,
         vtss::expose::ParamVal<vtss_appl_voice_vlan_port_conf_t *>
         > P;

    static constexpr const char *table_description =
        "This is a table for managing per port voice VLAN functions.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.add_leaf(
            vtss::AsInterfaceIndex(i),
            vtss::tag::Name("IfIndex"),
            vtss::expose::snmp::Status::Current,
            vtss::expose::snmp::OidElementValue(1),
            vtss::tag::Description(
                "Logical interface number of the Voice VLAN port.")
        );
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_voice_vlan_port_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2), vtss::tag::Name("port_conf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_voice_vlan_port_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_voice_vlan_port_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_voice_vlan_port_config_set);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_voice_vlan_port_config_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VOICE_VLAN);
};

struct VoiceVlanOuiTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_appl_voice_vlan_oui_index_t *>,
         vtss::expose::ParamVal<vtss_appl_voice_vlan_telephony_oui_conf_t *>
         > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "This is a table for managing the telephony OUI settings that will be used for voice VLAN functions.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_voice_vlan_oui_index_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1), vtss::tag::Name("oui_index"));
        serialize(h, i);
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_voice_vlan_telephony_oui_conf_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2), vtss::tag::Name("oui_conf"));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_voice_vlan_telephony_oui_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_voice_vlan_telephony_oui_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_voice_vlan_telephony_oui_config_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_voice_vlan_telephony_oui_config_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_voice_vlan_telephony_oui_config_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_voice_vlan_telephony_oui_config_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VOICE_VLAN);
};

}  // namespace interfaces
}  // namespace voice_vlan
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_VOICE_VLAN_SERIALIZER_HXX__ */
