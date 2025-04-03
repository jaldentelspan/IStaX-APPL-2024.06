/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include "vtss/basics/trace_basics.hxx"

#include "vtss/basics/expose/snmp/handlers/oid_handler.hxx"
#include "vtss/basics/algorithm.hxx"

// NOTE: Bug#13246. For get next operations to work properly we must return MAX
// value in case of out-of-bound-overflows when the request is a getnext

namespace vtss {
namespace expose {
namespace snmp {

// TODO, delete
void serialize(OidImporter &a, uint32_t &s) {
    if (!a.ok_) return;

    if (!a.consume(s)) {
        a.flag_error();
        return;
    }
}

// TODO, delete
void serialize(OidImporter &a, uint16_t &s) {
    if (!a.ok_) return;

    uint32_t x;

    if (!a.consume(x)) {
        a.flag_error();
        return;
    }

    if (x >= 0xffffu) {  // Out of range
        if (a.next_request()) {
            s = 0xffffu;
            a.flag_overflow();
        } else {
            a.flag_error();
        }
        return;
    }

    s = static_cast<uint16_t>(x);
}

// TODO, delete
void serialize(OidImporter &a, uint8_t &s) {
    if (!a.ok_) return;

    uint32_t x;

    if (!a.consume(x)) {
        a.flag_error();
        return;
    }

    if (x >= 0xffu) {  // Out of range
        if (a.next_request()) {
            s = 0xffu;
            a.flag_overflow();
        } else {
            a.flag_error();
        }
        return;
    }

    s = static_cast<uint8_t>(x);
}

void serialize(OidImporter &a, int32_t &s) {
    if (!a.ok_) return;

    uint32_t x;
    if (!a.consume(x)) {
        a.flag_error();
        return;
    }

    if (x > 0x7fffffffu) {  // Out of range
        if (a.next_request()) {
            s = 0x7fffffffu;
            a.flag_overflow();
        } else {
            a.flag_error();
        }
        return;
    }

    s = static_cast<int32_t>(x);
}

void serialize(OidImporter &a, AsInt &s) {
    uint32_t x;
    uint32_t max_value = 0x7fffffffu;

    if (!a.consume(x)) {
        a.flag_error();
        return;
    }

    if (s.val_uint8)
        max_value = 0xff;
    else if (s.val_uint16)
        max_value = 0xffff;
    else if (s.val_uint32)
        max_value = 0x7fffffff;

    if (x > max_value) {  // Out of range
        if (a.next_request()) {
            s.set_value(max_value);
            a.flag_overflow();
        } else {
            a.flag_error();
        }
        return;
    }

    s.set_value(x);
}

void serialize(OidImporter &a, mesa_mac_t &s) {
    // get-next
    // [] -> error
    // [1] -> error
    // [1.2] -> error
    // [1.2.3.4.5.6] -> 1.2.3.4.5.6
    // [1.0.300.0.1.2] -> 1.0.255.255.255.255


    mesa_mac_t mac = {{0, 0, 0, 0, 0, 0}};

    if (!a.ok_) return;

    for (uint32_t count = 0; count < 6; count++) {
        uint32_t x;

        // pop one octet, or default to zero
        if (!a.consume(x)) {
            a.flag_error();
            return;
        }

        // Do not allow octets greater than 255 when parsing mac-addresses
        if (x > 255) {
            // Even though this is a failure then we must still consume 6 octes
            // from the input OID sequence
            if (a.next_request()) {
                x = 255;
                a.flag_overflow();

            } else {
                // Normal get operations should just be terminated here with an
                // error code.
                a.flag_error();
                return;
            }
        }

        mac.addr[count] = x;
    }
    s = mac;
}

void serialize(OidImporter &a, mesa_ipv6_t &s) {
    // get-next
    // [] -> error
    // [1] -> error
    // [1.2] -> error
    // [1.2.3.4.5.6.7.8] -> 1.2.3.4.5.6
    // [1.2.3000000.4.5.6.7.8] ->
    //             1.2.0xffff.0xffff.0xffff.0xffff.0xffff.0xffff.0xffff.0xffff

    mesa_ipv6_t ipv6 = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    if (!a.ok_) return;

    for (uint32_t count = 0; count < 16; count++) {
        uint32_t x;

        // pop one octet, or default to zero
        if (!a.consume(x)) {
            a.flag_error();
            return;
        }

        if (x > 0xffff) {
            // Even though this is a failure then we must still consume 6 octes
            // from the input OID sequence
            if (a.next_request()) {
                x = 0xffff;
                a.flag_overflow();
            } else {
                // Normal get operations should just be terminated here with an
                // error code.
                a.flag_error();
                return;
            }
        }

        ipv6.addr[count] = x;
    }
    s = ipv6;
}

void serialize(OidImporter &a, Ipv4Address &s) {

    // get
    // [] -> error
    // [10] -> error
    // [10.1] -> error
    // [10.1.2.3] -> 10.1.2.3
    // [10.300.0.1] -> error

    // get-next
    // [] ->  nullptr
    // [10] -> [9.255.255.255]
    // [10.300.0.1] -> 10.255.255.255
    uint32_t ipv4 = 0;

    if (!a.ok_) return;

    for (uint32_t count = 0; count < 4; count++) {
        uint32_t x;

        // pop one octet, or default to zero
        if (!a.consume(x)) {
            a.flag_error();
            return;
        }

        // Do not allow octets greater than 255 when parsing ipv4
        if (x > 255) {
            // Even though this is a failure then we must still consume 4 octes
            // from the input OID sequence
            if (a.next_request()) {
                x = 255;
                a.flag_overflow();
            } else {
                // Normal get operations should just be terminated here with an
                // error code.
                a.flag_error();
                return;
            }
        }

        ipv4 <<= 8;
        ipv4 |= x;
    }

    s = Ipv4Address(ipv4);
}

void serialize(OidImporter &h, AsIpv4 &s) {
    Ipv4Address ipv4;
    serialize(h, ipv4);

    if (h.ok_) s.t = ipv4.as_api_type();
}

void serialize_enum(OidImporter &h, int32_t &i,
                    const vtss_enum_descriptor_t *d) {
    // If no exact match is found, return the lower bound (the highest number
    // which is still less than the query), and flag overflow!.
    //
    // We must return the lower-bound because the iterator will do the
    // increment, not the parser.
    //
    // We must flag overflow because the following keys must wrap-around.
    uint32_t query;
    uint32_t max = 0;
    bool max_valid = false;

    uint32_t exact_match = 0;
    bool exact_match_valid = false;

    uint32_t lower_bound = 0;
    bool lower_bound_valid = false;

    // Flag error if no query is found.
    if (!h.consume(query)) {
        h.flag_error();
        return;
    }

    VTSS_BASICS_TRACE(NOISE) << "Query: " << query;

    for (; d->valueName; d++) {
        // negative indexes are not allowed, they should not occure.
        if (d->intValue < 0) continue;

        // cast is safe as we just checked for negative values
        uint32_t candidate = static_cast<uint32_t>(d->intValue);

        // find the max value (must be >= as we want to get max_valid set to
        // true when we hit candidate==0)
        if (candidate >= max) {
            max = candidate;
            max_valid = true;
        }

        // break the search if we found an exact match
        if (query == candidate) {  // exact match
            exact_match = candidate;
            exact_match_valid = true;
            break;
        }

        // we are only in a lower bound!
        if (candidate > query) continue;

        // check if we found a better match
        if ((query - candidate) <= (query - lower_bound)) {
            lower_bound = candidate;
            lower_bound_valid = true;
        }
    }

    if (!max_valid) {
        VTSS_BASICS_TRACE(ERROR)
                << "Only negative values found in enum "
                   "descriptor! negative values are not allowed as indexes";
    }

    // If we found an exact match then return it!
    if (exact_match_valid) {
        i = exact_match;
        VTSS_BASICS_TRACE(NOISE) << "Exact match: " << i;
        return;
    }

    if (!h.next_request()) {
        h.flag_error();  // get exact must have exact matches!
        i = 0;
        VTSS_BASICS_TRACE(NOISE) << "Error get exact: " << i;
        return;
    }

    // Handling get_next ....
    h.flag_overflow();

    // A lower_bound is just as good as an exact match in a get_next request!
    // Next step is to call the iterator, which will find the next value.
    if (lower_bound_valid) {
        i = lower_bound;
        VTSS_BASICS_TRACE(NOISE) << "Lower bound: " << i;
        return;
    }

    // No lower_bound was found
    i = max;  // max is 0 if all entries in the enum descriptor are negative!
    VTSS_BASICS_TRACE(NOISE) << "Overflow: " << i;
}

void serialize(OidImporter &a, AsRowEditorState &s) { serialize(a, s.val); }

void serialize(OidImporter &a, AsPercent &s) { serialize(a, s.val); }

void serialize(OidImporter &a, AsDisplayString &s) {
    // [] -> ""  TODO, not true for get-next!
    // [0] -> ""
    // [1.32] -> " "
    // [1.127] -> error

    char dispString[256];

    VTSS_ASSERT(s.size_ < 256);

    if (!a.ok_) return;

    uint32_t stringLength;
    // First consume the length of the string, legal range 0-255
    //
    // In case of overflow this will consume all the remaining OID elements
    // (which is okay as the following serializer is also in overflow mode).
    if (!a.consume(stringLength)) {
        a.flag_error();
        return;
    }

    // Check if string length is smaller than allowed.
    if (stringLength < s.min_) {
        a.flag_error();
        return;
    }

    // If index string is longer than allowed, then it is an overflow.
    if (stringLength > s.size_) {
        if (a.next_request()) {
            a.flag_overflow();
        } else {
            a.flag_error();
            return;
        }
    }

    uint32_t count;
    bool underflow = false;
    for (count = 0; count < stringLength && count <= 255; count++) {
        uint32_t x;

        // pop one octet, or default to zero
        if (!a.consume(x)) {
            if (a.next_request()) {
                x = 31;
            } else {
                a.flag_error();
                return;
            }
        }
        // Only allow octets in the range 32-126
        if (x < 32 || x > 126 || underflow) {
            // Even though this is a failure then we must still consume
            // stringLength octes from the input OID sequence
            if (a.next_request() && x > 126) {
                x = 126;
                a.flag_overflow();
            } else if (a.next_request() && x < 32) {
                x = 31;
                underflow = true;
            } else {

                // Normal get operations should just be terminated here with an
                // error code.
                a.flag_error();
                return;
            }
        }
        dispString[count] = x;
    }

    // Fill the rest of the string with NULL
    for (count = stringLength; count < s.size_; count++) {
        dispString[count] = 0;
    }

    memcpy(s.ds_, dispString, s.size_);
}

void serialize_binary_length_(OidImporter &a, BinaryLen &s, uint32_t length) {
    VTSS_BASICS_TRACE(NOISE) << "Length: " << length;
    for (uint32_t i = 0; i < length; ++i) {
        uint32_t x;

        // pop one octet, or default to zero
        if (!a.consume(x)) {
            VTSS_BASICS_TRACE(NOISE) << "Consume error";
            a.flag_error();
            return;
        }

        // Do not allow octets greater than 255
        if (x > 255) {
            VTSS_BASICS_TRACE(NOISE) << "Overflow! x=" << x;
            // Even though this is a failure then we must still consume "length"
            // octes from the input OID sequence
            if (a.next_request()) {
                x = 255;
                a.flag_overflow();
            } else {
                // Normal get operations should just be terminated here with an
                // error code.
                a.flag_error();
                return;
            }
        }

        // better be safe
        if (i < s.max_len) {
            VTSS_BASICS_TRACE(NOISE) << "Push idx=" << i << " x=" << x;
            s.buf[i] = x;
            s.valid_len = i + 1;
        } else {
            VTSS_BASICS_TRACE(ERROR) << "Buffer overflow!" << i;
        }
    }

    for (uint32_t i = s.valid_len; i < s.min_len; ++i) {
        if (i < s.max_len) {
            VTSS_BASICS_TRACE(NOISE) << "Fill idx=" << i << " x=0";
            s.buf[i] = 0;
            s.valid_len = i + 1;
        }
    }

    if (s.valid_len < s.min_len || s.valid_len > s.max_len) {
        VTSS_BASICS_TRACE(ERROR) << "Something is wrong! s.valid_len="
                                 << s.valid_len << " s.min_len=" << s.min_len
                                 << " s.max_len=" << s.max_len;
    }
}

void serialize(OidImporter &a, BinaryLen &s) {
    if (!a.ok_) {
        VTSS_BASICS_TRACE(NOISE) << "Not okay";
        return;
    }

    uint32_t length = 0;
    if (s.min_len == s.max_len) {
        // Fixed length object string. The octet sequence will not start with
        // the length.
        length = s.min_len;

    } else {
        // Variable length octet string. First consume the length of the string,
        // legal range 0-255
        //
        // In case of overflow this will consume all the remaining OID elements
        // (which is okay as the following serializer is also in overflow mode).
        if (!a.consume(length)) {
            VTSS_BASICS_TRACE(NOISE) << "Consume error";
            a.flag_error();
            return;
        }

        // If index string is longer than allowed, then it is an overflow.
        if (length > s.max_len) {
            VTSS_BASICS_TRACE(NOISE) << "Overflow: " << length;
            if (a.next_request()) {
                a.flag_overflow();
                length = s.max_len;
            } else {
                a.flag_error();
                return;
            }
        }
    }

    VTSS_BASICS_TRACE(NOISE) << "Length: " << length;
    serialize_binary_length_(a, s, length);
}

void serialize_oid_length_(OidImporter &a, AsSnmpObjectIdentifier &s, uint32_t length) {
    VTSS_BASICS_TRACE(NOISE) << "Length: " << length;
    for (uint32_t i = 0; i < length; ++i) {
        uint32_t x;

        // pop one octet, or default to zero
        if (!a.consume(x)) {
            VTSS_BASICS_TRACE(NOISE) << "Consume error";
            a.flag_error();
            return;
        }

        // better be safe
        if (i < s.max_length) {
            VTSS_BASICS_TRACE(NOISE) << "Push idx=" << i << " x=" << x;
            s.buf[i] = x;
            s.length = i + 1;
        } else {
            VTSS_BASICS_TRACE(ERROR) << "Buffer overflow!" << i;
        }
    }

    if (s.length > s.max_length) {
        VTSS_BASICS_TRACE(ERROR) << "Something is wrong! s.length="
                                 << s.length << " s.max_length=" << s.max_length;
    }
}

void serialize(OidImporter &a, AsSnmpObjectIdentifier &s) {
    if (!a.ok_) {
        VTSS_BASICS_TRACE(NOISE) << "Not okay";
        return;
    }

    uint32_t length = 0;
    // Variable length octet string. First consume the length of the string,
    // legal range 0-255
    //
    // In case of overflow this will consume all the remaining OID elements
    // (which is okay as the following serializer is also in overflow mode).
    if (!a.consume(length)) {
        VTSS_BASICS_TRACE(NOISE) << "Consume error";
        a.flag_error();
        return;
    }

    // If index string is longer than allowed, then it is an overflow.
    if (length > s.max_length) {
        VTSS_BASICS_TRACE(NOISE) << "Overflow: " << length;
        if (a.next_request()) {
            a.flag_overflow();
            length = s.max_length;
        } else {
            a.flag_error();
            return;
        }
    }

    VTSS_BASICS_TRACE(NOISE) << "Length: " << length;
    serialize_oid_length_(a, s, length);
}

// TODO, delete
void serialize(OidExporter &a, uint32_t &s) { a.oids_.push(s); }

// TODO, delete
void serialize(OidExporter &a, uint16_t &s) { a.oids_.push(s); }

// TODO, delete
void serialize(OidExporter &a, uint8_t &s) { a.oids_.push(s); }

void serialize(OidExporter &a, int32_t &s) {
    if (s < 0) {
        a.flag_error();
        return;
    }

    uint32_t u = static_cast<uint32_t>(s);
    serialize(a, u);
}

void serialize(OidExporter &a, AsInt &s) {
    uint32_t x = s.get_value();
    int32_t y;

    if (x > 0x7fffffff) {
        VTSS_BASICS_TRACE(ERROR) << "Value too big: " << x;
        x = 0x7fffffff;
    }

    y = (int32_t)x;
    serialize(a, y);
}

void serialize(OidExporter &a, mesa_mac_t &s) {
    if (!a.ok_) return;

    a.oids_.push(s.addr[0]);
    a.oids_.push(s.addr[1]);
    a.oids_.push(s.addr[2]);
    a.oids_.push(s.addr[3]);
    a.oids_.push(s.addr[4]);
    a.oids_.push(s.addr[5]);
}

void serialize(OidExporter &a, mesa_ipv6_t &s) {
    if (!a.ok_) return;

    a.oids_.push(s.addr[0]);
    a.oids_.push(s.addr[1]);
    a.oids_.push(s.addr[2]);
    a.oids_.push(s.addr[3]);
    a.oids_.push(s.addr[4]);
    a.oids_.push(s.addr[5]);
    a.oids_.push(s.addr[6]);
    a.oids_.push(s.addr[7]);
    a.oids_.push(s.addr[8]);
    a.oids_.push(s.addr[9]);
    a.oids_.push(s.addr[10]);
    a.oids_.push(s.addr[11]);
    a.oids_.push(s.addr[12]);
    a.oids_.push(s.addr[13]);
    a.oids_.push(s.addr[14]);
    a.oids_.push(s.addr[15]);
}

void serialize_enum(OidExporter &h, int32_t &i,
                    const vtss_enum_descriptor_t *d) {
    if (i < 0) {
        h.flag_error();
        return;
    }

    uint32_t u = static_cast<uint32_t>(i);
    serialize(h, u);
}

void serialize(OidExporter &a, Ipv4Address &s) {
    if (!a.ok_) return;

    uint32_t x = s.as_api_type();
    a.oids_.push((x & 0xff000000) >> 24);
    a.oids_.push((x & 0xff0000) >> 16);
    a.oids_.push((x & 0xff00) >> 8);
    a.oids_.push((x & 0xff));
}

void serialize(OidExporter &a, AsIpv4 &s) {
    if (!a.ok_) return;

    a.oids_.push((s.t & 0xff000000) >> 24);
    a.oids_.push((s.t & 0xff0000) >> 16);
    a.oids_.push((s.t & 0xff00) >> 8);
    a.oids_.push((s.t & 0xff));
}

void serialize(OidExporter &a, AsRowEditorState &s) { serialize(a, s.val); }

void serialize(OidExporter &a, AsPercent &s) { serialize(a, s.val); }

void serialize(OidExporter &a, AsDisplayString &s) {
    if (!a.ok_) return;

    uint32_t length = vtss::min(s.size_, (uint32_t)(strlen(s.ds_)));

    a.oids_.push(length);
    for (uint32_t i = 0; i < length; ++i) {
        a.oids_.push(s.ds_[i]);
    }
}

void serialize(OidExporter &a, BinaryLen &s) {
    if (!a.ok_) return;

    uint32_t length = 0;
    if (s.max_len == s.min_len) {
        length = s.min_len;
    } else {
        length = vtss::min(s.valid_len, s.max_len);
        a.oids_.push(length);
    }

    for (uint32_t i = 0; i < length; ++i) {
        a.oids_.push(s.buf[i]);
    }
}

void serialize(OidExporter &a, AsSnmpObjectIdentifier &s) {
    if (!a.ok_) return;

    uint32_t length = 0;
    length = vtss::min(s.length, s.max_length);
    a.oids_.push(length);

    for (uint32_t i = 0; i < length; ++i) {
        a.oids_.push(s.buf[i]);
    }
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
