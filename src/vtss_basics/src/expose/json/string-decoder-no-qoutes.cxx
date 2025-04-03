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
#include "vtss/basics/expose/json/string-decoder-no-qoutes.hxx"

namespace vtss {
namespace expose {
namespace json {

char uneschape_char(char c) {
    switch (c) {
    case '"':
        return '"';
    case '\\':
        return '\\';
    case '/':
        return '/';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    default:
        return 0;
    }
}

bool string_decoder_no_qoutes(const char *&b, const char *e,
                              StringDecodeHandler *buf) {
    const char *i = b;
    bool escape_mode = false;

    // parse string body
    while (i != e) {
        if (escape_mode) {
            // We are in eschape mode
            char val = uneschape_char(*i);
            if (val) {
                (void)buf->push(val);

            } else {
                return false;
            }

            escape_mode = false;

        } else if (*i == '\\') {
            // We are entering eschape mode
            escape_mode = true;

        } else if (*i == '"') {
            // End of string
            break;

        } else {
            // Normal string processing
            (void)buf->push(*i);
        }

        ++i;
    }

    // String has been parsed
    b = i;
    return true;
};


}  // namespace json
}  // namespace expose
}  // namespace vtss

