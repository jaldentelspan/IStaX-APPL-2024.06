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

#include "main_conf.hxx"
#include <iostream>
#include "catch.hpp"

namespace vtss {
namespace appl {
namespace main {

TEST_CASE( "switch.conf", "[main_conf]" ) {
    SECTION("meba") {
        auto c = module_conf_get("meba");
        CHECK(c.str_get("lib", "notfound") == "/lib/meba_caracal.so");
        CHECK(c.str_get("board", "notfound") == "Luton10");
        CHECK(c.str_get("target", "notfound") == "0x7428");
    }

    SECTION("icli") {
        auto &c = module_conf_get("icli");
        CHECK(c.bool_get("enable", true) == true);
        CHECK(c.bool_get("enable", false) == true);
        CHECK(c.str_get("defaultConfig", "other") == "/etc/test");
        CHECK(c.bool_get("unknown", false) == false);
        CHECK(c.bool_get("unknown", true) == true);
    }

    SECTION("ssh") {
        auto &c = module_conf_get("ssh");
        CHECK(c.bool_get("enable", true) == false);
        CHECK(c.bool_get("enable", false) == false);
        CHECK(c.u32_get("u32", 41) == 42);
        CHECK(c.i32_get("i32", 41) == -42);
    }
    SECTION("unknown") {
        auto &c = module_conf_get("unknown");
        CHECK(c.bool_get("enable", true) == true);
        CHECK(c.bool_get("enable", false) == false);
    }
}

} // namespace main
} // namespace appl
} // namespace vtss
