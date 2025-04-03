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

#ifndef __VTSS_BASICS_SET_FIXED_HXX__
#define __VTSS_BASICS_SET_FIXED_HXX__

#include <initializer_list>
#include <vtss/basics/new.hxx>
#include <vtss/basics/set-base.hxx>
#include <vtss/basics/map-memory-fixed.hxx>

namespace vtss {

template <class KEY>
struct SetFixed : public SetBase<KEY, SetFixed<KEY>> {
    typedef SetBase<KEY, SetFixed<KEY>> BASE;

    template <class A, class B>
    friend struct MapSetCommon;

    SetFixed(size_t s) : max_size_(s), mem(s, sizeof(typename BASE::Node)) {}

    SetFixed(const SetFixed &m)
        : max_size_(0), mem(0, sizeof(typename BASE::Node)) {
        SetFixed _m(m.max_size());
        for (const auto &i : m)
            _m.insert(i);
        this->operator=(vtss::move(_m));
    }

    SetFixed(SetFixed &&m) : BASE(vtss::move(m)), mem(vtss::move(m.mem)) {
        max_size_ = m.max_size_;
        m.max_size_ = (size_t)-1;
    }

    SetFixed &operator=(const SetFixed &m) {
        if (m.size() > max_size_) return *this;

        BASE::operator=(m);
        return *this;
    }

    SetFixed &operator=(SetFixed &&m) {
        BASE::operator=(vtss::move(m));
        mem.operator=(vtss::move(m.mem));
        max_size_ = m.max_size_;

        return *this;
    }

    ~SetFixed() { BASE::clear(); }

    size_t max_size() const { return max_size_; };

  private:
    void *allocate_() { return mem.allocate(); }
    void deallocate_(void *p) { mem.deallocate(p); }
    size_t max_size_;
    MapMemoryFixed mem;
};

template <class K>
inline bool operator==(const SetFixed<K> &a, const SetFixed<K> &b) {
    return a.size() == b.size() && equal(a.begin(), a.end(), b.begin());
}

template <class K>
inline bool operator!=(const SetFixed<K> &a, const SetFixed<K> &b) {
    return !(a == b);
}

}  // namespace vtss

#endif  // __VTSS_BASICS_SET_FIXED_HXX__
