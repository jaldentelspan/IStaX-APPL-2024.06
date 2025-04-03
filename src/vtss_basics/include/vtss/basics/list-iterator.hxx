/*

 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_LIST_ITERATOR_HXX__
#define __VTSS_BASICS_LIST_ITERATOR_HXX__

#include <vtss/basics/common.hxx>
#include <vtss/basics/utility.hxx>
#include <vtss/basics/iterator.hxx>
#include <vtss/basics/list-node-base.hxx>

namespace vtss {

template <class TYPE>
struct List;

template <class TYPE>
struct ListNode;

template <typename TYPE>
class ListIterator : public iterator<bidirectional_iterator_tag, TYPE> {
  public:
    template <class A>
    friend class ListIterator;

    template <class A>
    friend struct List;

    typedef typename std::conditional<std::is_const<TYPE>::value,
                                      const ListNode<TYPE>,
                                      ListNode<TYPE>>::type LIST_NODE;

    typedef typename std::conditional<std::is_const<TYPE>::value,
                                      const ListNodeBase,
                                      ListNodeBase>::type LIST_NODE_BASE;

    ListIterator() : n(nullptr) {}

    ListIterator &operator=(const ListIterator &rhs) {
        n = rhs.n;
        return *this;
    }

    explicit ListIterator(LIST_NODE_BASE *_n) : n(_n) {}
    ListIterator(const ListIterator &rhs) : n(rhs.n) {}

    template <class OTHER_TYPE>
    ListIterator(const ListIterator<OTHER_TYPE> &rhs,
                 typename std::enable_if<std::is_convertible<
                         OTHER_TYPE *, TYPE *>::value>::type * = 0)
        : n(rhs.n) {}

    TYPE *operator->() { return &static_cast<LIST_NODE *>(n)->data; }
    TYPE &operator*() { return static_cast<LIST_NODE *>(n)->data; }

    ListIterator &operator++() {
        n = n->next;
        return *this;
    }

    ListIterator operator++(int) {
        n = n->next;
        return ListIterator(n->prev);
    }

    ListIterator &operator--() {
        n = n->prev;
        return *this;
    }

    ListIterator operator--(int) {
        n = n->prev;
        return ListIterator(n->next);
    }

    void swap_nodes(ListIterator &rhs) {
        n->swap_nodes(rhs.n);
        swap(n, rhs.n);
    }

    bool operator==(const ListIterator &rhs) { return n == rhs.n; }
    bool operator!=(const ListIterator &rhs) { return n != rhs.n; }

  private:
    LIST_NODE_BASE *n;
};

}  // namespace vtss


#endif  // __VTSS_BASICS_LIST_ITERATOR_HXX__
