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



#include <vtss/basics/trace.hxx>
#include <vtss/basics/memory.hxx>
#include <vtss/basics/parser_impl.hxx>

#include "alarm-expression.hxx"
#include "alarm-expression/token.hxx"
#include "alarm-expression/tree-element.hxx"
#include "alarm-expression/tree-element-var.hxx"
#include "alarm-expression/shunting-yard.hxx"
#include "alarm-trace.h"

namespace vtss {
namespace appl {
namespace alarm {

Expression::Expression(const std::string &expr,
                       const expose::json::specification::Inventory &i)
    : expr_(expr) {
    CheckSyntaxOfExpressionAndBuildExpressionTree(i);
}

// It first scan the alarm expression and generates a vector of tree elements.
// Thereafter this is used as input to a ShuntingYard object, that generates the
// tree using ShuntingYard method, and store to top in the root_.  if ok_ ==
// true, then the tree was generated with succes, and else the rc_ can be used
// to see RC error code detail.
void Expression::CheckSyntaxOfExpressionAndBuildExpressionTree(
        const expose::json::specification::Inventory &i) {
    using namespace expression;
    using vtss::make_unique;

    const char *b = expr_.c_str();
    const char *e = b + expr_.size();
    ok_ = true;        // starting as true;
    rc_ = VTSS_RC_OK;  // starting as ok
    root_ = nullptr;   // starting as nullptr;


    DEFAULT(INFO) << "Parsing: " << expr_.c_str() << "\n";

    while (b != e) {
        parser::JsonString string_;
        TreeElementVarParser var;

        parser::Lit nil("nil");
        parser::Lit true_("true");
        parser::Lit false_("false");

        parser::Lit plus("+");
        parser::Lit minus("-");
        parser::Lit mult("*");
        parser::Lit div("/");
        parser::Lit and_("&&");
        parser::Lit or_("||");
        parser::Lit start_p("(");
        parser::Lit end_p(")");
        parser::Lit equal("==");
        parser::Lit not_equal("!=");
        parser::Lit great(">");
        parser::Lit less("<");
        parser::Lit great_equal(">=");
        parser::Lit less_equal("<=");
        parser::Lit neg("!");
        parser::OneOrMoreSpaces space;
        parser::Int<uint32_t, 10, 1, 0> uint32_;
        parser::Int<int32_t, 10, 1, 0> int32_;
        parser::Int<uint64_t, 10, 1, 0> uint64_;
        parser::Int<int64_t, 10, 1, 0> int64_;

        DEFAULT(NOISE) << "Parsing (so far): " << str(b, e);
        if (string_(b, e)) {
            DEFAULT(NOISE) << "Got a string";
            tree_elements.emplace_back(
                    vtss::make_unique<TreeElementConst>(vtss::move(string_.value)));

        } else if (nil(b, e)) {
            DEFAULT(NOISE) << "Got a nil";
            tree_elements.emplace_back(make_unique<TreeElementConst>(nullptr));

        } else if (true_(b, e)) {
            DEFAULT(NOISE) << "Got a true";
            tree_elements.emplace_back(make_unique<TreeElementConst>(true));

        } else if (false_(b, e)) {
            DEFAULT(NOISE) << "Got a false";
            tree_elements.emplace_back(make_unique<TreeElementConst>(false));

        } else if (var(b, e)) {
            DEFAULT(NOISE) << "Got a var: " << var.value->json_method_name
                           << "@" << var.value->json_variable_name;
            tree_elements.emplace_back(vtss::move(var.value));

        } else if (uint32_(b, e)) {
            DEFAULT(NOISE) << "Got a uint32_";
            tree_elements.emplace_back(
                    make_unique<TreeElementConst>(uint32_.get()));

        } else if (uint64_(b, e)) {
            DEFAULT(NOISE) << "Got a uint64_";
            tree_elements.emplace_back(
                    make_unique<TreeElementConst>(uint64_.get()));

        } else if (int32_(b, e)) {
            DEFAULT(NOISE) << "Got a int32_";
            tree_elements.emplace_back(
                    make_unique<TreeElementConst>(int32_.get()));

        } else if (int64_(b, e)) {
            DEFAULT(NOISE) << "Got a int64_";
            tree_elements.emplace_back(
                    make_unique<TreeElementConst>(int64_.get()));

        } else if (plus(b, e)) {
            DEFAULT(NOISE) << "Got a plus";
            tree_elements.emplace_back(make_unique<TreeElementOpr>(Token::plus));

        } else if (minus(b, e)) {
            DEFAULT(NOISE) << "Got a minus";
            tree_elements.emplace_back(make_unique<TreeElementOpr>(Token::minus));

        } else if (mult(b, e)) {
            DEFAULT(NOISE) << "Got a mult";
            tree_elements.emplace_back(make_unique<TreeElementOpr>(Token::mult));

        } else if (div(b, e)) {
            DEFAULT(NOISE) << "Got a div";
            tree_elements.emplace_back(make_unique<TreeElementOpr>(Token::div));

        } else if (and_(b, e)) {
            DEFAULT(NOISE) << "Got a and_";
            tree_elements.emplace_back(make_unique<TreeElementOpr>(Token::and_));

        } else if (or_(b, e)) {
            DEFAULT(NOISE) << "Got a or_";
            tree_elements.emplace_back(make_unique<TreeElementOpr>(Token::or_));

        } else if (start_p(b, e)) {
            DEFAULT(NOISE) << "Got a start_p";
            tree_elements.emplace_back(
                    make_unique<TreeElementOpr>(Token::start_p));

        } else if (end_p(b, e)) {
            DEFAULT(NOISE) << "Got a end_p";
            tree_elements.emplace_back(make_unique<TreeElementOpr>(Token::end_p));

        } else if (equal(b, e)) {
            DEFAULT(NOISE) << "Got a equal";
            tree_elements.emplace_back(make_unique<TreeElementOpr>(Token::equal));

        } else if (not_equal(b, e)) {
            DEFAULT(NOISE) << "Got a not_equal";
            tree_elements.emplace_back(
                    make_unique<TreeElementOpr>(Token::not_equal));

        } else if (great_equal(b, e)) {
            DEFAULT(NOISE) << "Got a great_equal";
            tree_elements.emplace_back(
                    make_unique<TreeElementOpr>(Token::great_equal));

        } else if (less_equal(b, e)) {
            DEFAULT(NOISE) << "Got a less_equal";
            tree_elements.emplace_back(
                    make_unique<TreeElementOpr>(Token::less_equal));

        } else if (great(b, e)) {
            DEFAULT(NOISE) << "Got a great";
            tree_elements.emplace_back(make_unique<TreeElementOpr>(Token::great));

        } else if (less(b, e)) {
            DEFAULT(NOISE) << "Got a less";
            tree_elements.emplace_back(make_unique<TreeElementOpr>(Token::less));

        } else if (neg(b, e)) {
            DEFAULT(NOISE) << "Got a neg";
            tree_elements.emplace_back(make_unique<TreeElementOpr>(Token::neg));

        } else if (space(b, e)) {
            DEFAULT(NOISE) << "Got a space";

        } else {
            DEFAULT(INFO) << "Got a error!";
            rc_ = VTSS_RC_ERROR_ALARM_EXPR_SYNTAX_PARSE_UNKNOWN;
            ok_ = false;
            return;
        }
    }

    DEFAULT(DEBUG) << "Parsing Done";
    ShuntingYard shunting_yard;
    for (auto &e : tree_elements) ok_ = ok_ && shunting_yard.push(e.get());

    if (!ok_) {
        DEFAULT(INFO) << "ShuntingYard failed to accept all elements";
        return;
    }

    root_ = shunting_yard.finish();
    if (root_ == nullptr) {
        DEFAULT(INFO) << "ShuntingYard failed finish";
        ok_ = false;
        rc_ = shunting_yard.rc_;
        return;
    }

    if (!root_->check(i, json_vars)) {
        DEFAULT(INFO) << "Type check failed";
        ok_ = false;
        return;
    }

    DEFAULT(INFO) << "Expression OK";
}

bool Expression::evaluate(const Map<str, std::string> &bindings) {
    if (ok_ == false) return false;

    DEFAULT(INFO) << "Call eval";
    auto res = root_->eval(bindings);

    if (!res) {
        DEFAULT(WARNING) << "return nullptr";
        // TODO: FLAG ERROR
        return false;
    }

    if (res->name_of_type() == str("vtss_bool_t")) {
        auto b = std::static_pointer_cast<expression::AnyBool>(res);
        return b->value;

    } else if (res->name_of_type() == expression::AnyJsonPrimitive::NAME_OF_TYPE) {
        auto b = std::static_pointer_cast<expression::AnyJsonPrimitive>(res);
        if (b->type() != expression::AnyJsonPrimitive::BOOL) {
            DEFAULT(INFO) << "Unexpected type: " << res->name_of_type();
            // TODO: FLAG ERROR
            return false;
        }

        return b->as_bool();

    } else {
        DEFAULT(INFO) << "Unexpected type: " << res->name_of_type();
        // TODO: FLAG ERROR
        return false;
    }
}

static void parsed_expression__(ostream &o, const expression::TreeElement *e_) {
    switch (e_->type()) {
    case expression::TreeElement::OPR: {
        const auto *e = static_cast<const expression::TreeElementOpr *>(e_);
        if (e->child_lhs && e->child_rhs) {
            o << "(";
            parsed_expression__(o, e->child_lhs);
            o << " " << e->symbol() << " ";
            parsed_expression__(o, e->child_rhs);
            o << ")";
        } else if (e->child_lhs) {
            o << e->symbol();
            parsed_expression__(o, e->child_lhs);
        }

        break;
    }

    case expression::TreeElement::VAR: {
        const auto *e = static_cast<const expression::TreeElementVar *>(e_);
        o << e->input_string;
        break;
    }

    case expression::TreeElement::CONST: {
        o << "<const>";
        break;
    }
    }
}

std::string Expression::parsed_expression() const {
    if (!ok_) return "";

    StringStream o;
    parsed_expression__(o, root_);
    return o.buf;
}

}  // namespace alarm
}  // namespace appl
}  // namespace vtss
