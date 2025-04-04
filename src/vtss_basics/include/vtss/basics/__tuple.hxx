// -*- C++ -*-
//===----------------------------------------------------------------------===//
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

#ifndef __VTSS_BASICS___TUPLE_HXX__
#define __VTSS_BASICS___TUPLE_HXX__

#include <vtss/basics/__libcxx_config.hxx>
#include <cstddef>
#include <vtss/basics/type_traits.hxx>


_LIBCPP_BEGIN_NAMESPACE_STD

template <class _Tp> class _LIBCPP_TYPE_VIS_ONLY tuple_size;

template <class _Tp>
class _LIBCPP_TYPE_VIS_ONLY tuple_size<const _Tp>
    : public tuple_size<_Tp> {};

template <class _Tp>
class _LIBCPP_TYPE_VIS_ONLY tuple_size<volatile _Tp>
    : public tuple_size<_Tp> {};

template <class _Tp>
class _LIBCPP_TYPE_VIS_ONLY tuple_size<const volatile _Tp>
    : public tuple_size<_Tp> {};

template <size_t _Ip, class _Tp> class _LIBCPP_TYPE_VIS_ONLY tuple_element;

template <size_t _Ip, class _Tp>
class _LIBCPP_TYPE_VIS_ONLY tuple_element<_Ip, const _Tp>
{
public:
    typedef typename add_const<typename tuple_element<_Ip, _Tp>::type>::type type;
};

template <size_t _Ip, class _Tp>
class _LIBCPP_TYPE_VIS_ONLY tuple_element<_Ip, volatile _Tp>
{
public:
    typedef typename add_volatile<typename tuple_element<_Ip, _Tp>::type>::type type;
};

template <size_t _Ip, class _Tp>
class _LIBCPP_TYPE_VIS_ONLY tuple_element<_Ip, const volatile _Tp>
{
public:
    typedef typename add_cv<typename tuple_element<_Ip, _Tp>::type>::type type;
};

template <class _Tp> struct __tuple_like : false_type {};

template <class _Tp> struct __tuple_like<const _Tp> : public __tuple_like<_Tp> {};
template <class _Tp> struct __tuple_like<volatile _Tp> : public __tuple_like<_Tp> {};
template <class _Tp> struct __tuple_like<const volatile _Tp> : public __tuple_like<_Tp> {};

// tuple specializations

#if !defined(_LIBCPP_HAS_NO_VARIADICS)

template <size_t...> struct __tuple_indices {};

template <class _IdxType, _IdxType... _Values>
struct __integer_sequence {
  template <template <class _OIdxType, _OIdxType...> class _ToIndexSeq, class _ToIndexType>
  using __convert = _ToIndexSeq<_ToIndexType, _Values...>;

  template <size_t _Sp>
  using __to_tuple_indices = __tuple_indices<(_Values + _Sp)...>;
};

#if !__has_builtin(__make_integer_seq) || defined(_LIBCPP_TESTING_FALLBACK_MAKE_INTEGER_SEQUENCE)
namespace __detail {

template<typename _Tp, size_t ..._Extra> struct __repeat;
template<typename _Tp, _Tp ..._Np, size_t ..._Extra> struct __repeat<__integer_sequence<_Tp, _Np...>, _Extra...> {
  typedef __integer_sequence<_Tp,
                           _Np...,
                           sizeof...(_Np) + _Np...,
                           2 * sizeof...(_Np) + _Np...,
                           3 * sizeof...(_Np) + _Np...,
                           4 * sizeof...(_Np) + _Np...,
                           5 * sizeof...(_Np) + _Np...,
                           6 * sizeof...(_Np) + _Np...,
                           7 * sizeof...(_Np) + _Np...,
                           _Extra...> type;
};

template<size_t _Np> struct __parity;
template<size_t _Np> struct __make : __parity<_Np % 8>::template __pmake<_Np> {};

template<> struct __make<0> { typedef __integer_sequence<size_t> type; };
template<> struct __make<1> { typedef __integer_sequence<size_t, 0> type; };
template<> struct __make<2> { typedef __integer_sequence<size_t, 0, 1> type; };
template<> struct __make<3> { typedef __integer_sequence<size_t, 0, 1, 2> type; };
template<> struct __make<4> { typedef __integer_sequence<size_t, 0, 1, 2, 3> type; };
template<> struct __make<5> { typedef __integer_sequence<size_t, 0, 1, 2, 3, 4> type; };
template<> struct __make<6> { typedef __integer_sequence<size_t, 0, 1, 2, 3, 4, 5> type; };
template<> struct __make<7> { typedef __integer_sequence<size_t, 0, 1, 2, 3, 4, 5, 6> type; };

template<> struct __parity<0> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type> {}; };
template<> struct __parity<1> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 1> {}; };
template<> struct __parity<2> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 2, _Np - 1> {}; };
template<> struct __parity<3> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 3, _Np - 2, _Np - 1> {}; };
template<> struct __parity<4> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 4, _Np - 3, _Np - 2, _Np - 1> {}; };
template<> struct __parity<5> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 5, _Np - 4, _Np - 3, _Np - 2, _Np - 1> {}; };
template<> struct __parity<6> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 6, _Np - 5, _Np - 4, _Np - 3, _Np - 2, _Np - 1> {}; };
template<> struct __parity<7> { template<size_t _Np> struct __pmake : __repeat<typename __make<_Np / 8>::type, _Np - 7, _Np - 6, _Np - 5, _Np - 4, _Np - 3, _Np - 2, _Np - 1> {}; };

} // namespace detail

#endif  // !__has_builtin(__make_integer_seq) || defined(_LIBCPP_TESTING_FALLBACK_MAKE_INTEGER_SEQUENCE)

#if __has_builtin(__make_integer_seq)
template <size_t _Ep, size_t _Sp>
using __make_indices_imp =
    typename __make_integer_seq<__integer_sequence, size_t, _Ep - _Sp>::template
    __to_tuple_indices<_Sp>;
#else
template <size_t _Ep, size_t _Sp>
using __make_indices_imp =
    typename __detail::__make<_Ep - _Sp>::type::template __to_tuple_indices<_Sp>;

#endif

template <size_t _Ep, size_t _Sp = 0>
struct __make_tuple_indices
{
    static_assert(_Sp <= _Ep, "__make_tuple_indices input error");
    typedef __make_indices_imp<_Ep, _Sp> type;
};


template <class ..._Tp> class _LIBCPP_TYPE_VIS_ONLY tuple;

template <class... _Tp> struct __tuple_like<tuple<_Tp...> > : true_type {};

template <size_t _Ip, class ..._Tp>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
typename tuple_element<_Ip, tuple<_Tp...> >::type&
get(tuple<_Tp...>&) _NOEXCEPT;

template <size_t _Ip, class ..._Tp>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
const typename tuple_element<_Ip, tuple<_Tp...> >::type&
get(const tuple<_Tp...>&) _NOEXCEPT;

template <size_t _Ip, class ..._Tp>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
typename tuple_element<_Ip, tuple<_Tp...> >::type&&
get(tuple<_Tp...>&&) _NOEXCEPT;

template <size_t _Ip, class ..._Tp>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
const typename tuple_element<_Ip, tuple<_Tp...> >::type&&
get(const tuple<_Tp...>&&) _NOEXCEPT;
#endif

// pair specializations

template <class _T1, class _T2> struct __tuple_like<pair<_T1, _T2> > : true_type {};

template <size_t _Ip, class _T1, class _T2>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
typename tuple_element<_Ip, pair<_T1, _T2> >::type&
get(pair<_T1, _T2>&) _NOEXCEPT;

template <size_t _Ip, class _T1, class _T2>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
const typename tuple_element<_Ip, pair<_T1, _T2> >::type&
get(const pair<_T1, _T2>&) _NOEXCEPT;

#if !defined(_LIBCPP_HAS_NO_RVALUE_REFERENCES)
template <size_t _Ip, class _T1, class _T2>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
typename tuple_element<_Ip, pair<_T1, _T2> >::type&&
get(pair<_T1, _T2>&&) _NOEXCEPT;

template <size_t _Ip, class _T1, class _T2>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
const typename tuple_element<_Ip, pair<_T1, _T2> >::type&&
get(const pair<_T1, _T2>&&) _NOEXCEPT;
#endif

// array specializations

template <class _Tp, size_t _Size> struct _LIBCPP_TYPE_VIS_ONLY array;

template <class _Tp, size_t _Size> struct __tuple_like<array<_Tp, _Size> > : true_type {};

template <size_t _Ip, class _Tp, size_t _Size>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
_Tp&
get(array<_Tp, _Size>&) _NOEXCEPT;

template <size_t _Ip, class _Tp, size_t _Size>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
const _Tp&
get(const array<_Tp, _Size>&) _NOEXCEPT;

#if !defined(_LIBCPP_HAS_NO_RVALUE_REFERENCES)
template <size_t _Ip, class _Tp, size_t _Size>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
_Tp&&
get(array<_Tp, _Size>&&) _NOEXCEPT;

template <size_t _Ip, class _Tp, size_t _Size>
_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
const _Tp&&
get(const array<_Tp, _Size>&&) _NOEXCEPT;
#endif

#if !defined(_LIBCPP_HAS_NO_VARIADICS)


// __tuple_types

template <class ..._Tp> struct __tuple_types {};

#if !__has_builtin(__type_pack_element)

namespace __indexer_detail {

template <size_t _Idx, class _Tp>
struct __indexed { using type = _Tp; };

template <class _Types, class _Indexes> struct __indexer;

template <class ..._Types, size_t ..._Idx>
struct __indexer<__tuple_types<_Types...>, __tuple_indices<_Idx...>>
    : __indexed<_Idx, _Types>...
{};

template <size_t _Idx, class _Tp>
__indexed<_Idx, _Tp> __at_index(__indexed<_Idx, _Tp> const&);

} // namespace __indexer_detail

template <size_t _Idx, class ..._Types>
using __type_pack_element = typename decltype(
    __indexer_detail::__at_index<_Idx>(
        __indexer_detail::__indexer<
            __tuple_types<_Types...>,
            typename __make_tuple_indices<sizeof...(_Types)>::type
        >{})
  )::type;
#endif

template <size_t _Ip, class ..._Types>
class _LIBCPP_TYPE_VIS_ONLY tuple_element<_Ip, __tuple_types<_Types...>>
{
public:
    static_assert(_Ip < sizeof...(_Types), "tuple_element index out of range");
    typedef __type_pack_element<_Ip, _Types...> type;
};


template <class ..._Tp>
class _LIBCPP_TYPE_VIS_ONLY tuple_size<__tuple_types<_Tp...> >
    : public integral_constant<size_t, sizeof...(_Tp)>
{
};

template <class... _Tp> struct __tuple_like<__tuple_types<_Tp...> > : true_type {};

template <bool _ApplyLV, bool _ApplyConst, bool _ApplyVolatile>
struct __apply_cv_mf;
template <>
struct __apply_cv_mf<false, false, false> {
  template <class _Tp> using __apply = _Tp;
};
template <>
struct __apply_cv_mf<false, true, false> {
  template <class _Tp> using __apply = const _Tp;
};
template <>
struct __apply_cv_mf<false, false, true> {
  template <class _Tp> using __apply = volatile _Tp;
};
template <>
struct __apply_cv_mf<false, true, true> {
  template <class _Tp> using __apply = const volatile _Tp;
};
template <>
struct __apply_cv_mf<true, false, false> {
  template <class _Tp> using __apply = _Tp&;
};
template <>
struct __apply_cv_mf<true, true, false> {
  template <class _Tp> using __apply = const _Tp&;
};
template <>
struct __apply_cv_mf<true, false, true> {
  template <class _Tp> using __apply = volatile _Tp&;
};
template <>
struct __apply_cv_mf<true, true, true> {
  template <class _Tp> using __apply = const volatile _Tp&;
};
template <class _Tp, class _RawTp = typename remove_reference<_Tp>::type>
using __apply_cv_t = __apply_cv_mf<
    is_lvalue_reference<_Tp>::value,
    is_const<_RawTp>::value,
    is_volatile<_RawTp>::value>;

// __make_tuple_types

// __make_tuple_types<_Tuple<_Types...>, _Ep, _Sp>::type is a
// __tuple_types<_Types...> using only those _Types in the range [_Sp, _Ep).
// _Sp defaults to 0 and _Ep defaults to tuple_size<_Tuple>.  If _Tuple is a
// lvalue_reference type, then __tuple_types<_Types&...> is the result.

template <class _TupleTypes, class _TupleIndices>
struct __make_tuple_types_flat;

template <template <class...> class _Tuple, class ..._Types, size_t ..._Idx>
struct __make_tuple_types_flat<_Tuple<_Types...>, __tuple_indices<_Idx...>> {
  // Specialization for pair, tuple, and __tuple_types
  template <class _Tp, class _ApplyFn = __apply_cv_t<_Tp>>
  using __apply_quals = __tuple_types<
      typename _ApplyFn::template __apply<__type_pack_element<_Idx, _Types...>>...
    >;
};

template <class _Vt, size_t _Np, size_t ..._Idx>
struct __make_tuple_types_flat<array<_Vt, _Np>, __tuple_indices<_Idx...>> {
  template <size_t>
  using __value_type = _Vt;
  template <class _Tp, class _ApplyFn = __apply_cv_t<_Tp>>
  using __apply_quals = __tuple_types<
      typename _ApplyFn::template __apply<__value_type<_Idx>>...
    >;
};

template <class _Tp, size_t _Ep = tuple_size<typename remove_reference<_Tp>::type>::value,
          size_t _Sp = 0,
          bool _SameSize = (_Ep == tuple_size<typename remove_reference<_Tp>::type>::value)>
struct __make_tuple_types
{
    static_assert(_Sp <= _Ep, "__make_tuple_types input error");
    using _RawTp = typename remove_cv<typename remove_reference<_Tp>::type>::type;
    using _Maker = __make_tuple_types_flat<_RawTp, typename __make_tuple_indices<_Ep, _Sp>::type>;
    using type = typename _Maker::template __apply_quals<_Tp>;
};

template <class ..._Types, size_t _Ep>
struct __make_tuple_types<tuple<_Types...>, _Ep, 0, true> {
  typedef __tuple_types<_Types...> type;
};

template <class ..._Types, size_t _Ep>
struct __make_tuple_types<__tuple_types<_Types...>, _Ep, 0, true> {
  typedef __tuple_types<_Types...> type;
};

template <bool ..._Preds>
struct __all_dummy;

template <bool ..._Pred>
using __all = is_same<__all_dummy<_Pred...>, __all_dummy<((void)_Pred, true)...>>;

struct __tuple_sfinae_base {
  template <template <class, class...> class _Trait,
            class ..._LArgs, class ..._RArgs>
  static auto __do_test(__tuple_types<_LArgs...>, __tuple_types<_RArgs...>)
    -> __all<typename enable_if<_Trait<_LArgs, _RArgs>::value, bool>::type{true}...>;
  template <template <class...> class>
  static auto __do_test(...) -> false_type;

  template <class _FromArgs, class _ToArgs>
  using __constructible = decltype(__do_test<is_constructible>(_ToArgs{}, _FromArgs{}));
  template <class _FromArgs, class _ToArgs>
  using __convertible = decltype(__do_test<is_convertible>(_FromArgs{}, _ToArgs{}));
  template <class _FromArgs, class _ToArgs>
  using __assignable = decltype(__do_test<is_assignable>(_ToArgs{}, _FromArgs{}));
};

// __tuple_convertible

template <class _Tp, class _Up, bool = __tuple_like<typename remove_reference<_Tp>::type>::value,
                                bool = __tuple_like<_Up>::value>
struct __tuple_convertible
    : public false_type {};

template <class _Tp, class _Up>
struct __tuple_convertible<_Tp, _Up, true, true>
    : public __tuple_sfinae_base::__convertible<
      typename __make_tuple_types<_Tp>::type
    , typename __make_tuple_types<_Up>::type
    >
{};

// __tuple_constructible

template <class _Tp, class _Up, bool = __tuple_like<typename remove_reference<_Tp>::type>::value,
                                bool = __tuple_like<_Up>::value>
struct __tuple_constructible
    : public false_type {};

template <class _Tp, class _Up>
struct __tuple_constructible<_Tp, _Up, true, true>
    : public __tuple_sfinae_base::__constructible<
      typename __make_tuple_types<_Tp>::type
    , typename __make_tuple_types<_Up>::type
    >
{};

// __tuple_assignable

template <class _Tp, class _Up, bool = __tuple_like<typename remove_reference<_Tp>::type>::value,
                                bool = __tuple_like<_Up>::value>
struct __tuple_assignable
    : public false_type {};

template <class _Tp, class _Up>
struct __tuple_assignable<_Tp, _Up, true, true>
    : public __tuple_sfinae_base::__assignable<
      typename __make_tuple_types<_Tp>::type
    , typename __make_tuple_types<_Up&>::type
    >
{};

#endif  // _LIBCPP_HAS_NO_VARIADICS

_LIBCPP_END_NAMESPACE_STD

#endif  // __VTSS_BASICS___TUPLE_HXX__
