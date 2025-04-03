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

#ifndef __VTSS_BASICS_EXPOSE_JSON_SERIALIZE_CLASS_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_SERIALIZE_CLASS_HXX__
#include "vtss/basics/api_types.h"
#include "vtss/basics/expose/json/json-type.hxx"
#include "vtss/basics/expose/json/serialize.hxx"
#include "vtss/basics/expose/json/serialize-enum.hxx"
#include "vtss/basics/expose/json/type-class-enum.hxx"
#include "vtss/basics/expose/json/type-class-unknown.hxx"

namespace vtss {
namespace expose {
namespace json {

// Hack to overcome the partical speciliazion limitation
template<typename Handler, typename Value, typename TypeClass>
struct SerializeClass;

// matches all TypeClassEnum
template<typename Value>
struct SerializeClass<HandlerReflector, Value, TypeClassEnum> {
    void operator()(HandlerReflector &h, Value &v) {
        serialize_enum(h, JsonType<Value>::type_name(),
                       JsonType<Value>::type_descr(),
                       JsonType<Value>::enum_descriptor());
    }
};

template<typename Handler, typename Value>
struct SerializeClass<Handler, Value, TypeClassEnum> {
    void operator()(Handler &h, Value &v) {
        // delete type information and treat enums as ints from here
        int32_t &tmp = reinterpret_cast<int32_t &>(v);

        // JsonType must provide a enum_descriptor
        serialize_enum(h, tmp, JsonType<Value>::enum_descriptor());
    }
};

// matches all TypeClassUnknown
template<typename Handler, typename Value>
struct SerializeClass<Handler, Value, TypeClassUnknown> {
    void operator()(Handler &h, Value &v) { serialize(h, v); }
};

// wrapper function to call the class aware serialize function
template<typename Handler, typename Value>
void serialize_class(Handler &handler, Value &value) {
    typedef typename JsonType<Value>::type_class_t Class;
    SerializeClass<Handler, Value, Class> s;
    s(handler, value);
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_SERIALIZE_CLASS_HXX__
