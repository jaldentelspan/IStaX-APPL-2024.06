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

#include "vtss/basics/expose/json/loader.hxx"
#include "vtss/basics/expose/json/exporter.hxx"
#include "vtss/basics/expose/json/json-value-ptr.hxx"

#include "vtss/basics/expose/json/literal.hxx"
#include "vtss/basics/expose/json/skip-array.hxx"
#include "vtss/basics/expose/json/skip-number.hxx"
#include "vtss/basics/expose/json/skip-object.hxx"
#include "vtss/basics/expose/json/skip-string.hxx"
#include "vtss/basics/expose/json/skip-value.hxx"

namespace vtss {
namespace expose {
namespace json {

bool parse(const char *&b, const char *e, JsonValuePtr &s) {
    const char *i = b;

    SkipString string;
    if (parse(b, e, string)) {
        s.data = str(i, b);
        s.type_ = ValueType::String;
        return true;
    }

    SkipNumber number;
    if (parse(b, e, number)) {
        s.data = str(i, b);
        s.type_ = ValueType::Number;
        return true;
    }

    SkipObject object;
    if (parse(b, e, object)) {
        s.data = str(i, b);
        s.type_ = ValueType::Object;
        return true;
    }

    SkipArray array;
    if (parse(b, e, array)) {
        s.data = str(i, b);
        s.type_ = ValueType::Array;
        return true;
    }

    if (parse(b, e, lit_true)) {
        s.data = str(i, b);
        s.type_ = ValueType::Boolean;
        return true;
    }

    if (parse(b, e, lit_false)) {
        s.data = str(i, b);
        s.type_ = ValueType::Boolean;
        return true;
    }

    if (parse(b, e, lit_null)) {
        s.data = str(i, b);
        s.type_ = ValueType::Null;
        return true;
    }

    return false;
}

void serialize(Exporter &, JsonValuePtr &) { VTSS_ASSERT(0); }

void serialize(Loader &l, JsonValuePtr &s) {
    if (!parse(l.pos_, l.end_, s)) {
        l.flag_error();
    }
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

