// -*- C++ -*-
//===------------------------ type_traits ---------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Importet from https://github.com/llvm-mirror/libcxx.git
// SHA 563647a240b9997cb9e414cbb09c82f19f00c89b
//
// Patches has been applied by MSCC on top of that - use a diff tool to see the
// changes.

#ifndef __VTSS_BASICS_TYPE_TRAITS_HXX__
#define __VTSS_BASICS_TYPE_TRAITS_HXX__

/*
    type_traits synopsis

namespace std
{

    // helper class:
    template <class T, T v> struct integral_constant;
    typedef integral_constant<bool, true>  true_type;   // C++11
    typedef integral_constant<bool, false> false_type;  // C++11
    
    template <bool B>                                   // C++14
    using bool_constant = integral_constant<bool, B>;   // C++14
    typedef bool_constant<true> true_type;              // C++14
    typedef bool_constant<false> false_type;            // C++14

    // helper traits
    template <bool, class T = void> struct enable_if;
    template <bool, class T, class F> struct conditional;

    // Primary classification traits:
    template <class T> struct is_void;
    template <class T> struct is_null_pointer;  // C++14
    template <class T> struct is_integral;
    template <class T> struct is_floating_point;
    template <class T> struct is_array;
    template <class T> struct is_pointer;
    template <class T> struct is_lvalue_reference;
    template <class T> struct is_rvalue_reference;
    template <class T> struct is_member_object_pointer;
    template <class T> struct is_member_function_pointer;
    template <class T> struct is_enum;
    template <class T> struct is_union;
    template <class T> struct is_class;
    template <class T> struct is_function;

    // Secondary classification traits:
    template <class T> struct is_reference;
    template <class T> struct is_arithmetic;
    template <class T> struct is_fundamental;
    template <class T> struct is_member_pointer;
    template <class T> struct is_scalar;
    template <class T> struct is_object;
    template <class T> struct is_compound;

    // Const-volatile properties and transformations:
    template <class T> struct is_const;
    template <class T> struct is_volatile;
    template <class T> struct remove_const;
    template <class T> struct remove_volatile;
    template <class T> struct remove_cv;
    template <class T> struct add_const;
    template <class T> struct add_volatile;
    template <class T> struct add_cv;

    // Reference transformations:
    template <class T> struct remove_reference;
    template <class T> struct add_lvalue_reference;
    template <class T> struct add_rvalue_reference;

    // Pointer transformations:
    template <class T> struct remove_pointer;
    template <class T> struct add_pointer;

    // Integral properties:
    template <class T> struct is_signed;
    template <class T> struct is_unsigned;
    template <class T> struct make_signed;
    template <class T> struct make_unsigned;

    // Array properties and transformations:
    template <class T> struct rank;
    template <class T, unsigned I = 0> struct extent;
    template <class T> struct remove_extent;
    template <class T> struct remove_all_extents;

    // Member introspection:
    template <class T> struct is_pod;
    template <class T> struct is_trivial;
    template <class T> struct is_trivially_copyable;
    template <class T> struct is_standard_layout;
    template <class T> struct is_literal_type;
    template <class T> struct is_empty;
    template <class T> struct is_polymorphic;
    template <class T> struct is_abstract;
    template <class T> struct is_final; // C++14

    template <class T, class... Args> struct is_constructible;
    template <class T>                struct is_default_constructible;
    template <class T>                struct is_copy_constructible;
    template <class T>                struct is_move_constructible;
    template <class T, class U>       struct is_assignable;
    template <class T>                struct is_copy_assignable;
    template <class T>                struct is_move_assignable;
    template <class T, class U>       struct is_swappable_with;       // C++17
    template <class T>                struct is_swappable;            // C++17
    template <class T>                struct is_destructible;

    template <class T, class... Args> struct is_trivially_constructible;
    template <class T>                struct is_trivially_default_constructible;
    template <class T>                struct is_trivially_copy_constructible;
    template <class T>                struct is_trivially_move_constructible;
    template <class T, class U>       struct is_trivially_assignable;
    template <class T>                struct is_trivially_copy_assignable;
    template <class T>                struct is_trivially_move_assignable;
    template <class T>                struct is_trivially_destructible;

    template <class T, class... Args> struct is_nothrow_constructible;
    template <class T>                struct is_nothrow_default_constructible;
    template <class T>                struct is_nothrow_copy_constructible;
    template <class T>                struct is_nothrow_move_constructible;
    template <class T, class U>       struct is_nothrow_assignable;
    template <class T>                struct is_nothrow_copy_assignable;
    template <class T>                struct is_nothrow_move_assignable;
    template <class T, class U>       struct is_nothrow_swappable_with; // C++17
    template <class T>                struct is_nothrow_swappable;      // C++17
    template <class T>                struct is_nothrow_destructible;

    template <class T> struct has_virtual_destructor;

    // Relationships between types:
    template <class T, class U> struct is_same;
    template <class Base, class Derived> struct is_base_of;
    template <class From, class To> struct is_convertible;

    template <class, class R = void> struct is_callable; // not defined
    template <class Fn, class... ArgTypes, class R>
      struct is_callable<Fn(ArgTypes...), R>;

    template <class, class R = void> struct is_nothrow_callable; // not defined
    template <class Fn, class... ArgTypes, class R>
      struct is_nothrow_callable<Fn(ArgTypes...), R>;

    // Alignment properties and transformations:
    template <class T> struct alignment_of;
    template <size_t Len, size_t Align = most_stringent_alignment_requirement>
        struct aligned_storage;
    template <size_t Len, class... Types> struct aligned_union;

    template <class T> struct decay;
    template <class... T> struct common_type;
    template <class T> struct underlying_type;
    template <class> class result_of; // undefined
    template <class Fn, class... ArgTypes> class result_of<Fn(ArgTypes...)>;

    // const-volatile modifications:
    template <class T>
      using remove_const_t    = typename remove_const<T>::type;  // C++14
    template <class T>
      using remove_volatile_t = typename remove_volatile<T>::type;  // C++14
    template <class T>
      using remove_cv_t       = typename remove_cv<T>::type;  // C++14
    template <class T>
      using add_const_t       = typename add_const<T>::type;  // C++14
    template <class T>
      using add_volatile_t    = typename add_volatile<T>::type;  // C++14
    template <class T>
      using add_cv_t          = typename add_cv<T>::type;  // C++14
  
    // reference modifications:
    template <class T>
      using remove_reference_t     = typename remove_reference<T>::type;  // C++14
    template <class T>
      using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;  // C++14
    template <class T>
      using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;  // C++14
  
    // sign modifications:
    template <class T>
      using make_signed_t   = typename make_signed<T>::type;  // C++14
    template <class T>
      using make_unsigned_t = typename make_unsigned<T>::type;  // C++14
  
    // array modifications:
    template <class T>
      using remove_extent_t      = typename remove_extent<T>::type;  // C++14
    template <class T>
      using remove_all_extents_t = typename remove_all_extents<T>::type;  // C++14

    // pointer modifications:
    template <class T>
      using remove_pointer_t = typename remove_pointer<T>::type;  // C++14
    template <class T>
      using add_pointer_t    = typename add_pointer<T>::type;  // C++14

    // other transformations:
    template <size_t Len, std::size_t Align=default-alignment>
      using aligned_storage_t = typename aligned_storage<Len,Align>::type;  // C++14
    template <std::size_t Len, class... Types>
      using aligned_union_t   = typename aligned_union<Len,Types...>::type;  // C++14
    template <class T>
      using decay_t           = typename decay<T>::type;  // C++14
    template <bool b, class T=void>
      using enable_if_t       = typename enable_if<b,T>::type;  // C++14
    template <bool b, class T, class F>
      using conditional_t     = typename conditional<b,T,F>::type;  // C++14
    template <class... T>
      using common_type_t     = typename common_type<T...>::type;  // C++14
    template <class T>
      using underlying_type_t = typename underlying_type<T>::type;  // C++14
    template <class F, class... ArgTypes>
      using result_of_t       = typename result_of<F(ArgTypes...)>::type;  // C++14

    template <class...>
      using void_t = void;   // C++17
      
      // See C++14 20.10.4.1, primary type categories
      template <class T> constexpr bool is_void_v
        = is_void<T>::value;                                             // C++17
      template <class T> constexpr bool is_null_pointer_v
        = is_null_pointer<T>::value;                                     // C++17
      template <class T> constexpr bool is_integral_v
        = is_integral<T>::value;                                         // C++17
      template <class T> constexpr bool is_floating_point_v
        = is_floating_point<T>::value;                                   // C++17
      template <class T> constexpr bool is_array_v
        = is_array<T>::value;                                            // C++17
      template <class T> constexpr bool is_pointer_v
        = is_pointer<T>::value;                                          // C++17
      template <class T> constexpr bool is_lvalue_reference_v
        = is_lvalue_reference<T>::value;                                 // C++17
      template <class T> constexpr bool is_rvalue_reference_v
        = is_rvalue_reference<T>::value;                                 // C++17
      template <class T> constexpr bool is_member_object_pointer_v
        = is_member_object_pointer<T>::value;                            // C++17
      template <class T> constexpr bool is_member_function_pointer_v
        = is_member_function_pointer<T>::value;                          // C++17
      template <class T> constexpr bool is_enum_v
        = is_enum<T>::value;                                             // C++17
      template <class T> constexpr bool is_union_v
        = is_union<T>::value;                                            // C++17
      template <class T> constexpr bool is_class_v
        = is_class<T>::value;                                            // C++17
      template <class T> constexpr bool is_function_v
        = is_function<T>::value;                                         // C++17

      // See C++14 20.10.4.2, composite type categories
      template <class T> constexpr bool is_reference_v
        = is_reference<T>::value;                                        // C++17
      template <class T> constexpr bool is_arithmetic_v
        = is_arithmetic<T>::value;                                       // C++17
      template <class T> constexpr bool is_fundamental_v
        = is_fundamental<T>::value;                                      // C++17
      template <class T> constexpr bool is_object_v
        = is_object<T>::value;                                           // C++17
      template <class T> constexpr bool is_scalar_v
        = is_scalar<T>::value;                                           // C++17
      template <class T> constexpr bool is_compound_v
        = is_compound<T>::value;                                         // C++17
      template <class T> constexpr bool is_member_pointer_v
        = is_member_pointer<T>::value;                                   // C++17

      // See C++14 20.10.4.3, type properties
      template <class T> constexpr bool is_const_v
        = is_const<T>::value;                                            // C++17
      template <class T> constexpr bool is_volatile_v
        = is_volatile<T>::value;                                         // C++17
      template <class T> constexpr bool is_trivial_v
        = is_trivial<T>::value;                                          // C++17
      template <class T> constexpr bool is_trivially_copyable_v
        = is_trivially_copyable<T>::value;                               // C++17
      template <class T> constexpr bool is_standard_layout_v
        = is_standard_layout<T>::value;                                  // C++17
      template <class T> constexpr bool is_pod_v
        = is_pod<T>::value;                                              // C++17
      template <class T> constexpr bool is_literal_type_v
        = is_literal_type<T>::value;                                     // C++17
      template <class T> constexpr bool is_empty_v
        = is_empty<T>::value;                                            // C++17
      template <class T> constexpr bool is_polymorphic_v
        = is_polymorphic<T>::value;                                      // C++17
      template <class T> constexpr bool is_abstract_v
        = is_abstract<T>::value;                                         // C++17
      template <class T> constexpr bool is_final_v
        = is_final<T>::value;                                            // C++17
      template <class T> constexpr bool is_signed_v
        = is_signed<T>::value;                                           // C++17
      template <class T> constexpr bool is_unsigned_v
        = is_unsigned<T>::value;                                         // C++17
      template <class T, class... Args> constexpr bool is_constructible_v
        = is_constructible<T, Args...>::value;                           // C++17
      template <class T> constexpr bool is_default_constructible_v
        = is_default_constructible<T>::value;                            // C++17
      template <class T> constexpr bool is_copy_constructible_v
        = is_copy_constructible<T>::value;                               // C++17
      template <class T> constexpr bool is_move_constructible_v
        = is_move_constructible<T>::value;                               // C++17
      template <class T, class U> constexpr bool is_assignable_v
        = is_assignable<T, U>::value;                                    // C++17
      template <class T> constexpr bool is_copy_assignable_v
        = is_copy_assignable<T>::value;                                  // C++17
      template <class T> constexpr bool is_move_assignable_v
        = is_move_assignable<T>::value;                                  // C++17
      template <class T, class U> constexpr bool is_swappable_with_v
        = is_swappable_with<T, U>::value;                                // C++17
      template <class T> constexpr bool is_swappable_v
        = is_swappable<T>::value;                                        // C++17
      template <class T> constexpr bool is_destructible_v
        = is_destructible<T>::value;                                     // C++17
      template <class T, class... Args> constexpr bool is_trivially_constructible_v
        = is_trivially_constructible<T, Args...>::value;                 // C++17
      template <class T> constexpr bool is_trivially_default_constructible_v
        = is_trivially_default_constructible<T>::value;                  // C++17
      template <class T> constexpr bool is_trivially_copy_constructible_v
        = is_trivially_copy_constructible<T>::value;                     // C++17
      template <class T> constexpr bool is_trivially_move_constructible_v
        = is_trivially_move_constructible<T>::value;                     // C++17
      template <class T, class U> constexpr bool is_trivially_assignable_v
        = is_trivially_assignable<T, U>::value;                          // C++17
      template <class T> constexpr bool is_trivially_copy_assignable_v
        = is_trivially_copy_assignable<T>::value;                        // C++17
      template <class T> constexpr bool is_trivially_move_assignable_v
        = is_trivially_move_assignable<T>::value;                        // C++17
      template <class T> constexpr bool is_trivially_destructible_v
        = is_trivially_destructible<T>::value;                           // C++17
      template <class T, class... Args> constexpr bool is_nothrow_constructible_v
        = is_nothrow_constructible<T, Args...>::value;                   // C++17
      template <class T> constexpr bool is_nothrow_default_constructible_v
        = is_nothrow_default_constructible<T>::value;                    // C++17
      template <class T> constexpr bool is_nothrow_copy_constructible_v
        = is_nothrow_copy_constructible<T>::value;                       // C++17
      template <class T> constexpr bool is_nothrow_move_constructible_v
        = is_nothrow_move_constructible<T>::value;                       // C++17
      template <class T, class U> constexpr bool is_nothrow_assignable_v
        = is_nothrow_assignable<T, U>::value;                            // C++17
      template <class T> constexpr bool is_nothrow_copy_assignable_v
        = is_nothrow_copy_assignable<T>::value;                          // C++17
      template <class T> constexpr bool is_nothrow_move_assignable_v
        = is_nothrow_move_assignable<T>::value;                          // C++17
      template <class T, class U> constexpr bool is_nothrow_swappable_with_v
        = is_nothrow_swappable_with<T, U>::value;                       // C++17
      template <class T> constexpr bool is_nothrow_swappable_v
        = is_nothrow_swappable<T>::value;                               // C++17
      template <class T> constexpr bool is_nothrow_destructible_v
        = is_nothrow_destructible<T>::value;                             // C++17
      template <class T> constexpr bool has_virtual_destructor_v
        = has_virtual_destructor<T>::value;                              // C++17

      // See C++14 20.10.5, type property queries
      template <class T> constexpr size_t alignment_of_v
        = alignment_of<T>::value;                                        // C++17
      template <class T> constexpr size_t rank_v
        = rank<T>::value;                                                // C++17
      template <class T, unsigned I = 0> constexpr size_t extent_v
        = extent<T, I>::value;                                           // C++17

      // See C++14 20.10.6, type relations
      template <class T, class U> constexpr bool is_same_v
        = is_same<T, U>::value;                                          // C++17
      template <class Base, class Derived> constexpr bool is_base_of_v
        = is_base_of<Base, Derived>::value;                              // C++17
      template <class From, class To> constexpr bool is_convertible_v
        = is_convertible<From, To>::value;                               // C++17
      template <class T, class R = void> constexpr bool is_callable_v
        = is_callable<T, R>::value;                                      // C++17
      template <class T, class R = void> constexpr bool is_nothrow_callable_v
        = is_nothrow_callable<T, R>::value;                              // C++17

      // [meta.logical], logical operator traits:
      template<class... B> struct conjunction;                           // C++17
      template<class... B> 
        constexpr bool conjunction_v = conjunction<B...>::value;         // C++17
      template<class... B> struct disjunction;                           // C++17
      template<class... B>
        constexpr bool disjunction_v = disjunction<B...>::value;         // C++17
      template<class B> struct negation;                                 // C++17
      template<class B> 
        constexpr bool negation_v = negation<B>::value;                  // C++17

}

*/
#include <vtss/basics/__libcxx_config.hxx>
#include <cstddef>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

typedef decltype(nullptr) nullptr_t;
template <class _T1, class _T2> struct _LIBCPP_TYPE_VIS_ONLY pair;
template <class _Tp> class _LIBCPP_TYPE_VIS_ONLY reference_wrapper;
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY hash;

template <class>
struct __void_t { typedef void type; };

template <class _Tp>
struct __identity { typedef _Tp type; };

template <class _Tp, bool>
struct _LIBCPP_TYPE_VIS_ONLY __dependent_type : public _Tp {};

template <bool _Bp, class _If, class _Then>
    struct _LIBCPP_TYPE_VIS_ONLY conditional {typedef _If type;};
template <class _If, class _Then>
    struct _LIBCPP_TYPE_VIS_ONLY conditional<false, _If, _Then> {typedef _Then type;};

#if _LIBCPP_STD_VER > 11
template <bool _Bp, class _If, class _Then> using conditional_t = typename conditional<_Bp, _If, _Then>::type;
#endif

template <bool, class _Tp> struct _LIBCPP_TYPE_VIS_ONLY __lazy_enable_if {};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY __lazy_enable_if<true, _Tp> {typedef typename _Tp::type type;};

template <bool, class _Tp = void> struct _LIBCPP_TYPE_VIS_ONLY enable_if {};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY enable_if<true, _Tp> {typedef _Tp type;};

#if _LIBCPP_STD_VER > 11
template <bool _Bp, class _Tp = void> using enable_if_t = typename enable_if<_Bp, _Tp>::type;
#endif

// addressof
#if __has_builtin(__builtin_addressof)

template <class _Tp>
inline _LIBCPP_CONSTEXPR_AFTER_CXX14
_LIBCPP_NO_CFI _LIBCPP_INLINE_VISIBILITY
_Tp*
addressof(_Tp& __x) _NOEXCEPT
{
    return __builtin_addressof(__x);
}

#else

template <class _Tp>
inline _LIBCPP_NO_CFI _LIBCPP_INLINE_VISIBILITY
_Tp*
addressof(_Tp& __x) _NOEXCEPT
{
    return (_Tp*)&reinterpret_cast<const volatile char&>(__x);
}

#endif // __has_builtin(__builtin_addressof)

#if defined(_LIBCPP_HAS_OBJC_ARC) && !defined(_LIBCPP_PREDEFINED_OBJC_ARC_ADDRESSOF)
// Objective-C++ Automatic Reference Counting uses qualified pointers
// that require special addressof() signatures. When
// _LIBCPP_PREDEFINED_OBJC_ARC_ADDRESSOF is defined, the compiler
// itself is providing these definitions. Otherwise, we provide them.
template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
__strong _Tp*
addressof(__strong _Tp& __x) _NOEXCEPT
{
  return &__x;
}

#ifdef _LIBCPP_HAS_OBJC_ARC_WEAK
template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
__weak _Tp*
addressof(__weak _Tp& __x) _NOEXCEPT
{
  return &__x;
}
#endif

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
__autoreleasing _Tp*
addressof(__autoreleasing _Tp& __x) _NOEXCEPT
{
  return &__x;
}

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
__unsafe_unretained _Tp*
addressof(__unsafe_unretained _Tp& __x) _NOEXCEPT
{
  return &__x;
}
#endif

struct __two {char __lx[2];};

// helper class:

template <class _Tp, _Tp __v>
struct _LIBCPP_TYPE_VIS_ONLY integral_constant
{
    static _LIBCPP_CONSTEXPR const _Tp      value = __v;
    typedef _Tp               value_type;
    typedef integral_constant type;
    _LIBCPP_INLINE_VISIBILITY
        _LIBCPP_CONSTEXPR operator value_type() const _NOEXCEPT {return value;}
#if _LIBCPP_STD_VER > 11
    _LIBCPP_INLINE_VISIBILITY
         constexpr value_type operator ()() const _NOEXCEPT {return value;}
#endif
};

template <class _Tp, _Tp __v>
_LIBCPP_CONSTEXPR const _Tp integral_constant<_Tp, __v>::value;

#if _LIBCPP_STD_VER > 14
template <bool __b>
using bool_constant = integral_constant<bool, __b>;
#define _LIBCPP_BOOL_CONSTANT(__b) bool_constant<(__b)>
#else
#define _LIBCPP_BOOL_CONSTANT(__b) integral_constant<bool,(__b)>
#endif

typedef _LIBCPP_BOOL_CONSTANT(true)  true_type;
typedef _LIBCPP_BOOL_CONSTANT(false) false_type;

#if !defined(_LIBCPP_HAS_NO_VARIADICS)

// __lazy_and

template <bool _Last, class ..._Preds>
struct __lazy_and_impl;

template <class ..._Preds>
struct __lazy_and_impl<false, _Preds...> : false_type {};

template <>
struct __lazy_and_impl<true> : true_type {};

template <class _Pred>
struct __lazy_and_impl<true, _Pred> : integral_constant<bool, _Pred::type::value> {};

template <class _Hp, class ..._Tp>
struct __lazy_and_impl<true, _Hp, _Tp...> : __lazy_and_impl<_Hp::type::value, _Tp...> {};

template <class _P1, class ..._Pr>
struct __lazy_and : __lazy_and_impl<_P1::type::value, _Pr...> {};

// __lazy_or

template <bool _List, class ..._Preds>
struct __lazy_or_impl;

template <class ..._Preds>
struct __lazy_or_impl<true, _Preds...> : true_type {};

template <>
struct __lazy_or_impl<false> : false_type {};

template <class _Hp, class ..._Tp>
struct __lazy_or_impl<false, _Hp, _Tp...>
        : __lazy_or_impl<_Hp::type::value, _Tp...> {};

template <class _P1, class ..._Pr>
struct __lazy_or : __lazy_or_impl<_P1::type::value, _Pr...> {};

// __lazy_not

template <class _Pred>
struct __lazy_not : integral_constant<bool, !_Pred::type::value> {};

// __and_
template<class...> struct __and_;
template<> struct __and_<> : true_type {};

template<class _B0> struct __and_<_B0> : _B0 {};

template<class _B0, class _B1>
struct __and_<_B0, _B1> : conditional<_B0::value, _B1, _B0>::type {};

template<class _B0, class _B1, class _B2, class... _Bn>
struct __and_<_B0, _B1, _B2, _Bn...> 
        : conditional<_B0::value, __and_<_B1, _B2, _Bn...>, _B0>::type {};

// __or_
template<class...> struct __or_;
template<> struct __or_<> : false_type {};

template<class _B0> struct __or_<_B0> : _B0 {};

template<class _B0, class _B1>
struct __or_<_B0, _B1> : conditional<_B0::value, _B0, _B1>::type {};

template<class _B0, class _B1, class _B2, class... _Bn>
struct __or_<_B0, _B1, _B2, _Bn...> 
        : conditional<_B0::value, _B0, __or_<_B1, _B2, _Bn...> >::type {};

// __not_
template<class _Tp> 
struct __not_ : conditional<_Tp::value, false_type, true_type>::type {};

#endif // !defined(_LIBCPP_HAS_NO_VARIADICS)

// is_const

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_const            : public false_type {};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_const<_Tp const> : public true_type {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_const_v
    = is_const<_Tp>::value;
#endif

// is_volatile

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_volatile               : public false_type {};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_volatile<_Tp volatile> : public true_type {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_volatile_v
    = is_volatile<_Tp>::value;
#endif

// remove_const

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_const            {typedef _Tp type;};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_const<const _Tp> {typedef _Tp type;};
#if _LIBCPP_STD_VER > 11
template <class _Tp> using remove_const_t = typename remove_const<_Tp>::type;
#endif

// remove_volatile

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_volatile               {typedef _Tp type;};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_volatile<volatile _Tp> {typedef _Tp type;};
#if _LIBCPP_STD_VER > 11
template <class _Tp> using remove_volatile_t = typename remove_volatile<_Tp>::type;
#endif

// remove_cv

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_cv
{typedef typename remove_volatile<typename remove_const<_Tp>::type>::type type;};
#if _LIBCPP_STD_VER > 11
template <class _Tp> using remove_cv_t = typename remove_cv<_Tp>::type;
#endif

// is_void

template <class _Tp> struct __libcpp_is_void       : public false_type {};
template <>          struct __libcpp_is_void<void> : public true_type {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_void
    : public __libcpp_is_void<typename remove_cv<_Tp>::type> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_void_v
    = is_void<_Tp>::value;
#endif

// __is_nullptr_t

template <class _Tp> struct __is_nullptr_t_impl       : public false_type {};
template <>          struct __is_nullptr_t_impl<nullptr_t> : public true_type {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY __is_nullptr_t
    : public __is_nullptr_t_impl<typename remove_cv<_Tp>::type> {};

#if _LIBCPP_STD_VER > 11
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_null_pointer
    : public __is_nullptr_t_impl<typename remove_cv<_Tp>::type> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_null_pointer_v
    = is_null_pointer<_Tp>::value;
#endif
#endif

// is_integral

template <class _Tp> struct __libcpp_is_integral                     : public false_type {};
template <>          struct __libcpp_is_integral<bool>               : public true_type {};
template <>          struct __libcpp_is_integral<char>               : public true_type {};
template <>          struct __libcpp_is_integral<signed char>        : public true_type {};
template <>          struct __libcpp_is_integral<unsigned char>      : public true_type {};
template <>          struct __libcpp_is_integral<wchar_t>            : public true_type {};
#ifndef _LIBCPP_HAS_NO_UNICODE_CHARS
template <>          struct __libcpp_is_integral<char16_t>           : public true_type {};
template <>          struct __libcpp_is_integral<char32_t>           : public true_type {};
#endif  // _LIBCPP_HAS_NO_UNICODE_CHARS
template <>          struct __libcpp_is_integral<short>              : public true_type {};
template <>          struct __libcpp_is_integral<unsigned short>     : public true_type {};
template <>          struct __libcpp_is_integral<int>                : public true_type {};
template <>          struct __libcpp_is_integral<unsigned int>       : public true_type {};
template <>          struct __libcpp_is_integral<long>               : public true_type {};
template <>          struct __libcpp_is_integral<unsigned long>      : public true_type {};
template <>          struct __libcpp_is_integral<long long>          : public true_type {};
template <>          struct __libcpp_is_integral<unsigned long long> : public true_type {};
#ifndef _LIBCPP_HAS_NO_INT128
template <>          struct __libcpp_is_integral<__int128_t>         : public true_type {};
template <>          struct __libcpp_is_integral<__uint128_t>        : public true_type {};
#endif

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_integral
    : public __libcpp_is_integral<typename remove_cv<_Tp>::type> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_integral_v
    = is_integral<_Tp>::value;
#endif

// is_floating_point

template <class _Tp> struct __libcpp_is_floating_point              : public false_type {};
template <>          struct __libcpp_is_floating_point<float>       : public true_type {};
template <>          struct __libcpp_is_floating_point<double>      : public true_type {};
template <>          struct __libcpp_is_floating_point<long double> : public true_type {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_floating_point
    : public __libcpp_is_floating_point<typename remove_cv<_Tp>::type> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_floating_point_v
    = is_floating_point<_Tp>::value;
#endif

// is_array

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_array
    : public false_type {};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_array<_Tp[]>
    : public true_type {};
template <class _Tp, size_t _Np> struct _LIBCPP_TYPE_VIS_ONLY is_array<_Tp[_Np]>
    : public true_type {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_array_v
    = is_array<_Tp>::value;
#endif

// is_pointer

template <class _Tp> struct __libcpp_is_pointer       : public false_type {};
template <class _Tp> struct __libcpp_is_pointer<_Tp*> : public true_type {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_pointer
    : public __libcpp_is_pointer<typename remove_cv<_Tp>::type> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_pointer_v
    = is_pointer<_Tp>::value;
#endif

// is_reference

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_lvalue_reference       : public false_type {};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_lvalue_reference<_Tp&> : public true_type {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_rvalue_reference        : public false_type {};
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_rvalue_reference<_Tp&&> : public true_type {};
#endif

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_reference        : public false_type {};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_reference<_Tp&>  : public true_type {};
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_reference<_Tp&&> : public true_type {};
#endif

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_reference_v
    = is_reference<_Tp>::value;

template <class _Tp> _LIBCPP_CONSTEXPR bool is_lvalue_reference_v
    = is_lvalue_reference<_Tp>::value;

template <class _Tp> _LIBCPP_CONSTEXPR bool is_rvalue_reference_v
    = is_rvalue_reference<_Tp>::value;
#endif
// is_union

#if __has_feature(is_union) || (_GNUC_VER >= 403)

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_union
    : public integral_constant<bool, __is_union(_Tp)> {};

#else

template <class _Tp> struct __libcpp_union : public false_type {};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_union
    : public __libcpp_union<typename remove_cv<_Tp>::type> {};

#endif

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_union_v
    = is_union<_Tp>::value;
#endif

// is_class

#if __has_feature(is_class) || (_GNUC_VER >= 403)

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_class
    : public integral_constant<bool, __is_class(_Tp)> {};

#else

namespace __is_class_imp
{
template <class _Tp> char  __test(int _Tp::*);
template <class _Tp> __two __test(...);
}

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_class
    : public integral_constant<bool, sizeof(__is_class_imp::__test<_Tp>(0)) == 1 && !is_union<_Tp>::value> {};

#endif

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_class_v
    = is_class<_Tp>::value;
#endif

// is_same

template <class _Tp, class _Up> struct _LIBCPP_TYPE_VIS_ONLY is_same           : public false_type {};
template <class _Tp>            struct _LIBCPP_TYPE_VIS_ONLY is_same<_Tp, _Tp> : public true_type {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp, class _Up> _LIBCPP_CONSTEXPR bool is_same_v
    = is_same<_Tp, _Up>::value;
#endif

// is_function

namespace __libcpp_is_function_imp
{
struct __dummy_type {};
template <class _Tp> char  __test(_Tp*);
template <class _Tp> char __test(__dummy_type);
template <class _Tp> __two __test(...);
template <class _Tp> _Tp&  __source(int);
template <class _Tp> __dummy_type __source(...);
}

template <class _Tp, bool = is_class<_Tp>::value ||
                            is_union<_Tp>::value ||
                            is_void<_Tp>::value  ||
                            is_reference<_Tp>::value ||
                            __is_nullptr_t<_Tp>::value >
struct __libcpp_is_function
    : public integral_constant<bool, sizeof(__libcpp_is_function_imp::__test<_Tp>(__libcpp_is_function_imp::__source<_Tp>(0))) == 1>
    {};
template <class _Tp> struct __libcpp_is_function<_Tp, true> : public false_type {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_function
    : public __libcpp_is_function<_Tp> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_function_v
    = is_function<_Tp>::value;
#endif

// is_member_function_pointer

// template <class _Tp> struct            __libcpp_is_member_function_pointer             : public false_type {};
// template <class _Tp, class _Up> struct __libcpp_is_member_function_pointer<_Tp _Up::*> : public is_function<_Tp> {};
// 

template <class _MP, bool _IsMemberFunctionPtr, bool _IsMemberObjectPtr>
struct __member_pointer_traits_imp
{  // forward declaration; specializations later
};


template <class _Tp> struct __libcpp_is_member_function_pointer
    : public false_type {};

template <class _Ret, class _Class>
struct __libcpp_is_member_function_pointer<_Ret _Class::*>
    : public is_function<_Ret> {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_member_function_pointer
    : public __libcpp_is_member_function_pointer<typename remove_cv<_Tp>::type>::type {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_member_function_pointer_v
    = is_member_function_pointer<_Tp>::value;
#endif

// is_member_pointer

template <class _Tp>            struct __libcpp_is_member_pointer             : public false_type {};
template <class _Tp, class _Up> struct __libcpp_is_member_pointer<_Tp _Up::*> : public true_type {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_member_pointer
    : public __libcpp_is_member_pointer<typename remove_cv<_Tp>::type> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_member_pointer_v
    = is_member_pointer<_Tp>::value;
#endif

// is_member_object_pointer

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_member_object_pointer
    : public integral_constant<bool, is_member_pointer<_Tp>::value &&
                                    !is_member_function_pointer<_Tp>::value> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_member_object_pointer_v
    = is_member_object_pointer<_Tp>::value;
#endif

// is_enum

#if __has_feature(is_enum) || (_GNUC_VER >= 403)

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_enum
    : public integral_constant<bool, __is_enum(_Tp)> {};

#else

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_enum
    : public integral_constant<bool, !is_void<_Tp>::value             &&
                                     !is_integral<_Tp>::value         &&
                                     !is_floating_point<_Tp>::value   &&
                                     !is_array<_Tp>::value            &&
                                     !is_pointer<_Tp>::value          &&
                                     !is_reference<_Tp>::value        &&
                                     !is_member_pointer<_Tp>::value   &&
                                     !is_union<_Tp>::value            &&
                                     !is_class<_Tp>::value            &&
                                     !is_function<_Tp>::value         > {};

#endif

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_enum_v
    = is_enum<_Tp>::value;
#endif

// is_arithmetic

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_arithmetic
    : public integral_constant<bool, is_integral<_Tp>::value      ||
                                     is_floating_point<_Tp>::value> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_arithmetic_v
    = is_arithmetic<_Tp>::value;
#endif

// is_fundamental

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_fundamental
    : public integral_constant<bool, is_void<_Tp>::value        ||
                                     __is_nullptr_t<_Tp>::value ||
                                     is_arithmetic<_Tp>::value> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_fundamental_v
    = is_fundamental<_Tp>::value;
#endif

// is_scalar

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_scalar
    : public integral_constant<bool, is_arithmetic<_Tp>::value     ||
                                     is_member_pointer<_Tp>::value ||
                                     is_pointer<_Tp>::value        ||
                                     __is_nullptr_t<_Tp>::value    ||
                                     is_enum<_Tp>::value           > {};

template <> struct _LIBCPP_TYPE_VIS_ONLY is_scalar<nullptr_t> : public true_type {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_scalar_v
    = is_scalar<_Tp>::value;
#endif

// is_object

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_object
    : public integral_constant<bool, is_scalar<_Tp>::value ||
                                     is_array<_Tp>::value  ||
                                     is_union<_Tp>::value  ||
                                     is_class<_Tp>::value  > {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_object_v
    = is_object<_Tp>::value;
#endif

// is_compound

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_compound
    : public integral_constant<bool, !is_fundamental<_Tp>::value> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_compound_v
    = is_compound<_Tp>::value;
#endif


// __is_referenceable  [defns.referenceable]

struct __is_referenceable_impl {
    template <class _Tp> static _Tp& __test(int);
    template <class _Tp> static __two __test(...);
};

template <class _Tp>
struct __is_referenceable : integral_constant<bool,
    !is_same<decltype(__is_referenceable_impl::__test<_Tp>(0)), __two>::value> {};


// add_const

template <class _Tp, bool = is_reference<_Tp>::value ||
                            is_function<_Tp>::value  ||
                            is_const<_Tp>::value     >
struct __add_const             {typedef _Tp type;};

template <class _Tp>
struct __add_const<_Tp, false> {typedef const _Tp type;};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY add_const
    {typedef typename __add_const<_Tp>::type type;};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using add_const_t = typename add_const<_Tp>::type;
#endif

// add_volatile

template <class _Tp, bool = is_reference<_Tp>::value ||
                            is_function<_Tp>::value  ||
                            is_volatile<_Tp>::value  >
struct __add_volatile             {typedef _Tp type;};

template <class _Tp>
struct __add_volatile<_Tp, false> {typedef volatile _Tp type;};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY add_volatile
    {typedef typename __add_volatile<_Tp>::type type;};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using add_volatile_t = typename add_volatile<_Tp>::type;
#endif

// add_cv

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY add_cv
    {typedef typename add_const<typename add_volatile<_Tp>::type>::type type;};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using add_cv_t = typename add_cv<_Tp>::type;
#endif

// remove_reference

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_reference        {typedef _Tp type;};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_reference<_Tp&>  {typedef _Tp type;};
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_reference<_Tp&&> {typedef _Tp type;};
#endif

#if _LIBCPP_STD_VER > 11
template <class _Tp> using remove_reference_t = typename remove_reference<_Tp>::type;
#endif

// add_lvalue_reference

template <class _Tp, bool = __is_referenceable<_Tp>::value> struct __add_lvalue_reference_impl            { typedef _Tp  type; };
template <class _Tp                                       > struct __add_lvalue_reference_impl<_Tp, true> { typedef _Tp& type; };

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY add_lvalue_reference
{typedef typename __add_lvalue_reference_impl<_Tp>::type type;};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using add_lvalue_reference_t = typename add_lvalue_reference<_Tp>::type;
#endif

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp, bool = __is_referenceable<_Tp>::value> struct __add_rvalue_reference_impl            { typedef _Tp   type; };
template <class _Tp                                       > struct __add_rvalue_reference_impl<_Tp, true> { typedef _Tp&& type; };

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY add_rvalue_reference
{typedef typename __add_rvalue_reference_impl<_Tp>::type type;};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using add_rvalue_reference_t = typename add_rvalue_reference<_Tp>::type;
#endif

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp> _Tp&& __declval(int);
template <class _Tp> _Tp   __declval(long);

template <class _Tp>
decltype(_VSTD::__declval<_Tp>(0))
declval() _NOEXCEPT;

#else  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp>
typename add_lvalue_reference<_Tp>::type
declval();

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

// __uncvref

template <class _Tp>
struct __uncvref  {
    typedef typename remove_cv<typename remove_reference<_Tp>::type>::type type;
};

template <class _Tp>
struct __unconstref {
    typedef typename remove_const<typename remove_reference<_Tp>::type>::type type;
};

// __is_same_uncvref

template <class _Tp, class _Up>
struct __is_same_uncvref : is_same<typename __uncvref<_Tp>::type,
                                   typename __uncvref<_Up>::type> {};

struct __any
{
    __any(...);
};

// remove_pointer

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_pointer                      {typedef _Tp type;};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_pointer<_Tp*>                {typedef _Tp type;};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_pointer<_Tp* const>          {typedef _Tp type;};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_pointer<_Tp* volatile>       {typedef _Tp type;};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_pointer<_Tp* const volatile> {typedef _Tp type;};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using remove_pointer_t = typename remove_pointer<_Tp>::type;
#endif

// add_pointer

template <class _Tp, 
        bool = __is_referenceable<_Tp>::value || 
                is_same<typename remove_cv<_Tp>::type, void>::value>
struct __add_pointer_impl
    {typedef typename remove_reference<_Tp>::type* type;};
template <class _Tp> struct __add_pointer_impl<_Tp, false> 
    {typedef _Tp type;};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY add_pointer
    {typedef typename __add_pointer_impl<_Tp>::type type;};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using add_pointer_t = typename add_pointer<_Tp>::type;
#endif

// is_signed

template <class _Tp, bool = is_integral<_Tp>::value>
struct __libcpp_is_signed_impl : public _LIBCPP_BOOL_CONSTANT(_Tp(-1) < _Tp(0)) {};

template <class _Tp>
struct __libcpp_is_signed_impl<_Tp, false> : public true_type {};  // floating point

template <class _Tp, bool = is_arithmetic<_Tp>::value>
struct __libcpp_is_signed : public __libcpp_is_signed_impl<_Tp> {};

template <class _Tp> struct __libcpp_is_signed<_Tp, false> : public false_type {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_signed : public __libcpp_is_signed<_Tp> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_signed_v
    = is_signed<_Tp>::value;
#endif

// is_unsigned

template <class _Tp, bool = is_integral<_Tp>::value>
struct __libcpp_is_unsigned_impl : public _LIBCPP_BOOL_CONSTANT(_Tp(0) < _Tp(-1)) {};

template <class _Tp>
struct __libcpp_is_unsigned_impl<_Tp, false> : public false_type {};  // floating point

template <class _Tp, bool = is_arithmetic<_Tp>::value>
struct __libcpp_is_unsigned : public __libcpp_is_unsigned_impl<_Tp> {};

template <class _Tp> struct __libcpp_is_unsigned<_Tp, false> : public false_type {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_unsigned : public __libcpp_is_unsigned<_Tp> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_unsigned_v
    = is_unsigned<_Tp>::value;
#endif

// rank

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY rank
    : public integral_constant<size_t, 0> {};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY rank<_Tp[]>
    : public integral_constant<size_t, rank<_Tp>::value + 1> {};
template <class _Tp, size_t _Np> struct _LIBCPP_TYPE_VIS_ONLY rank<_Tp[_Np]>
    : public integral_constant<size_t, rank<_Tp>::value + 1> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR size_t rank_v
    = rank<_Tp>::value;
#endif

// extent

template <class _Tp, unsigned _Ip = 0> struct _LIBCPP_TYPE_VIS_ONLY extent
    : public integral_constant<size_t, 0> {};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY extent<_Tp[], 0>
    : public integral_constant<size_t, 0> {};
template <class _Tp, unsigned _Ip> struct _LIBCPP_TYPE_VIS_ONLY extent<_Tp[], _Ip>
    : public integral_constant<size_t, extent<_Tp, _Ip-1>::value> {};
template <class _Tp, size_t _Np> struct _LIBCPP_TYPE_VIS_ONLY extent<_Tp[_Np], 0>
    : public integral_constant<size_t, _Np> {};
template <class _Tp, size_t _Np, unsigned _Ip> struct _LIBCPP_TYPE_VIS_ONLY extent<_Tp[_Np], _Ip>
    : public integral_constant<size_t, extent<_Tp, _Ip-1>::value> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp, unsigned _Ip = 0> _LIBCPP_CONSTEXPR size_t extent_v
    = extent<_Tp, _Ip>::value;
#endif

// remove_extent

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_extent
    {typedef _Tp type;};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_extent<_Tp[]>
    {typedef _Tp type;};
template <class _Tp, size_t _Np> struct _LIBCPP_TYPE_VIS_ONLY remove_extent<_Tp[_Np]>
    {typedef _Tp type;};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using remove_extent_t = typename remove_extent<_Tp>::type;
#endif

// remove_all_extents

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_all_extents
    {typedef _Tp type;};
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY remove_all_extents<_Tp[]>
    {typedef typename remove_all_extents<_Tp>::type type;};
template <class _Tp, size_t _Np> struct _LIBCPP_TYPE_VIS_ONLY remove_all_extents<_Tp[_Np]>
    {typedef typename remove_all_extents<_Tp>::type type;};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using remove_all_extents_t = typename remove_all_extents<_Tp>::type;
#endif

// decay

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY decay
{
private:
    typedef typename remove_reference<_Tp>::type _Up;
public:
    typedef typename conditional
                     <
                         is_array<_Up>::value,
                         typename remove_extent<_Up>::type*,
                         typename conditional
                         <
                              is_function<_Up>::value,
                              typename add_pointer<_Up>::type,
                              typename remove_cv<_Up>::type
                         >::type
                     >::type type;
};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using decay_t = typename decay<_Tp>::type;
#endif

// is_abstract

namespace __is_abstract_imp
{
template <class _Tp> char  __test(_Tp (*)[1]);
template <class _Tp> __two __test(...);
}

template <class _Tp, bool = is_class<_Tp>::value>
struct __libcpp_abstract : public integral_constant<bool, sizeof(__is_abstract_imp::__test<_Tp>(0)) != 1> {};

template <class _Tp> struct __libcpp_abstract<_Tp, false> : public false_type {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_abstract : public __libcpp_abstract<_Tp> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_abstract_v
    = is_abstract<_Tp>::value;
#endif

// is_final

#if defined(_LIBCPP_HAS_IS_FINAL)
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY
__libcpp_is_final : public integral_constant<bool, __is_final(_Tp)> {};
#else
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY
__libcpp_is_final : public false_type {};
#endif

#if defined(_LIBCPP_HAS_IS_FINAL) && _LIBCPP_STD_VER > 11
template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY
is_final : public integral_constant<bool, __is_final(_Tp)> {};
#endif

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_final_v
    = is_final<_Tp>::value;
#endif

// is_base_of

#ifdef _LIBCPP_HAS_IS_BASE_OF

template <class _Bp, class _Dp>
struct _LIBCPP_TYPE_VIS_ONLY is_base_of
    : public integral_constant<bool, __is_base_of(_Bp, _Dp)> {};

#else  // _LIBCPP_HAS_IS_BASE_OF

namespace __is_base_of_imp
{
template <class _Tp>
struct _Dst
{
    _Dst(const volatile _Tp &);
};
template <class _Tp>
struct _Src
{
    operator const volatile _Tp &();
    template <class _Up> operator const _Dst<_Up> &();
};
template <size_t> struct __one { typedef char type; };
template <class _Bp, class _Dp> typename __one<sizeof(_Dst<_Bp>(declval<_Src<_Dp> >()))>::type __test(int);
template <class _Bp, class _Dp> __two __test(...);
}

template <class _Bp, class _Dp>
struct _LIBCPP_TYPE_VIS_ONLY is_base_of
    : public integral_constant<bool, is_class<_Bp>::value &&
                                     sizeof(__is_base_of_imp::__test<_Bp, _Dp>(0)) == 2> {};

#endif  // _LIBCPP_HAS_IS_BASE_OF

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Bp, class _Dp> _LIBCPP_CONSTEXPR bool is_base_of_v
    = is_base_of<_Bp, _Dp>::value;
#endif

// is_convertible

#if __has_feature(is_convertible_to) && !defined(_LIBCPP_USE_IS_CONVERTIBLE_FALLBACK)

template <class _T1, class _T2> struct _LIBCPP_TYPE_VIS_ONLY is_convertible
    : public integral_constant<bool, _is_convertible_to(_T1, _T2) &&
                                     !is_abstract<_T2>::value> {};

#else  // __has_feature(is_convertible_to)

namespace _is_convertible_imp
{
template <class _Tp> void  __test_convert(_Tp);

template <class _From, class _To, class = void>
struct _is_convertible_test : public false_type {};

template <class _From, class _To>
struct _is_convertible_test<_From, _To,
    decltype(_VSTD::_is_convertible_imp::__test_convert<_To>(_VSTD::declval<_From>()))> : public true_type
{};

template <class _Tp, bool _IsArray =    is_array<_Tp>::value,
                     bool _IsFunction = is_function<_Tp>::value,
                     bool _IsVoid =     is_void<_Tp>::value>
                     struct __is_array_function_or_void                          {enum {value = 0};};
template <class _Tp> struct __is_array_function_or_void<_Tp, true, false, false> {enum {value = 1};};
template <class _Tp> struct __is_array_function_or_void<_Tp, false, true, false> {enum {value = 2};};
template <class _Tp> struct __is_array_function_or_void<_Tp, false, false, true> {enum {value = 3};};
}

template <class _Tp,
    unsigned = _is_convertible_imp::__is_array_function_or_void<typename remove_reference<_Tp>::type>::value>
struct _is_convertible_check
{
    static const size_t __v = 0;
};

template <class _Tp>
struct _is_convertible_check<_Tp, 0>
{
    static const size_t __v = sizeof(_Tp);
};

template <class _T1, class _T2,
    unsigned _T1_is_array_function_or_void = _is_convertible_imp::__is_array_function_or_void<_T1>::value,
    unsigned _T2_is_array_function_or_void = _is_convertible_imp::__is_array_function_or_void<_T2>::value>
struct _is_convertible
    : public integral_constant<bool,
        _is_convertible_imp::_is_convertible_test<_T1, _T2>::value
#if defined(_LIBCPP_HAS_NO_RVALUE_REFERENCES)
         && !(!is_function<_T1>::value && !is_reference<_T1>::value && is_reference<_T2>::value
              && (!is_const<typename remove_reference<_T2>::type>::value
                  || is_volatile<typename remove_reference<_T2>::type>::value)
                  && (is_same<typename remove_cv<_T1>::type,
                              typename remove_cv<typename remove_reference<_T2>::type>::type>::value
                      || is_base_of<typename remove_reference<_T2>::type, _T1>::value))
#endif
    >
{};

template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 0, 1> : public false_type {};
template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 1, 1> : public false_type {};
template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 2, 1> : public false_type {};
template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 3, 1> : public false_type {};

template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 0, 2> : public false_type {};
template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 1, 2> : public false_type {};
template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 2, 2> : public false_type {};
template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 3, 2> : public false_type {};

template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 0, 3> : public false_type {};
template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 1, 3> : public false_type {};
template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 2, 3> : public false_type {};
template <class _T1, class _T2> struct _is_convertible<_T1, _T2, 3, 3> : public true_type {};

template <class _T1, class _T2> struct _LIBCPP_TYPE_VIS_ONLY is_convertible
    : public _is_convertible<_T1, _T2>
{
    static const size_t __complete_check1 = _is_convertible_check<_T1>::__v;
    static const size_t __complete_check2 = _is_convertible_check<_T2>::__v;
};

#endif  // __has_feature(is_convertible_to)

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _From, class _To> _LIBCPP_CONSTEXPR bool is_convertible_v
    = is_convertible<_From, _To>::value;
#endif

// is_empty

#if __has_feature(is_empty) || (_GNUC_VER >= 407)

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_empty
    : public integral_constant<bool, __is_empty(_Tp)> {};

#else  // __has_feature(is_empty)

template <class _Tp>
struct __is_empty1
    : public _Tp
{
    double __lx;
};

struct __is_empty2
{
    double __lx;
};

template <class _Tp, bool = is_class<_Tp>::value>
struct __libcpp_empty : public integral_constant<bool, sizeof(__is_empty1<_Tp>) == sizeof(__is_empty2)> {};

template <class _Tp> struct __libcpp_empty<_Tp, false> : public false_type {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_empty : public __libcpp_empty<_Tp> {};

#endif  // __has_feature(is_empty)

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_empty_v
    = is_empty<_Tp>::value;
#endif

// is_polymorphic

#if __has_feature(is_polymorphic) || defined(_LIBCPP_MSVC)

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_polymorphic
    : public integral_constant<bool, __is_polymorphic(_Tp)> {};

#else

template<typename _Tp> char &__is_polymorphic_impl(
    typename enable_if<sizeof((_Tp*)dynamic_cast<const volatile void*>(declval<_Tp*>())) != 0,
                       int>::type);
template<typename _Tp> __two &__is_polymorphic_impl(...);

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_polymorphic
    : public integral_constant<bool, sizeof(__is_polymorphic_impl<_Tp>(0)) == 1> {};

#endif // __has_feature(is_polymorphic)

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_polymorphic_v
    = is_polymorphic<_Tp>::value;
#endif

// has_virtual_destructor

#if __has_feature(has_virtual_destructor) || (_GNUC_VER >= 403)

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY has_virtual_destructor
    : public integral_constant<bool, __has_virtual_destructor(_Tp)> {};

#else

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY has_virtual_destructor
    : public false_type {};

#endif

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool has_virtual_destructor_v
    = has_virtual_destructor<_Tp>::value;
#endif

// alignment_of

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY alignment_of
    : public integral_constant<size_t, __alignof__(_Tp)> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR size_t alignment_of_v
    = alignment_of<_Tp>::value;
#endif

// aligned_storage

template <class _Hp, class _Tp>
struct __type_list
{
    typedef _Hp _Head;
    typedef _Tp _Tail;
};

struct __nat
{
#ifndef _LIBCPP_HAS_NO_DELETED_FUNCTIONS
    __nat() = delete;
    __nat(const __nat&) = delete;
    __nat& operator=(const __nat&) = delete;
    ~__nat() = delete;
#endif
};

template <class _Tp>
struct __align_type
{
    static const size_t value = alignment_of<_Tp>::value;
    typedef _Tp type;
};

struct __struct_double {long double __lx;};
struct __struct_double4 {double __lx[4];};

typedef
    __type_list<__align_type<unsigned char>,
    __type_list<__align_type<unsigned short>,
    __type_list<__align_type<unsigned int>,
    __type_list<__align_type<unsigned long>,
    __type_list<__align_type<unsigned long long>,
    __type_list<__align_type<double>,
    __type_list<__align_type<long double>,
    __type_list<__align_type<__struct_double>,
    __type_list<__align_type<__struct_double4>,
    __type_list<__align_type<int*>,
    __nat
    > > > > > > > > > > __all_types;

template <class _TL, size_t _Align> struct __find_pod;

template <class _Hp, size_t _Align>
struct __find_pod<__type_list<_Hp, __nat>, _Align>
{
    typedef typename conditional<
                             _Align == _Hp::value,
                             typename _Hp::type,
                             void
                         >::type type;
};

template <class _Hp, class _Tp, size_t _Align>
struct __find_pod<__type_list<_Hp, _Tp>, _Align>
{
    typedef typename conditional<
                             _Align == _Hp::value,
                             typename _Hp::type,
                             typename __find_pod<_Tp, _Align>::type
                         >::type type;
};

template <class _TL, size_t _Len> struct __find_max_align;

template <class _Hp, size_t _Len>
struct __find_max_align<__type_list<_Hp, __nat>, _Len> : public integral_constant<size_t, _Hp::value> {};

template <size_t _Len, size_t _A1, size_t _A2>
struct __select_align
{
private:
    static const size_t __min = _A2 < _A1 ? _A2 : _A1;
    static const size_t __max = _A1 < _A2 ? _A2 : _A1;
public:
    static const size_t value = _Len < __max ? __min : __max;
};

template <class _Hp, class _Tp, size_t _Len>
struct __find_max_align<__type_list<_Hp, _Tp>, _Len>
    : public integral_constant<size_t, __select_align<_Len, _Hp::value, __find_max_align<_Tp, _Len>::value>::value> {};

template <size_t _Len, size_t _Align = __find_max_align<__all_types, _Len>::value>
struct _LIBCPP_TYPE_VIS_ONLY aligned_storage
{
    typedef typename __find_pod<__all_types, _Align>::type _Aligner;
    static_assert(!is_void<_Aligner>::value, "");
    union type
    {
        _Aligner __align;
        unsigned char __data[(_Len + _Align - 1)/_Align * _Align];
    };
};

#if _LIBCPP_STD_VER > 11
template <size_t _Len, size_t _Align = __find_max_align<__all_types, _Len>::value>
    using aligned_storage_t = typename aligned_storage<_Len, _Align>::type;
#endif

#define _CREATE_ALIGNED_STORAGE_SPECIALIZATION(n) \
template <size_t _Len>\
struct _LIBCPP_TYPE_VIS_ONLY aligned_storage<_Len, n>\
{\
    struct _ALIGNAS(n) type\
    {\
        unsigned char __lx[(_Len + n - 1)/n * n];\
    };\
}

_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x1);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x2);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x4);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x8);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x10);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x20);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x40);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x80);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x100);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x200);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x400);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x800);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x1000);
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x2000);
// MSDN says that MSVC does not support alignment beyond 8192 (=0x2000)
#if !defined(_LIBCPP_MSVC)
_CREATE_ALIGNED_STORAGE_SPECIALIZATION(0x4000);
#endif // !_LIBCPP_MSVC

#undef _CREATE_ALIGNED_STORAGE_SPECIALIZATION

#ifndef _LIBCPP_HAS_NO_VARIADICS

// aligned_union

template <size_t _I0, size_t ..._In>
struct __static_max;

template <size_t _I0>
struct __static_max<_I0>
{
    static const size_t value = _I0;
};

template <size_t _I0, size_t _I1, size_t ..._In>
struct __static_max<_I0, _I1, _In...>
{
    static const size_t value = _I0 >= _I1 ? __static_max<_I0, _In...>::value :
                                             __static_max<_I1, _In...>::value;
};

template <size_t _Len, class _Type0, class ..._Types>
struct aligned_union
{
    static const size_t alignment_value = __static_max<__alignof__(_Type0),
                                                       __alignof__(_Types)...>::value;
    static const size_t __len = __static_max<_Len, sizeof(_Type0),
                                             sizeof(_Types)...>::value;
    typedef typename aligned_storage<__len, alignment_value>::type type;
};

#if _LIBCPP_STD_VER > 11
template <size_t _Len, class ..._Types> using aligned_union_t = typename aligned_union<_Len, _Types...>::type;
#endif

#endif  // _LIBCPP_HAS_NO_VARIADICS

template <class _Tp>
struct __numeric_type
{
   static void __test(...);
   static float __test(float);
   static double __test(char);
   static double __test(int);
   static double __test(unsigned);
   static double __test(long);
   static double __test(unsigned long);
   static double __test(long long);
   static double __test(unsigned long long);
   static double __test(double);
   static long double __test(long double);

   typedef decltype(__test(declval<_Tp>())) type;
   static const bool value = !is_same<type, void>::value;
};

template <>
struct __numeric_type<void>
{
   static const bool value = true;
};

// __promote

template <class _A1, class _A2 = void, class _A3 = void,
          bool = __numeric_type<_A1>::value &&
                 __numeric_type<_A2>::value &&
                 __numeric_type<_A3>::value>
class __promote_imp
{
public:
    static const bool value = false;
};

template <class _A1, class _A2, class _A3>
class __promote_imp<_A1, _A2, _A3, true>
{
private:
    typedef typename __promote_imp<_A1>::type __type1;
    typedef typename __promote_imp<_A2>::type __type2;
    typedef typename __promote_imp<_A3>::type __type3;
public:
    typedef decltype(__type1() + __type2() + __type3()) type;
    static const bool value = true;
};

template <class _A1, class _A2>
class __promote_imp<_A1, _A2, void, true>
{
private:
    typedef typename __promote_imp<_A1>::type __type1;
    typedef typename __promote_imp<_A2>::type __type2;
public:
    typedef decltype(__type1() + __type2()) type;
    static const bool value = true;
};

template <class _A1>
class __promote_imp<_A1, void, void, true>
{
public:
    typedef typename __numeric_type<_A1>::type type;
    static const bool value = true;
};

template <class _A1, class _A2 = void, class _A3 = void>
class __promote : public __promote_imp<_A1, _A2, _A3> {};

// make_signed / make_unsigned

typedef
    __type_list<signed char,
    __type_list<signed short,
    __type_list<signed int,
    __type_list<signed long,
    __type_list<signed long long,
#ifndef _LIBCPP_HAS_NO_INT128
    __type_list<__int128_t,
#endif
    __nat
#ifndef _LIBCPP_HAS_NO_INT128
    >
#endif
    > > > > > __signed_types;

typedef
    __type_list<unsigned char,
    __type_list<unsigned short,
    __type_list<unsigned int,
    __type_list<unsigned long,
    __type_list<unsigned long long,
#ifndef _LIBCPP_HAS_NO_INT128
    __type_list<__uint128_t,
#endif
    __nat
#ifndef _LIBCPP_HAS_NO_INT128
    >
#endif
    > > > > > __unsigned_types;

template <class _TypeList, size_t _Size, bool = _Size <= sizeof(typename _TypeList::_Head)> struct __find_first;

template <class _Hp, class _Tp, size_t _Size>
struct __find_first<__type_list<_Hp, _Tp>, _Size, true>
{
    typedef _Hp type;
};

template <class _Hp, class _Tp, size_t _Size>
struct __find_first<__type_list<_Hp, _Tp>, _Size, false>
{
    typedef typename __find_first<_Tp, _Size>::type type;
};

template <class _Tp, class _Up, bool = is_const<typename remove_reference<_Tp>::type>::value,
                             bool = is_volatile<typename remove_reference<_Tp>::type>::value>
struct __apply_cv
{
    typedef _Up type;
};

template <class _Tp, class _Up>
struct __apply_cv<_Tp, _Up, true, false>
{
    typedef const _Up type;
};

template <class _Tp, class _Up>
struct __apply_cv<_Tp, _Up, false, true>
{
    typedef volatile _Up type;
};

template <class _Tp, class _Up>
struct __apply_cv<_Tp, _Up, true, true>
{
    typedef const volatile _Up type;
};

template <class _Tp, class _Up>
struct __apply_cv<_Tp&, _Up, false, false>
{
    typedef _Up& type;
};

template <class _Tp, class _Up>
struct __apply_cv<_Tp&, _Up, true, false>
{
    typedef const _Up& type;
};

template <class _Tp, class _Up>
struct __apply_cv<_Tp&, _Up, false, true>
{
    typedef volatile _Up& type;
};

template <class _Tp, class _Up>
struct __apply_cv<_Tp&, _Up, true, true>
{
    typedef const volatile _Up& type;
};

template <class _Tp, bool = is_integral<_Tp>::value || is_enum<_Tp>::value>
struct __make_signed {};

template <class _Tp>
struct __make_signed<_Tp, true>
{
    typedef typename __find_first<__signed_types, sizeof(_Tp)>::type type;
};

template <> struct __make_signed<bool,               true> {};
template <> struct __make_signed<  signed short,     true> {typedef short     type;};
template <> struct __make_signed<unsigned short,     true> {typedef short     type;};
template <> struct __make_signed<  signed int,       true> {typedef int       type;};
template <> struct __make_signed<unsigned int,       true> {typedef int       type;};
template <> struct __make_signed<  signed long,      true> {typedef long      type;};
template <> struct __make_signed<unsigned long,      true> {typedef long      type;};
template <> struct __make_signed<  signed long long, true> {typedef long long type;};
template <> struct __make_signed<unsigned long long, true> {typedef long long type;};
#ifndef _LIBCPP_HAS_NO_INT128
template <> struct __make_signed<__int128_t,         true> {typedef __int128_t type;};
template <> struct __make_signed<__uint128_t,        true> {typedef __int128_t type;};
#endif

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY make_signed
{
    typedef typename __apply_cv<_Tp, typename __make_signed<typename remove_cv<_Tp>::type>::type>::type type;
};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using make_signed_t = typename make_signed<_Tp>::type;
#endif

template <class _Tp, bool = is_integral<_Tp>::value || is_enum<_Tp>::value>
struct __make_unsigned {};

template <class _Tp>
struct __make_unsigned<_Tp, true>
{
    typedef typename __find_first<__unsigned_types, sizeof(_Tp)>::type type;
};

template <> struct __make_unsigned<bool,               true> {};
template <> struct __make_unsigned<  signed short,     true> {typedef unsigned short     type;};
template <> struct __make_unsigned<unsigned short,     true> {typedef unsigned short     type;};
template <> struct __make_unsigned<  signed int,       true> {typedef unsigned int       type;};
template <> struct __make_unsigned<unsigned int,       true> {typedef unsigned int       type;};
template <> struct __make_unsigned<  signed long,      true> {typedef unsigned long      type;};
template <> struct __make_unsigned<unsigned long,      true> {typedef unsigned long      type;};
template <> struct __make_unsigned<  signed long long, true> {typedef unsigned long long type;};
template <> struct __make_unsigned<unsigned long long, true> {typedef unsigned long long type;};
#ifndef _LIBCPP_HAS_NO_INT128
template <> struct __make_unsigned<__int128_t,         true> {typedef __uint128_t        type;};
template <> struct __make_unsigned<__uint128_t,        true> {typedef __uint128_t        type;};
#endif

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY make_unsigned
{
    typedef typename __apply_cv<_Tp, typename __make_unsigned<typename remove_cv<_Tp>::type>::type>::type type;
};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using make_unsigned_t = typename make_unsigned<_Tp>::type;
#endif

#ifdef _LIBCPP_HAS_NO_VARIADICS

template <class _Tp, class _Up = void, class _Vp = void>
struct _LIBCPP_TYPE_VIS_ONLY common_type
{
public:
    typedef typename common_type<typename common_type<_Tp, _Up>::type, _Vp>::type type;
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY common_type<_Tp, void, void>
{
public:
    typedef typename decay<_Tp>::type type;
};

template <class _Tp, class _Up>
struct _LIBCPP_TYPE_VIS_ONLY common_type<_Tp, _Up, void>
{
    typedef typename decay<decltype(
        true ? _VSTD::declval<_Tp>() : _VSTD::declval<_Up>()
      )>::type type;
};

#else  // _LIBCPP_HAS_NO_VARIADICS

// bullet 1 - sizeof...(Tp) == 0

template <class ..._Tp>
struct _LIBCPP_TYPE_VIS_ONLY common_type {};

// bullet 2 - sizeof...(Tp) == 1

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY common_type<_Tp>
{
    typedef typename decay<_Tp>::type type;
};

// bullet 3 - sizeof...(Tp) == 2

template <class _Tp, class _Up, class = void>
struct __common_type2 {};

template <class _Tp, class _Up>
struct __common_type2<_Tp, _Up,
    typename __void_t<decltype(
        true ? _VSTD::declval<_Tp>() : _VSTD::declval<_Up>()
    )>::type>
{
    typedef typename decay<decltype(
        true ? _VSTD::declval<_Tp>() : _VSTD::declval<_Up>()
    )>::type type;
};

template <class _Tp, class _Up>
struct _LIBCPP_TYPE_VIS_ONLY common_type<_Tp, _Up>
    : __common_type2<_Tp, _Up> {};

// bullet 4 - sizeof...(Tp) > 2

template <class ...Tp> struct __common_types;

template <class, class = void>
struct __common_type_impl {};

template <class _Tp, class _Up, class ..._Vp>
struct __common_type_impl<__common_types<_Tp, _Up, _Vp...>,
    typename __void_t<typename common_type<_Tp, _Up>::type>::type>
{
    typedef typename common_type<
        typename common_type<_Tp, _Up>::type, _Vp...
    >::type type;
};

template <class _Tp, class _Up, class ..._Vp>
struct _LIBCPP_TYPE_VIS_ONLY common_type<_Tp, _Up, _Vp...>
    : __common_type_impl<__common_types<_Tp, _Up, _Vp...> > {};

#if _LIBCPP_STD_VER > 11
template <class ..._Tp> using common_type_t = typename common_type<_Tp...>::type;
#endif

#endif  // _LIBCPP_HAS_NO_VARIADICS

// is_assignable

template<typename, typename _Tp> struct __select_2nd { typedef _Tp type; };

template <class _Tp, class _Arg>
typename __select_2nd<decltype((_VSTD::declval<_Tp>() = _VSTD::declval<_Arg>())), true_type>::type
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
__is_assignable_test(_Tp&&, _Arg&&);
#else
__is_assignable_test(_Tp, _Arg&);
#endif

template <class _Arg>
false_type
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
__is_assignable_test(__any, _Arg&&);
#else
__is_assignable_test(__any, _Arg&);
#endif

template <class _Tp, class _Arg, bool = is_void<_Tp>::value || is_void<_Arg>::value>
struct __is_assignable_imp
    : public common_type
        <
            decltype(_VSTD::__is_assignable_test(declval<_Tp>(), declval<_Arg>()))
        >::type {};

template <class _Tp, class _Arg>
struct __is_assignable_imp<_Tp, _Arg, true>
    : public false_type
{
};

template <class _Tp, class _Arg>
struct is_assignable
    : public __is_assignable_imp<_Tp, _Arg> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp, class _Arg> _LIBCPP_CONSTEXPR bool is_assignable_v
    = is_assignable<_Tp, _Arg>::value;
#endif

// is_copy_assignable

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_copy_assignable
    : public is_assignable<typename add_lvalue_reference<_Tp>::type,
                  typename add_lvalue_reference<typename add_const<_Tp>::type>::type> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_copy_assignable_v
    = is_copy_assignable<_Tp>::value;
#endif

// is_move_assignable

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_move_assignable
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    : public is_assignable<typename add_lvalue_reference<_Tp>::type,
                     const typename add_rvalue_reference<_Tp>::type> {};
#else
    : public is_copy_assignable<_Tp> {};
#endif

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_move_assignable_v
    = is_move_assignable<_Tp>::value;
#endif

// is_destructible

//  if it's a reference, return true
//  if it's a function, return false
//  if it's   void,     return false
//  if it's an array of unknown bound, return false
//  Otherwise, return "std::declval<_Up&>().~_Up()" is well-formed
//    where _Up is remove_all_extents<_Tp>::type

template <class>
struct __is_destructible_apply { typedef int type; };

template <typename _Tp>
struct __is_destructor_wellformed {
    template <typename _Tp1>
    static char  __test (
        typename __is_destructible_apply<decltype(_VSTD::declval<_Tp1&>().~_Tp1())>::type
    );

    template <typename _Tp1>
    static __two __test (...);
    
    static const bool value = sizeof(__test<_Tp>(12)) == sizeof(char);
};

template <class _Tp, bool>
struct __destructible_imp;

template <class _Tp>
struct __destructible_imp<_Tp, false> 
   : public _VSTD::integral_constant<bool, 
        __is_destructor_wellformed<typename _VSTD::remove_all_extents<_Tp>::type>::value> {};

template <class _Tp>
struct __destructible_imp<_Tp, true>
    : public _VSTD::true_type {};

template <class _Tp, bool>
struct __destructible_false;

template <class _Tp>
struct __destructible_false<_Tp, false> : public __destructible_imp<_Tp, _VSTD::is_reference<_Tp>::value> {};

template <class _Tp>
struct __destructible_false<_Tp, true> : public _VSTD::false_type {};

template <class _Tp>
struct is_destructible
    : public __destructible_false<_Tp, _VSTD::is_function<_Tp>::value> {};

template <class _Tp>
struct is_destructible<_Tp[]>
    : public _VSTD::false_type {};

template <>
struct is_destructible<void>
    : public _VSTD::false_type {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_destructible_v
    = is_destructible<_Tp>::value;
#endif

// move

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
typename remove_reference<_Tp>::type&&
move(_Tp&& __t) _NOEXCEPT
{
    typedef typename remove_reference<_Tp>::type _Up;
    return static_cast<_Up&&>(__t);
}

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
_Tp&&
forward(typename remove_reference<_Tp>::type& __t) _NOEXCEPT
{
    return static_cast<_Tp&&>(__t);
}

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
_Tp&&
forward(typename remove_reference<_Tp>::type&& __t) _NOEXCEPT
{
    static_assert(!is_lvalue_reference<_Tp>::value,
                  "Can not forward an rvalue as an lvalue.");
    return static_cast<_Tp&&>(__t);
}

#else  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
_Tp&
move(_Tp& __t)
{
    return __t;
}

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
const _Tp&
move(const _Tp& __t)
{
    return __t;
}

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
_Tp&
forward(typename remove_reference<_Tp>::type& __t) _NOEXCEPT
{
    return __t;
}


template <class _Tp>
class __rv
{
    typedef typename remove_reference<_Tp>::type _Trr;
    _Trr& t_;
public:
    _LIBCPP_INLINE_VISIBILITY
    _Trr* operator->() {return &t_;}
    _LIBCPP_INLINE_VISIBILITY
    explicit __rv(_Trr& __t) : t_(__t) {}
};

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
typename decay<_Tp>::type
__decay_copy(_Tp&& __t)
{
    return _VSTD::forward<_Tp>(__t);
}

#else

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
typename decay<_Tp>::type
__decay_copy(const _Tp& __t)
{
    return _VSTD::forward<_Tp>(__t);
}

#endif

#ifndef _LIBCPP_HAS_NO_VARIADICS

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...), true, false>
{
    typedef _Class _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...), true, false>
{
    typedef _Class _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const, true, false>
{
    typedef _Class const _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...) const, true, false>
{
    typedef _Class const _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...) volatile, true, false>
{
    typedef _Class volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...) volatile, true, false>
{
    typedef _Class volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const volatile, true, false>
{
    typedef _Class const volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...) const volatile, true, false>
{
    typedef _Class const volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

#if __has_feature(cxx_reference_qualified_functions) || \
    (defined(_GNUC_VER) && _GNUC_VER >= 409)

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...) &, true, false>
{
    typedef _Class& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...) &, true, false>
{
    typedef _Class& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const&, true, false>
{
    typedef _Class const& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...) const&, true, false>
{
    typedef _Class const& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...) volatile&, true, false>
{
    typedef _Class volatile& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...) volatile&, true, false>
{
    typedef _Class volatile& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const volatile&, true, false>
{
    typedef _Class const volatile& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...) const volatile&, true, false>
{
    typedef _Class const volatile& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...) &&, true, false>
{
    typedef _Class&& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...) &&, true, false>
{
    typedef _Class&& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const&&, true, false>
{
    typedef _Class const&& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...) const&&, true, false>
{
    typedef _Class const&& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...) volatile&&, true, false>
{
    typedef _Class volatile&& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...) volatile&&, true, false>
{
    typedef _Class volatile&& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param...) const volatile&&, true, false>
{
    typedef _Class const volatile&& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param...);
};

template <class _Rp, class _Class, class ..._Param>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_Param..., ...) const volatile&&, true, false>
{
    typedef _Class const volatile&& _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_Param..., ...);
};

#endif  // __has_feature(cxx_reference_qualified_functions) || _GNUC_VER >= 409

#else  // _LIBCPP_HAS_NO_VARIADICS

template <class _Rp, class _Class>
struct __member_pointer_traits_imp<_Rp (_Class::*)(), true, false>
{
    typedef _Class _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) ();
};

template <class _Rp, class _Class>
struct __member_pointer_traits_imp<_Rp (_Class::*)(...), true, false>
{
    typedef _Class _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (...);
};

template <class _Rp, class _Class, class _P0>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0), true, false>
{
    typedef _Class _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0);
};

template <class _Rp, class _Class, class _P0>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, ...), true, false>
{
    typedef _Class _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, ...);
};

template <class _Rp, class _Class, class _P0, class _P1>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1), true, false>
{
    typedef _Class _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1);
};

template <class _Rp, class _Class, class _P0, class _P1>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, ...), true, false>
{
    typedef _Class _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, ...);
};

template <class _Rp, class _Class, class _P0, class _P1, class _P2>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, _P2), true, false>
{
    typedef _Class _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, _P2);
};

template <class _Rp, class _Class, class _P0, class _P1, class _P2>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, _P2, ...), true, false>
{
    typedef _Class _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, _P2, ...);
};

template <class _Rp, class _Class>
struct __member_pointer_traits_imp<_Rp (_Class::*)() const, true, false>
{
    typedef _Class const _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) ();
};

template <class _Rp, class _Class>
struct __member_pointer_traits_imp<_Rp (_Class::*)(...) const, true, false>
{
    typedef _Class const _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (...);
};

template <class _Rp, class _Class, class _P0>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0) const, true, false>
{
    typedef _Class const _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0);
};

template <class _Rp, class _Class, class _P0>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, ...) const, true, false>
{
    typedef _Class const _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, ...);
};

template <class _Rp, class _Class, class _P0, class _P1>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1) const, true, false>
{
    typedef _Class const _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1);
};

template <class _Rp, class _Class, class _P0, class _P1>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, ...) const, true, false>
{
    typedef _Class const _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, ...);
};

template <class _Rp, class _Class, class _P0, class _P1, class _P2>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, _P2) const, true, false>
{
    typedef _Class const _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, _P2);
};

template <class _Rp, class _Class, class _P0, class _P1, class _P2>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, _P2, ...) const, true, false>
{
    typedef _Class const _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, _P2, ...);
};

template <class _Rp, class _Class>
struct __member_pointer_traits_imp<_Rp (_Class::*)() volatile, true, false>
{
    typedef _Class volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) ();
};

template <class _Rp, class _Class>
struct __member_pointer_traits_imp<_Rp (_Class::*)(...) volatile, true, false>
{
    typedef _Class volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (...);
};

template <class _Rp, class _Class, class _P0>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0) volatile, true, false>
{
    typedef _Class volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0);
};

template <class _Rp, class _Class, class _P0>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, ...) volatile, true, false>
{
    typedef _Class volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, ...);
};

template <class _Rp, class _Class, class _P0, class _P1>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1) volatile, true, false>
{
    typedef _Class volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1);
};

template <class _Rp, class _Class, class _P0, class _P1>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, ...) volatile, true, false>
{
    typedef _Class volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, ...);
};

template <class _Rp, class _Class, class _P0, class _P1, class _P2>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, _P2) volatile, true, false>
{
    typedef _Class volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, _P2);
};

template <class _Rp, class _Class, class _P0, class _P1, class _P2>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, _P2, ...) volatile, true, false>
{
    typedef _Class volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, _P2, ...);
};

template <class _Rp, class _Class>
struct __member_pointer_traits_imp<_Rp (_Class::*)() const volatile, true, false>
{
    typedef _Class const volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) ();
};

template <class _Rp, class _Class>
struct __member_pointer_traits_imp<_Rp (_Class::*)(...) const volatile, true, false>
{
    typedef _Class const volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (...);
};

template <class _Rp, class _Class, class _P0>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0) const volatile, true, false>
{
    typedef _Class const volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0);
};

template <class _Rp, class _Class, class _P0>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, ...) const volatile, true, false>
{
    typedef _Class const volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, ...);
};

template <class _Rp, class _Class, class _P0, class _P1>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1) const volatile, true, false>
{
    typedef _Class const volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1);
};

template <class _Rp, class _Class, class _P0, class _P1>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, ...) const volatile, true, false>
{
    typedef _Class const volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, ...);
};

template <class _Rp, class _Class, class _P0, class _P1, class _P2>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, _P2) const volatile, true, false>
{
    typedef _Class const volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, _P2);
};

template <class _Rp, class _Class, class _P0, class _P1, class _P2>
struct __member_pointer_traits_imp<_Rp (_Class::*)(_P0, _P1, _P2, ...) const volatile, true, false>
{
    typedef _Class const volatile _ClassType;
    typedef _Rp _ReturnType;
    typedef _Rp (_FnType) (_P0, _P1, _P2, ...);
};

#endif  // _LIBCPP_HAS_NO_VARIADICS

template <class _Rp, class _Class>
struct __member_pointer_traits_imp<_Rp _Class::*, false, true>
{
    typedef _Class _ClassType;
    typedef _Rp _ReturnType;
};

template <class _MP>
struct __member_pointer_traits
    : public __member_pointer_traits_imp<typename remove_cv<_MP>::type,
                    is_member_function_pointer<_MP>::value,
                    is_member_object_pointer<_MP>::value>
{
//     typedef ... _ClassType;
//     typedef ... _ReturnType;
//     typedef ... _FnType;
};


template <class _DecayedFp>
struct __member_pointer_class_type {};

template <class _Ret, class _ClassType>
struct __member_pointer_class_type<_Ret _ClassType::*> {
  typedef _ClassType type;
};

// result_of

template <class _Callable> class result_of;

#ifdef _LIBCPP_HAS_NO_VARIADICS

template <class _Fn, bool, bool>
class __result_of
{
};

template <class _Fn>
class __result_of<_Fn(), true, false>
{
public:
    typedef decltype(declval<_Fn>()()) type;
};

template <class _Fn, class _A0>
class __result_of<_Fn(_A0), true, false>
{
public:
    typedef decltype(declval<_Fn>()(declval<_A0>())) type;
};

template <class _Fn, class _A0, class _A1>
class __result_of<_Fn(_A0, _A1), true, false>
{
public:
    typedef decltype(declval<_Fn>()(declval<_A0>(), declval<_A1>())) type;
};

template <class _Fn, class _A0, class _A1, class _A2>
class __result_of<_Fn(_A0, _A1, _A2), true, false>
{
public:
    typedef decltype(declval<_Fn>()(declval<_A0>(), declval<_A1>(), declval<_A2>())) type;
};

template <class _MP, class _Tp, bool _IsMemberFunctionPtr>
struct __result_of_mp;

// member function pointer

template <class _MP, class _Tp>
struct __result_of_mp<_MP, _Tp, true>
    : public __identity<typename __member_pointer_traits<_MP>::_ReturnType>
{
};

// member data pointer

template <class _MP, class _Tp, bool>
struct __result_of_mdp;

template <class _Rp, class _Class, class _Tp>
struct __result_of_mdp<_Rp _Class::*, _Tp, false>
{
    typedef typename __apply_cv<decltype(*_VSTD::declval<_Tp>()), _Rp>::type& type;
};

template <class _Rp, class _Class, class _Tp>
struct __result_of_mdp<_Rp _Class::*, _Tp, true>
{
    typedef typename __apply_cv<_Tp, _Rp>::type& type;
};

template <class _Rp, class _Class, class _Tp>
struct __result_of_mp<_Rp _Class::*, _Tp, false>
    : public __result_of_mdp<_Rp _Class::*, _Tp,
            is_base_of<_Class, typename remove_reference<_Tp>::type>::value>
{
};



template <class _Fn, class _Tp>
class __result_of<_Fn(_Tp), false, true>  // _Fn must be member pointer
    : public __result_of_mp<typename remove_reference<_Fn>::type,
                            _Tp,
                            is_member_function_pointer<typename remove_reference<_Fn>::type>::value>
{
};

template <class _Fn, class _Tp, class _A0>
class __result_of<_Fn(_Tp, _A0), false, true>  // _Fn must be member pointer
    : public __result_of_mp<typename remove_reference<_Fn>::type,
                            _Tp,
                            is_member_function_pointer<typename remove_reference<_Fn>::type>::value>
{
};

template <class _Fn, class _Tp, class _A0, class _A1>
class __result_of<_Fn(_Tp, _A0, _A1), false, true>  // _Fn must be member pointer
    : public __result_of_mp<typename remove_reference<_Fn>::type,
                            _Tp,
                            is_member_function_pointer<typename remove_reference<_Fn>::type>::value>
{
};

template <class _Fn, class _Tp, class _A0, class _A1, class _A2>
class __result_of<_Fn(_Tp, _A0, _A1, _A2), false, true>  // _Fn must be member pointer
    : public __result_of_mp<typename remove_reference<_Fn>::type,
                            _Tp,
                            is_member_function_pointer<typename remove_reference<_Fn>::type>::value>
{
};

// result_of

template <class _Fn>
class _LIBCPP_TYPE_VIS_ONLY result_of<_Fn()>
    : public __result_of<_Fn(),
                         is_class<typename remove_reference<_Fn>::type>::value ||
                         is_function<typename remove_pointer<typename remove_reference<_Fn>::type>::type>::value,
                         is_member_pointer<typename remove_reference<_Fn>::type>::value
                        >
{
};

template <class _Fn, class _A0>
class _LIBCPP_TYPE_VIS_ONLY result_of<_Fn(_A0)>
    : public __result_of<_Fn(_A0),
                         is_class<typename remove_reference<_Fn>::type>::value ||
                         is_function<typename remove_pointer<typename remove_reference<_Fn>::type>::type>::value,
                         is_member_pointer<typename remove_reference<_Fn>::type>::value
                        >
{
};

template <class _Fn, class _A0, class _A1>
class _LIBCPP_TYPE_VIS_ONLY result_of<_Fn(_A0, _A1)>
    : public __result_of<_Fn(_A0, _A1),
                         is_class<typename remove_reference<_Fn>::type>::value ||
                         is_function<typename remove_pointer<typename remove_reference<_Fn>::type>::type>::value,
                         is_member_pointer<typename remove_reference<_Fn>::type>::value
                        >
{
};

template <class _Fn, class _A0, class _A1, class _A2>
class _LIBCPP_TYPE_VIS_ONLY result_of<_Fn(_A0, _A1, _A2)>
    : public __result_of<_Fn(_A0, _A1, _A2),
                         is_class<typename remove_reference<_Fn>::type>::value ||
                         is_function<typename remove_pointer<typename remove_reference<_Fn>::type>::type>::value,
                         is_member_pointer<typename remove_reference<_Fn>::type>::value
                        >
{
};

#endif  // _LIBCPP_HAS_NO_VARIADICS

// template <class T, class... Args> struct is_constructible;

namespace __is_construct
{
struct __nat {};
}

#if __has_feature(is_constructible)

template <class _Tp, class ..._Args>
struct _LIBCPP_TYPE_VIS_ONLY is_constructible
    : public integral_constant<bool, __is_constructible(_Tp, _Args...)>
    {};

#else

#ifndef _LIBCPP_HAS_NO_VARIADICS

//      main is_constructible test

template <class _Tp, class ..._Args>
typename __select_2nd<decltype(_VSTD::move(_Tp(_VSTD::declval<_Args>()...))), true_type>::type
__is_constructible_test(_Tp&&, _Args&& ...);

template <class ..._Args>
false_type
__is_constructible_test(__any, _Args&& ...);

template <bool, class _Tp, class... _Args>
struct __libcpp_is_constructible // false, _Tp is not a scalar
    : public common_type
             <
                 decltype(__is_constructible_test(declval<_Tp>(), declval<_Args>()...))
             >::type
    {};

//      function types are not constructible

template <class _Rp, class... _A1, class... _A2>
struct __libcpp_is_constructible<false, _Rp(_A1...), _A2...>
    : public false_type
    {};

//      handle scalars and reference types

//      Scalars are default constructible, references are not

template <class _Tp>
struct __libcpp_is_constructible<true, _Tp>
    : public is_scalar<_Tp>
    {};

//      Scalars and references are constructible from one arg if that arg is
//          implicitly convertible to the scalar or reference.

template <class _Tp>
struct __is_constructible_ref
{
    true_type static __lxx(_Tp);
    false_type static __lxx(...);
};

template <class _Tp, class _A0>
struct __libcpp_is_constructible<true, _Tp, _A0>
    : public common_type
             <
                 decltype(__is_constructible_ref<_Tp>::__lxx(declval<_A0>()))
             >::type
    {};

//      Scalars and references are not constructible from multiple args.

template <class _Tp, class _A0, class ..._Args>
struct __libcpp_is_constructible<true, _Tp, _A0, _Args...>
    : public false_type
    {};

//      Treat scalars and reference types separately

template <bool, class _Tp, class... _Args>
struct __is_constructible_void_check
    : public __libcpp_is_constructible<is_scalar<_Tp>::value || is_reference<_Tp>::value,
                                _Tp, _Args...>
    {};

//      If any of T or Args is void, is_constructible should be false

template <class _Tp, class... _Args>
struct __is_constructible_void_check<true, _Tp, _Args...>
    : public false_type
    {};

template <class ..._Args> struct __contains_void;

template <> struct __contains_void<> : false_type {};

template <class _A0, class ..._Args>
struct __contains_void<_A0, _Args...>
{
    static const bool value = is_void<_A0>::value ||
                              __contains_void<_Args...>::value;
};

//      is_constructible entry point

template <class _Tp, class... _Args>
struct _LIBCPP_TYPE_VIS_ONLY is_constructible
    : public __is_constructible_void_check<__contains_void<_Tp, _Args...>::value
                                        || is_abstract<_Tp>::value,
                                           _Tp, _Args...>
    {};

//      Array types are default constructible if their element type
//      is default constructible

template <class _Ap, size_t _Np>
struct __libcpp_is_constructible<false, _Ap[_Np]>
    : public is_constructible<typename remove_all_extents<_Ap>::type>
    {};

//      Otherwise array types are not constructible by this syntax

template <class _Ap, size_t _Np, class ..._Args>
struct __libcpp_is_constructible<false, _Ap[_Np], _Args...>
    : public false_type
    {};

//      Incomplete array types are not constructible

template <class _Ap, class ..._Args>
struct __libcpp_is_constructible<false, _Ap[], _Args...>
    : public false_type
    {};

#else  // _LIBCPP_HAS_NO_VARIADICS

// template <class T> struct is_constructible0;

//      main is_constructible0 test

template <class _Tp>
decltype((_Tp(), true_type()))
__is_constructible0_test(_Tp&);

false_type
__is_constructible0_test(__any);

template <class _Tp, class _A0>
decltype((_Tp(_VSTD::declval<_A0>()), true_type()))
__is_constructible1_test(_Tp&, _A0&);

template <class _A0>
false_type
__is_constructible1_test(__any, _A0&);

template <class _Tp, class _A0, class _A1>
decltype((_Tp(_VSTD::declval<_A0>(), _VSTD::declval<_A1>()), true_type()))
__is_constructible2_test(_Tp&, _A0&, _A1&);

template <class _A0, class _A1>
false_type
__is_constructible2_test(__any, _A0&, _A1&);

template <bool, class _Tp>
struct __is_constructible0_imp // false, _Tp is not a scalar
    : public common_type
             <
                 decltype(__is_constructible0_test(declval<_Tp&>()))
             >::type
    {};

template <bool, class _Tp, class _A0>
struct __is_constructible1_imp // false, _Tp is not a scalar
    : public common_type
             <
                 decltype(__is_constructible1_test(declval<_Tp&>(), declval<_A0&>()))
             >::type
    {};

template <bool, class _Tp, class _A0, class _A1>
struct __is_constructible2_imp // false, _Tp is not a scalar
    : public common_type
             <
                 decltype(__is_constructible2_test(declval<_Tp&>(), declval<_A0>(), declval<_A1>()))
             >::type
    {};

//      handle scalars and reference types

//      Scalars are default constructible, references are not

template <class _Tp>
struct __is_constructible0_imp<true, _Tp>
    : public is_scalar<_Tp>
    {};

template <class _Tp, class _A0>
struct __is_constructible1_imp<true, _Tp, _A0>
    : public is_convertible<_A0, _Tp>
    {};

template <class _Tp, class _A0, class _A1>
struct __is_constructible2_imp<true, _Tp, _A0, _A1>
    : public false_type
    {};

//      Treat scalars and reference types separately

template <bool, class _Tp>
struct __is_constructible0_void_check
    : public __is_constructible0_imp<is_scalar<_Tp>::value || is_reference<_Tp>::value,
                                _Tp>
    {};

template <bool, class _Tp, class _A0>
struct __is_constructible1_void_check
    : public __is_constructible1_imp<is_scalar<_Tp>::value || is_reference<_Tp>::value,
                                _Tp, _A0>
    {};

template <bool, class _Tp, class _A0, class _A1>
struct __is_constructible2_void_check
    : public __is_constructible2_imp<is_scalar<_Tp>::value || is_reference<_Tp>::value,
                                _Tp, _A0, _A1>
    {};

//      If any of T or Args is void, is_constructible should be false

template <class _Tp>
struct __is_constructible0_void_check<true, _Tp>
    : public false_type
    {};

template <class _Tp, class _A0>
struct __is_constructible1_void_check<true, _Tp, _A0>
    : public false_type
    {};

template <class _Tp, class _A0, class _A1>
struct __is_constructible2_void_check<true, _Tp, _A0, _A1>
    : public false_type
    {};

//      is_constructible entry point

template <class _Tp, class _A0 = __is_construct::__nat,
                     class _A1 = __is_construct::__nat>
struct _LIBCPP_TYPE_VIS_ONLY is_constructible
    : public __is_constructible2_void_check<is_void<_Tp>::value
                                        || is_abstract<_Tp>::value
                                        || is_function<_Tp>::value
                                        || is_void<_A0>::value
                                        || is_void<_A1>::value,
                                           _Tp, _A0, _A1>
    {};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_constructible<_Tp, __is_construct::__nat, __is_construct::__nat>
    : public __is_constructible0_void_check<is_void<_Tp>::value
                                        || is_abstract<_Tp>::value
                                        || is_function<_Tp>::value,
                                           _Tp>
    {};

template <class _Tp, class _A0>
struct _LIBCPP_TYPE_VIS_ONLY is_constructible<_Tp, _A0, __is_construct::__nat>
    : public __is_constructible1_void_check<is_void<_Tp>::value
                                        || is_abstract<_Tp>::value
                                        || is_function<_Tp>::value
                                        || is_void<_A0>::value,
                                           _Tp, _A0>
    {};

//      Array types are default constructible if their element type
//      is default constructible

template <class _Ap, size_t _Np>
struct __is_constructible0_imp<false, _Ap[_Np]>
    : public is_constructible<typename remove_all_extents<_Ap>::type>
    {};

template <class _Ap, size_t _Np, class _A0>
struct __is_constructible1_imp<false, _Ap[_Np], _A0>
    : public false_type
    {};

template <class _Ap, size_t _Np, class _A0, class _A1>
struct __is_constructible2_imp<false, _Ap[_Np], _A0, _A1>
    : public false_type
    {};

//      Incomplete array types are not constructible

template <class _Ap>
struct __is_constructible0_imp<false, _Ap[]>
    : public false_type
    {};

template <class _Ap, class _A0>
struct __is_constructible1_imp<false, _Ap[], _A0>
    : public false_type
    {};

template <class _Ap, class _A0, class _A1>
struct __is_constructible2_imp<false, _Ap[], _A0, _A1>
    : public false_type
    {};

#endif  // _LIBCPP_HAS_NO_VARIADICS
#endif  // __has_feature(is_constructible)

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES) && !defined(_LIBCPP_HAS_NO_VARIADICS)
template <class _Tp, class ..._Args> _LIBCPP_CONSTEXPR bool is_constructible_v
    = is_constructible<_Tp, _Args...>::value;
#endif

// is_default_constructible

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_default_constructible
    : public is_constructible<_Tp>
    {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_default_constructible_v
    = is_default_constructible<_Tp>::value;
#endif

// is_copy_constructible

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_copy_constructible
    : public is_constructible<_Tp, 
                  typename add_lvalue_reference<typename add_const<_Tp>::type>::type> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_copy_constructible_v
    = is_copy_constructible<_Tp>::value;
#endif

// is_move_constructible

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_move_constructible
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    : public is_constructible<_Tp, typename add_rvalue_reference<_Tp>::type>
#else
    : public is_copy_constructible<_Tp>
#endif
    {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_move_constructible_v
    = is_move_constructible<_Tp>::value;
#endif

// is_trivially_constructible

#ifndef _LIBCPP_HAS_NO_VARIADICS

#if __has_feature(is_trivially_constructible) || _GNUC_VER >= 501

template <class _Tp, class... _Args>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible
    : integral_constant<bool, __is_trivially_constructible(_Tp, _Args...)>
{
};

#else  // !__has_feature(is_trivially_constructible)

template <class _Tp, class... _Args>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible
    : false_type
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp>
#if __has_feature(has_trivial_constructor) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_trivial_constructor(_Tp)>
#else
    : integral_constant<bool, is_scalar<_Tp>::value>
#endif
{
};

template <class _Tp>
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, _Tp&&>
#else
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, _Tp>
#endif
    : integral_constant<bool, is_scalar<_Tp>::value>
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, const _Tp&>
    : integral_constant<bool, is_scalar<_Tp>::value>
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, _Tp&>
    : integral_constant<bool, is_scalar<_Tp>::value>
{
};

#endif  // !__has_feature(is_trivially_constructible)

#else  // _LIBCPP_HAS_NO_VARIADICS

template <class _Tp, class _A0 = __is_construct::__nat,
                     class _A1 = __is_construct::__nat>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible
    : false_type
{
};

#if __has_feature(is_trivially_constructible) || _GNUC_VER >= 501

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, __is_construct::__nat,
                                                       __is_construct::__nat>
    : integral_constant<bool, __is_trivially_constructible(_Tp)>
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, _Tp,
                                                       __is_construct::__nat>
    : integral_constant<bool, __is_trivially_constructible(_Tp, _Tp)>
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, const _Tp&,
                                                       __is_construct::__nat>
    : integral_constant<bool, __is_trivially_constructible(_Tp, const _Tp&)>
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, _Tp&,
                                                       __is_construct::__nat>
    : integral_constant<bool, __is_trivially_constructible(_Tp, _Tp&)>
{
};

#else  // !__has_feature(is_trivially_constructible)

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, __is_construct::__nat,
                                                       __is_construct::__nat>
    : integral_constant<bool, is_scalar<_Tp>::value>
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, _Tp,
                                                       __is_construct::__nat>
    : integral_constant<bool, is_scalar<_Tp>::value>
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, const _Tp&,
                                                       __is_construct::__nat>
    : integral_constant<bool, is_scalar<_Tp>::value>
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_trivially_constructible<_Tp, _Tp&,
                                                       __is_construct::__nat>
    : integral_constant<bool, is_scalar<_Tp>::value>
{
};

#endif  // !__has_feature(is_trivially_constructible)

#endif  // _LIBCPP_HAS_NO_VARIADICS

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES) && !defined(_LIBCPP_HAS_NO_VARIADICS)
template <class _Tp, class... _Args> _LIBCPP_CONSTEXPR bool is_trivially_constructible_v
    = is_trivially_constructible<_Tp, _Args...>::value;
#endif

// is_trivially_default_constructible

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_trivially_default_constructible
    : public is_trivially_constructible<_Tp>
    {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_trivially_default_constructible_v
    = is_trivially_default_constructible<_Tp>::value;
#endif

// is_trivially_copy_constructible

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_trivially_copy_constructible
    : public is_trivially_constructible<_Tp, typename add_lvalue_reference<const _Tp>::type>
    {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_trivially_copy_constructible_v
    = is_trivially_copy_constructible<_Tp>::value;
#endif

// is_trivially_move_constructible

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_trivially_move_constructible
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    : public is_trivially_constructible<_Tp, typename add_rvalue_reference<_Tp>::type>
#else
    : public is_trivially_copy_constructible<_Tp>
#endif
    {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_trivially_move_constructible_v
    = is_trivially_move_constructible<_Tp>::value;
#endif

// is_trivially_assignable

#if __has_feature(is_trivially_assignable) || _GNUC_VER >= 501

template <class _Tp, class _Arg>
struct is_trivially_assignable
    : integral_constant<bool, __is_trivially_assignable(_Tp, _Arg)>
{
};

#else  // !__has_feature(is_trivially_assignable)

template <class _Tp, class _Arg>
struct is_trivially_assignable
    : public false_type {};

template <class _Tp>
struct is_trivially_assignable<_Tp&, _Tp>
    : integral_constant<bool, is_scalar<_Tp>::value> {};

template <class _Tp>
struct is_trivially_assignable<_Tp&, _Tp&>
    : integral_constant<bool, is_scalar<_Tp>::value> {};

template <class _Tp>
struct is_trivially_assignable<_Tp&, const _Tp&>
    : integral_constant<bool, is_scalar<_Tp>::value> {};

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp>
struct is_trivially_assignable<_Tp&, _Tp&&>
    : integral_constant<bool, is_scalar<_Tp>::value> {};

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

#endif  // !__has_feature(is_trivially_assignable)

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp, class _Arg> _LIBCPP_CONSTEXPR bool is_trivially_assignable_v
    = is_trivially_assignable<_Tp, _Arg>::value;
#endif

// is_trivially_copy_assignable

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_trivially_copy_assignable
    : public is_trivially_assignable<typename add_lvalue_reference<_Tp>::type,
                  typename add_lvalue_reference<typename add_const<_Tp>::type>::type> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_trivially_copy_assignable_v
    = is_trivially_copy_assignable<_Tp>::value;
#endif

// is_trivially_move_assignable

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_trivially_move_assignable
    : public is_trivially_assignable<typename add_lvalue_reference<_Tp>::type,
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
                                     typename add_rvalue_reference<_Tp>::type>
#else
                                     typename add_lvalue_reference<_Tp>::type>
#endif
    {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_trivially_move_assignable_v
    = is_trivially_move_assignable<_Tp>::value;
#endif

// is_trivially_destructible

#if __has_feature(has_trivial_destructor) || (_GNUC_VER >= 403)

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_trivially_destructible
    : public integral_constant<bool, is_destructible<_Tp>::value && __has_trivial_destructor(_Tp)> {};

#else

template <class _Tp> struct __libcpp_trivial_destructor
    : public integral_constant<bool, is_scalar<_Tp>::value ||
                                     is_reference<_Tp>::value> {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_trivially_destructible
    : public __libcpp_trivial_destructor<typename remove_all_extents<_Tp>::type> {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_trivially_destructible<_Tp[]>
    : public false_type {};

#endif

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_trivially_destructible_v
    = is_trivially_destructible<_Tp>::value;
#endif

// is_nothrow_constructible

#if 0
template <class _Tp, class... _Args>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible
    : public integral_constant<bool, __is_nothrow_constructible(_Tp(_Args...))>
{
};

#else

#ifndef _LIBCPP_HAS_NO_VARIADICS

#if __has_feature(cxx_noexcept) || (_GNUC_VER >= 407 && __cplusplus >= 201103L)

template <bool, bool, class _Tp, class... _Args> struct __libcpp_is_nothrow_constructible;

template <class _Tp, class... _Args>
struct __libcpp_is_nothrow_constructible</*is constructible*/true, /*is reference*/false, _Tp, _Args...>
    : public integral_constant<bool, noexcept(_Tp(declval<_Args>()...))>
{
};

template <class _Tp>
void __implicit_conversion_to(_Tp) noexcept { }

template <class _Tp, class _Arg>
struct __libcpp_is_nothrow_constructible</*is constructible*/true, /*is reference*/true, _Tp, _Arg>
    : public integral_constant<bool, noexcept(__implicit_conversion_to<_Tp>(declval<_Arg>()))>
{
};

template <class _Tp, bool _IsReference, class... _Args>
struct __libcpp_is_nothrow_constructible</*is constructible*/false, _IsReference, _Tp, _Args...>
    : public false_type
{
};

template <class _Tp, class... _Args>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible
    : __libcpp_is_nothrow_constructible<is_constructible<_Tp, _Args...>::value, is_reference<_Tp>::value, _Tp, _Args...>
{
};

template <class _Tp, size_t _Ns>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible<_Tp[_Ns]>
    : __libcpp_is_nothrow_constructible<is_constructible<_Tp>::value, is_reference<_Tp>::value, _Tp>
{
};

#else  // __has_feature(cxx_noexcept)

template <class _Tp, class... _Args>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible
    : false_type
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible<_Tp>
#if __has_feature(has_nothrow_constructor) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_constructor(_Tp)>
#else
    : integral_constant<bool, is_scalar<_Tp>::value>
#endif
{
};

template <class _Tp>
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible<_Tp, _Tp&&>
#else
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible<_Tp, _Tp>
#endif
#if __has_feature(has_nothrow_copy) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_copy(_Tp)>
#else
    : integral_constant<bool, is_scalar<_Tp>::value>
#endif
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible<_Tp, const _Tp&>
#if __has_feature(has_nothrow_copy) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_copy(_Tp)>
#else
    : integral_constant<bool, is_scalar<_Tp>::value>
#endif
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible<_Tp, _Tp&>
#if __has_feature(has_nothrow_copy) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_copy(_Tp)>
#else
    : integral_constant<bool, is_scalar<_Tp>::value>
#endif
{
};

#endif  // __has_feature(cxx_noexcept)

#else  // _LIBCPP_HAS_NO_VARIADICS

template <class _Tp, class _A0 = __is_construct::__nat,
                     class _A1 = __is_construct::__nat>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible
    : false_type
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible<_Tp, __is_construct::__nat,
                                                       __is_construct::__nat>
#if __has_feature(has_nothrow_constructor) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_constructor(_Tp)>
#else
    : integral_constant<bool, is_scalar<_Tp>::value>
#endif
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible<_Tp, _Tp,
                                                       __is_construct::__nat>
#if __has_feature(has_nothrow_copy) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_copy(_Tp)>
#else
    : integral_constant<bool, is_scalar<_Tp>::value>
#endif
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible<_Tp, const _Tp&,
                                                       __is_construct::__nat>
#if __has_feature(has_nothrow_copy) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_copy(_Tp)>
#else
    : integral_constant<bool, is_scalar<_Tp>::value>
#endif
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_constructible<_Tp, _Tp&,
                                                       __is_construct::__nat>
#if __has_feature(has_nothrow_copy) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_copy(_Tp)>
#else
    : integral_constant<bool, is_scalar<_Tp>::value>
#endif
{
};

#endif  // _LIBCPP_HAS_NO_VARIADICS
#endif  // __has_feature(is_nothrow_constructible)

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES) && !defined(_LIBCPP_HAS_NO_VARIADICS)
template <class _Tp, class ..._Args> _LIBCPP_CONSTEXPR bool is_nothrow_constructible_v
    = is_nothrow_constructible<_Tp, _Args...>::value;
#endif

// is_nothrow_default_constructible

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_default_constructible
    : public is_nothrow_constructible<_Tp>
    {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_nothrow_default_constructible_v
    = is_nothrow_default_constructible<_Tp>::value;
#endif

// is_nothrow_copy_constructible

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_copy_constructible
    : public is_nothrow_constructible<_Tp,
                  typename add_lvalue_reference<typename add_const<_Tp>::type>::type> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_nothrow_copy_constructible_v
    = is_nothrow_copy_constructible<_Tp>::value;
#endif

// is_nothrow_move_constructible

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_move_constructible
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    : public is_nothrow_constructible<_Tp, typename add_rvalue_reference<_Tp>::type>
#else
    : public is_nothrow_copy_constructible<_Tp>
#endif
    {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_nothrow_move_constructible_v
    = is_nothrow_move_constructible<_Tp>::value;
#endif

// is_nothrow_assignable

#if __has_feature(cxx_noexcept) || (_GNUC_VER >= 407 && __cplusplus >= 201103L)

template <bool, class _Tp, class _Arg> struct __libcpp_is_nothrow_assignable;

template <class _Tp, class _Arg>
struct __libcpp_is_nothrow_assignable<false, _Tp, _Arg>
    : public false_type
{
};

template <class _Tp, class _Arg>
struct __libcpp_is_nothrow_assignable<true, _Tp, _Arg>
    : public integral_constant<bool, noexcept(_VSTD::declval<_Tp>() = _VSTD::declval<_Arg>()) >
{
};

template <class _Tp, class _Arg>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_assignable
    : public __libcpp_is_nothrow_assignable<is_assignable<_Tp, _Arg>::value, _Tp, _Arg>
{
};

#else  // __has_feature(cxx_noexcept)

template <class _Tp, class _Arg>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_assignable
    : public false_type {};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_assignable<_Tp&, _Tp>
#if __has_feature(has_nothrow_assign) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_assign(_Tp)> {};
#else
    : integral_constant<bool, is_scalar<_Tp>::value> {};
#endif

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_assignable<_Tp&, _Tp&>
#if __has_feature(has_nothrow_assign) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_assign(_Tp)> {};
#else
    : integral_constant<bool, is_scalar<_Tp>::value> {};
#endif

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_assignable<_Tp&, const _Tp&>
#if __has_feature(has_nothrow_assign) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_assign(_Tp)> {};
#else
    : integral_constant<bool, is_scalar<_Tp>::value> {};
#endif

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp>
struct is_nothrow_assignable<_Tp&, _Tp&&>
#if __has_feature(has_nothrow_assign) || (_GNUC_VER >= 403)
    : integral_constant<bool, __has_nothrow_assign(_Tp)> {};
#else
    : integral_constant<bool, is_scalar<_Tp>::value> {};
#endif

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

#endif  // __has_feature(cxx_noexcept)

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp, class _Arg> _LIBCPP_CONSTEXPR bool is_nothrow_assignable_v
    = is_nothrow_assignable<_Tp, _Arg>::value;
#endif

// is_nothrow_copy_assignable

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_copy_assignable
    : public is_nothrow_assignable<typename add_lvalue_reference<_Tp>::type,
                  typename add_lvalue_reference<typename add_const<_Tp>::type>::type> {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_nothrow_copy_assignable_v
    = is_nothrow_copy_assignable<_Tp>::value;
#endif

// is_nothrow_move_assignable

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_move_assignable
    : public is_nothrow_assignable<typename add_lvalue_reference<_Tp>::type,
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
                                     typename add_rvalue_reference<_Tp>::type>
#else
                                     typename add_lvalue_reference<_Tp>::type>
#endif
    {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_nothrow_move_assignable_v
    = is_nothrow_move_assignable<_Tp>::value;
#endif

// is_nothrow_destructible

#if __has_feature(cxx_noexcept) || (_GNUC_VER >= 407 && __cplusplus >= 201103L)

template <bool, class _Tp> struct __libcpp_is_nothrow_destructible;

template <class _Tp>
struct __libcpp_is_nothrow_destructible<false, _Tp>
    : public false_type
{
};

template <class _Tp>
struct __libcpp_is_nothrow_destructible<true, _Tp>
    : public integral_constant<bool, noexcept(_VSTD::declval<_Tp>().~_Tp()) >
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_destructible
    : public __libcpp_is_nothrow_destructible<is_destructible<_Tp>::value, _Tp>
{
};

template <class _Tp, size_t _Ns>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_destructible<_Tp[_Ns]>
    : public is_nothrow_destructible<_Tp>
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_destructible<_Tp&>
    : public true_type
{
};

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_destructible<_Tp&&>
    : public true_type
{
};

#endif

#else

template <class _Tp> struct __libcpp_nothrow_destructor
    : public integral_constant<bool, is_scalar<_Tp>::value ||
                                     is_reference<_Tp>::value> {};

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_destructible
    : public __libcpp_nothrow_destructor<typename remove_all_extents<_Tp>::type> {};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_destructible<_Tp[]>
    : public false_type {};

#endif

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_nothrow_destructible_v
    = is_nothrow_destructible<_Tp>::value;
#endif

// is_pod

#if __has_feature(is_pod) || (_GNUC_VER >= 403)

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_pod
    : public integral_constant<bool, __is_pod(_Tp)> {};

#else

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_pod
    : public integral_constant<bool, is_trivially_default_constructible<_Tp>::value   &&
                                     is_trivially_copy_constructible<_Tp>::value      &&
                                     is_trivially_copy_assignable<_Tp>::value    &&
                                     is_trivially_destructible<_Tp>::value> {};

#endif

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_pod_v
    = is_pod<_Tp>::value;
#endif

// is_literal_type;

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_literal_type
#ifdef _LIBCPP_IS_LITERAL
    : public integral_constant<bool, _LIBCPP_IS_LITERAL(_Tp)>
#else
    : integral_constant<bool, is_scalar<typename remove_all_extents<_Tp>::type>::value ||
                              is_reference<typename remove_all_extents<_Tp>::type>::value>
#endif
    {};
    
#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_literal_type_v
    = is_literal_type<_Tp>::value;
#endif

// is_standard_layout;

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_standard_layout
#if __has_feature(is_standard_layout) || (_GNUC_VER >= 407)
    : public integral_constant<bool, __is_standard_layout(_Tp)>
#else
    : integral_constant<bool, is_scalar<typename remove_all_extents<_Tp>::type>::value>
#endif
    {};
    
#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_standard_layout_v
    = is_standard_layout<_Tp>::value;
#endif

// is_trivially_copyable;

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_trivially_copyable
#if __has_feature(is_trivially_copyable)
    : public integral_constant<bool, __is_trivially_copyable(_Tp)>
#elif _GNUC_VER >= 501
    : public integral_constant<bool, !is_volatile<_Tp>::value && __is_trivially_copyable(_Tp)>
#else
    : integral_constant<bool, is_scalar<typename remove_all_extents<_Tp>::type>::value>
#endif
    {};
    
#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_trivially_copyable_v
    = is_trivially_copyable<_Tp>::value;
#endif

// is_trivial;

template <class _Tp> struct _LIBCPP_TYPE_VIS_ONLY is_trivial
#if __has_feature(is_trivial) || _GNUC_VER >= 407
    : public integral_constant<bool, __is_trivial(_Tp)>
#else
    : integral_constant<bool, is_trivially_copyable<_Tp>::value &&
                                 is_trivially_default_constructible<_Tp>::value>
#endif
    {};

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_VARIABLE_TEMPLATES)
template <class _Tp> _LIBCPP_CONSTEXPR bool is_trivial_v
    = is_trivial<_Tp>::value;
#endif

template <class _Tp> struct __is_reference_wrapper_impl : public false_type {};
template <class _Tp> struct __is_reference_wrapper_impl<reference_wrapper<_Tp> > : public true_type {};
template <class _Tp> struct __is_reference_wrapper
    : public __is_reference_wrapper_impl<typename remove_cv<_Tp>::type> {};

#ifndef _LIBCPP_CXX03_LANG

// Check for complete types

template <class ..._Tp> struct __check_complete;

template <>
struct __check_complete<>
{
};

template <class _Hp, class _T0, class ..._Tp>
struct __check_complete<_Hp, _T0, _Tp...>
    : private __check_complete<_Hp>,
      private __check_complete<_T0, _Tp...>
{
};

template <class _Hp>
struct __check_complete<_Hp, _Hp>
    : private __check_complete<_Hp>
{
};

template <class _Tp>
struct __check_complete<_Tp>
{
    static_assert(sizeof(_Tp) > 0, "Type must be complete.");
};

template <class _Tp>
struct __check_complete<_Tp&>
    : private __check_complete<_Tp>
{
};

template <class _Tp>
struct __check_complete<_Tp&&>
    : private __check_complete<_Tp>
{
};

template <class _Rp, class ..._Param>
struct __check_complete<_Rp (*)(_Param...)>
    : private __check_complete<_Rp>
{
};

template <class ..._Param>
struct __check_complete<void (*)(_Param...)>
{
};

template <class _Rp, class ..._Param>
struct __check_complete<_Rp (_Param...)>
    : private __check_complete<_Rp>
{
};

template <class ..._Param>
struct __check_complete<void (_Param...)>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...)>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...) const>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...) volatile>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...) const volatile>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...) &>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...) const&>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...) volatile&>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...) const volatile&>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...) &&>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...) const&&>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...) volatile&&>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class, class ..._Param>
struct __check_complete<_Rp (_Class::*)(_Param...) const volatile&&>
    : private __check_complete<_Class>
{
};

template <class _Rp, class _Class>
struct __check_complete<_Rp _Class::*>
    : private __check_complete<_Class>
{
};


template <class _Fp, class _A0,
         class _DecayFp = typename decay<_Fp>::type,
         class _DecayA0 = typename decay<_A0>::type,
         class _ClassT = typename __member_pointer_class_type<_DecayFp>::type>
using __enable_if_bullet1 = typename enable_if
    <
        is_member_function_pointer<_DecayFp>::value
        && is_base_of<_ClassT, _DecayA0>::value
    >::type;

template <class _Fp, class _A0,
         class _DecayFp = typename decay<_Fp>::type,
         class _DecayA0 = typename decay<_A0>::type>
using __enable_if_bullet2 = typename enable_if
    <
        is_member_function_pointer<_DecayFp>::value
        && __is_reference_wrapper<_DecayA0>::value
    >::type;

template <class _Fp, class _A0,
         class _DecayFp = typename decay<_Fp>::type,
         class _DecayA0 = typename decay<_A0>::type,
         class _ClassT = typename __member_pointer_class_type<_DecayFp>::type>
using __enable_if_bullet3 = typename enable_if
    <
        is_member_function_pointer<_DecayFp>::value
        && !is_base_of<_ClassT, _DecayA0>::value
        && !__is_reference_wrapper<_DecayA0>::value
    >::type;

template <class _Fp, class _A0,
         class _DecayFp = typename decay<_Fp>::type,
         class _DecayA0 = typename decay<_A0>::type,
         class _ClassT = typename __member_pointer_class_type<_DecayFp>::type>
using __enable_if_bullet4 = typename enable_if
    <
        is_member_object_pointer<_DecayFp>::value
        && is_base_of<_ClassT, _DecayA0>::value
    >::type;

template <class _Fp, class _A0,
         class _DecayFp = typename decay<_Fp>::type,
         class _DecayA0 = typename decay<_A0>::type>
using __enable_if_bullet5 = typename enable_if
    <
        is_member_object_pointer<_DecayFp>::value
        && __is_reference_wrapper<_DecayA0>::value
    >::type;

template <class _Fp, class _A0,
         class _DecayFp = typename decay<_Fp>::type,
         class _DecayA0 = typename decay<_A0>::type,
         class _ClassT = typename __member_pointer_class_type<_DecayFp>::type>
using __enable_if_bullet6 = typename enable_if
    <
        is_member_object_pointer<_DecayFp>::value
        && !is_base_of<_ClassT, _DecayA0>::value
        && !__is_reference_wrapper<_DecayA0>::value
    >::type;

// __invoke forward declarations

// fall back - none of the bullets

#define _LIBCPP_INVOKE_RETURN(...) \
    noexcept(noexcept(__VA_ARGS__)) -> decltype(__VA_ARGS__) \
    { return __VA_ARGS__; }

template <class ..._Args>
auto __invoke(__any, _Args&& ...__args) -> __nat;

template <class ..._Args>
auto __invoke_constexpr(__any, _Args&& ...__args) -> __nat;

// bullets 1, 2 and 3

template <class _Fp, class _A0, class ..._Args,
          class = __enable_if_bullet1<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
auto
__invoke(_Fp&& __f, _A0&& __a0, _Args&& ...__args)
_LIBCPP_INVOKE_RETURN((_VSTD::forward<_A0>(__a0).*__f)(_VSTD::forward<_Args>(__args)...))

template <class _Fp, class _A0, class ..._Args,
          class = __enable_if_bullet1<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
_LIBCPP_CONSTEXPR auto
__invoke_constexpr(_Fp&& __f, _A0&& __a0, _Args&& ...__args)
_LIBCPP_INVOKE_RETURN((_VSTD::forward<_A0>(__a0).*__f)(_VSTD::forward<_Args>(__args)...))

template <class _Fp, class _A0, class ..._Args,
          class = __enable_if_bullet2<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
auto
__invoke(_Fp&& __f, _A0&& __a0, _Args&& ...__args)
_LIBCPP_INVOKE_RETURN((__a0.get().*__f)(_VSTD::forward<_Args>(__args)...))

template <class _Fp, class _A0, class ..._Args,
          class = __enable_if_bullet2<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
_LIBCPP_CONSTEXPR auto
__invoke_constexpr(_Fp&& __f, _A0&& __a0, _Args&& ...__args)
_LIBCPP_INVOKE_RETURN((__a0.get().*__f)(_VSTD::forward<_Args>(__args)...))

template <class _Fp, class _A0, class ..._Args,
          class = __enable_if_bullet3<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
auto
__invoke(_Fp&& __f, _A0&& __a0, _Args&& ...__args)
_LIBCPP_INVOKE_RETURN(((*_VSTD::forward<_A0>(__a0)).*__f)(_VSTD::forward<_Args>(__args)...))

template <class _Fp, class _A0, class ..._Args,
          class = __enable_if_bullet3<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
_LIBCPP_CONSTEXPR auto
__invoke_constexpr(_Fp&& __f, _A0&& __a0, _Args&& ...__args)
_LIBCPP_INVOKE_RETURN(((*_VSTD::forward<_A0>(__a0)).*__f)(_VSTD::forward<_Args>(__args)...))

// bullets 4, 5 and 6

template <class _Fp, class _A0,
          class = __enable_if_bullet4<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
auto
__invoke(_Fp&& __f, _A0&& __a0)
_LIBCPP_INVOKE_RETURN(_VSTD::forward<_A0>(__a0).*__f)

template <class _Fp, class _A0,
          class = __enable_if_bullet4<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
_LIBCPP_CONSTEXPR auto
__invoke_constexpr(_Fp&& __f, _A0&& __a0)
_LIBCPP_INVOKE_RETURN(_VSTD::forward<_A0>(__a0).*__f)

template <class _Fp, class _A0,
          class = __enable_if_bullet5<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
auto
__invoke(_Fp&& __f, _A0&& __a0)
_LIBCPP_INVOKE_RETURN(__a0.get().*__f)

template <class _Fp, class _A0,
          class = __enable_if_bullet5<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
_LIBCPP_CONSTEXPR auto
__invoke_constexpr(_Fp&& __f, _A0&& __a0)
_LIBCPP_INVOKE_RETURN(__a0.get().*__f)

template <class _Fp, class _A0,
          class = __enable_if_bullet6<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
auto
__invoke(_Fp&& __f, _A0&& __a0)
_LIBCPP_INVOKE_RETURN((*_VSTD::forward<_A0>(__a0)).*__f)

template <class _Fp, class _A0,
          class = __enable_if_bullet6<_Fp, _A0>>
inline _LIBCPP_INLINE_VISIBILITY
_LIBCPP_CONSTEXPR auto
__invoke_constexpr(_Fp&& __f, _A0&& __a0)
_LIBCPP_INVOKE_RETURN((*_VSTD::forward<_A0>(__a0)).*__f)

// bullet 7

template <class _Fp, class ..._Args>
inline _LIBCPP_INLINE_VISIBILITY
auto
__invoke(_Fp&& __f, _Args&& ...__args)
_LIBCPP_INVOKE_RETURN(_VSTD::forward<_Fp>(__f)(_VSTD::forward<_Args>(__args)...))

template <class _Fp, class ..._Args>
inline _LIBCPP_INLINE_VISIBILITY
_LIBCPP_CONSTEXPR auto
__invoke_constexpr(_Fp&& __f, _Args&& ...__args)
_LIBCPP_INVOKE_RETURN(_VSTD::forward<_Fp>(__f)(_VSTD::forward<_Args>(__args)...))

#undef _LIBCPP_INVOKE_RETURN

// __invokable

template <class _Ret, class _Fp, class ..._Args>
struct __invokable_r
    : private __check_complete<_Fp>
{
    using _Result = decltype(
        _VSTD::__invoke(_VSTD::declval<_Fp>(), _VSTD::declval<_Args>()...));

    static const bool value =
        conditional<
            !is_same<_Result, __nat>::value,
            typename conditional<
                is_void<_Ret>::value,
                true_type,
                is_convertible<_Result, _Ret>
            >::type,
            false_type
        >::type::value;
};

template <class _Fp, class ..._Args>
using __invokable = __invokable_r<void, _Fp, _Args...>;

template <bool _IsInvokable, bool _IsCVVoid, class _Ret, class _Fp, class ..._Args>
struct __nothrow_invokable_r_imp {
  static const bool value = false;
};

template <class _Ret, class _Fp, class ..._Args>
struct __nothrow_invokable_r_imp<true, false, _Ret, _Fp, _Args...>
{
    typedef __nothrow_invokable_r_imp _ThisT;

    template <class _Tp>
    static void __test_noexcept(_Tp) noexcept;

    static const bool value = noexcept(_ThisT::__test_noexcept<_Ret>(
        _VSTD::__invoke(_VSTD::declval<_Fp>(), _VSTD::declval<_Args>()...)));
};

template <class _Ret, class _Fp, class ..._Args>
struct __nothrow_invokable_r_imp<true, true, _Ret, _Fp, _Args...>
{
    static const bool value = noexcept(
        _VSTD::__invoke(_VSTD::declval<_Fp>(), _VSTD::declval<_Args>()...));
};

template <class _Ret, class _Fp, class ..._Args>
using __nothrow_invokable_r =
    __nothrow_invokable_r_imp<
            __invokable_r<_Ret, _Fp, _Args...>::value,
            is_void<_Ret>::value,
            _Ret, _Fp, _Args...
    >;

template <class _Fp, class ..._Args>
struct __invoke_of
    : public enable_if<
        __invokable<_Fp, _Args...>::value,
        typename __invokable_r<void, _Fp, _Args...>::_Result>
{
};

// result_of

template <class _Fp, class ..._Args>
class _LIBCPP_TYPE_VIS_ONLY result_of<_Fp(_Args...)>
    : public __invoke_of<_Fp, _Args...>
{
};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using result_of_t = typename result_of<_Tp>::type;
#endif

#if _LIBCPP_STD_VER > 14

// is_callable

template <class _Fn, class _Ret = void>
struct _LIBCPP_TYPE_VIS_ONLY is_callable;

template <class _Fn, class ..._Args, class _Ret>
struct _LIBCPP_TYPE_VIS_ONLY is_callable<_Fn(_Args...), _Ret>
    : integral_constant<bool, __invokable_r<_Ret, _Fn, _Args...>::value> {};

template <class _Fn, class _Ret = void>
constexpr bool is_callable_v = is_callable<_Fn, _Ret>::value;

// is_nothrow_callable

template <class _Fn, class _Ret = void>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_callable;

template <class _Fn, class ..._Args, class _Ret>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_callable<_Fn(_Args...), _Ret>
    : integral_constant<bool, __nothrow_invokable_r<_Ret, _Fn, _Args...>::value>
{};

template <class _Fn, class _Ret = void>
constexpr bool is_nothrow_callable_v = is_nothrow_callable<_Fn, _Ret>::value;

#endif // _LIBCPP_STD_VER > 14

#endif  // !defined(_LIBCPP_CXX03_LANG)

template <class _Tp> struct __is_swappable;
template <class _Tp> struct __is_nothrow_swappable;

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
#ifndef _LIBCPP_HAS_NO_ADVANCED_SFINAE
typename enable_if
<
    is_move_constructible<_Tp>::value &&
    is_move_assignable<_Tp>::value
>::type
#else
void
#endif
swap(_Tp& __x, _Tp& __y) _NOEXCEPT_(is_nothrow_move_constructible<_Tp>::value &&
                                    is_nothrow_move_assignable<_Tp>::value)
{
    _Tp __t(_VSTD::move(__x));
    __x = _VSTD::move(__y);
    __y = _VSTD::move(__t);
}

template<class _Tp, size_t _Np>
inline _LIBCPP_INLINE_VISIBILITY
typename enable_if<
    __is_swappable<_Tp>::value
>::type
swap(_Tp (&__a)[_Np], _Tp (&__b)[_Np]) _NOEXCEPT_(__is_nothrow_swappable<_Tp>::value);

template <class _ForwardIterator1, class _ForwardIterator2>
inline _LIBCPP_INLINE_VISIBILITY
void
iter_swap(_ForwardIterator1 __a, _ForwardIterator2 __b)
    //                                  _NOEXCEPT_(_NOEXCEPT_(swap(*__a, *__b)))
               _NOEXCEPT_(_NOEXCEPT_(swap(*_VSTD::declval<_ForwardIterator1>(),
                                          *_VSTD::declval<_ForwardIterator2>())))
{
    swap(*__a, *__b);
}

// __swappable

namespace __detail
{
// ALL generic swap overloads MUST already have a declaration available at this point.

template <class _Tp, class _Up = _Tp,
          bool _NotVoid = !is_void<_Tp>::value && !is_void<_Up>::value>
struct __swappable_with
{
    template <class _LHS, class _RHS>
    static decltype(swap(_VSTD::declval<_LHS>(), _VSTD::declval<_RHS>()))
    __test_swap(int);
    template <class, class>
    static __nat __test_swap(long);

    // Extra parens are needed for the C++03 definition of decltype.
    typedef decltype((__test_swap<_Tp, _Up>(0))) __swap1;
    typedef decltype((__test_swap<_Up, _Tp>(0))) __swap2;

    static const bool value = !is_same<__swap1, __nat>::value
                           && !is_same<__swap2, __nat>::value;
};

template <class _Tp, class _Up>
struct __swappable_with<_Tp, _Up,  false> : false_type {};

template <class _Tp, class _Up = _Tp, bool _Swappable = __swappable_with<_Tp, _Up>::value>
struct __nothrow_swappable_with {
  static const bool value =
#ifndef _LIBCPP_HAS_NO_NOEXCEPT
      noexcept(swap(_VSTD::declval<_Tp>(), _VSTD::declval<_Up>()))
  &&  noexcept(swap(_VSTD::declval<_Up>(), _VSTD::declval<_Tp>()));
#else
      false;
#endif
};

template <class _Tp, class _Up>
struct __nothrow_swappable_with<_Tp, _Up, false> : false_type {};

}  // __detail

template <class _Tp>
struct __is_swappable
    : public integral_constant<bool, __detail::__swappable_with<_Tp&>::value>
{
};

template <class _Tp>
struct __is_nothrow_swappable
    : public integral_constant<bool, __detail::__nothrow_swappable_with<_Tp&>::value>
{
};

#if _LIBCPP_STD_VER > 14

template <class _Tp, class _Up>
struct _LIBCPP_TYPE_VIS_ONLY is_swappable_with
    : public integral_constant<bool, __detail::__swappable_with<_Tp, _Up>::value>
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_swappable
    : public conditional<
        __is_referenceable<_Tp>::value,
        is_swappable_with<
            typename add_lvalue_reference<_Tp>::type,
            typename add_lvalue_reference<_Tp>::type>,
        false_type
    >::type
{
};

template <class _Tp, class _Up>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_swappable_with
    : public integral_constant<bool, __detail::__nothrow_swappable_with<_Tp, _Up>::value>
{
};

template <class _Tp>
struct _LIBCPP_TYPE_VIS_ONLY is_nothrow_swappable
    : public conditional<
        __is_referenceable<_Tp>::value,
        is_nothrow_swappable_with<
            typename add_lvalue_reference<_Tp>::type,
            typename add_lvalue_reference<_Tp>::type>,
        false_type
    >::type
{
};

template <class _Tp, class _Up>
constexpr bool is_swappable_with_v = is_swappable_with<_Tp, _Up>::value;

template <class _Tp>
constexpr bool is_swappable_v = is_swappable<_Tp>::value;

template <class _Tp, class _Up>
constexpr bool is_nothrow_swappable_with_v = is_nothrow_swappable_with<_Tp, _Up>::value;

template <class _Tp>
constexpr bool is_nothrow_swappable_v = is_nothrow_swappable<_Tp>::value;

#endif // _LIBCPP_STD_VER > 14

#ifdef _LIBCPP_UNDERLYING_TYPE

template <class _Tp>
struct underlying_type
{
    typedef _LIBCPP_UNDERLYING_TYPE(_Tp) type;
};

#if _LIBCPP_STD_VER > 11
template <class _Tp> using underlying_type_t = typename underlying_type<_Tp>::type;
#endif

#else  // _LIBCPP_UNDERLYING_TYPE

template <class _Tp, bool _Support = false>
struct underlying_type
{
    static_assert(_Support, "The underyling_type trait requires compiler "
                            "support. Either no such support exists or "
                            "libc++ does not know how to use it.");
};

#endif // _LIBCPP_UNDERLYING_TYPE


template <class _Tp, bool = is_enum<_Tp>::value>
struct __sfinae_underlying_type
{
    typedef typename underlying_type<_Tp>::type type;
    typedef decltype(((type)1) + 0) __promoted_type;
};

template <class _Tp>
struct __sfinae_underlying_type<_Tp, false> {};

inline _LIBCPP_INLINE_VISIBILITY
int __convert_to_integral(int __val) { return __val; }

inline _LIBCPP_INLINE_VISIBILITY
unsigned __convert_to_integral(unsigned __val) { return __val; }

inline _LIBCPP_INLINE_VISIBILITY
long __convert_to_integral(long __val) { return __val; }

inline _LIBCPP_INLINE_VISIBILITY
unsigned long __convert_to_integral(unsigned long __val) { return __val; }

inline _LIBCPP_INLINE_VISIBILITY
long long __convert_to_integral(long long __val) { return __val; }

inline _LIBCPP_INLINE_VISIBILITY
unsigned long long __convert_to_integral(unsigned long long __val) {return __val; }

#ifndef _LIBCPP_HAS_NO_INT128
inline _LIBCPP_INLINE_VISIBILITY
__int128_t __convert_to_integral(__int128_t __val) { return __val; }

inline _LIBCPP_INLINE_VISIBILITY
__uint128_t __convert_to_integral(__uint128_t __val) { return __val; }
#endif

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
typename __sfinae_underlying_type<_Tp>::__promoted_type
__convert_to_integral(_Tp __val) { return __val; }

#ifndef _LIBCPP_HAS_NO_ADVANCED_SFINAE

template <class _Tp>
struct __has_operator_addressof_member_imp
{
    template <class _Up>
        static auto __test(int)
            -> typename __select_2nd<decltype(_VSTD::declval<_Up>().operator&()), true_type>::type;
    template <class>
        static auto __test(long) -> false_type;

    static const bool value = decltype(__test<_Tp>(0))::value;
};

template <class _Tp>
struct __has_operator_addressof_free_imp
{
    template <class _Up>
        static auto __test(int)
            -> typename __select_2nd<decltype(operator&(_VSTD::declval<_Up>())), true_type>::type;
    template <class>
        static auto __test(long) -> false_type;

    static const bool value = decltype(__test<_Tp>(0))::value;
};

template <class _Tp>
struct __has_operator_addressof
    : public integral_constant<bool, __has_operator_addressof_member_imp<_Tp>::value
                                  || __has_operator_addressof_free_imp<_Tp>::value>
{};

#endif  // _LIBCPP_HAS_NO_ADVANCED_SFINAE

#if _LIBCPP_STD_VER > 14
template <class...> using void_t = void;

# ifndef _LIBCPP_HAS_NO_VARIADICS
template <class... _Args>
struct conjunction : __and_<_Args...> {};
template<class... _Args> constexpr bool conjunction_v = conjunction<_Args...>::value;

template <class... _Args>
struct disjunction : __or_<_Args...> {};
template<class... _Args> constexpr bool disjunction_v = disjunction<_Args...>::value;

template <class _Tp>
struct negation : __not_<_Tp> {};
template<class _Tp> constexpr bool negation_v = negation<_Tp>::value;
# endif // _LIBCPP_HAS_NO_VARIADICS
#endif  // _LIBCPP_STD_VER > 14

// These traits are used in __tree and __hash_table
#ifndef _LIBCPP_CXX03_LANG
struct __extract_key_fail_tag {};
struct __extract_key_self_tag {};
struct __extract_key_first_tag {};

template <class _ValTy, class _Key,
          class _RawValTy = typename __unconstref<_ValTy>::type>
struct __can_extract_key
    : conditional<is_same<_RawValTy, _Key>::value, __extract_key_self_tag,
                  __extract_key_fail_tag>::type {};

template <class _Pair, class _Key, class _First, class _Second>
struct __can_extract_key<_Pair, _Key, pair<_First, _Second>>
    : conditional<is_same<typename remove_const<_First>::type, _Key>::value,
                  __extract_key_first_tag, __extract_key_fail_tag>::type {};

// __can_extract_map_key uses true_type/false_type instead of the tags.
// It returns true if _Key != _ContainerValueTy (the container is a map not a set)
// and _ValTy == _Key.
template <class _ValTy, class _Key, class _ContainerValueTy,
          class _RawValTy = typename __unconstref<_ValTy>::type>
struct __can_extract_map_key
    : integral_constant<bool, is_same<_RawValTy, _Key>::value> {};

// This specialization returns __extract_key_fail_tag for non-map containers
// because _Key == _ContainerValueTy
template <class _ValTy, class _Key, class _RawValTy>
struct __can_extract_map_key<_ValTy, _Key, _Key, _RawValTy>
    : false_type {};

#endif

_LIBCPP_END_NAMESPACE_STD

#endif  // __VTSS_BASICS_TYPE_TRAITS_HXX__
