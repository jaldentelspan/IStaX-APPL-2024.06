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

#ifndef __VTSS_BASICS___FUNCTIONAL_BASE_HXX__
#define __VTSS_BASICS___FUNCTIONAL_BASE_HXX__


#include <vtss/basics/__libcxx_config.hxx>
#include <vtss/basics/type_traits.hxx>

_LIBCPP_BEGIN_NAMESPACE_STD

template <class _Arg, class _Result>
struct _LIBCPP_TYPE_VIS_ONLY unary_function
{
    typedef _Arg    argument_type;
    typedef _Result result_type;
};

template <class _Arg1, class _Arg2, class _Result>
struct _LIBCPP_TYPE_VIS_ONLY binary_function
{
    typedef _Arg1   first_argument_type;
    typedef _Arg2   second_argument_type;
    typedef _Result result_type;
};

template <class _Tp>
struct __has_result_type
{
private:
    struct __two {char __lx; char __lxx;};
    template <class _Up> static __two __test(...);
    template <class _Up> static char __test(typename _Up::result_type* = 0);
public:
    static const bool value = sizeof(__test<_Tp>(0)) == 1;
};

#if _LIBCPP_STD_VER > 11
template <class _Tp = void>
#else
template <class _Tp>
#endif
struct _LIBCPP_TYPE_VIS_ONLY less : binary_function<_Tp, _Tp, bool>
{
    _LIBCPP_CONSTEXPR_AFTER_CXX11 _LIBCPP_INLINE_VISIBILITY 
    bool operator()(const _Tp& __x, const _Tp& __y) const
        {return __x < __y;}
};

#if _LIBCPP_STD_VER > 11
template <>
struct _LIBCPP_TYPE_VIS_ONLY less<void>
{
    template <class _T1, class _T2> 
    _LIBCPP_CONSTEXPR_AFTER_CXX11 _LIBCPP_INLINE_VISIBILITY
    auto operator()(_T1&& __t, _T2&& __u) const
    _NOEXCEPT_(noexcept(_VSTD::forward<_T1>(__t) < _VSTD::forward<_T2>(__u)))
    -> decltype        (_VSTD::forward<_T1>(__t) < _VSTD::forward<_T2>(__u))
        { return        _VSTD::forward<_T1>(__t) < _VSTD::forward<_T2>(__u); }
    typedef void is_transparent;
};
#endif

// __weak_result_type

template <class _Tp>
struct __derives_from_unary_function
{
private:
    struct __two {char __lx; char __lxx;};
    static __two __test(...);
    template <class _Ap, class _Rp>
        static unary_function<_Ap, _Rp>
        __test(const volatile unary_function<_Ap, _Rp>*);
public:
    static const bool value = !is_same<decltype(__test((_Tp*)0)), __two>::value;
    typedef decltype(__test((_Tp*)0)) type;
};

template <class _Tp>
struct __derives_from_binary_function
{
private:
    struct __two {char __lx; char __lxx;};
    static __two __test(...);
    template <class _A1, class _A2, class _Rp>
        static binary_function<_A1, _A2, _Rp>
        __test(const volatile binary_function<_A1, _A2, _Rp>*);
public:
    static const bool value = !is_same<decltype(__test((_Tp*)0)), __two>::value;
    typedef decltype(__test((_Tp*)0)) type;
};

template <class _Tp, bool = __derives_from_unary_function<_Tp>::value>
struct __maybe_derive_from_unary_function  // bool is true
    : public __derives_from_unary_function<_Tp>::type
{
};

template <class _Tp>
struct __maybe_derive_from_unary_function<_Tp, false>
{
};

template <class _Tp, bool = __derives_from_binary_function<_Tp>::value>
struct __maybe_derive_from_binary_function  // bool is true
    : public __derives_from_binary_function<_Tp>::type
{
};

template <class _Tp>
struct __maybe_derive_from_binary_function<_Tp, false>
{
};

template <class _Tp, bool = __has_result_type<_Tp>::value>
struct __weak_result_type_imp // bool is true
    : public __maybe_derive_from_unary_function<_Tp>,
      public __maybe_derive_from_binary_function<_Tp>
{
    typedef typename _Tp::result_type result_type;
};

template <class _Tp>
struct __weak_result_type_imp<_Tp, false>
    : public __maybe_derive_from_unary_function<_Tp>,
      public __maybe_derive_from_binary_function<_Tp>
{
};

template <class _Tp>
struct __weak_result_type
    : public __weak_result_type_imp<_Tp>
{
};

// 0 argument case

template <class _Rp>
struct __weak_result_type<_Rp ()>
{
    typedef _Rp result_type;
};

template <class _Rp>
struct __weak_result_type<_Rp (&)()>
{
    typedef _Rp result_type;
};

template <class _Rp>
struct __weak_result_type<_Rp (*)()>
{
    typedef _Rp result_type;
};

// 1 argument case

template <class _Rp, class _A1>
struct __weak_result_type<_Rp (_A1)>
    : public unary_function<_A1, _Rp>
{
};

template <class _Rp, class _A1>
struct __weak_result_type<_Rp (&)(_A1)>
    : public unary_function<_A1, _Rp>
{
};

template <class _Rp, class _A1>
struct __weak_result_type<_Rp (*)(_A1)>
    : public unary_function<_A1, _Rp>
{
};

template <class _Rp, class _Cp>
struct __weak_result_type<_Rp (_Cp::*)()>
    : public unary_function<_Cp*, _Rp>
{
};

template <class _Rp, class _Cp>
struct __weak_result_type<_Rp (_Cp::*)() const>
    : public unary_function<const _Cp*, _Rp>
{
};

template <class _Rp, class _Cp>
struct __weak_result_type<_Rp (_Cp::*)() volatile>
    : public unary_function<volatile _Cp*, _Rp>
{
};

template <class _Rp, class _Cp>
struct __weak_result_type<_Rp (_Cp::*)() const volatile>
    : public unary_function<const volatile _Cp*, _Rp>
{
};

// 2 argument case

template <class _Rp, class _A1, class _A2>
struct __weak_result_type<_Rp (_A1, _A2)>
    : public binary_function<_A1, _A2, _Rp>
{
};

template <class _Rp, class _A1, class _A2>
struct __weak_result_type<_Rp (*)(_A1, _A2)>
    : public binary_function<_A1, _A2, _Rp>
{
};

template <class _Rp, class _A1, class _A2>
struct __weak_result_type<_Rp (&)(_A1, _A2)>
    : public binary_function<_A1, _A2, _Rp>
{
};

template <class _Rp, class _Cp, class _A1>
struct __weak_result_type<_Rp (_Cp::*)(_A1)>
    : public binary_function<_Cp*, _A1, _Rp>
{
};

template <class _Rp, class _Cp, class _A1>
struct __weak_result_type<_Rp (_Cp::*)(_A1) const>
    : public binary_function<const _Cp*, _A1, _Rp>
{
};

template <class _Rp, class _Cp, class _A1>
struct __weak_result_type<_Rp (_Cp::*)(_A1) volatile>
    : public binary_function<volatile _Cp*, _A1, _Rp>
{
};

template <class _Rp, class _Cp, class _A1>
struct __weak_result_type<_Rp (_Cp::*)(_A1) const volatile>
    : public binary_function<const volatile _Cp*, _A1, _Rp>
{
};


#ifndef _LIBCPP_HAS_NO_VARIADICS
// 3 or more arguments

template <class _Rp, class _A1, class _A2, class _A3, class ..._A4>
struct __weak_result_type<_Rp (_A1, _A2, _A3, _A4...)>
{
    typedef _Rp result_type;
};

template <class _Rp, class _A1, class _A2, class _A3, class ..._A4>
struct __weak_result_type<_Rp (&)(_A1, _A2, _A3, _A4...)>
{
    typedef _Rp result_type;
};

template <class _Rp, class _A1, class _A2, class _A3, class ..._A4>
struct __weak_result_type<_Rp (*)(_A1, _A2, _A3, _A4...)>
{
    typedef _Rp result_type;
};

template <class _Rp, class _Cp, class _A1, class _A2, class ..._A3>
struct __weak_result_type<_Rp (_Cp::*)(_A1, _A2, _A3...)>
{
    typedef _Rp result_type;
};

template <class _Rp, class _Cp, class _A1, class _A2, class ..._A3>
struct __weak_result_type<_Rp (_Cp::*)(_A1, _A2, _A3...) const>
{
    typedef _Rp result_type;
};

template <class _Rp, class _Cp, class _A1, class _A2, class ..._A3>
struct __weak_result_type<_Rp (_Cp::*)(_A1, _A2, _A3...) volatile>
{
    typedef _Rp result_type;
};

template <class _Rp, class _Cp, class _A1, class _A2, class ..._A3>
struct __weak_result_type<_Rp (_Cp::*)(_A1, _A2, _A3...) const volatile>
{
    typedef _Rp result_type;
};

#endif // _LIBCPP_HAS_NO_VARIADICS

#ifndef _LIBCPP_CXX03_LANG

template <class _Tp, class ..._Args>
struct __invoke_return
{
    typedef decltype(__invoke(_VSTD::declval<_Tp>(), _VSTD::declval<_Args>()...)) type;
};

#else // _LIBCPP_HAS_NO_VARIADICS

#include <__functional_base_03>

#endif  // _LIBCPP_HAS_NO_VARIADICS


template <class _Ret>
struct __invoke_void_return_wrapper
{
#ifndef _LIBCPP_HAS_NO_VARIADICS
    template <class ..._Args>
    static _Ret __call(_Args&&... __args) {
        return __invoke(_VSTD::forward<_Args>(__args)...);
    }
#else
    template <class _Fn>
    static _Ret __call(_Fn __f) {
        return __invoke(__f);
    }

    template <class _Fn, class _A0>
    static _Ret __call(_Fn __f, _A0& __a0) {
        return __invoke(__f, __a0);
    }

    template <class _Fn, class _A0, class _A1>
    static _Ret __call(_Fn __f, _A0& __a0, _A1& __a1) {
        return __invoke(__f, __a0, __a1);
    }

    template <class _Fn, class _A0, class _A1, class _A2>
    static _Ret __call(_Fn __f, _A0& __a0, _A1& __a1, _A2& __a2){
        return __invoke(__f, __a0, __a1, __a2);
    }
#endif
};

template <>
struct __invoke_void_return_wrapper<void>
{
#ifndef _LIBCPP_HAS_NO_VARIADICS
    template <class ..._Args>
    static void __call(_Args&&... __args) {
        __invoke(_VSTD::forward<_Args>(__args)...);
    }
#else
    template <class _Fn>
    static void __call(_Fn __f) {
        __invoke(__f);
    }

    template <class _Fn, class _A0>
    static void __call(_Fn __f, _A0& __a0) {
        __invoke(__f, __a0);
    }

    template <class _Fn, class _A0, class _A1>
    static void __call(_Fn __f, _A0& __a0, _A1& __a1) {
        __invoke(__f, __a0, __a1);
    }

    template <class _Fn, class _A0, class _A1, class _A2>
    static void __call(_Fn __f, _A0& __a0, _A1& __a1, _A2& __a2) {
        __invoke(__f, __a0, __a1, __a2);
    }
#endif
};

template <class _Tp>
class _LIBCPP_TYPE_VIS_ONLY reference_wrapper
    : public __weak_result_type<_Tp>
{
public:
    // types
    typedef _Tp type;
private:
    type* __f_;

public:
    // construct/copy/destroy
    _LIBCPP_INLINE_VISIBILITY reference_wrapper(type& __f) _NOEXCEPT
        : __f_(_VSTD::addressof(__f)) {}
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    private: reference_wrapper(type&&); public: // = delete; // do not bind to temps
#endif

    // access
    _LIBCPP_INLINE_VISIBILITY operator type&    () const _NOEXCEPT {return *__f_;}
    _LIBCPP_INLINE_VISIBILITY          type& get() const _NOEXCEPT {return *__f_;}

#ifndef _LIBCPP_HAS_NO_VARIADICS
    // invoke
    template <class... _ArgTypes>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_of<type&, _ArgTypes...>::type
    operator() (_ArgTypes&&... __args) const {
        return __invoke(get(), _VSTD::forward<_ArgTypes>(__args)...);
    }
#else

    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return<type>::type
    operator() () const {
        return __invoke(get());
    }

    template <class _A0>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return0<type, _A0>::type
    operator() (_A0& __a0) const {
        return __invoke(get(), __a0);
    }

    template <class _A0>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return0<type, _A0 const>::type
    operator() (_A0 const& __a0) const {
        return __invoke(get(), __a0);
    }

    template <class _A0, class _A1>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return1<type, _A0, _A1>::type
    operator() (_A0& __a0, _A1& __a1) const {
        return __invoke(get(), __a0, __a1);
    }

    template <class _A0, class _A1>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return1<type, _A0 const, _A1>::type
    operator() (_A0 const& __a0, _A1& __a1) const {
        return __invoke(get(), __a0, __a1);
    }

    template <class _A0, class _A1>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return1<type, _A0, _A1 const>::type
    operator() (_A0& __a0, _A1 const& __a1) const {
        return __invoke(get(), __a0, __a1);
    }

    template <class _A0, class _A1>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return1<type, _A0 const, _A1 const>::type
    operator() (_A0 const& __a0, _A1 const& __a1) const {
        return __invoke(get(), __a0, __a1);
    }

    template <class _A0, class _A1, class _A2>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return2<type, _A0, _A1, _A2>::type
    operator() (_A0& __a0, _A1& __a1, _A2& __a2) const {
        return __invoke(get(), __a0, __a1, __a2);
    }

    template <class _A0, class _A1, class _A2>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return2<type, _A0 const, _A1, _A2>::type
    operator() (_A0 const& __a0, _A1& __a1, _A2& __a2) const {
        return __invoke(get(), __a0, __a1, __a2);
    }

    template <class _A0, class _A1, class _A2>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return2<type, _A0, _A1 const, _A2>::type
    operator() (_A0& __a0, _A1 const& __a1, _A2& __a2) const {
        return __invoke(get(), __a0, __a1, __a2);
    }

    template <class _A0, class _A1, class _A2>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return2<type, _A0, _A1, _A2 const>::type
    operator() (_A0& __a0, _A1& __a1, _A2 const& __a2) const {
        return __invoke(get(), __a0, __a1, __a2);
    }

    template <class _A0, class _A1, class _A2>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return2<type, _A0 const, _A1 const, _A2>::type
    operator() (_A0 const& __a0, _A1 const& __a1, _A2& __a2) const {
        return __invoke(get(), __a0, __a1, __a2);
    }

    template <class _A0, class _A1, class _A2>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return2<type, _A0 const, _A1, _A2 const>::type
    operator() (_A0 const& __a0, _A1& __a1, _A2 const& __a2) const {
        return __invoke(get(), __a0, __a1, __a2);
    }

    template <class _A0, class _A1, class _A2>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return2<type, _A0, _A1 const, _A2 const>::type
    operator() (_A0& __a0, _A1 const& __a1, _A2 const& __a2) const {
        return __invoke(get(), __a0, __a1, __a2);
    }

    template <class _A0, class _A1, class _A2>
    _LIBCPP_INLINE_VISIBILITY
    typename __invoke_return2<type, _A0 const, _A1 const, _A2 const>::type
    operator() (_A0 const& __a0, _A1 const& __a1, _A2 const& __a2) const {
        return __invoke(get(), __a0, __a1, __a2);
    }
#endif // _LIBCPP_HAS_NO_VARIADICS
};


template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
reference_wrapper<_Tp>
ref(_Tp& __t) _NOEXCEPT
{
    return reference_wrapper<_Tp>(__t);
}

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
reference_wrapper<_Tp>
ref(reference_wrapper<_Tp> __t) _NOEXCEPT
{
    return ref(__t.get());
}

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
reference_wrapper<const _Tp>
cref(const _Tp& __t) _NOEXCEPT
{
    return reference_wrapper<const _Tp>(__t);
}

template <class _Tp>
inline _LIBCPP_INLINE_VISIBILITY
reference_wrapper<const _Tp>
cref(reference_wrapper<_Tp> __t) _NOEXCEPT
{
    return cref(__t.get());
}

#ifndef _LIBCPP_HAS_NO_VARIADICS
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
#ifndef _LIBCPP_HAS_NO_DELETED_FUNCTIONS

template <class _Tp> void ref(const _Tp&&) = delete;
template <class _Tp> void cref(const _Tp&&) = delete;

#else  // _LIBCPP_HAS_NO_DELETED_FUNCTIONS

template <class _Tp> void ref(const _Tp&&);// = delete;
template <class _Tp> void cref(const _Tp&&);// = delete;

#endif  // _LIBCPP_HAS_NO_DELETED_FUNCTIONS

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

#endif  // _LIBCPP_HAS_NO_VARIADICS

#if _LIBCPP_STD_VER > 11
template <class _Tp1, class _Tp2 = void>
struct __is_transparent
{
private:
    struct __two {char __lx; char __lxx;};
    template <class _Up> static __two __test(...);
    template <class _Up> static char __test(typename _Up::is_transparent* = 0);
public:
    static const bool value = sizeof(__test<_Tp1>(0)) == 1;
};
#endif

_LIBCPP_END_NAMESPACE_STD

#endif  // __VTSS_BASICS___FUNCTIONAL_BASE_HXX__
