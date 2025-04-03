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

#ifndef __VTSS_BASICS_AS_REF_HXX__
#define __VTSS_BASICS_AS_REF_HXX__

#include <type_traits>
namespace vtss {

template <typename T>
struct AsRef;

template <typename T>
struct AsRef {
    typedef T R;
    typedef T A;
};

template <typename T>
struct AsRef<T*> {
    typedef T& R;
    typedef T* A;
};

template <typename T>
struct AsRef<T&> {
    typedef T& R;
    typedef T& A;
};

template <typename T>
struct AsRef<const T> {
    typedef const T R;
    typedef const T A;
};

template <typename T>
struct AsRef<const T*> {
    typedef const T& R;
    typedef const T* A;
};

template <typename T>
struct AsRef<const T&> {
    typedef const T& R;
    typedef const T& A;
};

// WARNING: Do not change this code to allow type deduction of the template
// argument!
//
// Read the following link if you need an argument:
// http://stackoverflow.com/questions/7779900/why-is-template-argument-deduction-disabled-with-stdforward

template <typename T>
typename std::enable_if<(std::is_pointer<T>::value), typename AsRef<T>::R>::type
as_ref(typename AsRef<T>::A t) {
    return *t;
}

template <typename T>
typename std::enable_if<!std::is_pointer<T>::value, typename AsRef<T>::R>::type
as_ref(typename AsRef<T>::A t) {
    return t;
}

}  // namespace vtss

#endif  // __VTSS_BASICS_AS_REF_HXX__
