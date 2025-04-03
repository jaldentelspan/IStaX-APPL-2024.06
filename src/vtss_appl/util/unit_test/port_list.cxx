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
#include "gtest/gtest.h"
#include "vtss/appl/types.hxx"
#include <iostream>
#include <vtss/basics/trace.hxx>

using namespace std;

namespace vtss {

TEST(SetGetClear, isid_port) {
    PortList l;
    PortList l2;

    EXPECT_EQ(l, l2);

    l.clear_all();
    EXPECT_TRUE(l.set(1, 0));
    EXPECT_EQ(l.as_api_type().data[0], 1);
    EXPECT_NE(l, l2);
    EXPECT_EQ(l.get(1, 0), true);
    EXPECT_TRUE(l.clear(1, 0));
    EXPECT_EQ(l.get(1, 0), false);

    l.clear_all();
    EXPECT_EQ(l, l2);

    EXPECT_TRUE(l.set(1, 10));
    EXPECT_EQ(l.as_api_type().data[1], 4);
    EXPECT_TRUE(l.set(1, 11));
    EXPECT_EQ(l.as_api_type().data[1], 12);
    EXPECT_TRUE(l.get(1, 11));
    EXPECT_TRUE(l.clear(1, 11));
    EXPECT_FALSE(l.get(1, 11));
    EXPECT_EQ(l.as_api_type().data[1], 4);
}

TEST(SetGetClear, ifindex) {
    PortList l;
    vtss_ifindex_t i;

    vtss_ifindex_from_port(3, 4, &i);
    EXPECT_TRUE(l.set(i));
    EXPECT_TRUE(l.get(3, 4));
    EXPECT_TRUE(l.clear(i));
    EXPECT_FALSE(l.get(3, 4));
}

TEST(iterate, 1) {
    vtss_ifindex_t i[6];

    vtss_ifindex_from_port( 1,   4, &i[0]);
    vtss_ifindex_from_port( 3,   4, &i[1]);
    vtss_ifindex_from_port( 3,   5, &i[2]);
    vtss_ifindex_from_port( 3,   9, &i[3]);
    vtss_ifindex_from_port(16,   4, &i[4]);
    vtss_ifindex_from_port(16,  11, &i[5]);

    PortList l;
    for (int j = 0; j < 6; ++j)
        EXPECT_TRUE(l.set(i[j]));

    uint32_t cnt = 0;
    for (const auto &port : l)
        EXPECT_EQ(port, i[cnt++]);

    EXPECT_EQ(cnt, 6);
}

TEST(print, empty) {
    PortList l;

    BufStream<SBuf64> s;
    s << l;
    EXPECT_TRUE(s.ok());

    const char *c = s.cstring();
    EXPECT_TRUE(s.ok());  // no overflow
    EXPECT_STREQ(c, "[]");
}

TEST(print, one_element_1) {
    PortList l;
    l.set(1, 1);

    BufStream<SBuf64> s;
    s << l;
    EXPECT_TRUE(s.ok());

    const char *c = s.cstring();
    EXPECT_TRUE(s.ok());  // no overflow
    EXPECT_STREQ(c, "[1/1]");
}

TEST(print, one_element_2) {
    PortList l;
    l.set(3, 9);

    BufStream<SBuf64> s;
    s << l;
    EXPECT_TRUE(s.ok());

    const char *c = s.cstring();
    EXPECT_TRUE(s.ok());  // no overflow
    EXPECT_STREQ(c, "[3/9]");
}

TEST(print, two_elements) {
    PortList l;
    l.set(3, 9);
    l.set(2, 4);

    BufStream<SBuf64> s;
    s << l;
    EXPECT_TRUE(s.ok());

    const char *c = s.cstring();
    EXPECT_TRUE(s.ok());  // no overflow
    EXPECT_STREQ(c, "[2/4, 3/9]");
}

TEST(print, seq_1) {
    PortList l;
    l.set(3, 9);
    l.set(3, 10);

    BufStream<SBuf64> s;
    s << l;
    EXPECT_TRUE(s.ok());

    const char *c = s.cstring();
    EXPECT_TRUE(s.ok());  // no overflow
    EXPECT_STREQ(c, "[3/9-10]");
}

TEST(print, seq_2) {
    PortList l;
    l.set(3, 9);
    l.set(3, 10);
    l.set(3, 11);

    BufStream<SBuf64> s;
    s << l;
    EXPECT_TRUE(s.ok());

    const char *c = s.cstring();
    EXPECT_TRUE(s.ok());  // no overflow
    EXPECT_TRUE(str(c) == str("[3/9-11]"));
}

TEST(print, seq_3) {
    PortList l;
    l.set(3, 9);
    l.set(3, 10);
    l.set(3, 11);
    l.set(1, 1);
    l.set(1, 2);
    l.set(1, 3);

    BufStream<SBuf64> s;
    s << l;
    EXPECT_TRUE(s.ok());

    const char *c = s.cstring();
    EXPECT_TRUE(s.ok());  // no overflow
    EXPECT_TRUE(str(c) == str("[1/1-3, 3/9-11]"));
}

TEST(print, no_seq) {
    PortList l;
    l.set(3, 9);
    l.set(4, 10);
    l.set(5, 11);

    BufStream<SBuf64> s;
    s << l;
    EXPECT_TRUE(s.ok());

    const char *c = s.cstring();
    EXPECT_TRUE(s.ok());  // no overflow
    EXPECT_TRUE(str(c) == str("[3/9, 4/10, 5/11]"));
}

}  // namespace vtss
