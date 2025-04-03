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

#ifdef VTSS_BASICS_STANDALONE
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#include "frr_router_access.hxx"
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
#include "ip_utils.hxx"  // For the operator of mesa_ipv4_network_t
#include "subject.hxx"
#include "frr_utils.hxx"
#endif

/****************************************************************************/
/** Module default trace group declaration                                  */
/****************************************************************************/
#ifndef VTSS_BASICS_STANDALONE
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_ROUTER
#include "frr_trace.hxx"  // For module trace group definitions
#else
#include <vtss/basics/trace.hxx>
#endif

//----------------------------------------------------------------------------
//** The string mapping for enumeration
//----------------------------------------------------------------------------

namespace vtss
{
//----------------------------------------------------------------------------
//** Key chain
//----------------------------------------------------------------------------
/* The hierarchy of key chain commands:
 * key chain <key_chain_name>
 *     key <key_id>
 *         key-string <key_str>
 */
Vector<std::string> to_vty_key_chain_name_set(const std::string &key_chain,
                                              const bool is_delete)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    if (is_delete) {
        buf << "no ";
    }
    /* Add or delete the key chain by name */
    buf << "key chain " << key_chain;
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_key_chain_name_set(const std::string &key_chain,
                               const bool is_delete)
{
    auto cmds = to_vty_key_chain_name_set(key_chain, is_delete);
    std::string vty_res;
    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

Vector<std::string> to_vty_key_chain_key_id_set(const std::string &keychain_name,
                                                const uint32_t key_id,
                                                const bool is_delete)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    /* Enter in key chain mode */
    buf << "key chain " << keychain_name;
    res.emplace_back(std::move(buf.buf));
    buf.clear();
    if (is_delete) {
        buf << "no ";
    }
    /* Add or delete the key id */
    buf << "key " << key_id;
    res.emplace_back(std::move(buf.buf));
    return res;
}

mesa_rc frr_key_chain_key_id_set(const std::string &keychain_name,
                                 const uint32_t key_id, const bool is_delete)
{
    auto cmds = to_vty_key_chain_key_id_set(keychain_name, key_id, is_delete);
    std::string vty_res;
    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

Vector<std::string> to_vty_key_chain_key_conf_set(
    const std::string &keychain_name, const uint32_t key_id,
    const FrrKeyChainConf &key_conf, const bool is_delete)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    /* Enter in key chain and key id mode */
    buf << "key chain " << keychain_name;
    res.emplace_back(std::move(buf.buf));
    buf.clear();
    buf << "key " << key_id;
    res.emplace_back(std::move(buf.buf));

    /* Set or erase the key configuration */
    if (key_conf.key_str.valid()) {
        buf.clear();
        if (is_delete) {
            buf << "no ";
        }
        // set/erase the key string
        buf << "key-string " << key_conf.key_str.get();
        res.emplace_back(std::move(buf.buf));
    }

    return res;
}

mesa_rc frr_key_chain_key_conf_set(const std::string &keychain_name,
                                   const uint32_t key_id,
                                   const FrrKeyChainConf &key_conf,
                                   const bool is_delete)
{
    auto cmds = to_vty_key_chain_key_conf_set(keychain_name, key_id, key_conf,
                                              is_delete);
    std::string vty_res;
    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

struct FrrParseKeyChain : public FrrUtilsConfStreamParserCB {
    void key_chain(const std::string &keychainname, const str &line) override
    {
        /* Add the key chain name */
        // The key id 0xffffffff means empty
        if (_name_only) {
            res.set({keychainname, 0xffffffff}, {});
        }
    }

    void key_chain_key_id(const std::string &keychainname,
                          const std::string &key_id_str,
                          const str &line) override
    {
        if (_name_only) {
            return;
        }

        parser::Lit key("key");
        parser::IntUnsignedBase10<uint32_t, 1, 10> key_id_num;
        parser::OneOrMore<FrrUtilsEatAll> eatall;
        uint32_t empty_key_id = 0xffffffff;
        FrrKeyChainConf empty_conf;

        /* Check if entering submode command (must exactly matched) */
        if (frr_util_group_spaces(line, {&key, &key_id_num}, {&eatall}) &&
            !eatall.get().size()) {
            // Replace the original entry with the new key ID since key id is
            // the sub mode of key chain
            // Delete the original one and then replace it with a new one
            res.erase({keychainname, empty_key_id});
            res.set({keychainname, key_id_num.get()}, {});
            return;
        }

        /* Process submode commands */
        uint32_t key_id = std::stoul(key_id_str, nullptr, 10);

        // Cmd syntax: 'key-string <key_string>'
        parser::Lit key_str("key-string");
        parser::OneOrMore<FrrUtilsEatAll> str_val;
        if (frr_util_group_spaces(line, {&key_str, &str_val})) {
            // Set key-string configuration
            FrrKeyChainConf key_conf;
            key_conf.key_str =
                std::string(str_val.get().begin(), str_val.get().end());
            res.set({keychainname, key_id}, key_conf);
        }
    }

    explicit FrrParseKeyChain(
        Map<vtss::Pair<std::string, uint32_t>, FrrKeyChainConf> &map,
        bool name_only)
        : res {map}, _name_only {name_only} {}

    Map<vtss::Pair<std::string, uint32_t>, FrrKeyChainConf> &res;
    bool _name_only;
};

/* The Key ID 0xffffffff means empty */
Map<vtss::Pair<std::string, uint32_t>, FrrKeyChainConf> frr_key_chain_conf_get(std::string &conf, bool name_only)
{
    Map<vtss::Pair<std::string, uint32_t>, FrrKeyChainConf> res;
    FrrParseKeyChain cb {res, name_only};

    frr_util_conf_parser(conf, cb);
    return cb.res;
}

//----------------------------------------------------------------------------
//** access-list
//----------------------------------------------------------------------------
//
Vector<std::string> to_vty_access_list_conf_set(const FrrAccessList &val)
{
    // access-list [name] permit 1.2.3.4/24

    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "access-list " << val.name;

    if (val.mode == FrrAccessListMode_Deny) {
        buf << " deny " << val.net;
    } else if (val.mode == FrrAccessListMode_Permit) {
        buf << " permit " << val.net;
    } else {
        VTSS_ASSERT(true);
    }

    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_access_list_conf_set(const FrrAccessList &val)
{
    auto cmds = to_vty_access_list_conf_set(val);
    std::string vty_res;

    /* If ripd isn't running, start it. The configuration is saved in ripd */
    if (frr_daemon_start(FRR_DAEMON_TYPE_RIP) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

Vector<std::string> to_vty_access_list_conf_del(FrrAccessList val, bool is_list)
{
    Vector<std::string> res;
    res.emplace_back("configure terminal");
    StringStream buf;
    buf << "no access-list " << val.name;
    if (is_list != true) {
        if (val.mode == FrrAccessListMode_Deny) {
            buf << " deny " << val.net;
        } else if (val.mode == FrrAccessListMode_Permit) {
            buf << " permit " << val.net;
        }
    }

    res.emplace_back(vtss::move(buf.buf));
    return res;
}

mesa_rc frr_access_list_conf_del(FrrAccessList &val, bool is_list)
{
    auto cmds = to_vty_access_list_conf_del(val, is_list);
    std::string vty_res;

    /* If ripd isn't running, start it. The configuration is saved in ripd */
    if (frr_daemon_start(FRR_DAEMON_TYPE_RIP) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    return frr_daemon_cmd(FRR_DAEMON_TYPE_RIP, cmds, vty_res);
}

/* The vtss::Set operator (<) for 'FrrAccessList' */
bool operator<(const FrrAccessList &a, const FrrAccessList &b)
{
    if (a.name != b.name) {
        return a.name < b.name;
    }

    if (a.mode != b.mode) {
        return a.mode < b.mode;
    }

    return a.net < b.net;
}

/* The vtss::Set operator (!=) for 'FrrAccessList' */
bool operator!=(const FrrAccessList &a, const FrrAccessList &b)
{
    if (a.name != b.name) {
        return true;
    }

    if (a.mode != b.mode) {
        return true;
    }

    return a.net != b.net;
}

/* The vtss::Set operator (==) for 'FrrAccessList' */
bool operator==(const FrrAccessList &a, const FrrAccessList &b)
{
    if (a.name != b.name) {
        return false;
    }

    if (a.mode != b.mode) {
        return false;
    }

    return a.net == b.net;
}

bool access_list_conv(const str &line, FrrAccessList &acclist)
{
    parser::Lit access_list("access-list");
    parser::OneOrMore<FrrUtilsGetWord> acc_name;
    parser::OneOrMore<FrrUtilsGetWord> access_mode;
    parser::Ipv4Network net_val;
    parser::Lit any("any");

    FrrAccessList acl_entry;
    if (frr_util_group_spaces(line, {&access_list, &acc_name, &access_mode, &net_val})) {
        acclist.name = std::string(acc_name.get().begin(), acc_name.get().end());

        auto mode =
            std::string(access_mode.get().begin(), access_mode.get().end());
        if (mode == "permit") {
            acclist.mode = FrrAccessListMode_Permit;
        } else {
            acclist.mode = FrrAccessListMode_Deny;
        }

        acclist.net = net_val.get().as_api_type();

        return true;
    }

    else if (frr_util_group_spaces(line, {&access_list, &acc_name, &access_mode, &any})) {
        acclist.name = std::string(acc_name.get().begin(), acc_name.get().end());

        auto mode =
            std::string(access_mode.get().begin(), access_mode.get().end());
        if (mode == "permit") {
            acclist.mode = FrrAccessListMode_Permit;
        } else {
            acclist.mode = FrrAccessListMode_Deny;
        }

        acclist.net = {};

        return true;
    }

    return false;
}

struct FrrParseAccessListSet : public FrrUtilsConfStreamParserCB {
    void root(const str &line) override
    {
        FrrAccessList acl_entry;

        bool rc = access_list_conv(line, acl_entry);
        if (rc == true) {
            res_set.insert(std::move(acl_entry));
        }
    }

    Set<FrrAccessList> res_set;
};

Set<FrrAccessList> frr_access_list_conf_get_set(std::string &conf)
{
    FrrParseAccessListSet cb;

    frr_util_conf_parser(conf, cb);
    return std::move(cb.res_set);
}

struct FrrParseAccessList : public FrrUtilsConfStreamParserCB {
    void root(const str &line) override
    {
        FrrAccessList acl_entry;

        bool rc = access_list_conv(line, acl_entry);
        if (rc == true) {
            res.emplace_back(std::move(acl_entry));
        }
    }

    Vector<FrrAccessList> res;
};

Vector<FrrAccessList> frr_access_list_conf_get(std::string &conf)
{
    FrrParseAccessList cb;

    frr_util_conf_parser(conf, cb);
    return std::move(cb.res);
}

}  // namespace vtss

