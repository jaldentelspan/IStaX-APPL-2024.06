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
#ifndef __VTSS_MIRROR_SERIALIZER_HXX__
#define __VTSS_MIRROR_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/rmirror.h"
#include "vtss/appl/mirror.h"
#include "vtss/appl/types.hxx"

/*****************************************************************************
    Enum serializer
*****************************************************************************/
extern vtss_enum_descriptor_t vtss_appl_mirror_session_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(
    vtss_appl_mirror_session_type_t,
    "mirrorSessionType",
    vtss_appl_mirror_session_type_txt,
    "This enumeration defines the session type in Mirror function.");

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(mirror_session_id_index, u16, a, s ) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::expose::snmp::RangeSpec<u32>(1, 65535),
        vtss::tag::Name("SessionId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Session ID."
        " Valid range is (1..maximum). The maximum is platform-specific"
        " and can be retrieved from the"
        " Mirror capabilities.")
    );
}


/****************************************************************************
 * Capabilities
 ****************************************************************************/
struct MirrorHasReflectorPortSupport {
    static constexpr const char *json_ref = "vtss_appl_mirror_capabilities_t";
    static constexpr const char *name = "ReflectorPortSupport";
    static constexpr const char *desc = "If true, user needs to sepcify reflector port.";
    static bool get() {
        vtss_appl_mirror_capabilities_t cap;
        if (vtss_appl_mirror_capabilities_get(&cap) == VTSS_RC_OK) {
            return (!cap.internal_reflector_port_support);
        } else {
            return false;
        }
    }
};

struct MirrorCapHasRMirrorSupport {
    static constexpr const char *json_ref = "vtss_appl_mirror_capabilities_t";
    static constexpr const char *name = "RMirrorSupport";
    static constexpr const char *desc = "If true, the platform supports RMirror VLAN.";
    static bool get() {
        vtss_appl_mirror_capabilities_t cap;
        if (vtss_appl_mirror_capabilities_get(&cap) == VTSS_RC_OK) {
            return cap.rmirror_support;
        } else {
            return false;
        }
    }
};


/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_mirror_capabilities_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mirror_capabilities_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsInt(s.session_cnt_max),
        vtss::tag::Name("SessionCountMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum number of sessions.")
    );

    m.add_leaf(
        vtss::AsInt(s.session_source_cnt_max),
        vtss::tag::Name("SessionSourceCountMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The maximum number of Mirror and RMirror source sessions.")
    );

    m.add_leaf(
        vtss::AsBool(s.rmirror_support),
        vtss::tag::Name("RMirrorSuport"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicate if RMirror feature is supported or not. "
            "true means supported. false means not supported.")
    );

    m.add_leaf(
        vtss::AsBool(s.internal_reflector_port_support),
        vtss::tag::Name("InternalReflectorPortSupport"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicate if Internal reflector port is supported or not. "
            "true means supported. false means not supported.")
    );

    m.add_leaf(
        vtss::AsBool(s.cpu_mirror_support),
        vtss::tag::Name("CpuMirrorSupport"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicate if mirroring CPU traffic is supported or not. "
            "true means supported. false means not supported.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_mirror_session_entry_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mirror_config_session_entry_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.enable),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Controls whether this session is enabled or disabled. "
            "true is to enable the function. false is to disable it.\n"
            "Multiple criteria must be fulfilled in order to be able to enable a session."
            "The criteria depend on the \'SessionType\'."
            )
    );

    m.add_leaf(
        s.type,
        vtss::tag::Name("Type"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Session type in Mirror. "
            "\'mirror\' means to do the Mirror function on the device.\n"
            "\'rMirrorSource\' means the device acts as source node for monitor flow.\n"
            "\'rMirrorDestination\' means the device acts as end node for monitor flow.\n")
    );

    m.add_leaf(
        s.rmirror_vid,
        vtss::tag::Name("RMirrorVlan"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<MirrorCapHasRMirrorSupport>(),
        vtss::tag::Description("The VLAN ID points out where the monitor packet will copy to. "
            "The remote Mirror VLAN ID. Only used for RMirror types.\n"
            "RMirror source session:\n"
            "  The mirrored traffic is copied onto this VLAN ID.\n"
            "  Traffic will flood to all ports that are members of the remote Mirror VLAN ID.\n"
            "RMirror destination session:\n"
            "  The #destination_port_list contains the port(s) that the Mirror VLAN will be copied to\n"
            "  in addition to ports that are already configured (through the VLAN module) to be members "
            "  of this VLAN.")
    );

    m.add_leaf(
        vtss::AsInterfaceIndex(s.reflector_port),
        vtss::tag::Name("ReflectorPort"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<MirrorHasReflectorPortSupport>(),
        vtss::tag::Description("A reflector port is a port that the administrator may have to specify " 
            "in case the device does not have internal (unused) ports available. Whether this is the "
            "case or not for this device can be derived from Mirror capabilities. "
            "When \'ReflectorPort\' is used, it must be specified when an RMirror "
            "source session is enabled. In this case, the reflector port will be shut down for normal "
            "front port usage, because the switch needs a port where it can loop frames in order to get "
            "mirrored traffic copied onto a particular VLAN ID (the \'RMirrorVlan\').")
    );

    m.add_leaf(
        vtss::AsBool(s.strip_inner_tag),
        vtss::tag::Name("StripInnerTag"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "This configuration is used to strip the original VLAN ID of the mirrored traffic or not. When "
            "it is set to TRUE, the the original VLAN ID of the mirrored traffic will be stripped, otherwise "
            "the original VLAN ID will be carried to destination interface. It may have to specify in case  "
            "the device does not have internal (unused) ports available. Whether this is the case or not for "
            "this device can be derived from Mirror capabilities."
            )
    );

    m.add_rpc_leaf(vtss::AsVlanList(s.source_vids.data, 512),
                   vtss::tag::Name("SourceVlans"),
                   vtss::tag::Description(
            "Source VLAN list.\n"
            "All traffic in the VLANs specified in this list will get mirrored onto either "
            "the destination port (Mirror session) or the destination VLAN (RMirror source session). "
            "It's a bit-mask that indicates the VLANs. A '1' indicates the VLAN ID is selected, "
            "a '0' indicates that the VLAN ID isn't selected. "
            )
    );

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.source_vids.data +   0, 128),
                    vtss::tag::Name("SourceVlans0KTo1K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(ix++),
                    vtss::tag::Description(
            "First quarter of bit-array indicating source VLAN list. "
            "All traffic in the VLANs specified in this list will get mirrored onto either "
            "the destination port (Mirror session) or the destination VLAN (RMirror source session). "
            "It's a bit-mask that indicates the VLANs. A '1' indicates the VLAN ID is selected, "
            "a '0' indicates that the VLAN ID isn't selected. "
            )
    );

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.source_vids.data +   128, 128),
                    vtss::tag::Name("SourceVlans1KTo2K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(ix++),
                    vtss::tag::Description(
            "Second quarter of bit-array indicating source VLAN list. "
            "All traffic in the VLANs specified in this list will get mirrored onto either "
            "the destination port (Mirror session) or the destination VLAN (RMirror source session). "
            "It's a bit-mask that indicates the VLANs. A '1' indicates the VLAN ID is selected, "
            "a '0' indicates that the VLAN ID isn't selected. "
            )
    );

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.source_vids.data +   256, 128),
                    vtss::tag::Name("SourceVlans2KTo3K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(ix++),
                    vtss::tag::Description(
            "Third quarter of bit-array indicating source VLAN list. "
            "All traffic in the VLANs specified in this list will get mirrored onto either "
            "the destination port (Mirror session) or the destination VLAN (RMirror source session). "
            "It's a bit-mask that indicates the VLANs. A '1' indicates the VLAN ID is selected, "
            "a '0' indicates that the VLAN ID isn't selected. "
            )
    );

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.source_vids.data +   384, 128),
                    vtss::tag::Name("SourceVlans3KTo4K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(ix++),
                    vtss::tag::Description(
            "Fourth quarter of bit-array indicating source VLAN list. "
            "All traffic in the VLANs specified in this list will get mirrored onto either "
            "the destination port (Mirror session) or the destination VLAN (RMirror source session). "
            "It's a bit-mask that indicates the VLANs. A '1' indicates the VLAN ID is selected, "
            "a '0' indicates that the VLAN ID isn't selected. "
            )
    );

    m.add_leaf(
        (vtss::PortListStackable &)s.source_port_list_rx,
        vtss::tag::Name("SourcePortListRx"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "A bit-mask that controls whether a given port is enabled for mirroring of incoming traffic. "
            "A '1' indicates that the port is included, whereas a '0' indicates it isn't. "
            "Only source sessions (Mirror and RMirror Source) use this value. "
            )
    );

    m.add_leaf(
        (vtss::PortListStackable &)s.source_port_list_tx,
        vtss::tag::Name("SourcePortListTx"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "A bit-mask that controls whether a given port is enabled for mirroring of outgoing traffic. "
            "A '1' indicates that the port is included, whereas a '0' indicates it isn't. "
            "Only source sessions (Mirror and RMirror Source) use this value. "
            )
    );

    m.add_leaf(
        vtss::AsBool(s.cpu_rx),
        vtss::tag::Name("CpuRx"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Controls whether mirroring of traffic received by the internal CPU is enabled or disabled. "
            " It is supported or not can be derived from Mirror capabilities."
            "Only source sessions (Mirror and RMirror Source) use this value. "
            )
    );

    m.add_leaf(
        vtss::AsBool(s.cpu_tx),
        vtss::tag::Name("CpuTx"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Controls whether mirroring of traffic transmitted by the internal CPU is enabled or disabled. "
            " It is supported or not can be derived from Mirror capabilities."
            "Only source sessions (Mirror and RMirror Source) use this value. "
            )
    );

    m.add_leaf(
        (vtss::PortListStackable &)s.destination_port_list,
        vtss::tag::Name("DestinationPortList"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Destination port list implemented as a bit-mask, where a '1' indicates "
            "that the port is included and a '0' indicates that it isn't. "
            "Only used in plain Mirror sessions and RMirror destination sessions.\n"
            "Mirror session:\n"
            "  At most one bit may be set in this mask.\n"
            "RMirror destination session:\n"
            "  Zero or more bits may be set in this mask."
            )
    );
}

namespace vtss {
namespace appl {
namespace mirror {
namespace interfaces {
struct MirrorCapabilitiesLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_mirror_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mirror_capabilities_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mirror_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_RMIRROR);
};

struct MirrorConfigSessionEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u16>,
        vtss::expose::ParamVal<vtss_appl_mirror_session_entry_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of configuration per session";

    static constexpr const char *index_description =
        "Each session has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u16 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        h.argument_properties(tag::Name("sessionId"));
        serialize(h, mirror_session_id_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mirror_session_entry_t &i) {
        h.argument_properties(tag::Name("sessionConfig"));
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mirror_session_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_mirror_session_entry_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mirror_session_entry_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RMIRROR);
};

}  // namespace interfaces
}  // namespace aggr
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_MIRROR_SERIALIZER_HXX__ */
