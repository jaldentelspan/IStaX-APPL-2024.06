/*

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

*/

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/expose/snmp/handlers/oid_handler.hxx"

namespace vtss {
namespace expose {
namespace snmp {

OidImporter::OidImporter(const OidSequence &o, bool next, Mode::E m)
    : oids_(o), mode_(m), next_request_(next) {
    // If we are parsing an incomplete OID sequence we must subtract on from the
    // oid sequence to get the perviouse.
    // This means that if we are parsing "1.0.0" as an IP address (which expect
    // 4 oidelements) we must present this as "0.255.255.255".

    // Some extra examples
    // []        -> [] (get-first)
    // [0]       -> [] (get-first)
    // [0, 0, 0] -> [] (get-first)
    // [1]       -> [0.255.255.255]
    // [2]       -> [1.255.255.255]
    // [2.0]     -> [1.255.255.255]
    // [2.1]     -> [2.0.255.255]
    // [2.0.0]   -> [1.255.255.255]
    // [2.1.1]   -> [2.1.0.255]
    // [2.1.1.1] -> [2.1.1.1]

    // The above example '[0, 0, 0] -> [] (get-first)' is only for the case
    // the IP address is the primary key, if not, e.g., the key is
    // {key1: int, key2: [IP address]}, the case {key1: 1, key2:[0, 0, 0]}
    // should be convert to {key1: 0, key2: [255, 255, 255, 255]} for get-next.

    if (mode_ == Mode::Incomplete) {
        uint32_t zero_oid_cnt = 0;
        while (oids_.valid) {
            uint32_t e = oids_.pop();

            if (e != 0) {
                oids_.push(e - 1);
                for (uint32_t i = 0; i < zero_oid_cnt; i++) {
                    // Fill maximum OID value here and then serialize() will
                    // correct the incompleted value later.
                    oids_.push(0xFFFFFFFF);
                }
                break;
            } else {
                zero_oid_cnt++;
            }
        }
        VTSS_BASICS_TRACE(DEBUG) << "Oid import sequence: " << o << " -> "
                                 << oids_;
    }
}

bool OidImporter::consume(uint32_t &i) {
    // Okay, this really look weird... If we encounter an overflow then all
    // the following fields are overflowed.
    bool res = oids_.pop_head(i);

    switch (mode_) {
    case Mode::Overflow:
        i = 0xffffffff;
        VTSS_BASICS_TRACE(NOISE) << "Consumed: " << i << " true";
        return true;

    case Mode::Incomplete:
        if (!res) i = 0xffffffff;
        VTSS_BASICS_TRACE(NOISE) << "Consumed: " << i << " true";
        return true;

    case Mode::Normal:
    default:
        VTSS_BASICS_TRACE(NOISE) << "Consumed: " << i << " " << res;
        return res;
    }
}

void OidImporter::flag_overflow() {
    if (mode_ == Mode::Incomplete) return;

    mode_ = Mode::Overflow;
}

bool OidImporter::force_get_first() const {
    switch (mode_) {
    case Mode::Overflow:
        return false;

    case Mode::Normal:
    case Mode::Incomplete:
    default:
        return oids_.valid == 0;
    }
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
