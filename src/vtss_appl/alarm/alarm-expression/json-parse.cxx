/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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


#include "alarm-expression/json-parse.hxx"
#include "alarm-trace.h"
#include <vtss/basics/trace.hxx>

namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

ostream &operator<<(ostream &o, Mode m) {
    switch (m) {
    case Mode::done:
        o << "done";
        return o;

    case Mode::normal:
        o << "normal";
        return o;

    case Mode::key_search:
        o << "key_search";
        return o;

    case Mode::key_capture:
        o << "key_capture";

        return o;
    case Mode::val_search:
        o << "val_search";
        return o;

    case Mode::capture_value:
        o << "capture_value";
        return o;
    }

    o << "<unknown>";
    return o;
}

JsonParse::JsonParse(const Vector<JsonParseToken> &stack,
                     const Vector<TreeElementConst> &key_expect)
    : stack_(stack), key_expect_(key_expect) {}

Mode JsonParse::mode() {
    if (stack_ptr == stack_.size()) return mode_;

    JsonParseToken::E t = stack_[stack_ptr].t;
    if (t == JsonParseToken::set_mode_normal) {
        mode(__LINE__, Mode::normal);
        stack_ptr++;
    } else if (t == JsonParseToken::set_mode_key_search) {
        mode(__LINE__, Mode::key_search);
        stack_ptr++;
    } else if (t == JsonParseToken::set_mode_capture_value) {
        mode(__LINE__, Mode::capture_value);
        stack_ptr++;
    }

    return mode_;
}

void JsonParse::mode(int line, Mode m) {
    if (m != mode_) {
        DEFAULT(DEBUG) << "New mode(" << line << "): " << m;
        mode_ = m;
    }
}

const JsonParseToken *JsonParse::back(JsonParseToken::E t) {
    if (stack_ptr == stack_.size()) return nullptr;

    auto *p = &stack_[stack_ptr];

    if (p->t == t) return p;

    return nullptr;
}

const JsonParseToken *JsonParse::pop(JsonParseToken::E t) {
    auto *p = back(t);

    if (p) {
        stack_ptr++;
        DEFAULT(DEBUG) << "POP-FROM-STACK";
    } else {
        DEFAULT(DEBUG) << "POP-FAILED";
    }

    return p;
}

json::StreamParserCallback::Action JsonParse::array_start() {
    auto m = mode();
    DEFAULT(DEBUG) << "array_end/" << m;

    switch (m) {
    case Mode::done:
        return SKIP;

    case Mode::normal:
        if (pop(JsonParseToken::array_start)) {
            DEFAULT(DEBUG) << "xx";
            return ACCEPT;
        } else {
            DEFAULT(DEBUG) << "xx";
            return SKIP;
        }

    case Mode::key_capture:
        DEFAULT(DEBUG) << "array start in key_capture mode is ok";
        return ACCEPT;

    default:
        flag_error(__LINE__);
        return SKIP;
    }
}

void JsonParse::array_end() {
    auto m = mode();
    DEFAULT(DEBUG) << "array_end/" << m;

    switch (m) {
    case Mode::normal:
        pop(JsonParseToken::array_end);
        break;

    default:;
    }
}

json::StreamParserCallback::Action JsonParse::object_start() {
    auto m = mode();
    DEFAULT(DEBUG) << "object_start/" << m;

    switch (m) {
    case Mode::done:
        return SKIP;

    case Mode::normal:
        if (pop(JsonParseToken::object_start)) {
            DEFAULT(DEBUG) << "xx";
            return ACCEPT;
        } else {
            DEFAULT(DEBUG) << "xx";
            return SKIP;
        }

    case Mode::key_search:
        return ACCEPT;

    default:
        flag_error(__LINE__);
        return SKIP;
    }
};

void JsonParse::object_end() {
    auto m = mode();
    DEFAULT(DEBUG) << "object_end/" << m;

    switch (m) {
    case Mode::normal:
        pop(JsonParseToken::object_end);
        break;

    default:;
    }
};

json::StreamParserCallback::Action JsonParse::object_element_start(
        const std::string &s) {
    auto m = mode();
    DEFAULT(DEBUG) << "object_element_start/" << m;

    switch (m) {
    case Mode::done:
        return SKIP;

    case Mode::normal: {
        auto b = back(JsonParseToken::object_element_start);
        if (b && b->s == s) {
            DEFAULT(DEBUG) << "xx";
            pop(JsonParseToken::object_element_start);
            return ACCEPT;

        } else {
            DEFAULT(DEBUG) << "xx";
            return SKIP;
        }
    }

    case Mode::key_search: {
        if (s == "key") {
            mode(__LINE__, Mode::key_capture);
            key_actual_.clear();
            return ACCEPT;

        } else {
            DEFAULT(DEBUG) << "xx";
            return SKIP;
        }
    }

    case Mode::val_search: {
        if (s == "val") {
            mode(__LINE__, Mode::normal);
            return ACCEPT;

        } else {
            DEFAULT(DEBUG) << "xx";
            return SKIP;
        }
    }

    case Mode::key_capture:
    case Mode::capture_value:
        flag_error(__LINE__);
        return SKIP;
    }

    flag_error(__LINE__);
    return SKIP;
};

bool JsonParse::cmp(const TreeElementConst &expect, TreeElementConst &actual,
                    size_t n) {
    if (!actual.convert_to(&expect)) {
        DEFAULT(INFO) << "Could not convert index #" << n
                      << " expected-type: " << expect.name_of_type()
                      << " actual-type: " << actual.name_of_type();
        mode(__LINE__, Mode::key_search);
        return false;
    }

    DEFAULT(DEBUG) << "Index #" << n << " converted to " << expect.name_of_type();

    // ready to check that we have the operator needed.
    auto operator_result_ptr =
            ANY_TYPE_INVENTORY.operator_result(expect.name_of_type());

    if (!operator_result_ptr) {
        DEFAULT(INFO) << "has_operator function pointer was not registered for "
                         "type: "
                      << expect.name_of_type();
        return false;
    }

    str result_type = operator_result_ptr(Token::equal);
    if (result_type.size() == 0) {
        DEFAULT(INFO) << "Compare operator is not implemented for type: "
                      << expect.name_of_type();
        return false;
    }

    auto res = expect.value->opr(Token::equal, nullptr, actual.value);

    if (res->name_of_type() != AnyBool::NAME_OF_TYPE) {
        DEFAULT(INFO) << "Compare did not return a bool!!";
        return false;
    }

    auto res_ = std::static_pointer_cast<AnyBool>(res);

    DEFAULT(DEBUG) << "Compare result: " << res_->value;

    return res_->value;
}

void JsonParse::object_element_end() {
    auto m = mode();
    DEFAULT(DEBUG) << "object_element_end/" << m;

    switch (m) {
    case Mode::normal:
        pop(JsonParseToken::object_element_end);
        break;

    case Mode::key_capture: {
        if (key_expect_.size() != key_actual_.size()) {
            DEFAULT(INFO) << "Expected " << key_expect_.size()
                           << " indexies, got: " << key_actual_.size()
                           << " indexes";

            mode(__LINE__, Mode::key_search);
            return;
        }

        DEFAULT(DEBUG) << "Got " << key_expect_.size() << " indexes";
        AnySharedPtr result_buffer = std::make_shared<AnyBool>(false);
        for (size_t n = 0; n < key_expect_.size(); ++n) {
            // Check that all expect/actual pairs matches
            if (!cmp(key_expect_[n], key_actual_[n], n)) {
                mode(__LINE__, Mode::key_search);
                return;
            }
        }

        DEFAULT(DEBUG) << "Compare OK - searching for value";
        mode(__LINE__, Mode::val_search);
        break;
    }

    default:;
    }
};

void JsonParse::null() {
    DEFAULT(DEBUG) << "Capture null";

    switch (mode()) {
    case Mode::key_capture: {
        key_actual_.emplace_back(nullptr);
        break;
    }

    case Mode::capture_value: {
        value = AnyJsonPrimitive(nullptr);
        mode(__LINE__, Mode::done);
        break;
    }

    default:;
    }
};

void JsonParse::boolean(bool val) {
    DEFAULT(INFO) << "Capture bool: " << val;

    switch (mode()) {
    case Mode::key_capture: {
        key_actual_.emplace_back(val);
        break;
    }

    case Mode::capture_value: {
        value = AnyJsonPrimitive(val);
        mode(__LINE__, Mode::done);
        break;
    }

    default:;
    }
};

void JsonParse::number_value(uint32_t i) {
    DEFAULT(INFO) << "Capture uint32_t: " << i;

    switch (mode()) {
    case Mode::key_capture: {
        key_actual_.emplace_back(i);
        break;
    }

    case Mode::capture_value: {
        value = AnyJsonPrimitive(i);
        mode(__LINE__, Mode::done);
        break;
    }

    default:;
    }
};

void JsonParse::number_value(int32_t i) {
    DEFAULT(INFO) << "Capture int32_t: " << i;

    switch (mode()) {
    case Mode::key_capture: {
        key_actual_.emplace_back(i);
        break;
    }

    case Mode::capture_value: {
        value = AnyJsonPrimitive(i);
        mode(__LINE__, Mode::done);
        break;
    }

    default:;
    }
};

void JsonParse::number_value(uint64_t i) {
    DEFAULT(INFO) << "Capture uint64_t: " << i;

    switch (mode()) {
    case Mode::key_capture: {
        key_actual_.emplace_back(i);
        break;
    }

    case Mode::capture_value: {
        value = AnyJsonPrimitive(i);
        mode(__LINE__, Mode::done);
        break;
    }

    default:;
    }
};

void JsonParse::number_value(int64_t i) {
    DEFAULT(INFO) << "Capture int64_t: " << i;

    switch (mode()) {
    case Mode::key_capture: {
        key_actual_.emplace_back(i);
        break;
    }

    case Mode::capture_value: {
        value = AnyJsonPrimitive(i);
        mode(__LINE__, Mode::done);
        break;
    }

    default:;
    }
};

void JsonParse::string_value(const std::string &&s) {
    DEFAULT(INFO) << "Capture string: " << s;

    switch (mode()) {
    case Mode::key_capture: {
        key_actual_.emplace_back(std::move(s));
        break;
    }

    case Mode::capture_value: {
        value = AnyJsonPrimitive(std::move(s));
        mode(__LINE__, Mode::done);
        break;
    }

    default:;
    }
}

void JsonParse::stream_error() { flag_error(__LINE__); }

void JsonParse::flag_error(int line) {
    DEFAULT(INFO) << "Error at line: " << line;
    err++;
}

bool JsonParse::ok() {
    if (mode() != Mode::done) {
        return false;
    }

    if (err != 0) {
        return false;
    }

    return true;
}

}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss
