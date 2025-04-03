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
#include "catch.hpp"
#include "../alarm-leaf.hxx"
#include "../alarm-node.hxx"
#include "../alarm-expression/tree-element-var.hxx"

#include "vtss/basics/string.hxx"
#include "vtss/basics/json-rpc-server.hxx"
#include "vtss/basics/memcmp-operator.hxx"

#include "vtss/basics/expose/json.hxx"
#include "vtss/basics/expose/types.hxx"
#include "vtss/basics/expose/snmp/types.hxx"
#include "vtss/basics/expose/json/specification/walk.hxx"


#define TEST_TRUE_(X, Y)            \
    SECTION(#X ": " Y " -> TRUE") { \
        Expression e(Y, inv);       \
        REQUIRE(e.ok());            \
        REQUIRE(e.evaluate(X));     \
    }

#define TEST_FALSE(X, Y)              \
    SECTION(#X ": " Y " -> FALSE") {  \
        Expression e(Y, inv);         \
        REQUIRE(e.ok());              \
        REQUIRE_FALSE(e.evaluate(X)); \
    }

#define TYPE_CHECKS__(VAR, BINDING, EXPR, TYPE1, TYPE2) \
    auto VAR = TreeElementVar::create(EXPR);            \
    CHECK(VAR->check(inv, json_vars));                  \
    CHECK(VAR->name_of_type() == str(TYPE1));           \
    result_str << json_get(root, str(BINDING));         \
    bindings.set(str(BINDING), result_str.cstring());   \
    auto VAR##_e1 = VAR->eval(bindings);                \
    REQUIRE(VAR##_e1);                                  \
    REQUIRE(VAR##_e1->name_of_type() == str(TYPE2));

#define TYPE_CHECKS_(BINDING, EXPR, TYPE1, TYPE2) \
    { TYPE_CHECKS__(x, BINDING, EXPR, TYPE1, TYPE2) }

#define TYPE_CHECKS(VAR, BINDING, EXPR, TYPE) \
    TYPE_CHECKS__(VAR, BINDING, BINDING EXPR, TYPE, TYPE)

#define EVAL_VAR_TEST(BINDING, EXPR, TYPE, ANYTYPE, TESTEXPR) \
    {                                                         \
        result_str.clear();                                   \
        TYPE_CHECKS__(v, BINDING, EXPR, TYPE, TYPE);          \
        auto x = std::static_pointer_cast<ANYTYPE>(v_e1);     \
        CHECK(TESTEXPR);                                      \
    }



#define SYNTAX_ERROR(EXPR)                             \
    {                                                  \
        bool ok = true;                                \
        auto x = TreeElementVar::create(EXPR);         \
        if (!x) {                                      \
            ok = false;                                \
        } else {                                       \
            if (!x->check(inv, json_vars)) ok = false; \
        }                                              \
        CHECK_FALSE(ok);                               \
    }

namespace vtss {
static inline std::ostream &operator<<(std::ostream &o, vtss::str s) {
    o << std::string(s.begin(), s.end());
    return o;
}

namespace appl {
namespace alarm {
enum Key { A = 0, B = 3, C = 10 };
const vtss_enum_descriptor_t key_txt[] = {{A, "A"}, {B, "B"}, {C, "C"}, {}};
}  // namespace alarm
}  // namespace appl
}  // namespace vtss

VTSS_JSON_SERIALIZE_ENUM(vtss::appl::alarm::Key, "Key",
                         vtss::appl::alarm::key_txt, "-");

namespace vtss {
namespace appl {
namespace alarm {
namespace leaf_test {

using namespace vtss::appl::alarm::expression;
using namespace vtss::expose::json;
using namespace vtss::expose;

struct Bar {
    uint32_t a;
    uint32_t b;
    uint32_t c;
};

struct Foo {
    bool b;
    uint8_t u8;
    uint16_t u16;

    uint32_t u32_a;
    uint32_t u32_b;

    uint64_t u64;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    Key e;
    Key e_a;
    Key e_b;
    char s[128];

    Bar b1;
    Bar b2;
};

VTSS_BASICS_MEMCMP_OPERATOR(Bar);

template <typename HANDLER>
void serialize(HANDLER &h, Bar &b) {
    typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("Bar"));
    m.add_leaf(b.a, vtss::tag::Name("a"), vtss::tag::Description("a"));
    m.add_leaf(b.b, vtss::tag::Name("b"), vtss::tag::Description("b"));
    m.add_leaf(b.c, vtss::tag::Name("c"), vtss::tag::Description("c"));
}

VTSS_BASICS_MEMCMP_OPERATOR(Foo);

TableStatus<expose::ParamKey<int>, expose::ParamVal<Bar>> t1("t1", 0);
TableStatus<expose::ParamKey<int>, expose::ParamVal<int>, expose::ParamVal<int>> t2("t2", 0);
TableStatus<expose::ParamKey<int>, expose::ParamKey<int>, expose::ParamVal<int>> t3("t3", 0);
TableStatus<expose::ParamKey<int>, expose::ParamKey<int>, expose::ParamVal<int>,
            expose::ParamVal<int>> t4("t4", 0);

TableStatus<expose::ParamKey<int>> t5("t5", 0);
TableStatus<expose::ParamKey<Key>, expose::ParamVal<Bar>> t6("t6", 0);

StructStatus<expose::ParamVal<Foo *>> s1;
StructStatus<expose::ParamVal<int32_t *>, expose::ParamVal<uint32_t *>> s2;
StructStatus<expose::ParamVal<Key *>> s3;

StructStatus<expose::ParamVal<Bar>> b1;
StructStatus<expose::ParamVal<Bar>> b2;
StructStatus<expose::ParamVal<Bar>> b3;
StructStatus<expose::ParamVal<Bar>> b4;


// TODO, please add a table where the key is a enum

// TODO, please add a table where the value is a struct with which contains a
// bool, u8, i8, u16, i16, u32, i32, u64, i64, enum

struct T1 {
    typedef expose::ParamList<expose::ParamKey<int>, expose::ParamVal<Bar>> P;

    static constexpr const char *table_description = "T1 - table_description ";
    static constexpr const char *index_description = "T1 - index_description ";

    VTSS_EXPOSE_SERIALIZE_ARG_1(int &k) {
        h.add_leaf(k, vtss::tag::Name("k"), vtss::tag::Description("key"),
                   snmp::Status::Current, snmp::OidElementValue(1));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(Bar &v) { serialize(h, v); }
};

struct T2 {
    typedef expose::ParamList<expose::ParamKey<int>, expose::ParamVal<int>,
                              expose::ParamVal<int>> P;

    static constexpr const char *table_description = "T2 - table_description ";

    static constexpr const char *index_description = "T2 - index_description ";

    VTSS_EXPOSE_SERIALIZE_ARG_1(int &k) {
        h.add_leaf(k, vtss::tag::Name("k"), vtss::tag::Description("key"),
                   snmp::Status::Current, snmp::OidElementValue(3));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(int &v) {
        h.add_leaf(v, vtss::tag::Name("v1"), vtss::tag::Description("val1"),
                   snmp::Status::Current, snmp::OidElementValue(4));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(int &v) {
        h.add_leaf(v, vtss::tag::Name("v2"), vtss::tag::Description("val2"),
                   snmp::Status::Current, snmp::OidElementValue(5));
    }
};

struct T3 {
    typedef expose::ParamList<expose::ParamKey<int>, expose::ParamKey<int>,
                              expose::ParamVal<int>> P;

    static constexpr const char *table_description = "T3 - table_description ";

    static constexpr const char *index_description = "T3 - index_description ";

    VTSS_EXPOSE_SERIALIZE_ARG_1(int &k) {
        h.add_leaf(k, vtss::tag::Name("k1"), vtss::tag::Description("key1"),
                   snmp::Status::Current, snmp::OidElementValue(6));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(int &k) {
        h.add_leaf(k, vtss::tag::Name("k2"), vtss::tag::Description("key2"),
                   snmp::Status::Current, snmp::OidElementValue(7));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(int &v) {
        h.add_leaf(v, vtss::tag::Name("v"), vtss::tag::Description("val"),
                   snmp::Status::Current, snmp::OidElementValue(8));
    }
};

struct T4 {
    typedef expose::ParamList<expose::ParamKey<int>, expose::ParamKey<int>,
                              expose::ParamVal<int>, expose::ParamVal<int>> P;

    static constexpr const char *table_description = "T4 - table_description ";

    static constexpr const char *index_description = "T4 - index_description ";

    VTSS_EXPOSE_SERIALIZE_ARG_1(int &k) {
        h.add_leaf(k, vtss::tag::Name("k1"), vtss::tag::Description("key1"),
                   snmp::Status::Current, snmp::OidElementValue(9));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(int &k) {
        h.add_leaf(k, vtss::tag::Name("k2"), vtss::tag::Description("key2"),
                   snmp::Status::Current, snmp::OidElementValue(10));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(int &v) {
        h.add_leaf(v, vtss::tag::Name("v1"), vtss::tag::Description("val1"),
                   snmp::Status::Current, snmp::OidElementValue(11));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_4(int &v) {
        h.add_leaf(v, vtss::tag::Name("v2"), vtss::tag::Description("val2"),
                   snmp::Status::Current, snmp::OidElementValue(12));
    }
};

struct T5 {
    typedef expose::ParamList<expose::ParamKey<int>> P;

    static constexpr const char *table_description = "T5 - table_description ";

    static constexpr const char *index_description = "T5 - index_description ";

    VTSS_EXPOSE_SERIALIZE_ARG_1(int &k) {
        h.add_leaf(k, vtss::tag::Name("k"), vtss::tag::Description("key"),
                   snmp::Status::Current, snmp::OidElementValue(13));
    }
};

struct T6 {
    typedef expose::ParamList<expose::ParamKey<Key>, expose::ParamVal<Bar>> P;

    static constexpr const char *table_description = "T6 - table_description ";

    static constexpr const char *index_description = "T6 - index_description ";

    VTSS_EXPOSE_SERIALIZE_ARG_1(Key &k) {
        h.add_leaf(k, vtss::tag::Name("k"), vtss::tag::Description("key"),
                   snmp::Status::Current, snmp::OidElementValue(14));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(Bar &v) { serialize(h, v); }
};


struct S1 {
    typedef expose::ParamList<expose::ParamVal<Foo *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(Foo &f) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("Foo"));

        m.add_leaf(f.b, vtss::tag::Name("b"), vtss::tag::Description("b"));
        m.add_leaf(f.u8, vtss::tag::Name("u8"), vtss::tag::Description("u8"));
        m.add_leaf(f.u16, vtss::tag::Name("u16"),
                   vtss::tag::Description("u16"));
        m.add_leaf(f.u32_a, vtss::tag::Name("u32_a"),
                   vtss::tag::Description("u32_a"));
        m.add_leaf(f.u32_b, vtss::tag::Name("u32_b"),
                   vtss::tag::Description("u32_b"));
        m.add_leaf(f.u64, vtss::tag::Name("u64"),
                   vtss::tag::Description("u64"));
        m.add_leaf(f.i16, vtss::tag::Name("i16"),
                   vtss::tag::Description("i16"));
        m.add_leaf(f.i32, vtss::tag::Name("i32"),
                   vtss::tag::Description("i32"));
        m.add_leaf(f.i64, vtss::tag::Name("i64"),
                   vtss::tag::Description("i64"));
        m.add_leaf(f.e, vtss::tag::Name("e"), vtss::tag::Description("e"));
        m.add_leaf(f.e_a, vtss::tag::Name("e_a"),
                   vtss::tag::Description("e_a"));
        m.add_leaf(f.e_b, vtss::tag::Name("e_b"),
                   vtss::tag::Description("e_b"));
        m.add_leaf(vtss::AsDisplayString(f.s, sizeof(f.s)),
                   vtss::tag::Name("s"), vtss::tag::Description("s"));

        m.add_leaf(f.b1, vtss::tag::Name("b1"), vtss::tag::Description("b1"));
        m.add_leaf(f.b2, vtss::tag::Name("b2"), vtss::tag::Description("b2"));
    }
};

struct BAR {
    typedef expose::ParamList<expose::ParamVal<Bar>> P;
    VTSS_EXPOSE_SERIALIZE_ARG_1(Bar &b) { serialize(h, b); }
};

TEST_CASE("Verify setup", "[alarm]") {
    expose::json::RootNode root;
    expose::json::TableReadOnlyNotification<T1> ro1(&root, "ro1", &t1);
    Bar b1 = { 1, 1, 1};
    Bar b2 = { 2, 2, 2};
    t1.set(1, b1);
    t1.set(2, b2);

    BufStream<SBuf512> out;
    str msg1_str("{\"method\":\"ro1.get\",\"params\":[1],\"id\":3}");
    REQUIRE(vtss::json::Result::OK ==
            vtss::json::process_request(msg1_str, out, &root));
    std::string expres1("{\"id\":3,\"error\":null,\"result\":{\"a\":1,\"b\":1,\"c\":1}}");
    REQUIRE(strcmp(expres1.c_str(), out.cstring()) == 0);

    out.clear();
    str msg2_str("{\"method\":\"ro1.get\",\"params\":[2],\"id\":3}");
    REQUIRE(vtss::json::Result::OK ==
            vtss::json::process_request(msg2_str, out, &root));
    std::string expres2("{\"id\":3,\"error\":null,\"result\":{\"a\":2,\"b\":2,\"c\":2}}");
    REQUIRE(strcmp(expres2.c_str(), out.cstring()) == 0);

    out.clear();
    str msg3_str("{\"method\":\"ro1.get\",\"params\":[3],\"id\":3}");
    REQUIRE(vtss::json::Result::INTERNAL_ERROR ==
            vtss::json::process_request(msg3_str, out, &root));

    t1.clear();
}

/*
TEST_CASE("alarm-leaf-table", "[alarm]") {
    vtss::notifications::SubjectRunner main_thread("main", 0, true);

    expose::json::RootNode root;
    expose::json::TableReadOnlyNotification<T1> ro1(&root, "ro1", &t1);
    REQUIRE(root.lookup(str("ro1")) == &ro1);
    REQUIRE(root.lookup(str("ro1"))->is_namespace_node());
    REQUIRE(root.lookup(str("ro1.update")) != nullptr);
    REQUIRE(root.lookup(str("ro1.update"))->is_notification());
    REQUIRE(root.lookup(str("ro1.get")) != nullptr);


    t1.set(1, 1);
    t1.set(2, 2);

    vtss::expose::json::specification::Inventory inv;

    for (auto &i : root.leafs) walk(inv, &i, false);

    // StringStream output;
    // output << inv;
    // printf("%s\n", output.cstring());

    Leaf first_leaf(&main_thread, "first", "ro1[1] == 1", inv, root);

    // Verify initial state
    REQUIRE(first_leaf.bindings[str("ro1")] ==
            std::string(
                    R"foo({"id":1,"error":null,"result":[{"key":1,"val":1},{"key":2,"val":2}]})foo"));

    // Make a change and run all pending events
    t1.set(1, 7);
    main_thread.run(true);

    // Verify new state
    REQUIRE(first_leaf.bindings[str("ro1")] ==
            std::string(
                    R"foo({"id":1,"error":null,"result":[{"key":1,"val":7},{"key":2,"val":2}]})foo"));

    t1.clear();
}
*/
/*

TEST_CASE("alarm-node", "[alarm]") {
    vtss::notifications::SubjectRunner main_thread("main", 0, true);

    expose::json::RootNode root;
    expose::json::TableReadOnlyNotification<T1> ro1(&root, "ro1", &t1);
    t1.set(1, 1);
    t1.set(2, 2);

    vtss::expose::json::specification::Inventory inv;

    for (auto &i : root.leafs) walk(inv, &i, false);

    Node alarm_root_node(&main_thread, "alarm");
    alarm_root_node.make_alarm(&main_thread, "alarm.node.leaf",
                               "ro1[1]==7&&ro1[2]==2", inv, root);

    Element *first_leaf = alarm_root_node.lookup("alarm.node.leaf");
    REQUIRE(first_leaf != nullptr);
    REQUIRE(first_leaf->name() == "leaf");
    Element *first_node = alarm_root_node.lookup("alarm.node");
    REQUIRE(first_node != nullptr);
    REQUIRE(first_node->name() == "node");

    REQUIRE_FALSE(first_leaf->get());
    REQUIRE_FALSE(first_node->get());

    t1.set(1, 7);
    main_thread.run(true);
    REQUIRE(first_node->get());

    t1.set(1, 1);
    main_thread.run(true);
    REQUIRE_FALSE(first_node->get());

    t1.set(1, 7);
    main_thread.run(true);
    REQUIRE(first_node->get());

    REQUIRE(VTSS_RC_OK == alarm_root_node.suppress("alarm.node.leaf", true));
    main_thread.run(true);
    REQUIRE_FALSE(first_node->get());

    t1.set(1, 1);
    main_thread.run(true);
    REQUIRE_FALSE(first_node->get());

    t1.set(1, 7);
    main_thread.run(true);
    REQUIRE(first_node->get());
}

TEST_CASE("alarm-node T2a", "[alarm]") {
    vtss::notifications::SubjectRunner main_thread("main", 0, true);

    expose::json::RootNode root;
    expose::json::TableReadOnlyNotification<T2> ro2(&root, "ro2", &t2);

    t2.set(1, 1, 1);
    t2.set(2, 2, 1);

    vtss::expose::json::specification::Inventory inv;

    // StringStream output;
    // output << inv;
    // printf("%s\n", output.cstring());

    for (auto &i : root.leafs) walk(inv, &i, false);

    Node alarm_root_node(&main_thread, "alarm");
    alarm_root_node.make_alarm(
            &main_thread, "alarm.node.leaf",
            "ro2[1,1]==1 && ro2[1,2]==4 && ro2[2,1]==1 && ro2[2,2]==7", inv,
            root);

    Element *first_leaf = alarm_root_node.lookup("alarm.node.leaf");
    REQUIRE(first_leaf != nullptr);
    REQUIRE(first_leaf->name() == "leaf");
    Element *first_node = alarm_root_node.lookup("alarm.node");
    REQUIRE(first_node != nullptr);
    REQUIRE(first_node->name() == "node");

    REQUIRE_FALSE(first_leaf->get());
    REQUIRE_FALSE(first_node->get());

    t2.set(1, 1, 4);
    t2.set(2, 1, 7);

    main_thread.run(true);
    REQUIRE(first_node->get());
}

TEST_CASE("alarm-node T2b", "[alarm]") {
    vtss::notifications::SubjectRunner main_thread("main", 0, true);

    expose::json::RootNode root;
    expose::json::TableReadOnlyNotification<T2> ro2(&root, "ro2", &t2);

    t2.set(1, 1, 1);
    t2.set(2, 2, 1);

    vtss::expose::json::specification::Inventory inv;

    // StringStream output;
    // output << inv;
    // printf("%s\n", output.cstring());

    for (auto &i : root.leafs) walk(inv, &i, false);

    Node alarm_root_node(&main_thread, "alarm");
    alarm_root_node.make_alarm(
            &main_thread, "alarm.node.leaf",
            "ro2[1][1]==1 && ro2[1][2]==4 && ro2[2][1]==1 && ro2[2][2]==7", inv,
            root);

    Element *first_leaf = alarm_root_node.lookup("alarm.node.leaf");
    REQUIRE(first_leaf != nullptr);
    REQUIRE(first_leaf->name() == "leaf");
    Element *first_node = alarm_root_node.lookup("alarm.node");
    REQUIRE(first_node != nullptr);
    REQUIRE(first_node->name() == "node");

    REQUIRE_FALSE(first_leaf->get());
    REQUIRE_FALSE(first_node->get());

    t2.set(1, 1, 4);
    t2.set(2, 1, 7);

    main_thread.run(true);
    REQUIRE(first_node->get());
}

TEST_CASE("alarm-node T3", "[alarm]") {
    vtss::notifications::SubjectRunner main_thread("main", 0, true);

    expose::json::RootNode root;
    expose::json::TableReadOnlyNotification<T3> ro3(&root, "ro3", &t3);

    t3.set(1, 1, 1);
    t3.set(1, 2, 1);
    t3.set(2, 1, 1);
    t3.set(2, 2, 1);

    vtss::expose::json::specification::Inventory inv;


    for (auto &i : root.leafs) walk(inv, &i, false);

    StringStream output;
    output << inv;
    printf("%s\n", output.cstring());

    Node alarm_root_node(&main_thread, "alarm");
    alarm_root_node.make_alarm(
            &main_thread, "alarm.node.leaf",
            "ro3[1,1]==3 && ro3[1,2]==4 && ro3[2,1]==1 && ro3[2,2]==7", inv,
            root);
    Element *first_leaf = alarm_root_node.lookup("alarm.node.leaf");
    REQUIRE(first_leaf != nullptr);
    REQUIRE(first_leaf->name() == "leaf");
    Element *first_node = alarm_root_node.lookup("alarm.node");
    REQUIRE(first_node != nullptr);
    REQUIRE(first_node->name() == "node");

    REQUIRE_FALSE(first_leaf->get());
    REQUIRE_FALSE(first_node->get());

    t3.set(1, 1, 3);
    t3.set(1, 2, 4);
    t3.set(2, 1, 1);
    t3.set(2, 2, 7);

    main_thread.run(true);
    REQUIRE(first_node->get());
}
*/

std::string json_get(expose::json::RootNode &r, str s) {
    StringStream msg;
    msg << "{\"method\":\"" << s << ".get\",\"params\":[],\"id\":1}";

    StringStream out;
    vtss::json::process_request(str(msg.cstring()), out, &r);

    return out.buf;
}

TEST_CASE("TreeElementVar", "[alarm]") {
#define NS(N, P, D) vtss::expose::json::NamespaceNode N(&P, D);
    vtss::notifications::SubjectRunner main_thread("main", 0, true);

    expose::json::RootNode root;
    NS(ns_test, root, "test");

    expose::json::StructReadOnlyNotification<S1> s1_(&ns_test, "s1", &s1);

    NS(ns_status, ns_test, "status");
    NS(ns_status_g1, ns_status, "g1");
    NS(ns_status_g2, ns_status, "g2");

    expose::json::StructReadOnlyNotification<BAR> b1_(&ns_status, "b1", &b1);
    expose::json::StructReadOnlyNotification<BAR> b2_(&ns_status_g1, "b2", &b2);
    expose::json::StructReadOnlyNotification<BAR> b3_(&ns_status_g2, "b3", &b3);
    expose::json::StructReadOnlyNotificationNoNS<BAR> b4_(&ns_status_g1, &b4);

    expose::json::TableReadOnlyNotification<T1> t1_(&ns_status, "t1", &t1);
    expose::json::TableReadOnlyNotification<T2> t2_(&ns_status, "t2", &t2);
    expose::json::TableReadOnlyNotification<T3> t3_(&ns_status, "t3", &t3);
    expose::json::TableReadOnlyNotification<T4> t4_(&ns_status, "t4", &t4);
    expose::json::TableReadOnlyNotification<T5> t5_(&ns_status, "t5", &t5);
    expose::json::TableReadOnlyNotification<T6> t6_(&ns_status, "t6", &t6);

    vtss::expose::json::specification::Inventory inv;

    Set<std::string> json_vars;

    Foo f{};
    s1.set(f);

    Map<str, std::string> bindings;
    StringStream result_str;

    for (auto &i : root.leafs) walk(inv, &i, false);
    SECTION("DUMP") {
        StringStream output;
        output << inv;
        // printf("%s\n", output.cstring());
    }

    SECTION("1") {
        TYPE_CHECKS(v1, "test.s1", "@b", "vtss_bool_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":0,\"u16\":0,\"u32_a\":0,\"u64\":0,"
                            "\"i16\":0,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":0,\"b\":0,\"c\":0},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v1_e2 = v1->eval(bindings);
        REQUIRE(v1_e2->name_of_type() == str("vtss_bool_t"));
        auto v1_e2_ = std::static_pointer_cast<AnyBool>(v1_e2);
        CHECK(v1_e2_->value);
    }

    SECTION("1.1") {
        auto v1 = TreeElementVar::create("test.s1@b");
        CHECK(v1->check(inv, json_vars));
        CHECK(v1->name_of_type() == str("vtss_bool_t"));
        result_str << json_get(root, str("test.s1"));
        bindings.set(str("test.s1"), result_str.cstring());
        auto v1_e1 = v1->eval(bindings);
        REQUIRE(v1_e1);
        REQUIRE(v1_e1->name_of_type() == str("vtss_bool_t"));
        auto v1_e1_ = std::static_pointer_cast<AnyBool>(v1_e1);
        CHECK_FALSE(v1_e1_->value);

        bindings.set(
                str("test.s1"),
                std::string(
                        "{\"id\":1,\"error\":null,\"result\":{\"b\":true}}"));

        auto v1_e2 = v1->eval(bindings);
        REQUIRE(v1_e2->name_of_type() == str("vtss_bool_t"));
        auto v1_e2_ = std::static_pointer_cast<AnyBool>(v1_e2);
        CHECK(v1_e2_->value);
    }

    SECTION("2") {
        TYPE_CHECKS(v2, "test.s1", "@u8", "uint8_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":0,\"u32_a\":0,\"u64\":0,"
                            "\"i16\":0,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":0,\"b\":0,\"c\":0},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v2_e2 = v2->eval(bindings);
        REQUIRE(v2_e2->name_of_type() == str("uint8_t"));
        auto v2_e2_ = std::static_pointer_cast<AnyUint8>(v2_e2);
        CHECK(v2_e2_->value == 1);
    }

    SECTION("3") {
        TYPE_CHECKS(v3, "test.s1", "@u16", "uint16_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":0,\"u64\":0,"
                            "\"i16\":0,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":0,\"b\":0,\"c\":0},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v3_e2 = v3->eval(bindings);
        REQUIRE(v3_e2->name_of_type() == str("uint16_t"));
        auto v3_e2_ = std::static_pointer_cast<AnyUint16>(v3_e2);
        CHECK(v3_e2_->value == 3);
    }

    SECTION("4") {
        TYPE_CHECKS(v4, "test.s1", "@u32_a", "uint32_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":0,"
                            "\"i16\":0,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":0,\"b\":0,\"c\":0},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v4_e2 = v4->eval(bindings);
        REQUIRE(v4_e2->name_of_type() == str("uint32_t"));
        auto v4_e2_ = std::static_pointer_cast<AnyUint32>(v4_e2);
        CHECK(v4_e2_->value == 4);
    }

    SECTION("5") {
        TYPE_CHECKS(v5, "test.s1", "@u64", "uint64_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":5,"
                            "\"i16\":0,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":0,\"b\":0,\"c\":0},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v5_e2 = v5->eval(bindings);
        REQUIRE(v5_e2->name_of_type() == str("uint64_t"));
        auto v5_e2_ = std::static_pointer_cast<AnyUint64>(v5_e2);
        CHECK(v5_e2_->value == 5);
    }

    SECTION("6") {
        TYPE_CHECKS(v6, "test.s1", "@i16", "int16_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":5,"
                            "\"i16\":6,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":0,\"b\":0,\"c\":0},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v6_e2 = v6->eval(bindings);
        REQUIRE(v6_e2->name_of_type() == str("int16_t"));
        auto v6_e2_ = std::static_pointer_cast<AnyInt16>(v6_e2);
        CHECK(v6_e2_->value == 6);
    }

    SECTION("7") {
        TYPE_CHECKS(v7, "test.s1", "@i32", "int32_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":5,"
                            "\"i16\":6,\"i32\":7,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":0,\"b\":0,\"c\":0},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v7_e2 = v7->eval(bindings);
        REQUIRE(v7_e2->name_of_type() == str("int32_t"));
        auto v7_e2_ = std::static_pointer_cast<AnyInt32>(v7_e2);
        CHECK(v7_e2_->value == 7);
    }

    SECTION("8") {
        TYPE_CHECKS(v8, "test.s1", "@i64", "int64_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":5,"
                            "\"i16\":6,\"i32\":7,\"i64\":8,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":0,\"b\":0,\"c\":0},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v8_e2 = v8->eval(bindings);
        REQUIRE(v8_e2->name_of_type() == str("int64_t"));
        auto v8_e2_ = std::static_pointer_cast<AnyInt64>(v8_e2);
        CHECK(v8_e2_->value == 8);
    }

    SECTION("9") {
        TYPE_CHECKS(v9, "test.s1", "@e", "int32_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":5,"
                            "\"i16\":6,\"i32\":7,\"i64\":8,\"e\":\"B\","
                            "\"s\":\"\",\"b1\":{\"a\":0,\"b\":0,\"c\":0},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v9_e2 = v9->eval(bindings);
        REQUIRE(v9_e2);
        REQUIRE(v9_e2->name_of_type() == str("int32_t"));
        auto v9_e2_ = std::static_pointer_cast<AnyInt32>(v9_e2);
        CHECK(v9_e2_->value == 3);
    }

    SECTION("10") {
        TYPE_CHECKS(v10, "test.s1", "@s", "string");

        bindings.set(
                str("test.s1"),
                std::string(
                        "{\"id\":1,\"error\":null,\"result\":{\"b\":"
                        "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":5,"
                        "\"i16\":6,\"i32\":7,\"i64\":8,\"e\":\"hello9\","
                        "\"s\":\"hello10\",\"b1\":{\"a\":0,\"b\":0,\"c\":0},"
                        "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v10_e2 = v10->eval(bindings);
        REQUIRE(v10_e2->name_of_type() == str("string"));
        auto v10_e2_ = std::static_pointer_cast<AnyStr>(v10_e2);
        CHECK(str(v10_e2_->value) == str("hello10"));
    }

    SECTION("11") {
        TYPE_CHECKS(v11, "test.s1", "@b1.a", "uint32_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":0,"
                            "\"i16\":0,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":11,\"b\":0,\"c\":0},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v11_e2 = v11->eval(bindings);
        REQUIRE(v11_e2->name_of_type() == str("uint32_t"));
        auto v11_e2_ = std::static_pointer_cast<AnyUint32>(v11_e2);
        CHECK(v11_e2_->value == 11);
    }

    SECTION("12") {
        TYPE_CHECKS(v12, "test.s1", "@b1.b", "uint32_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":0,"
                            "\"i16\":0,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":11,\"b\":12,\"c\":0},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v12_e2 = v12->eval(bindings);
        REQUIRE(v12_e2->name_of_type() == str("uint32_t"));
        auto v12_e2_ = std::static_pointer_cast<AnyUint32>(v12_e2);
        CHECK(v12_e2_->value == 12);
    }

    SECTION("13") {
        TYPE_CHECKS(v13, "test.s1", "@b1.c", "uint32_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":0,"
                            "\"i16\":0,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":11,\"b\":12,\"c\":13},"
                            "\"b2\":{\"a\":0,\"b\":0,\"c\":0}}}"));
        auto v13_e2 = v13->eval(bindings);
        REQUIRE(v13_e2->name_of_type() == str("uint32_t"));
        auto v13_e2_ = std::static_pointer_cast<AnyUint32>(v13_e2);
        CHECK(v13_e2_->value == 13);
    }

    SECTION("14") {
        TYPE_CHECKS(v14, "test.s1", "@b2.a", "uint32_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":0,"
                            "\"i16\":0,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":11,\"b\":12,\"c\":13},"
                            "\"b2\":{\"a\":14,\"b\":0,\"c\":0}}}"));
        auto v14_e2 = v14->eval(bindings);
        REQUIRE(v14_e2->name_of_type() == str("uint32_t"));
        auto v14_e2_ = std::static_pointer_cast<AnyUint32>(v14_e2);
        CHECK(v14_e2_->value == 14);
    }

    SECTION("15") {
        TYPE_CHECKS(v15, "test.s1", "@b2.b", "uint32_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":0,"
                            "\"i16\":0,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":11,\"b\":12,\"c\":13},"
                            "\"b2\":{\"a\":14,\"b\":15,\"c\":0}}}"));
        auto v15_e2 = v15->eval(bindings);
        REQUIRE(v15_e2->name_of_type() == str("uint32_t"));
        auto v15_e2_ = std::static_pointer_cast<AnyUint32>(v15_e2);
        CHECK(v15_e2_->value == 15);
    }

    SECTION("16") {
        TYPE_CHECKS(v16, "test.s1", "@b2.c", "uint32_t");

        bindings.set(
                str("test.s1"),
                std::string("{\"id\":1,\"error\":null,\"result\":{\"b\":"
                            "true,\"u8\":1,\"u16\":3,\"u32_a\":4,\"u64\":0,"
                            "\"i16\":0,\"i32\":0,\"i64\":0,\"e\":\"A\","
                            "\"s\":\"\",\"b1\":{\"a\":11,\"b\":12,\"c\":13},"
                            "\"b2\":{\"a\":14,\"b\":15,\"c\":16}}}"));
        auto v16_e2 = v16->eval(bindings);
        REQUIRE(v16_e2->name_of_type() == str("uint32_t"));
        auto v16_e2_ = std::static_pointer_cast<AnyUint32>(v16_e2);
        CHECK(v16_e2_->value == 16);
    }

    SECTION("17") {
        TYPE_CHECKS(v17, "test.status.g1.b2", "@a", "uint32_t");

        bindings.set(str("test.status.g1.b2"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "17,\"b\":0,\"c\":0}}"));
        auto v17_e2 = v17->eval(bindings);
        REQUIRE(v17_e2->name_of_type() == str("uint32_t"));
        auto v17_e2_ = std::static_pointer_cast<AnyUint32>(v17_e2);
        CHECK(v17_e2_->value == 17);
    }

    SECTION("18") {
        TYPE_CHECKS(v18, "test.status.g1.b2", "@b", "uint32_t");

        bindings.set(str("test.status.g1.b2"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "17,\"b\":18,\"c\":0}}"));
        auto v18_e2 = v18->eval(bindings);
        REQUIRE(v18_e2->name_of_type() == str("uint32_t"));
        auto v18_e2_ = std::static_pointer_cast<AnyUint32>(v18_e2);
        CHECK(v18_e2_->value == 18);
    }

    SECTION("19") {
        TYPE_CHECKS(v19, "test.status.g1.b2", "@c", "uint32_t");

        bindings.set(str("test.status.g1.b2"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "17,\"b\":18,\"c\":19}}"));
        auto v19_e2 = v19->eval(bindings);
        REQUIRE(v19_e2->name_of_type() == str("uint32_t"));
        auto v19_e2_ = std::static_pointer_cast<AnyUint32>(v19_e2);
        CHECK(v19_e2_->value == 19);
    }

    SECTION("20") {
        TYPE_CHECKS(v20, "test.status.g1", "@a", "uint32_t");

        bindings.set(str("test.status.g1"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "20,\"b\":0,\"c\":0}}"));
        auto v20_e2 = v20->eval(bindings);
        REQUIRE(v20_e2->name_of_type() == str("uint32_t"));
        auto v20_e2_ = std::static_pointer_cast<AnyUint32>(v20_e2);
        CHECK(v20_e2_->value == 20);
    }

    SECTION("21") {
        TYPE_CHECKS(v21, "test.status.g1", "@b", "uint32_t");

        bindings.set(str("test.status.g1"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "20,\"b\":21,\"c\":0}}"));
        auto v21_e2 = v21->eval(bindings);
        REQUIRE(v21_e2->name_of_type() == str("uint32_t"));
        auto v21_e2_ = std::static_pointer_cast<AnyUint32>(v21_e2);
        CHECK(v21_e2_->value == 21);
    }

    SECTION("22") {
        TYPE_CHECKS(v22, "test.status.g1", "@c", "uint32_t");

        bindings.set(str("test.status.g1"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "20,\"b\":21,\"c\":22}}"));
        auto v22_e2 = v22->eval(bindings);
        REQUIRE(v22_e2->name_of_type() == str("uint32_t"));
        auto v22_e2_ = std::static_pointer_cast<AnyUint32>(v22_e2);
        CHECK(v22_e2_->value == 22);
    }

    SECTION("23") {
        TYPE_CHECKS(v23, "test.status.g2.b3", "@a", "uint32_t");

        bindings.set(str("test.status.g2.b3"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "23,\"b\":0,\"c\":0}}"));
        auto v23_e2 = v23->eval(bindings);
        REQUIRE(v23_e2->name_of_type() == str("uint32_t"));
        auto v23_e2_ = std::static_pointer_cast<AnyUint32>(v23_e2);
        CHECK(v23_e2_->value == 23);
    }

    SECTION("24") {
        TYPE_CHECKS(v24, "test.status.g2.b3", "@b", "uint32_t");

        bindings.set(str("test.status.g2.b3"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "23,\"b\":24,\"c\":0}}"));
        auto v24_e2 = v24->eval(bindings);
        REQUIRE(v24_e2->name_of_type() == str("uint32_t"));
        auto v24_e2_ = std::static_pointer_cast<AnyUint32>(v24_e2);
        CHECK(v24_e2_->value == 24);
    }

    SECTION("25") {
        TYPE_CHECKS(v25, "test.status.g2.b3", "@c", "uint32_t");

        bindings.set(str("test.status.g2.b3"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "23,\"b\":24,\"c\":25}}"));
        auto v25_e2 = v25->eval(bindings);
        REQUIRE(v25_e2->name_of_type() == str("uint32_t"));
        auto v25_e2_ = std::static_pointer_cast<AnyUint32>(v25_e2);
        CHECK(v25_e2_->value == 25);
    }

    SECTION("26") {
        TYPE_CHECKS(v26, "test.status.b1", "@a", "uint32_t");

        bindings.set(str("test.status.b1"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "26,\"b\":0,\"c\":0}}"));
        auto v26_e2 = v26->eval(bindings);
        REQUIRE(v26_e2->name_of_type() == str("uint32_t"));
        auto v26_e2_ = std::static_pointer_cast<AnyUint32>(v26_e2);
        CHECK(v26_e2_->value == 26);
    }

    SECTION("27") {
        TYPE_CHECKS(v27, "test.status.b1", "@b", "uint32_t");

        bindings.set(str("test.status.b1"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "26,\"b\":27,\"c\":0}}"));
        auto v27_e2 = v27->eval(bindings);
        REQUIRE(v27_e2->name_of_type() == str("uint32_t"));
        auto v27_e2_ = std::static_pointer_cast<AnyUint32>(v27_e2);
        CHECK(v27_e2_->value == 27);
    }

    SECTION("28") {
        TYPE_CHECKS(v28, "test.status.b1", "@c", "uint32_t");

        bindings.set(str("test.status.b1"),
                     std::string("{\"id\":1,\"error\":null,\"result\":{\"a\":"
                                 "26,\"b\":27,\"c\":28}}"));
        auto v28_e2 = v28->eval(bindings);
        REQUIRE(v28_e2->name_of_type() == str("uint32_t"));
        auto v28_e2_ = std::static_pointer_cast<AnyUint32>(v28_e2);
        CHECK(v28_e2_->value == 28);
    }

    SECTION("Table index tests") {
        t1.set(1, {1, 2, 3});
        t1.set(10, {10, 20, 30});
        t6.set(A, {1, 2, 3});
        t6.set(B, {10, 20, 30});

        SYNTAX_ERROR("test.status.t1@a");
        SYNTAX_ERROR("test[1].status[1].t1@a");
        SYNTAX_ERROR("test.status.t1[1, 2]@a");
        SYNTAX_ERROR("test.status.t1[1][2]@a");
        SYNTAX_ERROR("test.status.t1[1@a");
        SYNTAX_ERROR("test.status.t1[[1]]@a");
        SYNTAX_ERROR("test.status.t1[{}]@a");
        SYNTAX_ERROR("test.status.t1[]@a");

        SYNTAX_ERROR("test.status.t6@a");
        SYNTAX_ERROR("test[1].status[1].t6@a");
        SYNTAX_ERROR("test.status.t6[1, 2]@a");
        SYNTAX_ERROR("test.status.t6[1][2]@a");
        SYNTAX_ERROR("test.status.t6[1@a");
        SYNTAX_ERROR("test.status.t6[[1]]@a");
        SYNTAX_ERROR("test.status.t6[{}]@a");
        SYNTAX_ERROR("test.status.t6[]@a");

        EVAL_VAR_TEST("test.status.t1", "test[1].status.t1@a", "uint32_t",
                      AnyUint32, x->value == 1);
        EVAL_VAR_TEST("test.status.t1", "test.status[1].t1@a", "uint32_t",
                      AnyUint32, x->value == 1);
        EVAL_VAR_TEST("test.status.t1", "test.status.t1[1]@a", "uint32_t",
                      AnyUint32, x->value == 1);
        EVAL_VAR_TEST("test.status.t1", "test.status.t1[1]@b", "uint32_t",
                      AnyUint32, x->value == 2);
        EVAL_VAR_TEST("test.status.t1", "test.status.t1[1]@c", "uint32_t",
                      AnyUint32, x->value == 3);

        EVAL_VAR_TEST("test.status.t1", "test.status.t1[10]@a", "uint32_t",
                      AnyUint32, x->value == 10);
        EVAL_VAR_TEST("test.status.t1", "test.status.t1[10]@b", "uint32_t",
                      AnyUint32, x->value == 20);
        EVAL_VAR_TEST("test.status.t1", "test.status.t1[10]@c", "uint32_t",
                      AnyUint32, x->value == 30);

        EVAL_VAR_TEST("test.status.t6", "test[\"A\"].status.t6@a", "uint32_t",
                      AnyUint32, x->value == 1);
        EVAL_VAR_TEST("test.status.t6", "test.status[\"A\"].t6@a", "uint32_t",
                      AnyUint32, x->value == 1);
        EVAL_VAR_TEST("test.status.t6", "test.status.t6[\"A\"]@a", "uint32_t",
                      AnyUint32, x->value == 1);
        EVAL_VAR_TEST("test.status.t6", "test.status.t6[\"A\"]@b", "uint32_t",
                      AnyUint32, x->value == 2);
        EVAL_VAR_TEST("test.status.t6", "test.status.t6[\"A\"]@c", "uint32_t",
                      AnyUint32, x->value == 3);

        EVAL_VAR_TEST("test.status.t6", "test.status.t6[\"B\"]@a", "uint32_t",
                      AnyUint32, x->value == 10);
        EVAL_VAR_TEST("test.status.t6", "test.status.t6[\"B\"]@b", "uint32_t",
                      AnyUint32, x->value == 20);
        EVAL_VAR_TEST("test.status.t6", "test.status.t6[\"B\"]@c", "uint32_t",
                      AnyUint32, x->value == 30);

        result_str.clear();
        TYPE_CHECKS_("test.status.t1", "test.status.t1[11]@c", "uint32_t",
                     "nullptr_t");
    }
}

TEST_CASE("Bool expression tests", "[alarm]") {
#define NS(N, P, D) vtss::expose::json::NamespaceNode N(&P, D);
    vtss::notifications::SubjectRunner main_thread("main", 0, true);

    expose::json::RootNode root;
    NS(ns_test, root, "test");

    expose::json::StructReadOnlyNotification<S1> s1_(&ns_test, "s1", &s1);

    NS(ns_status, ns_test, "status");
    NS(ns_status_g1, ns_status, "g1");
    NS(ns_status_g2, ns_status, "g2");

    expose::json::StructReadOnlyNotification<BAR> b1_(&ns_status, "b1", &b1);
    expose::json::StructReadOnlyNotification<BAR> b2_(&ns_status_g1, "b2", &b2);
    expose::json::StructReadOnlyNotification<BAR> b3_(&ns_status_g2, "b3", &b3);
    expose::json::StructReadOnlyNotificationNoNS<BAR> b4_(&ns_status_g1, &b4);

    vtss::expose::json::specification::Inventory inv;
    for (auto &i : root.leafs) walk(inv, &i, false);

    Map<str, std::string> b1;
    b1.set(str("test.s1"), "{\"id\":1,\"error\":null,\"result\":{\"b\":true}}");

    Map<str, std::string> b2;
    b2.set(str("test.s1"),
           "{\"id\":1,\"error\":null,\"result\":{\"b\":false}}");

    TEST_TRUE_(b1, "true");
    TEST_FALSE(b1, "false");
    TEST_TRUE_(b1, "!false");
    TEST_FALSE(b1, "!true");

    TEST_TRUE_(b1, "test.s1@b");
    TEST_FALSE(b2, "test.s1@b");

    TEST_FALSE(b1, "!test.s1@b");
    TEST_TRUE_(b2, "!test.s1@b");

    TEST_TRUE_(b1, "test.s1@b == test.s1@b");
    TEST_TRUE_(b2, "test.s1@b == test.s1@b");
    TEST_TRUE_(b1, "test.s1@b == true");
    TEST_TRUE_(b1, "true == test.s1@b");
    TEST_FALSE(b2, "test.s1@b == true");
    TEST_FALSE(b2, "true == test.s1@b");
    TEST_FALSE(b1, "test.s1@b == false");
    TEST_FALSE(b1, "false == test.s1@b");
    TEST_TRUE_(b2, "test.s1@b == false");
    TEST_TRUE_(b2, "false == test.s1@b");

    TEST_FALSE(b1, "test.s1@b != test.s1@b");
    TEST_FALSE(b2, "test.s1@b != test.s1@b");
    TEST_FALSE(b1, "test.s1@b != true");
    TEST_FALSE(b1, "true != test.s1@b");
    TEST_TRUE_(b2, "test.s1@b != true");
    TEST_TRUE_(b2, "true != test.s1@b");
    TEST_TRUE_(b1, "test.s1@b != false");
    TEST_TRUE_(b1, "false != test.s1@b");
    TEST_FALSE(b2, "test.s1@b != false");
    TEST_FALSE(b2, "false != test.s1@b");

    TEST_FALSE(b1, "test.s1@b && false");
    TEST_FALSE(b1, "false && test.s1@b");
    TEST_FALSE(b2, "test.s1@b && false");
    TEST_FALSE(b2, "false && test.s1@b");

    TEST_TRUE_(b1, "test.s1@b && true");
    TEST_TRUE_(b1, "true && test.s1@b");
    TEST_FALSE(b2, "test.s1@b && true");
    TEST_FALSE(b2, "true && test.s1@b");

    TEST_TRUE_(b1, "test.s1@b || false");
    TEST_TRUE_(b1, "false || test.s1@b");
    TEST_FALSE(b2, "test.s1@b || false");
    TEST_FALSE(b2, "false || test.s1@b");

    TEST_TRUE_(b1, "test.s1@b || true");
    TEST_TRUE_(b1, "true || test.s1@b");
    TEST_TRUE_(b2, "test.s1@b || true");
    TEST_TRUE_(b2, "true || test.s1@b");
}

TEST_CASE("Int expression tests", "[alarm]") {
#define NS(N, P, D) vtss::expose::json::NamespaceNode N(&P, D);
    vtss::notifications::SubjectRunner main_thread("main", 0, true);

    expose::json::RootNode root;
    NS(ns_test, root, "test");

    expose::json::StructReadOnlyNotification<S1> s1_(&ns_test, "s1", &s1);

    NS(ns_status, ns_test, "status");
    NS(ns_status_g1, ns_status, "g1");
    NS(ns_status_g2, ns_status, "g2");

    expose::json::StructReadOnlyNotification<BAR> b1_(&ns_status, "b1", &b1);
    expose::json::StructReadOnlyNotification<BAR> b2_(&ns_status_g1, "b2", &b2);
    expose::json::StructReadOnlyNotification<BAR> b3_(&ns_status_g2, "b3", &b3);
    expose::json::StructReadOnlyNotificationNoNS<BAR> b4_(&ns_status_g1, &b4);

    vtss::expose::json::specification::Inventory inv;
    for (auto &i : root.leafs) walk(inv, &i, false);

    Map<str, std::string> i1;
)");

    Map<str, std::string> i2;
    i2.set(str("test.s1"), R"(
{
    "id":1,
    "error":null,
    "result":{
        "b":true,
        "u8":100,
        "u16":200,
        "u32_a":100,
        "u32_b":100,
        "u64":400,
        "i16":-120,
        "i32":500,
        "i64":-100000,
    }
}
)");

    TEST_TRUE_(i1, "6 == 6");
    TEST_FALSE(i1, "6 != 6");
    TEST_FALSE(i1, "5 == 6");
    TEST_TRUE_(i1, "6 != 5");

    TEST_TRUE_(i1, "-6 != 6");
    TEST_TRUE_(i1, "-6 < 6");
    TEST_FALSE(i1, "-6 > 6");

    TEST_TRUE_(i1, "5 < 6");
    TEST_FALSE(i1, "5 > 6");
    TEST_FALSE(i1, "6 < 5");
    TEST_TRUE_(i1, "6 > 5");

    TEST_TRUE_(i1, "5 <= 6");
    TEST_FALSE(i1, "5 >= 6");
    TEST_FALSE(i1, "6 <= 5");
    TEST_TRUE_(i1, "6 >= 5");

    TEST_TRUE_(i1, "6 <= 6");
    TEST_TRUE_(i1, "6 >= 6");

    TEST_FALSE(i1, "6 < 6");
    TEST_FALSE(i1, "6 > 6");

    TEST_TRUE_(i1, "6 * 6 == 36");
    TEST_FALSE(i1, "6 * 6 != 36");

    TEST_TRUE_(i1, "36 == 6 * 6");
    TEST_FALSE(i1, "36 != 6 * 6");

    TEST_TRUE_(i1, "17 == 3 * 4 + 5");
    TEST_TRUE_(i1, "17 == 5 + 3 * 4");
    TEST_TRUE_(i1, "17 == (3 * 4) + 5");
    TEST_TRUE_(i1, "17 == 5 + (3 * 4)");

    TEST_TRUE_(i1, "27 == 3 * (4 + 5)");
    TEST_TRUE_(i1, "27 == (5 + 4) * 3");

    TEST_FALSE(i1, "test.s1@u32_a == test.s1@u32_b");
    TEST_TRUE_(i2, "test.s1@u32_a == test.s1@u32_b");

    TEST_TRUE_(i1, "test.s1@u32_a != test.s1@u32_b");
    TEST_FALSE(i2, "test.s1@u32_a != test.s1@u32_b");

    TEST_TRUE_(i1, "test.s1@u32_a > 5");
}

TEST_CASE("String expression tests", "[alarm]") {
#define NS(N, P, D) vtss::expose::json::NamespaceNode N(&P, D);
    vtss::notifications::SubjectRunner main_thread("main", 0, true);

    expose::json::RootNode root;
    NS(ns_test, root, "test");

    expose::json::StructReadOnlyNotification<S1> s1_(&ns_test, "s1", &s1);

    vtss::expose::json::specification::Inventory inv;
    for (auto &i : root.leafs) walk(inv, &i, false);

    Map<str, std::string> i1;
    i1.set(str("test.s1"), R"(
{
    "id":1,
    "error":null,
    "result":{
        "s":"hello"
    }
}
)");

    Map<str, std::string> i2;
    i2.set(str("test.s1"), R"(
{
    "id":1,
    "error":null,
    "result":{
        "s":"asdf"
    }
}
)");

    TEST_TRUE_(i1, R"("asdf" == "asdf")");
    TEST_TRUE_(i1, R"("asdf" != "aSdf")");
    TEST_FALSE(i1, R"("xxx" == "asdf")");
    TEST_FALSE(i1, R"("asdf" != "asdf")");

    TEST_TRUE_(i1, R"("hello" == test.s1@s)");
    TEST_FALSE(i2, R"("hello" == test.s1@s)");
    TEST_FALSE(i1, R"("asdf" == test.s1@s)");
    TEST_TRUE_(i2, R"("asdf" == test.s1@s)");

    TEST_FALSE(i1, R"("hello" != test.s1@s)");
    TEST_TRUE_(i2, R"(test.s1@s != "hello")");
    TEST_TRUE_(i1, R"("asdf" != test.s1@s)");
    TEST_FALSE(i2, R"("asdf" != test.s1@s)");
    TEST_TRUE_(i2, R"(!("asdf" != test.s1@s))");
}

TEST_CASE("Enum expression tests", "[alarm]") {
#define NS(N, P, D) vtss::expose::json::NamespaceNode N(&P, D);
    vtss::notifications::SubjectRunner main_thread("main", 0, true);

    expose::json::RootNode root;
    NS(ns_test, root, "test");

    expose::json::StructReadOnlyNotification<S1> s1_(&ns_test, "s1", &s1);

    vtss::expose::json::specification::Inventory inv;
    for (auto &i : root.leafs) walk(inv, &i, false);

    Map<str, std::string> i1;
    i1.set(str("test.s1"), R"(
{
    "id":1,
    "error":null,
    "result":{
        "e":"A",
        "e_a":"B",
        "e_b":"C"
    }
}
)");

    Map<str, std::string> i2;
    i2.set(str("test.s1"), R"(
{
    "id":1,
    "error":null,
    "result":{
        "e":"A",
        "e_a":"A",
        "e_b":"A"
    }
}
)");

    TEST_FALSE(i1, R"(test.s1@e_a == "A")");
    TEST_TRUE_(i2, R"(test.s1@e_a == "A")");

    TEST_FALSE(i1, R"(test.s1@e_a == test.s1@e_b)");
    TEST_TRUE_(i2, R"(test.s1@e_a == test.s1@e_b)");
}

TEST_CASE("Table expression tests", "[alarm]") {
#define NS(N, P, D) vtss::expose::json::NamespaceNode N(&P, D);
    vtss::notifications::SubjectRunner main_thread("main", 0, true);

    expose::json::RootNode root;
    NS(ns_test, root, "test");
    NS(ns_status, ns_test, "status");

    expose::json::TableReadOnlyNotification<T1> t1_(&ns_status, "t1", &t1);
    expose::json::TableReadOnlyNotification<T2> t2_(&ns_status, "t2", &t2);
    expose::json::TableReadOnlyNotification<T3> t3_(&ns_status, "t3", &t3);
    expose::json::TableReadOnlyNotification<T4> t4_(&ns_status, "t4", &t4);
    expose::json::TableReadOnlyNotification<T5> t5_(&ns_status, "t5", &t5);

    vtss::expose::json::specification::Inventory inv;
    for (auto &i : root.leafs) walk(inv, &i, false);

    t1.clear();
    t1.set(1, {1, 2, 3});
    t1.set(10, {10, 20, 30});
    Map<str, std::string> b1;
    b1.set(str("test.status.t1"), json_get(root, str("test.status.t1")));

    TEST_TRUE_(b1, "test.status.t1[1]@a < 5");
    TEST_TRUE_(b1, "test.status.t1[1]@c > 2");
    TEST_FALSE(b1, "test.status.t1[2]@c > 2");
    TEST_TRUE_(b1, "test.status.t1[2]@c == nil");
    TEST_FALSE(b1, "test.status.t1[2]@c != nil");
    TEST_FALSE(b1, "test.status.t1[10]@c == nil");
    TEST_TRUE_(b1, "test.status.t1[10]@c != nil");
    TEST_TRUE_(b1, "test.status.t1[2]@c == nil || test.status.t1[2]@c > 2");
    TEST_TRUE_(b1, "test.status.t1[2]@c > 1 || test.status.t1[10]@c > 2");
    TEST_FALSE(b1, "test.status.t1[2]@c > 1 && test.status.t1[10]@c > 2");
}

}  // namespace leaf_test
}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#undef TEST_TRUE_
#undef TEST_FALSE
