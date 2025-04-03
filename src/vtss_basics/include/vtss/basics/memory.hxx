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

#ifndef __VTSS_BASICS_MEMORY_HXX__
#define __VTSS_BASICS_MEMORY_HXX__

#include <memory>
#include "vtss/basics/new.hxx"
#include "vtss/basics/meta.hxx"

namespace vtss {

template <class Type, class... Args>
typename std::unique_ptr<Type> make_unique(Args &&... __args) {
    return std::unique_ptr<Type>(new Type(::vtss::forward<Args>(__args)...));
}

template <class TYPE>
class unique_ptr {
public:
    typedef TYPE element_type;
    typedef TYPE *pointer;
private:
    pointer p_;

public:
    unique_ptr() : p_(pointer()) { }
    unique_ptr(nullptr_t) : p_(pointer()) { }
    explicit unique_ptr(pointer p) : p_(p) { }

    // TODO, enable if convertable...
    template<typename U>
    unique_ptr(unique_ptr<U>& u) : p_(u.release()) { }
    unique_ptr(unique_ptr&& u) : p_(u.release()) { }

    unique_ptr& operator=(unique_ptr&& u) {
        reset(u.release());
        return *this;
    }

    ~unique_ptr() { reset(); }

    unique_ptr& operator=(nullptr_t) {
        reset();
        return *this;
    }

    typename add_lvalue_reference<TYPE>::type operator*() const { return *p_; }

    pointer operator->() const { return p_; }
    pointer get() const {        return p_; }

    // TODO, apply safe-bool pattern
    //_LIBCPP_EXPLICIT
    operator bool() const { return p_ != nullptr; }
    bool ok() const { return p_ != nullptr; }

    pointer release() {
        pointer __t = p_;
        p_ = pointer();
        return __t;
    }

    void reset(pointer p = pointer()) {
        pointer __tmp = p_;
        p_ = p;
        if (__tmp) {
            destroy(__tmp);
        }
    }

    void swap(unique_ptr& u) {
        pointer tmp = p_;
        p_ = u.p_;
        u.p_ = tmp;
    }
};

template <class TYPE>
class unique_ptr <TYPE[]>{
public:
    typedef TYPE element_type;
    typedef TYPE *pointer;
private:
    pointer p_;

public:
    unique_ptr() : p_(pointer()) { }
    unique_ptr(nullptr_t) : p_(pointer()) { }
    explicit unique_ptr(pointer p) : p_(p) { }

    // TODO, enable if convertable...
    template<typename U>
    unique_ptr(unique_ptr<U>& u) : p_(u.release()) { }
    unique_ptr(unique_ptr&& u) : p_(u.release()) { }

    unique_ptr& operator=(unique_ptr&& u) {
        reset(u.release());
        return *this;
    }

    ~unique_ptr() { reset(); }

    unique_ptr& operator=(nullptr_t) {
        reset();
        return *this;
    }

    typename add_lvalue_reference<TYPE>::type operator*() const { return *p_; }
    typename add_lvalue_reference<TYPE>::type operator[](size_t i) const {
        return p_[i];
    }

    pointer operator->() const { return p_; }
    pointer get() const {        return p_; }

    // TODO, apply safe-bool pattern
    //_LIBCPP_EXPLICIT
    operator bool() const { return p_ != nullptr; }
    bool ok() const { return p_ != nullptr; }

    pointer release() {
        pointer __t = p_;
        p_ = pointer();
        return __t;
    }

    void reset(pointer p = pointer()) {
        pointer __tmp = p_;
        p_ = p;
        if (__tmp) {
            destroy(__tmp);
        }
    }

    void swap(unique_ptr& u) {
        pointer tmp = p_;
        p_ = u.p_;
        u.p_ = tmp;
    }
};

#define VTSS_CALLOC_UNIQ(__TYPE__, __CNT__)                                \
    ::vtss::unique_ptr<__TYPE__[]> (                                       \
            (__TYPE__ *)calloc(__CNT__, sizeof(__TYPE__)))

#define VTSS_ALLOC_UNIQ(__TYPE__)                                          \
    ::vtss::unique_ptr<__TYPE__> ((__TYPE__ *)calloc(1, sizeof(__TYPE__)))

}  // namespace vtss
#endif  // __VTSS_BASICS_MEMORY_HXX__
