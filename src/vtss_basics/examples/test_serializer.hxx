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

#ifndef __EXAMPLE_TEST_SERIALIZER_HXX__
#define __EXAMPLE_TEST_SERIALIZER_HXX__
#include "test_api.h"
#include "vtss/basics/snmp.hxx"
#include "vtss/basics/types.hxx"

extern vtss_enum_descriptor_t vtss_test_enum_type_txt[];
VTSS_SNMP_SERIALIZE_ENUM(enumTestType_t, "EnumTestType", vtss_test_enum_type_txt, 
                         "This is the description of the type EnumTestType");

extern vtss_enum_descriptor_t vtss_table_enum_type_txt[];
VTSS_SNMP_SERIALIZE_ENUM(enumTableType_t, "EnumTableType", vtss_table_enum_type_txt,
                         "This is the description of the type EnumTableType");

extern vtss_enum_descriptor_t vtss_table2_enum_type_txt[];
VTSS_SNMP_SERIALIZE_ENUM(enumTable2Type_t, "EnumTable2Type", vtss_table2_enum_type_txt, 
                         "This is the description of the type EnumTable2Type");

template<typename T>
void serialize(T &a, ab_t &c) {
    a.add_leaf(c.a, vtss::tag::Name("a"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("This is member a having a rather long and \
                                       uninteresting description that needs to be\
                                       properly formatted."));

    a.add_leaf(c.b, vtss::tag::Name("b"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("This is member b"));
}

template<typename T>
void serialize(T &a, SomeConf &c) {
    a.add_leaf(c.a, vtss::tag::Name("a"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("This is member a having a rather long and \
                                       uninteresting description that needs to be\
                                       properly formatted. It contains the\
                                       character ' as in Paul's tree, but also\
                                       the character  which has been seen to cause\
                                       problems."));

    a.add_leaf(c.c, vtss::tag::Name("c"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("This is member c"));
}

template<typename T>
void serialize(T &a, SubConf &c) {
    a.add_leaf(c.sub1, vtss::tag::Name("sub1"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("This is member sub1"));

    a.add_leaf(c.sub2, vtss::tag::Name("sub2"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(2),
               vtss::tag::Description("This is member sub2"));

    a.add_leaf(c.sub3, vtss::tag::Name("sub3"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(3),
               vtss::tag::Description("This is member sub3"));

    a.add_leaf(c.aBoolean, vtss::tag::Name("aBoolean"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(4),
               vtss::tag::Description("This is member aBoolean"));

    a.add_leaf(vtss::AsBool(c.asBoolean), vtss::tag::Name("asBoolean"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(5),
               vtss::tag::Description("This is member asBoolean"));
}

template<typename T>
void serialize(T &a, SomeStatus &s) {
    a.add_leaf(s.d, vtss::tag::Name("d"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(11),
               vtss::tag::Description("This is member d"));

    a.add_leaf(s.e, vtss::tag::Name("e"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(12),
               vtss::tag::Description("This is member e"));

    a.add_leaf(s.f, vtss::tag::Name("f"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(13),
               vtss::tag::Description("This is member f"));
}

template<typename T>
void serialize(T &a, SomeDetailedStatus  &s) {
    a.add_leaf(s.g, vtss::tag::Name("g"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(10),
               vtss::tag::Description("This is member g"));

    a.add_leaf(s.h, vtss::tag::Name("h"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(20),
               vtss::tag::Description("This is member h"));

    a.add_leaf(s.i, vtss::tag::Name("i"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(30),
               vtss::tag::Description("This is member i"));
}


namespace vtss {
namespace expose {
namespace snmp {

namespace test {
// Start a new mib. All the mib text details are added in the serializer
// function.

NamespaceNode my_test_mib("testMib", 42);


NamespaceNode mib("testMib", 42);
inline void build_mib(MibGenerator &g) {
    g.add_history_element("201407010000Z", "Initial description");
    g.description("This is a completly useless test mib with\
                   a rather long an uninteresting description\
                   that needs to be formatted.\n In order to ease\
                   readability a line break has been inserted.\n \
                   Beware that tabs\t are removed from the text.");
    g.contact("x", "x");
    // g.xx("VTSS-TEST-MIB");
    g.conformance_oid(3);
    g.go(my_test_mib);
}

static NamespaceNode objects(&my_test_mib, OidElement(1, "testMibObjects"));
static NamespaceNode conf(  &objects, OidElement(1,  "conf"));
static NamespaceNode status(&objects, OidElement(2,  "status"));

// Add some object identifier to the MIB (folders)
// and add some leaf(s) to the mib
static StructRW<
    PARAM_VAL<SomeConf *>
> SomeLeaf1(&conf, OidElement(1, "aaa"),
            &vtss_test_some_conf_get,
            &vtss_test_some_conf_set);

static StructRW<PARAM_VAL<SubConf *>
> SubSomeLeaf1(&conf, OidElement(2, "bbb"),
               &vtss_test_sub_conf_get, &vtss_test_sub_conf_set);


static StructRO<
    PARAM_VAL<SomeStatus *>
> SomeLeaf2(&status, OidElement(100, "ccc"), &vtss_test_some_status_get);

static StructRO<
    PARAM_VAL<SomeDetailedStatus *>
> SomeLeaf3(&status, OidElement(200, "ddd"), &vtss_test_some_status2_get);


}  // namespace test
}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __EXAMPLE_TEST_SERIALIZER_HXX__

