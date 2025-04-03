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

#ifndef __VTSS_BASICS_RBTREE_STL_HXX__
#define __VTSS_BASICS_RBTREE_STL_HXX__

#include "vtss/basics/rbtree-base.hxx"

namespace vtss {

struct RbtreeStl : public RbtreeBase {
    RbtreeStl() {}
    RbtreeStl(RbtreeStl &&m) : RbtreeBase(vtss::move(m)) {}
    RbtreeStl &operator=(RbtreeStl &&m) {
        clear();
        RbtreeBase::operator=(vtss::move(m));
        return *this;
    }

    // Recycle the nodes. This is required as we are not allowed to fail if
    // lhs.size() == rhs.size() (there would be no-way to detect the
    // failure). If we do not recycle then there would be a change we can
    // not allocate the required memory.
    void *recycle_nodes(const size_t target_size);

    void erase(RbtreeBase::NodeBase *n);

    size_t erase(const KeyBase &key);

    void erase(RbtreeBase::NodeBase *i, RbtreeBase::NodeBase *e);

    void clear();

    ~RbtreeStl() { clear(); }

    // Allocate 'cnt' chunks. Create a single linked list of the chunks and
    // return the head. If all chunks could not be allocated return 'nullptr'
    void *allocate(size_t cnt);

    virtual void *allocate() = 0;
    virtual void delete_(RbtreeBase::NodeBase *n) = 0;
    virtual void *destruct(RbtreeBase::NodeBase *n) = 0;
    virtual void deallocate(void *p) = 0;
};

}  // namespace vtss

#endif  // __VTSS_BASICS_RBTREE_STL_HXX__
