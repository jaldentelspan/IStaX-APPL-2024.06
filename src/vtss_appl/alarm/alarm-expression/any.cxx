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

#include "alarm-expression/any.hxx"
#include "alarm-trace.h"
#include <vtss/basics/trace.hxx>
#include <vtss/basics/memory.hxx>

namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

namespace {
str has_opr_arith(Token t, str same_type) {
    switch (t) {
    case Token::div:
    case Token::minus:
    case Token::mult:
    case Token::plus:
        return same_type;

    case Token::equal:
    case Token::great:
    case Token::great_equal:
    case Token::less:
    case Token::less_equal:
    case Token::not_equal:
        return str("vtss_bool_t");

    default:
        return str();
    }
}

template <typename T>
AnySharedPtr int_opr(const T *lhs, Token opr, AnySharedPtr res, AnySharedPtr rhs) {
    if (rhs && lhs->name_of_type() != rhs->name_of_type()) {
        DEFAULT(ERROR) << "Invalid type: " << lhs->name_of_type()
                       << " != " << rhs->name_of_type();
        return nullptr;
    }

    switch (opr) {
    case Token::div:
    case Token::equal:
    case Token::great:
    case Token::great_equal:
    case Token::less:
    case Token::less_equal:
    case Token::minus:
    case Token::mult:
    case Token::not_equal:
    case Token::plus:
        if (!rhs) {
            DEFAULT(ERROR) << "Missing argument";
            return nullptr;
        }
        break;

    default:;
    }

    auto rhs_ = std::static_pointer_cast<T>(rhs);

    bool b;
    decltype(lhs->value) i;
    switch (opr) {
    case Token::div:
        i = ((lhs->value) / (rhs_->value));
        break;

    case Token::minus:
        i = lhs->value - rhs_->value;
        break;

    case Token::mult:
        i = lhs->value * rhs_->value;
        break;

    case Token::plus:
        i = lhs->value + rhs_->value;
        break;

    // -----------------------------------
    case Token::equal:
        b = lhs->value == rhs_->value;
        break;

    case Token::great:
        b = lhs->value > rhs_->value;
        break;

    case Token::great_equal:
        b = lhs->value >= rhs_->value;
        break;

    case Token::less:
        b = lhs->value < rhs_->value;
        break;

    case Token::less_equal:
        b = lhs->value <= rhs_->value;
        break;

    case Token::not_equal:
        b = lhs->value != rhs_->value;
        break;

    default:
        // Can not be reached
        return nullptr;
    }

    switch (opr) {
    case Token::div:
    case Token::minus:
    case Token::mult:
    case Token::plus:
        if (res) {
            auto res_ = std::static_pointer_cast<T>(res);
            res_->value = i;
            return res;
        } else {
            return make_unique<T>(i);
        }
        break;

    case Token::equal:
    case Token::great:
    case Token::great_equal:
    case Token::less:
    case Token::less_equal:
    case Token::not_equal:
        return make_unique<AnyBool>(b);

    default:
        // Can not be reached
        return nullptr;
    }

    return nullptr;
}
}  // namespace

////////////////////////////////////////////////////////////////////////////////
const str AnyJsonPrimitive::NAME_OF_TYPE = str("GenericJson");

AnySharedPtr AnyJsonPrimitive::opr(Token opr, AnySharedPtr res,
                                   AnySharedPtr rhs) const {
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
const str AnyBool::NAME_OF_TYPE = str("vtss_bool_t");

AnyPtr AnyBool::construct_from(const AnyJsonPrimitive &a) {
    DEFAULT(DEBUG) << "Construct AnyBool from " << a.type();
    if (a.type() == AnyJsonPrimitive::BOOL) {
        return make_unique<AnyBool>(a.as_bool());
    }
    return AnyPtr(nullptr);
}

str AnyBool::operator_result(Token t) {
    switch (t) {
    case Token::and_:
    case Token::equal:
    case Token::neg:
    case Token::not_equal:
    case Token::or_:
        return NAME_OF_TYPE;

    default:
        return str();
    }
}

AnySharedPtr AnyBool::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    if (rhs && name_of_type() != rhs->name_of_type()) {
        DEFAULT(ERROR) << "Invalid type: " << name_of_type()
                       << " != " << rhs->name_of_type();
        return nullptr;
    }

    switch (opr) {
    case Token::and_:
    case Token::or_:
    case Token::equal:
    case Token::not_equal:
        if (!rhs) {
            DEFAULT(ERROR) << "Missing argument: " << opr;
            return AnySharedPtr(nullptr);
        }
        break;

    default:;
    }

    auto rhs_ = std::static_pointer_cast<AnyBool>(rhs);

    bool b;
    switch (opr) {
    case Token::and_:
        b = (value && rhs_->value);
        DEFAULT(DEBUG) << value << " && " << rhs_->value << " -> " << b;
        break;

    case Token::or_:
        b = (value || rhs_->value);
        DEFAULT(DEBUG) << value << " || " << rhs_->value << " -> " << b;
        break;

    case Token::equal:
        b = (value == rhs_->value);
        DEFAULT(DEBUG) << value << " == " << rhs_->value << " -> " << b;
        break;

    case Token::not_equal:
        b = (value != rhs_->value);
        DEFAULT(DEBUG) << value << " != " << rhs_->value << " -> " << b;
        break;

    case Token::neg:
        b = !value;
        DEFAULT(DEBUG) << "!" << value << " -> " << b;
        break;

    default:
        // Can not be reached
        return AnySharedPtr(nullptr);
    }

    if (res) {
        auto res_ = std::static_pointer_cast<AnyBool>(res);
        res_->value = b;
        return res;
    } else {
        return make_unique<AnyBool>(b);
    }
}

////////////////////////////////////////////////////////////////////////////////
const str AnyStr::NAME_OF_TYPE = str("string");

AnyStr::AnyStr(std::string x) : value(x) {}
AnyStr::AnyStr(str s) : value(s.begin(), s.end()) {}

AnyPtr AnyStr::construct_from(const AnyJsonPrimitive &a) {
    if (a.type() == AnyJsonPrimitive::STR) {
        return make_unique<AnyStr>(a.as_str());
    }
    return AnyPtr(nullptr);
}

str AnyStr::operator_result(Token t) {
    switch (t) {
    case Token::equal:
    case Token::not_equal:
        return AnyBool::NAME_OF_TYPE;

    default:
        return str();
    }
}

AnySharedPtr AnyStr::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    if (!rhs) {
        DEFAULT(ERROR) << "No 'rhs'";
        return nullptr;
    }

    if (name_of_type() != rhs->name_of_type()) {
        DEFAULT(ERROR) << "Invalid type: " << name_of_type()
                       << " != " << rhs->name_of_type();
        return nullptr;
    }

    auto rhs_ = std::static_pointer_cast<AnyStr>(rhs);

    bool b;
    switch (opr) {
    case Token::equal:
        b = (value == rhs_->value);
        DEFAULT(DEBUG) << value << " == " << rhs_->value << " -> " << b;
        break;

    case Token::not_equal:
        b = (value != rhs_->value);
        DEFAULT(DEBUG) << value << " != " << rhs_->value << " -> " << b;
        break;

    default:
        // Can not be reached
        return AnySharedPtr(nullptr);
    }

    return make_unique<AnyBool>(b);
}

////////////////////////////////////////////////////////////////////////////////
const str AnyNull::NAME_OF_TYPE = str("nullptr_t");

AnyPtr AnyNull::construct_from(const AnyJsonPrimitive &a) {
    return AnyPtr(nullptr);
}

AnySharedPtr AnyNull::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    return std::make_shared<AnyNull>();
}

////////////////////////////////////////////////////////////////////////////////
const str AnyInt16::NAME_OF_TYPE = str("int16_t");

AnyPtr AnyInt16::construct_from(const AnyJsonPrimitive &a) {
    DEFAULT(DEBUG) << "Construct AnyInt16";
    switch (a.type()) {
    case AnyJsonPrimitive::U32:
        if (a.as_uint32() <= 32767) {
            return make_unique<AnyInt16>(a.as_uint32());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::U64:
        if (a.as_uint64() <= 32767) {
            return make_unique<AnyInt16>(a.as_uint64());
        } else {
            return AnyPtr(nullptr);
        }
    case AnyJsonPrimitive::I32:
        if (a.as_int32() >= -32768 && a.as_int32() <= 32767) {
            return make_unique<AnyInt16>(a.as_int32());
        } else {
            return AnyPtr(nullptr);
        }
    case AnyJsonPrimitive::I64:
        if (a.as_int64() >= -32768 && a.as_int64() <= 32767) {
            return make_unique<AnyInt16>(a.as_int64());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::NULL_:
    case AnyJsonPrimitive::BOOL:
    case AnyJsonPrimitive::STR:
    default:
        return AnyPtr(nullptr);
    }
}

str AnyInt16::operator_result(Token t) {
    return has_opr_arith(t, NAME_OF_TYPE);
}

AnySharedPtr AnyInt16::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    return int_opr(this, opr, res, rhs);
}


////////////////////////////////////////////////////////////////////////////////
const str AnyInt32::NAME_OF_TYPE = str("int32_t");

AnyPtr AnyInt32::construct_from(const AnyJsonPrimitive &a) {
    DEFAULT(DEBUG) << "Construct AnyInt32";
    switch (a.type()) {
    case AnyJsonPrimitive::U32:
        if (a.as_uint32() <= 2147483647) {
            return make_unique<AnyInt32>(a.as_uint32());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::U64:
        if (a.as_uint64() <= 2147483647) {
            return make_unique<AnyInt32>(a.as_uint64());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::I32:
        return make_unique<AnyInt32>(a.as_int32());

    case AnyJsonPrimitive::I64:
        if (a.as_int64() >= -2147483648 && a.as_int64() <= 2147483647) {
            return make_unique<AnyInt32>(a.as_int64());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::NULL_:
    case AnyJsonPrimitive::BOOL:
    case AnyJsonPrimitive::STR:
    default:
        return AnyPtr(nullptr);
    }
}

str AnyInt32::operator_result(Token t) {
    return has_opr_arith(t, NAME_OF_TYPE);
}

AnySharedPtr AnyInt32::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    return int_opr(this, opr, res, rhs);
}

////////////////////////////////////////////////////////////////////////////////
const str AnyInt64::NAME_OF_TYPE = str("int64_t");

AnyPtr AnyInt64::construct_from(const AnyJsonPrimitive &a) {
    DEFAULT(DEBUG) << "Construct AnyInt64";
    switch (a.type()) {
    case AnyJsonPrimitive::U32:
        return make_unique<AnyInt64>(a.as_uint32());

    case AnyJsonPrimitive::U64:
        if (a.as_uint64() <= 9223372036854775807LL) {
            return make_unique<AnyInt64>(a.as_uint64());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::I32:
        return make_unique<AnyInt64>(a.as_int32());

    case AnyJsonPrimitive::I64:
        return make_unique<AnyInt64>(a.as_int64());

    case AnyJsonPrimitive::NULL_:
    case AnyJsonPrimitive::BOOL:
    case AnyJsonPrimitive::STR:
    default:
        return AnyPtr(nullptr);
    }
}

str AnyInt64::operator_result(Token t) {
    return has_opr_arith(t, NAME_OF_TYPE);
}

AnySharedPtr AnyInt64::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    return int_opr(this, opr, res, rhs);
}

////////////////////////////////////////////////////////////////////////////////
const str AnyUint8::NAME_OF_TYPE = str("uint8_t");

AnyPtr AnyUint8::construct_from(const AnyJsonPrimitive &a) {
    DEFAULT(DEBUG) << "Construct AnyUint8";
    switch (a.type()) {
    case AnyJsonPrimitive::U32:
        if (a.as_uint32() <= 255) {
            return make_unique<AnyUint8>(a.as_uint32());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::U64:
        if (a.as_uint64() <= 255) {
            return make_unique<AnyUint8>(a.as_uint64());
        } else {
            return AnyPtr(nullptr);
        }
    case AnyJsonPrimitive::I32:
        if (a.as_int32() >= 0 && a.as_int32() <= 255) {
            return make_unique<AnyUint8>(a.as_int32());
        } else {
            return AnyPtr(nullptr);
        }
    case AnyJsonPrimitive::I64:
        if (a.as_int64() >= 255 && a.as_int64() <= 0) {
            return make_unique<AnyUint8>(a.as_int64());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::NULL_:
    case AnyJsonPrimitive::BOOL:
    case AnyJsonPrimitive::STR:
    default:
        return AnyPtr(nullptr);
    }
}

str AnyUint8::operator_result(Token t) {
    return has_opr_arith(t, NAME_OF_TYPE);
}

AnySharedPtr AnyUint8::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    return int_opr(this, opr, res, rhs);
}

////////////////////////////////////////////////////////////////////////////////
const str AnyUint16::NAME_OF_TYPE = str("uint16_t");

AnyPtr AnyUint16::construct_from(const AnyJsonPrimitive &a) {
    DEFAULT(DEBUG) << "Construct AnyUint16";
    switch (a.type()) {
    case AnyJsonPrimitive::U32:
        if (a.as_uint32() <= 65535) {
            return make_unique<AnyUint16>(a.as_uint32());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::U64:
        if (a.as_uint64() <= 65535) {
            return make_unique<AnyUint16>(a.as_uint64());
        } else {
            return AnyPtr(nullptr);
        }
    case AnyJsonPrimitive::I32:
        if (a.as_int32() >= 0 && a.as_int32() <= 65535) {
            return make_unique<AnyUint16>(a.as_int32());
        } else {
            return AnyPtr(nullptr);
        }
    case AnyJsonPrimitive::I64:
        if (a.as_int64() >= 65535 && a.as_int64() <= 0) {
            return make_unique<AnyUint16>(a.as_int64());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::NULL_:
    case AnyJsonPrimitive::BOOL:
    case AnyJsonPrimitive::STR:
    default:
        return AnyPtr(nullptr);
    }
}

str AnyUint16::operator_result(Token t) {
    return has_opr_arith(t, NAME_OF_TYPE);
}

AnySharedPtr AnyUint16::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    return int_opr(this, opr, res, rhs);
}

////////////////////////////////////////////////////////////////////////////////
const str AnyUint32::NAME_OF_TYPE = str("uint32_t");

AnyPtr AnyUint32::construct_from(const AnyJsonPrimitive &a) {
    DEFAULT(DEBUG) << "Construct AnyUint32";
    switch (a.type()) {
    case AnyJsonPrimitive::U32:
        return make_unique<AnyUint32>(a.as_uint32());

    case AnyJsonPrimitive::U64:
        if (a.as_uint64() <= 4294967295) {
            return make_unique<AnyUint32>(a.as_uint64());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::I32:
        if (a.as_int32() >= 0) {
            return make_unique<AnyUint32>(a.as_int32());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::I64:
        if (a.as_int64() >= 0 && a.as_int64() <= 4294967295) {
            return make_unique<AnyUint32>(a.as_int64());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::NULL_:
    case AnyJsonPrimitive::BOOL:
    case AnyJsonPrimitive::STR:
    default:
        return AnyPtr(nullptr);
    }
}

str AnyUint32::operator_result(Token t) {
    return has_opr_arith(t, NAME_OF_TYPE);
}

AnySharedPtr AnyUint32::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    return int_opr(this, opr, res, rhs);
}

////////////////////////////////////////////////////////////////////////////////
const str AnyUint64::NAME_OF_TYPE = str("uint64_t");

AnyPtr AnyUint64::construct_from(const AnyJsonPrimitive &a) {
    DEFAULT(DEBUG) << "Construct AnyUint64";
    switch (a.type()) {
    case AnyJsonPrimitive::U32:
        return make_unique<AnyUint64>(a.as_uint32());

    case AnyJsonPrimitive::U64:
        return make_unique<AnyUint64>(a.as_uint64());

    case AnyJsonPrimitive::I32:
        return make_unique<AnyUint64>(a.as_int32());

    case AnyJsonPrimitive::I64:
        if (a.as_int64() >= 0) {
            return make_unique<AnyUint64>(a.as_int64());
        } else {
            return AnyPtr(nullptr);
        }

    case AnyJsonPrimitive::NULL_:
    case AnyJsonPrimitive::BOOL:
    case AnyJsonPrimitive::STR:
    default:
        return AnyPtr(nullptr);
    }
}

str AnyUint64::operator_result(Token t) {
    return has_opr_arith(t, NAME_OF_TYPE);
}

AnySharedPtr AnyUint64::opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const {
    return int_opr(this, opr, res, rhs);
}

////////////////////////////////////////////////////////////////////////////////
AnyTypeInventory ANY_TYPE_INVENTORY;
AnyTypeInventory::AnyTypeInventory() {
    any_add_type<AnyInt16>();
    any_add_type<AnyInt32>();
    any_add_type<AnyInt64>();
    any_add_type<AnyUint8>();
    any_add_type<AnyUint16>();
    any_add_type<AnyUint32>();
    any_add_type<AnyUint64>();
    any_add_type<AnyBool>();
    any_add_type<AnyNull>();
    any_add_type<AnyStr>();
}

AnyTypeInventory::OperatorResult AnyTypeInventory::operator_result(str type) {
    auto i = inventory.find(type);
    if (i == inventory.end()) return nullptr;
    return i->second.operator_result;
}

AnyPtr AnyTypeInventory::construct(str type, const AnyJsonPrimitive &a) {
    auto i = inventory.find(type);
    if (i == inventory.end()) {
        DEFAULT(WARNING) << "No Any type registered for: " << type;
        return nullptr;
    }
    // return i->second.create(type); PFH fix
    return i->second.create(a);
}

////////////////////////////////////////////////////////////////////////////////

}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss
