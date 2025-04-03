/*

 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __HTTP_MESSAGE_STREAM_PARSER_HXX__
#define __HTTP_MESSAGE_STREAM_PARSER_HXX__

#include "vtss/basics/common.hxx"
#include "vtss/basics/stream.hxx"
#include "vtss/basics/parser_impl.hxx"

namespace vtss {
namespace http {

class DomainName {
};

struct HostAddress {
    enum Type {
        TYPE_IPV4,
        TYPE_IPV6,
        TYPE_DNS,
    } type;

    union Address {
        Ipv4Address ipv4;
        Ipv6Address ipv6;
        DomainName  dns;
    } u;

    HostAddress() = default;

    explicit HostAddress(const Ipv4Address &i) {
        type = TYPE_IPV4;
        u.ipv4 = i;
    }

    explicit HostAddress(const Ipv6Address &i) {
        type = TYPE_IPV6;
        u.ipv6 = i;
    }

    explicit HostAddress(const DomainName &d) {
        type = TYPE_DNS;
        u.dns = d;
    }

    explicit HostAddress(const HostAddress &rhs) {
        type = rhs.type;
        switch (type) {
        case TYPE_IPV4: u.ipv4 = rhs.u.ipv4; break;
        case TYPE_IPV6: u.ipv6 = rhs.u.ipv6; break;
        case TYPE_DNS:  u.dns  = rhs.u.dns;  break;
        }
    }

    HostAddress& operator=(const Ipv4Address &i) {
        type = TYPE_IPV4;
        u.ipv4 = i;
        return *this;
    }

    HostAddress& operator=(const Ipv6Address &i) {
        type = TYPE_IPV6;
        u.ipv6 = i;
        return *this;
    }

    HostAddress& operator=(const DomainName &d) {
        type = TYPE_DNS;
        u.dns = d;
        return *this;
    }
};

enum RequestType {OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT};
enum HttpVersion {v_1_0, v_1_1};
enum HttpCode    {c200_OK                                   = 200,
                  c201_Created                              = 201,
                  c202_Accepted                             = 202,
                  c203_Non_Authoritative_Information        = 203,
                  c204_No_Content                           = 204,
                  c205_Reset_Content                        = 205,
                  c206_Partial_Content                      = 206,
                  c300_Multiple_Choices                     = 300,
                  c301_Moved_Permanently                    = 301,
                  c302_Found                                = 302,
                  c303_See_Other                            = 303,
                  c304_Not_Modified                         = 304,
                  c305_Use_Proxy                            = 305,
                  c307_Temporary_Redirect                   = 307,
                  c400_Bad_Request                          = 400,
                  c401_Unauthorized                         = 401,
                  c402_Payment_Required                     = 402,
                  c403_Forbidden                            = 403,
                  c404_Not_Found                            = 404,
                  c405_Method_Not_Allowed                   = 405,
                  c406_Not_Acceptable                       = 406,
                  c407_Proxy_Authentication_Required        = 407,
                  c408_Request_Timeout                      = 408,
                  c409_Conflict                             = 409,
                  c410_Gone                                 = 410,
                  c411_Length_Required                      = 411,
                  c412_Precondition_Failed                  = 412,
                  c413_Request_Entity_Too_Large             = 413,
                  c414_Request_URI_Too_Long                 = 414,
                  c415_Unsupported_Media_Type               = 415,
                  c416_Requested_Range_Not_Satisfiable      = 416,
                  c417_Expectation_Failed                   = 417,
                  c500_Internal_Server_Error                = 500,
                  c501_Not_Implemented                      = 501,
                  c502_Bad_Gateway                          = 502,
                  c503_Service_Unavailable                  = 503,
                  c504_Gateway_Timeout                      = 504,
                  c505_HTTP_Version_Not_Supported           = 505};

ostream& operator<<(ostream& o, const RequestType& rhs);
ostream& operator<<(ostream& o, const HttpVersion& rhs);
ostream& operator<<(ostream& o, const HttpCode& rhs);

struct StreamTree {
    /* *************************************************************************
     *  ID::             One byte identifaication number (0-255)
     *  HDR-NULL::       N + 1 bytes assigned to 0x00
     *  HDR-NESTED-ID::  One byte assigned to 0x01
     *  HDR-CONTENT-ID:: One byte assigned to 0x02
     *  HDR-NESTED::     {   HDR-NESTED-ID,     // Header type identifier
     *                       HEAD-PTR           // Pointer to first NODE
     *                   }
     *  HDR-CONTENT::    {   HDR-CONTENT-ID,    // Header type identifier
     *                       LENGTH,            // Length of content section
     *                       BYTE-DATA[LENGTH]  // Sequence of LENGTH data bytes
     *                   }
     *  NODE::           {   ID,                // User ID of section
     *                       NEXT-PTR,          // Pointer to next section
     *                       PARENT-PTR,        // Pointer to parent nested node
     *                       [  HDR-NULL    |   // A null header
     *                          HDR-NESTED  |   // A new section sequence
     *                          HDR-CONTENT ]   // Actually content
     *                   }
     * START::           {   DUMMY,             // A dummy byte to avoid valid
     *                                          // content at index 0
     *                       NODE               // The first NODE
     *                   }
     ************************************************************************ */
    enum HdrType { HDR_NULL = 0, HDR_NODE = 1, HDR_LEAF = 2 };

    struct CommonNode {
        HdrType type() const;
        bool    ok()   const { return this_node_ != 0 && parent_.ok(); }

      protected:
        CommonNode(StreamTree &p_obj, size_t this_node) :
                parent_(p_obj), this_node_(this_node) { }
        CommonNode(StreamTree &p_obj, uint8_t id, size_t parent, size_t prev,
                   HdrType type) : parent_(p_obj), this_node_(0) {
            if (parent_.push_node(id, parent, prev, type))
                this_node_ = parent_.active_node_;
        }

        StreamTree& parent_;  // Reference to the object holding buffer
        size_t this_node_;  // Pointer to the ID byte in the NODE
    };

    struct ConstCommon {
        friend struct StreamTree;
        bool    ok()   const { return this_node_ != 0 && parent_.ok(); }

      protected:
        ConstCommon(const StreamTree &p, size_t t) : parent_(p),
                this_node_(t) { }

        const StreamTree& parent_;  // Reference to the object holding buffer
        size_t this_node_;  // Pointer to the ID byte in the NODE
    };

    struct ConstLeaf : public ConstCommon {
        friend struct StreamTree;
        const str content() const { return parent_.leaf_content(this_node_); }
        size_t size() const { return parent_.leaf_size(this_node_); }

      protected:
        ConstLeaf(const StreamTree &p, size_t t) : ConstCommon(p, t) { }
    };

    struct Leaf : public ConstLeaf {
        friend struct StreamTree;
        struct ContentStream : public ostreamBuf {
            friend struct Leaf;
            bool ok() const { return this_node_ != 0 && parent_.ok(); }
            bool push(char val) {
                return parent_.leaf_push_grow(this_node_, val);
            }
            size_t write(const char *b, const char *e) {
                str s(b, e);
                if (parent_.leaf_push_grow(this_node_, s)) {
                    return s.size();
                } else {
                    return 0;
                }
            }
            const char * begin() const { return 0; }
            const char * end() const { return 0; }
            void clear() {
                if (!parent_.leaf_clear(this_node_)) {
                    VTSS_ASSERT(0);
                }
            }
            ~ContentStream() { }

          private:
            ContentStream(StreamTree &p, size_t n) : parent_(p), this_node_(n) {
            }

            StreamTree& parent_;
            size_t      this_node_;
        };

        bool push(char c) {
            return parent().leaf_push_grow(this_node_, c);
        }

        bool push(const str& s) {
            return parent().leaf_push_grow(this_node_, s);
        }

        bool push(const char *b, ssize_t size) {
            return parent().leaf_push_grow(this_node_, b, size);
        }

        ContentStream stream() { return ContentStream(parent(), this_node_); }

      private:
        StreamTree &parent() { return const_cast<StreamTree &>(parent_); }

        // Existing leaf
        explicit Leaf(StreamTree &p, size_t t) : ConstLeaf(p, t) { }

        // Create a new leaf
        Leaf(StreamTree &p, uint8_t id, size_t _parent, size_t prev,
             HdrType t) : ConstLeaf(p, 0) {
            if (parent().push_node(id, _parent, prev, t))
                this_node_ = parent_.active_node_;
        }
    };

    struct ConstNode : public ConstCommon {
        friend struct StreamTree;

        const ConstLeaf content_node() const {
            return ConstLeaf(parent_, this_node_);
        }

        // find an existing section
        ConstNode section_find(uint8_t section_id) const;

      protected:
        size_t find(uint8_t id) const;

        // Existing node
        ConstNode(const StreamTree &p, size_t t) : ConstCommon(p, t) { }
    };

    struct Node : public ConstNode {
        friend struct StreamTree;
        Leaf content_node() { return Leaf(parent(), this_node_); }
        Node section(uint8_t section_id);  // find or add a section
        Node section_add(uint8_t section_id);  // add a new section

      private:
        StreamTree &parent() { return const_cast<StreamTree &>(parent_); }

        // Existing node
        Node(StreamTree &p, size_t t) : ConstNode(p, t) { }

        // Create a new node
        Node(StreamTree &p, uint8_t id, size_t _parent, size_t prev,
             HdrType t) : ConstNode(p, 0) {
            if (parent().push_node(id, _parent, prev, t))
                this_node_ = parent_.active_node_;
        }
    };

    explicit StreamTree(Buf *b) : buf_(b) { reset(); }
    explicit StreamTree(StreamTree const&) = delete;
    StreamTree& operator=(StreamTree const&) = delete;

    // Discard all content and overflow status
    void reset();

    // Access members of the root node
    Node section(uint8_t id) { return root().section(id); }
    Node section_add(uint8_t id) { return root().section_add(id); }
    const ConstNode section_find(uint8_t i) const {
        return root().section_find(i);
    }

    // Check for overflows
    bool ok() const { return !overflow_; }

    size_t next_content_node(size_t orginal, size_t i,
                             size_t comming_from) const;

    // Returns a reference to the first chunk of non-committed data
    str  output_write_chunk(size_t &byte_cnt, size_t &node) const;

    // Informs how much of the returned chunk was written
    void output_write_commit(size_t &byte_cnt, size_t s) const {
        byte_cnt += s;
    }

    // Returns true if all data has been committed.
    bool output_write_done(size_t &byte_cnt, size_t &node) const {
        return output_write_chunk(byte_cnt, node).size() == 0;
    }

  private:
    size_t root_pos() const { return 1; }  // hard-coded possition of root node

    // Get the root node object
    Node root() { return Node(*this, root_pos()); }
    const ConstNode root() const { return ConstNode(*this, root_pos()); }

    bool flag_overflow() {
        overflow_ = true;
        return false;
    }

    bool has_free_or_overflow(size_t s);
    bool push_node(uint8_t id, size_t parent, size_t prev, HdrType type);

    bool leaf_push_grow(size_t node, const char val);
    bool leaf_push_grow(size_t node, str data);
    bool leaf_push_grow(size_t node, const char *data, size_t length) {
        return leaf_push_grow(node, str(data, data+length));
    }
    bool leaf_clear(size_t node);

    size_t leaf_size(size_t node) const;
    str leaf_content(size_t node) const;


    Buf   *buf_;
    char  *pos_;
    bool   overflow_;
    size_t active_node_;
};

inline ostream& operator<<(ostream& o, const StreamTree &s) {
    size_t output_node_ = 0;
    size_t output_byte_cnt_ = 0;

    while (!s.output_write_done(output_byte_cnt_, output_node_)) {
        str _s = s.output_write_chunk(output_byte_cnt_, output_node_);
        o << _s;
        s.output_write_commit(output_byte_cnt_, _s.size());
    }

    return o;
}

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

    explicit MessageParser(Buf *b) : buf_(b) { reset(); }
    virtual ~MessageParser() { }

    State state() const { return state_; }

    // Reset the internal state of the stream process.
    // This will not release an attached buffer, but it will reset pointerers
    // into the buffer.
    void reset();

    // Process a new chunk of data. It will return a pointer to where the
    // processing stopped. If the returned pointer is equal to end, the complete
    // chunk was processed. If the returned pointer is not equal to end, then we
    // have hit a message boundary, or encoutered an error.
    template<class I> I *process(I *i, I *end) {
        for (; i != end; ++i) {
            if (complete()) return i;
            if (error())    return i;
            if (!push(*i))  return i;
        }
        return i;
    }

    // Check if the attached buffer contains a complete message
    bool complete() const { return state() == MESSAGE_COMPLETE; }

    // Check if an error was found in the input stream
    bool error() const { return state() == ERROR; }

    // Get message details
    const str msg() const { return str(message_begin, message_end); }
    const str header() const { return str(header_begin, header_end); }
    const str header_main() const { return str(header_first_end, header_end); }
    const str header_first_line() const {
        return str(header_first_begin, header_first_end);
    }

  protected:
    virtual bool process_first_header_line(const str& a, const str& b,
                                           const str& c) = 0;
    virtual bool process_header_line(const str& name, const str& val) = 0;

    void flag_error(int line_number);  // Flag a parse error

  private:
    // Parse and copy input
    bool push(char c);
    bool input_header_push(char c);
    bool input_message_push(char c);

    void state(State s);     // update state
    bool header_complete();  // returns if the header has been received

    // Utility functions used when parsing
    bool do_expect(char input, char expect, State next);
    bool try_expect(char input, char expect, State next);
    bool process_header_line_(const char *b, const char *e);
    bool process_header_line_(const str& name, const str& val);
    bool process_header_line_content_length(const str& val);
    bool process_first_header_line(const char *begin, const char *end);

    State  state_;
    BufPtrStream buf_;

    size_t expected_message_size_;
    bool   has_expected_message_size_;

    const char *header_line_start_;
    const char *header_begin,       *header_end,
               *header_first_begin, *header_first_end,
               *message_begin,      *message_end;
};

struct ResponseParser : public MessageParser {
    explicit ResponseParser(Buf *b) : MessageParser(b) { }
    virtual ~ResponseParser() { }

    HttpVersion http_version() const { return http_version_; }
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
    bool process_first_header_line(const str& a, const str& b, const str& c);
    bool process_header_line(const str& name, const str& val) { return true; }

    HttpVersion http_version_;
    str status_code_string_;
    int status_code_ = 0;
};

struct RequestParser : public MessageParser {
    explicit RequestParser(Buf *b) : MessageParser(b) { }
    virtual ~RequestParser() { }

    str path() const { return path_; }
    HttpVersion http_version() const { return http_version_; }
    RequestType request_type() const { return request_type_; }

  protected:
    bool process_first_header_line(const str& a, const str& b, const str& c);
    bool process_header_line(const str& name, const str& val) { return true; }

    str path_;
    RequestType request_type_;
    HttpVersion http_version_;
};

struct MessageWriter {
    friend ostream& operator<<(ostream&, const MessageWriter&);

    enum Sections {SECTION_FIRST_LINE  = 0,
                   SECTION_HEADER_USER = 1,
                   SECTION_HEADER_AUTO = 2,
                   SECTION_HEADER_SEP  = 3,
                   SECTION_MSG         = 4};

    explicit MessageWriter(Buf *b) : buf_(b), stream_tree(b) { reset(); }
    virtual ~MessageWriter() { }

    void reset();

    StreamTree::Leaf::ContentStream message_stream() {
        return stream_tree.section(SECTION_MSG).content_node().stream();
    }

    template<typename T> bool header(str name, const T& t) {
        auto o = stream_tree.section(SECTION_HEADER_USER).content_node()
            .stream();
        o << name << ": " << t << "\r\n";
        return stream_tree.ok();
    }

    template<typename T> bool header(const char *name, const T& t) {
        return header(str(name), t);
    }

    bool add_content_length();

    // Non-blocking friendly output methid
    void output_reset() const;
    str  output_write_chunk() const;
    bool output_write_done() const;
    void output_write_commit(size_t s) const;

  protected:
    Buf *buf_;
    StreamTree stream_tree;

    mutable size_t output_node_;
    mutable size_t output_byte_cnt_;
};

ostream& operator<<(ostream& o, const MessageWriter& m);

struct RequestWriter : MessageWriter {
    explicit RequestWriter(Buf *b) : MessageWriter(b) { }
    virtual ~RequestWriter() { }
    bool first_line(HttpVersion v, RequestType t, str path);
};

struct ResponseWriter : MessageWriter {
    explicit ResponseWriter(Buf *b) : MessageWriter(b) { }
    virtual ~ResponseWriter() { }

    bool first_line(HttpVersion v, HttpCode code);
};


}  // namespace http
}  // namespace vtss

#endif  // __HTTP_MESSAGE_STREAM_PARSER_HXX__

