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

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/expose/snmp/handlers/getset_base.hxx"

namespace vtss {
namespace expose {
namespace snmp {

bool GetSetHandlerCommon::consume_oid(uint32_t o) {
    if (oid_index_ >= seq_.valid) {
        VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, NOISE)
                << "No oid to consume. oid_index=" << oid_index_
                << " seq=" << seq_;

        return false;
    }

    if (seq_.oids[oid_index_] == o) {
        VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, NOISE)
                << "OID match: expect=" << seq_.oids[oid_index_] << " got=" << o
                << " oid_index=" << oid_index_ << " seq=" << seq_;

        oid_index_++;
        return true;
    }

    VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, RACKET)
            << "OID does not match: expect=" << seq_.oids[oid_index_]
            << " got=" << o << " oid_index=" << oid_index_ << " seq=" << seq_;
    return false;
}

bool GetSetHandlerCommon::consume_oid_leaf(uint32_t o) {
    if (!consume_oid(o)) return false;

    return oid_index_ == seq_.valid;
}

void GetSetHandlerCommon::consume_oid_rollback() {
    VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, NOISE)
            << "rollback: oid_index=" << oid_index_ << " seq=" << seq_;
    oid_index_--;
}

void GetSetHandlerCommon::error_code(ErrorCode::E e, const char *f,
                                     unsigned line) {
    VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_SNMP, DEBUG)
            << f << ":" << line << "Updating error code " << error_code_
            << " -> " << e;
    if (e != ErrorCode::noError) state_ = HandlerState::FAILED;
    error_code_ = e;
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
