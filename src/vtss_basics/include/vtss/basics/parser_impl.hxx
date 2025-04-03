/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __PARSER_IMPL_HXX__
#define __PARSER_IMPL_HXX__

#include <math.h>

#include <vtss/basics/types.hxx>
#include <vtss/basics/meta.hxx>
#include <vtss/basics/algorithm.hxx>
#include <vtss/basics/parse_group.hxx>
#include <vtss/basics/string.hxx>
#include <vtss/basics/arithmetic-overflow.hxx>

namespace vtss {
namespace parser {

namespace group {
struct Word {
    bool operator()(char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') || c == '_';
    }
};

struct Space {
    bool operator()(char c) { return c == ' ' || c == '\t' || c == '\n'; }
};

struct Digit {
    bool operator()(char c) { return c >= '0' && c <= '9'; }
};
}  // namespace group


template<unsigned BASE>
bool lexical_convert(const char c, unsigned& v);

template<>
inline bool lexical_convert<2>(const char c, unsigned& v) {
    if (c >= '0' && c <= '1') {
        v = c - '0';
        return true;
    }
    return false;
}

template<>
inline bool lexical_convert<8>(const char c, unsigned& v) {
    if (c >= '0' && c <= '7') {
        v = c - '0';
        return true;
    }
    return false;
}

template<>
inline bool lexical_convert<10>(const char c, unsigned& v) {
    if (c >= '0' && c <= '9') {
        v = c - '0';
        return true;
    }
    return false;
}

template<>
inline bool lexical_convert<16>(const char c, unsigned& v) {
    if (c >= '0' && c <= '9') {
        v = c - '0';
        return true;

    } else if ( c >= 'A' && c <= 'F' ) {
        v = (c - 'A') + 10;
        return true;

    } else if ( c >= 'a' && c <= 'f' ) {
        v = (c - 'a') + 10;
        return true;
    }

    return false;
}

struct Lit : public ParserBase {
    typedef const str& val;

    explicit Lit(const str& s) : s_(s) { }
    explicit Lit(const char * s) : s_(s) { }

    bool operator()(const char *& b, const char * e) {
        const char * _b = b;
        const char * i = s_.begin();

        while (b != e && i != s_.end()) {
            if (*b != *i )
                break;
            ++b;
            ++i;
        }

        if (i == s_.end())
            return true;

        b = _b;
        return false;
    }

    val get() const {
        return s_;
    }

    str s_;
};

struct OneOrMoreSpaces : public ParserBase {
    typedef const str& val;

    bool operator()(const char *& b, const char * e) {
        const char * i = b;

        while (i != e && (*i == ' ' || *i == '\t'))
            i++;

        // if equal then we have not parsed a single space
        if (i == b)
            return false;

        s_ = str(b, i);
        b = i;
        return true;
    }

    val get() const {
        return s_;
    }

    str s_;
};

template<typename PRED>
struct OneOrMore : public ParserBase {
    typedef const str& val;

    bool operator()(const char *& b, const char * e) {
        const char * i = b;

        PRED pred;
        while (i != e && pred(*i))
            i++;

        // if equal then we have not parsed a single space
        if (i == b)
            return false;

        s_ = str(b, i);
        b = i;
        return true;
    }

    val get() const {
        return s_;
    }

    str s_;
};

struct ZeroOrMoreSpaces : public ParserBase {
    typedef const str& val;

    bool operator()(const char *& b, const char * e) {
        const char * i = b;

        while (i != e && (*i == ' ' || *i == '\t'))
            i++;

        s_ = str(b, i);
        b = i;
        return true;
    }

    val get() const {
        return s_;
    }

    str s_;
};

struct EmptyLine : public ParserBase {
    typedef const str& val;

    bool operator()(const char *& b, const char * e) {
        const char * i = b;

        while (i != e && (*i == ' ' || *i == '\t'))
            i++;

        s_ = str(b, i);
        b = i;
        return b == e;
    }

    val get() const {
        return s_;
    }

    str s_;
};

struct EndOfInput : public ParserBase {
    bool operator()(const char *& b, const char * e) {
        return b == e;
    }
};

template<typename T>
struct SequenceNR : public ParserBase {
    typedef const str& val;

    SequenceNR(const T &t, uint32_t min, uint32_t max) : min_(min), max_(max),
                t_(t) { }
    SequenceNR(const T &t, uint32_t min) : min_(min), max_(0), t_(t) { }
    SequenceNR(const T &t) : min_(0), max_(0), t_(t) { }

    bool operator()(const char *& b, const char * e) {
        cnt_ = 0;
        const char * _b = b;

        while (b != e && (cnt_ < max_ || max_ == 0)) {
            if (t_(b, e)) {
                cnt_ += 1;
            } else {
                break;
            }
        }

        if (cnt_ >= min_)
            return true;

        b = _b;
        return false;
    }

    val get() const { return s_; }

    uint32_t min_;
    uint32_t max_;
    uint32_t cnt_;
    str s_;
    T t_;
};

template<>
struct SequenceNR<char> : public ParserBase {
    typedef const str& val;

    SequenceNR(char t, uint32_t min, uint32_t max) : min_(min), max_(max),
                t_(t) { }
    SequenceNR(char t, uint32_t min) : min_(min), max_(0), t_(t) { }
    SequenceNR(char t) : min_(0), max_(0), t_(t) { }

    bool operator()(const char *& b, const char * e) {
        cnt_ = 0;
        const char * _b = b;

        while (b != e && (cnt_ < max_ || max_ == 0)) {
            if (*b == t_)
                ++cnt_, ++b;
            else
                break;
        }

        if (cnt_ >= min_)
            return true;

        b = _b;
        return false;
    }

    val get() const { return s_; }

    uint32_t min_;
    uint32_t max_;
    uint32_t cnt_;
    str s_;
    char t_;
};

template <typename T, typename... Args>
struct TagValue : public ParserBase {
    explicit TagValue(const str& s, Args&&... args)
        : l_(s), t_(vtss::forward<Args>(args)...) {}
    explicit TagValue(const char * s, Args&&... args)
        : l_(s), t_(vtss::forward<Args>(args)...) {}

    bool operator()(const char *& b, const char *e) {
        return Group(b, e, l_, s_, t_);
    }

    const T& get() const { return t_; }

    Lit l_;
    OneOrMoreSpaces s_;
    T t_;
};

template<unsigned N>
struct String : public ParserBase {
    String(uint32_t size) : maxsize_(size) { VTSS_ASSERT(size < N); }
    bool operator()(const char *& b, const char *e) {
        size_ = 0;
        const char * _b = b;

        if (*_b++ != '"') return false;

        while (*_b != '"' && size_ < maxsize_ && *_b >= ' ' && *_b <= '~')  {
            value[size_++]=*_b++;
        }
        if (*_b != '"' || size_ == maxsize_) return false;
        value[size_]=0;
        return true;
    }
    const char* get() const { return value; }
    uint32_t maxsize_;
    uint32_t size_;
    char value[N];
};

struct StringRef : public ParserBase {
    bool operator()(const char *& b, const char *e) {
        const char * _b = b;

        if (_b == e || *_b++ != '"') return false;

        const char *str_b = _b;
        while (_b != e && *_b != '"' && *_b >= ' ' && *_b <= '~') _b++;
        if (*_b != '"') return false;

        value = str(str_b, _b);

        // Consume the trailing '"' - which we know is there...
        _b++;
        b = _b;
        return true;
    }

    str get() const { return value; }
    str value;
};

struct JsonString : public ParserBase {
    bool operator()(const char *& b, const char *e);
    str get() const { return value; }
    std::string value;
};

template<typename T, unsigned BASE, unsigned MIN = 0, unsigned MAX = 0>
struct Int;

template<typename T, unsigned MIN, unsigned MAX>
struct IntSignedBase10 : public ParserBase {
    typedef T value_type;
    IntSignedBase10(T v = 0) : i(v), d(v) { }

    bool operator()(const char *& b, const char * e) {
        const char * _b = b;

        i = 0;
        bool sign = false;
        unsigned digit_cnt = 0;

        if (b != e && *b == '-') {
            sign = true;
            b++;
        }

        while (b != e) {
            bool overflow;
            unsigned d;

            if (!lexical_convert<10>(*b, d))
                break;

            ++b;
            ++digit_cnt;

            i = mult_overflow(i, (T)10, &overflow);
            if (overflow)
                goto Error;

            if (sign) {
                i = sub_overflow(i, (T)d, &overflow);
                if (overflow)  // wrap around detection
                    goto Error;

            } else {
                i = add_overflow(i, (T)d, &overflow);
                if (overflow)  // wrap around detection
                    goto Error;
            }

            if (digit_cnt == MAX) break;
        }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        if (MIN != 0 && digit_cnt < MIN)
            goto Error;
#ifdef __clang__
#pragma clang diagnostic pop
#endif

        if (MAX != 0 && digit_cnt > MAX)
            goto Error;

        return true;

 Error:
        i = d;
        b = _b;
        return false;
    }

    const value_type& get() const {
        return i;
    }

    T i;
    T d;
};

template<typename T, unsigned MIN, unsigned MAX>
struct IntUnsignedBase10 : public ParserBase {
    typedef T value_type;
    IntUnsignedBase10(T v = 0) : i(v), d(v) { }

    bool operator()(const char *& b, const char * e) {
        i = 0;
        const char * _b = b;
        unsigned digit_cnt = 0;

        while (b != e) {
            bool overflow;
            unsigned d;

            if (!lexical_convert<10>(*b, d))
                break;

            ++b;
            ++digit_cnt;
            i = mult_overflow(i, (T)10, &overflow);
            if (overflow) goto Error;
            i = add_overflow(i, (T)d, &overflow);
            if (overflow) goto Error;

            if (digit_cnt == MAX) break;
        }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        if (MIN != 0 && digit_cnt < MIN)
            goto Error;
#ifdef __clang__
#pragma clang diagnostic pop
#endif

        if (MAX != 0 && digit_cnt > MAX)
            goto Error;

        return true;

 Error:
        i = d;
        b = _b;
        return false;
    }

    const value_type& get() const {
        return i;
    }

    T i;
    T d;
};

template<typename T, unsigned MIN, unsigned MAX>
struct Int<T, 10, MIN, MAX> : public
    meta::_if<
        typename meta::equal_type<
            typename meta::IntTraits<T>::SignType,
            meta::Signed
        >::type,
        IntSignedBase10<T, MIN, MAX>,
        IntUnsignedBase10<T, MIN, MAX>
    >::type {
};

template<typename T, unsigned BASE> struct MaxDigitCnt;
template<> struct MaxDigitCnt<uint8_t,  16> { enum { cnt = 2 }; };
template<> struct MaxDigitCnt<uint16_t, 16> { enum { cnt = 4 }; };
template<> struct MaxDigitCnt<uint32_t, 16> { enum { cnt = 8 }; };

// This one is only used for base 2, 8, and 16.
// TODO(anielsen) add an compile time assert ensuring that!
template<typename T, unsigned BASE, unsigned MIN, unsigned MAX>
struct Int : public ParserBase {
    typedef T value_type;
    Int() : i(0) { }

    bool operator()(const char *& b, const char * e) {
        typedef typename meta::IntTraits<T>::WithUnsigned UT;
        i = 0;
        UT val = 0;
        const char * _b = b;
        unsigned digit_cnt = 0;
        unsigned digit_cnt_non_zero = 0;

        while (b != e) {
            unsigned d;

            if (!lexical_convert<BASE>(*b, d))
                break;

            ++b;
            ++digit_cnt;
            val *= BASE;
            val += d;

            if (val)
                ++digit_cnt_non_zero;

            if (digit_cnt_non_zero > MaxDigitCnt<T, BASE>::cnt)
                goto Error;

            if (digit_cnt == MAX) break;
        }

        if (MIN != 0 && digit_cnt < MIN)
            goto Error;

        if (MAX != 0 && digit_cnt > MAX)
            goto Error;

        i = (T)val;
        return true;

 Error:
        b = _b;
        return false;
    }

    const value_type& get() const {
        return i;
    }

    T i;
};


inline bool conv(const str& s, uint16_t& t) {
    const char *i = s.begin();
    Int<uint16_t, 10, 1> p;

    if (!p(i, s.end())) {
        return false;
    }

    t = p.get();
    return true;
}

struct Double : public ParserBase {
    typedef double value_type;
    Double() : error(0), exponent(0), significand(0), value(0.) { }

    bool operator()(const char *&b, const char * e) {
        const char * _b = b;

        significand = 0;
        exponent = 0;
        bool significand_sign = false;
        uint32_t digit_cnt = 0;
        bool period_found = false;
        decimals = 0;
        bool exponent_sign = false;
        uint32_t exponent_cnt = 0;


        if (b != e && *b == '-') {
            significand_sign = true;
            b++;
        } else if (b != e && *b == '+') {
            // ignore + sign
            b++;
        }

        while (b != e) {
            if (*b == 'e' || *b == 'E') {
                // skip to exponent
                break;
            }

            if (*b == '.') {
                if (period_found) {
                    error = __LINE__;
                    goto Error;
                }
                ++b;
                period_found = true;
                continue;
            }
            bool overflow;
            unsigned d;

            if (!lexical_convert<10>(*b, d))
                break;

            ++b;
            ++digit_cnt;
            if (period_found) {
                ++decimals;
            }

            significand = mult_overflow(significand, (int64_t)10, &overflow);
            if (overflow) {
                error = __LINE__;
                goto Error;
            }

            if (significand_sign) {
                significand = sub_overflow(significand, (int64_t)d, &overflow);
                if (overflow) {  // wrap around detection
                    error = __LINE__;
                    goto Error;
                }
            } else {
                significand = add_overflow(significand, (int64_t)d, &overflow);
                if (overflow)  { // wrap around detection
                    error = __LINE__;
                    goto Error;
                }
            }

        }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
        if (digit_cnt == 0) {
            error = __LINE__;
            goto Error;
        }
#ifdef __clang__
#pragma clang diagnostic pop
#endif

        // Now parse exponent
        if (*b == 'e' || *b == 'E') {
            b++;
            // Exponent found
            if (b != e && *b == '-') {
                exponent_sign = true;
                b++;
            } else if (b != e && *b == '+') {
                // ignore + sign
                b++;
            }

            while (b != e) {

                bool overflow;
                unsigned d;

                if (!lexical_convert<10>(*b, d))
                    break;

                ++b;
                ++exponent_cnt;

                exponent = mult_overflow(exponent, 10, &overflow);
                if (overflow) {
                    error = __LINE__;
                    goto Error;
                }

                if (exponent_sign) {
                    exponent = sub_overflow(exponent, (int32_t)d, &overflow);
                    if (overflow) { // wrap around detection
                        error = __LINE__;
                        goto Error;
                    }

                } else {
                    exponent = add_overflow(exponent, (int32_t)d, &overflow);
                    if (overflow) { // wrap around detection
                        error = __LINE__;
                        goto Error;
                    }
                }

            }
            if (exponent_cnt == 0) {
                error = __LINE__;
                goto Error;
            }
        }

        value = significand;
        value = value * pow(10,exponent-decimals);
        return true;

      Error:
        value = 0.;
        b = _b;
        return false;
    }

    const value_type& get() const {
        return value;
    }

    uint32_t error;
    int32_t decimals;
    int32_t exponent;
    int64_t significand;
    double value;
};

struct MacAddress : public ParserBase {
    typedef vtss::MacAddress value_type;

    bool operator()(const char *&b, const char * e) {
        const char *_b = b;
        Lit sep(":");
        Int<unsigned char, 16, 1, 2> e0;
        Int<unsigned char, 16, 1, 2> e1;
        Int<unsigned char, 16, 1, 2> e2;
        Int<unsigned char, 16, 1, 2> e3;
        Int<unsigned char, 16, 1, 2> e4;
        Int<unsigned char, 16, 1, 2> e5;

        if (Group(b, e, e0, sep, e1, sep, e2, sep, e3, sep, e4, sep, e5)) {
            mesa_mac_t tmp = { { e0.get(), e1.get(), e2.get(),
                                 e3.get(), e4.get(), e5.get() } };
            mac.as_api_type() = tmp;
            return true;
        }

        b = _b;
        return false;
    }
    vtss::MacAddress mac;
};

struct IPv4 : public ParserBase {
    typedef vtss::Ipv4Address value_type;

    bool operator()(const char *& b, const char * e) {
        const char * _b = b;

        Lit sep(".");
        Int<unsigned char, 10, 1, 3> e1;
        Int<unsigned char, 10, 1, 3> e2;
        Int<unsigned char, 10, 1, 3> e3;
        Int<unsigned char, 10, 1, 3> e4;

        if (Group(b, e, e1, sep, e2, sep, e3, sep, e4)) {
            ip.as_api_type() = ((uint32_t)e1.get()) << 24 |
                               ((uint32_t)e2.get()) << 16 |
                               ((uint32_t)e3.get()) <<  8 |
                               ((uint32_t)e4.get());
            return true;
        }

        b = _b;
        return false;
    }

    const value_type& get() const {
        return ip;
    }

    Ipv4Address ip;
};

struct Ipv4Network : public ParserBase {
    typedef vtss::Ipv4Network value_type;

    bool operator()(const char *& b, const char * e) {
        const char * _b = b;

        IPv4 ip;
        Lit sep("/");
        Lit sep_esc("\\/");
        Int<unsigned char, 10, 1, 2> prefix;

        if (Group(b, e, ip, sep, prefix) || Group(b, e, ip, sep_esc, prefix)) {
            if (prefix.get() > 32 )
                goto Error;

            network = vtss::Ipv4Network(ip.get(), prefix.get());
            return true;
        }

 Error:
        b = _b;
        return false;
    }

    const value_type& get() const {
        return network;
    }

    value_type network;
};

struct IPv6 : public ParserBase {
    typedef Ipv6Address value_type;
    bool operator()(const char *& b, const char * e) {
        const char * _b = b;

        Lit s(":");
        Int<uint16_t, 16, 1, 4> e1; e1.i = 0;
        Int<uint16_t, 16, 1, 4> e2; e2.i = 0;
        Int<uint16_t, 16, 1, 4> e3; e3.i = 0;
        Int<uint16_t, 16, 1, 4> e4; e4.i = 0;
        Int<uint16_t, 16, 1, 4> e5; e5.i = 0;
        Int<uint16_t, 16, 1, 4> e6; e6.i = 0;
        Int<uint16_t, 16, 1, 4> e7; e7.i = 0;
        Int<uint16_t, 16, 1, 4> e8; e8.i = 0;
        IPv4 ipv4;

        if (Group(b, e, e1, s))       goto Start1;        // look for a:
        else if (Group(b, e, s, s))   goto End6;          // look for ::
        else if (s(b, e))             goto Start1;        // look for :
        else                          goto Error;
 Start1:
        if (Group(b, e, e2, s))       goto Start2;        // look for b:
        else if (s(b, e))             goto End6;          // look for :
        else                          goto Error;
 Start2:
        if (Group(b, e, e3, s))       goto Start3;        // look for c:
        else if (s(b, e))             goto End5;          // look for :
        else                          goto Error;
 Start3:
        if (Group(b, e, e4, s))       goto Start4;        // look for d:
        else if (s(b, e))             goto End4;          // look for :
        else                          goto Error;
 Start4:
        if (Group(b, e, e5, s))       goto Start5;        // look for e:
        else if (s(b, e))             goto End3;          // look for :
        else                          goto Error;
 Start5:
        if (Group(b, e, e6, s))       goto Start6;        // look for f:
        else if (s(b, e))             goto End2;          // look for :
        else                          goto Error;
 Start6:
        if (Group(b, e, e7, s))       goto Start7;        // look for g:
        else if (s(b, e))             goto End1;          // look for :
        else if (ipv4(b, e))          goto Finish_v4;
        else                          goto Error;
 Start7:
        if (e8(b, e))                 goto Finish;        // look for h
        else                          goto Error;

        // This is the part after "::" which may be one of the following:
 End6:  // c:d:e:f:g:h or c:d:e:f:ipv4
        if (Group(b, e, e3, s, e4, s, e5, s, e6, s, e7, s, e8)) goto Finish;
        else if (Group(b, e, e3, s, e4, s, e5, s, e6, s, ipv4)) goto Finish_v4;
        else                                                    e3.i = 0;
 End5:  // d:e:f:g:h or d:e:f:ipv4
        if (Group(b, e, e4, s, e5, s, e6, s, e7, s, e8))        goto Finish;
        else if (Group(b, e, e4, s, e5, s, e6, s, ipv4))        goto Finish_v4;
        else                                                    e4.i = 0;
 End4:  // e:f:g:h or e:f:ipv4
        if (Group(b, e, e5, s, e6, s, e7, s, e8))               goto Finish;
        else if (Group(b, e, e5, s, e6, s, ipv4))               goto Finish_v4;
        else                                                    e5.i = 0;
 End3:  // f:g:h or f:ipv4
        if (Group(b, e, e6, s, e7, s, e8))                      goto Finish;
        else if (Group(b, e, e6, s, ipv4))                      goto Finish_v4;
        else                                                    e6.i = 0;
 End2:  // g:h or ipv4
        if (Group(b, e, e7, s, e8))                             goto Finish;
        else if (ipv4(b, e))                                    goto Finish_v4;
        else                                                    e7.i = 0;
 End1:  // h or nothing at all
        if (!e8(b, e))                                          e8.i = 0;
        goto Finish;

 Finish_v4:
        e7.i = (ipv4.get().as_api_type() >> 16) & 0xffff;
        e8.i = (ipv4.get().as_api_type())       & 0xffff;
 Finish:
        ip.as_api_type().addr[ 0] = (e1.get() >> 8) & 0xff;
        ip.as_api_type().addr[ 1] = e1.get() & 0xff;
        ip.as_api_type().addr[ 2] = (e2.get() >> 8) & 0xff;
        ip.as_api_type().addr[ 3] = e2.get() & 0xff;
        ip.as_api_type().addr[ 4] = (e3.get() >> 8) & 0xff;
        ip.as_api_type().addr[ 5] = e3.get() & 0xff;
        ip.as_api_type().addr[ 6] = (e4.get() >> 8) & 0xff;
        ip.as_api_type().addr[ 7] = e4.get() & 0xff;
        ip.as_api_type().addr[ 8] = (e5.get() >> 8) & 0xff;
        ip.as_api_type().addr[ 9] = e5.get() & 0xff;
        ip.as_api_type().addr[10] = (e6.get() >> 8) & 0xff;
        ip.as_api_type().addr[11] = e6.get() & 0xff;
        ip.as_api_type().addr[12] = (e7.get() >> 8) & 0xff;
        ip.as_api_type().addr[13] = e7.get() & 0xff;
        ip.as_api_type().addr[14] = (e8.get() >> 8) & 0xff;
        ip.as_api_type().addr[15] = e8.get() & 0xff;
        return true;
 Error:
        b = _b;
        return false;
    }

    const value_type& get() const {
        return ip;
    }

    value_type ip;
};

struct Ipv6Network : public ParserBase {
    typedef vtss::Ipv6Network value_type;

    bool operator()(const char *& b, const char * e) {
        const char * _b = b;

        IPv6 ip;
        Lit sep("/");
        Lit sep_esc("\\/");
        Int<unsigned char, 10, 1, 3> prefix;

        if (Group(b, e, ip, sep, prefix) || Group(b, e, ip, sep_esc, prefix)) {
            if (prefix.get() > 128 )
                goto Error;

            network = value_type(ip.get(), prefix.get());
            return true;
        }

 Error:
        b = _b;
        return false;
    }

    const value_type& get() const {
        return network;
    }

    value_type network;
};

struct IpAddress : public ParserBase {
    typedef vtss::IpAddress value_type;

    bool operator()(const char *&b, const char *e) {
        return ip.string_parse(b, e);
    }

    const value_type& get() const {
        return ip;
    }

    value_type ip;
};

struct IpNetwork : public ParserBase {
    typedef vtss::IpNetwork value_type;

    bool operator()(const char *& b, const char * e) {
        const char * _b = b;
        Ipv6Network ipv6;
        Ipv4Network ipv4;

        if (ipv4(b, e)) {
            ip = ipv4.get();
            return true;
        }

        if (ipv6(b, e)) {
            ip = ipv6.get();
            return true;
        }

        b = _b;
        return false;
    }

    const value_type& get() const {
        return ip;
    }

    value_type ip;
};

struct DomainName : public ParserBase {
    struct DomainNameLabel : public ParserBase {
        static bool digit_alpha(char c) {
            return (c >= 'a' && c <= 'z') ||
                   (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9');
        }

        static bool digit_alpha_hyphen(char c) {
            return digit_alpha(c) || c == '-';
        }

        bool operator()(const char *& b, const char * e) {
            // Min 1 char, max 63 chars, may not start and end with an hyphen

            // Starting point
            const char *b_old = b;

            // Check that we do not start with an hyphen
            if (!digit_alpha(*b))
                return false;

            while (b != e)
                if (digit_alpha_hyphen(*b))
                    ++b;
                else
                    break;

            // Check length
            unsigned length = b - b_old;
            if (length < 1 || length > 63) {
                b = b_old;  // reset pos pointer
                return false;
            }

            // Check that we do not end with an hyphen
            if (!digit_alpha(*(b - 1))) {
                b = b_old;  // reset pos pointer
                return false;
            }

            return true;
        }
    };

    bool operator()(const char *& b, const char * e) {
        // label [ *(dot label) [dot] ]
        // No more than 127 levels!

        int cnt;
        Lit dot(".");
        DomainNameLabel label;
        const char *b_old = b;
        b_ = b, e_ = b;  // no range is captured

        // mandotory: label
        if (!label(b, e))
            return false;

        cnt = 1;  // we just got one

        // optional: *(dot label)
        while (Group(b, e, dot, label)) {
            cnt++;
            if (cnt > 127) break;
        }

        // optional: dot
        dot(b, e);

        // Enforce max-depth rule
        if (cnt > 127) {
            b = b_old;
            return false;
        }

        // Encorce max length
        if ((b - b_old) > 253) {
            b = b_old;
            return false;
        }

        e_ = b;  // update captured range
        return true;
    }

    const char *b_, *e_;
};

// URI parser as defined in rfc3986.txt with some exceptions
// - Relative URL's are not permitted
// - The host part must either be an IPv4, IPv6 or a valid DomainName
struct Uri {
    struct Alpha {
        static bool check(char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        }
    };

    struct Digit {
        static bool check(char c) {
            return c >= '0' && c <= '9';
        }
    };

    struct GenDelims {
        // gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
        static bool check(char c) {
            return c == ':' || c == '/' || c == '?' || c == '#' ||
                   c == '[' || c == ']' || c == '@';
        }
    };

    struct SubDelims {
        // sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
        //               / "*" / "+" / "," / ";" / "="
        static bool check(char c) {
            return c == '!' || c == '$' || c == '&' || c == '\'' ||
                   c == '(' || c == ')' || c == '*' || c == '+'  ||
                   c == ',' || c == ';' || c == '=';
        }
    };

    struct Reserved {
        // reserved      = gen-delims / sub-delims
        static bool check(char c) {
            return GenDelims::check(c) || SubDelims::check(c);
        }
    };

    struct Unreserved {
        // unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
        static bool check(char c) {
            return Alpha::check(c) || Digit::check(c) ||
                   c == '-' || c == '.' || c == '_' || c == '~';
        }
    };

    struct PctEncoded : public ParserBase {
        // pct-encoded   = "%" HEXDIG HEXDIG
        bool operator()(const char *& b, const char * e) {
            Lit lit("%");
            Int<uint8_t, 16, 2, 2> num;

            if (!Group(b, e, lit, num)) {
                return false;
            }

            data = num.get();
            return true;
        }

        uint8_t data;
    };

    struct Pchar : public ParserBase {
        // pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
        bool operator()(const char *& b, const char * e) {
            if (b == e) return false;

            // unreserved / sub-delims / ":" / "@"
            if (Unreserved::check(*b) || SubDelims::check(*b) ||
                *b == ':' || *b == '@') {
                data = *b++;  // sample result and increment pointer
                return true;
            }

            // pct-encoded
            PctEncoded pct_encode;
            if (pct_encode(b, e)) {
                data = pct_encode.data;  // sample result
                return true;
            }

            return false;
        }

        uint8_t data;
    };

    struct FragmentQuery : public ParserBase {
        // fragment      = *( pchar / "/" / "?" )
        // query         = *( pchar / "/" / "?" )

        bool operator()(const char *& b, const char * e) {
            b_ = b, e_ = b;  // no range is captured
            while (b != e) {
                // "/" / "?"
                if (*b == '/' || *b == '?') {
                    ++b;
                    continue;
                }

                Pchar pchar;
                if (pchar(b, e)) {
                    continue;
                }

                break;
            }

            e_ = b;  // update captured range
            return true;
        }
        const char *b_, *e_;
    };

#define VTSS_RFC3986_STRICT_PATH_PARSER
#if defined(VTSS_RFC3986_STRICT_PATH_PARSER)
    struct Segment : public ParserBase {
        // segment       = *pchar
        bool operator()(const char *& b, const char * e) {
            b_ = b, e_ = b;  // no range is captured

            Pchar pchar;
            while (pchar(b, e)) { }

            e_ = b;  // update captured range
            return true;
        }

        const char *b_, *e_;
    };

    struct SegmentNz : public ParserBase {
        // segment-nz    = 1*pchar
        bool operator()(const char *& b, const char * e) {
            Pchar pchar;
            b_ = b, e_ = b;  // no range is captured

            // consume at lease one pchar
            if (!pchar(b, e)) {
                return false;
            }

            // consume N pchar
            while (pchar(b, e)) { }

            e_ = b;  // update captured range
            return true;
        }

        const char *b_, *e_;
    };

    struct SegmentNzNc : public ParserBase {
        struct Element : public ParserBase {
            // unreserved / pct-encoded / sub-delims / "@"
            bool operator()(const char *& b, const char * e) {
                if (b == e) return false;

                // unreserved / sub-delims / "@"
                if (Unreserved::check(*b) || SubDelims::check(*b) ||
                        *b == '@') {
                    data = *b++;  // sample result and increment pointer
                    return true;
                }

                // pct-encoded
                PctEncoded pct_encode;
                if (pct_encode(b, e)) {
                    data = pct_encode.data;  // sample result
                    return true;
                }

                return false;
            }

            uint8_t data;
        };

        // segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
        //               ; non-zero-length segment without any colon ":"
        bool operator()(const char *& b, const char * e) {
            Element element;
            b_ = b, e_ = b;  // no range is captured

            // consume at lease one element
            if (!element(b, e)) {
                return false;
            }

            // consume N element
            while (element(b, e)) { }

            e_ = b;  // update captured range
            return true;
        }

        const char *b_, *e_;
    };

    struct PathAbempty : public ParserBase {
        // path-abempty  = *( "/" segment )

        bool operator()(const char *& b, const char * e) {
            Lit slash("/");
            Segment segment;
            b_ = b, e_ = b;  // no range is captured

            while (b != e)
                if (!Group(b, e, slash, segment))
                    break;

            e_ = b;  // update captured range
            return true;
        }

        const char *b_, *e_;
    };

    struct PathRootless : public ParserBase {
        // path-abempty  =            *( "/" segment )
        // path-rootless = segment-nz *( "/" segment )
        // path-rootless = segment-nz path-abempty
        bool operator()(const char *& b, const char * e) {
            SegmentNz segment_nz;
            PathAbempty path_abempty;

            b_ = b, e_ = b;  // no range is captured
            if (!Group(b, e, segment_nz, path_abempty)) {
                return false;
            }

            e_ = b;  // update captured range
            return true;
        }

        const char *b_, *e_;
    };


    struct PathAbsolute : public ParserBase {
        // path-rootless =       segment-nz *( "/" segment )
        // path-absolute = "/" [ segment-nz *( "/" segment ) ]
        // path-absolute = "/" path-rootless
        bool operator()(const char *& b, const char * e) {
            Lit slash("/");
            PathRootless path_root_less;

            b_ = b, e_ = b;  // no range is captured
            if (!Group(b, e, slash, path_root_less)) {
                return false;
            }

            e_ = b;  // update captured range
            return true;
        }

        const char *b_, *e_;
    };

    struct PathEmpty : public ParserBase {
        // path-empty    = 0<pchar>
        bool operator()(const char *& b, const char * e) {
            if (b == e) return true;

            const char *b_old = b;
            Pchar pchar;

            if (pchar(b, e)) {
                b = b_old;
                return false;
            }

            return true;
        }
    };
#else
    struct PathRelaxed : public ParserBase {
        // path-relaxed = *( pchar / "/" )

        bool operator()(const char *& b, const char * e) {
            b_ = b, e_ = b;  // no range is captured
            while (b != e) {
                // "/" / "?"
                if (*b == '/') {
                    ++b;
                    continue;
                }

                Pchar pchar;
                if (pchar(b, e)) {
                    continue;
                }

                break;
            }

            e_ = b;  // update captured range
            return true;
        }
        const char *b_, *e_;
    };

#endif

    struct IpLiteral : public ParserBase {
        // IP-literal    = "[" IPv6address "]"
        //                     ^         ^
        // NOTE: begin and end points to the IPv6 address on successfully
        // parsing

        bool operator()(const char *& b, const char * e) {
            Lit start("[");
            Lit end("]");
            IPv6 ipv6;

            b_ = b, e_ = b;  // no range is captured
            const char *before = b;
            if (Group(b, e, start, ipv6, end)) {
                // hack... we know that start and end consumes one char, and we
                // can use this to calcolate the pointers to the IPv6 address.
                b_ = before + 1;
                e_ = b - 1;
                return true;
            }

            return false;
        }
        const char *b_, *e_;
    };

    struct Host : public ParserBase {
        // host          = IP-literal / IPv4address / reg-name

        bool operator()(const char *& b, const char * e) {
            b_ = b, e_ = b;  // no range is captured

            IpLiteral   ipv6;
            IPv4        ipv4;
            DomainName  reg_name;

            if (ipv6(b, e)) {
                b_ = ipv6.b_;
                e_ = ipv6.e_;
                return true;
            }

            if (ipv4(b, e)) {
                e_ = b;  // update captured range
                return true;
            }

            if (reg_name(b, e)) {  // May consume nothing!
                e_ = b;  // update captured range
                return true;
            }

            // This functions returns true because the RFC3986 defins reg-name
            // as: *( unreserved / pct-encoded / sub-delims )
            // This may be an empty string, and it may be an invalid dns name.
            // This implementation requires either an empty string or a valid
            // DNS name
            return true;
        }
        const char *b_, *e_;
    };

    struct UserInfo : public ParserBase {
        // userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )

        bool operator()(const char *& b, const char * e) {
            b_ = b, e_ = b;  // no range is captured

            // *(...)
            while (b != e) {
                // unreserved / sub-delims / ":"
                if (Unreserved::check(*b) || SubDelims::check(*b) ||
                        *b == ':') {
                    ++b;
                    continue;
                }

                // pct-encoded
                PctEncoded pct_encoded;
                if (pct_encoded(b, e))
                    continue;

                break;
            }

            e_ = b;  // update captured range
            return true;
        }
        const char *b_, *e_;
    };

    struct Authority : public ParserBase {
        // authority     = [ userinfo "@" ] host [ ":" port ]

        bool operator()(const char *& b, const char * e) {
            const char *b_old = b;
            Lit at("@");
            Lit colon(":");

            has_userinfo = false;
            has_port = false;

            // optional
            if (Group(b, e, userinfo, at))
                has_userinfo = true;

            // mandotory (but may be empty!)
            if (!host(b, e))
                goto Error;

            // optional
            if (colon(b, e)) {
                if (port(b, e)) {
                    has_port = true;
                } else {
                    port.i = 0;
                }
            }

            return true;

          Error:
            b = b_old;
            return false;
        }

        UserInfo userinfo;
        bool has_userinfo;
        Host host;
        Int<uint16_t, 10, 1, 5> port;
        bool has_port;
    };

    struct Scheme : public ParserBase {
        // scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
        bool operator()(const char *& b, const char * e) {
            // Do not accept empty string
            if (b == e) return false;

            b_ = e_ = b;  // no range is captured

            // require to start with an ALPHA
            if (!Alpha::check(*b))
                return false;

            // Arbetarry long sequence may follow
            while (b != e) {
                if (Alpha::check(*b) || Digit::check(*b) ||
                        *b == '+' || *b == '-' || *b == '.') {
                    ++b;
                    continue;
                }

                break;
            }

            e_ = b;  // update captured range
            return true;
        }
        const char *b_, *e_;
    };

    struct HierPart : public ParserBase {
        // hier-part     = "//" authority path-abempty
        //               / path-absolute
        //               / path-rootless
        //               / path-empty
        bool operator()(const char *& b, const char * e) {
            b_ = e_ = b;  // no range is captured
            Lit slash_slash("//");

            path_end          = 0;
            path_begin        = 0;
            has_authority     = false;

#if defined(VTSS_RFC3986_STRICT_PATH_PARSER)
            // "//" authority path-abempty
            PathAbempty path_abempty;
            if (Group(b, e, slash_slash, authority, path_abempty)) {
                path_begin = path_abempty.b_;
                path_end   = path_abempty.e_;
                has_authority = true;
                e_ = b;  // update captured range
                return true;
            }

            // path-absolute
            PathAbsolute path_absolute;
            if (path_absolute(b, e)) {
                path_begin = path_absolute.b_;
                path_end   = path_absolute.e_;
                e_ = b;  // update captured range
                return true;
            }

            // path-rootless
            PathRootless path_rootless;
            if (path_rootless(b, e)) {
                path_begin = path_rootless.b_;
                path_end   = path_rootless.e_;
                e_ = b;  // update captured range
                return true;
            }

            // path-empty
            PathEmpty path_empty;
            if (path_empty(b, e)) {
                path_begin = b;
                path_end   = b;
                e_ = b;  // update captured range
                return true;
            }
#else
            // path-relaxed
            PathRelaxed path_relaxed;

            if (Group(b, e, slash_slash, authority, path_relaxed)) {
                path_begin = path_relaxed.b_;
                path_end   = path_relaxed.e_;
                has_authority = true;
                e_ = b;  // update captured range
                return true;
            }


            if (path_relaxed(b, e)) {
                path_begin = path_relaxed.b_;
                path_end   = path_relaxed.e_;
                e_ = b;  // update captured range
                return true;
            }
#endif


            return false;
        }

        const char *b_, *e_;
        Authority authority;
        bool has_authority;
        const char *path_begin, *path_end;
    };

    // URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
    bool operator()(const char *&b, const char * e) {
        Scheme _scheme;
        HierPart hier_part;
        FragmentQuery _query;
        FragmentQuery _fragment;

        Lit colon(":");
        port = -1;

        // scheme ":" hier-part
        if (!Group(b, e, _scheme, colon, hier_part)) {
            return false;
        }

        // update infor from scheme and hier_part
        scheme = str(_scheme.b_, _scheme.e_);
        if (hier_part.has_authority) {
            if (hier_part.authority.has_userinfo) {
                userinfo = str(hier_part.authority.userinfo.b_,
                               hier_part.authority.userinfo.e_);
            }

            if (hier_part.authority.has_port) {
                port = hier_part.authority.port.get();
            }

            host = str(hier_part.authority.host.b_,
                       hier_part.authority.host.e_);
        }

        path = str(hier_part.path_begin, hier_part.path_end);

        // [ "?" query ]
        Lit qmark("?");
        if (Group(b, e, qmark, _query)) {
            query = str(_query.b_, _query.e_);
        }

        // [ "#" fragment ]
        Lit hmark("#");
        if (Group(b, e, hmark, _fragment)) {
            fragment = str(_fragment.b_, _fragment.e_);
        }

        return true;
    }

    str host;
    str path;
    str query;
    str scheme;
    str fragment;
    str userinfo;
    int port;
};

struct InetAddress {
    typedef ::vtss::InetAddress value_type;

    bool operator()(const char *&b, const char *e) {
        return a.string_parse(b, e);
    }

    const value_type& get() const { return a; }
    vtss::InetAddress a;
};

template<typename T>
bool parser_cstr(const char *c, T &t) {
    const char *b = c;
    const char *e = c + strlen(c);
    return t(b, e);
}

#if 0
template<typename T>
struct IntMultiBase : public ParserBase {
    typedef T value_type;
    IntMultiBase() : i(0) { }

    bool operator()(const char *& b, const char * e) {
        Lit prefix_hex("0x");
        Lit prefix_oct("0o");
        Lit prefix_bin("0b");

        Int<T, 16> hex;
        Int<T, 8>  oct;
        Int<T, 2>  bin;
        Int<T, 10> dec;

       if (Group(b, e, prefix_hex, hex)) {
            i = hex.get();
            return true;
        }

        if (Group(b, e, prefix_oct, oct)) {
            i = oct.get();
            return true;
        }

        if (Group(b, e, prefix_bin, bin)) {
            i = bin.get();
            return true;
        }

        if (dec(b, e)) {
            i = dec.get();
            return true;
        }

        return false;
    }

    const value_type& get() const {
        return i;
    }

    T i;
};
#endif
}  // namespace parse
}  // namespace vtss

#endif  // __PARSER_IMPL_HXX__

