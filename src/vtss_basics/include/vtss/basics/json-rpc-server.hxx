/* *****************************************************************************
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
 **************************************************************************** */

#ifndef __VTSS_JSON_RPC_SERVER_HXX__
#define __VTSS_JSON_RPC_SERVER_HXX__

#include <chrono>

#include <vtss/basics/trace_grps.hxx>
#include <vtss/basics/trace_basics.hxx>

#include "vtss/basics/expose/json/root-node.hxx"
#include "vtss/basics/json-rpc-function.hxx"
#include "vtss/basics/http-message-stream-parser.hxx"

#define TRACE(X) VTSS_BASICS_TRACE(113, VTSS_BASICS_TRACE_GRP_JSON, X)
namespace vtss {
namespace json {

struct BlockRead {
    BlockRead() : fd_(-1) { local_reset(); }

    enum ProcessingState {CLOSED, DO_WRITE, DO_READ, DO_NOTHING};

    bool is_free() { return fd_ < 0; }
    bool fd_match(int fd) { return fd_ == fd; }

    void open(int f);
    virtual ProcessingState process() =0;
    virtual ProcessingState write() =0;
    virtual void reset() =0;
    ProcessingState close();
    ProcessingState read();

    ssize_t write(const char *b, size_t s);

    virtual ~BlockRead() { }

  protected:
    ProcessingState next_state_read() { return DO_READ; }
    ProcessingState next_state_write() { return DO_WRITE; }
    ProcessingState next_state_closed() { return CLOSED; }
    void local_reset();

    int fd_;
    SBuf128 buf_;
    char *bufptr_begin_, *bufptr_end_;
};

struct Envelope {
    // Return a pointer to the first char which was not consumed. If the
    // returned pointer is equal to b, nothing was consumed, and the message is
    // either complete or an error has been raised
    virtual char *input_process(char *b, char *e) = 0;

    // Query status
    virtual bool input_completed() const = 0;
    virtual bool input_error() const = 0;

    // Reset all state, and prepare for next message
    virtual void input_reset() = 0;

    // Get the received input message
    virtual const str input_message() const = 0;

    // Prepare the output message
    virtual void output_prepare(Result::Code c) = 0;

    // Request a chunk of the message to write back to the client
    virtual str  output_get_chunk() = 0;

    // Inform how much which was actually written
    virtual void output_commit(size_t s) = 0;

    // Inform if this message has been complety written
    virtual bool output_completed() const = 0;

    // Some protocols requires that the connection is closed after a message
    // exchange while other allows multiple message exchanges in one session.
    virtual bool force_connection_close() = 0;

    virtual ~Envelope() { }
};

// Encapsulate the JSON message inside a HTTP message
struct HttpEnvelope : public Envelope {
    enum Sections {SECTION_HEADER = 10, SECTION_MESSAGE = 11};

    HttpEnvelope(Buf *in, Buf *out) : in_(in), out_(out), req(in_),
                res(out_) { }

    void reset() { req.reset(); res.reset(); }
    char *input_process(char *b, char *e) { return req.process(b, e); }
    bool input_completed() const          { return req.complete(); }
    bool input_error() const              { return req.error(); }
    void input_reset()                    { req.reset(); }
    const str input_message() const       { return req.msg(); }

    void output_reset()                   { res.reset(); }
    void output_prepare(Result::Code c);
    str  output_get_chunk()               { return res.output_write_chunk(); }
    void output_commit(size_t s)          { res.output_write_commit(s); }
    bool output_completed() const         { return res.output_write_done(); }
    bool force_connection_close()         { return true; }

    http::StreamTree::Leaf::ContentStream output_message() {
        return res.message_stream();
    }

  private:
    Buf *in_, *out_;
    http::RequestParser  req;
    http::ResponseWriter res;
};

Result::Code process_request(str in, ostreamBuf& out,
                             vtss::expose::json::RootNode *r);

template<typename Envelope_type>
struct ServerPeer : public BlockRead {
    ServerPeer() : BlockRead(), root(0), buf_in_(4096), buf_out_(4096),
            envelope_(&buf_in_, &buf_out_) { }

    ProcessingState process() {
        // Buf pointer must point inside the buffer
        assert(bufptr_end_ <= buf_.end());
        assert(bufptr_end_ >= buf_.begin());
        assert(bufptr_begin_ <= buf_.end());
        assert(bufptr_begin_ >= buf_.begin());

        // Can not do anything with an empty buffer
        if (bufptr_begin_ == bufptr_end_) {
            return close();
        }

        // Do the envelope processing
        bufptr_begin_ = envelope_.input_process(bufptr_begin_, bufptr_end_);
        assert(bufptr_begin_ <= buf_.end());
        assert(bufptr_begin_ >= buf_.begin());

        // This is not a ring buffer, so we need to update the pointer manually
        if (bufptr_begin_ == bufptr_end_) {
            // All input has been consumed and moved to the message buffer
            bufptr_end_ = buf_.begin();
            bufptr_begin_ = buf_.begin();
        }

        // If the envelope encounted an error, then the strategy is just to exit
        if (envelope_.input_error()) {
            return close();
        }

        // If the message is completed, then proceed and process the actually
        // message
        if (envelope_.input_completed()) {
            auto os = envelope_.output_message();
            Result::Code r = process_request(envelope_.input_message(),
                                             os, root);
            envelope_.output_prepare(r);
            return next_state_write();
        }

        // Otherwice, continue to gathere data
        return next_state_read();
    }

    ProcessingState write() {
        str  chunk = envelope_.output_get_chunk();
        TRACE(NOISE) << "Got chunk of " << chunk.size() << " bytes";
        ssize_t res = BlockRead::write(chunk.begin(), chunk.size());
        if (res < 0) return close();
        envelope_.output_commit(res);

        if (!envelope_.output_completed()) {
            return DO_WRITE;
        }

        if (envelope_.force_connection_close()) {
            return close();
        }

        envelope_.reset();
        return DO_READ;
    }

    void set_root_node(vtss::expose::json::RootNode *r) {
        root = r;
    }

    void reset() { envelope_.reset(); }
    virtual ~ServerPeer() { }

  private:
    vtss::expose::json::RootNode *root;
    FixedBuffer buf_in_, buf_out_;
    Envelope_type envelope_;
};

struct GenericServer {
    enum EventType {EVENT_READ, EVENT_WRITE};
    explicit GenericServer(uint32_t p);

    int start();
    int run_one(bool b);
    int run_one(const std::chrono::steady_clock::time_point& tp);

    void run();
    void service();
    bool notify();

    virtual ~GenericServer();

    virtual bool client_accept(int fd) = 0;
    virtual BlockRead::ProcessingState client_read(int fd) =0;
    virtual BlockRead::ProcessingState client_write(int fd) =0;

  private:
    void handle_fd_event(int fd, EventType e);
    void client_accept_(int fd);
    void client_remove(int fd);
    void handle_fd_listen_event();
    void handle_fd_notified_event();
    void handle_fd_client_event(int fd, EventType e);

    int fdmax;
    volatile int notified_;
    fd_set read_fds_, write_fds_;
    int listen_fd;
    int self_pipe_[2];
    uint32_t tcp_port_no;
};

template<typename ServerPeer_type, int MAX_CLIENTS>
struct HttpServer : public vtss::expose::json::RootNode, public GenericServer {
    explicit HttpServer(uint32_t p) : GenericServer(p) { }

    bool client_accept(int fd) {
        for (uint32_t i = 0; i < MAX_CLIENTS; ++i) {
            if (!clients[i].is_free()) {
                continue;
            }

            clients[i].open(fd);
            clients[i].set_root_node(this);
            return true;
        }

        // No free clients
        return false;
    }

    BlockRead::ProcessingState client_read(int fd) {
        for (uint32_t i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].fd_match(fd)) {
                return clients[i].read();
            }
        }
        return BlockRead::CLOSED;
    }

    BlockRead::ProcessingState client_write(int fd) {
        for (uint32_t i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].fd_match(fd)) {
                return clients[i].write();
            }
        }
        return BlockRead::CLOSED;
    }

  private:
    ServerPeer_type clients[MAX_CLIENTS];
};

typedef HttpServer<
  vtss::json::ServerPeer<
    vtss::json::HttpEnvelope
  >,
  5  // Maximum 5 concurrent clients
> HttpServer5;

}  // namespace json
}  // namespace vtss

#undef TRACE
#endif  // __VTSS_JSON_RPC_SERVER_HXX__
