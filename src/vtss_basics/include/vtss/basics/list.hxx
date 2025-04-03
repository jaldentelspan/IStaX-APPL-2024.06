/*

 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_LIST_HXX__
#define __VTSS_BASICS_LIST_HXX__

#include <new>
#include <initializer_list>
#include <vtss/basics/new.hxx>
#include <vtss/basics/common.hxx>
#include <vtss/basics/utility.hxx>
#include <vtss/basics/algorithm.hxx>
#include <vtss/basics/list-iterator.hxx>
#include <vtss/basics/list-node-base.hxx>

namespace vtss {

/*

template <class TYPE>
struct List : private ListNodeBase {
  public:
    // Typedefs ////////////////////////////////////////////////////////////////
    typedef TYPE value_type;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef TYPE *pointer;
    typedef const TYPE *const_pointer;
    typedef ListIterator<TYPE> iterator;
    typedef ListIterator<const TYPE> const_iterator;
    typedef vtss::reverse_iterator<iterator> reverse_iterator;
    typedef vtss::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef iterator itr;
    typedef const_iterator c_itr;
    typedef const_reverse_iterator cr_itr;
    typedef reverse_iterator r_itr;

    // Constructor, assignment and destructor //////////////////////////////////
    //
    // Error handling: All constructors and assignment operators may fail if an
    // allocation fails (if we ran out of memory). The error handling
    // strategy for all methods is that if a method is failing, then we will
    // rollback such that the 'List' looks exactly as it did before calling the
    // failing method.
    //
    // The constructors and operators can not return a 'bool' to indicate if the
    // operations failed of succeeds, it is therefor necessary to check that the
    // size if the list matches the expectation after such operation.

    // Default constructor. Construct a new empty list.
    // Can not fail!
    List();

    // Construct a new list with 'n' elements. The elements are "value"
    // initialized.
    explicit List(size_t n);

    // Construct a new list and initialize it with 'n' copies of 'x'
    List(size_t n, const value_type &x);

    // Copy constructor. Construct a new list, and initialize it with a copy of
    // the elements from 'rhs'.
    List(const List &rhs) : max_size_(rhs.max_size());

    // Construct a new list, and initialize it with a copy of the elements in
    // the range [i, e[.
    template <class I>
    List(I i, I e);

    // Construct a new list, and initialize it with the values given in 'il'.
    List(std::initializer_list<value_type> il);

    // Assign the content of rhs to this. Note that the existing content of
    // 'this' will be overwritten doing this.
    List &operator=(const List &rhs);

    // Move-constructor. Move the content of 'rhs' into this, and leave 'rhs'
    // empty
    List(List &&rhs);

    // Move assignment operator. Empty the content of 'this' and move the
    // content of 'rhs' into this, and leave 'rhs' empty.
    List &operator=(List &&rhs);

    // Assign 'this' with the elements of 'il'.
    List &operator=(std::initializer_list<value_type> il);

    // Destructor, will clean up all resources.
    ~List();

    // State and status ////////////////////////////////////////////////////////

    // Get the current number of elements stored in the list
    size_t size() const;

    // Check if the list is empty
    bool empty() const;

    // Get/set the upper bound of elements this list can hold
    size_t max_size() const;
    void max_size(size_t m);

    // Data access /////////////////////////////////////////////////////////////
    // Data accessors will fail if trying to access a non-existing element.
    //
    // When accessing by-reference the error reporting strategy is to assert.
    // When accessing by-iterator the error reporting strategy is to return the
    // 'end()' iterator.
    // Dereference and "end" iterator is undefined behavioral.

    // Returns an iterator the first element in the list.
    iterator begin();

    // Returns an const-iterator the first element in the list.
    const_iterator begin() const;

    // Returns an iterator the end of the list.
    iterator end();

    // Returns an const-iterator the end of the list.
    const_iterator end() const;

    // Returns an const-iterator the first element in the list.
    c_itr cbegin() const;

    // Returns an const-iterator the end of the list.
    c_itr cend() const;

    // Return a "reverse" iterator to the last element of the list
    r_itr rbegin();

    // Return a "reverse" iterator to the "reverse-end" of the list
    r_itr rend();

    // Return a "reverse" const-iterator to the last element of the list
    cr_itr rbegin() const;

    // Return a "reverse" const-iterator to the "reverse-end" of the list
    cr_itr rend() const;

    // Return a "reverse" const-iterator to the last element of the list
    cr_itr crbegin() const;

    // Return a "reverse" const-iterator to the "reverse-end" of the list
    cr_itr crend() const;

    // Access the first element in the list - not allowed to call on an empty
    // list.
    reference front();
    const_reference front() const;

    // Access the last element in the list - not allowed to call on an empty
    // list.
    reference back();
    const_reference back() const;

    // Data manipulator - insert new elements //////////////////////////////////
    // All insert methods may fail if the max elements is reached, or if
    // allocation fails. All insert methods returns true on success, and false
    // on failure - in case of failure they will leave the list in the same
    // state as it was before calling the failing methods.

    // Assign the list with the elements in the range [i, e[. Note this will
    // overwrite the existing content of the list.
    template <class I>
    bool assign(I i, I e);

    // Assign the list with n copies of 'x'. Note this will overwrite the
    // existing content of the list.
    bool assign(size_t n, const value_type &x);

    // Assign the list with the values provided in 'il'. Note this will
    // overwrite the existing content of the list.
    bool assign(std::initializer_list<value_type> il);

    // Push a new element to the front of the list. The element 'x' is being
    // moved into the list.
    bool push_front(value_type &&x);

    // Push a new element to the back of the list. The element 'x' is being
    // moved into the list.
    bool push_back(value_type &&x);

    // Push a new element into the front of the list.
    void push_front(const value_type &x);

    // Push a new element into the back of the list.
    bool push_back(const value_type &x);

    // Construct a new element (using the arguments args...) at the front of the
    // list.
    template <class... Args>
    bool emplace_front(Args &&... args);

    // Construct a new element (using the argument args...) at the back of the
    // list.
    template <class... Args>
    bool emplace_back(Args &&... args);

    // Construct a new element (using the argument args...) in front of position
    // 'p' in the list.
    template <class... Args>
    bool emplace(const_iterator p, Args &&... args);

    // Insert a new element in front of position 'p' in the list. The new
    // element is being moved into the list.
    bool insert(const_iterator p, value_type &&x);

    // Insert a new element in front of position 'p' in the list.
    bool insert(const_iterator p, const value_type &x);

    // Insert 'n' copies of 'x' in front of position p, into the list.
    bool insert(const_iterator p, size_t n, const value_type &x);

    // Insert a copy of the range [i, e[ in front of position 'p', into the
    // list.
    template <class I>
    bool insert_range(const_iterator p, I i, I e);

    // Insert the values provided by 'il' in front of position 'p', into the
    // list.
    bool insert(const_iterator p, std::initializer_list<value_type> il);

    // Move the elements from 'rhs' into this, in front of position 'p'.
    bool splice(const_iterator p, List &rhs);
    bool splice(const_iterator p, List &&rhs);

    // Move the element 'i' from 'rhs' into this, in front of position 'p'.
    bool splice(const_iterator p, List &rhs, const_iterator i);
    bool splice(const_iterator p, List &&rhs, const_iterator i);

    // Move the elements [b, e[ from 'rhs' into this, in front of position 'p'.
    bool splice(const_iterator p, List &rhs, const_iterator b,
                const_iterator e);
    bool splice(const_iterator p, List &&rhs, const_iterator b,
                const_iterator e);

    // Data manipulator - delete elements //////////////////////////////////////

    // Empty the list
    void clear();

    // Remove the first element in the list.
    bool pop_front();

    // Remove the last element in the list.
    bool pop_back();

    // Remove the element at position 'p' from the list.
    iterator erase(const_iterator p);

    // Remove the element in the range [i, e[ from the list.
    iterator erase(const_iterator i, const_iterator e);

    // Remove all element who's value is equal to 'x' from the list.
    void remove(const value_type &x);

    // Remove all elements for which 'pred(TYPE)' returns true, from the list.
    template <class Pred>
    void remove_if(Pred pred);

    // Data manipulator - misc operations //////////////////////////////////////

    // Swap the content between this list and 'c'.
    void swap(List &c);

    // Resize the current list such that it will contain 'n' elements. If the
    // current size is larger than 'n' then elements are removed from the end,
    // if the current size is less than 'n' then new values are being inserted.
    // New values are being value initialized.
    bool resize(size_t n);

    // Resize the current list such that it will contain 'n' elements. If the
    // current size is larger than 'n' then elements are removed from the end,
    // if the current size is less than 'n' then new values are being inserted.
    // New values are copies of 'x'.
    bool resize(size_t n, const value_type &x);

    // Reverse the order of the element in the list.
    void reverse();
};

// Swap the content of the two lists 'a' and 'b'
template <typename T>
void swap(vtss::List<T> &a, vtss::List<T> &b);

// Compare if the 'a' and 'b' are equal.
template <class TYPE>
inline bool operator==(const List<TYPE> &a, const List<TYPE> &b);

// Compare if 'a' are different from 'b'
template <class TYPE>
inline bool operator!=(const List<TYPE> &a, const List<TYPE> &b);

// Lexicographical compare the elements in 'a' and 'b'
template <class TYPE>
inline bool operator<(const List<TYPE> &a, const List<TYPE> &b);

// Lexicographical compare the elements in 'a' and 'b'
template <class TYPE>
inline bool operator>(const List<TYPE> &a, const List<TYPE> &b);

// Lexicographical compare the elements in 'a' and 'b'
template <class TYPE>
inline bool operator>=(const List<TYPE> &a, const List<TYPE> &b);

// Lexicographical compare the elements in 'a' and 'b'
template <class TYPE>
inline bool operator<=(const List<TYPE> &a, const List<TYPE> &b);

*/

template <typename TYPE>
struct ListNode : public ListNodeBase {
    ListNode(const TYPE &t) : data(t) {}

    template <class... Args>
    ListNode(Args &&... args)
        : data(vtss::forward<Args>(args)...) {}

    TYPE data;
};

template <class TYPE>
struct List : private ListNodeBase {
  public:
    typedef TYPE value_type;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef TYPE *pointer;
    typedef const TYPE *const_pointer;
    typedef ListIterator<TYPE> iterator;
    typedef ListIterator<const TYPE> const_iterator;
    typedef vtss::reverse_iterator<iterator> reverse_iterator;
    typedef vtss::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef iterator itr;
    typedef const_iterator c_itr;
    typedef const_reverse_iterator cr_itr;
    typedef reverse_iterator r_itr;

    List() {}
    explicit List(size_t n) { resize(n); };

    List(size_t n, const value_type &x) { resize(n, x); }

    List(const List &rhs) : max_size_(rhs.max_size()) {
        construct_range(rhs.begin(), rhs.end());
    }

    template <class I>
    List(I i, I e,
         typename std::enable_if<is_input_iterator<I>::value>::type * = 0) {
        construct_range(i, e);
    }

    List(std::initializer_list<value_type> il) {
        construct_range(il.begin(), il.end());
    }

    // No "good" error reporting facilities... Check for error by comparing the
    // size of lhs and rhs after assigning.
    List &operator=(const List &rhs) {
        // Safe the old max_size - we need to restore to this in case of error
        size_t old_max_size = max_size_;

        // Assign the new max_size
        max_size_ = rhs.max_size();

        // Reserve the required size
        if (!resize(rhs.size())) {
            // operation failed - we must rollback
            max_size_ = old_max_size;
            return *this;
        }

        // Perform the copy
        overwrite(rhs.begin(), rhs.end());

        // Always return reference to self
        return *this;
    }

    List(List &&rhs) { move_(vtss::move(rhs)); }
    List &operator=(List &&rhs) {
        clear();
        move_(vtss::move(rhs));
    }

    List &operator=(std::initializer_list<value_type> il) {
        if (resize(il.size())) overwrite(il.begin(), il.end());

        return *this;
    }

    ~List() { clear(); }

    template <class I>
    bool assign(
            I i, I e,
            typename std::enable_if<is_input_iterator<I>::value>::type * = 0) {
        if (!resize(distance(i, e))) return false;
        overwrite(i, e);
        return true;
    }

    bool assign(size_t n, const value_type &x) {
        if (!resize(n)) return false;

        auto itr = begin();

        for (size_t i = 0; i < n; ++i, ++itr) *itr = x;
    }

    bool assign(std::initializer_list<value_type> il) {
        if (!resize(il.size())) return false;

        overwrite(il.begin(), il.end());

        return true;
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    size_t max_size() const { return max_size_; }
    void max_size(size_t m) { max_size_ = m; }

    iterator begin() { return iterator(next); }
    const_iterator begin() const { return const_iterator(next); }
    iterator end() { return iterator(this); }
    const_iterator end() const { return const_iterator(this); }
    c_itr cbegin() const { return begin(); }
    c_itr cend() const { return end(); }
    r_itr rbegin() { return r_itr(end()); }
    r_itr rend() { return r_itr(begin()); }
    cr_itr rbegin() const { return cr_itr(end()); }
    cr_itr rend() const { return cr_itr(begin()); }
    cr_itr crbegin() const { return cr_itr(end()); }
    cr_itr crend() const { return cr_itr(begin()); }

    reference front() {
        VTSS_ASSERT(size_ > 0);
        return static_cast<ListNode<TYPE> *>(next)->data;
    }

    const_reference front() const {
        VTSS_ASSERT(size_ > 0);
        return static_cast<ListNode<TYPE> *>(next)->data;
    }

    reference back() {
        VTSS_ASSERT(size_ > 0);
        return static_cast<ListNode<TYPE> *>(prev)->data;
    }

    const_reference back() const {
        VTSS_ASSERT(size_ > 0);
        return static_cast<ListNode<TYPE> *>(prev)->data;
    }

    bool push_front(value_type &&x) { return insert(begin(), vtss::move(x)); }
    bool push_back(value_type &&x) { return insert(end(), vtss::move(x)); }
    void push_front(const value_type &x) { return insert(begin(), x); }
    bool push_back(const value_type &x) { return insert(end(), x); }

    template <class... Args>
    bool emplace_front(Args &&... args) {
        return emplace(begin(), vtss::forward<Args>(args)...);
    }

    template <class... Args>
    bool emplace_back(Args &&... args) {
        return emplace(end(), vtss::forward<Args>(args)...);
    }

    template <class... Args>
    bool emplace(const_iterator p, Args &&... args) {
        void *ptr = alloc_and_increase_size();
        if (!ptr) return false;

        auto n = ::new (ptr) ListNode<TYPE>(vtss::forward<Args>(args)...);
        const_cast<ListNodeBase *>(p.n)->insert_before(n);

        return true;
    }

    bool insert(const_iterator p, value_type &&x) {
        void *ptr = alloc_and_increase_size();
        if (!ptr) return false;

        auto n = ::new (ptr) ListNode<TYPE>(vtss::move(x));
        const_cast<ListNodeBase *>(p.n)->insert_before(n);

        return true;
    }

    bool insert(const_iterator p, const value_type &x) {
        void *ptr = alloc_and_increase_size();
        if (!ptr) return false;

        auto n = ::new (ptr) ListNode<TYPE>(x);
        const_cast<ListNodeBase *>(p.n)->insert_before(n);

        return true;
    }

    bool insert(const_iterator p, size_t n, const value_type &x) {
        size_t _n = n;

        // Just insert one at a time
        for (; n; --n)
            if (!insert(p, x)) break;

        // If all elements could not be inserted - then clean up and go back to
        // the state before starting inserting.
        if (n) {
            --p;  // go one back as we inserted before
            for (; n != _n; ++n) erase(p--);
            return false;
        }

        return true;
    }

    template <class I>
    bool insert_range(const_iterator p, I i, I e) {
        size_t cnt = 0;

        // Just insert one at a time
        for (; i != e; ++i, ++cnt)
            if (!insert(p, *i)) break;

        // If all elements could not be inserted - then clean up and go back to
        // the state before starting inserting.
        if (i != e) {
            --p;  // go one back as we inserted before
            for (; cnt; --cnt) erase(p--);
            return false;
        }

        return true;
    }

    bool insert(const_iterator p, std::initializer_list<value_type> il) {
        return insert_range(p, il.begin(), il.end());
    }

    void swap(List &c) {
        List tmp(vtss::move(c));
        c.move_(vtss::move(*this));
        move_(vtss::move(tmp));
    }

    void clear() { erase(begin(), end()); }

    bool pop_front() {
        if (empty()) return false;
        erase(begin());
        return true;
    }

    bool pop_back() {
        if (empty()) return false;
        erase(const_iterator(prev));
        return true;
    }

    iterator erase(const_iterator p) {
        iterator next(p.n->next);

        // Const cast...
        auto n = const_cast<ListNodeBase *>(p.n);

        // Take it out of the current list
        n->unlink();

        // Cast to base type, destruct and delete
        auto x = static_cast<ListNode<TYPE> *>(n);
        x->~ListNode<TYPE>();
        VTSS_BASICS_FREE(x);

        // Decrement size
        --size_;

        return next;
    }

    iterator erase(const_iterator i, const_iterator e) {
        while (i != e) i = erase(i);
        return iterator(const_cast<ListNodeBase *>(e.n));
    }

    bool resize(size_t n) {
        TYPE t = {};
        return resize(n, t);
    }

    bool resize(size_t n, const value_type &x) {
        // Check if there are any thing to be done
        if (size_ == n) return true;
        if (n > max_size_) return false;

        if (size_ > n) {
            // delete elements until we are down to only 'n' elements
            auto itr = const_iterator(prev);
            size_t remove_cnt = size_ - n;

            // remove elements from the end
            for (size_t i = 0; i < remove_cnt; ++i) erase(itr--);

            // Can not fail...
            return true;

        } else {
            size_t i = 0;
            size_t add_cnt = n - size_;
            auto last_before_insert = const_iterator(prev);

            // insert the elements - break on error
            for (; i < add_cnt; ++i)
                if (!push_back(x)) break;

            // Role back on error!
            if (i != add_cnt) {
                erase(++last_before_insert, end());
                return false;
            }
        }

        return true;
    }

    bool splice(const_iterator p, List &rhs) {
        return __splice(const_cast<ListNodeBase *>(p.n), rhs, rhs.size(),
                        rhs.next, rhs.prev);
    }

    bool splice(const_iterator p, List &&rhs) { return splice(p, rhs); }

    bool splice(const_iterator p, List &rhs, const_iterator i) {
        return __splice(const_cast<ListNodeBase *>(p.n), rhs, 1,
                        const_cast<ListNodeBase *>(i.n),
                        const_cast<ListNodeBase *>(i.n));
    }

    bool splice(const_iterator p, List &&rhs, const_iterator i) {
        return splice(p, rhs, i);
    }

    bool splice(const_iterator p, List &rhs, const_iterator b,
                const_iterator e) {
        return __splice(const_cast<ListNodeBase *>(p.n), rhs, distance(b, e),
                        const_cast<ListNodeBase *>(b.n),
                        const_cast<ListNodeBase *>(e.n->prev));
    }

    bool splice(const_iterator p, List &&rhs, const_iterator b,
                const_iterator e) {
        return splice(p, rhs, b, e);
    }

    void remove(const value_type &x) {
        auto i = begin();
        auto e = end();

        while (i != e)
            if (*i == x)
                erase(i++);
            else
                ++i;
    }

    template <class Pred>
    void remove_if(Pred pred) {
        auto i = begin();
        auto e = end();

        while (i != e)
            if (pred(*i))
                erase(i++);
            else
                ++i;
    }

    // void unique();

    // template <class _BinaryPred>
    // void unique(_BinaryPred __binary_pred);

    // void merge(List &rhs);
    // void merge(List &&rhs) { merge(rhs); }

    // template <class _Comp>
    // void merge(List &rhs, _Comp __comp);

    // template <class _Comp>
    // void merge(List &&rhs, _Comp __comp) {
    //    merge(rhs, __comp);
    //}

    // void sort() { __list_sort(begin(), end()); }

    // template <class Cmp>
    // void sort(Cmp cmp) {
    //    __list_sort(begin(), end(), cmp);
    //}

    void reverse() {
        auto a = begin();
        auto b1 = end();
        auto b2 = end();
        b2--;

        for (; a != b1 && a != b2; ++a, --b1, --b2) a.swap_nodes(b2);
    }

    /*
    void check() const {
        auto o = static_cast<const ListNodeBase *>(this);
        auto p = o->next;
        size_t s = 0;

        while (p != o) {
            p = p->next;
            ++s;
        }

        VTSS_ASSERT(s == size_);

        o = static_cast<const ListNodeBase *>(this);
        p = o->prev;
        s = 0;

        while (p != o) {
            p = p->prev;
            ++s;
        }

        VTSS_ASSERT(s == size_);
    }
    */

  private:
    bool __splice(ListNodeBase *pos, List &rhs, size_t s, ListNodeBase *begin,
                  ListNodeBase *last) {
        if (s == 0) return true;
        if (size_ + s > max_size_) return false;
        if (&rhs == this) return false;

        size_ += s;
        rhs.size_ -= s;

        // Unlink the range from the current list
        begin->prev->next = last->next;
        last->next->prev = begin->prev;

        // Hook in the new list
        begin->prev = pos->prev;
        last->next = pos;
        pos->prev->next = begin;
        pos->prev = last;

        return true;
    }

    void move_(List &&rhs) {
        if (!rhs.empty()) {
            // steal the pointers and size from 'rhs'
            size_ = rhs.size_;
            next = rhs.next;
            prev = rhs.prev;

            // make sure that they points back to this - to close the ring
            next->prev = static_cast<ListNodeBase *>(this);
            prev->next = static_cast<ListNodeBase *>(this);

            // update 'rhs'
            rhs.size_ = 0;
            rhs.next = static_cast<ListNodeBase *>(&rhs);
            rhs.prev = static_cast<ListNodeBase *>(&rhs);
        }
    }

    template <class I>
    void overwrite(I i, I e) {
        auto ii = begin();

        // Just assign one at a time
        for (; i != e; ++i, ++ii) *ii = *i;
    }

    void *alloc_and_increase_size() {
        if (size_ >= max_size_) return nullptr;

        auto p = VTSS_BASICS_MALLOC(sizeof(ListNode<TYPE>));

        if (p) size_++;

        return p;
    }

    template <class I>
    void construct_range(I i, I e) {
        // Error handling assumes that we are already empty

        // Just insert one at a time
        while (i != e)
            if (!push_back(*i++)) break;

        // Clear if we did not succede
        if (i != e) clear();
    }

    size_t max_size_ = (size_t)-1;
    size_t size_ = 0;
};

template <typename T>
void swap(vtss::List<T> &a, vtss::List<T> &b) {
    return a.swap(b);
}

template <class TYPE>
inline bool operator==(const List<TYPE> &a, const List<TYPE> &b) {
    return a.size() == b.size() && equal(a.begin(), a.end(), b.begin());
}

template <class TYPE>
inline bool operator<(const List<TYPE> &a, const List<TYPE> &b) {
    return lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

template <class TYPE>
inline bool operator!=(const List<TYPE> &a, const List<TYPE> &b) {
    return !(a == b);
}

template <class TYPE>
inline bool operator>(const List<TYPE> &a, const List<TYPE> &b) {
    return b < a;
}

template <class TYPE>
inline bool operator>=(const List<TYPE> &a, const List<TYPE> &b) {
    return !(a < b);
}

template <class TYPE>
inline bool operator<=(const List<TYPE> &a, const List<TYPE> &b) {
    return !(b < a);
}

}  // namespace vtss

#endif  // __VTSS_BASICS_LIST_HXX__
