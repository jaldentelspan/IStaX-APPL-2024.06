/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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


#include "alarm-expression/tree-element.hxx"
#include "alarm-expression/tree-element-var.hxx"

#include <vtss/basics/trace.hxx>
#include <vtss/basics/memory.hxx>
#include <vtss/basics/expose/json/method-split.hxx>
#include <vtss/basics/expose/json/specification/type-descriptor-enum.hxx>
#include <vtss/basics/expose/json/specification/type-descriptor-struct.hxx>

#include "alarm-trace.h"


namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

static const int PRIO[] = {
        5,   // Token::plus
        5,   // Token::minus
        6,   // Token::mult
        6,   // Token::div
        2,   // Token::and_
        1,   // Token::or_
        -1,  // Token::start_p
        -2,  // Token::end_p
        3,   // Token::equal
        3,   // Token::not_equal
        3,   // Token::great
        3,   // Token::less
        3,   // Token::great_equal
        3,   // Token::less_equal
        4,   // Token::neg
};

int TreeElement::prio() const {
    if (type() == OPR) {
        auto *o = static_cast<const TreeElementOpr *>(this);
        return PRIO[(int)o->opr];
    } else {
        return 0;
    }
}

////////////////////////////////////////////////////////////////////////////////

static AnySharedPtr _eval_null(Token opr, AnySharedPtr a, AnySharedPtr b,
                               str a_type, str b_type) {
    bool a_is_null = (a_type == AnyNull::NAME_OF_TYPE);
    bool b_is_null = (b_type == AnyNull::NAME_OF_TYPE);

    switch (opr) {
    case Token::and_:
        return std::make_shared<AnyBool>(false);

    case Token::or_:
        if (a_is_null && b_is_null) {
            return std::make_shared<AnyBool>(false);
        } else if (a_is_null) {
            if (b->name_of_type() == AnyBool::NAME_OF_TYPE) {
                auto x = std::static_pointer_cast<AnyBool>(b);
                return std::make_shared<AnyBool>(x->value);

            } else {
                DEFAULT(WARNING) << "  unexpected!";
                return std::make_shared<AnyNull>();
            }

        } else if (b_is_null) {
            if (a->name_of_type() == AnyBool::NAME_OF_TYPE) {
                auto x = std::static_pointer_cast<AnyBool>(a);
                return std::make_shared<AnyBool>(x->value);

            } else {
                DEFAULT(WARNING) << "  unexpected!";
                return std::make_shared<AnyNull>();
            }

        } else {
            DEFAULT(WARNING) << "  unexpected!";
            return std::make_shared<AnyNull>();
        }

    case Token::neg:
        return std::make_shared<AnyBool>(true);

    case Token::equal:
        if (a_is_null && b_is_null) {
            return std::make_shared<AnyBool>(true);
        } else {
            return std::make_shared<AnyBool>(false);
        }

    case Token::not_equal:
        if (a_is_null && b_is_null) {
            return std::make_shared<AnyBool>(false);
        } else {
            return std::make_shared<AnyBool>(true);
        }

    default:
        return std::make_shared<AnyNull>();
    }
}

static AnySharedPtr _eval(Token opr, AnySharedPtr a, AnySharedPtr b,
                          AnySharedPtr result_buffer, str res_type) {
    str a_type, b_type;
    if (a) a_type = a->name_of_type();
    if (b) b_type = b->name_of_type();


    if (a_type == AnyNull::NAME_OF_TYPE || b_type == AnyNull::NAME_OF_TYPE) {
        DEFAULT(DEBUG) << "Evaluate: " << a_type << " " << opr << " " << b_type
                       << " -> " << res_type;

        if (res_type != AnyBool::NAME_OF_TYPE) {
            DEFAULT(INFO)
                    << "Either rhs or lhs is null, and the result type is not "
                    << "a boolean. Returning null";
            return std::make_shared<AnyNull>();
        }

        auto res = _eval_null(opr, a, b, a_type, b_type);
        DEFAULT(DEBUG) << "result: " << res->name_of_type();
        return res;
    }

    // Normal evaluation with no null arguments ////////////////////////////////
    switch (opr) {
    case Token::and_:
    case Token::div:
    case Token::equal:
    case Token::great:
    case Token::great_equal:
    case Token::less:
    case Token::less_equal:
    case Token::minus:
    case Token::mult:
    case Token::neg:
    case Token::not_equal:
    case Token::or_:
    case Token::plus: {
        return a->opr(opr, result_buffer, b);
    }

    default:
        DEFAULT(ERROR) << "Unexpected token";
        return AnySharedPtr(nullptr);
    }
}


AnySharedPtr TreeElementOpr::eval(const Map<str, std::string> &bindings) {
    DEFAULT(DEBUG) << "Eval opr: " << opr;

    AnySharedPtr a, b, result_buffer;

    // Evaluate RHS and LHS ////////////////////////////////////////////////////
    switch (opr) {
    case Token::and_:
    case Token::div:
    case Token::equal:
    case Token::great:
    case Token::great_equal:
    case Token::less:
    case Token::less_equal:
    case Token::minus:
    case Token::mult:
    case Token::neg:
    case Token::not_equal:
    case Token::or_:
    case Token::plus:
        a = child_lhs->eval(bindings);
        if (child_rhs) b = child_rhs->eval(bindings);

        // We are allowed to re-cycle the any pointer if it is not returned from
        // a constant.
        if (child_lhs->type() != TreeElement::CONST) {
            result_buffer = a;
        } else if (child_rhs && child_rhs->type() != TreeElement::CONST) {
            result_buffer = b;
        }


        break;

    default:
        DEFAULT(ERROR) << "Unexpected token";
        return AnySharedPtr(nullptr);
    }

    return _eval(opr, a, b, result_buffer, name_of_type());
}

static bool negotiate_common_types(TreeElementConst *a, TreeElementConst *b) {
    AnyPtr a_;
    AnyPtr b_;

#define TRY(S)                                       \
    a_ = a->copy_to(str(S));                         \
    b_ = b->copy_to(str(S));                         \
    if (a_ && b_) {                                  \
        a->value = vtss::move(a_);                   \
        b->value = vtss::move(b_);                   \
        a->name_of_type_ = a->value->name_of_type(); \
        b->name_of_type_ = b->value->name_of_type(); \
                                                     \
        return true;                                 \
    }

    TRY("uint32_t");
    TRY("int32_t");
    TRY("uint64_t");
    TRY("int64_t");
    TRY("vtss_bool_t");
    TRY("string");
// TODO, handle strings and maybe null

#undef TRY

    return false;
}

bool TreeElementOpr::check(const expose::json::specification::Inventory &i,
                           Set<std::string> &json_vars) {
    // Operators requires that both its children has the same type. If this is
    // not the case, and one or more of the children is representing constants,
    // then try to convert the type of the constant(s) to an suitable type.
    //
    // Once both children has the same type, check that the requested operator
    // is implemented for the given type.

    DEFAULT(DEBUG) << "Check operator: " << opr;

    // Traverse the tree - this will ensure that the name_of_type_ is valid for
    // operator children
    if (child_lhs && !child_lhs->check(i, json_vars)) return false;
    if (child_rhs && !child_rhs->check(i, json_vars)) return false;

    // We can only have a type conflict if we have two children
    if (child_lhs && child_rhs) {
        auto type_lhs = child_lhs->type();
        auto type_rhs = child_rhs->type();

        if (type_lhs == TreeElement::CONST && type_rhs == TreeElement::CONST) {
            // Try negotiate a common type - that is more specific than
            // AnyJsonPrimitive
            auto a = static_cast<TreeElementConst *>(child_lhs);
            auto b = static_cast<TreeElementConst *>(child_rhs);
            if (!negotiate_common_types(a, b)) {
                return false;
            }

        } else if (type_lhs != TreeElement::CONST &&
                   type_rhs == TreeElement::CONST) {
            // See if child_rhs can be converted to the type of child_lhs
            auto *c = static_cast<TreeElementConst *>(child_rhs);
            str type_before_conversion = c->name_of_type();
            if (!c->convert_to(child_lhs)) {
                DEFAULT(INFO) << "Could not convert " << type_before_conversion
                              << " to " << child_lhs->name_of_type();
            }

        } else if (type_lhs == TreeElement::CONST &&
                   type_rhs != TreeElement::CONST) {
            // See if child_lhs can be converted to the type of chil_rhsd
            auto *c = static_cast<TreeElementConst *>(child_lhs);
            str type_before_conversion = c->name_of_type();
            if (!c->convert_to(child_rhs)) {
                DEFAULT(INFO) << "Could not convert " << type_before_conversion
                              << " to " << child_rhs->name_of_type();
            }
        }

        // We have tried to convert the types to match, lets see if we succeeded
        if (child_lhs->name_of_type() != AnyNull::NAME_OF_TYPE &&
            child_rhs->name_of_type() != AnyNull::NAME_OF_TYPE &&
            child_lhs->name_of_type() != child_rhs->name_of_type()) {
            DEFAULT(INFO) << "Types does not match: " << child_lhs->name_of_type()
                          << " != " << child_rhs->name_of_type();
            return false;
        }

    } else if (child_lhs && child_lhs->type() == TreeElement::CONST) {
        // If we only have a single child, then we still want it to be converted
        // to someting else than a generic.
        auto a = static_cast<TreeElementConst *>(child_lhs);
        a->convert_to(str("vtss_bool_t"));  // Dont care if this succedes or not
    }

    // Check if one of the children is enums. We need to know this as enums are
    // handled as int's internally, but we only allow the == and != operators on
    // enums.
    bool is_enum = false;
    if (child_lhs && child_lhs->type() == TreeElement::VAR) {
        auto s = static_cast<TreeElementVar *>(child_lhs);
        if (s->is_enum) is_enum = true;
    }
    if (child_rhs && child_rhs->type() == TreeElement::VAR) {
        auto s = static_cast<TreeElementVar *>(child_rhs);
        if (s->is_enum) is_enum = true;
    }
    if (is_enum) {
        switch (opr) {
        case Token::equal:
        case Token::not_equal:
            name_of_type_ = AnyBool::NAME_OF_TYPE;
            return true;

        default:
            return false;
        }
    }

    if (!child_lhs) {
        return false;
    }
    // At this point we have have settled on common and usable types. We are now
    // ready to check that we have the operator needed.
    auto operator_result_ptr =
            ANY_TYPE_INVENTORY.operator_result(child_lhs->name_of_type());

    if (!operator_result_ptr) {
        DEFAULT(INFO) << "has_operator function pointer was not registered for "
                         "type: "
                      << child_lhs->name_of_type();
        return false;
    }

    str result_type = operator_result_ptr(opr);
    if (result_type.size() == 0) {
        DEFAULT(INFO) << "The required operator: " << opr
                      << " is not implemented for type: "
                      << child_lhs->name_of_type();
        return false;
    }
    name_of_type_ = result_type;

    return true;
}

str TreeElementOpr::symbol() const {
    switch (opr) {
    case Token::and_:
        return str("&&");
    case Token::div:
        return str("/");
    case Token::end_p:
        return str(")");
    case Token::equal:
        return str("==");
    case Token::great:
        return str(">");
    case Token::great_equal:
        return str(">=");
    case Token::less:
        return str("<");
    case Token::less_equal:
        return str("<=");
    case Token::minus:
        return str("-");
    case Token::mult:
        return str("*");
    case Token::neg:
        return str("!");
    case Token::not_equal:
        return str("!=");
    case Token::or_:
        return str("||");
    case Token::plus:
        return str("+");
    case Token::start_p:
        return str("(");
    default:
        return str("<UNEXPECTED>");
    }
}

////////////////////////////////////////////////////////////////////////////////
TreeElementConst::TreeElementConst(int32_t x)
    : TreeElement(TreeElement::CONST) {
    value = std::make_shared<AnyJsonPrimitive>(x);
}

TreeElementConst::TreeElementConst(uint32_t x)
    : TreeElement(TreeElement::CONST) {
    value = std::make_shared<AnyJsonPrimitive>(x);
}

TreeElementConst::TreeElementConst(int64_t x)
    : TreeElement(TreeElement::CONST) {
    value = std::make_shared<AnyJsonPrimitive>(x);
}

TreeElementConst::TreeElementConst(uint64_t x)
    : TreeElement(TreeElement::CONST) {
    value = std::make_shared<AnyJsonPrimitive>(x);
}

TreeElementConst::TreeElementConst(const str &x)
    : TreeElement(TreeElement::CONST) {
    value = std::make_shared<AnyJsonPrimitive>(x);
}

TreeElementConst::TreeElementConst(const std::string &x)
    : TreeElement(TreeElement::CONST) {
    value = std::make_shared<AnyJsonPrimitive>(x);
}

TreeElementConst::TreeElementConst(std::string &&x)
    : TreeElement(TreeElement::CONST) {
    value = std::make_shared<AnyJsonPrimitive>(x);
}

TreeElementConst::TreeElementConst(bool x) : TreeElement(TreeElement::CONST) {
    value = std::make_shared<AnyJsonPrimitive>(x);
}

TreeElementConst::TreeElementConst(nullptr_t)
    : TreeElement(TreeElement::CONST) {
    value = std::make_shared<AnyJsonPrimitive>(nullptr);
}

AnySharedPtr TreeElementConst::eval(const Map<str, std::string> &bindings) {
    return value;
}

bool TreeElementConst::check(const expose::json::specification::Inventory &i,
                             Set<std::string> &json_vars) {
    name_of_type_ = value->name_of_type();
    return true;
}

bool TreeElementConst::convert_to(const TreeElement *type) {
    using namespace expose::json::specification;

    AnyPtr v;
    std::shared_ptr<TypeDescriptor> td;
    if (type->type() == TreeElement::VAR) {
        auto *x = static_cast<const TreeElementVar *>(type);
        td = x->type_descriptor;
    } else if (type->type() == TreeElement::CONST) {
        auto *x = static_cast<const TreeElementConst *>(type);
        td = x->type_descriptor;
    }

    if (td && td->type_class == TypeClass::Enum) {
        DEFAULT(DEBUG) << "convert_to (TreeElement) - enum";
        auto *d = static_cast<TypeDescriptorEnum *>(td.get());
        v = copy_to(d);

    } else {
        DEFAULT(DEBUG) << "convert_to (TreeElement) - " << type->name_of_type();
        v = copy_to(type->name_of_type());
    }

    if (v) {
        value = vtss::move(v);
        name_of_type_ = value->name_of_type();
        if (td && td->type_class == TypeClass::Enum) {
            type_descriptor = td;
        }
        return true;
    }

    return false;
}

bool TreeElementConst::convert_to(const TypeDescriptorPtr td) {
    using namespace expose::json::specification;
    AnyPtr v;

    DEFAULT(DEBUG) << "Convert to (TypeDescriptorPtr): " << td->name << "("
                   << td->type_class << ")";

    if (td->type_class == TypeClass::Enum) {
        auto *d = static_cast<TypeDescriptorEnum *>(td.get());
        v = copy_to(d);

    } else {
        v = copy_to(td->name);
    }

    if (v) {
        value = vtss::move(v);
        name_of_type_ = value->name_of_type();
        if (td && td->type_class == TypeClass::Enum) {
            type_descriptor = td;
        }
        return true;
    }

    return false;
}

bool TreeElementConst::convert_to(str type) {
    DEFAULT(DEBUG) << "Convert to: " << type;

    auto x = copy_to(type);
    if (x) {
        value = vtss::move(x);
        name_of_type_ = value->name_of_type();
        return true;
    }

    return false;
}

AnyPtr TreeElementConst::copy_to(str type) {
    auto *p = value.get();
    if (!p) return AnyPtr(nullptr);

    DEFAULT(INFO) << "Trying to copy construct a " << value->name_of_type()
                  << " into a " << type;
    if (p->name_of_type() != AnyJsonPrimitive::NAME_OF_TYPE) {
        DEFAULT(INFO) << "Unexpected type";
        return AnyPtr(nullptr);
    }

    auto *j = static_cast<const AnyJsonPrimitive *>(p);
    if (j->type() == AnyJsonPrimitive::NULL_) {
        DEFAULT(INFO) << "Null type returned";
        return make_unique<AnyNull>();
    }

    return ANY_TYPE_INVENTORY.construct(type, *j);
}

AnyPtr TreeElementConst::copy_to(
        expose::json::specification::TypeDescriptorEnum *type) {
    auto *p = value.get();
    if (!p) return AnyPtr(nullptr);

    DEFAULT(INFO) << "Trying to copy construct a " << value->name_of_type()
                  << " into a (enum)" << type->name;

    if (p->name_of_type() != AnyJsonPrimitive::NAME_OF_TYPE) {
        DEFAULT(INFO) << "Unexpected type";
        return AnyPtr(nullptr);
    }

    auto *j = static_cast<const AnyJsonPrimitive *>(p);
    if (j->type() == AnyJsonPrimitive::NULL_) {
        DEFAULT(INFO) << "Null type returned";
        return make_unique<AnyNull>();
    }

    if (j->type() != AnyJsonPrimitive::STR) {
        DEFAULT(WARNING) << "Only strings can be converted to enums";
        return nullptr;
    }

    for (const auto &e : type->elements) {
        if (str(e.name) != j->as_str()) continue;
        DEFAULT(DEBUG) << "Enum " << e.name << " to int: " << e.value;
        return make_unique<AnyInt32>(e.value);
    }

    DEFAULT(WARNING) << "Failed to convert: " << j->as_str()
                     << " to a enum of type " << type;
    return nullptr;
}

}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss
