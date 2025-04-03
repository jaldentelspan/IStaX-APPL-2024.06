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

/**
 * \file frr_access_test.cxx
 * \brief This file is used to test the APIs in FRR access layer.
 * Follow the procedures below to execute the file.
 * 1. cd webstax2/vtss_appl/vtss_basics
 * 2. mkdir build
 * 3. cd build
 * 4. cmake ..
 * 5. make -j8
 * 6. ./frr/frr_tests
*/

#include <string.h>
#include <vtss/basics/api_types.h>
#include <frr_daemon.hxx>
#include <frr_ip_route.hxx>
#include <vtss/basics/string.hxx>
#include "catch.hpp"

TEST_CASE("frr_ip_route_set", "[frr]") {
    SECTION("set config ipv4 with distance") {
        vtss::FrrIpRoute r;
        r.net.route.ipv4_uc.network.address = 0;
        r.net.route.ipv4_uc.network.prefix_size = 24;
        r.net.route.ipv4_uc.destination = 1;
        r.net.type = MESA_ROUTING_ENTRY_TYPE_IPV4_UC;
        r.distance = 2;

        auto result = to_vty_ip_route_conf_set(r);
        CHECK(result.size() == 2);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "ip route 0.0.0.0/24 0.0.0.1 2");
    }
    SECTION("set config ipv4 without distance") {
        vtss::FrrIpRoute r;
        r.net.route.ipv4_uc.network.address = 3;
        r.net.route.ipv4_uc.network.prefix_size = 16;
        r.net.route.ipv4_uc.destination = 4;
        r.net.type = MESA_ROUTING_ENTRY_TYPE_IPV4_UC;
        r.distance = 1;

        auto result = to_vty_ip_route_conf_set(r);
        CHECK(result.size() == 2);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "ip route 0.0.0.3/16 0.0.0.4");
    }
    SECTION("set config ipv6 with distance") {
        vtss::FrrIpRoute r;
        r.net.route.ipv6_uc.network.address = {1, 2, 3};
        r.net.route.ipv6_uc.network.prefix_size = 60;
        r.net.route.ipv6_uc.destination = {5, 6, 7};
        r.net.type = MESA_ROUTING_ENTRY_TYPE_IPV6_UC;
        r.distance = 2;

        auto result = to_vty_ip_route_conf_set(r);
        CHECK(result.size() == 2);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "ipv6 route 102:300::/60 506:700:: 2");
    }

    SECTION("set config ipv6 withtout distance") {
        vtss::FrrIpRoute r;
        r.net.route.ipv6_uc.network.address = {1, 5, 8};
        r.net.route.ipv6_uc.network.prefix_size = 20;
        r.net.route.ipv6_uc.destination = {5, 6, 7};
        r.net.type = MESA_ROUTING_ENTRY_TYPE_IPV6_UC;
        r.distance = 1;

        auto result = to_vty_ip_route_conf_set(r);
        CHECK(result.size() == 2);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "ipv6 route 105:800::/20 506:700::");
    }
}

TEST_CASE("frr_ip_route_del", "[frr]") {
    SECTION("delete config ipv4") {
        vtss::FrrIpRoute r;
        r.net.route.ipv4_uc.network.address = 5;
        r.net.route.ipv4_uc.network.prefix_size = 8;
        r.net.route.ipv4_uc.destination = 7;
        r.net.type = MESA_ROUTING_ENTRY_TYPE_IPV4_UC;

        auto result = to_vty_ip_route_conf_del(r);
        CHECK(result.size() == 2);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "no ip route 0.0.0.5/8 0.0.0.7");
    }
    SECTION("delete config ipv6") {
        vtss::FrrIpRoute r;
        r.net.route.ipv6_uc.network.address = {1, 2, 3, 0, 5};
        r.net.route.ipv6_uc.network.prefix_size = 40;
        r.net.route.ipv6_uc.destination = {2, 6, 7};
        r.net.type = MESA_ROUTING_ENTRY_TYPE_IPV6_UC;

        auto result = to_vty_ip_route_conf_del(r);
        CHECK(result.size() == 2);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "no ipv6 route 102:300:500::/40 206:700::");
    }
}

TEST_CASE("frr_ip_route_get", "[frr]") {
    SECTION("get config without distance") {
        std::string conf = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf network 192.168.1.0/24 area 0.0.0.0
!
ip route 0.0.0.1/8 0.0.0.5
!
line vty
!
)";
        auto result = frr_ip_route_conf_get(conf);
        CHECK(result.size() == 1);
        CHECK(result[0].net.route.ipv4_uc.network.address == 1);
        CHECK(result[0].net.route.ipv4_uc.network.prefix_size == 8);
        CHECK(result[0].net.route.ipv4_uc.destination == 5);
        CHECK(result[0].distance == 1);
        CHECK(result[0].tag == 0);
    }

    SECTION("get config ipv6 without distance") {
        std::string conf = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf network 192.168.1.0/24 area 0.0.0.0
!
ipv6 route 2001:2::/96 fe80::2
!
line vty
!
)";
        auto result = frr_ip_route_conf_get(conf);
        CHECK(result[0].net.route.ipv6_uc.network.address.addr[0] == 0x20);
        CHECK(result[0].net.route.ipv6_uc.network.address.addr[1] == 0x1);
        CHECK(result[0].net.route.ipv6_uc.network.address.addr[3] == 0x2);
        CHECK(result[0].net.route.ipv6_uc.network.prefix_size == 96);
        CHECK(result[0].net.route.ipv6_uc.destination.addr[0] == 0xFE);
        CHECK(result[0].net.route.ipv6_uc.destination.addr[1] == 0x80);
        CHECK(result[0].net.route.ipv6_uc.destination.addr[15] == 0x2);
        CHECK(result[0].distance == 1);
        CHECK(result[0].tag == 0);
    }

    SECTION("get config with distance") {
        std::string conf = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf network 192.168.1.0/24 area 0.0.0.0
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";
        auto result = frr_ip_route_conf_get(conf);
        CHECK(result.size() == 1);
        CHECK(result[0].net.route.ipv4_uc.network.address == 1);
        CHECK(result[0].net.route.ipv4_uc.network.prefix_size == 8);
        CHECK(result[0].net.route.ipv4_uc.destination == 6);
        CHECK(result[0].distance == 9);
        CHECK(result[0].tag == 0);
    }

    SECTION("get config with distance and tag") {
        std::string conf = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf network 192.168.1.0/24 area 0.0.0.0
!
ip route 0.0.0.1/8 0.0.0.6 tag 4 50
!
line vty
!
)";
        auto result = frr_ip_route_conf_get(conf);
        CHECK(result.size() == 1);
        CHECK(result[0].net.route.ipv4_uc.network.address == 1);
        CHECK(result[0].net.route.ipv4_uc.network.prefix_size == 8);
        CHECK(result[0].net.route.ipv4_uc.destination == 6);
        CHECK(result[0].distance == 50);
        CHECK(result[0].tag == 4);
    }

    SECTION("get config ipv4 and ipv6") {
        std::string conf = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
ip route 0.0.0.1/8 0.0.0.6
ipv6 route 2041:5::/20 fe80::2
!
line vty
!
)";

        auto result = frr_ip_route_conf_get(conf);
        CHECK(result.size() == 2);
        CHECK(result[0].net.route.ipv4_uc.network.address == 1);
        CHECK(result[0].net.route.ipv4_uc.network.prefix_size == 8);
        CHECK(result[0].net.route.ipv4_uc.destination == 6);
        CHECK(result[0].distance == 1);
        CHECK(result[0].tag == 0);
        CHECK(result[1].net.route.ipv6_uc.network.address.addr[0] == 0x20);
        CHECK(result[1].net.route.ipv6_uc.network.address.addr[1] == 0x41);
        CHECK(result[1].net.route.ipv6_uc.network.address.addr[3] == 0x5);
        CHECK(result[1].net.route.ipv6_uc.network.prefix_size == 20);
        CHECK(result[1].net.route.ipv6_uc.destination.addr[0] == 0xFE);
        CHECK(result[1].net.route.ipv6_uc.destination.addr[1] == 0x80);
        CHECK(result[1].net.route.ipv6_uc.destination.addr[15] == 0x2);
        CHECK(result[1].distance == 1);
        CHECK(result[1].tag == 0);
    }
}

struct FrrParserRouteTest : public vtss::FrrUtilsConfStreamParserCB {
    void root(const vtss::str &line) override {
        root_lines.emplace_back(std::string(line.begin(), line.end()));
    }
    void interface(const std::string &ifname, const vtss::str &line) override {
        interfaces_lines.emplace_back(std::string(line.begin(), line.end()));
    }
    void router(const std::string &name, const vtss::str &line) override {
        router_lines.emplace_back(std::string(line.begin(), line.end()));
    }
    vtss::Vector<std::string> root_lines;
    vtss::Vector<std::string> interfaces_lines;
    vtss::Vector<std::string> router_lines;
};

TEST_CASE("frr_util_conf_parser", "[frr]") {
    std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
interface vtss.ifh
!
router ospf
    network 192.168.1.0/24 area 0.0.0.0
!
ip route 0.0.0.1/8 0.0.0.6 9
!
line vty
!
)";

    SECTION("frr_util_conf_parser route") {
        FrrParserRouteTest parser;
        frr_util_conf_parser(config, parser);
        CHECK(parser.root_lines.size() == 7);
        CHECK(parser.root_lines[0] == "frr version 2.0");
        CHECK(parser.root_lines[1] == "frr defaults traditional");
        CHECK(parser.root_lines[2] == "hostname ospfd");
        CHECK(parser.root_lines[3] == "password zebra");
        CHECK(parser.root_lines[4] == "log file /tmp/ospfd.log");
        CHECK(parser.root_lines[5] == "ip route 0.0.0.1/8 0.0.0.6 9");
        CHECK(parser.root_lines[6] == "line vty");
    }

    SECTION("frr_util_conf_parser interface") {
        FrrParserRouteTest parser;
        frr_util_conf_parser(config, parser);
        CHECK(parser.interfaces_lines.size() == 1);
        CHECK(parser.interfaces_lines[0] == "interface vtss.ifh");
    }

    SECTION("frr_conf parser router") {
        FrrParserRouteTest parser;
        frr_util_conf_parser(config, parser);
        CHECK(parser.router_lines.size() == 2);
        CHECK(parser.router_lines[0] == "router ospf");
        CHECK(parser.router_lines[1] ==
              "    network 192.168.1.0/24 area 0.0.0.0");
    }
}

TEST_CASE("frr_conf_parser_blank_line", "[frr]") {
    std::string config = R"(frr version 4.0
frr defaults traditional

!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
!
!
!
router ospf
    area 0.0.0.1 stub
    area 0.0.0.2 nssa
    area 0.0.0.3 nssa
    area 0.0.0.3 nssa no-summary
    area 0.0.0.4 nssa translate-always
    area 0.0.0.5 nssa translate-never
    area 0.0.0.6 nssa translate-never
    area 0.0.0.7 stub
    area 0.0.0.9 stub no-summary

    area 0.0.0.10 nssa translate-never
!
)";

    SECTION("frr_util_conf_parser global") {
        FrrParserRouteTest parser;
        frr_util_conf_parser(config, parser);
        CHECK(parser.root_lines.size() == 5);
        CHECK(parser.root_lines[0] == "frr version 4.0");
        CHECK(parser.root_lines[1] == "frr defaults traditional");
        CHECK(parser.root_lines[2] == "hostname ospfd");
        CHECK(parser.root_lines[3] == "password zebra");
        CHECK(parser.root_lines[4] == "log file /tmp/ospfd.log");
    }
    SECTION("frr_util_conf_parser router") {
        FrrParserRouteTest parser;
        frr_util_conf_parser(config, parser);
        CHECK(parser.router_lines.size() == 11);
        CHECK(parser.router_lines[0] == "router ospf");
        CHECK(parser.router_lines[1] == "    area 0.0.0.1 stub");
        CHECK(parser.router_lines[2] == "    area 0.0.0.2 nssa");
        CHECK(parser.router_lines[3] == "    area 0.0.0.3 nssa");
        CHECK(parser.router_lines[4] == "    area 0.0.0.3 nssa no-summary");
        CHECK(parser.router_lines[5] ==
              "    area 0.0.0.4 nssa translate-always");
        CHECK(parser.router_lines[6] ==
              "    area 0.0.0.5 nssa translate-never");
        CHECK(parser.router_lines[7] ==
              "    area 0.0.0.6 nssa translate-never");
        CHECK(parser.router_lines[8] == "    area 0.0.0.7 stub");
        CHECK(parser.router_lines[9] == "    area 0.0.0.9 stub no-summary");
        CHECK(parser.router_lines[10] ==
              "    area 0.0.0.10 nssa translate-never");
    }
}

TEST_CASE("frr_ip_route_status", "[frr]") {
    std::string ip_route = R"({
"0.0.0.0\/0":[
    {
    "prefix":"0.0.0.0\/0",
    "protocol":"static",
    "distance":3,
    "metric":0,
    "nexthops":[
        {
        "ip":"0.0.0.1",
        "afi":"ipv4"
        }
    ]
    },
    {
    "prefix":"0.0.0.1/4",
    "protocol":"kernel",
    "selected":true,
    "nexthops":[
        {
        "fib":true,
        "ip":"10.10.128.1",
        "afi":"ipv4",
        "interfaceIndex":2,
        "interfaceName":"vtss.vlan.2",
        "active":true
        }
    ]
    },
    {
    "prefix":"0.0.0.2/4",
    "protocol":"rip",
    "selected":true,
    "nexthops":[
        {
        "fib":true,
        "ip":"10.10.128.1",
        "afi":"ipv4",
        "interfaceIndex":2,
        "interfaceName":"vtss.vlan.2",
        "active":true
        }
    ]
    }
],
"10.10.128.0/22":[
    {
    "prefix":"10.10.128.0/22",
    "protocol":"connected",
    "selected":true,
    "nexthops":[
        {
        "fib":true,
        "directlyConnected":true,
        "interfaceIndex":2,
        "interfaceName":"vtss.vlan.2",
        "active":true
        }
    ]
    }
],
"10.10.138.15/32":[
    {
    "prefix":"10.10.138.15/32",
    "protocol":"kernel",
    "selected":true,
    "nexthops":[
        {
        "fib":true,
        "ip":"10.10.128.1",
        "afi":"ipv4",
        "interfaceIndex":2,
        "interfaceName":"vtss.vlan.2",
        "active":true
        }
    ]
    }
],
"96.0.0.0/30":[
    {
    "prefix":"96.0.0.0/30",
    "protocol":"static",
    "distance":255,
    "metric":0,
    "nexthops":[
        {
        "ip":"1.2.3.4",
        "afi":"ipv4"
        }
    ]
    }
],
"97.0.0.0/30":[
    {
    "prefix":"97.0.0.0/30",
    "protocol":"static",
    "distance":1,
    "metric":0,
    "nexthops":[
        {
        "ip":"1.2.3.4",
        "afi":"ipv4"
        }
    ]
    }
],
"98.0.0.0/30":[
    {
    "prefix":"98.0.0.0/30",
    "protocol":"static",
    "distance":4,
    "metric":0,
    "nexthops":[
        {
        "ip":"1.2.3.4",
        "afi":"ipv4"
        }
    ]
    }
],
"99.0.0.0/30":[
    {
    "prefix":"99.0.0.0/30",
    "protocol":"static",
    "distance":1,
    "metric":0,
    "nexthops":[
        {
        "ip":"1.2.3.4",
        "afi":"ipv4"
        }
    ]
    }
],
"169.254.0.0/16":[
    {
    "prefix":"169.254.0.0/16",
    "protocol":"kernel",
    "selected":true,
    "nexthops":[
        {
        "fib":true,
        "directlyConnected":true,
        "interfaceIndex":2,
        "interfaceName":"vtss.vlan.2",
        "active":true
        }
    ]
    }
],
"192.168.1.0/24":[
    {
    "prefix":"192.168.1.0/24",
    "protocol":"ospf",
    "distance":110,
    "metric":1,
    "uptime":"01:16:52",
    "nexthops":[
        {
        "directlyConnected":true,
        "interfaceIndex":5,
        "interfaceName":"vtss.vlan.5",
        "active":true
        }
    ]
    },
    {
    "prefix":"192.168.1.0/24",
    "protocol":"connected",
    "selected":true,
    "nexthops":[
        {
        "fib":true,
        "directlyConnected":true,
        "interfaceIndex":5,
        "interfaceName":"vtss.vlan.5",
        "active":true
        }
    ]
    }
]
}
)";

    auto result = vtss::frr_ip_route_status_parse(ip_route);
    CHECK(result.size() == 9);
    CHECK(result[0].routes.size() == 3);
    CHECK(result[0].routes[0].prefix.address == 0);
    CHECK(result[0].routes[0].prefix.prefix_size == 0);
    CHECK(result[0].routes[0].distance == 3);
    CHECK(result[0].routes[0].protocol == vtss::Route_Static);
    CHECK(result[0].routes[0].selected == false);
    CHECK(result[0].routes[0].metric == 0);
    CHECK(result[0].routes[0].next_hops.size() == 1);
    CHECK(result[0].routes[0].next_hops[0].ip == 1);
    CHECK(result[0].routes[0].next_hops[0].os_ifindex == 0);
    CHECK(result[0].routes[1].prefix.address == 1);
    CHECK(result[0].routes[1].prefix.prefix_size == 4);
    CHECK(result[0].routes[1].distance == 1);
    CHECK(result[0].routes[1].protocol == vtss::Route_Kernel);
    CHECK(result[0].routes[2].protocol == vtss::Route_Rip);
    CHECK(result[0].routes[1].selected == true);
    CHECK(result[0].routes[1].metric == 0);
    CHECK(result[0].routes[1].next_hops.size() == 1);
    CHECK(result[0].routes[1].next_hops[0].ip == 0xa0a8001);
    CHECK(result[0].routes[1].next_hops[0].selected == true);
    CHECK(result[1].routes[0].distance == 1);
    CHECK(result[8].routes[0].up_time.raw() == 4612);
}

TEST_CASE("frr_ip_route_status_selected", "[frr]") {
    std::string ip_route = R"({
"192.168.5.0\/24":[
    {
        "prefix":"192.168.5.0\/24",
        "protocol":"ospf",
        "distance":110,
        "metric":30,
        "uptime":"00:02:47",
        "nexthops":[
        {
            "ip":"192.168.4.1",
            "afi":"ipv4",
            "interfaceIndex":5,
            "interfaceName":"vtss.vlan.5",
            "active":true
        },
        {
            "ip":"192.168.3.1",
            "afi":"ipv4",
            "interfaceIndex":4,
            "interfaceName":"vtss.vlan.4",
            "active":true
        }
        ]
    },
    {
        "prefix":"192.168.5.0\/24",
        "protocol":"static",
        "selected":true,
        "distance":1,
        "metric":0,
        "nexthops":[
        {
            "ip":"192.168.4.1",
            "afi":"ipv4"
        },
        {
            "fib":true,
            "ip":"192.168.3.1",
            "afi":"ipv4",
            "interfaceIndex":4,
            "interfaceName":"vtss.vlan.4",
            "active":true
        }
        ]
    }
]
}
)";
    auto result = vtss::frr_ip_route_status_parse(ip_route);
    CHECK(result.size() == 1);
    CHECK(result[0].routes.size() == 2);
    CHECK(result[0].routes[0].selected == false);
    CHECK(result[0].routes[1].selected == true);
    CHECK(result[0].routes[1].next_hops.size() == 2);
    CHECK(result[0].routes[1].next_hops[0].selected == false);
    CHECK(result[0].routes[1].next_hops[1].selected == true);
}

TEST_CASE("frr_util_time_to_seconds", "[frr]") {
    SECTION("parse DDd HH:MM:SS") {
        std::string str_DayHHMMSS = R"(24d 20:31:23)";
        auto result = vtss::frr_util_time_to_seconds(str_DayHHMMSS);
        CHECK(result == vtss::seconds(2147483));
    }

    SECTION("parse WwdDDdHHh") {
        std::string str_WwdDDdHHh = R"(1wd5d3h)";
        auto result = vtss::frr_util_time_to_seconds(str_WwdDDdHHh);
        CHECK(result == vtss::seconds(1047600));
    }

    SECTION("parse DDdHHhMMm") {
        std::string str_DDdHHhMMm = R"(6d13h20m)";
        auto result = vtss::frr_util_time_to_seconds(str_DDdHHhMMm);
        CHECK(result == vtss::seconds(566400));
    }

    SECTION("parse HHMMSS") {
        std::string str_HHMMSS = R"(10:56:30)";
        auto result = vtss::frr_util_time_to_seconds(str_HHMMSS);
        CHECK(result == vtss::seconds(39390));
    }

    SECTION("parse MMSS") {
        std::string str_MMSS = R"(56:30)";
        auto result = vtss::frr_util_time_to_seconds(str_MMSS);
        CHECK(result == vtss::seconds(3390));
    }
}
