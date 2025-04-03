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

#ifndef __VTSS_BASICS_VECTOR_STATIC_HXX__
#define __VTSS_BASICS_VECTOR_STATIC_HXX__

#include <vtss/basics/vector-base.hxx>
#include <vtss/basics/vector-memory.hxx>

namespace vtss {

/*

The purpose of VectorStatic is to provide a static allocated container which has
an interface very similar to std::vector. The two most important differences is
this class does not use exceptions, and it will never call any memory
allocation/deallocation methods.

See also: Vector (vector.hxx) and VectorFixed (vector-fixed.hxx).

Like the std::vector variant, this class may be used to hold both plain-old-data
(POD) and complex objects.

Instead of using exceptions this class will use return values or assertions to
signal errors. All methods which a using return values to signal error will
guaranteed that the content of the current vector is unchanged in case of error.

The class will only assert when detecting improper usage of this class
(typically when accessing non-existing elements by reference).

Synopsis:

template <
    typename TYPE,   // Type of object's that this container can hold
    size_t SIZE      // Size (number of instances - not byte)
> class VectorStatic {
  public:
    // Typedefs ////////////////////////////////////////////////////////////////
    typedef TYPE                       value_type;
    typedef TYPE*                      pointer;
    typedef const TYPE*                const_pointer;
    typedef TYPE& reference;
    typedef const TYPE&                const_reference;
    typedef depends-on-implementation  iterator;
    typedef depends-on-implementation  const_iterator;

    // Constructor, assignment and destructor //////////////////////////////////

    // Default constructor.
    VectorStatic() {}

    // Move constructor.
    //
    // Error reporting: Can not fail as 'rhs' is guaranteed to be of same size
    // as this. Each element in 'rhs' is moved into this, one at a time, and
    // 'rhs' is cleared after wards.
    VectorStatic(VectorStatic &&rhs);

    // Copy constructor.
    //
    // Error reporting: can not fail as 'rhs' is guaranteed to be of same size
    // as this. Each element in 'rhs' is copied into this.
    VectorStatic(const VectorStatic &rhs);

    // Initialize the vector with a list of elements.
    // Example: StaticVector<int, 5> v({1, 2, 3});
    //
    // Error reporting: If the size of the initializer list increase the
    // capacity then the vector will be constructed empty. Check the size of
    // "this" after using this method.
    VectorStatic(std::initializer_list<TYPE> il);

    // Assignment operator.
    //
    // Error reporting: Can not fail as 'rhs' is guaranteed to be of same size
    // as this. The current vector will be emptied and all elements from 'rhs'
    // is copied into this.
    VectorStatic &operator=(const VectorStatic &rhs);

    // Destructor - will destruct all elements contained.
    ~VectorStatic();


    // State and status ////////////////////////////////////////////////////////

    // The highest amount of objects this container can hold. Will always
    // return SIZE as this class does not support dynamic allocation;
    size_t max_size() const;

    // The number of objects that this container can hold with the current
    // allocation. Will always return SIZE as this class does not support
    // dynamic allocation.
    size_t capacity() const;

    // Number of elements currently stored in 'this'.
    size_t size() const;

    // returns true if the vector is empty, otherwise false.
    bool empty() const;

    // Data access /////////////////////////////////////////////////////////////
    // Data accessors will fail if trying to access a non existing element.
    //
    // When accessing by-reference the error reporting strategy is to assert.
    // When accessing by-iterator the error reporting strategy is to return the
    // 'end()' iterator.

    // Returns an iterator to the first element in 'this'. If the vector is
    // empty then 'begin() == end()'
    iterator begin();

    // Returns a const-iterator to the first element in 'this'. If the vector is
    // empty then 'begin() == end()'
    const_iterator begin() const;

    // Returns an iterator to the last + 1 element in 'this' (this iterator may
    // never be dereferenced as this will result in an assert).
    iterator end();

    // Returns a const-iterator to the last + 1 element in 'this' (this iterator
    // may never be dereferenced as this will result in an assert).
    const_iterator end() const;

    // Returns a const-iterator to the first element in 'this'. If the vector is
    // empty then 'begin() == end()'
    const_iterator cbegin() const;

    // Returns a const-iterator to the last + 1 element in 'this' (this iterator
    // may never be dereferenced as this will result in an assert).
    const_iterator cend() const;

    // Access the element at index 'n' (by reference - will assert on error).
    reference operator[](size_t n);

    // Access the element at index 'n' (by const-reference - will assert on
    // error).
    const_reference operator[](size_t n) const;

    // Access the element at index 'n' (by iterator).
    iterator at(size_t n);

    // Access the element at index 'n' (by const-iterator).
    const_iterator at(size_t n) const;

    // Access the first element in the vector (by reference - will assert on
    // error).
    reference front();

    // Access the first element in the vector (by const reference - will assert
    // on error).
    const_reference front() const;

    // Access the last element in the vector (by reference - will assert on
    // error).
    reference back();

    // Access the last element in the vector (by const reference - will assert
    // on error).
    const_reference back() const;

    // Returns a pointer to the data this class operates on.
    value_type *data();

    // Returns a const-pointer to the data this class operates on.
    const value_type *data() const;

    // Data manipulator - insert new elements //////////////////////////////////
    // All push_back, emplace_back, insert, emplace, and assign overloads will
    // fail if the current vector does not have the capacity to hold the
    // requested new elements.  In case of error, then these methods will return
    // false, and the vector will remain untouched.

    // Append the value 'x' to the end of 'this'.
    bool push_back(const value_type &x);

    // Append the value 'x' to the end of 'this' - by moving 'x'.
    bool push_back(value_type &&x);

    // Construct a new instance of TYPE at the end of this vector, using the
    // argument args....
    template <class... Args>
    bool emplace_back(Args &&... args);

    // Insert value 'x' at position 'p'
    bool insert(const_iterator p, const value_type &x);

    // Insert value 'x' at position 'p' - by moving
    bool insert(const_iterator p, value_type &&x);

    // Insert 'n' copies of value 'x' at position 'p'
    bool insert(const_iterator p, size_t n, const value_type &x);

    // Construct a new instance of 'TYPE' at position 'p' using the argument
    // 'args...'
    template <class... Args>
    bool emplace(const_iterator p, Args &&... args);

    // Insert the objects in the range [b, e[ at position 'p'
    template <class InputIterator>
    bool insert(const_iterator p, InputIterator b, InputIterator e);

    // Insert the objects provided in the initializer list at position 'p'
    bool insert(const_iterator p, std::initializer_list<TYPE> il);

    // Assign a range of elements to this. The current content will be replaced
    // with a copy of the provided range.
    template <class InputIterator>
    bool assign(InputIterator range_i, InputIterator range_end);

    // Replace the current content with 'n' copies of X.
    bool assign(size_t n, const value_type &x);

    // Replace the current content with the content of the initializer list
    bool assign(std::initializer_list<TYPE> il);

    // Data manipulator - delete elements //////////////////////////////////////

    // Delete the last element in the vector. Will return false if the vector is
    // already empty.
    bool pop_back();

    // Delete all elements in the list.
    void clear();

    // Delete the element at position 'p'.
    void erase(const_iterator p);

    // Delete the elements in the range [range_begin, range_end[
    void erase(const_iterator range_begin, const_iterator range_end);
};

*/

template <typename TYPE, size_t SIZE>
struct VectorStatic : public VectorBase<TYPE, VectorStatic<TYPE, SIZE>> {
    template <typename A, typename B, bool C>
    friend struct VectorBase;

    typedef VectorBase<TYPE, VectorStatic<TYPE, SIZE>> BASE;
    typedef TYPE value_type;

    size_t max_size() const { return SIZE; }
    size_t capacity() const { return SIZE; }

    value_type *data() { return (value_type *)data_; };
    const value_type *data() const { return (const value_type *)data_; };

    VectorStatic() {}

    // can not fail as rhs is of same size as this
    VectorStatic(VectorStatic &&rhs) {
        size_t s = rhs.size();
        TYPE *dst = data();
        TYPE *src = rhs.data();
        TYPE *src_e = rhs.data() + s;
        while (src != src_e) ::new ((void *)dst++) TYPE(vtss::move(*src++));
        BASE::size_ = s;
        rhs.clear();
    }

    // can not fail as rhs is of same size as this
    VectorStatic(const VectorStatic &rhs) {
        BASE::assign(rhs.begin(), rhs.end());
    }

    // May fail...
    VectorStatic(std::initializer_list<TYPE> il) {
        BASE::assign(il.begin(), il.end());
    }

    // can not fail as rhs is of same size as this
    VectorStatic &operator=(const VectorStatic &rhs) {
        BASE::assign(rhs.begin(), rhs.end());
        return *this;
    }

    ~VectorStatic() { BASE::clear(); }

  private:
    VectorMemory memory_allocate(size_t cap) { return VectorMemory(); }
    VectorMemory memory_grows(size_t min) { return VectorMemory(); }
    void memory_replace(VectorMemory &&m) {}

    typename std::aligned_storage<sizeof(TYPE), alignof(TYPE)>::type
            data_[SIZE];
};

template <typename TYPE, size_t SIZE>
inline void swap(VectorStatic<TYPE, SIZE> &a, VectorStatic<TYPE, SIZE> &b) {
    auto ae = a.end();
    auto ai = a.begin();
    auto bi = b.begin();

    for (; ai != ae; ai++, bi++) swap(*ai, *bi);
}

template <typename TYPE, size_t SIZE>
inline bool operator==(const VectorStatic<TYPE, SIZE> &a,
                       const VectorStatic<TYPE, SIZE> &b) {
    return equal(a.begin(), a.end(), b.begin());
}

template <typename TYPE, size_t SIZE>
inline bool operator<(const VectorStatic<TYPE, SIZE> &a,
                      const VectorStatic<TYPE, SIZE> &b) {
    return lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

template <typename TYPE, size_t SIZE>
inline bool operator!=(const VectorStatic<TYPE, SIZE> &a,
                       const VectorStatic<TYPE, SIZE> &b) {
    return !(a == b);
}

template <typename TYPE, size_t SIZE>
inline bool operator>(const VectorStatic<TYPE, SIZE> &a,
                      const VectorStatic<TYPE, SIZE> &b) {
    return b < a;
}

template <typename TYPE, size_t SIZE>
inline bool operator>=(const VectorStatic<TYPE, SIZE> &a,
                       const VectorStatic<TYPE, SIZE> &b) {
    return !(a < b);
}

template <typename TYPE, size_t SIZE>
inline bool operator<=(const VectorStatic<TYPE, SIZE> &a,
                       const VectorStatic<TYPE, SIZE> &b) {
    return !(b < a);
}

}  // namespace vtss

#endif  // __VTSS_BASICS_VECTOR_STATIC_HXX__
