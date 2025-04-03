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

#ifndef __VTSS_JSON_RPC_HXX__
#define __VTSS_JSON_RPC_HXX__

#include "vtss/basics/string.hxx"

namespace vtss {
namespace json {

namespace Result {
    enum Code {
        OK               =      0,
        PARSE_ERROR      = -32700,
        INVALID_REQUEST  = -32600,
        METHOD_NOT_FOUND = -32601,
        INVALID_PARAMS   = -32602,
        INTERNAL_ERROR   = -32603
    };
}

struct JsonSession {
    const str input_message() const;
#if 0
    explicit JsonSession(StreamBuf *s);

    bool parse_input();
    void prepare_error(Result::Code r);
    json::Exporter::Tuple prepare_result();
    const Request& req() { return req_; }

  private:
    Request req_;
    StreamBuf *io_buf;
    StreamBuf::OutputMessageStream out;
    vtss::json::Exporter exporter;
    json::Exporter::Map map;
#endif
};

}  // namespace json
}  // namespace vtss

#endif  // __VTSS_JSON_RPC_HXX__
