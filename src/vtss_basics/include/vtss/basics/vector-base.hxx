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

#ifndef __VTSS_BASICS_VECTOR_BASE_HXX__
#define __VTSS_BASICS_VECTOR_BASE_HXX__

#include <new>
#include <initializer_list>
#include <vtss/basics/common.hxx>
#include <vtss/basics/utility.hxx>
#include <vtss/basics/iterator.hxx>
#include <vtss/basics/vector-memory.hxx>

namespace vtss {
template <typename TYPE, typename BASE, bool DYNMEM>
struct VectorBase;

template <typename Type, typename Container>
class VectorIterator : public iterator<random_access_iterator_tag, Type> {
  public:
    template <typename A, typename B, bool C>
    friend struct VectorBase;

    template <class A, class B>
    friend class VectorIterator;

    VectorIterator(Container* b, size_t i) : base(b), index(i) {}
    VectorIterator(const VectorIterator& rhs)
        : base(rhs.base), index(rhs.index) {}

    template <class OTHER_TYPE, class OTHER_CONTAINER>
    VectorIterator(const VectorIterator<OTHER_TYPE, OTHER_CONTAINER>& x,
                   typename std::enable_if<std::is_convertible<
                           OTHER_CONTAINER*, Container*>::value>::type* = 0)
        : base(x.base), index(x.index) {}

    Type* operator->() const { return &((*base)[index]); }
    Type& operator*() const { return (*base)[index]; }

    VectorIterator& operator++() {
        ++index;
        return *this;
    }

    VectorIterator& operator--() {
        --index;
        return *this;
    }

    VectorIterator operator+=(size_t x) {
        index += x;
        return *this;
    }

    bool operator==(const VectorIterator& rhs) {
        VTSS_ASSERT(base == rhs.base);
        return index == rhs.index;
    }

    bool operator<(const VectorIterator& rhs) {
        VTSS_ASSERT(base == rhs.base);
        return index < rhs.index;
    }

    size_t operator-(const VectorIterator<Type, Container>& rhs) const {
        return index - rhs.index;
    }

    explicit operator bool() const { return index < base->size(); }

  private:
    Container* base;
    size_t index;
};

template <typename TYPE, typename BASE, bool DYNMEM = false>
struct VectorBase {
    typedef VectorBase<TYPE, BASE, DYNMEM> THIS;
    typedef TYPE value_type;
    typedef TYPE* pointer;
    typedef const TYPE* const_pointer;

    typedef TYPE& reference;
    typedef const TYPE& const_reference;

    typedef ItrWrap<VectorIterator<TYPE, THIS>> iterator;
    typedef ItrWrap<VectorIterator<const TYPE, const THIS>> const_iterator;

    typedef vtss::reverse_iterator<iterator> reverse_iterator;
    typedef vtss::reverse_iterator<const_iterator> const_reverse_iterator;

    VectorBase() : size_(0) {}

    // No good options for error reporting here - therefore deleted
    VectorBase(const VectorBase& rhs) = delete;
    VectorBase& operator=(const VectorBase& rhs) = delete;

    size_t capacity() { return static_cast<BASE*>(this)->capacity(); };
    value_type* data() { return static_cast<BASE*>(this)->data(); };
    const value_type* data() const {
        return static_cast<const BASE*>(this)->data();
    };

    template <class I>
    bool assign(
            I range_i, I range_end,
            typename std::enable_if<is_input_iterator<I>::value>::type* = 0) {
        // Calculate size of range to be assigned
        size_t n = vtss::distance(range_i, range_end);

        // Check if we have room for this
        if (!make_room_assign(n)) return false;

        size_t s = n;
        TYPE* i = data();
        TYPE* e = data() + size_;

        // Copy assign the overlapping area
        for (; i != e && n; --n, ++i, ++range_i) *i = *range_i;

        // Delete the remaining part (will only iterate if n < size_)
        for (; i != e; ++i) i->~TYPE();

        // Copy construct the new area (will only iterate if n > size_)
        for (; n; --n, ++i, ++range_i) ::new ((void*)i) TYPE(*range_i);

        // Update size (not exception safe)
        size_ = s;

        return true;
    }

    bool assign(size_t n, const value_type& x) {
        // Check if we have room for this
        if (!make_room_assign(n)) return false;

        size_t s = n;
        TYPE* i = data();
        TYPE* e = data() + size_;

        // Copy assign the overlapping area
        for (; i != e && n; --n, ++i) *i = x;

        // Delete the remaining part (will only iterate if n < size_)
        for (; i != e; ++i) i->~TYPE();

        // Copy construct the new area (will only iterate if n > size_)
        for (; n; ++i, --n) ::new ((void*)i) TYPE(x);

        // Update size (not exception safe)
        size_ = s;

        return true;
    }

    bool assign(std::initializer_list<TYPE> il) {
        return assign(il.begin(), il.end());
    }

    iterator begin() { return make_iter(0); }
    const_iterator begin() const { return make_iter(0); }
    iterator end() { return make_iter(size_); }
    const_iterator end() const { return make_iter(size_); }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const { return rbegin(); }
    const_reverse_iterator crend() const { return rend(); }


    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    reference operator[](size_t n) {
        VTSS_ASSERT(n < size_);
        return *(data() + n);
    }

    const_reference operator[](size_t n) const {
        VTSS_ASSERT(n < size_);
        return *(data() + n);
    }

    iterator at(size_t n) { return make_iter(n); }
    const_iterator at(size_t n) const { return make_iter(n); }

    reference front() {
        VTSS_ASSERT(size_ > 0);
        return *(data());
    }

    const_reference front() const {
        VTSS_ASSERT(size_ > 0);
        return *(data());
    }

    reference back() {
        VTSS_ASSERT(size_ > 0);
        return *(data() + size_ - 1);
    }

    const_reference back() const {
        VTSS_ASSERT(size_ > 0);
        return *(data() + size_ - 1);
    }

    bool push_back(const value_type& x) {
        if (!make_room_push_back()) return false;

        ::new ((void*)(data() + size_)) TYPE(x);
        size_++;

        return true;
    }

    bool push_back(value_type&& x) {
        if (!make_room_push_back()) return false;

        ::new ((void*)(data() + size_)) TYPE(vtss::move(x));
        size_++;

        return true;
    }

    template <class... Args>
    bool emplace_back(Args&&... args) {
        if (!make_room_push_back()) return false;

        ::new ((void*)(data() + size_)) TYPE(vtss::forward<Args>(args)...);
        size_++;

        return true;
    }

    bool pop_back() {
        if (size_ == 0) return false;

        TYPE* p = data() + (size_ - 1);
        p->~TYPE();
        size_--;

        return true;
    }

    void clear() {
        TYPE* p = data();
        for (size_t i = 0; i < size_; ++i, ++p) p->~TYPE();
        size_ = 0;
    }

    template <class... Args>
    bool emplace(const_iterator p, Args&&... args) {
        VTSS_ASSERT(p.base().base == this && p.base().index <= size_);

        // Make room for the new entry, exit if there is not room for the new
        // entries.
        if (!make_room_insert(1, p.base().index)) return false;

        // Where to insert
        TYPE* i = data() + p.base().index;

        // If inserted at the end, then copy-construct otherwise assign
        // construct
        if (p.base().index == size_)
            ::new ((void*)(i)) TYPE(vtss::forward<Args>(args)...);
        else
            *i = TYPE(vtss::forward<Args>(args)...);

        ++size_;

        return true;
    }

    bool insert(const_iterator p, const value_type& x) {
        return insert(p, (size_t)1, x);
    }

    bool insert(const_iterator p, value_type&& x) {
        VTSS_ASSERT(p.base().base == this && p.base().index <= size_);

        // Make room for the new entry, exit if there is not room for the new
        // entries.
        if (!make_room_insert(1, p.base().index)) return false;

        // Where to insert
        TYPE* i = data() + p.base().index;

        // If inserted at the end, then copy-construct otherwise assign
        // construct
        if (p.base().index == size_)
            ::new ((void*)(i)) TYPE(vtss::move(x));
        else
            *i = vtss::move(x);

        ++size_;

        return true;
    }

    bool insert(const_iterator p, size_t n, const value_type& x) {
        VTSS_ASSERT(p.base().base == this && p.base().index <= size_);

        // Make room for the new entries, exit if there is not room for the new
        // entries.
        if (!make_room_insert(n, p.base().index)) return false;

        // Where to start insert from
        TYPE* i = data() + p.base().index;

        // End mark of where to stop assign constructing (and continue copy
        // constructing)
        TYPE* end = data() + size_;

        // Update size before we start to modify n (not exception safe)
        size_ += n;

        // assign-copy the overlapping area
        for (; i != end && n; --n, ++i) *i = x;

        // copy construct the parts that go into new storage
        for (; n; --n, ++i) ::new ((void*)(i)) TYPE(x);

        return true;
    }

    template <class I>
    bool insert(
            const_iterator p, I b, I e,
            typename std::enable_if<is_input_iterator<I>::value>::type* = 0) {
        VTSS_ASSERT(p.base().base == this && p.base().index <= size_);

        // Calculate how many elements there is in the range [b, e[
        size_t n = vtss::distance(b, e);

        // Make room for the new entries, exit if there is not room for the new
        // entries.
        if (!make_room_insert(n, p.base().index)) return false;

        // Where to start insert from
        TYPE* i = data() + p.base().index;

        // End mark of where to stop assign constructing (and continue copy
        // constructing)
        TYPE* end = data() + size_;

        // Update size before we start to modify n (not exception safe)
        size_ += n;

        // assign-copy the overlapping area
        for (; i != end && n; --n, ++i, ++b) *i = *b;

        // copy construct the parts that go into new storage
        for (; n; --n, ++i, ++b) ::new ((void*)(i)) TYPE(*b);

        return true;
    }

    bool insert(const_iterator p, std::initializer_list<TYPE> il) {
        return insert(p, il.begin(), il.end());
    }

    void erase(const_iterator p) { erase(p, p + 1); }

    void erase(const_iterator range_begin, const_iterator range_end) {
        VTSS_ASSERT(range_begin.base().base == this &&
                    range_end.base().base == this &&
                    range_end.base().index <= size_ &&
                    range_begin.base().index <= range_end.base().index);

        TYPE* dst = data() + range_begin.base().index;
        TYPE* src = data() + range_end.base().index;
        TYPE* e = data() + size_;

        // Assign copy the overlapping area
        for (; src != e; ++src, ++dst) *dst = vtss::move(*src);

        // Destruct the remaining part
        for (; dst != e; ++dst) dst->~TYPE();

        // Update the new size
        size_ -= (range_end - range_begin);
    }

  protected:
    size_t size_ = 0;

  private:
    VectorMemory memory_allocate(size_t cap) {
        return static_cast<BASE*>(this)->memory_allocate(cap);
    }

    VectorMemory memory_grows(size_t min) {
        return static_cast<BASE*>(this)->memory_grows(min);
    }

    void memory_replace(VectorMemory&& m) {
        static_cast<BASE*>(this)->memory_replace(vtss::move(m));
    }

    bool make_room_assign(size_t s) {
        // If we have room for one more - then we are done
        if (s <= capacity()) return true;

        // If we do not have access to dynamic memory - then there is no more we
        // can do
        if (!DYNMEM) return false;

        // Needs to allocate new memory area to increase capacity
        auto m = memory_allocate(s);
        if (m.capacity() < s) return false;

        // Deletes all existing elements (will set size_ to zero)
        clear();

        // Replace the memory area
        memory_replace(vtss::move(m));

        // Report success
        return true;
    }

    bool make_room_push_back() {
        // If we have room for one more - then we are done
        if (size_ + 1 <= capacity()) return true;

        // If we do not have access to dynamic memory - then there is noting we
        // can do
        if (!DYNMEM) return false;

        // Needs to allocate new memory area to increase capacity
        auto m = memory_grows(size_ + 1);
        if (m.capacity() < size_ + 1) return false;

        // move construct the existing elements to the new storage
        TYPE* src = data();
        TYPE* src_e = data() + size_;
        TYPE* dst = (TYPE*)m.data();

        for (; src != src_e; ++src, ++dst) {
            // move the element from the old storage
            ::new ((void*)(dst)) TYPE(vtss::move(*src));

            // destruct the moved element
            src->~TYPE();
        }

        // Replace the memory area
        memory_replace(vtss::move(m));

        // Report success
        return true;
    }

    bool make_room_insert(size_t distance, size_t index) {
        if (size_ + distance > capacity()) {
            // Try to help the compiler optimizer making the right decisions
            if (!DYNMEM) return false;

            // Needs to re-reallocate to increase capacity
            auto m = memory_grows(size_ + distance);
            if (m.capacity() < size_ + distance) return false;

            // move construct the existing elements to the new storage
            TYPE* src = data();
            TYPE* src_e = data() + index;
            TYPE* dst = (TYPE*)m.data();

            // Move the elements before the split
            for (; src != src_e; ++src, ++dst) {
                // move the element from the old storage
                ::new ((void*)(dst)) TYPE(vtss::move(*src));

                // destruct the moved element
                src->~TYPE();
            }

            // Adjust pointers to work on elements after the split
            src_e = data() + size_;
            dst += distance;

            // Move the elements after the split
            for (; src != src_e; ++src, ++dst) {
                // move the element from the old storage
                ::new ((void*)(dst)) TYPE(vtss::move(*src));

                // destruct the moved element
                src->~TYPE();
            }

            // Replace the memory area
            memory_replace(vtss::move(m));

            // Report success
            return true;
        }

        // Setup reverse pointers (assume we want to insert at the '*' and
        // distance == 5
        TYPE* r_dst = data() + size_ + distance - 1;
        TYPE* r_begin = data() + size_ - 1;
        TYPE* r_end = data() + index - 1;
        TYPE* r_i = r_begin;
        // |1|2|3*|4|5|6|.|.|.|.|.|
        //    ^        ^         ^
        //   r_end   r_begin    r_dst
        //           r_i

        // Copy construct the element which goes to "new" storage
        for (size_t i = 0; i < distance && r_i != r_end; ++i)
            ::new ((void*)(r_dst--)) TYPE(vtss::move(*r_i--));

        // assign copy the overlapping area
        while (r_i != r_end) *r_dst-- = vtss::move(*r_i--);

        return true;
    }

    iterator make_iter(size_t s) {
        return iterator::create(VectorIterator<TYPE, THIS>(this, s));
    }

    const_iterator make_iter(size_t s) const {
        return const_iterator::create(
                VectorIterator<const TYPE, const THIS>(this, s));
    }
};
}  // namespace vtss

#endif  // __VTSS_BASICS_VECTOR_BASE_HXX__
