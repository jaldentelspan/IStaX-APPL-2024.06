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

#ifndef __VTSS_APPL_ALARM_EXPRESSION_JSON_PARSE_HXX__
#define __VTSS_APPL_ALARM_EXPRESSION_JSON_PARSE_HXX__

#include "alarm-expression/any.hxx"
#include "alarm-expression/tree-element.hxx"
#include "alarm-expression/json-parse-token.hxx"

#include <vtss/basics/json/stream-parser.hxx>
#include <vtss/basics/json/stream-parser-callback.hxx>


namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

enum class Mode {
    // Done processing, just return
    done,

    // Process the stack
    normal,

    // Wait for an object element with name "key"
    key_search,

    // Capture the keys provided
    key_capture,

    // Wait for an object element with name "val"
    val_search,

    // store the next value
    capture_value,
};

ostream &operator<<(ostream &o, Mode m);

struct JsonParse : public json::StreamParserCallback {
    using json::StreamParserCallback::Action;

    JsonParse(const Vector<JsonParseToken> &stack,
              const Vector<TreeElementConst> &key_expect);

    // Implements the callback functions from json::StreamParserCallback
    Action array_start() override;
    void array_end() override;
    Action object_start() override;
    void object_end() override;
    Action object_element_start(const std::string &s) override;
    void object_element_end() override;
    void null() override;
    void boolean(bool val) override;
    void number_value(uint32_t i) override;
    void number_value(int32_t i) override;
    void number_value(uint64_t i) override;
    void number_value(int64_t i) override;
    void string_value(const std::string &&s) override;
    void stream_error() override;

    void flag_error(int line);

    Mode mode();
    void mode(int line, Mode m);

    bool ok();

    const JsonParseToken *pop(JsonParseToken::E t);
    const JsonParseToken *back(JsonParseToken::E t);

    const AnyJsonPrimitive &get_value() const { return value; }

    bool cmp(const TreeElementConst &expect, TreeElementConst &actual, size_t n);

    uint32_t err = 0;

    Mode mode_ = Mode::normal;

    uint32_t stack_ptr = 0;
    const Vector<JsonParseToken> &stack_;
    Vector<TreeElementConst> key_actual_;
    const Vector<TreeElementConst> &key_expect_;

    AnyJsonPrimitive value;
};

}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#endif  // __VTSS_APPL_ALARM_EXPRESSION_JSON_PARSE_HXX__
