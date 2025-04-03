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

#include "vtss/basics/expose/snmp/oid_sequence.hxx"

namespace vtss {
namespace expose {
namespace snmp {

bool OidSequence::pop_head(uint32_t &r) {
    if (valid <= 0)
        return false;

    --valid;
    r = oids[0];

    for (uint32_t i = 0; i < valid; ++i)
        oids[i] = oids[i + 1];

    return true;
}

void OidSequence::push(const OidSequence &in) {
    for (uint32_t i = 0; i < in.valid; ++i)
        push(in.oids[i]);
}

bool OidSequence::operator==(const OidSequence &rhs) const {
    if (valid != rhs.valid)
        return false;

    for (uint32_t i = 0; i < valid; ++i)
        if (oids[i] != rhs.oids[i])
            return false;

    return true;
}

bool OidSequence::operator<(const OidSequence &rhs) const {
    for (uint32_t i = 0; i < min(valid, rhs.valid); ++i) {
        if (oids[i] == rhs.oids[i])
            continue;

        return oids[i] < rhs.oids[i];
    }

    return valid < rhs.valid;
}

bool OidSequence::operator<=(const OidSequence &rhs) const {
    for (uint32_t i = 0; i < min(valid, rhs.valid); ++i) {
        if (oids[i] == rhs.oids[i])
            continue;

        return oids[i] < rhs.oids[i];
    }

    return valid <= rhs.valid;
}

bool OidSequence::operator>(const OidSequence &rhs) const {
    for (uint32_t i = 0; i < min(valid, rhs.valid); ++i) {
        if (oids[i] == rhs.oids[i])
            continue;

        return oids[i] > rhs.oids[i];
    }

    return valid > rhs.valid;
}

bool OidSequence::operator>=(const OidSequence &rhs) const {
    for (uint32_t i = 0; i < min(valid, rhs.valid); ++i) {
        if (oids[i] == rhs.oids[i])
            continue;

        return oids[i] > rhs.oids[i];
    }

    return valid >= rhs.valid;
}

OidSequence OidSequence::operator%(const OidSequence &rhs) const {
    OidSequence res;
    if (!isSubtreeOf(rhs))
        return res;

    for (uint32_t i = rhs.valid; i < valid; ++i)
        res.push(oids[i]);

    return res;
}

bool OidSequence::isSubtreeOf(const OidSequence &oid) const {
    if (valid <= oid.valid)
        return false;

    for (uint32_t i = 0; i < oid.valid; ++i)
        if (oids[i] != oid.oids[i])
            return false;

    return true;
}

ostream& operator<<(ostream &o, const OidSequence &s) {
    for (uint32_t i = 0; i < s.valid; ++i) {
        o << "." << s.oids[i];
    }
    return o;
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
