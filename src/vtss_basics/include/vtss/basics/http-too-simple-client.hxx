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

#ifndef __HTTP_TOO_SIMPLE_CLIENT__
#define __HTTP_TOO_SIMPLE_CLIENT__

#include "vtss/basics/http-message-stream-parser.hxx"

namespace vtss {
namespace http {

struct TooSimpleHttpClient {
    explicit TooSimpleHttpClient(size_t buf_size);

    void reset();
    bool get(const char *ipv4, int port, str path);
    bool post(const char *ipv4, int port, str path);

    const http::RequestWriter& req() const { return req_; }
    const http::ResponseParser& res() const { return res_; }

    const str msg() const;
    StreamTree::Leaf::ContentStream post_data();
    void output_reset() const { req_.output_reset(); }

  private:
    int  fd_get(const char *ipv4, int port);
    bool fd_read(int fd);
    bool fd_write(int fd);

    FixedBuffer buf_in_, buf_out_;
    http::RequestWriter  req_;
    http::ResponseParser res_;
};

}  // namespace http
}  // namespace vtss

#endif  // __HTTP_TOO_SIMPLE_CLIENT__

