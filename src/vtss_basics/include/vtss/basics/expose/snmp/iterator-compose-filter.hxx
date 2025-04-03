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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_FILTER_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_FILTER_HXX__

#include "vtss/basics/api_types.h"  // For mesa_rc and friends

namespace vtss {
namespace expose {
namespace snmp {

template<typename T, typename ITERATOR, typename CONDITION>
mesa_rc IteratorComposeFilter(ITERATOR &&f, const T *prev, T *next, CONDITION &&c) {
    T prev_;
    const T *prev_ptr = prev; // null or value in first round

    do {
        // call the normal iterator
        mesa_rc rc = f(prev_ptr, next);

        // check for end of sequence
        if (rc != MESA_RC_OK)
            return rc;

        // check if condition is true
        if (c(*next))
            return MESA_RC_OK;

        // prepare next round
        prev_ = *next;
        prev_ptr = &prev_;
    } while (1);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_FILTER_HXX__
