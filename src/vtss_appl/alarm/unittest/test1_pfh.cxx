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

#include "catch.hpp"
#include "../alarm-expression.hxx"
#include <vtss/basics/trace.hxx>

namespace vtss {
namespace appl {
namespace alarm {

#define OK(A, B)                           \
    SECTION(A) {                           \
        Expression e(A);                   \
        CHECK(e.ok());                     \
        CHECK(e.parsed_expression() == B); \
    }

#define NOT_OK(A)        \
    SECTION(A) {         \
        Expression e(A); \
        CHECK(!e.ok());  \
    }


TEST_CASE("test1_pfh", "[alarm]") {
#if 0
    OK("port == \"this is a string\" && asdf == 5 + 7",
       "((port == \"this is a string\") && (asdf == (5 + 7)))");

    OK("port1 == nil", "(port1 == nil)");

    OK("port1 == true", "(port1 == true)");

    OK("port1 == false", "(port1 == false)");

    OK("false", "false");

    OK("!false", "!false");

    OK("!!false", "!!false");
    OK("var == !!false", "(var == !!false)");
    OK("var1 == !!false && \"a string\" != var2 ",
       "((var1 == !!false) && (\"a string\" != var2))");
#endif
}

#if 0

TEST_CASE("test1_alarm-expression", "[alarm, alarm-compare_numeric]") {
    OK("port.index == 600", "(port.index == 600)");
    OK("port.index[1] == 600", "(port.index[1] == 600)");
    OK("port.index[2,\"hej1\"].status == true",
       "(port.index[2,\"hej1\"].status == true)");
    OK("port.index[2,\"hej1\",3].status == true",
       "(port.index[2,\"hej1\",3].status == true)");
    OK("port == -600", "(port == -600)");
    OK("port3 != 50", "(port3 != 50)");
    OK("port3 != -50", "(port3 != -50)");
    OK("port3 <= 5", "(port3 <= 5)");
    OK("port3 <= -5", "(port3 <= -5)");
    OK("port3 <  75", "(port3 < 75)");
    OK("port3 <  -75", "(port3 < -75)");
    OK("port3 >= 700", "(port3 >= 700)");
    OK("port3 >= -700", "(port3 >= -700)");
    OK("port3 >  73", "(port3 > 73)");
    OK("port3 >  -73", "(port3 > -73)");
    NOT_OK("port == 600 3");
    NOT_OK("port == -600 +");
    NOT_OK("port3 != 50  -");
    NOT_OK("port3 != -50 *");
    NOT_OK("port3 <= 5 /");
    NOT_OK("port3 <= -5 >");
    NOT_OK("port3 <  75 <");
    NOT_OK("port3 <  -75 <=");
    NOT_OK("port3 >= 700 >=");
    NOT_OK("port3 >= -700 ==");
    NOT_OK("port3 >  73  !=");
    NOT_OK("port3 >  -73 !");
    NOT_OK("port 4 == 600 3");
    NOT_OK("port 4 == -600 +");
    NOT_OK("port3 4 != 50  -");
    NOT_OK("port3 4 != -50 *");
    NOT_OK("port3 4 <= 5 /");
    NOT_OK("port3 4 <= -5 >");
    NOT_OK("port3 4 <  75 <");
    NOT_OK("port3 4 <  -75 <=");
    NOT_OK("port3 4 >= 700 >=");
    NOT_OK("port3 4 >= -700 ==");
    NOT_OK("port3 4 >  73  !=");
    NOT_OK("port3 4 >  -73 !");
    NOT_OK("port == || 600");
    NOT_OK("port == + -600");
    NOT_OK("port3 != || 50");
    NOT_OK("port3 != - -50");
    NOT_OK("port3 <= + 5");
    NOT_OK("port3 <= + -5");
    NOT_OK("port3 <  * 75");
    NOT_OK("port3 <  / -75");
    NOT_OK("port3 >= + 700");
    NOT_OK("port3 >= && -700");
    NOT_OK("port3 >  ||73");
    NOT_OK("port3 >  -73 !");
    NOT_OK("(port == -600");
    NOT_OK("(port3 != 50");
    NOT_OK("(port3 != -50");
    NOT_OK("(port3 <= 5");
    NOT_OK("(port3 <= -5");
    NOT_OK("(port3 <  75");
    NOT_OK("(port3 <  -75");
    NOT_OK("(port3 >= 700");
    NOT_OK("(port3 >= -700");
    NOT_OK("(port3 >  73");
    NOT_OK("(port3 >  -73");
    NOT_OK(")port == -600");
    NOT_OK(")port3 != 50");
    NOT_OK(")port3 != -50");
    NOT_OK(")port3 <= 5");
    NOT_OK(")port3 <= -5");
    NOT_OK(")port3 <  75");
    NOT_OK(")port3 <  -75");
    NOT_OK(")port3 >= 700");
    NOT_OK(")port3 >= -700");
    NOT_OK(")port3 >  73");
    NOT_OK(")port3 >  -73");
    NOT_OK("port3 != 50)");
    NOT_OK("port3 != -50)");
    NOT_OK("port3 <= 5)");
    NOT_OK("port3 <= -5)");
    NOT_OK("port3 <  75)");
    NOT_OK("port3 <  -75)");
    NOT_OK("port3 >= 700)");
    NOT_OK("port3 >= -700)");
    NOT_OK("port3 >  73)");
    NOT_OK("port3 >  -73)");
}

TEST_CASE("test2_alarm-expression", "[alarm, alarm-compare_numericBinOp] OK") {
    OK("port1 == (3 + 5)", "(port1 == (3 + 5))");
    OK("port == 3 + 5 * 7", "(port == (3 + (5 * 7)))");

    OK("port == (3 + 5) * 7", "(port == ((3 + 5) * 7))");

    OK("port == (3 + 5) * (7 + 3)", "(port == ((3 + 5) * (7 + 3)))");

    OK("port1 == (3 + 5)", "(port1 == (3 + 5))");


    OK("port1 == (3 + 5) || port2 >= 7 && port3 <= 5",
       "((port1 == (3 + 5)) || ((port2 >= 7) && (port3 <= 5)))");

    //    OK("port == 31 30", "");
    NOT_OK("port == 31 30");
    NOT_OK("port == 31 30");
    //    NOT_OK("port == 3+7");
    NOT_OK("port == 3+");
    NOT_OK("port == 30+*");
    NOT_OK("port == 31+7/");
    NOT_OK("port == 32+7-");
    NOT_OK("port >= 33 &&");
    NOT_OK("port ## 33 &&");
    NOT_OK("(port == -600");
}

#endif

// TEST_CASE("test3_alarm-expression", "[numeric] NOT OK") {
// }
}  // namespace vtss
}  // namespace appl
}  // namespace alarm
