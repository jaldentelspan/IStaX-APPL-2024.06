/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __RINGBUF_HXX__
#define __RINGBUF_HXX__

#include <vtss/basics/notifications/lock-global-subject.hxx>
#include <vtss/basics/iterator.hxx>

namespace vtss {

template <typename TYPE, unsigned SIZE>
class RingBuf;

template <typename TYPE, typename Container, unsigned SIZE>
class RingBufIterator : public iterator<bidirectional_iterator_tag, TYPE> {
  public:
    RingBufIterator() : base(nullptr), offset(0) {}
    RingBufIterator(Container *b, size_t i) : base(b), offset(i) {}
    RingBufIterator(const RingBufIterator& rhs) : base(rhs.base), offset(rhs.offset) {}

    TYPE* operator->() const {
        size_t idx = base->head_ + offset;

        if (idx >= SIZE) {
            idx -= SIZE;
        }

        return &base->data_[idx];
    }

    TYPE& operator*() const {
        size_t idx = base->head_ + offset;

        if (idx >= SIZE) {
            idx -= SIZE;
        }

        return base->data_[idx];
    }

    RingBufIterator& operator++() {
        ++offset;
        return *this;
    }

    RingBufIterator& operator--() {
        --offset;
        return *this;
    }

    bool operator==(const RingBufIterator& rhs) {
        VTSS_ASSERT(base == rhs.base);
        return offset == rhs.offset;
    }

  private:
    Container* base;
    size_t offset; // Offset from base->head_ ([0; SIZE])
};

template<typename T, unsigned SIZE>
class RingBuf {
  public:
    typedef RingBuf<T, SIZE> THIS;

    template <typename A, typename B, unsigned C> friend class RingBufIterator;

    typedef ItrWrap<RingBufIterator<T, THIS, SIZE>> iterator;
    typedef ItrWrap<RingBufIterator<const T, const THIS, SIZE>> const_iterator;

    typedef vtss::reverse_iterator<iterator> reverse_iterator;
    typedef vtss::reverse_iterator<const_iterator> const_reverse_iterator;


    RingBuf() {
        clear();
    }

    // Push will overwrite if needed! returns true if not overwrite was needed,
    // and false if tail was overwrited
    template <typename TT>
    bool push(const TT &t) {
        bool res;

        // make sure there is room for one more
        if (full_) {
            head_increment();
            res = false;
        } else {
            res = true;
        }

        // Store data at tails current position.
        data_[tail_] = t;
        tail_increment();

        // can not be empty after we have pushed an element
        if (tail_ == head_) // full
            full_ = true;

        return res;
    }

    // Returns true if a value was pop'ed
    bool pop(T& t) {
        if (empty())
            return false;

        t = data_[head_];
        head_increment();
        full_ = false; // can't be full after a pop

        return true;
    }

    bool empty() const {
        return head_ == tail_ && !full_;
    }

    bool full() const {
        return full_;
    }

    unsigned size() const {
        if (full_)
            return SIZE;

        if (head_ <=  tail_)
            return tail_ - head_;

        return (SIZE - head_) + tail_;
    }

    void clear() {
        full_ = false;
        head_ = 0;
        tail_ = 0;
    }

    iterator begin() { return make_iter(0); }
    const_iterator begin() const { return make_iter(0); }
    iterator end() { return make_iter(size()); }
    const_iterator end() const { return make_iter(size()); }
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

  private:
    void head_increment() {
        head_++;
        if (head_ >= SIZE) // wrap
            head_ = 0;
    }

    void tail_increment() {
        tail_++;
        if (tail_ >= SIZE) // wrap
            tail_ = 0;
    }

    iterator make_iter(size_t s) {
        return iterator::create(RingBufIterator<T, THIS, SIZE>(this, s));
    }

    const_iterator make_iter(size_t s) const {
        return const_iterator::create(
            RingBufIterator<const T, const THIS, SIZE>(this, s));
    }

    bool full_;
    unsigned head_, tail_;
    T data_[SIZE];
};

template<typename T, unsigned SIZE>
class RingBufConcurrent {
  public:
    RingBufConcurrent() {
        vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        impl.clear();
    }

    template <typename TT>
    bool push(const TT& t) {
        vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        return impl.template push<TT>(t);
    }

    bool pop(T& t) {
        vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        return impl.pop(t);
    }

    bool empty() const {
        vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        return impl.empty();
    }

    bool full() const {
        vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        return impl.full();
    }

    unsigned size() const {
        vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        return impl.size();
    }

    void clear() {
        vtss::notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        impl.clear();
    }

  private:
    RingBuf<T, SIZE> impl;
};

}  // namespace vtss
#endif  // __RINGBUF_HXX__
