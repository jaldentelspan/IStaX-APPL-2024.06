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

#ifndef __VTSS_BASICS_MAP_HXX__
#define __VTSS_BASICS_MAP_HXX__

#include <initializer_list>
#include <vtss/basics/new.hxx>
#include <vtss/basics/map-base.hxx>
#include <vtss/basics/algorithm.hxx>


namespace vtss {

/*

The purpose of Map is to provide a container similar to std::map. The most
important difference is that this class does not use exceptions, and errors are
propagated through other channels.

The biggest difference (between std::map and vtss::Map) is that the '[]'
operator is not implemented for vtss::Map, the functionality is instead provided
by the Map::get() and Map::set() methods. These methods can insert and/or get
elements in the map, and in opposite to std::map they use iterators instead of
references. Where std::map will throw an exception, vtss::Map will instead
return the Map::end() iterator.

NOTE: vtss::Map::get() will like std::map::operator[] create the elements if
they do not exist already. If this is not what you want, then use Map::find
(which works exactly like std::map).

Example of differences:

    vtss::Map<int, int> m2;            |    std::map<int, int> m1;
    if (!m2.set(1, 2)) {               |    try {
        // handle error                |        m1[1] = 2;
    }                                  |    } catch (...) {
                                       |         // handler error
                                       |    }
                                       |
                                       |
    auto i = m2.get(5);                |    try {
    if (i == m2.end()) {               |        VTSS_TRACE(INFO) << m1[5];
        // handle error                |    } catch (...) {
    } else {                           |        // handler error
        VTSS_TRACE(INFO) i->second;    |    }
    }                                  |


template <class KEY, class VAL>
struct Map {
    // Constructor, assignment and destructor //////////////////////////////////

    // Default construct an empty map.
    Map();

    // Construct new map using the elements from [i; e[.
    //
    // Error handling: If an allocation error occur, then the resulting map will
    // be empty.
    template <class I>
    Map(I i, I e);

    // Copy construct a new map.
    //
    // Error handling: If an allocation error occur, then the resulting map will
    // be empty.
    Map(const Map &rhs);

    // Move construct a map. The elements will be moved from rhs into this map.
    //
    // Error reporting: Can not fail as no allocation is needed.
    Map(Map &&rhs);

    // Construct new map using the elements from [il; el[.
    //
    // Error handling: If an allocation error occur, then the resulting map will
    // be empty.
    Map(std::initializer_list<typename BASE::value_type> il);

    // Assignment operator.
    //
    // Error handling: If an allocation error occur, then the resulting map will
    // be empty.
    Map &operator=(const Map &rhs);

    // Move assign operator.
    Map &operator=(Map &&rhs);

    // Swap the content of two maps
    void swap(Map &rhs);

    // Destructor.
    ~Map();

    // State and status ////////////////////////////////////////////////////////

    // Get the maximum amount of elements this map can hold.
    size_t max_size() const;

    // Set the maximum amount of elements this map can hold.
    void max_size(size_t m);

    // Check if the map is empty.
    bool empty() const;

    // Get the number of elements in the map.
    size_t size() const;

    // Data access /////////////////////////////////////////////////////////////

    // Look for a given key in the map - if not found then create it. If the
    // entry could not be found and the allocation failed - then return a 'end'
    // iterator, otherwise return an iterator to the entry.
    iterator get(const K &k);

    // Begin/end methods.
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

    reverse_iterator rbegin();
    reverse_iterator rend();
    const_reverse_iterator crbegin() const;
    const_reverse_iterator crend() const;
    const_reverse_iterator rbegin() const;
    const_reverse_iterator rend() const;


    // Search the map for a given key. Return an iterator to the element or
    // map::end() if it does not exists.
    iterator find(const KEY &k);
    iterator greater_than(const KEY &k);
    iterator lesser_than(const KEY &k);
    iterator greater_than_or_equal(const KEY &k);
    iterator lesser_than_or_equal(const KEY &k);
    const_iterator find(const KEY &k) const;
    const_iterator greater_than(const KEY &k) const;
    const_iterator lesser_than(const KEY &k) const;
    const_iterator greater_than_or_equal(const KEY &k) const;
    const_iterator lesser_than_or_equal(const KEY &k) const;

    // Data manipulator - insert new elements //////////////////////////////////

    // Insert a new (key, value) pair, if the key already exists then update it
    bool set(const K &k, const V &v);
    bool set(const K &k, V &&v);

    // Create a new element in the map. If the key already exists then update
    // it.
    template <class... Args>
    Pair<iterator, bool> emplace(Args &&... args);

    // Insert a new element in the map. If the key already exists then update
    // it.
    Pair<iterator, bool> insert(const value_type &v);
    Pair<iterator, bool> insert(value_type &&v);

    // Erase the element represented by 'i'.
    void erase(const_iterator i);

    // Erase the elements in the range [i; e[
    void erase(const_iterator i, const_iterator e);

    // Erase the element by key.
    size_t erase(const KEY &k);

    // Erase all elements in the map
    void clear();
};

*/

template <class KEY, class VAL>
struct Map : public MapBase<KEY, VAL, Map<KEY, VAL>> {
    typedef MapBase<KEY, VAL, Map<KEY, VAL>> BASE;

    template <class A, class B>
    friend struct MapSetCommon;

    Map() : BASE() {}

    template <class I>
    Map(I i, I e) {
        Map _m;

        while (i != e) {
            if (!_m.insert(*i++).first.n) {
                return;
            }
        }

        this->operator=(vtss::move(_m));
    }

    Map(const Map &m) {
        Map _m;
        for (const auto &i : m)
            _m.insert(i);
        this->operator=(vtss::move(_m));
    }

    Map(Map &&m) : BASE(vtss::move(m)) {
        max_size_ = m.max_size_;
        m.max_size_ = (size_t)-1;
    }

    Map(std::initializer_list<typename BASE::value_type> il)
        : BASE(il.begin(), il.end()) {}

    Map &operator=(const Map &m) {
        if (m.size() > max_size_) return *this;

        BASE::operator=(m);
        return *this;
    }

    Map &operator=(Map &&m) {
        BASE::operator=(vtss::move(m));

        max_size_ = m.max_size_;
        m.max_size_ = (size_t)-1;

        return *this;
    }

    ~Map() { BASE::clear(); }

    size_t max_size() const { return max_size_; };
    void max_size(size_t m) { max_size_ = m; };

  private:
    void *allocate_() {
        return VTSS_BASICS_MALLOC(sizeof(typename BASE::Node));
    }
    void deallocate_(void *p) { VTSS_BASICS_FREE(p); }
    size_t max_size_ = (size_t)-1;
};

template <class K, class V>
inline bool operator==(const Map<K, V> &a, const Map<K, V> &b) {
    return a.size() == b.size() && equal(a.begin(), a.end(), b.begin());
}

template <class K, class V>
inline bool operator!=(const Map<K, V> &a, const Map<K, V> &b) {
    return !(a == b);
}

}  // namespace vtss

#endif  // __VTSS_BASICS_MAP_HXX__
