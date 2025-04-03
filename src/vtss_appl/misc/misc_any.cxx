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

#include "misc.h"
#include "../util/vtss_appl_formatting_tags.hxx"
#include "../alarm/alarm-expression/any.hxx"
#include <vtss/basics/trace.hxx>

namespace vtss {

struct AnyIfIndex final : public appl::alarm::expression::Any {
    AnyIfIndex(vtss_ifindex_t x) : value(x) {};
    static appl::alarm::expression::AnyPtr construct_from(const appl::alarm::expression::AnyJsonPrimitive &a);
    static str operator_result(appl::alarm::expression::Token t);
    str name_of_type() const override { return NAME_OF_TYPE; };
    appl::alarm::expression::AnySharedPtr opr(appl::alarm::expression::Token opr, appl::alarm::expression::AnySharedPtr res, appl::alarm::expression::AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
    vtss_ifindex_t value;
};

const str AnyIfIndex::NAME_OF_TYPE = str("vtss_ifindex_t");

////////////////////////////////////////////////////////////////////////////////

using namespace appl::alarm::expression;

AnyPtr AnyIfIndex::construct_from(const AnyJsonPrimitive &a) {
    if (a.type() == AnyJsonPrimitive::STR) {
        const char *b = a.as_str().begin();
        const char *e = a.as_str().end();
        vtss_ifindex_t v;
        vtss::AsInterfaceIndex x(v);
        if (vtss::parse(b, e, x))
           return make_unique<AnyIfIndex>(x.val);
    }
    return AnyPtr(nullptr);
}

str AnyIfIndex::operator_result(Token t) {
    switch (t) {
    case Token::equal:
    case Token::not_equal:
        return AnyBool::NAME_OF_TYPE;

    default:
        return str();
    }
}

AnySharedPtr AnyIfIndex::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    if (!rhs) {
        VTSS_TRACE(ERROR) << "No 'rhs'";
        return nullptr;
    }

    if (name_of_type() != rhs->name_of_type()) {
        VTSS_TRACE(ERROR) << "Invalid type: " << name_of_type()
                       << " != " << rhs->name_of_type();
        return nullptr;
    }

    auto rhs_ = std::static_pointer_cast<AnyIfIndex>(rhs);

    bool b;
    switch (opr) {
    case Token::equal:
        b = (value == rhs_->value);
        VTSS_TRACE(DEBUG) << value << " == " << rhs_->value << " -> " << b;
        break;

    case Token::not_equal:
        b = (value != rhs_->value);
        VTSS_TRACE(DEBUG) << value << " != " << rhs_->value << " -> " << b;
        break;

    default:
        // Can not be reached
        return AnySharedPtr(nullptr);
    }

    return make_unique<AnyBool>(b);
}

}  // namespace

void misc_any_init() {
    vtss::appl::alarm::expression::any_add_type<vtss::AnyIfIndex>();
}
