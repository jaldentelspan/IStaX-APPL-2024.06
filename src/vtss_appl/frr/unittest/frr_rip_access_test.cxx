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

#include "../rip/frr_rip_access.hxx"
#include <string.h>
#include <vtss/basics/api_types.h>
#include <frr_daemon.hxx>
#include <vtss/basics/string.hxx>
#include "catch.hpp"

//----------------------------------------------------------------------------
//** RIP running configuration mode parsing
//----------------------------------------------------------------------------
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

TEST_CASE("frr_rip_running_conf_parser", "[frr]") {
    std::string config = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1
!
router rip
!
line vty
!
)";

    SECTION("frr_rip_conf_parser root_mode") {
        FrrParserRouteTest parser;
        frr_util_conf_parser(config, parser);
        CHECK(parser.root_lines.size() == 6);
        CHECK(parser.root_lines[0] == "frr version 4.0");
        CHECK(parser.root_lines[1] == "frr defaults traditional");
        CHECK(parser.root_lines[2] == "hostname ripd");
        CHECK(parser.root_lines[3] == "password zebra");
        CHECK(parser.root_lines[4] == "log file /tmp/ripd.log");
    }

    SECTION("frr_rip_conf_parser interface_mode") {
        FrrParserRouteTest parser;
        frr_util_conf_parser(config, parser);
        CHECK(parser.interfaces_lines.size() == 1);
        CHECK(parser.interfaces_lines[0] == "interface vtss.vlan.1");
    }

    SECTION("frr_rip_conf_parser router_mode") {
        FrrParserRouteTest parser;
        frr_util_conf_parser(config, parser);
        CHECK(parser.router_lines.size() == 1);
        CHECK(parser.router_lines[0] == "router rip");
    }
}

//----------------------------------------------------------------------------
//** RIP router configuration
//----------------------------------------------------------------------------
TEST_CASE("to_vty_rip_router_conf_set", "[frr]") {
    SECTION("enable rip router mode") {
        auto result = vtss::to_vty_rip_router_mode_set(true);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "no router rip");
        CHECK(result[2] == "router rip");
    }

    SECTION("disable rip router mode") {
        auto result = vtss::to_vty_rip_router_mode_set(false);
        CHECK(result.size() == 2);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "no router rip");
    }

    SECTION("config global version") {
        vtss::FrrRipRouterConf conf;
        conf.router_mode = true;
        for (vtss::FrrRipVer idx = vtss::FrrRipVer_1; idx != vtss::FrrRipVer_Both;
             idx = static_cast<vtss::FrrRipVer>(static_cast<int>(idx) + 1)) {
            conf.version = idx;
            auto result = vtss::to_vty_rip_router_conf_set(conf);
            CHECK(result.size() == 3);
            CHECK(result[0] == "configure terminal");
            CHECK(result[1] == "router rip");
            if (idx == vtss::FrrRipVer_1) {
                CHECK(result[2] == "version 1");
            } else if (idx == vtss::FrrRipVer_2) {
                CHECK(result[2] == "version 2");
            } else {
                CHECK(result[2] == "no version");
            }
        }
    }

    SECTION("config timers basic") {
        vtss::FrrRipRouterConf conf;
        vtss::FrrRipRouterTimersConf timers_conf = {20, 40, 50};
        conf.router_mode = true;
        conf.timers = timers_conf;
        auto result = vtss::to_vty_rip_router_conf_set(conf);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "timers basic 20 40 50");
    }

    SECTION("config redistributed default metric") {
        vtss::FrrRipRouterConf conf;
        conf.router_mode = true;
        conf.redist_def_metric = 1;
        auto result = vtss::to_vty_rip_router_conf_set(conf);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "default-metric 1");
    }

    SECTION("config default route redistribution") {
        vtss::FrrRipRouterConf conf;
        conf.router_mode = true;
        conf.def_route_redist = true;
        auto result = vtss::to_vty_rip_router_conf_set(conf);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "default-information originate");
    }

    SECTION("config passive-interface default") {
        vtss::FrrRipRouterConf conf;
        conf.router_mode = true;
        conf.def_passive_intf = true;
        auto result = vtss::to_vty_rip_router_conf_set(conf);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "passive-interface default");
    }

    SECTION("config redistributed default metric") {
        vtss::FrrRipRouterConf conf;
        conf.router_mode = true;
        conf.admin_distance = 1;
        auto result = vtss::to_vty_rip_router_conf_set(conf);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "distance 1");
    }

    SECTION("set all fields in FrrOspRouterConf") {
        vtss::FrrRipRouterConf conf;
        conf.router_mode = true;

        conf.version = vtss::FrrRipVer_1;
        vtss::FrrRipRouterTimersConf timers_conf = {30, 90, 60};
        conf.timers = timers_conf;
        conf.redist_def_metric = 2;
        conf.def_route_redist = true;
        conf.def_passive_intf = true;
        conf.admin_distance = 130;

        auto result = vtss::to_vty_rip_router_conf_set(conf);
        CHECK(result.size() == 8);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "version 1");
        CHECK(result[3] == "timers basic 30 90 60");
        CHECK(result[4] == "default-metric 2");
        CHECK(result[5] == "default-information originate");
        CHECK(result[6] == "passive-interface default");
        CHECK(result[7] == "distance 130");
    }
}

TEST_CASE("frr_rip_router_conf_get", "[frr]") {
    std::string config_rip_enabled = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1
!
router rip
!
line vty
!
)";

    SECTION("parse rip router enabled mode") {
        auto result = frr_rip_router_conf_get(config_rip_enabled);
        CHECK(result->router_mode == true);
    }

    std::string config_rip_disabled = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1
!
!
line vty
!
)";

    SECTION("parse rip router disabled mode") {
        auto result = frr_rip_router_conf_get(config_rip_disabled);
        CHECK(result->router_mode == false);
    }

    std::string config_specific = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1
!
router rip
 version 2
 timers basic 20 40 30
 default-metric 2
 default-information originate
 passive-interface default
 distance 130
!
line vty
!
)";

    SECTION("parse RIP general configuration (specific setting)") {
        auto result = frr_rip_router_conf_get(config_specific);

        // Global version
        CHECK(result->version.valid() == true);
        CHECK(result->version.get() == vtss::FrrRipVer_2);

        // Timers
        CHECK(result->timers.valid() == true);
        CHECK(result->timers->update_timer == 20);
        CHECK(result->timers->invalid_timer == 40);
        CHECK(result->timers->garbage_collection_timer == 30);

        // Redistributed default metric
        CHECK(result->redist_def_metric.valid() == true);
        CHECK(result->redist_def_metric.get() == 2);

        // Default route redistribution
        CHECK(result->def_route_redist.valid() == true);
        CHECK(result->def_route_redist.get() == true);

        // Passive-interface default
        CHECK(result->def_passive_intf.valid() == true);
        CHECK(result->def_passive_intf.get() == true);

        // Administrative distance
        CHECK(result->admin_distance.valid() == true);
        CHECK(result->admin_distance.get() == 130);
    }

    std::string config_def = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1
!
router rip
!
line vty
!
)";

    SECTION("parse RIP general configuration (default setting)") {
        auto result = frr_rip_router_conf_get(config_def);

        // Global version
        CHECK(result->version.valid() == true);
        CHECK(result->version.get() == vtss::FrrRipVer_Both);

        // Timers
        CHECK(result->timers.valid() == true);
        CHECK(result->timers->update_timer == 30);
        CHECK(result->timers->invalid_timer == 180);
        CHECK(result->timers->garbage_collection_timer == 120);

        // Redistributed default metric
        CHECK(result->redist_def_metric.valid() == true);
        CHECK(result->redist_def_metric.get() == 1);

        // Default route redistribution
        CHECK(result->def_route_redist.valid() == true);
        CHECK(result->def_route_redist.get() == false);

        // Passive-interface default
        CHECK(result->def_passive_intf.valid() == true);
        CHECK(result->def_passive_intf.get() == false);

        // Administrative distance
        CHECK(result->admin_distance.valid() == true);
        CHECK(result->admin_distance.get() == 120);
    }
}

//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
TEST_CASE("to_vty_rip_network_conf_set", "[frr]") {
    vtss::FrrRipNetwork val;
    val.net.address = 0x01;
    val.net.prefix_size = 8;

    auto result = to_vty_rip_network_conf_set(val);
    CHECK(result.size() == 3);
    CHECK(result[0] == "configure terminal");
    CHECK(result[1] == "router rip");
    CHECK(result[2] == "network 0.0.0.1/8");
}

TEST_CASE("to_vty_rip_network_conf_del", "[frr]") {
    vtss::FrrRipNetwork val;
    val.net.address = 0x03;
    val.net.prefix_size = 16;

    auto result = to_vty_rip_network_conf_del(val);
    CHECK(result.size() == 3);
    CHECK(result[0] == "configure terminal");
    CHECK(result[1] == "router rip");
    CHECK(result[2] == "no network 0.0.0.3/16");
}

TEST_CASE("frr_rip_network_conf_get", "[frr]") {
    std::string config = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1
!
router rip
 network 2.1.2.0/24
 network 2.1.3.0/24
!
line vty
!
)";

    SECTION("parse network") {
        auto result = frr_rip_network_conf_get(config);
        CHECK(result.size() == 2);
        CHECK(result[0].net.address == 0x02010200);
        CHECK(result[0].net.prefix_size == 24);
        CHECK(result[1].net.address == 0x02010300);
        CHECK(result[1].net.prefix_size == 24);
    }

    std::string no_network = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1
!
router rip
!
line vty
!
)";

    SECTION("parse no network") {
        auto result = frr_rip_network_conf_get(no_network);
        CHECK(result.size() == 0);
    }
}

//----------------------------------------------------------------------------
//** RIP passive interface
//----------------------------------------------------------------------------
TEST_CASE("frr_rip_router_passive_if_conf_get", "[frr]") {
    std::string default_disabled = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1234
!
router rip
 timers basic 5 180 120
 network 2.1.2.0/24
 network 2.1.3.0/24
 network 2.0.0.0/8
 passive-interface vtss.vlan.1234

!
line vty
!
)";

    SECTION("parse passive interface with default mode disabled") {
        auto result = frr_rip_router_passive_if_conf_get(default_disabled, {1234});
        CHECK(result.val == true);
    }

    std::string default_enabled = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1234
!
router rip
 timers basic 5 180 120
 network 2.1.2.0/24
 network 2.1.3.0/24
 network 2.0.0.0/8
 passive-interface default
 no passive-interface vtss.vlan.1234
!
line vty
!
)";

    SECTION("parse non-passive interface with default mode enabled") {
        auto result = frr_rip_router_passive_if_conf_get(default_enabled, {1234});
        CHECK(result.val == false);
    }

    std::string default_disabled_hidden = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1234
!
router rip
 timers basic 5 180 120
 network 2.1.2.0/24
 network 2.1.3.0/24
 network 2.0.0.0/8
!
line vty
!
)";

    // The interface must be non-passive if default mode isn't enabled and
    // there's no related command for the interface in running-config.
    SECTION("parse non-passive interface with default mode disabled") {
        auto result = frr_rip_router_passive_if_conf_get(default_disabled_hidden, {1234});
        CHECK(result.val == false);
    }

    std::string default_enabled_hidden = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1234
!
router rip
 timers basic 5 180 120
 network 2.1.2.0/24
 network 2.1.3.0/24
 network 2.0.0.0/8
 passive-interface default
!
line vty
!
)";

    // The interface must be passive if default mode enabled and
    // there's no related command for the interface in running-config.
    SECTION("parse passive interface with default mode enabled") {
        auto result = frr_rip_router_passive_if_conf_get(default_enabled_hidden, {1234});
        CHECK(result.val == true);
    }
}

//----------------------------------------------------------------------------
//** RIP neighbor connection
//----------------------------------------------------------------------------
TEST_CASE("to_vty_rip_neighbor_conf_set", "[frr]") {
    SECTION("neighbor 2.2.1.7") {
        mesa_ipv4_t val = 0x02020107;
        auto result = vtss::to_vty_rip_neighbor_conf_set(val);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "neighbor 2.2.1.7");
    }

    SECTION("neighbor 2.2.0.0") {
        mesa_ipv4_t val = 0x02020000;
        auto result = vtss::to_vty_rip_neighbor_conf_set(val);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "neighbor 2.2.0.0");
    }

    SECTION("neighbor 2.2.5.255") {
        mesa_ipv4_t val = 0x020205ff;
        auto result = vtss::to_vty_rip_neighbor_conf_set(val);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "neighbor 2.2.5.255");
    }
}

TEST_CASE("to_vty_rip_neighbor_conf_del", "[frr]") {
    SECTION("neighbor 2.2.1.7") {
        mesa_ipv4_t val = 0x02020107;
        auto result = vtss::to_vty_rip_neighbor_conf_del(val);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "no neighbor 2.2.1.7");
    }

    SECTION("neighbor 2.2.0.0") {
        mesa_ipv4_t val = 0x02020000;
        auto result = vtss::to_vty_rip_neighbor_conf_del(val);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "no neighbor 2.2.0.0");
    }

    SECTION("neighbor 2.2.5.255") {
        mesa_ipv4_t val = 0x020205ff;
        auto result = vtss::to_vty_rip_neighbor_conf_del(val);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "router rip");
        CHECK(result[2] == "no neighbor 2.2.5.255");
    }
}

TEST_CASE("frr_rip_neighbor_conf_get", "[frr]") {
    std::string running_config = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1234
!
router rip
 network 2.0.0.0/8
 neighbor 2.2.0.0
 neighbor 2.2.1.7
 neighbor 2.2.5.255
!
line vty
!
)";

    SECTION("parse neighbor connections") {
        auto result = frr_rip_neighbor_conf_get(running_config);
        CHECK(result.size() == 3);
        CHECK(*result.begin() == 0x02020000);
        CHECK(*result.greater_than(0x02020000) == 0x02020107);
        CHECK(*result.greater_than(0x02020107) == 0x020205ff);
        CHECK(result.greater_than(0x020205ff) == result.end());
    }

    std::string no_neighbor_conn = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1234
!
router rip
 network 2.0.0.0/8
!
line vty
!
)";

    SECTION("parse empty neighbor connections") {
        auto result = frr_rip_neighbor_conf_get(no_neighbor_conn);
        CHECK(result.size() == 0);
    }
}

//----------------------------------------------------------------------------
//** RIP authentication
//----------------------------------------------------------------------------
// frr_rip_if_authentication_mode_conf_set
TEST_CASE("to_vty_rip_if_authentication_mode_conf_set", "[frr]") {
    SECTION("set auth mode 'Simple Pwd'") {
        auto result = vtss::to_vty_rip_if_authentication_mode_conf_set(
                "vtss.vlan.100", vtss::FrrRipIfAuthMode_Pwd);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface vtss.vlan.100");
        CHECK(result[2] == "ip rip authentication mode text");
    }

    SECTION("set auth mode 'MD5'") {
        auto result = vtss::to_vty_rip_if_authentication_mode_conf_set(
                "vtss.vlan.200", vtss::FrrRipIfAuthMode_MsgDigest);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface vtss.vlan.200");
        CHECK(result[2] == "ip rip authentication mode md5");
    }

    SECTION("set auth mode 'NULL'") {
        auto result = vtss::to_vty_rip_if_authentication_mode_conf_set(
                "vtss.vlan.300", vtss::FrrRipIfAuthMode_Null);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface vtss.vlan.300");
        CHECK(result[2] == "no ip rip authentication mode");
    }
}

// frr_rip_if_authentication_key_chain_set
TEST_CASE("to_vty_rip_if_authentication_key_chain_set", "[frr]") {
    SECTION("set the key chain name") {
        auto result = vtss::to_vty_rip_if_authentication_key_chain_set(
                "vtss.vlan.100", "kcname_test", false);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface vtss.vlan.100");
        CHECK(result[2] == "ip rip authentication key-chain kcname_test");
    }

    SECTION("erase the key chain name") {
        auto result = vtss::to_vty_rip_if_authentication_key_chain_set(
                "vtss.vlan.100", {}, true);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface vtss.vlan.100");
        CHECK(result[2] == "no ip rip authentication key-chain");
    }
}

// frr_rip_if_authentication_simple_pwd_set
TEST_CASE("to_vty_rip_if_authentication_simple_pwd_set", "[frr]") {
    SECTION("set the simple password") {
        auto result = vtss::to_vty_rip_if_authentication_simple_pwd_set(
                "vtss.vlan.100", "fdfjdkls", false);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface vtss.vlan.100");
        CHECK(result[2] == "ip rip authentication string fdfjdkls");
    }

    SECTION("erase the simple password") {
        auto result = vtss::to_vty_rip_if_authentication_simple_pwd_set(
                "vtss.vlan.100", {}, true);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface vtss.vlan.100");
        CHECK(result[2] == "no ip rip authentication string");
    }
}

// frr_rip_if_authentication_conf_get
TEST_CASE("frr_rip_if_authentication_conf_get", "[frr]") {
    std::string config_auth = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.100
 ip rip authentication mode md5 auth-length old-ripd
 ip rip authentication key-chain ttt
!
interface vtss.vlan.20
 ip rip authentication mode text
 ip rip authentication string 123
!
interface vtss.vlan.35
 ip rip authentication string dj783tjlbv6
!
router rip
!
line vty
!
)";

    SECTION("parse auth mode 'simple pwd'") {
        auto result = frr_rip_if_authentication_conf_get(config_auth, {20});
        CHECK(result.val.auth_mode == vtss::FrrRipIfAuthMode_Pwd);
        CHECK(result.val.simple_pwd == "123");
    }

    SECTION("parse auth mode md5") {
        auto result = frr_rip_if_authentication_conf_get(config_auth, {100});
        CHECK(result.val.auth_mode == vtss::FrrRipIfAuthMode_MsgDigest);
        CHECK(result.val.keychain_name == "ttt");
    }

    SECTION("parse auth mode null") {
        auto result = frr_rip_if_authentication_conf_get(config_auth, {35});
        CHECK(result.val.auth_mode == vtss::FrrRipIfAuthMode_Null);
        CHECK(result.val.simple_pwd == "dj783tjlbv6");
    }

    std::string no_auth_conf = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1234
!
router rip
 network 2.0.0.0/8
!
line vty
!
)";

    SECTION("parse default auth config") {
        auto result = frr_rip_if_authentication_conf_get(no_auth_conf, {1234});
        CHECK(result.val.auth_mode == vtss::FrrRipIfAuthMode_Null);
        CHECK(result.val.simple_pwd == "");
        CHECK(result.val.keychain_name == "");
    }
}

//----------------------------------------------------------------------------
//** RIP interface configuration: Version support
//----------------------------------------------------------------------------
TEST_CASE("to_vty_rip_intf_recv_ver_set", "[frr]") {
    SECTION("Set RIP interface receive version") {
        // All possible cases
        vtss::Vector<std::string> ver_str = {"none", "1", "2", "1 2"};
        for (vtss::FrrRipVer idx = vtss::FrrRipVer_None;
             idx != vtss::FrrRipVer_End;
             idx = static_cast<vtss::FrrRipVer>(static_cast<int>(idx) + 1)) {
            auto result =
                    vtss::to_vty_rip_intf_recv_ver_set("vtss.vlan.1234", idx);
            CHECK(result.size() == 3);
            CHECK(result[0] == "configure terminal");
            CHECK(result[1] == "interface vtss.vlan.1234");
            vtss::StringStream str_buf;
            if (idx == vtss::FrrRipVer_NotSpecified) {
                str_buf << "no ip rip receive version";
            } else {
                str_buf << "ip rip receive version " << ver_str[idx];
            }
            CHECK(result[2] == str_buf.cstring());
        }
    }
}

TEST_CASE("to_vty_rip_intf_send_ver_set", "[frr]") {
    SECTION("Set RIP interface receive version") {
        // All possible cases ('none' is unsupported)
        vtss::Vector<std::string> ver_str = {"none", "1", "2", "1 2"};
        for (vtss::FrrRipVer idx = vtss::FrrRipVer_1; idx != vtss::FrrRipVer_End;
             idx = static_cast<vtss::FrrRipVer>(static_cast<int>(idx) + 1)) {
            auto result =
                    vtss::to_vty_rip_intf_send_ver_set("vtss.vlan.1234", idx);
            CHECK(result.size() == 3);
            CHECK(result[0] == "configure terminal");
            CHECK(result[1] == "interface vtss.vlan.1234");
            vtss::StringStream str_buf;

            if (idx == vtss::FrrRipVer_NotSpecified) {
                str_buf << "no ip rip send version";
            } else {
                str_buf << "ip rip send version " << ver_str[idx];
            }
            CHECK(result[2] == str_buf.cstring());
        }
    }
}

TEST_CASE("frr_rip_intf_ver_conf_get", "[frr]") {
    std::string running_config = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1001
 ip rip receive version 1
 ip rip send version 1
interface vtss.vlan.1002
 ip rip receive version 2
 ip rip send version 2
interface vtss.vlan.1003
 ip rip receive version 1 2
 ip rip send version 1 2
interface vtss.vlan.1004
 ip rip receive version none
interface vtss.vlan.1005
!
router rip
!
line vty
!
)";

    // RIPv1
    SECTION("parse RIP receive/send version 1") {
        auto result = frr_rip_intf_ver_conf_get(running_config, {1001});
        CHECK(result->recv_ver.get() == vtss::FrrRipVer_1);
        CHECK(result->send_ver.get() == vtss::FrrRipVer_1);
    }

    // RIPv2
    SECTION("parse RIP receive/send version 2") {
        auto result = frr_rip_intf_ver_conf_get(running_config, {1002});
        CHECK(result->recv_ver.get() == vtss::FrrRipVer_2);
        CHECK(result->send_ver.get() == vtss::FrrRipVer_2);
    }

    // Both
    SECTION("parse RIP receive/send version 1 2") {
        auto result = frr_rip_intf_ver_conf_get(running_config, {1003});
        CHECK(result->recv_ver.get() == vtss::FrrRipVer_Both);
        CHECK(result->send_ver.get() == vtss::FrrRipVer_Both);
    }

    // None (For receive version only)
    SECTION("parse RIP receive version none") {
        auto result = frr_rip_intf_ver_conf_get(running_config, {1004});
        CHECK(result->recv_ver.get() == vtss::FrrRipVer_None);
    }

    // Not specified
    SECTION("parse RIP receive version none") {
        auto result = frr_rip_intf_ver_conf_get(running_config, {1005});
        CHECK(result->recv_ver.get() == vtss::FrrRipVer_NotSpecified);
        CHECK(result->send_ver.get() == vtss::FrrRipVer_NotSpecified);
    }
}

//----------------------------------------------------------------------------
//** RIP interface configuration: Split horizon
//----------------------------------------------------------------------------
TEST_CASE("to_vty_rip_if_split_horizon_set", "[frr]") {
    SECTION("set mode poisoned-reverse") {
        auto result = vtss::to_vty_rip_if_split_horizon_set(
                "vtss.vlan100",
                vtss::FRR_RIP_IF_SPLIT_HORIZON_MODE_POISONED_REVERSE);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface vtss.vlan100");
        CHECK(result[2] == "ip rip split-horizon poisoned-reverse");
    }

    SECTION("set mode disable") {
        auto result = vtss::to_vty_rip_if_split_horizon_set(
                "vtss.vlan200", vtss::FRR_RIP_IF_SPLIT_HORIZON_MODE_DISABLED);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface vtss.vlan200");
        CHECK(result[2] == "no ip rip split-horizon");
    }

    SECTION("set mode simple") {
        auto result = vtss::to_vty_rip_if_split_horizon_set(
                "vtss.vlan300", vtss::FRR_RIP_IF_SPLIT_HORIZON_MODE_SIMPLE);
        CHECK(result.size() == 3);
        CHECK(result[0] == "configure terminal");
        CHECK(result[1] == "interface vtss.vlan300");
        CHECK(result[2] == "ip rip split-horizon");
    }
}

TEST_CASE("frr_rip_if_split_horizon_conf_get", "[frr]") {
    std::string config_poison_reverse = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1234
 ip rip split-horizon poisoned-reverse
!
router rip
!
line vty
!
)";

    SECTION("parse split-hozizon mode poisoned-reverse") {
        auto result = frr_rip_if_split_horizon_conf_get(config_poison_reverse, {1234});
        CHECK(result.val == vtss::FRR_RIP_IF_SPLIT_HORIZON_MODE_POISONED_REVERSE);
    }

    std::string config_poison_disabled = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1234
 no ip rip split-horizon
!
router rip
!
line vty
!
)";

    SECTION("parse split-hozizon mode disabled") {
        auto result = frr_rip_if_split_horizon_conf_get(config_poison_disabled, {1234});
        CHECK(result.val == vtss::FRR_RIP_IF_SPLIT_HORIZON_MODE_DISABLED);
    }

    std::string config_simple = R"(
frr version 4.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
interface vtss.vlan.1234
!
router rip
!
line vty
!
)";

    SECTION("parse split-hozizon mode simple") {
        auto result = frr_rip_if_split_horizon_conf_get(config_simple, {1234});
        CHECK(result.val == vtss::FRR_RIP_IF_SPLIT_HORIZON_MODE_SIMPLE);
    }
}

//----------------------------------------------------------------------------
//** RIP general status
//----------------------------------------------------------------------------
TEST_CASE("frr_rip_status_parse", "[frr]") {
    std::string vty_output = R"(
{
  "routingProtocol":"rip",
  "updateTime":10,
  "updateRemainTime":4,
  "timeoutTime":20,
  "garbageTime":30,
  "globalRouteChanges":51,
  "globalQueries":1,
  "defaultMetric":1,
  "versionSend":2,
  "versionRecv":3,
  "distanceDefault":120
}
)";

    SECTION("parse rip status") {
        auto res = vtss::frr_rip_status_parse(vty_output);

        // Check the status data
        CHECK(res.updateTimer == 10);
        CHECK(res.invalidTimer == 20);
        CHECK(res.garbageTimer == 30);
        CHECK(res.updateRemainTime == 4);
        CHECK(res.default_metric == 1);
        CHECK(res.sendVer == vtss::FrrRipVer_2);
        CHECK(res.recvVer == vtss::FrrRipVer_Both);
        CHECK(res.default_distance == 120);
        CHECK(res.globalRouteChanges == 51);
        CHECK(res.globalQueries == 1);
    }
}

//----------------------------------------------------------------------------
//** RIP interface status
//----------------------------------------------------------------------------
TEST_CASE("frr_rip_interface_status_parse", "[frr]") {
    std::string vty_output = R"(
{
  "activeInterfaces":[
    {
      "vtss.vlan.2":{
        "sendVersion":"2",
        "receiveVersion":"1 2",
        "keychain":"",
        "ifStatRcvBadPackets":0,
        "ifStatRcvBadRoutes":0,
        "ifStatSentUpdates":184,
        "passive":1,
        "authType":3
      }
    },
    {
      "vtss.vlan.3":{
        "sendVersion":"1 2",
        "receiveVersion":"none",
        "keychain":"test",
        "ifStatRcvBadPackets":1,
        "ifStatRcvBadRoutes":3,
        "ifStatSentUpdates":185,
        "passive":0,
        "authType":0
      }
    }
  ]
}
)";

    SECTION("parse rip interface status") {
        auto res = vtss::frr_rip_interface_status_parse(vty_output);
        vtss_ifindex_t ifIndex{2};
        auto itr = res.find(ifIndex);
        vtss::FrrRipActiveIfStatus &status = itr->second;
        CHECK(res.size() == 2);
        CHECK(status.sendVer == vtss::FrrRipVer_2);
        CHECK(status.recvVer == vtss::FrrRipVer_Both);
        CHECK(status.key_chain == "");
        CHECK(status.auth_type == VTSS_APPL_RIP_AUTH_TYPE_MD5);
        CHECK(status.is_passive_intf == true);
        CHECK(status.ifStatRcvBadPackets == 0);
        CHECK(status.ifStatRcvBadRoutes == 0);
        CHECK(status.ifStatSentUpdates == 184);

        vtss_ifindex_t ifIndex2{3};
        auto itr2 = res.find(ifIndex2);
        vtss::FrrRipActiveIfStatus &status2 = itr2->second;
        CHECK(status2.sendVer == vtss::FrrRipVer_Both);
        CHECK(status2.recvVer == vtss::FrrRipVer_None);
        CHECK(status2.key_chain == "test");
        CHECK(status2.auth_type == VTSS__APPL_RIP_AUTH_TYPE_NULL);
        CHECK(status2.is_passive_intf == false);
        CHECK(status2.ifStatRcvBadPackets == 1);
        CHECK(status2.ifStatRcvBadRoutes == 3);
        CHECK(status2.ifStatSentUpdates == 185);
    }
}

//----------------------------------------------------------------------------
//** RIP neighbor status
//----------------------------------------------------------------------------
TEST_CASE("frr_rip_peer_parse", "[frr]") {
    std::string vty_output = R"(
{
  "peers":[
    {
      "1.0.1.3":{
        "recvBadPackets":33,
        "recvBadRoutes":0,
        "lastUpdate":"00:01:02",
        "domain":0,
        "version":2
      }
    },
    {
      "10.0.1.3":{
        "recvBadPackets":55,
        "recvBadRoutes":0,
        "lastUpdate":"00:00:02",
        "domain":0,
        "version":1
      }
    }
  ]
}
)";

    SECTION("parse rip peer") {
        auto res = vtss::frr_rip_peer_parse(vty_output);

        // Check the total entry count
        CHECK(res.size() == 2);

        // Check the entry data
        auto itr = res.begin();
        CHECK(itr->first == 0x01000103);
        CHECK(itr->second.recv_bad_packets == 33);
        CHECK(itr->second.rip_ver == vtss::FrrRipVer_2);
        CHECK(itr->second.last_update_time.raw32() == 62);
        itr++;
        CHECK(itr->first == 0x0a000103);
        CHECK(itr->second.recv_bad_packets == 55);
        CHECK(itr->second.rip_ver == vtss::FrrRipVer_1);
        CHECK(itr->second.last_update_time.raw32() == 2);
    }
}

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------
TEST_CASE("frr_rip_db_parse", "[frr]") {
    std::string vty_output = R"(
{
  "0.0.0.0\/0":{
    "type":"rip",
    "subType":"default",
    "self":true,
    "from":"1.2.3.4",
    "tag":1,
    "nextHops":[
      {
        "nextHopType":"ipv4",
        "nextHop":"1.1.1.1",
        "nextHopMetric":123
      }
    ],
    "time":"00:01:01"
  },
  "3.3.3.3\/24":{
    "type":"ospf",
    "subType":"redistribute",
    "self":true,
    "externalMetric":16711690,
    "tag":0,
    "nextHops":[
      {
        "nextHopType":"ipv4",
        "nextHop":"4.4.4.4",
        "nextHopMetric":1
      },
      {
        "nextHopType":"ifindex",
        "nextHop":"5.5.5.5",
        "nextHopMetric":1
      }
    ]
  }
}
)";

    SECTION("parse rip database") {
        auto res = vtss::frr_rip_db_parse(vty_output);

        // Check the total entry count
        CHECK(res.size() == 3);

        // Check the entry data
        auto entry0 = res.find({{0, 0}, 0x01010101});
        CHECK(entry0 != res.end());
        CHECK(entry0->second.type == vtss::FrrRipDbProtoType_Rip);
        CHECK(entry0->second.subtype == vtss::FrrRIpDbProtoSubType_Default);
        CHECK(entry0->second.src_addr == 0x01020304);
        CHECK(entry0->second.external_metric == 0);
        CHECK(entry0->second.tag == 1);
        CHECK(entry0->second.nexthop_type == vtss::FrrRipDbNextHopType_IPv4);
        CHECK(entry0->second.metric == 123);
        CHECK(entry0->second.uptime.raw32() == 61);

        // Check multiple nexthops
        auto entry1 = res.find({{0x03030303, 24}, 0x04040404});
        CHECK(entry1 != res.end());
        auto entry2 = res.find({{0x03030303, 24}, 0x05050505});
        CHECK(entry2 != res.end());
    }
}

//----------------------------------------------------------------------------
//** RIP redistribution
//----------------------------------------------------------------------------
// frr_rip_router_distribute_conf
TEST_CASE("frr_rip_router_redistribute_conf_set", "[frr]") {
    vtss::FrrRipRouterRedistribute val;
    val.protocol = vtss::Protocol_Connected;
    val.metric = 42;

    auto cmds = vtss::to_vty_rip_router_redistribute_conf_set(val);
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router rip");
    CHECK(cmds[2] == "redistribute connected metric 42");
}

TEST_CASE("frr_rip_router_redistribute_conf_del", "[frr]") {
    vtss_appl_ip_route_protocol_t protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED;
    auto cmds = vtss::to_vty_rip_router_redistribute_conf_del(protocol);
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router rip");
    CHECK(cmds[2] == "no redistribute connected");
}

TEST_CASE("frr_rip_router_redistribute_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_rip_router_redistribute_conf_get(empty_config);
        CHECK(res.size() == 0);
    }

    SECTION("parse multiple protocols") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ospfd
password zebra
log file /tmp/ospfd.log
!
router rip
    redistribute connected metric 16
    redistribute ospf
    redistribute static route-map ADD
    redistribute bgp metric 1
!
)";

        auto res = vtss::frr_rip_router_redistribute_conf_get(config);
        CHECK(res.size() == 4);
        CHECK(res[0].protocol == vtss::Protocol_Connected);
        CHECK(res[0].metric.valid() == true);
        CHECK(res[0].metric.get() == 16);
        CHECK(res[0].route_map.valid() == false);
        CHECK(res[1].protocol == vtss::Protocol_Ospf);
        CHECK(res[1].metric.valid() == false);
        CHECK(res[1].route_map.valid() == false);
        CHECK(res[2].protocol == vtss::Protocol_Static);
        CHECK(res[2].metric.valid() == false);
        CHECK(res[2].route_map.valid() == true);
        CHECK(res[2].route_map.get() == "ADD");
        CHECK(res[3].protocol == vtss::Protocol_Bgp);
        CHECK(res[3].metric.valid() == true);
        CHECK(res[3].metric.get() == 1);
        CHECK(res[3].route_map.valid() == false);
    }
}

//----------------------------------------------------------------------------
//** RIP metric manipulation
//----------------------------------------------------------------------------
// frr_rip_offset_list_conf
TEST_CASE("frr_rip_offset_list_conf_set", "[frr]") {
    vtss::FrrRipOffsetList val;
    val.name = "testOffset";
    val.mode = vtss::FrrRipOffsetListDirection_In;
    val.metric = 3;
    auto cmds = vtss::to_vty_offset_list_conf_set(val, 1);
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router rip");
    CHECK(cmds[2] == "offset-list testOffset in 3");

    val.name = "testOffset-1";
    val.mode = vtss::FrrRipOffsetListDirection_In;
    val.metric = 10;
    val.ifindex = {4001};
    cmds = vtss::to_vty_offset_list_conf_set(val, 1);
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router rip");
    CHECK(cmds[2] == "offset-list testOffset-1 in 10 vtss.vlan.4001");
}

// frr_rip_offset_list_conf_del
TEST_CASE("frr_rip_offset_list_conf_del", "[frr]") {
    vtss::FrrRipOffsetList val;
    val.name = "testOffset";
    val.mode = vtss::FrrRipOffsetListDirection_In;
    val.metric = 3;

    auto cmds = vtss::to_vty_offset_list_conf_set(val, 0);
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router rip");
    CHECK(cmds[2] == "no offset-list testOffset in 3");

    val.name = "testOffset-1";
    val.mode = vtss::FrrRipOffsetListDirection_In;
    val.metric = 10;
    val.ifindex = {4001};
    // cmds = vtss::to_vty_offset_list_conf_del(val);
    cmds = vtss::to_vty_offset_list_conf_set(val, 0);
    CHECK(cmds.size() == 3);
    CHECK(cmds[0] == "configure terminal");
    CHECK(cmds[1] == "router rip");
    CHECK(cmds[2] == "no offset-list testOffset-1 in 10 vtss.vlan.4001");
}

TEST_CASE("frr_rip_offset_list_conf_get", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_rip_offset_list_conf_get(empty_config);
        CHECK(res.size() == 0);
    }

    SECTION("parse multiple protocols") {
        const std::string config = R"(frr version 4.0-MyOwnFRRVersion
frr defaults traditional
hostname Router-4
no ipv6 forwarding
username cumulus nopassword
!
service integrated-vtysh-config
!
log syslog informational
!
interface eth0
 ip address 1.0.2.4/24
!
interface eth1
 ip address 1.0.3.4/24
!
router rip
 offset-list test in 11 vtss.vlan.4
 offset-list rip1 out 3 vtss.vlan.14
 offset-list rip2 in 14 vtss.vlan.1242
 offset-list rip3 in 0
 network 1.0.2.0/24
 network 1.0.3.0/24
!
ip route 0.0.0.0/32 1.2.3.4
!
line vty
!
)";

        auto res = vtss::frr_rip_offset_list_conf_get(config);
        CHECK(res.size() == 4);
        CHECK(res[0].name == "test");
        CHECK(res[0].mode == vtss::FrrRipOffsetListDirection_In);
        CHECK(res[0].metric == 11);
        CHECK(VTSS_IFINDEX_PRINTF_ARG(res[0].ifindex.get()) == 4);
        CHECK(res[1].name == "rip1");
        CHECK(res[1].mode == vtss::FrrRipOffsetListDirection_Out);
        CHECK(res[1].metric == 3);
        CHECK(VTSS_IFINDEX_PRINTF_ARG(res[1].ifindex.get()) == 14);
        CHECK(res[2].name == "rip2");
        CHECK(res[2].mode == vtss::FrrRipOffsetListDirection_In);
        CHECK(res[2].metric == 14);
        CHECK(VTSS_IFINDEX_PRINTF_ARG(res[2].ifindex.get()) == 1242);
        CHECK(res[3].name == "rip3");
        CHECK(res[3].mode == vtss::FrrRipOffsetListDirection_In);
        CHECK(res[3].metric == 0);
        CHECK(VTSS_IFINDEX_PRINTF_ARG(res[3].ifindex.get()) == 0);
    }
}

TEST_CASE("frr_rip_offset_list_conf_get_map", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_rip_offset_list_conf_get_map(empty_config);
        // CHECK(res.size() == 0);
    }

    SECTION("parse multiple protocols") {
        const std::string config = R"(frr version 4.0-MyOwnFRRVersion
!
interface eth1
 ip address 1.0.3.4/24
!
router rip
 offset-list test in 11 vtss.vlan.4
 offset-list rip1 out 3 vtss.vlan.14
 offset-list rip2 in 14 vtss.vlan.1242
 offset-list rip3 in 0
 offset-list rip1 in 3 vtss.vlan.14
 network 1.0.2.0/24
 network 1.0.3.0/24
!
ip route 0.0.0.0/32 1.2.3.4
!
line vty
!
)";

        auto res = vtss::frr_rip_offset_list_conf_get_map(config);
        CHECK(res.size() == 5);

        vtss::Map<vtss::FrrRipOffsetListKey, vtss::FrrRipOffsetListData>::iterator it;
        it = res.begin();

        CHECK(VTSS_IFINDEX_PRINTF_ARG(it->first.ifindex) == 0);
        CHECK(it->first.mode == vtss::FrrRipOffsetListDirection_In);
        it++;
        CHECK(VTSS_IFINDEX_PRINTF_ARG(it->first.ifindex) == 4);
        CHECK(it->first.mode == vtss::FrrRipOffsetListDirection_In);
        it++;
        CHECK(VTSS_IFINDEX_PRINTF_ARG(it->first.ifindex) == 14);
        CHECK(it->first.mode == vtss::FrrRipOffsetListDirection_In);
        it++;
        CHECK(VTSS_IFINDEX_PRINTF_ARG(it->first.ifindex) == 14);
        CHECK(it->first.mode == vtss::FrrRipOffsetListDirection_Out);
        it++;
        CHECK(VTSS_IFINDEX_PRINTF_ARG(it->first.ifindex) == 1242);
        CHECK(it->first.mode == vtss::FrrRipOffsetListDirection_In);
        it++;
        CHECK(it == res.end());

        auto entry = res.find({{4}, vtss::FrrRipOffsetListDirection_In});
        CHECK(entry != res.end());
        CHECK(entry->second.name == "test");
        CHECK(entry->second.metric == 11);

        entry = res.find({{14}, vtss::FrrRipOffsetListDirection_Out});
        CHECK(entry != res.end());
        CHECK(entry->second.name == "rip1");
        CHECK(entry->second.metric == 3);

        entry = res.find({{1242}, vtss::FrrRipOffsetListDirection_In});
        CHECK(entry != res.end());
        CHECK(entry->second.name == "rip2");
        CHECK(entry->second.metric == 14);

        entry = res.find({{0}, vtss::FrrRipOffsetListDirection_In});
        CHECK(entry != res.end());
        CHECK(entry->second.name == "rip3");
        CHECK(entry->second.metric == 0);
    }
}
