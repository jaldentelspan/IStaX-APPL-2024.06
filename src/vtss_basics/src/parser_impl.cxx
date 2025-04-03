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

#include "vtss/basics/stream.hxx"
#include "vtss/basics/parser_impl.hxx"
#include "vtss/basics/expose/json/string-decoder.hxx"

namespace vtss {
namespace parser {

namespace {
struct CaptureStdString : public expose::json::StringDecodeHandler {
    explicit CaptureStdString() {}
    void push(char c) { s.push(c); }
    StringStream s;
};
}  // namespace

bool JsonString::operator()(const char *&b, const char *e) {
    const char *b_ = b;

    CaptureStdString cap;
    bool res = expose::json::string_decoder(b, e, &cap);

    if (res && cap.s.ok()) {
        value = vtss::move(cap.s.buf);
        return true;
    }

    b = b_;
    return false;
}

}  // namespace parser
}  // namespace vtss
