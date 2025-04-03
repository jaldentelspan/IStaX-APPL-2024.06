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


#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/json-rpc.hxx"
#include "vtss/basics/json-rpc-server.hxx"

namespace vtss {
namespace json {

Result::Code process_request(str in, ostreamBuf& out,
                             vtss::expose::json::RootNode *r) {
    VTSS_BASICS_TRACE(DEBUG) << "Input: " << in;

    if (!r) {
        VTSS_BASICS_TRACE(NOISE) << "No root node provided";
        return Result::METHOD_NOT_FOUND;
    }

    return r->process(0x7fffffff, in, out);
}

GenericServer::GenericServer(uint32_t p)
        : fdmax(0), listen_fd(-1), tcp_port_no(p) {
    FD_ZERO(&read_fds_);
    FD_ZERO(&write_fds_);
}

int GenericServer::start() {
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port = htons(tcp_port_no);
    memset(&(serveraddr.sin_zero), '\0', 8);
    int one = 1;

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        VTSS_BASICS_TRACE(INFO) << "Failed to create socket";
        return -1;
    }

    if (setsockopt(listen_fd, SOL_SOCKET,
                   SO_REUSEADDR, &one, sizeof(one)) == -1) {
        VTSS_BASICS_TRACE(INFO) << "Failed to set socket option";
        goto Error;
    }


    if (bind(listen_fd,
            (struct sockaddr *)&serveraddr,
            sizeof(serveraddr)) == -1) {
        VTSS_BASICS_TRACE(INFO) << "Failed to bind";
        goto Error;
    }

    if (listen(listen_fd, 10) == -1) {
        goto Error;
    }

    FD_SET(listen_fd, &read_fds_);
    fdmax = listen_fd;

    VTSS_BASICS_TRACE(INFO) << "Server is started and is listning on port: " <<
        tcp_port_no;

    // Setup sefl pipe
    //if (pipe2(self_pipe_,  O_CLOEXEC | O_NONBLOCK) != 0) {
    if (pipe(self_pipe_) != 0) {
        goto Error;
    }

    FD_SET(self_pipe_[0], &read_fds_);
    fdmax = max(self_pipe_[0], fdmax);

    return 0;

  Error:
    ::close(listen_fd);
    listen_fd = -1;

    return -1;
}

GenericServer::~GenericServer() {
    if (listen_fd != -1) {
        ::close(listen_fd);
        ::close(self_pipe_[0]);
        ::close(self_pipe_[1]);
    }
}

bool GenericServer::notify() {
    return ::write(self_pipe_[1], "\0", 1) == 1;
}

int GenericServer::run_one(bool blocking) {
    int cnt = 0;

    if (listen_fd < 0) {
        int res = start();
        if (res != 0) {
            return res;
        }
    }

    fd_set read_fds = read_fds_;
    fd_set write_fds = write_fds_;

    if (blocking) {
        cnt = ::select(fdmax+1, &read_fds, &write_fds, NULL, NULL);
    } else {
        struct timeval tv;
        memset(&tv, 0, sizeof(tv));
        cnt = ::select(fdmax+1, &read_fds, &write_fds, NULL, &tv);
    }

    if (cnt < 0) return -1;

    for (int i = 0; i <= fdmax; i++)
        if (FD_ISSET(i, &read_fds))
            handle_fd_event(i, EVENT_READ);

    for (int i = 0; i <= fdmax; i++)
        if (FD_ISSET(i, &write_fds))
            handle_fd_event(i, EVENT_WRITE);

    return cnt;
}

int GenericServer::run_one(const std::chrono::steady_clock::time_point& tp) {
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    steady_clock::duration timeout = tp - steady_clock::now();
    timeout = vtss::max(timeout, steady_clock::duration(0));  // no negative time

    struct timeval tv;
    tv.tv_sec = duration_cast<std::chrono::seconds>(timeout).count();
    tv.tv_usec = duration_cast<std::chrono::microseconds>(
            duration_cast<std::chrono::microseconds>(timeout) -
            std::chrono::seconds(tv.tv_sec)).count();

    fd_set read_fds = read_fds_;
    fd_set write_fds = write_fds_;
    int cnt = ::select(fdmax+1, &read_fds, &write_fds, NULL, &tv);

    if (cnt < 0) return -1;

    for (int i = 0; i <= fdmax; i++)
        if (FD_ISSET(i, &read_fds))
            handle_fd_event(i, EVENT_READ);

    for (int i = 0; i <= fdmax; i++)
        if (FD_ISSET(i, &write_fds))
            handle_fd_event(i, EVENT_WRITE);

    return cnt;
}

void GenericServer::run() {
    notified_ = 0;

    while (!notified_)
        run_one(true);
}

void GenericServer::service() {
    notified_ = 0;

    while ((run_one(false) > 0) && !!notified_) { }
}

void GenericServer::handle_fd_event(int fd, EventType e) {
    if (fd == listen_fd)          handle_fd_listen_event();
    else if (fd == self_pipe_[0]) handle_fd_notified_event();
    else                          handle_fd_client_event(fd, e);
}

void GenericServer::client_accept_(int fd) {
    if (!client_accept(fd)) {
        // No free client
        // LOG(INFO) << "Closing connection as we are out of resources. FD: "
        // << fd;
        ::close(fd);
        return;
    }

    FD_SET(fd, &read_fds_);
    fdmax = max(fd, fdmax);
}

void GenericServer::client_remove(int fd) {
    // TODO(anielsen)  new max
    FD_CLR(fd, &read_fds_);
}

void GenericServer::handle_fd_notified_event() {
    char c;
    while (::read(self_pipe_[0], &c, 1) == 1) { }
    notified_ = 1;
}

void GenericServer::handle_fd_listen_event() {
    int newfd;
    struct sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(clientaddr);

    if ((newfd = accept(listen_fd,
                        (struct sockaddr *)&clientaddr,
                        &addrlen)) < 0) {
        perror("Server-accept() error");
        return;
    }

    // VLOG(100) << "Accepted new client, fd=" << newfd;
    client_accept_(newfd);
}

void GenericServer::handle_fd_client_event(int fd, EventType e) {
    BlockRead::ProcessingState st = BlockRead::CLOSED;
    switch (e) {
      case EVENT_READ:
        FD_CLR(fd, &read_fds_);
        st = client_read(fd);
        break;

      case EVENT_WRITE:
        FD_CLR(fd, &write_fds_);
        st = client_write(fd);
        break;

      default:
        assert(0);
    }

    switch (st) {
      case BlockRead::CLOSED:
        client_remove(fd);
        return;

      case BlockRead::DO_READ:
        FD_SET(fd, &read_fds_);
        return;

      case BlockRead::DO_WRITE:
        FD_SET(fd, &write_fds_);
        return;

      default:
        assert(0);
    }
}

void BlockRead::local_reset() {
    bufptr_end_ = buf_.begin();
    bufptr_begin_ = buf_.begin();
}

void BlockRead::open(int f) {
    close();
    fd_ = f;
    reset();
}

BlockRead::ProcessingState BlockRead::close() {
    if (fd_ >= 0) {
        // LOG(INFO) << "Closing file descriptor " << fd_;
        ::close(fd_);
    }
    fd_ = -1;
    local_reset();
    return next_state_closed();
}

BlockRead::ProcessingState BlockRead::read() {
    int max_read = buf_.end() - bufptr_end_;
    assert(max_read <= 128 && max_read >= 0);

    if (max_read > 0) {
        ssize_t res = ::read(fd_, bufptr_end_, max_read);
        if (res <= 0) {
            VTSS_BASICS_TRACE(DEBUG) << "Read returned error";
            return close();
        }

        bufptr_end_ += res;
    }

    return process();
}

ssize_t BlockRead::write(const char *b, size_t s) {
    ssize_t res = ::write(fd_, b, s);

    if (res > 0) {
        VTSS_BASICS_TRACE(NOISE) << "WRITE\n" << str(b, b+res);
        return res;
    }

    if (errno == EAGAIN) {
        VTSS_BASICS_TRACE(DEBUG) << "NO-WRITE os buffer is full";
        return  0;

    } else {
        VTSS_BASICS_TRACE(INFO) << "Write failed";
        return -1;
    }
}

void HttpEnvelope::output_prepare(Result::Code c) {
    http::HttpCode http_code;

    switch (c) {
        case Result::OK:
            http_code = http::c200_OK;
            break;

        case Result::Code::METHOD_NOT_FOUND:

            http_code = http::c404_Not_Found;
            break;

        case Result::Code::PARSE_ERROR:
        case Result::Code::INVALID_REQUEST:
        case Result::Code::INVALID_PARAMS:
        case Result::Code::INTERNAL_ERROR:
        default:
            http_code = http::c500_Internal_Server_Error;
            break;
    }

    res.first_line(req.http_version(), http_code);  // TODO(awn) use result code
    res.add_content_length();
    res.header("Server", "vtss-json-rpc");
    res.header("Cache-Control", "private");
    res.header("Content-Type", "application/json-rpc");
    if (req.http_version() == http::v_1_1)
        res.header("Connection", "close");
}

}  // namespace json
}  // namespace vtss

