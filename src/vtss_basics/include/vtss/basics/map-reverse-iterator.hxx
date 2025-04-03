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

#ifndef __VTSS_BASICS_MAP_REVERSE_ITERATOR_HXX__
#define __VTSS_BASICS_MAP_REVERSE_ITERATOR_HXX__

#include <vtss/basics/common.hxx>
#include <vtss/basics/utility.hxx>
#include <vtss/basics/iterator.hxx>
#include "vtss/basics/rbtree-base.hxx"

namespace vtss {

template <class KEY, class VAL, class BASE>
struct MapBase;

template <class KEY, class BASE>
struct SetBase;

template <class VAL>
struct MapNode;

template <typename TYPE>
class MapReverseIterator : public iterator<bidirectional_iterator_tag, TYPE> {
  public:
    template <class A>
    friend class MapReverseIterator;

    template <class KEY, class VAL, class SUPER>
    friend struct MapBase;

    template <class KEY, class SUPER>
    friend struct SetBase;

    template <class SUPER, class KeyRef>
    friend struct MapSetCommon;

    typedef typename std::conditional<std::is_const<TYPE>::value,
                                      const MapNode<TYPE>,
                                      MapNode<TYPE>>::type MAP_NODE;

    typedef typename std::conditional<std::is_const<TYPE>::value,
                                      const RbtreeBase::NodeBase,
                                      RbtreeBase::NodeBase>::type MAP_NODE_BASE;

    MapReverseIterator() : n(nullptr) {}
    explicit MapReverseIterator(MAP_NODE_BASE *_n) : n(_n) {}
    MapReverseIterator(const MapReverseIterator &rhs) : n(rhs.n) {}
    MapReverseIterator &operator=(const MapReverseIterator &rhs) {
        n = rhs.n;
        return *this;
    }

    template <class OTHER_TYPE>
    MapReverseIterator(const MapReverseIterator<OTHER_TYPE> &rhs,
                       typename std::enable_if<std::is_convertible<
                               OTHER_TYPE *, TYPE *>::value>::type * = 0)
        : n(rhs.n) {}

    TYPE *operator->() { return &static_cast<MAP_NODE *>(n)->data; }
    TYPE &operator*() { return static_cast<MAP_NODE *>(n)->data; }

    MapReverseIterator &operator++() {
        n = n->prev();
        return *this;
    }

    MapReverseIterator operator++(int) {
        MapReverseIterator i(n);
        n = n->prev();
        return i;
    }

    MapReverseIterator &operator--() {
        n = n->next();
        return *this;
    }

    MapReverseIterator operator--(int) {
        MapReverseIterator i(n);
        n = n->next();
        return i;
    }

    bool operator==(const MapReverseIterator &rhs) { return n == rhs.n; }
    bool operator!=(const MapReverseIterator &rhs) { return n != rhs.n; }

  private:
    MAP_NODE_BASE *n;
};

}  // namespace vtss

#endif  // __VTSS_BASICS_MAP_REVERSE_ITERATOR_HXX__
