//===-------------------------- utility -----------------------------------===//
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

#ifndef __VTSS_BASICS_UTILITY_HXX__
#define __VTSS_BASICS_UTILITY_HXX__

/*
    utility synopsis

namespace std
{

template <class T>
    void
    swap(T& a, T& b);

namespace rel_ops
{
    template<class T> bool operator!=(const T&, const T&);
    template<class T> bool operator> (const T&, const T&);
    template<class T> bool operator<=(const T&, const T&);
    template<class T> bool operator>=(const T&, const T&);
}

template<class T>
void
swap(T& a, T& b) noexcept(is_nothrow_move_constructible<T>::value &&
                          is_nothrow_move_assignable<T>::value);

template <class T, size_t N>
void
swap(T (&a)[N], T (&b)[N]) noexcept(noexcept(swap(*a, *b)));

template <class T> T&& forward(typename remove_reference<T>::type& t) noexcept;  // constexpr in C++14
template <class T> T&& forward(typename remove_reference<T>::type&& t) noexcept; // constexpr in C++14

template <class T> typename remove_reference<T>::type&& move(T&&) noexcept;      // constexpr in C++14

template <class T>
    typename conditional
    <
        !is_nothrow_move_constructible<T>::value && is_copy_constructible<T>::value,
        const T&,
        T&&
    >::type
    move_if_noexcept(T& x) noexcept; // constexpr in C++14

template <class T> constexpr add_const<T>_t& as_const(T& t) noexcept;      // C++17
template <class T>                      void as_const(const T&&) = delete; // C++17

template <class T> typename add_rvalue_reference<T>::type declval() noexcept;

template <class T1, class T2>
struct Pair
{
    typedef T1 first_type;
    typedef T2 second_type;

    T1 first;
    T2 second;

    Pair(const Pair&) = default;
    Pair(Pair&&) = default;
    constexpr Pair();
    Pair(const T1& x, const T2& y);                          // constexpr in C++14
    template <class U, class V> Pair(U&& x, V&& y);          // constexpr in C++14
    template <class U, class V> Pair(const Pair<U, V>& p);   // constexpr in C++14
    template <class U, class V> Pair(Pair<U, V>&& p);        // constexpr in C++14
    template <class... Args1, class... Args2>
        Pair(piecewise_construct_t, tuple<Args1...> first_args,
             tuple<Args2...> second_args);

    template <class U, class V> Pair& operator=(const Pair<U, V>& p);
    Pair& operator=(Pair&& p) noexcept(is_nothrow_move_assignable<T1>::value &&
                                       is_nothrow_move_assignable<T2>::value);
    template <class U, class V> Pair& operator=(Pair<U, V>&& p);

    void swap(Pair& p) noexcept(is_nothrow_swappable_v<T1> &&
                                is_nothrow_swappable_v<T2>);
};

template <class T1, class T2> bool operator==(const Pair<T1,T2>&, const Pair<T1,T2>&); // constexpr in C++14
template <class T1, class T2> bool operator!=(const Pair<T1,T2>&, const Pair<T1,T2>&); // constexpr in C++14
template <class T1, class T2> bool operator< (const Pair<T1,T2>&, const Pair<T1,T2>&); // constexpr in C++14
template <class T1, class T2> bool operator> (const Pair<T1,T2>&, const Pair<T1,T2>&); // constexpr in C++14
template <class T1, class T2> bool operator>=(const Pair<T1,T2>&, const Pair<T1,T2>&); // constexpr in C++14
template <class T1, class T2> bool operator<=(const Pair<T1,T2>&, const Pair<T1,T2>&); // constexpr in C++14

template <class T1, class T2> Pair<V1, V2> make_pair(T1&&, T2&&);   // constexpr in C++14
template <class T1, class T2>
void
swap(Pair<T1, T2>& x, Pair<T1, T2>& y) noexcept(noexcept(x.swap(y)));

struct piecewise_construct_t { };
constexpr piecewise_construct_t piecewise_construct = piecewise_construct_t();

template <class T> class tuple_size;
template <size_t I, class T> class tuple_element;

template <class T1, class T2> struct tuple_size<Pair<T1, T2> >;
template <class T1, class T2> struct tuple_element<0, Pair<T1, T2> >;
template <class T1, class T2> struct tuple_element<1, Pair<T1, T2> >;

template<size_t I, class T1, class T2>
    typename tuple_element<I, Pair<T1, T2> >::type&
    get(Pair<T1, T2>&) noexcept; // constexpr in C++14

template<size_t I, class T1, class T2>
    const typename tuple_element<I, Pair<T1, T2> >::type&
    get(const Pair<T1, T2>&) noexcept; // constexpr in C++14

template<size_t I, class T1, class T2>
    typename tuple_element<I, Pair<T1, T2> >::type&&
    get(Pair<T1, T2>&&) noexcept; // constexpr in C++14

template<size_t I, class T1, class T2>
    const typename tuple_element<I, Pair<T1, T2> >::type&&
    get(const Pair<T1, T2>&&) noexcept; // constexpr in C++14

template<class T1, class T2>
    constexpr T1& get(Pair<T1, T2>&) noexcept; // C++14

template<class T1, class T2>
    constexpr const T1& get(const Pair<T1, T2>&) noexcept; // C++14

template<class T1, class T2>
    constexpr T1&& get(Pair<T1, T2>&&) noexcept; // C++14

template<class T1, class T2>
    constexpr const T1&& get(const Pair<T1, T2>&&) noexcept; // C++14

template<class T1, class T2>
    constexpr T1& get(Pair<T2, T1>&) noexcept; // C++14

template<class T1, class T2>
    constexpr const T1& get(const Pair<T2, T1>&) noexcept; // C++14

template<class T1, class T2>
    constexpr T1&& get(Pair<T2, T1>&&) noexcept; // C++14

template<class T1, class T2>
    constexpr const T1&& get(const Pair<T2, T1>&&) noexcept; // C++14

// C++14

template<class T, T... I>
struct integer_sequence
{
    typedef T value_type;

    static constexpr size_t size() noexcept;
};

template<size_t... I>
  using index_sequence = integer_sequence<size_t, I...>;

template<class T, T N>
  using make_integer_sequence = integer_sequence<T, 0, 1, ..., N-1>;
template<size_t N>
  using make_index_sequence = make_integer_sequence<size_t, N>;

template<class... T>
  using index_sequence_for = make_index_sequence<sizeof...(T)>;

template<class T, class U=T>
    T exchange(T& obj, U&& new_value);
}  // std

*/

#include <vtss/basics/__libcxx_config.hxx>
#include <vtss/basics/__tuple.hxx>
#include <vtss/basics/type_traits.hxx>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace rel_ops
{

template<class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
bool
operator!=(const _Tp& __x, const _Tp& __y)
{
    return !(__x == __y);
}

template<class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
bool
operator> (const _Tp& __x, const _Tp& __y)
{
    return __y < __x;
}

template<class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
bool
operator<=(const _Tp& __x, const _Tp& __y)
{
    return !(__y < __x);
}

template<class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
bool
operator>=(const _Tp& __x, const _Tp& __y)
{
    return !(__x < __y);
}

}  // rel_ops

// swap_ranges


template <class _ForwardIterator1, class _ForwardIterator2>
inline _LIBCPP_INLINE_VISIBILITY
_ForwardIterator2
swap_ranges(_ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2)
{
    for(; __first1 != __last1; ++__first1, (void) ++__first2)
        swap(*__first1, *__first2);
    return __first2;
}

// forward declared in <type_traits>
template<class _Tp, size_t _Np>
inline _LIBCPP_INLINE_VISIBILITY
typename enable_if<
    __is_swappable<_Tp>::value
>::type
swap(_Tp (&__a)[_Np], _Tp (&__b)[_Np]) _NOEXCEPT_(__is_nothrow_swappable<_Tp>::value)
{
    _VSTD::swap_ranges(__a, __a + _Np, __b);
}

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
typename conditional
<
    !is_nothrow_move_constructible<_Tp>::value && is_copy_constructible<_Tp>::value,
    const _Tp&,
    _Tp&&
>::type
#else  // _LIBCPP_HAS_NO_RVALUE_REFERENCES
const _Tp&
#endif
move_if_noexcept(_Tp& __x) _NOEXCEPT
{
    return _VSTD::move(__x);
}

#if _LIBCPP_STD_VER > 14
template <class _Tp> constexpr add_const_t<_Tp>& as_const(_Tp& __t) noexcept { return __t; }
template <class _Tp>                        void as_const(const _Tp&&) = delete;
#endif

struct _LIBCPP_TYPE_VIS_ONLY piecewise_construct_t { };
#if defined(_LIBCPP_HAS_NO_CONSTEXPR) || defined(_LIBCPP_BUILDING_UTILITY)
extern const piecewise_construct_t piecewise_construct;// = piecewise_construct_t();
#else
constexpr piecewise_construct_t piecewise_construct = piecewise_construct_t();
#endif

template <class _T1, class _T2>
struct _LIBCPP_TYPE_VIS_ONLY Pair
{
    typedef _T1 first_type;
    typedef _T2 second_type;

    _T1 first;
    _T2 second;

#ifndef _LIBCPP_HAS_NO_DEFAULT_FUNCTION_TEMPLATE_ARGS
    template <bool _Dummy = true, class = typename enable_if<
        __dependent_type<is_default_constructible<_T1>, _Dummy>::value &&
        __dependent_type<is_default_constructible<_T2>, _Dummy>::value
      >::type>
#endif
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR Pair() : first(), second() {}

    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    Pair(const _T1& __x, const _T2& __y)
        : first(__x), second(__y) {}

    template<class _U1, class _U2>
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        Pair(const Pair<_U1, _U2>& __p
#ifndef _LIBCPP_HAS_NO_ADVANCED_SFINAE
                 ,typename enable_if<is_convertible<const _U1&, _T1>::value &&
                                    is_convertible<const _U2&, _T2>::value>::type* = 0
#endif
                                      )
            : first(__p.first), second(__p.second) {}

#if defined(_LIBCPP_DEPRECATED_ABI_DISABLE_PAIR_TRIVIAL_COPY_CTOR)
    _LIBCPP_INLINE_VISIBILITY
    Pair(const Pair& __p)
        _NOEXCEPT_(is_nothrow_copy_constructible<first_type>::value &&
                   is_nothrow_copy_constructible<second_type>::value)
        : first(__p.first),
          second(__p.second)
    {
    }

# ifndef _LIBCPP_CXX03_LANG
    _LIBCPP_INLINE_VISIBILITY
    Pair(Pair&& __p) _NOEXCEPT_(is_nothrow_move_constructible<first_type>::value &&
                                is_nothrow_move_constructible<second_type>::value)
        : first(_VSTD::forward<first_type>(__p.first)),
          second(_VSTD::forward<second_type>(__p.second))
    {
    }
# endif
#elif !defined(_LIBCPP_CXX03_LANG)
    Pair(Pair const&) = default;
    Pair(Pair&&) = default;
#else
  // Use the implicitly declared copy constructor in C++03
#endif

    _LIBCPP_INLINE_VISIBILITY
    Pair& operator=(const Pair& __p)
        _NOEXCEPT_(is_nothrow_copy_assignable<first_type>::value &&
                   is_nothrow_copy_assignable<second_type>::value)
    {
        first = __p.first;
        second = __p.second;
        return *this;
    }

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

    template <class _U1, class _U2,
              class = typename enable_if<is_convertible<_U1, first_type>::value &&
                                         is_convertible<_U2, second_type>::value>::type>
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        Pair(_U1&& __u1, _U2&& __u2)
            : first(_VSTD::forward<_U1>(__u1)),
              second(_VSTD::forward<_U2>(__u2))
            {}

    template<class _U1, class _U2>
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        Pair(Pair<_U1, _U2>&& __p,
                 typename enable_if<is_convertible<_U1, _T1>::value &&
                                    is_convertible<_U2, _T2>::value>::type* = 0)
            : first(_VSTD::forward<_U1>(__p.first)),
              second(_VSTD::forward<_U2>(__p.second)) {}

    _LIBCPP_INLINE_VISIBILITY
    Pair&
    operator=(Pair&& __p) _NOEXCEPT_(is_nothrow_move_assignable<first_type>::value &&
                                     is_nothrow_move_assignable<second_type>::value)
    {
        first = _VSTD::forward<first_type>(__p.first);
        second = _VSTD::forward<second_type>(__p.second);
        return *this;
    }

#ifndef _LIBCPP_HAS_NO_VARIADICS

    template<class _Tuple,
             class = typename enable_if<__tuple_convertible<_Tuple, Pair>::value>::type>
        _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
        Pair(_Tuple&& __p)
            : first(_VSTD::forward<typename tuple_element<0,
                                  typename __make_tuple_types<_Tuple>::type>::type>(_VSTD::get<0>(__p))),
              second(_VSTD::forward<typename tuple_element<1,
                                   typename __make_tuple_types<_Tuple>::type>::type>(_VSTD::get<1>(__p)))
            {}



    template <class... _Args1, class... _Args2>
        _LIBCPP_INLINE_VISIBILITY
        Pair(piecewise_construct_t __pc, tuple<_Args1...> __first_args,
                                    tuple<_Args2...> __second_args)
            : Pair(__pc, __first_args, __second_args,
                   typename __make_tuple_indices<sizeof...(_Args1)>::type(),
                   typename __make_tuple_indices<sizeof...(_Args2) >::type())
            {}

    template <class _Tuple,
              class = typename enable_if<__tuple_assignable<_Tuple, Pair>::value>::type>
        _LIBCPP_INLINE_VISIBILITY
        Pair&
        operator=(_Tuple&& __p)
        {
            typedef typename __make_tuple_types<_Tuple>::type _TupleRef;
            typedef typename tuple_element<0, _TupleRef>::type _U0;
            typedef typename tuple_element<1, _TupleRef>::type _U1;
            first  = _VSTD::forward<_U0>(_VSTD::get<0>(__p));
            second = _VSTD::forward<_U1>(_VSTD::get<1>(__p));
            return *this;
        }

#endif  // _LIBCPP_HAS_NO_VARIADICS

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES
    _LIBCPP_INLINE_VISIBILITY
    void
    swap(Pair& __p) _NOEXCEPT_(__is_nothrow_swappable<first_type>::value &&
                               __is_nothrow_swappable<second_type>::value)
    {
        using _VSTD::swap;
        swap(first,  __p.first);
        swap(second, __p.second);
    }
private:

#ifndef _LIBCPP_HAS_NO_VARIADICS
    template <class... _Args1, class... _Args2, size_t... _I1, size_t... _I2>
        _LIBCPP_INLINE_VISIBILITY
        Pair(piecewise_construct_t,
             tuple<_Args1...>& __first_args, tuple<_Args2...>& __second_args,
             __tuple_indices<_I1...>, __tuple_indices<_I2...>);
#endif  // _LIBCPP_HAS_NO_VARIADICS
};

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator==(const Pair<_T1,_T2>& __x, const Pair<_T1,_T2>& __y)
{
    return __x.first == __y.first && __x.second == __y.second;
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator!=(const Pair<_T1,_T2>& __x, const Pair<_T1,_T2>& __y)
{
    return !(__x == __y);
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator< (const Pair<_T1,_T2>& __x, const Pair<_T1,_T2>& __y)
{
    return __x.first < __y.first || (!(__y.first < __x.first) && __x.second < __y.second);
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator> (const Pair<_T1,_T2>& __x, const Pair<_T1,_T2>& __y)
{
    return __y < __x;
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator>=(const Pair<_T1,_T2>& __x, const Pair<_T1,_T2>& __y)
{
    return !(__x < __y);
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
bool
operator<=(const Pair<_T1,_T2>& __x, const Pair<_T1,_T2>& __y)
{
    return !(__y < __x);
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY
typename enable_if
<
    __is_swappable<_T1>::value &&
    __is_swappable<_T2>::value,
    void
>::type
swap(Pair<_T1, _T2>& __x, Pair<_T1, _T2>& __y)
                     _NOEXCEPT_((__is_nothrow_swappable<_T1>::value &&
                                 __is_nothrow_swappable<_T2>::value))
{
    __x.swap(__y);
}

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES


template <class _Tp>
struct __make_pair_return_impl
{
    typedef _Tp type;
};

template <class _Tp>
struct __make_pair_return_impl<reference_wrapper<_Tp>>
{
    typedef _Tp& type;
};

template <class _Tp>
struct __make_pair_return
{
    typedef typename __make_pair_return_impl<typename decay<_Tp>::type>::type type;
};

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
Pair<typename __make_pair_return<_T1>::type, typename __make_pair_return<_T2>::type>
make_pair(_T1&& __t1, _T2&& __t2)
{
    return Pair<typename __make_pair_return<_T1>::type, typename __make_pair_return<_T2>::type>
               (_VSTD::forward<_T1>(__t1), _VSTD::forward<_T2>(__t2));
}

#else  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY
Pair<_T1,_T2>
make_pair(_T1 __x, _T2 __y)
{
    return Pair<_T1, _T2>(__x, __y);
}

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _T1, class _T2>
  class _LIBCPP_TYPE_VIS_ONLY tuple_size<Pair<_T1, _T2> >
    : public integral_constant<size_t, 2> {};

template <class _T1, class _T2>
class _LIBCPP_TYPE_VIS_ONLY tuple_element<0, Pair<_T1, _T2> >
{
public:
    typedef _T1 type;
};

template <class _T1, class _T2>
class _LIBCPP_TYPE_VIS_ONLY tuple_element<1, Pair<_T1, _T2> >
{
public:
    typedef _T2 type;
};

template <size_t _Ip> struct __get_pair;

template <>
struct __get_pair<0>
{
    template <class _T1, class _T2>
    static
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    _T1&
    get(Pair<_T1, _T2>& __p) _NOEXCEPT {return __p.first;}

    template <class _T1, class _T2>
    static
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    const _T1&
    get(const Pair<_T1, _T2>& __p) _NOEXCEPT {return __p.first;}

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

    template <class _T1, class _T2>
    static
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    _T1&&
    get(Pair<_T1, _T2>&& __p) _NOEXCEPT {return _VSTD::forward<_T1>(__p.first);}

    template <class _T1, class _T2>
    static
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    const _T1&&
    get(const Pair<_T1, _T2>&& __p) _NOEXCEPT {return _VSTD::forward<const _T1>(__p.first);}

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES
};

template <>
struct __get_pair<1>
{
    template <class _T1, class _T2>
    static
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    _T2&
    get(Pair<_T1, _T2>& __p) _NOEXCEPT {return __p.second;}

    template <class _T1, class _T2>
    static
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    const _T2&
    get(const Pair<_T1, _T2>& __p) _NOEXCEPT {return __p.second;}

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

    template <class _T1, class _T2>
    static
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    _T2&&
    get(Pair<_T1, _T2>&& __p) _NOEXCEPT {return _VSTD::forward<_T2>(__p.second);}

    template <class _T1, class _T2>
    static
    _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
    const _T2&&
    get(const Pair<_T1, _T2>&& __p) _NOEXCEPT {return _VSTD::forward<const _T2>(__p.second);}

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES
};

template <size_t _Ip, class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
typename tuple_element<_Ip, Pair<_T1, _T2> >::type&
get(Pair<_T1, _T2>& __p) _NOEXCEPT
{
    return __get_pair<_Ip>::get(__p);
}

template <size_t _Ip, class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
const typename tuple_element<_Ip, Pair<_T1, _T2> >::type&
get(const Pair<_T1, _T2>& __p) _NOEXCEPT
{
    return __get_pair<_Ip>::get(__p);
}

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <size_t _Ip, class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
typename tuple_element<_Ip, Pair<_T1, _T2> >::type&&
get(Pair<_T1, _T2>&& __p) _NOEXCEPT
{
    return __get_pair<_Ip>::get(_VSTD::move(__p));
}

template <size_t _Ip, class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX11
const typename tuple_element<_Ip, Pair<_T1, _T2> >::type&&
get(const Pair<_T1, _T2>&& __p) _NOEXCEPT
{
    return __get_pair<_Ip>::get(_VSTD::move(__p));
}

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

#if _LIBCPP_STD_VER > 11
template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1 & get(Pair<_T1, _T2>& __p) _NOEXCEPT
{
    return __get_pair<0>::get(__p);
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1 const & get(Pair<_T1, _T2> const& __p) _NOEXCEPT
{
    return __get_pair<0>::get(__p);
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1 && get(Pair<_T1, _T2>&& __p) _NOEXCEPT
{
    return __get_pair<0>::get(_VSTD::move(__p));
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1 const && get(Pair<_T1, _T2> const&& __p) _NOEXCEPT
{
    return __get_pair<0>::get(_VSTD::move(__p));
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1 & get(Pair<_T2, _T1>& __p) _NOEXCEPT
{
    return __get_pair<1>::get(__p);
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1 const & get(Pair<_T2, _T1> const& __p) _NOEXCEPT
{
    return __get_pair<1>::get(__p);
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1 && get(Pair<_T2, _T1>&& __p) _NOEXCEPT
{
    return __get_pair<1>::get(_VSTD::move(__p));
}

template <class _T1, class _T2>
inline _LIBCPP_INLINE_VISIBILITY
constexpr _T1 const && get(Pair<_T2, _T1> const&& __p) _NOEXCEPT
{
    return __get_pair<1>::get(_VSTD::move(__p));
}

#endif

#if _LIBCPP_STD_VER > 11

template<class _Tp, _Tp... _Ip>
struct _LIBCPP_TYPE_VIS_ONLY integer_sequence
{
    typedef _Tp value_type;
    static_assert( is_integral<_Tp>::value,
                  "std::integer_sequence can only be instantiated with an integral type" );
    static
    _LIBCPP_INLINE_VISIBILITY
    constexpr
    size_t
    size() noexcept { return sizeof...(_Ip); }
};

template<size_t... _Ip>
    using index_sequence = integer_sequence<size_t, _Ip...>;

#if __has_builtin(__make_integer_seq) && !defined(_LIBCPP_TESTING_FALLBACK_MAKE_INTEGER_SEQUENCE)

template <class _Tp, _Tp _Ep>
using __make_integer_sequence = __make_integer_seq<integer_sequence, _Tp, _Ep>;

#else

template<typename _Tp, _Tp _Np> using __make_integer_sequence_unchecked =
  typename __detail::__make<_Np>::type::template __convert<integer_sequence, _Tp>;

template <class _Tp, _Tp _Ep>
struct __make_integer_sequence_checked
{
    static_assert(is_integral<_Tp>::value,
                  "std::make_integer_sequence can only be instantiated with an integral type" );
    static_assert(0 <= _Ep, "std::make_integer_sequence must have a non-negative sequence length");
    // Workaround GCC bug by preventing bad installations when 0 <= _Ep
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68929
    typedef __make_integer_sequence_unchecked<_Tp, 0 <= _Ep ? _Ep : 0> type;
};

template <class _Tp, _Tp _Ep>
using __make_integer_sequence = typename __make_integer_sequence_checked<_Tp, _Ep>::type;

#endif

template<class _Tp, _Tp _Np>
    using make_integer_sequence = __make_integer_sequence<_Tp, _Np>;

template<size_t _Np>
    using make_index_sequence = make_integer_sequence<size_t, _Np>;

template<class... _Tp>
    using index_sequence_for = make_index_sequence<sizeof...(_Tp)>;

#endif  // _LIBCPP_STD_VER > 11

#if _LIBCPP_STD_VER > 11
template<class _T1, class _T2 = _T1>
inline _LIBCPP_INLINE_VISIBILITY
_T1 exchange(_T1& __obj, _T2 && __new_value)
{
    _T1 __old_value = _VSTD::move(__obj);
    __obj = _VSTD::forward<_T2>(__new_value);
    return __old_value;
}
#endif  // _LIBCPP_STD_VER > 11

_LIBCPP_END_NAMESPACE_STD

#endif  // __VTSS_BASICS_UTILITY_HXX__
