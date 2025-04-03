/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <iostream>
#include <string.h>
#include "catch.hpp"
#include "alarm_api.hxx"
#include "vtss/appl/alarm.h"
#include "../alarm-expose.hxx"
#include "vtss/basics/synchronized.hxx"
#include "vtss/basics/expose/json/specification/walk.hxx"
#include "alarm_test_definitions.hxx"

namespace vtss {
vtss::expose::json::RootNode JSON_RPC_ROOT;
}

namespace vtss {
namespace appl {
namespace alarm {
const vtss_enum_descriptor_t key_txt[] = {{A, "A"}, {B, "B"}, {C, "C"}, {}};
}  // namespace alarm
}  // namespace appl
}  // namespace vtss

namespace vtss {
void json_node_add(vtss::expose::json::Node *node) {
    // TODO, mutex
    node->attach_to(&JSON_RPC_ROOT);
}


namespace notifications {
vtss::notifications::SubjectRunner subject_main_thread("main", 0, true);
}  // notifications
}  // vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_alarm("alarm");
extern "C" void vtss_appl_alarm_json_init() { vtss::json_node_add(&ns_alarm); }

namespace vtss {
namespace appl {
namespace alarm {

TEST_CASE("alarm-expose-basic", "[alarm]") {
    vtss_appl_alarm_json_init();

    REQUIRE(DottedName("") < DottedName("a"));
    REQUIRE_FALSE(DottedName("a") < DottedName(""));
    REQUIRE(DottedName("a") < DottedName("ab"));
    REQUIRE_FALSE(DottedName("abc") < DottedName("abc"));
    REQUIRE_FALSE(DottedName("abc") < DottedName("ab"));
    REQUIRE(DottedName("a") < DottedName("a.m"));
    REQUIRE(DottedName("a") < DottedName("a.m.x"));
    REQUIRE(DottedName("a.m") < DottedName("a.m.x"));
    REQUIRE_FALSE(DottedName("a.m.x") < DottedName("a.m"));
    REQUIRE(DottedName("ab") < DottedName("ab.m"));
    REQUIRE(DottedName("ab") < DottedName("ab.m.x"));
    REQUIRE(DottedName("ab.m") < DottedName("ab.m.x"));
    REQUIRE(DottedName("a") < DottedName("ab"));
    REQUIRE(DottedName("a.m") < DottedName("ab.m"));
    REQUIRE(DottedName("a.m.x") < DottedName("ab.m.x"));

    REQUIRE_FALSE(DottedName("ab") < DottedName("a"));
    REQUIRE_FALSE(DottedName("ab.m") < DottedName("a.m"));
    REQUIRE_FALSE(DottedName("ab.m.x") < DottedName("a.m.x"));
}

TEST_CASE("Verify-creation-of-alarm", "[alarm]") {
    vtss::expose::json::specification::Inventory inv;

    for (auto &i : JSON_RPC_ROOT.leafs) walk(inv, &i, false);

    // StringStream output;
    // output << inv;
    // printf("%s\n", output.cstring());
}

StructStatus<expose::ParamVal<Foo *>> s_foo;
expose::json::NamespaceNode ns_test(&JSON_RPC_ROOT, "test");
expose::json::StructReadOnlyNotification<S1> s1_(&ns_test, "s1", &s_foo);


TEST_CASE("alarm-expose", "[alarm]") {
    expose::json::RootNode root;
    expose::json::NamespaceNode ns_test(&root, "test");

    expose::json::StructReadOnlyNotification<S1> s1_(&ns_test, "s1", &s_foo);

    Foo foo;
    foo.b = true;
    foo.u8 = 0;
    foo.u16 = 0;
    foo.u32 = 5;
    foo.u64 = 0;
    foo.i16 = 0;
    foo.i32 = 5;
    foo.i64 = 0;
    foo.e = A;
    foo.s[0] = 0;
    foo.b1 = {0, 0, 0};
    foo.b2 = {0, 0, 0};

    s_foo.set(foo);

    vtss_appl_alarm_name_t leafname1 = {"alarm.node1.leaf1"};
    vtss_appl_alarm_name_t leafname2 = {"alarm.node1.leaf2"};
    vtss_appl_alarm_expression_t expression1 = {"test.s1@u32 != test.s1@u32"};
    SECTION("Add-one-alarm") {
        vtss_appl_alarm_name_t itr;
        vtss_appl_alarm_expression_t res;

        REQUIRE(VTSS_RC_ERROR == vtss_appl_alarm_conf_add(nullptr, &expression1));
        REQUIRE(VTSS_RC_ERROR == vtss_appl_alarm_conf_add(&leafname1, nullptr));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_add(&leafname1, &expression1));
        REQUIRE(VTSS_RC_ERROR ==
                vtss_appl_alarm_conf_add(&leafname1, &expression1));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_itr(nullptr, &itr));
        REQUIRE(strcmp(leafname1.alarm_name, itr.alarm_name) == 0);
        REQUIRE(VTSS_RC_ERROR == vtss_appl_alarm_conf_itr(&itr, &itr));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_get(&leafname1, &res));
        REQUIRE(strcmp(res.alarm_expression, expression1.alarm_expression) == 0);
        REQUIRE(VTSS_RC_ERROR_ALARM_ENTRY_NOT_FOUND ==
                vtss_appl_alarm_conf_get(&leafname2, &res));
        REQUIRE(VTSS_RC_ERROR_ALARM_ENTRY_NOT_FOUND ==
                vtss_appl_alarm_conf_del(&leafname2));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_del(&leafname1));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_add(&leafname1, &expression1));

        subject_main_thread.run(true);
        StringStream msg;
        msg << "{\"method\":\"alarm.config.get\",\"params\":[],\"id\":"
               "1}";
        REQUIRE(msg.ok());

        StringStream out;
        REQUIRE(vtss::json::Result::OK ==
                vtss::json::process_request(str(msg.cstring()), out,
                                            &JSON_RPC_ROOT));
        StringStream exp_out;
        exp_out << R"foo({"id":1,"error":null,"result":[{"key":{"alarmName":")foo"
                << leafname1.alarm_name << R"foo("},"val":{"expression":")foo"
                << expression1.alarm_expression << R"foo("}}]})foo";
        REQUIRE(str(out.cstring()) == str(exp_out.cstring()));
    }

    SECTION("Verify status of alarm") {
        vtss_appl_alarm_name_t itr;
        vtss_appl_alarm_status_t status;

        StringStream msg;
        msg << "{\"method\":\"alarm.status.get\",\"params\":[],\"id\":"
               "1}";
        REQUIRE(msg.ok());

        StringStream out;
        REQUIRE(vtss::json::Result::OK ==
                vtss::json::process_request(str(msg.cstring()), out,
                                            &JSON_RPC_ROOT));
        StringStream exp_out;
        exp_out << R"foo({"id":1,"error":null,"result":[{"key":{"alarmName":"alarm"},"val":{"suppressed":false,"Active":true}},{"key":{"alarmName":"alarm.node1"},"val":{"suppressed":false,"Active":true}},{"key":{"alarmName":"alarm.node1.leaf1"},"val":{"suppressed":false,"Active":true}}]})foo";

        REQUIRE(str(out.cstring()) == str(exp_out.cstring()));

        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_itr(nullptr, &itr));
        REQUIRE(strcmp(itr.alarm_name, "alarm") == 0);
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_itr(&itr, &itr));
        REQUIRE(strcmp(itr.alarm_name, "alarm.node1") == 0);
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_itr(&itr, &itr));
        REQUIRE(strcmp(itr.alarm_name, "alarm.node1.leaf1") == 0);
        REQUIRE_FALSE(VTSS_RC_OK == vtss_appl_alarm_status_itr(&itr, &itr));

        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_get(&leafname1, &status));
        subject_main_thread.run(true);
        CHECK(status.active);
        out.clear();
        REQUIRE(vtss::json::Result::OK ==
                vtss::json::process_request(str(msg.cstring()), out,
                                            &JSON_RPC_ROOT));
        exp_out.clear();

        exp_out << R"foo({"id":1,"error":null,"result":[{"key":{"alarmName":"alarm"},"val":{"suppressed":false,"Active":true}},{"key":{"alarmName":"alarm.node1"},"val":{"suppressed":false,"Active":true}},{"key":{"alarmName":"alarm.node1.leaf1"},"val":{"suppressed":false,"Active":true}}]})foo";

        REQUIRE(str(out.cstring()) == str(exp_out.cstring()));

        vtss_appl_alarm_suppression_t suppress;
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_suppress_get(&leafname1, &suppress));
        REQUIRE_FALSE(suppress.suppress);
        suppress.suppress = true;
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_suppress_set(&leafname1, &suppress));
        REQUIRE(suppress.suppress);
        suppress.suppress = false;
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_suppress_set(&leafname1, &suppress));
        REQUIRE_FALSE(suppress.suppress);
        strcpy(itr.alarm_name, "alarm.node1");
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_get(&itr, &status));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_suppress_get(&itr, &suppress));
        REQUIRE_FALSE(suppress.suppress);
        strcpy(itr.alarm_name, "alarm.node2");
        REQUIRE(VTSS_RC_ERROR_ALARM_ENTRY_NOT_FOUND ==
                vtss_appl_alarm_status_get(&itr, &status));
        REQUIRE(VTSS_RC_ERROR_ALARM_ENTRY_NOT_FOUND ==
                vtss_appl_alarm_suppress_get(&itr, &suppress));
        strcpy(itr.alarm_name, "alarm");
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_get(&itr, &status));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_suppress_get(&itr, &suppress));
        REQUIRE_FALSE(suppress.suppress);
    }

    vtss_appl_alarm_name_t leafname3 = {"alarm.node3.leaf3"};
    vtss_appl_alarm_expression_t expression3 = {"test.s1@e == test.s1@u32"};
    SECTION("invalid-alarm") {
        vtss_appl_alarm_name_t itr;
        vtss_appl_alarm_expression_t res;
        REQUIRE(VTSS_RC_ERROR ==
                vtss_appl_alarm_conf_add(&leafname3, &expression3));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_itr(nullptr, &itr));
        REQUIRE(VTSS_RC_ERROR == vtss_appl_alarm_conf_itr(&itr, &itr));
    }

    vtss_appl_alarm_name_t leafname4 = {"alarm.node4.leaf4"};
    vtss_appl_alarm_expression_t expression4 = {"test.s1@b1.a != test.s1@b2.a"};
    SECTION("two-alarms") {
        vtss_appl_alarm_name_t itr;
        vtss_appl_alarm_expression_t res;
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_add(&leafname4, &expression4));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_itr(nullptr, &itr));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_itr(&itr, &itr));
        REQUIRE(strcmp(leafname4.alarm_name, itr.alarm_name) == 0);
        REQUIRE_FALSE(VTSS_RC_OK == vtss_appl_alarm_conf_itr(&itr, &itr));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_get(&leafname4, &res));
        REQUIRE(strcmp(res.alarm_expression, expression4.alarm_expression) == 0);
        strcpy(itr.alarm_name, "alarm.node2");
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_itr(&itr, &itr));
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_get(&leafname4, &res));
    }

    SECTION("Verify-status-of-second-alarm") {
        vtss_appl_alarm_name_t itr1, itr2;
        vtss_appl_alarm_status_t status;
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_itr(nullptr, &itr1));
        REQUIRE(strcmp(itr1.alarm_name, "alarm") == 0);
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_itr(&itr1, &itr2));
        REQUIRE(strcmp(itr2.alarm_name, "alarm.node1") == 0);
        itr1 = itr2;
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_itr(&itr1, &itr2));
        REQUIRE(strcmp(itr2.alarm_name, "alarm.node1.leaf1") == 0);
        itr1 = itr2;
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_itr(&itr1, &itr2));
        REQUIRE(strcmp(itr2.alarm_name, "alarm.node4") == 0);
        itr1 = itr2;
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_itr(&itr1, &itr2));
        REQUIRE(strcmp(itr2.alarm_name, "alarm.node4.leaf4") == 0);
        itr1 = itr2;
        REQUIRE_FALSE(VTSS_RC_OK == vtss_appl_alarm_status_itr(&itr1, &itr2));


        subject_main_thread.run(true);
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_get(&leafname4, &status));
        //        CHECK(status.active);
        foo.b1 = {1, 1, 1};
        s_foo.set(foo);
        subject_main_thread.run(true);

        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_get(&leafname4, &status));
        CHECK_FALSE(status.active);

        strcpy(itr1.alarm_name, "alarm.node4");
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_get(&itr1, &status));
        strcpy(itr1.alarm_name, "alarm");
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_status_get(&itr1, &status));
    }

    SECTION("delete-alarm") {
        vtss_appl_alarm_name_t itr;
        vtss_appl_alarm_expression_t res;
        REQUIRE(VTSS_RC_OK == vtss_appl_alarm_conf_del(&leafname1));
    }
}
}  // namespace alarm
}  // namespace appl
}  // namespace vtss
