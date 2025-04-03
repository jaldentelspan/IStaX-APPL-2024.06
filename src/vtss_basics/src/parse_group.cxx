/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss/basics/parse_group.hxx"

namespace vtss {
namespace parser {

bool Group(const char *& b, const char * e,
           ParserBase& t1) {
    const char * _b = b;
    if (!t1(b, e)) goto Error;
    return true;
 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    if (!t3(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    if (!t3(b, e)) goto Error;
    if (!t4(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    if (!t3(b, e)) goto Error;
    if (!t4(b, e)) goto Error;
    if (!t5(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    if (!t3(b, e)) goto Error;
    if (!t4(b, e)) goto Error;
    if (!t5(b, e)) goto Error;
    if (!t6(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    if (!t3(b, e)) goto Error;
    if (!t4(b, e)) goto Error;
    if (!t5(b, e)) goto Error;
    if (!t6(b, e)) goto Error;
    if (!t7(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    if (!t3(b, e)) goto Error;
    if (!t4(b, e)) goto Error;
    if (!t5(b, e)) goto Error;
    if (!t6(b, e)) goto Error;
    if (!t7(b, e)) goto Error;
    if (!t8(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8,
           ParserBase& t9) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    if (!t3(b, e)) goto Error;
    if (!t4(b, e)) goto Error;
    if (!t5(b, e)) goto Error;
    if (!t6(b, e)) goto Error;
    if (!t7(b, e)) goto Error;
    if (!t8(b, e)) goto Error;
    if (!t9(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8,
           ParserBase& t9, ParserBase& t10) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    if (!t3(b, e)) goto Error;
    if (!t4(b, e)) goto Error;
    if (!t5(b, e)) goto Error;
    if (!t6(b, e)) goto Error;
    if (!t7(b, e)) goto Error;
    if (!t8(b, e)) goto Error;
    if (!t9(b, e)) goto Error;
    if (!t10(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8,
           ParserBase& t9, ParserBase& t10, ParserBase& t11) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    if (!t3(b, e)) goto Error;
    if (!t4(b, e)) goto Error;
    if (!t5(b, e)) goto Error;
    if (!t6(b, e)) goto Error;
    if (!t7(b, e)) goto Error;
    if (!t8(b, e)) goto Error;
    if (!t9(b, e)) goto Error;
    if (!t10(b, e)) goto Error;
    if (!t11(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8,
           ParserBase& t9, ParserBase& t10, ParserBase& t11, ParserBase& t12) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    if (!t3(b, e)) goto Error;
    if (!t4(b, e)) goto Error;
    if (!t5(b, e)) goto Error;
    if (!t6(b, e)) goto Error;
    if (!t7(b, e)) goto Error;
    if (!t8(b, e)) goto Error;
    if (!t9(b, e)) goto Error;
    if (!t10(b, e)) goto Error;
    if (!t11(b, e)) goto Error;
    if (!t12(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8,
           ParserBase& t9, ParserBase& t10, ParserBase& t11, ParserBase& t12,
           ParserBase& t13) {
    const char * _b = b;

    if (!t1(b, e)) goto Error;
    if (!t2(b, e)) goto Error;
    if (!t3(b, e)) goto Error;
    if (!t4(b, e)) goto Error;
    if (!t5(b, e)) goto Error;
    if (!t6(b, e)) goto Error;
    if (!t7(b, e)) goto Error;
    if (!t8(b, e)) goto Error;
    if (!t9(b, e)) goto Error;
    if (!t10(b, e)) goto Error;
    if (!t11(b, e)) goto Error;
    if (!t12(b, e)) goto Error;
    if (!t13(b, e)) goto Error;
    return true;

 Error:
    b = _b;
    return false;
}

}  // namespace parser
}  // namespace vtss
