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

#ifndef __VTSS_BASICS_PTR_BITS2_HXX__
#define __VTSS_BASICS_PTR_BITS2_HXX__

#include <cstdint>

namespace vtss {

// The purpose of this class is to act as a point of type 'T *', but squeeze in
// two extra bits. The two extra bits are located in the two least significant
// bits of the pointer, which means that it only works for types with an
// alignment of 4 or more. The alignment assumption is validated by a static
// assert in the constructor.

template <typename T>
struct PtrBits2 {
    static_assert(alignof(T) >= 4, "Assumption: 'alignof(T) >= 4' not true");

    PtrBits2() = default;
    explicit PtrBits2(T *t, bool b0 = false, bool b1 = false) {
        ptr(t);
        bit0(b0);
        bit1(b1);
    }

    // Read pointer value
    T *ptr() { return (T *)(ptr_ & MASK_BOTH); }
    const T *ptr() const { return (T *)(ptr_ & MASK_BOTH); }

    // Access a member by following the pointer 'T *'
    T *operator->() { return ptr(); }
    const T *operator->() const { return ptr(); }

    // Dereference the object as if it was a normal pointer of type 'T *'
    T &operator*() { return *ptr(); }
    const T &operator*() const { return *ptr(); }

    // Set a new pointer value
    void ptr(T *t) { ptr_ = (ptr_ & SELECT_BOTH) | ((uintptr_t)t & MASK_BOTH); }

    // Assign a new pointer value
    PtrBits2 &operator=(T *t) {
        ptr(t);
        return *this;
    }

    // Check if the pointer is assigned
    operator bool() const { return ptr() != nullptr; }

    // Query bit0 and bit1 encoded in the pointer
    bool bit0() const { return ptr_ & SELECT_BIT_0; }
    bool bit1() const { return ptr_ & SELECT_BIT_1; }

    // Set bit0 and bit1 encoded in the pointer
    void bit0(bool b) { ptr_ = (ptr_ & MASK_BIT_0) | (b << POS_BIT_0); }
    void bit1(bool b) { ptr_ = (ptr_ & MASK_BIT_1) | (b << POS_BIT_1); }

  private:
    static constexpr uintptr_t POS_BIT_0 = 0x0;
    static constexpr uintptr_t POS_BIT_1 = 0x1;
    static constexpr uintptr_t SELECT_BIT_0 = 0x1 << POS_BIT_0;
    static constexpr uintptr_t SELECT_BIT_1 = 0x1 << POS_BIT_1;
    static constexpr uintptr_t SELECT_BOTH = SELECT_BIT_0 | SELECT_BIT_1;
    static constexpr uintptr_t MASK_BIT_0 = ~SELECT_BIT_0;
    static constexpr uintptr_t MASK_BIT_1 = ~SELECT_BIT_1;
    static constexpr uintptr_t MASK_BOTH = ~SELECT_BOTH;
    uintptr_t ptr_;
};

// All comparison operators operates on the "pointer" value ignoring the flags.

#define VTSS_BASICS_PTR_BIT_OPR(X)                                \
    template <typename T>                                         \
    bool operator X(const PtrBits2<T> &a, const PtrBits2<T> &b) { \
        return a.ptr() X b.ptr();                                 \
    }                                                             \
    template <typename T>                                         \
    bool operator X(const PtrBits2<T> &a, const T *const b) {     \
        return a.ptr() X b;                                       \
    }                                                             \
    template <typename T>                                         \
    bool operator X(const T *const a, const PtrBits2<T> &b) {     \
        return a X b.ptr();                                       \
    }

VTSS_BASICS_PTR_BIT_OPR(==)
VTSS_BASICS_PTR_BIT_OPR(!=)
VTSS_BASICS_PTR_BIT_OPR(<)
VTSS_BASICS_PTR_BIT_OPR(>)
VTSS_BASICS_PTR_BIT_OPR(<=)
VTSS_BASICS_PTR_BIT_OPR(>=)

}  // namespace vtss

#endif  // __VTSS_BASICS_PTR_BITS2_HXX__
