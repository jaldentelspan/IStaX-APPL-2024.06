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

#include "critd_api.h"
#include "mac_api.h"
#include "mac.h"
#include "../util/vtss_appl_formatting_tags.hxx"
#include "../alarm/alarm-expression/any.hxx"
#include <vtss/basics/trace.hxx>

namespace vtss {


static bool mac_parse(const char *b, const char *e, mesa_mac_t *x) {
    int j;
    const char *i = b;
    char tmpH, tmpL;
    if (i+17 > e) return false;

    for (j=0; j<17;j=j+3) {
       if      (i[j] >= '0' && i[j]  <= '9') tmpH = i[j] - '0';
       else if (i[j] >= 'a' && i[j]  <= 'f') tmpH = i[j] - 'a' + 10;
       else if (i[j] >= 'A' && i[j]  <= 'F') tmpH = i[j] - 'A' + 10;
       else return false;

       if      (i[j+1] >= '0' && i[j+1]  <= '9') tmpL = i[j+1] - '0';
       else if (i[j+1] >= 'a' && i[j+1]  <= 'f') tmpL = i[j+1] - 'a' + 10;
       else if (i[j+1] >= 'A' && i[j+1]  <= 'F') tmpL = i[j+1] - 'A' + 10;
       else return false;

       if (j<15 && i[j+2] != ':' && i[j+2] != '-') return false;
       x->addr[j/3] = tmpH*16 + tmpL;
    }
    b = i;
    return true;
}

struct AnyMacAddr final : public appl::alarm::expression::Any {
    AnyMacAddr(mesa_mac_t x) : value(x) {};
    static appl::alarm::expression::AnyPtr construct_from(const appl::alarm::expression::AnyJsonPrimitive &a);
    static str operator_result(appl::alarm::expression::Token t);
    str name_of_type() const override { return NAME_OF_TYPE; };
    appl::alarm::expression::AnySharedPtr opr(appl::alarm::expression::Token opr, appl::alarm::expression::AnySharedPtr res, appl::alarm::expression::AnySharedPtr rhs) const override;
    static const str NAME_OF_TYPE;
    mesa_mac_t value;
};

const str AnyMacAddr::NAME_OF_TYPE = str("mesa_mac_t");

////////////////////////////////////////////////////////////////////////////////

using namespace appl::alarm::expression;

AnyPtr AnyMacAddr::construct_from(const AnyJsonPrimitive &a) {
    if (a.type() == AnyJsonPrimitive::STR) {
        const char *b = a.as_str().begin();
        const char *e = a.as_str().end();
        mesa_mac_t v;
        if (mac_parse(b, e, &v))
           return make_unique<AnyMacAddr>(v);
    }
    return AnyPtr(nullptr);
}

str AnyMacAddr::operator_result(Token t) {
    switch (t) {
    case Token::equal:
    case Token::not_equal:
        return AnyBool::NAME_OF_TYPE;

    default:
        return str();
    }
}

AnySharedPtr AnyMacAddr::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    if (!rhs) {
        VTSS_TRACE(ERROR) << "No 'rhs'";
        return nullptr;
    }

    if (name_of_type() != rhs->name_of_type()) {
        VTSS_TRACE(ERROR) << "Invalid type: " << name_of_type()
                       << " != " << rhs->name_of_type();
        return nullptr;
    }

    auto rhs_ = std::static_pointer_cast<AnyMacAddr>(rhs);

    bool b;
    switch (opr) {
    case Token::equal:
        b = (value.addr[0] == rhs_->value.addr[0] &&
             value.addr[1] == rhs_->value.addr[1] &&
             value.addr[2] == rhs_->value.addr[2] &&
             value.addr[3] == rhs_->value.addr[3] &&
             value.addr[4] == rhs_->value.addr[4] &&
             value.addr[5] == rhs_->value.addr[5]);
        VTSS_TRACE(DEBUG) << value << " == " << rhs_->value << " -> " << b;
        break;

    case Token::not_equal:
        b = (value.addr[0] != rhs_->value.addr[0] ||
             value.addr[1] != rhs_->value.addr[1] ||
             value.addr[2] != rhs_->value.addr[2] ||
             value.addr[3] != rhs_->value.addr[3] ||
             value.addr[4] != rhs_->value.addr[4] ||
             value.addr[5] != rhs_->value.addr[5]);
        VTSS_TRACE(DEBUG) << value << " != " << rhs_->value << " -> " << b;
        break;

    default:
        // Can not be reached
        return AnySharedPtr(nullptr);
    }

    return make_unique<AnyBool>(b);
}

}  // namespace

void mac_any_init() {
    vtss::appl::alarm::expression::any_add_type<vtss::AnyMacAddr>();
}
