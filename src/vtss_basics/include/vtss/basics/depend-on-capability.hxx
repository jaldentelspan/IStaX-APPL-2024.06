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

#ifndef __VTSS_BASICS_DEPEND_ON_CAPABILITY_HXX__
#define __VTSS_BASICS_DEPEND_ON_CAPABILITY_HXX__

#include "vtss/basics/meta.hxx"
#include "vtss/basics/utility.hxx"

namespace vtss {
namespace tag {

typedef bool(*capability_get_ptr_t)();

struct DependOnCapabilityAbstract {
    virtual bool get() = 0;
    virtual const char *name() = 0;
    virtual const char *json_ref() = 0;
};

template <typename T>
struct DependOnCapabilityImpl : public DependOnCapabilityAbstract {
    bool get() { return T::get(); }
    const char *name() { return T::name(); }
    const char *json_ref() { return T::json_ref(); }
};

template <typename T>
struct DependOnCapability {
    static bool get() { return T::get(); }
    static const char *name() { return T::name; }
    static const char *json_ref() { return T::json_ref; }
};

namespace DetailsDependOnCapability {

template <typename T>
struct is_a {
    static constexpr bool val = false;
};

template <typename T>
struct is_a<DependOnCapability<T>> {
    static constexpr bool val = true;
};

////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct void_ { typedef void type; };

template<typename T, typename = void>
struct default_or_type {
    static bool get() {
        return true;
    }

    static capability_get_ptr_t get_ptr() {
        return nullptr;
    }

    static const char *name() {
        return nullptr;
    }

    static const char *json_ref() {
        return nullptr;
    }

    DependOnCapabilityAbstract *create_abstract() {
        return nullptr;
    }
};

template<typename T>
struct default_or_type <T, typename void_<typename T::depends_on_t>::type> {
    static bool get() {
        return T::depends_on_t::get();
    }

    static capability_get_ptr_t get_ptr() {
        return &(T::depends_on_t::get);
    }

    static const char *name() {
        return T::depends_on_t::name;
    }

    static const char *json_ref() {
        return T::depends_on_t::json_ref;
    }

    DependOnCapabilityAbstract *create_abstract() {
        return new DependOnCapabilityImpl<T>();
    }
};


////////////////////////////////////////////////////////////////////////////////
template <typename... Args>
bool check(const Args... args);

template <>
inline bool check() {
    return true;
}

template <typename Head, typename... Tail>
typename meta::enable_if<is_a<Head>::val, bool>::type check(
        const Head, const Tail... tail) {
    return Head::get();
}

template <typename Head, typename... Tail>
typename meta::enable_if<!is_a<Head>::val, bool>::type check(
        const Head, const Tail... tail) {
    return check(forward<const Tail>(tail)...);
}

////////////////////////////////////////////////////////////////////////////////
template <typename... Args>
const char *name(const Args... args);

template <>
inline const char *name() {
    return nullptr;
}

template <typename Head, typename... Tail>
typename meta::enable_if<is_a<Head>::val, const char *>::type name(
        const Head, const Tail... tail) {
    return Head::name();
}

template <typename Head, typename... Tail>
typename meta::enable_if<!is_a<Head>::val, const char *>::type name(
        const Head, const Tail... tail) {
    return name(forward<const Tail>(tail)...);
}

////////////////////////////////////////////////////////////////////////////////
template <typename... Args>
const char *json_ref(const Args... args);

template <>
inline const char *json_ref() {
    return nullptr;
}

template <typename Head, typename... Tail>
typename meta::enable_if<is_a<Head>::val, const char *>::type json_ref(
        const Head, const Tail... tail) {
    return Head::json_ref();
}

template <typename Head, typename... Tail>
typename meta::enable_if<!is_a<Head>::val, const char *>::type json_ref(
        const Head, const Tail... tail) {
    return json_ref(forward<const Tail>(tail)...);
}
}  // namespace DetailsDependOnCapability

template <typename... Args>
bool check_depend_on_capability(const Args... args) {
    return DetailsDependOnCapability::check(forward<const Args>(args)...);
}

template <typename IF>
bool check_depend_on_capability_t() {
    return DetailsDependOnCapability::default_or_type<IF>::get();
}

template <typename... Args>
const char *depend_on_capability_name(const Args... args) {
    return DetailsDependOnCapability::name(forward<const Args>(args)...);
}

template <typename IF>
const char *depend_on_capability_name_t() {
    return DetailsDependOnCapability::default_or_type<IF>::name();
}

template <typename... Args>
const char *depend_on_capability_json_ref(const Args... args) {
    return DetailsDependOnCapability::json_ref(forward<const Args>(args)...);
}

template <typename IF>
const char *depend_on_capability_json_ref_t() {
    return DetailsDependOnCapability::default_or_type<IF>::json_ref();
}

template <typename IF>
capability_get_ptr_t depend_on_capability_get_ptr() {
    return DetailsDependOnCapability::default_or_type<IF>::get_ptr();
}

}  // namespace tag
}  // namespace vtss

#endif  // __VTSS_BASICS_DEPEND_ON_CAPABILITY_HXX__
