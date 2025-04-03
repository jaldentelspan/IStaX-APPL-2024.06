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

#ifndef __VTSS_BASICS_MAP_SET_COMMON_HXX__
#define __VTSS_BASICS_MAP_SET_COMMON_HXX__

#include "vtss/basics/common.hxx"
#include "vtss/basics/utility.hxx"
#include "vtss/basics/rbtree-stl.hxx"

#include "vtss/basics/map-iterator.hxx"
#include "vtss/basics/map-reverse-iterator.hxx"

namespace vtss {

template <class SUPER, class KeyRef>
struct MapSetCommon : protected RbtreeStl {
    typedef typename KeyRef::value_type TYPE;
    typedef typename KeyRef::key_type KEY;
    typedef typename KeyRef::node_type Node;

    // types:
    typedef MapSetCommon<SUPER, KeyRef> THIS;
    typedef RbtreeStl BASE;

    typedef TYPE value_type;

    typedef value_type *pointer;
    typedef const value_type *const_pointer;

    typedef value_type &reference;
    typedef const value_type &const_reference;

    typedef MapIterator<value_type> iterator;
    typedef MapIterator<const value_type> const_iterator;

    typedef MapReverseIterator<value_type> reverse_iterator;
    typedef MapReverseIterator<const value_type> const_reverse_iterator;

    // construct/copy/destroy:
    MapSetCommon() {}

    MapSetCommon(const MapSetCommon &m) {
        void *p = BASE::allocate(m.size());
        construct_range(p, m.begin(), m.end());
    }

    MapSetCommon(MapSetCommon &&m) : BASE(vtss::move(m)) {}

    MapSetCommon &operator=(const MapSetCommon &m) {
        // Free all existing elements and return back a linked-list of memory
        // chunks
        void *head = recycle_nodes(m.size());

        // Return if we could not get the memory we need (or if m.size() == 0)
        if (!head) return *this;

        // Insert the elements from 'm'
        construct_range(head, m.begin(), m.end());

        return *this;
    }

    MapSetCommon &operator=(MapSetCommon &&m) {
        BASE::operator=(vtss::move(m));
        return *this;
    }

    // iterators:
    iterator begin() { return make_iter(BASE::begin()); }
    iterator end() { return make_iter(nullptr); }
    const_iterator begin() const { return make_iter(BASE::begin()); }
    const_iterator end() const { return make_iter(nullptr); }
    const_iterator cbegin() const { return make_iter(BASE::begin()); }
    const_iterator cend() const { return make_iter(nullptr); }

    reverse_iterator rbegin() { return make_riter(BASE::last()); }
    reverse_iterator rend() { return make_riter(nullptr); }
    const_reverse_iterator crbegin() const { return make_riter(BASE::last()); }
    const_reverse_iterator crend() const { return make_riter(nullptr); }
    const_reverse_iterator rbegin() const { return make_riter(BASE::last()); }
    const_reverse_iterator rend() const { return make_riter(nullptr); }

    // capacity:
    bool empty() const { return BASE::size() == 0; };
    size_t size() const { return BASE::size(); };

    void erase(const_iterator i) {
        BASE::erase(const_cast<RbtreeBase::NodeBase *>(i.n));
    }

    size_t erase(const KEY &k) {
        KeyRef key(&k);
        return BASE::erase(key);
    }

    void erase(const_iterator i, const_iterator e) {
        BASE::erase(const_cast<RbtreeBase::NodeBase *>(i.n),
                    const_cast<RbtreeBase::NodeBase *>(e.n));
    }

    void clear() { BASE::clear(); }

    void swap(MapSetCommon &m) {}

    // MapSetCommon operations:
    iterator find(const KEY &k) {
        KeyRef key(&k);
        return iterator(BASE::find(key));
    }

    const_iterator find(const KEY &k) const {
        KeyRef key(&k);
        return const_iterator(BASE::find(key));
    }

    iterator greater_than(const KEY &k) {
        KeyRef key(&k);
        return iterator(BASE::greater_than(key));
    }

    const_iterator greater_than(const KEY &k) const {
        KeyRef key(&k);
        return const_iterator(BASE::greater_than(key));
    }

    iterator lesser_than(const KEY &k) {
        KeyRef key(&k);
        return iterator(BASE::lesser_than(key));
    }

    const_iterator lesser_than(const KEY &k) const {
        KeyRef key(&k);
        return const_iterator(BASE::lesser_than(key));
    }

    iterator greater_than_or_equal(const KEY &k) {
        KeyRef key(&k);
        return iterator(BASE::greater_than_or_equal(key));
    }

    const_iterator greater_than_or_equal(const KEY &k) const {
        KeyRef key(&k);
        return const_iterator(BASE::greater_than_or_equal(key));
    }

    iterator lesser_than_or_equal(const KEY &k) {
        KeyRef key(&k);
        return iterator(BASE::lesser_than_or_equal(key));
    }

    const_iterator lesser_than_or_equal(const KEY &k) const {
        KeyRef key(&k);
        return const_iterator(BASE::lesser_than_or_equal(key));
    }

  protected:
    iterator make_iter(RbtreeBase::NodeBase *n) { return iterator(n); }
    const_iterator make_iter(const RbtreeBase::NodeBase *n) const {
        return const_iterator(n);
    }
    reverse_iterator make_riter(RbtreeBase::NodeBase *n) {
        return reverse_iterator(n);
    }
    const_reverse_iterator make_riter(const RbtreeBase::NodeBase *n) const {
        return const_reverse_iterator(n);
    }

    size_t max_size() const {
        return static_cast<const SUPER *>(this)->max_size();
    }

    void *allocate() {
        if (size() + 1 > max_size()) return nullptr;
        return static_cast<SUPER *>(this)->allocate_();
    }

    void delete_(RbtreeBase::NodeBase *n_) {
        Node *n = static_cast<Node *>(n_);
        n->~Node();
        deallocate(n);
    }

    void *destruct(RbtreeBase::NodeBase *n_) {
        Node *n = static_cast<Node *>(n_);
        n->~Node();
        return n;
    }

    void deallocate(void *p) { static_cast<SUPER *>(this)->deallocate_(p); }


    // May only be used when the range [i, e[ is guaranteed to be sorted
    template <class I>
    void construct_range(void *head, I i, I e) {
        if (!head) return;

        VTSS_ASSERT(size() == 0);

        Node *n = nullptr;
        Node *last_n = nullptr;

        while (i != e) {
            // Pop a memory chunk from the linked list
            void *ptr = head;
            head = *((void **)head);

            // Construct the new memory chunk
            n = ::new (ptr) Node(*i);

            // Insert it into the tree - if this is the first then insert this
            // element as the root - otherwise insert it at the right side of
            // the last element
            BASE::insert_new_element(
                    last_n, n,
                    last_n ? RbtreeBase::RIGHT : RbtreeBase::EMPTY_TREE);

            // Update last pointer
            last_n = n;

            // Move the iterator
            ++i;
        }
    }
};

}  // namespace vtss

#endif  // __VTSS_BASICS_MAP_SET_COMMON_HXX__
