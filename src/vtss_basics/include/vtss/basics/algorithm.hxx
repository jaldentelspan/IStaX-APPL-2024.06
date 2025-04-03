/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _ALGORITHM_H_
#define _ALGORITHM_H_

#include <stddef.h>
#include <initializer_list>
#include "vtss/basics/api_types.h"
#include "vtss/basics/utility.hxx"
#include "vtss/basics/iterator.hxx"
#include "vtss/basics/__functional_base.hxx"

namespace vtss {

inline const char* find_end(const char* s) {
    if (s == 0) return 0;
    if (*s == 0) return s;
    while (*(++s) != 0) continue;
    return s;
}

template <class I, class T0>
I find_not(I first, I last, const T0& value) {
    for (; first != last; ++first)
        if (*first != value) break;
    return first;
}

template <typename T0, typename P>
T0 find_first_if(T0 b, T0 e, P p) {
    for (; b != e; b++)
        if (p(*b)) break;
    return b;
}

template <typename T0, typename P>
T0 find_last_if(T0 b, T0 e, P p) {
    T0 i = b;
    for (; b != e; b++)
        if (p(*b)) i = b;
    return i;
}

template <typename T0, typename B>
void signed_to_dec_rbuf(T0 val, B& buf, bool force_sign = false) {
    bool sign = false;

    if (val < 0)  // do not multiply by -1 here, it will cause overflow!
        sign = true;

    // First iteration is unroled to print 0, and to avoid overflow
    if (sign) {
        buf.push((static_cast<char>(val % 10) * -1) + '0');
        val /= 10;
        val *= -1;
    } else {
        buf.push(static_cast<char>(val % 10) + '0');
        val /= 10;
    }

    while (val) {
        buf.push(static_cast<char>(val % 10) + '0');
        val /= 10;
    }

    if (sign)
        buf.push('-');
    else if (force_sign)
        buf.push('+');
}

template <typename T0, typename B>
void unsigned_to_dec_rbuf(T0 val, B& buf) {
    if (val == 0) buf.push('0');

    while (val) {
        buf.push(static_cast<char>(val % 10) + '0');
        val /= 10;
    }
}

template <typename T0, typename B>
void unsigned_to_dec_rbuf_fill(T0 val, B& buf, uint32_t s, char c = '0') {
    size_t size_old = buf.end() - buf.begin();
    unsigned_to_dec_rbuf(val, buf);
    size_t size = (buf.end() - buf.begin()) - size_old;

    for (uint32_t i = size; i < s; ++i) buf.push(c);
}

template <typename T0, typename B>
void unsigned_to_oct_rbuf(T0 val, B& buf) {
    if (val == 0) buf.push('0');

    while(val) {
        buf.push(static_cast<char>(val % 8) + '0');
        val /= 8;
    }
}

template <typename T0, typename B>
void unsigned_to_hex_rbuf(T0 val, B& buf, char base, int min_width = 0,
                          char fill = '0') {
    if (val == 0) {
        buf.push('0');
        --min_width;
    }

    while (val) {
        char v = val & 0xf;
        if (v > 9)
            buf.push((v - 10) + base);
        else
            buf.push(v + '0');
        val >>= 4;
        // If val has the most sginificant bit set, right shifting will padd with 0b'1
        // Therefore we mask out the 4 most significant bits.
        val &= ~(((T0)0xf) << (sizeof(T0) * 8 - 4));
        --min_width;
    }

    while (min_width > 0) {
        buf.push(fill);
        --min_width;
    }
}

template <class I, class S, class O>
O& join(O& o, const S& s, I begin, I end) {
    if (begin == end) return o;

    o << *begin;
    ++begin;

    for (; begin != end; ++begin) o << s << *begin;

    return o;
}

template <class I, class S, class O, class FMT>
O& join(O& o, const S& s, I begin, I end, FMT fmt) {
    if (begin == end) return o;

    o << fmt(*begin);
    ++begin;

    for (; begin != end; ++begin) o << s << fmt(*begin);

    return o;
}



// Non-modifying sequence operations ///////////////////////////////////////////

// all_of
template <class I, class P>
inline bool all_of(I i, I e, P pred) {
    for (; i != e; ++i)
        if (!pred(*i)) return false;
    return true;
}

// any_of
template <class I, class P>
inline bool any_of(I i, I e, P pred) {
    for (; i != e; ++i)
        if (pred(*i)) return true;
    return false;
}

// none_of
template <class I, class P>
inline bool none_of(I i, I e, P pred) {
    for (; i != e; ++i)
        if (pred(*i)) return false;
    return true;
}

// for_each
template <class I, class F>
inline F for_each(I i, I e, F f) {
    for (; i != e; ++i) f(*i);
    return vtss::move(f);
}

// find
template <class I, class T>
inline I find(I i, I e, const T& v) {
    for (; i != e; ++i)
        if (*i == v) break;
    return i;
}

// find_if
template <class I, class P>
inline I find_if(I i, I e, P pred) {
    for (; i != e; ++i)
        if (pred(*i)) break;
    return i;
}

// find_if_not
template <class I, class P>
inline I find_if_not(I i, I e, P pred) {
    for (; i != e; ++i)
        if (!pred(*i)) break;
    return i;
}

// find_end
// Not implemented yet...

// find_first_of
template <class I1, class I2, class P>
I1 find_first_of(I1 i1, I1 e1, I2 i2, I2 e2, P pred) {
    for (; i1 != e1; ++i1)
        for (I2 j = i2; j != e2; ++j)
            if (pred(*i1, *j)) return i1;
    return e1;
}

template <class I1, class I2>
inline I1 find_first_of(I1 i1, I1 e1, I2 i2, I2 e2) {
    for (; i1 != e1; ++i1)
        for (I2 j = i2; j != e2; ++j)
            if (*i1 == *j) return i1;
    return e1;
}

// adjacent_find
template <class I, class P>
inline I adjacent_find(I i, I e, P pred) {
    if (i != e) {
        I __i = i;

        while (++__i != e) {
            if (pred(*i, *__i)) return i;
            i = __i;
        }
    }

    return e;
}

template <class I>
inline I adjacent_find(I i, I e) {
    if (i != e) {
        I __i = i;

        while (++__i != e) {
            if (*i == *__i) return i;
            i = __i;
        }
    }

    return e;
}

// count
template <class I, class T>
inline size_t count(I i, I e, const T& v) {
    size_t r = 0;
    for (; i != e; ++i)
        if (*i == v) ++r;
    return r;
}

// count_if
template <class I, class P>
inline size_t count_if(I i, I e, P pred) {
    size_t r = 0;
    for (; i != e; ++i)
        if (pred(*i)) ++r;
    return r;
}

// mismatch
// Not implemented yet

// equal
template <class I1, class I2, class P>
inline bool equal(I1 i1, I1 e1, I2 i2, P pred) {
    for (; i1 != e1; ++i1, ++i2)
        if (!pred(*i1, *i2)) return false;
    return true;
}

template <class I1, class I2>
inline bool equal(I1 i1, I1 e1, I2 i2) {
    for (; i1 != e1; ++i1, ++i2)
        if (!(*i1 == *i2)) return false;
    return true;
}

template <class I1, class I2, class P>
inline bool equal(I1 i1, I1 e1, I2 i2, I2 e2, P pred) {
    for (; i1 != e1 && i2 != e2; ++i1, ++i2)
        if (!pred(*i1, *i2)) return false;
    return i1 == e1 && i2 == e2;
}

template <class I1, class I2>
inline bool equal(I1 i1, I1 e1, I2 i2, I2 e2) {
    for (; i1 != e1 && i2 != e2; ++i1, ++i2)
        if (!(*i1 == *i2)) return false;
    return i1 == e1 && i2 == e2;
}

// is_permutation
// search
// search_n
// Not implemented yet


// Modifying sequence operations ///////////////////////////////////////////////

// copy
template <class I, class O>
inline O copy(I i, I e, O r) {
    while (i != e) *r++ = *i++;
    return r;
}

// copy_n
template <class I, class S, class O>
inline O copy_n(I i, S n, O r) {
    for (; n > 0; --n, ++r, ++i) *r = *i;
    return r;
}

// copy_if
template <class I, class O, class P>
inline O copy_if(I i, I e, O r, P pred) {
    for (; i != e; ++i) {
        if (pred(*i)) {
            *r = *i;
            ++r;
        }
    }
    return r;
}

// copy_backward
template <class I, class O>
inline O copy_backward(I i, I e, O r) {
    while (i != e) *--r = *--e;
    return r;
}

// move
template <class I, class O>
inline O move(I i, I e, O r) {
    for (; i != e; ++i, ++r) *r = vtss::move(*i);
    return r;
}

// move_backward
template <class I, class O>
inline O move_backward(I i, I e, O r) {
    while (i != e) *--r = vtss::move(*--e);
    return r;
}

// swap
// TODO...

// swap_ranges
// TODO...

// iter_swap
// TODO...

// transform
template <class I, class O, class OPR>
inline O transform(I i, I e, O r, OPR opr) {
    for (; i != e; ++i, ++r) *r = opr(*i);
    return r;
}

template <class I1, class I2, class O, class OPR>
inline O transform(I1 i1, I1 e1, I2 i2, O r, OPR opr) {
    for (; i1 != e1; ++i1, ++i2, ++r) *r = opr(*i1, *i2);
    return r;
}

// replace
template <class I, class T>
inline void replace(I i, I e, const T& old_value, const T& new_value) {
    for (; i != e; ++i)
        if (*i == old_value) *i = new_value;
}

// replace_if
template <class I, class P, class T>
inline void replace_if(I i, I e, P pred, const T& new_value) {
    for (; i != e; ++i)
        if (pred(*i)) *i = new_value;
}

// replace_copy
template <class I, class O, class T>
inline O replace_copy(I i, I e, O r, const T& old_value, const T& new_value) {
    for (; i != e; ++i, ++r)
        if (*i == old_value)
            *r = new_value;
        else
            *r = *i;
    return r;
}

// replace_copy_if
template <class I, class O, class P, class T>
inline O replace_copy_if(I i, I e, O r, P pred, const T& new_value) {
    for (; i != e; ++i, ++r)
        if (pred(*i))
            *r = new_value;
        else
            *r = *i;
    return r;
}

// fill
template <class I, class T>
inline void fill(I i, I e, const T& v) {
    for (; i != e; ++i) *i = v;
}

// fill_n
template <class O, class T>
inline O fill_n(O i, size_t n, const T& v) {
    for (; n > 0; ++i, --n) *i = v;
    return i;
}

// generate
template <class I, class G>
inline void generate(I i, I e, G gen) {
    for (; i != e; ++i) *i = gen();
}

// generate_n
template <class O, class G>
inline O generate_n(O i, size_t n, G gen) {
    for (; n > 0; ++i, --n) *i = gen();
    return i;
}

// remove
template <class I, class T>
I remove(I i, I e, const T& v) {
    I r = i;

    while (i != e) {
        if (!(*i == v)) {
            if (i != r) *r = vtss::move(*i);
            ++r;
        }
        ++i;
    }

    return r;
}

// remove_if
template <class I, class P>
I remove_if(I i, I e, P pred) {
    I r = i;

    while (i != e) {
        if (!pred(*i)) {
            if (i != r) *r = vtss::move(*i);
            ++r;
        }
        ++i;
    }

    return r;
}

// remove_copy
template <class I, class O, class T>
inline O remove_copy(I i, I e, O r, const T& v) {
    for (; i != e; ++i) {
        if (!(*i == v)) {
            if (i != r) *r = *i;
            ++r;
        }
    }
    return r;
}

// remove_copy_if
template <class I, class O, class P>
inline O remove_copy_if(I i, I e, O r, P pred) {
    for (; i != e; ++i) {
        if (!pred(*i)) {
            if (i != r) *r = *i;
            ++r;
        }
    }

    return r;
}

// unique
// unique_copy
// Not implemented yet

// reverse
template <class I>
inline void reverse(I i, I e) {
    while (i != e) {
        if (i == --e) break;
        swap(*i, *e);
        ++i;
    }
}

// reverse_copy
template <class I, class O>
inline O reverse_copy(I i, I e, O r) {
    for (; i != e; ++r) *r = *--e;
    return r;
}

// rotate
// rotate_copy
// random_shuffle
// shuffle
// Not implemented yet


// Partitions //////////////////////////////////////////////////////////////////

// is_partitioned
// partition
// stable_partition
// partition_copy
// partition_point
// Not implemented yet



// Sorting /////////////////////////////////////////////////////////////////////

// sort
template <class I, class Cmp>
inline I __sort_partition(I begin, I end, Cmp &cmp) {
    --end;
    auto x = end;
    I i = begin;
    I j = begin;

    // Notice - we are not swapping iterators here - but iterators which has
    // been dereferenced
    for (; j != end; ++j) {
        if (!(cmp(*x, *j))) {
            swap(*i, *j);
            ++i;
        }
    }

    swap(*i, *end);

    return i;
}

template <class I, class Cmp = vtss::less<>>
inline void sort(I b, I e, Cmp cmp = Cmp{}) {
    // Check if distance(b, e) > 1
    if (b == e) return;
    I b1 = b;
    ++b1;
    if (b1 == e) return;

    // Classic quick sort - could be improved performance wise
    auto q = __sort_partition(b, e, cmp);
    sort(b, q, cmp);
    sort(++q, e, cmp);
}

// stable_sort
// partial_sort
// partial_sort_copy
// is_sorted
template <class I, class Cmp = vtss::less<>>
inline bool is_sorted(I i, I e, Cmp cmp = Cmp{}) {
    if (i == e) return true;
    auto next = i;
    for (; ++next != e && cmp(*i, *next); ++i);
    return next == e;
}

// is_sorted_until
template <class I, class Cmp = vtss::less<>>
inline I is_sorted_until(I i, I e, Cmp cmp = Cmp{}) {
    if (i == e) return i;
    auto next = i;
    for (; ++next != e && cmp(*i, *next); ++i);
    return next;
}

// nth_element


// Binary search (operating on partitioned/sorted ranges) //////////////////////

// lower_bound
template <class I, class T, class Cmp = vtss::less<>>
inline I lower_bound(I i, I e, const T& v, Cmp cmp = Cmp{}) {
    size_t len = vtss::distance(i, e);

    while (len != 0) {
        size_t len_half = len / 2;
        I x = i;

        vtss::advance(x, len_half);

        if (cmp(*x,v)) {
            i = ++x;
            len -= len_half + 1;
        } else {
            len = len_half;
        }
    }

    return i;
}

// upper_bound
template <class I, class T, class Cmp = vtss::less<>>
inline I upper_bound(I i, I e, const T& v, Cmp cmp = Cmp{}) {
    size_t len = vtss::distance(i, e);

    while (len != 0) {
        size_t len_half = len / 2;
        I x = i;
        vtss::advance(x, len_half);
        if (cmp(v,*x)) {
            len = len_half;
        } else {
            i = ++x;
            len -= len_half + 1;
        }
    }

    return i;
}

// equal_range
template <class I, class T, class Cmp = vtss::less<>>
inline Pair<I, I> equal_range(I i, I e, const T& v, Cmp cmp = Cmp{}) {
    return Pair<I,I>(lower_bound(i, e, v, cmp), upper_bound(i, e, v, cmp));
}

// binary_search
template <class I, class T, class Cmp = vtss::less<>>
inline bool binary_search(I i, I e, const T& v, Cmp cmp = Cmp{}) {
    auto tmp = lower_bound(i, e, v, cmp);
    if (tmp != e) {
        return v == *tmp;
    }
    return false;
}

// Merge (operating on sorted ranges) //////////////////////////////////////////

// merge
// inplace_merge
// includes
// set_union
// set_intersection
// set_difference
// set_symmetric_difference
// Not implemented


// Heap ////////////////////////////////////////////////////////////////////////

// push_heap
// pop_heap
// make_heap
// sort_heap
// is_heap
// is_heap_until
// Not implemented


// Min/max /////////////////////////////////////////////////////////////////////

// min_element
template <class I, class C>
inline I min_element(I i, I e, C comp) {
    if (i != e) {
        I x = i;
        while (++x != e)
            if (comp(*x, *i)) i = x;
    }

    return i;
}

template <class I>
inline I min_element(I i, I e) {
    if (i != e) {
        I x = i;
        while (++x != e)
            if (*x < *i) i = x;
    }

    return i;
}

// max_element
template <class I, class C>
inline I max_element(I i, I e, C comp) {
    if (i != e) {
        I x = i;
        while (++x != e)
            if (comp(*i, *x)) i = x;
    }

    return i;
}

template <class I>
inline I max_element(I i, I e) {
    if (i != e) {
        I x = i;
        while (++x != e)
            if (*i < *x) i = x;
    }

    return i;
}

// min
template <class T, class C>
inline const T& min(const T& a, const T& b, C comp) {
    return comp(a, b) ? a : b;
}

template <class T>
inline const T& min(const T& a, const T& b) {
    return a < b ? a : b;
}

template <class T, class C>
inline T min(std::initializer_list<T> t, C comp) {
    return *vtss::min_element(t.begin(), t.end(), comp);
}

template <class T>
inline T min(std::initializer_list<T> il) {
    return *vtss::min_element(il.begin(), il.end());
}

// max
template <class T, class C>
inline const T& max(const T& a, const T& b, C comp) {
    return comp(a, b) ? b : a;
}

template <class T>
inline const T& max(const T& a, const T& b) {
    return a < b ? b : a;
}

template <class T, class C>
inline T max(std::initializer_list<T> il, C comp) {
    return *vtss::max_element(il.begin(), il.end(), comp);
}

template <class T>
inline T max(std::initializer_list<T> il) {
    return *vtss::max_element(il.begin(), il.end());
}

// minmax
// Not implemented yet

// minmax_element
// Not implemented yet

// Other ///////////////////////////////////////////////////////////////////////

// lexicographical_compare
template <class I1, class I2>
inline bool lexicographical_compare(I1 i1, I1 e1, I2 i2, I2 e2) {
    for (; i2 != e2; ++i1, ++i2) {
        if (i1 == e1 || *i1 < *i2) return true;
        if (*i2 < *i1) return false;
    }
    return false;
}

// next_permutation

// prev_permutation


}  // namespace vtss

#endif /* _ALGORITHM_H_ */
