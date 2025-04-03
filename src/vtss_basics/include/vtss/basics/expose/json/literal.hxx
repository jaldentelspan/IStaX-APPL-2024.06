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

#ifndef __VTSS_BASICS_EXPOSE_JSON_LITERAL_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_LITERAL_HXX__

#include <vtss/basics/string.hxx>
#include <vtss/basics/expose/json/pre-loader.hxx>
#include <vtss/basics/expose/json/pre-exporter.hxx>

namespace vtss {
namespace expose {
namespace json {

struct Literal {
    explicit Literal(const str &s, bool pre = false, bool post = false)
        : s_(s), accept_pre_whitespace_(pre), accept_post_whitespace_(post) {}

    explicit Literal(const char *s, bool pre = false, bool post = false)
        : s_(s), accept_pre_whitespace_(pre), accept_post_whitespace_(post) {}

    str s_;
    bool accept_pre_whitespace_;
    bool accept_post_whitespace_;
};

extern const Literal map_start;
extern const Literal map_end;
extern const Literal array_start;
extern const Literal array_end;
extern const Literal map_assign;
extern const Literal delimetor;
extern const Literal quote_start;
extern const Literal quote_end;
extern const Literal lit_true;
extern const Literal lit_false;
extern const Literal lit_null;
extern const Literal null_char;

void serialize(Loader &e, const Literal &l);
void serialize(Exporter &e, const Literal &l);
bool parse(const char *&b, const char *e, const Literal &);

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_LITERAL_HXX__
