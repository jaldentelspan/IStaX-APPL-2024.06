/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifdef VTSS_BASICS_STANDALONE
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#include "frr_rip_access.hxx"
#include "frr_utils.hxx"
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <initializer_list>
#include <vtss/basics/algorithm.hxx>
#include <vtss/basics/expose/json.hxx>
#include <vtss/basics/expose/json/enum-macros.hxx>
#include <vtss/basics/fd.hxx>
#include <vtss/basics/notifications/process-daemon.hxx>
#include <vtss/basics/notifications/subject-runner.hxx>
#include <vtss/basics/parse_group.hxx>
#include <vtss/basics/parser_impl.hxx>
#include <vtss/basics/string-utils.hxx>
#include <vtss/basics/time.hxx>
#include <vtss/basics/vector.hxx>
#ifdef VTSS_BASICS_STANDALONE
#include <signal.h>
#else
#include <vtss_timer_api.h>
#include "icli_api.h"
#include "ip_utils.hxx"  // For mesa_ipv4_network_t operators
#include "subject.hxx"
#endif

/****************************************************************************/
/** Module default trace group declaration                                  */
/****************************************************************************/
#ifndef VTSS_BASICS_STANDALONE
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_RIP
#include "frr_trace.hxx"  // For module trace group definitions
#else
#include <vtss/basics/trace.hxx>
#endif

//----------------------------------------------------------------------------
//** The string mapping for enumeration
//----------------------------------------------------------------------------
vtss_enum_descriptor_t FrrRipDbProtoType_txt[] {
    {vtss::FrrRipDbProtoType_Rip,       "rip"},
    {vtss::FrrRipDbProtoType_Connected, "connected"},
    {vtss::FrrRipDbProtoType_Static,    "static"},
    {vtss::FrrRipDbProtoType_Ospf,      "ospf"},
    {0, 0}
};

VTSS_JSON_SERIALIZE_ENUM(FrrRipDbProtoType, "FrrRipDbProtoType", FrrRipDbProtoType_txt, "FrrRipDbProtoType_txt");

vtss_enum_descriptor_t FrrRIpDbProtoSubType_txt[] {
    {vtss::FrrRIpDbProtoSubType_Normal, "normal"},
    {vtss::FrrRIpDbProtoSubType_Static, "static"},
    {vtss::FrrRIpDbProtoSubType_Default, "default"},
    {vtss::FrrRIpDbProtoSubType_Redistribute, "redistribute"},
    {vtss::FrrRIpDbProtoSubType_Interface, "interface"},
    {0, 0}
};
VTSS_JSON_SERIALIZE_ENUM(FrrRIpDbProtoSubType, "FrrRIpDbProtoSubType",
                         FrrRIpDbProtoSubType_txt, "FrrRIpDbProtoSubType_txt");

vtss_enum_descriptor_t FrrRipDbNextHopType_txt[] {
    {vtss::FrrRipDbNextHopType_IPv4, "ipv4"},
    {vtss::FrrRipDbNextHopType_IPv6, "ipv6"},
    {vtss::FrrRipDbNextHopType_Ifindex, "ifindex"},
    {vtss::FrrRipDbNextHopType_Blackhole, "blackhole"},
    {0, 0}
};
VTSS_JSON_SERIALIZE_ENUM(FrrRipDbNextHopType, "FrrRipDbNextHopType",
                         FrrRipDbNextHopType_txt, "FrrRipDbNextHopType_txt");

vtss_enum_descriptor_t FrrRipVerIf_txt[] {{vtss::FrrRipVer_None, "none"},
    {vtss::FrrRipVer_1, "1"},
    {vtss::FrrRipVer_2, "2"},
    {vtss::FrrRipVer_Both, "1 2"},
    {}
};
VTSS_JSON_SERIALIZE_ENUM(FrrRipVer, "FrrRipVer", FrrRipVerIf_txt,
                         "FrrRipVerIf_txt");

namespace vtss
{

//----------------------------------------------------------------------------
//** Enable/Disable RIP router mode
//----------------------------------------------------------------------------
/* RIP vty command: router rip */
Vector<std::string> to_vty_rip_router_mode_set(bool is_enable)
{
    Vector<std::string> res;
    res.push_back("configure terminal");

    if (!is_enable) {
        /* APPL-1652: The passive interface configuration isn't removed
         *            after RIP router mode is disabled.
         *
         * Background:
         * When the RIP router mode is disabled, all router mode related
         * commands are expected to be removed but the passive interface
         * configuration doesn't. It is because the configuration is stored
         * in the interface data structure.
         * See the details in source file 'frr\ripd\ripd.h' (refer to FRR v4.0)
         * #246 struct rip_interface {
         *          ...
         * #299     // Passive interface.
         * #300     int passive;
         *          ...
         *      }
         *
         * Solution:
         * In order to perform the same behavior as others router mode commands.
         * We restore the passive interface configuration before the RIP router
         * mode is disabled.
         */
        std::string running_conf;
        if (frr_daemon_running_config_get(FRR_DAEMON_TYPE_RIP, running_conf) == VTSS_RC_OK) {
            auto frr_conf = frr_rip_router_conf_get(running_conf);
            if (frr_conf.rc == VTSS_RC_OK && frr_conf->router_mode) {
                res.push_back("router rip");
                res.push_back("no passive-interface default");
                res.push_back("exit");
            }
        }
    }

    res.push_back("no router rip");
    if (is_enable) {
        res.push_back("router rip");
    }

    return res;
}

/* Enable/Disable RIP router mode */
mesa_rc frr_rip_router_conf_set(bool is_enable)
{
    VTSS_RC(frr_daemon_start(FRR_DAEMON_TYPE_RIP));

    auto cmds = to_vty_rip_router_mode_set(is_enable);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

//----------------------------------------------------------------------------
//** RIP router configuration
//----------------------------------------------------------------------------
struct FrrParseRipRouter : public FrrUtilsConfStreamParserCB {
private:
    // Parse version
    void try_parse_version(const str &line)
    {
        parser::Lit version_lit("version");
        parser::Lit ripv1_lit("1");
        parser::Lit ripv2_lit("2");
        if (frr_util_group_spaces(line, {&version_lit, &ripv1_lit})) {
            res.version = FrrRipVer_1;
        } else if (frr_util_group_spaces(line, {&version_lit, &ripv2_lit})) {
            res.version = FrrRipVer_2;
        }
    }

    // Parse redistributed default meteric
    void try_parse_redist_def_metric(const str &line)
    {
        parser::Lit def_metric_lit("default-metric");
        /* Notice: the usage of IntUnsignedBase10<TYPE, MIN, MAX>.
         * The <MIN, MAX> means input character length, not valid range
         * value. In this case, the valid ranges are all 1-16, so the
         * valid character input length is 1-2.
         */
        parser::IntUnsignedBase10<uint8_t, 1, 2> def_metric_val;

        if (frr_util_group_spaces(line, {&def_metric_lit, &def_metric_val})) {
            res.redist_def_metric = def_metric_val.get();
        }
    }

    void try_parse_def_route_redist(const str &line)
    {
        parser::Lit lit_def_info("default-information");
        parser::Lit lit_originate("originate");
        if (frr_util_group_spaces(line, {&lit_def_info, &lit_originate})) {
            res.def_route_redist = true;
        }
    }

    bool try_parse_timers(const str &line)
    {
        parser::Lit timers_lit("timers");
        parser::Lit basic_lit("basic");
        /* Notice: the usage of IntUnsignedBase10<TYPE, MIN, MAX>.
         * The <MIN, MAX> means input character length, not valid range
         * value. In this case, the valid ranges are all 5-2147483, so the valid
         * character input length is 1-7.
         */
        parser::IntUnsignedBase10<uint32_t, 1, 7> update_timer;
        parser::IntUnsignedBase10<uint32_t, 1, 7> invalid_timer;
        parser::IntUnsignedBase10<uint32_t, 1, 7> garbarge_collection_timer;

        if (frr_util_group_spaces(line, {
        &timers_lit, &basic_lit, &update_timer,
        &invalid_timer, &garbarge_collection_timer
    })) {
            FrrRipRouterTimersConf timers_conf = {
                update_timer.get(), invalid_timer.get(),
                garbarge_collection_timer.get()
            };

            res.timers = timers_conf;

            return true;
        }

        return false;
    }

    // Parse passive-interface default mode
    void try_parse_def_passive_intf(const str &line)
    {
        parser::Lit lit_def_passive_intf("passive-interface");
        parser::Lit lit_defalut("default");
        if (frr_util_group_spaces(line, {&lit_def_passive_intf, &lit_defalut})) {
            res.def_passive_intf = true;
        }
    }

    // Parse administrative distance
    void try_parse_admin_distance(const str &line)
    {
        parser::Lit distance_lit("distance");
        /* Notice: the usage of IntUnsignedBase10<TYPE, MIN, MAX>.
         * The <MIN, MAX> means input character length, not valid range
         * value. In this case, the valid ranges are all 1-255, so the
         * valid character input length is 1-3.
         */
        parser::IntUnsignedBase10<uint32_t, 1, 3> distance_val;

        if (frr_util_group_spaces(line, {&distance_lit, &distance_val})) {
            res.admin_distance = distance_val.get();
        }
    }

public:
    void router(const std::string &name, const str &line) override
    {
        res.router_mode = true;
        // Void it since it is been initialized already
        try_parse_version(line);
        try_parse_redist_def_metric(line);
        try_parse_def_route_redist(line);
        (void)try_parse_timers(line);
        try_parse_def_passive_intf(line);
        try_parse_admin_distance(line);
    }

    /* Given a default value in the inital state since the frr vty output may
     * empty if the value of rip timers equal default setting
     *
    */
    FrrRipRouterTimersConf def_timers_conf = {30, 180, 120};
    FrrRipRouterConf res = {
        .router_mode = false,
        .version = (vtss::Optional<FrrRipVer>)FrrRipVer_Both,
        .timers = (vtss::Optional<FrrRipRouterTimersConf>)def_timers_conf,
        .redist_def_metric = (vtss::Optional<uint8_t>)1,
        .def_route_redist = (vtss::Optional<bool>)false,
        .def_passive_intf = (vtss::Optional<bool>)false,
        .admin_distance = (vtss::Optional<uint8_t>)120
    };
};

/* Set RIP router configuration */
FrrRes<FrrRipRouterConf> frr_rip_router_conf_get(std::string &running_conf)
{
    FrrParseRipRouter cb;

    frr_util_conf_parser(running_conf, cb);
    return cb.res;
}

Vector<std::string> to_vty_rip_router_conf_set(const FrrRipRouterConf &conf)
{
    Vector<std::string> res;
    StringStream buf;
    res.push_back("configure terminal");

    if (!conf.router_mode) {
        res.push_back("no router rip");
        return res;
    }

    // Enter the RIP router configuration mode
    res.push_back("router rip");

    // Configure global version
    if (conf.version.valid()) {
        buf.clear();
        if (conf.version.get() == FrrRipVer_1) {
            buf << "version 1";
        } else if (conf.version.get() == FrrRipVer_2) {
            buf << "version 2";
        } else {
            buf << "no version";
        }

        res.emplace_back(vtss::move(buf.buf));
    }

    // Configure timers basic command
    if (conf.timers.valid()) {
        buf.clear();
        buf << "timers basic " << conf.timers.get().update_timer << " "
            << conf.timers.get().invalid_timer << " "
            << conf.timers.get().garbage_collection_timer;
        res.emplace_back(vtss::move(buf.buf));
    }

    // Configure the redistributed default metric
    if (conf.redist_def_metric.valid()) {
        buf.clear();
        if (conf.redist_def_metric.get()) {
            buf << "default-metric " << conf.redist_def_metric.get();
        } else {
            buf << "no default-metric";
        }

        res.emplace_back(vtss::move(buf.buf));
    }

    // Configure default route redistribution
    if (conf.def_route_redist.valid()) {
        buf.clear();
        buf << (conf.def_route_redist.get() ? "" : "no ")
            << "default-information originate";
        res.emplace_back(vtss::move(buf.buf));
    }

    // Configure passive-interface default mode
    if (conf.def_passive_intf.valid()) {
        buf.clear();
        buf << (conf.def_passive_intf.get() ? "" : "no ")
            << "passive-interface default";
        res.emplace_back(vtss::move(buf.buf));
    }

    // Configure administrative distance
    if (conf.admin_distance.valid()) {
        buf.clear();
        if (conf.admin_distance.get()) {
            buf << "distance " << conf.admin_distance.get();
        } else {
            buf << "no distance";
        }

        res.emplace_back(vtss::move(buf.buf));
    }

    return res;
}

mesa_rc frr_rip_router_conf_set(const FrrRipRouterConf &conf)
{
    auto cmds = to_vty_rip_router_conf_set(conf);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

//----------------------------------------------------------------------------
//** RIP passive interface
//----------------------------------------------------------------------------
Vector<std::string> to_vty_rip_router_passive_if_conf_set(const std::string &ifname,
                                                          bool passive)
{
    Vector<std::string> res;
    res.push_back("configure terminal");
    res.push_back("router rip");
    StringStream buf;
    buf << (passive ? "" : "no ") << "passive-interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_rip_router_passive_if_conf_set(const vtss_ifindex_t &i, bool passive)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_rip_router_passive_if_conf_set(interface_name.val, passive);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

struct FrrParseRipPassiveIfDefault : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit lit_passive_default("passive-interface default");
        if (frr_util_group_spaces(line, {&lit_passive_default})) {
            res = true;
        }
    }

    bool res = {false};
};

struct FrrParseRipPassiveIf : public FrrUtilsConfStreamParserCB {
    FrrParseRipPassiveIf(const std::string &name, bool enabled)
        : if_name {name}, default_enabled {enabled}, res {enabled} {}

    void router(const std::string &name, const str &line) override
    {
        // If the default passive mode is enabled, only the interface with
        // passive disabled need to be returned, vice versa.
        parser::Lit lit_passive(default_enabled ? "no passive-interface"
                                : "passive-interface");
        parser::OneOrMore<FrrUtilsEatAll> lit_if_name;
        if (frr_util_group_spaces(line, {&lit_passive, &lit_if_name})) {
            if (vtss::equal(lit_if_name.get().begin(), lit_if_name.get().end(),
                            if_name.begin()) &&
                (size_t)(lit_if_name.get().end() - lit_if_name.get().begin()) ==
                if_name.size()) {
                // 'if_name' and 'lit_if_name' match exactly.
                res = !default_enabled;
            }
        }
    }

    const std::string &if_name;
    const bool default_enabled;
    bool res;
};

FrrRes<bool> frr_rip_router_passive_if_conf_get(std::string &running_conf, const vtss_ifindex_t &ifindex)
{
    FrrRes<std::string> if_name = frr_util_os_ifname_get(ifindex);

    if (!if_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseRipPassiveIfDefault cb_default;
    frr_util_conf_parser(running_conf, cb_default);

    FrrParseRipPassiveIf cb(if_name.val, cb_default.res);
    frr_util_conf_parser(running_conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
Vector<std::string> to_vty_rip_network_conf_set(const FrrRipNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router rip");
    StringStream buf;
    buf << "network " << val.net;
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_rip_network_conf_set(const FrrRipNetwork &val)
{
    auto cmds = to_vty_rip_network_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

Vector<std::string> to_vty_rip_network_conf_del(const FrrRipNetwork &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router rip");
    StringStream buf;
    buf << "no network " << val.net;
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_rip_network_conf_del(const FrrRipNetwork &val)
{
    auto cmds = to_vty_rip_network_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

struct FrrParseRipNetwork : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit net("network");
        parser::Ipv4Network net_val;

        if (frr_util_group_spaces(line, {&net, &net_val})) {
            res.emplace_back(net_val.get().as_api_type());
        }
    }

    Vector<FrrRipNetwork> res;
};

Vector<FrrRipNetwork> frr_rip_network_conf_get(std::string &running_conf)
{
    Vector<FrrRipNetwork> res;
    FrrParseRipNetwork    cb;

    frr_util_conf_parser(running_conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** RIP neighbor connection
//----------------------------------------------------------------------------
Vector<std::string> to_vty_rip_neighbor_conf_set(const mesa_ipv4_t neighbor_addr)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router rip");
    StringStream buf;
    // AsIpv4() doens't accept const variable, so initialize a local variable
    // with 'val.neighbor_addr'
    mesa_ipv4_t ipv4_addr {neighbor_addr};
    buf << "neighbor " << AsIpv4(ipv4_addr);
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_rip_neighbor_conf_set(const mesa_ipv4_t neighbor_addr)
{
    auto cmds = to_vty_rip_neighbor_conf_set(neighbor_addr);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

Vector<std::string> to_vty_rip_neighbor_conf_del(const mesa_ipv4_t neighbor_addr)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router rip");
    StringStream buf;
    // AsIpv4() doens't accept const variable, so initialize a local variable
    // with 'val.neighbor_addr'
    mesa_ipv4_t ipv4_addr {neighbor_addr};
    buf << "no neighbor " << AsIpv4(ipv4_addr);
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_rip_neighbor_conf_del(const mesa_ipv4_t neighbor_addr)
{
    auto cmds = to_vty_rip_neighbor_conf_del(neighbor_addr);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

struct FrrRipNeighborConnect {
    FrrRipNeighborConnect() = default;
    FrrRipNeighborConnect(const mesa_ipv4_t &ipv4_addr)
        : neighbor_addr {ipv4_addr} {}

    mesa_ipv4_t neighbor_addr;
};

struct FrrParseRipNeighborConnect : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit neighbor("neighbor");
        parser::IPv4 neighbor_addr;

        if (frr_util_group_spaces(line, {&neighbor, &neighbor_addr})) {
            res.set(neighbor_addr.get().as_api_type());
        }
    }

    Set<mesa_ipv4_t> res;
};

Set<mesa_ipv4_t> frr_rip_neighbor_conf_get(std::string &running_conf)
{
    FrrParseRipNeighborConnect cb;

    frr_util_conf_parser(running_conf, cb);
    return std::move(cb.res);
}

//----------------------------------------------------------------------------
//** RIP authentication
//----------------------------------------------------------------------------
Vector<std::string> to_vty_rip_if_authentication_key_chain_set(const std::string &ifname, const std::string &keychain_name, const bool is_delete)
{
    Vector<std::string> res;
    StringStream buf;

    buf << "interface " << ifname;
    res.emplace_back("configure terminal");
    res.emplace_back(vtss::move(buf.buf));

    buf.clear();
    if (is_delete) {
        buf << "no ";
    }

    buf << "ip rip authentication key-chain" << (is_delete ? "" : (" " + keychain_name));
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_rip_if_authentication_key_chain_set(const vtss_ifindex_t i,
                                                const std::string &keychain_name,
                                                const bool is_delete)
{
    /* Start RIP daemon if it isn't running */
    if (frr_daemon_start(FRR_DAEMON_TYPE_RIP) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* Check if the interface exists or not */
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_rip_if_authentication_key_chain_set(interface_name.val, keychain_name, is_delete);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

Vector<std::string> to_vty_rip_if_authentication_simple_pwd_set(
    const std::string &ifname, const std::string &simple_pwd,
    const bool is_delete)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    if (is_delete) {
        buf << "no ";
    }

    buf << "ip rip authentication string" << (is_delete ? "" : " " + simple_pwd);
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_rip_if_authentication_simple_pwd_set(vtss_ifindex_t i,
                                                 const std::string &simple_pwd,
                                                 const bool is_delete)
{
    /* Start RIP daemon if it isn't running */
    if (frr_daemon_start(FRR_DAEMON_TYPE_RIP) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* Check if the interface exists or not */
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_rip_if_authentication_simple_pwd_set(interface_name.val, simple_pwd, is_delete);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

Vector<std::string> to_vty_rip_if_authentication_mode_conf_set(const std::string &ifname, const FrrRipIfAuthMode mode)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    switch (mode) {
    case FrrRipIfAuthMode_Null:
        buf << "no ip rip authentication mode";
        break;
    case FrrRipIfAuthMode_Pwd:
        buf << "ip rip authentication mode text";
        break;
    case FrrRipIfAuthMode_MsgDigest:
        buf << "ip rip authentication mode md5";
        break;
    }

    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_rip_if_authentication_mode_conf_set(const vtss_ifindex_t i,
                                                const FrrRipIfAuthMode mode)
{
    /* Start RIP daemon if it isn't running */
    if (frr_daemon_start(FRR_DAEMON_TYPE_RIP) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* Check if the interface exists or not */
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds =
        to_vty_rip_if_authentication_mode_conf_set(interface_name.val, mode);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

struct FrrParseRipIfAuthentication : public FrrUtilsConfStreamParserCB {
    void interface(const std::string &ifname, const str &line)
    {
        if (ifname == intf_name) {
            parser::Lit auth("ip rip authentication");
            parser::Lit mode("mode");
            parser::Lit string("string");
            parser::Lit key_chain("key-chain");
            parser::Lit auth_len("auth-length");
            parser::Lit old_ripd("old-ripd");
            parser::OneOrMore<FrrUtilsEatAll> eatall;
            parser::OneOrMore<FrrUtilsGetWord> word;
            std::string word_buf;
            /* Parse authentication mode */
            // Syntax: ip rip authentication mode { text | md5 }

            if (frr_util_group_spaces(line, {&auth, &mode, &word}, {&eatall})) {
                word_buf = std::string(word.get().begin(), word.get().end());
                if (word_buf == "md5") {
                    /* FRR supports two kinds of authentication length for the
                     * 'Keyed Message Digest' algorithem - 16 (RFC compatible)
                     * and 20 (Old ripd compatible). FRR default is 20.
                     * 20 is also Cisco compatible.
                     * config: rip authentication mode md5 auth-length old-ripd
                     */
                    res.auth_mode = FrrRipIfAuthMode_MsgDigest;
                } else if (word_buf == "text") {
                    res.auth_mode = FrrRipIfAuthMode_Pwd;
                } else {
                    VTSS_TRACE(DEBUG)
                            << "Can not parse RIP auth mode : " << eatall.get();
                }

                return;
            }

            /* Parse authentication simple password */
            // Syntax: ip rip authentication string <password>
            if (frr_util_group_spaces(line, {&auth, &string, &word}, {&eatall}) &&
                !eatall.get().size()) {
                word_buf = std::string(word.get().begin(), word.get().end());
                res.simple_pwd = word_buf;
                return;
            }

            /* Parse authentication key chain name setting */
            // Syntax: ip rip authentication key-chain <key-chain-name>
            if (frr_util_group_spaces(line, {&auth, &key_chain, &word}, {&eatall}) &&
                !eatall.get().size()) {
                word_buf = std::string(word.get().begin(), word.get().end());
                res.keychain_name = word_buf;
                return;
            }
        }
    }

    explicit FrrParseRipIfAuthentication(const std::string &ifname)
        : intf_name {ifname} {}

    const std::string &intf_name;
    FrrRipIfAuthConfig res;
};

FrrRes<FrrRipIfAuthConfig> frr_rip_if_authentication_conf_get(std::string &running_conf, const vtss_ifindex_t ifindex)
{
    /* Check if the interface exists or not */
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(ifindex);

    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    FrrParseRipIfAuthentication cb(interface_name.val);
    frr_util_conf_parser(running_conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** RIP interface configuration: Version support
//----------------------------------------------------------------------------
/* (For unit-test purpose)
 * Convert RIP interface version configuration to FRR VTY command */
Vector<std::string> to_vty_rip_intf_recv_ver_set(const std::string &intf_name,
                                                 FrrRipVer recv_ver)
{
    /* Enter to global configuration mode */
    Vector<std::string> res;
    res.emplace_back("configure terminal");

    /* Enter to interface configuration mode */
    StringStream buf;
    buf << "interface " << intf_name;
    res.emplace_back(vtss::move(buf.buf));

    /* Convert to FRR VTY commands */
    buf.clear();
    if (recv_ver == FrrRipVer_NotSpecified) {
        buf << "no ";
    }

    buf << "ip rip receive version";
    if (recv_ver != FrrRipVer_NotSpecified) {
        buf << (recv_ver == FrrRipVer_None
                ? " none"
                : recv_ver == FrrRipVer_1
                ? " 1"
                : recv_ver == FrrRipVer_2 ? " 2" : " 1 2");
    }

    res.emplace_back(vtss::move(buf.buf));

    buf.clear();
    return res;
}

Vector<std::string> to_vty_rip_intf_send_ver_set(const std::string &intf_name,
                                                 FrrRipVer send_ver)
{
    /* Enter to global configuration mode */
    Vector<std::string> res;
    res.emplace_back("configure terminal");

    /* Enter to interface configuration mode */
    StringStream buf;
    buf << "interface " << intf_name;
    res.emplace_back(vtss::move(buf.buf));

    /* Convert to FRR VTY commands */
    buf.clear();
    if (send_ver == FrrRipVer_NotSpecified) {
        buf << "no ";
    }

    buf << "ip rip send version";
    if (send_ver != FrrRipVer_NotSpecified) {
        buf << (send_ver == FrrRipVer_1
                ? " 1"
                : send_ver == FrrRipVer_2 ? " 2" : " 1 2");
    }

    res.emplace_back(vtss::move(buf.buf));

    buf.clear();
    return res;
}

/* Set the RIP interface version configuration */
mesa_rc frr_rip_intf_ver_conf_set(vtss_ifindex_t ifindex,
                                  FrrRipConfIntfVer &intf_ver_conf)
{
    /* Start RIP daemon if it isn't running */
    if (frr_daemon_start(FRR_DAEMON_TYPE_RIP) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* Check if the interface exists or not */
    FrrRes<std::string> intf_name = frr_util_os_ifname_get(ifindex);
    if (!intf_name) {
        return VTSS_RC_ERROR;
    }

    /* Convert configuration to FRR VTY commands */
    Vector<std::string> cmds;
    if (intf_ver_conf.recv_ver.valid()) {
        cmds = to_vty_rip_intf_recv_ver_set(intf_name.val,
                                            intf_ver_conf.recv_ver.get());
    } else if (intf_ver_conf.send_ver.valid()) {
        cmds = to_vty_rip_intf_send_ver_set(intf_name.val,
                                            intf_ver_conf.send_ver.get());
    } else {
        return VTSS_RC_ERROR;
    }

    /* Apply FRR VTY commands */
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

struct FrrParseRipIntfVerCb : public FrrUtilsConfStreamParserCB {
    FrrParseRipIntfVerCb(const std::string &intf_name)
        : searching_name {intf_name} {}

    void interface(const std::string &intf_name, const str &line) override
    {
        if (intf_name != searching_name) {
            return;
        }

        parser::Lit lit_recv_ver("ip rip receive version");
        parser::Lit lit_send_ver("ip rip send version");
        parser::Lit lit_ver_none("none");
        parser::Lit lit_ver_1("1");
        parser::Lit lit_ver_2("2");
        parser::Lit lit_ver_both("1 2");

        /* Parser RIP interface receive version */
        if (frr_util_group_spaces(line, {&lit_recv_ver, &lit_ver_both})) {
            res.recv_ver = FrrRipVer_Both;
        }

        else if (frr_util_group_spaces(line, {&lit_recv_ver, &lit_ver_2})) {
            res.recv_ver = FrrRipVer_2;
        }

        else if (frr_util_group_spaces(line, {&lit_recv_ver, &lit_ver_1})) {
            res.recv_ver = FrrRipVer_1;
        }

        else if (frr_util_group_spaces(line, {&lit_recv_ver, &lit_ver_none})) {
            res.recv_ver = FrrRipVer_None;
        }

        /* Parser RIP interface send version */
        if (frr_util_group_spaces(line, {&lit_send_ver, &lit_ver_both})) {
            res.send_ver = FrrRipVer_Both;
        }

        else if (frr_util_group_spaces(line, {&lit_send_ver, &lit_ver_2})) {
            res.send_ver = FrrRipVer_2;
        }

        else if (frr_util_group_spaces(line, {&lit_send_ver, &lit_ver_1})) {
            res.send_ver = FrrRipVer_1;
        }
    }

    const std::string &searching_name;
    FrrRipConfIntfVer res = {
        .recv_ver = (vtss::Optional<FrrRipVer>)FrrRipVer_NotSpecified,
        .send_ver = (vtss::Optional<FrrRipVer>)FrrRipVer_NotSpecified
    };
};

/* Get the RIP interface version configuration */
FrrRes<FrrRipConfIntfVer> frr_rip_intf_ver_conf_get(std::string &running_conf, vtss_ifindex_t ifindex)
{
    /* Check if the interface exists or not */
    FrrRes<std::string> intf_name = frr_util_os_ifname_get(ifindex);
    if (!intf_name) {
        return VTSS_RC_ERROR;
    }

    /* Parse RIP interface version via running configuration */
    FrrParseRipIntfVerCb cb(intf_name.val);
    frr_util_conf_parser(running_conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** RIP interface configuration: Split horizon
//----------------------------------------------------------------------------
Vector<std::string> to_vty_rip_if_split_horizon_set(const std::string &ifname,
                                                    FrrRipIfSplitHorizonMode mode)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "interface " << ifname;
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    buf << (mode == FRR_RIP_IF_SPLIT_HORIZON_MODE_DISABLED ? "no " : "");
    buf << "ip rip split-horizon";
    buf << (mode == FRR_RIP_IF_SPLIT_HORIZON_MODE_POISONED_REVERSE
            ? " poisoned-reverse"
            : "");
    res.emplace_back(vtss::move(buf.buf));
    buf.clear();
    return res;
}

struct FrrParseRipIfSplitHorizon : public FrrUtilsConfStreamParserCB {
    FrrParseRipIfSplitHorizon(const std::string &ifname) : name {ifname} {}

    void interface(const std::string &ifname, const str &line)
    {
        if (ifname == name) {
            parser::Lit no("no");
            parser::Lit ip("ip");
            parser::Lit rip("rip");
            parser::Lit split_horizon("split-horizon");
            parser::Lit poison_reverse("poisoned-reverse");
            if (frr_util_group_spaces(line, {&ip, &rip, &split_horizon, &poison_reverse})) {
                res = FRR_RIP_IF_SPLIT_HORIZON_MODE_POISONED_REVERSE;
                return;
            }

            if (frr_util_group_spaces(line, {&ip, &rip, &split_horizon})) {
                res = FRR_RIP_IF_SPLIT_HORIZON_MODE_SIMPLE;
                return;
            }

            if (frr_util_group_spaces(line, {&no, &ip, &rip, &split_horizon})) {
                res = FRR_RIP_IF_SPLIT_HORIZON_MODE_DISABLED;
                return;
            }
        }
    }

    const std::string &name;
    FrrRipIfSplitHorizonMode res = FRR_RIP_IF_SPLIT_HORIZON_MODE_SIMPLE;
};

FrrRes<FrrRipIfSplitHorizonMode> frr_rip_if_split_horizon_conf_get(std::string &running_conf, vtss_ifindex_t ifindex)
{
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(ifindex);

    // There are two possibilities of frr_util_os_ifname_get() failure:
    // 1. All configuration of the interface are default.
    // 2. The interface doesn't exist, it might be due to an invalid ifIndex or
    // the interface isn't created yet.
    // However, the parser isn't able to determine the reasons of failure, the
    // caller must take the responsibility.
    if (!interface_name) {
        return FRR_RIP_IF_SPLIT_HORIZON_MODE_SIMPLE;
    }

    FrrParseRipIfSplitHorizon cb(interface_name.val);
    frr_util_conf_parser(running_conf, cb);
    return cb.res;
}

mesa_rc frr_rip_if_split_horizon_conf_set(vtss_ifindex_t i, FrrRipIfSplitHorizonMode mode)
{
    /* If ripd isn't running, start it */
    if (frr_daemon_start(FRR_DAEMON_TYPE_RIP) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    /* Check if the interface exists or not */
    FrrRes<std::string> interface_name = frr_util_os_ifname_get(i);
    if (!interface_name) {
        return VTSS_RC_ERROR;
    }

    auto cmds = to_vty_rip_if_split_horizon_set(interface_name.val, mode);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

//----------------------------------------------------------------------------
//** RIP metric manipulation
//----------------------------------------------------------------------------
Vector<std::string> to_vty_offset_list_conf_set(const FrrRipOffsetList &val,
                                                bool is_set)
{
    // offset-list name in 11 eth0
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router rip");
    StringStream buf;
    std::string no_str;

    if (is_set == true) {
        no_str = "";
    } else {
        no_str = "no ";
    }

    if (val.mode == FrrRipOffsetListDirection_In) {
        buf << no_str << "offset-list " << val.name << " in " << val.metric;
    } else if (val.mode == FrrRipOffsetListDirection_Out) {
        buf << no_str << "offset-list " << val.name << " out " << val.metric;
    }

    if (val.ifindex.valid()) {
        FrrRes<std::string> interface_name =
            frr_util_os_ifname_get(val.ifindex.get());
        buf << " " << interface_name.val;
    }

    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_rip_offset_list_conf_set(const FrrRipOffsetList &val)
{
    auto cmds = to_vty_offset_list_conf_set(val, true);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

mesa_rc frr_rip_offset_list_conf_del(FrrRipOffsetList &val)
{
    auto cmds = to_vty_offset_list_conf_set(val, false);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

#ifdef VTSS_BASICS_STANDALONE
struct FrrParseRipOffsetList : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit offset_lit {"offset-list"};
        parser::OneOrMore<FrrUtilsGetWord> off_name;
        parser::OneOrMore<FrrUtilsGetWord> off_mode;
        /* Notice: the usage of IntUnsignedBase10<TYPE, MIN, MAX>.
         * The <MIN, MAX> means input character length, not valid range
         * value. In this case, the valid ranges are all 1-16, so the
         * valid character input length is 1-2.
         */
        parser::IntUnsignedBase10<uint8_t, 1, 2> metric_val;
        parser::OneOrMore<FrrUtilsEatAll> if_name;

        if (frr_util_group_spaces(line, {&offset_lit, &off_name, &off_mode, &metric_val},
    {&if_name})) {
            FrrRipOffsetList offset_list;
            offset_list.name =
                std::string(off_name.get().begin(), off_name.get().end());

            auto mode = std::string(off_mode.get().begin(), off_mode.get().end());
            if (mode == "in") {
                offset_list.mode = FrrRipOffsetListDirection_In;
            } else {
                offset_list.mode = FrrRipOffsetListDirection_Out;
            }

            offset_list.metric = metric_val.get();

            if (if_name.get().begin() == if_name.get().end()) {
                offset_list.ifindex = {0};
            } else {
                auto os_ifname =
                    std::string(if_name.get().begin(), if_name.get().end());
                offset_list.ifindex = vtss_ifindex_from_os_ifname(os_ifname);
            }

            res.emplace_back(vtss::move(offset_list));
        }
    }

    Vector<FrrRipOffsetList> res;
};

/** For unit-test purpose */
Vector<FrrRipOffsetList> frr_rip_offset_list_conf_get(std::string &running_conf)
{
    FrrParseRipOffsetList cb;

    frr_util_conf_parser(running_conf, cb);
    return cb.res;
}
#endif

/* The vtss::Set operator (<) for 'FrrRipOffsetListKey' */
bool operator<(const FrrRipOffsetListKey &a, const FrrRipOffsetListKey &b)
{
    if (a.ifindex != b.ifindex) {
        return a.ifindex < b.ifindex;
    }

    return a.mode < b.mode;
}

/* The vtss::Set operator (!=) for 'FrrRipOffsetListKey' */
bool operator!=(const FrrRipOffsetListKey &a, const FrrRipOffsetListKey &b)
{
    if (a.ifindex != b.ifindex) {
        return true;
    }

    return a.mode != b.mode;
}

/* The vtss::Set operator (==) for 'FrrRipOffsetListKey' */
bool operator==(const FrrRipOffsetListKey &a, const FrrRipOffsetListKey &b)
{
    if (a.ifindex != b.ifindex) {
        return false;
    }

    return a.mode == b.mode;
}

struct FrrParseRipOffsetListMap : public FrrUtilsConfStreamParserCB {
    void router(const std::string &name, const str &line) override
    {
        parser::Lit offset_lit {"offset-list"};
        parser::OneOrMore<FrrUtilsGetWord> off_name;
        parser::OneOrMore<FrrUtilsGetWord> off_mode;
        /* Notice: the usage of IntUnsignedBase10<TYPE, MIN, MAX>.
         * The <MIN, MAX> means input character length, not valid range
         * value. In this case, the valid ranges are all 1-16, so the
         * valid character input length is 1-2.
         */
        parser::IntUnsignedBase10<uint8_t, 1, 2> metric_val;
        parser::OneOrMore<FrrUtilsEatAll> if_name;

        if (frr_util_group_spaces(line, {&offset_lit, &off_name, &off_mode, &metric_val},
    {&if_name})) {
            FrrRipOffsetListKey offset_list_key;
            FrrRipOffsetListData offset_list_data;

            offset_list_data.name =
                std::string(off_name.get().begin(), off_name.get().end());

            auto mode = std::string(off_mode.get().begin(), off_mode.get().end());
            if (mode == "in") {
                offset_list_key.mode = FrrRipOffsetListDirection_In;
            } else {
                offset_list_key.mode = FrrRipOffsetListDirection_Out;
            }

            offset_list_data.metric = metric_val.get();

            if (if_name.get().begin() == if_name.get().end()) {
                offset_list_key.ifindex = {0};
            } else {
                auto os_ifname =
                    std::string(if_name.get().begin(), if_name.get().end());
                offset_list_key.ifindex = vtss_ifindex_from_os_ifname(os_ifname);
            }

            res.set(std::move(offset_list_key), std::move(offset_list_data));
        }
    }

    FrrRipOffsetListMap res;
};

FrrRipOffsetListMap frr_rip_offset_list_conf_get_map(std::string &running_conf)
{
    FrrParseRipOffsetListMap cb;

    frr_util_conf_parser(running_conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** RIP global status
//----------------------------------------------------------------------------
FrrRipVer FrrRipVerInt2Enum(const uint8_t &ver)
{
    if (ver == 1) {
        return FrrRipVer_1;
    }

    if (ver == 2) {
        return FrrRipVer_2;
    }

    if (ver == 3) {
        return FrrRipVer_Both;
    }

    return FrrRipVer_None;
}

/* The serializer for 'FrrRipGeneralStatus' */
void serialize(vtss::expose::json::Loader &l, vtss::FrrRipGeneralStatus &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrRipGeneralStatus"));

    (void)m.add_leaf(s.updateTimer, vtss::tag::Name("updateTime"));
    (void)m.add_leaf(s.invalidTimer, vtss::tag::Name("timeoutTime"));
    (void)m.add_leaf(s.garbageTimer, vtss::tag::Name("garbageTime"));
    (void)m.add_leaf(s.updateRemainTime, vtss::tag::Name("updateRemainTime"));
    (void)m.add_leaf(s.default_metric, vtss::tag::Name("defaultMetric"));

    uint8_t sendVer;
    uint8_t recvVer;
    (void)m.add_leaf(sendVer, vtss::tag::Name("versionSend"));
    (void)m.add_leaf(recvVer, vtss::tag::Name("versionRecv"));
    s.sendVer = FrrRipVerInt2Enum(sendVer);
    s.recvVer = FrrRipVerInt2Enum(recvVer);

    (void)m.add_leaf(s.default_distance, vtss::tag::Name("distanceDefault"));
    (void)m.add_leaf(s.globalRouteChanges,
                     vtss::tag::Name("globalRouteChanges"));
    (void)m.add_leaf(s.globalQueries, vtss::tag::Name("globalQueries"));
}

/* Parse the RIP general status from VTY output string */
FrrRipGeneralStatus frr_rip_status_parse(const std::string &vty_output)
{
    FrrRipGeneralStatus result = {};
    vtss::expose::json::Loader loader(&*vty_output.begin(), vty_output.c_str() + vty_output.size());
    loader.patch_mode_ = true;
    loader.load(result);
    return result;
}

/* RIP vty command: show ip rip status global */
FrrRes<Vector<std::string>> to_vty_rip_general_status_get()
{
    Vector<std::string> cmds;
    cmds.push_back("show ip rip status global json");
    return cmds;
}

/* Get the RIP general status information */
FrrRes<FrrRipGeneralStatus> frr_rip_general_status_get()
{
    /* Get the VTY commands */
    auto cmds = to_vty_rip_general_status_get();
    if (!cmds) {
        return cmds.rc;
    }

    /* Execute the VTY commands */
    std::string vty_output;
    mesa_rc rc = frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds.val, vty_output);

    if (rc != VTSS_RC_OK ||
        !vty_output.size() /* no json output when the router mode is disabled. */) {
        return VTSS_RC_ERROR;
    }

    /* Parse the VTY output string to FRR structure */
    return frr_rip_status_parse(vty_output);
}

//----------------------------------------------------------------------------
//** RIP interface status
//----------------------------------------------------------------------------
/* The JSON data structure for the RIP interface status entry.
 * It must be correspond with the JSON output format. */
/* Access JSON layer uses the same data structure as APPL layer */
typedef struct FrrRipActiveIfStatus FrrRipActiveIfStatusEntry;

template <typename VAL>
void serialize(vtss::expose::json::Loader &l, vtss::Map<vtss_ifindex_t, VAL> &m)
{
    const char *b = l.pos_;
    vtss_ifindex_t ifindex;
    std::string intf_str;
    VAL v {};

    // Make sure start from map_start '{', should be pair with '}'.
    CHECK_LOAD(l.load(vtss::expose::json::map_start), Error);

    /* Parse entry key */
    CHECK_LOAD(l.load(intf_str), Error);
    ifindex = vtss_ifindex_from_os_ifname(intf_str);
    CHECK_LOAD(l.load(vtss::expose::json::map_assign), Error);

    /* Parse entry data */
    CHECK_LOAD(l.load(v), Error);
    /* Store entry when key is valid */
    if (frr_util_ifindex_valid(ifindex)) {
        m[ifindex] = std::move(v);
    }

    CHECK_LOAD(l.load(vtss::expose::json::map_end), Error);
    return;
Error:
    l.pos_ = b;
}

/* The serializer for 'FrrRipIfStatusEntry' */
void serialize(vtss::expose::json::Loader &l, vtss::FrrRipActiveIfStatusEntry &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrRipIfStatusEntry"));

    s = {};

    (void)m.add_leaf(s.sendVer, vtss::tag::Name("sendVersion"));
    (void)m.add_leaf(s.recvVer, vtss::tag::Name("receiveVersion"));
    (void)m.add_leaf(s.key_chain, vtss::tag::Name("keychain"));
    (void)m.add_leaf(s.ifStatRcvBadPackets,
                     vtss::tag::Name("ifStatRcvBadPackets"));
    (void)m.add_leaf(s.ifStatRcvBadRoutes,
                     vtss::tag::Name("ifStatRcvBadRoutes"));
    (void)m.add_leaf(s.ifStatSentUpdates, vtss::tag::Name("ifStatSentUpdates"));

    int iPassive;
    if (m.add_leaf(iPassive, vtss::tag::Name("passive"))) {
        if (iPassive) {
            s.is_passive_intf = true;
        }
    }

    int iAuthType;
    if (m.add_leaf(iAuthType, vtss::tag::Name("authType"))) {
        if (iAuthType == 0) {
            s.auth_type = VTSS_APPL_RIP_AUTH_TYPE_NULL;
        } else if (iAuthType == 2) {
            s.auth_type = VTSS_APPL_RIP_AUTH_TYPE_SIMPLE_PASSWORD;
        } else if (iAuthType == 3) {
            s.auth_type = VTSS_APPL_RIP_AUTH_TYPE_MD5;
        } else {
            // iAuthType == 1 means AuthData. It should never be observed in
            // this case. RBNTBD: Why?
            s.auth_type = VTSS_APPL_RIP_AUTH_TYPE_NULL;
        }
    }
}

/* The entry callback function for the RIP interface */
struct FrrRipInterfaceEntryCb
    : public MapSortedCb <
      std::string,
      vtss::Vector<vtss::Map<vtss_ifindex_t, FrrRipActiveIfStatusEntry> >> {
    void entry(const std::string &entry_key,
               vtss::Vector<vtss::Map<vtss_ifindex_t, FrrRipActiveIfStatusEntry>>
               &entry_data) override
    {
        /* The vector has multiple elements.
         * The element is a Map, each Map contains one entry.
         */
        for (auto &m : entry_data) {
            for (auto &e : m) {
                result.set(vtss::move(e.first), vtss::move(e.second));
            }
        }
    }

    explicit FrrRipInterfaceEntryCb(FrrRipActiveIfStatusMap &map) : result {map} {}

    FrrRipActiveIfStatusMap &result;
};

/* Parse the RIP peer information from VTY output string */
FrrRipActiveIfStatusMap frr_rip_interface_status_parse(
    const std::string &vty_output)
{
    FrrRipActiveIfStatusMap result;
    FrrRipInterfaceEntryCb entry_cb {result};
    vtss::expose::json::Loader loader(&*vty_output.begin(), vty_output.c_str() + vty_output.size());
    loader.patch_mode_ = true;
    loader.load(entry_cb);
    return result;
}

/* RIP vty command: show ip rip status interface json */
FrrRes<Vector<std::string>> to_vty_rip_interface_get()
{
    Vector<std::string> cmds;
    cmds.push_back("show ip rip status interface json");
    return cmds;
}

/* Get the RIP interface information */
FrrRes<FrrRipActiveIfStatusMap> frr_rip_interface_status_get()
{
    /* Get the VTY commands */
    auto cmds = to_vty_rip_interface_get();
    if (!cmds) {
        return cmds.rc;
    }

    /* Execute the VTY commands */
    std::string vty_output;
    mesa_rc rc = frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds.val, vty_output);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Parse the VTY output string to FRR structure */
    return frr_rip_interface_status_parse(vty_output);
}

//----------------------------------------------------------------------------
//** RIP neighbor status
//----------------------------------------------------------------------------
/* The JSON data structure for the RIP peer entry.
 * It must be correspond with the JSON output format. */
/* Access JSON layer uses the same data structure as APPL layer */
typedef struct FrrRipPeerData FrrRipJsonPeerEntry;

/* The serializer for 'FrrRipJsonPeerEntry' */
void serialize(vtss::expose::json::Loader &l, vtss::FrrRipJsonPeerEntry &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrRipJsonPeerEntry"));

    s = {};
    (void)m.add_leaf(s.recv_bad_packets, vtss::tag::Name("recvBadPackets"));
    (void)m.add_leaf(s.recv_bad_routes, vtss::tag::Name("recvBadRoutes"));
    std::string time_str;
    if (m.add_leaf(time_str, vtss::tag::Name("lastUpdate"))) {
        s.last_update_time = frr_util_time_to_seconds(time_str);
    }

    uint8_t ver;
    (void)m.add_leaf(ver, vtss::tag::Name("version"));
    s.rip_ver = FrrRipVerInt2Enum(ver);
}

/* The entry callback function for the RIP peer */
struct FrrRipPeerEntryCb
    : public MapSortedCb<std::string,
      vtss::Vector<vtss::Map<mesa_ipv4_t, FrrRipJsonPeerEntry>>> {
    void entry(const std::string &entry_key,
               vtss::Vector<vtss::Map<mesa_ipv4_t, FrrRipJsonPeerEntry>>
               &entry_data) override
    {
        /* The vector has multiple elements.
         * The element is a Map, each Map contains one entry
         */
        for (auto &m : entry_data) {
            for (auto &e : m) {
                result.set(std::move(e.first), std::move(e.second));
            }
        }
    }

    explicit FrrRipPeerEntryCb(FrrRipPeerMap &map) : result {map} {}

    FrrRipPeerMap &result;
};

/* Parse the RIP peer information from VTY output string */
FrrRipPeerMap frr_rip_peer_parse(const std::string &vty_output)
{
    FrrRipPeerMap result;
    FrrRipPeerEntryCb entry_cb {result};
    vtss::expose::json::Loader loader(&*vty_output.begin(), vty_output.c_str() + vty_output.size());
    loader.patch_mode_ = true;
    loader.load(entry_cb);
    return result;
}

/* RIP vty command: show ip rip status peer json */
FrrRes<Vector<std::string>> to_vty_rip_peer_get()
{
    Vector<std::string> cmds;
    cmds.push_back("show ip rip status peer json");
    return cmds;
}

/* Get the RIP peer information */
FrrRes<FrrRipPeerMap> frr_rip_peer_get()
{
    /* Get the VTY commands */
    auto cmds = to_vty_rip_peer_get();
    if (!cmds) {
        return cmds.rc;
    }

    /* Execute the VTY commands */
    std::string vty_output;
    mesa_rc rc = frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds.val, vty_output);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Parse the VTY output string to FRR structure */
    return frr_rip_peer_parse(vty_output);
}

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------
/* The JSON data structure for the RIP database next hop.
 * It must be correspond with the JSON output format. */
struct FrrRipJsonDbNextHop {
    /* The following declarations are used for saving the process time.
     * We use move assignment operator instead of copy assignment operator. */
    // Remove the copy constructor
    FrrRipJsonDbNextHop(const FrrRipJsonDbNextHop &) = delete;
    // Remove the copy assignment operator
    FrrRipJsonDbNextHop &operator=(const vtss::FrrRipJsonDbNextHop &) = delete;
    // Add the move constructor
    FrrRipJsonDbNextHop(FrrRipJsonDbNextHop &&) = default;
    // Add the move assignment operator
    FrrRipJsonDbNextHop &operator=(FrrRipJsonDbNextHop && ) = default;

    FrrRipDbNextHopType type;
    mesa_ipv4_t ipaddr;
    uint8_t metric;
};

/* The JSON data structure for the RIP database entry.
 * It must be correspond with the JSON output format. */
struct FrrRipJsonDbEntry {
    /* The following declarations are used for saving the process time.
     * We use move assignment operator instead of copy assignment operator. */
    // Remove the copy constructor
    FrrRipJsonDbEntry(const FrrRipJsonDbEntry &) = delete;
    // Remove the copy assignment operator
    FrrRipJsonDbEntry &operator=(const vtss::FrrRipJsonDbEntry &) = delete;
    // Add the move constructor
    FrrRipJsonDbEntry(FrrRipJsonDbEntry &&) = default;
    // Add the move assignment operator
    FrrRipJsonDbEntry &operator=(FrrRipJsonDbEntry && ) = default;

    FrrRipDbProtoType type;
    FrrRIpDbProtoSubType subtype;
    bool self_intf;
    mesa_ipv4_t src_addr;
    uint32_t external_metric;
    uint32_t tag;
    seconds uptime;
    Vector<FrrRipJsonDbNextHop> nexthops;
};

/* The serializer for 'FrrRipJsonDbEntry' */
void serialize(vtss::expose::json::Loader &l, vtss::FrrRipJsonDbEntry &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrRipJsonDbEntry"));

    s = {};
    (void)m.add_leaf(s.type, vtss::tag::Name("type"));
    (void)m.add_leaf(s.subtype, vtss::tag::Name("subType"));
    (void)m.add_leaf(s.self_intf, vtss::tag::Name("self"));
    (void)m.add_leaf(AsIpv4(s.src_addr), vtss::tag::Name("from"));
    (void)m.add_leaf(s.tag, vtss::tag::Name("tag"));
    (void)m.add_leaf(s.external_metric, vtss::tag::Name("externalMetric"));
    std::string time_str;
    if (m.add_leaf(time_str, vtss::tag::Name("time"))) {
        s.uptime = frr_util_time_to_seconds(time_str);
    }
    (void)m.add_leaf(s.nexthops, vtss::tag::Name("nextHops"));
}

/* The serializer for 'FrrRipJsonDbNextHop' */
void serialize(vtss::expose::json::Loader &l, vtss::FrrRipJsonDbNextHop &s)
{
    vtss::expose::json::Loader::Map_t m =
        l.as_map(vtss::tag::Typename("FrrRipJsonDbNextHop"));

    s = {};
    (void)m.add_leaf(s.type, vtss::tag::Name("nextHopType"));
    (void)m.add_leaf(AsIpv4(s.ipaddr), vtss::tag::Name("nextHop"));
    (void)m.add_leaf(s.metric, vtss::tag::Name("nextHopMetric"));
}

/* The vtss::Set operator (<) for 'FrrRipDbKey' */
bool operator<(const FrrRipDbKey &a, const FrrRipDbKey &b)
{
    if (a.network != b.network) {
        return a.network < b.network;
    }

    return a.nexthop < b.nexthop;
}

/* The vtss::Set operator (!=) for 'FrrRipDbKey' */
bool operator!=(const FrrRipDbKey &a, const FrrRipDbKey &b)
{
    if (a.network != b.network) {
        return true;
    }

    return a.nexthop != b.nexthop;
}

/* The vtss::Set operator (==) for 'FrrRipDbKey' */
bool operator==(const FrrRipDbKey &a, const FrrRipDbKey &b)
{
    if (a.network != b.network) {
        return false;
    }

    return a.nexthop == b.nexthop;
}

/* The entry callback function for the RIP database */
struct FrrRipDbDbEntryCb
    : public MapSortedCb<mesa_ipv4_network_t, FrrRipJsonDbEntry> {
    void entry(const mesa_ipv4_network_t &entry_key,
               FrrRipJsonDbEntry &entry_data) override
    {
        FrrRipDbKey k;
        FrrRipDbData v;

        /* Mapping JSON to FRR RIP structure */
        k.network = entry_key;
        for (auto &nexthop : entry_data.nexthops) {
            k.nexthop = nexthop.ipaddr;
            v.type = entry_data.type;
            v.subtype = entry_data.subtype;
            v.self_intf = entry_data.self_intf;
            v.src_addr = entry_data.src_addr;
            v.metric = nexthop.metric;
            v.external_metric = entry_data.external_metric;
            v.tag = entry_data.tag;
            v.uptime = entry_data.uptime;
            v.nexthop_type = nexthop.type;
            result.set(k, v);
        }
    }

    explicit FrrRipDbDbEntryCb(FrrRipDbMap &map) : result {map} {}

    FrrRipDbMap &result;
};

/* Parse the RIP database information from VTY output string */
FrrRipDbMap frr_rip_db_parse(const std::string &vty_output)
{
    FrrRipDbMap result;
    FrrRipDbDbEntryCb entry_cb {result};
    vtss::expose::json::Loader loader(&*vty_output.begin(), vty_output.c_str() + vty_output.size());
    loader.patch_mode_ = true;
    loader.load(entry_cb);
    return result;
}

/* RIP vty command: show ip rip */
FrrRes<Vector<std::string>> to_vty_rip_db_get()
{
    Vector<std::string> cmds;
    cmds.push_back("show ip rip json");
    return cmds;
}

/* Get the RIP database information */
FrrRes<FrrRipDbMap> frr_rip_db_get()
{
    /* Get the VTY commands */
    auto cmds = to_vty_rip_db_get();
    if (!cmds) {
        return cmds.rc;
    }

    /* Execute the VTY commands */
    std::string vty_output;
    mesa_rc rc = frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds.val, vty_output);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    /* Parse the VTY output string to FRR structure */
    return frr_rip_db_parse(vty_output);
}

//----------------------------------------------------------------------------
//** RIP redistribution
//----------------------------------------------------------------------------
Vector<std::string> to_vty_rip_router_redistribute_conf_set(
    const FrrRipRouterRedistribute &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router rip");
    StringStream buf;
    buf << "redistribute " << ip_util_route_protocol_to_str(val.protocol, false);

    if (val.metric.valid()) {
        buf << " metric " << val.metric.get();
    }

    if (val.route_map.valid()) {
        buf << " route-map " << val.route_map.get();
    }

    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_rip_router_redistribute_conf_set(const FrrRipRouterRedistribute &val)
{
    auto cmds = to_vty_rip_router_redistribute_conf_set(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

Vector<std::string> to_vty_rip_router_redistribute_conf_del(vtss_appl_ip_route_protocol_t &val)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    res.emplace_back("router rip");
    StringStream buf;
    buf << "no redistribute " << ip_util_route_protocol_to_str(val, false);
    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_rip_router_redistribute_conf_del(vtss_appl_ip_route_protocol_t val)
{
    auto cmds = to_vty_rip_router_redistribute_conf_del(val);
    std::string vty_res;

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

struct FrrParseRipRouterRedistribute : public FrrUtilsConfStreamParserCB {
    void parse_protocol(const std::string &protocol, const str &line)
    {
        parser::Lit redistribute_lit {"redistribute"};
        parser::Lit protocol_lit {protocol};

        // Command output 'metric <0-15>' are optional arguments
        // Command output 'route-map <WORD>' are optional arguments
        // We assign an invalid value when the optional argument are not
        // existing in the command output that we don't need to parse various
        // combined conditions.

        // Notice that the usage of second/third arguments in
        // IntUnsignedBase10<TYPE, MIN, MAX>
        // The <MIN, MAX> means input character length, not valid range value.
        // In this case, the valid metric range is 0-16, so the valid
        // character input length is 1-2.
        parser::TagValue<parser::IntUnsignedBase10<uint8_t, 1, 2>, int> metric(
            "metric", 16 + 1);  // plus 1 to become an invalid value
        parser::TagValue<parser::OneOrMore<FrrUtilsEatAll>> route_map("route-map");

        if (frr_util_group_spaces(line, {&redistribute_lit, &protocol_lit},
    {&metric, &route_map})) {
            FrrRipRouterRedistribute redistribute;
            redistribute.protocol = frr_util_route_protocol_from_str(protocol);

            if (metric.get().get() != 17) {
                redistribute.metric = metric.get().get();
            }

            if (route_map.get().get().begin() != route_map.get().get().end()) {
                redistribute.route_map =
                    std::string(route_map.get().get().begin(),
                                route_map.get().get().end());
            }

            res.push_back(vtss::move(redistribute));
        }
    }

    void router(const std::string &name, const str &line) override
    {
        for (auto protocol : protocols) {
            parse_protocol(protocol, line);
        }
    }

    const Vector<std::string> protocols {"kernel", "connected", "static", "ospf",
        "bgp"
    };
    Vector<FrrRipRouterRedistribute> res;
};

Vector<FrrRipRouterRedistribute> frr_rip_router_redistribute_conf_get(std::string &running_conf)
{
    FrrParseRipRouterRedistribute cb;
    frr_util_conf_parser(running_conf, cb);
    return cb.res;
}

}  // namespace vtss

