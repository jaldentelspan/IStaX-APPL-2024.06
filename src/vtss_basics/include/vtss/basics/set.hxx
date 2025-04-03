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

#ifndef __VTSS_BASICS_SET_HXX__
#define __VTSS_BASICS_SET_HXX__

#include <initializer_list>
#include <vtss/basics/new.hxx>
#include <vtss/basics/set-base.hxx>
#include <vtss/basics/algorithm.hxx>

namespace vtss {

template<class TYPE>
struct Set : public SetBase<TYPE, Set<TYPE>> {
    typedef SetBase<TYPE, Set<TYPE>> BASE;

    template <class A, class B>
    friend struct MapSetCommon;

    Set() : BASE() {}

    template <class I>
    Set(I i, I e) {
        Set _s;

        while (i != e) {
            if (!_s.insert(*i++).first.n) {
                return;
            }
        }

        this->operator=(vtss::move(_s));
    }

    Set(const Set &s) {
        Set _s;
        for (const auto &i : s)
            _s.insert(i);
        this->operator=(vtss::move(_s));
    }

    Set(Set &&m) : BASE(vtss::move(m)) {
        max_size_ = m.max_size_;
        m.max_size_ = (size_t)-1;
    }

    Set(std::initializer_list<typename BASE::value_type> il)
        : BASE(il.begin(), il.end()) {}

    Set &operator=(const Set &m) {
        if (m.size() > max_size_) return *this;

        BASE::operator=(m);
        return *this;
    }

    Set &operator=(Set &&m) {
        BASE::operator=(vtss::move(m));

        max_size_ = m.max_size_;
        m.max_size_ = (size_t)-1;

        return *this;
    }

    ~Set() { BASE::clear(); }

    size_t max_size() const { return max_size_; };
    void max_size(size_t m) { max_size_ = m; };

  private:
    void *allocate_() { return VTSS_BASICS_MALLOC(sizeof(typename BASE::Node)); }
    void deallocate_(void *p) { VTSS_BASICS_FREE(p); }
    size_t max_size_ = (size_t)-1;
};

template <class K>
inline bool operator==(const Set<K> &a, const Set<K> &b) {
    return a.size() == b.size() && equal(a.begin(), a.end(), b.begin());
}

template <class K>
inline bool operator!=(const Set<K> &a, const Set<K> &b) {
    return !(a == b);
}

}  // namespace vtss

#endif  // __VTSS_BASICS_SET_HXX__
