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

#include "vtss/basics/assert.hxx"
#include "vtss/basics/string-utils.hxx"
#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/http-message-stream-parser.hxx"

namespace vtss {
namespace http {

static const uint8_t ptr_size = 2;

static void write_ptr(size_t p, char *pos_) {
    uint8_t byte_val;
    uint8_t *pos = reinterpret_cast<uint8_t *>(pos_);
    pos += ptr_size - 1;

    for (int i = 0; i < ptr_size; ++i) {
        byte_val = p & 0xff;
        *pos-- = byte_val;
        p >>= 8;
    }
}

static size_t read_ptr(const char *pos_) {
    size_t val = 0;
    const uint8_t *pos = reinterpret_cast<const uint8_t *>(pos_);

    for (int i = 0; i < ptr_size; ++i) {
        val <<= 8;
        val |= *pos++;
    }

    return val;
}

static size_t node_size() {
    // ID, NEXT, PARENT, NESTED-ID,  HEAD
    // ID, NEXT, PARENT, CONTENT-ID, LENGTH
    return 2 + (3 * ptr_size);
}

static char *buf_ptr(Buf *b, size_t offset) {
    char *p = b->begin() + offset;
    VTSS_ASSERT(p >= b->begin() && p < b->end());
    return p;
}

static const char *buf_ptr(const Buf *b, size_t offset) {
    const char *p = b->begin() + offset;
    VTSS_ASSERT(p >= b->begin() && p < b->end());
    return p;
}

// ID-access primitives
static size_t id_size() { return 1; }
static size_t id_offset() { return 0; }
static void node_id_write(Buf *b, size_t node_offset, uint8_t val) {
    *buf_ptr(b, node_offset + id_offset()) = val;
}
static uint8_t node_id_read(const Buf *b, size_t node_offset) {
    return *buf_ptr(b, node_offset + id_offset());
}

// NEXT-access primitives
static size_t next_size() { return ptr_size; }
static size_t next_offset() { return id_size() + id_offset(); }
static void node_next_write(Buf *b, size_t node_offset, size_t val) {
    write_ptr(val, buf_ptr(b, node_offset + next_offset()));
}
static size_t node_next_read(const Buf *b, size_t node_offset) {
    return read_ptr(buf_ptr(b, node_offset + next_offset()));
}

// PARENT-access primitives
static size_t parent_size() { return ptr_size; }
static size_t parent_offset() { return next_size() + next_offset(); }
static void node_parent_write(Buf *b, size_t node_offset, size_t val) {
    write_ptr(val, buf_ptr(b, node_offset + parent_offset()));
}
static size_t node_parent_read(const Buf *b, size_t node_offset) {
    return read_ptr(buf_ptr(b, node_offset + parent_offset()));
}

// HDR-TYPE-access primitives
static size_t hdr_type_size() { return 1; }
static size_t hdr_type_offset() { return parent_size() + parent_offset(); }
static void node_hdr_type_write(Buf *b, size_t node_offset, uint8_t val) {
    *buf_ptr(b, node_offset + hdr_type_offset()) = val;
}
static uint8_t node_hdr_type_read(const Buf *b, size_t node_offset) {
    return *buf_ptr(b, node_offset + hdr_type_offset());
}

// HEAD-access primitives (placed on same location as LENGTH)
// static size_t head_size() { return ptr_size; }
static size_t head_offset() { return hdr_type_size() + hdr_type_offset(); }
static void node_head_write(Buf *b, size_t node_offset, size_t val) {
    write_ptr(val, buf_ptr(b, node_offset + head_offset()));
}
static size_t node_head_read(const Buf *b, size_t node_offset) {
    return read_ptr(buf_ptr(b, node_offset + head_offset()));
}

// LENGTH-access primitives (placed on same location as HEAD)
static size_t length_size() { return ptr_size; }
static size_t length_offset() { return hdr_type_size() + hdr_type_offset(); }
static void node_length_write(Buf *b, size_t node_offset, size_t val) {
    write_ptr(val, buf_ptr(b, node_offset + length_offset()));
}
static size_t node_length_read(const Buf *b, size_t node_offset) {
    return read_ptr(buf_ptr(b, node_offset + length_offset()));
}

// data-ptr
static const char *node_data(const Buf *b, size_t node_offset) {
    return buf_ptr(b, node_offset + length_offset() + length_size());
}

static bool as(Buf *b, size_t s, StreamTree::HdrType t) {
    if (node_hdr_type_read(b, s) == t) return true;

    if (node_hdr_type_read(b, s) == StreamTree::HDR_NULL) {
        node_hdr_type_write(b, s, t);
        return true;
    }

    return false;
}

static bool as(const Buf *b, size_t s, StreamTree::HdrType t) {
    if (node_hdr_type_read(b, s) == t) return true;
    if (node_hdr_type_read(b, s) == StreamTree::HDR_NULL) return true;
    return false;
}



StreamTree::ConstNode StreamTree::ConstNode::section_find(
        uint8_t section_id) const {
    if (!as(const_cast<const Buf *>(parent_.buf_), this_node_,
            StreamTree::HDR_NODE)) {
        // TODO(anielsen) trace
        return ConstNode(parent_, 0);  // return a null node
    }

    size_t pos = find(section_id);

    if (pos != 0 && node_id_read(parent_.buf_, pos) == section_id) {
        return ConstNode(parent_, pos);
    }

    VTSS_BASICS_TRACE(DEBUG) << "Section ID: " << section_id << " not found";
    return ConstNode(parent_, 0);  // return a null node
}

size_t StreamTree::ConstNode::find(uint8_t id) const {
    // returns either a node with the excat ID or a node with a ID less
    // than "id"
    VTSS_ASSERT(node_hdr_type_read(parent_.buf_, this_node_) ==
            StreamTree::HDR_NODE);
    size_t i = node_head_read(parent_.buf_, this_node_);
    size_t prev = 0;

    while (i != 0 && node_id_read(parent_.buf_, i) <= id) {
        prev = i;
        i = node_next_read(parent_.buf_, i);
    }

    return prev;
}

StreamTree::Node StreamTree::Node::section(uint8_t section_id) {
    // find or add a section

    if (!as(parent().buf_, this_node_, StreamTree::HDR_NODE)) {
        // TODO(anielsen) trace
        return Node(parent(), 0);  // return a null node
    }

    size_t pos = find(section_id);

    if (pos != 0 && node_id_read(parent().buf_, pos) == section_id) {
        VTSS_BASICS_TRACE(RACKET) << "Found existing section with ID: " <<
            section_id;
        return Node(parent(), pos);  // node found
    } else {
        // new node created
        VTSS_BASICS_TRACE(NOISE) << "Creating new section with ID " <<
            section_id;
        return Node(parent(), section_id, this_node_, pos, HDR_NULL);
    }
}

StreamTree::Node StreamTree::Node::section_add(uint8_t section_id) {
    // add a new section

    if (!as(parent().buf_, this_node_, StreamTree::HDR_NODE)) {
        // TODO(anielsen) trace
        return Node(parent(), 0);  // return a null node
    }

    size_t pos = find(section_id);

    if (pos != 0 && node_id_read(parent().buf_, pos) == section_id) {
        // Found a duplicate!
        // TODO(anielsen) trace
        return Node(parent(), 0);  // return a null node
    }

    // return a new node which may be used as a content or a nested node
    return Node(parent(), section_id, this_node_, pos, StreamTree::HDR_NULL);
}


size_t StreamTree::next_content_node(size_t orginal, size_t i,
                                     size_t comming_from) const {
    if (i == 0) return 0;

    uint8_t type = node_hdr_type_read(buf_, i);

    // Check if we found a new content node
    if (type == HDR_LEAF && orginal != i) return i;

    if (type == HDR_NODE && comming_from == 0) {  // try go deeper
        size_t res = next_content_node(orginal, node_head_read(buf_, i), 0);
        if (res) return res;
    }

    if (type == HDR_NODE && comming_from != 0) {
        // go next starting from comming_from, or go up
        size_t next = next_content_node(orginal,
                node_next_read(buf_, comming_from), 0);
        if (next) return next;
    }

    // For all other types: has next? -> go next
    size_t next = next_content_node(orginal, node_next_read(buf_, i), 0);
    if (next) return next;

    // go up
    return next_content_node(orginal, node_parent_read(buf_, i), i);
}

str  StreamTree::output_write_chunk(size_t &byte_cnt, size_t &node) const {
    if (!ok()) return str();
    if (!node) {
        // find the first content node
        node = root_pos();
        node = next_content_node(node, node, 0);
    }

    if (node_hdr_type_read(buf_, node) != HDR_LEAF) return str();
    size_t length = node_length_read(buf_, node);

    while (byte_cnt >= length) {  // zero length content may occure
        // Node has been consumed completly, find the next content node
        node = next_content_node(node, node, 0);

        // We got a new node, reset perv. byte cnt
        byte_cnt = 0;

        // Check if more content nodes exists
        if (node_hdr_type_read(buf_, node) != HDR_LEAF) return str();

        // latch the length of currnet content node
        length = node_length_read(buf_, node);
    }

    str res(node_data(buf_, node) + byte_cnt, node_data(buf_, node) + length);
    VTSS_BASICS_TRACE(NOISE) << "Write chunk of " << res.size() << "\n" << res;
    return res;
}

bool StreamTree::has_free_or_overflow(size_t s) {
    if (overflow_) return false;
    if (!buf_ || !pos_) return flag_overflow();
    if (pos_ + s > buf_->end()) return flag_overflow();
    return true;
}

bool StreamTree::leaf_push_grow(size_t node, const char val) {
    if (node == 0)                 return false;
    if (active_node_ != node)      return false;
    if (!as(buf_, node, HDR_LEAF)) return false;
    if (!has_free_or_overflow(1))  return false;
    *pos_++ = val;

    size_t _s = node_length_read(buf_, node);
    node_length_write(buf_, node, _s + 1);
    return true;
}

bool StreamTree::leaf_clear(size_t node) {
    if (node == 0)                 return false;
    if (active_node_ != node)      return false;
    if (!as(buf_, node, HDR_LEAF)) return false;

    size_t _s = node_length_read(buf_, node);
    pos_-=_s;
    node_length_write(buf_, node, 0);
    return true;
}

bool StreamTree::leaf_push_grow(size_t node, str data) {
    if (node == 0)                          return false;
    if (active_node_ != node)               return false;
    if (!as(buf_, node, HDR_LEAF))          return false;
    if (!has_free_or_overflow(data.size())) return false;
    copy(data.begin(), data.end(), pos_);
    pos_ += data.size();

    size_t _s = node_length_read(buf_, node);
    node_length_write(buf_, node, _s + data.size());
    return true;
}

size_t StreamTree::leaf_size(size_t node) const {
    if (node == 0) return 0;
    if (node_hdr_type_read(buf_, node) != HDR_LEAF) return 0;
    return node_length_read(buf_, node);
}

str StreamTree::leaf_content(size_t node) const {
    if (node == 0) return str();
    if (node_hdr_type_read(buf_, node) != HDR_LEAF) return str();
    size_t s = node_length_read(buf_, node);
    return str(node_data(buf_, node), node_data(buf_, node) + s);
}

bool StreamTree::push_node(uint8_t id, size_t parent, size_t prev,
                           HdrType type) {
    if (!has_free_or_overflow(node_size())) return false;
    size_t this_node = pos_ - buf_->begin();

    if (!parent) {  // root node
        node_id_write(buf_, this_node, 0);
        node_next_write(buf_, this_node, 0);
        node_parent_write(buf_, this_node, 0);
        node_hdr_type_write(buf_, this_node, HDR_NULL);
        node_head_write(buf_, this_node, 0);

    } else {
        node_id_write(buf_, this_node, id);

        if (prev) {
            // link this node in after an existing node
            size_t prev_next = node_next_read(buf_, prev);

            // hook in this node in "prev"
            node_next_write(buf_, prev, this_node);

            // add the list tail
            node_next_write(buf_, this_node, prev_next);

        } else {
            // this node must become the new head of a list which may or may
            // not be empty
            VTSS_BASICS_TRACE(NOISE) << "Creating new HEAD with ID: " <<
                node_id_read(buf_, this_node);

            size_t old_head = node_head_read(buf_, parent);

            if (old_head) {
                // non-empty list
                node_next_write(buf_, this_node, old_head);
            } else {
                // empty list
                node_next_write(buf_, this_node, 0);
            }
            node_head_write(buf_, parent, this_node);
        }

        node_parent_write(buf_, this_node, parent);
        node_hdr_type_write(buf_, this_node, type);
        node_head_write(buf_, this_node, 0);
    }

    active_node_ = this_node;
    pos_ += node_size();
    return true;
}

void StreamTree::reset() {
    active_node_ = 0;
    overflow_ = false;

    if (buf_) {
        pos_ = buf_->begin();
    } else {
        pos_ = 0;
        overflow_ = true;
    }

    if (pos_) {
        ++pos_;  // push the dummy!
    } else {
        overflow_ = true;
    }

    push_node(0, 0, 0, HDR_NODE);  // push the root node
}

#define CASE(X, Y) case X: o << Y; return o
ostream& operator<<(ostream& o, const RequestType& rhs) {
    switch (rhs) {
        CASE(OPTIONS, "OPTIONS");
        CASE(GET,     "GET");
        CASE(HEAD,    "HEAD");
        CASE(POST,    "POST");
        CASE(PUT,     "PUT");
        CASE(DELETE,  "DELETE");
        CASE(TRACE,   "TRACE");
        CASE(CONNECT, "CONNECT");
        default:
            o << "<invalid valid: " << static_cast<uint64_t>(rhs) << ">";
            return o;
    }
}

ostream& operator<<(ostream& o, const HttpVersion& rhs) {
    switch (rhs) {
        CASE(v_1_0, "HTTP/1.0");
        CASE(v_1_1, "HTTP/1.1");
        default:
            o << "<invalid valid: " << static_cast<uint64_t>(rhs) << ">";
            return o;
    }
}

ostream& operator<<(ostream& o, const HttpCode& rhs) {
    switch (rhs) {
        CASE(c200_OK,                         "200 OK");
        CASE(c201_Created,                    "201 Created");
        CASE(c202_Accepted,                   "202 Accepted");
        CASE(c203_Non_Authoritative_Information,
             "203 Non-Authoritative Information");
        CASE(c204_No_Content,                 "204 No Content");
        CASE(c205_Reset_Content,              "205 Reset Content");
        CASE(c206_Partial_Content,            "206 Partial Content");
        CASE(c300_Multiple_Choices,           "300 Multiple Choices");
        CASE(c301_Moved_Permanently,          "301 Moved Permanently");
        CASE(c302_Found,                      "302 Found");
        CASE(c303_See_Other,                  "303 See Other");
        CASE(c304_Not_Modified,               "304 Not Modified");
        CASE(c305_Use_Proxy,                  "305 Use Proxy");
        CASE(c307_Temporary_Redirect,         "307 Temporary Redirect");
        CASE(c400_Bad_Request,                "400 Bad Request");
        CASE(c401_Unauthorized,               "401 Unauthorized");
        CASE(c402_Payment_Required,           "402 Payment Required");
        CASE(c403_Forbidden,                  "403 Forbidden");
        CASE(c404_Not_Found,                  "404 Not Found");
        CASE(c405_Method_Not_Allowed,         "405 Method Not Allowed");
        CASE(c406_Not_Acceptable,             "406 Not Acceptable");
        CASE(c407_Proxy_Authentication_Required,
             "407 Proxy Authentication Required");
        CASE(c408_Request_Timeout,            "408 Request Timeout");
        CASE(c409_Conflict,                   "409 Conflict");
        CASE(c410_Gone,                       "410 Gone");
        CASE(c411_Length_Required,            "411 Length Required");
        CASE(c412_Precondition_Failed,        "412 Precondition Failed");
        CASE(c413_Request_Entity_Too_Large,   "413 Request Entity Too Large");
        CASE(c414_Request_URI_Too_Long,       "414 Request-URI Too Long");
        CASE(c415_Unsupported_Media_Type,     "415 Unsupported Media Type");
        CASE(c416_Requested_Range_Not_Satisfiable,
             "416 Requested Range Not Satisfiable");
        CASE(c417_Expectation_Failed,         "417 Expectation Failed");
        CASE(c500_Internal_Server_Error,      "500 Internal Server Error");
        CASE(c501_Not_Implemented,            "501 Not Implemented");
        CASE(c502_Bad_Gateway,                "502 Bad Gateway");
        CASE(c503_Service_Unavailable,        "503 Service Unavailable");
        CASE(c504_Gateway_Timeout,            "504 Gateway Timeout");
        CASE(c505_HTTP_Version_Not_Supported, "505 HTTP Version Not Supported");
        default:
            o << static_cast<uint32_t>(rhs) << " unknown code";
            return o;
    }
}
#undef CASE

bool MessageParser::input_header_push(char c) {
    if (!buf_.push(c)) return false;
    header_end = buf_.end();
    message_end = buf_.end();
    message_begin= buf_.end();
    return true;
}

bool MessageParser::input_message_push(char c) {
    if (!buf_.push(c)) return false;
    message_end = buf_.end();
    return true;
}

void MessageParser::flag_error(int line_number) {
    VTSS_BASICS_TRACE(DEBUG) << "Error raised at: " << line_number;
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
    has_expected_message_size_ = false;
    buf_.clear();
    header_line_start_ = 0;

    header_begin = buf_.end();
    header_end = buf_.end();
    message_end = buf_.end();
    message_begin= buf_.end();
    header_first_begin = buf_.end();
    header_first_end = buf_.end();
}

bool MessageParser::push(char c) {
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
            try_expect(c, '\r', HEADER_FIRST_LINE_GOT_CARRIAGE_RETURN);
            return input_header_push(c);

        case HEADER_FIRST_LINE_GOT_CARRIAGE_RETURN:
            if (!do_expect(c, '\n', HEADER_CONTENT)) {
                return false;  // Syntax error in header
            }

            if (!input_header_push(c)) {
                return false;  // Buffer overflow caused by delimitor
            }

            if (!process_first_header_line(header_begin, header_end)) {
                return false;
            }

            header_line_start_ = header_end;  // Start new header line
            return true;

        case HEADER_CONTENT:
            try_expect(c, '\r', HEADER_GOT_FIRST_CARRIAGE_RETURN);
            return input_header_push(c);

        case HEADER_GOT_FIRST_CARRIAGE_RETURN:
            if (!do_expect(c, '\n', HEADER_GOT_FIRST_LINE_FEED)) {
                return false;  // Syntax error in header
            }

            if (!input_header_push(c)) {
                return false;  // Buffer overflow
            }

            if (!process_header_line_(header_line_start_, header_end)) {
                return false;
            }

            header_line_start_ = header_end;  // Start new header line
            return true;

        case HEADER_GOT_FIRST_LINE_FEED:
            if (try_expect(c, '\r', HEADER_GOT_SECOND_CARRIAGE_RETURN)) {
                return input_header_push(c);
            }

            // Start a new header line! header_line_start has been set
            state(HEADER_CONTENT);
            return input_header_push(c);

        case HEADER_GOT_SECOND_CARRIAGE_RETURN:
            if (!do_expect(c, '\n', MESSAGE)) {
                return false;  // syntax error in header
            }

            if (!input_header_push(c)) {
                return false;  // Buffer overflow
            }

            // Check that we got all mandotory fields (size).
            return header_complete();

        case MESSAGE:
            if (!input_message_push(c)) {
                return false;  // Overflow
            }

            // Check if we got the complete message
            if (msg().size() >= expected_message_size_) {
                state(MESSAGE_COMPLETE);
            }
            return true;

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
    header_first_end   = header_end;
    header_first_begin = header_begin;

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

    VTSS_BASICS_TRACE(NOISE) << "Processing header line: " << str(b, e);

    if (!split(str(b, e), ':', name, value)) {
        flag_error(__LINE__);
        return false;
    }
    value = trim(value);
    return process_header_line_(name, value);
}

bool MessageParser::process_header_line_(const str& name, const str& val) {
    if (name == str("Content-Length")) {
        return process_header_line_content_length(val);
    }

    return process_header_line(name, val);
}

bool MessageParser::process_header_line_content_length(const str& val) {
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

void MessageParser::state(RequestParser::State s) {
    state_ = s;
}

static bool parse_http_version(const str& in, HttpVersion& out) {
    if (in == str("HTTP/1.1")) {
        out = v_1_1;
        return true;

    } else if (in == str("HTTP/1.0")) {
        out = v_1_0;
        return true;
    }

    return false;
}

bool ResponseParser::process_first_header_line(const str& s_version,
                                               const str& s_code_number,
                                               const str& s_code_string) {
    if (!parse_http_version(s_version, http_version_)) {
        VTSS_BASICS_TRACE(DEBUG) << "Invalid http version: " << s_version;
        flag_error(__LINE__);
        return false;
    }

    parser::Int<size_t, 10> int_parser;
    const char *b = s_code_number.begin(), *e = s_code_number.end();
    if (!int_parser(b, e)) {
        VTSS_BASICS_TRACE(DEBUG) << "Failed to parse code as a int: "
            << s_code_number;
        flag_error(__LINE__);
        return false;
    }

    status_code_ = int_parser.get();
    status_code_string_ = s_code_string;
    return true;
}

bool RequestParser::process_first_header_line(const str& s_method,
                                              const str& s_path,
                                              const str& s_http_version) {
    if (s_method == str("OPTIONS")) {
        request_type_ = OPTIONS;
    } else if (s_method == str("GET")) {
        request_type_ = GET;
    } else if (s_method == str("HEAD")) {
        request_type_ = HEAD;
    } else if (s_method == str("POST")) {
        request_type_ = POST;
    } else if (s_method == str("PUT")) {
        request_type_ = PUT;
    } else if (s_method == str("DELETE")) {
        request_type_ = DELETE;
    } else if (s_method == str("TRACE")) {
        request_type_ = TRACE;
    } else if (s_method == str("CONNECT")) {
        request_type_ = CONNECT;
    } else {
        VTSS_BASICS_TRACE(DEBUG) << "Invalid method: " << s_method;
        flag_error(__LINE__);
        return false;
    }

    path_ = s_path;

    if (!parse_http_version(s_http_version, http_version_)) {
        VTSS_BASICS_TRACE(DEBUG) << "Invalid http version: " << s_http_version;
        flag_error(__LINE__);
        return false;
    }

    return true;
}

void MessageWriter::reset() {
    stream_tree.reset();
    stream_tree.section(SECTION_HEADER_SEP).content_node().push(str("\r\n"));
    output_node_ = 0;
    output_byte_cnt_ = 0;
}

bool MessageWriter::add_content_length() {
    size_t s = stream_tree.section(SECTION_MSG).content_node().size();
    auto o = stream_tree.section(SECTION_HEADER_AUTO).content_node() .stream();
    o << "Content-Length: " << s << "\r\n";
    return stream_tree.ok();
}

void MessageWriter::output_reset() const {
    output_node_ = 0;
    output_byte_cnt_ = 0;
}

str MessageWriter::output_write_chunk() const {
    return stream_tree.output_write_chunk(output_byte_cnt_, output_node_);
}

void MessageWriter::output_write_commit(size_t s) const {
    return stream_tree.output_write_commit(output_byte_cnt_, s);
}

bool MessageWriter::output_write_done() const {
    return stream_tree.output_write_done(output_byte_cnt_, output_node_);
}

bool RequestWriter::first_line(HttpVersion v, RequestType t, str path) {
    auto o = stream_tree.section(SECTION_FIRST_LINE).content_node().stream();
    o << t << " " << path << " " << v << "\r\n";
    return stream_tree.ok();
}

bool ResponseWriter::first_line(HttpVersion v, HttpCode code) {
    auto o = stream_tree.section(SECTION_FIRST_LINE).content_node().stream();
    o << v << " " << code << "\r\n";
    return stream_tree.ok();
}

ostream& operator<<(ostream& o, const MessageWriter& m) {
    size_t output_node_ = 0;
    size_t output_byte_cnt_ = 0;

    const StreamTree& t = m.stream_tree;

    while (!t.output_write_done(output_byte_cnt_, output_node_)) {
        str _s = t.output_write_chunk(output_byte_cnt_, output_node_);
        o << _s;
        t.output_write_commit(output_byte_cnt_, _s.size());
    }

    return o;
}

}  // namespace http
}  // namespace vtss

