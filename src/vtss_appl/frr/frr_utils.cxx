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

#include "sysutil_api.h"  // For VTSS_SYS_PASSWORD_MAGIC_STR
#include "frr_utils.hxx"
#include "frr_trace.hxx"
#include <vtss/basics/vector.hxx>
#include <vtss/basics/string-utils.hxx>

using namespace vtss;

/******************************************************************************/
// FRR_UTIL_is_comment()
/******************************************************************************/
static bool FRR_UTIL_is_comment(str line)
{
    parser::ZeroOrMoreSpaces spaces;
    parser::Lit              comment("!");
    const char               *b = &*line.begin();
    const char               *e = line.end();

    return parser::Group(b, e, spaces, comment);
}

/******************************************************************************/
// FRR_UTIL_is_empty_line()
/******************************************************************************/
static bool FRR_UTIL_is_empty_line(str line)
{
    parser::EmptyLine empty;
    const char        *b = &*line.begin();
    const char        *e = line.end();

    return parser::Group(b, e, empty);
}

/******************************************************************************/
// FRR_UTIL_is_interface()
/******************************************************************************/
bool FRR_UTIL_is_interface(str line)
{
    parser::Lit interface("interface");
    const char  *b = &*line.begin();
    const char  *e = line.end();

    return parser::Group(b, e, interface);
}

/******************************************************************************/
// FRR_UTIL_is_key_chain()
// Check if this line command is "key chain" mode
/******************************************************************************/
static bool FRR_UTIL_is_key_chain(str line)
{
    parser::Lit key_chain("key chain");
    const char  *b = &*line.begin();
    const char  *e = line.end();

    return parser::Group(b, e, key_chain);
}

/******************************************************************************/
// FRR_UTIL_is_key_chain()
// Check if this line command is "key chain key id" mode
// "key id" is a sub mode of "key chain" mode
/******************************************************************************/
static bool FRR_UTIL_is_key_chain_key_id(str line)
{
    parser::Lit key(" key");
    const char  *b = &*line.begin();
    const char  *e = line.end();

    return parser::Group(b, e, key);
}

/******************************************************************************/
// FRR_UTIL_is_router()
/******************************************************************************/
static bool FRR_UTIL_is_router(str line)
{
    parser::Lit router("router");
    const char  *b = &*line.begin();
    const char  *e = line.end();

    return parser::Group(b, e, router);
}

/******************************************************************************/
// FRR_UTIL_is_root()
/******************************************************************************/
static bool FRR_UTIL_is_root(str line)
{
    parser::OneOrMoreSpaces space;
    const char              *b = &*line.begin();
    const char              *e = line.end();

    return !parser::Group(b, e, space);
}

/******************************************************************************/
//
// Public API implementation starts here
//
/******************************************************************************/

/******************************************************************************/
// frr_util_secret_key_cryptography()
/******************************************************************************/
mesa_rc frr_util_secret_key_cryptography(const bool is_encrypt, const char *const input, const uint32_t output_len, char *const output)
{
    StringStream key;

    key << VTSS_SYS_PASSWORD_MAGIC_STR;

    if (is_encrypt) {
        return vtss_aes256_encrypt((char *)input, (unsigned char *)key.cstring(), key.buf.size(), output_len, output);
    } else {
        return vtss_aes256_decrypt((char *)input, (unsigned char *)key.cstring(), key.buf.size(), output_len, output);
    }
}

/******************************************************************************/
// frr_util_os_ifname_get()
/******************************************************************************/
FrrRes<std::string> frr_util_os_ifname_get(const vtss_ifindex_t &ifindex)
{
    StringStream buf;
    mesa_vid_t   vlan;

    if ((vlan = vtss_ifindex_as_vlan(ifindex)) == 0) {
        return FRR_RC_IFINDEX_MUST_BE_OF_TYPE_VLAN;
    }

    buf << "vtss.vlan." << vlan;
    return FrrRes<std::string>(vtss::move(buf.buf));
}

/******************************************************************************/
// frr_util_group_spaces()
/******************************************************************************/
bool frr_util_group_spaces(const str &line, std::initializer_list<parser::ParserBase *> args, std::initializer_list<parser::ParserBase *> optional)
{
    parser::ZeroOrMoreSpaces space;
    const char               *b = &*line.begin();
    const char               *c = &*line.end();
    bool                     result = true, tmp;

    for (auto *p : args) {
        // Consume white spaces
        space(b, c);

        result &= (*p)(b, c);
        if (!result) {
            return false;
        }
    }

    if (optional.size() == 0) {
        return true;
    }

    Vector<parser::ParserBase *> opt {optional};
    while (1) {
        result = false;
        Vector<parser::ParserBase *>::iterator i = opt.begin();
        while (i != opt.end()) {
            space(b, c);
            tmp = (**i)(b, c);
            result |= tmp;
            if (tmp) {
                opt.erase(i);
            } else {
                ++i;
            }
        }

        if (!result) {
            break;
        }
    }

    return true;
}

/******************************************************************************/
// frr_util_conf_parser()
/******************************************************************************/
void frr_util_conf_parser(std::string conf, FrrUtilsConfStreamParserCB &cb)
{
    static const std::string if_start = "interface";
    bool key_chain = false, key_chain_key_id = false;
    std::string keychain_name = "", key_id = "";

    bool interface = false;
    std::string if_name = "";

    bool router = false;
    std::string router_name = "";

    for (str line : LineIterator(conf)) {
        if (FRR_UTIL_is_comment(line) || FRR_UTIL_is_empty_line(line)) {
            continue;
        }

        // Clear all sub-mode parameters when current mode is root
        if (FRR_UTIL_is_root(line)) {
            if_name = router_name = keychain_name = key_id = "";
            interface = router = key_chain = key_chain_key_id = false;
        }

        // Check if entering sub mode (from the deepest sub mode)
        if (key_chain && FRR_UTIL_is_key_chain_key_id(line)) {  //"key id" is a submode of "key chain"
            str name = split(line, ' ')[2];
            key_id = std::string(name.begin(), name.end());
            key_chain_key_id = true;
            cb.key_chain_key_id(keychain_name, key_id, line);
            continue;
        } else if (FRR_UTIL_is_key_chain(line)) {
            str name = split(line, ' ')[2];
            keychain_name = std::string(name.begin(), name.end());
            key_chain = true;
            cb.key_chain(keychain_name, line);
            continue;
        } else if (FRR_UTIL_is_interface(line)) {
            str name = split(line, ' ')[1];
            if_name = std::string(name.begin(), name.end());
            interface = true;
            cb.interface(if_name, line);
            continue;
        } else if (FRR_UTIL_is_router(line)) {
            str name = split(line, ' ')[1];
            router_name = std::string(name.begin(), name.end());
            router = true;
            cb.router(router_name, line);
            continue;
        } else if (FRR_UTIL_is_root(line)) {
            // Process root commands
            cb.root(line);
        }

        // Process sub mode commands
        if (key_chain_key_id) {
            cb.key_chain_key_id(keychain_name, key_id, line);
        } else if (key_chain) {
            cb.key_chain(keychain_name, line);
        } else if (interface) {
            cb.interface(if_name, line);
        } else if (router) {
            cb.router(router_name, line);
        }
    }
}

/******************************************************************************/
// frr_util_time_to_seconds()
// Convert time string to integer seconds
// The supported formarts are:
//   1)  MM:SS         (e.g. 56:02         don't know where this format is used)
//   2)  HH:MM:SS      (e.g. 03:56:30,     zebra_vty.c if uptime < ONE_DAY_SECOND)
//   3)  DdHHhMMm      (e.g. 6d13h20m,     zebra_vty.c if uptime < ONE_WEEK_SECOND)
//   4a) WWwDdHHh      (e.g. 07w4d01h,     zebra_vty.c if uptime < ONE_YEAR_SECOND)
//   4b) WWwdDDdHHh    (e.g. 07wd5d3h,     don't know where this format is used)
//   5)  YYyWWwDd      (e.g. 07y03w6d,     zebra.vty.c if uptime >= ONE_YEAR_SECOND)
//   6)  DDDd HH:MM:SS (e.g. 04d 02:31:15, ripd.c if uptime >= ONE_DAY_SECOND)
/******************************************************************************/
vtss::seconds frr_util_time_to_seconds(const std::string &up_time)
{
    const char                                *b = &*up_time.begin();
    const char                                *e = up_time.c_str() + up_time.size();
    parser::Lit                               sep(":");
    parser::Lit                               sep_d_space("d "); // Days  signifier
    parser::Lit                               sep_y("y");        // Years signifier
    parser::Lit                               sep_w("w");        // Weeks signifier
    parser::Lit                               sep_wd("wd");      // Weeks signifier
    parser::Lit                               sep_d("d");        // Days  signifier
    parser::Lit                               sep_h("h");        // Hours signifier
    parser::IntUnsignedBase10<uint32_t, 1, 2> y; // Years
    parser::IntUnsignedBase10<uint32_t, 1, 2> w; // Weeks
    parser::IntUnsignedBase10<uint32_t, 1, 3> d; // Days
    parser::IntUnsignedBase10<uint32_t, 1, 2> h; // Hours
    parser::IntUnsignedBase10<uint32_t, 1, 2> m; // Minutes
    parser::IntUnsignedBase10<uint32_t, 1, 2> s; // Seconds
    seconds                                   result;
    int                                       the_case;

#define ONE_MINUTE_SECS 60
#define ONE_HOUR_SECS   (ONE_MINUTE_SECS *  60)
#define ONE_DAY_SECS    (ONE_HOUR_SECS   *  24)
#define ONE_WEEK_SECS   (ONE_DAY_SECS    *   7)
#define ONE_YEAR_SECS   (ONE_DAY_SECS    * 365)

    if (parser::Group(b, e, d, sep_d_space, h, sep, m, sep, s)) {
        // Case #6
        the_case = 6;
        result = seconds(d.get() * ONE_DAY_SECS + h.get() * ONE_HOUR_SECS + m.get() * ONE_MINUTE_SECS + s.get());
    } else if (parser::Group(b, e, y, sep_y, w, sep_w, d, sep_d)) {
        // Case #5
        the_case = 5;
        result = seconds(y.get() * ONE_YEAR_SECS + w.get() * ONE_WEEK_SECS + d.get() * ONE_DAY_SECS);
    } else if (parser::Group(b, e, w, sep_w, d, sep_d, h, sep_h) || parser::Group(b, e, w, sep_wd, d, sep_d, h)) {
        // Case #4a and #4b
        the_case = 4;
        result = seconds(w.get() * ONE_WEEK_SECS + d.get() * ONE_DAY_SECS + h.get() * ONE_HOUR_SECS);
    } else if (parser::Group(b, e, d, sep_d, h, sep_h, m)) {
        // Case #3
        the_case = 3;
        result = seconds(d.get() * ONE_DAY_SECS + h.get() * ONE_HOUR_SECS + m.get() * ONE_MINUTE_SECS);
    } else if (parser::Group(b, e, h, sep, m, sep, s)) {
        // Case #2
        the_case = 2;
        result = seconds(h.get() * ONE_HOUR_SECS + m.get() * ONE_MINUTE_SECS + s.get());
    } else if (parser::Group(b, e, m, sep, s)) {
        // Case #1
        the_case = 1;
        result = seconds(m.get() * ONE_MINUTE_SECS + s.get());
    } else {
        T_E("Unable to parse time string (%s)", up_time.c_str());
        result = seconds(0);
    }

#undef ONE_MINUTE_SECS
#undef ONE_HOUR_SECS
#undef ONE_DAY_SECS
#undef ONE_WEEK_SECS
#undef ONE_YEAR_SECS

    // Prevent "variable set but not used" when compiling for Bringup, which
    // does not include T_I() trace.
    the_case = the_case;
    T_I("Time string \"%s\" => " VPRI64u " seconds using case #%d", up_time.c_str(), result.raw(), the_case);

    return result;
}

/******************************************************************************/
// frr_util_ifindex_valid()
/******************************************************************************/
bool frr_util_ifindex_valid(vtss_ifindex_t vtss_ifindex)
{
    return ifindex_is_vlan(vtss_ifindex) || ifindex_is_frr_vlink(vtss_ifindex);
}

/******************************************************************************/
// frr_util_route_protocol_from_strl()
/******************************************************************************/
vtss_appl_ip_route_protocol_t frr_util_route_protocol_from_str(const std::string &val)
{
    if (val == "kernel") {
        return VTSS_APPL_IP_ROUTE_PROTOCOL_KERNEL;
    }

    if (val == "connected") {
        return VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED;
    }

    if (val == "static") {
        return VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC;
    }

    if (val == "ospf") {
        return VTSS_APPL_IP_ROUTE_PROTOCOL_OSPF;
    }

    if (val == "rip") {
        return VTSS_APPL_IP_ROUTE_PROTOCOL_RIP;
    }

    T_E("Unknown route protocol: %s", val.c_str());
    return VTSS_APPL_IP_ROUTE_PROTOCOL_KERNEL;
}

