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

#include "vtss_process_manager.hxx"

#include "vtss/basics/stream.hxx"
#include "vtss/basics/formatting_tags.hxx"
#include "vtss/basics/notifications/process.hxx"
#include "vtss/basics/synchronized.hxx"

#include "vtss/basics/trace_basics.hxx"

#define TRACE(X) VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_PROCESS, X)

namespace vtss {
namespace appl {
namespace util {

void process_manager_print(cli_print *pr) {
    FixedBuffer buf(2048);
    BufPtrStream o(&buf);

    o << left<32>("Name") << " " << left<16>("State") << " " << left<6>("Pid")
      << "\n";

    o << left<32>("", '-') << " " << left<16>("", '-') << " "
      << left<6>("", '-') << "\n";

    SYNCHRONIZED(list, vtss::notifications::LIST_OF_PROCESSSES) {
        for (const auto *p : list) {
            auto st = p->status();
            o << left<32>(p->name()) << " " << left<16>(st.state()) << " "
              << left<6>(st.pid()) << "\n";
        }
    }

    pr("%s", o.cstring());
}

}  // namespace util
}  // namespace appl
}  // namespace vtss
