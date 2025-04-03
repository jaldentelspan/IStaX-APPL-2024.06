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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/http-too-simple-client.hxx"

namespace vtss {
namespace http {

TooSimpleHttpClient::TooSimpleHttpClient(size_t buf_size) : buf_in_(buf_size),
    buf_out_(buf_size), req_(&buf_in_), res_(&buf_out_) { }

void TooSimpleHttpClient::reset() {
    req_.reset();
    res_.reset();
}

int TooSimpleHttpClient::fd_get(const char *ipv4, int port) {
    int fd = -1;
    struct sockaddr_in addr;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        VTSS_BASICS_TRACE(DEBUG) << "Failed to create client socket";
        return fd;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipv4);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        VTSS_BASICS_TRACE(DEBUG) << "Connect failed";
        ::close(fd);
        return -1;
    }

    return fd;
}

bool TooSimpleHttpClient::fd_read(int fd) {
    while (!res_.complete()) {
        SBuf128 read_buf;
        ssize_t s = ::read(fd, read_buf.begin(), read_buf.size());
        if (s <= 0) {
            VTSS_BASICS_TRACE(DEBUG) << "Error reading http message. s=" << s <<
                " Err-no: " << errno << " Err-msg: " << strerror(errno);
            return false;
        }

        res_.process(read_buf.begin(), read_buf.begin() + s);
    }

    return true;
}

bool TooSimpleHttpClient::fd_write(int fd) {
    while (!req_.output_write_done()) {
        str s = req_.output_write_chunk();
        ssize_t res = ::write(fd, s.begin(), s.size());

        if (res <= 0) {
            VTSS_BASICS_TRACE(DEBUG) << "Error writing http message. res = "
                << res << " Err-no: " << errno
                << " Err-msg: " << strerror(errno);
            return false;
        }

        req_.output_write_commit(res);
    }

    return true;
}

bool TooSimpleHttpClient::get(const char *ipv4, int port, str path) {
    int fd = fd_get(ipv4, port);
    if (fd < 0) return false;

    reset();
    req_.first_line(v_1_1, GET, path);
    req_.header("Connection", "Keep-Alive");
    req_.header("Accept",     "*/*");
    req_.header("User-Agent", "VTSS-BASICS");

    if (!fd_write(fd)) goto Error;
    if (!fd_read(fd))  goto Error;

    req_.output_reset();
    ::close(fd);
    return true;

  Error:
    req_.output_reset();
    ::close(fd);
    return false;
}

bool TooSimpleHttpClient::post(const char *ipv4, int port, str path) {
    int fd = fd_get(ipv4, port);
    if (fd < 0) return false;

    req_.first_line(v_1_1, POST, path);
    req_.header("Connection",    "Keep-Alive");
    req_.header("Accept",        "*/*");
    req_.header("User-Agent",    "VTSS-BASICS");
    req_.header("Authorization", "Basic YWRtaW46");  // TODO(anielsen) hardcoded
    req_.add_content_length();

    if (!fd_write(fd)) goto Error;
    if (!fd_read(fd))  goto Error;

    req_.output_reset();
    ::close(fd);
    return true;

  Error:
    req_.output_reset();
    ::close(fd);
    return false;
}

const str TooSimpleHttpClient::msg() const {
    return res_.msg();
}

StreamTree::Leaf::ContentStream TooSimpleHttpClient::post_data() {
    return req_.message_stream();
}

}  // namespace http
}  // namespace vtss

