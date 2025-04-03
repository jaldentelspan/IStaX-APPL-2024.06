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

#include "frr_ip_route.hxx"
#include "frr_daemon.hxx"
#include "frr_utils.hxx"
#include "ip_utils.hxx"
#include "ip_os.hxx"      // For ip_os_ifindex_to_ifindex()
#include "frr_trace.hxx"  // For module trace group definitions
#include <vtss/basics/expose/json.hxx>
#include "vtss/basics/json/stream-parser.hxx"

using namespace vtss;

/******************************************************************************/
// FrrIpRouteStatusParser class.
// Called back during parsing of 'show ip[v6] route json' output.
// The trace-conf file and the corresponding callbacks look like:
//
// {                                        => object_start (new level 1)
//  "0.0.0.0\/0":[                          =>   object_element_start("0.0.0.0/0"), array_start
//    {                                     =>   object_start (new level 2)
//      "prefix":"0.0.0.0\/0",              =>     object_element_start("prefix", string_value("0.0.0.0\/0"), object_element_end
//      "protocol":"static",                =>     object_element_start("protocol", string_value("static"), object_element_end
//      "selected":true,                    =>     object_element_start("selected", boolean(b == true), object_element_end
//      "distance":1,                       =>     object_element_start("distance", number_value(u == 1), object_element_end
//      "metric":0,                         =>     object_element_start("metric", number_value(u == 0), object_element_end
//      "uptime":"02:07:34",                =>     object_element_start("uptime", string_value("02:07:34"), object_element_end
//      "nexthops":[                        =>     object_element_start("nexthops"), array_start
//        {                                 =>     object_start (new level 3)
//          "fib":true,                     =>       object_element_start("fib"), boolean(b == true), object_element_end
//          "ip":"10.10.137.1",             =>       object_element_start("ip"), string_value("10.10.137.1"), object_element_end
//          "afi":"ipv4",                   =>       object_element_start("afi"), string_value("ipv4"), object_element_end
//          "interfaceIndex":4,             =>       object_element_start("interfaceIndex", number_value(u == 4), object_element_end
//          "interfaceName":"vtss.vlan.1",  =>       object_element_start("interfaceName"), string_value("vtss.vlan.1"), object_element_end
//          "active":true                   =>       object_element_start("active"), boolean_value(b == true), object_element_end
//        }                                 =>     object_end (new level 2)
//      ]                                   =>     array_end
//    }                                     =>   object_end (new level 1)
//  ],                                      =>   array_end
//  "0.0.1.0\/24":[                         =>   object_element_start("0.0.0.0/0"), array_start
//    {                                     =>   object_start (new level 2)
//      "prefix":"0.0.1.0\/24",             =>
//      "protocol":"static",                =>
//      "selected":true,                    =>
//      "distance":1,                       =>
//      "metric":0,                         =>
//      "uptime":"02:07:34",                =>
//      "nexthops":[                        =>
//        {                                 =>     object_start (new level 3)
//          "fib":true,                     =>
//          "ip":"10.10.137.1",             =>
//          "afi":"ipv4",                   =>
//          "interfaceIndex":4,             =>
//          "interfaceName":"vtss.vlan.1",  =>
//          "active":true                   =>
//        },                                =>     object_end (new level 2)
//        {                                 =>     object_start (new level 3)
//          "fib":true,                     =>
//          "ip":"10.10.137.2",             =>
//          "afi":"ipv4",                   =>
//          "interfaceIndex":4,             =>
//          "interfaceName":"vtss.vlan.1",  =>
//          "active":true                   =>
//        }                                 =>     object_end (new level 2)
//      ]                                   =>     array_end
//    }                                     =>   object_end (new level 1)
//  ]                                       =>   array_end
//  "8.8.8.0\/24":[
//    {
//      "prefix":"8.8.8.0\/24",
//      "protocol":"static",
//      "selected":true,
//      "destSelected":true,
//      "distance":1,
//      "metric":0,
//      "internalStatus":0,
//      "internalFlags":2064,
//      "uptime":"00:02:25",
//      "nexthops":[
//        {
//          "flags":3,
//          "fib":true,
//          "unreachable":true,
//          "blackhole":true,               <----- Blackhole. No nexthop IP.
//          "active":true
//        }
//      ]
//    }
//  ],
// }                                        => object_end (new level 0 => done)
/******************************************************************************/
struct FrrIpRouteStatusParser : public vtss::json::StreamParserCallback {
    FrrIpRouteStatusParser(vtss_appl_ip_route_status_map_t &routes, vtss_appl_ip_route_type_t type, vtss_appl_ip_route_protocol_t protocol) :
        routes(routes), type(type), protocol(protocol)
    {
        reset();
    }

    void reset(void)
    {
        vtss_clear(net_key);        // Only holds net and protocol
        vtss_clear(net_status);     // Only holds distance, metric, uptime, and selected flags.
        vtss_clear(nexthop_key);    // Holds nexthop part of the key
        vtss_clear(nexthop_status); // Holds the status-part of the nexthop
        net_key.route.type = type;

        // These are the ones that must have been seen in the network-part of a
        // route.
        prefix_seen     = false;
        protocol_seen   = false;
        uptime_seen     = false;
        skip_this_route = false;

        // These are the ones that must have been seen in the nexthop-part of a
        // route
        ip_or_os_ifindex_or_unreachable_seen = false;
        skip_this_nexthop                    = false;
    }

    Action array_start() override
    {
        // Must be overridden, because the default implementation returns SKIP.
        return error ? STOP : ACCEPT;
    }

    void array_end() override
    {
    }

    Action object_start() override
    {
        level++;

        if (level == 3) {
            // Going from net to nexthop. Since we have a flat table consisting
            // of both network and destination, we need to copy whatever we have
            // in net_key and net_status into nexthop_key and nexthop_status,
            // since that is the one we are going to add to the flat table.
            nexthop_key    = net_key;
            nexthop_status = net_status;

            // We must have seen prefix, protocol, and uptime.
            if (!error) {
                if (!prefix_seen || !protocol_seen || !uptime_seen) {
                    error = true;
                    error_str << "Either prefix, protocol, or uptime not seen: " << prefix_seen << protocol_seen << uptime_seen;
                }
            }
        }

        return error ? STOP : ACCEPT;
    }

    void object_end() override
    {
        level--;

        if (level == 1) {
            // End of a route
            reset();
        } else if (level == 2) {
            // End of a nexthop.
            if (!error && !ip_or_os_ifindex_or_unreachable_seen) {
                error = true;
                error_str << "Either ip, interfaceIndex, or unreachable not seen";
                T_DG(FRR_TRACE_GRP_IP_ROUTE, "Error: %s:%s", nexthop_key, nexthop_status);
            } else if (skip_this_route || skip_this_nexthop) {
                T_DG(FRR_TRACE_GRP_IP_ROUTE, "Skipping:  %s:%s", nexthop_key, nexthop_status);
            } else {
                T_DG(FRR_TRACE_GRP_IP_ROUTE, "Inserting: %s:%s", nexthop_key, nexthop_status);
                // Create new entry
                auto itr = routes.get(nexthop_key);
                if (itr == routes.end()) {
                    error = true;
                    error_str << "Out of memory";
                } else {
                    itr->second = nexthop_status;
                }
            }

            // Prepare for next nexthop.
            ip_or_os_ifindex_or_unreachable_seen = false;
            skip_this_nexthop                    = false;
            vtss_clear(nexthop_key);
            vtss_clear(nexthop_status);
        }
    }

    Action object_element_start(const std::string &s) override
    {
        cur_key = s;
        return ACCEPT;
    }

    void boolean(bool b) override
    {
        if (!b) {
            // Only interested in true values, since the rest are default.
            return;
        }

        if (level == 2) {
            // Updating net_key and net_status
            if (cur_key == "selected") {
                net_status.flags |= VTSS_APPL_IP_ROUTE_STATUS_FLAG_SELECTED;
            }
        } else if (level == 3) {
            // Updating nexthop_key and nexthop_status
            if (cur_key == "duplicate") {
                nexthop_status.flags |= VTSS_APPL_IP_ROUTE_STATUS_FLAG_DUPLICATE;
            } else if (cur_key == "active") {
                nexthop_status.flags |= VTSS_APPL_IP_ROUTE_STATUS_FLAG_ACTIVE;
            } else if (cur_key == "onLink") {
                nexthop_status.flags |= VTSS_APPL_IP_ROUTE_STATUS_FLAG_ONLINK;
            } else if (cur_key == "recursive") {
                nexthop_status.flags |= VTSS_APPL_IP_ROUTE_STATUS_FLAG_RECURSIVE;
            } else if (cur_key == "fib") {
                nexthop_status.flags |= VTSS_APPL_IP_ROUTE_STATUS_FLAG_FIB;
            } else if (cur_key == "directlyConnected") {
                nexthop_status.flags |= VTSS_APPL_IP_ROUTE_STATUS_FLAG_DIRECTLY_CONNECTED;
            } else if (cur_key == "unreachable") {
                // From NEXTHOP_TYPE_BLACKHOLE
                nexthop_status.flags |= VTSS_APPL_IP_ROUTE_STATUS_FLAG_UNREACHABLE;
                ip_or_os_ifindex_or_unreachable_seen = true;
            } else if (cur_key == "reject") {
                // From NEXTHOP_TYPE_BLACKHOLE
                nexthop_status.flags |= VTSS_APPL_IP_ROUTE_STATUS_FLAG_REJECT;
            } else if (cur_key == "admin-prohibited") {
                // From NEXTHOP_TYPE_BLACKHOLE
                nexthop_status.flags |= VTSS_APPL_IP_ROUTE_STATUS_FLAG_ADMIN_PROHIBITED;
            } else if (cur_key == "blackhole") {
                // From NEXTHOP_TYPE_BLACKHOLE
                nexthop_status.flags |= VTSS_APPL_IP_ROUTE_STATUS_FLAG_BLACKHOLE;
                if (type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
                    nexthop_key.route.route.ipv4_uc.destination = vtss_ipv4_blackhole_route;
                } else {
                    nexthop_key.route.route.ipv6_uc.destination = vtss_ipv6_blackhole_route;
                }
            }
        }
    }

    void number_value(uint32_t u) override
    {
        if (level == 2) {
            // Updating net_key and net_status
            if (cur_key == "distance") {
                net_status.distance = u;
            } else if (cur_key == "metric") {
                net_status.metric = u;
            }
        } else if (level == 3) {
            // Updating nexthop_key and nexthop_status
            if (cur_key == "interfaceIndex") {
                ip_or_os_ifindex_or_unreachable_seen = true;
                nexthop_status.os_ifindex = u;
                if ((nexthop_key.route.vlan_ifindex = ip_os_ifindex_to_ifindex(u)) == VTSS_IFINDEX_NONE) {
                    // Unable to convert os_ifindex to vtss_ifindex.
                    skip_this_nexthop = true;
                } else {
                    nexthop_status.nexthop_ifindex = nexthop_key.route.vlan_ifindex;
                }
            }
        }
    }

    void string_value(const std::string &&s) override
    {
        if (level == 2) {
            // Updating net_key and net_status
            if (cur_key == "prefix") {
                prefix_seen = true;

                if (type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
                    parser::Ipv4Network net;
                    const char *b = &*s.begin();
                    const char *e = s.c_str() + s.size();
                    if (!net(b, e)) {
                        error = true;
                        error_str << "Unable to parse IPv4 network: " << s;
                    }

                    net_key.route.route.ipv4_uc.network = net.get().as_api_type();
                } else {
                    parser::Ipv6Network net;
                    const char *b = &*s.begin();
                    const char *e = s.c_str() + s.size();
                    if (!net(b, e)) {
                        error = true;
                        error_str << "Unable to parse IPv6 network: " << s;
                    }

                    net_key.route.route.ipv6_uc.network = net.get().as_api_type();
                }
            } else if (cur_key == "protocol") {
                protocol_seen = true;

                if (s == "kernel") {
                    // We don't report kernel-installed routes
                    skip_this_route = true;
                } else if (s == "connected") {
                    net_key.protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED;
                } else if (s == "static") {
                    net_key.protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC;
                } else if (s == "ospf" || s == "ospf6") {
                    if (frr_has_ospfd() || frr_has_ospf6d()) {
                        net_key.protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_OSPF;
                    } else {
                        skip_this_route = true;
                    }
                } else if (s == "rip") {
                    if (frr_has_ripd()) {
                        net_key.protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_RIP;
                    } else {
                        skip_this_route = true;
                    }
                } else {
                    error = true;
                    error_str << "Unknown protocol: " << s;
                }

                if (protocol != VTSS_APPL_IP_ROUTE_PROTOCOL_ANY && net_key.protocol != protocol) {
                    // Caller wants to filter the returned routes.
                    skip_this_route = true;
                }
            } else if (cur_key == "uptime") {
                uptime_seen = true;

                // net_status.uptime is a 64-bit integer, but the underlying
                // data type for vtss::seconds is only a 32-bit integer. I guess
                // that's fine to be forward compatible.
                net_status.uptime = frr_util_time_to_seconds(s).raw32();
            }
        } else if (level == 3) {
            // Nexthop
            if (cur_key == "ip") {
                ip_or_os_ifindex_or_unreachable_seen = true;
                if (type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
                    parser::IPv4 dst;
                    const char *b = &*s.begin();
                    const char *e = s.c_str() + s.size();
                    if (!dst(b, e)) {
                        error = true;
                        error_str << "Unable to parse IPv4 address: " << s;
                    }

                    nexthop_key.route.route.ipv4_uc.destination = dst.get().as_api_type();
                } else {
                    parser::IPv6 dst;
                    const char *b = &*s.begin();
                    const char *e = s.c_str() + s.size();
                    if (!dst(b, e)) {
                        error = true;
                        error_str << "Unable to parse IPv6 address: " << s;
                    }

                    nexthop_key.route.route.ipv6_uc.destination = dst.get().as_api_type();
                }
            }
        }
    }

    vtss_appl_ip_route_status_map_t &routes;
    std::string                     cur_key;
    vtss_appl_ip_route_status_key_t net_key,    nexthop_key;
    vtss_appl_ip_route_status_t     net_status, nexthop_status;
    vtss_appl_ip_route_type_t       type;
    vtss_appl_ip_route_protocol_t   protocol;
    bool                            prefix_seen;
    bool                            protocol_seen;
    bool                            uptime_seen;
    bool                            ip_or_os_ifindex_or_unreachable_seen;
    bool                            skip_this_nexthop;
    bool                            skip_this_route;
    vtss::StringStream              error_str;
    bool                            error = false;
    int                             level = 0;
};

/******************************************************************************/
// FRRIpRouteConfigParser
// Used to get routes out of staticd daemon's running-config.
/******************************************************************************/
struct FrrIpRouteConfigParser : public FrrUtilsConfStreamParserCB {
    FrrIpRouteConfigParser(vtss_appl_ip_route_status_map_t &routes, vtss_appl_ip_route_type_t type) :
        routes(routes), type(type), lit_ipv4("ip"), lit_ipv6("ipv6"), lit_route("route")
    {
    }

    // This function gets invoked for every line in the running-config.
    void root(const str &line) override
    {
        vtss_appl_ip_route_status_key_t     key;
        vtss_appl_ip_route_status_t         status;
        vtss_appl_ip_route_status_map_itr_t itr;
        bool                                is_blackhole = false;

        if (type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
            // Looking for IPv4 routes.
            // IPv4 lines look like
            //   ip route 0.0.0.0/0 10.10.137.1
            //   ip route 1.2.3.4/32 10.10.10.9 20
            //   ip route 1.2.3.4/32 10.10.10.10 30
            //   ip route 8.8.8.0/24 Null0

            parser::Ipv4Network net;
            parser::IPv4        dst;
            parser::IntUnsignedBase10<uint32_t, 1, 3> dist(1); // Default distance is 1.

            if (!frr_util_group_spaces(line, {&lit_ipv4, &lit_route, &net, &dst}, {&dist})) {
                T_NG(FRR_TRACE_GRP_IP_ROUTE, "IPv4: No 1st match: %s");
                // Try to match net == Null0
                parser::Lit lit_blackhole("Null0");
                if (!frr_util_group_spaces(line, {&lit_ipv4, &lit_route, &net, &lit_blackhole}, {&dist})) {
                    T_NG(FRR_TRACE_GRP_IP_ROUTE, "IPv4: No 2nd match: %s", line);
                    return;
                }

                is_blackhole = true;
            }

            vtss_clear(key);
            key.protocol                        = VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC;
            key.route.type                      = type;
            key.route.route.ipv4_uc.network     = net.get().as_api_type();
            if (is_blackhole) {
                key.route.route.ipv4_uc.destination = vtss_ipv4_blackhole_route;
            } else {
                key.route.route.ipv4_uc.destination = dst.get().as_api_type();
            }

            // Check if it's already in our route table (from "show ip route")
            // We cannot use routes.find(key), because we don't have the
            // interface in the running-config (it's automatically added by FRR
            // when it gets installed in the route table), so we need to iterate
            for (itr = routes.begin(); itr != routes.end(); ++itr) {
                if (itr->first.route.route.ipv4_uc == key.route.route.ipv4_uc) {
                    break;
                }
            }

            if (itr != routes.end()) {
                T_DG(FRR_TRACE_GRP_IP_ROUTE, "Skipping:  %s", key);
                return;
            }

            // It's not. Create new entry
            itr = routes.get(key);
            if (itr == routes.end()) {
                T_DG(FRR_TRACE_GRP_IP_ROUTE, "IPv4: Unable to create entry: %s (%s)", line, key);
                error = true;
                error_str << "Out of memory";
                return;
            }

            vtss_clear(status);
            status.distance = dist.i;
            itr->second = status;

            T_DG(FRR_TRACE_GRP_IP_ROUTE, "Inserting: %s:%s", key, status);
        } else if (type == VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC) {
            // Looking for IPv6 routes.
            // IPv6 lines look like
            //   ipv6 route 2000::1/128 2000::2
            //   ipv6 route 2000::4/128 fe80::13 vtss.vlan.1
            //   ipv6 route 2000::4/128 fe80::13 vtss.vlan.8 123
            //   ipv6 route 2002::1/128 blackhole 12

            parser::ZeroOrMoreSpaces                  spaces;
            parser::Ipv6Network                       net;
            parser::IPv6                              dst;
            parser::Lit                               lit_ifname("vtss.vlan.");
            parser::IntUnsignedBase10<uint32_t, 1, 4> vlan(0); // Default is that no interface is specified
            parser::IntUnsignedBase10<uint32_t, 1, 3> dist(1); // Default distance is 1.
            parser::OneOrMore<FrrUtilsEatAll>         rem1, rem2;

            if (!frr_util_group_spaces(line, {&lit_ipv6, &lit_route, &net, &dst}, {&rem1})) {
                T_NG(FRR_TRACE_GRP_IP_ROUTE, "IPv6: No 1st match: %s", line);

                parser::Lit lit_blackhole("blackhole");
                if (!frr_util_group_spaces(line, {&lit_ipv6, &lit_route, &net, &lit_blackhole}, {&rem1})) {
                    T_NG(FRR_TRACE_GRP_IP_ROUTE, "IPv6: No 2nd match: %s", line);
                    return;
                }

                is_blackhole = true;
            }

            vtss_clear(key);
            key.protocol                        = VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC;
            key.route.type                      = type;
            key.route.route.ipv6_uc.network     = net.get().as_api_type();

            if (is_blackhole) {
                key.route.route.ipv6_uc.destination = vtss_ipv6_blackhole_route;
            } else {
                key.route.route.ipv6_uc.destination = dst.get().as_api_type();
            }

            // Parse an optional VLAN interface (vtss.vlan.X). This is mandatory
            // on link-local addresses.
            const char *b1 = &*rem1.get().begin();
            if (parser::Group(b1, rem1.get().end(), lit_ifname, vlan, rem2) ||
                parser::Group(b1, rem1.get().end(), lit_ifname, vlan)) {
                (void)vtss_ifindex_from_vlan(vlan.get(), &key.route.vlan_ifindex);
            }

            // Parse an optional distance
            const char *b2 = &*rem2.get().begin();
            (void)parser::Group(b2, rem2.get().end(), spaces, dist);

            // Now, if the destination is link-local we must find an exact match
            // in the already existing routes if it's installed, because we have
            // installed it with a particular interface, which has been parsed
            // by now (vtss.vlan.X).
            // If the destination is not link-local, we don't have the next-hop
            // interface, so we only match on the network and destination.
            if (vtss_ipv6_addr_is_link_local(&key.route.route.ipv6_uc.destination)) {
                if (key.route.vlan_ifindex == 0) {
                    error = true;
                    error_str << "Link-local IPv6 destination does not have an interface. Line = " << line;
                }

                itr = routes.find(key);
            } else {
                if (key.route.vlan_ifindex != 0) {
                    error = true;
                    error_str << "Non-link-local IPv6 destination with an interface. Line = " << line;
                }

                for (itr = routes.begin(); itr != routes.end(); ++itr) {
                    if (itr->first.route.route.ipv6_uc == key.route.route.ipv6_uc) {
                        break;
                    }
                }
            }

            if (itr != routes.end()) {
                T_DG(FRR_TRACE_GRP_IP_ROUTE, "Skipping:  %s", key);
                return;
            }

            // It's not already in the route table. Create new entry
            itr = routes.get(key);
            if (itr == routes.end()) {
                T_DG(FRR_TRACE_GRP_IP_ROUTE, "IPv6: Unable to create entry: %s (%s)", line, key);
                error = true;
                error_str << "Out of memory";
                return;
            }

            vtss_clear(status);
            status.distance = dist.i;
            status.nexthop_ifindex = key.route.vlan_ifindex;
            itr->second = status;

            T_DG(FRR_TRACE_GRP_IP_ROUTE, "Inserting: %s:%s", key, status);
        }
    }

    vtss_appl_ip_route_status_map_t &routes;
    vtss_appl_ip_route_type_t       type;
    parser::Lit                     lit_ipv4;
    parser::Lit                     lit_ipv6;
    parser::Lit                     lit_route;
    vtss::StringStream              error_str;
    bool                            error = false;
};

/******************************************************************************/
// FRR_ip_route_cmd_construct()
/******************************************************************************/
static mesa_rc FRR_ip_route_cmd_construct(const vtss_appl_ip_route_key_t &key, vtss::StringStream &buf)
{
    if (key.type == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
        if (key.route.ipv4_uc.destination == vtss_ipv4_blackhole_route) {
            buf << "ip route "   << key.route.ipv4_uc.network << " Null0";
        } else {
            buf << "ip route "   << key.route.ipv4_uc.network << " " << Ipv4Address(key.route.ipv4_uc.destination);
        }
    } else if (key.type == VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC) {
        if (key.route.ipv6_uc.destination == vtss_ipv6_blackhole_route) {
            buf << "ipv6 route " << key.route.ipv6_uc.network << " blackhole";
        } else {
            buf << "ipv6 route " << key.route.ipv6_uc.network << " " << Ipv6Address(key.route.ipv6_uc.destination);
            if (vtss_ipv6_addr_is_link_local(&key.route.ipv6_uc.destination)) {
                // Routes going to link-local addresses also need the interface name
                // since the same link-local IPv6 address may exist on multiple
                // interfaces and refer to multiple hosts.
                FrrRes<std::string> interface_name = frr_util_os_ifname_get(key.vlan_ifindex);
                if (!interface_name) {
                    T_EG(FRR_TRACE_GRP_IP_ROUTE, "Unable to convert %s to interface name", key.vlan_ifindex);
                    return VTSS_RC_ERROR;
                }

                buf << " " << interface_name.val;
            }
        }
    } else {
        T_EG(FRR_TRACE_GRP_IP_ROUTE, "Invalid route type (%d)", key.type);
        return FRR_RC_INVALID_ARGUMENT;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// frr_ip_route_conf_set()
/******************************************************************************/
mesa_rc frr_ip_route_conf_set(const vtss_appl_ip_route_key_t &key, const vtss_appl_ip_route_conf_t &conf)
{
    vtss::Vector<std::string> cmds;
    vtss::StringStream        buf;
    std::string               result;

    cmds.push_back("configure terminal");
    VTSS_RC(FRR_ip_route_cmd_construct(key, buf));

    if (conf.distance != 1) {
        buf << " " << conf.distance;
    }

    cmds.push_back(vtss::move(buf.buf));

    return frr_daemon_cmd(frr_has_staticd() ? FRR_DAEMON_TYPE_STATIC : FRR_DAEMON_TYPE_ZEBRA, cmds, result);
}

/******************************************************************************/
// frr_ip_route_conf_del()
/******************************************************************************/
mesa_rc frr_ip_route_conf_del(const vtss_appl_ip_route_key_t &key)
{
    vtss::Vector<std::string> cmds;
    vtss::StringStream        buf;
    std::string               result;

    cmds.push_back("configure terminal");

    buf << "no ";
    VTSS_RC(FRR_ip_route_cmd_construct(key, buf));

    cmds.emplace_back(vtss::move(buf.buf));

    return frr_daemon_cmd(frr_has_staticd() ? FRR_DAEMON_TYPE_STATIC : FRR_DAEMON_TYPE_ZEBRA, cmds, result);
}

// Allow this type to be used as iterator.
VTSS_ENUM_INC(vtss_appl_ip_route_type_t);

/******************************************************************************/
// frr_ip_route_status_get()
/******************************************************************************/
mesa_rc frr_ip_route_status_get(vtss_appl_ip_route_status_map_t &routes, vtss_appl_ip_route_type_t type, vtss_appl_ip_route_protocol_t protocol)
{
    vtss_appl_ip_route_type_t type_itr;

    routes.clear();

    T_DG(FRR_TRACE_GRP_IP_ROUTE, "Enter, type = %d", type);

    for (type_itr = VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC; type_itr <= VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC; type_itr++) {
        if (type == VTSS_APPL_IP_ROUTE_TYPE_ANY || type == type_itr) {
            Vector<std::string> cmds;
            std::string         vty_res;
            std::string         staticd_running_config;

            if (type_itr == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC) {
                cmds.push_back("do show ip route json");
            } else {
                cmds.push_back("do show ipv6 route json");
            }

            VTSS_RC(frr_daemon_cmd(FRR_DAEMON_TYPE_ZEBRA, cmds, vty_res));

            FrrIpRouteStatusParser status_parser(routes, type_itr, protocol);
            vtss::json::StreamParser s(&status_parser);
            s.process(vtss::str(vty_res));

            if (status_parser.error) {
                // User gotta know that he should enable debug trace in order to
                // get something more useful out.
                routes.clear();
                T_EG(FRR_TRACE_GRP_IP_ROUTE, "IPv%d: Unable to parse output. error = %s\n%s", type_itr == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC ? 4 : 6, status_parser.error_str.buf.c_str(), vty_res.c_str());
                return FRR_RC_IP_ROUTE_PARSE_ERROR;
            }

            // Before FRR 6.0, there was no such thing as a staticd daemon.
            // Static routes were added through the zebra daemon.
            // Starting with FRR 6.0, this was changed so that it's no longer
            // possible to add static routes through zebra. Instead, the new
            // staticd daemon must be used.
            // For IPv4, this has changed what comes out of "show ip route" of
            // zebra. Inactive routes are no longer displayed, since they are
            // kept internally in staticd.
            // For IPv6, it has always been a problem that inactive routes were
            // not displayed, whether using zebra or staticd to install the
            // routes.
            // This is why we also parse the running-config of staticd (if it
            // exists, otherwise of zebra) and add the missing statically
            // configured routes. The routes that get added this way are marked
            // as inactive.
            if (protocol != VTSS_APPL_IP_ROUTE_PROTOCOL_ANY && protocol != VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC) {
                // We only handle static routes in staticd daemon.
                continue;
            }

            VTSS_RC(frr_daemon_running_config_get(frr_has_staticd() ? FRR_DAEMON_TYPE_STATIC : FRR_DAEMON_TYPE_ZEBRA, staticd_running_config));
            FrrIpRouteConfigParser config_parser(routes, type_itr);
            frr_util_conf_parser(staticd_running_config, config_parser);

            if (config_parser.error) {
                // User gotta know that he should enable debug trace in order to
                // get something more useful out.
                routes.clear();
                T_EG(FRR_TRACE_GRP_IP_ROUTE, "IPv%d: Unable to parse running-config output. error = %s\n%s", type_itr == VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC ? 4 : 6, config_parser.error_str.buf.c_str(), staticd_running_config.c_str());
                return FRR_RC_IP_ROUTE_PARSE_ERROR;
            }
        }
    }

    return MESA_RC_OK;
}

