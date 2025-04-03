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

#include "vtss/basics/expose/json/loader.hxx"
#include "vtss/basics/expose/json/literal.hxx"
#include "vtss/basics/expose/json/char-out-iterator.hxx"

namespace vtss {
namespace expose {
namespace json {

const Literal __attribute__((init_priority (999))) map_start(str("{"), true, true);
const Literal __attribute__((init_priority (999))) map_end("}", true, true);
const Literal __attribute__((init_priority (999))) array_start("[", true, true);
const Literal __attribute__((init_priority (999))) array_end("]", true, true);
const Literal __attribute__((init_priority (999))) map_assign(":", true, true);
const Literal __attribute__((init_priority (999))) delimetor(",", true, true);
const Literal __attribute__((init_priority (999))) quote_start("\"", true, false);
const Literal __attribute__((init_priority (999))) quote_end("\"", false, true);
const Literal __attribute__((init_priority (999))) lit_true("true", true, true);
const Literal __attribute__((init_priority (999))) lit_false("false", true, true);
const Literal __attribute__((init_priority (999))) lit_null("null", true, true);
const Literal __attribute__((init_priority (999))) null_char("0", false, false);

static bool is_whitespace(char c) { return c >= 0 && c <= 32; }

bool parse(const char *&b, const char *e, const Literal &lit) {
    const char *_b = b;
    const char *i = lit.s_.begin();

    // skip white spaces
    while (lit.accept_pre_whitespace_ && b != e && is_whitespace(*b)) {
        ++b;
    }

    // skip the string defined in lit
    // TODO(anielsen) use generic algorithm
    while (b != e && i != lit.s_.end()) {
        if (*b != *i) goto Error;
        ++b;
        ++i;
    }

    // skip white spaces
    while (lit.accept_post_whitespace_ && b != e && is_whitespace(*b)) {
        ++b;
    }

    // Check that the complete literal is parsed
    if (i != lit.s_.end()) goto Error;

    return true;

Error:
    b = _b;
    return false;
}

void serialize(Exporter &e, const Literal &l) {
    copy(l.s_.begin(), l.s_.end(), CharOutIterator(e));
}

void serialize(Loader &e, const Literal &l) {
    if (!parse(e.pos_, e.end_, l)) {
        e.flag_error();
    }
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

