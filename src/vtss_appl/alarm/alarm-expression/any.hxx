/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_APPL_ALARM_EXPRESSION_ANY_HXX__
#define __VTSS_APPL_ALARM_EXPRESSION_ANY_HXX__

#include <memory>
#include <vtss/basics/map.hxx>
#include <vtss/basics/string.hxx>
#include <vtss/basics/stream.hxx>

#include "alarm-expression/token.hxx"

namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

struct Any;

typedef std::unique_ptr<Any> AnyPtr;
typedef std::shared_ptr<Any> AnySharedPtr;

// Base class for a value of unknown type.
struct Any {
    // Implements the operators for a given type.
    // opr:           The operator to evaluate
    //
    // result_buffer: On object of same type that should be used to store the
    //                result, unless it is a null pointer.
    //
    //                NOTE: The result_buffer may be a pointer to the same
    //                object as 'this', or the same object as 'rhs'. It it
    //                therefore important that 'this' and 'rhs' is not accessed
    //                after 'result_buffer' is updated.
    //
    // rhs:           Right-hand-side of the operator ('this' is the left
    //                handside). This may be a nullpointer, so check before use.
    //
    // return:        The result is returned. If the 'result_buffer' is being
    //                used, then it must be returned.
    virtual AnySharedPtr opr(Token opr, AnySharedPtr result_buffer,
                             AnySharedPtr rhs) const = 0;

    // Return the name of represented type
    virtual str name_of_type() const = 0;

    virtual ~Any() {}
};

struct AnyJsonPrimitive : public Any {
    enum Type { NULL_, U32, U64, I32, I64, BOOL, STR };

    AnyJsonPrimitive() : type_(NULL_) {}
    AnyJsonPrimitive(nullptr_t) : type_(NULL_) {}
    AnyJsonPrimitive(bool x) : type_(BOOL) { value.b = x; }
    AnyJsonPrimitive(uint32_t x) : type_(U32) { value.u32 = x; }
    AnyJsonPrimitive(uint64_t x) : type_(U64) { value.u64 = x; }
    AnyJsonPrimitive(int32_t x) : type_(I32) { value.i32 = x; }
    AnyJsonPrimitive(int64_t x) : type_(I64) { value.i64 = x; }
    AnyJsonPrimitive(str s) : value_str(s.begin(), s.end()), type_(STR) {}
    AnyJsonPrimitive(const std::string &s) : value_str(s), type_(STR) {}
    AnyJsonPrimitive(std::string &&s) : value_str(s), type_(STR) {}

    str name_of_type() const override { return NAME_OF_TYPE; }
    static str operator_result(Token t) { return str(); }
    Type type() const { return type_; }
    bool as_bool() const { return value.b; }
    uint32_t as_uint32() const { return value.u32; }
    uint64_t as_uint64() const { return value.u64; }
    int32_t as_int32() const { return value.i32; }
    int64_t as_int64() const { return value.i64; }
    str as_str() const { return str(&*value_str.begin(), value_str.c_str() + value_str.size()); }

    AnySharedPtr opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;

  private:
    union Value {
        bool b;
        uint32_t u32;
        uint64_t u64;
        int32_t i32;
        int64_t i64;
    } value;

    std::string value_str;
    Type type_ = NULL_;
};

struct AnyBool : public Any {
    AnyBool(bool x) : value(x) {}
    static AnyPtr construct_from(const AnyJsonPrimitive &a);
    static str operator_result(Token t);
    str name_of_type() const override { return NAME_OF_TYPE; }
    AnySharedPtr opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
    bool value;
};

struct AnyStr final : public Any {
    AnyStr(str s);
    AnyStr(std::string x);
    static AnyPtr construct_from(const AnyJsonPrimitive &a);
    static str operator_result(Token t);
    str name_of_type() const override { return NAME_OF_TYPE; }
    AnySharedPtr opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
    std::string value;
};

struct AnyNull : public Any {
    static AnyPtr construct_from(const AnyJsonPrimitive &a);
    static str operator_result(Token t) { return str(); }
    str name_of_type() const override { return NAME_OF_TYPE; }
    AnySharedPtr opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
};


struct AnyInt16 : public Any {
    AnyInt16(int16_t x) : value(x) {}
    static AnyPtr construct_from(const AnyJsonPrimitive &a);
    static str operator_result(Token t);
    str name_of_type() const override { return NAME_OF_TYPE; }
    AnySharedPtr opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
    int16_t value;
};

struct AnyInt32 : public Any {
    AnyInt32(int32_t x) : value(x) {}
    static AnyPtr construct_from(const AnyJsonPrimitive &a);
    static str operator_result(Token t);
    str name_of_type() const override { return NAME_OF_TYPE; }
    AnySharedPtr opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
    int32_t value;
};

struct AnyInt64 : public Any {
    AnyInt64(int64_t x) : value(x) {}
    static AnyPtr construct_from(const AnyJsonPrimitive &a);
    static str operator_result(Token t);
    str name_of_type() const override { return NAME_OF_TYPE; }
    AnySharedPtr opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
    int64_t value;
};

struct AnyUint8 : public Any {
    AnyUint8(uint8_t x) : value(x) {}
    static AnyPtr construct_from(const AnyJsonPrimitive &a);
    static str operator_result(Token t);
    str name_of_type() const override { return NAME_OF_TYPE; }
    AnySharedPtr opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
    uint16_t value;
};


struct AnyUint16 : public Any {
    AnyUint16(uint16_t x) : value(x) {}
    static AnyPtr construct_from(const AnyJsonPrimitive &a);
    static str operator_result(Token t);
    str name_of_type() const override { return NAME_OF_TYPE; }
    AnySharedPtr opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
    uint16_t value;
};

struct AnyUint32 : public Any {
    AnyUint32(uint32_t x) : value(x) {}
    static AnyPtr construct_from(const AnyJsonPrimitive &a);
    static str operator_result(Token t);
    str name_of_type() const override { return NAME_OF_TYPE; }
    AnySharedPtr opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
    uint32_t value;
};

struct AnyUint64 : public Any {
    AnyUint64(uint64_t x) : value(x) {}
    static AnyPtr construct_from(const AnyJsonPrimitive &a);
    static str operator_result(Token t);
    str name_of_type() const override { return NAME_OF_TYPE; }
    AnySharedPtr opr(Token opr, AnySharedPtr res, AnySharedPtr rhs) const override;

    static const str NAME_OF_TYPE;
    uint64_t value;
};

// TODO, locking
struct AnyTypeInventory {
    typedef AnyPtr (*Create)(const AnyJsonPrimitive &a);
    typedef str (*OperatorResult)(Token t);

    struct StaticOperators {
        StaticOperators(Create c, OperatorResult h)
            : create(c), operator_result(h) {}
        Create create;
        OperatorResult operator_result;
    };

    OperatorResult operator_result(str type);

    template <typename T>
    void add_type() {
        inventory.emplace(
                vtss::piecewise_construct,
                vtss::forward_as_tuple(str(T::NAME_OF_TYPE)),
                vtss::forward_as_tuple(&T::construct_from, &T::operator_result));
    }

    AnyTypeInventory();
    AnyPtr construct(str type, const AnyJsonPrimitive &a);
    Map<str, StaticOperators> inventory;
};

extern AnyTypeInventory ANY_TYPE_INVENTORY;

template <typename T>
void any_add_type() {
    ANY_TYPE_INVENTORY.add_type<T>();
}

}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#endif  // __VTSS_APPL_ALARM_EXPRESSION_ANY_HXX__
