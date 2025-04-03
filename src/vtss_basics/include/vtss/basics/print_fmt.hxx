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

#ifndef __VTSS_BASICS_PRINT_FMT_HXX__
#define __VTSS_BASICS_PRINT_FMT_HXX__

#include <cstdint>
#include "vtss/basics/api_types.h"
#include "vtss/basics/type_traits.hxx"

namespace vtss {

struct ostream;
struct str;
struct Buf;


/**
 * When adding a new fmt function, Fmt contains the data on how to format
 * the type. Each type can have one or multiple encoding_flags. Also it is
 * possible to extend Fmt with other variables but then the function
 * parse_flags needs to be updated.
 */
struct Fmt {
    /**
     * Mapping of the printf flags to the structure Fmt
     * %0+3d -> encoding_flag = d
     *       -> signed_flag   = +
     * 0 and 3 are part from a global parser
     */
    char encoding_flag;       // d, i, o, x, X, c, s, p
    char signed_flag = '\0';  // ' ', +
    size_t precision = {0};
};

size_t fmt(ostream &stream, const Fmt &fmt, const unsigned char *p);
size_t fmt(ostream &stream, const Fmt &fmt, const unsigned short *p);
size_t fmt(ostream &stream, const Fmt &fmt, const unsigned int *p);
size_t fmt(ostream &stream, const Fmt &fmt, const unsigned long *p);
size_t fmt(ostream &stream, const Fmt &fmt, const unsigned long long int *p);
size_t fmt(ostream &stream, const Fmt &fmt, const signed char *p);
size_t fmt(ostream &stream, const Fmt &fmt, const signed short *p);
size_t fmt(ostream &stream, const Fmt &fmt, const signed int *p);
size_t fmt(ostream &stream, const Fmt &fmt, const signed long *p);
size_t fmt(ostream &stream, const Fmt &fmt, const signed long long int *p);
size_t fmt(ostream &stream, const Fmt &fmt, const bool *p);
size_t fmt(ostream &stream, const Fmt &fmt, const double *p);
size_t fmt(ostream &stream, const Fmt &fmt, const intptr_t **p);
size_t fmt(ostream &stream, const Fmt &fmt, const char *p);
size_t fmt(ostream &stream, const Fmt &fmt, const char **p);

size_t fmt(ostream &stream, const Fmt &fmt, const str *p);
size_t fmt(ostream &stream, const Fmt &fmt, const Buf *p);
size_t fmt(ostream &stream, const Fmt &fmt, const mesa_mac_t *p);
size_t fmt(ostream &strema, const Fmt &fmt, const mesa_ipv6_t *p);
size_t fmt(ostream &stream, const Fmt &fmt, const mesa_ipv4_network_t *p);
size_t fmt(ostream &stream, const Fmt &fmt, const mesa_ipv6_network_t *p);
size_t fmt(ostream &stream, const Fmt &fmt, const mesa_ip_addr_t *p);
size_t fmt(ostream &stream, const Fmt &fmt, const mesa_ip_network_t *p);
size_t fmt(ostream &stream, const Fmt &fmt, const vtss_inet_address_t *p);
size_t fmt(ostream &stream, const Fmt &fmt, const mesa_port_list_t *p);

vtss::ostream &print_fmt_impl(vtss::ostream &stream, const char *format, int,
                              void *[], void *[]);
size_t print_fmt_impl(char *d, size_t len, const char *format, int, void *[],
                      void *[]);

namespace FmtDetails {

template <typename T>
struct TypeResolution;


template <typename T>
struct TypeResolution<T &> {
    typedef const T *type;
    struct Inner {
        static constexpr size_t
        foo(vtss::ostream &o, const Fmt &f,
            typename vtss::conditional<vtss::is_enum<T>::value, const int32_t *,
                                       typename TypeResolution<T>::type>::type v) {
            return fmt(o, f, v);
        }
    };
    static constexpr auto *func_ptr = Inner::foo;
};

template <size_t N>
struct TypeResolution<const char (&)[N]> {
    struct Inner {
        static constexpr size_t foo(vtss::ostream &o, const Fmt &f,
                                    const char *v) {
            return fmt(o, f, v);
        }
    };
    static constexpr auto *func_ptr = Inner::foo;
};

template <size_t N>
struct TypeResolution<char (&)[N]> {
    struct Inner {
        static constexpr size_t foo(vtss::ostream &o, const Fmt &f,
                                    const char *v) {
            return fmt(o, f, v);
        }
    };
    static constexpr auto *func_ptr = Inner::foo;
};

template <size_t N>
struct TypeResolution<const unsigned char (&)[N]> {
    struct Inner {
        static constexpr size_t foo(vtss::ostream &o, const Fmt &f,
                                    const unsigned char *v) {
            return fmt(o, f, v);
        }
    };
    static constexpr auto *func_ptr = Inner::foo;
};

template <size_t N>
struct TypeResolution<unsigned char (&)[N]> {
    struct Inner {
        static constexpr size_t foo(vtss::ostream &o, const Fmt &f,
                                    const unsigned char *v) {
            return fmt(o, f, v);
        }
    };
    static constexpr auto *func_ptr = Inner::foo;
};

template <>
struct TypeResolution<const char *&> {
    typedef const char *type;
    static constexpr size_t (*func_ptr)(vtss::ostream &, const Fmt &, const char**) = fmt;
};

template <>
struct TypeResolution<char *&> {
    typedef const char *type;
    static constexpr size_t (*func_ptr)(vtss::ostream &, const Fmt &, const char**) = fmt;
};

template <>
struct TypeResolution<char *> {
    typedef const char *type;
    static constexpr size_t (*func_ptr)(vtss::ostream &, const Fmt &, const char**) = fmt;
};

template <>
struct TypeResolution<const char *> {
    typedef const char *type;
    static constexpr size_t (*func_ptr)(vtss::ostream &, const Fmt &, const char**) = fmt;
};

template <typename T>
struct TypeResolution<T *> {
    typedef const T **type;
    struct Inner {
        static constexpr size_t foo(vtss::ostream &o, const Fmt &f,
                                    const intptr_t **v) {
            return fmt(o, f, v);
        }
    };
    static constexpr auto *func_ptr = Inner::foo;
};

template <typename T>
struct TypeResolution<T *const &> {
    typedef const T **type;
    struct Inner {
        static constexpr size_t foo(vtss::ostream &o, const Fmt &f,
                                    const intptr_t **v) {
            return fmt(o, f, v);
        }
    };
    static constexpr auto *func_ptr = Inner::foo;
};

template <typename T>
struct TypeResolution<T *&> {
    typedef const T **type;
    struct Inner {
        static constexpr size_t foo(vtss::ostream &o, const Fmt &f,
                                    const intptr_t **v) {
            return fmt(o, f, v);
        }
    };
    static constexpr auto *func_ptr = Inner::foo;
};

template <typename T>
struct TypeResolution {
    typedef const T *type;
    struct Inner {
        static constexpr size_t
        foo(vtss::ostream &o, const Fmt &f,
            typename vtss::conditional<vtss::is_enum<T>::value, const int32_t *,
                                       typename TypeResolution<T>::type>::type v) {
            return fmt(o, f, v);
        }
    };
    static constexpr auto *func_ptr = Inner::foo;
};


template <typename ARG>
using RemoveConst = typename vtss::conditional<
        vtss::is_lvalue_reference<ARG>::value,
        typename vtss::add_lvalue_reference<typename vtss::remove_volatile<typename vtss::remove_const<
                typename vtss::remove_reference<ARG>::type>::type>::type>::type,
        typename vtss::remove_volatile<typename vtss::remove_const<ARG>::type>::type>::type;
}  // namespace FmtDetails


/**
 * This function implements an C++ish version of the traditional snprintf
 * function (and it is backwards compatible with the _typical_ use of snprintf),
 * but with some important differences:
 *
 * - It is easy to extend with custom types, just implement the 'fmt' of your
 *   favorite type:
 *
 *   size_t fmt(vtss::ostream &stream, const Fmt &fmt, const MY_TYPE *p);
 *
 *   NOTE: this should be declared/implemented in the same namespace as
 *   'MY_TYPE'
 *
 * - The encoding flags in the format string (%s, %d, %x etc) is only used as
 *   _hits_ and does not need to match the actual types. This means that it is
 *   now allowed to print a uint64_t with a "%s".
 *
 *   - The individual formatter, for the individual chooses which encoding flags
 *     is supported. If the specified flag is not supported, then a default
 *     encoding should be used.
 *
 *   - Alignment operator is implemented by the generic code (and thereby
 *     supported for all types.
 *
 * - The size modifier (%hhd, %lld etc) is not used (they are accepted but
 *   skipped).
 *
 * - The 'printf("%*d", width, num)' syntax is supported, but 'printf("%2$*1$d",
 *   width, num)' is not.
 *
 * \param stream [OUT] where all data is written, it is null terminated
 * \param format [IN]  contains the format of the output, similar to printf
 *                     format
 * \param args   [IN]  arguments to be displayed
 * \return             vtss::ostream
 */
template <typename... ARGS>
vtss::ostream &print_fmt(vtss::ostream &stream, const char *format,
                         ARGS &&... args) {
    void *func_ptrs[] = {
            (void *)FmtDetails::TypeResolution<FmtDetails::RemoveConst<ARGS>>::func_ptr...};
    void *args_ptrs[] = {(void *)&args...};

    return print_fmt_impl(stream, format, sizeof(func_ptrs) / sizeof(void *),
                          func_ptrs, args_ptrs);
}

/**
 * This function implements an C++ish version of the traditional snprintf
 * function (and it is backwards compatible with the _typical_ use of snprintf),
 * but with some important differences:
 *
 * - The output buffer is always null-terminated. If the buffer is too small
 *   content will be skipped, but the null-termination will be there.
 *
 * - The return value is the number of bytes written, excluding
 *   null-termination. This means that it can not return more than 'len - 1'.
 *   (this is different when comparing to snprintf()).
 *
 *   - If the traditional snprintf() value is needed (the number of bytes
 *     written if the output buffer was big enough) then you should use one of
 *     the other overloads that can re-allocate when needed.
 *
 * - It is easy to extend with custom types, just implement the 'fmt' of your
 *   favorite type:
 *
 *   size_t fmt(vtss::ostream &stream, const Fmt &fmt, const MY_TYPE *p);
 *
 *   NOTE: this should be declared/implemented in the same namespace as
 *   'MY_TYPE'
 * 
 *   and a operator<< for the vtss::ostream:
 * 
 *   ostream& operator<<(ostream& o_, const MY_TYPE &p);
 *
 * - The encoding flags in the format string (%s, %d, %x etc) is only used as
 *   _hits_ and does not need to match the actual types. This means that it is
 *   now allowed to print a uint64_t with a "%s".
 *
 *   - The individual formatter, for the individual chooses which encoding flags
 *     is supported. If the specified flag is not supported, then a default
 *     encoding should be used.
 *
 *   - Alignment operator is implemented by the generic code (and thereby
 *     supported for all types.
 *
 * - The size modifier (%hhd, %lld etc) is not used (they are accepted but
 *   skipped).
 *
 * - The 'printf("%*d", width, num)' syntax is supported, but 'printf("%2$*1$d",
 *   width, num)' is not.
 *
 * \param dst    [OUT] buffer where all data is written, it is null terminated
 * \param len    [IN]  size of the buffer
 * \param format [IN]  contains the format of the output, similar to printf
 *                     format
 * \param args   [IN]  arguments to be displayed
 * \return             Number of bytes written (including the null-termination)
 */
template <typename... ARGS>
size_t print_fmt(char *dst, size_t len, const char *format, ARGS &&... args) {
    void *func_ptrs[] = {
            (void *)FmtDetails::TypeResolution<FmtDetails::RemoveConst<ARGS>>::func_ptr...};
    void *args_ptrs[] = {(void *)&args...};

    return print_fmt_impl(dst, len, format, sizeof(func_ptrs) / sizeof(void *),
                          func_ptrs, args_ptrs);
}

}  // namespace vtss

#endif  // __VTSS_BASICS_PRINT_FMT_HXX__
