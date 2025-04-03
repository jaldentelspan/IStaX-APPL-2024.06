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

#ifndef __VTSS_BASICS_MAP_STATIC_HXX__
#define __VTSS_BASICS_MAP_STATIC_HXX__

#include <initializer_list>
#include <vtss/basics/new.hxx>
#include <vtss/basics/map-base.hxx>
#include <vtss/basics/map-memory-static.hxx>

namespace vtss {

template <class KEY, class VAL, size_t SIZE>
struct MapStatic : public MapBase<KEY, VAL, MapStatic<KEY, VAL, SIZE>> {
    typedef MapBase<KEY, VAL, MapStatic<KEY, VAL, SIZE>> BASE;

    template <class A, class B>
    friend struct MapSetCommon;

    MapStatic() : mem(data_, SIZE, sizeof(typename BASE::Node)) {}

    // Do not implement copy-constructor as it will end up calling virtual
    // methods in the constructors.
    MapStatic(const MapStatic &m) = delete;

    // Moving make no sense as data is allocated on the stack and life-time are
    // bound to the scope!
    MapStatic(MapStatic &&m) = delete;

    MapStatic &operator=(const MapStatic &m) {
        if (m.size() > SIZE) return *this;

        BASE::operator=(m);
        return *this;
    }

    MapStatic &operator=(MapStatic &&m) = delete;

    ~MapStatic() { BASE::clear(); }

    size_t max_size() const { return SIZE; };

  private:
    void *allocate_() { return mem.allocate(); }
    void deallocate_(void *p) { mem.deallocate(p); }

    typename std::aligned_storage<sizeof(typename BASE::Node),
                                  alignof(typename BASE::Node)>::type
            data_[SIZE];

    MapMemoryStatic mem;
};

template <class K, class V, size_t S>
inline bool operator==(const MapStatic<K, V, S> &a,
                       const MapStatic<K, V, S> &b) {
    return a.size() == b.size() && equal(a.begin(), a.end(), b.begin());
}

template <class K, class V, size_t S>
inline bool operator!=(const MapStatic<K, V, S> &a,
                       const MapStatic<K, V, S> &b) {
    return !(a == b);
}

}  // namespace vtss

#endif  // __VTSS_BASICS_MAP_STATIC_HXX__
