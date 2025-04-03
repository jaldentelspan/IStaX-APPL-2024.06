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

#ifndef __VTSS_BASICS_ITERATOR_HXX__
#define __VTSS_BASICS_ITERATOR_HXX__

#include <vtss/basics/common.hxx>
#include <type_traits>


namespace vtss {

// Declare the various iterator tags that are defined in the standard
struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

// Declare trait class such that a pointer can be used directly as an iterator
template <class ITR>
struct iterator_traits {
    typedef typename ITR::value_type value_type;
    typedef typename ITR::pointer pointer;
    typedef typename ITR::reference reference;
    typedef typename ITR::iterator_category iterator_category;
};

// traits for pointers
template <class PTR>
struct iterator_traits<PTR*> {
    typedef typename std::remove_const<PTR>::type value_type;
    typedef PTR* pointer;
    typedef PTR& reference;
    typedef random_access_iterator_tag iterator_category;
};

// Helper function to detect figure out if a given iterator (A) is convertible
// to a given iterator category
template <class A, class B>
struct __itr_conv_to
        : public std::integral_constant<
                  bool, std::is_convertible<
                                typename iterator_traits<A>::iterator_category,
                                B>::value> {};

template <class ITR>
struct is_input_iterator : public __itr_conv_to<ITR, input_iterator_tag> {};

template <class ITR>
struct is_forward_iterator : public __itr_conv_to<ITR, forward_iterator_tag> {};

template <class ITR>
struct is_bidirectional_iterator
        : public __itr_conv_to<ITR, bidirectional_iterator_tag> {};

template <class ITR>
struct is_random_access_iterator
        : public __itr_conv_to<ITR, random_access_iterator_tag> {};

// All iterator should be derived from this class
template <class _Category, class VALUE, class _Pointer = VALUE*,
          class _Reference = VALUE&>
struct iterator {
    typedef VALUE value_type;
    typedef _Pointer pointer;
    typedef _Reference reference;
    typedef _Category iterator_category;
};

// Advance /////////////////////////////////////////////////////////////////////
template <class ITR>
inline void __advance(ITR& i, ptrdiff_t n, input_iterator_tag) {
    for (; n > 0; --n) ++i;
}

template <class ITR>
inline void __advance(ITR& i, ptrdiff_t n, bidirectional_iterator_tag) {
    if (n >= 0)
        for (; n > 0; --n) ++i;
    else
        for (; n < 0; ++n) --i;
}

template <class _RandIter>
inline void __advance(_RandIter& i, ptrdiff_t n, random_access_iterator_tag) {
    i += n;
}

template <class ITR>
inline void advance(ITR& i, ptrdiff_t n) {
    __advance(i, n, typename iterator_traits<ITR>::iterator_category());
}

// Distance ////////////////////////////////////////////////////////////////////
template <class ITR>
inline ptrdiff_t __distance(ITR b, ITR e, input_iterator_tag) {
    ptrdiff_t r(0);
    for (; b != e; ++b) ++r;
    return r;
}

template <class ITR>
inline ptrdiff_t __distance(ITR b, ITR e, random_access_iterator_tag) {
    return e - b;
}

template <class ITR>
inline ptrdiff_t distance(ITR b, ITR e) {
    return __distance(b, e, typename iterator_traits<ITR>::iterator_category());
}

// Next ////////////////////////////////////////////////////////////////////////
template <class ITR>
inline ITR next(ITR x, ptrdiff_t n = 1,
                typename std::enable_if<is_forward_iterator<ITR>::value>::type *
                = 0) {
    advance(x, n);
    return x;
}

// Prev ////////////////////////////////////////////////////////////////////////
template <class ITR>
inline ITR prev(
        ITR x, ptrdiff_t n = 1,
        typename std::enable_if<is_bidirectional_iterator<ITR>::value>::type *
        = 0) {
    advance(x, -n);
    return x;
}


// Reverse iterator ////////////////////////////////////////////////////////////
template <class ITR>
class reverse_iterator
        : public iterator<typename iterator_traits<ITR>::iterator_category,
                          typename iterator_traits<ITR>::value_type,
                          typename iterator_traits<ITR>::pointer,
                          typename iterator_traits<ITR>::reference> {
  public:
    typedef ITR iterator_type;
    typedef typename iterator_traits<ITR>::reference reference;
    typedef typename iterator_traits<ITR>::pointer pointer;

    reverse_iterator() : current() {}

    explicit reverse_iterator(ITR x) : current(x), tmp(x) {}

    template <class _Up>
    reverse_iterator(const reverse_iterator<_Up>& rhs)
        : tmp(rhs.base()), current(rhs.base()) {}

    ITR base() const { return current; }
    reference operator*() const {
        tmp = current;
        return *--tmp;
    }

    pointer operator->() const { return &(operator*()); }
    reverse_iterator& operator++() {
        --current;
        return *this;
    }

    reverse_iterator operator++(int) {
        reverse_iterator x(*this);
        --current;
        return x;
    }

    reverse_iterator& operator--() {
        ++current;
        return *this;
    }

    reverse_iterator operator--(int) {
        reverse_iterator x(*this);
        ++current;
        return x;
    }

    reverse_iterator operator+(ptrdiff_t n) const {
        return reverse_iterator(current - n);
    }

    reverse_iterator& operator+=(ptrdiff_t n) {
        current -= n;
        return *this;
    }

    reverse_iterator operator-(ptrdiff_t n) const {
        return reverse_iterator(current + n);
    }

    reverse_iterator& operator-=(ptrdiff_t n) {
        current += n;
        return *this;
    }

    reference operator[](ptrdiff_t n) const { return current[-n - 1]; }

  protected:
    ITR current;

  private:
    mutable ITR tmp;
};

template <class ITR1, class ITR2>
inline bool operator==(const reverse_iterator<ITR1>& a,
                       const reverse_iterator<ITR2>& b) {
    return a.base() == b.base();
}

template <class ITR1, class ITR2>
inline bool operator<(const reverse_iterator<ITR1>& a,
                      const reverse_iterator<ITR2>& b) {
    return a.base() > b.base();
}

template <class ITR1, class ITR2>
inline bool operator!=(const reverse_iterator<ITR1>& a,
                       const reverse_iterator<ITR2>& b) {
    return a.base() != b.base();
}

template <class ITR1, class ITR2>
inline bool operator>(const reverse_iterator<ITR1>& a,
                      const reverse_iterator<ITR2>& b) {
    return a.base() < b.base();
}

template <class ITR1, class ITR2>
inline bool operator>=(const reverse_iterator<ITR1>& a,
                       const reverse_iterator<ITR2>& b) {
    return a.base() <= b.base();
}

template <class ITR1, class ITR2>
inline bool operator<=(const reverse_iterator<ITR1>& a,
                       const reverse_iterator<ITR2>& b) {
    return a.base() >= b.base();
}

template <class ITR1, class ITR2>
inline ptrdiff_t operator-(const reverse_iterator<ITR1>& a,
                           const reverse_iterator<ITR2>& b) {
    return b.base() - a.base();
}

template <class ITR>
inline reverse_iterator<ITR> operator+(ptrdiff_t n,
                                       const reverse_iterator<ITR>& a) {
    return reverse_iterator<ITR>(a.base() - n);
}

// ItrWrap /////////////////////////////////////////////////////////////////////
// Implement (some of) the following iterators in your "base" implementations
// and then use ItrWrap<YourBaseIterator> to allow this class to implement the
// remaining operator overloads:
//
//    operator*() const
//    operator->() const
//    operator++()
//    operator--()
//    operator-(rhs)
//    operator+=(ptrdiff_t n)
//    operator[](ptrdiff_t n) const

template <class ITR>
class ItrWrap;

template <class ITR1, class ITR2>
bool operator==(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

template <class ITR1, class ITR2>
bool operator<(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

template <class ITR1, class ITR2>
bool operator!=(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

template <class ITR1, class ITR2>
bool operator>(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

template <class ITR1, class ITR2>
bool operator>=(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

template <class ITR1, class ITR2>
bool operator<=(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

template <class ITR1, class ITR2>
ptrdiff_t operator-(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

template <class ITR>
ItrWrap<ITR> operator+(ptrdiff_t, ItrWrap<ITR>);

template <class ITR>
class ItrWrap {
  public:
    typedef ITR iterator_type;
    typedef typename iterator_traits<iterator_type>::iterator_category
            iterator_category;
    typedef typename iterator_traits<iterator_type>::value_type value_type;
    typedef typename iterator_traits<iterator_type>::pointer pointer;
    typedef typename iterator_traits<iterator_type>::reference reference;

  private:
    iterator_type i;

  public:
    ItrWrap() {}

    template <class OTHER>
    ItrWrap(const ItrWrap<OTHER>& x,
            typename std::enable_if<
                    std::is_convertible<OTHER, iterator_type>::value>::type* =
                    0)
        : i(x.base()) {}

    reference operator*() const { return *i; }
    pointer operator->() const {
        // hack hack hack...
        return (pointer) & reinterpret_cast<const volatile char&>(*i);
    }

    ItrWrap& operator++() {
        ++i;
        return *this;
    }

    ItrWrap operator++(int) {
        ItrWrap __tmp(*this);
        ++(*this);
        return __tmp;
    }

    ItrWrap& operator--() {
        --i;
        return *this;
    }

    ItrWrap operator--(int) {
        ItrWrap __tmp(*this);
        --(*this);
        return __tmp;
    }

    ItrWrap operator+(ptrdiff_t n) const {
        ItrWrap __w(*this);
        __w += n;
        return __w;
    }

    ItrWrap& operator+=(ptrdiff_t n) {
        i += n;
        return *this;
    }

    ItrWrap operator-(ptrdiff_t n) const { return *this + (-n); }

    ItrWrap& operator-=(ptrdiff_t n) {
        *this += -n;
        return *this;
    }

    reference operator[](ptrdiff_t n) const { return i[n]; }

    explicit operator bool() const { return i.operator bool(); }

    iterator_type base() const { return i; }

    static ItrWrap create(iterator_type x) { return ItrWrap(x); }

  private:
    ItrWrap(iterator_type x) : i(x) {}

    template <class _Up>
    friend class ItrWrap;

    template <class ITR1, class ITR2>
    friend bool operator==(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

    template <class ITR1, class ITR2>
    friend bool operator<(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

    template <class ITR1, class ITR2>
    friend bool operator!=(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

    template <class ITR1, class ITR2>
    friend bool operator>(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

    template <class ITR1, class ITR2>
    friend bool operator>=(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

    template <class ITR1, class ITR2>
    friend bool operator<=(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

    template <class ITR1, class ITR2>
    friend ptrdiff_t operator-(const ItrWrap<ITR1>&, const ItrWrap<ITR2>&);

    template <class ITR1>
    friend ItrWrap<ITR1> operator+(ptrdiff_t, ItrWrap<ITR1>);
};

template <class ITR1, class ITR2>
inline bool operator==(const ItrWrap<ITR1>& x, const ItrWrap<ITR2>& y) {
    return x.base() == y.base();
}

template <class ITR1, class ITR2>
inline bool operator<(const ItrWrap<ITR1>& x, const ItrWrap<ITR2>& y) {
    return x.base() < y.base();
}

template <class ITR1, class ITR2>
inline bool operator!=(const ItrWrap<ITR1>& x, const ItrWrap<ITR2>& y) {
    return !(x == y);
}

template <class ITR1, class ITR2>
inline bool operator>(const ItrWrap<ITR1>& x, const ItrWrap<ITR2>& y) {
    return y < x;
}

template <class ITR1, class ITR2>
inline bool operator>=(const ItrWrap<ITR1>& x, const ItrWrap<ITR2>& y) {
    return !(x < y);
}

template <class ITR1, class ITR2>
inline bool operator<=(const ItrWrap<ITR1>& x, const ItrWrap<ITR2>& y) {
    return !(y < x);
}

template <class ITR1>
inline bool operator!=(const ItrWrap<ITR1>& x, const ItrWrap<ITR1>& y) {
    return !(x == y);
}

template <class ITR1>
inline bool operator>(const ItrWrap<ITR1>& x, const ItrWrap<ITR1>& y) {
    return y < x;
}

template <class ITR1>
inline bool operator>=(const ItrWrap<ITR1>& x, const ItrWrap<ITR1>& y) {
    return !(x < y);
}

template <class ITR1>
inline bool operator<=(const ItrWrap<ITR1>& x, const ItrWrap<ITR1>& y) {
    return !(y < x);
}

template <class ITR1, class ITR2>
inline ptrdiff_t operator-(const ItrWrap<ITR1>& x, const ItrWrap<ITR2>& y) {
    return x.base() - y.base();
}

template <class ITR>
inline ItrWrap<ITR> operator+(ptrdiff_t n, ItrWrap<ITR> x) {
    x += n;
    return x;
}

}  // namespace vtss

#endif  // __VTSS_BASICS_ITERATOR_HXX__
