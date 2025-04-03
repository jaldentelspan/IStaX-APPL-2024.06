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

#ifndef _META_H_
#define _META_H_

#include "vtss/basics/common.hxx"
#include "vtss/basics/type_traits.hxx"

namespace vtss {
namespace meta {

struct _true {};
struct _false {};

template <typename C, typename A, typename B>
struct _if;
template <typename A, typename B>
struct _if<_true, A, B> {
    typedef A type;
};
template <typename A, typename B>
struct _if<_false, A, B> {
    typedef B type;
};

template <bool C, typename A, typename B>
struct __if;
template <typename A, typename B>
struct __if<true, A, B> {
    typedef A type;
};
template <typename A, typename B>
struct __if<false, A, B> {
    typedef B type;
};

template <typename T1, typename T2>
struct AssertTypeEqual;
template <typename T1>
struct AssertTypeEqual<T1, T1> {};

template <typename T1, typename T2>
struct equal_type {
    typedef _false type;
    static constexpr bool value = false;
};
template <typename _T>
struct equal_type<_T, _T> {
    typedef _true type;
    static constexpr bool value = true;
};

template <bool C, typename T = void>
struct enable_if {};
template <typename T>
struct enable_if<true, T> {
    using type = T;
};

template <typename _T>
struct remove_const {
    typedef _T type;
};
template <typename _T>
struct remove_const<const _T> {
    typedef _T type;
};

template <typename _T>
struct add_const {
    typedef const _T type;
};
template <typename _T>
struct add_const<const _T> {
    typedef const _T type;
};
template <typename _T>
struct add_const<_T *> {
    typedef const _T *type;
};
template <typename _T>
struct add_const<const _T *> {
    typedef const _T *type;
};
template <typename _T>
struct add_const<_T &> {
    typedef const _T &type;
};
template <typename _T>
struct add_const<const _T &> {
    typedef const _T &type;
};

template <typename _T>
struct IntTraits;

struct Signed {};
struct Unsigned {};

template <>
struct IntTraits<int8_t> {
    typedef Signed SignType;
    typedef int8_t WithOutUnsigned;
    typedef uint8_t WithUnsigned;
};

template <>
struct IntTraits<uint8_t> {
    typedef Unsigned SignType;
    typedef int8_t WithOutUnsigned;
    typedef uint8_t WithUnsigned;
};

template <>
struct IntTraits<int16_t> {
    typedef Signed SignType;
    typedef int16_t WithOutUnsigned;
    typedef uint16_t WithUnsigned;
};

template <>
struct IntTraits<uint16_t> {
    typedef Unsigned SignType;
    typedef int16_t WithOutUnsigned;
    typedef uint16_t WithUnsigned;
};

template <>
struct IntTraits<int32_t> {
    typedef Signed SignType;
    typedef int32_t WithOutUnsigned;
    typedef uint32_t WithUnsigned;
};

template <>
struct IntTraits<uint32_t> {
    typedef Unsigned SignType;
    typedef int32_t WithOutUnsigned;
    typedef uint32_t WithUnsigned;
};

template <>
struct IntTraits<int64_t> {
    typedef Signed SignType;
    typedef int64_t WithOutUnsigned;
    typedef uint64_t WithUnsigned;
};

template <>
struct IntTraits<uint64_t> {
    typedef Unsigned SignType;
    typedef int64_t WithOutUnsigned;
    typedef uint64_t WithUnsigned;
};

template <typename T>
struct BaseType {
    typedef T type;
};
template <typename T>
struct BaseType<T *> {
    typedef T type;
};
template <typename T>
struct BaseType<T *const> {
    typedef T type;
};
template <typename T>
struct BaseType<T &> {
    typedef T type;
};
template <typename T>
struct BaseType<const T> {
    typedef T type;
};
template <typename T>
struct BaseType<const T *> {
    typedef T type;
};
template <typename T>
struct BaseType<T const *const> {
    typedef T type;
};
template <typename T>
struct BaseType<const T &> {
    typedef T type;
};
template <typename T>
struct BaseType<volatile T> {
    typedef T type;
};
template <typename T>
struct BaseType<volatile T *> {
    typedef T type;
};
template <typename T>
struct BaseType<volatile T *const> {
    typedef T type;
};
template <typename T>
struct BaseType<volatile T &> {
    typedef T type;
};
template <typename T>
struct BaseType<const volatile T> {
    typedef T type;
};
template <typename T>
struct BaseType<const volatile T *> {
    typedef T type;
};
template <typename T>
struct BaseType<const volatile T *const> {
    typedef T type;
};
template <typename T>
struct BaseType<const volatile T &> {
    typedef T type;
};

template <typename T>
struct OutputType {
    typedef T *type;
};
template <typename T>
struct OutputType<T *> {
    typedef T *type;
};
template <typename T>
struct OutputType<T *const> {
    typedef T *type;
};
template <typename T>
struct OutputType<T &> {
    typedef T &type;
};
template <typename T>
struct OutputType<const T> {
    typedef T *type;
};
template <typename T>
struct OutputType<const T *> {
    typedef T *type;
};
template <typename T>
struct OutputType<T const *const> {
    typedef T *type;
};
template <typename T>
struct OutputType<const T &> {
    typedef T &type;
};

template <typename... Args>
struct vector {};

template <typename T, typename VECTOR>
struct push_back;

template <typename T, typename... Args>
struct push_back<T, vector<Args...>> {
    typedef vector<Args..., T> type;
};

template <typename T, typename VECTOR>
struct push_head;

template <typename T, typename... Args>
struct push_head<T, vector<Args...>> {
    typedef vector<T, Args...> type;
};

// Check if a type is included in a typelist
template <typename T, typename VECTOR>
struct vector_include;

// Base condition, type not found in list
template <typename T>
struct vector_include<T, vector<>> {
    typedef _false type;
    static constexpr bool value = false;
};

// Type was found at current head!
template <typename T, typename... TAIL>
struct vector_include<T, vector<T, TAIL...>> {
    typedef _true type;
    static constexpr bool value = true;
};

// Type still not found, continue iterate
template <typename T, typename HEAD, typename... TAIL>
struct vector_include<T, vector<HEAD, TAIL...>> {
    typedef typename vector_include<T, vector<TAIL...>>::type type;
    static constexpr bool value = vector_include<T, vector<TAIL...>>::value;
};

}  // namespace meta
}  // namespace vtss

#endif /* _META_H_ */
