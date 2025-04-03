/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss/basics/string.hxx"
#include "vtss/basics/json-rpc-server.hxx"
#include "vtss/basics/memcmp-operator.hxx"

#include "vtss/basics/expose/json.hxx"
#include "vtss/basics/expose/types.hxx"
#include "vtss/basics/expose/snmp/types.hxx"

namespace vtss {
namespace appl {
namespace alarm {

enum Key { A = 0, B = 3, C = 10 };
extern const vtss_enum_descriptor_t key_txt[];

}
}
}

using namespace vtss::expose::json;
using namespace vtss::expose;
using namespace vtss::notifications;

VTSS_JSON_SERIALIZE_ENUM(vtss::appl::alarm::Key, "Key",
                         vtss::appl::alarm::key_txt, "-");

namespace vtss {
namespace appl {
namespace alarm {

struct Bar {
    uint32_t a;
    uint32_t b;
    uint32_t c;
};

struct Foo {
    bool b;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    Key e;
    char s[128];

    Bar b1;
    Bar b2;
};

VTSS_BASICS_MEMCMP_OPERATOR(Foo);



// TODO, we need to handle ambiguity in the naming of elements

// TODO, please add a table where the key is a enum

// TODO, please add a table where the value is a struct with which contains a
// bool, u8, i8, u16, i16, u32, i32, u64, i64, enum

struct T1 {
    typedef expose::ParamList<expose::ParamKey<int>, expose::ParamVal<int>> P;

    static constexpr const char *table_description = "T1 - table_description ";
    static constexpr const char *index_description = "T1 - index_description ";

    VTSS_EXPOSE_SERIALIZE_ARG_1(int &k) {
        h.add_leaf(k, vtss::tag::Name("k"), vtss::tag::Description("key"),
                   snmp::Status::Current, snmp::OidElementValue(1));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(int &v) {
        h.add_leaf(v, vtss::tag::Name("v"), vtss::tag::Description("val"),
                   snmp::Status::Current, snmp::OidElementValue(2));
    }
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

template <typename HANDLER>
void serialize(HANDLER &h, Bar &b) {
    typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("Bar"));
    m.add_leaf(b.a, vtss::tag::Name("a"), vtss::tag::Description("a"));
    m.add_leaf(b.b, vtss::tag::Name("b"), vtss::tag::Description("b"));
    m.add_leaf(b.c, vtss::tag::Name("c"), vtss::tag::Description("c"));
}

struct S1 {
    typedef expose::ParamList<expose::ParamVal<Foo *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(Foo &f) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("Foo"));

        m.add_leaf(f.b, vtss::tag::Name("b"), vtss::tag::Description("b"));
        m.add_leaf(f.u8, vtss::tag::Name("u8"), vtss::tag::Description("u8"));
        m.add_leaf(f.u16, vtss::tag::Name("u16"),
                   vtss::tag::Description("u16"));
        m.add_leaf(f.u32, vtss::tag::Name("u32"),
                   vtss::tag::Description("u32"));
        m.add_leaf(f.u64, vtss::tag::Name("u64"),
                   vtss::tag::Description("u64"));
        m.add_leaf(f.i16, vtss::tag::Name("i16"),
                   vtss::tag::Description("i16"));
        m.add_leaf(f.i32, vtss::tag::Name("i32"),
                   vtss::tag::Description("i32"));
        m.add_leaf(f.i64, vtss::tag::Name("i64"),
                   vtss::tag::Description("i64"));
        m.add_leaf(f.e, vtss::tag::Name("e"), vtss::tag::Description("e"));
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

}  // namespace alarm
}  // namespace appl
}  // namespace vtss
