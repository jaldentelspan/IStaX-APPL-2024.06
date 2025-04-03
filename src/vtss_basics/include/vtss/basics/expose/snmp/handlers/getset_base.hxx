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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_GETSET_BASE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_GETSET_BASE_HXX__

#include "vtss/basics/expose/snmp/types.hxx"

namespace vtss {
namespace expose {
namespace snmp {

struct GetSetHandlerCommon {
    template <typename TT> friend void serialize_get_set(TT &, NamespaceNode &);

    explicit GetSetHandlerCommon(OidSequence &s, OidSequence &i, bool n) : seq_(s),
                oid_seq_index(i), oid_index_(0), getnext(n),
                state_(HandlerState::SEARCHING)/*, iter(nullptr)*/ { }

    GetSetHandlerCommon(const GetSetHandlerCommon&) = delete;
    GetSetHandlerCommon &operator=(const GetSetHandlerCommon&) = delete;

    bool consume_oid(uint32_t o);
    bool consume_oid_leaf(uint32_t o);
    void consume_oid_rollback();

    HandlerState::E state() const { return state_; }
    void state(HandlerState::E s) { state_ = s; }

    ErrorCode::E error_code() const { return error_code_; }
    void error_code(ErrorCode::E e, const char *f, unsigned line);

    OidSequence oid_seq_out() {
        OidSequence o(seq_);
        o.push(next_index);
        return o;
    }

    const OidSequence &seq_;
    const OidSequence &oid_seq_index;
    OidSequence next_index;
    uint32_t oid_index_;
    ErrorCode::E error_code_ = ErrorCode::noError;

    uint32_t recursive_call = 0;
    uint32_t oid_offset_ = 0;
    bool getnext = false;

  private:
    HandlerState::E state_;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_GETSET_BASE_HXX__
