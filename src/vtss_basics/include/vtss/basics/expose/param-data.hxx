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

#ifndef __VTSS_BASICS_EXPOSE_PARAM_DATA_HXX__
#define __VTSS_BASICS_EXPOSE_PARAM_DATA_HXX__

#include <vtss/basics/expose/param-val.hxx>
#include <vtss/basics/expose/param-key.hxx>

namespace vtss {
namespace expose {

template <typename T>
struct ParamData;

template <typename T>
struct ParamData<ParamVal<T>> {
    void get(T* rhs) const { *rhs = data; }
    void set(const T& rhs) { data = rhs; }
    bool equal(const T& rhs) const { return data == rhs; }

    static T& dereference_get_ptr_type(T* v) { return *v; }
    static T& dereference_get_next_type(T* v) { return *v; }
    static const T& dereference_set_ptr_type(const T& v) { return v; }

    T data;
};

template <typename T>
struct ParamData<ParamVal<T*>> {
    void get(T* rhs) const { *rhs = data; }
    void set(const T* rhs) { data = *rhs; }
    bool equal(const T* rhs) const { return data == *rhs; }

    static T& dereference_get_ptr_type(T* v) { return *v; }
    static T& dereference_get_next_type(T* v) { return *v; }
    static const T& dereference_set_ptr_type(const T* v) { return *v; }

    T data;
};

template <typename T>
struct ParamData<ParamVal<T&>> {
    void get(T& rhs) const { rhs = data; }
    void set(const T& rhs) { data = rhs; }
    bool equal(const T& rhs) const { return data == rhs; }

    static T& dereference_get_ptr_type(T& v) { return v; }
    static T& dereference_get_next_type(T& v) { return v; }
    static const T& dereference_set_ptr_type(const T& v) { return v; }

    T data;
};

template <typename T>
struct ParamData<ParamKey<T>> {
    // void get(T* rhs) { *rhs = data; }
    void set(const T& rhs) { data = rhs; }
    // bool equal(const T& rhs) { return data == rhs; }

    const T& as_get_arg() { return data; }

    static const T& dereference_get_ptr_type(const T& v) { return v; }
    static T& dereference_get_next_type(T* v) { return *v; }
    static const T& dereference_set_ptr_type(const T& v) { return v; }

    T data;
};

template <typename T>
struct ParamData<ParamKey<T*>> {
    // void get(T* rhs) { *rhs = data; }
    void set(const T* rhs) { data = *rhs; }
    // bool equal(const T* rhs) { return data == *rhs; }
    const T* as_get_arg() { return &data; }

    static const T& dereference_get_ptr_type(const T* v) { return *v; }
    static T& dereference_get_next_type(T* v) { return *v; }
    static const T& dereference_set_ptr_type(const T* v) { return *v; }

    T data;
};

template <typename T>
struct ParamData<ParamKey<T&>> {
    // void get(T& rhs) { rhs = data; }
    void set(const T& rhs) { data = rhs; }
    // bool equal(const T& rhs) { return data == rhs; }
    const T& as_get_arg() { return data; }

    static const T& dereference_get_ptr_type(const T& v) { return v; }
    static T& dereference_get_ptr_type(T& v) { return v; }
    static const T& dereference_set_ptr_type(const T& v) { return v; }

    T data;
};

}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_PARAM_DATA_HXX__
