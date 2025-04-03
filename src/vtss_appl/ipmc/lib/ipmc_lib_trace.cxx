/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

// Functions in this file are used for tracing.
// The fmt() functions make it possible to use them in T_x() statements with
// "%s" format specifier.

#include "ipmc_lib_trace.hxx"
#include "ipmc_lib_utils.hxx"
#include "ipmc_lib_pdu.hxx"
#include "misc_api.h"         /* For iport2uport() */
#include <vtss/appl/ipmc_lib.h>

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_ip_t &ip)
{
    char buf[40];
    o << ip.print(buf);
    return o;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_ip_t *ip)
{
    o << *ip;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_key_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_key_t &key)
{
    o << "<" << (key.is_mvr ? "MVR" : "IPMC") << "-" << (key.is_ipv4 ? "IGMP" : "MLD") << ">";

    return o;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_key_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_key_t *key)
{
    o << *key; // Using vtss_appl_ipmc_lib_key_t::operator<<()

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_key_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_vlan_key_t &key)
{
    o << "<" << (key.is_mvr ? "MVR" : "IPMC") << "-" << (key.is_ipv4 ? "IGMP" : "MLD") << ", VID = " << key.vid << ">";

    return o;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_key_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_vlan_key_t *key)
{
    o << *key; // Using vtss_appl_ipmc_lib_vlan_key_t::operator<<()

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// ipmc_lib_src_state_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_src_state_t &src_state)
{
    mesa_port_no_t port_no;
    char           if_str[40];

    o << "{next_src_timeout = "    << src_state.next_src_timeout
      << ", next_query_timeout = " << src_state.next_query_timeout
      << ", include_port_list = "  << src_state.include_port_list
      << ", exclude_port_list = "  << src_state.exclude_port_list
      << ", hw_location = "        << ipmc_lib_util_hw_location_to_str(src_state.hw_location)
      << ", changed = "            << src_state.changed;

    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if (!src_state.include_port_list[port_no] && !src_state.exclude_port_list[port_no]) {
            // Port is not active, and we only trace active ports.
            continue;
        }

        const ipmc_lib_src_port_state_t &src_port_state = src_state.ports[port_no];

        o << ", [" << port_no << " (" << icli_port_info_txt_short(VTSS_USID_START, iport2uport(port_no), if_str) << ")] = {"
          << "src_timeout = "     << src_port_state.src_timeout
          << ", query_timeout = " << src_port_state.query_timeout
          << ", tx_cnt_left = "   << src_port_state.tx_cnt_left
          << "}";
    }

    o << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_src_state_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_src_state_t *src_state)
{
    o << *src_state;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// ipmc_lib_src_map_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_src_map_t &src_map)
{
    ipmc_lib_src_map_const_itr_t itr;

    o << "{src_cnt = "             << src_map.size()
      << ", next_src_timeout = "   << src_map.next_src_timeout
      << ", next_query_timeout = " << src_map.next_query_timeout
      << ", changed = "            << src_map.changed;

    for (itr = src_map.cbegin(); itr != src_map.cend(); ++itr) {
        o << ",\n{key = "  << itr->first   // Using vtss_appl_ipmc_lib_ip_t::operator<<()
          << ", value = "  << itr->second  // Using ipmc_lib_src_state_t::operator<<()
          << "}";
    }

    o << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_src_map_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_src_map_t *src_map)
{
    o << *src_map;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// ipmc_lib_src_list_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_src_list_t &src_list)
{
    ipmc_lib_src_list_const_itr_t itr;
    bool                          first = true;

    o << "{";

    for (itr = src_list.cbegin(); itr != src_list.cend(); ++itr) {
        o << (first ? "" : ", ") << *itr; // Using vtss_appl_ipmc_lib_ip_t::operator<<()
        first = false;
    }

    o << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_src_list_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_src_list_t *src_list)
{
    o << *src_list;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// ipmc_lib_grp_port_state_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_grp_port_state_t &port_state)
{
    o << "{grp_timeout = " << port_state.grp_timeout << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_grp_key_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_grp_key_t &grp_key)
{
    // We change the format a bit to what we are used to, because it's annoying
    // to see "{vid = X, grp_addr = a.b.c.d}"
    // This will become something like "<<MVR-IGMP, VID = 17>, 1.2.3.4>"
    o << "<"  << grp_key.vlan_key // Using vtss_appl_ipmc_lib_vlan_key_t::operator<<()
      << ", " << grp_key.grp_addr
      << ">";

    return o;
}

/******************************************************************************/
// ipmc_lib_grp_key_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_grp_key_t *grp_key)
{
    o << *grp_key;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// ipmc_lib_grp_state_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_grp_state_t &grp_state)
{
    mesa_port_no_t port_no;
    char           if_str[40];
    bool           first;

    o << "{active_ports = "                                << grp_state.active_ports
      << ", exclude_mode_ports = "                         << grp_state.exclude_mode_ports
      << ", grp_compat = "                                 << grp_state.grp_compat
      << ", grp_older_version_host_present_timeout_old = " << grp_state.grp_older_version_host_present_timeout_old
      << ", grp_older_version_host_present_timeout_gen = " << grp_state.grp_older_version_host_present_timeout_gen
      << ",\nasm_state = "                                 << grp_state.asm_state     // Using ipmc_lib_src_state_t::operator<<()
      << ",\nsrc_map = "                                   << grp_state.src_map       // Using ipmc_lib_src_map_t::operator<<()
      << ",\nports = ";

    first = true;
    for (port_no = 0; port_no < IPMC_LIB_port_cnt; port_no++) {
        if (grp_state.active_ports[port_no]) {
            o << (first ? "" : ", ") << "[" << port_no << " (" << icli_port_info_txt_short(VTSS_USID_START, iport2uport(port_no), if_str) << ")] = {"
              << grp_state.ports[port_no] // Using ipmc_lib_grp_port_state_t::operator<<()
              << "}";

            first = false;
        }
    }

    o << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_grp_state_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_grp_state_t *grp_state)
{
    o << *grp_state;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// ipmc_lib_grp_itr_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_grp_itr_t &grp_itr)
{
    ipmc_lib_grp_const_itr_t itr = grp_itr;

    o << "{key = "    << itr->first  // Using ipmc_lib_grp_key_t::operator<<()
      << ", value = " << itr->second // Using ipmc_lib_grp_state_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_grp_itr_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_grp_itr_t *grp_itr)
{
    o << *grp_itr;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// ipmc_lib_pdu_query_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_pdu_query_t &q)
{
    ipmc_lib_src_list_const_itr_t src_list_itr;
    int                           cnt;

    o << "{version = "               << ipmc_lib_pdu_version_to_str(q.version)
      << ", max_response_time_ms = " << q.max_response_time_ms
      << ", grp_addr = "             << q.grp_addr; // Using vtss_appl_ipmc_lib_ip_t::operator<<()

    if (q.version == IPMC_LIB_PDU_VERSION_IGMP_V3 || q.version == IPMC_LIB_PDU_VERSION_MLD_V2) {
        o << ", s_flag = "  << q.s_flag
          << ", qrv = "     << q.qrv
          << ", qqi = "     << q.qqi
          << ", src_cnt = " << q.src_list.size();

        cnt = 0;
        for (src_list_itr = q.src_list.cbegin(); src_list_itr != q.src_list.cend(); ++src_list_itr) {
            o << ", src_list[" << cnt++ << "] = " << *src_list_itr; // Using vtss_appl_ipmc_lib_ip_t::operator<<()
        }
    }

    o << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_pdu_group_record_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_pdu_group_record_t &g)
{
    ipmc_lib_src_list_const_itr_t src_list_itr;
    int                           cnt;

    o << "{valid = "        << g.valid
      << ", record_type = " << ipmc_lib_pdu_record_type_to_str(g.record_type)
      << ", grp_addr = "    << g.grp_addr
      << ", src_cnt = "     << g.src_list.size();

    cnt = 0;
    for (src_list_itr = g.src_list.cbegin(); src_list_itr != g.src_list.cend(); ++src_list_itr) {
        o << ", src_list[" << cnt++ << "] = " << *src_list_itr; // Using vtss_appl_ipmc_lib_ip_t::operator<<()
    }

    o << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_pdu_group_record_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_pdu_group_record_t *grp_rec)
{
    o << *grp_rec;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// ipmc_lib_pdu_report_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_pdu_report_t &r)
{
    int cnt;

    o << "{version = "   << ipmc_lib_pdu_version_to_str(r.version)
      << ", is_leave = " << r.is_leave
      << ", rec_cnt = "  << r.rec_cnt;

    for (cnt = 0; cnt < r.rec_cnt; cnt++) {
        o << ", group_recs[" << cnt << "] = " << r.group_recs[cnt]; // Using ipmc_lib_pdu_group_record_t::operator<<()
    }

    o << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_pdu_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_pdu_t &pdu)
{
    o << "{type = "     << ipmc_lib_pdu_type_to_str(pdu.type)
      << ", version = " << ipmc_lib_pdu_version_to_str(pdu.version)
      << ", dmac = "    << pdu.dmac
      << ", smac = "    << pdu.smac
      << ", sip = "     << pdu.sip  // Using vtss_appl_ipmc_lib_ip_t::operator<<()
      << ", dip = "     << pdu.dip; // Using vtss_appl_ipmc_lib_ip_t::operator<<()

    switch (pdu.type) {
    case IPMC_LIB_PDU_TYPE_QUERY:
        o << ", query = " << pdu.query; // Using ipmc_lib_pdu_query_t::operator<<()
        break;

    case IPMC_LIB_PDU_TYPE_REPORT:
        o << ", report = " << pdu.report; // Using ipmc_lib_pdu_report_t::operator<<()
        break;

    default:
        break;
    }

    o << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_pdu_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_pdu_t *pdu)
{
    o << *pdu;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_global_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_global_conf_t &conf)
{
    o << "{admin_active = "                  << conf.admin_active
      << ", unregistered_flooding_enable = " << conf.unregistered_flooding_enable
      << ", proxy_enable = "                 << conf.proxy_enable
      << ", leave_proxy_enable = "           << conf.leave_proxy_enable
      << ", ssm_prefix = "                   << conf.ssm_prefix // Using vtss_appl_ipmc_lib_ip_t::operator<<()
      << ", ssm_prefix_len = "               << conf.ssm_prefix_len
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_global_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_global_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_port_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_port_conf_t &conf)
{
    o << "{router = "       << conf.router
      << ", fast_leave = "  << conf.fast_leave
      << ", grp_cnt_max = " << conf.grp_cnt_max
      << ", profile_key = " << conf.profile_key.name
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_port_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_port_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_vlan_conf_t &conf)
{
    o << "{admin_active = "     << conf.admin_active
      << ", name = "            << conf.name
      << ", compatible_mode = " << conf.compatible_mode
      << ", querier_enable = "  << conf.querier_enable
      << ", querier_address = " << conf.querier_address // Using vtss_appl_ipmc_lib_ip_t::operator<<()
      << ", compatibility = "   << ipmc_lib_util_compatibility_to_str(conf.compatibility, conf.querier_address.is_ipv4, true)
      << ", tx_tagged = "       << conf.tx_tagged
      << ", pcp = "             << conf.pcp
      << ", rv = "              << conf.rv
      << ", qi = "              << conf.qi
      << ", qri = "             << conf.qri
      << ", lmqi = "            << conf.lmqi
      << ", uri = "             << conf.uri
      << ", channel_profile = " << conf.channel_profile.name
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_vlan_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_port_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_vlan_port_conf_t &conf)
{
    o << "{role = " << ipmc_lib_util_port_role_to_str(conf.role, true /* capital */) << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_port_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_vlan_port_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_vlan_status_t &s)
{
    char buf[1000];

    o << "{now = "                                        << (uint32_t)vtss::uptime_seconds()
      << ", oper_warnings = {"                            << ipmc_lib_util_vlan_oper_warnings_to_txt(buf, sizeof(buf), s.oper_warnings) << "}"
      << ", querier_state = "                             << s.querier_state
      << ", active_querier_address = "                    << s.active_querier_address
      << ", querier_uptime = "                            << s.querier_uptime
      << ", query_interval_left = "                       << s.query_interval_left
      << ", other_querier_expiry_time = "                 << s.other_querier_expiry_time
      << ", querier_compat = "                            << ipmc_lib_util_compatibility_to_str(s.querier_compat, s.active_querier_address.is_ipv4, true)
      << ", older_version_querier_present_timeout_old = " << s.older_version_querier_present_timeout_old
      << ", older_version_querier_present_timeout_gen = " << s.older_version_querier_present_timeout_gen
      << ", host_compat = "                               << ipmc_lib_util_compatibility_to_str(s.host_compat, s.active_querier_address.is_ipv4, true)
      << ", older_version_host_present_timeout_old = "    << s.older_version_host_present_timeout_old
      << ", older_version_host_present_timeout_gen = "    << s.older_version_host_present_timeout_gen
      << ", grp_cnt = "                                   << s.grp_cnt
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_vlan_status_t *s)
{
    o << *s;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_range_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_profile_range_conf_t &conf)
{
    o << "{start = " << conf.start // Using vtss_appl_ipmc_lib_ip_t::operator<<()
      << "end = "    << conf.end   // Using vtss_appl_ipmc_lib_ip_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_profile_range_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_profile_range_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}


/******************************************************************************/
// vtss_appl_ipmc_lib_igmp_vlan_statistics_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_igmp_vlan_statistics_t &s)
{
    o << "{v1_report = "     << s.v1_report
      << ", v1_query = "     << s.v1_query
      << ", v2_report = "    << s.v2_report
      << ", v2_leave = "     << s.v2_leave
      << ", v2_g_query = "   << s.v2_g_query
      << ", v2_gs_query = "  << s.v2_gs_query
      << ", v3_report = "    << s.v3_report
      << ", v3_g_query = "   << s.v3_g_query
      << ", v3_gs_query = "  << s.v3_gs_query
      << ", v3_gss_query = " << s.v3_gss_query
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_mld_vlan_statistics_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_mld_vlan_statistics_t &s)
{
    o << "{v1_report = "     << s.v1_report
      << ", v1_done = "      << s.v1_done
      << ", v1_g_query = "   << s.v1_g_query
      << ", v1_gs_query = "  << s.v1_gs_query
      << ", v2_report = "    << s.v2_report
      << ", v2_g_query = "   << s.v2_g_query
      << ", v2_gs_query = "  << s.v2_gs_query
      << ", v2_gss_query = " << s.v2_gss_query
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_vlan_statistics_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_vlan_statistics_t &s)
{
    // We don't know whether this object represents IGMP or MLD, so we print
    // both.
    o << "{rx_errors = "          << s.rx_errors
      << ", tx_query = "          << s.tx_query
      << ", tx_specific_query = " << s.tx_specific_query
      << ", rx_query = "          << s.rx_query
      << ", rx.igmp.utilized = "  << s.rx.igmp.utilized // Using vtss_appl_ipmc_lib_igmp_vlan_statistics_t::operator<<()
      << ", rx.igmp.ignored = "   << s.rx.igmp.ignored  // Using vtss_appl_ipmc_lib_igmp_vlan_statistics_t::operator<<()
      << ", rx.mld.utilized = "   << s.rx.mld.utilized  // Using vtss_appl_ipmc_lib_mld_vlan_statistics_t::operator<<()
      << ", rx.mld.ignored = "    << s.rx.mld.ignored   // Using vtss_appl_ipmc_lib_mld_vlan_statistics_t::operator<<()
      << ", tx.igmp = "           << s.tx.igmp          // Using vtss_appl_ipmc_lib_igmp_vlan_statistics_t::operator<<()
      << ", tx.mld = "            << s.tx.mld           // Using vtss_appl_ipmc_lib_mld_vlan_statistics_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_vlan_internal_state_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_vlan_internal_state_t &i)
{
    o << "{proxy_query_timeout = "     << i.proxy_query_timeout
      << ", cur_rv = "                 << i.cur_rv
      << ", cur_qi = "                 << i.cur_qi
      << ", cur_qri = "                << i.cur_qri
      << ", cur_lmqi = "               << i.cur_lmqi
      << ", startup_query_cnt_left = " << i.startup_query_cnt_left
      << ", proxy_report_timeout = "   << i.proxy_report_timeout
      << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_vlan_state_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const ipmc_lib_vlan_state_t &s)
{
    o << "{key = "                                        << s.vlan_key             // Using vtss_appl_ipmc_lib_vlan_key_t::operator<<()
      << ", conf = "                                      << s.conf                 // Using vtss_appl_ipmc_lib_vlan_conf_t::operator<<()
      << ", status = "                                    << s.status               // Using vtss_appl_ipmc_lib_vlan_status_t::operator<<()
      << ", statistics = "                                << s.statistics           // Using vtss_appl_ipmc_lib_vlan_statistics_t::operator<<()
      << ", internal_state = "                            << s.internal_state       // Using ipmc_lib_vlan_internal_state_t::operator<<()
      << ", global = "                                    << s.global               // Just a pointer
      << "}";

    return o;
}

/******************************************************************************/
// ipmc_lib_vlan_state_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const ipmc_lib_vlan_state_t *vlan_state)
{
    o << *vlan_state; // Using ipmc_lib_vlan_state_t::operator<<()

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

