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

#ifndef _MSCC_ETHERNET_SWITCH_API_UTILS_
#define _MSCC_ETHERNET_SWITCH_API_UTILS_

#include <stdio.h>
#include <microchip/ethernet/switch/api/types.h>
#include <microchip/ethernet/board/api/types.h>
#include <type_traits>
#include "vtss_alloc.h"
#include "primitives.hxx"

extern uint32_t VTSS_APPL_CACHE_MEBA_CAP_BOARD_PORT_COUNT;
extern uint32_t VTSS_APPL_CACHE_MEBA_CAP_BOARD_PORT_MAP_COUNT;

uint32_t vtss_appl_capability(const void *_inst_unused_, int cap);

extern const char *VTSS_F;
extern const char *VTSS_C;
extern int VTSS_L;

inline uint32_t fast_cap__(int cap, const char *f, int l, const char *c)
{
    VTSS_L = l;
    VTSS_F = f;
    VTSS_C = c;

    // A few capabilities which should be extra fast
    if (cap == MEBA_CAP_BOARD_PORT_COUNT &&
        VTSS_APPL_CACHE_MEBA_CAP_BOARD_PORT_COUNT != 0) {
        return VTSS_APPL_CACHE_MEBA_CAP_BOARD_PORT_COUNT;
    }

    if (cap == MEBA_CAP_BOARD_PORT_MAP_COUNT &&
        VTSS_APPL_CACHE_MEBA_CAP_BOARD_PORT_MAP_COUNT != 0) {
        return VTSS_APPL_CACHE_MEBA_CAP_BOARD_PORT_MAP_COUNT;
    }

    return vtss_appl_capability(nullptr, cap);
}

#define fast_cap(cap) fast_cap__(cap, __FILE__, __LINE__, #cap)

void cap_array_check_dim(size_t idx, size_t max);
#define CAP_ARRAY_CHECK_DIM(IDX, MAX) cap_array_check_dim(IDX, MAX)

template <typename T>
class A1 {
  public:
    A1(const A1 &) = delete;
    A1 &operator=(const A1 &rhs) = delete;
    A1(T *d, size_t _1) : data_(d), d1(_1) {}
    A1(A1 &&rhs) : data_(rhs.data_), d1(rhs.d1) {}
    ~A1(){} // Needed to make it non-POD, for static analysis to catch ARRSZ problem

    T &operator[](size_t idx) {
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return data_[idx];
    }
    const T &operator[](size_t idx) const {
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return data_[idx];
    }
    size_t size() const { return d1; }
    size_t mem_size() const { return d1 * sizeof(T); }
    const T *data() const { return data_; }
    T *data() { return data_; }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, void>::type clear() {
        memset(data_, 0, mem_size());
    }

  protected:
    T *const data_;
    const size_t d1;
};

template <typename T>
class A2 {
  public:
    A2(const A2 &) = delete;
    A2 &operator=(const A2 &rhs) = delete;
    A2(A2 &&rhs) : data_(rhs.data_), d1(rhs.d1), d2(rhs.d2) {}
    A2(T *d, size_t _1, size_t _2) : data_(d), d1(_1), d2(_2) {}
    ~A2(){} // Needed to make it non-POD, for static analysis to catch ARRSZ problem

    A1<T> operator[](size_t idx) {
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return A1<T>(data_ + (idx * d2), d2);
    }
    const A1<T> operator[](size_t idx) const {
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return A1<T>(data_ + (idx * d2), d2);
    }
    size_t size() const { return d1; }
    size_t mem_size() const { return d1 * d2 * sizeof(T); }
    const T *data() const { return data_; }
    T *data() { return data_; }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, void>::type clear() {
        memset(data_, 0, mem_size());
    }


  protected:
    T *data_ = nullptr;
    size_t d1 = 0, d2 = 0;
};

template <typename T>
class A3 {
  public:
    A3(const A3 &) = delete;
    A3 &operator=(const A3 &rhs) = delete;
    A3(A3 &&rhs) : data_(rhs.data_), d1(rhs.d1), d2(rhs.d2), d3(rhs.d3) {}
    A3(T *d, size_t x, size_t y, size_t z) : data_(d), d1(x), d2(y), d3(z) {}
    ~A3(){} // Needed to make it non-POD, for static analysis to catch ARRSZ problem

    A2<T> operator[](size_t idx) {
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return A2<T>(data_ + (idx * d2 * d3), d2, d3);
    }
    const A2<T> operator[](size_t idx) const {
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return A2<T>(data_ + (idx * d2 * d3), d2, d3);
    }
    size_t size() const { return d1; }
    size_t mem_size() const { return d1 * d2 * d3 * sizeof(T); }
    const T *data() const { return data_; }
    T *data() { return data_; }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, void>::type clear() {
        memset(data_, 0, mem_size());
    }

  protected:
    T *data_ = nullptr;
    size_t d1 = 0, d2 = 0, d3 = 0;
};

template <typename T, int... CAPS>
class CapArray;

template <typename T, int C1>
class CapArray<T, C1> {
  public:
    CapArray() {}
    CapArray &operator=(const CapArray &rhs) {
        init();
        for (size_t i = 0; i < d1; ++i) data_[i] = rhs.data()[i];
        return *this;
    }

    CapArray(const CapArray &rhs) {
        init();
        for (size_t i = 0; i < d1; ++i) data_[i] = rhs.data()[i];
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, bool>::type operator==(
            const CapArray &rhs) const {
        init();
        return memcmp(data(), rhs.data(), mem_size()) == 0;
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, bool>::type operator!=(
            const CapArray &rhs) const {
        return !this->operator==(rhs);
    }

    void init() const {
        if (data_) return;
        d1 = fast_cap(C1);
        data_ = (T *)VTSS_CALLOC_MODID(VTSS_MODULE_ID, d1, sizeof(T), __FILE__,
                                       __LINE__);
        if (!data_) return;
        for (size_t i = 0; i < d1; ++i) new (&(data_[i])) T();
    }

    ~CapArray() {
        if (!data_) return;
        for (size_t i = 0; i < d1; ++i) data_[i].~T();
        VTSS_FREE(data_);
    }

    T &operator[](size_t idx) {
        init();
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return data_[idx];
    }

    const T &operator[](size_t idx) const {
        init();
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return data_[idx];
    }

    size_t size() const {
        init();
        return d1;
    }

    size_t mem_size() const {
        init();
        return d1 * sizeof(T);
    }

    const T *data() const {
        init();
        return data_;
    }

    T *data() {
        init();
        return data_;
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, void>::type clear() {
        init();
        memset(data_, 0, mem_size());
    }

  private:
    mutable size_t d1;
    mutable T *data_ = nullptr;
};

template <typename T, int C1, int C2>
class CapArray<T, C1, C2> {
  public:
    CapArray() {}
    CapArray &operator=(const CapArray &rhs) {
        init();
        for (size_t i = 0; i < d1 * d2; ++i) data_[i] = rhs.data()[i];
        return *this;
    }

    CapArray(const CapArray &rhs) {
        init();
        for (size_t i = 0; i < d1 * d2; ++i) data_[i] = rhs.data()[i];
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, bool>::type operator==(
            const CapArray &rhs) const {
        init();
        return memcmp(data(), rhs.data(), mem_size()) == 0;
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, bool>::type operator!=(
            const CapArray &rhs) const {
        return !this->operator==(rhs);
    }

    void init() const {
        if (data_) return;
        d1 = fast_cap(C1);
        d2 = fast_cap(C2);
        data_ = (T *)VTSS_CALLOC_MODID(VTSS_MODULE_ID, d1 * d2, sizeof(T),
                                       __FILE__, __LINE__);
        if (!data_) return;
        for (size_t i = 0; i < d1 * d2; ++i) new (&(data_[i])) T();
    }

    ~CapArray() {
        if (!data_) return;
        for (size_t i = 0; i < d1 * d2; ++i) data_[i].~T();
        VTSS_FREE(data_);
    }

    A1<T> operator[](size_t idx) {
        init();
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return A1<T>(data_ + (idx * d2), d2);
    }

    const A1<const T> operator[](size_t idx) const {
        init();
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return A1<const T>(data_ + (idx * d2), d2);
    }

    size_t mem_size() const {
        init();
        return d1 * d2 * sizeof(T);
    }

    size_t size() const {
        init();
        return d1;
    }

    const T *data() const {
        init();
        return data_;
    }

    T *data() {
        init();
        return data_;
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, void>::type clear() {
        init();
        memset(data_, 0, mem_size());
    }

  private:
    mutable size_t d1, d2;
    mutable T *data_ = nullptr;
};

template <typename T, int C1, int C2, int C3>
class CapArray<T, C1, C2, C3> {
  public:
    CapArray() {}
    CapArray &operator=(const CapArray &rhs) {
        init();
        size_t n = d1 * d2 * d3;
        for (size_t i = 0; i < n; ++i) data_[i] = rhs.data()[i];
        return *this;
    }

    CapArray(const CapArray &rhs) {
        init();
        size_t n = d1 * d2 * d3;
        for (size_t i = 0; i < n; ++i) data_[i] = rhs.data()[i];
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, bool>::type operator==(
            const CapArray &rhs) const {
        init();
        return memcmp(data(), rhs.data(), mem_size()) == 0;
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, bool>::type operator!=(
            const CapArray &rhs) const {
        return !this->operator==(rhs);
    }

    void init() {
        if (data_) return;
        d1 = fast_cap(C1);
        d2 = fast_cap(C2);
        d3 = fast_cap(C3);
        size_t n = d1 * d2 * d3;

        data_ = (T *)VTSS_CALLOC_MODID(VTSS_MODULE_ID, n, sizeof(T), __FILE__,
                                       __LINE__);
        if (!data_) return;
        for (size_t i = 0; i < n; ++i) new (&(data_[i])) T();
    }

    ~CapArray() {
        if (!data_) return;
        size_t n = d1 * d2 * d3;
        for (size_t i = 0; i < n; ++i) data_[i].~T();
        VTSS_FREE(data_);
    }

    A2<T> operator[](size_t idx) {
        init();
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return A2<T>(data_ + (idx * d2 * d3), d2, d3);
    }

    const A2<T> operator[](size_t idx) const {
        init();
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return A2<const T>(data_ + (idx * d2 * d3), d2, d3);
    }

    size_t mem_size() const {
        init();
        return d1 * d2 * d3 * sizeof(T);
    }

    size_t size() const {
        init();
        return d1;
    }

    const T *data() const {
        init();
        return data_;
    }

    T *data() {
        init();
        return data_;
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, void>::type clear() {
        init();
        memset(data_, 0, mem_size());
    }


  private:
    mutable size_t d1, d2, d3;
    mutable T *data_ = nullptr;
};

template <typename T, int C1, int C2, int C3, int C4>
class CapArray<T, C1, C2, C3, C4> {
  public:
    CapArray() {}
    CapArray &operator=(const CapArray &rhs) {
        init();
        size_t n = d1 * d2 * d3 * d4;
        for (size_t i = 0; i < n; ++i) data_[i] = rhs.data()[i];
        return *this;
    }

    CapArray(const CapArray &rhs) {
        init();
        size_t n = d1 * d2 * d3 * d4;
        for (size_t i = 0; i < n; ++i) data_[i] = rhs.data()[i];
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, bool>::type operator==(
            const CapArray &rhs) const {
        init();
        return memcmp(data(), rhs.data(), mem_size()) == 0;
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, bool>::type operator!=(
            const CapArray &rhs) const {
        return !this->operator==(rhs);
    }

    void init() {
        if (data_) return;
        d1 = fast_cap(C1);
        d2 = fast_cap(C2);
        d3 = fast_cap(C3);
        d4 = fast_cap(C4);
        size_t n = d1 * d2 * d3 * d4;

        data_ = (T *)VTSS_CALLOC_MODID(VTSS_MODULE_ID, n, sizeof(T), __FILE__,
                                       __LINE__);
        if (!data_) return;
        for (size_t i = 0; i < n; ++i) new (&(data_[i])) T();
    }

    ~CapArray() {
        if (!data_) return;
        size_t n = d1 * d2 * d3 * d4;
        for (size_t i = 0; i < n; ++i) data_[i].~T();
        VTSS_FREE(data_);
    }

    A3<T> operator[](size_t idx) {
        init();
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return A3<T>(data_ + (idx * d2 * d3 * d4), d2, d3, d4);
    }

    const A3<const T> operator[](size_t idx) const {
        init();
        CAP_ARRAY_CHECK_DIM(idx, d1);
        return A3<const T>(data_ + (idx * d2 * d3 * d4), d2, d3, d4);
    }

    size_t mem_size() const {
        init();
        return d1 * d2 * d3 * d4 * sizeof(T);
    }

    size_t size() const {
        init();
        return d1;
    }

    const T *data() const {
        init();
        return data_;
    }

    T *data() {
        init();
        return data_;
    }

    template <class Q = T>
    typename std::enable_if<std::is_pod<Q>::value, void>::type clear() {
        init();
        memset(data_, 0, mem_size());
    }


  private:
    mutable size_t d1, d2, d3, d4;
    mutable T *data_ = nullptr;
};

#endif  //  _MSCC_ETHERNET_SWITCH_API_UTILS_
