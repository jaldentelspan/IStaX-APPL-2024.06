/* *****************************************************************************
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 **************************************************************************** */

#include "vtss/basics/print_fmt.hxx"
#include "vtss/basics/stream.hxx"
#include "vtss/basics/type_traits.hxx"

namespace vtss {

template <typename T>
void append_sign(vtss::ostream &stream, const Fmt &fmt, const T p) {
    if (fmt.signed_flag == ' ' && p > 0) {
        stream.push(' ');
    }
    if (fmt.signed_flag == '+' && p > 0) {
        stream.push('+');
    }
}

template <typename T>
size_t fmt_impl(vtss::ostream &stream, const Fmt &fmt, const T p) {
    vtss::ReverseBufStream<SBuf32> buf;
    switch (fmt.encoding_flag) {
    case 'i':
    case 'd': {
        vtss::signed_to_dec_rbuf(
                static_cast<typename vtss::make_signed<T>::type>(p), buf);
        append_sign(buf, fmt,
                    static_cast<typename vtss::make_signed<T>::type>(p));
        break;
    }
    case 'u': {
        vtss::unsigned_to_dec_rbuf(
                static_cast<typename vtss::make_unsigned<T>::type>(p), buf);
        break;
    }
    case 'x': {
        vtss::unsigned_to_hex_rbuf(
                static_cast<typename vtss::make_unsigned<T>::type>(p), buf, 'a');
        break;
    }
    case 'X': {
        vtss::unsigned_to_hex_rbuf(
                static_cast<typename vtss::make_unsigned<T>::type>(p), buf, 'A');
        break;
    }
    case 'o': {
        vtss::unsigned_to_oct_rbuf(
                static_cast<typename vtss::make_unsigned<T>::type>(p), buf);
        break;
    }
    case 's':
    default: {
        vtss::signed_to_dec_rbuf(p, buf);
        append_sign(buf, fmt, p);
    }
    }
    return stream.write(buf.begin(), buf.end());
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const unsigned char *p) {
    return fmt_impl<uint32_t>(stream, fmt, *p);
}
size_t fmt(vtss::ostream &stream, const Fmt &fmt, const unsigned short *p) {
    return fmt_impl<uint32_t>(stream, fmt, *p);
}
size_t fmt(vtss::ostream &stream, const Fmt &fmt, const unsigned int *p) {
    return fmt_impl<uint32_t>(stream, fmt, *p);
}
size_t fmt(vtss::ostream &stream, const Fmt &fmt, const unsigned long *p) {
    return fmt_impl<uint64_t>(stream, fmt, *p);
}
size_t fmt(vtss::ostream &stream, const Fmt &fmt,
           const unsigned long long int *p) {
    return fmt_impl<uint64_t>(stream, fmt, *p);
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const signed char *p) {
    return fmt_impl<int32_t>(stream, fmt, *p);
}
size_t fmt(vtss::ostream &stream, const Fmt &fmt, const signed short *p) {
    return fmt_impl<int32_t>(stream, fmt, *p);
}
size_t fmt(vtss::ostream &stream, const Fmt &fmt, const signed int *p) {
    return fmt_impl<int32_t>(stream, fmt, *p);
}
size_t fmt(vtss::ostream &stream, const Fmt &fmt, const signed long *p) {
    return fmt_impl<int64_t>(stream, fmt, *p);
}
size_t fmt(vtss::ostream &stream, const Fmt &fmt, const signed long long int *p) {
    return fmt_impl<int64_t>(stream, fmt, *p);
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const bool *p) {
    return fmt_impl<int32_t>(stream, fmt, *p);
}

void create_format(char *format, size_t len, const Fmt &fmt) {
    BufPtr buf(format, format + len);
    vtss::BufPtrStream stream(&buf);

    stream << "%";
    if (fmt.precision != 0) {
        stream << "." << fmt.precision;
    }
    stream << fmt.encoding_flag;
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const double *p) {
    char format[64];
    memset(format, '\0', 64);
    create_format(format, 64, fmt);

    char buf[64];
    int res = snprintf(buf, 64, format, *p);
    return stream.write(buf, buf + vtss::min(res, 64));
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const intptr_t **p) {
    switch (fmt.encoding_flag) {
    case 'p':
    default:
        vtss::ReverseBufStream<SBuf32> buf;
        vtss::unsigned_to_hex_rbuf((intptr_t)(*p), buf, 'a');
        buf.push('x');
        buf.push('0');
        return stream.write(buf.begin(), buf.end());
    }
    return 0;
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const char **p) {
    switch (fmt.encoding_flag) {
    case 's': {
        str ss(*p);
        return stream.write(ss.begin(), ss.end());
    }
    case 'p': {
        vtss::ReverseBufStream<SBuf32> buf;
        vtss::unsigned_to_hex_rbuf((intptr_t)(*p), buf, 'a');
        buf.push('x');
        buf.push('0');
        return stream.write(buf.begin(), buf.end());
    }
    }
    return 0;
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const char *p) {
    switch (fmt.encoding_flag) {
    case 's': {
        str ss(p);
        return stream.write(ss.begin(), ss.end());
    }
    case 'c': {
        stream.push(*p);
        return 1;
    }
    case 'd':
    case 'i': {
        vtss::ReverseBufStream<SBuf32> buf;
        vtss::signed_to_dec_rbuf(*p, buf);
        append_sign(buf, fmt, *p);
        return stream.write(buf.begin(), buf.end());
    }
    case 'u': {
        vtss::ReverseBufStream<SBuf32> buf;
        vtss::unsigned_to_dec_rbuf(static_cast<uint32_t>(*p), buf);
        return stream.write(buf.begin(), buf.end());
    }
    case 'x': {
        vtss::ReverseBufStream<SBuf32> buf;
        vtss::unsigned_to_hex_rbuf(static_cast<uint32_t>(*p), buf, 'a');
        return stream.write(buf.begin(), buf.end());
    }
    case 'X': {
        vtss::ReverseBufStream<SBuf32> buf;
        vtss::unsigned_to_hex_rbuf(static_cast<uint32_t>(*p), buf, 'A');
        return stream.write(buf.begin(), buf.end());
    }
    case 'o': {
        vtss::ReverseBufStream<SBuf32> buf;
        vtss::unsigned_to_oct_rbuf(static_cast<uint32_t>(*p), buf);
        return stream.write(buf.begin(), buf.end());
    }
    }
    return 0;
}

size_t fmt_helper(vtss::ostream &stream, Ipv4Address val) {
    stream << ((val.as_api_type() >> 24) & 0xff) << '.'
           << ((val.as_api_type() >> 16) & 0xff) << '.'
           << ((val.as_api_type() >> 8) & 0xff) << '.'
           << (val.as_api_type() & 0xff);
    return 11;
}

namespace {
template <typename T>
FormatHex<T> hex__(T &t) {
    return FormatHex<T>(t, 'a', 0, 0, '0');
}

size_t fmt_helper(vtss::ostream &stream, const int32_t val) {
    ReverseBufStream<SBuf16> buf;
    signed_to_dec_rbuf(val, buf);
    return stream.write(buf.begin(), buf.end());
}

size_t fmt_helper(vtss::ostream &stream, const char *str, size_t size) {
    return stream.write(str, str + size);
}

size_t fmt_helper(vtss::ostream &stream, const mesa_ipv6_t p) {
    uint16_t ip[8];
    uint16_t *iter = ip, *end = ip + 8;
    uint16_t *zero_begin = 0, *zero_end = 0;
    BufStream<SBuf64> o;

    // Copy to an array of shorts
    for (int i = 0; i < 8; ++i) ip[i] = p.addr[2 * i] << 8 | p.addr[2 * i + 1];

    // Find the longest sequence of zero's
    while (iter != end) {
        uint16_t *b = find(iter, end, 0);
        uint16_t *e = find_not(b, end, 0);

        if (b == end)  // no zero group found
            break;

        iter = e;  // next search should start where this ended
        if ((zero_end - zero_begin) < (e - b)) {
            zero_end = e;
            zero_begin = b;
        }
    }

    // Print the IPv6 address
    if (zero_begin == zero_end) {
        join(o, ":", ip, end, &hex__<uint16_t>);  // regular join-print

    } else if (zero_begin == ip && zero_end == ip + 6) {  // Print as ipv4
        o << "::";
        uint32_t _v4 = ip[6];
        _v4 <<= 16;
        _v4 |= ip[7];
        Ipv4Address v4(_v4);
        o << v4;

    } else if (zero_begin == ip && zero_end == ip + 5 && ip[5] == 0xffff) {
        // Print as ipv4 (almost )
        o << "::";
        o << hex(ip[5]) << ":";
        uint32_t _v4 = ip[6];
        _v4 <<= 16;
        _v4 |= ip[7];
        Ipv4Address v4(_v4);
        o << v4;

    } else {
        join(o, ":", ip, zero_begin, &hex__<uint16_t>);  // before zero-sequence
        if (zero_begin == &ip[0] && zero_end == &ip[1])
            o << "0:";
        else if (zero_begin == &ip[7] && zero_end == &ip[8])
            o << ":0";
        else
            o << "::";
        join(o, ":", zero_end, end, &hex__<uint16_t>);  // after zero-sequence
    }
    return stream.write(&*o.begin(), &*o.end());
}
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const str *p) {
    return stream.write(&*p->begin(), &*p->end());
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const Buf *p) {
    return stream.write(&*p->begin(), &*p->end());
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const mesa_mac_t *p) {
    stream << HEX_fixed<2>(p->addr[0]) << ":" << HEX_fixed<2>(p->addr[1]) << ":"
           << HEX_fixed<2>(p->addr[2]) << ":" << HEX_fixed<2>(p->addr[3]) << ":"
           << HEX_fixed<2>(p->addr[4]) << ":" << HEX_fixed<2>(p->addr[5]);
    return 17;
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const mesa_ipv6_t *p) {
    return fmt_helper(stream, *p);
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const mesa_ipv4_network_t *p) {
    size_t res = 0;
    res += fmt_helper(stream, Ipv4Address(p->address));
    res += fmt_helper(stream, "/", 1);
    res += fmt_helper(stream, static_cast<int32_t>(p->prefix_size));
    return res;
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const mesa_ipv6_network_t *p) {
    size_t res = 0;
    res += fmt_helper(stream, p->address);
    res += fmt_helper(stream, "/", 1);
    res += fmt_helper(stream, static_cast<int32_t>(p->prefix_size));
    return res;
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const mesa_ip_addr_t *p) {
    switch (p->type) {
    case MESA_IP_TYPE_NONE:
        return fmt_helper(stream, "<NONE>", 6);

    case MESA_IP_TYPE_IPV4:
        return fmt_helper(stream, Ipv4Address(p->addr.ipv4));

    case MESA_IP_TYPE_IPV6:
        return fmt_helper(stream, p->addr.ipv6);

    default:
        return fmt_helper(stream, "<Invalid>", 9);
    }
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const mesa_ip_network_t *p) {
    switch (p->address.type) {
    case MESA_IP_TYPE_NONE:
        return fmt_helper(stream, "<NONE>", 6);

    case MESA_IP_TYPE_IPV4: {
        size_t res = 0;
        res += fmt_helper(stream, Ipv4Address(p->address.addr.ipv4));
        res += fmt_helper(stream, "/", 1);
        res += fmt_helper(stream, static_cast<int32_t>(p->prefix_size));
        return res;
    }

    case MESA_IP_TYPE_IPV6: {
        size_t res = 0;
        res += fmt_helper(stream, p->address.addr.ipv6);
        res += fmt_helper(stream, "/", 1);
        res += fmt_helper(stream, static_cast<int32_t>(p->prefix_size));
        return res;
    }

    default:
        return fmt_helper(stream, "<Invalid>", 9);
    }
}

size_t fmt(vtss::ostream &stream, const Fmt &fmt, const vtss_inet_address_t *p) {
    switch (p->type) {
    case VTSS_INET_ADDRESS_TYPE_NONE:
        return fmt_helper(stream, "<no-address>", 12);

    case VTSS_INET_ADDRESS_TYPE_IPV4: {
        return fmt_helper(stream, Ipv4Address(p->address.ipv4));
    }

    case VTSS_INET_ADDRESS_TYPE_IPV6:
        return fmt_helper(stream, p->address.ipv6);

    case VTSS_INET_ADDRESS_TYPE_DNS: {
        const char *e = vtss::find(p->address.domain_name.name,
                                   p->address.domain_name.name + 254, '\0');
        return fmt_helper(stream, p->address.domain_name.name,
                          e - p->address.domain_name.name);
    }

    default: {
        size_t res = 0;
        res += fmt_helper(stream, "<invalid-address-type:", 22);
        res += fmt_helper(stream, static_cast<int32_t>(p->type));
        res += fmt_helper(stream, ">", 1);
        return res;
    }
    }
}

size_t fmt(vtss::ostream &stream, const vtss::Fmt &fmt, const mesa_port_list_t *ports) {
    size_t length = 0;
    for (size_t idx = 0; idx < ports->size(); ++idx) {
        if (idx > 0 && (idx & 0x3) == 0) {
            stream << ".";
            ++length;
        }
        if (ports->get(idx)) {
            stream << "1";
        } else {
            stream << "0";
        }
        ++length;
    }
    return length;
}

struct GlobalParser {
    enum class FieldWidthType { Next, Length };

    char flag = ' ';  // '0', '-'
    FieldWidthType field_width_type;
    size_t length = {0};
    Fmt fmt;
};

void parse_flags(const char *&format, GlobalParser &result) {
    // ignore for now
    if (*format == '#' || *format == '\'') ++format;

    // parse global flags
    if (*format == '0' || *format == '-') {
        result.flag = *format;
        ++format;
    }

    // parse local flags
    if (*format == ' ' || *format == '+') {
        result.fmt.signed_flag = *format;
        ++format;
    }
}

void parse_field_width(const char *&format, GlobalParser &result) {
    if (*format == '*') {
        result.field_width_type = GlobalParser::FieldWidthType::Next;
        ++format;
        return;
    }

    size_t length = 0;
    while (*format >= '0' && *format <= '9') {
        length = length * 10 + (*format - '0');
        ++format;
    }
    result.field_width_type = GlobalParser::FieldWidthType::Length;
    result.length = length;
}

void parse_length_modifier(const char *&format, GlobalParser &result) {
    // parse length modifier
    while (*format == 'l' || *format == 'h' || *format == 'L' ||
           *format == 'j' || *format == 'z' || *format == 't')
        ++format;
}

void parse_precision(const char *&format, GlobalParser &result) {
    if (*format == '.') {
        ++format;
        size_t length = 0;
        while (*format >= '0' && *format <= '9') {
            length += length * 10 + (*format - '0');
            ++format;
        }
        result.fmt.precision = length;
    }
}

GlobalParser parse_flags(const char *&format, void *data_args[], int &argc) {
    GlobalParser result;

    parse_flags(format, result);
    parse_field_width(format, result);
    parse_length_modifier(format, result);
    parse_precision(format, result);

    // parse format, currently can be any
    result.fmt.encoding_flag = *format;
    ++format;

    // update length in case parse_length_modifier couldn't parse it.
    if (result.field_width_type != GlobalParser::FieldWidthType::Length) {
        result.length = *(int *)data_args[argc++];
    }

    return result;
}

void append_result(vtss::ostream &stream, const GlobalParser &parser,
                   vtss::ostreamBuf &tmp) {
    size_t tmp_length = tmp.end() - tmp.begin();
    if (tmp_length < parser.length) {
        switch (parser.flag) {
        case '-': {
            stream.write(tmp.begin(), tmp.end());
            stream.fill(parser.length - tmp_length, ' ');
            break;
        }
        case '0': {
            // in case there is a sign, it has to come first
            auto it = tmp.begin();
            if (*it == '+' || *it == '-') {
                stream.push(*tmp.begin());
                ++it;
            }
            stream.fill(parser.length - tmp_length, parser.flag);
            stream.write(it, tmp.end());
            break;
        }
        case ' ': {
            stream.fill(parser.length - tmp_length, parser.flag);
            stream.write(tmp.begin(), tmp.end());
            break;
        }
        }
    } else {
        stream.write(tmp.begin(), tmp.end());
    }
}

vtss::ostream &print_fmt_impl(vtss::ostream &result, const char *format,
                              int argc_count, void *func_args[],
                              void *data_args[]) {
    using to_str_ptr_t = size_t (*)(vtss::ostream &, const Fmt &, const void *);
    int argc = 0;

    while (*format != '\0') {
        if (*format != '%') {
            result.push(*format);
            ++format;
            continue;
        }

        ++format;
        if (*format == '%') {
            result.push(*format);
            ++format;
            continue;
        }

        GlobalParser parser = parse_flags(format, data_args, argc);
        to_str_ptr_t p = (to_str_ptr_t)func_args[argc];
        if (parser.flag == '-') {
            size_t written = p(result, parser.fmt, data_args[argc]);
            if (written < parser.length) {
                result.fill(parser.length - written, ' ');
            }
        } else {
            vtss::StringStream tmp;
            p(tmp, parser.fmt, data_args[argc]);
            append_result(result, parser, tmp);
        }
        ++argc;
    }

    return result;
}

size_t print_fmt_impl(char *dst, size_t len, const char *format, int argc_count,
                      void *func_args[], void *data_args[]) {
    if (!dst || len == 0) {
        return 0;
    }

    BufPtr buf(dst, dst + len);
    vtss::BufPtrStream stream(&buf);
    print_fmt_impl(stream, format, argc_count, func_args, data_args);
    stream.push('\0');
    dst[len - 1] = '\0';
    return stream.size() - 1;
}

}  // namespace vtss
