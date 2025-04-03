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

#ifndef __VTSS_BASICS_SET_BASE_HXX__
#define __VTSS_BASICS_SET_BASE_HXX__

#include "vtss/basics/map-node.hxx"
#include "vtss/basics/map-set-common.hxx"

namespace vtss {

template <class KEY, class VAL>
struct SetKeyRef : public RbtreeBase::KeyBase {
    typedef KEY key_type;
    typedef VAL value_type;
    typedef MapNode<value_type> node_type;

    const KEY *key;
    SetKeyRef(const KEY *k) : key(k) {}

    int compare_key(const RbtreeBase::NodeBase *rhs) const {
        const node_type *n = static_cast<const node_type *>(rhs);
        if (*key < n->data) return -1;
        if (n->data < *key) return 1;
        return 0;
    }
};

template <class K, class SUPER>
struct SetBase : public MapSetCommon<SUPER, SetKeyRef<K, K>> {
    typedef SetBase<K, SUPER> THIS;
    typedef MapSetCommon<SUPER, SetKeyRef<K, K>> BASE;

    typedef SetKeyRef<K, K> KeyRef;
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

    iterator get(const K &k) {
        KeyRef key(&k);
        RbtreeBase::Where where;
        auto *n = static_cast<Node *>(RbtreeBase::find(key, where));

        // No allocation needed
        if (where == RbtreeBase::MATCH) return iterator(n);

        void *ptr = BASE::allocate();
        if (!ptr) return iterator(nullptr);

        auto nn = ::new (ptr) Node(value_type(k));

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
        KeyRef key(&(nn->data));

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

    bool set(const K &k) {
        auto i = insert(k);
        return i.first.n;
    }

    bool set(K &&k) {
        auto i = insert(k);
        return i.first.n;
    }

    Pair<iterator, bool> insert(const K &k) {
        KeyRef key(&k);
        RbtreeBase::Where where;
        auto *n = static_cast<Node *>(RbtreeBase::find(key, where));

        // No allocation needed
        if (where == RbtreeBase::MATCH)
            return Pair<iterator, bool>(iterator(n), false);

        void *ptr = BASE::allocate();
        if (!ptr) return Pair<iterator, bool>(iterator(nullptr), false);

        auto nn = ::new (ptr) Node(k);
        BASE::insert_new_element(n, nn, where);

        return Pair<iterator, bool>(iterator(nn), true);
    }

    Pair<iterator, bool> insert(K &&k) {
        KeyRef key(&k);
        RbtreeBase::Where where;
        auto *n = static_cast<Node *>(RbtreeBase::find(key, where));

        // No allocation needed
        if (where == RbtreeBase::MATCH)
            return Pair<iterator, bool>(iterator(n), false);

        void *ptr = BASE::allocate();
        if (!ptr) return Pair<iterator, bool>(iterator(nullptr), false);

        auto nn = ::new (ptr) Node(vtss::move(k));
        BASE::insert_new_element(n, nn, where);

        return Pair<iterator, bool>(iterator(nn), true);
    }
};

}  // namespace vtss

#endif  // __VTSS_BASICS_SET_BASE_HXX__
