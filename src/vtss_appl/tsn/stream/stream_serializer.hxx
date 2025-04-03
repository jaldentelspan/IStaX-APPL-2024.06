/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _STREAM_SERIALIZER_HXX_
#define _STREAM_SERIALIZER_HXX_

#include <vtss/appl/stream.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/types.hxx>
#include <vtss_appl_serialize.hxx>
#include "vtss_appl_formatting_tags.hxx" /* For AsEtherType()                  */
#include "stream_api.h"                  /* For stream_util_XXX()              */
#include "stream_trace.h"
#include "mgmt_api.h"                    /* For mgmt_port_list_XXX_stackable() */

// This struct is used to wrap vtss_appl_stream_conf_t::port_list into
// a vtss_port_list_stackable_t type, which is serializable.
struct stream_serializer_conf_t {
    /**
     * The original stream configuration
     */
    vtss_appl_stream_conf_t conf;

    /**
     * conf.port_list converted to PortListStackable
     */
    vtss_port_list_stackable_t port_list_stackable;

    /**
     * conf.protocol.snap.oui converted to array
     */
    uint8_t snap_oui[3];
};

/******************************************************************************/
// stream_serializer_conf_default_get()
/******************************************************************************/
static inline mesa_rc stream_serializer_conf_default_get(vtss_appl_stream_id_t *stream_id, stream_serializer_conf_t *s)
{
    if (!stream_id || !s) {
        return VTSS_RC_ERROR;
    }

    *stream_id = VTSS_APPL_STREAM_ID_NONE;
    vtss_clear(*s); // Make sure to clear s->snap_oui[].
    VTSS_RC(vtss_appl_stream_conf_default_get(&s->conf));
    mgmt_port_list_to_stackable(s->port_list_stackable, s->conf.port_list);

    return VTSS_RC_OK;
}

/******************************************************************************/
// stream_serializer_collection_conf_default_get()
/******************************************************************************/
static inline mesa_rc stream_serializer_collection_conf_default_get(vtss_appl_stream_collection_id_t *stream_collection_id, vtss_appl_stream_collection_conf_t *s)
{
    if (!stream_collection_id || !s) {
        return VTSS_RC_ERROR;
    }

    *stream_collection_id = VTSS_APPL_STREAM_COLLECTION_ID_NONE;
    return vtss_appl_stream_collection_conf_default_get(s);
}

/******************************************************************************/
// stream_serializer_conf_default_get_no_id()
/******************************************************************************/
static inline mesa_rc stream_serializer_conf_default_get_no_id(stream_serializer_conf_t *s)
{
    vtss_appl_stream_id_t stream_id;
    return stream_serializer_conf_default_get(&stream_id, s);
}

/******************************************************************************/
// stream_serializer_conf_get()
/******************************************************************************/
static inline mesa_rc stream_serializer_conf_get(vtss_appl_stream_id_t stream_id, stream_serializer_conf_t *s)
{
    if (!s) {
        return VTSS_RC_ERROR;
    }

    vtss_clear(*s); // Make sure to clear s->snap_oui[]
    VTSS_RC(vtss_appl_stream_conf_get(stream_id, &s->conf));
    mgmt_port_list_to_stackable(s->port_list_stackable, s->conf.port_list);

    if (s->conf.protocol.type == MESA_VCE_TYPE_SNAP) {
        // Convert from uint32_t to uint8_t[3].
        s->snap_oui[0] = (s->conf.protocol.value.snap.oui >> 16) & 0xFF;
        s->snap_oui[1] = (s->conf.protocol.value.snap.oui >>  8) & 0xFF;
        s->snap_oui[2] = (s->conf.protocol.value.snap.oui >>  0) & 0xFF;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// stream_serializer_conf_set()
/******************************************************************************/
static inline mesa_rc stream_serializer_conf_set(vtss_appl_stream_id_t stream_id, const stream_serializer_conf_t *s)
{
    vtss_appl_stream_conf_t conf;

    if (!s) {
        return VTSS_RC_ERROR;
    }

    conf = s->conf;
    mgmt_port_list_from_stackable(s->port_list_stackable, conf.port_list);

    if (conf.protocol.type == MESA_VCE_TYPE_SNAP) {
        // Convert from uint8_t[3] to uint32_t
        conf.protocol.value.snap.oui = (s->snap_oui[0] << 16) | (s->snap_oui[1] << 8) | (s->snap_oui[2] << 0);
    }

    return vtss_appl_stream_conf_set(stream_id, &conf);
}

static inline const vtss_enum_descriptor_t stream_serializer_dmac_match_type_txt[] = {
    {VTSS_APPL_STREAM_DMAC_MATCH_TYPE_ANY,    "any"},
    {VTSS_APPL_STREAM_DMAC_MATCH_TYPE_MC,     "multicast"},
    {VTSS_APPL_STREAM_DMAC_MATCH_TYPE_BC,     "broadcast"},
    {VTSS_APPL_STREAM_DMAC_MATCH_TYPE_UC,     "unicast"},
    {VTSS_APPL_STREAM_DMAC_MATCH_TYPE_NOT_BC, "notBroadcast"},
    {VTSS_APPL_STREAM_DMAC_MATCH_TYPE_NOT_UC, "notUnicast"},
    {VTSS_APPL_STREAM_DMAC_MATCH_TYPE_VALUE,  "valueMask"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_stream_dmac_match_type_t,
                         "StreamDmacMatchType",
                         stream_serializer_dmac_match_type_txt,
                         "DMAC match type");

static inline const vtss_enum_descriptor_t stream_serializer_vlan_tag_match_type_txt[] = {
    {VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_BOTH,     "both"},
    {VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_UNTAGGED, "untagged"},
    {VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED,   "tagged"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_stream_vlan_tag_match_type_t,
                         "StreamlanTagMatchType",
                         stream_serializer_vlan_tag_match_type_txt,
                         "VLAN Tag match type");

static inline const vtss_enum_descriptor_t stream_serializer_vlan_tag_type_txt[] = {
    {VTSS_APPL_STREAM_VLAN_TAG_TYPE_ANY,      "any"},
    {VTSS_APPL_STREAM_VLAN_TAG_TYPE_C_TAGGED, "cTagged"},
    {VTSS_APPL_STREAM_VLAN_TAG_TYPE_S_TAGGED, "sTagged"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_stream_vlan_tag_type_t,
                         "StreamVlanTagType",
                         stream_serializer_vlan_tag_type_txt,
                         "VLAN Tag type");

static inline vtss_enum_descriptor_t stream_serializer_protocol_type_txt[] = {
    {MESA_VCE_TYPE_ANY,   "any"},
    {MESA_VCE_TYPE_ETYPE, "etype"},
    {MESA_VCE_TYPE_LLC,   "llc"},
    {MESA_VCE_TYPE_SNAP,  "snap"},
    {MESA_VCE_TYPE_IPV4,  "ipv4"},
    {MESA_VCE_TYPE_IPV6,  "ipv6"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(mesa_vce_type_t,
                         "StreamProtocolType",
                         stream_serializer_protocol_type_txt,
                         "Protocol type");

static inline vtss_enum_descriptor_t stream_serializer_snap_oui_type_txt[] = {
    {VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_RFC1042, "rfc1042"},
    {VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_8021H,   "dot1h"},
    {VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_CUSTOM,  "custom"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_stream_proto_snap_oui_type_t,
                         "StreamSnapOuiType",
                         stream_serializer_snap_oui_type_txt,
                         "SNAP OUI type");

static inline vtss_enum_descriptor_t stream_serializer_range_match_type_txt[] = {
    {VTSS_APPL_STREAM_RANGE_MATCH_TYPE_ANY,   "any"},
    {VTSS_APPL_STREAM_RANGE_MATCH_TYPE_VALUE, "value"},
    {VTSS_APPL_STREAM_RANGE_MATCH_TYPE_RANGE, "range"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_stream_range_match_type_t,
                         "StreamRangeMatchType",
                         stream_serializer_range_match_type_txt,
                         "Range match type");

static inline vtss_enum_descriptor_t stream_serializer_ip_proto_type_txt[] = {
    {VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_ANY,    "any"},
    {VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_CUSTOM, "custom"},
    {VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_TCP,    "tcp"},
    {VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_UDP,    "udp"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_stream_ip_protocol_type_t,
                         "StreamIpProtocolType",
                         stream_serializer_ip_proto_type_txt,
                         "IP protocol type");

/******************************************************************************/
// stream_serializer_id_index()
/******************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(stream_serializer_id_index, vtss_appl_stream_id_t, a, s)
{
    a.add_leaf(s.inner,
               vtss::tag::Name("StreamId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Stream ID."));
}

/******************************************************************************/
// stream_serializer_collection_id_index()
/******************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(stream_serializer_collection_id_index, vtss_appl_stream_collection_id_t, a, s)
{
    a.add_leaf(s.inner,
               vtss::tag::Name("StreamCollectionId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Stream collection ID."));
}

template <typename T>
void serialize(T &a, vtss_appl_stream_capabilities_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_stream_capabilities_t"));
    int ix = 1;

    m.add_leaf(s.inst_cnt_max,
               vtss::tag::Name("InstanceMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of creatable stream instances, numbered [1; max]."));
}

template <typename T>
void serialize_vlan_tag(T &m, vtss_appl_stream_vlan_tag_t &t, int &ix, const char *prefix)
{
    char name_buf[50], dscr_buf[200];

    // This is converted to text with stream_serializer_vlan_tag_match_type_txt[]
    sprintf(dscr_buf, "Select whether to match on a VLAN tag, untagged, or both. Only if matching on a VLAN tag is the remaining %sXXX options used.", prefix);
    m.add_leaf(t.match_type,
               vtss::tag::Name(prefix),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(dscr_buf));

    // This is converted to text with stream_serializer_vlan_tag_type_txt[]
    sprintf(name_buf, "%sTagType", prefix);
    m.add_leaf(t.tag_type,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Select whether to match on both C-, S-, and S-custom tagged frames or only C-tagged or only S- and S-custom tagged."));

    sprintf(name_buf, "%sVidValue", prefix);
    m.add_leaf(t.vid_value,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Use this and VidMask together to form the VLAN ID(s) to match on."));

    sprintf(name_buf, "%sVidMask", prefix);
    m.add_leaf(t.vid_mask,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Use this and VidValue together to form the VLAN ID(s) to match on. Set to all zeros in order not to match on any particular VLAN ID(s)."));

    sprintf(name_buf, "%sPcpValue", prefix);
    m.add_leaf(t.pcp_value,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Use this and PcpMask together to form the PCP value(s) to match on."));

    sprintf(name_buf, "%sPcpMask", prefix);
    m.add_leaf(t.pcp_mask,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Use this and PcpValue together to form the PCP value(s) to match on. Set to all zeros in order not to match on any particular PCP value(s)."));

    // This is converted to text with vtss_appl_vcap_bit_txt[]
    sprintf(name_buf, "%sDei", prefix);
    m.add_leaf(t.dei,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Select whether to match any DEI, DEI value 0 or DEI value 1."));
}

template <typename T>
void serialize_protocol_inner(T &m, stream_serializer_conf_t &c, int &ix)
{
    vtss_appl_stream_protocol_conf_t &p = c.conf.protocol;

    switch (p.type) {
    case MESA_VCE_TYPE_ETYPE:
        m.add_leaf(vtss::AsEtherType(p.value.etype.etype),
                   vtss::tag::Name("etherType"),
                   vtss::expose::snmp::RangeSpec<u32>(0x600, 0xffff),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("EtherType used when the selected protocol type is 'etype'. Must be in range 0x600-0xffff"));
        break;

    case MESA_VCE_TYPE_LLC:
        m.add_leaf(p.value.llc.dsap,
                   vtss::tag::Name("llcDsap"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Destination Service Access Point. Used when the selected protocol type is 'llc'"));

        m.add_leaf(p.value.llc.ssap,
                   vtss::tag::Name("llcSsap"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Source Service Access Point. Used when the selected protocol type is 'llc'"));
        break;

    case MESA_VCE_TYPE_SNAP:
        // This is converted to text with stream_serializer_snap_oui_type_txt[]
        m.add_leaf(p.value.snap.oui_type,
                   vtss::tag::Name("snapOuiType"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("SNAP OUI type. Used when the selected protocol type is 'snap'"));

        m.add_leaf(vtss::AsOctetString(c.snap_oui, sizeof(c.snap_oui)),
                   vtss::tag::Name("snapOui"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("SNAP OUI. Used when the selected protocol is 'snap' and the snapOuiType is 'custom'"));

        m.add_leaf(p.value.snap.pid,
                   vtss::tag::Name("snapPid"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("SNAP Protocol ID. Used when the selected protocol type is 'snap'"));
        break;

    case MESA_VCE_TYPE_IPV4:
        m.add_leaf(vtss::AsIpv4(p.value.ipv4.sip.address),
                   vtss::tag::Name("ipv4SipAddress"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Source IPv4 address to match on"));

        m.add_leaf(p.value.ipv4.sip.prefix_size,
                   vtss::tag::Name("ipv4SipPrefixSize"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("IPv4 source address' prefix size (0-32)"));

        m.add_leaf(vtss::AsIpv4(p.value.ipv4.dip.address),
                   vtss::tag::Name("ipv4DipAddress"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Destination IPv4 address to match on"));

        m.add_leaf(p.value.ipv4.dip.prefix_size,
                   vtss::tag::Name("ipv4DipPrefixSize"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("IPv4 destination address' prefix size (0-32)"));

        // This is converted to text with stream_serializer_range_match_type_txt
        m.add_leaf(p.value.ipv4.dscp.match_type,
                   vtss::tag::Name("ipv4DscpMatchType"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Controls whether to match on any DSCP value, a particular DSCP value or a range of DSCP values"));

        m.add_leaf(p.value.ipv4.dscp.low,
                   vtss::tag::Name("ipv4DscpLow"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("When ipv4DscpMatchType is value or range, this indicates the low value in the DSCP range"));

        m.add_leaf(p.value.ipv4.dscp.high,
                   vtss::tag::Name("ipv4DscpHigh"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("When ipv4DscpMatchType is range, this indicates the high value in the DSCP range"));

        m.add_leaf(p.value.ipv4.fragment,
                   vtss::tag::Name("ipv4Fragment"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Match on whether IPv4 header's MF is set or Fragment Offset > 0"));

        // This is converted to text with stream_serializer_ip_proto_type_txt[]
        m.add_leaf(p.value.ipv4.proto.type,
                   vtss::tag::Name("ipv4ProtoType"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Select whether to match on IPv4 header's protocol type, and if so, which value"));

        m.add_leaf(p.value.ipv4.proto.value,
                   vtss::tag::Name("ipv4ProtoValue"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("If ipv4ProtoType is Custom, this indicates the protocol value to match on in IPv4 header"));

        // This is converted to text with stream_serializer_range_match_type_txt
        m.add_leaf(p.value.ipv4.dport.match_type,
                   vtss::tag::Name("ipv4DportMatchType"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Controls whether to match on any TCP/UDP destination port value, a particular value or a range of values"));

        m.add_leaf(p.value.ipv4.dport.low,
                   vtss::tag::Name("ipv4DportLow"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("When ipv4DportMatchType is value or range, this indicates the low value in the UDP/TCP destination port range"));

        m.add_leaf(p.value.ipv4.dport.high,
                   vtss::tag::Name("ipv4DportHigh"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("When ipv4DportMatchType is range, this indicates the high value in the UDP/TCP destination poort range"));
        break;

    case MESA_VCE_TYPE_IPV6:
        m.add_leaf(p.value.ipv6.sip.address,
                   vtss::tag::Name("ipv6SipAddress"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Source IPv6 address to match on"));

        m.add_leaf(p.value.ipv6.sip.prefix_size,
                   vtss::tag::Name("ipv6SipPrefixSize"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("IPv6 source address' prefix size (0-128)"));

        m.add_leaf(p.value.ipv6.dip.address,
                   vtss::tag::Name("ipv6DipAddress"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Destination IPv6 address to match on"));

        m.add_leaf(p.value.ipv6.dip.prefix_size,
                   vtss::tag::Name("ipv6DipPrefixSize"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("IPv6 destination address' prefix size (0-128)"));

        // This is converted to text with stream_serializer_range_match_type_txt
        m.add_leaf(p.value.ipv6.dscp.match_type,
                   vtss::tag::Name("ipv6DscpMatchType"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Controls whether to match on any DSCP value, a particular DSCP value or a range of DSCP values"));

        m.add_leaf(p.value.ipv6.dscp.low,
                   vtss::tag::Name("ipv6DscpLow"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("When ipv6DscpMatchType is value or range, this indicates the low value in the DSCP range"));

        m.add_leaf(p.value.ipv6.dscp.high,
                   vtss::tag::Name("ipv6DscpHigh"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("When ipv6DscpMatchType is range, this indicates the high value in the DSCP range"));

        // This is converted to text with stream_serializer_ip_proto_type_txt[]
        m.add_leaf(p.value.ipv6.proto.type,
                   vtss::tag::Name("ipv6ProtoType"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Select whether to match on IPv6 header's protocol type, and if so, which value"));

        m.add_leaf(p.value.ipv6.proto.value,
                   vtss::tag::Name("ipv6ProtoValue"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("If ipv6ProtoType is Custom, this indicates the protocol value to match on in IPv6 header"));

        // This is converted to text with stream_serializer_range_match_type_txt
        m.add_leaf(p.value.ipv6.dport.match_type,
                   vtss::tag::Name("ipv6DportMatchType"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("Controls whether to match on any TCP/UDP destination port value, a particular value or a range of values"));

        m.add_leaf(p.value.ipv6.dport.low,
                   vtss::tag::Name("ipv6DportLow"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("When ipv6DportMatchType is value or range, this indicates the low value in the UDP/TCP destination port range"));

        m.add_leaf(p.value.ipv6.dport.high,
                   vtss::tag::Name("ipv6DportHigh"),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description("When ipv6DportMatchType is range, this indicates the high value in the UDP/TCP destination poort range"));
        break;

    case MESA_VCE_TYPE_ANY:
    default:
        // No fields.
        break;
    }
}

template <typename T>
void serialize_protocol_outer(T &m, stream_serializer_conf_t &c, int &ix, mesa_vce_type_t type)
{
    stream_serializer_conf_t default_conf;
    mesa_rc                  rc;

    // Always get a default conf. Otherwise, we cannot serialize correctly,
    // because we are serializing into a union, and when setting the
    // configuration, we are not allowed to overwrite the configuration we set
    // if it's not of the correct type, so we serialize into the local
    // variable, default_protocol_conf, which gets lost.
    // If we get, it's OK (actually required) to get a default configuration
    // when it's not of the correct type.
    if ((rc = stream_serializer_conf_default_get_no_id(&default_conf)) != VTSS_RC_OK) {
        T_E("stream_serializer_conf_default_get_no_id() failed: %s", error_txt(rc));
        vtss_clear(default_conf);
    }

    // Overwrite the protocol part with defaults for the particular type.
    if ((rc = vtss_appl_stream_conf_protocol_default_get(type, &default_conf.conf.protocol)) != VTSS_RC_OK) {
        T_E("vtss_apl_stream_conf_protocol_default_get(%s) failed: %s", stream_util_protocol_type_to_str(type), error_txt(rc));
        vtss_clear(default_conf.conf.protocol);
        default_conf.conf.protocol.type = type;
    }

    if (type == MESA_VCE_TYPE_SNAP) {
        // Convert uint32_t to uint8_t[3]
        default_conf.snap_oui[0] = (default_conf.conf.protocol.value.snap.oui >> 16) & 0xFF;
        default_conf.snap_oui[1] = (default_conf.conf.protocol.value.snap.oui >>  8) & 0xFF;
        default_conf.snap_oui[2] = (default_conf.conf.protocol.value.snap.oui >>  0) & 0xFF;
    }

    stream_serializer_conf_t &p = c.conf.protocol.type == type ? c : default_conf;

    // Do the actual serialization
    serialize_protocol_inner(m, p, ix);
}

template <typename T>
void serialize(T &a, stream_serializer_conf_t &s)
{
    typename T::Map_t       m = a.as_map(vtss::tag::Typename("vtss_appl_stream_conf_t"));
    int                     ix = 0;
    vtss_appl_stream_conf_t &c = s.conf;

    // This is converted to text with stream_serializer_dmac_match_type_txt[]
    m.add_leaf(c.dmac.match_type,
               vtss::tag::Name("dmacMatchType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates which destination MAC address type to match on"));
    m.add_leaf(c.dmac.value,
               vtss::tag::Name("dmac"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Destination MAC Address"));
    m.add_leaf(c.dmac.mask,
               vtss::tag::Name("dmacMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Destination MAC mask"));
    m.add_leaf(c.smac.value,
               vtss::tag::Name("smac"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("source MAC address"));
    m.add_leaf(c.smac.mask,
               vtss::tag::Name("smacMask"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("source MAC mask"));

    serialize_vlan_tag(m, c.outer_tag, ix, "outerTag");
    serialize_vlan_tag(m, c.inner_tag, ix, "innerTag");

    // This is converted to text with stream_serializer_protocol_type_txt[]
    m.add_leaf(c.protocol.type,
               vtss::tag::Name("protocolType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Protocol type"));

    // Serialize each of the protocol types separately.
    serialize_protocol_outer(m, s, ix, MESA_VCE_TYPE_ANY);
    serialize_protocol_outer(m, s, ix, MESA_VCE_TYPE_ETYPE);
    serialize_protocol_outer(m, s, ix, MESA_VCE_TYPE_LLC);
    serialize_protocol_outer(m, s, ix, MESA_VCE_TYPE_SNAP);
    serialize_protocol_outer(m, s, ix, MESA_VCE_TYPE_IPV4);
    serialize_protocol_outer(m, s, ix, MESA_VCE_TYPE_IPV6);

    vtss::PortListStackable &port_list = (vtss::PortListStackable &)s.port_list_stackable;

    m.add_leaf(port_list,
               vtss::tag::Name("portList"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Ports on which this stream are installed."));
}

template <typename T>
void serialize_client(T &m, vtss_appl_stream_action_t &s, int &ix, vtss_appl_stream_client_t client, bool is_stream)
{
    char namebuf[30], dscrbuf[100];

    sprintf(namebuf, "%sClientAttached", stream_util_client_to_str(client));
    sprintf(dscrbuf, "Indicates whether %s is attached to this stream%s", stream_util_client_to_str(client), is_stream ? "" : " collection");
    m.add_leaf(vtss::AsBool(s.enable),
               vtss::tag::Name(namebuf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(dscrbuf));

    sprintf(namebuf, "%sClientId", stream_util_client_to_str(client));
    sprintf(dscrbuf, "If %s is attached to this stream%s, this indicates its internal reference.", stream_util_client_to_str(client), is_stream ? "" : " collection");
    m.add_leaf(s.client_id,
               vtss::tag::Name(namebuf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(dscrbuf));
}

template <typename T>
void serialize(T &a, vtss_appl_stream_status_t &p)
{
    typename T::Map_t                m = a.as_map(vtss::tag::Typename("vtss_appl_stream_status_t"));
    vtss_appl_stream_oper_warnings_t w;
    vtss_appl_stream_client_t        client;
    mesa_bool_t                      b;
    char                             buf[400];
    int                              ix = 0;

    m.add_leaf(p.stream_collection_id,
               vtss::tag::Name("StreamCollectionId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If non-zero, indicates the stream collection this stream is part of."));

    b = p.oper_warnings == VTSS_APPL_STREAM_OPER_WARNING_NONE;
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNone"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("No configurational warnings found."));

    w = VTSS_APPL_STREAM_OPER_WARNING_NOT_INSTALLED_ON_ANY_PORT;
    b = (p.oper_warnings & w) != 0;
    (void)stream_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNotInstalledOnAnyPort"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    // This warning is obsolete and will always be false
    b = false;
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNoClientsAttached"),
               vtss::expose::snmp::Status::Obsoleted,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Obsolete. Will always be false"));

    for (client = (vtss_appl_stream_client_t)0; client < ARRSZ(p.client_status.clients); client++) {
        serialize_client(m, p.client_status.clients[client], ix, client, true /* is stream */);
    }
}

template <typename T>
void serialize(T &a, vtss_appl_stream_collection_capabilities_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_stream_collection_capabilities_t"));
    int ix = 1;

    m.add_leaf(s.inst_cnt_max,
               vtss::tag::Name("InstanceMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of creatable stream collection instances, numbered [1; max]."));

    m.add_leaf(s.streams_per_collection_max,
               vtss::tag::Name("StreamsPerCollectionMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of streams per collection."));
}

template <typename T>
void serialize(T &a, vtss_appl_stream_collection_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_stream_collection_conf_t"));
    int               ix = 0;
    uint32_t          i;
    char              namebuf[128], dscrbuf[128];

    for (i = 0; i < ARRSZ(s.stream_ids); i++) {
        sprintf(namebuf, "StreamId%u", i);
        sprintf(dscrbuf, "Stream ID list element %u", i);
        m.add_leaf(s.stream_ids[i],
                   vtss::tag::Name(namebuf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscrbuf));
    }
}

template <typename T>
void serialize(T &a, vtss_appl_stream_collection_status_t &p)
{
    typename T::Map_t                           m = a.as_map(vtss::tag::Typename("vtss_appl_stream_collection_status_t"));
    vtss_appl_stream_collection_oper_warnings_t w;
    vtss_appl_stream_client_t                   client;
    mesa_bool_t                                 b;
    char                                        buf[400];
    int                                         ix = 0;

    b = p.oper_warnings == VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NONE;
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNone"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("No configurational warnings found."));

    w = VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NO_STREAMS_ATTACHED;
    b = (p.oper_warnings & w) != 0;
    (void)stream_collection_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNoStreamsAttached"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    w = VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NO_CLIENTS_ATTACHED;
    b = (p.oper_warnings & w) != 0;
    (void)stream_collection_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNoClientsAttached"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    w = VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_AT_LEAST_ONE_STREAM_HAS_OPERATIONAL_WARNINGS;
    b = (p.oper_warnings & w) != 0;
    (void)stream_collection_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningAtLeastOneStreamHasOperWarn"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    for (client = (vtss_appl_stream_client_t)0; client < ARRSZ(p.client_status.clients); client++) {
        serialize_client(m, p.client_status.clients[client], ix, client, false /* is stream collection */);
    }
}

namespace vtss
{
namespace appl
{
namespace stream
{
namespace interfaces
{

/******************************************************************************/
// StreamCapabilities
/******************************************************************************/
struct StreamCapabilities {
    typedef vtss::expose::ParamList <vtss::expose::ParamVal<vtss_appl_stream_capabilities_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_stream_capabilities_t &s)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, s);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_stream_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_STREAM);
};

/******************************************************************************/
// StreamDefaultConf
/******************************************************************************/
struct StreamDefaultConf {
    typedef expose::ParamList <expose::ParamVal<stream_serializer_conf_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(stream_serializer_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(stream_serializer_conf_default_get_no_id);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_STREAM);
};

/******************************************************************************/
// StreamConf
/******************************************************************************/
struct StreamConf {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_stream_id_t>, expose::ParamVal<stream_serializer_conf_t *>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;

    static constexpr const char *table_description =
        "Stream configuration table.";

    static constexpr const char *index_description =
        "Each entry in this table represents a stream used for traffic filtering.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_stream_id_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, stream_serializer_id_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(stream_serializer_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_stream_itr);
    VTSS_EXPOSE_GET_PTR(stream_serializer_conf_get);
    VTSS_EXPOSE_SET_PTR(stream_serializer_conf_set);
    VTSS_EXPOSE_ADD_PTR(stream_serializer_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_stream_conf_del);
    VTSS_EXPOSE_DEF_PTR(stream_serializer_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_STREAM);
};

/******************************************************************************/
// StreamStatus
/******************************************************************************/
struct StreamStatus {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_stream_id_t>, expose::ParamVal<vtss_appl_stream_status_t *>> P;

    static constexpr const char *table_description =
        "Contains status per stream\n";

    static constexpr const char *index_description =
        "Stream ID\n";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_stream_id_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, stream_serializer_id_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_stream_status_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_stream_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_stream_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_STREAM);
};

/******************************************************************************/
// StreamCollectionCapabilities
/******************************************************************************/
struct StreamCollectionCapabilities {
    typedef vtss::expose::ParamList <vtss::expose::ParamVal<vtss_appl_stream_collection_capabilities_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_stream_collection_capabilities_t &s)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, s);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_stream_collection_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_STREAM);
};

/******************************************************************************/
// StreamCollectionDefaultConf
/******************************************************************************/
struct StreamCollectionDefaultConf {
    typedef expose::ParamList <expose::ParamVal<vtss_appl_stream_collection_conf_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_stream_collection_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_stream_collection_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_STREAM);
};

/******************************************************************************/
// StreamCollectionConf
/******************************************************************************/
struct StreamCollectionConf {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_stream_collection_id_t>, expose::ParamVal<vtss_appl_stream_collection_conf_t *>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;

    static constexpr const char *table_description =
        "Stream collection configuration table.";

    static constexpr const char *index_description =
        "Each entry in this table represents a stream collection used to gather streams for traffic filtering.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_stream_collection_id_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, stream_serializer_collection_id_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_stream_collection_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_stream_collection_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_stream_collection_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_stream_collection_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_stream_collection_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_stream_collection_conf_del);
    VTSS_EXPOSE_DEF_PTR(stream_serializer_collection_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_STREAM);
};

/******************************************************************************/
// StreamCollectionStatus
/******************************************************************************/
struct StreamCollectionStatus {
    typedef expose::ParamList <expose::ParamKey<vtss_appl_stream_collection_id_t>, expose::ParamVal<vtss_appl_stream_collection_status_t *>> P;

    static constexpr const char *table_description =
        "Contains status per stream collection\n";

    static constexpr const char *index_description =
        "Stream Collection ID\n";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_stream_collection_id_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, stream_serializer_collection_id_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_stream_collection_status_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_stream_collection_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_stream_collection_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_STREAM);
};

}  // namespace interfaces
}  // namespace stream
}  // namespace appl
}  // namespace vtss

#endif  // _STREAM_SERIALIZER_HXX_

