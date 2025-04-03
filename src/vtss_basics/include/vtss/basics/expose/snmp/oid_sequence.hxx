/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_OID_SEQUENCE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_OID_SEQUENCE_HXX__

#include <vtss/basics/stream.hxx>
#include <vtss/basics/api_types.h>
#include <vtss/basics/algorithm.hxx>
#include <vtss/basics/expose/snmp/vtss_oid.hxx>

namespace vtss {
namespace expose {
namespace snmp {

struct OidSequence {
    enum { max_length = 256 };
    uint32_t valid;
    vtss_oid oids[max_length];

    OidSequence() : valid(0) {}
    OidSequence(const vtss_oid *buf, size_t length) : valid(0) {
        copy_from(buf, length);
    }

    vtss_oid *data() { return oids; }
    vtss_oid *begin() { return oids; }
    vtss_oid *end() { return oids + valid; }
    void push_back(uint32_t i) { push(i); }
    void push(uint32_t i) {
        if ((valid + 1) >= max_length) return;
        oids[valid++] = i;
    }

    void push(const OidSequence &i);

    uint32_t pop_back() { return pop(); }
    uint32_t pop() {
        if (valid <= 0) return 0;
        return oids[--valid];
    }

    bool pop_head(uint32_t &h);

    bool operator==(const OidSequence &rhs) const;
    bool operator!=(const OidSequence &rhs) const { return !(operator==(rhs)); }
    bool operator<(const OidSequence &rhs) const;
    bool operator<=(const OidSequence &rhs) const;
    bool operator>(const OidSequence &rhs) const;
    bool operator>=(const OidSequence &rhs) const;
    OidSequence operator%(const OidSequence &rhs) const;
    OidSequence operator/(const OidSequence &rhs) const;
    bool isSubtreeOf(const OidSequence &oid) const;

    bool posible_next(const OidSequence &next) {
        return *this >= next || next.isSubtreeOf(*this);
    }

    template <typename T>
    uint32_t copy_to(T *buf) const {
        for (uint32_t i = 0; i < valid; ++i, ++buf) {
            *buf = oids[i];
        }
        return valid;
    }

    template <typename T, typename U>
    void copy_from(const T *buf, U length) {
        uint32_t l = min(static_cast<uint32_t>(max_length),
                         static_cast<uint32_t>(length));
        for (uint32_t i = 0; i < l; ++i, ++buf) {
            oids[i] = *buf;
        }
        valid = l;
    }

    void clear() { valid = 0; }

    uint32_t length() const { return valid; }
    uint32_t size() const { return valid; }
};

ostream &operator<<(ostream &o, const OidSequence &s);

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_OID_SEQUENCE_HXX__
