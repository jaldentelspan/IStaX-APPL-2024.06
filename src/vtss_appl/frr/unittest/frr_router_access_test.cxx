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
 * \file frr_router_access_test.cxx
 * \brief This file is used to test the APIs in FRR access layer.
 * Follow the procedures below to execute the file.
 * 1. cd webstax2/vtss_appl/vtss_basics
 * 2. mkdir build
 * 3. cd build
 * 4. cmake ..
 * 5. make -j8
 * 6. ./frr/frr_tests
*/

#include "../router/frr_router_access.hxx"
#include <string.h>
#include <vtss/basics/api_types.h>
#include <frr_daemon.hxx>
#include <vtss/basics/string.hxx>
#include "catch.hpp"

//----------------------------------------------------------------------------
//** Key chain
//----------------------------------------------------------------------------
// frr_key_chain_name
TEST_CASE("frr_key_chain_name_set", "[frr]") {
    SECTION("add a key chain name") {
        std::string str = "kctest";
        auto cmds = vtss::to_vty_key_chain_name_set(str, false);
        CHECK(cmds.size() == 2);
        CHECK(cmds[0] == "configure terminal");
        CHECK(cmds[1] == "key chain kctest");
    }
    SECTION("delete a key chain name") {
        std::string str = "kktt";
        auto cmds = vtss::to_vty_key_chain_name_set(str, true);
        CHECK(cmds.size() == 2);
        CHECK(cmds[0] == "configure terminal");
        CHECK(cmds[1] == "no key chain kktt");
    }
}

// frr_key_chain_key_id
TEST_CASE("frr_key_chain_key_id_set", "[frr]") {
    SECTION("add a new key id") {
        std::string keychain_name = "kctest11";
        uint32_t key_id = 44;
        vtss::FrrKeyChainConf key_conf;
        std::string str = "kctest";
        auto cmds =
                vtss::to_vty_key_chain_key_id_set(keychain_name, key_id, false);
        CHECK(cmds.size() == 3);
        CHECK(cmds[0] == "configure terminal");
        CHECK(cmds[1] == "key chain kctest11");
        CHECK(cmds[2] == "key 44");
    }

    SECTION("frr_key_chain_key_id_del") {
        std::string keychain_name = "kctest11";
        uint32_t key_id = 2;
        auto cmds =
                vtss::to_vty_key_chain_key_id_set(keychain_name, key_id, true);
        CHECK(cmds.size() == 3);
        CHECK(cmds[0] == "configure terminal");
        CHECK(cmds[1] == "key chain kctest11");
        CHECK(cmds[2] == "no key 2");
    }
}

// frr_key_chain_key_conf_set/erase
TEST_CASE("frr_key_chain_key_conf_set", "[frr]") {
    SECTION("set a key config") {
        std::string keychain_name = "kctest11";
        uint32_t key_id = 100;
        vtss::FrrKeyChainConf key_conf = {};
        key_conf.key_str = "kctest";

        auto cmds = vtss::to_vty_key_chain_key_conf_set(keychain_name, key_id,
                                                        key_conf, false);
        CHECK(cmds.size() == 4);
        CHECK(cmds[0] == "configure terminal");
        CHECK(cmds[1] == "key chain kctest11");
        CHECK(cmds[2] == "key 100");
        CHECK(cmds[3] == "key-string kctest");
    }

    SECTION("erase a key config") {
        std::string keychain_name = "kctest11";
        uint32_t key_id = 100;
        vtss::FrrKeyChainConf key_conf = {};
        key_conf.key_str = "kctest";

        auto cmds = vtss::to_vty_key_chain_key_conf_set(keychain_name, key_id,
                                                        key_conf, true);
        CHECK(cmds.size() == 4);
        CHECK(cmds[0] == "configure terminal");
        CHECK(cmds[1] == "key chain kctest11");
        CHECK(cmds[2] == "key 100");
        CHECK(cmds[3] == "no key-string kctest");
    }
}
// frr_key_chain_key_conf_get
TEST_CASE("frr_key_chain_conf_get", "[frr]") {
    SECTION("parse no key chain") {
        const std::string config = R"(frr version 2.0
frr defaults traditional
!
hostname ripd
password zebra
log file /tmp/ripd.log
!
router rip
!
line vty
!
)";

        auto res = frr_key_chain_conf_get(config, true);
        CHECK(res.size() == 0);
    }

    SECTION("parse multiple keys") {
        const std::string config = R"(frr version 4.0-MyOwnFRRVersion
frr defaults traditional
username cumulus nopassword
!
service integrated-vtysh-config
!
key chain test
 key 1
  key-string ttt
!
key chain xyzzzz
!
key chain lala
 key 2147483647
  key-string ujxdiof
!
key chain def9
 key 40
  key-string 012345678901234567890123456789012345678901234567890123456789012
 key 5
  key-string 555
!
log syslog informational
!
)";

        auto res = frr_key_chain_conf_get(config, true);
        CHECK(res.size() == 4);
        res = frr_key_chain_conf_get(config, false);
        CHECK(res.size() == 4);
        CHECK((res[{"test", 1}].key_str.valid()) == true);
        CHECK((res[{"test", 1}].key_str.get()) == "ttt");
        CHECK((res[{"test", 3}].key_str.valid()) == false);
        CHECK((res[{"xyzzzz", 4294967295}].key_str.valid()) == false);
        CHECK((res[{"lala", 2147483647}].key_str.valid()) == true);
        CHECK((res[{"lala", 2147483647}].key_str.get()) == "ujxdiof");
        CHECK((res[{"def9", 40}].key_str.get()) == "012345678901234567890123456789012345678901234567890123456789012");
        CHECK((res[{"def9", 5}].key_str.get()) == "555");
    }
}

//----------------------------------------------------------------------------
//** access-list configuration
//----------------------------------------------------------------------------
TEST_CASE("to_vty_access_list_conf_set", "[frr]") {
    vtss::FrrAccessList val;
    val.name = "test";
    val.mode = vtss::FrrAccessListMode_Deny;
    val.net.address = 0x01020304;
    val.net.prefix_size = 8;

    auto result = to_vty_access_list_conf_set(val);
    CHECK(result.size() == 2);
    CHECK(result[0] == "configure terminal");
    CHECK(result[1] == "access-list test deny 1.2.3.4/8");

    val.mode = vtss::FrrAccessListMode_Permit;
    result = to_vty_access_list_conf_set(val);
    CHECK(result.size() == 2);
    CHECK(result[0] == "configure terminal");
    CHECK(result[1] == "access-list test permit 1.2.3.4/8");
}

TEST_CASE("to_vty_access_list_conf_del", "[frr]") {
    vtss::FrrAccessList val;
    val.name = "test";
    val.mode = vtss::FrrAccessListMode_Deny;
    val.net.address = 0x01020304;
    val.net.prefix_size = 8;
    bool is_list = 0;

    auto result = to_vty_access_list_conf_del(val, is_list);
    CHECK(result.size() == 2);
    CHECK(result[0] == "configure terminal");
    CHECK(result[1] == "no access-list test deny 1.2.3.4/8");

    val.mode = vtss::FrrAccessListMode_Permit;
    val.net.prefix_size = 19;
    result = to_vty_access_list_conf_del(val, is_list);
    CHECK(result.size() == 2);
    CHECK(result[0] == "configure terminal");
    CHECK(result[1] == "no access-list test permit 1.2.3.4/19");

    is_list = 1;
    val.mode = vtss::FrrAccessListMode_Permit;
    result = to_vty_access_list_conf_del(val, is_list);
    CHECK(result.size() == 2);
    CHECK(result[0] == "configure terminal");
    CHECK(result[1] == "no access-list test");
}

TEST_CASE("frr_access_list_conf_get_and_get_set", "[frr]") {
    SECTION("invalid instance") {
        std::string empty_config = "";
        auto res = vtss::frr_access_list_conf_get(empty_config);
        CHECK(res.size() == 0);

        auto res_set = vtss::frr_access_list_conf_get_set(empty_config);
        CHECK(res_set.size() == 0);
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
 redistribute static route-map xxx
 network 1.0.2.0/24
 network 1.0.3.0/24
!
ip route 0.0.0.0/32 1.2.3.4
!
access-list 1xxx permit any
access-list test2 deny 192.1.3.10/13
access-list test2 permit 1.1.3.10/13
access-list yyy permit 1.2.3.4/25
access-list testtest deny any
!
line vty
!
)";

        auto res = vtss::frr_access_list_conf_get(config);
        CHECK(res.size() == 5);
        CHECK(res[0].name == "1xxx");
        CHECK(res[0].mode == vtss::FrrAccessListMode_Permit);
        CHECK(res[0].net.address == 0x0);
        CHECK(res[0].net.prefix_size == 0);
        CHECK(res[1].name == "test2");
        CHECK(res[1].mode == vtss::FrrAccessListMode_Deny);
        CHECK(res[1].net.address == 0xC001030a);
        CHECK(res[2].name == "test2");
        CHECK(res[2].mode == vtss::FrrAccessListMode_Permit);
        CHECK(res[2].net.address == 0x0101030a);
        CHECK(res[2].net.prefix_size == 13);
        CHECK(res[3].name == "yyy");
        CHECK(res[3].mode == vtss::FrrAccessListMode_Permit);
        CHECK(res[3].net.address == 0x01020304);
        CHECK(res[3].net.prefix_size == 25);
        CHECK(res[4].name == "testtest");
        CHECK(res[4].mode == vtss::FrrAccessListMode_Deny);
        CHECK(res[4].net.address == 0x0);
        CHECK(res[4].net.prefix_size == 0);

        auto res_set = vtss::frr_access_list_conf_get_set(config);
        CHECK(res_set.size() == 5);

        auto itr = res_set.greater_than_or_equal({"peter", vtss::FrrAccessListMode_Permit, {}});
        CHECK(itr->name == "test2");

        auto it = res_set.begin();
        CHECK(it->name == "1xxx");
        CHECK(it->mode == vtss::FrrAccessListMode_Permit);
        CHECK(it->net.address == 0x0);
        CHECK(it->net.prefix_size == 0);
        ++it;
        CHECK(it->name == "test2");
        CHECK(it->mode == vtss::FrrAccessListMode_Deny);
        CHECK(it->net.address == 0xC001030a);
        ++it;
        CHECK(it->name == "test2");
        CHECK(it->mode == vtss::FrrAccessListMode_Permit);
        CHECK(it->net.address == 0x0101030a);
        CHECK(it->net.prefix_size == 13);
        ++it;
        CHECK(it->name == "testtest");
        CHECK(it->mode == vtss::FrrAccessListMode_Deny);
        CHECK(it->net.address == 0x0);
        CHECK(it->net.prefix_size == 0);
        ++it;
        CHECK(it->name == "yyy");
        CHECK(it->mode == vtss::FrrAccessListMode_Permit);
        CHECK(it->net.address == 0x01020304);
        CHECK(it->net.prefix_size == 25);

        auto entry = res_set.find({"1xxx", vtss::FrrAccessListMode_Permit, {0, 0}});
        CHECK(entry != res_set.end());
        entry = res_set.find( {"test2", vtss::FrrAccessListMode_Deny, {0xc001030a, 13}});
        CHECK(entry != res_set.end());
        entry = res_set.find({"test2", vtss::FrrAccessListMode_Permit, {0x101030a, 13}});
        CHECK(entry != res_set.end());
        entry = res_set.find({"testtest", vtss::FrrAccessListMode_Deny, {0, 0}});
        CHECK(entry != res_set.end());
        entry = res_set.find({"yyy", vtss::FrrAccessListMode_Permit, {0x1020304, 25}});
        CHECK(entry != res_set.end());
    }
}

