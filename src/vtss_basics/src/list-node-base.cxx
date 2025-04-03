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

#include "vtss/basics/list-node-base.hxx"

namespace vtss {

void ListNodeBase::swap_nodes(ListNodeBase *rhs) {
    VTSS_ASSERT(is_linked());
    VTSS_ASSERT(rhs->is_linked());

    if (this == rhs) return;

    auto rhs_new_next = next;
    auto rhs_new_prev = prev;
    auto n_new_prev = rhs->prev;
    auto n_new_next = rhs->next;

    if (prev != rhs) {
        prev->next = rhs;
        rhs->next->prev = this;
    } else {
        rhs_new_prev = this;
        n_new_next = rhs;
    }

    if (next != rhs) {
        next->prev = rhs;
        rhs->prev->next = this;
    } else {
        rhs_new_next = this;
        n_new_prev = rhs;
    }

    next = n_new_next;
    rhs->next = rhs_new_next;

    prev = n_new_prev;
    rhs->prev = rhs_new_prev;
}

}  // namespace vtss
