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

#ifndef __INTRUSIVE_LIST_HXX__
#define __INTRUSIVE_LIST_HXX__

#include "vtss/basics/common.hxx"
#include "vtss/basics/meta.hxx"

namespace vtss {
namespace intrusive {
template <typename T>
class List;
template <typename Type, typename ListNode_type>
class list_iterator;

struct ListNode {
    template <typename>
    friend class List;
    template <typename, typename>
    friend class list_iterator;

    // Marking this as constexpr will make instances of this object
    // "pre-constructed" when creating global instances - and it will be
    // constructed the usual way when constructed on a stack.
    constexpr ListNode() : next(nullptr), prev(nullptr) {}
    constexpr ListNode(ListNode* n, ListNode* p) : next(n), prev(p) {}

    bool is_linked() const { return next != 0 && prev != 0; }

    virtual ~ListNode() {
        VTSS_ASSERT(next == 0);
        VTSS_ASSERT(prev == 0);
    }

    void unlink() {
        if (prev) prev->next = next;
        if (next) next->prev = prev;
        next = 0;
        prev = 0;
    }

  private:
    ListNode* next;
    ListNode* prev;
};


template <typename Super, typename T>
struct IterTypes {
    typedef ListNode* member;
    typedef ListNode const* const_member;
};

template <typename Super, typename T>
struct IterTypes<Super, const T> {
    typedef ListNode const* member;
    typedef ListNode const* const_member;
};


template <typename Type, typename ListNode_type>
class list_iterator {
  public:
    typedef typename meta::remove_const<Type>::type Type_no_const;
    friend class List<Type_no_const>;

    explicit list_iterator(ListNode_type* _n) : n(_n) {}
    list_iterator(const list_iterator& rhs) : n(rhs.n) {}
    Type* operator->() { return static_cast<Type*>(n); }
    Type& operator*() { return *static_cast<Type*>(n); }

    list_iterator& operator++() {
        n = n->next;
        return *this;
    }

    list_iterator operator++(int) {
        n = n->next;
        return list_iterator(n->prev);
    }

    list_iterator& operator--() {
        n = n->prev;
        return *this;
    }

    list_iterator operator--(int) {
        n = n->prev;
        return list_iterator(n->next);
    }

    bool operator==(const list_iterator& rhs) { return n == rhs.n; }

    bool operator!=(const list_iterator& rhs) { return n != rhs.n; }

  private:
    ListNode_type* n;
};

template <typename T>
class List : ListNode {
  public:
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef list_iterator<T, ListNode> iterator;
    typedef list_iterator<const T, const ListNode> const_iterator;

    // Must be constexpr as we relay on initializing list in "global" data.
    constexpr List() {}

    ~List() {
        VTSS_ASSERT(next == static_cast<ListNode*>(this) || next == nullptr);
        VTSS_ASSERT(prev == static_cast<ListNode*>(this) || prev == nullptr);
        next = 0, prev = 0;
    }

    void init() {
        if (!next) {
            next = static_cast<ListNode*>(this);
            prev = static_cast<ListNode*>(this);
        }
    }

    inline iterator begin() {
        init();
        return iterator(next);
    }

    inline const_iterator begin() const {
        init();
        return const_iterator(static_cast<const ListNode*>(next));
    }

    inline iterator end() {
        init();
        return iterator(static_cast<ListNode*>(this));
    }

    inline const_iterator end() const {
        init();
        return const_iterator(static_cast<const ListNode*>(this));
    }

    inline iterator insert(iterator it, T& _v) {
        VTSS_ASSERT(!_v.is_linked());

        ListNode* v = static_cast<ListNode*>(&_v);
        VTSS_ASSERT(it.n != v);

        v->prev = it.n->prev;
        v->next = it.n;

        it.n->prev->next = v;
        it.n->prev = v;

        return iterator(v);
    }

    size_t size() const {
        size_t cnt = 0;
        for (auto i = begin(); i != end(); ++i) ++cnt;
        return cnt;
    }

    // TODO(anielsen) make an external function
    inline void insert_sorted(T& _v) {
        VTSS_ASSERT(!_v.is_linked());
        iterator i = begin();
        while (i != end() && *i < _v) {
            ++i;
        }
        insert(i, _v);
    }

    inline void unlink(T& t) {
        // TODO(anielsen) check that t is actually a member of this list
        if (t.is_linked()) t.ListNode::unlink();
    }

    inline iterator erase(iterator it) {
        iterator i = it;
        ++i;
        it->ListNode::unlink();
        return i;
    }

    inline void push_front(T& v) { insert(begin(), v); }
    inline void push_back(T& v) { insert(end(), v); }
    inline void pop_front() { erase(begin()); }
    inline void pop_back() { erase(--end()); }
    inline const_reference front(void) const { return (*begin()); }
    inline reference front() { return (*begin()); }
    inline const_reference back(void) const { return (*(--end())); }
    inline reference back() { return (*(--end())); }

    bool empty() const {
        return next == static_cast<const ListNode*>(this) || next == nullptr;
    }
};

}  // namespace intrusive
}  // namespace vtss
#endif /* __INTRUSIVE_LIST_HXX__ */
