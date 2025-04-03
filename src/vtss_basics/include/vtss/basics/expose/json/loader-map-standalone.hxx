/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_EXPOSE_JSON_LOADER_MAP_STANDALONE_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_LOADER_MAP_STANDALONE_HXX__

#include <vtss/basics/expose/json/loader.hxx>
#include <vtss/basics/expose/json/error-reporting-path.hxx>

namespace vtss {
namespace expose {
namespace json {

struct LoaderMapStandalone {
    LoaderMapStandalone() : loader(nullptr, nullptr), map(&loader, true) {}

    LoaderMapStandalone(const char *begin, const char *end,
                        ErrorReportingPath *e, bool patch_mode)
        : loader(begin, end), map(&loader, patch_mode), ep(e) {}

    ~LoaderMapStandalone() {
        if (!loader.ok() && ep)
            ep->error(vtss::json::Result::INVALID_PARAMS);
    }

    bool ok() const { return map.ok(); }

    template <typename T, typename... Args>
    void add_leaf(T &&value, const Args &&... args) {
        map.add_leaf(vtss::forward<T>(value), vtss::forward<const Args>(args)...);
    }

    template <typename T, typename... Args>
    void add_snmp_leaf(T &&value, const Args &&... args) {}

    template <typename T, typename... Args>
    void add_rpc_leaf(T &&value, const Args &&... args) {
        add_leaf(forward<T>(value), forward<const Args>(args)...);
    }

    template <typename... Args>
    void argument_properties(const Args... args) {}
    void argument_properties_clear() {}
    void increment_argument_index() {}

  private:
    Loader loader;
    Loader::Map map;
    ErrorReportingPath *ep = nullptr;
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_LOADER_MAP_STANDALONE_HXX__
