/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_VECTOR_HXX__
#define __VTSS_BASICS_VECTOR_HXX__

#include <vtss/basics/utility.hxx>
#include <vtss/basics/algorithm.hxx>
#include <vtss/basics/vector-base.hxx>
#include <vtss/basics/vector-memory.hxx>

namespace vtss {

/*

The purpose of Vector is to provide a sized container similar to std::vector.
The most important difference is that this class does not use exceptions, so
errors are propagated through other means. Besides this, a more fine-grained
allocation control is also offered through the settings: 'grow_size' and
'max_size'.

vtss::Vector will - like std::vector - grow in size when needed. This growing
is done by allocating new memory area and then copy/move the existing objects
to the new memory area. It is therefore important never to access the elements
in a vector by using pointers after a re-allocation! If you need to refer to
an element in a vector, always do it by using its index.

See also: VectorFixed (vector-fixed.hxx) and VectorStatic (vector-static.hxx).

Like std::vector, this class may be used to hold both plain-old-data (POD) and
complex objects.

Instead of using exceptions this class will use return values or assertions to
signal errors. All methods using return values to signal error guarantee that
the content of the current vector is unchanged in case of error.

The class will only assert when detecting improper use of it, typically when
accessing non-existing elements by reference.

Synopsis:

template <
    typename TYPE // Type of objects that this container holds
> class Vector {
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

    // Default constructor. Construct a new vector with the given capacity.
    //
    // Error reporting: If the allocation fails, the vector will be constructed
    // with a capacity of zero. Check the capacity after constructing to verify
    // that no error has occurred.
    Vector(size_t capacity = 0) {}

    // Move constructor. Construct a new vector by moving the allocated memory
    // from an already constructed vector.
    //
    // Error reporting: Can not fail as no allocation is needed.
    Vector(Vector &&rhs);

    // Copy constructor.
    // May fail, check size after calling
    Vector(const Vector &rhs);

    // Initialize the vector with a list of elements - not necessarily filling
    // it up.
    // Example: StaticVector<int, 5> v({1, 2, 3});
    //
    // Error reporting: If the allocation fails or if the size of the
    // initializer list exceeds the 'max_size()' then the vector will be
    // constructed empty. Check the size of "this" after using this method.
    Vector(std::initializer_list<TYPE> il);

    // Assignment operator.
    Vector &operator=(const Vector &rhs);

    // Move assignment operator. Move the allocated storage and other member
    // variables from 'rhs' to 'this' and leave 'rhs' empty.
    //
    // Error reporting: Can not fail as no allocation is needed.
    Vector &operator=(Vector &&rhs);

    // Destructor - will destruct all elements contained, and free the allocated
    // memory.
    ~Vector();

    // State and status ////////////////////////////////////////////////////////

    // Max amount of objects this container can hold. The allocation routines
    // will not try to allocate room for more objects than 'max_size'.
    size_t max_size() const;
    void max_size(size_t s);

    // Specify how much to grow the vector capacity when inserting new elements.
    // Notice, grow_size is only used when inserting elements - if assigning
    // (ranges or vectors) then the size of the range is used.
    //
    // The constructor will initialize the grow_size to:
    // sizeof(TYPE) > 4096 ? 1 : 4096 / sizeof(TYPE)
    //
    // If grow_size() is zero, then it will double the current capacity
    // (if the current capacity is zero then grow_size() is set to one).
    //
    // If grow_size() is not zero, then the capacity is incremented with
    // 'grow_size()' elements if needed.
    size_t grow_size() const;
    void grow_size(size_t g);

    // The number of objects that this container can hold with the current
    // allocation.
    size_t capacity() const;

    // Number of elements currently stored in 'this'.
    size_t size() const;

    // returns true if the vector is empty, false otherwise.
    bool empty() const;

    // Data access /////////////////////////////////////////////////////////////
    // Data accessors will fail if trying to access a non-existing element.
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

    // Access the element at index 'n' by reference. Asserts on error.
    reference operator[](size_t n);

    // Access the element at index 'n' by const-reference. Asserts on error.
    const_reference operator[](size_t n) const;

    // Access the element at index 'n' by iterator.
    iterator at(size_t n);

    // Access the element at index 'n' by const-iterator.
    const_iterator at(size_t n) const;

    // Access the first element in the vector by reference. Asserts on error.
    reference front();

    // Access the first element in the vector by const reference. Asserts on
    // error.
    const_reference front() const;

    // Access the last element in the vector by reference. Asserts on error.
    reference back();

    // Access the last element in the vector by const reference. Asserts on error.
    const_reference back() const;

    // Returns a pointer to the data this class operates on.
    value_type *data();

    // Returns a const-pointer to the data this class operates on.
    const value_type *data() const;

    // Data manipulator - insert new elements //////////////////////////////////
    // All push_back, emplace_back, insert, emplace, and assign overloads will
    // fail if the current vector does not have the capacity to hold the
    // requested new elements and new allocation fails.  The new allocation may
    // fail if it reaches the 'max_size' or if malloc fails.
    //
    // In case of error, these methods will return false, and the vector will
    // remain untouched.

    // Append the value 'x' to the end of 'this'.
    bool push_back(const value_type &x);

    // Append the value 'x' to the end of 'this' by moving 'x'.
    bool push_back(value_type &&x);

    // Construct a new instance of TYPE at the end of this vector, using the
    // argument 'args...'.
    template <class... Args>
    bool emplace_back(Args &&... args);

    // Insert value 'x' at position 'p'.
    bool insert(const_iterator p, const value_type &x);

    // Insert value 'x' at position 'p' by moving.
    bool insert(const_iterator p, value_type &&x);

    // Insert 'n' copies of value 'x' at position 'p'.
    bool insert(const_iterator p, size_t n, const value_type &x);

    // Construct a new instance of 'TYPE' at position 'p' using the argument
    // 'args...'.
    template <class... Args>
    bool emplace(const_iterator p, Args &&... args);

    // Insert the objects in the range [b; e[ at position 'p'.
    template <class InputIterator>
    bool insert(const_iterator p, InputIterator b, InputIterator e);

    // Insert the objects provided in the initializer list at position 'p'.
    bool insert(const_iterator p, std::initializer_list<TYPE> il);

    // Assign a range of elements to this. The current contents will be replaced
    // by a copy of the provided range.
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
template <typename TYPE>
struct Vector : public VectorBase<TYPE, Vector<TYPE>, true> {
    template <typename A, typename B, bool C>
    friend struct VectorBase;

    typedef VectorBase<TYPE, Vector<TYPE>, true> BASE;
    typedef TYPE value_type;

    size_t max_size() const { return max_size_; }
    size_t grow_size() const { return grow_size_; }
    size_t capacity() const { return data_.capacity(); }

    void grow_size(size_t g) { grow_size_ = g; }
    void max_size(size_t s) { max_size_ = s; }

    value_type *data() { return (value_type *)data_.data(); };
    const value_type *data() const { return (const value_type *)data_.data(); };

    constexpr Vector() {}
    explicit constexpr Vector(size_t size) : data_(size, sizeof(TYPE) * size) {}

    Vector(size_t n, const TYPE &t) : data_(n, sizeof(TYPE) * n) {
        BASE::insert(BASE::begin(), n, t);
    }

    // can not fail as no allocation is required
    Vector(Vector &&rhs)
        : data_(vtss::move(rhs.data_)),
          grow_size_(rhs.grow_size_),
          max_size_(rhs.max_size_) {
        BASE::size_ = rhs.size_;
        rhs.size_ = 0;
    }

    // May fail... Check: assert(size() == rhs.size())
    Vector(const Vector &rhs)
        : data_(rhs.capacity(), sizeof(TYPE) * rhs.capacity()),
          grow_size_(rhs.grow_size_),
          max_size_(rhs.max_size_) {
        BASE::assign(rhs.begin(), rhs.end());
    }

    // May fail... Check that size() != 0 (unless 'il' is empty)
    Vector(std::initializer_list<TYPE> il) {
        BASE::assign(il.begin(), il.end());
    }

    // May fail, check size after wards
    Vector &operator=(const Vector &rhs) {
        BASE::assign(rhs.begin(), rhs.end());
        return *this;
    }

    Vector &operator=(Vector &&rhs) {
        // empty the existing content
        BASE::clear();

        // Move the memory area
        data_ = vtss::move(rhs.data_);

        // Copy 'size' from rhs
        BASE::size_ = rhs.size_;
        grow_size_ = rhs.grow_size_;
        max_size_ = rhs.max_size_;

        // inform rhs that we have stolen it's inner parts
        rhs.size_ = 0;

        return *this;
    }

    bool reserve(size_t s) {
        if (s < capacity()) return true;

        Vector v(s);
        if (v.capacity() != s) return false;
        if (!v.assign(BASE::begin(), BASE::end())) return false;
        *this = vtss::move(v);

        return true;
    }

    void swap(Vector &rhs) {
        using vtss::swap;
        data_.swap(rhs.data_);
        swap(grow_size_, rhs.grow_size_);
        swap(max_size_, rhs.max_size_);
    }

    ~Vector() { BASE::clear(); }

  private:
    VectorMemory memory_allocate(size_t cap) {
        if (cap > max_size_)
            return VectorMemory();
        else
            return VectorMemory(cap, sizeof(TYPE) * cap);
    }

    VectorMemory memory_grows(size_t min) {
        if (min > max_size_) return VectorMemory();

        size_t s = data_.capacity();

        // Not robust to integer wrap-arounds...
        if (grow_size_) {
            while (min > s) s += grow_size_;
        } else {
            s = s ? s : 1;
            while (min > s) s *= 2;
        }

        if (s > max_size_) s = max_size_;

        return VectorMemory(s, sizeof(TYPE) * s);
    }

    void memory_replace(VectorMemory &&m) { data_ = vtss::move(m); }

    VectorMemory data_;
    size_t grow_size_ = sizeof(TYPE) > 4096 ? 1 : 4096 / sizeof(TYPE);
    size_t max_size_ = (size_t)-1;
};

template <typename T>
void swap(Vector<T> &a, Vector<T> &b) {
    return a.swap(b);
}

template <class TYPE>
inline bool operator==(const Vector<TYPE> &a, const Vector<TYPE> &b) {
    return a.size() == b.size() && equal(a.begin(), a.end(), b.begin());
}

template <class TYPE>
inline bool operator<(const Vector<TYPE> &a, const Vector<TYPE> &b) {
    return lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

template <class TYPE>
inline bool operator!=(const Vector<TYPE> &a, const Vector<TYPE> &b) {
    return !(a == b);
}

template <class TYPE>
inline bool operator>(const Vector<TYPE> &a, const Vector<TYPE> &b) {
    return b < a;
}

template <class TYPE>
inline bool operator>=(const Vector<TYPE> &a, const Vector<TYPE> &b) {
    return !(a < b);
}

template <class TYPE>
inline bool operator<=(const Vector<TYPE> &a, const Vector<TYPE> &b) {
    return !(b < a);
}

}  // namespace vtss

#endif  // __VTSS_BASICS_VECTOR_HXX__
