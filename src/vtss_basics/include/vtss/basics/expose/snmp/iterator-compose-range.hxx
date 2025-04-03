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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_RANGE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_RANGE_HXX__

#include "vtss/basics/api_types.h"  // For mesa_rc and friends
#include "vtss/basics/expose/snmp/iterator-compose-filter.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename T>
struct IteratorComposeRange {
    IteratorComposeRange(const T s, const T e) : start(s), end(e) {}

    mesa_rc operator()(const T *prev, T *next) {

        // check for empty range
        if (start > end) return MESA_RC_ERROR;

        if (!prev) {  // get-first?
            *next = start;
            return MESA_RC_OK;
        }

        // check for underflow
        if (*prev < start) {
            *next = start;
            return MESA_RC_OK;
        }

        // end of sequence
        if (*prev >= end) return MESA_RC_ERROR;

        // normal increment
        *next = (T)(*prev + 1);

        return MESA_RC_OK;
    }

    template<typename F>
    mesa_rc filtered(const T *prev, T *next, F f) {
        return IteratorComposeFilter<T>(*this, prev, next, f);
    }

    const T start;
    const T end;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_RANGE_HXX__
