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

#include <vtss/basics/trace_grps.hxx>
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_JSON

#include <vtss/basics/trace_basics.hxx>
#include <vtss/basics/expose/json/string-literal.hxx>
#include <vtss/basics/expose/json/string-decoder-no-qoutes.hxx>

#define TRACE VTSS_BASICS_TRACE

namespace vtss {
namespace expose {
namespace json {

bool StringLiteral::operator()(const char*& b, const char* e) {
    const char* _b = b;
    const char* i = s_.begin();
    bool escape_mode = false;

    while (b != e && i != s_.end()) {
        TRACE(DEBUG) << *b << "(" << (int)*b << ") " << *i << "(" << (int)*i
                     << ")";

        if (escape_mode) {
            // We are in eschape mode.
            char val = uneschape_char(*b);
            if (val) {
                if (*b != *i) goto FAILED;
            } else {
                // Eschape mode, unexpected char...
                return false;
            }

            escape_mode = false;
            ++b;
            ++i;

        } else if (*b == '\\') {
            // We are entering eschape mode
            escape_mode = true;
            ++b;

        } else if (*b == '"') {
            // End of string - but we have not seen the end of the literal.
            goto FAILED;

        } else {
            // Normal string processing
            if (*b != *i) goto FAILED;
            ++b;
            ++i;
        }
    }

    if (i == s_.end()) return true;

FAILED:
    b = _b;
    return false;
}

}  // namespace json
}  // namespace expose
}  // namespace vtss
