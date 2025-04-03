/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __VTSS_VLAN_SERIALIZER_HXX__
#define __VTSS_VLAN_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include <vtss/appl/vlan.h>
#include <vtss/appl/types.hxx> /* For PortListStackable */
#include "vlan_trace.h"

// Hacks go here....
struct VLAN_MIB_config_globals_main_t {
    mesa_etype_t custom_s_port_ether_type;
    u8           access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
};
struct VLAN_MIB_name_t {
    char name[VTSS_APPL_VLAN_NAME_MAX_LEN];
};

mesa_rc VLAN_MIB_name_get(mesa_vid_t vid, VLAN_MIB_name_t *const name);
mesa_rc VLAN_MIB_name_set(mesa_vid_t vid, const VLAN_MIB_name_t *const name);
mesa_rc VLAN_MIB_vid_all_iter(const mesa_vid_t *prev, mesa_vid_t *next);
mesa_rc VLAN_MIB_config_globals_main_get(VLAN_MIB_config_globals_main_t *const g);
mesa_rc VLAN_MIB_config_globals_main_set(const VLAN_MIB_config_globals_main_t *g);
mesa_rc VLAN_MIB_interface_conf_get(vtss_ifindex_t ifindex, vtss_appl_vlan_port_conf_t *const conf);
mesa_rc VLAN_MIB_ifindex_user_iter(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex, const vtss_appl_vlan_user_t *prev_user, vtss_appl_vlan_user_t *next_user);
mesa_rc VLAN_MIB_vid_user_iter(const mesa_vid_t *prev_vid,  mesa_vid_t *next_vid, const vtss_appl_vlan_user_t *prev_user, vtss_appl_vlan_user_t *next_user);
mesa_rc VLAN_MIB_interface_detailed_conf_get(vtss_ifindex_t ifindex, vtss_appl_vlan_user_t user, vtss_appl_vlan_port_detailed_conf_t *const conf);
mesa_rc VLAN_MIB_membership_vid_user_get(mesa_vid_t vid, vtss_appl_vlan_user_t user, vtss::PortListStackable *s);
mesa_rc VLAN_MIB_svl_conf_get(mesa_vid_t vid, mesa_vid_t *fid);
mesa_rc VLAN_MIB_svl_conf_itr(const mesa_vid_t *prev, mesa_vid_t *next);
mesa_rc VLAN_MIB_svl_conf_del(mesa_vid_t vid);
mesa_rc VLAN_MIB_svl_conf_default(mesa_vid_t *vid, mesa_vid_t *fid);


struct VlanCapHasFlooding {
    static constexpr const char *json_ref = "vtss_appl_vlan_capabilities_t";
    static constexpr const char *name = "HasFlooding";
    static constexpr const char *desc = "If true, the platform supports flooding tables";
    static bool get() { return fast_cap(VTSS_APPL_CAP_VLAN_FLOODING); }
};

//******************************************************************************
// Tag serializers
//******************************************************************************
VTSS_SNMP_TAG_SERIALIZE(VLAN_MIB_vid_dscr_t, mesa_vid_t, a, s) {
    a.add_leaf(vtss::AsVlan(s.inner),
               vtss::tag::Name("vlanId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0), // Is offset when used
               vtss::tag::Description("VLAN ID. Valid range is " vtss_xstr(VTSS_APPL_VLAN_ID_MIN) " - " vtss_xstr(VTSS_APPL_VLAN_ID_MAX) "."));
}

VTSS_SNMP_TAG_SERIALIZE(VLAN_MIB_fid_dscr_t, mesa_vid_t, a, s) {
    a.add_leaf(s.inner,
               vtss::tag::Name("filterId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0), // Is offset when used
               vtss::tag::Description("Filter ID (FID) used in Shared VLAN Learning. Zero or more VLANs may map into the same FID."));
}

VTSS_SNMP_TAG_SERIALIZE(VLAN_MIB_ifindex_dscr_t, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner),
               vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface index."));
}

VTSS_SNMP_TAG_SERIALIZE(VLAN_MIB_vlan_user_dscr_t, vtss_appl_vlan_user_t, a, s) {
    a.add_leaf(s.inner,
               vtss::tag::Name("vlanUser"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("VLAN user."));
}

VTSS_SNMP_TAG_SERIALIZE(VLAN_MIB_PortListStackable_dscr_t, vtss::PortListStackable, a, s) {
    a.add_leaf(s.inner,
               vtss::tag::Name("PortList"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0), // Is offset when used
               vtss::tag::Description("Port list."));
}

VTSS_SNMP_TAG_SERIALIZE(VLAN_MIB_flooding_dscr_t, mesa_bool_t, a, s) {
    a.add_leaf(vtss::AsBool(s.inner),
               vtss::tag::Name("flooding"),
               vtss::expose::snmp::Status::Current,
               vtss::tag::DependOnCapability<VlanCapHasFlooding>(),
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Flooding."));
}

//******************************************************************************
// Enum serializers
//******************************************************************************
extern vtss_enum_descriptor_t vlan_user_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_vlan_user_t,
                         "VlanUserType",
                         vlan_user_txt,
                         "An integer that indicates the VLAN user type. "
                         "A value of 'combined' indicates the VLAN settings as programmed to hardware. "
                         "A value of 'admin' indicates the VLAN settings as programmed by the administrative user, "
                         "and any other value indicates a software module that changes VLAN settings 'behind the scenes'.");

extern vtss_enum_descriptor_t vlan_tx_tag_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_vlan_tx_tag_type_t,
                         "VlanEgressTagging",
                         vlan_tx_tag_type_txt,
                         "An integer that indicates how egress tagging occurs.");

extern vtss_enum_descriptor_t vlan_port_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_vlan_port_type_t,
                         "VlanPortType",
                         vlan_port_type_txt,
                         "An integer that indicates if a port is VLAN aware, and if so, to which EtherType it is sensitive.");

extern vtss_enum_descriptor_t vlan_frame_txt[];
VTSS_XXXX_SERIALIZE_ENUM(mesa_vlan_frame_t,
                         "VlanIngressAcceptance",
                         vlan_frame_txt,
                         "An integer that indicates the type of frames that are not discarded on ingress w.r.t. VLAN tagging.");

extern vtss_enum_descriptor_t vlan_port_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_vlan_port_mode_t,
                         "VlanPortMode",
                         vlan_port_mode_txt,
                         "Determines the underlying port mode.\n"
                         "Access ports are only members of one VLAN, the AccessVlan.\n"
                         "Trunk ports are by default members of all VLANs, which can be limited with TrunkVlans.\n"
                         "Hybrid ports allow for changing all port VLAN parameters. As trunk ports, hybrid ports are by default members of all VLANs, which can be limited with HybridVlans.");

//******************************************************************************
// Struct serializers
//******************************************************************************
template<typename T>
void serialize(T &a, vtss_appl_vlan_capabilities_t &cap)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_vlan_capabilities_t"));

    m.add_leaf(vtss::AsVlan(cap.vlan_id_min),
               vtss::tag::Name("vlanIdMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("The minimum VLAN ID that can be configured on the device."));

    m.add_leaf(vtss::AsVlan(cap.vlan_id_max),
               vtss::tag::Name("vlanIdMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("The maximum VLAN ID that can be configured on the device."));

    m.add_leaf(cap.fid_cnt,
               vtss::tag::Name("fidCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("The number of Shared VLAN Learning (SVL) Filter IDs (FIDs) supported by this device. 0 if SVL is not supported."));

    m.add_leaf(vtss::AsBool(cap.has_flooding),
               vtss::tag::Name("hasFlooding"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("If true, flooding can be managed."));
}

template<typename T>
void serialize(T &a, VLAN_MIB_config_globals_main_t &g)
{
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_vlan_config_globals_main_t"));

    int idx = 0;

    m.add_leaf(vtss::AsEtherType(g.custom_s_port_ether_type),
               vtss::tag::Name("CustomSPortEtherType"),
               vtss::expose::snmp::RangeSpec<u32>(1536, 65535),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("TPID (EtherType) for ports marked as Custom-S tag aware."));

    m.add_rpc_leaf(vtss::AsVlanList(g.access_vids, 512),
                   vtss::tag::Name("AccessVlans"),
                   vtss::tag::Description("Enabled access VLANs."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(g.access_vids +   0, 128),
                    vtss::tag::Name("AccessVlans0To1K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("First quarter of bit-array indicating the enabled access VLANs."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(g.access_vids + 128, 128),
                    vtss::tag::Name("AccessVlans1KTo2K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Second quarter of bit-array indicating the enabled access VLANs."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(g.access_vids + 256, 128),
                    vtss::tag::Name("AccessVlans2KTo3K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Third quarter of bit-array indicating the enabled access VLANs."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(g.access_vids + 384, 128),
                    vtss::tag::Name("AccessVlans3KTo4K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Last quarter of bit-array indicating the enabled access VLANs."));
}

template<typename T>
void serialize(T &a, VLAN_MIB_name_t &s) {
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_vlan_name_t"));

    m.add_leaf(vtss::AsDisplayString(s.name, VTSS_APPL_VLAN_NAME_MAX_LEN),
               vtss::tag::Name("Name"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0), // We use 0-based indexing and let the user of this serializer offset it to its needs.
               vtss::tag::Description("VLAN Name. Default for VLAN " vtss_xstr(VTSS_APPL_VLAN_ID_DEFAULT) " is '" VTSS_APPL_VLAN_NAME_DEFAULT "'. "
                                "Default for any other VLAN is 'VLANxxxx', where 'xxxx' is a decimal representation of the VLAN ID with leading zeroes."));
}

template<typename T>
void serialize(T &a, vtss_appl_vlan_port_conf_t &s)
{
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_vlan_port_conf_t"));

    int idx = 0; // Start at 0, because we offset it when instantiating it.

    m.add_leaf(s.mode,
               vtss::tag::Name("Mode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("VLAN mode of the port."));

    m.add_leaf(vtss::AsVlan(s.access_pvid),
               vtss::tag::Name("AccessVlan"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("The port VLAN ID the port will be assigned when Mode is Access."));

    m.add_leaf(vtss::AsVlan(s.trunk_pvid),
               vtss::tag::Name("TrunkNativeVlan"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("The port VLAN ID the port will be assigned when Mode is trunk."));

    m.add_leaf(vtss::AsBool(s.trunk_tag_pvid),
               vtss::tag::Name("TrunkTagNativeVlan"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Controls whether frames classified to TrunkNativeVlan get tagged on egress. Used when Mode is trunk."));

    m.add_rpc_leaf(vtss::AsVlanList(s.trunk_allowed_vids, 512),
                   vtss::tag::Name("TrunkVlans"),
                   vtss::tag::Description("List of VLANs the port is member of. Used when Mode is trunk."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.trunk_allowed_vids +   0, 128),
                    vtss::tag::Name("TrunkVlans0KTo1K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("First quarter of bit-array indicating whether the port is member of a VLAN ('1') or not ('0'). Used when Mode is trunk."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.trunk_allowed_vids + 128, 128),
                    vtss::tag::Name("TrunkVlans1KTo2K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Second quarter of bit-array indicating whether the port is member of a VLAN ('1') or not ('0'). Used when Mode is trunk."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.trunk_allowed_vids + 256, 128),
                    vtss::tag::Name("TrunkVlans2KTo3K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Third quarter of bit-array indicating whether the port is member of a VLAN ('1') or not ('0'). Used when Mode is trunk."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.trunk_allowed_vids + 384, 128),
                    vtss::tag::Name("TrunkVlans3KTo4K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Last quarter of bit-array indicating whether the port is member of a VLAN ('1') or not ('0'). Used when Mode is trunk."));

    m.add_leaf(vtss::AsVlan(s.hybrid.pvid),
               vtss::tag::Name("HybridNativeVlan"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("The port VLAN ID the port will be assigned when Mode is hybrid."));

    m.add_leaf(s.hybrid.port_type,
               vtss::tag::Name("HybridPortType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Controls awareness and whether it reacts to C-tags, S-tags, Custom-S-tags. Used when Mode is hybrid."));

    m.add_leaf(vtss::AsBool(s.hybrid.ingress_filter),
               vtss::tag::Name("HybridIngressFiltering"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Controls whether frames classified to a certain VLAN ID get discarded (true) or not (false) if the port is not member of the VLAN ID. Used when Mode is hybrid."));

    m.add_leaf(s.hybrid.frame_type,
               vtss::tag::Name("HybridIngressAcceptance"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Controls whether frames are accepted on ingress depending on VLAN tag in frame. Used when Mode is hybrid."));

    m.add_leaf(s.hybrid.tx_tag_type,
               vtss::tag::Name("HybridEgressTagging"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Controls tagging of frames on egress. tagThis(1) is not allowed. Used when Mode is hybrid."));

    m.add_rpc_leaf(vtss::AsVlanList(s.hybrid_allowed_vids, 512),
                   vtss::tag::Name("HybridVlans"),
                   vtss::tag::Description("List of VLANs the port is member of. Used when Mode is hybrid."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.hybrid_allowed_vids +   0, 128),
                    vtss::tag::Name("HybridVlans0KTo1K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("First quarter of bit-array indicating whether the port is member of a VLAN ('1') or not ('0'). Used when Mode is hybrid."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.hybrid_allowed_vids + 128, 128),
                    vtss::tag::Name("HybridVlans1KTo2K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Second quarter of bit-array indicating whether the port is member of a VLAN ('1') or not ('0'). Used when Mode is hybrid."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.hybrid_allowed_vids + 256, 128),
                    vtss::tag::Name("HybridVlans2KTo3K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Third quarter of bit-array indicating whether the port is member of a VLAN ('1') or not ('0'). Used when Mode is hybrid."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.hybrid_allowed_vids + 384, 128),
                    vtss::tag::Name("HybridVlans3KTo4K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Last quarter of bit-array indicating whether the port is member of a VLAN ('1') or not ('0'). Used when Mode is hybrid."));

    m.add_rpc_leaf(vtss::AsVlanList(s.forbidden_vids, 512),
                   vtss::tag::Name("ForbiddenVlans"),
                   vtss::tag::Description("List of VLANs the port cannot become member of. Used in all modes."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.forbidden_vids +   0, 128),
                    vtss::tag::Name("ForbiddenVlans0KTo1K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("First quarter of bit-array indicating whether the port can ever become a member of a VLAN ('0') or not ('1'). Used in all modes."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.forbidden_vids + 128, 128),
                    vtss::tag::Name("ForbiddenVlans1KTo2K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Second quarter of bit-array indicating whether the port can ever become a member of a VLAN ('0') or not ('1'). Used in all modes."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.forbidden_vids + 256, 128),
                    vtss::tag::Name("ForbiddenVlans2KTo3K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Third quarter of bit-array indicating whether the port can ever become a member of a VLAN ('0') or not ('1'). Used in all modes."));

    m.add_snmp_leaf(vtss::AsVlanListQuarter(s.forbidden_vids + 384, 128),
                    vtss::tag::Name("ForbiddenVlans3KTo4K"),
                    vtss::expose::snmp::Status::Current,
                    vtss::expose::snmp::OidElementValue(idx++),
                    vtss::tag::Description("Last quarter of bit-array indicating whether the port can ever become a member of a VLAN ('0') or not ('1'). Used in all modes."));
}

template<typename T>
void serialize(T &a, vtss_appl_vlan_port_detailed_conf_t &s)
{
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_vlan_port_detailed_conf_t"));

    int idx = 0;

    // The vtss::expose::snmp::PreGetCondition() is needed in order
    // to evaluate whether this element needs to go out.
    // If one did an "if (s.flags && VTSS_APPL_VLAN_PORT_FLAGS_xxx) {"
    // around the m.add_leaf() then the MIB-generator would not necessarily
    // output all elements of the structure.
    // PreGetCondition() takes an argument, which is called a lambda-function (C++11),
    // and passes #s by reference ([&]). It's basically an anonymous, inline function.
#define FIELD_OVERRIDDEN(X) ((s.flags & VTSS_APPL_VLAN_PORT_FLAGS_ ## X) != 0)

    m.add_leaf(vtss::AsVlan(s.pvid),
               vtss::tag::Name("Pvid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Port VLAN ID set by this user."),
               vtss::expose::snmp::PreGetCondition([&]() {
                   T_I("Overridden: %d", FIELD_OVERRIDDEN(PVID));
                   return FIELD_OVERRIDDEN(PVID);
               }));

    m.add_leaf(vtss::AsVlan(s.untagged_vid),
               vtss::tag::Name("Uvid"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Untagged VLAN ID set by a user. This may only be populated by non-admin users."),
               vtss::expose::snmp::PreGetCondition([&]() {
                   // Only output if user has chosen a specific UVID to either tag or untag (and not just PVID).
                   return FIELD_OVERRIDDEN(TX_TAG_TYPE) && (!FIELD_OVERRIDDEN(PVID) || s.untagged_vid != s.pvid) &&
                          (s.tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS || s.tx_tag_type == VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS);
               }));

    m.add_leaf(s.port_type,
               vtss::tag::Name("PortType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("VLAN Awareness and tag reaction set by this user."),
               vtss::expose::snmp::PreGetCondition([&]() {
                   return FIELD_OVERRIDDEN(AWARE);
               }));


    m.add_leaf(vtss::AsBool(s.ingress_filter),
               vtss::tag::Name("IngressFiltering"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Ingress filtering enabled or disabled by this user."),
               vtss::expose::snmp::PreGetCondition([&]() {
                   return FIELD_OVERRIDDEN(INGR_FILT);
               }));

    m.add_leaf(s.frame_type,
               vtss::tag::Name("IngressAcceptance"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("VLAN tagging accepted upon ingress configured by this user."),
               vtss::expose::snmp::PreGetCondition([&]() {
                   return FIELD_OVERRIDDEN(RX_TAG_TYPE);
               }));

    m.add_leaf(s.tx_tag_type,
               vtss::tag::Name("EgressTagging"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(idx++),
               vtss::tag::Description("Egress tagging configured by this user."),
               vtss::expose::snmp::PreGetCondition([&]() {
                   return FIELD_OVERRIDDEN(TX_TAG_TYPE);
               }));

#undef FIELD_OVERRIDDEN
}

namespace vtss {
namespace appl {
namespace vlan {
namespace interfaces {

struct Capabilities {
    typedef expose::ParamList<expose::ParamVal<vtss_appl_vlan_capabilities_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_vlan_capabilities_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_vlan_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_VLAN);
};

struct ConfigGlobal {
    typedef expose::ParamList<expose::ParamVal<VLAN_MIB_config_globals_main_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(VLAN_MIB_config_globals_main_t &i) {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(VLAN_MIB_config_globals_main_get);
    VTSS_EXPOSE_SET_PTR(VLAN_MIB_config_globals_main_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN);
};

struct NameTable {
    typedef expose::ParamList<expose::ParamKey<mesa_vid_t>,
                              expose::ParamVal<VLAN_MIB_name_t *>> P;

    static constexpr const char *table_description =
            R"(Table of VLAN names.)";

    static constexpr const char *index_description =
            R"(Each row contains the name of a given VLAN.)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_vid_t &i) {
        h.argument_properties(tag::Name("vid"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, VLAN_MIB_vid_dscr_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(VLAN_MIB_name_t &i) {
        h.argument_properties(tag::Name("name"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(VLAN_MIB_name_get);
    VTSS_EXPOSE_ITR_PTR(VLAN_MIB_vid_all_iter);
    VTSS_EXPOSE_SET_PTR(VLAN_MIB_name_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN);
};

struct FloodingTable {
    typedef expose::ParamList<expose::ParamKey<mesa_vid_t>,
                              expose::ParamVal<mesa_bool_t >> P;

    typedef VlanCapHasFlooding depends_on_t;

    static constexpr const char *table_description =
            "(Table of VLAN flooding configuration.)"
            "This is an optional table and is only present if vlanCapabilities.hasFlooding is true.";

    static constexpr const char *index_description =
            R"(Each row contains the flooding configuration of a given VLAN.)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_vid_t &i) {
        h.argument_properties(tag::Name("vid"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, VLAN_MIB_vid_dscr_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_bool_t &i) {
        h.argument_properties(tag::Name("flooding"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, VLAN_MIB_flooding_dscr_t(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_vlan_flooding_get);
    VTSS_EXPOSE_ITR_PTR(VLAN_MIB_vid_all_iter);
    VTSS_EXPOSE_SET_PTR(vtss_appl_vlan_flooding_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN);
};

struct PortConf {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
                              expose::ParamVal<vtss_appl_vlan_port_conf_t *>> P;

    static constexpr const char *table_description =
            R"(Table of per-port configuration.)";

    static constexpr const char *index_description =
            R"(Each row contains the VLAN configuration for a port interface.)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, VLAN_MIB_ifindex_dscr_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_vlan_port_conf_t &i) {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(VLAN_MIB_interface_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_SET_PTR(vtss_appl_vlan_interface_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN);
};

struct SvlTable {
    typedef expose::ParamList<expose::ParamKey<mesa_vid_t>,
                              expose::ParamVal<mesa_vid_t>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
            "Shared VLAN Learning (SVL) allows for having one or more VLAN IDs "
            "map to the same Filter ID (FID). For a given set of VLANs, if an "
            "individual MAC address is learned in one VLAN, that learned information "
            "is used in forwarding decisions taken for that address relative to all "
            "other VLANs in the given set.\n"
            "fidCnt, which can be found in the capabilities section, "
            "indicates the number of FIDs available on this platform. The feature "
            "is not available if this number is 0.";

    static constexpr const char *index_description =
            "The table is indexed by VLAN ID";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_vid_t &i) {
        h.argument_properties(tag::Name("vid"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, VLAN_MIB_vid_dscr_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_vid_t &i) {
        h.argument_properties(tag::Name("fid"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, VLAN_MIB_fid_dscr_t(i));
    }

    VTSS_EXPOSE_GET_PTR(VLAN_MIB_svl_conf_get);
    VTSS_EXPOSE_ITR_PTR(VLAN_MIB_svl_conf_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_vlan_fid_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_vlan_fid_set);
    VTSS_EXPOSE_DEL_PTR(VLAN_MIB_svl_conf_del);
    VTSS_EXPOSE_DEF_PTR(VLAN_MIB_svl_conf_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_VLAN);
};

struct StatisIf {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
                              expose::ParamKey<vtss_appl_vlan_user_t>,
                              expose::ParamVal<vtss_appl_vlan_port_detailed_conf_t *>> P;

    static constexpr const char *table_description =
            R"(Table of per-interface (port) status.)";

    static constexpr const char *index_description =
            R"(Each row contains the VLAN configuration for a port interface for a given VLAN user.)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, VLAN_MIB_ifindex_dscr_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_vlan_user_t &i) {
        h.argument_properties(tag::Name("user"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, VLAN_MIB_vlan_user_dscr_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_vlan_port_detailed_conf_t &i) {
        h.argument_properties(tag::Name("details"));
        h.argument_properties(expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(VLAN_MIB_interface_detailed_conf_get);
    VTSS_EXPOSE_ITR_PTR(VLAN_MIB_ifindex_user_iter);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_VLAN);
};

struct StatusMembership {
    typedef expose::ParamList<expose::ParamKey<mesa_vid_t>,
                              expose::ParamKey<vtss_appl_vlan_user_t>,
                              expose::ParamVal<PortListStackable *>> P;

    static constexpr const char *table_description =
            "Table of per-VLAN, per-VLAN user port memberships.";

    static constexpr const char *index_description =
            "Each row contains a port list of VLAN memberships for a given VLAN and VLAN user."
            "The table is sparsely populated, so if a VLAN user doesn't contribute, the row is non-existent.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_vid_t &i) {
        h.argument_properties(tag::Name("vid"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, VLAN_MIB_vid_dscr_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_vlan_user_t &i) {
        h.argument_properties(tag::Name("user"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, VLAN_MIB_vlan_user_dscr_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(PortListStackable &i) {
        h.argument_properties(tag::Name("portlist"));
        h.argument_properties(expose::snmp::OidOffset(3));
        serialize(h, VLAN_MIB_PortListStackable_dscr_t(i));
    }

    VTSS_EXPOSE_GET_PTR(VLAN_MIB_membership_vid_user_get);
    VTSS_EXPOSE_ITR_PTR(VLAN_MIB_vid_user_iter);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_VLAN);
};

}  // namespace interfaces
}  // namespace vlan
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_VLAN_SERIALIZER_HXX__ */
