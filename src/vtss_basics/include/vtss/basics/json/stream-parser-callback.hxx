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

#ifndef __VTSS_BASICS_JSON_STREAM_PARSER_CALLBACK_HXX__
#define __VTSS_BASICS_JSON_STREAM_PARSER_CALLBACK_HXX__

#include <string>
#include <stdint.h>

namespace vtss {
namespace json {

struct StreamParserCallback {
    enum Action { SKIP = 0, STOP = 1, ACCEPT = 2 };

    virtual Action array_start() { return SKIP; }

    virtual void array_end() { }

    virtual Action object_start() { return SKIP; }

    virtual void object_end() { }

    virtual Action object_element_start(const std::string &s) { return SKIP; }

    virtual void object_element_end() { }

    virtual void null() { }

    virtual void boolean(bool b) { }

    virtual void number_value(uint32_t i) { }

    virtual void number_value(int32_t i) { }

    virtual void number_value(uint64_t i) { }

    virtual void number_value(int64_t i) { }

    virtual void string_value(const std::string &&s) { }

    virtual void stream_error() { }

};

}  // namespace json
}  // namespace vtss

#endif  // __VTSS_BASICS_JSON_STREAM_PARSER_CALLBACK_HXX__
