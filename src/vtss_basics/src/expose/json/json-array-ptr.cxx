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

#include "vtss/basics/expose/json/loader.hxx"
#include "vtss/basics/expose/json/literal.hxx"
#include "vtss/basics/expose/json/json-array-ptr.hxx"

namespace vtss {
namespace expose {
namespace json {

void serialize(Loader &l, JsonArrayPtr &d) {
    if (!parse(l.pos_, l.end_, array_start)) {
        l.flag_error();
        return;
    }

    // We got an Array start
    //*size = 0;
    d.data.clear();


    // check for empty array
    if (parse(l.pos_, l.end_, array_end)) {
        return;
    }

    while (1) {
        // value into JsonValuePtr
        JsonValuePtr v;
        if (!parse(l.pos_, l.end_, v)) {
            l.flag_error();
            return;
        }

        // Another values has been parsed
        d.data.push_back(v);

        // check for end-of-array
        if (parse(l.pos_, l.end_, array_end)) {
            return;
        }

        // check of comma (more values)
        if (!parse(l.pos_, l.end_, delimetor)) {
            // not end-of-array, and not comma (more values)
            l.flag_error();
            return;
        }
    }
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

