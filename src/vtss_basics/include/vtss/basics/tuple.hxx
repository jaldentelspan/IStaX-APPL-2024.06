// -*- C++ -*-
//===--------------------------- tuple ------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Importet from https://github.com/llvm-mirror/libcxx.git
// SHA 563647a240b9997cb9e414cbb09c82f19f00c89b
//
// Patches has been applied by MSCC on top of that - use a diff tool to see the
// changes.

#ifndef __VTSS_BASICS_TUPLE_HXX__
#define __VTSS_BASICS_TUPLE_HXX__

/*
    tuple synopsis

namespace std
{

template <class... T>
class tuple {
public:
    constexpr tuple();
    explicit tuple(const T&...);  // constexpr in C++14
    template <class... U>
        explicit tuple(U&&...);  // constexpr in C++14
    tuple(const tuple&) = default;
    tuple(tuple&&) = default;
    template <class... U>
        tuple(const tuple<U...>&);  // constexpr in C++14
    template <class... U>
        tuple(tuple<U...>&&);  // constexpr in C++14
    template <class U1, class U2>
        tuple(const pair<U1, U2>&); // iff sizeof...(T) == 2 // constexpr in C++14
    template <class U1, class U2>
        tuple(pair<U1, U2>&&); // iff sizeof...(T) == 2  // constexpr in C++14

    tuple& operator=(const tuple&);
    tuple&
        operator=(tuple&&) noexcept(AND(is_nothrow_move_assignable<T>::value ...));
    template <class... U>
        tuple& operator=(const tuple<U...>&);
    template <class... U>
        tuple& operator=(tuple<U...>&&);
    template <class U1, class U2>
        tuple& operator=(const pair<U1, U2>&); // iff sizeof...(T) == 2
    template <class U1, class U2>
        tuple& operator=(pair<U1, U2>&&); //iffsizeof...(T) == 2

    void swap(tuple&) noexcept(AND(swap(declval<T&>(), declval<T&>())...));
};

const unspecified ignore;

template <class... T> tuple<V...>  make_tuple(T&&...); // constexpr in C++14
template <class... T> tuple<ATypes...> forward_as_tuple(T&&...) noexcept; // constexpr in C++14
template <class... T> tuple<T&...> tie(T&...) noexcept; // constexpr in C++14
template <class... Tuples> tuple<CTypes...> tuple_cat(Tuples&&... tpls); // constexpr in C++14

// [tuple.apply], calling a function with a tuple of arguments:
template <class F, class Tuple>
  constexpr decltype(auto) apply(F&& f, Tuple&& t); // C++17
template <class T, class Tuple>
  constexpr T make_from_tuple(Tuple&& t); // C++17

// 20.4.1.4, tuple helper classes:
template <class T> class tuple_size; // undefined
template <class... T> class tuple_size<tuple<T...>>;
template <class T>
 constexpr size_t tuple_size_v = tuple_size<T>::value; // C++17
template <size_t I, class T> class tuple_element; // undefined
template <size_t I, class... T> class tuple_element<I, tuple<T...>>;
template <size_t I, class T>
  using tuple_element_t = typename tuple_element <I, T>::type; // C++14

// 20.4.1.5, element access:
template <size_t I, class... T>
    typename tuple_element<I, tuple<T...>>::type&
    get(tuple<T...>&) noexcept; // constexpr in C++14
template <size_t I, class... T>
    const typename tuple_element<I, tuple<T...>>::type&
    get(const tuple<T...>&) noexcept; // constexpr in C++14
template <size_t I, class... T>
    typename tuple_element<I, tuple<T...>>::type&&
    get(tuple<T...>&&) noexcept; // constexpr in C++14
template <size_t I, class... T>
    const typename tuple_element<I, tuple<T...>>::type&&
    get(const tuple<T...>&&) noexcept; // constexpr in C++14

template <class T1, class... T>
    constexpr T1& get(tuple<T...>&) noexcept;  // C++14
template <class T1, class... T>
    constexpr const T1& get(const tuple<T...>&) noexcept;   // C++14
template <class T1, class... T>
    constexpr T1&& get(tuple<T...>&&) noexcept;   // C++14
template <class T1, class... T>
    constexpr const T1&& get(const tuple<T...>&&) noexcept;   // C++14

// 20.4.1.6, relational operators:
template<class... T, class... U> bool operator==(const tuple<T...>&, const tuple<U...>&); // constexpr in C++14
template<class... T, class... U> bool operator<(const tuple<T...>&, const tuple<U...>&);  // constexpr in C++14
template<class... T, class... U> bool operator!=(const tuple<T...>&, const tuple<U...>&); // constexpr in C++14
template<class... T, class... U> bool operator>(const tuple<T...>&, const tuple<U...>&);  // constexpr in C++14
template<class... T, class... U> bool operator<=(const tuple<T...>&, const tuple<U...>&); // constexpr in C++14
template<class... T, class... U> bool operator>=(const tuple<T...>&, const tuple<U...>&); // constexpr in C++14

template <class... Types>
  void
  swap(tuple<Types...>& x, tuple<Types...>& y) noexcept(noexcept(x.swap(y)));

}  // std

*/

#include <vtss/basics/__libcxx_config.hxx>
#include <vtss/basics/__tuple.hxx>
#include <cstddef>
#include <vtss/basics/type_traits.hxx>
#include <vtss/basics/__functional_base.hxx>
#include <vtss/basics/utility.hxx>

_LIBCPP_BEGIN_NAMESPACE_STD

#ifndef _LIBCPP_HAS_NO_VARIADICS

// tuple_size

template <class ..._Tp>
class _LIBCPP_TYPE_VIS_ONLY tuple_size<tuple<_Tp...> >
    : public integral_constant<size_t, sizeof...(_Tp)>
{
};

// tuple_element

template <size_t _Ip, class ..._Tp>
class _LIBCPP_TYPE_VIS_ONLY tuple_element<_Ip, tuple<_Tp...> >
{
public:
    typedef typename tuple_element<_Ip, __tuple_types<_Tp...> >::type type;
};

#if _LIBCPP_STD_VER > 11
template <size_t _Ip, class ..._Tp>
using tuple_element_t = typename tuple_element <_Ip, _Tp...>::type;
#endif

// __tuple_leaf

template <size_t _Ip, class _Hp,
          bool=is_empty<_Hp>::value && !__libcpp_is_final<_Hp>::value
         >
class __tuple_leaf;

template <size_t _Ip, class _Hp, bool _Ep>
inline _LIBCPP_INLINE_VISIBILITY
void swap(__tuple_leaf<_Ip, _Hp, _Ep>& __x, __tuple_leaf<_Ip, _Hp, _Ep>& __y)
    _NOEXCEPT_(__is_nothrow_swappable<_Hp>::value)
{
    swap(__x.get(), __y.get());
}

template <size_t _Ip, class _Hp, bool>
class __tuple_leaf
{
    _Hp value;

    __tuple_leaf& operator=(const __tuple_leaf&);
public:
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR __tuple_leaf()
             _NOEXCEPT_(is_nothrow_default_constructible<_Hp>::value) : value()
       {static_assert(!is_reference<_Hp>::value,
              "Attempted to default construct a reference element in a tuple");}

    template <class _Tp,
              class = typename enable_if<
                  __lazy_and<
                      __lazy_not<is_same<typename decay<_Tp>::type, __tuple_leaf>>
                    , is_constructible<_Hp, _Tp>
                    >::value
                >::type
            >
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        explicit __tuple_leaf(_Tp&& __t) _NOEXCEPT_((is_nothrow_constructible<_Hp, _Tp>::value))
            : value(_VSTD::forward<_Tp>(__t))
        {static_assert(!is_reference<_Hp>::value ||
                       (is_lvalue_reference<_Hp>::value &&
                        (is_lvalue_reference<_Tp>::value ||
                         is_same<typename remove_reference<_Tp>::type,
                                 reference_wrapper<
                                    typename remove_reference<_Hp>::type
                                 >
                                >::value)) ||
                        (is_rvalue_reference<_Hp>::value &&
                         !is_lvalue_reference<_Tp>::value),
       "Attempted to construct a reference element in a tuple with an rvalue");}

    __tuple_leaf(const __tuple_leaf& __t) = default;
    __tuple_leaf(__tuple_leaf&& __t) = default;

    template <class _Tp>
        _LIBCPP_INLINE_VISIBILITY
        __tuple_leaf&
        operator=(_Tp&& __t) _NOEXCEPT_((is_nothrow_assignable<_Hp&, _Tp>::value))
        {
            value = _VSTD::forward<_Tp>(__t);
            return *this;
        }

    _LIBCPP_INLINE_VISIBILITY
    int swap(__tuple_leaf& __t) _NOEXCEPT_(__is_nothrow_swappable<__tuple_leaf>::value)
    {
        _VSTD::swap(*this, __t);
        return 0;
    }

    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11       _Hp& get()       _NOEXCEPT {return value;}
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11 const _Hp& get() const _NOEXCEPT {return value;}
};

template <size_t _Ip, class _Hp>
class __tuple_leaf<_Ip, _Hp, true>
    : private _Hp
{

    __tuple_leaf& operator=(const __tuple_leaf&);
public:
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR __tuple_leaf()
             _NOEXCEPT_(is_nothrow_default_constructible<_Hp>::value) {}

    template <class _Tp,
              class = typename enable_if<
                  __lazy_and<
                        __lazy_not<is_same<typename decay<_Tp>::type, __tuple_leaf>>
                      , is_constructible<_Hp, _Tp>
                    >::value
                >::type
            >
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        explicit __tuple_leaf(_Tp&& __t) _NOEXCEPT_((is_nothrow_constructible<_Hp, _Tp>::value))
            : _Hp(_VSTD::forward<_Tp>(__t)) {}

    __tuple_leaf(__tuple_leaf const &) = default;
    __tuple_leaf(__tuple_leaf &&) = default;

    template <class _Tp>
        _LIBCPP_INLINE_VISIBILITY
        __tuple_leaf&
        operator=(_Tp&& __t) _NOEXCEPT_((is_nothrow_assignable<_Hp&, _Tp>::value))
        {
            _Hp::operator=(_VSTD::forward<_Tp>(__t));
            return *this;
        }

    _LIBCPP_INLINE_VISIBILITY
    int
    swap(__tuple_leaf& __t) _NOEXCEPT_(__is_nothrow_swappable<__tuple_leaf>::value)
    {
        _VSTD::swap(*this, __t);
        return 0;
    }

    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11       _Hp& get()       _NOEXCEPT {return static_cast<_Hp&>(*this);}
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11 const _Hp& get() const _NOEXCEPT {return static_cast<const _Hp&>(*this);}
};

template <class ..._Tp>
_LIBCPP_INLINE_VISIBILITY
void __swallow(_Tp&&...) _NOEXCEPT {}

template <class ..._Tp>
struct __lazy_all : __all<_Tp::value...> {};

template <class _Tp>
struct __all_default_constructible;

template <class ..._Tp>
struct __all_default_constructible<__tuple_types<_Tp...>>
    : __all<is_default_constructible<_Tp>::value...>
{ };

// __tuple_impl

template<class _Indx, class ..._Tp> struct __tuple_impl;

template<size_t ..._Indx, class ..._Tp>
struct __tuple_impl<__tuple_indices<_Indx...>, _Tp...>
    : public __tuple_leaf<_Indx, _Tp>...
{
    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_CONSTEXPR __tuple_impl()
        _NOEXCEPT_(__all<is_nothrow_default_constructible<_Tp>::value...>::value) {}

    template <size_t ..._Uf, class ..._Tf,
              size_t ..._Ul, class ..._Tl, class ..._Up>
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        explicit
        __tuple_impl(__tuple_indices<_Uf...>, __tuple_types<_Tf...>,
                     __tuple_indices<_Ul...>, __tuple_types<_Tl...>,
                     _Up&&... __u)
                     _NOEXCEPT_((__all<is_nothrow_constructible<_Tf, _Up>::value...>::value &&
                                 __all<is_nothrow_default_constructible<_Tl>::value...>::value)) :
            __tuple_leaf<_Uf, _Tf>(_VSTD::forward<_Up>(__u))...,
            __tuple_leaf<_Ul, _Tl>()...
            {}

    template <class _Tuple,
              class = typename enable_if
                      <
                         __tuple_constructible<_Tuple, tuple<_Tp...> >::value
                      >::type
             >
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        __tuple_impl(_Tuple&& __t) _NOEXCEPT_((__all<is_nothrow_constructible<_Tp, typename tuple_element<_Indx,
                                       typename __make_tuple_types<_Tuple>::type>::type>::value...>::value))
            : __tuple_leaf<_Indx, _Tp>(_VSTD::forward<typename tuple_element<_Indx,
                                       typename __make_tuple_types<_Tuple>::type>::type>(_VSTD::get<_Indx>(__t)))...
            {}

    template <class _Tuple>
        _LIBCPP_INLINE_VISIBILITY
        typename enable_if
        <
            __tuple_assignable<_Tuple, tuple<_Tp...> >::value,
            __tuple_impl&
        >::type
        operator=(_Tuple&& __t) _NOEXCEPT_((__all<is_nothrow_assignable<_Tp&, typename tuple_element<_Indx,
                                       typename __make_tuple_types<_Tuple>::type>::type>::value...>::value))
        {
            __swallow(__tuple_leaf<_Indx, _Tp>::operator=(_VSTD::forward<typename tuple_element<_Indx,
                                       typename __make_tuple_types<_Tuple>::type>::type>(_VSTD::get<_Indx>(__t)))...);
            return *this;
        }

    __tuple_impl(const __tuple_impl&) = default;
    __tuple_impl(__tuple_impl&&) = default;

    _LIBCPP_INLINE_VISIBILITY
    __tuple_impl&
    operator=(const __tuple_impl& __t) _NOEXCEPT_((__all<is_nothrow_copy_assignable<_Tp>::value...>::value))
    {
        __swallow(__tuple_leaf<_Indx, _Tp>::operator=(static_cast<const __tuple_leaf<_Indx, _Tp>&>(__t).get())...);
        return *this;
    }

    _LIBCPP_INLINE_VISIBILITY
    __tuple_impl&
    operator=(__tuple_impl&& __t) _NOEXCEPT_((__all<is_nothrow_move_assignable<_Tp>::value...>::value))
    {
        __swallow(__tuple_leaf<_Indx, _Tp>::operator=(_VSTD::forward<_Tp>(static_cast<__tuple_leaf<_Indx, _Tp>&>(__t).get()))...);
        return *this;
    }

    _LIBCPP_INLINE_VISIBILITY
    void swap(__tuple_impl& __t)
        _NOEXCEPT_(__all<__is_nothrow_swappable<_Tp>::value...>::value)
    {
        __swallow(__tuple_leaf<_Indx, _Tp>::swap(static_cast<__tuple_leaf<_Indx, _Tp>&>(__t))...);
    }
};

template <bool _IsTuple, class _SizeTrait, size_t _Expected>
struct __tuple_like_with_size_imp : false_type {};

template <class _SizeTrait, size_t _Expected>
struct __tuple_like_with_size_imp<true, _SizeTrait, _Expected>
    : integral_constant<bool, _SizeTrait::value == _Expected> {};

template <class _Tuple, size_t _ExpectedSize,
          class _RawTuple = typename __uncvref<_Tuple>::type>
using __tuple_like_with_size = __tuple_like_with_size_imp<
                                   __tuple_like<_RawTuple>::value,
                                   tuple_size<_RawTuple>, _ExpectedSize
                              >;


struct _LIBCPP_TYPE_VIS __check_tuple_constructor_fail {
    template <class ...>
    static constexpr bool __enable_explicit() { return false; }
    template <class ...>
    static constexpr bool __enable_implicit() { return false; }
};

template <class ..._Tp>
class _LIBCPP_TYPE_VIS_ONLY tuple
{
    typedef __tuple_impl<typename __make_tuple_indices<sizeof...(_Tp)>::type, _Tp...> base;

    base base_;

    template <class ..._Args>
    struct _PackExpandsToThisTuple : false_type {};

    template <class _Arg>
    struct _PackExpandsToThisTuple<_Arg>
        : is_same<typename __uncvref<_Arg>::type, tuple> {};

    template <bool _MaybeEnable, class _Dummy = void>
    struct _CheckArgsConstructor : __check_tuple_constructor_fail {};

    template <class _Dummy>
    struct _CheckArgsConstructor<true, _Dummy>
    {
        template <class ..._Args>
        static constexpr bool __enable_explicit() {
            return
                __tuple_constructible<
                    tuple<_Args...>,
                    typename __make_tuple_types<tuple,
                             sizeof...(_Args) < sizeof...(_Tp) ?
                                 sizeof...(_Args) :
                                 sizeof...(_Tp)>::type
                >::value &&
                !__tuple_convertible<
                    tuple<_Args...>,
                    typename __make_tuple_types<tuple,
                             sizeof...(_Args) < sizeof...(_Tp) ?
                                 sizeof...(_Args) :
                                 sizeof...(_Tp)>::type
                >::value &&
                __all_default_constructible<
                    typename __make_tuple_types<tuple, sizeof...(_Tp),
                             sizeof...(_Args) < sizeof...(_Tp) ?
                                 sizeof...(_Args) :
                                 sizeof...(_Tp)>::type
                >::value;
        }

        template <class ..._Args>
        static constexpr bool __enable_implicit() {
            return
                __tuple_convertible<
                    tuple<_Args...>,
                    typename __make_tuple_types<tuple,
                             sizeof...(_Args) < sizeof...(_Tp) ?
                                 sizeof...(_Args) :
                                 sizeof...(_Tp)>::type
                >::value &&
                __all_default_constructible<
                    typename __make_tuple_types<tuple, sizeof...(_Tp),
                             sizeof...(_Args) < sizeof...(_Tp) ?
                                 sizeof...(_Args) :
                                 sizeof...(_Tp)>::type
                >::value;
        }
    };

    template <bool _MaybeEnable,
              bool = sizeof...(_Tp) == 1,
              class _Dummy = void>
    struct _CheckTupleLikeConstructor : __check_tuple_constructor_fail {};

    template <class _Dummy>
    struct _CheckTupleLikeConstructor<true, false, _Dummy>
    {
        template <class _Tuple>
        static constexpr bool __enable_implicit() {
            return __tuple_convertible<_Tuple, tuple>::value;
        }

        template <class _Tuple>
        static constexpr bool __enable_explicit() {
            return __tuple_constructible<_Tuple, tuple>::value
               && !__tuple_convertible<_Tuple, tuple>::value;
        }
    };

    template <class _Dummy>
    struct _CheckTupleLikeConstructor<true, true, _Dummy>
    {
        // This trait is used to disable the tuple-like constructor when
        // the UTypes... constructor should be selected instead.
        // See LWG issue #2549.
        template <class _Tuple>
        using _PreferTupleLikeConstructor = __lazy_or<
            // Don't attempt the two checks below if the tuple we are given
            // has the same type as this tuple.
            is_same<typename __uncvref<_Tuple>::type, tuple>,
            __lazy_and<
                __lazy_not<is_constructible<_Tp..., _Tuple>>,
                __lazy_not<is_convertible<_Tuple, _Tp...>>
            >
        >;

        template <class _Tuple>
        static constexpr bool __enable_implicit() {
            return __lazy_and<
                __tuple_convertible<_Tuple, tuple>,
                _PreferTupleLikeConstructor<_Tuple>
            >::value;
        }

        template <class _Tuple>
        static constexpr bool __enable_explicit() {
            return __lazy_and<
                __tuple_constructible<_Tuple, tuple>,
                _PreferTupleLikeConstructor<_Tuple>,
                __lazy_not<__tuple_convertible<_Tuple, tuple>>
            >::value;
        }
    };

    template <size_t _Jp, class ..._Up> friend _LIBCPP_CONSTEXPR_AFTER_CXX11
        typename tuple_element<_Jp, tuple<_Up...> >::type& get(tuple<_Up...>&) _NOEXCEPT;
    template <size_t _Jp, class ..._Up> friend _LIBCPP_CONSTEXPR_AFTER_CXX11
        const typename tuple_element<_Jp, tuple<_Up...> >::type& get(const tuple<_Up...>&) _NOEXCEPT;
    template <size_t _Jp, class ..._Up> friend _LIBCPP_CONSTEXPR_AFTER_CXX11
        typename tuple_element<_Jp, tuple<_Up...> >::type&& get(tuple<_Up...>&&) _NOEXCEPT;
    template <size_t _Jp, class ..._Up> friend _LIBCPP_CONSTEXPR_AFTER_CXX11
        const typename tuple_element<_Jp, tuple<_Up...> >::type&& get(const tuple<_Up...>&&) _NOEXCEPT;
public:

    template <bool _Dummy = true, class = typename enable_if<
        __all<__dependent_type<is_default_constructible<_Tp>, _Dummy>::value...>::value
    >::type>
    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_CONSTEXPR tuple()
        _NOEXCEPT_(__all<is_nothrow_default_constructible<_Tp>::value...>::value) {}

    template <bool _Dummy = true,
              typename enable_if
                      <
                         _CheckArgsConstructor<
                            _Dummy
                         >::template __enable_implicit<_Tp const&...>(),
                         bool
                      >::type = false
        >
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    tuple(const _Tp& ... __t) _NOEXCEPT_((__all<is_nothrow_copy_constructible<_Tp>::value...>::value))
        : base_(typename __make_tuple_indices<sizeof...(_Tp)>::type(),
                typename __make_tuple_types<tuple, sizeof...(_Tp)>::type(),
                typename __make_tuple_indices<0>::type(),
                typename __make_tuple_types<tuple, 0>::type(),
                __t...
               ) {}

    template <bool _Dummy = true,
              typename enable_if
                      <
                         _CheckArgsConstructor<
                            _Dummy
                         >::template __enable_explicit<_Tp const&...>(),
                         bool
                      >::type = false
        >
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    explicit tuple(const _Tp& ... __t) _NOEXCEPT_((__all<is_nothrow_copy_constructible<_Tp>::value...>::value))
        : base_(typename __make_tuple_indices<sizeof...(_Tp)>::type(),
                typename __make_tuple_types<tuple, sizeof...(_Tp)>::type(),
                typename __make_tuple_indices<0>::type(),
                typename __make_tuple_types<tuple, 0>::type(),
                __t...
               ) {}

    template <class ..._Up,
              typename enable_if
                      <
                         _CheckArgsConstructor<
                             sizeof...(_Up) <= sizeof...(_Tp)
                             && !_PackExpandsToThisTuple<_Up...>::value
                         >::template __enable_implicit<_Up...>(),
                         bool
                      >::type = false
             >
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        tuple(_Up&&... __u)
            _NOEXCEPT_((
                is_nothrow_constructible<base,
                    typename __make_tuple_indices<sizeof...(_Up)>::type,
                    typename __make_tuple_types<tuple, sizeof...(_Up)>::type,
                    typename __make_tuple_indices<sizeof...(_Tp), sizeof...(_Up)>::type,
                    typename __make_tuple_types<tuple, sizeof...(_Tp), sizeof...(_Up)>::type,
                    _Up...
                >::value
            ))
            : base_(typename __make_tuple_indices<sizeof...(_Up)>::type(),
                    typename __make_tuple_types<tuple, sizeof...(_Up)>::type(),
                    typename __make_tuple_indices<sizeof...(_Tp), sizeof...(_Up)>::type(),
                    typename __make_tuple_types<tuple, sizeof...(_Tp), sizeof...(_Up)>::type(),
                    _VSTD::forward<_Up>(__u)...) {}

    template <class ..._Up,
              typename enable_if
                      <
                         _CheckArgsConstructor<
                             sizeof...(_Up) <= sizeof...(_Tp)
                             && !_PackExpandsToThisTuple<_Up...>::value
                         >::template __enable_explicit<_Up...>(),
                         bool
                      >::type = false
             >
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        explicit
        tuple(_Up&&... __u)
            _NOEXCEPT_((
                is_nothrow_constructible<base,
                    typename __make_tuple_indices<sizeof...(_Up)>::type,
                    typename __make_tuple_types<tuple, sizeof...(_Up)>::type,
                    typename __make_tuple_indices<sizeof...(_Tp), sizeof...(_Up)>::type,
                    typename __make_tuple_types<tuple, sizeof...(_Tp), sizeof...(_Up)>::type,
                    _Up...
                >::value
            ))
            : base_(typename __make_tuple_indices<sizeof...(_Up)>::type(),
                    typename __make_tuple_types<tuple, sizeof...(_Up)>::type(),
                    typename __make_tuple_indices<sizeof...(_Tp), sizeof...(_Up)>::type(),
                    typename __make_tuple_types<tuple, sizeof...(_Tp), sizeof...(_Up)>::type(),
                    _VSTD::forward<_Up>(__u)...) {}

    template <class _Tuple,
              typename enable_if
                      <
                         _CheckTupleLikeConstructor<
                             __tuple_like_with_size<_Tuple, sizeof...(_Tp)>::value
                             && !_PackExpandsToThisTuple<_Tuple>::value
                         >::template __enable_implicit<_Tuple>(),
                         bool
                      >::type = false
             >
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        tuple(_Tuple&& __t) _NOEXCEPT_((is_nothrow_constructible<base, _Tuple>::value))
            : base_(_VSTD::forward<_Tuple>(__t)) {}

    template <class _Tuple,
              typename enable_if
                      <
                         _CheckTupleLikeConstructor<
                             __tuple_like_with_size<_Tuple, sizeof...(_Tp)>::value
                             && !_PackExpandsToThisTuple<_Tuple>::value
                         >::template __enable_explicit<_Tuple>(),
                         bool
                      >::type = false
             >
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        explicit
        tuple(_Tuple&& __t) _NOEXCEPT_((is_nothrow_constructible<base, _Tuple>::value))
            : base_(_VSTD::forward<_Tuple>(__t)) {}


    template <class _Tuple,
              class = typename enable_if
                      <
                         __tuple_assignable<_Tuple, tuple>::value
                      >::type
             >
        _LIBCPP_INLINE_VISIBILITY
        tuple&
        operator=(_Tuple&& __t) _NOEXCEPT_((is_nothrow_assignable<base&, _Tuple>::value))
        {
            base_.operator=(_VSTD::forward<_Tuple>(__t));
            return *this;
        }

    _LIBCPP_INLINE_VISIBILITY
    void swap(tuple& __t) _NOEXCEPT_(__all<__is_nothrow_swappable<_Tp>::value...>::value)
        {base_.swap(__t.base_);}
};

template <>
class _LIBCPP_TYPE_VIS_ONLY tuple<>
{
public:
    _LIBCPP_INLINE_VISIBILITY
    _LIBCPP_CONSTEXPR tuple() _NOEXCEPT {}
    template <class _Up>
    _LIBCPP_INLINE_VISIBILITY
        tuple(array<_Up, 0>) _NOEXCEPT {}
    _LIBCPP_INLINE_VISIBILITY
    void swap(tuple&) _NOEXCEPT {}
};

template <class ..._Tp>
inline _LIBCPP_INLINE_VISIBILITY
typename enable_if
<
    __all<__is_swappable<_Tp>::value...>::value,
    void
>::type
swap(tuple<_Tp...>& __t, tuple<_Tp...>& __u)
                 _NOEXCEPT_(__all<__is_nothrow_swappable<_Tp>::value...>::value)
    {__t.swap(__u);}

// get

template <size_t _Ip, class ..._Tp>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
typename tuple_element<_Ip, tuple<_Tp...> >::type&
get(tuple<_Tp...>& __t) _NOEXCEPT
{
    typedef typename tuple_element<_Ip, tuple<_Tp...> >::type type;
    return static_cast<__tuple_leaf<_Ip, type>&>(__t.base_).get();
}

template <size_t _Ip, class ..._Tp>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
const typename tuple_element<_Ip, tuple<_Tp...> >::type&
get(const tuple<_Tp...>& __t) _NOEXCEPT
{
    typedef typename tuple_element<_Ip, tuple<_Tp...> >::type type;
    return static_cast<const __tuple_leaf<_Ip, type>&>(__t.base_).get();
}

template <size_t _Ip, class ..._Tp>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
typename tuple_element<_Ip, tuple<_Tp...> >::type&&
get(tuple<_Tp...>&& __t) _NOEXCEPT
{
    typedef typename tuple_element<_Ip, tuple<_Tp...> >::type type;
    return static_cast<type&&>(
             static_cast<__tuple_leaf<_Ip, type>&&>(__t.base_).get());
}

template <size_t _Ip, class ..._Tp>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
const typename tuple_element<_Ip, tuple<_Tp...> >::type&&
get(const tuple<_Tp...>&& __t) _NOEXCEPT
{
    typedef typename tuple_element<_Ip, tuple<_Tp...> >::type type;
    return static_cast<const type&&>(
             static_cast<const __tuple_leaf<_Ip, type>&&>(__t.base_).get());
}

#if _LIBCPP_STD_VER > 11
// get by type
template <typename _T1, size_t _Idx, typename... _Args>
struct __find_exactly_one_t_helper;

namespace __find_detail {

static constexpr size_t __not_found = -1;
static constexpr size_t __ambiguous = __not_found - 1;

inline _LIBCPP_INLINE_VISIBILITY
constexpr size_t __find_idx_return(size_t __curr_i, size_t __res, bool __matches) {
    return !__matches ? __res :
        (__res == __not_found ? __curr_i : __ambiguous);
}

template <size_t _Nx>
inline _LIBCPP_INLINE_VISIBILITY
constexpr size_t __find_idx(size_t __i, const bool (&__matches)[_Nx]) {
  return __i == _Nx ? __not_found :
      __find_idx_return(__i, __find_idx(__i + 1, __matches), __matches[__i]);
}

template <class _T1, class ..._Args>
struct __find_exactly_one_checked {
  static constexpr bool __matches[] = {is_same<_T1, _Args>::value...};
    static constexpr size_t value = __find_detail::__find_idx(0, __matches);
    static_assert (value != __not_found, "type not found in type list" );
    static_assert(value != __ambiguous,"type occurs more than once in type list");
};

template <class _T1>
struct __find_exactly_one_checked<_T1> {
    static_assert(!is_same<_T1, _T1>::value, "type not in empty type list");
};

} // namespace __find_detail;

template <typename _T1, typename... _Args>
struct __find_exactly_one_t
    : public __find_detail::__find_exactly_one_checked<_T1, _Args...> {
};

template <class _T1, class... _Args>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1& get(tuple<_Args...>& __tup) noexcept
{
    return _VSTD::get<__find_exactly_one_t<_T1, _Args...>::value>(__tup);
}

template <class _T1, class... _Args>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1 const& get(tuple<_Args...> const& __tup) noexcept
{
    return _VSTD::get<__find_exactly_one_t<_T1, _Args...>::value>(__tup);
}

template <class _T1, class... _Args>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1&& get(tuple<_Args...>&& __tup) noexcept
{
    return _VSTD::get<__find_exactly_one_t<_T1, _Args...>::value>(_VSTD::move(__tup));
}

template <class _T1, class... _Args>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1 const&& get(tuple<_Args...> const&& __tup) noexcept
{
    return _VSTD::get<__find_exactly_one_t<_T1, _Args...>::value>(_VSTD::move(__tup));
}

#endif

// tie

template <class ..._Tp>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
tuple<_Tp&...>
tie(_Tp&... __t) _NOEXCEPT
{
    return tuple<_Tp&...>(__t...);
}

template <class _Up>
struct __ignore_t
{
    template <class _Tp>
        _LIBCPP_INLINE_VISIBILITY
        const __ignore_t& operator=(_Tp&&) const {return *this;}
};

namespace { const __ignore_t<unsigned char> ignore = __ignore_t<unsigned char>(); }

template <class _Tp>
struct __make_tuple_return_impl
{
    typedef _Tp type;
};

template <class _Tp>
struct __make_tuple_return_impl<reference_wrapper<_Tp> >
{
    typedef _Tp& type;
};

template <class _Tp>
struct __make_tuple_return
{
    typedef typename __make_tuple_return_impl<typename decay<_Tp>::type>::type type;
};

template <class... _Tp>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
tuple<typename __make_tuple_return<_Tp>::type...>
make_tuple(_Tp&&... __t)
{
    return tuple<typename __make_tuple_return<_Tp>::type...>(_VSTD::forward<_Tp>(__t)...);
}

template <class... _Tp>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
tuple<_Tp&&...>
forward_as_tuple(_Tp&&... __t) _NOEXCEPT
{
    return tuple<_Tp&&...>(_VSTD::forward<_Tp>(__t)...);
}

template <size_t _Ip>
struct __tuple_equal
{
    template <class _Tp, class _Up>
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    bool operator()(const _Tp& __x, const _Up& __y)
    {
        return __tuple_equal<_Ip - 1>()(__x, __y) && _VSTD::get<_Ip-1>(__x) == _VSTD::get<_Ip-1>(__y);
    }
};

template <>
struct __tuple_equal<0>
{
    template <class _Tp, class _Up>
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    bool operator()(const _Tp&, const _Up&)
    {
        return true;
    }
};

template <class ..._Tp, class ..._Up>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator==(const tuple<_Tp...>& __x, const tuple<_Up...>& __y)
{
    return __tuple_equal<sizeof...(_Tp)>()(__x, __y);
}

template <class ..._Tp, class ..._Up>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator!=(const tuple<_Tp...>& __x, const tuple<_Up...>& __y)
{
    return !(__x == __y);
}

template <size_t _Ip>
struct __tuple_less
{
    template <class _Tp, class _Up>
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    bool operator()(const _Tp& __x, const _Up& __y)
    {
        const size_t __idx = tuple_size<_Tp>::value - _Ip;
        if (_VSTD::get<__idx>(__x) < _VSTD::get<__idx>(__y))
            return true;
        if (_VSTD::get<__idx>(__y) < _VSTD::get<__idx>(__x))
            return false;
        return __tuple_less<_Ip-1>()(__x, __y);
    }
};

template <>
struct __tuple_less<0>
{
    template <class _Tp, class _Up>
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    bool operator()(const _Tp&, const _Up&)
    {
        return false;
    }
};

template <class ..._Tp, class ..._Up>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator<(const tuple<_Tp...>& __x, const tuple<_Up...>& __y)
{
    return __tuple_less<sizeof...(_Tp)>()(__x, __y);
}

template <class ..._Tp, class ..._Up>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator>(const tuple<_Tp...>& __x, const tuple<_Up...>& __y)
{
    return __y < __x;
}

template <class ..._Tp, class ..._Up>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator>=(const tuple<_Tp...>& __x, const tuple<_Up...>& __y)
{
    return !(__x < __y);
}

template <class ..._Tp, class ..._Up>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator<=(const tuple<_Tp...>& __x, const tuple<_Up...>& __y)
{
    return !(__y < __x);
}

// tuple_cat

template <class _Tp, class _Up> struct __tuple_cat_type;

template <class ..._Ttypes, class ..._Utypes>
struct __tuple_cat_type<tuple<_Ttypes...>, __tuple_types<_Utypes...> >
{
    typedef tuple<_Ttypes..., _Utypes...> type;
};

template <class _ResultTuple, bool _Is_Tuple0TupleLike, class ..._Tuples>
struct __tuple_cat_return_1
{
};

template <class ..._Types, class _Tuple0>
struct __tuple_cat_return_1<tuple<_Types...>, true, _Tuple0>
{
    typedef typename __tuple_cat_type<tuple<_Types...>,
            typename __make_tuple_types<typename remove_reference<_Tuple0>::type>::type>::type
                                                                           type;
};

template <class ..._Types, class _Tuple0, class _Tuple1, class ..._Tuples>
struct __tuple_cat_return_1<tuple<_Types...>, true, _Tuple0, _Tuple1, _Tuples...>
    : public __tuple_cat_return_1<
                 typename __tuple_cat_type<
                     tuple<_Types...>,
                     typename __make_tuple_types<typename remove_reference<_Tuple0>::type>::type
                 >::type,
                 __tuple_like<typename remove_reference<_Tuple1>::type>::value,
                 _Tuple1, _Tuples...>
{
};

template <class ..._Tuples> struct __tuple_cat_return;

template <class _Tuple0, class ..._Tuples>
struct __tuple_cat_return<_Tuple0, _Tuples...>
    : public __tuple_cat_return_1<tuple<>,
         __tuple_like<typename remove_reference<_Tuple0>::type>::value, _Tuple0,
                                                                     _Tuples...>
{
};

template <>
struct __tuple_cat_return<>
{
    typedef tuple<> type;
};

inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
tuple<>
tuple_cat()
{
    return tuple<>();
}

template <class _Rp, class _Indices, class _Tuple0, class ..._Tuples>
struct __tuple_cat_return_ref_imp;

template <class ..._Types, size_t ..._I0, class _Tuple0>
struct __tuple_cat_return_ref_imp<tuple<_Types...>, __tuple_indices<_I0...>, _Tuple0>
{
    typedef typename remove_reference<_Tuple0>::type _T0;
    typedef tuple<_Types..., typename __apply_cv<_Tuple0,
                          typename tuple_element<_I0, _T0>::type>::type&&...> type;
};

template <class ..._Types, size_t ..._I0, class _Tuple0, class _Tuple1, class ..._Tuples>
struct __tuple_cat_return_ref_imp<tuple<_Types...>, __tuple_indices<_I0...>,
                                  _Tuple0, _Tuple1, _Tuples...>
    : public __tuple_cat_return_ref_imp<
         tuple<_Types..., typename __apply_cv<_Tuple0,
               typename tuple_element<_I0,
                  typename remove_reference<_Tuple0>::type>::type>::type&&...>,
         typename __make_tuple_indices<tuple_size<typename
                                 remove_reference<_Tuple1>::type>::value>::type,
         _Tuple1, _Tuples...>
{
};

template <class _Tuple0, class ..._Tuples>
struct __tuple_cat_return_ref
    : public __tuple_cat_return_ref_imp<tuple<>,
               typename __make_tuple_indices<
                        tuple_size<typename remove_reference<_Tuple0>::type>::value
               >::type, _Tuple0, _Tuples...>
{
};

template <class _Types, class _I0, class _J0>
struct __tuple_cat;

template <class ..._Types, size_t ..._I0, size_t ..._J0>
struct __tuple_cat<tuple<_Types...>, __tuple_indices<_I0...>, __tuple_indices<_J0...> >
{
    template <class _Tuple0>
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    typename __tuple_cat_return_ref<tuple<_Types...>&&, _Tuple0&&>::type
    operator()(tuple<_Types...> __t, _Tuple0&& __t0)
    {
        return forward_as_tuple(_VSTD::forward<_Types>(_VSTD::get<_I0>(__t))...,
                                      _VSTD::get<_J0>(_VSTD::forward<_Tuple0>(__t0))...);
    }

    template <class _Tuple0, class _Tuple1, class ..._Tuples>
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    typename __tuple_cat_return_ref<tuple<_Types...>&&, _Tuple0&&, _Tuple1&&, _Tuples&&...>::type
    operator()(tuple<_Types...> __t, _Tuple0&& __t0, _Tuple1&& __t1, _Tuples&& ...__tpls)
    {
        typedef typename remove_reference<_Tuple0>::type _T0;
        typedef typename remove_reference<_Tuple1>::type _T1;
        return __tuple_cat<
           tuple<_Types..., typename __apply_cv<_Tuple0, typename tuple_element<_J0, _T0>::type>::type&&...>,
           typename __make_tuple_indices<sizeof ...(_Types) + tuple_size<_T0>::value>::type,
           typename __make_tuple_indices<tuple_size<_T1>::value>::type>()
                           (forward_as_tuple(
                              _VSTD::forward<_Types>(_VSTD::get<_I0>(__t))...,
                              _VSTD::get<_J0>(_VSTD::forward<_Tuple0>(__t0))...
                            ),
                            _VSTD::forward<_Tuple1>(__t1),
                            _VSTD::forward<_Tuples>(__tpls)...);
    }
};

template <class _Tuple0, class... _Tuples>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
typename __tuple_cat_return<_Tuple0, _Tuples...>::type
tuple_cat(_Tuple0&& __t0, _Tuples&&... __tpls)
{
    typedef typename remove_reference<_Tuple0>::type _T0;
    return __tuple_cat<tuple<>, __tuple_indices<>,
                  typename __make_tuple_indices<tuple_size<_T0>::value>::type>()
                  (tuple<>(), _VSTD::forward<_Tuple0>(__t0),
                                            _VSTD::forward<_Tuples>(__tpls)...);
}

template <class _T1, class _T2>
template <class... _Args1, class... _Args2, size_t ..._I1, size_t ..._I2>
inline _LIBCPP_INLINE_VISIBILITY
Pair<_T1, _T2>::Pair(piecewise_construct_t,
                     tuple<_Args1...>& __first_args, tuple<_Args2...>& __second_args,
                     __tuple_indices<_I1...>, __tuple_indices<_I2...>)
    :  first(_VSTD::forward<_Args1>(_VSTD::get<_I1>( __first_args))...),
      second(_VSTD::forward<_Args2>(_VSTD::get<_I2>(__second_args))...)
{
}

#endif  // _LIBCPP_HAS_NO_VARIADICS

#if _LIBCPP_STD_VER > 14
template <class _Tp>
constexpr size_t tuple_size_v = tuple_size<_Tp>::value;

#define _LIBCPP_NOEXCEPT_RETURN(...) noexcept(noexcept(__VA_ARGS__)) { return __VA_ARGS__; }

template <class _Fn, class _Tuple, size_t ..._Id>
inline _LIBCPP_INLINE_VISIBILITY
constexpr decltype(auto) __apply_tuple_impl(_Fn && __f, _Tuple && __t,
                                            __tuple_indices<_Id...>)
_LIBCPP_NOEXCEPT_RETURN(
    _VSTD::__invoke_constexpr(
        _VSTD::forward<_Fn>(__f),
        _VSTD::get<_Id>(_VSTD::forward<_Tuple>(__t))...)
)

template <class _Fn, class _Tuple>
inline _LIBCPP_INLINE_VISIBILITY
constexpr decltype(auto) apply(_Fn && __f, _Tuple && __t)
_LIBCPP_NOEXCEPT_RETURN(
    _VSTD::__apply_tuple_impl(
        _VSTD::forward<_Fn>(__f), _VSTD::forward<_Tuple>(__t),
        typename __make_tuple_indices<tuple_size_v<decay_t<_Tuple>>>::type{})
)

template <class _Tp, class _Tuple, size_t... _Idx>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _Tp __make_from_tuple_impl(_Tuple&& __t, __tuple_indices<_Idx...>)
_LIBCPP_NOEXCEPT_RETURN(
    _Tp(_VSTD::get<_Idx>(_VSTD::forward<_Tuple>(__t))...)
)

template <class _Tp, class _Tuple>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _Tp make_from_tuple(_Tuple&& __t)
_LIBCPP_NOEXCEPT_RETURN(
    _VSTD::__make_from_tuple_impl<_Tp>(_VSTD::forward<_Tuple>(__t),
        typename __make_tuple_indices<tuple_size_v<decay_t<_Tuple>>>::type{})
)

#undef _LIBCPP_NOEXCEPT_RETURN

#endif // _LIBCPP_STD_VER > 14

_LIBCPP_END_NAMESPACE_STD

#endif  // __VTSS_BASICS_TUPLE_HXX__
