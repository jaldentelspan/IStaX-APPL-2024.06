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

#include "vtss/basics/expose/json/literal.hxx"
#include "vtss/basics/expose/json/skip-value.hxx"
#include "vtss/basics/expose/json/skip-string.hxx"
#include "vtss/basics/expose/json/skip-object.hxx"

namespace vtss {
namespace expose {
namespace json {

bool parse(const char *&b, const char *e, const SkipObject &) {
    if (!parse(b, e, map_start)) {
        return false;
    }

    // check for empty map
    if (parse(b, e, map_end)) {
        return true;
    }

    while (1) {
        SkipString string;
        SkipValue value;

        // element name
        if (!parse(b, e, string)) {
            return false;
        }

        // delimitor
        if (!parse(b, e, map_assign)) {
            return false;
        }

        // value
        if (!parse(b, e, value)) {
            return false;
        }

        // check for end-of-map
        if (parse(b, e, map_end)) {
            return true;
        }

        // check of comma (more values)
        if (!parse(b, e, delimetor)) {
            // not end-of-map, and not comma (more values)
            return false;
        }
    }

    return true;
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

