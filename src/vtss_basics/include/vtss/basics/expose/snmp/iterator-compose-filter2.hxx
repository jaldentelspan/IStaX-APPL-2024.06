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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_FILTER2_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_FILTER2_HXX__

#include <vtss/basics/common.hxx>

namespace vtss {
namespace expose {
namespace snmp {

template<typename T1, typename T2, typename iterator, typename condition>
mesa_rc IteratorComposeFilter2(
    iterator            &&f,
    condition           &&c,
    const T1    *const  prev1,
    T1          *const  next1,
    const T2    *const  prev2,
    T2          *const  next2
)
{
    T1 prev1_;
    const T1 *prev1_ptr = prev1; // null or value in first round

    T2 prev2_;
    const T2 *prev2_ptr = prev2 ? prev2 : NULL; // null or value in first round

    do {
        // call the normal iterator
        mesa_rc rc = f(prev1_ptr, next1, prev2_ptr, next2);

        // check for end of sequence
        if (rc != MESA_RC_OK) {
            return rc;
        }

        // check if condition is true
        if (c(*next1, *next2)) {
            return MESA_RC_OK;
        }

        // prepare next round
        prev1_ = *next1;
        prev2_ = *next2;
        prev1_ptr = &prev1_;
        prev2_ptr = &prev2_;
    } while (1);
} // IteratorComposeFilter2

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_FILTER2_HXX__
