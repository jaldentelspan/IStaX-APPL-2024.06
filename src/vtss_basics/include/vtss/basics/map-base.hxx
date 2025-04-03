
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

#ifndef __VTSS_BASICS_MAP_BASE_HXX__
#define __VTSS_BASICS_MAP_BASE_HXX__

#include "vtss/basics/meta.hxx"
#include "vtss/basics/tuple.hxx"
#include "vtss/basics/map-node.hxx"
#include "vtss/basics/map-set-common.hxx"

namespace vtss {

template <class KEY, class VAL>
struct MapKeyRef : public RbtreeBase::KeyBase {
    typedef KEY key_type;
    typedef VAL value_type;
    typedef MapNode<value_type> node_type;

    const KEY *key;
    MapKeyRef(const KEY *k) : key(k) {}

    int compare_key(const RbtreeBase::NodeBase *rhs) const {
        const node_type *n = static_cast<const node_type *>(rhs);
        if (*key < n->data.first) return -1;
        if (n->data.first < *key) return 1;
        return 0;
    }
};

template <class K, class V, class SUPER>
struct MapBase : public MapSetCommon<SUPER, MapKeyRef<K, Pair<const K, V>>> {
    typedef MapBase<K, V, SUPER> THIS;
    typedef MapSetCommon<SUPER, MapKeyRef<K, Pair<const K, V>>> BASE;

    typedef K key_type;
    typedef V mapped_type;

    typedef MapKeyRef<K, Pair<const K, V>> KeyRef;
    typedef typename KeyRef::value_type value_type;
    typedef typename KeyRef::node_type Node;

    typedef typename BASE::pointer pointer;
    typedef typename BASE::const_pointer const_pointer;

    typedef typename BASE::reference reference;
    typedef typename BASE::const_reference const_reference;

    typedef typename BASE::iterator iterator;
    typedef typename BASE::const_iterator const_iterator;

    typedef typename BASE::reverse_iterator reverse_iterator;
    typedef typename BASE::const_reverse_iterator const_reverse_iterator;

    MapBase() = default;

    // element access:
    // Insert a new (key, value) pair, if the key already exists then update it
    bool set(const K &k, const V &v) {
        KeyRef key(&k);
        RbtreeBase::Where where;
        auto *n = static_cast<Node *>(RbtreeBase::find(key, where));

        // No allocation needed
        if (where == RbtreeBase::MATCH) {
            n->data.second = v;
            return true;
        }

        void *ptr = BASE::allocate();
        if (!ptr) return false;

        auto nn = ::new (ptr) Node(k, v);
        return BASE::insert_new_element(n, nn, where);
    }

    bool set(const K &k, V &&v) {
        KeyRef key(&k);
        RbtreeBase::Where where;
        auto *n = static_cast<Node *>(RbtreeBase::find(key, where));

        // No allocation needed
        if (where == RbtreeBase::MATCH) {
            n->data.second = vtss::move(v);
            return true;
        }

        void *ptr = BASE::allocate();
        if (!ptr) return false;

        auto nn = ::new (ptr) Node(k, vtss::move(v));
        return BASE::insert_new_element(n, nn, where);
    }

    // Look for a given key in the map - if not found then create it. If the
    // entry could not be found and the allocation failed - then return a 'end'
    // iterator, otherwise return an iterator to the entry.
    iterator get(const K &k) {
        KeyRef key(&k);
        RbtreeBase::Where where;
        auto *n = static_cast<Node *>(RbtreeBase::find(key, where));

        // No allocation needed
        if (where == RbtreeBase::MATCH) return iterator(n);

        void *ptr = BASE::allocate();
        if (!ptr) return iterator(nullptr);

        // Construct 'key'
        ::new ((void *)(&((Node *)ptr)->data.first)) K(k);

        // Default construct 'val'
        ::new (&((Node *)ptr)->data.second) V();

        // RbtreeBase::NodeBase has no construct and it is therefor safe to cast
        static_assert(is_pod<RbtreeBase::NodeBase>::value, "");
        Node *nn = (Node *)ptr;

        BASE::insert_new_element(n, nn, where);

        return iterator(nn);
    }

    template <class... Args>
    Pair<iterator, bool> emplace(Args &&... args) {
        RbtreeBase::Where where;

        // Allocate a node for the element
        void *ptr = BASE::allocate();
        if (!ptr) return Pair<iterator, bool>(iterator(nullptr), false);

        // construct the node
        auto nn = ::new (ptr) Node(vtss::forward<Args>(args)...);

        // Create a key-ref related to the node
        KeyRef key(&(nn->data.first));

        // Search the tree
        auto *n = static_cast<Node *>(RbtreeBase::find(key, where));

        // No allocation needed
        if (where == RbtreeBase::MATCH) {
            // Destruct the node as it could not be inserted anyway
            nn->~Node();
            BASE::deallocate(nn);
            return Pair<iterator, bool>(iterator(n), false);
        }

        BASE::insert_new_element(n, nn, where);

        return Pair<iterator, bool>(iterator(nn), true);
    }

    Pair<iterator, bool> insert(const value_type &v) {
        KeyRef key(&v.first);
        RbtreeBase::Where where;
        auto *n = static_cast<Node *>(RbtreeBase::find(key, where));

        // No allocation needed
        if (where == RbtreeBase::MATCH)
            return Pair<iterator, bool>(iterator(n), false);

        void *ptr = BASE::allocate();
        if (!ptr) return Pair<iterator, bool>(iterator(nullptr), false);

        auto nn = ::new (ptr) Node(v);
        BASE::insert_new_element(n, nn, where);

        return Pair<iterator, bool>(iterator(nn), true);
    }

    Pair<iterator, bool> insert(value_type &&v) {
        KeyRef key(&v.first);
        RbtreeBase::Where where;
        auto *n = static_cast<Node *>(RbtreeBase::find(key, where));

        // No allocation needed
        if (where == RbtreeBase::MATCH)
            return Pair<iterator, bool>(iterator(n), false);

        void *ptr = BASE::allocate();
        if (!ptr) return Pair<iterator, bool>(iterator(nullptr), false);

        auto nn = ::new (ptr) Node(vtss::move(v));
        BASE::insert_new_element(n, nn, where);

        return Pair<iterator, bool>(iterator(nn), true);
    }

    V &operator[](const K &k) {
        return (*get(k)).second;
    }

};

}  // namespace vtss
#endif  // __VTSS_BASICS_MAP_BASE_HXX__
