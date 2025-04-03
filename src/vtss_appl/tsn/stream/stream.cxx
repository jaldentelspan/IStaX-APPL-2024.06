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

#include "stream_api.h"
#include "vcl_api.h"                       // For vcl_XXX() and VCL_TYPE_STREAM
#include "stream_trace.h"
#include "mac_utils.hxx"                   // For mac_is_XXX()
#include "misc_api.h"                      // For misc_ipv4_txt()
#include "ip_utils.hxx"                    // For vtss_ipv4_prefix_to_mask()
#include <vtss/appl/stream.h>
#include <vtss/appl/vlan.h>                // For VTSS_APPL_VLAN_ID_MAX
#include <vtss/basics/memcmp-operator.hxx> // For VTSS_BASICS_MEMCMP_OPERATOR()

#define STREAM_SNAP_OUI_RFC1042 0x000000 /* 00:00:00 */
#define STREAM_SNAP_OUI_8021H   0x0000F8 /* 00:00:f8 */

#define STREAM_IP_PROTO_TCP  6
#define STREAM_IP_PROTO_UDP 17

//*****************************************************************************/
// Trace definitions
/******************************************************************************/
static vtss_trace_reg_t STREAM_trace_reg = {
    VTSS_TRACE_MODULE_ID, "stream", "Streams"
};

static vtss_trace_grp_t STREAM_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC,
    },

    [STREAM_TRACE_GRP_API] = {
        "api",
        "MESA calls",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC,
    },

    [STREAM_TRACE_GRP_ICLI] = {
        "CLI",
        "Command Line Interface",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC,
    },

    [STREAM_TRACE_GRP_NOTIF] = {
        "notifications",
        "Notifications",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC,
    },

    [STREAM_TRACE_GRP_COLLECTION] = {
        "collection",
        "Stream collections",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC,
    },
};

VTSS_TRACE_REGISTER(&STREAM_trace_reg, STREAM_trace_grps);

// We need this one when inserting a stream into a set.
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_stream_conf_t);

// We need this when comparing PSFP configuration in STREAM_action_client_set()
VTSS_BASICS_MEMCMP_OPERATOR(mesa_psfp_iflow_conf_t);

// We need this when comparing FRER configuration in STREAM_action_client_set()
VTSS_BASICS_MEMCMP_OPERATOR(mesa_frer_iflow_conf_t);

// We keep track of streams in a map.
typedef struct {
    // This stream is identified by this ID
    vtss_appl_stream_id_t stream_id;

    // The stream's user configuration
    vtss_appl_stream_conf_t conf;

    // The VCE we are populating.
    //   - vce.key is populated by user configuration.
    //   - vce.action is populated by clients that are attached.
    mesa_vce_t vce;

    // If this stream is NOT part of a stream collection, this is the IFLOW
    // configuration applied to vce.action. The IFLOW is only allocated if at
    // least one client is attached.
    mesa_iflow_conf_t iflow_conf;

    // The IFLOW ID of the iflow_conf.
    mesa_iflow_id_t iflow_id;

    // This stream's status. This is valid whether or not the stream is part of
    // a stream collection.
    // If the stream is not part of a stream collection,
    // status.stream_collection_id is VTSS_APPL_STREAM_COLLECTION_ID_NONE.
    // Otherwise it points to the stream-collection it's part of.
    vtss_appl_stream_status_t status;
} stream_state_t;

typedef vtss::Map<vtss_appl_stream_id_t, stream_state_t> stream_map_t;
typedef stream_map_t::iterator stream_itr_t;

// We keep track of stream collections in a map
typedef struct {
    // This stream collection is identified by this ID
    vtss_appl_stream_collection_id_t stream_collection_id;

    // The stream collection's user configuration
    vtss_appl_stream_collection_conf_t conf;

    // All streams in the stream collection use the same IFLOW configuration.
    mesa_iflow_conf_t iflow_conf;

    // The IFLOW ID of the iflow_conf.
    mesa_iflow_id_t iflow_id;

    // This stream collection's status.
    vtss_appl_stream_collection_status_t status;
} stream_collection_state_t;

typedef vtss::Map<vtss_appl_stream_collection_id_t, stream_collection_state_t> stream_collection_map_t;
typedef stream_collection_map_t::iterator stream_collection_itr_t;

static critd_t                                    STREAM_crit;
static stream_map_t                               STREAM_map;
static stream_collection_map_t                    STREAM_collection_map;
static vtss_appl_stream_capabilities_t            STREAM_cap;
static vtss_appl_stream_collection_capabilities_t STREAM_collection_cap;
static mesa_vce_action_t                          STREAM_vce_action_default;
static vtss_appl_stream_action_t                  STREAM_action_default[VTSS_APPL_STREAM_CLIENT_CNT];
static mesa_iflow_conf_t                          STREAM_iflow_conf_default;

// We can send notifications to observers when someone adds or deletes a stream
// or changes the configuration's portlist (the latter is needed by FRER).
stream_notif_table_t stream_notif_table("stream_notif_table", VTSS_MODULE_ID_STREAM);

// We can send notifications to observers when someone adds, modifies or deletes
// a stream collection.
stream_collection_notif_table_t stream_collection_notif_table("stream_collection_notif_table", VTSS_MODULE_ID_STREAM);

// Whenever we update either of the two notif_tables, we either add, modify, or
// delete an entry
typedef enum {
    STREAM_NOTIF_STATUS_ADD,
    STREAM_NOTIF_STATUS_MOD,
    STREAM_NOTIF_STATUS_DEL
} stream_notif_status_t;

struct STREAM_Lock {
    STREAM_Lock(const char *file, int line)
    {
        critd_enter(&STREAM_crit, file, line);
    }

    ~STREAM_Lock()
    {
        critd_exit(&STREAM_crit, __FILE__, 0);
    }
};

#define STREAM_LOCK_SCOPE() STREAM_Lock __stream_lock_guard__(__FILE__, __LINE__)
#define STREAM_LOCK_ASSERT_LOCKED(_fmt_, ...) if (!critd_is_locked(&STREAM_crit)) {T_E(_fmt_, ##__VA_ARGS__);}

// There's no such define in MESA :-(
#define STREAM_VCE_ID_NONE 0xFFFFFFFF

/******************************************************************************/
// mesa_frer_iflow_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_frer_iflow_conf_t &conf)
{
    o << "{mstream_enable = " << conf.mstream_enable
      << ", mstream_id = "    << conf.mstream_id
      << ", generation = "    << conf.generation
      << ", pop = "           << conf.pop
      << "}";

    return o;
}

/******************************************************************************/
// mesa_psfp_iflow_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_psfp_iflow_conf_t &conf)
{
    o << "{filter_enable = " << conf.filter_enable
      << ", filter_id = "    << conf.filter_id
      << "}";

    return o;
}

/******************************************************************************/
// mesa_iflow_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_iflow_conf_t &conf)
{
    o << "{cnt_enable = "           << conf.cnt_enable
      << ", cnt_id = "              << conf.cnt_id
      << ", dlb_enable = "          << conf.dlb_enable
      << ", dlb_id = "              << conf.dlb_id
      << ", voe_idx = "             << conf.voe_idx
      << ", voi_idx = "             << conf.voi_idx
      << ", frer = "                << conf.frer // Using mesa_frer_iflow_conf_t::operator<<()
      << ", psfp = "                << conf.psfp // Using mesa_psfp_iflow_conf_t::operator<<()
      << ", cut_through_disable = " << conf.cut_through_disable
      << ", ot = "                  << conf.ot
      << "}";

    return o;
}

/******************************************************************************/
// mesa_iflow_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_iflow_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_vcap_vid_t:operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vcap_vid_t &v)
{
    o << "{value = "  << v.value
      << ", mask = "  << "0x" << vtss::hex(v.mask)
      << "}";

    return o;
}

/******************************************************************************/
// STREAM_vcap_bit_to_str()
/******************************************************************************/
static const char *STREAM_vcap_bit_to_str(mesa_vcap_bit_t vcap_bit)
{
    switch (vcap_bit) {
    case MESA_VCAP_BIT_ANY:
        return "Any";

    case MESA_VCAP_BIT_0:
        return "0";

    case MESA_VCAP_BIT_1:
        return "1";

    default:
        T_E("Invalid vcap_bit (%d)", vcap_bit);
        return "Unknown";
    }
}

/******************************************************************************/
// STREAM_vlan_tag_match_type_to_str()
/******************************************************************************/
static const char *STREAM_vlan_tag_match_type_to_str(vtss_appl_stream_vlan_tag_match_type_t m)
{
    switch (m) {
    case VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_BOTH:
        return "both";

    case VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_UNTAGGED:
        return "untagged";

    case VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED:
        return "tagged";

    default:
        T_E("Invalid match (%d)", m);
        return "Unknown";
    }
}

/******************************************************************************/
// STREAM_vlan_tag_type_to_str()
/******************************************************************************/
static const char *STREAM_vlan_tag_type_to_str(vtss_appl_stream_vlan_tag_type_t t)
{
    switch (t) {
    case VTSS_APPL_STREAM_VLAN_TAG_TYPE_ANY:
        return "any";

    case VTSS_APPL_STREAM_VLAN_TAG_TYPE_C_TAGGED:
        return "c-tagged";

    case VTSS_APPL_STREAM_VLAN_TAG_TYPE_S_TAGGED:
        return "s-tagged";

    default:
        T_E("Invalid tag_type (%d)", t);
        return "Unknown";
    }
}

/******************************************************************************/
// STREAM_snap_oui_type_to_str()
/******************************************************************************/
static const char *STREAM_snap_oui_type_to_str(vtss_appl_stream_proto_snap_oui_type_t oui_type)
{
    switch (oui_type) {
    case VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_CUSTOM:
        return "Custom";

    case VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_RFC1042:
        return "RFC1042";

    case VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_8021H:
        return "802.1H";

    default:
        T_E("Invalid SNAP OUI type (%d)", oui_type);
        return "Invalid";
    }
}

/******************************************************************************/
// STREAM_vcap_vr_type_to_str()
/******************************************************************************/
static const char *STREAM_vcap_vr_type_to_str(mesa_vcap_vr_type_t type)
{
    switch (type) {
    case MESA_VCAP_VR_TYPE_VALUE_MASK:
        return "Value/Mask";

    case MESA_VCAP_VR_TYPE_RANGE_INCLUSIVE:
        return "RangeInclusive";

    case MESA_VCAP_VR_TYPE_RANGE_EXCLUSIVE:
        return "RangeExclusive";

    default:
        T_E("Invalid type (%d)", type);
        return "Invalid";
    }
}

/******************************************************************************/
// STREAM_range_match_type_to_str()
/******************************************************************************/
static const char *STREAM_range_match_type_to_str(vtss_appl_stream_range_match_type_t t)
{
    switch (t) {
    case VTSS_APPL_STREAM_RANGE_MATCH_TYPE_ANY:
        return "Any";

    case VTSS_APPL_STREAM_RANGE_MATCH_TYPE_VALUE:
        return "Value";

    case VTSS_APPL_STREAM_RANGE_MATCH_TYPE_RANGE:
        return "Range";

    default:
        T_E("Invalid type (%d)", t);
        return "Invalid";
    }
}

/******************************************************************************/
// STREAM_ip_proto_type_to_str()
/******************************************************************************/
static const char *STREAM_ip_proto_type_to_str(vtss_appl_stream_ip_protocol_type_t p)
{
    switch (p) {
    case VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_ANY:
        return "Any";

    case VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_CUSTOM:
        return "Custom";

    case VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_TCP:
        return "TCP";

    case VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_UDP:
        return "UDP";

    default:
        T_E("Invalid type (%d)", p);
        return "Invalid";
    }
}

/******************************************************************************/
// mesa_vcap_u8_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vcap_u8_t &v)
{
    o << "{value = " << v.value
      << ", mask = "  << "0x" << vtss::hex(v.mask)
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vcap_u16_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vcap_u16_t &val)
{
    uint16_t v, m;

    v = (val.value[0] << 16) | (val.value[1] << 0);
    m = (val.mask[0]  << 16) | (val.mask[1]  << 0);
    o << "{value = " << v
      << ", mask = "  << "0x" << vtss::hex(m)
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vcap_u32_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vcap_u32_t &val)
{
    uint32_t v, m;

    v = (val.value[0] << 24) | (val.value[1] << 16) | (val.value[2] << 8) | (val.value[3] << 0);
    m = (val.mask[0]  << 24) | (val.mask[1]  << 16) | (val.mask[2]  << 8) | (val.mask[3]  << 0);

    o << "{value = " << v
      << ", mask = "  << "0x" << vtss::hex(m)
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vcap_u48_t:operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vcap_u48_t &b)
{
    mesa_mac_t mv, mm;
    memcpy(mv.addr, b.value, sizeof(mv.addr));
    memcpy(mm.addr, b.mask,  sizeof(mm.addr));

    o << "{value = "  << mv
      << ", mask = "  << mm
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vcap_u128_t:operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vcap_u128_t &b)
{
    mesa_ipv6_t iv, im;
    memcpy(iv.addr, b.value, sizeof(iv.addr));
    memcpy(im.addr, b.mask,  sizeof(im.addr));

    o << "{value = "  << iv // From ip_utils.hxx#mesa_ipv6_t::operator<<()
      << ", mask = "  << im // From ip_utils.hxx#mesa_ipv6_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vcap_vr_v_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vcap_vr_v_t &v)
{
    o << ", value = " << v.value
      << ", mask = "  << "0x" << vtss::hex(v.mask);

    return o;
}

/******************************************************************************/
// mesa_vcap_vr_r_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vcap_vr_r_t &r)
{
    o << ", low = "  << r.low
      << ", high = " << r.high;

    return o;
}

/******************************************************************************/
// mesa_vcap_vr_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vcap_vr_t &vr)
{
    o << "{type = " << STREAM_vcap_vr_type_to_str(vr.type);

    if (vr.type == MESA_VCAP_VR_TYPE_VALUE_MASK) {
        o << vr.vr.v; // Using mesa_vcap_vr_v_t::operator<<()
    } else {
        o << vr.vr.r; // Using mesa_vcap_vr_r_t::operator<<()
    }

    o << "}";

    return o;
}

/******************************************************************************/
// mesa_vcap_ip_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vcap_ip_t &ip)
{
    char ip_val_buf[20], ip_mask_buf[20];

    o << "{value = " << misc_ipv4_txt(ip.value, ip_val_buf)
      << ", mask = " << misc_ipv4_txt(ip.mask,  ip_mask_buf)
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vce_mac_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vce_mac_t &m)
{
    o << "{dmac_mc = "  << STREAM_vcap_bit_to_str(m.dmac_mc)
      << ", dmac_bc = " << STREAM_vcap_bit_to_str(m.dmac_bc)
      << ", dmac = "    << m.dmac    // Using mesa_vcap_u48_t::operator<<()
      << ", smac = "    << m.smac    // Using mesa_vcap_u48_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vce_tag_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vce_tag_t &t)
{
    o << "{vid = "     << t.vid // Using mesa_vcap_vid_t::operator<<()
      << ", pcp = "    << t.pcp // Using mesa_vcap_u8_t::operator<<()
      << ", dei = "    << STREAM_vcap_bit_to_str(t.dei)
      << ", tagged = " << STREAM_vcap_bit_to_str(t.tagged)
      << ", s_tag = "  << STREAM_vcap_bit_to_str(t.s_tag)
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vce_frame_etype_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vce_frame_etype_t &e)
{
    o << "{etype = " << e.etype // Using mesa_vcap_u16_t::operator<<()
      << ", data = " << e.data  // Using mesa_vcap_u32_t::operator<<()
      << ", mel = "  << e.mel   // Using mesa_vcap_u8_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vce_frame_llc_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vce_frame_llc_t &l)
{
    o << "{data = " << l.data // Using mesa_vcap_u48_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vce_frame_snap_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vce_frame_snap_t &s)
{
    o << "{data = " << s.data // Using mesa_vcap_u48_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vce_frame_ipv4_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vce_frame_ipv4_t &i)
{
    o << "{fragment = " << STREAM_vcap_bit_to_str(i.fragment)
      << ", options = " << STREAM_vcap_bit_to_str(i.options)
      << ", dscp = "    << i.dscp  // Using mesa_vcap_vr_t::operator<<()
      << ", proto = "   << i.proto // Using mesa_vcap_u8_t::operator<<()
      << ", sip = "     << i.sip   // Using mesa_vcap_ip_t::operator<<()
      << ", dip = "     << i.dip   // Using mesa_vcap_ip_t::operator<<()
      << ", sport = "   << i.sport // Using mesa_vcap_vr_t::operator<<()
      << ", dport = "   << i.dport // Using mesa_vcap_vr_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vce_frame_ipv6_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vce_frame_ipv6_t &i)
{
    o << "{dscp = "   << i.dscp  // Using mesa_vcap_vr_t::operator<<()
      << ", proto = " << i.proto // Using mesa_vcap_u8_t::operator<<()
      << ", sip = "   << i.sip   // Using mesa_vcap_u128_t::operator<<()
      << ", dip = "   << i.dip   // Using mesa_vcap_u128_t::operator<<()
      << ", sport = " << i.sport // Using mesa_vcap_vr_t::operator<<()
      << ", dport = " << i.dport // Using mesa_vcap_vr_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vce_key_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vce_key_t &key)
{
    o << "{port_list = "  << key.port_list
      << ", mac = "       << key.mac        // Using mesa_vce_mac_t::operator<<()
      << ", tag = "       << key.tag        // Using mesa_vce_tag_t::operator<<()
      << ", inner_tag = " << key.inner_tag  // Using mesa_vce_tag_t::operator<<()
      << ", type = "      << stream_util_protocol_type_to_str(key.type)
      << ", frame = ";

    switch (key.type) {
    case MESA_VCE_TYPE_ANY:
        break;

    case MESA_VCE_TYPE_ETYPE:
        o << key.frame.etype; // Using mesa_vce_frame_etype_t::operator<<()
        break;

    case MESA_VCE_TYPE_LLC:
        o << key.frame.llc; // Using mesa_vce_frame_llc_t::operator<<()
        break;

    case MESA_VCE_TYPE_SNAP:
        o << key.frame.snap; // Using mesa_vce_frame_snap_t::operator<<()
        break;

    case MESA_VCE_TYPE_IPV4:
        o << key.frame.ipv4; // Using mesa_vce_frame_ipv4_t::operator<<()
        break;

    case MESA_VCE_TYPE_IPV6:
        o << key.frame.ipv6; // Using mesa_vce_frame_ipv6_t::operator<<()
        break;
    }

    o << "}";

    return o;
}

/******************************************************************************/
// mesa_vce_action_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vce_action_t &a)
{
    o << "{vid = "          << a.vid
      << ", policy_no = "   << a.policy_no
      << ", pop_enable = "  << a.pop_enable
      << ", pop_cnt = "     << a.pop_cnt
      << ", map_sel = "     << a.map_sel // Using mesa_imap_sel_t::operator<<()
      << ", map_id = "      << a.map_id
      << ", flow_id = "     << a.flow_id
      << ", oam_detect = "  << a.oam_detect
      << ", mrp_enable = "  << a.mrp_enable
      << ", prio_enable = " << a.prio_enable
      << ", prio = "        << a.prio
      << ", dp_enable = "   << a.dp_enable
      << ", dp = "          << a.dp
      << ", dscp_enable = " << a.dscp_enable
      << ", dscp = "        << a.dscp
      << ", pcp_enable = "  << a.pcp_enable
      << ", pcp = "         << a.pcp
      << ", dei_enable = "  << a.dei_enable
      << ", dei = "         << a.dei
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vce_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_vce_t &conf)
{
    o << "{id = "      << "0x" << vtss::hex(conf.id)
      << ", key = "    << conf.key    // Using mesa_vce_key_t::operator<<()
      << ", action = " << conf.action // Using mesa_vce_action_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_vce_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_vce_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_stream_vlan_tag_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_stream_vlan_tag_t &v)
{
    o << "{match_type = " << STREAM_vlan_tag_match_type_to_str(v.match_type)
      << ", tag_type = "  << STREAM_vlan_tag_type_to_str(v.tag_type)
      << ", vid_value = " << v.vid_value
      << ", vid_mask = "  << "0x" << vtss::hex(v.vid_mask)
      << ", pcp_value = " << v.pcp_value
      << ", pcp_mask = "  << "0x" << vtss::hex(v.pcp_mask)
      << ", dei = "       << STREAM_vcap_bit_to_str(v.dei)
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_stream_range_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_stream_range_t &r)
{
    o << "{match = " << STREAM_range_match_type_to_str(r.match_type)
      << ", low = "  << r.low
      << ", high = " << r.high
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_stream_ip_protocol_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_stream_ip_protocol_t &p)
{
    o << "{type = "   << STREAM_ip_proto_type_to_str(p.type)
      << ", value = " << p.value
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_stream_protocol_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_stream_protocol_conf_t &p)
{
    o << "{type = " << stream_util_protocol_type_to_str(p.type);

    switch (p.type) {
    case MESA_VCE_TYPE_ANY:
        break;

    case MESA_VCE_TYPE_ETYPE:
        o << ", etype = "    << "0x" << vtss::hex(p.value.etype.etype);
        break;

    case MESA_VCE_TYPE_LLC:
        o << ", dsap = "     << p.value.llc.dsap
          << ", ssap = "     << p.value.llc.ssap;
        break;

    case MESA_VCE_TYPE_SNAP:
        o << ", oui_type = " << STREAM_snap_oui_type_to_str(p.value.snap.oui_type)
          << ", oui = "      << "0x" << vtss::hex(p.value.snap.oui)
          << ", pid = "      << "0x" << vtss::hex(p.value.snap.pid);
        break;

    case MESA_VCE_TYPE_IPV4:
        o << ", sip = "      << p.value.ipv4.sip
          << ", dip = "      << p.value.ipv4.dip
          << ", dscp = "     << p.value.ipv4.dscp   // Using vtss_appl_stream_range_t::operator<<()
          << ", fragment = " << STREAM_vcap_bit_to_str(p.value.ipv4.fragment)
          << ", proto = "    << p.value.ipv4.proto  // Using vtss_appl_stream_ip_protocol_t::operator<<()
          << ", dport = "    << p.value.ipv4.dport; // Using vtss_appl_stream_range_t::operator<<()
        break;

    case MESA_VCE_TYPE_IPV6:
    default:
        o << ", sip = "      << p.value.ipv6.sip
          << ", dip = "      << p.value.ipv6.dip
          << ", dscp = "     << p.value.ipv6.dscp   // Using vtss_appl_stream_range_t::operator<<()
          << ", proto = "    << p.value.ipv6.proto  // Using vtss_appl_stream_ip_protocol_t::operator<<()
          << ", dport = "    << p.value.ipv6.dport; // Using vtss_appl_stream_range_t::operator<<()
        break;
    }

    o << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_stream_dmac_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_stream_dmac_t &d)
{
    o << "{match_type = " << stream_util_dmac_match_type_to_str(d.match_type)
      << ", value = "     << d.value
      << ", mask = "      << d.mask
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_stream_smac_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_stream_smac_t &s)
{
    o << "{value = " << s.value
      << ", mask = " << s.mask
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_stream_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_stream_conf_t &conf)
{
    o << "{dmac = "       << conf.dmac      // Using vtss_appl_stream_dmac_t::operator<<()
      << ", smac = "      << conf.smac      // Using vtss_appl_stream_smac_t::operator<<()
      << ", outer_tag = " << conf.outer_tag // Using vtss_appl_stream_vlan_tag_t::operator<<()
      << ", inner_tag = " << conf.inner_tag // Using vtss_appl_stream_vlan_tag_t::operator<<()
      << ", protocol = "  << conf.protocol  // Using vtss_appl_stream_protocol_conf_t::operator<<()
      << ", port_list = " << conf.port_list
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_stream_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_stream_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_stream_collection_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_stream_collection_conf_t &conf)
{
    char buf[100];

    o << "{stream_ids = " << stream_collection_util_stream_id_list_to_str(conf.stream_ids, ARRSZ(conf.stream_ids), buf) << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_stream_collection_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_stream_collection_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// stream_action_t.
// Used to be able to trace the union, because without it we wouldn't know
// whether it belonged to PSFP or FRER.
/******************************************************************************/
struct stream_action_t {
    vtss_appl_stream_action_t action;
    vtss_appl_stream_client_t client;

public:
    stream_action_t(vtss_appl_stream_action_t &a, vtss_appl_stream_client_t c) : action(a), client(c) {};
};

/******************************************************************************/
// vtss_appl_stream_action_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const struct stream_action_t &a)
{
    o << "{enable = "                << a.action.enable
      << ", cut_through_override = " << a.action.cut_through_override
      << ", cut_through_disable = "  << a.action.cut_through_disable
      << ", client_id = "            << a.action.client_id;

    if (a.client == VTSS_APPL_STREAM_CLIENT_PSFP) {
        o << ", dlb_enable = " << a.action.psfp.dlb_enable
          << ", dlb_id = "     << a.action.psfp.dlb_id
          << ", psfp = "       << a.action.psfp.psfp // Using mesa_psfp_iflow_conf_t::operator<<()
          << "}";
    } else {
        o << ", vid =  "       << a.action.frer.vid
          << ", pop_enable = " << a.action.frer.pop_enable
          << ", pop_cnt = "    << a.action.frer.pop_cnt
          << ", frer = "       << a.action.frer.frer // Using mesa_frer_iflow_conf_t::operator<<()
          << "}";
    }

    return o;
}

/******************************************************************************/
// vtss_appl_stream_action_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const struct stream_action_t *a)
{
    o << *a;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_stream_client_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_stream_client_status_t &s)
{
    vtss_appl_stream_client_t client;
    char                      clients_str[100], buf[100];
    bool                      first;

    clients_str[0] = '\0';
    first          = true;

    for (client = (vtss_appl_stream_client_t)0; client < ARRSZ(s.clients); client++) {
        if (!s.clients[client].enable) {
            // This client is not associated with this stream
            continue;
        }

        sprintf(buf, "%s%s (%u)", first ? "" : ", ", stream_util_client_to_str(client, true), s.clients[client].client_id);
        strcat(clients_str, buf);
        first = false;
    }

    o << "{" << clients_str << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_stream_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_stream_status_t &s)
{
    char buf[400];

    o << "{stream_collection_id = " << s.stream_collection_id
      << ", oper_warnings = "       << stream_util_oper_warnings_to_str(buf, sizeof(buf), s.oper_warnings)
      << ", clients = "             << s.client_status // Using vtss_appl_stream_client_status_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_stream_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_stream_status_t *status)
{
    o << *status;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_stream_collection_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_stream_collection_status_t &s)
{
    char buf[400];

    o << "{oper_warnings = " << stream_collection_util_oper_warnings_to_str(buf, sizeof(buf), s.oper_warnings)
      << ", clients = "      << s.client_status // Using vtss_appl_stream_client_status_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_stream_collection_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_stream_collection_status_t *status)
{
    o << *status;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// STREAM_mesa_iflow_and_ingress_cnt_alloc()
/******************************************************************************/
static mesa_rc STREAM_mesa_iflow_and_ingress_cnt_alloc(mesa_iflow_conf_t &iflow_conf, mesa_iflow_id_t &iflow_id, bool &iflow_changed, bool &vce_changed)
{
    mesa_rc rc;

    // First, allocate an ingress counter if not already allocated. We cannot
    // use the cnt_id to determine whether a counter is allocated, because the
    // default cnt_id is 0, which is also a cnt_id we can get from a call to
    // mesa_ingress_cnt_alloc() :-(. So we have to rely on cnt_enable.
    if (!iflow_conf.cnt_enable) {
        iflow_changed = true;

        if ((rc = mesa_ingress_cnt_alloc(nullptr, 1, &iflow_conf.cnt_id)) != VTSS_RC_OK) {
            T_EG(STREAM_TRACE_GRP_API, "mesa_ingress_cnt_alloc() failed: %s", error_txt(rc));
            return VTSS_APPL_STREAM_RC_HW_RESOURCES;
        }

        T_IG(STREAM_TRACE_GRP_API, "mesa_ingress_cnt_alloc(cnt = 1) => cnt_id = %u", iflow_conf.cnt_id);

        // Enable the counter
        iflow_conf.cnt_enable = true;
    }

    // Allocate an IFLOW if not already allocated
    if (iflow_id == MESA_IFLOW_ID_NONE) {
        vce_changed = true;
        if ((rc = mesa_iflow_alloc(nullptr, &iflow_id)) != VTSS_RC_OK) {
            // Out of IFLOW IDs.
            T_EG(STREAM_TRACE_GRP_API, "mesa_iflow_alloc() failed: %s", error_txt(rc));
            return VTSS_APPL_STREAM_RC_HW_RESOURCES;
        }

        T_IG(STREAM_TRACE_GRP_API, "mesa_iflow_alloc() => flow_id = %u", iflow_id);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_mesa_iflow_and_ingress_cnt_free()
/******************************************************************************/
static void STREAM_mesa_iflow_and_ingress_cnt_free(mesa_iflow_conf_t &iflow_conf, mesa_iflow_id_t &iflow_id, bool &iflow_changed, bool &vce_changed)
{
    mesa_rc rc;

    // First, we free the IFLOW.
    if (iflow_id != MESA_IFLOW_ID_NONE) {
        vce_changed = true;

        T_IG(STREAM_TRACE_GRP_API, "mesa_iflow_free(%u)", iflow_id);
        if ((rc = mesa_iflow_free(nullptr, iflow_id)) != VTSS_RC_OK) {
            T_EG(STREAM_TRACE_GRP_API, "mesa_iflow_free(%u) failed: %s", iflow_id, error_txt(rc));
        }

        iflow_id = MESA_IFLOW_ID_NONE;
    }

    // Whether mesa_iflow_free() succeeded or not, we must also free the ingress
    // counter ID. We cannot use the cnt_id to determine whether a counter is
    // allocated, because the default cnt_id is 0, which is also a cnt_id we can
    // get from a call to mesa_ingress_cnt_alloc(), so we have to rely on
    // cnt_enable.
    if (iflow_conf.cnt_enable) {
        iflow_changed = true; // Doesn't really matter, since we've just deleted the IFLOW.

        T_IG(STREAM_TRACE_GRP_API, "mesa_ingress_cnt_free(%u)", iflow_conf.cnt_id);
        if ((rc = mesa_ingress_cnt_free(nullptr, iflow_conf.cnt_id)) != VTSS_RC_OK) {
            T_EG(STREAM_TRACE_GRP_API, "mesa_ingress_cnt_free(%u) failed: %s", iflow_conf.cnt_id, error_txt(rc));
        }

        iflow_conf.cnt_id     = STREAM_iflow_conf_default.cnt_id;
        iflow_conf.cnt_enable = false;
    }
}

/******************************************************************************/
// STREAM_clients_attached()
/******************************************************************************/
static bool STREAM_clients_attached(stream_state_t &stream_state)
{
    vtss_appl_stream_client_t client;

    for (client = (vtss_appl_stream_client_t)0; client < VTSS_APPL_STREAM_CLIENT_CNT; client++) {
        if (stream_state.status.client_status.clients[client].enable) {
            return true;
        }
    }

    return false;
}

/******************************************************************************/
// STREAM_collection_clients_attached()
/******************************************************************************/
static bool STREAM_collection_clients_attached(stream_collection_state_t &stream_collection_state)
{
    vtss_appl_stream_client_t client;

    for (client = (vtss_appl_stream_client_t)0; client < VTSS_APPL_STREAM_CLIENT_CNT; client++) {
        if (stream_collection_state.status.client_status.clients[client].enable) {
            return true;
        }
    }

    return false;
}

/******************************************************************************/
// STREAM_mesa_iflow_and_ingress_cnt_alloc_or_free()
/******************************************************************************/
static mesa_rc STREAM_mesa_iflow_and_ingress_cnt_alloc_or_free(stream_state_t &stream_state, bool &iflow_changed, bool &vce_changed)
{
    mesa_rc rc;

    if (STREAM_clients_attached(stream_state)) {
        // At least one client is attached. Make sure we have an IFLOW and an
        // ingress counter allocated.
        if ((rc = STREAM_mesa_iflow_and_ingress_cnt_alloc(stream_state.iflow_conf, stream_state.iflow_id, iflow_changed, vce_changed)) != VTSS_RC_OK) {
            STREAM_mesa_iflow_and_ingress_cnt_free(stream_state.iflow_conf, stream_state.iflow_id, iflow_changed, vce_changed);
        }
    } else {
        // All clients have abandoned this stream. Free IFLOW and ingress
        // counter if they are allocated.
        STREAM_mesa_iflow_and_ingress_cnt_free(stream_state.iflow_conf, stream_state.iflow_id, iflow_changed, vce_changed);

        // Freeing cannot fail.
        rc = VTSS_RC_OK;
    }

    return rc;
}

/******************************************************************************/
// STREAM_collection_mesa_iflow_and_ingress_cnt_alloc_or_free()
/******************************************************************************/
static mesa_rc STREAM_collection_mesa_iflow_and_ingress_cnt_alloc_or_free(stream_collection_state_t &stream_collection_state, bool &iflow_changed, bool &vce_changed)
{
    mesa_rc rc;

    if (STREAM_collection_clients_attached(stream_collection_state)) {
        // At least one client is attached. Make sure we have an IFLOW and an
        // ingress counter allocated.
        if ((rc = STREAM_mesa_iflow_and_ingress_cnt_alloc(stream_collection_state.iflow_conf, stream_collection_state.iflow_id, iflow_changed, vce_changed)) != VTSS_RC_OK) {
            STREAM_mesa_iflow_and_ingress_cnt_free(stream_collection_state.iflow_conf, stream_collection_state.iflow_id, iflow_changed, vce_changed);
        }
    } else {
        // All clients have abandoned this stream collection. Free IFLOW and
        // ingress counter if they are allocated.
        STREAM_mesa_iflow_and_ingress_cnt_free(stream_collection_state.iflow_conf, stream_collection_state.iflow_id, iflow_changed, vce_changed);

        // Freeing cannot fail.
        rc = VTSS_RC_OK;
    }

    return rc;
}

/******************************************************************************/
// STREAM_capabilities_set()
// This is the only place where we define maximum values for various parameters,
// so if you need different max. values, change here - only!
/******************************************************************************/
static void STREAM_capabilities_set(void)
{
    // I don't know where this limitation comes from.
    STREAM_cap.inst_cnt_max = 127;
}

/******************************************************************************/
// STREAM_collection_capabilities_set()
// This is the only place where we define maximum values for various parameters,
// so if you need different max. values, change here - only!
/******************************************************************************/
static void STREAM_collection_capabilities_set(void)
{
    // Let's limit the number of stream collections to half the number of
    // streams, because it doesn't make sense to have a stream collection with
    // only one stream in it.
    STREAM_collection_cap.inst_cnt_max = STREAM_cap.inst_cnt_max / 2;

    // Maximum number of streams per collection. Arbitrarily chosen.
    STREAM_collection_cap.streams_per_collection_max = VTSS_APPL_STREAM_COLLECTION_STREAM_CNT_MAX;
}

/******************************************************************************/
// STREAM_ptr_check()
/******************************************************************************/
static mesa_rc STREAM_ptr_check(const void *ptr)
{
    return ptr == nullptr ? (mesa_rc)VTSS_APPL_STREAM_RC_INVALID_PARAMETER : VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_id_check()
/******************************************************************************/
static mesa_rc STREAM_id_check(vtss_appl_stream_id_t id)
{
    if (id < 1 || id > STREAM_cap.inst_cnt_max) {
        return VTSS_APPL_STREAM_RC_INVALID_ID;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_collection_id_check()
/******************************************************************************/
static mesa_rc STREAM_collection_id_check(vtss_appl_stream_collection_id_t id)
{
    if (id < 1 || id > STREAM_collection_cap.inst_cnt_max) {
        return VTSS_APPL_STREAM_RC_COLLECTION_INVALID_ID;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_vce_type_check()
/******************************************************************************/
static mesa_rc STREAM_vce_type_check(mesa_vce_type_t type)
{
    if (type != MESA_VCE_TYPE_ANY   &&
        type != MESA_VCE_TYPE_ETYPE &&
        type != MESA_VCE_TYPE_LLC   &&
        type != MESA_VCE_TYPE_SNAP  &&
        type != MESA_VCE_TYPE_IPV4  &&
        type != MESA_VCE_TYPE_IPV6) {
        return VTSS_APPL_STREAM_RC_INVALID_PROTOCOL_TYPE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_client_check()
/******************************************************************************/
static mesa_rc STREAM_client_check(vtss_appl_stream_client_t client)
{
    if (client != VTSS_APPL_STREAM_CLIENT_PSFP && client != VTSS_APPL_STREAM_CLIENT_FRER) {
        return VTSS_APPL_STREAM_RC_INVALID_CLIENT;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_collection_counters_get()
/******************************************************************************/
static mesa_rc STREAM_collection_counters_get(vtss_appl_stream_collection_id_t stream_collection_id, mesa_ingress_counters_t &counters)
{
    stream_collection_itr_t stream_collection_itr;

    if ((stream_collection_itr = STREAM_collection_map.find(stream_collection_id)) == STREAM_collection_map.end()) {
        return VTSS_APPL_STREAM_RC_COLLECTION_NO_SUCH_ID;
    }

    if (!stream_collection_itr->second.iflow_conf.cnt_enable) {
        return VTSS_APPL_STREAM_RC_COLLECTION_COUNTERS_NOT_ALLOCATED;
    }

    return mesa_ingress_cnt_get(nullptr, stream_collection_itr->second.iflow_conf.cnt_id, 0, &counters);
}

/******************************************************************************/
// STREAM_collection_counters_clear()
/******************************************************************************/
static mesa_rc STREAM_collection_counters_clear(vtss_appl_stream_collection_id_t stream_collection_id)
{
    stream_collection_itr_t stream_collection_itr;

    if ((stream_collection_itr = STREAM_collection_map.find(stream_collection_id)) == STREAM_collection_map.end()) {
        return VTSS_APPL_STREAM_RC_COLLECTION_NO_SUCH_ID;
    }

    if (!stream_collection_itr->second.iflow_conf.cnt_enable) {
        return VTSS_APPL_STREAM_RC_COLLECTION_COUNTERS_NOT_ALLOCATED;
    }

    return mesa_ingress_cnt_clear(nullptr, stream_collection_itr->second.iflow_conf.cnt_id, 0);
}

/******************************************************************************/
// STREAM_counters_get()
/******************************************************************************/
static mesa_rc STREAM_counters_get(vtss_appl_stream_id_t stream_id, mesa_ingress_counters_t &counters)
{
    stream_itr_t stream_itr;

    if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
        return VTSS_APPL_STREAM_RC_NO_SUCH_ID;
    }

    if (!stream_itr->second.iflow_conf.cnt_enable) {
        return VTSS_APPL_STREAM_RC_COUNTERS_NOT_ALLOCATED;
    }

    return mesa_ingress_cnt_get(nullptr, stream_itr->second.iflow_conf.cnt_id, 0, &counters);
}

/******************************************************************************/
// STREAM_counters_clear()
/******************************************************************************/
static mesa_rc STREAM_counters_clear(vtss_appl_stream_id_t stream_id)
{
    stream_itr_t stream_itr;

    if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
        return VTSS_APPL_STREAM_RC_NO_SUCH_ID;
    }

    // If this stream is part of a stream collection, clear the stream
    // collection's counters.
    if (stream_itr->second.status.stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        return STREAM_collection_counters_clear(stream_itr->second.status.stream_collection_id);
    }

    if (!stream_itr->second.iflow_conf.cnt_enable) {
        return VTSS_APPL_STREAM_RC_COUNTERS_NOT_ALLOCATED;
    }

    return mesa_ingress_cnt_clear(nullptr, stream_itr->second.iflow_conf.cnt_id, 0);
}

/******************************************************************************/
// STREAM_id_to_vce_id()
/******************************************************************************/
static mesa_vce_id_t STREAM_id_to_vce_id(vtss_appl_stream_id_t stream_id)
{
    return stream_id == VTSS_APPL_STREAM_ID_NONE ? MESA_VCE_ID_LAST : (VCL_TYPE_STREAM << 16) + stream_id;
}

/******************************************************************************/
// STREAM_vce_insertion_point_get()
/******************************************************************************/
static mesa_vce_id_t STREAM_vce_insertion_point_get(vtss_appl_stream_id_t stream_id)
{
    stream_itr_t stream_itr;

    // Search for the next installed stream. There is a one-to-one
    // correspondance between stream_id and VCE ID.

    // Get an iterator to ourselves.
    if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
        T_E("%u. Why can't we find ourselves in the map?", stream_id);
        return MESA_VCE_ID_LAST;
    }

    // Look at the subsequence streams.
    while (++stream_itr != STREAM_map.end()) {
        // A stream may be defined, but not installed in MESA. In that case we
        // cannot use the stream's VCE ID.
        if (stream_itr->second.vce.id != STREAM_VCE_ID_NONE) {
            return stream_itr->second.vce.id;
        }
    }

    // No more streams. Add ourselves last.
    return MESA_VCE_ID_LAST;
}

/******************************************************************************/
// STREAM_tag_appl_to_mesa()
/******************************************************************************/
static void STREAM_tag_appl_to_mesa(const vtss_appl_stream_vlan_tag_t &a, mesa_vce_tag_t &m)
{
    m.vid.value = a.vid_value;
    m.vid.mask  = a.vid_mask;
    m.pcp.value = a.pcp_value;
    m.pcp.mask  = a.pcp_mask;
    m.dei       = a.dei;
    m.tagged    = a.match_type == VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_BOTH ? MESA_VCAP_BIT_ANY : a.match_type == VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED  ? MESA_VCAP_BIT_1 : MESA_VCAP_BIT_0;
    m.s_tag     = a.tag_type   == VTSS_APPL_STREAM_VLAN_TAG_TYPE_ANY        ? MESA_VCAP_BIT_ANY : a.tag_type   == VTSS_APPL_STREAM_VLAN_TAG_TYPE_C_TAGGED      ? MESA_VCAP_BIT_0 : MESA_VCAP_BIT_1;
}

/******************************************************************************/
// STREAM_mesa_vce_del()
/******************************************************************************/
static mesa_rc STREAM_mesa_vce_del(stream_state_t &stream_state)
{
    mesa_rc rc;

    if (stream_state.vce.id == STREAM_VCE_ID_NONE) {
        // It is not currently added.
        return VTSS_RC_OK;
    }

    T_IG(STREAM_TRACE_GRP_API, "%u: mesa_vce_del(0x%x)", stream_state.stream_id, stream_state.vce.id);
    if ((rc = mesa_vce_del(nullptr, stream_state.vce.id)) != MESA_RC_OK) {
        T_EG(STREAM_TRACE_GRP_API, "%u: mesa_vce_del(0x%x) failed: %s", stream_state.stream_id, stream_state.vce.id, error_txt(rc));
    }

    stream_state.vce.id = STREAM_VCE_ID_NONE;
    return rc;
}

/******************************************************************************/
// STREAM_mesa_vce_update()
/******************************************************************************/
static mesa_rc STREAM_mesa_vce_update(stream_state_t &stream_state, mesa_iflow_id_t iflow_id)
{
    mesa_vce_id_t insertion_point;
    mesa_rc       rc;

    stream_state.vce.action.flow_id = iflow_id;
    stream_state.vce.id = STREAM_id_to_vce_id(stream_state.stream_id);
    insertion_point = STREAM_vce_insertion_point_get(stream_state.stream_id);

    T_IG(STREAM_TRACE_GRP_API, "%u: mesa_vce_add(vce.id = 0x%x, vce = %s)", stream_state.stream_id, insertion_point, stream_state.vce);
    if ((rc = mesa_vce_add(NULL, insertion_point, &stream_state.vce)) != VTSS_RC_OK) {
        T_EG(STREAM_TRACE_GRP_API, "%u: mesa_vce_add(vce.id = 0x%x, conf = %s) failed: %s", stream_state.stream_id, insertion_point, stream_state.vce, error_txt(rc));
        stream_state.vce.id = STREAM_VCE_ID_NONE;
        return rc;
    }

    return MESA_RC_OK;
}

/******************************************************************************/
// STREAM_oper_warnings_update()
/******************************************************************************/
static void STREAM_oper_warnings_update(stream_state_t &stream_state)
{
    stream_state.status.oper_warnings &= ~VTSS_APPL_STREAM_OPER_WARNING_NOT_INSTALLED_ON_ANY_PORT;

    if (stream_state.conf.port_list.is_empty()) {
        stream_state.status.oper_warnings |= VTSS_APPL_STREAM_OPER_WARNING_NOT_INSTALLED_ON_ANY_PORT;
        T_I("%u: Not installed on any port", stream_state.stream_id);
    }
}

/******************************************************************************/
// STREAM_collection_oper_warnings_update()
/******************************************************************************/
static void STREAM_collection_oper_warnings_update(stream_collection_state_t &stream_collection_state)
{
    vtss_appl_stream_id_t stream_id;
    stream_itr_t          stream_itr;
    int                   i;

    stream_collection_state.status.oper_warnings &= ~VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NO_STREAMS_ATTACHED;
    stream_collection_state.status.oper_warnings &= ~VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NO_CLIENTS_ATTACHED;
    stream_collection_state.status.oper_warnings &= ~VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_AT_LEAST_ONE_STREAM_HAS_OPERATIONAL_WARNINGS;

    if (stream_collection_state.conf.stream_ids[0] == VTSS_APPL_STREAM_ID_NONE) {
        stream_collection_state.status.oper_warnings |= VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NO_STREAMS_ATTACHED;
        T_IG(STREAM_TRACE_GRP_COLLECTION, "%u: No streams attached", stream_collection_state.stream_collection_id);
    }

    if (!STREAM_collection_clients_attached(stream_collection_state)) {
        stream_collection_state.status.oper_warnings |= VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NO_CLIENTS_ATTACHED;
        T_IG(STREAM_TRACE_GRP_COLLECTION, "%u: No clients attached", stream_collection_state.stream_collection_id);
    }

    for (i = 0; i < ARRSZ(stream_collection_state.conf.stream_ids); i++) {
        stream_id = stream_collection_state.conf.stream_ids[i];

        if (stream_id == VTSS_APPL_STREAM_ID_NONE) {
            // End of list
            break;
        }

        if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
            T_E("%u: Unable to find stream_id = %u", stream_collection_state.stream_collection_id, stream_id);
            continue;
        }

        if (stream_itr->second.status.oper_warnings) {
            stream_collection_state.status.oper_warnings |= VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_AT_LEAST_ONE_STREAM_HAS_OPERATIONAL_WARNINGS;
            T_IG(STREAM_TRACE_GRP_COLLECTION, "%u: Stream %u has operational warnings", stream_collection_state.stream_collection_id, stream_itr->second.stream_id);
            break;
        }
    }
}

/******************************************************************************/
// STREAM_notif_table_update()
/******************************************************************************/
static void STREAM_notif_table_update(vtss_appl_stream_id_t stream_id, stream_notif_status_t status)
{
    uint32_t cur_cnt;

    switch (status) {
    case STREAM_NOTIF_STATUS_ADD:
        T_IG(STREAM_TRACE_GRP_NOTIF, "Stream ID #%u: Adding", stream_id);
        stream_notif_table.set(stream_id, 0);
        break;

    case STREAM_NOTIF_STATUS_MOD:
        T_IG(STREAM_TRACE_GRP_NOTIF, "Stream ID #%u: Modifying", stream_id);
        cur_cnt = 0;
        (void)stream_notif_table.get(stream_id, &cur_cnt);
        stream_notif_table.set(stream_id, cur_cnt + 1);
        break;

    case STREAM_NOTIF_STATUS_DEL:
    default:
        T_IG(STREAM_TRACE_GRP_NOTIF, "Stream ID #%u: Deleting", stream_id);
        stream_notif_table.del(stream_id);
        break;
    }
}

/******************************************************************************/
// STREAM_collection_notif_table_update()
/******************************************************************************/
static void STREAM_collection_notif_table_update(vtss_appl_stream_collection_id_t stream_collection_id, stream_notif_status_t status)
{
    uint32_t cur_cnt;

    switch (status) {
    case STREAM_NOTIF_STATUS_ADD:
        T_IG(STREAM_TRACE_GRP_NOTIF, "Stream Collection ID #%u: Adding", stream_collection_id);
        stream_collection_notif_table.set(stream_collection_id, 0);
        break;

    case STREAM_NOTIF_STATUS_MOD:
        T_IG(STREAM_TRACE_GRP_NOTIF, "Stream Collection ID #%u: Modifying", stream_collection_id);
        cur_cnt = 0;
        (void)stream_collection_notif_table.get(stream_collection_id, &cur_cnt);
        stream_collection_notif_table.set(stream_collection_id, cur_cnt + 1);
        break;

    case STREAM_NOTIF_STATUS_DEL:
    default:
        T_IG(STREAM_TRACE_GRP_NOTIF, "Stream Collection ID #%u: Deleting", stream_collection_id);
        stream_collection_notif_table.del(stream_collection_id);
        break;
    }
}

/******************************************************************************/
// STREAM_collection_detach_stream()
// It's up to the caller to possibly remove this stream from it's conf.
/******************************************************************************/
static void STREAM_collection_detach_stream(stream_collection_state_t &stream_collection_state, stream_state_t &stream_state)
{
    T_I("%u: Detaching stream %u", stream_collection_state.stream_collection_id, stream_state.stream_id);

    // Update the VCE with no IFLOW
    (void)STREAM_mesa_vce_update(stream_state, MESA_IFLOW_ID_NONE);

    // There's no IFLOW to free, because that's the one owned by this stream
    // collection.
    if (stream_state.iflow_id != MESA_IFLOW_ID_NONE) {
        T_E("Stream %u is removed from collection %u, but has a non-default IFLOW ID (%u)", stream_state.stream_id, stream_collection_state.stream_collection_id, stream_state.iflow_id);
    }

    // The stream already doesn't have any clients attached on its own.

    // Reset the owning stream collection
    stream_state.status.stream_collection_id = VTSS_APPL_STREAM_COLLECTION_ID_NONE;

    // Update the stream's operational warnings.
    STREAM_oper_warnings_update(stream_state);

    // Notify observers of this stream that it is now ready to get clients
    // attached on its own.
    STREAM_notif_table_update(stream_state.stream_id, STREAM_NOTIF_STATUS_MOD);
}

/******************************************************************************/
// STREAM_collection_attach_stream()
/******************************************************************************/
static void STREAM_collection_attach_stream(stream_collection_state_t &stream_collection_state, stream_state_t &stream_state)
{
    T_I("%u: Attaching stream %u", stream_collection_state.stream_collection_id, stream_state.stream_id);

    // Make the stream part of this stream collection
    stream_state.status.stream_collection_id = stream_collection_state.stream_collection_id;

    // Apply the stream collection's IFLOW to the stream's VCE and update.
    (void)STREAM_mesa_vce_update(stream_state, stream_collection_state.iflow_id);

    // Update the stream's operational warnings.
    STREAM_oper_warnings_update(stream_state);

    // Notify observers of this stream that something has changed.
    STREAM_notif_table_update(stream_state.stream_id, STREAM_NOTIF_STATUS_MOD);
}

//*****************************************************************************/
// STREAM_collection_stream_ids_sort()
//*****************************************************************************/
static int STREAM_collection_stream_ids_sort(const void *a_, const void *b_)
{
    vtss_appl_stream_id_t a = *(vtss_appl_stream_id_t *)a_, b = *(vtss_appl_stream_id_t *)b_;

    if (a == b) {
        return 0;
    }

    // Make sure that VTSS_APPL_STREAM_ID_NONE entries come last.
    if (a == VTSS_APPL_STREAM_ID_NONE) {
        return 1;
    }

    if (b == VTSS_APPL_STREAM_ID_NONE) {
        return -1;
    }

    return a < b ? -1 : 1;
}

/******************************************************************************/
// STREAM_collection_stream_ids_normalize()
/******************************************************************************/
static void STREAM_collection_stream_ids_normalize(vtss_appl_stream_id_t *arr, size_t cnt)
{
    int i, j;

    // First we need to remove duplicates from the array.
    for (i = 0; i < cnt - 1; i++) {
        if (arr[i] == VTSS_APPL_STREAM_ID_NONE) {
            continue;
        }

        for (j = i + 1; j < cnt; j++) {
            if (arr[j] == arr[i]) {
                // Duplicate. Remove it.
                arr[j] = VTSS_APPL_STREAM_ID_NONE;
            }
        }
    }

    // Then sort the array.
    qsort(arr, cnt, sizeof(arr[0]), STREAM_collection_stream_ids_sort);
}

/******************************************************************************/
// STREAM_collection_detach_stream_and_consolidate()
/******************************************************************************/
static void STREAM_collection_detach_stream_and_consolidate(stream_collection_state_t &stream_collection_state, stream_state_t &stream_state)
{
    vtss_appl_stream_id_t stream_id;
    int                   i;

    // Search for the stream in the stream collection's configuration.
    for (i = 0; i < ARRSZ(stream_collection_state.conf.stream_ids); i++) {
        stream_id = stream_collection_state.conf.stream_ids[i];

        if (stream_id == VTSS_APPL_STREAM_ID_NONE) {
            // The end is reached, but we haven't found the stream ID in the
            // collection's configuration. That's a code bug.
            T_E("%u: Unable to find stream %u in stream collection.", stream_collection_state.stream_collection_id, stream_state.stream_id);
            return;
        }

        if (stream_id == stream_state.stream_id) {
            // Got it at index i.
            break;
        }
    }

    if (i == ARRSZ(stream_collection_state.conf.stream_ids)) {
        // The end is reached, but we haven't found the stream ID in the
        // collection's configuration. That's a code bug.
        T_E("%u: Unable to find stream %u in stream collection.", stream_collection_state.stream_collection_id, stream_state.stream_id);
        return;
    }

    // Do detach the stream from this collection
    STREAM_collection_detach_stream(stream_collection_state, stream_state);

    // The stream is no longer part of this stream collection.
    stream_collection_state.conf.stream_ids[i] = VTSS_APPL_STREAM_ID_NONE;

    // Consolidate the stream configuration, so that unused streams come last.
    STREAM_collection_stream_ids_normalize(stream_collection_state.conf.stream_ids, ARRSZ(stream_collection_state.conf.stream_ids));

    // Even if there are no more streams attached to this stream collection, we
    // should not free IFLOW/Counter ID, because there might be clients
    // attached. The stream collections IFLOW and counter are allocated by the
    // attachments of clients, and deallocated only when all clients are
    // detached or the stream collection itself gets deleted.

    // Update the stream collection's operational warnings.
    STREAM_collection_oper_warnings_update(stream_collection_state);

    // And issue a notification on the stream collection
    STREAM_collection_notif_table_update(stream_collection_state.stream_collection_id, STREAM_NOTIF_STATUS_MOD);
}

/******************************************************************************/
// STREAM_free_resources()
/******************************************************************************/
static void STREAM_free_resources(stream_state_t &stream_state)
{
    bool iflow_changed = false, vce_changed = false; // Unused

    // Free the VCE, if applied to MESA.
    (void)STREAM_mesa_vce_del(stream_state);

    // And free the IFLOW and ingress counter if allocated.
    STREAM_mesa_iflow_and_ingress_cnt_free(stream_state.iflow_conf, stream_state.iflow_id, iflow_changed, vce_changed);

    // No clients attached to this stream anymore
    vtss_clear(stream_state.status.client_status);

    // Notify them
    STREAM_notif_table_update(stream_state.stream_id, STREAM_NOTIF_STATUS_MOD);

    // And update operational warnings
    STREAM_oper_warnings_update(stream_state);
}

/******************************************************************************/
// STREAM_conf_del()
/******************************************************************************/
static void STREAM_conf_del(stream_state_t &stream_state)
{
    stream_collection_itr_t stream_collection_itr;

    STREAM_free_resources(stream_state);

    // If the stream is part of a stream collection, we also need to remove it
    // from that.
    if (stream_state.status.stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        if ((stream_collection_itr = STREAM_collection_map.find(stream_state.status.stream_collection_id)) == STREAM_collection_map.end()) {
            T_E("%u: Unable to find stream collection %u", stream_state.stream_id, stream_state.status.stream_collection_id);
        } else {
            STREAM_collection_detach_stream_and_consolidate(stream_collection_itr->second, stream_state);
        }
    }

    // Let observers know that the stream no longer exists.
    STREAM_notif_table_update(stream_state.stream_id, STREAM_NOTIF_STATUS_DEL);

    // Delete the instance from our map
    STREAM_map.erase(stream_state.stream_id);
}

/******************************************************************************/
// STREAM_collection_conf_del()
/******************************************************************************/
static void STREAM_collection_conf_del(stream_collection_state_t &stream_collection_state)
{
    stream_itr_t          stream_itr;
    vtss_appl_stream_id_t stream_id;
    bool                  iflow_changed = false, vce_changed = false; // Unused.
    int                   i;

    // First detach all streams from this collection
    for (i = 0; i < ARRSZ(stream_collection_state.conf.stream_ids); i++) {
        stream_id = stream_collection_state.conf.stream_ids[i];

        if (stream_id == VTSS_APPL_STREAM_ID_NONE) {
            break;
        }

        if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
            T_E("%u: Cannot find stream %u", stream_collection_state.stream_collection_id, stream_id);
            continue;
        }

        STREAM_collection_detach_stream(stream_collection_state, stream_itr->second);
    }

    // Free the stream collection's IFLOW and ingress counter
    STREAM_mesa_iflow_and_ingress_cnt_free(stream_collection_state.iflow_conf, stream_collection_state.iflow_id, iflow_changed, vce_changed);

    // Let observers know that the stream collection no longer exists.
    STREAM_collection_notif_table_update(stream_collection_state.stream_collection_id, STREAM_NOTIF_STATUS_DEL);

    // Delete the instance from our map
    STREAM_collection_map.erase(stream_collection_state.stream_collection_id);
}

/******************************************************************************/
// STREAM_appl_to_mesa_range()
/******************************************************************************/
static void STREAM_appl_to_mesa_range(mesa_vcap_vr_t &m, vtss_appl_stream_range_t &a)
{
    if (a.match_type != VTSS_APPL_STREAM_RANGE_MATCH_TYPE_ANY) {
        // a.low and a.high are already normalized in case match_type is VALUE.
        m.type   = MESA_VCAP_VR_TYPE_RANGE_INCLUSIVE;
        m.vr.r.low  = a.low;
        m.vr.r.high = a.high;
    } else {
        // The default already is not to match.
    }
}

/******************************************************************************/
// STREAM_vce_key_update()
/******************************************************************************/
static void STREAM_vce_key_update(stream_state_t &stream_state)
{
    vtss_appl_stream_conf_t &c = stream_state.conf;
    mesa_vce_key_t          &k = stream_state.vce.key;
    mesa_ipv6_t             ipv6_mask;
    mesa_vce_t              temp_vce;
    mesa_rc                 rc;

    // We need a temporary ECE, because if c.protocol.type changes, the VCE's
    // key also changes defaults, so we need to call mesa_vce_init() every time.
    T_IG(STREAM_TRACE_GRP_API, "%u: mesa_vce_init(%u)", stream_state.stream_id, c.protocol.type);
    if ((rc = mesa_vce_init(nullptr,  c.protocol.type, &temp_vce)) != VTSS_RC_OK) {
        T_EG(STREAM_TRACE_GRP_API, "%u: mesa_vce_init(%u) failed: %s", stream_state.stream_id, c.protocol.type, error_txt(rc));
        return;
    }

    stream_state.vce.key = temp_vce.key;

    k.port_list = c.port_list;

    // =-----------------------------------------------=
    // | MC    | BC    || Match        | Name          |
    // |-------|-------||------------------------------|
    // | Any   | Any   || UC + MC + BC | any           |
    // | Any   | Clear || UC + MC      | not-broadcast |
    // | Any   | Set   ||           BC | broadcast     |
    // | Clear | Any   || UC           | unicast       |
    // | Clear | Clear || UC           | unicast       |
    // | Clear | Set   || INVALID      |               |
    // | Set   | Any   ||      MC + BC | not-unicast   |
    // | Set   | Clear ||      MC      | multicast     |
    // | Set   | Set   ||           BC | broadcast     |
    // =-----------------------------------------------=
    switch (c.dmac.match_type) {
    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_ANY:
        k.mac.dmac_mc = MESA_VCAP_BIT_ANY;
        k.mac.dmac_bc = MESA_VCAP_BIT_ANY;
        break;

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_MC:
        k.mac.dmac_mc = MESA_VCAP_BIT_1;
        k.mac.dmac_bc = MESA_VCAP_BIT_0;
        break;

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_BC:
        k.mac.dmac_mc = MESA_VCAP_BIT_1;
        k.mac.dmac_bc = MESA_VCAP_BIT_1;
        break;

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_UC:
        k.mac.dmac_mc = MESA_VCAP_BIT_0;
        k.mac.dmac_bc = MESA_VCAP_BIT_0;
        break;

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_NOT_BC:
        k.mac.dmac_mc = MESA_VCAP_BIT_ANY;
        k.mac.dmac_bc = MESA_VCAP_BIT_0;
        break;

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_NOT_UC:
        k.mac.dmac_mc = MESA_VCAP_BIT_1;
        k.mac.dmac_bc = MESA_VCAP_BIT_ANY;
        break;

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_VALUE:
        memcpy(k.mac.dmac.value, &c.dmac.value, sizeof(k.mac.dmac.value));
        memcpy(k.mac.dmac.mask,  &c.dmac.mask,  sizeof(k.mac.dmac.mask));
        break;

    default:
        T_E("Invalid dmac.match_type (%d)", c.dmac.match_type);
        return;
    }

    memcpy(k.mac.smac.value, &c.smac.value, sizeof(k.mac.smac.value));
    memcpy(k.mac.smac.mask,  &c.smac.mask, sizeof(k.mac.smac.mask));

    STREAM_tag_appl_to_mesa(c.outer_tag, k.tag);
    STREAM_tag_appl_to_mesa(c.inner_tag, k.inner_tag);

    switch (k.type) {
    case MESA_VCE_TYPE_ANY:
        break;

    case MESA_VCE_TYPE_ETYPE: {
        mesa_etype_t    &a = c.protocol.value.etype.etype;
        mesa_vcap_u16_t &m = k.frame.etype.etype;

        m.value[0] = (a >> 8) & 0xff;
        m.value[1] = (a >> 0) & 0xff;
        m.mask[0]  = 0xff;
        m.mask[1]  = 0xff;
        break;
    }

    case MESA_VCE_TYPE_LLC: {
        vtss_appl_stream_proto_llc_t &a = c.protocol.value.llc;
        mesa_vcap_u48_t              &m = k.frame.llc.data;

        m.value[0] = a.dsap;
        m.value[1] = a.ssap;
        m.mask[0]  = 0xFF;
        m.mask[1]  = 0xFF;
        break;
    }

    case MESA_VCE_TYPE_SNAP: {
        vtss_appl_stream_proto_snap_t &a = c.protocol.value.snap;
        mesa_vcap_u48_t               &m = k.frame.snap.data;

        m.value[0] = (a.oui >> 16) & 0xFF;
        m.value[1] = (a.oui >>  8) & 0xFF;
        m.value[2] = (a.oui >>  0) & 0xFF;
        m.value[3] = (a.pid >>  8) & 0xFF;
        m.value[4] = (a.pid >>  0) & 0xFF;
        m.mask[0]  = 0xFF;
        m.mask[1]  = 0xFF;
        m.mask[2]  = 0xFF;
        m.mask[3]  = 0xFF;
        m.mask[4]  = 0xFF;
        break;
    }

    case MESA_VCE_TYPE_IPV4: {
        vtss_appl_stream_proto_ipv4_t &a = c.protocol.value.ipv4;
        mesa_vce_frame_ipv4_t         &m = k.frame.ipv4;

        // fragment
        m.fragment = a.fragment;

        // options not settable from application

        // dscp
        STREAM_appl_to_mesa_range(m.dscp, a.dscp);

        // proto
        if (a.proto.type != VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_ANY) {
            m.proto.value = a.proto.value;
            m.proto.mask  = 0xFF;
        }

        // sip
        m.sip.value = a.sip.address;
        m.sip.mask  = vtss_ipv4_prefix_to_mask(a.sip.prefix_size);

        // dip
        m.dip.value = a.dip.address;
        m.dip.mask  = vtss_ipv4_prefix_to_mask(a.dip.prefix_size);

        // sport not settable from application

        // dport
        STREAM_appl_to_mesa_range(m.dport, a.dport);
        break;
    }

    case MESA_VCE_TYPE_IPV6: {
        vtss_appl_stream_proto_ipv6_t &a = c.protocol.value.ipv6;
        mesa_vce_frame_ipv6_t         &m = k.frame.ipv6;

        // dscp
        STREAM_appl_to_mesa_range(m.dscp, a.dscp);

        // proto
        if (a.proto.type != VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_ANY) {
            m.proto.value = a.proto.value;
            m.proto.mask  = 0xFF;
        }

        // sip
        memcpy(m.sip.value, a.sip.address.addr, sizeof(m.sip.value));
        (void)vtss_conv_prefix_to_ipv6mask(a.sip.prefix_size, &ipv6_mask);
        memcpy(m.sip.mask, ipv6_mask.addr, sizeof(m.sip.mask));

        // dip
        memcpy(m.dip.value, a.dip.address.addr, sizeof(m.dip.value));
        (void)vtss_conv_prefix_to_ipv6mask(a.dip.prefix_size, &ipv6_mask);
        memcpy(m.dip.mask, ipv6_mask.addr, sizeof(m.dip.mask));

        // sport not settable from application

        // dport
        STREAM_appl_to_mesa_range(m.dport, a.dport);
        break;
    }

    default:
        T_E("Unexpected key type: %d", k.type);
    }
}

/******************************************************************************/
// STREAM_mesa_iflow_conf_set()
/******************************************************************************/
static mesa_rc STREAM_mesa_iflow_conf_set(mesa_iflow_id_t iflow_id, mesa_iflow_conf_t &iflow_conf)
{
    mesa_rc rc;

    if (iflow_id == MESA_IFLOW_ID_NONE) {
        // An IFLOW is not allocated. Nothing to do.
        return VTSS_RC_OK;
    }

    T_IG(STREAM_TRACE_GRP_API, "mesa_iflow_conf_set(flow_id = %u, conf = %s)", iflow_id, iflow_conf);
    if ((rc = mesa_iflow_conf_set(nullptr, iflow_id, &iflow_conf)) != VTSS_RC_OK) {
        T_EG(STREAM_TRACE_GRP_API, "mesa_iflow_conf_set(flow_id = %u, conf = %s) failed: %s", iflow_id, iflow_conf, error_txt(rc));
    }

    return rc;
}

/******************************************************************************/
// STREAM_vcap_bit_validate()
/******************************************************************************/
static mesa_rc STREAM_vcap_bit_validate(mesa_vcap_bit_t b, mesa_rc rc)
{
    if (b != MESA_VCAP_BIT_ANY && b != MESA_VCAP_BIT_0 && b != MESA_VCAP_BIT_1) {
        return rc;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_vlan_tag_conf_normalize_and_validate()
/******************************************************************************/
static mesa_rc STREAM_vlan_tag_conf_normalize_and_validate(vtss_appl_stream_vlan_tag_t &c, bool outer_tag)
{
    vtss_appl_stream_vlan_tag_match_type_t match_type = c.match_type;

    if (match_type == VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_BOTH || match_type == VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_UNTAGGED) {
        // Accept both tagged and untagged frames or only untagged.
        // In either case, none of the VLAN-specific parameters are used, so
        // clear them.
        vtss_clear(c);

        // And restore 'match_type'
        c.match_type = match_type;
        return VTSS_RC_OK;
    }

    if (match_type != VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED) {
        return outer_tag ? VTSS_APPL_STREAM_RC_INVALID_VLAN_MATCH_TYPE_OUTER_TAG : VTSS_APPL_STREAM_RC_INVALID_VLAN_MATCH_TYPE_INNER_TAG;
    }

    if (c.tag_type != VTSS_APPL_STREAM_VLAN_TAG_TYPE_ANY      &&
        c.tag_type != VTSS_APPL_STREAM_VLAN_TAG_TYPE_C_TAGGED &&
        c.tag_type != VTSS_APPL_STREAM_VLAN_TAG_TYPE_S_TAGGED) {
        return outer_tag ? VTSS_APPL_STREAM_RC_INVALID_TAG_TYPE_OUTER_TAG : VTSS_APPL_STREAM_RC_INVALID_TAG_TYPE_INNER_TAG;
    }

    if (c.vid_value > VTSS_APPL_VLAN_ID_MAX) {
        return outer_tag ? VTSS_APPL_STREAM_RC_INVALID_VID_VALUE_OUTER_TAG : VTSS_APPL_STREAM_RC_INVALID_VID_VALUE_INNER_TAG;
    }

    if (c.vid_mask > VTSS_APPL_VLAN_ID_MAX) {
        return outer_tag ? VTSS_APPL_STREAM_RC_INVALID_VID_MASK_OUTER_TAG : VTSS_APPL_STREAM_RC_INVALID_VID_MASK_INNER_TAG;
    }

    if (c.pcp_value > 7) {
        return outer_tag ? VTSS_APPL_STREAM_RC_INVALID_PCP_VALUE_OUTER_TAG : VTSS_APPL_STREAM_RC_INVALID_PCP_VALUE_INNER_TAG;
    }

    if (c.pcp_mask > 7) {
        return outer_tag ? VTSS_APPL_STREAM_RC_INVALID_PCP_MASK_OUTER_TAG : VTSS_APPL_STREAM_RC_INVALID_PCP_MASK_INNER_TAG;
    }

    VTSS_RC(STREAM_vcap_bit_validate(c.dei, outer_tag ? VTSS_APPL_STREAM_RC_INVALID_DEI_OUTER_TAG : VTSS_APPL_STREAM_RC_INVALID_DEI_INNER_TAG));

    // Set XXX_value = XXX_value & XXX_mask, to indicate to the user what is
    // actually mached against. MESA does this before applying to H/W anyway, or
    // we could end up never being able to match the VCE.
    c.vid_value &= c.vid_mask;
    c.pcp_value &= c.pcp_mask;

    return VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_dscp_normalize_and_validate()
/******************************************************************************/
static mesa_rc STREAM_dscp_normalize_and_validate(vtss_appl_stream_range_t &dscp, bool is_ipv4)
{
    switch (dscp.match_type) {
    case VTSS_APPL_STREAM_RANGE_MATCH_TYPE_ANY:
        // Normalize
        dscp.high = 0;
        dscp.low  = 0;
        break;

    case VTSS_APPL_STREAM_RANGE_MATCH_TYPE_VALUE:
        if (dscp.low > 63) {
            return is_ipv4 ? VTSS_APPL_STREAM_RC_IPV4_DSCP_OUT_OF_RANGE : VTSS_APPL_STREAM_RC_IPV6_DSCP_OUT_OF_RANGE;
        }

        // Normalize
        dscp.high = dscp.low;
        break;

    case VTSS_APPL_STREAM_RANGE_MATCH_TYPE_RANGE:
        if (dscp.low > 63) {
            return is_ipv4 ? VTSS_APPL_STREAM_RC_IPV4_DSCP_LOW_OUT_OF_RANGE : VTSS_APPL_STREAM_RC_IPV6_DSCP_LOW_OUT_OF_RANGE;
        }

        if (dscp.high > 63) {
            return is_ipv4 ? VTSS_APPL_STREAM_RC_IPV4_DSCP_HIGH_OUT_OF_RANGE : VTSS_APPL_STREAM_RC_IPV6_DSCP_HIGH_OUT_OF_RANGE;
        }

        if (dscp.high < dscp.low) {
            return is_ipv4 ? VTSS_APPL_STREAM_RC_IPV4_DSCP_HIGH_SMALLER_THAN_LOW : VTSS_APPL_STREAM_RC_IPV6_DSCP_HIGH_SMALLER_THAN_LOW;
        }

        // Change match type if low == high
        if (dscp.low == dscp.high) {
            dscp.match_type = VTSS_APPL_STREAM_RANGE_MATCH_TYPE_VALUE;
        }

        break;

    default:
        return is_ipv4 ? VTSS_APPL_STREAM_RC_IPV4_DSCP_MATCH_TYPE : VTSS_APPL_STREAM_RC_IPV6_DSCP_MATCH_TYPE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_ip_proto_normalize_and_validate()
/******************************************************************************/
static mesa_rc STREAM_ip_proto_normalize_and_validate(vtss_appl_stream_ip_protocol_t &proto, bool is_ipv4)
{
    switch (proto.type) {
    case VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_ANY:
        // Normalize the value.
        proto.value = 0;
        break;

    case VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_CUSTOM:
        // Transform the type to another if it's one of the others.
        if (proto.value == STREAM_IP_PROTO_TCP) {
            proto.type = VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_TCP;
        } else if (proto.value == STREAM_IP_PROTO_UDP) {
            proto.type = VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_UDP;
        }

        break;

    case VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_TCP:
        // Transform the proto_value to TCP
        proto.value = STREAM_IP_PROTO_TCP;
        break;

    case VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_UDP:
        proto.value = STREAM_IP_PROTO_UDP;
        break;

    default:
        return is_ipv4 ? VTSS_APPL_STREAM_RC_INVALID_IPV4_PROTO_TYPE : VTSS_APPL_STREAM_RC_INVALID_IPV6_PROTO_TYPE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_dport_normalize_and_validate()
/******************************************************************************/
static mesa_rc STREAM_dport_normalize_and_validate(vtss_appl_stream_range_t &dport, const vtss_appl_stream_ip_protocol_t &proto, bool is_ipv4)
{
    // If protocol is not UDP or TCP, silently change the dport matching to ANY
    // for backwards compatibility, because that doesn't make sense.
    if (proto.type != VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_TCP && proto.type != VTSS_APPL_STREAM_IP_PROTOCOL_TYPE_UDP) {
        // Can't match on UDP/TCP port.
        dport.match_type = VTSS_APPL_STREAM_RANGE_MATCH_TYPE_ANY;
    }

    // Then verify while normalizing
    switch (dport.match_type) {
    case VTSS_APPL_STREAM_RANGE_MATCH_TYPE_ANY:
        // Normalize
        dport.high = 0;
        dport.low  = 0;
        break;

    case VTSS_APPL_STREAM_RANGE_MATCH_TYPE_VALUE:
        // Normalize
        dport.high = dport.low;
        break;

    case VTSS_APPL_STREAM_RANGE_MATCH_TYPE_RANGE:
        if (dport.high < dport.low) {
            return is_ipv4 ? VTSS_APPL_STREAM_RC_IPV4_DPORT_HIGH_SMALLER_THAN_LOW : VTSS_APPL_STREAM_RC_IPV6_DPORT_HIGH_SMALLER_THAN_LOW;
        }

        // Change match type if low == high
        if (dport.low == dport.high) {
            dport.match_type = VTSS_APPL_STREAM_RANGE_MATCH_TYPE_VALUE;
        }

        break;

    default:
        return is_ipv4 ? VTSS_APPL_STREAM_RC_IPV4_DPORT_MATCH_TYPE : VTSS_APPL_STREAM_RC_IPV6_DPORT_MATCH_TYPE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_protocol_conf_normalize_and_validate()
/******************************************************************************/
static mesa_rc STREAM_protocol_conf_normalize_and_validate(vtss_appl_stream_protocol_conf_t &p)
{
    vtss_appl_stream_protocol_conf_t saved;
    mesa_ipv6_t                      ipv6_mask;
    int                              i;

    // Save a copy of the configuration.
    saved = p;

    // Clear the input
    vtss_clear(p);

    // And copy the saved back to the input depending on type. This is in order
    // to get unused data of the union cleared.
    p.type = saved.type;

    switch (p.type) {
    case MESA_VCE_TYPE_ANY:
        break;

    case MESA_VCE_TYPE_ETYPE:
        p.value.etype = saved.value.etype;
        if (p.value.etype.etype < 0x600) {
            return VTSS_APPL_STREAM_RC_INVALID_ETYPE;
        }

        break;

    case MESA_VCE_TYPE_LLC:
        p.value.llc = saved.value.llc;
        break;

    case MESA_VCE_TYPE_SNAP:
        p.value.snap = saved.value.snap;

        switch (p.value.snap.oui_type) {
        case VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_CUSTOM:
            // Transform the type to another if it's one of the others.
            if (p.value.snap.oui == STREAM_SNAP_OUI_RFC1042) {
                p.value.snap.oui_type = VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_RFC1042;
            } else if (p.value.snap.oui == STREAM_SNAP_OUI_8021H) {
                p.value.snap.oui_type = VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_8021H;
            } else if (p.value.snap.oui > 0xFFFFFF) {
                return VTSS_APPL_STREAM_RC_INVALID_SNAP_OUI;
            }

            break;

        case VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_RFC1042:
            // Transform this to an OUI
            p.value.snap.oui = STREAM_SNAP_OUI_RFC1042;
            break;

        case VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_8021H:
            // Transform this to an OUI
            p.value.snap.oui = STREAM_SNAP_OUI_8021H;
            break;

        default:
            return VTSS_APPL_STREAM_RC_INVALID_SNAP_OUI_TYPE;
        }

        if (p.value.snap.oui_type == VTSS_APPL_STREAM_PROTO_SNAP_OUI_TYPE_RFC1042 && p.value.snap.pid < 0x600) {
            return VTSS_APPL_STREAM_RC_INVALID_SNAP_PID;
        }

        break;

    case MESA_VCE_TYPE_IPV4: {
        vtss_appl_stream_proto_ipv4_t &ipv4 = p.value.ipv4;
        ipv4 = saved.value.ipv4;

        // sip
        if (ipv4.sip.prefix_size > 32) {
            return VTSS_APPL_STREAM_RC_INVALID_IPV4_SIP_PREFIX_SIZE;
        }

        // Silently clear bits outside the mask
        ipv4.sip.address &= vtss_ipv4_prefix_to_mask(ipv4.sip.prefix_size);

        // dip
        if (ipv4.dip.prefix_size > 32) {
            return VTSS_APPL_STREAM_RC_INVALID_IPV4_DIP_PREFIX_SIZE;
        }

        // Silently clear bits outside the mask
        ipv4.dip.address &= vtss_ipv4_prefix_to_mask(ipv4.dip.prefix_size);

        // dscp
        VTSS_RC(STREAM_dscp_normalize_and_validate(ipv4.dscp, true /* use IPv4 return codes */));

        // fragment
        VTSS_RC(STREAM_vcap_bit_validate(ipv4.fragment, VTSS_APPL_STREAM_RC_INVALID_IPV4_FRAGMENT));

        // proto
        VTSS_RC(STREAM_ip_proto_normalize_and_validate(ipv4.proto, true /* use IPv4 return codes */));

        // dport
        VTSS_RC(STREAM_dport_normalize_and_validate(ipv4.dport, ipv4.proto, true /* use IPv4 return codes */));
        break;
    }

    case MESA_VCE_TYPE_IPV6: {
        vtss_appl_stream_proto_ipv6_t &ipv6 = p.value.ipv6;
        ipv6 = saved.value.ipv6;

        // sip
        if (ipv6.sip.prefix_size > 128) {
            return VTSS_APPL_STREAM_RC_INVALID_IPV6_SIP_PREFIX_SIZE;
        }

        // Silently clear bits outside the mask
        (void)vtss_conv_prefix_to_ipv6mask(ipv6.sip.prefix_size, &ipv6_mask);
        for (i = 0; i < ARRSZ(ipv6.sip.address.addr); i++) {
            ipv6.sip.address.addr[i] &= ipv6_mask.addr[i];
        }

        // dip
        if (ipv6.dip.prefix_size > 128) {
            return VTSS_APPL_STREAM_RC_INVALID_IPV6_DIP_PREFIX_SIZE;
        }

        // Silently clear bits outside the mask
        (void)vtss_conv_prefix_to_ipv6mask(ipv6.dip.prefix_size, &ipv6_mask);
        for (i = 0; i < ARRSZ(ipv6.dip.address.addr); i++) {
            ipv6.dip.address.addr[i] &= ipv6_mask.addr[i];
        }

        // dscp
        VTSS_RC(STREAM_dscp_normalize_and_validate(ipv6.dscp, false /* use IPv6 return codes */));

        // proto
        VTSS_RC(STREAM_ip_proto_normalize_and_validate(ipv6.proto, true /* use IPv6 return codes */));

        // dport
        VTSS_RC(STREAM_dport_normalize_and_validate(ipv6.dport, ipv6.proto, true /* use IPv6 return codes */));
        break;
    }

    default:
        return VTSS_APPL_STREAM_RC_INVALID_PROTOCOL_TYPE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// STREAM_default()
/******************************************************************************/
static void STREAM_default(void)
{
    stream_itr_t            stream_itr, next_itr;
    stream_collection_itr_t stream_collection_itr, next_collection_itr;

    STREAM_LOCK_SCOPE();

    // Delete stream collections.
    stream_collection_itr = STREAM_collection_map.begin();
    while (stream_collection_itr != STREAM_collection_map.end()) {
        next_collection_itr = stream_collection_itr;
        ++next_collection_itr;

        STREAM_collection_conf_del(stream_collection_itr->second);
        stream_collection_itr = next_collection_itr;
    }

    // Delete streams
    stream_itr = STREAM_map.begin();
    while (stream_itr != STREAM_map.end()) {
        next_itr = stream_itr;
        ++next_itr;

        STREAM_conf_del(stream_itr->second);
        stream_itr = next_itr;
    }
}

/******************************************************************************/
// STREAM_action_client_set()
/******************************************************************************/
static void STREAM_action_client_set(mesa_iflow_conf_t &iflow_conf, mesa_vce_t &vce, vtss_appl_stream_client_t client, vtss_appl_stream_action_t &action, bool &iflow_changed, bool &vce_changed)
{
    if (client == VTSS_APPL_STREAM_CLIENT_PSFP) {
        if (iflow_conf.dlb_enable != action.psfp.dlb_enable) {
            iflow_conf.dlb_enable  = action.psfp.dlb_enable;
            iflow_changed = true;
        }

        if (iflow_conf.dlb_id != action.psfp.dlb_id) {
            iflow_conf.dlb_id  = action.psfp.dlb_id;
            iflow_changed = true;
        }

        // Using VTSS_BASICS_MEMCMP_OPERATOR(mesa_psfp_iflow_conf_t)
        if (iflow_conf.psfp != action.psfp.psfp) {
            iflow_conf.psfp  = action.psfp.psfp;
            iflow_changed = true;
        }
    } else {
        if (vce.action.vid != action.frer.vid) {
            vce.action.vid  = action.frer.vid;
            vce_changed = true;
        }

        if (vce.action.pop_enable != action.frer.pop_enable) {
            vce.action.pop_enable  = action.frer.pop_enable;
            vce_changed = true;
        }

        if (vce.action.pop_cnt != action.frer.pop_cnt) {
            vce.action.pop_cnt  = action.frer.pop_cnt;
            vce_changed = true;
        }

        // Using VTSS_BASICS_MEMCMP_OPERATOR(mesa_frer_iflow_conf_t)
        if (iflow_conf.frer != action.frer.frer) {
            iflow_conf.frer  = action.frer.frer;
            iflow_changed = true;
        }
    }
}

/******************************************************************************/
// STREAM_action_cut_through_disable_set()
/******************************************************************************/
static void STREAM_action_cut_through_disable_set(vtss_appl_stream_client_status_t &client_status, mesa_iflow_conf_t &iflow_conf, bool &iflow_changed)
{
    vtss_appl_stream_action_t &p = client_status.clients[VTSS_APPL_STREAM_CLIENT_PSFP];
    vtss_appl_stream_action_t &f = client_status.clients[VTSS_APPL_STREAM_CLIENT_FRER];
    bool                      new_ct_state = STREAM_iflow_conf_default.cut_through_disable;

    // This is special, because PSFP may request to clear cut_through_disable,
    // while FRER may request to set it. If both are enabled and have an
    // opinion, FRER wins.
    if (p.enable && f.enable) {
        // Both are enabled. See whether they have an opinion on whether to
        // enable or disable CT.
        if (f.cut_through_override) {
            // FRER has an opinion and wins.
            new_ct_state = f.cut_through_disable;
        } else if (p.cut_through_override) {
            // PSFP has an opinion and wins.
            new_ct_state = p.cut_through_disable;
        }
    } else if (p.enable && p.cut_through_override) {
        // Only PSFP is enabled and it has an opinion
        new_ct_state = p.cut_through_disable;
    } else if (f.enable && f.cut_through_override) {
        // Only FRER is enabled and it has an opinion
        new_ct_state = f.cut_through_disable;
    }

    if (iflow_conf.cut_through_disable != new_ct_state) {
        iflow_conf.cut_through_disable  = new_ct_state;
        iflow_changed = true;
    }
}

/******************************************************************************/
// vtss_appl_stream_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_stream_capabilities_get(vtss_appl_stream_capabilities_t *cap)
{
    VTSS_RC(STREAM_ptr_check(cap));
    *cap = STREAM_cap;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_conf_protocol_default_get()
/******************************************************************************/
mesa_rc vtss_appl_stream_conf_protocol_default_get(mesa_vce_type_t type, vtss_appl_stream_protocol_conf_t *protocol)
{
    VTSS_RC(STREAM_vce_type_check(type));
    VTSS_RC(STREAM_ptr_check(protocol));

    vtss_clear(*protocol);
    protocol->type = type;

    // Currently, there are no other defaults that what comes out of clearing
    // the structure.

    return MESA_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_stream_conf_default_get(vtss_appl_stream_conf_t *stream_conf)
{
    VTSS_RC(STREAM_ptr_check(stream_conf));

    vtss_clear(*stream_conf);
    stream_conf->outer_tag.match_type = VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_BOTH;
    stream_conf->inner_tag.match_type = VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_BOTH;
    vtss_appl_stream_conf_protocol_default_get(MESA_VCE_TYPE_ANY, &stream_conf->protocol);

    return MESA_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_stream_conf_get(vtss_appl_stream_id_t stream_id, vtss_appl_stream_conf_t *stream_conf)
{
    stream_itr_t stream_itr;

    VTSS_RC(STREAM_id_check(stream_id));
    VTSS_RC(STREAM_ptr_check(stream_conf));

    STREAM_LOCK_SCOPE();

    if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
        return VTSS_APPL_STREAM_RC_NO_SUCH_ID;
    }

    *stream_conf = stream_itr->second.conf;
    T_I("%u: %s", stream_id, *stream_conf);
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_stream_conf_set(vtss_appl_stream_id_t stream_id, const vtss_appl_stream_conf_t *stream_conf)
{
    stream_itr_t            stream_itr;
    stream_collection_itr_t stream_collection_itr;
    mesa_iflow_id_t         iflow_id;
    vtss_appl_stream_conf_t local_conf;
    bool                    new_entry;

    VTSS_RC(STREAM_id_check(stream_id));
    VTSS_RC(STREAM_ptr_check(stream_conf));
    VTSS_RC(STREAM_vce_type_check(stream_conf->protocol.type));

    T_I("Enter. stream_conf = %s", *stream_conf);

    // Make a copy that we can modify
    local_conf = *stream_conf;

    // DMAC
    switch (local_conf.dmac.match_type) {
    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_ANY:
    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_MC:
    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_BC:
    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_UC:
    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_NOT_BC:
    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_NOT_UC:
        vtss_clear(local_conf.dmac.value);
        vtss_clear(local_conf.dmac.mask);
        break;

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_VALUE:
        // We only match on bits set in the dmac_mask, so clear the remining of
        // dmac_value
        local_conf.dmac.value &= local_conf.dmac.mask;

        // Mask must not be all-zeros. Doing so is like not matching on DMAC.
        if (mac_is_zero(local_conf.dmac.mask)) {
            return VTSS_APPL_STREAM_RC_INVALID_DMAC_MASK;
        }

        break;

    default:
        return VTSS_APPL_STREAM_RC_INVALID_DMAC_MATCH_TYPE;
    }

    // SMAC
    // We only match on bits set in the smac.mask, so clear the remining of
    // smac.value
    local_conf.smac.value &= local_conf.smac.mask;
    if (!mac_is_unicast(local_conf.smac.value)) {
        // We only allow matching on unicast SMACs.
        return VTSS_APPL_STREAM_RC_INVALID_UNICAST_SMAC;
    }

    // Outer tag
    VTSS_RC(STREAM_vlan_tag_conf_normalize_and_validate(local_conf.outer_tag, true  /* outer tag */));

    // Inner tag
    VTSS_RC(STREAM_vlan_tag_conf_normalize_and_validate(local_conf.inner_tag, false /* inner tag */));

    // If outer_tag.match_type is "untagged", then it's not possible to match on
    // an inner tag.
    // If outer_tag.match_type is "I don't care", and inner_tag.match_type is
    // "Tagged", it's OK. I think that only double-tagged frames will match
    // then.
    if (local_conf.outer_tag.match_type == VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_UNTAGGED &&
        local_conf.inner_tag.match_type == VTSS_APPL_STREAM_VLAN_TAG_MATCH_TYPE_TAGGED) {
        return VTSS_APPL_STREAM_RC_OUTER_UNTAGGED_INNER_TAGGED;
    }

    VTSS_RC(STREAM_protocol_conf_normalize_and_validate(local_conf.protocol));

    STREAM_LOCK_SCOPE();

    if ((stream_itr = STREAM_map.find(stream_id)) != STREAM_map.end()) {
        if (memcmp(&local_conf, &stream_itr->second.conf, sizeof(local_conf)) == 0) {
            T_D("%u: No changes", stream_id);
            return VTSS_RC_OK;
        }

        new_entry = false;
    } else {
        new_entry = true;
    }

    // Create a new or update an existing entry
    if ((stream_itr = STREAM_map.get(stream_id)) == STREAM_map.end()) {
        return VTSS_APPL_STREAM_RC_OUT_OF_MEMORY;
    }

    if (new_entry) {
        vtss_clear(stream_itr->second);
        stream_itr->second.stream_id  = stream_id;
        stream_itr->second.iflow_conf = STREAM_iflow_conf_default;
        stream_itr->second.iflow_id   = MESA_IFLOW_ID_NONE;

        // The stream is not part of any collection.
        stream_itr->second.status.stream_collection_id = VTSS_APPL_STREAM_COLLECTION_ID_NONE;

        // Initialize parts of the VCE. The VCE's key is initialized by
        // STREAM_vce_key_update(), and can't be initialized once and for all,
        // because it may change with the key's protocol.
        stream_itr->second.vce.id     = STREAM_VCE_ID_NONE;
        stream_itr->second.vce.action = STREAM_vce_action_default;

        // Notify observers of the newly created stream.
        STREAM_notif_table_update(stream_id, STREAM_NOTIF_STATUS_ADD);
    } else if (stream_itr->second.conf.port_list != local_conf.port_list) {
        // Gotta make a notification to observers of stream_notif_table, because
        // they might use the port list.
        STREAM_notif_table_update(stream_id, STREAM_NOTIF_STATUS_MOD);

        // If the stream is also part of a collection, notify collection
        // observers.
        if (stream_itr->second.status.stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
            STREAM_collection_notif_table_update(stream_itr->second.status.stream_collection_id, STREAM_NOTIF_STATUS_MOD);
        }
    }

    // Update our configuration.
    stream_itr->second.conf = local_conf;
    STREAM_vce_key_update(stream_itr->second);
    STREAM_oper_warnings_update(stream_itr->second);

    if (stream_itr->second.status.stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        // Notify observers of the stream collection this stream is part of that
        // a member has changed.
        STREAM_collection_notif_table_update(stream_itr->second.status.stream_collection_id, STREAM_NOTIF_STATUS_MOD);

        if ((stream_collection_itr = STREAM_collection_map.find(stream_itr->second.status.stream_collection_id)) == STREAM_collection_map.end()) {
            T_E("Stream %u claims to be part of stream_collection %u, which can't be found", stream_itr->first, stream_itr->second.status.stream_collection_id);
            return VTSS_APPL_STREAM_RC_INTERNAL_ERROR;
        }

        // A change in the stream's operational warnings may also affect the
        // stream collection's operational warnings.
        STREAM_collection_oper_warnings_update(stream_collection_itr->second);

        iflow_id = stream_collection_itr->second.iflow_id;
    } else {
        iflow_id = stream_itr->second.iflow_id;
    }

    return STREAM_mesa_vce_update(stream_itr->second, iflow_id);
}

/******************************************************************************/
// vtss_appl_stream_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_stream_conf_del(vtss_appl_stream_id_t stream_id)
{
    stream_itr_t stream_itr;

    VTSS_RC(STREAM_id_check(stream_id));

    STREAM_LOCK_SCOPE();

    if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
        return VTSS_APPL_STREAM_RC_NO_SUCH_ID;
    }

    T_I("%u", stream_id);
    STREAM_conf_del(stream_itr->second);

    return MESA_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_itr()
/******************************************************************************/
mesa_rc vtss_appl_stream_itr(const vtss_appl_stream_id_t *prev_id, vtss_appl_stream_id_t *next_id)
{
    stream_itr_t stream_itr;

    VTSS_RC(STREAM_ptr_check(next_id));

    STREAM_LOCK_SCOPE();

    if (prev_id) {
        // Here we have a valid prev_id. Find the next from that one.
        stream_itr = STREAM_map.greater_than(*prev_id);
    } else {
        // We don't have a valid prev_id. Get the first.
        stream_itr = STREAM_map.begin();
    }

    if (stream_itr != STREAM_map.end()) {
        *next_id = stream_itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_stream_action_set()
//
// Currently, two clients are defined, PSFP and FRER. They can attach to the
// stream simultaneously, because they (mostly) don't use the same action
// fields.
// Notice: Not only the stream itself has action fields, also the embedded IFLOW
// has action fields. The IFLOW ID is allocated and freed by this module (the
// stream module). Also the iflow_conf.cnt_id is allocated and freed by this
// module, and may or may not be utilized by the clients. Other action fields
// are configured by the clients themselves. For completeness, the utilized
// IFLOW action fields are listed below as well.
//
// PSFP (VTSS_APPL_STREAM_CLIENT_PSFP):
//   stream_conf.action fields:
//       <None>
//
//   iflow_conf.action fields:
//     dlb_enable
//     dlb_id
//     psfp.filter_enable
//     psfp.filter_id
//     cut_through_disable
//
// FRER (VTSS_APPL_STREAM_CLIENT_FRER):
//   stream_conf.action fields:
//     vid
//     pop_enable
//     pop_cnt
//
//   iflow_conf.action fields:
//     frer.generation
//     frer.pop
//     frer.mstream_enable
//     frer.mstream_id
//     cut_through_disable
//
// The vigilant reader will notice that iflow_conf.cut_through_disable is
// utilized by both PSFP and FRER. PSFP wants to enable cut-through, whereas
// FRER wants to disable it.
// FRER will not work if cut-through is enabled, whereas PSFP will work fine.
// Therefore, the two modules are aware of each other, so that if PSFP detects
// that FRER also is attaching to the stream, it doesn't change its value.
// Likewise, if FRER no longer attaches to the stream, but PSFP is still
// attached, it enables cut-through. Otherwise, the field is set to its
// default.
/******************************************************************************/
mesa_rc vtss_appl_stream_action_set(vtss_appl_stream_id_t stream_id, vtss_appl_stream_client_t client, vtss_appl_stream_action_t *action, bool frer_seq_gen_reset)
{
    stream_itr_t stream_itr;
    bool         iflow_changed, vce_changed;

    VTSS_RC(STREAM_id_check(stream_id));
    VTSS_RC(STREAM_client_check(client));

    if (!frer_seq_gen_reset) {
        VTSS_RC(STREAM_ptr_check(action));
        struct stream_action_t a(*action, client);
        T_I("%u: %s: action = %s", stream_id, stream_util_client_to_str(client, true), a);
    }

    STREAM_LOCK_SCOPE();

    if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
        return VTSS_APPL_STREAM_RC_NO_SUCH_ID;
    }

    stream_state_t &stream_state = stream_itr->second;

    if (stream_state.status.stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        // The stream is part of a collection, so the user must attach via
        // vtss_appl_stream_collection_action_set()
        return VTSS_APPL_STREAM_RC_PART_OF_COLLECTION;
    }

    iflow_changed = false;
    vce_changed   = false;

    if (!frer_seq_gen_reset) {
        stream_state.status.client_status.clients[client] = *action;

        // Figure out whether to allocate/free/keep an IFLOW and ingress counter
        // based on whether all clients have detached or at least one has
        // attached or is still attached to this stream.
        VTSS_RC(STREAM_mesa_iflow_and_ingress_cnt_alloc_or_free(stream_state, iflow_changed, vce_changed));

        vtss_appl_stream_action_t &a = action->enable ? stream_state.status.client_status.clients[client] : STREAM_action_default[client];

        STREAM_action_client_set(stream_state.iflow_conf, stream_state.vce, client, a, iflow_changed, vce_changed);
        STREAM_action_cut_through_disable_set(stream_state.status.client_status, stream_state.iflow_conf, iflow_changed);
        STREAM_oper_warnings_update(stream_itr->second);
    }

    if (frer_seq_gen_reset || iflow_changed) {
        STREAM_mesa_iflow_conf_set(stream_state.iflow_id, stream_state.iflow_conf);
    }

    if (vce_changed) {
        return STREAM_mesa_vce_update(stream_state, stream_state.iflow_id);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_status_get()
/******************************************************************************/
mesa_rc vtss_appl_stream_status_get(vtss_appl_stream_id_t stream_id, vtss_appl_stream_status_t *status)
{
    stream_itr_t stream_itr;

    VTSS_RC(STREAM_id_check(stream_id));
    VTSS_RC(STREAM_ptr_check(status));

    STREAM_LOCK_SCOPE();

    if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
        return VTSS_APPL_STREAM_RC_NO_SUCH_ID;
    }

    *status = stream_itr->second.status;
    T_I("%u: status = %s", stream_id, *status);
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_collection_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_stream_collection_capabilities_get(vtss_appl_stream_collection_capabilities_t *cap)
{
    VTSS_RC(STREAM_ptr_check(cap));
    *cap = STREAM_collection_cap;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_collection_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_stream_collection_conf_default_get(vtss_appl_stream_collection_conf_t *stream_collection_conf)
{
    VTSS_RC(STREAM_ptr_check(stream_collection_conf));

    vtss_clear(*stream_collection_conf);
    return MESA_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_collection_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_stream_collection_conf_get(vtss_appl_stream_collection_id_t stream_collection_id, vtss_appl_stream_collection_conf_t *stream_collection_conf)
{
    stream_collection_itr_t stream_collection_itr;

    VTSS_RC(STREAM_collection_id_check(stream_collection_id));
    VTSS_RC(STREAM_ptr_check(stream_collection_conf));

    STREAM_LOCK_SCOPE();

    if ((stream_collection_itr = STREAM_collection_map.find(stream_collection_id)) == STREAM_collection_map.end()) {
        return VTSS_APPL_STREAM_RC_COLLECTION_NO_SUCH_ID;
    }

    *stream_collection_conf = stream_collection_itr->second.conf;
    T_IG(STREAM_TRACE_GRP_COLLECTION, "%u: %s", stream_collection_id, *stream_collection_conf);
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_collection_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_stream_collection_conf_set(vtss_appl_stream_collection_id_t stream_collection_id, const vtss_appl_stream_collection_conf_t *stream_collection_conf)
{
    stream_collection_itr_t            stream_collection_itr;
    stream_itr_t                       stream_itr;
    vtss_appl_stream_id_t              stream_id;
    vtss_appl_stream_collection_conf_t local_conf;
    bool                               new_entry, found;
    int                                i, j;

    VTSS_RC(STREAM_collection_id_check(stream_collection_id));
    VTSS_RC(STREAM_ptr_check(stream_collection_conf));

    // We need to normalize the configuration, so take a copy of the user's
    // configuration.
    local_conf = *stream_collection_conf;

    // Stream IDs.
    // Normalize list of stream IDs by first removing duplicates and then
    // sorting it, so that entries with VTSS_APPL_STREAM_ID_NONE come last.
    // We work on local_conf, which is a copy of the user's conf.
    STREAM_collection_stream_ids_normalize(local_conf.stream_ids, ARRSZ(local_conf.stream_ids));

    T_IG(STREAM_TRACE_GRP_COLLECTION, "%u: %s", stream_collection_id, *stream_collection_conf);

    STREAM_LOCK_SCOPE();

    // Go through all requested stream IDs and see if they exist.
    for (i = 0; i < ARRSZ(local_conf.stream_ids); i++) {
        stream_id = local_conf.stream_ids[i];

        if (stream_id == VTSS_APPL_STREAM_ID_NONE) {
            // End of list.
            break;
        }

        // The stream must exist.
        if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
            return VTSS_APPL_STREAM_RC_COLLECTION_STREAM_ID_DOESNT_EXIST;
        }

        // The stream cannot be part of another stream collection.
        if (stream_itr->second.status.stream_collection_id == VTSS_APPL_STREAM_COLLECTION_ID_NONE ||
            stream_itr->second.status.stream_collection_id == stream_collection_id) {
            // OK not to be part of a stream collection or already being part of
            // this.
            continue;
        }

        return VTSS_APPL_STREAM_RC_COLLECTION_STREAM_PART_OF_OTHER_COLLECTION;
    }

    if ((stream_collection_itr = STREAM_collection_map.find(stream_collection_id)) != STREAM_collection_map.end()) {
        if (memcmp(&local_conf, &stream_collection_itr->second.conf, sizeof(local_conf)) == 0) {
            T_DG(STREAM_TRACE_GRP_COLLECTION, "%u: No changes", stream_collection_id);
            return VTSS_RC_OK;
        }

        new_entry = false;
    } else {
        new_entry = true;
    }

    // Create a new or update an existing entry
    if ((stream_collection_itr = STREAM_collection_map.get(stream_collection_id)) == STREAM_collection_map.end()) {
        return VTSS_APPL_STREAM_RC_OUT_OF_MEMORY;
    }

    if (new_entry) {
        vtss_clear(stream_collection_itr->second);
        stream_collection_itr->second.stream_collection_id = stream_collection_id;
        stream_collection_itr->second.iflow_conf = STREAM_iflow_conf_default;
        stream_collection_itr->second.iflow_id   = MESA_IFLOW_ID_NONE;

        // Notify observers of the newly created stream collection
        STREAM_collection_notif_table_update(stream_collection_id, STREAM_NOTIF_STATUS_ADD);
    }

    if (!new_entry) {
        // Go through all existing streams in this stream collection and remove
        // those that are no longer part of the new stream collection.
        for (i = 0; i < ARRSZ(stream_collection_itr->second.conf.stream_ids); i++) {
            stream_id = stream_collection_itr->second.conf.stream_ids[i];

            if (stream_id == VTSS_APPL_STREAM_ID_NONE) {
                // End of list.
                break;
            }

            found = false;
            for (j = 0; j < ARRSZ(local_conf.stream_ids); j++) {
                if (local_conf.stream_ids[j] == VTSS_APPL_STREAM_ID_NONE) {
                    // End of list.
                    break;
                }

                if (local_conf.stream_ids[j] == stream_id) {
                    found = true;
                    break;
                }
            }

            if (found) {
                // The stream is also part of the new collection. Nothing to do.
                continue;
            }

            // The stream is no longer part of this collection.
            if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
                // Not possible, because we ahve the lock, so it can't just
                // disappear.
                T_EG(STREAM_TRACE_GRP_COLLECTION, "%u: stream_id = %u existsed a milliseconds ago. Where's it gone?", stream_collection_id, stream_id);
                return VTSS_APPL_STREAM_RC_INTERNAL_ERROR;
            }

            STREAM_collection_detach_stream(stream_collection_itr->second, stream_itr->second);
        }
    }

    // Update our configuration
    stream_collection_itr->second.conf = local_conf;

    // Go through all streams in the new stream collection to see if they must
    // be moved from a standalone state to this one.
    for (i = 0; i < ARRSZ(stream_collection_itr->second.conf.stream_ids); i++) {
        stream_id = stream_collection_itr->second.conf.stream_ids[i];

        if (stream_id == VTSS_APPL_STREAM_ID_NONE) {
            // End of list.
            break;
        }

        if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
            // Not possible, because we have the lock, so it can't just
            // disappear.
            T_EG(STREAM_TRACE_GRP_COLLECTION, "%u: stream_id = %u existed a millisecond ago. Where's it gone?", stream_collection_id, stream_id);
            return VTSS_APPL_STREAM_RC_INTERNAL_ERROR;
        }

        if (stream_itr->second.status.stream_collection_id == stream_collection_id) {
            // Already owned by this stream collection.
            continue;
        }

        // We have made a check above that the stream is not part of another
        // collection.
        if (stream_itr->second.status.stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
            T_EG(STREAM_TRACE_GRP_COLLECTION, "stream_id = %u was not part of any other collection a millisecond ago. Now it is (%u)", stream_id, stream_itr->second.status.stream_collection_id);
            return VTSS_APPL_STREAM_RC_COLLECTION_STREAM_PART_OF_OTHER_COLLECTION;
        }

        // The stream may be active all by itself. Go and deactivate it.
        STREAM_free_resources(stream_itr->second);

        // Attach the stream to this collection
        STREAM_collection_attach_stream(stream_collection_itr->second, stream_itr->second);
    }

    // Update operational warnings
    STREAM_collection_oper_warnings_update(stream_collection_itr->second);

    // Notify observers of the stream collection that something has changed.
    STREAM_collection_notif_table_update(stream_collection_id, STREAM_NOTIF_STATUS_MOD);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_collection_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_stream_collection_conf_del(vtss_appl_stream_collection_id_t stream_collection_id)
{
    stream_collection_itr_t stream_collection_itr;

    VTSS_RC(STREAM_collection_id_check(stream_collection_id));

    STREAM_LOCK_SCOPE();

    if ((stream_collection_itr = STREAM_collection_map.find(stream_collection_id)) == STREAM_collection_map.end()) {
        return VTSS_APPL_STREAM_RC_COLLECTION_NO_SUCH_ID;
    }

    T_IG(STREAM_TRACE_GRP_COLLECTION, "%u", stream_collection_id);
    STREAM_collection_conf_del(stream_collection_itr->second);
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_collection_itr()
/******************************************************************************/
mesa_rc vtss_appl_stream_collection_itr(const vtss_appl_stream_collection_id_t *prev_id, vtss_appl_stream_collection_id_t *next_id)
{
    stream_collection_itr_t stream_collection_itr;

    VTSS_RC(STREAM_ptr_check(next_id));

    STREAM_LOCK_SCOPE();

    if (prev_id) {
        // Here we have a valid prev_id. Find the next from that one.
        stream_collection_itr = STREAM_collection_map.greater_than(*prev_id);
    } else {
        // We don't have a valid prev_id. Get the first.
        stream_collection_itr = STREAM_collection_map.begin();
    }

    if (stream_collection_itr != STREAM_collection_map.end()) {
        *next_id = stream_collection_itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_stream_collection_action_set()
/******************************************************************************/
mesa_rc vtss_appl_stream_collection_action_set(vtss_appl_stream_collection_id_t stream_collection_id, vtss_appl_stream_client_t client, vtss_appl_stream_action_t *action, bool frer_seq_gen_reset)
{
    stream_collection_itr_t stream_collection_itr;
    stream_itr_t            stream_itr;
    vtss_appl_stream_id_t   stream_id;
    bool                    iflow_changed, vce_changed, iflow_conf_already_set;
    int                     i;

    VTSS_RC(STREAM_collection_id_check(stream_collection_id));
    VTSS_RC(STREAM_client_check(client));

    if (!frer_seq_gen_reset) {
        VTSS_RC(STREAM_ptr_check(action));
        struct stream_action_t a(*action, client);
        T_IG(STREAM_TRACE_GRP_COLLECTION, "%u: %s: action = %s", stream_collection_id, stream_util_client_to_str(client, true), a);
    }

    STREAM_LOCK_SCOPE();

    if ((stream_collection_itr = STREAM_collection_map.find(stream_collection_id)) == STREAM_collection_map.end()) {
        return VTSS_APPL_STREAM_RC_COLLECTION_NO_SUCH_ID;
    }

    stream_collection_state_t &stream_collection_state = stream_collection_itr->second;

    if (!frer_seq_gen_reset) {
        stream_collection_state.status.client_status.clients[client] = *action;

        iflow_changed          = false;
        vce_changed            = false;
        iflow_conf_already_set = false;

        // Figure out whether to allocate/free/keep an IFLOW and ingress counter
        // based on whether all clients have detached or at least one has
        // attached or is still attached to this stream collection.
        VTSS_RC(STREAM_collection_mesa_iflow_and_ingress_cnt_alloc_or_free(stream_collection_state, iflow_changed, vce_changed));

        vtss_appl_stream_action_t &a = action->enable ? stream_collection_state.status.client_status.clients[client] : STREAM_action_default[client];

        STREAM_action_cut_through_disable_set(stream_collection_state.status.client_status, stream_collection_itr->second.iflow_conf, iflow_changed);

        // Set action on all streams in the collection.
        for (i = 0; i < ARRSZ(stream_collection_itr->second.conf.stream_ids); i++) {
            stream_id = stream_collection_itr->second.conf.stream_ids[i];

            if (stream_id == VTSS_APPL_STREAM_ID_NONE) {
                break;
            }

            if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
                // This should not be possible.
                T_EG(STREAM_TRACE_GRP_COLLECTION, "%u: stream_id = %u not found", stream_collection_id, stream_id);
                continue;
            }

            STREAM_action_client_set(stream_collection_state.iflow_conf, stream_itr->second.vce, client, a, iflow_changed, vce_changed);

            if (iflow_changed && !iflow_conf_already_set) {
                // We only need to set the IFLOW configuration once, because its
                // identical for all streams.
                STREAM_mesa_iflow_conf_set(stream_collection_state.iflow_id, stream_collection_state.iflow_conf);
                iflow_conf_already_set = true;
            }

            if (vce_changed || stream_itr->second.vce.action.flow_id != stream_collection_state.iflow_id) {
                (void)STREAM_mesa_vce_update(stream_itr->second, stream_collection_state.iflow_id);

                // Prepare for next stream
                vce_changed = false;
            }

            STREAM_oper_warnings_update(stream_itr->second);
        }

        STREAM_collection_oper_warnings_update(stream_collection_itr->second);
    }

    if (frer_seq_gen_reset) {
        STREAM_mesa_iflow_conf_set(stream_collection_state.iflow_id, stream_collection_state.iflow_conf);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_stream_collection_status_get()
/******************************************************************************/
mesa_rc vtss_appl_stream_collection_status_get(vtss_appl_stream_collection_id_t stream_collection_id, vtss_appl_stream_collection_status_t *status)
{
    stream_collection_itr_t stream_collection_itr;

    VTSS_RC(STREAM_collection_id_check(stream_collection_id));
    VTSS_RC(STREAM_ptr_check(status));

    STREAM_LOCK_SCOPE();

    if ((stream_collection_itr = STREAM_collection_map.find(stream_collection_id)) == STREAM_collection_map.end()) {
        return VTSS_APPL_STREAM_RC_COLLECTION_NO_SUCH_ID;
    }

    *status = stream_collection_itr->second.status;
    T_I("%u: status = %s", stream_collection_id, *status);
    return VTSS_RC_OK;
}

/******************************************************************************/
// stream_util_client_to_str()
/******************************************************************************/
const char *stream_util_client_to_str(vtss_appl_stream_client_t client, bool capital)
{
    switch (client) {
    case VTSS_APPL_STREAM_CLIENT_PSFP:
        return capital ? "PSFP" : "psfp";

    case VTSS_APPL_STREAM_CLIENT_FRER:
        return capital ? "FRER" : "frer";

    default:
        T_E("Unknown client type (%d)", client);
        return capital ? "UNDEFINED" : "undefined";
    }
}

/******************************************************************************/
// stream_util_counters_get()
/******************************************************************************/
mesa_rc stream_util_counters_get(vtss_appl_stream_id_t stream_id, mesa_ingress_counters_t &counters)
{
    VTSS_RC(STREAM_id_check(stream_id));

    STREAM_LOCK_SCOPE();
    return STREAM_counters_get(stream_id, counters);
}

/******************************************************************************/
// stream_util_counters_clear()
// Notice, if stream_id is part of a stream collection, the stream collection's
// counters will be cleared.
// One could do the same thing for stream_util_counters_get(), but both PSFP and
// FRER know whether they are attaching to a stream or a stream collection, so
// they will call the correct function.
/******************************************************************************/
mesa_rc stream_util_counters_clear(vtss_appl_stream_id_t stream_id)
{
    VTSS_RC(STREAM_id_check(stream_id));

    STREAM_LOCK_SCOPE();
    return STREAM_counters_clear(stream_id);
}

/******************************************************************************/
// stream_collection_util_counters_get()
/******************************************************************************/
mesa_rc stream_collection_util_counters_get(vtss_appl_stream_collection_id_t stream_collection_id, mesa_ingress_counters_t &counters)
{
    VTSS_RC(STREAM_collection_id_check(stream_collection_id));

    STREAM_LOCK_SCOPE();
    return STREAM_collection_counters_get(stream_collection_id, counters);
}

/******************************************************************************/
// stream_collection_util_counters_clear()
/******************************************************************************/
mesa_rc stream_collection_util_counters_clear(vtss_appl_stream_collection_id_t stream_collection_id)
{
    VTSS_RC(STREAM_collection_id_check(stream_collection_id));

    STREAM_LOCK_SCOPE();
    return STREAM_collection_counters_clear(stream_collection_id);
}

/******************************************************************************/
// stream_util_dmac_match_type_to_str()
/******************************************************************************/
const char *stream_util_dmac_match_type_to_str(vtss_appl_stream_dmac_match_type_t dmac_match_type)
{
    switch (dmac_match_type) {
    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_ANY:
        return "any";

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_MC:
        return "multicast";

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_BC:
        return "broadcast";

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_UC:
        return "unicast";

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_NOT_BC:
        return "not-broadcast";

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_NOT_UC:
        return "not-unicast";

    case VTSS_APPL_STREAM_DMAC_MATCH_TYPE_VALUE:
        return "value/mask";

    default:
        T_E("Invalid dmac_match_type (%d)", dmac_match_type);
        return "unknown";
    }
}

/******************************************************************************/
// stream_util_protocol_type_to_str()
/******************************************************************************/
const char *stream_util_protocol_type_to_str(mesa_vce_type_t t)
{
    switch (t) {
    case MESA_VCE_TYPE_ANY:
        return "Any";

    case MESA_VCE_TYPE_ETYPE:
        return "EtherType";

    case MESA_VCE_TYPE_LLC:
        return "LLC";

    case MESA_VCE_TYPE_SNAP:
        return "SNAP";

    case MESA_VCE_TYPE_IPV4:
        return "IPv4";

    case MESA_VCE_TYPE_IPV6:
        return "IPv6";

    default:
        T_E("Invalid VCE type (%d)", t);
        return "Invalid";
    }
}

/******************************************************************************/
// stream_debug_statistics_show()
/******************************************************************************/
void stream_debug_statistics_show(vtss_appl_stream_id_t stream_id, stream_icli_pr_t pr, bool first)
{
    stream_itr_t                     stream_itr;
    mesa_ingress_counters_t          counters;
    vtss_appl_stream_collection_id_t stream_collection_id = VTSS_APPL_STREAM_COLLECTION_ID_NONE;
    char                             buf[11];
    mesa_rc                          rc;

    if (first) {
        pr("Stream Coll rx_green      rx_yellow     rx_red        rx_match      rx_gate_pass  rx_gate_disc  rx_sdu_pass   rx_sdu_disc   rx_discard    tx_discard\n");
        pr("------ ---- ------------- ------------- ------------- ------------- ------------- ------------- ------------- ------------- ------------- -------------\n");
    }

    STREAM_LOCK_SCOPE();

    if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
        rc = VTSS_APPL_STREAM_RC_NO_SUCH_ID;
    } else {
        stream_collection_id = stream_itr->second.status.stream_collection_id;
        if (stream_collection_id == VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
            rc = STREAM_counters_get(stream_itr->first, counters);
        } else {
            rc = STREAM_collection_counters_get(stream_collection_id, counters);
        }
    }

    if (stream_collection_id == VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        strcpy(buf, "None");
    } else {
        sprintf(buf, "%u", stream_collection_id);
    }

    if (rc != VTSS_RC_OK) {
        pr("%6u %4s %s\n", stream_id, buf, error_txt(rc));
        return;
    }

    pr("%6u %4s"
       VPRI64Fu("13") " " VPRI64Fu("13") " " VPRI64Fu("13") " " VPRI64Fu("13") " " VPRI64Fu("13") " "
       VPRI64Fu("13") " " VPRI64Fu("13") " " VPRI64Fu("13") " " VPRI64Fu("13") " " VPRI64Fu("13") "\n",
       stream_id,
       buf,
       counters.rx_green.frames,
       counters.rx_yellow.frames,
       counters.rx_red.frames,
       counters.rx_match,
       counters.rx_gate_pass,
       counters.rx_gate_discard,
       counters.rx_sdu_pass,
       counters.rx_sdu_discard,
       counters.rx_discard.frames,
       counters.tx_discard.frames);
}

/******************************************************************************/
// stream_debug_vces_show()
/******************************************************************************/
void stream_debug_vces_show(vtss_appl_stream_id_t stream_id, stream_icli_pr_t pr, bool first)
{
    stream_itr_t                     stream_itr;
    stream_collection_itr_t          stream_collection_itr;
    vtss_appl_stream_collection_id_t stream_collection_id;
    vtss_appl_stream_client_status_t *client_status;
    char                             buf[11];

    STREAM_LOCK_SCOPE();

    if (first) {
        pr("Stream Coll VCE ID     PSFP PSFP ID FRER FRER ID flow_id    vid  pop_enable pop_cnt\n");
        pr("------ ---- ---------- ---- ------- ---- ------- ---------- ---- ---------- -------\n");
    }

    if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
        pr("%6u %s\n", stream_id, error_txt(VTSS_APPL_STREAM_RC_NO_SUCH_ID));
        return;
    }

    stream_collection_id = stream_itr->second.status.stream_collection_id;
    if (stream_collection_id == VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        strcpy(buf, "None");
        client_status = &stream_itr->second.status.client_status;
    } else {
        if ((stream_collection_itr = STREAM_collection_map.find(stream_collection_id)) == STREAM_collection_map.end()) {
            // Code bug
            T_E("Stream %u points to stream collection %u, but that doesn't exist in map", stream_id, stream_collection_id);
            return;
        }

        sprintf(buf, "%u", stream_collection_id);
        client_status = &stream_collection_itr->second.status.client_status;
    }

    if (stream_itr->second.vce.id == STREAM_VCE_ID_NONE) {
        pr("%6u %4s %s\n", stream_id, buf, "VCE not created");
        return;
    }

    pr("%6u %4s 0x%08x %-4s %7u %-4s %7u %10u %4u %10d %7d\n",
       stream_id,
       buf,
       stream_itr->second.vce.id,
       client_status->clients[VTSS_APPL_STREAM_CLIENT_PSFP].enable ? "Yes" : "No",
       client_status->clients[VTSS_APPL_STREAM_CLIENT_PSFP].client_id,
       client_status->clients[VTSS_APPL_STREAM_CLIENT_FRER].enable ? "Yes" : "No",
       client_status->clients[VTSS_APPL_STREAM_CLIENT_FRER].client_id,
       stream_itr->second.vce.action.flow_id,
       stream_itr->second.vce.action.vid,
       stream_itr->second.vce.action.pop_enable,
       stream_itr->second.vce.action.pop_cnt);
}

/******************************************************************************/
// stream_debug_iflows_show()
/******************************************************************************/
void stream_debug_iflows_show(vtss_appl_stream_id_t stream_id, stream_icli_pr_t pr, bool first)
{
    stream_itr_t                     stream_itr;
    stream_collection_itr_t          stream_collection_itr;
    vtss_appl_stream_collection_id_t stream_collection_id;
    mesa_iflow_id_t                  iflow_id;
    mesa_iflow_conf_t                *iflow_conf;
    vtss_appl_stream_client_status_t *client_status;
    char                             buf[11];

    STREAM_LOCK_SCOPE();

    if (first) {
        pr("Stream Coll IFLOW ID PSFP PSFP ID FRER FRER ID cnt_id ct_dis dlb_ena dlb_id psfp.ena psfp.id frer.ena frer.id frer.gen frer.pop\n");
        pr("------ ---- -------- ---- ------- ---- ------- ------ ------ ------- ------ -------- ------- -------- ------- -------- --------\n");
    }

    if ((stream_itr = STREAM_map.find(stream_id)) == STREAM_map.end()) {
        pr("%6u %s\n", stream_id, error_txt(VTSS_APPL_STREAM_RC_NO_SUCH_ID));
        return;
    }

    stream_collection_id = stream_itr->second.status.stream_collection_id;
    if (stream_collection_id == VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        strcpy(buf, "None");
        iflow_id      =  stream_itr->second.iflow_id;
        iflow_conf    = &stream_itr->second.iflow_conf;
        client_status = &stream_itr->second.status.client_status;
    } else {
        if ((stream_collection_itr = STREAM_collection_map.find(stream_collection_id)) == STREAM_collection_map.end()) {
            // Code bug
            T_E("Stream %u points to stream collection %u, but that doesn't exist in map", stream_id, stream_collection_id);
            return;
        }

        sprintf(buf, "%u", stream_collection_id);
        iflow_id      =  stream_collection_itr->second.iflow_id;
        iflow_conf    = &stream_collection_itr->second.iflow_conf;
        client_status = &stream_collection_itr->second.status.client_status;
    }

    if (iflow_id == MESA_IFLOW_ID_NONE) {
        pr("%6u %4s %s\n", stream_id, buf, "IFLOW not created");
        return;
    }

    pr("%6u %4s %8u %-4s %7u %-4s %7u %6u %6d %7d %6u %8d %7u %8d %7u %8d %8d\n",
       stream_id,
       buf,
       iflow_id,
       client_status->clients[VTSS_APPL_STREAM_CLIENT_PSFP].enable ? "Yes" : "No",
       client_status->clients[VTSS_APPL_STREAM_CLIENT_PSFP].client_id,
       client_status->clients[VTSS_APPL_STREAM_CLIENT_FRER].enable ? "Yes" : "No",
       client_status->clients[VTSS_APPL_STREAM_CLIENT_FRER].client_id,
       iflow_conf->cnt_id,
       iflow_conf->cut_through_disable,
       iflow_conf->dlb_enable,
       iflow_conf->dlb_id,
       iflow_conf->psfp.filter_enable,
       iflow_conf->psfp.filter_id,
       iflow_conf->frer.mstream_enable,
       iflow_conf->frer.mstream_id,
       iflow_conf->frer.generation,
       iflow_conf->frer.pop);
}

/******************************************************************************/
// stream_util_oper_warnings_to_str()
// Buf must be ~400 bytes long if all bits are set.
/******************************************************************************/
char *stream_util_oper_warnings_to_str(char *buf, size_t size, vtss_appl_stream_oper_warnings_t oper_warnings)
{
    int  s, res;
    bool first;

    if (!buf) {
        return buf;
    }

#define P(_str_)                                        \
    if (size - s > 0) {                                 \
        res = snprintf(buf + s, size - s, "%s", _str_); \
        if (res > 0) {                                  \
            s += res;                                   \
        }                                               \
    }

#define F(X, _name_)                                         \
    if (oper_warnings & VTSS_APPL_STREAM_OPER_WARNING_##X) { \
        oper_warnings &= ~VTSS_APPL_STREAM_OPER_WARNING_##X; \
        if (first) {                                         \
            first = false;                                   \
            P(_name_);                                       \
        } else {                                             \
            P(", " _name_);                                  \
        }                                                    \
    }

    buf[0] = 0;
    s      = 0;
    first  = true;

    // Example of a field name (just so that we can search for this function):
    // VTSS_APPL_STREAM_OPER_WARNING_NOT_INSTALLED_ON_ANY_PORT

    F(NOT_INSTALLED_ON_ANY_PORT, "The stream does not have any member ports");

    buf[MIN(size - 1, s)] = 0;
#undef F
#undef P

    if (oper_warnings != 0) {
        T_E("Not all operational warnings are handled. Missing = 0x%x", oper_warnings);
    }

    return buf;
}

/******************************************************************************/
// stream_collection_util_oper_warnings_to_str()
// Buf must be ~400 bytes long if all bits are set.
/******************************************************************************/
char *stream_collection_util_oper_warnings_to_str(char *buf, size_t size, vtss_appl_stream_collection_oper_warnings_t oper_warnings)
{
    int  s, res;
    bool first;

    if (!buf) {
        return buf;
    }

#define P(_str_)                                        \
    if (size - s > 0) {                                 \
        res = snprintf(buf + s, size - s, "%s", _str_); \
        if (res > 0) {                                  \
            s += res;                                   \
        }                                               \
    }

#define F(X, _name_)                                                    \
    if (oper_warnings & VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_##X) { \
        oper_warnings &= ~VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_##X; \
        if (first) {                                                    \
            first = false;                                              \
            P(_name_);                                                  \
        } else {                                                        \
            P(", " _name_);                                             \
        }                                                               \
    }

    buf[0] = 0;
    s      = 0;
    first  = true;

    // Example of a field name (just so that we can search for this function):
    // VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NO_STREAMS_ATTACHED

    F(NO_STREAMS_ATTACHED,                          "No streams attached");
    F(NO_CLIENTS_ATTACHED,                          "No clients attached");
    F(AT_LEAST_ONE_STREAM_HAS_OPERATIONAL_WARNINGS, "At least one of the attached streams has configurational warnings");

    buf[MIN(size - 1, s)] = 0;
#undef F
#undef P

    if (oper_warnings != 0) {
        T_E("Not all operational warnings are handled. Missing = 0x%x", oper_warnings);
    }

    return buf;
}

//*****************************************************************************/
// stream_collection_util_stream_id_list_to_str()
// sz contains the number of items in the \p ids array.
// We don't know the size of the buffer. Caller must ensure it's big enough.
// We assume the list is already sorted and that entries with ID equal to
// VTSS_APPL_STREAM_ID_NONE come last.
//
// Example of output: 1-3,17,29-31,33
/******************************************************************************/
char *stream_collection_util_stream_id_list_to_str(const vtss_appl_stream_id_t *ids, size_t sz, char *buf)
{
    vtss_appl_stream_id_t id, prev_id = VTSS_APPL_STREAM_ID_NONE;
    int                   i, count, printed;
    char                  *p;

    if (!buf) {
        return buf;
    }

    buf[0] = '\0';

    if (sz <= 0) {
        return buf;
    }

    count   = 0;
    printed = 0;
    p       = buf;

    // Invariant: Always print the first number of a new sequence.
    for (i = 0; i < sz; i++) {
        id = ids[i];

        if (id == VTSS_APPL_STREAM_ID_NONE) {
            continue;
        }

        if (prev_id == VTSS_APPL_STREAM_ID_NONE) {
            p += sprintf(p, "%s%u", printed == 0 ? "" : ",", id);
            printed++;
            count = 0;
        } else if (id == prev_id + 1) {
            // Got a sequence. Don't print (yet).
            count++;
        } else {
            if (count) {
                // Terminate previous sequence.
                p += sprintf(p, "%s%u", count == 1 ? "," : "-", prev_id);
                printed++;
                count = 0;
            }

            p += sprintf(p, ",%u", id);
        }

        prev_id = id;
    }

    if (count) {
        // Terminate previous sequence
        p += sprintf(p, "%s%u", count == 1 ? "," : "-", prev_id);
    }

    return buf;
}

/******************************************************************************/
// stream_error_txt()
/******************************************************************************/
const char *stream_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_STREAM_RC_INVALID_PARAMETER:
        return "Invalid parameter";

    case VTSS_APPL_STREAM_RC_NO_SUCH_ID:
        return "No such stream ID";

    case VTSS_APPL_STREAM_RC_HW_RESOURCES:
        return "Out of hardware resources";

    case VTSS_APPL_STREAM_RC_INTERNAL_ERROR:
        return "Internal error. A code-update is required. See console or crashlog for details";

    case VTSS_APPL_STREAM_RC_INVALID_ID:
        return "Invalid stream ID";

    case VTSS_APPL_STREAM_RC_INVALID_CLIENT:
        return "Invalid client ID";

    case VTSS_APPL_STREAM_RC_INVALID_DMAC_MATCH_TYPE:
        return "Invalid DMAC match type";

    case VTSS_APPL_STREAM_RC_INVALID_DMAC_MASK:
        return "The DMAC mask cannot be all-zeros.";

    case VTSS_APPL_STREAM_RC_INVALID_UNICAST_SMAC:
        return "The SMAC is not a unicast MAC address.";

    case VTSS_APPL_STREAM_RC_INVALID_VLAN_MATCH_TYPE_OUTER_TAG:
        return "Invalid outer tag match type";

    case VTSS_APPL_STREAM_RC_INVALID_VLAN_MATCH_TYPE_INNER_TAG:
        return "Invalid inner tag match type";

    case VTSS_APPL_STREAM_RC_INVALID_TAG_TYPE_OUTER_TAG:
        return "Invalid outer tag tag type";

    case VTSS_APPL_STREAM_RC_INVALID_TAG_TYPE_INNER_TAG:
        return "Invalid inner tag tag type";

    case VTSS_APPL_STREAM_RC_INVALID_VID_VALUE_OUTER_TAG:
        return "Invalid outer tag VLAN ID";

    case VTSS_APPL_STREAM_RC_INVALID_VID_VALUE_INNER_TAG:
        return "Invalid inner tag VLAN ID";

    case VTSS_APPL_STREAM_RC_INVALID_VID_MASK_OUTER_TAG:
        return "Invalid outer tag VLAN mask";

    case VTSS_APPL_STREAM_RC_INVALID_VID_MASK_INNER_TAG:
        return "Invalid inner tag VLAN mask";

    case VTSS_APPL_STREAM_RC_INVALID_PCP_VALUE_OUTER_TAG:
        return "Invalid outer tag PCP value";

    case VTSS_APPL_STREAM_RC_INVALID_PCP_VALUE_INNER_TAG:
        return "Invalid inner tag PCP value";

    case VTSS_APPL_STREAM_RC_INVALID_PCP_MASK_OUTER_TAG:
        return "Invalid outer tag PCP mask";

    case VTSS_APPL_STREAM_RC_INVALID_PCP_MASK_INNER_TAG:
        return "Invalid inner tag PCP mask";

    case VTSS_APPL_STREAM_RC_INVALID_DEI_OUTER_TAG:
        return "Invalid outer tag DEI";

    case VTSS_APPL_STREAM_RC_INVALID_DEI_INNER_TAG:
        return "Invalid inner tag DEI";

    case VTSS_APPL_STREAM_RC_OUTER_UNTAGGED_INNER_TAGGED:
        return "If outer-tag matches untagged frames, inner-tag cannot match tagged frames";

    case VTSS_APPL_STREAM_RC_INVALID_ETYPE:
        return "Invalid EtherType. Valid range is 0x600 - 0xFFFF";

    case VTSS_APPL_STREAM_RC_INVALID_SNAP_OUI:
        return "Invalid SNAP OUI. Valid range is 0x000000 - 0xFFFFFF";

    case VTSS_APPL_STREAM_RC_INVALID_SNAP_OUI_TYPE:
        return "Invalid SNAP OUI type";

    case VTSS_APPL_STREAM_RC_INVALID_SNAP_PID:
        return "If OUI is 00:00:00 (RFC-1042), the PID must be in range of EtherTypes (0x600 - 0xFFFF)";

    case VTSS_APPL_STREAM_RC_INVALID_IPV4_FRAGMENT:
        return "Invalid IPv4 fragment";

    case VTSS_APPL_STREAM_RC_IPV4_DSCP_OUT_OF_RANGE:
        return "IPv4's DSCP value is out of range (0-63)";

    case VTSS_APPL_STREAM_RC_IPV4_DSCP_LOW_OUT_OF_RANGE:
        return "IPv4's DSCP range's low value is out of range (0-63)";

    case VTSS_APPL_STREAM_RC_IPV4_DSCP_HIGH_OUT_OF_RANGE:
        return "IPv4's DSCP range's high value is out of range (0-63)";

    case VTSS_APPL_STREAM_RC_IPV4_DSCP_HIGH_SMALLER_THAN_LOW:
        return "IPv4's DSCP's high range value is smaller than the low range value";

    case VTSS_APPL_STREAM_RC_IPV4_DSCP_MATCH_TYPE:
        return "Invalid IPv4 DSCP match type value";

    case VTSS_APPL_STREAM_RC_INVALID_IPV4_PROTO_TYPE:
        return "IPv4's protocol type is invalid";

    case VTSS_APPL_STREAM_RC_INVALID_IPV4_SIP_PREFIX_SIZE:
        return "IPv4's source IP's prefix size out of range (0-32)";

    case VTSS_APPL_STREAM_RC_INVALID_IPV4_DIP_PREFIX_SIZE:
        return "IPv4's destination IP's prefix size out of range (0-32)";

    case VTSS_APPL_STREAM_RC_IPV4_DPORT_HIGH_SMALLER_THAN_LOW:
        return "IPv4's UDP/TCP destination port's high range value is smaller than the low range value";

    case VTSS_APPL_STREAM_RC_IPV4_DPORT_MATCH_TYPE:
        return "Invalid IPv4 UDP/TCP destination port match type value";

    case VTSS_APPL_STREAM_RC_IPV6_DSCP_OUT_OF_RANGE:
        return "IPv6's DSCP value is out of range (0-63)";

    case VTSS_APPL_STREAM_RC_IPV6_DSCP_LOW_OUT_OF_RANGE:
        return "IPv6's DSCP range's low value is out of range (0-63)";

    case VTSS_APPL_STREAM_RC_IPV6_DSCP_HIGH_OUT_OF_RANGE:
        return "IPv6's DSCP range's high value is out of range (0-63)";

    case VTSS_APPL_STREAM_RC_IPV6_DSCP_HIGH_SMALLER_THAN_LOW:
        return "IPv6's DSCP's high range value is smaller than the low range value";

    case VTSS_APPL_STREAM_RC_IPV6_DSCP_MATCH_TYPE:
        return "Invalid IPv6 DSCP match type value";

    case VTSS_APPL_STREAM_RC_INVALID_IPV6_PROTO_TYPE:
        return "IPv6's protocol type is invalid";

    case VTSS_APPL_STREAM_RC_INVALID_IPV6_SIP_PREFIX_SIZE:
        return "IPv6's source IP's prefix size out of range (0-128)";

    case VTSS_APPL_STREAM_RC_INVALID_IPV6_DIP_PREFIX_SIZE:
        return "IPv6's destination IP's prefix size out of range (0-128)";

    case VTSS_APPL_STREAM_RC_IPV6_DPORT_HIGH_SMALLER_THAN_LOW:
        return "IPv6's UDP/TCP destination port's high range value is smaller than the low range value";

    case VTSS_APPL_STREAM_RC_IPV6_DPORT_MATCH_TYPE:
        return "Invalid IPv6 UDP/TCP destination port match type value";

    case VTSS_APPL_STREAM_RC_INVALID_PROTOCOL_TYPE:
        return "Invalid protocol type";

    case VTSS_APPL_STREAM_RC_OUT_OF_MEMORY:
        return "Out of memory";

    case VTSS_APPL_STREAM_RC_COUNTERS_NOT_ALLOCATED:
        return "Stream counters are not allocated, because no clients are attached";

    case VTSS_APPL_STREAM_RC_PART_OF_COLLECTION:
        return "Stream is part of a stream collection. Cannot attach client directly";

    case VTSS_APPL_STREAM_RC_COLLECTION_INVALID_ID:
        return "Invalid stream collection ID";

    case VTSS_APPL_STREAM_RC_COLLECTION_NO_SUCH_ID:
        return "No such stream collection ID";

    case VTSS_APPL_STREAM_RC_COLLECTION_STREAM_ID_DOESNT_EXIST:
        return "At least one of the configured stream IDs doesn't exist";

    case VTSS_APPL_STREAM_RC_COLLECTION_STREAM_PART_OF_OTHER_COLLECTION:
        return "At least one of the configured stream IDs is already part of another stream collection";

    case VTSS_APPL_STREAM_RC_COLLECTION_COUNTERS_NOT_ALLOCATED:
        return "Stream collection counters are not allocated, because no clients are attached";

    default:
        T_E("Stream: Unknown error code (%d = 0x%08x)", rc, rc);
        return "Unknown";
    }
}

extern "C" int stream_icli_cmd_register(void);

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void stream_mib_init(void);
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void stream_json_init(void);
#endif

/******************************************************************************/
// stream_init()
/******************************************************************************/
mesa_rc stream_init(vtss_init_data_t *data)
{
    vtss_appl_stream_action_t &psfp = STREAM_action_default[VTSS_APPL_STREAM_CLIENT_PSFP];
    vtss_appl_stream_action_t &frer = STREAM_action_default[VTSS_APPL_STREAM_CLIENT_FRER];
    mesa_vce_t                vce;
    mesa_iflow_id_t           dummy_iflow_id;
    mesa_rc                   rc;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        // Create a mutex
        critd_init(&STREAM_crit, "stream", VTSS_MODULE_ID_STREAM, CRITD_TYPE_MUTEX);

        // Create a default mesa_vce_action_t instance.
        if ((rc = mesa_vce_init(nullptr, MESA_VCE_TYPE_ANY, &vce)) != VTSS_RC_OK) {
            T_E("mesa_vce_init() failed: %s", error_txt(rc));
            vtss_clear(vce);
        }

        STREAM_vce_action_default = vce.action;

        // Get a default iflow configuration, that we use when a client detaches
        // from a stream.
        // First allocate a temporary IFLOW ID.
        if ((rc = mesa_iflow_alloc(nullptr, &dummy_iflow_id)) != VTSS_RC_OK) {
            // At this point in time, we must be able to allocate such an ID, as
            // we cannot have run dry of H/W resources by now.
            T_EG(STREAM_TRACE_GRP_API, "mesa_iflow_alloc() failed: %s", error_txt(rc));
            return VTSS_RC_ERROR;
        }

        // Then get its configuration, which is the default we are looking for.
        if ((rc = mesa_iflow_conf_get(nullptr, dummy_iflow_id, &STREAM_iflow_conf_default)) != VTSS_RC_OK) {
            T_EG(STREAM_TRACE_GRP_API, "mesa_iflow_conf_get(%u) failed: %s", dummy_iflow_id, error_txt(rc));
            // Go on anyway.
        }

        // Free the temporary IFLOW ID again.
        if ((rc = mesa_iflow_free(nullptr, dummy_iflow_id)) != VTSS_RC_OK) {
            T_EG(STREAM_TRACE_GRP_API, "mesa_iflow_free(%u) failed: %s", dummy_iflow_id, error_txt(rc));
        }

        // Also create defaults for our two clients used when they detach from
        // a stream.
        psfp.cut_through_disable = STREAM_iflow_conf_default.cut_through_disable;
        psfp.psfp.dlb_enable     = STREAM_iflow_conf_default.dlb_enable;
        psfp.psfp.dlb_id         = STREAM_iflow_conf_default.dlb_id;
        psfp.psfp.psfp           = STREAM_iflow_conf_default.psfp;

        frer.cut_through_disable = STREAM_iflow_conf_default.cut_through_disable;
        frer.frer.vid            = STREAM_vce_action_default.vid;
        frer.frer.pop_enable     = STREAM_vce_action_default.pop_enable;
        frer.frer.pop_cnt        = STREAM_vce_action_default.pop_cnt;
        frer.frer.frer           = STREAM_iflow_conf_default.frer;

        STREAM_capabilities_set();
        STREAM_collection_capabilities_set();

        stream_icli_cmd_register();

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        stream_mib_init();
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC)
        stream_json_init();
#endif /* defined(VTSS_SW_OPTION_JSON_RPC) */
        break;

    case INIT_CMD_START:
#ifdef VTSS_SW_OPTION_ICFG
        mesa_rc stream_icfg_init(void);
        VTSS_RC(stream_icfg_init()); // ICFG initialization (show running-config)
#endif
        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_GLOBAL) {
            STREAM_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        STREAM_default();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

