/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_CLEAR_HXX__
#define __VTSS_BASICS_CLEAR_HXX__

#include <vtss/basics/common.hxx>
#include <vtss/basics/type_traits.hxx>

namespace vtss {

// Like memset, but works with NON-POD data
template<typename TYPE>
void clear(TYPE &type) {
    static_assert(!vtss::is_pointer<TYPE>::value,
                  "clear expect a reference not a pointer");
    type.~TYPE();
    memset((void*)&type, 0, sizeof(type));  // NOLINT
    new (&type) TYPE();
}

template<typename TYPE, int CNT>
void clear(TYPE (&type)[CNT]) {
    for (size_t i = 0; i < CNT; i++) {
        type[i].~TYPE();
        memset((void*)&type[i], 0, sizeof(type[0]));  // NOLINT
        new (&type[i]) TYPE();
    }
}

}  // namespace vtss

#endif  // __VTSS_BASICS_CLEAR_HXX__
