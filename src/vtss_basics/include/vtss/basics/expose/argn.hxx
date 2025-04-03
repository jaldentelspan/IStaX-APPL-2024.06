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

#ifndef __VTSS_BASICS_EXPOSE_ARGN_HXX__
#define __VTSS_BASICS_EXPOSE_ARGN_HXX__

// This file declare 16 simple dummy types named _1, _2..._16, these types can
// (must) be used as arguments in member serializer of interface descriptors.
// The purpose of this is to give the compiler a way to select the correct
// serializer if they accept the same type.

namespace vtss {
namespace expose {
namespace arg {

// A simple utility class to convert an unsigned to a type.
template <unsigned N>
struct Int2ArgN;

#define ARGN(X) \
struct _ ## X {};   \
template <>         \
struct Int2ArgN<X> { typedef _ ## X type; }

ARGN(1);
ARGN(2);
ARGN(3);
ARGN(4);
ARGN(5);
ARGN(6);
ARGN(7);
ARGN(8);
ARGN(9);
ARGN(10);
ARGN(11);
ARGN(12);
ARGN(13);
ARGN(14);
ARGN(15);
ARGN(16);

#undef ARGN
}  // namespace arg
}  // namespace expose
}  // namespace vtss

#endif  //  __VTSS_BASICS_EXPOSE_ARGN_HXX__
