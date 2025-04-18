// -*- C++ -*-
//===--------------------------- __config ---------------------------------===//
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

#ifndef __VTSS_BASICS_LIBCXX_CONFIG_HXX__
#define __VTSS_BASICS_LIBCXX_CONFIG_HXX__

// Some vtss-specific configurations
#define _LIBCPP_BEGIN_NAMESPACE_STD namespace vtss {
#define _LIBCPP_END_NAMESPACE_STD  }
#define _VSTD vtss
#define _LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER

#ifdef __cplusplus

#ifdef __GNUC__
#define _GNUC_VER (__GNUC__ * 100 + __GNUC_MINOR__)
#else
#define _GNUC_VER 0
#endif

#define _LIBCPP_VERSION 3900

#ifndef _LIBCPP_ABI_VERSION
#define _LIBCPP_ABI_VERSION 1
#endif

#if defined(_LIBCPP_ABI_UNSTABLE) || _LIBCPP_ABI_VERSION >= 2
// Change short string represention so that string data starts at offset 0,
// improving its alignment in some cases.
#define _LIBCPP_ABI_ALTERNATE_STRING_LAYOUT
// Fix deque iterator type in order to support incomplete types.
#define _LIBCPP_ABI_INCOMPLETE_TYPES_IN_DEQUE
// Fix undefined behavior in how std::list stores it's linked nodes.
#define _LIBCPP_ABI_LIST_REMOVE_NODE_POINTER_UB
// Fix undefined behavior in  how __tree stores its end and parent nodes.
#define _LIBCPP_ABI_TREE_REMOVE_NODE_POINTER_UB
#define _LIBCPP_ABI_FORWARD_LIST_REMOVE_NODE_POINTER_UB
#define _LIBCPP_ABI_FIX_UNORDERED_CONTAINER_SIZE_TYPE
#define _LIBCPP_ABI_VARIADIC_LOCK_GUARD
#elif _LIBCPP_ABI_VERSION == 1
// Feature macros for disabling pre ABI v1 features. All of these options
// are deprecated.
#if defined(__FreeBSD__)
#define _LIBCPP_DEPRECATED_ABI_DISABLE_PAIR_TRIVIAL_COPY_CTOR
#endif
#endif

#ifdef _LIBCPP_TRIVIAL_PAIR_COPY_CTOR
#error "_LIBCPP_TRIVIAL_PAIR_COPY_CTOR" is no longer supported. \
       use _LIBCPP_DEPRECATED_ABI_DISABLE_PAIR_TRIVIAL_COPY_CTOR instead
#endif

#define _LIBCPP_CONCAT1(_LIBCPP_X,_LIBCPP_Y) _LIBCPP_X##_LIBCPP_Y
#define _LIBCPP_CONCAT(_LIBCPP_X,_LIBCPP_Y) _LIBCPP_CONCAT1(_LIBCPP_X,_LIBCPP_Y)

#define _LIBCPP_NAMESPACE _LIBCPP_CONCAT(__,_LIBCPP_ABI_VERSION)


#ifndef __has_attribute
#define __has_attribute(__x) 0
#endif
#ifndef __has_builtin
#define __has_builtin(__x) 0
#endif
#ifndef __has_extension
#define __has_extension(__x) 0
#endif
#ifndef __has_feature
#define __has_feature(__x) 0
#endif
// '__is_identifier' returns '0' if '__x' is a reserved identifier provided by
// the compiler and '1' otherwise.
#ifndef __is_identifier
#define __is_identifier(__x) 1
#endif


#ifdef __LITTLE_ENDIAN__
#if __LITTLE_ENDIAN__
#define _LIBCPP_LITTLE_ENDIAN 1
#define _LIBCPP_BIG_ENDIAN    0
#endif  // __LITTLE_ENDIAN__
#endif  // __LITTLE_ENDIAN__

#ifdef __BIG_ENDIAN__
#if __BIG_ENDIAN__
#define _LIBCPP_LITTLE_ENDIAN 0
#define _LIBCPP_BIG_ENDIAN    1
#endif  // __BIG_ENDIAN__
#endif  // __BIG_ENDIAN__

#ifdef __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define _LIBCPP_LITTLE_ENDIAN 1
#define _LIBCPP_BIG_ENDIAN 0
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define _LIBCPP_LITTLE_ENDIAN 0
#define _LIBCPP_BIG_ENDIAN 1
#endif // __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#endif // __BYTE_ORDER__

#ifdef __FreeBSD__
# include <sys/endian.h>
#  if _BYTE_ORDER == _LITTLE_ENDIAN
#   define _LIBCPP_LITTLE_ENDIAN 1
#   define _LIBCPP_BIG_ENDIAN    0
# else  // _BYTE_ORDER == _LITTLE_ENDIAN
#   define _LIBCPP_LITTLE_ENDIAN 0
#   define _LIBCPP_BIG_ENDIAN    1
# endif  // _BYTE_ORDER == _LITTLE_ENDIAN
# ifndef __LONG_LONG_SUPPORTED
#  define _LIBCPP_HAS_NO_LONG_LONG
# endif  // __LONG_LONG_SUPPORTED
#endif  // __FreeBSD__

#ifdef __NetBSD__
# include <sys/endian.h>
#  if _BYTE_ORDER == _LITTLE_ENDIAN
#   define _LIBCPP_LITTLE_ENDIAN 1
#   define _LIBCPP_BIG_ENDIAN    0
# else  // _BYTE_ORDER == _LITTLE_ENDIAN
#   define _LIBCPP_LITTLE_ENDIAN 0
#   define _LIBCPP_BIG_ENDIAN    1
# endif  // _BYTE_ORDER == _LITTLE_ENDIAN
# define _LIBCPP_HAS_QUICK_EXIT
#endif  // __NetBSD__

#ifdef _WIN32
#  define _LIBCPP_LITTLE_ENDIAN 1
#  define _LIBCPP_BIG_ENDIAN    0
// Compiler intrinsics (MSVC)
#if defined(_MSC_VER) && _MSC_VER >= 1400
#    define _LIBCPP_HAS_IS_BASE_OF
#  endif
#  if defined(_MSC_VER) && !defined(__clang__)
#    define _LIBCPP_MSVC // Using Microsoft Visual C++ compiler
#    define _LIBCPP_TOSTRING2(x) #x
#    define _LIBCPP_TOSTRING(x) _LIBCPP_TOSTRING2(x)
#    define _LIBCPP_WARNING(x) __pragma(message(__FILE__ "(" _LIBCPP_TOSTRING(__LINE__) ") : warning note: " x))
#  endif
#  // If mingw not explicitly detected, assume using MS C runtime only.
#  ifndef __MINGW32__
#    define _LIBCPP_MSVCRT // Using Microsoft's C Runtime library
#  endif
#endif  // _WIN32

#ifdef __sun__
# include <sys/isa_defs.h>
# ifdef _LITTLE_ENDIAN
#   define _LIBCPP_LITTLE_ENDIAN 1
#   define _LIBCPP_BIG_ENDIAN    0
# else
#   define _LIBCPP_LITTLE_ENDIAN 0
#   define _LIBCPP_BIG_ENDIAN    1
# endif
#endif // __sun__

#if defined(__CloudABI__)
  // Certain architectures provide arc4random(). Prefer using
  // arc4random() over /dev/{u,}random to make it possible to obtain
  // random data even when using sandboxing mechanisms such as chroots,
  // Capsicum, etc.
# define _LIBCPP_USING_ARC4_RANDOM
#elif defined(__native_client__)
  // NaCl's sandbox (which PNaCl also runs in) doesn't allow filesystem access,
  // including accesses to the special files under /dev. C++11's
  // std::random_device is instead exposed through a NaCl syscall.
# define _LIBCPP_USING_NACL_RANDOM
#elif defined(_WIN32)
# define _LIBCPP_USING_WIN32_RANDOM
#else
# define _LIBCPP_USING_DEV_RANDOM
#endif

#if !defined(_LIBCPP_LITTLE_ENDIAN) || !defined(_LIBCPP_BIG_ENDIAN)
# include <endian.h>
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define _LIBCPP_LITTLE_ENDIAN 1
#  define _LIBCPP_BIG_ENDIAN    0
# elif __BYTE_ORDER == __BIG_ENDIAN
#  define _LIBCPP_LITTLE_ENDIAN 0
#  define _LIBCPP_BIG_ENDIAN    1
# else  // __BYTE_ORDER == __BIG_ENDIAN
#  error unable to determine endian
# endif
#endif  // !defined(_LIBCPP_LITTLE_ENDIAN) || !defined(_LIBCPP_BIG_ENDIAN)

#if __has_attribute(__no_sanitize__) && !defined(__GNUC__)
#  define _LIBCPP_NO_CFI __attribute__((__no_sanitize__("cfi")))
#else
#  define _LIBCPP_NO_CFI
#endif

#ifdef _WIN32

// only really useful for a DLL
#ifdef _LIBCPP_DLL // this should be a compiler builtin define ideally...
# ifdef cxx_EXPORTS
#  define _LIBCPP_HIDDEN
#  define _LIBCPP_FUNC_VIS __declspec(dllexport)
#  define _LIBCPP_TYPE_VIS __declspec(dllexport)
# else
#  define _LIBCPP_HIDDEN
#  define _LIBCPP_FUNC_VIS __declspec(dllimport)
#  define _LIBCPP_TYPE_VIS __declspec(dllimport)
# endif
#else
# define _LIBCPP_HIDDEN
# define _LIBCPP_FUNC_VIS
# define _LIBCPP_TYPE_VIS
#endif

#define _LIBCPP_TYPE_VIS_ONLY
#define _LIBCPP_FUNC_VIS_ONLY

#ifndef _LIBCPP_INLINE_VISIBILITY
# ifdef _LIBCPP_MSVC
#  define _LIBCPP_INLINE_VISIBILITY __forceinline
# else // MinGW GCC and Clang
#  define _LIBCPP_INLINE_VISIBILITY __attribute__ ((__always_inline__))
# endif
#endif

#ifndef _LIBCPP_EXCEPTION_ABI
#define _LIBCPP_EXCEPTION_ABI _LIBCPP_TYPE_VIS
#endif

#ifndef _LIBCPP_ALWAYS_INLINE
# ifdef _LIBCPP_MSVC
#  define _LIBCPP_ALWAYS_INLINE __forceinline
# endif
#endif

#endif // _WIN32

#ifndef _LIBCPP_HIDDEN
#define _LIBCPP_HIDDEN __attribute__ ((__visibility__("hidden")))
#endif

#ifndef _LIBCPP_FUNC_VIS
#define _LIBCPP_FUNC_VIS __attribute__ ((__visibility__("default")))
#endif

#ifndef _LIBCPP_TYPE_VIS
#  if __has_attribute(__type_visibility__)
#    define _LIBCPP_TYPE_VIS __attribute__ ((__type_visibility__("default")))
#  else
#    define _LIBCPP_TYPE_VIS __attribute__ ((__visibility__("default")))
#  endif
#endif

#ifndef _LIBCPP_PREFERRED_OVERLOAD
#  if __has_attribute(__enable_if__)
#    define _LIBCPP_PREFERRED_OVERLOAD __attribute__ ((__enable_if__(true, "")))
#  endif
#endif

#ifndef _LIBCPP_TYPE_VIS_ONLY
# define _LIBCPP_TYPE_VIS_ONLY _LIBCPP_TYPE_VIS
#endif

#ifndef _LIBCPP_FUNC_VIS_ONLY
# define _LIBCPP_FUNC_VIS_ONLY _LIBCPP_FUNC_VIS
#endif

#ifndef _LIBCPP_INLINE_VISIBILITY
#define _LIBCPP_INLINE_VISIBILITY __attribute__ ((__visibility__("hidden"), __always_inline__))
#endif

#ifndef _LIBCPP_EXCEPTION_ABI
#define _LIBCPP_EXCEPTION_ABI __attribute__ ((__visibility__("default")))
#endif

#ifndef _LIBCPP_ALWAYS_INLINE
#define _LIBCPP_ALWAYS_INLINE  __attribute__ ((__visibility__("hidden"), __always_inline__))
#endif

#if defined(__clang__)

// _LIBCPP_ALTERNATE_STRING_LAYOUT is an old name for
// _LIBCPP_ABI_ALTERNATE_STRING_LAYOUT left here for backward compatibility.
#if (defined(__APPLE__) && !defined(__i386__) && !defined(__x86_64__) &&       \
     !defined(__arm__)) ||                                                     \
    defined(_LIBCPP_ALTERNATE_STRING_LAYOUT)
#define _LIBCPP_ABI_ALTERNATE_STRING_LAYOUT
#endif

#if __has_feature(cxx_alignas)
#  define _ALIGNAS_TYPE(x) alignas(x)
#  define _ALIGNAS(x) alignas(x)
#else
#  define _ALIGNAS_TYPE(x) __attribute__((__aligned__(__alignof(x))))
#  define _ALIGNAS(x) __attribute__((__aligned__(x)))
#endif

#if !__has_feature(cxx_alias_templates)
#define _LIBCPP_HAS_NO_TEMPLATE_ALIASES
#endif

#if __cplusplus < 201103L
typedef __char16_t char16_t;
typedef __char32_t char32_t;
#endif

#define _LIBCPP_NO_EXCEPTIONS
#define _LIBCPP_NO_RTTI

#if !(__has_feature(cxx_strong_enums))
#define _LIBCPP_HAS_NO_STRONG_ENUMS
#endif

#if !(__has_feature(cxx_decltype))
#define _LIBCPP_HAS_NO_DECLTYPE
#endif

#if __has_feature(cxx_attributes)
#  define _LIBCPP_NORETURN [[noreturn]]
#else
#  define _LIBCPP_NORETURN __attribute__ ((noreturn))
#endif

#if !(__has_feature(cxx_default_function_template_args))
#define _LIBCPP_HAS_NO_DEFAULT_FUNCTION_TEMPLATE_ARGS
#endif

#if !(__has_feature(cxx_defaulted_functions))
#define _LIBCPP_HAS_NO_DEFAULTED_FUNCTIONS
#endif  // !(__has_feature(cxx_defaulted_functions))

#if !(__has_feature(cxx_deleted_functions))
#define _LIBCPP_HAS_NO_DELETED_FUNCTIONS
#endif  // !(__has_feature(cxx_deleted_functions))

#if !(__has_feature(cxx_lambdas))
#define _LIBCPP_HAS_NO_LAMBDAS
#endif

#if !(__has_feature(cxx_nullptr))
#define _LIBCPP_HAS_NO_NULLPTR
#endif

#if !(__has_feature(cxx_rvalue_references))
#define _LIBCPP_HAS_NO_RVALUE_REFERENCES
#endif

#if !(__has_feature(cxx_static_assert))
#define _LIBCPP_HAS_NO_STATIC_ASSERT
#endif

#if !(__has_feature(cxx_auto_type))
#define _LIBCPP_HAS_NO_AUTO_TYPE
#endif

#if !(__has_feature(cxx_access_control_sfinae)) || !__has_feature(cxx_trailing_return)
#define _LIBCPP_HAS_NO_ADVANCED_SFINAE
#endif

#if !(__has_feature(cxx_variadic_templates))
#define _LIBCPP_HAS_NO_VARIADICS
#endif

#if !(__has_feature(cxx_trailing_return))
#define _LIBCPP_HAS_NO_TRAILING_RETURN
#endif

#if !(__has_feature(cxx_generalized_initializers))
#define _LIBCPP_HAS_NO_GENERALIZED_INITIALIZERS
#endif

#if __has_feature(is_base_of)
#  define _LIBCPP_HAS_IS_BASE_OF
#endif

#if __has_feature(is_final)
#  define _LIBCPP_HAS_IS_FINAL
#endif

// Objective-C++ features (opt-in)
#if __has_feature(objc_arc)
#define _LIBCPP_HAS_OBJC_ARC
#endif

#if __has_feature(objc_arc_weak)
#define _LIBCPP_HAS_OBJC_ARC_WEAK
#define _LIBCPP_HAS_NO_STRONG_ENUMS
#endif

#if !(__has_feature(cxx_constexpr))
#define _LIBCPP_HAS_NO_CONSTEXPR
#endif

#if !(__has_feature(cxx_relaxed_constexpr))
#define _LIBCPP_HAS_NO_CXX14_CONSTEXPR
#endif

#if !(__has_feature(cxx_variable_templates))
#define _LIBCPP_HAS_NO_VARIABLE_TEMPLATES
#endif

#if __ISO_C_VISIBLE >= 2011 || __cplusplus >= 201103L
#if defined(__FreeBSD__)
#define _LIBCPP_HAS_QUICK_EXIT
#define _LIBCPP_HAS_C11_FEATURES
#elif defined(__ANDROID__)
#define _LIBCPP_HAS_QUICK_EXIT
#elif defined(__linux__)
#if !defined(_LIBCPP_HAS_MUSL_LIBC)
# include <features.h>
#if __GLIBC_PREREQ(2, 15)
#define _LIBCPP_HAS_QUICK_EXIT
#endif
#if __GLIBC_PREREQ(2, 17)
#define _LIBCPP_HAS_C11_FEATURES
#endif
#else // defined(_LIBCPP_HAS_MUSL_LIBC)
#define _LIBCPP_HAS_QUICK_EXIT
#define _LIBCPP_HAS_C11_FEATURES
#endif
#endif // __linux__
#endif

#if !(__has_feature(cxx_noexcept))
#define _LIBCPP_HAS_NO_NOEXCEPT
#endif

#if __has_feature(underlying_type)
#  define _LIBCPP_UNDERLYING_TYPE(T) __underlying_type(T)
#endif

#if __has_feature(is_literal)
#  define _LIBCPP_IS_LITERAL(T) __is_literal(T)
#endif

namespace std {
  inline namespace _LIBCPP_NAMESPACE {
  }
}

#if !defined(_LIBCPP_HAS_NO_ASAN) && !__has_feature(address_sanitizer)
#define _LIBCPP_HAS_NO_ASAN
#endif

// Allow for build-time disabling of unsigned integer sanitization
#if !defined(_LIBCPP_DISABLE_UBSAN_UNSIGNED_INTEGER_CHECK) && __has_attribute(no_sanitize)
#define _LIBCPP_DISABLE_UBSAN_UNSIGNED_INTEGER_CHECK __attribute__((__no_sanitize__("unsigned-integer-overflow")))
#endif 

#elif defined(__GNUC__)

#define _ALIGNAS(x) __attribute__((__aligned__(x)))
#define _ALIGNAS_TYPE(x) __attribute__((__aligned__(__alignof(x))))

#define _LIBCPP_NORETURN __attribute__((noreturn))

#if _GNUC_VER >= 407
#define _LIBCPP_UNDERLYING_TYPE(T) __underlying_type(T)
#define _LIBCPP_IS_LITERAL(T) __is_literal_type(T)
#define _LIBCPP_HAS_IS_FINAL
#endif

#if defined(__GNUC__) && _GNUC_VER >= 403
#  define _LIBCPP_HAS_IS_BASE_OF
#endif

#if !__EXCEPTIONS
#define _LIBCPP_NO_EXCEPTIONS
#endif

// constexpr was added to GCC in 4.6.
#if _GNUC_VER < 406
#define _LIBCPP_HAS_NO_CONSTEXPR
// Can only use constexpr in c++11 mode.
#elif !defined(__GXX_EXPERIMENTAL_CXX0X__) && __cplusplus < 201103L
#define _LIBCPP_HAS_NO_CONSTEXPR
#endif

// Determine if GCC supports relaxed constexpr
#if !defined(__cpp_constexpr) || __cpp_constexpr < 201304L
#define _LIBCPP_HAS_NO_CXX14_CONSTEXPR
#endif

// GCC 5 will support variable templates
#if !defined(__cpp_variable_templates) || __cpp_variable_templates < 201304L
#define _LIBCPP_HAS_NO_VARIABLE_TEMPLATES
#endif

#ifndef __GXX_EXPERIMENTAL_CXX0X__

#define _LIBCPP_HAS_NO_ADVANCED_SFINAE
#define _LIBCPP_HAS_NO_DECLTYPE
#define _LIBCPP_HAS_NO_DEFAULT_FUNCTION_TEMPLATE_ARGS
#define _LIBCPP_HAS_NO_DEFAULTED_FUNCTIONS
#define _LIBCPP_HAS_NO_DELETED_FUNCTIONS
#define _LIBCPP_HAS_NO_NULLPTR
#define _LIBCPP_HAS_NO_STATIC_ASSERT
#define _LIBCPP_HAS_NO_UNICODE_CHARS
#define _LIBCPP_HAS_NO_VARIADICS
#define _LIBCPP_HAS_NO_RVALUE_REFERENCES
#define _LIBCPP_HAS_NO_STRONG_ENUMS
#define _LIBCPP_HAS_NO_TEMPLATE_ALIASES
#define _LIBCPP_HAS_NO_NOEXCEPT

#else  // __GXX_EXPERIMENTAL_CXX0X__

#if _GNUC_VER < 403
#define _LIBCPP_HAS_NO_DEFAULT_FUNCTION_TEMPLATE_ARGS
#define _LIBCPP_HAS_NO_RVALUE_REFERENCES
#define _LIBCPP_HAS_NO_STATIC_ASSERT
#endif


#if _GNUC_VER < 404
#define _LIBCPP_HAS_NO_DECLTYPE
#define _LIBCPP_HAS_NO_DELETED_FUNCTIONS
#define _LIBCPP_HAS_NO_TRAILING_RETURN
#define _LIBCPP_HAS_NO_UNICODE_CHARS
#define _LIBCPP_HAS_NO_VARIADICS
#define _LIBCPP_HAS_NO_GENERALIZED_INITIALIZERS
#endif  // _GNUC_VER < 404

#if _GNUC_VER < 406
#define _LIBCPP_HAS_NO_NOEXCEPT
#define _LIBCPP_HAS_NO_NULLPTR
#define _LIBCPP_HAS_NO_TEMPLATE_ALIASES
#endif

#if _GNUC_VER < 407
#define _LIBCPP_HAS_NO_ADVANCED_SFINAE
#define _LIBCPP_HAS_NO_DEFAULTED_FUNCTIONS
#endif

#endif  // __GXX_EXPERIMENTAL_CXX0X__

#if !defined(_LIBCPP_HAS_NO_ASAN) && !defined(__SANITIZE_ADDRESS__)
#define _LIBCPP_HAS_NO_ASAN
#endif

#elif defined(_LIBCPP_MSVC)

#define _LIBCPP_HAS_NO_TEMPLATE_ALIASES
#define _LIBCPP_HAS_NO_CONSTEXPR
#define _LIBCPP_HAS_NO_CXX14_CONSTEXPR
#define _LIBCPP_HAS_NO_VARIABLE_TEMPLATES
#define _LIBCPP_HAS_NO_UNICODE_CHARS
#define _LIBCPP_HAS_NO_DELETED_FUNCTIONS
#define _LIBCPP_HAS_NO_DEFAULTED_FUNCTIONS
#define _LIBCPP_HAS_NO_NOEXCEPT
#define __alignof__ __alignof
#define _LIBCPP_NORETURN __declspec(noreturn)
#define _ALIGNAS(x) __declspec(align(x))
#define _LIBCPP_HAS_NO_VARIADICS


#  define _LIBCPP_WEAK
namespace std {
}

#define _LIBCPP_HAS_NO_ASAN

#elif defined(__IBMCPP__)

#define _ALIGNAS(x) __attribute__((__aligned__(x)))
#define _ALIGNAS_TYPE(x) __attribute__((__aligned__(__alignof(x))))
#define _ATTRIBUTE(x) __attribute__((x))
#define _LIBCPP_NORETURN __attribute__((noreturn))

#define _LIBCPP_HAS_NO_DEFAULT_FUNCTION_TEMPLATE_ARGS
#define _LIBCPP_HAS_NO_TEMPLATE_ALIASES
#define _LIBCPP_HAS_NO_ADVANCED_SFINAE
#define _LIBCPP_HAS_NO_GENERALIZED_INITIALIZERS
#define _LIBCPP_HAS_NO_NOEXCEPT
#define _LIBCPP_HAS_NO_NULLPTR
#define _LIBCPP_HAS_NO_UNICODE_CHARS
#define _LIBCPP_HAS_IS_BASE_OF
#define _LIBCPP_HAS_IS_FINAL
#define _LIBCPP_HAS_NO_VARIABLE_TEMPLATES

#if defined(_AIX)
#define __MULTILOCALE_API
#endif


namespace std {
  inline namespace _LIBCPP_NAMESPACE {
  }
}

#define _LIBCPP_HAS_NO_ASAN

#endif // __clang__ || __GNUC__ || _MSC_VER || __IBMCPP__

#ifdef __GNUC__
#  if _GNUC_VER >= 800
#    define _LIBCPP_CONSTEXPR_VOID_PTR  (size_t)
#  else
#    define _LIBCPP_CONSTEXPR_VOID_PTR
#  endif
#else
#  define _LIBCPP_CONSTEXPR_VOID_PTR
#endif

#ifndef _LIBCPP_HAS_NO_NOEXCEPT
#  define _NOEXCEPT noexcept
#  define _NOEXCEPT_(x) noexcept(x)
#else
#  define _NOEXCEPT throw()
#  define _NOEXCEPT_(x)
#endif

#ifdef _LIBCPP_HAS_NO_UNICODE_CHARS
typedef unsigned short char16_t;
typedef unsigned int   char32_t;
#endif  // _LIBCPP_HAS_NO_UNICODE_CHARS

#ifndef __SIZEOF_INT128__
#define _LIBCPP_HAS_NO_INT128
#endif

#ifdef _LIBCPP_HAS_NO_STATIC_ASSERT

extern "C++" {
template <bool> struct __static_assert_test;
template <> struct __static_assert_test<true> {};
template <unsigned> struct __static_assert_check {};
}
#define static_assert(__b, __m) \
    typedef __static_assert_check<sizeof(__static_assert_test<(__b)>)> \
    _LIBCPP_CONCAT(__t, __LINE__)

#endif  // _LIBCPP_HAS_NO_STATIC_ASSERT

#ifdef _LIBCPP_HAS_NO_DECLTYPE
// GCC 4.6 provides __decltype in all standard modes.
#if !__is_identifier(__decltype) || _GNUC_VER >= 406
#  define decltype(__x) __decltype(__x)
#else
#  define decltype(__x) __typeof__(__x)
#endif
#endif

#ifdef _LIBCPP_HAS_NO_CONSTEXPR
#define _LIBCPP_CONSTEXPR
#else
#define _LIBCPP_CONSTEXPR constexpr
#endif

#ifdef _LIBCPP_HAS_NO_DEFAULTED_FUNCTIONS
#define _LIBCPP_DEFAULT {}
#else
#define _LIBCPP_DEFAULT = default;
#endif

#ifdef _LIBCPP_HAS_NO_DELETED_FUNCTIONS
#define _LIBCPP_EQUAL_DELETE
#else
#define _LIBCPP_EQUAL_DELETE = delete
#endif

#ifdef __GNUC__
#define _NOALIAS __attribute__((__malloc__))
#else
#define _NOALIAS
#endif

#if __has_feature(cxx_explicit_conversions) || defined(__IBMCPP__)
#   define _LIBCPP_EXPLICIT explicit
#else
#   define _LIBCPP_EXPLICIT
#endif

#if !__has_builtin(__builtin_operator_new) || !__has_builtin(__builtin_operator_delete)
#   define _LIBCPP_HAS_NO_BUILTIN_OPERATOR_NEW_DELETE
#endif

#ifdef _LIBCPP_HAS_NO_STRONG_ENUMS
#define _LIBCPP_DECLARE_STRONG_ENUM(x) struct _LIBCPP_TYPE_VIS x { enum __lx
#define _LIBCPP_DECLARE_STRONG_ENUM_EPILOG(x) \
    __lx __v_; \
    _LIBCPP_ALWAYS_INLINE x(__lx __v) : __v_(__v) {} \
    _LIBCPP_ALWAYS_INLINE explicit x(int __v) : __v_(static_cast<__lx>(__v)) {} \
    _LIBCPP_ALWAYS_INLINE operator int() const {return __v_;} \
    };
#else  // _LIBCPP_HAS_NO_STRONG_ENUMS
#define _LIBCPP_DECLARE_STRONG_ENUM(x) enum class _LIBCPP_TYPE_VIS x
#define _LIBCPP_DECLARE_STRONG_ENUM_EPILOG(x)
#endif  // _LIBCPP_HAS_NO_STRONG_ENUMS

#ifdef _LIBCPP_DEBUG
#   if _LIBCPP_DEBUG == 0
#       define _LIBCPP_DEBUG_LEVEL 1
#   elif _LIBCPP_DEBUG == 1
#       define _LIBCPP_DEBUG_LEVEL 2
#   else
#       error Supported values for _LIBCPP_DEBUG are 0 and 1
#   endif
#   define _LIBCPP_EXTERN_TEMPLATE(...)
#endif

#ifndef _LIBCPP_EXTERN_TEMPLATE
#define _LIBCPP_EXTERN_TEMPLATE(...) extern template __VA_ARGS__;
#endif

#ifndef _LIBCPP_EXTERN_TEMPLATE2
#define _LIBCPP_EXTERN_TEMPLATE2(...) extern template __VA_ARGS__;
#endif

#if defined(__APPLE__) && defined(__LP64__) && !defined(__x86_64__)
#define _LIBCPP_NONUNIQUE_RTTI_BIT (1ULL << 63)
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(_WIN32) || \
    defined(__sun__) || defined(__NetBSD__) || defined(__CloudABI__)
#define _LIBCPP_LOCALE__L_EXTENSIONS 1
#endif

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
// Most unix variants have catopen.  These are the specific ones that don't.
#if !defined(_WIN32) && !defined(__ANDROID__) && !defined(_NEWLIB_VERSION)
#define _LIBCPP_HAS_CATOPEN 1
#endif
#endif

#ifdef __FreeBSD__
#define _DECLARE_C99_LDBL_MATH 1
#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
#define _LIBCPP_HAS_DEFAULTRUNELOCALE
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__sun__)
#define _LIBCPP_WCTYPE_IS_MASK
#endif

#ifndef _LIBCPP_STD_VER
#  if  __cplusplus <= 201103L
#    define _LIBCPP_STD_VER 11
#  elif __cplusplus <= 201402L
#    define _LIBCPP_STD_VER 14
#  else
#    define _LIBCPP_STD_VER 16  // current year, or date of c++17 ratification
#  endif
#endif  // _LIBCPP_STD_VER

#if _LIBCPP_STD_VER > 11
#define _LIBCPP_DEPRECATED [[deprecated]]
#else
#define _LIBCPP_DEPRECATED
#endif

#if _LIBCPP_STD_VER <= 11
#define _LIBCPP_EXPLICIT_AFTER_CXX11
#define _LIBCPP_DEPRECATED_AFTER_CXX11
#else
#define _LIBCPP_EXPLICIT_AFTER_CXX11 explicit
#define _LIBCPP_DEPRECATED_AFTER_CXX11 [[deprecated]]
#endif

#if _LIBCPP_STD_VER > 11 && !defined(_LIBCPP_HAS_NO_CXX14_CONSTEXPR)
#define _LIBCPP_CONSTEXPR_AFTER_CXX11 constexpr
#else
#define _LIBCPP_CONSTEXPR_AFTER_CXX11
#endif

#if _LIBCPP_STD_VER > 14 && !defined(_LIBCPP_HAS_NO_CXX14_CONSTEXPR)
#define _LIBCPP_CONSTEXPR_AFTER_CXX14 constexpr
#else
#define _LIBCPP_CONSTEXPR_AFTER_CXX14
#endif

#ifdef _LIBCPP_HAS_NO_RVALUE_REFERENCES
#  define _LIBCPP_EXPLICIT_MOVE(x) _VSTD::move(x)
#else
#  define _LIBCPP_EXPLICIT_MOVE(x) (x)
#endif

#ifndef _LIBCPP_HAS_NO_ASAN
extern "C" void __sanitizer_annotate_contiguous_container(
  const void *, const void *, const void *, const void *);
#endif

#ifndef _LIBCPP_WEAK
#  define _LIBCPP_WEAK __attribute__((__weak__))
#endif

// Thread API
#if !defined(_LIBCPP_HAS_NO_THREADS) && !defined(_LIBCPP_HAS_THREAD_API_PTHREAD)
# if defined(__FreeBSD__) || \
    defined(__NetBSD__) || \
    defined(__linux__) || \
    defined(__APPLE__) || \
    defined(__CloudABI__) || \
    defined(__sun__)
#  define _LIBCPP_HAS_THREAD_API_PTHREAD
# else
#  error "No thread API"
# endif // _LIBCPP_HAS_THREAD_API
#endif // _LIBCPP_HAS_NO_THREADS

#if defined(_LIBCPP_HAS_NO_THREADS) && defined(_LIBCPP_HAS_THREAD_API_PTHREAD)
#  error _LIBCPP_HAS_THREAD_API_PTHREAD may only be defined when \
         _LIBCPP_HAS_NO_THREADS is not defined.
#endif

#if defined(_LIBCPP_HAS_NO_MONOTONIC_CLOCK) && !defined(_LIBCPP_HAS_NO_THREADS)
#  error _LIBCPP_HAS_NO_MONOTONIC_CLOCK may only be defined when \
         _LIBCPP_HAS_NO_THREADS is defined.
#endif

// Systems that use capability-based security (FreeBSD with Capsicum,
// Nuxi CloudABI) may only provide local filesystem access (using *at()).
// Functions like open(), rename(), unlink() and stat() should not be
// used, as they attempt to access the global filesystem namespace.
#ifdef __CloudABI__
#define _LIBCPP_HAS_NO_GLOBAL_FILESYSTEM_NAMESPACE
#endif

// CloudABI is intended for running networked services. Processes do not
// have standard input and output channels.
#ifdef __CloudABI__
#define _LIBCPP_HAS_NO_STDIN
#define _LIBCPP_HAS_NO_STDOUT
#endif

#if defined(__ANDROID__) || defined(__CloudABI__) || defined(_LIBCPP_HAS_MUSL_LIBC)
#define _LIBCPP_PROVIDES_DEFAULT_RUNE_TABLE
#endif

// Thread-unsafe functions such as strtok(), mbtowc() and localtime()
// are not available.
#ifdef __CloudABI__
#define _LIBCPP_HAS_NO_THREAD_UNSAFE_C_FUNCTIONS
#endif

#if __has_feature(cxx_atomic) || __has_extension(c_atomic)
#define _LIBCPP_HAS_C_ATOMIC_IMP
#elif _GNUC_VER > 407
#define _LIBCPP_HAS_GCC_ATOMIC_IMP
#endif

#if (!defined(_LIBCPP_HAS_C_ATOMIC_IMP) && !defined(_LIBCPP_HAS_GCC_ATOMIC_IMP)) \
     || defined(_LIBCPP_HAS_NO_THREADS)
#define _LIBCPP_HAS_NO_ATOMIC_HEADER
#endif

#ifndef _LIBCPP_DISABLE_UBSAN_UNSIGNED_INTEGER_CHECK
#define _LIBCPP_DISABLE_UBSAN_UNSIGNED_INTEGER_CHECK
#endif

#if __cplusplus < 201103L
#define _LIBCPP_CXX03_LANG
#else
#if defined(_LIBCPP_HAS_NO_VARIADIC_TEMPLATES) || defined(_LIBCPP_HAS_NO_RVALUE_REFERENCES)
#error Libc++ requires a feature complete C++11 compiler in C++11 or greater.
#endif
#endif

#if (defined(_LIBCPP_ENABLE_THREAD_SAFETY_ANNOTATIONS) && defined(__clang__) \
      && __has_attribute(acquire_capability))
#define _LIBCPP_HAS_THREAD_SAFETY_ANNOTATIONS
#endif

#endif // __cplusplus


#endif  // __VTSS_BASICS_LIBCXX_CONFIG_HXX__
