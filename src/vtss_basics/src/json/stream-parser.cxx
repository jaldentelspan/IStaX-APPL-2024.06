/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#define TRACE(X) VTSS_BASICS_TRACE(X)

#include "vtss/basics/parser_impl.hxx"
#include "vtss/basics/json/stream-parser.hxx"
#include "vtss/basics/expose/json/string-decoder.hxx"


namespace vtss {
namespace json {

ostream &operator<<(ostream &o, StreamParser::Scope s) {
    switch (s) {
#define CASE(X)                  \
    case StreamParser::Scope::X: \
        o << #X;                 \
        return o
        CASE(NONE);
        CASE(PRIMITIVE);
        CASE(OBJECT);
        CASE(OBJECT_PAIR);
        CASE(OBJECT_NAME);
        CASE(OBJECT_NAME_ESCAPE);
        CASE(OBJECT_SEPERATE);
        CASE(OBJECT_VALUE);
        CASE(ARRAY);
        CASE(ARRAY_VALUE);
        CASE(STRING);
        CASE(STRING_ESCAPE);
#undef CASE
    default:
        o << "<Unknown value " << (int)s << ">";
    }

    return o;
}

StreamParser::StreamParser(StreamParserCallback *cb) : cb_(cb) { reset(); }

bool StreamParser::process(str s) {
    TRACE(DEBUG) << "Processing: " << s;
    reset();

    const char *e = s.end();
    const char *i = process(s.begin(), e);
    for (; i != e && is_white_space(*i); ++i)
        ;
    if (i != e) {
        TRACE(DEBUG) << "String not completly consumed: " << str(i, e);
        flag_error(__LINE__);
    }
    if (error()) return false;

    flush();

    return done_ok();
}

bool StreamParser::done_ok() const {
    TRACE(INFO) << "OK Done: Failed=" << failed_
                << " StackSize=" << stack_.size();
    if (failed_) return false;
    if (stack_.size()) return false;
    return true;
}

bool StreamParser::flag_error(uint32_t line) {
    TRACE(INFO) << "Flag error: " << line;
    failed_ = 1;
    return false;
}

void StreamParser::flush() {
    if (state() == Scope::PRIMITIVE) flush_primitiv();
}

void StreamParser::reset() {
    failed_ = 0;
    skip_mode_stack_ptr_ = (size_t)-1;
    stack_.clear();
    empty_stack = true;
}

StreamParser::Scope StreamParser::state() {
    if (stack_.size() == 0) {
        return Scope::NONE;
    } else {
        return stack_.back();
    }
}

bool StreamParser::flush_string(std::string &s) {
    TRACE(DEBUG) << "Parse string: >" << buf << "<";

    parser::JsonString p;
    const char *b = &*buf.begin();
    const char *e = &*buf.end();

    if (!p(b, e) || b != e) {
        buf.clear();
        return flag_error(__LINE__);
    }

    buf.clear();
    s = vtss::move(p.value);

    return true;
}

bool StreamParser::flush_primitiv() {
    parser::Lit null_("null");
    parser::Lit true_("true");
    parser::Lit false_("false");
    parser::Int<uint32_t, 10, 1, 0> uint32_;
    parser::Int<int32_t, 10, 1, 0> int32_;
    parser::Int<uint64_t, 10, 1, 0> uint64_;
    parser::Int<int64_t, 10, 1, 0> int64_;

    if (!buf.ok()) {
        buf.clear();
        return flag_error(__LINE__);
    }

    std::string buf_;
    std::swap(buf_, buf.buf);
    buf.clear();

    const char *b = &*buf_.begin();
    const char *e = buf_.c_str() + buf_.size();
    if (null_(b, e) && b == e) {
        TRACE(DEBUG) << "Primitive parse as 'null'";
        if (!skip_mode()) cb_->null();
        return true;
    }

    b = &*buf_.begin();
    if (true_(b, e) && b == e) {
        TRACE(DEBUG) << "Primitive parse as 'true'";
        if (!skip_mode()) cb_->boolean(true);
        return true;
    }

    b = &*buf_.begin();
    if (false_(b, e) && b == e) {
        TRACE(DEBUG) << "Primitive parse as 'false'";
        if (!skip_mode()) cb_->boolean(false);
        return true;
    }

    b = &*buf_.begin();
    if (uint32_(b, e) && b == e) {
        TRACE(DEBUG) << "Primitive parse as 'uint32_t'";
        if (!skip_mode()) cb_->number_value(uint32_.get());
        return true;
    }

    b = &*buf_.begin();
    if (int32_(b, e) && b == e) {
        TRACE(DEBUG) << "Primitive parse as 'int32_t'";
        if (!skip_mode()) cb_->number_value(int32_.get());
        return true;
    }

    b = &*buf_.begin();
    if (uint64_(b, e) && b == e) {
        TRACE(DEBUG) << "Primitive parse as 'uint64_t'";
        if (!skip_mode()) cb_->number_value(uint64_.get());
        return true;
    }

    b = &*buf_.begin();
    if (int64_(b, e) && b == e) {
        TRACE(DEBUG) << "Primitive parse as 'int64_t'";
        if (!skip_mode()) cb_->number_value(int64_.get());
        return true;
    }

    TRACE(INFO) << "Primitive failed to parse: >" << buf_ << "<";
    return flag_error(__LINE__);
}

bool StreamParser::pop_scope(Scope s) {
    TRACE(DEBUG) << "Pop scope: " << s;

    std::string _s;
    bool res = true;

    if (s != state() || stack_.size() == 0) {
        TRACE(DEBUG) << "s = " << s << ", state() = " << state() << ", stack_.size() == " << stack_.size();
        return flag_error(__LINE__);
    }

    switch (s) {
    case Scope::ARRAY:
        if (!skip_mode()) cb_->array_end();
        break;

    case Scope::OBJECT:
        if (!skip_mode()) cb_->object_end();
        break;

    case Scope::PRIMITIVE:
        res = flush_primitiv();
        break;

    case Scope::STRING:
        res = flush_string(_s);
        if (res && !skip_mode()) cb_->string_value(vtss::move(_s));
        break;

    case Scope::OBJECT_NAME:
        res = flush_string(_s);
        if (res && !skip_mode()) {
            auto a = cb_->object_element_start(_s);
            if (a != StreamParserCallback::ACCEPT) skip_mode_enter(1);
        }
        break;

    case Scope::OBJECT_VALUE:
        if (!skip_mode()) cb_->object_element_end();

    default:;
    }

    // Check if we should leave skip mode
    skip_mode_consider_leave();

    // Pop from stack
    stack_.pop_back();
    empty_stack = stack_.empty();

    return res;
}

bool StreamParser::push_scope(Scope s) {
    TRACE(DEBUG) << "Push scope: " << s;

    if (!stack_.push_back(s)) return flag_error(__LINE__);
    empty_stack = false;

    StreamParserCallback::Action a = StreamParserCallback::ACCEPT;
    switch (s) {
    case Scope::ARRAY:
        if (!skip_mode()) a = cb_->array_start();
        break;

    case Scope::OBJECT:
        if (!skip_mode()) a = cb_->object_start();
        break;

    case Scope::STRING:
    case Scope::OBJECT_NAME:
        buf.push('"');
        break;

    default:;
    }

    if (a != StreamParserCallback::ACCEPT) skip_mode_enter();

    return true;
}

bool StreamParser::skip_mode() const {
    return skip_mode_stack_ptr_ <= stack_.size() || cb_ == nullptr;
}

void StreamParser::skip_mode_enter(size_t i) {
    TRACE(DEBUG) << "Enter skip mode: " << i;
    if (stack_.size() < i) {
        TRACE(ERROR) << "Invalid skip mode offset: " << i << " " << stack_.size();
        flag_error(__LINE__);
        skip_mode_stack_ptr_ = 0;
    } else {
        skip_mode_stack_ptr_ = stack_.size() - i;
    }
}

void StreamParser::skip_mode_consider_leave() {
    if (skip_mode_stack_ptr_ != stack_.size()) return;
    TRACE(DEBUG) << "Leave skip mode";
    skip_mode_stack_ptr_ = (size_t)-1;
}

bool StreamParser::push(char c) {
    TRACE(NOISE) << "State: " << state() << " push char: '" << c << "'";
    switch (state()) {
    case Scope::PRIMITIVE:
        return push_accept_primitive(c);

    case Scope::NONE:
        return push_accept_value(c);


    case Scope::ARRAY:
    case Scope::ARRAY_VALUE:
        return push_accept_array_value(c);


    case Scope::OBJECT:
    case Scope::OBJECT_PAIR:
        return push_accept_object_pair(c);

    case Scope::OBJECT_NAME:
        return push_accept_object_name(c);

    case Scope::OBJECT_NAME_ESCAPE:
        return push_accept_object_name_escape(c);

    case Scope::OBJECT_SEPERATE:
        return push_accept_object_sep(c);

    case Scope::OBJECT_VALUE:
        return push_accept_object_value(c);


    case Scope::STRING:
        return push_accept_string(c);

    case Scope::STRING_ESCAPE:
        return push_accept_string_escape(c);

    default:
        failed_ = 1;
    }

    return flag_error(__LINE__);
}

bool StreamParser::in_range(char c, char a, char b) { return c >= a && c <= b; }

bool StreamParser::is_primitive(char c) {
    return in_range(c, 'a', 'z') || in_range(c, 'A', 'Z') ||
           in_range(c, '0', '9') || c == '+' || c == '-' || c == '.';
}

bool StreamParser::is_white_space(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

bool StreamParser::push_accept_primitive(char c) {
    if (is_primitive(c)) {
        // char accepted as part of current primitive value
        buf.push(c);
        return true;
    }

    // Current primitive has ended. If this ends a message, then we may not
    // accept this char
    pop_scope(Scope::PRIMITIVE);

    // continue processing this char
    push(c);
    return true;
}

bool StreamParser::push_accept_object_name(char c) {
    buf.push(c);

    if (c == '\\') return push_scope(Scope::OBJECT_NAME_ESCAPE);
    if (c == '"') {
        pop_scope(Scope::OBJECT_NAME);
        return push_scope(Scope::OBJECT_SEPERATE);
    }

    return true;
}

bool StreamParser::push_accept_object_name_escape(char c) {
    buf.push(c);
    pop_scope(Scope::OBJECT_NAME_ESCAPE);

    return true;
}

bool StreamParser::push_accept_object_sep(char c) {
    if (is_white_space(c)) return true;

    if (c == ':') {
        pop_scope(Scope::OBJECT_SEPERATE);
        return push_scope(Scope::OBJECT_VALUE);
    }

    return flag_error(__LINE__);
}

bool StreamParser::push_accept_object_value(char c) {
    if (push_accept_value(c)) {
        return true;

    } else if (c == ',') {
        return pop_scope(Scope::OBJECT_VALUE) &&
               pop_scope(Scope::OBJECT_PAIR) && push_scope(Scope::OBJECT_PAIR);

    } else if (c == '}') {
        return pop_scope(Scope::OBJECT_VALUE) &&
               pop_scope(Scope::OBJECT_PAIR) && pop_scope(Scope::OBJECT);
    }

    return flag_error(__LINE__);
}

bool StreamParser::push_accept_object_pair(char c) {
    if (is_white_space(c)) {
        return true;

    } else if (c == '}') {
        return pop_scope(Scope::OBJECT_PAIR) && pop_scope(Scope::OBJECT);

    } else if (c == '"') {
        return push_scope(Scope::OBJECT_NAME);
    }

    return flag_error(__LINE__);
}

bool StreamParser::push_accept_array_value(char c) {
    if (push_accept_value(c)) {
        return true;

    } else if (c == ',') {
        return pop_scope(Scope::ARRAY_VALUE) && push_scope(Scope::ARRAY_VALUE);

    } else if (c == ']') {
        return pop_scope(Scope::ARRAY_VALUE) && pop_scope(Scope::ARRAY);
    }

    return flag_error(__LINE__);
}

bool StreamParser::push_accept_value(char c) {
    if (is_white_space(c)) {
        return true;

    } else if (is_primitive(c)) {
        buf.push(c);
        return push_scope(Scope::PRIMITIVE);

    } else if (c == '[') {
        return push_scope(Scope::ARRAY) && push_scope(Scope::ARRAY_VALUE);

    } else if (c == '{') {
        return push_scope(Scope::OBJECT) && push_scope(Scope::OBJECT_PAIR);

    } else if (c == '"') {
        return push_scope(Scope::STRING);
    }

    // Should not flag error as this is sometimes expected
    return false;
}

bool StreamParser::push_accept_string(char c) {
    buf.push(c);

    if (c == '\\') push_scope(Scope::STRING_ESCAPE);
    if (c == '"') pop_scope(Scope::STRING);
    return true;
}

bool StreamParser::push_accept_string_escape(char c) {
    buf.push(c);

    pop_scope(Scope::STRING_ESCAPE);
    return true;
}

}  // namespace json
}  // namespace vtss
