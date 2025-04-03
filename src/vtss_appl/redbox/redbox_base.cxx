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

#include <vtss/appl/redbox.h>
#include "redbox_base.hxx"
#include "redbox_pdu.hxx"
#include "redbox_trace.h"
#include "redbox_lock.hxx"   // For REDBOX_LOCK_SCOPE()
#include "redbox_expose.hxx" // For redbox_notification_status
#include "acl_api.h"         // For acl_mgmt_ace_XXX()
#include "misc_api.h"        // For misc_mac_txt()

// We keep a set of global counters for SV frame reception (good and bad), per
// port.
// The "good_XXX" counters count even if receiving a SV frame on an LRE port
// where the RedBox is in a state that doesn't expect that type.
typedef struct {
    // Number of good and bad SV frames per type
    uint64_t cnt[VTSS_APPL_REDBOX_SV_TYPE_CNT];

    // Number of erroneous SV frames - either because of erroneous content, or
    // because TLV1.MAC or TLV2.MAC is our own.
    uint64_t err_cnt;

    // Number of SV frames that got ingress filtered or there were no
    // destination ports for a SV frame received on an LRE port for a RedBox in
    // HSR-PRP mode.
    uint64_t filtered_cnt;
} redbox_base_sv_rx_port_cnt_t;

static CapArray<redbox_base_sv_rx_port_cnt_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> REDBOX_BASE_sv_port_cnt;

CapArray<redbox_port_role_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> redbox_port_roles;

// For RedBoxes in HSR-PRP mode, we need to get PRP SV frames to the CPU, so we
// install an ACL rule that floods PRP SV frames in the VLAN they got classified
// except for RedBox interlink ports AND copies it to the CPU.
// We have one single instance that covers all redbox instances, and it is only
// active if at least one redbox is in HSR-PRP mode.
mesa_ace_id_t REDBOX_BASE_ace_id = ACL_MGMT_ACE_ID_NONE;

// We register for various kind of frame Rx.
typedef enum {
    // In PRP mode, we need SV frames arriving on an LRE port to be redirected
    // to the CPU.
    REDBOX_BASE_RX_FILTER_TYPE_PRP_SV,

    // In any HSR mode, we need SV frames arriving on an LRE port with an HSR
    // tag to be copied to the CPU.
    REDBOX_BASE_RX_FILTER_TYPE_HSR_SV,

    // Required number of filters.
    REDBOX_BASE_RX_FILTER_TYPE_CNT
} redbox_base_rx_filter_type_t;

// Allow enum++ operations on the filter type
VTSS_ENUM_INC(redbox_base_rx_filter_type_t);

// We have one global filter per filter type, which covers all RedBox instances.
static struct {
    packet_rx_filter_t filter;
    void               *filter_id;
} REDBOX_BASE_rx_filters[REDBOX_BASE_RX_FILTER_TYPE_CNT];

/******************************************************************************/
// REDBOX_BASE_port_role_type_to_str()
/******************************************************************************/
static const char *REDBOX_BASE_port_role_type_to_str(redbox_port_role_type_t role)
{
    switch (role) {
    case REDBOX_PORT_ROLE_TYPE_NORMAL:
        return "Normal";

    case REDBOX_PORT_ROLE_TYPE_INTERLINK:
        return "Interlink";

    case REDBOX_PORT_ROLE_TYPE_UNCONNECTED:
        return "Unconnected";

    default:
        T_EG(REDBOX_TRACE_GRP_BASE, "Unknown role (%d)", role);
        return "Unknown";
    }
}

/******************************************************************************/
// REDBOX_BASE_rx_filter_type_to_str()
/******************************************************************************/
static const char *REDBOX_BASE_rx_filter_type_to_str(redbox_base_rx_filter_type_t filter_type)
{
    switch (filter_type) {
    case REDBOX_BASE_RX_FILTER_TYPE_PRP_SV:
        return "PRP SV";

    case REDBOX_BASE_RX_FILTER_TYPE_HSR_SV:
        return "HSR SV";

    default:
        T_EG(REDBOX_TRACE_GRP_FRAME_RX, "Unknown filter_type (%d)", filter_type);
        return "Unknown";
    }
}

/******************************************************************************/
// REDBOX_BASE_mesa_mode_to_str()
/******************************************************************************/
static const char *REDBOX_BASE_mesa_mode_to_str(mesa_rb_mode_t mode)
{
    switch (mode) {
    case MESA_RB_MODE_DISABLED:
        return "Disabled";

    case MESA_RB_MODE_PRP_SAN:
        return "PRP-SAN";

    case MESA_RB_MODE_HSR_SAN:
        return "HSR-SAN";

    case MESA_RB_MODE_HSR_PRP:
        return "HSR-PRP";

    case MESA_RB_MODE_HSR_HSR:
        return "HSR-HSR";

    default:
        T_EG(REDBOX_TRACE_GRP_API, "Invalid mode (%d)", mode);
        return "Unknown";
    }
}

/******************************************************************************/
// REDBOX_BASE_mesa_sv_to_str()
/******************************************************************************/
static const char *REDBOX_BASE_mesa_sv_to_str(mesa_rb_sv_t sv)
{
    switch (sv) {
    case MESA_RB_SV_FORWARD:
        return "Forward";

    case MESA_RB_SV_DISCARD:
        return "Discard";

    case MESA_RB_SV_CPU_COPY:
        return "CPU Copy";

    case MESA_RB_SV_CPU_ONLY:
        return "CPU Redirect";

    default:
        T_EG(REDBOX_TRACE_GRP_API, "Invalid sv (%d)", sv);
        return "Unknown";
    }
}

/******************************************************************************/
// REDBOX_BASE_mesa_node_type_to_str()
/******************************************************************************/
static const char *REDBOX_BASE_mesa_node_type_to_str(mesa_rb_node_type_t type)
{
    switch (type) {
    case MESA_RB_NODE_TYPE_DAN:
        return "DANP/DANH";

    case MESA_RB_NODE_TYPE_SAN:
        return "SAN";

    default:
        T_EG(REDBOX_TRACE_GRP_API, "Invalid node-type (%d)", type);
        return "Unknown";
    }
}

/******************************************************************************/
// REDBOX_BASE_mesa_proxy_node_type_to_str()
/******************************************************************************/
static const char *REDBOX_BASE_mesa_proxy_node_type_to_str(mesa_rb_proxy_node_type_t type)
{
    switch (type) {
    case MESA_RB_PROXY_NODE_TYPE_DAN:
        return "DANP";

    case MESA_RB_PROXY_NODE_TYPE_SAN:
        return "SAN";

    default:
        T_EG(REDBOX_TRACE_GRP_API, "Invalid proxy-node-type (%d)", type);
        return "Unknown";
    }
}

/******************************************************************************/
// mesa_rb_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_rb_conf_t &conf)
{
    o << "{mode = "             << REDBOX_BASE_mesa_mode_to_str(conf.mode)
      << ", port_a = "          << conf.port_a
      << ", port_b = "          << conf.port_b
      << ", net_id = "          << conf.net_id
      << ", lan_id = "          << conf.lan_id
      << ", nt_dmac_disable = " << conf.nt_dmac_disable
      << ", nt_age_time = "     << conf.nt_age_time
      << ", pnt_age_time = "    << conf.pnt_age_time
      << ", dd_age_time = "     << conf.dd_age_time
      << ", sv = "              << REDBOX_BASE_mesa_sv_to_str(conf.sv)
      << "}";

    return o;
}

/******************************************************************************/
// mesa_rb_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_rb_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_rb_proxy_node_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_rb_proxy_node_conf_t &conf)
{
    o << "{type = "    << REDBOX_BASE_mesa_proxy_node_type_to_str(conf.type)
      << ", locked = " << conf.locked
      << "}";

    return o;
}

/******************************************************************************/
// mesa_rb_node_counters_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_rb_node_counters_t &c)
{
    o << "{rx = " << c.rx
      << "{, rx_wrong_lan =" << c.rx_wrong_lan
      << "}";

    return o;
}

/******************************************************************************/
// mesa_rb_proxy_node_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_rb_proxy_node_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_rb_node_port_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_rb_node_port_t &np)
{
    o << "{fwd = "  << np.fwd
      << ", age = " << np.age
      << ", cnt = " << np.cnt // Using mesa_rb_node_counters_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_rb_node_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_rb_node_t &node)
{
    char mac_buf[20];

    o << "{mac = "     << misc_mac_txt(node.mac.addr, mac_buf)
      << ", id = "     << node.id
      << ", locked = " << node.locked
      << ", type = "   << REDBOX_BASE_mesa_node_type_to_str(node.type)
      << ", port_a = " << node.port_a // Using mesa_rb_node_port_t::operator<<()
      << ", port_b = " << node.port_b // Using mesa_rb_node_port_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_rb_node_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_rb_node_t *node)
{
    o << *node;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_rb_proxy_node_counters_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_rb_proxy_node_counters_t &c)
{
    o << "{rx = " << c.rx
      << "{, rx_wrong_lan =" << c.rx_wrong_lan
      << "}";

    return o;
}

/******************************************************************************/
// mesa_rb_proxy_node_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_rb_proxy_node_t &node)
{
    char mac_buf[20];

    o << "{mac = "     << misc_mac_txt(node.mac.addr, mac_buf)
      << ", id = "     << node.id
      << ", locked = " << node.locked
      << ", type = "   << REDBOX_BASE_mesa_proxy_node_type_to_str(node.type)
      << ", age = "    << node.age
      << ", cnt = "    << node.cnt // Using mesa_rb_proxy_node_counters_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_rb_proxy_node_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_rb_proxy_node_t *node)
{
    o << *node;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_redbox_mac_port_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_redbox_mac_port_status_t &p)
{
    o << "{rx_cnt = "            << p.rx_cnt
      << ", last_seen_secs = "   << p.last_seen_secs
      << ", sv_rx_cnt = "        << p.sv_rx_cnt
      << ", sv_last_seen_secs"   << p.sv_last_seen_secs
      << ", sv_last_type = "     << redbox_util_sv_type_to_str(p.sv_last_type)
      << ", rx_wrong_lan_cnt = " << p.rx_wrong_lan_cnt
      << ", fwd = "              << p.fwd
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_redbox_nt_mac_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_redbox_nt_mac_status_t &s)
{
    o << "{node_type = "       << redbox_util_node_type_to_str(s.node_type)
      << ", port_a = "         << s.port[VTSS_APPL_REDBOX_PORT_TYPE_A] // Using vtss_appl_redbox_mac_port_status_t::operator<<()
      << ", port_b = "         << s.port[VTSS_APPL_REDBOX_PORT_TYPE_B] // Using vtss_appl_redbox_mac_port_status_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_redbox_pnt_mac_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_redbox_pnt_mac_status_t &s)
{
    o << "{node_type = "          << redbox_util_node_type_to_str(s.node_type)
      << ", port = "              << s.port // Using vtss_appl_redbox_mac_port_status_t::operator<<()
      << ", sv_tx_cnt = "         << s.sv_tx_cnt
      << ", locked = "            << s.locked
      << "}";

    return o;
}

/******************************************************************************/
// redbox_nt_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const redbox_nt_t &nt)
{
    o << "{status = "         << nt.status    // Using vtss_appl_redbox_nt_mac_status_t::operator<<()
      << ", present_in_nt = " << nt.present_in_nt
      << "}";

    return o;
}

/******************************************************************************/
// redbox_nt_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const redbox_nt_t *nt)
{
    o << *nt;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// redbox_pnt_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const redbox_pnt_t &pnt)
{
    o << "{status = "          << pnt.status         // Using vtss_appl_redbox_pnt_nac_status_t::operator<<()
      << ", present_in_pnt = " << pnt.present_in_pnt
      << "}";

    return o;
}

/******************************************************************************/
// redbox_pnt_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const redbox_pnt_t *pnt)
{
    o << *pnt;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_redbox_port_notification_status_t::operator<<()
// Used for tracing
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_redbox_port_notification_status_t &p)
{
    o << "{down = "               << p.down
      << ", hsr_untagged_rx = "   << p.hsr_untagged_rx
      << ", cnt_err_wrong_lan = " << p.cnt_err_wrong_lan
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_redbox_notification_status_t::operator<<()
// Used for tracing
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_redbox_notification_status_t &s)
{
    vtss_appl_redbox_port_type_t port_type;

    o << "{";

    for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_C; port_type++) {
        o << (port_type == VTSS_APPL_REDBOX_PORT_TYPE_A ? "" : ", ") << "port[" << redbox_util_port_type_to_str(port_type) << "] = " << s.port[port_type];
    }

    o << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_redbox_notification_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_redbox_notification_status_t *s)
{
    o << *s;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// REDBOX_BASE_notification_status_update()
/******************************************************************************/
static void REDBOX_BASE_notification_status_update(redbox_state_t &redbox_state, bool get_before_set = true)
{
    vtss_appl_redbox_port_type_t           port_type;
    vtss_appl_redbox_notification_status_t old_notif_status, notif_status = {};
    mesa_rc                                rc;

    for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_C; port_type++) {
        notif_status.port[port_type].cnt_err_wrong_lan = redbox_timer_active(redbox_state.cnt_err_wrong_lan_timers[port_type]);
        notif_status.port[port_type].hsr_untagged_rx   = redbox_timer_active(redbox_state.hsr_untagged_rx_timers[port_type]);

        if (port_type == VTSS_APPL_REDBOX_PORT_TYPE_C) {
            // The two othes are not active for Port C and are their
            // notifications are already reset to false.
            break;
        }

        notif_status.port[port_type].down = !redbox_state.port_link_get(port_type);
    }

    notif_status.nt_pnt_full = redbox_state.mac_map.size() >= REDBOX_cap.nt_pnt_size;

    if (get_before_set && (rc = redbox_notification_status.get(redbox_state.inst, &old_notif_status)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_NOTIF, "%u: Unable to get notification status: %s", redbox_state.inst, error_txt(rc));
    }

    if (!get_before_set || old_notif_status != notif_status) {
        T_IG(REDBOX_TRACE_GRP_NOTIF, "%u: notif_status = %s", redbox_state.inst, notif_status);
        if ((rc = redbox_notification_status.set(redbox_state.inst, &notif_status)) != VTSS_RC_OK) {
            T_EG(REDBOX_TRACE_GRP_NOTIF, "%u: Unable to update notification status with %s: %s", redbox_state.inst, notif_status, error_txt(rc));
        }
    }
}

/******************************************************************************/
// REDBOX_BASE_find_inst_by_port()
/******************************************************************************/
static redbox_state_t *REDBOX_BASE_find_inst_by_port(mesa_port_no_t rx_port_no)
{
    redbox_itr_t redbox_itr;

    // Find the RedBox instance with port A = rx_port_no
    for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
        if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
            continue;
        }

        // The frame arrives on the I/L which is personalized by either Port A
        // or Port B.
        if (rx_port_no == redbox_itr->second.interlink_port_no_get()) {
            return &redbox_itr->second;
        }
    }

    // It could be that a frame was in the pipeline while the corresponding
    // RedBox instance got disabled. This seems to be the case here.
    return nullptr;
}

/******************************************************************************/
// REDBOX_BASE_ace_add()
/******************************************************************************/
static mesa_rc REDBOX_BASE_ace_add(uint32_t inst, mesa_ace_id_t &ace_id, acl_entry_conf_t &ace_conf)
{
    mesa_ace_id_t old_ace_id = ace_conf.id; // Get a copy of the current ACE ID for tracing
    mesa_rc       rc;

    // Update or create a rule and insert it last (as it doesn't matter).
    if ((rc = acl_mgmt_ace_add(ACL_USER_REDBOX, ACL_MGMT_ACE_ID_NONE, &ace_conf)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_BASE, "%u: acl_mgmt_ace_add() failed: %s", inst, error_txt(rc));
        return VTSS_APPL_REDBOX_RC_HW_RESOURCES;
    }

    T_IG(REDBOX_TRACE_GRP_BASE, "%u: acl_mgmt_ace_add(old_ace_id = 0x%x, action.port_list = %s) => ace_id = 0x%x", inst, old_ace_id, ace_conf.action.port_list, ace_conf.id);
    ace_id = ace_conf.id;

    return VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_BASE_ace_del()
/******************************************************************************/
static void REDBOX_BASE_ace_del(uint32_t inst, mesa_ace_id_t &ace_id)
{
    mesa_rc rc;

    if (ace_id == ACL_MGMT_ACE_ID_NONE) {
        // Nothing to remove
        return;
    }

    T_IG(REDBOX_TRACE_GRP_BASE, "%u: acl_mgmt_ace_del(0x%x)", inst, ace_id);
    if ((rc = acl_mgmt_ace_del(ACL_USER_REDBOX, ace_id)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_BASE, "%u: acl_mgmt_ace_del(0x%x failed: %s)", inst, ace_id, error_txt(rc));
    }

    ace_id = ACL_MGMT_ACE_ID_NONE;
}

/******************************************************************************/
// REDBOX_BASE_rb_mac_discard()
/******************************************************************************/
static mesa_rc REDBOX_BASE_rb_mac_discard(redbox_state_t &redbox_state, bool add)
{
    acl_entry_conf_t ace_conf;
    mesa_rc          rc;

    // RBNTBD: Do we really need to add the RB's own MAC as a discard ACE?
    // Doing so may make it harder to actually test.
    //    return VTSS_RC_OK;

    // We cannot use mesa_mac_table_add(), because we need to discard frames in
    // all VLANs. mesa_mac_table_add() only supports a particular VLAN.
    // Therefore, we use an ACE.

    if (!add) {
        // Remove it.
        REDBOX_BASE_ace_del(redbox_state.inst, redbox_state.rb_mac_discard_ace_id);
        return VTSS_RC_OK;
    }

    if (redbox_state.rb_mac_discard_ace_id != ACL_MGMT_ACE_ID_NONE) {
        // How can this be?
        T_EG(REDBOX_TRACE_GRP_BASE, "%u: ace_id (%u) != %u", redbox_state.inst, redbox_state.rb_mac_discard_ace_id, ACL_MGMT_ACE_ID_NONE);
        REDBOX_BASE_ace_del(redbox_state.inst, redbox_state.rb_mac_discard_ace_id);
    }

    // Create it.
    T_IG(REDBOX_TRACE_GRP_BASE, "acl_mgmt_ace_init()");

    // Unfortunately, we need to pick TYPE_ETYPE, because TYPE_ANY doesn't take
    // any particular fields. RBNTBD: I think IPv4, IPv6, and ARP frames aren't
    // caught by this rule.
    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ETYPE, &ace_conf)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_BASE, "acl_mgmt_ace_init() failed: %s", error_txt(rc));
    }

    // Fill in invariable fields
    ace_conf.isid = VTSS_ISID_LOCAL;

    // Match on RB MAC as DMAC.
    memcpy(ace_conf.frame.etype.dmac.value, redbox_state.interlink_port_state_get()->redbox_mac.addr, sizeof(ace_conf.frame.etype.dmac.value));
    memset(ace_conf.frame.etype.dmac.mask, 0xff, sizeof(ace_conf.frame.etype.dmac.mask));

    // Match on all ports. We don't match on VLAN ID, because that would
    // require up to 4K ACEs.
    ace_conf.port_list.set_all();

    // And configure the ACE to use action.port_list for forwarding
    ace_conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;

    // Don't send anywhere.
    ace_conf.action.port_list.clear_all();

    return REDBOX_BASE_ace_add(redbox_state.inst, redbox_state.rb_mac_discard_ace_id, ace_conf);
}

/******************************************************************************/
// REDBOX_BASE_mesa_nt_add()
/******************************************************************************/
static void REDBOX_BASE_mesa_nt_add(redbox_state_t &redbox_state, redbox_mac_itr_t &mac_itr)
{
    mesa_rb_node_conf_t mesa_nt_entry_conf = {};
    mesa_rc             rc;

    // We always need to add the entry to the NT - even if it is identical to
    // what is already in the NT. The reason is that reception of an SV frame
    // shall cause the age time to reset to 0, so that it doesn't time out until
    // nt_age_time has elapsed. The counters, however, must not change.

    // We always add DANs, so the entry's san_a doesn't matter.
    // Also, we never set the locked flag for an NT entry.
    mesa_nt_entry_conf.type = MESA_RB_NODE_TYPE_DAN;

    T_IG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_node_add(%u, %s)", redbox_state.inst, redbox_state.rb_id_get(), mac_itr->first);
    if ((rc = mesa_rb_node_add(nullptr, redbox_state.rb_id_get(), &mac_itr->first, &mesa_nt_entry_conf)) != VTSS_RC_OK) {
        // NodesTable is probably full. We cannot throw a trace error here,
        // because this may have happened while we were processing the SV frame.
        T_IG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_node_add(%u, %s) failed: %s", redbox_state.inst, redbox_state.rb_id_get(), mac_itr->first, error_txt(rc));
    }
}

/******************************************************************************/
// REDBOX_BASE_mesa_nt_del()
/******************************************************************************/
static void REDBOX_BASE_mesa_nt_del(redbox_state_t &redbox_state, redbox_mac_itr_t &mac_itr)
{
    mesa_rc rc;

    T_IG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_node_del(%u, %s)", redbox_state.inst, redbox_state.rb_id_get(), mac_itr->first);
    if ((rc = mesa_rb_node_del(nullptr, redbox_state.rb_id_get(), &mac_itr->first)) != VTSS_RC_OK) {
        // Could be it has timed out since table was last polled, so don't throw
        // a trace error.
        T_IG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_node_del(%u, %s) failed: %s", redbox_state.inst, redbox_state.rb_id_get(), mac_itr->first, error_txt(rc));
    }
}

/******************************************************************************/
// REDBOX_BASE_mesa_nt_clear()
/******************************************************************************/
static void REDBOX_BASE_mesa_nt_clear(redbox_state_t &redbox_state, bool include_locked)
{
    mesa_rc rc;

    T_IG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_node_table_clear(id = %u, all = %d)", redbox_state.inst, redbox_state.rb_id_get(), include_locked);
    if ((rc = mesa_rb_node_table_clear(nullptr, redbox_state.rb_id_get(), include_locked ? MESA_RB_CLEAR_ALL : MESA_RB_CLEAR_UNLOCKED)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_node_table_clear(id = %u, all = %d) failed: %s", redbox_state.inst, redbox_state.rb_id_get(), include_locked, error_txt(rc));
    }
}

/******************************************************************************/
// REDBOX_BASE_mesa_pnt_add()
/******************************************************************************/
static mesa_rc REDBOX_BASE_mesa_pnt_add(redbox_state_t &redbox_state, const mesa_mac_t &mac, mesa_rb_proxy_node_type_t node_type, bool locked)
{
    mesa_rb_proxy_node_conf_t pnt_conf;
    mesa_rc                   rc;

    vtss_clear(pnt_conf);

    pnt_conf.type   = node_type;
    pnt_conf.locked = locked;

    T_IG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_proxy_node_add(id = %u, mac = %s, conf = %s)", redbox_state.inst, redbox_state.rb_id_get(), mac, pnt_conf);
    if ((rc = mesa_rb_proxy_node_add(nullptr, redbox_state.rb_id_get(), &mac, &pnt_conf)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_proxy_node_add(id = %u, mac= %s, conf = %s) failed: %s", redbox_state.inst, redbox_state.rb_id_get(), mac, pnt_conf, error_txt(rc));
        return VTSS_APPL_REDBOX_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_BASE_mesa_pnt_del()
/******************************************************************************/
static void REDBOX_BASE_mesa_pnt_del(redbox_state_t &redbox_state, redbox_mac_itr_t &mac_itr)
{
    mesa_rc rc;

    T_IG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_proxy_node_del(%u, %s)", redbox_state.inst, redbox_state.rb_id_get(), mac_itr->first);
    if ((rc = mesa_rb_proxy_node_del(nullptr, redbox_state.rb_id_get(), &mac_itr->first)) != VTSS_RC_OK) {
        // Could be it has timed out since table was last polled, so don't throw
        // a trace error.
        T_IG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_proxy_node_del(%u, %s) failed: %s", redbox_state.inst, redbox_state.rb_id_get(), mac_itr->first, error_txt(rc));
    }
}

/******************************************************************************/
// REDBOX_BASE_mesa_pnt_clear()
/******************************************************************************/
static void REDBOX_BASE_mesa_pnt_clear(redbox_state_t &redbox_state, bool include_locked)
{
    mesa_rc rc;

    T_IG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_proxy_node_table_clear(id = %u, all = %d)", redbox_state.inst, redbox_state.rb_id_get(), include_locked);
    if ((rc = mesa_rb_proxy_node_table_clear(nullptr, redbox_state.rb_id_get(), include_locked ? MESA_RB_CLEAR_ALL : MESA_RB_CLEAR_UNLOCKED)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_proxy_node_table_clear(id = %u, all = %d) failed: %s", redbox_state.inst, redbox_state.rb_id_get(), include_locked, error_txt(rc));
    }
}

/******************************************************************************/
// REDBOX_BASE_nt_pnt_add()
//
// This function gets called whenever a valid SV frame arrives and the NT or PNT
// needs an update.
//
// When adding to NT:
//   This happens in all modes whenever a valid SV frame is received on an LRE
//   port. We always update the NT to get the age time down to zero, and we
//   never save an entry as locked.
//
// When adding to PNT:
//   This only happens in HSR-PRP mode when a valid SV frame is received from
//   the PRP network. We always update the PNT to get the age time down to zero,
//   and we never save an entry as locked (the only two locked entries are our
//   management MAC address and the RB's own MAC address, which are added
//   directly through REDBOX_BASE_mesa_pnt_add()).
//
// An SV frame with a TLV1.MAC or TLV2.MAC that matches one of our own is
// considered invalid, and will not give rise to a call to this function.
/******************************************************************************/
static void REDBOX_BASE_nt_pnt_add(redbox_state_t &redbox_state, const redbox_pdu_info_t &rx_pdu_info, bool tlv2, bool add_to_pnt)
{
    redbox_mac_itr_t mac_itr;
    const mesa_mac_t &mac = tlv2 ? rx_pdu_info.tlv2_mac : rx_pdu_info.tlv1_mac;

    if (tlv2 && !rx_pdu_info.tlv2_present()) {
        // Asked to add TLV2, but TLV2 is not present in the SV frame.
        return;
    }

    if (add_to_pnt && redbox_state.conf.mode != VTSS_APPL_REDBOX_MODE_HSR_PRP) {
        T_EG(REDBOX_TRACE_GRP_FRAME_RX, "%u: S/W cannot add a MAC to the PNT unless it's in HSR-PRP mode, and it's not (%s)", redbox_state.inst, redbox_util_mode_to_str(redbox_state.conf.mode));
        return;
    }

    // Update our S/W-based PNT/NT first.
    if ((mac_itr = redbox_state.mac_map.find(mac)) == redbox_state.mac_map.end()) {
        // New entry

        // Make an upper limit on the number of entries, we can have in the NT.
        if (redbox_state.mac_map.size() >= REDBOX_cap.nt_pnt_size) {
            // Update notifications
            REDBOX_BASE_notification_status_update(redbox_state);
            T_IG(REDBOX_TRACE_GRP_FRAME_RX, "%u: NT/PNT full. Discarding %s", redbox_state.inst, mac);
            return;
        }

        if ((mac_itr = redbox_state.mac_map.get(mac)) == redbox_state.mac_map.end()) {
            T_EG(REDBOX_TRACE_GRP_FRAME_RX, "%u: Out of memory when attempting to add %s", redbox_state.inst, mac);
            // RBNTBD: Update operational state?
            return;
        }

        vtss_clear(mac_itr->second);
    } else {
        // Entry already exists.

        if (add_to_pnt) {
            if (mac_itr->second.is_pnt_entry) {
                // Since we are now receiving SV frames from the PRP network and
                // either forwarding as is or translating to HSR SV frames on
                // the HSR ring, there is no need for us to send SV frames on
                // behalf of this entry, so simply stop it.
                redbox_pdu_tx_stop(redbox_state, mac_itr);
            } else {
                // This is a MAC move from NT to PNT. This is handled below.
                vtss_clear(mac_itr->second);
            }
        } else {
            // If we are moving it from PNT to NT, we need to stop Tx of SV
            // frames for this proxied SAN.
            if (mac_itr->second.is_pnt_entry) {
                redbox_pdu_tx_stop(redbox_state, mac_itr);
                vtss_clear(mac_itr->second);
            }
        }
    }

    mac_itr->second.is_pnt_entry = add_to_pnt;
    if (add_to_pnt) {
        // Add to PNT
        // Update the node type. We know we are in HSR-PRP mode.
        vtss_appl_redbox_node_type_t &node_type = mac_itr->second.pnt.status.node_type;

        // Once a TLV2 is seen, it cannot change to anything else. Suppose an
        // RB sends both SV frames for a VDANx (TLV2 present) and its own DANx
        // SV frames (no TLV2 present), then the node type would change from
        // DANx to DANx_RB when we receive its own DANx SV frame and back to RB
        // when we receive the VDANx SV frame. Since we know it's a RB, we make
        // the node type "sticky".
        if (node_type != VTSS_APPL_REDBOX_NODE_TYPE_DANP_RB) {
            if (tlv2) {
                // We are adding a RB node.
                node_type = VTSS_APPL_REDBOX_NODE_TYPE_DANP_RB;
            } else {
                // We are adding a DANP of some sort based on TLV1.
                if (rx_pdu_info.tlv2_present()) {
                    // We are adding a VDANP.
                    node_type = VTSS_APPL_REDBOX_NODE_TYPE_VDANP;
                } else {
                    // We are adding a real DANP
                    node_type = VTSS_APPL_REDBOX_NODE_TYPE_DANP;
                }
            }
        }

        // We always change it to a DAN H/W entry
        REDBOX_BASE_mesa_pnt_add(redbox_state, mac_itr->first, MESA_RB_PROXY_NODE_TYPE_DAN, false /* add non-locked */);
    } else {
        // Add to NT
        if (rx_pdu_info.port_type != VTSS_APPL_REDBOX_PORT_TYPE_A && rx_pdu_info.port_type != VTSS_APPL_REDBOX_PORT_TYPE_B) {
            T_EG(REDBOX_TRACE_GRP_BASE, "%u: Expected port type A or B, but got %s", redbox_state.inst, redbox_util_port_type_to_str(rx_pdu_info.port_type));
            return;
        }

        // Update the node type.
        vtss_appl_redbox_node_type_t &node_type = mac_itr->second.nt.status.node_type;

        // Once a TLV2 is seen, it cannot change to anything else. Suppose an
        // RB sends both SV frames for a VDANx (TLV2 present) and its own DANx
        // SV frames (no TLV2 present), then the node type would change from
        // DANx to DANx_RB when we receive its own DANx SV frame and back to RB
        // when we receive the VDANx SV frame. Since we know it's a RB, we make
        // the node type "sticky".
        if (node_type != VTSS_APPL_REDBOX_NODE_TYPE_DANH_RB && node_type != VTSS_APPL_REDBOX_NODE_TYPE_DANP_RB) {
            if (tlv2) {
                // We are adding a RB node.
                node_type = redbox_state.any_hsr_mode() ? VTSS_APPL_REDBOX_NODE_TYPE_DANH_RB : VTSS_APPL_REDBOX_NODE_TYPE_DANP_RB;
            } else {
                // We are adding a DANx of some sort based on TLV1.
                if (rx_pdu_info.tlv2_present()) {
                    // We are adding a VDANx.
                    node_type = redbox_state.any_hsr_mode() ? VTSS_APPL_REDBOX_NODE_TYPE_VDANH : VTSS_APPL_REDBOX_NODE_TYPE_VDANP;
                } else {
                    // We are adding a real DANx
                    node_type = redbox_state.any_hsr_mode() ? VTSS_APPL_REDBOX_NODE_TYPE_DANH : VTSS_APPL_REDBOX_NODE_TYPE_DANP;
                }
            }
        }

        // IEC 62439-3, section 4.2.7.2 states:
        // "Even if the NodesTable is populated by observing normal traffic, it
        // also needs to be populated by SV frames sent at regular intervals to
        // detect quiet nodes."
        // So create or update the NT.
        REDBOX_BASE_mesa_nt_add(redbox_state, mac_itr);
    }

    // Common to NT and PNT
    vtss_appl_redbox_mac_port_status_t &port = add_to_pnt ? mac_itr->second.pnt.status.port : mac_itr->second.nt.status.port[rx_pdu_info.port_type];
    port.sv_rx_cnt++;
    port.sv_last_seen_secs = vtss::uptime_seconds();
    port.sv_last_type      = redbox_pdu_tlv_type_to_sv_type(rx_pdu_info.tlv1_type);

    // No need to clean up the rest of the NT/PNT map, because that will happen
    // whenever the NT/PNT is shown from a management interface or half of the
    // NT/PNT age time has expired.
}

/******************************************************************************/
// REDBOX_BASE_nt_add()
/******************************************************************************/
static void REDBOX_BASE_nt_add(redbox_state_t &redbox_state, const redbox_pdu_info_t &rx_pdu_info, bool tlv2)
{
    REDBOX_BASE_nt_pnt_add(redbox_state, rx_pdu_info, tlv2, false /* add to NT */);
}

/******************************************************************************/
// REDBOX_BASE_pnt_add()
/******************************************************************************/
static void REDBOX_BASE_pnt_add(redbox_state_t &redbox_state, const redbox_pdu_info_t &rx_pdu_info, bool tlv2)
{
    REDBOX_BASE_nt_pnt_add(redbox_state, rx_pdu_info, tlv2, true /* add to PNT */);
}

/******************************************************************************/
// REDBOX_BASE_rx_cntrs_update()
/******************************************************************************/
static void REDBOX_BASE_rx_cntrs_update(redbox_state_t *rb_state, const mesa_packet_rx_info_t *const rx_info, redbox_base_sv_rx_port_cnt_t &n)
{
    vtss_appl_redbox_sv_type_t   sv_type;
    redbox_base_sv_rx_port_cnt_t &o = REDBOX_BASE_sv_port_cnt[rx_info->port_no];

    // Update our own per-port counters (can be seen with a debug command).
    for (sv_type = (vtss_appl_redbox_sv_type_t)0; sv_type < VTSS_APPL_REDBOX_SV_TYPE_CNT; sv_type++) {
        o.cnt[sv_type] += n.cnt[sv_type];
    }

    o.err_cnt      += n.err_cnt;
    o.filtered_cnt += n.filtered_cnt;

    if (!rb_state) {
        return;
    }

    // Update the RedBox statistics (SV frame received on an LRE port).
    vtss_appl_redbox_port_statistics_t &p = rb_state->statistics.port[rx_info->rb_port_a ? VTSS_APPL_REDBOX_PORT_TYPE_A : VTSS_APPL_REDBOX_PORT_TYPE_B];
    for (sv_type = (vtss_appl_redbox_sv_type_t)0; sv_type < VTSS_APPL_REDBOX_SV_TYPE_CNT; sv_type++) {
        p.sv_rx_cnt[sv_type] += n.cnt[sv_type];
    }

    p.sv_rx_err_cnt      += n.err_cnt;
    p.sv_rx_filtered_cnt += n.filtered_cnt;
}

/******************************************************************************/
// REDBOX_BASE_our_own_mac()
// If mac is our own MAC return true. Otherwise return false.
// "our own MAC" is defined as the MAC either being the management MAC address
// or the RB's MAC address.
/******************************************************************************/
static bool REDBOX_BASE_our_own_mac(redbox_state_t &redbox_state, const mesa_mac_t &mac)
{
    redbox_port_state_t *interlink_port_state = redbox_state.interlink_port_state_get();
    return mac == *interlink_port_state->chassis_mac || mac == interlink_port_state->redbox_mac;
}

/******************************************************************************/
// REDBOX_BASE_which_tlvs_to_add()
/******************************************************************************/
static void REDBOX_BASE_which_tlvs_to_add(redbox_state_t &redbox_state, const redbox_pdu_info_t &rx_pdu_info, bool &add_tlv1, bool &add_tlv2)
{
    if (rx_pdu_info.tlv2_present()) {
        // TLV2 is present.
        if (REDBOX_BASE_our_own_mac(redbox_state, rx_pdu_info.tlv2_mac)) {
            // If this is one of our own MACs, we skip both adding TLV1.MAC and
            // TLV2.MAC.
            add_tlv1 = false;
            add_tlv2 = false;
            return;
        }

        // TLV2 is not our own MAC, so let's add that. One example, where this
        // makes sense is if we have two RedBoxes running on the same device
        // and they connect to each others' LRE ports. In that case RB#1's
        // NT should reflect RB#2's MAC address as a DANx-RedBox and vice versa.
        // If we didn't use TLV2, RB#1 would add RB#1 as a DANx, only, and not a
        // DANx-RedBox, because RB#1 would only receive SV frames without TLV2
        // from RB#2.
        add_tlv2 = true;
    } else {
        add_tlv2 = false;
    }

    // If we are still here, we need to see whether we need to add TLV1 also.
    // We do that if TLV1 is not one of our own MACs.
    add_tlv1 = !REDBOX_BASE_our_own_mac(redbox_state, rx_pdu_info.tlv1_mac);
}

/******************************************************************************/
// REDBOX_BASE_ingress_frame_filter()
// Returns true if this frame is supposed to be discarded because of ingress
// filtering.
/******************************************************************************/
static bool REDBOX_BASE_ingress_frame_filter(mesa_port_no_t rx_port_no, mesa_vid_t vid, const char *sv_type_str)
{
    mesa_packet_frame_info_t frame_info;
    mesa_packet_filter_t     frame_filter;
    mesa_rc                  rc;

    (void)mesa_packet_frame_info_init(&frame_info);
    frame_info.port_no = rx_port_no;
    frame_info.vid     = vid;

    T_NG(REDBOX_TRACE_GRP_FRAME_RX, "mesa_packet_frame_filter(rx_port_no = %u, vid = %u)", rx_port_no, vid);
    if ((rc = mesa_packet_frame_filter(nullptr, &frame_info, &frame_filter)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_FRAME_RX, "mesa_packet_frame_filter(rx_port_no = %u, vid = %u) failed: %s", rx_port_no, vid, error_txt(rc));
        return true;
    }

    if (frame_filter == MESA_PACKET_FILTER_DISCARD) {
        // We must discard this, since it should have gotten ingress filtered.
        T_DG(REDBOX_TRACE_GRP_FRAME_RX, "Discarding %s SV frame received on port_no = %u on VID = %u because of ingress filtering", sv_type_str, rx_port_no, vid);
        return true;
    }

    T_NG(REDBOX_TRACE_GRP_FRAME_RX, "%s SV frame received on port_no %u on VID = %u survives ingress filtering", sv_type_str, rx_port_no, vid);
    return false;
}

/******************************************************************************/
// REDBOX_BASE_egress_frame_filter()
// Returns a MESA frame filter that indicates whether to discard, Tx VLAN
// tagged, or Tx VLAN untagged this frame. It will be discarded if the egress
// port is not member of the vid.
/******************************************************************************/
static mesa_packet_filter_t REDBOX_BASE_egress_frame_filter(mesa_port_no_t tx_port_no, mesa_vid_t vid, const char *sv_type_str)
{
    mesa_packet_frame_info_t frame_info;
    mesa_packet_filter_t     frame_filter;
    mesa_rc                  rc;

    (void)mesa_packet_frame_info_init(&frame_info);
    frame_info.port_no = MESA_PORT_NO_NONE;
    frame_info.port_tx = tx_port_no;
    frame_info.vid     = vid;

    T_NG(REDBOX_TRACE_GRP_FRAME_RX, "mesa_packet_frame_filter(tx_port_no = %u, vid = %u)", tx_port_no, vid);
    if ((rc = mesa_packet_frame_filter(nullptr, &frame_info, &frame_filter)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_FRAME_RX, "mesa_packet_frame_filter(tx_port_no = %u, vid = %u) failed: %s", tx_port_no, vid, error_txt(rc));
        return MESA_PACKET_FILTER_DISCARD;
    }

    T_NG(REDBOX_TRACE_GRP_FRAME_RX, "%s SV frame to be transmitted to port %u on VID = %u: %s", sv_type_str, tx_port_no, vid, frame_filter == MESA_PACKET_FILTER_DISCARD ? "Discarding" : "Survives");
    return frame_filter;
}

/******************************************************************************/
// redbox_base_rx_sv_on_interlink()
/******************************************************************************/
void redbox_base_rx_sv_on_interlink(redbox_state_t &redbox_state, mesa_port_no_t rx_port_no, const redbox_pdu_info_t &rx_pdu_info, mesa_vlan_tag_t &class_tag, bool tx_vlan_tagged)
{
    bool add_tlv1, add_tlv2;

    if (rx_pdu_info.port_type != VTSS_APPL_REDBOX_PORT_TYPE_C) {
        T_EG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_port_no, "%u: Expected SV frame to be received on port C and not %s", redbox_state.inst, redbox_util_port_type_to_str(rx_pdu_info.port_type));
        return;
    }

    // Count the frame as received on the I/L port.
    redbox_state.statistics.port[VTSS_APPL_REDBOX_PORT_TYPE_C].sv_rx_cnt[redbox_pdu_tlv_type_to_sv_type(rx_pdu_info.tlv1_type)]++;

    // We might need to drop adding this frame if TLV1.MAC or TLV2.MAC is one of
    // own own.
    REDBOX_BASE_which_tlvs_to_add(redbox_state, rx_pdu_info, add_tlv1, add_tlv2);

    if (!add_tlv1 && !add_tlv2) {
        // Here the Rx counter is that of the I/L
        T_IG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_port_no, "%u: TLV1 or TLV2 contains one of our own MACs.", redbox_state.inst);
        redbox_state.statistics.port[VTSS_APPL_REDBOX_PORT_TYPE_C].sv_rx_filtered_cnt++;
        return;
    }

    // Check to see whether the SV frame has a valid RCT.
    if (!rx_pdu_info.rct_present) {
        // Count it as filtered on the I/L port and don't use it.
        T_IG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_port_no, "%u: RCT invalid or not present.", redbox_state.inst);
        redbox_state.statistics.port[VTSS_APPL_REDBOX_PORT_TYPE_C].sv_rx_filtered_cnt++;
        return;
    }

    // Check to see whether the SV frame is received with the correct
    // LanId (as they call it in the standard, although it's a PathId).
    if (rx_pdu_info.rct_lan_id != redbox_state.conf.lan_id) {
        T_IG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_port_no, "%u: RCT.lan_id wrong (%s). Expected %s", redbox_state.inst, redbox_util_lan_id_to_str(rx_pdu_info.rct_lan_id), redbox_util_lan_id_to_str(redbox_state.conf.lan_id));
        redbox_state.statistics.port[VTSS_APPL_REDBOX_PORT_TYPE_C].sv_rx_filtered_cnt++;
        return;
    }

    // Update the PNT for TLV1.MAC and TLV2.MAC (if present), so that
    // they become DANs rather than SANs. If entries for these don't
    // exist, we add them.
    if (add_tlv1) {
        REDBOX_BASE_pnt_add(redbox_state, rx_pdu_info, false /* TLV1 */);
    }

    if (add_tlv2) {
        REDBOX_BASE_pnt_add(redbox_state, rx_pdu_info, true  /* TLV2 */);
    }

    if (redbox_state.conf.sv_xlat_prp_to_hsr) {
        // We are asked to translate SV frames in the PRP-to-HSR
        // direction. When this is the case, our ACE is configured not
        // to forward the frame towards the I/L port (S/W-based
        // forwarding). When this is NOT the case, H/W will forward it.
        // The following function updates required statistics.
        redbox_pdu_tx_prp_to_hsr_sv(redbox_state, rx_pdu_info, class_tag, tx_vlan_tagged);
    }
}

/******************************************************************************/
// REDBOX_BASE_rx_callback_sv()
/******************************************************************************/
static BOOL REDBOX_BASE_rx_callback_sv(void *contxt, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    vtss_appl_redbox_sv_type_t   sv_type;
    mesa_vlan_tag_t              class_tag;
    mesa_etype_t                 outer_etype, inner_etype;
    redbox_itr_t                 redbox_itr;
    redbox_pdu_info_t            rx_pdu_info;
    redbox_base_sv_rx_port_cnt_t rx_cntrs = {};
    bool                         hsr_tagged, lre_port, add_tlv1, add_tlv2;
    const char                   *sv_type_str;
    uint32_t                     tx_cnt;
    redbox_state_t               *rb_state;
    mesa_packet_filter_t         frame_filter;

    // As long as we can't receive a frame behind two tags, the frame is always
    // normalized, so that the SV Ethertype comes at frm[12] and frm[13],
    // because the packet module strips the outer tag.
    outer_etype = frm[12] << 8 | frm[13];

    T_DG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_info->port_no, "Rx frame with outer etype = 0x%04x: %s", outer_etype, *rx_info); // Using packet_api.h's fmt(mesa_packet_rx_info_t)
    T_NG_HEX(REDBOX_TRACE_GRP_FRAME_RX, frm, rx_info->length);

    // This function is invoked both when receiving HSR and PRP SV frames.
    // Figure out which of them.
    hsr_tagged = outer_etype == REDBOX_ETYPE_HSR;
    sv_type_str = hsr_tagged ? "HSR" : "PRP";

    if (hsr_tagged) {
        // The SV ETYPE should be behind the 6 byte HSR-tag.
        inner_etype = frm[18] << 8 | frm[19];
    } else {
        inner_etype = outer_etype;
    }

    if (inner_etype != REDBOX_ETYPE_SUPERVISION) {
        // Why did we receive this frame? If it's HSR-tagged, I don't understand
        // how we can get other frames than SV beneath the HSR-tag - hence T_E()
        T_EG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_info->port_no, "Rx frame on port_no %u with outer_etype = 0x%04x and inner_etype = 0x%04x, which isn't the RedBox SV EtherType", rx_info->port_no, outer_etype, inner_etype);
        return FALSE; // Not consumed
    }

    REDBOX_LOCK_SCOPE();

    // We may receive SV frames both on LRE and non-LRE ports. If received on an
    // LRE port, we handle it in one way. If received on a non-LRE port, we
    // handle it in another.
    if ((rb_state = REDBOX_BASE_find_inst_by_port(rx_info->port_no)) != nullptr) {
        lre_port = true;
    } else {
        // The frame got received on a non-LRE port, or the RedBox instance got
        // disabled while this SV frame was in the pipeline. We handle it as if
        // received on a non-LRE port (see VID calculations below)
        lre_port = false;
    }

    if (lre_port) {
        // If received on an LRE-port, The H/W does *not* update the classified
        // VID, so we need to figure it out ourselves.
        // The packet module checks if a frame is received with the correct tag
        // type according to the port configuration, and if not, it discards
        // it before we get to receive it. If it's received with a tag, the
        // packet module strips it from the frame and puts it in the rx_info.
        // Notice, if the I/L port is configured with port-type == S-port, but
        // the RedBox will discard the S-tagged frames as if it did not contain
        // an HSR tag, because an HSR tag comes behind a possible VLAN tag, but
        // the S-tag is not recognized as a VLAN tag by the RedBox.
        // All this means that it's safe just to check if stripped_tag.tpid is
        // 0 or not.
        if (rx_info->stripped_tag.tpid != 0) {
            class_tag = rx_info->stripped_tag;

            if (class_tag.vid == 0) {
                // The frame was priority-tagged. Use the I/L port's PVID.
                class_tag.vid = rb_state->interlink_port_state_get()->vlan_conf.pvid;
            }
        } else {
            // The frame is VLAN untagged. Use the I/L port's PVID.
            vtss_clear(class_tag);
            class_tag.vid = rb_state->interlink_port_state_get()->vlan_conf.pvid;
        }

        if (class_tag.vid == 0) {
            T_EG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_info->port_no, "%u: Rx %s SV frame on LRE port %u, but VID is deducted as 0. Discarding.", rb_state->inst, sv_type_str, rx_info->port_no);
            rx_cntrs.err_cnt++;
            goto do_exit;
        }
    } else {
        // If received on non-LRE port, we get this frame, because of the ACE we
        // have installed. The classified VID is valid.
        class_tag = rx_info->tag;

        // However, if classified VID is 0, it must be a SV frame that was in
        // the pipeline while the corresponding RB instance got disabled.
        if (class_tag.vid == 0) {
            T_IG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_info->port_no, "Rx %s SV frame on non-LRE port, but VID is 0. Ignoring.", sv_type_str);
            rx_cntrs.err_cnt++;
            goto do_exit;
        }
    }

    T_IG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_info->port_no, "Rx %s SV frame on %sLRE port with classified tag = %s", sv_type_str, lre_port ? "" : "non-", class_tag);

    // Attempt to parse the SV frame. Do this before checking whether it should
    // be ingress filtered, so that we can update counters correctly. If the
    // SV frame is valid, we always update the SV type frame counters.
    if (!redbox_pdu_rx_frame(frm, rx_info, rx_pdu_info, hsr_tagged)) {
        rx_cntrs.err_cnt++;
        goto do_exit;
    }

    // Convert to a type that we can use when incrementing counters.
    // It has been agreed with the architects that we handle all valid SV frame
    // types, that is, if we e.g. receive a SV frame with TLV1.Type == HSR on a
    // PRP port, we accept it anyway - as long as it has an RCT. Likewise for
    // SV frames with TLV1.Type = PRP-DD/PRP-DA and received on an HSR port - as
    // long as it contains a valid HSR tag.
    sv_type = redbox_pdu_tlv_type_to_sv_type(rx_pdu_info.tlv1_type);
    rx_cntrs.cnt[sv_type]++;

    // Whether received on an LRE port (due to RB redirect/copy to CPU) or on a
    // non-LRE port (due to ACE copy to CPU), we get this frame whether or not
    // it should have been VLAN ingress filtered or filtered due to STP and
    // other things that should have filtered it, so we need to ask MESA whether
    // we should just discard it or process it.
    if (REDBOX_BASE_ingress_frame_filter(rx_info->port_no, class_tag.vid, sv_type_str)) {
        // Count it as filtered. This counter is visible by end-user if received
        // on an LRE port. Otherwise, it is only visible when dumping RB state.
        rx_cntrs.filtered_cnt++;
        goto do_exit;
    }

    if (lre_port) {
        // Received on an LRE port.
        rx_pdu_info.port_type = rx_info->rb_port_a ? VTSS_APPL_REDBOX_PORT_TYPE_A : VTSS_APPL_REDBOX_PORT_TYPE_B;

        // If we are subscribing to both HSR-tagged and RCT-tagged SV frames
        // (which is the case if we e.g. have one RedBox in HSR-SAN mode and
        // another in PRP-SAN mode), we might receive e.g. HSR-tagged SV frames
        // on the PRP-SAN RedBox if connecting the ports wrongly. The PRP-SAN
        // RedBox will not mark such HSR-tagged SV frames as
        // rx_info->rb_tagged but still send them to the CPU. What a shame.
        // Anyhow, let's filter such frames.
        if (!rx_info->rb_tagged) {
            T_IG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_info->port_no, "%u: The SV frame was not HSR- or PRP-tagged on LRE-port arrival. Filtering.", rb_state->inst);
            T_IG_HEX(REDBOX_TRACE_GRP_FRAME_RX, frm, rx_info->length);
            rx_cntrs.filtered_cnt++;
            goto do_exit;
        }

        // We might need to discard this frame if TLV1.MAC or TLV2.MAC is one of
        // our own.
        REDBOX_BASE_which_tlvs_to_add(*rb_state, rx_pdu_info, add_tlv1, add_tlv2);

        if (!add_tlv1 && !add_tlv2) {
            T_IG_PORT(REDBOX_TRACE_GRP_FRAME_RX, rx_info->port_no, "%u: TLV1 or TLV2 contains one of our own MACs.", rb_state->inst);
            rx_cntrs.filtered_cnt++;
            goto do_exit;
        }

        // In all modes, we need to update the NT.
        if (add_tlv1) {
            REDBOX_BASE_nt_add(*rb_state, rx_pdu_info, false /* TLV1 */);
        }

        if (add_tlv2) {
            REDBOX_BASE_nt_add(*rb_state, rx_pdu_info, true  /* TLV2 */);
        }

        // In HSR-PRP mode and if configured to, we need to modify it from any-
        // typed SV frame to a PRP SV frame and forward it to all other ports in
        // the VLAN. See IEC-62439-3:2021/COR1:2023 (the corrigendum).
        // If another RB instance is in HSR-PRP mode and in the same VLAN, it
        // shall "receive" this PRP SV frame, and convert it to an HSR SV frame
        // and send it to its LRE ports (SIC!). This is all handled by
        // redbox_pdu_tx_hsr_to_prp_sv().
        if (rb_state->conf.mode == VTSS_APPL_REDBOX_MODE_HSR_PRP) {
            if (rb_state->conf.sv_xlat_hsr_to_prp) {
                // If we are asked to translate, do that. Otherwise let H/W do
                // the forwarding
                tx_cnt = redbox_pdu_tx_hsr_to_prp_sv(*rb_state, rx_pdu_info, class_tag);
                if (tx_cnt == 0) {
                    rx_cntrs.filtered_cnt++;
                }
            }
        }
    } else {
        // Received on a normal switch port.
        rx_pdu_info.port_type = VTSS_APPL_REDBOX_PORT_TYPE_C;

        // Find the redbox(es) that would like to receive this.
        for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
            if (redbox_itr->second.conf.mode != VTSS_APPL_REDBOX_MODE_HSR_PRP) {
                // Only RBs in HSR-PRP mode are candidates.
                continue;
            }

            if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
                // Only active RBs are candidates.
                continue;
            }

            // Some of the following checks are identical for all RBs in HSR-PRP
            // mode. This is needed in order to get all RBs' I/L Rx counters
            // updated.

            // Check to see whether this RB should receive this frame. If not,
            // simply skip it.
            frame_filter = REDBOX_BASE_egress_frame_filter(redbox_itr->second.interlink_port_no_get(), class_tag.vid, sv_type_str);
            if (frame_filter == MESA_PACKET_FILTER_DISCARD) {
                // Don't count it, since this frame would never go from switch
                // core towards this RB's I/L port whether we H/W forwarded it
                // or not.
                continue;
            }

            // The remainder of the handling is done in a function that also
            // is used whenever another RB in HSR-PRP mode transmits a PRP SV
            // frame to the PRP network.
            redbox_base_rx_sv_on_interlink(redbox_itr->second, rx_info->port_no, rx_pdu_info, class_tag, frame_filter == MESA_PACKET_FILTER_TAGGED);
        }
    }

do_exit:
    REDBOX_BASE_rx_cntrs_update(rb_state, rx_info, rx_cntrs);
    return TRUE;
}

/******************************************************************************/
// REDBOX_BASE_packet_rx_filters_update()
/******************************************************************************/
static void REDBOX_BASE_packet_rx_filters_update(void)
{
    redbox_itr_t                 redbox_itr;
    redbox_base_rx_filter_type_t filter_type;
    bool                         register_rx_filter[REDBOX_BASE_RX_FILTER_TYPE_CNT] = {};
    mesa_rc                      rc;

    for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
        if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
            // Not active. Doesn't contribute to what kind of frames we need.
            continue;
        }

        switch (redbox_itr->second.conf.mode) {
        case VTSS_APPL_REDBOX_MODE_PRP_SAN:
            // We need PRP SV frames from packet module to us.
            register_rx_filter[REDBOX_BASE_RX_FILTER_TYPE_PRP_SV] = true;
            break;

        default:
            // In all HSR modes, we need HSR SV frames from packet module to us.
            register_rx_filter[REDBOX_BASE_RX_FILTER_TYPE_HSR_SV] = true;

            if (redbox_itr->second.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_PRP) {
                // In HSR-PRP mode, we also need PRP SV frames
                register_rx_filter[REDBOX_BASE_RX_FILTER_TYPE_PRP_SV] = true;
            }

            break;
        }
    }

    // We register each SV frame filter on all ports, because we know that SV
    // frames only come to the CPU if either our ACE matches or if a RedBox
    // sends such frames to the CPU.
    // For HSR untagged frames, we must also match on source port, because
    // otherwise, we cannot distinguish frames arriving without HSR tag on an
    // LRE port from such frames arriving on a non-LRE port.
    for (filter_type = (redbox_base_rx_filter_type_t)0; filter_type < REDBOX_BASE_RX_FILTER_TYPE_CNT; filter_type++) {
        packet_rx_filter_t &filter          = REDBOX_BASE_rx_filters[filter_type].filter;
        void               *&filter_id      = REDBOX_BASE_rx_filters[filter_type].filter_id;
        const char         *filter_type_str = REDBOX_BASE_rx_filter_type_to_str(filter_type);

        if (register_rx_filter[filter_type]) {
            // Register for this kind of frames unless already registered (the
            // match criteria never change).
            if (filter_id != nullptr) {
                continue;
            }

            // First time we register this filter.
            packet_rx_filter_init(&filter);
            filter.modid = VTSS_MODULE_ID_REDBOX;
            filter.prio  = PACKET_RX_FILTER_PRIO_NORMAL;

            switch (filter_type) {
            case REDBOX_BASE_RX_FILTER_TYPE_PRP_SV:
                // SV frames arrive with an RCT, but not with an HSR-tag.

                // Match on EtherType and DMAC
                filter.match = PACKET_RX_FILTER_MATCH_ETYPE | PACKET_RX_FILTER_MATCH_DMAC;

                // EtherType
                filter.etype = REDBOX_ETYPE_SUPERVISION;

                // SV frame DMAC except for the last byte.
                memcpy(filter.dmac, redbox_multicast_dmac.addr, sizeof(filter.dmac));
                filter.dmac_mask[5] = 0xff;

                filter.cb = REDBOX_BASE_rx_callback_sv;
                break;

            case REDBOX_BASE_RX_FILTER_TYPE_HSR_SV:
            default:
                // SV frames arrive with an HSR-tag.

                // Match on EtherType and DMAC
                filter.match = PACKET_RX_FILTER_MATCH_ETYPE | PACKET_RX_FILTER_MATCH_DMAC;

                // EtherType
                filter.etype = REDBOX_ETYPE_HSR;

                // SV frame DMAC except for the last byte.
                memcpy(filter.dmac, redbox_multicast_dmac.addr, sizeof(filter.dmac));
                filter.dmac_mask[5] = 0xff;

                filter.cb = REDBOX_BASE_rx_callback_sv;
                break;
            }

            if ((rc = packet_rx_filter_register(&filter, &filter_id)) != VTSS_RC_OK) {
                T_EG(REDBOX_TRACE_GRP_FRAME_RX, "packet_rx_filter_register(%s) failed: %s", filter_type_str, error_txt(rc));
                filter_id = nullptr;
            }

            T_IG(REDBOX_TRACE_GRP_FRAME_RX, "packet_rx_filter_register(%s) => %p", filter_type_str, filter_id);
        } else {
            // Unregister if already registered.
            if (filter_id == nullptr) {
                // Not currently registered. Nothing to do.
                continue;
            }

            T_IG(REDBOX_TRACE_GRP_FRAME_RX, "packet_rx_filter_unregister(%s, %p)", filter_type_str, filter_id);
            if ((rc = packet_rx_filter_unregister(filter_id)) != VTSS_RC_OK) {
                T_EG(REDBOX_TRACE_GRP_FRAME_RX, "packet_rx_filter_unregister(%s, %p) failed: %s", filter_type_str, filter_id, error_txt(rc));
            }

            filter_id = nullptr;
        }
    }
}

/******************************************************************************/
// REDBOX_BASE_mesa_to_appl_statistics()
/******************************************************************************/
static void REDBOX_BASE_mesa_to_appl_statistics(redbox_state_t &redbox_state, const mesa_rb_counters_t &mesa_counters, vtss_appl_redbox_statistics_t &statistics)
{
    vtss_appl_redbox_port_type_t port_type;
    vtss_appl_redbox_sv_type_t   sv_type;

    for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_C; port_type++) {
        const mesa_rb_port_counters_t      &m  = port_type == VTSS_APPL_REDBOX_PORT_TYPE_A ? mesa_counters.port_a : port_type == VTSS_APPL_REDBOX_PORT_TYPE_B ? mesa_counters.port_b : mesa_counters.port_c;
        vtss_appl_redbox_port_statistics_t &s  = statistics.port[port_type];
        vtss_appl_redbox_port_statistics_t &sw = redbox_state.statistics.port[port_type];

        s.tx_tagged_cnt     = m.tx_tagged;
        s.tx_untagged_cnt   = m.tx_untagged;
        s.rx_wrong_lan_cnt  = m.rx_wrong_lan;
        s.rx_tagged_cnt     = m.rx_tagged;
        s.rx_untagged_cnt   = m.rx_untagged;
        s.tx_dupl_zero_cnt  = m.tx_dupl_zero;
        s.tx_dupl_one_cnt   = m.tx_dupl_one;
        s.tx_dupl_multi_cnt = m.tx_dupl_multi;
        s.rx_own_cnt        = m.rx_own;
        s.rx_bpdu_cnt       = m.rx_local;
        s.tx_bpdu_cnt       = m.tx_local;

        // Also transfer S/W-based statistics
        for (sv_type = (vtss_appl_redbox_sv_type_t)0; sv_type < VTSS_APPL_REDBOX_SV_TYPE_CNT; sv_type++) {
            s.sv_rx_cnt[sv_type] = sw.sv_rx_cnt[sv_type];
            s.sv_tx_cnt[sv_type] = sw.sv_tx_cnt[sv_type];
        }

        s.sv_rx_err_cnt      = sw.sv_rx_err_cnt;
        s.sv_rx_filtered_cnt = sw.sv_rx_filtered_cnt;
    }
}

/******************************************************************************/
// REDBOX_BASE_appl_to_mesa_mode()
/******************************************************************************/
static mesa_rb_mode_t REDBOX_BASE_appl_to_mesa_mode(vtss_appl_redbox_mode_t mode)
{
    switch (mode) {
    case VTSS_APPL_REDBOX_MODE_PRP_SAN:
        return MESA_RB_MODE_PRP_SAN;

    case VTSS_APPL_REDBOX_MODE_HSR_SAN:
        return MESA_RB_MODE_HSR_SAN;

    case VTSS_APPL_REDBOX_MODE_HSR_PRP:
        return MESA_RB_MODE_HSR_PRP;

    case VTSS_APPL_REDBOX_MODE_HSR_HSR:
        return MESA_RB_MODE_HSR_HSR;

    default:
        T_EG(REDBOX_TRACE_GRP_BASE, "Invalid mode (%d)", mode);
        return MESA_RB_MODE_PRP_SAN;
    }
}

/******************************************************************************/
// REDBOX_BASE_mesa_statistics_get()
/******************************************************************************/
static mesa_rc REDBOX_BASE_mesa_statistics_get(redbox_state_t &redbox_state, mesa_rb_counters_t &mesa_counters)
{
    mesa_rc rc;

    vtss_clear(mesa_counters);

    T_IG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_counters_get(%u)", redbox_state.inst, redbox_state.rb_id_get());
    if ((rc = mesa_rb_counters_get(nullptr, redbox_state.rb_id_get(), &mesa_counters)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_counters_get(%u) failed: %s", redbox_state.inst, redbox_state.rb_id_get(), error_txt(rc));
        return VTSS_APPL_REDBOX_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_BASE_mesa_statistics_clear()
/******************************************************************************/
static mesa_rc REDBOX_BASE_mesa_statistics_clear(redbox_state_t &redbox_state)
{
    mesa_rc rc;

    if ((rc = mesa_rb_counters_clear(nullptr, redbox_state.rb_id_get())) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_counters_clear(%u) failed: %s", redbox_state.inst, redbox_state.rb_id_get(), error_txt(rc));
        return VTSS_APPL_REDBOX_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_BASE_notification_update_timeout()
/******************************************************************************/
static void REDBOX_BASE_notification_update_timeout(redbox_timer_t &timer, const void *context)
{
    redbox_itr_t redbox_itr;

    if ((redbox_itr = REDBOX_map.find(timer.instance)) == REDBOX_map.end()) {
        T_EG(REDBOX_TRACE_GRP_BASE, "Unable to find RedBox instance #%u in map", timer.instance);
        redbox_timer_stop(timer);
        return;
    }

    // Now this timer has stopped, so update the notifications.
    REDBOX_BASE_notification_status_update(redbox_itr->second);
}

/******************************************************************************/
// REDBOX_BASE_statistics_get_and_notif_update()
/******************************************************************************/
static mesa_rc REDBOX_BASE_statistics_get_and_notif_update(redbox_state_t &redbox_state, mesa_rb_counters_t &mesa_counters)
{
    vtss_appl_redbox_port_type_t port_type;
    bool                         check_wrong_lan, check_hsr_untagged, update_notifications;

    VTSS_RC(REDBOX_BASE_mesa_statistics_get(redbox_state, mesa_counters));

    // This may have changed the cnt_err_wrong_lan and/or hsr_untagged_rx
    // condition
    update_notifications = false;
    for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_C; port_type++) {
        mesa_rb_port_counters_t &c = port_type == VTSS_APPL_REDBOX_PORT_TYPE_A ? mesa_counters.port_a : port_type == VTSS_APPL_REDBOX_PORT_TYPE_B ? mesa_counters.port_b : mesa_counters.port_c;

        // Check if we have received frames with wrong LAN ID or frames without
        // HSR tag in relevant modes, so that we can raise an alarm
        switch (redbox_state.conf.mode) {
        case VTSS_APPL_REDBOX_MODE_PRP_SAN:
            check_wrong_lan    = port_type == VTSS_APPL_REDBOX_PORT_TYPE_A || port_type == VTSS_APPL_REDBOX_PORT_TYPE_B;
            check_hsr_untagged = false;
            break;

        case VTSS_APPL_REDBOX_MODE_HSR_SAN:
            check_wrong_lan    = false;
            check_hsr_untagged = port_type == VTSS_APPL_REDBOX_PORT_TYPE_A || port_type == VTSS_APPL_REDBOX_PORT_TYPE_B;
            break;

        case VTSS_APPL_REDBOX_MODE_HSR_PRP:
            check_wrong_lan    = port_type == VTSS_APPL_REDBOX_PORT_TYPE_C;
            check_hsr_untagged = port_type == VTSS_APPL_REDBOX_PORT_TYPE_A || port_type == VTSS_APPL_REDBOX_PORT_TYPE_B;
            break;

        case VTSS_APPL_REDBOX_MODE_HSR_HSR:
            check_wrong_lan    = false;
            check_hsr_untagged = true;
            break;

        default:
            T_EG(REDBOX_TRACE_GRP_BASE, "%u: Invalid mode (%d)", redbox_state.inst, redbox_state.conf.mode);
            return VTSS_RC_ERROR;
        }

        if (check_wrong_lan) {
            if (c.rx_wrong_lan && c.rx_wrong_lan != redbox_state.cnt_err_wrong_lan[port_type]) {
                redbox_state.cnt_err_wrong_lan[port_type] = c.rx_wrong_lan;
                redbox_timer_start(redbox_state.cnt_err_wrong_lan_timers[port_type], REDBOX_cap.alarm_raised_time_secs * 1000, false);
                update_notifications = true;
            }
        }

        if (check_hsr_untagged) {
            if (c.rx_untagged && c.rx_untagged != redbox_state.hsr_untagged_rx[port_type]) {
                redbox_state.hsr_untagged_rx[port_type] = c.rx_untagged;
                redbox_timer_start(redbox_state.hsr_untagged_rx_timers[port_type], REDBOX_cap.alarm_raised_time_secs * 1000, false);
                update_notifications = true;
            }
        }
    }

    if (update_notifications) {
        // Update notifications after at least one of the timers has been
        // (re-)started.
        REDBOX_BASE_notification_status_update(redbox_state);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_BASE_statistics_poll_timeout()
/******************************************************************************/
static void REDBOX_BASE_statistics_poll_timeout(redbox_timer_t &timer, const void *context)
{
    mesa_rb_counters_t mesa_counters;
    redbox_itr_t       redbox_itr;

    if ((redbox_itr = REDBOX_map.find(timer.instance)) == REDBOX_map.end()) {
        T_EG(REDBOX_TRACE_GRP_BASE, "Unable to find RedBox instance #%u in map", timer.instance);
        redbox_timer_stop(timer);
        return;
    }

    (void)REDBOX_BASE_statistics_get_and_notif_update(redbox_itr->second, mesa_counters);
}

/******************************************************************************/
// REDBOX_BASE_mesa_rb_conf_set()
/******************************************************************************/
static mesa_rc REDBOX_BASE_mesa_rb_conf_set(redbox_state_t &redbox_state, mesa_rb_conf_t &rb_conf)
{
    mesa_rc rc;

    T_IG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_conf_set(id = %u, conf = %s)", redbox_state.inst, redbox_state.rb_id_get(), rb_conf);
    if ((rc = mesa_rb_conf_set(nullptr, redbox_state.rb_id_get(), &rb_conf)) != VTSS_RC_OK) {
        T_EG(REDBOX_TRACE_GRP_API, "%u: mesa_rb_conf_set(id = %u, conf = %s) failed: %s", redbox_state.inst, redbox_state.rb_id_get(), rb_conf, error_txt(rc));
        return VTSS_APPL_REDBOX_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_BASE_mesa_rb_conf_update()
/******************************************************************************/
static mesa_rc REDBOX_BASE_mesa_rb_conf_update(redbox_state_t &redbox_state, bool activating)
{
    mesa_rb_conf_t rb_conf;

    vtss_clear(rb_conf);
    rb_conf.mode            = REDBOX_BASE_appl_to_mesa_mode(redbox_state.conf.mode);
    rb_conf.port_a          = redbox_state.port_no_get(VTSS_APPL_REDBOX_PORT_TYPE_A);
    rb_conf.port_b          = redbox_state.port_no_get(VTSS_APPL_REDBOX_PORT_TYPE_B);
    rb_conf.net_id          = redbox_state.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_PRP || redbox_state.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_HSR ? redbox_state.conf.net_id : 0;
    rb_conf.lan_id          = redbox_state.conf.lan_id;
    rb_conf.nt_dmac_disable = false;
    rb_conf.nt_age_time     = redbox_state.conf.nt_age_time_secs;
    rb_conf.pnt_age_time    = redbox_state.conf.pnt_age_time_secs;

    // SV frame CPU copy/redirect from LRE ports
    // HSR-HSR:
    //   SV frames are copied (not redirected) to the CPU, because they need to
    //   be forwarded as are from RB to switch core. They are used by the CPU
    //   only to be able to show better NT node types based on TLV1 and TLV2
    //   contents.
    // HSR-PRP:
    //   If we are configured to translate HSR SV frames to PRP SV frames, we
    //   redirect SV frames to the CPU and perform S/W forwarding.
    //   Otherwise, we copy SV frames to the CPU in order to get H/W to forward
    //   the SV frames and for S/W to be able to show better NT node types based
    //   on TLV1 and TLV2 contents.
    // Other modes:
    //   We redirect SV frame to the CPU to get the NT updated correctly. The
    //   switch-side should not get these frames.
    if (redbox_state.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_HSR) {
        rb_conf.sv = MESA_RB_SV_CPU_COPY;
    } else if (redbox_state.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_PRP) {
        rb_conf.sv = redbox_state.conf.sv_xlat_hsr_to_prp ? MESA_RB_SV_CPU_ONLY : MESA_RB_SV_CPU_COPY;
    } else {
        rb_conf.sv = MESA_RB_SV_CPU_ONLY;
    }

    // SV frame CPU forward from switch towards LRE ports - or rather: Configure
    // whether the RedBox discards SV frames arriving on the I/L port from the
    // switch core.
    // HSR-HSR:
    //   Never.
    // HSR-PRP:
    //   If we are configured to translate PRP SV frames to HSR SV frames, we
    //   let the RB discard SV frames. Otherwise, we let it forward towards LRE
    //   ports.
    // Other modes:
    //   Always
    if (redbox_state.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_HSR) {
        rb_conf.sv_discard = false;
    } else if (redbox_state.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_PRP) {
        rb_conf.sv_discard = redbox_state.conf.sv_xlat_prp_to_hsr ? true : false;
    } else {
        rb_conf.sv_discard = true;
    }

    if (activating) {
        // Start with a very low duplicate discard age time to get the DD table
        // flushed before we really start.
        rb_conf.dd_age_time = 1; // millisecond
        VTSS_RC(REDBOX_BASE_mesa_rb_conf_set(redbox_state, rb_conf));
    }

    // Then set it to the correct age time.
    rb_conf.dd_age_time = redbox_state.conf.duplicate_discard_age_time_msecs;
    return REDBOX_BASE_mesa_rb_conf_set(redbox_state, rb_conf);
}

/******************************************************************************/
// REDBOX_BASE_nt_pnt_port_fill()
/******************************************************************************/
static void REDBOX_BASE_nt_pnt_port_fill(vtss_appl_redbox_mac_port_status_t &dst, mesa_rb_node_port_t &src, uint64_t now)
{
    // Make the "last seen" run as smooth as possible. So rather than using the
    // H/W's age directly, we save the absolute time since boot that we think
    // this entry was added. This absolute time gets adjusted to a relative time
    // by vtss_appl_redbox_nt_mac_status_get() and
    // vtss_appl_redbox_pnt_mac_status_get().
    if (!src.cnt.rx) {
        // This is the best we can do. If we clear counters, we will get a wrong
        // last seen, but there's nothing to do about it, because reception of a
        // SV frame that touches this entry will cause the H/W age to go to
        // zero, and we don't want the Rx age to be affected by that.
        dst.last_seen_secs = 0;
    } else if (src.cnt.rx != dst.rx_cnt) {
        // For now, we compute the time when this entry was added. It will be
        // adjusted whenever the relvant management function gets called.
        dst.last_seen_secs = now - src.age;
    }

    dst.fwd              = src.fwd;
    dst.rx_cnt           = src.cnt.rx;
    dst.rx_wrong_lan_cnt = src.cnt.rx_wrong_lan;
}

/******************************************************************************/
// REDBOX_BASE_nt_poll()
// Poll NodesTable.
//
// This function is *always* called when a management interface wants to see the
// contents of a nodes table, to get both the S/W-based (supervision frames) and
// H/W-based (data frames) entries updated properly.
// If it's less than 2 seconds ago it was polled, however, we skip polling.
/******************************************************************************/
static void REDBOX_BASE_nt_poll(redbox_state_t &redbox_state)
{
    redbox_mac_itr_t  mac_itr, mac_itr_next;
    mesa_rb_node_t    mesa_nt_entry;
    mesa_rb_node_id_t id;
    uint64_t          now_secs;
    uint64_t          now_msecs = vtss::uptime_milliseconds();
    uint32_t          sv_last_seen_a, sv_last_seen_b, sv_last_seen;

    if (now_msecs - redbox_state.nt_last_poll_msecs < 2000) {
        // Don't poll faster than every two seconds
        return;
    }

    T_IG(REDBOX_TRACE_GRP_API, "%u: NT Poll Start", redbox_state.inst);

    // Clear present flag in all current NT entries.
    for (mac_itr = redbox_state.mac_map.begin(); mac_itr != redbox_state.mac_map.end(); ++mac_itr) {
        if (!mac_itr->second.is_pnt_entry) {
            mac_itr->second.nt.present_in_nt = false;
        }
    }

    now_secs = vtss::uptime_seconds();

    // Poll NodesTable. Poll by ID rather than by MAC, because polling by ID is
    // much faster.
    id = 0;
    while (mesa_rb_node_id_get_next(nullptr, redbox_state.rb_id_get(), id, &mesa_nt_entry) == VTSS_RC_OK) {
        T_DG(REDBOX_TRACE_GRP_API, "%u: %s", redbox_state.inst, mesa_nt_entry);

        // Prepare for next iteration
        id = mesa_nt_entry.id;

        if ((mac_itr = redbox_state.mac_map.find(mesa_nt_entry.mac)) == redbox_state.mac_map.end()) {
            // Create a new entry.
            if ((mac_itr = redbox_state.mac_map.get(mesa_nt_entry.mac)) == redbox_state.mac_map.end()) {
                T_EG(REDBOX_TRACE_GRP_API, "%u::%s: Out of memory", redbox_state.inst, mesa_nt_entry.mac);
                // RBNTBD: Update operational state?
                continue;
            }

            vtss_clear(mac_itr->second);
        } else {
            if (mac_itr->second.is_pnt_entry) {
                // This changes from being a PNT entry to being an NT entry, so
                // it's basically a MAC move.
                if (mac_itr->second.pnt.status.locked) {
                    // Move of a static PNT entry to a (dynamic) NT entry should
                    // not be possible
                    T_EG(REDBOX_TRACE_GRP_API, "%u: %s was locked in PNT (%s), but now found in NT (%s)", redbox_state.inst, mac_itr->first, mac_itr->second.pnt, mesa_nt_entry);
                    continue;
                }

                // Stop any SV frames transmitted for this PNT entry.
                redbox_pdu_tx_stop(redbox_state, mac_itr);

                // Clear the status to prevent the PNT status from being
                // interpreted as NT status.
                vtss_clear(mac_itr->second);
            }
        }

        // Update the entry
        mac_itr->second.is_pnt_entry     = false;
        mac_itr->second.nt.present_in_nt = true;
        REDBOX_BASE_nt_pnt_port_fill(mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_A], mesa_nt_entry.port_a, now_secs);
        REDBOX_BASE_nt_pnt_port_fill(mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_B], mesa_nt_entry.port_b, now_secs);

        // Update the node type.
        if (mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_A].sv_last_seen_secs ||
            mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_B].sv_last_seen_secs) {
            // A SV frame relating to this MAC address has been seen. Reception
            // of a SV frame causes S/W to change the node type in H/W to a DAN,
            // while setting the S/W node type in accordance with the received
            // SV frame. H/W cannot change the node type back to SAN, so there
            // is nothing else to do here.
        } else {
            // No SV frame has been received relating to this MAC address, so we
            // convert MESA's report of node type (SAN or DAN) to a S/W-based
            // node type.
            // H/W changes a node from being a SAN to being a DAN if receiving
            // traffic on both LRE ports. H/W can NOT change a node from being a
            // DAN to being a SAN - even if not receiving frames on one of the
            // LRE ports anymore.
            // Moreover, the node type is irrelevant in HSR modes, because there
            // is no such thing as a SAN directly connected to the HSR ring. H/W
            // however, reports it as a SAN if only receiving frames on one of
            // the LRE ports.
            // The SAN/DAN type in H/W is mostly used when transmitting frames
            // out the LRE ports in PRP-SAN mode, where the node type and fwd
            // fields determine whether to send to LRE-A or LRE-B or both.
            if (mesa_nt_entry.type == MESA_RB_NODE_TYPE_DAN) {
                // Actually, we don't know what type of DAN it is. It could be
                // both a RedBox DANx, a DANx, and a VDANx. We pick a DANx. If
                // at some point in time we receive a SV frame relating to this
                // MAC address, we know the exact type.
                mac_itr->second.nt.status.node_type = redbox_state.any_hsr_mode() ? VTSS_APPL_REDBOX_NODE_TYPE_DANH : VTSS_APPL_REDBOX_NODE_TYPE_DANP;
            } else {
                // This is a SAN. In HSR mode, there's no such thing as a SAN
                // directly connected to the ring. In that case, we report it
                // as a DANH. In PRP-SAN mode, however, this might be a real
                // SAN, so we report it as such (no such enumeration in the
                // public MIB, though!).
                // Notice: H/W doesn't care about the SAN/DAN type in HSR modes.
                mac_itr->second.nt.status.node_type = redbox_state.any_hsr_mode() ? VTSS_APPL_REDBOX_NODE_TYPE_DANH : VTSS_APPL_REDBOX_NODE_TYPE_SAN;
            }
        }
    }

    // Go through the map again and remove all entries where present_in_nt is
    // false, indicating that they are no longer in the NT.
    mac_itr = redbox_state.mac_map.begin();
    while (mac_itr != redbox_state.mac_map.end()) {
        mac_itr_next = mac_itr;
        ++mac_itr_next;

        if (mac_itr->second.is_pnt_entry) {
            goto next;
        }

        if (!mac_itr->second.nt.present_in_nt) {
            // The entry no longer exists in the NT. Remove it from S/W as well.
            redbox_state.mac_map.erase(mac_itr);
            goto next;
        }

        // If we are still here, the entry exists in the NT table.
        // We need to figure out whether the RedBox has seen frames from that
        // entry or if it is only in the NT because we have added it artifically
        // because of a reception of a SV frame.

        if (mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_A].rx_cnt ||
            mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_B].rx_cnt) {
            // Frames have really been received from this entry. Leave it alone.
            // Notice: The rx_cnt counters are NOT cleared when clearing
            // RedBox statistics, which is good, because otherwise this piece of
            // code wouldn't work!
            goto next;
        }

        // Find the smallest non-zero sv_last_seen amongst port A and B.
        sv_last_seen_a = mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_A].sv_last_seen_secs;
        sv_last_seen_b = mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_B].sv_last_seen_secs;

        if (sv_last_seen_b && (!sv_last_seen_a || sv_last_seen_b < sv_last_seen_a)) {
            sv_last_seen = sv_last_seen_b;
        } else {
            sv_last_seen = sv_last_seen_a;
        }

        if (sv_last_seen) {
            // Check to see if the SV frame has timed out. We time SV frame
            // receptions out after nt_age_time_secs.
            if (now_secs - sv_last_seen <= redbox_state.conf.nt_age_time_secs) {
                // SV frame not timed out. Leave the entry in the table.
                goto next;
            }

            // SV frame also timed out.
        } else {
            // If it is not there because of reception of a SV frame, it's a
            // code bug.
            T_EG(REDBOX_TRACE_GRP_FRAME_RX, "%u: Expected sv_last_seen_secs to be non-zero %s", redbox_state.inst, mac_itr->second.nt);
        }

        // We have added this entry ourselves, but it has timed out. Remove it
        // from H/W...
        REDBOX_BASE_mesa_nt_del(redbox_state, mac_itr);

        // ...and from S/W
        redbox_state.mac_map.erase(mac_itr);

next:
        mac_itr = mac_itr_next;
    }

    redbox_state.nt_last_poll_msecs = now_msecs;

    // This may have caused the NT/PNT table no longer to be full or to become
    // full. Update notifications.
    REDBOX_BASE_notification_status_update(redbox_state);
}

/******************************************************************************/
// REDBOX_BASE_mesa_proxy_node_to_mesa_node_port()
/******************************************************************************/
static mesa_rb_node_port_t REDBOX_BASE_mesa_proxy_node_to_mesa_node_port(mesa_rb_proxy_node_t mesa_pnt_entry)
{
    mesa_rb_node_port_t res = {};

    res.age              = mesa_pnt_entry.age;
    res.cnt.rx           = mesa_pnt_entry.cnt.rx;
    res.cnt.rx_wrong_lan = mesa_pnt_entry.cnt.rx_wrong_lan;

    return res;
}

/******************************************************************************/
// REDBOX_BASE_pnt_poll()
// Poll ProxyNodeTable.
/******************************************************************************/
static void REDBOX_BASE_pnt_poll(redbox_state_t &redbox_state)
{
    redbox_mac_itr_t        mac_itr, mac_itr_next;
    mesa_rb_proxy_node_t    mesa_pnt_entry;
    mesa_rb_proxy_node_id_t id;
    uint64_t                now_secs;
    uint64_t                now_msecs = vtss::uptime_milliseconds();
    bool                    new_entry;

    if (now_msecs - redbox_state.pnt_last_poll_msecs < 2000) {
        // Don't poll faster than every two seconds
        return;
    }

    T_IG(REDBOX_TRACE_GRP_API, "%u: PNT Poll Start", redbox_state.inst);

    // Clear present flag in all current PNT entries.
    for (mac_itr = redbox_state.mac_map.begin(); mac_itr != redbox_state.mac_map.end(); ++mac_itr) {
        if (mac_itr->second.is_pnt_entry) {
            mac_itr->second.pnt.present_in_pnt = false;
        }
    }

    now_secs = vtss::uptime_seconds();

    // Poll ProxyNodeTable. Poll by ID rather than by MAC, because polling by ID
    // is much faster.
    id = 0;
    while (mesa_rb_proxy_node_id_get_next(nullptr, redbox_state.rb_id_get(), id, &mesa_pnt_entry) == VTSS_RC_OK) {
        T_DG(REDBOX_TRACE_GRP_API, "%u: %s", redbox_state.inst, mesa_pnt_entry);

        // Prepare for next iteration
        id = mesa_pnt_entry.id;

        new_entry = false;

        if ((mac_itr = redbox_state.mac_map.find(mesa_pnt_entry.mac)) == redbox_state.mac_map.end()) {
            // Create a new entry
            if ((mac_itr = redbox_state.mac_map.get(mesa_pnt_entry.mac)) == redbox_state.mac_map.end()) {
                T_EG(REDBOX_TRACE_GRP_API, "%u::%s: Out of memory", redbox_state.inst, mesa_pnt_entry.mac);
                // RBNTBD: Update operational state?
                continue;
            }

            new_entry = true;
            vtss_clear(mac_itr->second);
        } else {
            if (!mac_itr->second.is_pnt_entry) {
                // This changes from being an NT entry to being a PNT entry, so
                // it's basically a MAC mode.
                // Clear the status to prevent the NT status from being
                // interpreted as PNT status.
                vtss_clear(mac_itr->second);
            }
        }

        // Update the entry
        mac_itr->second.is_pnt_entry       = true;
        mac_itr->second.pnt.present_in_pnt = true;
        mac_itr->second.pnt.status.locked  = mesa_pnt_entry.locked;

        // Convert mesa_pnt_entry to a mesa_rb_node_port_t to perform unified
        // NT/PNT operations below.
        mesa_rb_node_port_t node_port = REDBOX_BASE_mesa_proxy_node_to_mesa_node_port(mesa_pnt_entry);
        REDBOX_BASE_nt_pnt_port_fill(mac_itr->second.pnt.status.port, node_port, now_secs);

        // Update the node type.
        // Notice that the PNT only is affected by reception of SV frames in
        // HSR-PRP mode. This happens when a PRP SV frame is received from the
        // PRP network and hits this RB's interlink.
        // When this happens, we change the PNT entry's H/W node type from SAN
        // to DAN. When the H/W node type is DAN, all frames hitting the RB from
        // that SMAC on the PRP network MUST carry an RCT, or they will be
        // discarded.
        if (mac_itr->second.pnt.status.port.sv_last_seen_secs) {
            // A SV frame relating to this MAC address has been seen. Reception
            // of a SV frame causes S/W to change the node type in H/W to a DAN,
            // while setting the S/W node type in accordance with the received
            // SV frame. H/W cannot chage the node type back to SAN, so there is
            // nothing else to do here.
        } else {
            // No SV frame has been received relating to this MAC address.
            vtss_appl_redbox_node_type_t &node_type = mac_itr->second.pnt.status.node_type;
            if (mac_itr->second.pnt.status.locked) {
                // It's a locked entry, so we know it's one of our own, so it
                // will either be a SAN or a DAN. Convert to right types.
                switch (redbox_state.conf.mode) {
                case VTSS_APPL_REDBOX_MODE_PRP_SAN:
                    node_type = mesa_pnt_entry.type == MESA_RB_PROXY_NODE_TYPE_SAN ? VTSS_APPL_REDBOX_NODE_TYPE_VDANP : VTSS_APPL_REDBOX_NODE_TYPE_DANP_RB;
                    break;

                case VTSS_APPL_REDBOX_MODE_HSR_PRP:
                    // We report nodes as being P nodes, not H nodes - except
                    // for the RedBox itself.
                    node_type = mesa_pnt_entry.type == MESA_RB_PROXY_NODE_TYPE_SAN ? VTSS_APPL_REDBOX_NODE_TYPE_VDANP : VTSS_APPL_REDBOX_NODE_TYPE_DANH_RB;
                    break;

                default:
                    // HSR-SAN, HSR-HSR
                    node_type = mesa_pnt_entry.type == MESA_RB_PROXY_NODE_TYPE_SAN ? VTSS_APPL_REDBOX_NODE_TYPE_VDANH : VTSS_APPL_REDBOX_NODE_TYPE_DANH_RB;
                    break;
                }
            } else {
                // It's a non-locked entry, so it has been added by H/W by the
                // mere reception of a frame on the I/L port from the
                // switch-side. This is always learned in the PNT as a SAN
                // (until a possible SV frame arrives from the PRP network in
                // HSR-PRP mode).
                if (mesa_pnt_entry.type == MESA_RB_PROXY_NODE_TYPE_DAN) {
                    T_EG(REDBOX_TRACE_GRP_BASE, "%u: The H/W PNT contains a non-locked entry (%s) of type DAN, but we haven't received any SV frames from that MAC, so it can't be a DAN", redbox_state.inst, mac_itr->first);
                }

                switch (redbox_state.conf.mode) {
                case VTSS_APPL_REDBOX_MODE_PRP_SAN:
                case VTSS_APPL_REDBOX_MODE_HSR_PRP:
                    // Notice that in HSR-PRP mode, we report PNT nodes as P
                    // nodes, not H nodes.
                    node_type = VTSS_APPL_REDBOX_NODE_TYPE_VDANP;
                    break;

                default:
                    // HSR-SAN, HSR-HSR
                    node_type = VTSS_APPL_REDBOX_NODE_TYPE_VDANH;
                    break;
                }
            }
        }

        if (new_entry) {
            // Create a supervision PDU to be sent regularly for this one.
            redbox_pdu_tx_start(redbox_state, mac_itr);
        }
    }

    // Loop through map and remove those that were not set in the above loop.
    mac_itr = redbox_state.mac_map.begin();
    while (mac_itr != redbox_state.mac_map.end()) {
        mac_itr_next = mac_itr;
        ++mac_itr_next;

        if (!mac_itr->second.is_pnt_entry) {
            goto next;
        }

        if (!mac_itr->second.pnt.present_in_pnt) {
            redbox_pdu_tx_stop(redbox_state, mac_itr);
            redbox_state.mac_map.erase(mac_itr);
            goto next;
        }

        if (mac_itr->second.pnt.status.locked) {
            // This is our own entry that never times out.
            goto next;
        }

        // If we are still here, the entry exists in the PNT table.
        // We need to figure out whether the RedBox has seen frames from that
        // entry or if it is only in the PNT because we have added it
        // artifically because of a reception of a PRP SV frames from the PRP
        // network (in HSR-PRP mode).

        if (mac_itr->second.pnt.status.port.rx_cnt) {
            // Frames have really been received from this entry. Leave it alone.
            // Notice: The rx_cnt counter is NOT cleared when clearing RedBox
            // statistics, which is good, because otherwise this piece of code
            // wouldn't work!
            goto next;
        }

        if (mac_itr->second.pnt.status.port.sv_last_seen_secs) {
            // Check to see if the SV frame has timed out. We time SV frame
            // receptions out after the PNT's age time.
            if (now_secs - mac_itr->second.pnt.status.port.sv_last_seen_secs <= redbox_state.conf.pnt_age_time_secs) {
                // SV frame not timed out. Leave the entry in the table.
                goto next;
            }

            // SV frame also timed out.
        } else {
            // If it is not there because of reception of a SV frame, it's a
            // code bug.
            T_EG(REDBOX_TRACE_GRP_FRAME_RX, "%u: Expected sv_last_seen_secs to be non-zero %s", redbox_state.inst, mac_itr->second.pnt);
        }

        // We have added this entry ourselves, but it has timed out. Remove it
        // from H/W...
        REDBOX_BASE_mesa_pnt_del(redbox_state, mac_itr);

        // ...and from S/W
        redbox_pdu_tx_stop(redbox_state, mac_itr);
        redbox_state.mac_map.erase(mac_itr);

next:
        mac_itr = mac_itr_next;
    }

    redbox_state.pnt_last_poll_msecs = now_msecs;

    // This may have caused the NT/PNT table to no longer be full or to have
    // become full. Update notifications.
    REDBOX_BASE_notification_status_update(redbox_state);
}

/******************************************************************************/
// REDBOX_BASE_nt_poll_timeout()
// Poll NodesTable
/******************************************************************************/
static void REDBOX_BASE_nt_poll_timeout(redbox_timer_t &timer, const void *context)
{
    redbox_itr_t redbox_itr;

    if ((redbox_itr = REDBOX_map.find(timer.instance)) == REDBOX_map.end()) {
        T_EG(REDBOX_TRACE_GRP_BASE, "Unable to find RedBox instance #%u in map", timer.instance);
        redbox_timer_stop(timer);
        return;
    }

    REDBOX_BASE_nt_poll(redbox_itr->second);

    // Restart the timer if the current timeout is not equal to half the
    // configured age time.
    if (timer.period_ms != redbox_itr->second.conf.nt_age_time_secs * 500) {
        redbox_timer_start(timer, redbox_itr->second.conf.nt_age_time_secs * 500, true);
    }
}

/******************************************************************************/
// REDBOX_BASE_pnt_poll_timeout()
// Poll ProxyNodeTable
/******************************************************************************/
static void REDBOX_BASE_pnt_poll_timeout(redbox_timer_t &timer, const void *context)
{
    redbox_itr_t redbox_itr;

    if ((redbox_itr = REDBOX_map.find(timer.instance)) == REDBOX_map.end()) {
        T_EG(REDBOX_TRACE_GRP_BASE, "Unable to find RedBox instance #%u in map", timer.instance);
        redbox_timer_stop(timer);
        return;
    }

    REDBOX_BASE_pnt_poll(redbox_itr->second);

    // Restart the timer if the current timeout is not equal to the configured.
    if (timer.period_ms != redbox_itr->second.conf.sv_frame_interval_secs * 1000) {
        redbox_timer_start(timer, redbox_itr->second.conf.sv_frame_interval_secs * 1000, true);
    }
}

/******************************************************************************/
// REDBOX_BASE_ace_update()
/******************************************************************************/
static mesa_rc REDBOX_BASE_ace_update(void)
{
    redbox_itr_t     redbox_itr;
    mesa_port_list_t dest_port_list;
    bool             need_ace;
    acl_entry_conf_t ace_conf;
    mesa_rc          rc;

    // Assume we need to send the frame to all ports.
    dest_port_list.set_all();

    // Assume we don't need to install an ACE
    need_ace = false;

    // We don't send SV frames to any I/L ports, except for one special case:
    // If the RB is in HSR-PRP mode and it is configured to forward SV frames as
    // is (conf.sv_xlat_prp_to_hsr is false), we H/W forward towards
    // that RB from the PRP network. As long as the SMAC of the SV frame is not
    // in PNT or it is learned as a SAN in PNT, the RB will add its own HSR tag
    // and its own SeqNr for that SMAC and will not strip the RCT. This is fine,
    // because this is exactly as if it was a normal data frame.
    for (redbox_itr = REDBOX_map.begin(); redbox_itr != REDBOX_map.end(); ++redbox_itr) {
        if (redbox_itr->second.status.oper_state != VTSS_APPL_REDBOX_OPER_STATE_ACTIVE) {
            continue;
        }

        if (redbox_itr->second.conf.mode == VTSS_APPL_REDBOX_MODE_HSR_PRP) {
            // We need indeed an ACE.
            need_ace = true;

            if (!redbox_itr->second.conf.sv_xlat_prp_to_hsr) {
                // H/W forwards SV frames to this RB (see details above).
                continue;
            }
        }

        // Don't send the frame to the interlink of this RedBox.
        dest_port_list[redbox_itr->second.interlink_port_no_get()] = false;
    }

    if (need_ace) {
        if (REDBOX_BASE_ace_id == ACL_MGMT_ACE_ID_NONE) {
            // We haven't created an ACE yet.
            // Initialize it
            T_IG(REDBOX_TRACE_GRP_BASE, "acl_mgmt_ace_init()");
            if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ETYPE, &ace_conf)) != VTSS_RC_OK) {
                T_EG(REDBOX_TRACE_GRP_BASE, "acl_mgmt_ace_init() failed: %s", error_txt(rc));
            }

            // Fill in invariable fields
            ace_conf.isid = VTSS_ISID_LOCAL;
            ace_conf.frame.etype.etype.value[0] = (REDBOX_ETYPE_SUPERVISION >> 8) & 0xff;
            ace_conf.frame.etype.etype.value[1] = (REDBOX_ETYPE_SUPERVISION >> 0) & 0xff;
            ace_conf.frame.etype.etype.mask[0]  = 0xff;
            ace_conf.frame.etype.etype.mask[1]  = 0xff;

            // Match on all ports. We don't match on VLAN ID, because that would
            // require up to 4K ACEs in order to match on all VLANs that a
            // RedBox I/L port could be a member of.
            ace_conf.port_list.set_all();

            // Make sure to copy the SV frame to the CPU.
            ace_conf.action.force_cpu = true;

            // And configure the ACE to use action.port_list for forwarding
            ace_conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
        } else {
            // We have already created such an entry. Get the current conf.
            T_IG(REDBOX_TRACE_GRP_BASE, "acl_mgmt_ace_get(0x%x)", REDBOX_BASE_ace_id);
            if ((rc = acl_mgmt_ace_get(ACL_USER_REDBOX, REDBOX_BASE_ace_id, &ace_conf, nullptr, FALSE)) != VTSS_RC_OK) {
                T_EG(REDBOX_TRACE_GRP_BASE, "acl_mgmt_ace_get(0x%x) failed: %s", REDBOX_BASE_ace_id, error_txt(rc));
                return VTSS_APPL_REDBOX_RC_INTERNAL_ERROR;
            }

            if (ace_conf.action.port_list == dest_port_list) {
                // No changes, so nothing to do.
                return VTSS_RC_OK;
            }
        }

        // Variable fields
        ace_conf.action.port_list = dest_port_list;

        // Update or create the rule
        VTSS_RC(REDBOX_BASE_ace_add(-1, REDBOX_BASE_ace_id, ace_conf));
    } else {
        // We don't need an ACE anymore. Delete any we might have.
        REDBOX_BASE_ace_del(-1, REDBOX_BASE_ace_id);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// REDBOX_BASE_port_role_do_update()
/******************************************************************************/
static void REDBOX_BASE_port_role_do_update(redbox_state_t &redbox_state, mesa_port_no_t port_no, redbox_port_role_type_t new_role_type)
{
    redbox_port_role_t &cur_role = redbox_port_roles[port_no];

    T_IG_PORT(REDBOX_TRACE_GRP_BASE, port_no, "Role Type: %s->%s", REDBOX_BASE_port_role_type_to_str(cur_role.type), REDBOX_BASE_port_role_type_to_str(new_role_type));
    if (new_role_type != REDBOX_PORT_ROLE_TYPE_NORMAL && cur_role.type != REDBOX_PORT_ROLE_TYPE_NORMAL) {
        T_EG(REDBOX_TRACE_GRP_BASE, "Internal error: %u: Activating with role = %s, but port_roles[%u] is not normal but %s", redbox_state.inst, REDBOX_BASE_port_role_type_to_str(new_role_type), port_no, REDBOX_BASE_port_role_type_to_str(cur_role.type));
    } else {
        // Don't throw a trace error when deactivating, because it could happen
        // that one attempts to deactivate twice without activating in the
        // meantime.
    }

    cur_role.type = new_role_type;

    if (new_role_type == REDBOX_PORT_ROLE_TYPE_INTERLINK) {
        cur_role.redbox_state = &redbox_state;
    } else {
        cur_role.redbox_state = nullptr;
    }
}

/******************************************************************************/
// REDBOX_BASE_port_role_update()
/******************************************************************************/
static void REDBOX_BASE_port_role_update(redbox_state_t &redbox_state)
{
    vtss_appl_redbox_port_type_t port_type;
    redbox_port_role_type_t      new_role_type;
    bool                         activate;

    activate = redbox_state.status.oper_state == VTSS_APPL_REDBOX_OPER_STATE_ACTIVE;

    for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_B; port_type++) {
        if (!redbox_state.using_port_x(port_type)) {
            continue;
        }

        new_role_type = activate ? redbox_state.interlink_port_type_get() == port_type ? REDBOX_PORT_ROLE_TYPE_INTERLINK : REDBOX_PORT_ROLE_TYPE_UNCONNECTED : REDBOX_PORT_ROLE_TYPE_NORMAL;
        REDBOX_BASE_port_role_do_update(redbox_state, redbox_state.port_states[port_type]->port_no, new_role_type);
    }
}

/******************************************************************************/
// redbox_base_activate()
/******************************************************************************/
static mesa_rc REDBOX_BASE_do_activate(redbox_state_t &redbox_state)
{
    vtss_appl_redbox_port_type_t     port_type;
    redbox_port_state_t              *interlink_port_state;
    vtss_appl_redbox_oper_state_t    oper_state    = redbox_state.status.oper_state;
    vtss_appl_redbox_oper_warnings_t oper_warnings = redbox_state.status.oper_warnings;

    vtss_clear(redbox_state.status);
    redbox_state.status.oper_state    = oper_state;
    redbox_state.status.oper_warnings = oper_warnings;

    redbox_state.rb_mac_discard_ace_id = ACL_MGMT_ACE_ID_NONE;

    // Update the global port role array with our role.
    REDBOX_BASE_port_role_update(redbox_state);

    interlink_port_state = redbox_state.interlink_port_state_get();

    // Update the status' interlink port ifindex.
    (void)vtss_ifindex_from_port(VTSS_ISID_START, redbox_state.interlink_port_no_get(), &redbox_state.status.port_c);

    // Create a SAN entry in the PNT table for our management MAC address. It
    // will appear in our PNT status upon the next poll.
    VTSS_RC(REDBOX_BASE_mesa_pnt_add(redbox_state, *interlink_port_state->chassis_mac, MESA_RB_PROXY_NODE_TYPE_SAN, true /* create it locked */));

    // Create a DAN entry in the PNT table for this RedBox. It will appear in
    // our PNT status upon the next poll.
    VTSS_RC(REDBOX_BASE_mesa_pnt_add(redbox_state, interlink_port_state->redbox_mac, MESA_RB_PROXY_NODE_TYPE_DAN, true /* create it locked */));

    // Prevent the RB MAC address from being forwarded anywhere. This is
    // particularly useful in HSR-PRP mode, where frames arriving from the PRP
    // network destined to the RB MAC must not go to the HSR ring. We add it in
    // all modes, however.
    VTSS_RC(REDBOX_BASE_rb_mac_discard(redbox_state, true /* add it */));

    // Create ACL rule neccessary to get SV frames to the CPU from non-LRE
    // ports if we are in HSR-PRP or HSR-HSR mode.
    VTSS_RC(REDBOX_BASE_ace_update());

    // We create supervision PDU templates once and for all and use the same
    // frame for all proxied SANs.
    redbox_pdu_tx_templates_create(redbox_state);

    // Create and start the NodesTable poll timer. We poll with half the NT age
    // time.
    redbox_timer_init(redbox_state.nt_poll_timer, "NT Poll", redbox_state.inst, REDBOX_BASE_nt_poll_timeout, nullptr);
    redbox_timer_start(redbox_state.nt_poll_timer, redbox_state.conf.nt_age_time_secs * 500, true);

    // Create and start the ProxyNodeTable poll timer. We poll with the same
    // interval as we transmit proxied Supervision PDUs with.
    redbox_timer_init(redbox_state.pnt_poll_timer, "PNT Poll", redbox_state.inst, REDBOX_BASE_pnt_poll_timeout, nullptr);
    redbox_timer_start(redbox_state.pnt_poll_timer, redbox_state.conf.sv_frame_interval_secs * 1000, true);

    // Create and start a 10-second timer that checks whether the NT/PNT table
    // is (still) full.
    redbox_timer_init(redbox_state.nt_pnt_table_full_timer, "NT/PNT Full", redbox_state.inst, REDBOX_BASE_notification_update_timeout, nullptr);
    redbox_timer_start(redbox_state.nt_pnt_table_full_timer, 10000, true);

    // Create and start a that checks poll statistics.
    redbox_timer_init(redbox_state.statistics_poll_timer, "Stati Poll", redbox_state.inst, REDBOX_BASE_statistics_poll_timeout, nullptr);
    redbox_timer_start(redbox_state.statistics_poll_timer, REDBOX_cap.statistics_poll_interval_secs * 1000, true);

    // Create two types of timers that - when started - indicate that an
    // untagged HSR frame was received on an LRE port and an RCT-tagged frame
    // was received on Port A/B/C with wrong LanId.
    for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_C; port_type++) {
        redbox_timer_init(redbox_state.cnt_err_wrong_lan_timers[port_type], VTSS_APPL_REDBOX_PORT_TYPE_A ? "CntErrWrongLanA"        : VTSS_APPL_REDBOX_PORT_TYPE_B ? "CntErrWrongLanB"        : "CntErrWrongLanC",        redbox_state.inst, REDBOX_BASE_notification_update_timeout, nullptr);
        redbox_timer_init(redbox_state.hsr_untagged_rx_timers[port_type],   VTSS_APPL_REDBOX_PORT_TYPE_A ? "HSR-untagged Rx Port A" : VTSS_APPL_REDBOX_PORT_TYPE_B ? "HSR-untagged Rx Port B" : "HSR-untagged Rx Port C", redbox_state.inst, REDBOX_BASE_notification_update_timeout, nullptr);
    }

    // Create an entry in the global notification status table.
    REDBOX_BASE_notification_status_update(redbox_state, false /* Don't get status first, because that will result in a trace error */);

    // Register for reception of various frame types
    REDBOX_BASE_packet_rx_filters_update();

    VTSS_RC(REDBOX_BASE_mesa_rb_conf_update(redbox_state, true /* activating */));

    // Force an update of the S/W-based PNT and NT, because if the user asks
    // for any of these before the NT or PNT poll timers have elapsed, they will
    // be empty.
    redbox_state.nt_last_poll_msecs = 0;
    redbox_state.pnt_last_poll_msecs = 0;
    REDBOX_BASE_nt_poll( redbox_state);
    REDBOX_BASE_pnt_poll(redbox_state);

    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_base_deactivate()
/******************************************************************************/
mesa_rc redbox_base_deactivate(redbox_state_t &redbox_state)
{
    vtss_appl_redbox_port_type_t port_type;
    mesa_rb_conf_t               rb_conf;

    for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_C; port_type++) {
        redbox_timer_stop(redbox_state.cnt_err_wrong_lan_timers[port_type]);
        redbox_timer_stop(redbox_state.hsr_untagged_rx_timers[port_type]);
    }

    T_IG(REDBOX_TRACE_GRP_BASE, "Deactivating mode %s. oper_state = %s", redbox_util_mode_to_str(redbox_state.conf.mode), redbox_util_oper_state_to_str(redbox_state.status.oper_state));

    redbox_timer_stop(redbox_state.nt_pnt_table_full_timer);
    redbox_timer_stop(redbox_state.nt_poll_timer);
    redbox_timer_stop(redbox_state.pnt_poll_timer);
    redbox_timer_stop(redbox_state.statistics_poll_timer);
    redbox_pdu_tx_free(redbox_state); // Also stops all PNT timers.

    // Unregister reception of various frame types
    REDBOX_BASE_packet_rx_filters_update();

    // Unregister us from a possible ACE
    (void)REDBOX_BASE_ace_update();

    // Disable the RedBox
    vtss_clear(rb_conf);
    rb_conf.mode = MESA_RB_MODE_DISABLED;
    (void)REDBOX_BASE_mesa_rb_conf_set(redbox_state, rb_conf);

    // Clear the NT and PNT in H/W...
    REDBOX_BASE_mesa_nt_clear( redbox_state, true /* also locked entries */);
    REDBOX_BASE_mesa_pnt_clear(redbox_state, true /* also locked entries */);

    // ...and in S/W
    redbox_state.mac_map.clear();
    redbox_state.nt_last_poll_msecs = 0;
    redbox_state.pnt_last_poll_msecs = 0;

    // Clear statistics
    (void)REDBOX_BASE_mesa_statistics_clear(redbox_state);
    vtss_clear(redbox_state.cnt_err_wrong_lan);
    vtss_clear(redbox_state.statistics);

    redbox_state.sup_sequence_number = 0;
    redbox_state.redundancy_tag_sequence_number = 0;
    redbox_state.interlink_member_of_tx_vlan = false;

    // Delete our entry in the global notification status table.
    (void)redbox_notification_status.del(redbox_state.inst);

    // Remove the ACE that prevents frames to the RB MAC from going anywhere.
    (void)REDBOX_BASE_rb_mac_discard(redbox_state, false /* delete */);

    REDBOX_BASE_port_role_update(redbox_state);

    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_base_deactivate()
/******************************************************************************/
mesa_rc redbox_base_activate(redbox_state_t &redbox_state)
{
    mesa_rc rc;

    T_IG(REDBOX_TRACE_GRP_BASE, "Activating mode %s. oper_state = %s", redbox_util_mode_to_str(redbox_state.conf.mode), redbox_util_oper_state_to_str(redbox_state.status.oper_state));

    if ((rc = REDBOX_BASE_do_activate(redbox_state)) != VTSS_RC_OK) {
        // Gotta undo all the initializations we did. We heavily rely on
        // oper_state, so set it to non-active.
        redbox_state.status.oper_state = VTSS_APPL_REDBOX_OPER_STATE_INTERNAL_ERROR;
        (void)redbox_base_deactivate(redbox_state);
    }

    return rc;
}

/******************************************************************************/
// redbox_base_notification_status_update()
/******************************************************************************/
void redbox_base_notification_status_update(redbox_state_t &redbox_state)
{
    REDBOX_BASE_notification_status_update(redbox_state);
}

/******************************************************************************/
// redbox_base_api_conf_changed()
/******************************************************************************/
void redbox_base_api_conf_changed(redbox_state_t &redbox_state)
{
    (void)REDBOX_BASE_mesa_rb_conf_update(redbox_state, false /* updating */);
}

/******************************************************************************/
// redbox_base_sv_frame_interval_changed()
/******************************************************************************/
void redbox_base_sv_frame_interval_changed(redbox_state_t &redbox_state)
{
    uint64_t new_period_ms = redbox_state.conf.sv_frame_interval_secs * 1000;

    if (redbox_timer_active(redbox_state.pnt_poll_timer)) {
        // If the new timeout is smaller than the old, we restart the PNT poll timer
        if (new_period_ms < redbox_state.pnt_poll_timer.period_ms) {
            // Restart.
            redbox_timer_start(redbox_state.pnt_poll_timer, new_period_ms, true);
        } else {
            // New timeout is larger than the current. Let the timeout function
            // restart it.
        }
    }

    // Also update the per-MAC-address timers.
    redbox_pdu_frame_interval_changed(redbox_state);
}

/******************************************************************************/
// redbox_base_nt_age_time_changed()
/******************************************************************************/
void redbox_base_nt_age_time_changed(redbox_state_t &redbox_state)
{
    redbox_mac_itr_t mac_itr;

    // We need to clear parts of the S/W-based NT map when the age time changes
    // (not the H/W-based), because we need to calculate new "last_seen_secs"
    // upon the next call to REDBOX_BASE_nt_poll().
    // We only clear the H/W-based entries (not the SV frame properties).
    for (mac_itr = redbox_state.mac_map.begin(); mac_itr != redbox_state.mac_map.end(); ++mac_itr) {
        if (mac_itr->second.is_pnt_entry) {
            continue;
        }

        vtss_clear(mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_A]);
        vtss_clear(mac_itr->second.nt.status.port[VTSS_APPL_REDBOX_PORT_TYPE_B]);
    }

    // Force a poll next time.
    redbox_state.nt_last_poll_msecs = 0;

    // And get the API updated.
    redbox_base_api_conf_changed(redbox_state);
}

/******************************************************************************/
// redbox_base_xlat_prp_to_hsr_changed()
/******************************************************************************/
void redbox_base_xlat_prp_to_hsr_changed(redbox_state_t &redbox_state)
{
    // Gotta update the ACEs to either get SV frames forwarded by H/W towards
    // LRE ports or to get them copied to the CPU.
    (void)REDBOX_BASE_ace_update();
}

/******************************************************************************/
// redbox_base_nt_poll()
/******************************************************************************/
void redbox_base_nt_poll(redbox_state_t &redbox_state)
{
    REDBOX_BASE_nt_poll(redbox_state);
}

/******************************************************************************/
// redbox_base_pnt_poll()
/******************************************************************************/
void redbox_base_pnt_poll(redbox_state_t &redbox_state)
{
    REDBOX_BASE_pnt_poll(redbox_state);
}

/******************************************************************************/
// redbox_base_nt_clear()
/******************************************************************************/
mesa_rc redbox_base_nt_clear(redbox_state_t &redbox_state)
{
    redbox_mac_itr_t mac_itr, mac_itr_next;

    // Clear H/W table
    REDBOX_BASE_mesa_nt_clear(redbox_state, false /* only unlocked entries */);

    // And S/W table
    mac_itr = redbox_state.mac_map.begin();
    while (mac_itr != redbox_state.mac_map.end()) {
        mac_itr_next = mac_itr;
        ++mac_itr_next;

        if (!mac_itr->second.is_pnt_entry) {
            redbox_state.mac_map.erase(mac_itr);
        }

        mac_itr = mac_itr_next;
    }

    // This may have caused the NT/PNT table no longer to be full or to become
    // full. Update notifications.
    REDBOX_BASE_notification_status_update(redbox_state);

    // Force a poll next time.
    redbox_state.nt_last_poll_msecs = 0;
    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_base_pnt_clear()
/******************************************************************************/
mesa_rc redbox_base_pnt_clear(redbox_state_t &redbox_state)
{
    redbox_mac_itr_t mac_itr, mac_itr_next;

    REDBOX_BASE_mesa_pnt_clear(redbox_state, false /* only unlocked entries */);

    // Also clear all non-locked S/W entries.
    mac_itr = redbox_state.mac_map.begin();
    while (mac_itr != redbox_state.mac_map.end()) {
        mac_itr_next = mac_itr;
        ++mac_itr_next;

        if (mac_itr->second.is_pnt_entry && !mac_itr->second.pnt.status.locked) {
            redbox_pdu_tx_stop(redbox_state, mac_itr);
            redbox_state.mac_map.erase(mac_itr);
        }

        mac_itr = mac_itr_next;
    }

    // This may have caused the NT/PNT table no longer to be full or to become
    // full. Update notifications.
    REDBOX_BASE_notification_status_update(redbox_state);

    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_base_statistics_get()
/******************************************************************************/
mesa_rc redbox_base_statistics_get(redbox_state_t &redbox_state, vtss_appl_redbox_statistics_t &statistics)
{
    mesa_rb_counters_t mesa_counters;

    vtss_clear(statistics);
    VTSS_RC(REDBOX_BASE_statistics_get_and_notif_update(redbox_state, mesa_counters));
    REDBOX_BASE_mesa_to_appl_statistics(redbox_state, mesa_counters, statistics);
    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_base_statistics_clear()
/******************************************************************************/
mesa_rc redbox_base_statistics_clear(redbox_state_t &redbox_state)
{
    vtss_appl_redbox_port_type_t port_type;

    VTSS_RC(REDBOX_BASE_mesa_statistics_clear(redbox_state));

    // Also clear S/W-based counters.
    vtss_clear(redbox_state.statistics);

    // Clearing the statistics potentially clears the wrong_lan and/or
    // hsr_untagged notifications, so we need to update the notification status
    // as well.
    for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_C; port_type++) {
        redbox_state.cnt_err_wrong_lan[port_type] = 0;
        redbox_timer_stop(redbox_state.cnt_err_wrong_lan_timers[port_type]);
        redbox_state.hsr_untagged_rx[port_type] = 0;
        redbox_timer_stop(redbox_state.hsr_untagged_rx_timers[port_type]);
    }

    REDBOX_BASE_notification_status_update(redbox_state);

    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_base_port_is_lre_port()
/******************************************************************************/
bool redbox_base_port_is_lre_port(mesa_port_no_t port_no)
{
    return redbox_port_roles[port_no].type != REDBOX_PORT_ROLE_TYPE_NORMAL;
}

/******************************************************************************/
// redbox_base_debug_port_state_dump()
/******************************************************************************/
void redbox_base_debug_port_state_dump(redbox_icli_pr_t pr)
{
    vtss_appl_redbox_sv_type_t sv_type;
    bool                       all_are_zero;
    char                       if_str[40];
    uint32_t                   printed_cnt;
    mesa_port_no_t             port_no;

    pr("Interface  Role\n");
    pr("---------- -----------\n");

    for (port_no = 0; port_no < REDBOX_cap_port_cnt; port_no++) {
        pr("%-10s %s\n",
           icli_port_info_txt_short(VTSS_ISID_START, iport2uport(port_no), if_str),
           REDBOX_BASE_port_role_type_to_str(redbox_port_roles[port_no].type));
    }

    pr("\n");

    // Print per-port counters
    pr("Interface  %-10s %-10s %-10s Erroneous  Filtered\n", redbox_util_sv_type_to_str(VTSS_APPL_REDBOX_SV_TYPE_PRP_DD), redbox_util_sv_type_to_str(VTSS_APPL_REDBOX_SV_TYPE_PRP_DA), redbox_util_sv_type_to_str(VTSS_APPL_REDBOX_SV_TYPE_HSR));
    pr("---------- ---------- ---------- ---------- ---------- ----------\n");

    printed_cnt = 0;
    for (port_no = 0; port_no < REDBOX_cap_port_cnt; port_no++) {
        redbox_base_sv_rx_port_cnt_t &p = REDBOX_BASE_sv_port_cnt[port_no];

        all_are_zero = true;
        for (sv_type = (vtss_appl_redbox_sv_type_t)0; sv_type < VTSS_APPL_REDBOX_SV_TYPE_CNT; sv_type++) {
            if (p.cnt[sv_type]) {
                all_are_zero = false;
                break;
            }
        }

        if (all_are_zero && p.err_cnt == 0 && p.filtered_cnt == 0) {
            continue;
        }

        pr("%-10s " VPRI64Fu("10") " " VPRI64Fu("10") " " VPRI64Fu("10") " " VPRI64Fu("10") " " VPRI64Fu("10") "\n",
           icli_port_info_txt_short(VTSS_ISID_START, iport2uport(port_no), if_str),
           p.cnt[VTSS_APPL_REDBOX_SV_TYPE_PRP_DD],
           p.cnt[VTSS_APPL_REDBOX_SV_TYPE_PRP_DA],
           p.cnt[VTSS_APPL_REDBOX_SV_TYPE_HSR],
           p.err_cnt,
           p.filtered_cnt);

        printed_cnt++;
    }

    if (printed_cnt == 0) {
        pr("<No ports have received any SV frames captured by the CPU>\n");
    }

    pr("\n");
}

/******************************************************************************/
// redbox_base_debug_state_dump()
/******************************************************************************/
void redbox_base_debug_state_dump(redbox_state_t &redbox_state, redbox_icli_pr_t pr)
{
    pr("Inst:                           %u\n", redbox_state.inst);
    pr("Operational state:              %s\n", redbox_util_oper_state_to_str(redbox_state.status.oper_state));
    pr("tx_spv_suspend_own:             %d\n", redbox_state.tx_spv_suspend_own);
    pr("tx_spv_suspend_proxied:         %d\n", redbox_state.tx_spv_suspend_proxied);
    pr("sup_sequence_number:            %u\n", redbox_state.sup_sequence_number);
    pr("redundancy_tag_sequence_number: %u\n", redbox_state.redundancy_tag_sequence_number);
    pr("\n");
}

/******************************************************************************/
// redbox_base_debug_tx_spv_suspend_get()
/******************************************************************************/
void redbox_base_debug_tx_spv_suspend_get(redbox_state_t &redbox_state, bool &own_suspended, bool &proxied_suspended)
{
    own_suspended     = redbox_state.tx_spv_suspend_own;
    proxied_suspended = redbox_state.tx_spv_suspend_proxied;
}

/******************************************************************************/
// redbox_base_debug_tx_spv_suspend_set()
/******************************************************************************/
void redbox_base_debug_tx_spv_suspend_set(redbox_state_t &redbox_state, bool suspend_own, bool suspend_proxied)
{
    redbox_state.tx_spv_suspend_own     = suspend_own;
    redbox_state.tx_spv_suspend_proxied = suspend_proxied;
}

/******************************************************************************/
// redbox_base_debug_clear_notifications()
/******************************************************************************/
void redbox_base_debug_clear_notifications(redbox_state_t &redbox_state)
{
    vtss_appl_redbox_port_type_t port_type;

    // Stop timer-based notifications
    for (port_type = VTSS_APPL_REDBOX_PORT_TYPE_A; port_type <= VTSS_APPL_REDBOX_PORT_TYPE_C; port_type++) {
        redbox_timer_stop(redbox_state.cnt_err_wrong_lan_timers[port_type]);
        redbox_timer_stop(redbox_state.hsr_untagged_rx_timers[port_type]);
    }

    // And update notifications
    REDBOX_BASE_notification_status_update(redbox_state);
}

