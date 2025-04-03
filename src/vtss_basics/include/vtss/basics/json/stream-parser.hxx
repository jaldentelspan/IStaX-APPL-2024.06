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

#ifndef __VTSS_BASICS_JSON_STREAM_PARSER_HXX__
#define __VTSS_BASICS_JSON_STREAM_PARSER_HXX__

#include <stdint.h>
#include <vtss/basics/stream.hxx>
#include <vtss/basics/vector.hxx>
#include <vtss/basics/json/stream-parser-callback.hxx>

namespace vtss {
namespace json {

struct StreamParser {
    explicit StreamParser(StreamParserCallback *cb = nullptr);

    // Reset the internal state of the stream process.
    // This will not release an attached buffer, but it will reset pointerers
    // into the buffer.
    void reset();

    // Process a new chunk of data. It will return a pointer to where the
    // processing stopped.
    //  - If:    The returned pointer is equal to 'end'
    //    Then:  The complete chunk was processed
    //    But:   It may not represent a complete message, use error()/ok()/
    //           done_ok() to inspect the actual state.
    //
    //  - If:    The returned pointer is not equal to 'end'
    //    Then:  The stream processor has either encounter an error or a message
    //           boundary.
    //
    //  - Note:  If the message only contains a number "123" then the stream
    //           parser can not determinate if it has reached a message
    //           boundary, or if it will receive more digits. In such cases the
    //           'flush()' method must be called to signal end-of-message. This
    //           is normally not needed, as most messages is encapsulated in an
    //           object.
    template <class I>
    I *process(I *i, I *end) {
        for (; i != end && is_white_space(*i); ++i)
            ;

        while (i != end) {
            if (!push(*i)) return i;

            ++i;

            if (empty_stack) return i;
        }
        return i;
    }

    // Process a complete message. This will call flush as part of the
    // processing.
    bool process(str s);

    // Check if an error was found in the input stream
    bool error() const { return failed_; }

    // Check if the state is okay
    bool ok() const { return !error(); }

    // Check if a complete message has bee processed without any error.
    bool done_ok() const;

    // Indicate end-of-stream. This is only needed if running in chunk mode.
    void flush();

  private:
    enum class Scope {
        NONE,
        PRIMITIVE,
        OBJECT,
        OBJECT_PAIR,
        OBJECT_NAME,
        OBJECT_NAME_ESCAPE,
        OBJECT_SEPERATE,
        OBJECT_VALUE,
        ARRAY,
        ARRAY_VALUE,
        STRING,
        STRING_ESCAPE
    };
    friend ostream &operator<<(ostream &o, StreamParser::Scope e);

    uint32_t failed_ = 0;
    bool empty_stack = true;
    size_t skip_mode_stack_ptr_;
    StreamParserCallback *cb_ = nullptr;
    StringStream buf;

    Vector<Scope> stack_;

    bool flag_error(uint32_t line);

    bool skip_mode() const;
    void skip_mode_enter(size_t i = 0);
    void skip_mode_consider_leave();

    Scope state();
    bool push(char c);
    bool pop_scope(Scope s);
    bool push_scope(Scope s);

    bool in_range(char c, char a, char b);
    bool is_primitive(char c);
    bool is_white_space(char c);
    bool push_accept_primitive(char c);
    bool push_accept_object_name(char c);
    bool push_accept_object_name_escape(char c);
    bool push_accept_object_sep(char c);
    bool push_accept_object_value(char c);
    bool push_accept_object_pair(char c);
    bool push_accept_array_value(char c);
    bool push_accept_value(char c);
    bool push_accept_string(char c);
    bool push_accept_string_escape(char c);

    bool flush_primitiv();
    bool flush_object_name();
    bool flush_string(std::string &s);
};

}  // namespace json
}  // namespace vtss

#endif  // __VTSS_BASICS_JSON_STREAM_PARSER_HXX__
