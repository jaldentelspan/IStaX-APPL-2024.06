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

#ifndef _FRR_HXX_
#define _FRR_HXX_

#include "main_types.h"
#include <vtss/appl/module_id.h>
#include <string>
#include <vtss/basics/expose/json.hxx>
#include <vtss/basics/expose/json/enum-macros.hxx>

/** \brief FRR error return codes (mesa_rc) */
enum {
    FRR_RC_INVALID_ARGUMENT = MODULE_ERROR_START(VTSS_MODULE_ID_FRR),
    FRR_RC_IFINDEX_MUST_BE_OF_TYPE_VLAN,
    FRR_RC_INTERNAL_ERROR,
    FRR_RC_DAEMON_TYPE_INVALID,
    FRR_RC_DAEMON_NOT_STARTED,
    FRR_RC_IP_ROUTE_PARSE_ERROR,
    FRR_RC_NOT_SUPPORTED,
    FRR_RC_INTERNAL_ACCESS,
    FRR_RC_ENTRY_NOT_FOUND,
    FRR_RC_ENTRY_ALREADY_EXISTS,
    FRR_RC_LIMIT_REACHED,
    FRR_RC_ADDR_RANGE_OVERLAP,
    FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST,
};

namespace vtss
{
//------------------------------------------------------------------------------
//** FRR configuration/status result
//------------------------------------------------------------------------------
template <typename Type>
struct FrrRes {
    // explicit not explicit, because we want the conversion
    FrrRes(Type &t) : rc {VTSS_RC_OK}, val {t}
    {
    }

    FrrRes(Type &&t) : rc {VTSS_RC_OK}, val {std::move(t)}
    {
    }

    FrrRes(mesa_rc r) : rc {r}
    {
    }

    explicit operator bool()
    {
        return rc == VTSS_RC_OK;
    }

    Type *operator->()
    {
        return &val;
    }

    mesa_rc rc;
    Type val;
};

//------------------------------------------------------------------------------
//** FRR VTY JSON output callback functions
//------------------------------------------------------------------------------
#define CHECK_LOAD(X, error_label) \
    if (!X) {                      \
        goto error_label;          \
    }

template <typename KEY, typename VAL>
struct MapSortedCb {
    virtual void entry(const KEY &key, VAL &val)
    {
    }

    virtual ~MapSortedCb() = default;
};

template <typename KEY, typename VAL>
void serialize(vtss::expose::json::Loader &l, MapSortedCb<KEY, VAL> &m)
{
    const char *b = l.pos_;

    CHECK_LOAD(l.load(vtss::expose::json::map_start), Error);

    while (true) {
        KEY k {};
        CHECK_LOAD(l.load(k), Error);
        CHECK_LOAD(l.load(vtss::expose::json::map_assign), Error);
        VAL v {};
        CHECK_LOAD(l.load(v), Error);
        m.entry(k, v);
        if (!l.load(vtss::expose::json::delimetor)) {
            l.reset_error();
            break;
        }
    }

    CHECK_LOAD(l.load(vtss::expose::json::map_end), Error);
    return;

Error:
    l.pos_ = b;
}

template <typename VAL>
void serialize(vtss::expose::json::Loader &l, Vector<VAL> &rez)
{
    const char *b = l.pos_;

    CHECK_LOAD(l.load(vtss::expose::json::array_start), Error);
    while (true) {
        VAL v {};
        CHECK_LOAD(l.load(v), Error);
        rez.push_back(std::move(v));
        if (!l.load(vtss::expose::json::delimetor)) {
            l.reset_error();
            break;
        }
    }

    CHECK_LOAD(l.load(vtss::expose::json::array_end), Error);
    return;

Error:
    l.pos_ = b;
}

template <typename VAL>
void serialize(vtss::expose::json::Loader &l, vtss::Map<mesa_ipv4_t, VAL> &m)
{
    const char *b = l.pos_;
    CHECK_LOAD(l.load(vtss::expose::json::map_start), Error);
    while (true) {
        mesa_ipv4_t k;
        CHECK_LOAD(l.load(AsIpv4(k)), Error);
        CHECK_LOAD(l.load(vtss::expose::json::map_assign), Error);
        VAL v {};
        CHECK_LOAD(l.load(v), Error);
        m[k] = std::move(v);
        if (!l.load(vtss::expose::json::delimetor)) {
            l.reset_error();
            break;
        }
    }

    CHECK_LOAD(l.load(vtss::expose::json::map_end), Error);
    return;

Error:
    l.pos_ = b;
}

} // namespace vtss

#endif // _FRR_HXX_

