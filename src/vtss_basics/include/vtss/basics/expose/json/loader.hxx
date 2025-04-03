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

#ifndef __VTSS_BASICS_EXPOSE_JSON_LOADER_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_LOADER_HXX__

#include "vtss/basics/tags.hxx"
#include "vtss/basics/vector.hxx"
#include "vtss/basics/meta-data-packet.hxx"
#include "vtss/basics/expose/json/literal.hxx"
#include "vtss/basics/expose/json/serialize-class.hxx"
#include "vtss/basics/depend-on-capability.hxx"

namespace vtss {
namespace expose {
namespace json {

struct Loader {
    struct Map {
      public:
        // Initialize a map loader attached to an upstream importer.
        Map(Loader *p, bool patch);

        ~Map() {
            if (!ok()) return;

            // If the map is associated to a loader, then the upstream cursor
            // must be updated to end of map. This is done regardless of if all
            // elements in the map has been accessed.
            parent->pos_ = end_of_map;
        }

        bool ok() const { return parent && parent->ok_; }

        template <typename T, typename... Args>
        bool add_leaf(T &&value, const Args &&... args) {
            // Build the argument pack, which allows the position independent
            // interface
            vtss::meta::VAArgs<Args...> argpack(args...);

            // filter out attribute which are not available due to capabilities
            if (!tag::check_depend_on_capability(forward<const Args>(args)...))
                return false;

            // Calculate the name.
            const char *name = argpack.template get<tag::Name>().s;

            return element_impl(name, value);
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


      private:
        template <typename T>
        bool element_impl(const char *name, T &t) {
            if (!ok()) return false;

            // move the cursor to where the given map element-value start.
            bool found_it = search(name);
            if (found_it) {
                expose::json::serialize_class(*parent, t);
                return true;
            }

            // current element was not found in the input request - is this a
            // problem?
            if (!allow_uninitialized_values) {
                parent->not_found_ = name;
                parent->flag_error();
            }
            return false;
        }

        bool search(const char *name);

        Loader *parent = nullptr;
        const char *end_of_map = nullptr;
        Vector<const char *> index;
        bool allow_uninitialized_values;
    };

    template <typename T, typename... Args>
    void add_leaf(T &&value, const Args &&... args) {
        expose::json::serialize_class(*this, value);
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

    Loader(const char *begin, const char *end)
        : ok_(true), pos_(begin), begin_(begin), end_(end) {}

    typedef Map Map_t;
    template <typename... Args>
    Map as_map(const Args &&... args) {
        return Map(this, patch_mode_);
    }

    template <typename T>
    bool load(T &&t) {
        return load_impl(t);
    }

    void reset_error() { ok_ = true; }
    void flag_error() { ok_ = false; }

    bool is_exporter() const { return false; }
    bool is_loader() const { return true; }
    bool ok() const { return ok_; }

    bool ok_;
    const char *pos_;
    const char *begin_;
    const char *end_;
    const char *not_found_ = nullptr;
    bool patch_mode_ = false;  // if true, then allow uninitialized values when
                               // serializing structs

  private:
    template <typename T>
    bool load_impl(T &t) {
        expose::json::serialize_class(*this, t);
        return ok_;
    }
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_LOADER_HXX__
