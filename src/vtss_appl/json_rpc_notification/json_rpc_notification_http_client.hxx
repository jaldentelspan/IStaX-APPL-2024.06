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

#ifndef __JSON_RPC_NOTIFICATION_JSON_RPC_NOTIFICATION_HTTP_CLIENT_HXX__
#define __JSON_RPC_NOTIFICATION_JSON_RPC_NOTIFICATION_HTTP_CLIENT_HXX__

#include "json_rpc_notification.hxx"

namespace vtss {
namespace appl {
namespace json_rpc_notification {

struct MessageParser {
    enum State {
        HEADER_CONTENT_FIRST_LINE,
        HEADER_FIRST_LINE_GOT_CARRIAGE_RETURN,

        HEADER_CONTENT,
        HEADER_GOT_FIRST_CARRIAGE_RETURN,
        HEADER_GOT_FIRST_LINE_FEED,
        HEADER_GOT_SECOND_CARRIAGE_RETURN,
        MESSAGE,
        MESSAGE_COMPLETE,
        ERROR
    };

    explicit MessageParser() { reset(); }
    virtual ~MessageParser() {}

    State state() const { return state_; }

    // Reset the internal state of the stream process.
    // This will not release an attached buffer, but it will reset pointerers
    // into the buffer.
    void reset();

    // Process a new chunk of data. It will return a pointer to where the
    // processing stopped. If the returned pointer is equal to end, the complete
    // chunk was processed. If the returned pointer is not equal to end, then we
    // have hit a message boundary, or encoutered an error.
    size_t process(const char *i, const char *end);

    // Check if the attached buffer contains a complete message
    bool complete() const { return state() == MESSAGE_COMPLETE; }

    // Check if an error was found in the input stream
    bool error() const { return state() == ERROR; }

  protected:
    virtual bool process_first_header_line(const str &a, const str &b,
                                           const str &c) = 0;
    virtual bool process_header_line(const str &name, const str &val) = 0;
    virtual size_t input_message_push(const char *b, const char *e) = 0;
    void flag_error(int line_number);  // Flag a parse error

  private:
    // Parse and copy input
    size_t push(const char *b, const char *e);
    void input_header_push(char c) { header_.push_back(c); }
    void state(State s) { state_ = s; }

    bool header_complete();  // returns if the header has been received

    // Utility functions used when parsing
    bool do_expect(char input, char expect, State next);
    bool try_expect(char input, char expect, State next);
    bool process_header_line_(const char *b, const char *e);
    bool process_header_line_(const str &name, const str &val);
    bool process_header_line_content_length(const str &val);
    bool process_first_header_line(const char *begin, const char *end);

    State state_;
    size_t expected_message_size_ = 0;
    size_t body_size_ = 0;
    bool has_expected_message_size_ = 0;

    std::string header_;
    std::string body_;
};

struct ResponseParser : public MessageParser {
    ~ResponseParser() {}

    str status_code_string() const { return status_code_string_; }
    int status_code() const { return status_code_; }

    bool status_code_200() const {
        return status_code_ >= 200 && status_code_ < 300;
    }
    bool status_code_300() const {
        return status_code_ >= 300 && status_code_ < 400;
    }
    bool status_code_400() const {
        return status_code_ >= 400 && status_code_ < 500;
    }
    bool status_code_500() const {
        return status_code_ >= 500 && status_code_ < 600;
    }

  protected:
    bool process_first_header_line(const str &a, const str &b, const str &c) {
        return true;
    }
    bool process_header_line(const str &name, const str &val) { return true; }
    size_t input_message_push(const char *b, const char *e) { return e - b; }

    str status_code_string_;
    int status_code_ = 0;
};

int32_t http_post(const DestConf &conf, const std::string &data);


}  // namespace json_rpc_notification
}  // namespace appl
}  // namespace vtss

#endif  // __JSON_RPC_NOTIFICATION_JSON_RPC_NOTIFICATION_HTTP_CLIENT_HXX__
