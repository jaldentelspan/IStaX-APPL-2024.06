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

#ifndef __VTSS_BASICS_LIST_NODE_BASE_HXX__
#define __VTSS_BASICS_LIST_NODE_BASE_HXX__

#include <vtss/basics/common.hxx>

namespace vtss {

template <typename TYPE>
class ListIterator;

struct ListNodeBase {
    template <class TYPE>
    friend struct List;

    template <typename A>
    friend class ListIterator;

    bool is_linked() const { return next != this /* && prev != this */; }

    void unlink() {
        prev->next = next;
        next->prev = prev;
        next = this;
        prev = this;
    }

    void insert_before(ListNodeBase *x) {
        x->prev = prev;
        x->next = this;
        prev->next = x;
        prev = x;
    }

    void swap_nodes(ListNodeBase *rhs);

  private:
    ListNodeBase *next = this;
    ListNodeBase *prev = this;
};

}  // namespace vtss

#endif  // __VTSS_BASICS_LIST_NODE_BASE_HXX__
