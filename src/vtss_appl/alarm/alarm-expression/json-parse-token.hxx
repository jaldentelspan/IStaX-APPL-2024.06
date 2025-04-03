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

#ifndef __VTSS_APPL_ALARM_EXPRESSION_JSON_PARSE_TOKEN_HXX__
#define __VTSS_APPL_ALARM_EXPRESSION_JSON_PARSE_TOKEN_HXX__

#include <string>

namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

struct JsonParseToken {
    enum E {
        array_start,
        array_end,
        object_start,
        object_end,
        object_element_start,
        object_element_end,
        null,
        boolean,
        number_value_uint32_t,
        number_value_int32_t,
        number_value_uint64_t,
        number_value_int64_t,
        string_value,

        set_mode_normal,
        set_mode_key_search,
        set_mode_capture_value,
    };

    JsonParseToken() {}
    JsonParseToken(const JsonParseToken &e) = default;

    JsonParseToken(E t_) : t(t_) {}
    JsonParseToken(E t_, std::string s_) : t(t_), s(s_) {}

    E t;
    std::string s;
};

}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#endif  // __VTSS_APPL_ALARM_EXPRESSION_JSON_PARSE_TOKEN_HXX__
