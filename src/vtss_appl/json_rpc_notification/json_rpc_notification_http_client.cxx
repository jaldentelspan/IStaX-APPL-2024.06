/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <string>
#include "json_rpc_trace.h"
#include "vtss/basics/trace.hxx"
#include "vtss/basics/stream.hxx"
#include "json_rpc_notification_http_client.hxx"
#include "vtss/basics/parser_impl.hxx"
#include "vtss/basics/string-utils.hxx"

#include "vtss_os_wrapper_network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TRACE(X) VTSS_TRACE(VTSS_TRACE_JSON_RPC_GRP_NOTI_ASYNC, X)

namespace vtss {
namespace appl {
namespace json_rpc_notification {

size_t MessageParser::process(const char *i, const char *end) {
    size_t total = 0;
    while (i < end) {
        if (complete()) return total;
        if (error()) return total;

        size_t r = push(i, end);
        if (r == 0) return total;
        i += r, total += r;
    }
    return total;
}

void MessageParser::flag_error(int line_number) {
    TRACE(DEBUG) << "Error raised at: " << line_number;
    state(ERROR);
}

bool MessageParser::do_expect(char input, char expect, State next) {
    if (input != expect) {
        flag_error(__LINE__);
        return false;
    } else {
        state(next);
        return true;
    }
}

bool MessageParser::try_expect(char input, char expect, State next) {
    if (input != expect) {
        return false;
    } else {
        state(next);
        return true;
    }
}

bool MessageParser::header_complete() {
    if (!has_expected_message_size_) {
        flag_error(__LINE__);
        return false;
    }
    return true;
}

// Reset the internal state of the stream process.
// This will not release an attached buffer, but it will reset pointerers into
// the buffer.
void MessageParser::reset() {
    state(HEADER_CONTENT_FIRST_LINE);
    expected_message_size_ = 0;
    body_size_ = 0;
    has_expected_message_size_ = false;
    header_.clear();
    body_.clear();
}

size_t MessageParser::push(const char *b, const char *e) {
    if (b == e) return 0;
    char c = *b;

    /* *********************************************************************
     *  !CR                                                                *
     * +-----+                                                             *
     * |     |                                                             *
     * |   +------------+ CR  +---------------+ NL                         *
     * +-->| FIRST-LINE |---->| CR-FIRST-LINE |----+                       *
     *     +------------+     +---------------+    |                       *
     *                                             |                       *
     *      +--------------------------------------+                       *
     *      |                                                              *
     *      |                !CR                                           *
     *      |   +-------------------------------+                          *
     *      |   |                               |                          *
     *      v   v                               |                          *
     *     +-----+ CR  +----------+ NL  +----------+ CR  +-----------+     *
     * +-->| HDR |---->| CR-FIRST |---->| NL-FIRST |---->| CR-SECOND |-+   *
     * |   +-----+     +----------+     +----------+     +-----------+ |   *
     * |     |                                                         |   *
     * +-----+              while(recv.size < msg.size)                |   *
     *   !CR                          +-----+                          |   *
     *                                |     |                          |   *
     *                                |  +---------+        NL         |   *
     *                                +->| MESSAGE |<------------------+   *
     *                                   +---------+                       *
     *                     +---------+       |                             *
     *                     | MESSAGE |       | recv.size == msg.size       *
     *                     | COMPLET |<------+                             *
     *                     +---------+                                     *
     *                                                                     *
     * NOTE) All states can goto error state if it gets an unexpected      *
     * charecater.                                                         *
     *                                                                     *
     * All state can goto to the HDR if the reset function is called       *
     *                                                                     *
     * ****************************************************************** */
    switch (state()) {
    case HEADER_CONTENT_FIRST_LINE:
        if (try_expect(c, '\r', HEADER_FIRST_LINE_GOT_CARRIAGE_RETURN)) {
        }
        input_header_push(c);
        return 1;

    case HEADER_FIRST_LINE_GOT_CARRIAGE_RETURN:
        if (!do_expect(c, '\n', HEADER_CONTENT)) {
            return 0;  // Syntax error in header
        }

        input_header_push(c);

        if (!process_first_header_line(&*header_.begin(), header_.c_str() + header_.size())) {
            return 0;
        }

        header_.clear();
        return 1;

    case HEADER_CONTENT:
        if (try_expect(c, '\r', HEADER_GOT_FIRST_CARRIAGE_RETURN)) {
            input_header_push(c);
            return 1;
        }

        input_header_push(c);
        return 1;

    case HEADER_GOT_FIRST_CARRIAGE_RETURN:
        if (!do_expect(c, '\n', HEADER_GOT_FIRST_LINE_FEED)) {
            return 0;  // Syntax error in header
        }

        input_header_push(c);

        if (!process_header_line_(&*header_.begin(), header_.c_str() + header_.size())) {
            return 0;
        }

        header_.clear();
        return 1;

    case HEADER_GOT_FIRST_LINE_FEED:
        if (try_expect(c, '\r', HEADER_GOT_SECOND_CARRIAGE_RETURN)) {
            input_header_push(c);
            return 1;
        }

        // Start a new header line! header_line_start has been set
        state(HEADER_CONTENT);
        input_header_push(c);
        return 1;

    case HEADER_GOT_SECOND_CARRIAGE_RETURN:
        if (!do_expect(c, '\n', MESSAGE)) {
            return 0;  // syntax error in header
        }

        input_header_push(c);

        // Check that we got all mandotory fields (size).
        if (!header_complete()) return 0;
        if (expected_message_size_ == 0) state(MESSAGE_COMPLETE);
        return 1;

    case MESSAGE: {
        ssize_t consume_max = 0;
        if (body_size_ + (e - b) > expected_message_size_) {
            consume_max = expected_message_size_ - body_size_;
        } else {
            consume_max = (e - b);
        }

        ssize_t res = input_message_push(b, b + consume_max);
        if (res <= 0) {
            return 0;
        }
        assert(res <= consume_max);
        body_size_ += res;

        // Check if we got the complete message
        assert(body_size_ <= expected_message_size_);
        if (body_size_ >= expected_message_size_) {
            state(MESSAGE_COMPLETE);
        }

        return res;
    }

    case MESSAGE_COMPLETE:
        // Will not start parsing the next message until we got a reset
        return false;

    default:
        return false;
    }
}

bool MessageParser::process_first_header_line(const char *begin,
                                              const char *end) {
    str a, b, c, tmp;
    // header_first_end   = header_end;
    // header_first_begin = header_begin;

    if (!split(str(begin, end), ' ', a, tmp)) {
        flag_error(__LINE__);
        return false;
    }

    if (!split(tmp, ' ', b, c)) {
        flag_error(__LINE__);
        return false;
    }
    c = trim(c);

    return process_first_header_line(a, b, c);
}

bool MessageParser::process_header_line_(const char *b, const char *e) {
    str name;
    str value;

    TRACE(NOISE) << "Processing header line: " << str(b, e);
    if (!split(str(b, e), ':', name, value)) {
        flag_error(__LINE__);
        return false;
    }
    value = trim(value);
    return process_header_line_(name, value);
}

bool MessageParser::process_header_line_(const str &name, const str &val) {
    if (name == str("Content-Length")) {
        return process_header_line_content_length(val);
    }
    return process_header_line(name, val);
}

bool MessageParser::process_header_line_content_length(const str &val) {
    if (has_expected_message_size_) {
        flag_error(__LINE__);
        return false;
    }

    parser::Int<size_t, 10> int_parser;
    const char *b = val.begin(), *e = val.end();
    if (!int_parser(b, e)) {
        flag_error(__LINE__);
        return false;
    }

    expected_message_size_ = int_parser.get();
    has_expected_message_size_ = true;
    return true;
}

struct Fd {
    Fd(int f) : fd(f) {}
    ~Fd() {
        if (fd >= 0) ::close(fd);
    }
    int get() const { return fd; }

    int fd;
};

bool write_(int fd, const std::string &s) {
    const char *p = s.c_str();
    const char *e = s.c_str() + s.size();
    while (p != e) {
        ssize_t res = ::write(fd, p, e - p);
        if (res < 0) {
            TRACE(INFO) << "write: " << fd << " " << strerror(errno);
            return false;
        }

        p += res;
    }

    return true;
}

int32_t http_post(const DestConf &conf, const std::string &data) {
    StringStream hdr;
    vtss::parser::Uri url;

    uint16_t port;
    const char *url_b = &*conf.url.begin();
    const char *url_e = conf.url.c_str() + conf.url.size();

    if (!url(url_b, url_e)) {
        TRACE(INFO) << "Failed to parse URL: " << conf.url.c_str();
        return -1;
    }

    if (url_b != url_e) {
        TRACE(INFO) << "Failed to parse URL: " << conf.url.c_str();
        return -1;
    }


    if (url.path.size())
        hdr << "POST " << url.path << " HTTP/1.1\r\n";
    else
        hdr << "POST / HTTP/1.1\r\n";
    hdr << "Accept: */*\r\n";
    hdr << "User-Agent: VTSS-JSON-RPC-NOTIFICATION\r\n";

    switch (conf.auth_type) {
    case VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_AUTH_TYPE_NONE:
        break;

    case VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_AUTH_TYPE_BASIC: {
        StringStream auth_buf_in;
        size_t len_in, len_out;
        auth_buf_in << conf.user.c_str() << ":" << conf.pass.c_str();
        len_in = auth_buf_in.buf.size();
        len_out = (len_in * 2) + 2;
        FixedBuffer auth_buf_out(len_out);

        mesa_rc rc = vtss_httpd_base64_encode(auth_buf_out.begin(), len_out, auth_buf_in.buf.c_str(), len_in);
        if (rc != VTSS_RC_OK) {
            TRACE(INFO) << "Base64 encoding failed: " << rc;
            return -1;
        }

        hdr << "WWW-Authenticate: Basic realm=\""
            << str(auth_buf_out.begin(), auth_buf_out.begin() + strlen(auth_buf_out.begin()))
            << "\"\r\n";
        break;
    }
    }

    hdr << "Content-Type: application/json-rpc\r\n";
    hdr << "Content-Length: " << data.size() << "\r\n";
    hdr << "\r\n";

    Fd fd(::vtss_socket(AF_INET, SOCK_STREAM, 0));
    if (fd.get() < 0) {
        TRACE(INFO) << "socket: " << fd.get() << " " << strerror(errno);
        return -1;
    }
    TRACE(INFO) << "http_post fd: " << fd.get() << " url: " << conf.url.c_str();
    struct hostent *hp;
    struct sockaddr_in host = {};
    host.sin_family = AF_INET;
    std::string h(url.host.begin(), url.host.end());
    if (!inet_aton(h.c_str(), &host.sin_addr)) {
        TRACE(DEBUG) << "Hostname: " << h.c_str();
        hp = gethostbyname(h.c_str());
        if (hp == NULL) {
            TRACE(INFO) << "gethostbyname: " << fd.get() << " "
                        << strerror(errno);
            return -1;
        } else {
            memcpy(&host.sin_addr, hp->h_addr, hp->h_length);
        }
    }

    if (url.port == -1)
        port = 80;
    else
        port = url.port;

    host.sin_port = htons(port);
    if (connect(fd.get(), (struct sockaddr *)&host, sizeof(host)) != 0) {
        TRACE(INFO) << "connect: " << fd.get() << " " << strerror(errno);
        return -1;
    }

    if (!write_(fd.get(), hdr.buf)) return -1;
    if (!write_(fd.get(), data)) return -1;

    ResponseParser response;

    ssize_t res;
    char buf[128];
    while (true) {
      res = read(fd.get(), buf, sizeof(buf));
        if (res <= 0) {
            TRACE(INFO) << "read: " << fd.get() << " " << strerror(errno);
            return -1;
        }
        res = response.process(buf, buf + res);
        if (res == 0) {
            TRACE(INFO) << "error process msg: " << fd.get();
            return -1;
        }

        if (response.complete()) {
            TRACE(INFO) << "Completed: " << fd.get() << " "
                        << response.status_code();
            break;
        }

        if (response.error()) {
            TRACE(INFO) << "Error: " << fd.get();
            return -1;
        }
    }

    return response.status_code();
}

}  // namespace json_rpc_notification
}  // namespace appl
}  // namespace vtss

