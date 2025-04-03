/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_ARRAY_HXX__
#define __VTSS_BASICS_ARRAY_HXX__

#include <stddef.h>
#include <algorithm>
#include <vtss/basics/iterator.hxx>
#include <vtss/basics/common.hxx>

namespace vtss {

template <typename Type, size_t N>
class ArrayIterator : public iterator<random_access_iterator_tag, Type>{
  public:
    explicit ArrayIterator(Type* b, size_t i) : base(b), index(i) {}
    ArrayIterator(const ArrayIterator& rhs)
        : base(rhs.base), index(rhs.index) {}

    Type* operator->() {
        VTSS_ASSERT(index < N);
        return &base[index];
    }

    Type& operator*() {
        VTSS_ASSERT(index < N);
        return base[index];
    }

    ArrayIterator& operator++() {
        ++index;
        return *this;
    }

    ArrayIterator operator++(int) {
        ArrayIterator old(*this);
        ++index;
        return old;
    }

    ArrayIterator& operator--() {
        --index;
        return *this;
    }

    ArrayIterator operator--(int) {
        ArrayIterator old(*this);
        --index;
        return old;
    }

    bool operator==(const ArrayIterator& rhs) {
        return base == rhs.base && index == rhs.index;
    }

    bool operator!=(const ArrayIterator& rhs) {
        return base != rhs.base || index != rhs.index;
    }

    size_t operator-(const ArrayIterator& rhs) const {
        return index - rhs.index;
    }

    ArrayIterator operator+=(size_t x) {
        index += x;
        return *this;
    }

  private:
    Type* base;
    size_t index;
};

template <typename Type, size_t N>
struct Array {
    typedef Type& reference;
    typedef const Type& const_reference;
    typedef ArrayIterator<Type, N> iterator;
    typedef ArrayIterator<const Type, N> const_iterator;

    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef Type value_type;
    typedef Type* pointer;
    typedef const Type* const_pointer;

    Array() = default;

    size_t size() const { return N; }
    iterator begin() { return iterator(data, 0); }
    iterator end()   { return iterator(data, N); }
    const_iterator begin()  const { return const_iterator(data, 0); }
    const_iterator end()    const { return const_iterator(data, N); }
    const_iterator cbegin() const { return const_iterator(data, 0); }
    const_iterator cend()   const { return const_iterator(data, N); }

    reference at(size_t i) {
        VTSS_ASSERT(i < N);
        return data[i];
    }

    const_reference at(size_t i) const {
        VTSS_ASSERT(i < N);
        return data[i];
    }

    // An implementation of belong iterator in-case this array is used along
    // with the VTSS-Expose framework.
    mesa_rc itr(const size_t *in, size_t *out) const {
        if (!in) {
            *out = 0;
            return MESA_RC_OK;
        }
        if (*in >= (N - 1)) return MESA_RC_ERROR;
        *out = *in + 1;
        return MESA_RC_OK;
    }

    // An implementation of belong iterator in-case this array is used along
    // with the VTSS-Expose framework. This iterator is assuming that the
    // get/set functions is converting the normal zero indexing to 1-indexing.
    mesa_rc itr_1based(const size_t *in, size_t *out) const {
        if (!in) {
            *out = 1;
            return MESA_RC_OK;
        }

        if (*in >= N) return MESA_RC_ERROR;

        *out = *in + 1;
        return MESA_RC_OK;
    }

    reference operator[](size_t i) { return at(i); }
    const_reference operator[](size_t i) const { return at(i); }

    Type data[N];
};

template<typename Type, size_t N>
inline bool operator==(const Array<Type, N> &a, const Array<Type, N> &b) {
    return std::equal(a.data, a.data + N, b.data);
}

template<typename Type, size_t N>
inline bool operator!=(const Array<Type, N> &a, const Array<Type, N> &b) {
    return !(a == b);
}

template<typename Type, size_t N>
inline bool operator<(const Array<Type, N>& a, const Array<Type, N>& b) {
    return std::lexicographical_compare(a.data, a.data + N, b.data, b.data + N);
}

template<typename Type, size_t N>
inline bool operator>(const Array<Type, N>& a, const Array<Type, N>& b) {
    return b < a;
}

template<typename Type, size_t N>
inline bool operator<=(const Array<Type, N>& a, const Array<Type, N>& b) {
    return !(b < a);
}

template<typename Type, size_t N>
inline bool operator>=(const Array<Type, N>& a, const Array<Type, N>& b) {
    return !(a < b);
}

}  // namespace vtss

#endif  // __VTSS_BASICS_ARRAY_HXX__
