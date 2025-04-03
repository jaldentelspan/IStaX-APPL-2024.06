/*

 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_EXPOSE_JSON_EXPORTER_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_EXPORTER_HXX__

#include <vtss/basics/tags.hxx>
#include <vtss/basics/stream.hxx>
#include <vtss/basics/meta-data-packet.hxx>
#include <vtss/basics/depend-on-capability.hxx>

#include <vtss/basics/expose/json/literal.hxx>
#include <vtss/basics/expose/json/serialize.hxx>
#include <vtss/basics/expose/json/serialize-class.hxx>

namespace vtss {
namespace expose {
namespace json {

struct Exporter {
    struct Map;
    struct Ref;
    struct Tuple;
    struct EncodedStream;

    typedef Map Map_t;

    // no copies
    Exporter(const Exporter &) = delete;
    Exporter &operator=(const Exporter &) = delete;

    explicit Exporter(ostream *o) : ok_(true), out(o) {}

    struct Placeholder {
        Placeholder(const Placeholder &) = delete;
        Placeholder &operator=(const Placeholder &) = delete;
        Placeholder &operator=(Placeholder &&rhs) {
            parent = rhs.parent;
            rhs.parent = nullptr;
            return *this;
        }

        Placeholder(Placeholder &&rhs) {
            parent = rhs.parent;
            rhs.parent = nullptr;
        }

        Placeholder() : parent(nullptr) {}
        explicit Placeholder(Exporter *p) : parent(p) {}

        void close() {
            finalize();
            parent = nullptr;
        }

        bool connected() const { return parent ? true : false; }

        bool ok() const {
            if (!parent) return false;
            return parent->ok();
        }

      protected:
        Exporter *parent = nullptr;
        virtual void initialize() = 0;
        virtual void finalize() = 0;
    };

    struct Ref : public Placeholder {
        typedef Map Map_t;

        Ref() : Placeholder() {}
        explicit Ref(Exporter *p) : Placeholder(p) { initialize(); }
        Ref(Ref &&rhs) : Placeholder(vtss::move(rhs)) {}
        ~Ref() { finalize(); }

        template <typename T, typename... Args>
        void add_leaf(T &&value, const Args &&... args) {
            if (!parent) return;
            expose::json::serialize_class(*parent, value);
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

        Ref as_ref() { return Ref(parent); }

        template <typename... Args>
        Exporter::Map as_map(const Args &&... args) {
            return Exporter::Map(parent);
        }

        Tuple as_tuple() { return Tuple(parent); }

        size_t raw_write(const char *b, const char *e) {
            if (!parent) return 0;
            return parent->out->write(b, e);
        }

      protected:
        void initialize() {};
        void finalize() {};
    };

    struct Map : public Placeholder {
      public:
        typedef Map Map_t;

        Map() : Placeholder() {}

        explicit Map(Exporter *p) : Placeholder(p) { initialize(); }

        Map(Map &&rhs) : Placeholder(vtss::move(rhs)), first(rhs.first) {}

        ~Map() {
            finalize();
        }

        Ref as_ref(const char *name) {
            prepare_map_entry(name);
            return Ref(parent);
        }

        template <typename... Args>
        Map as_map(const Args &&... args) {
            vtss::meta::VAArgs<Args...> argpack(args...);
            const char *n = argpack.template get<tag::Name>().s;

            prepare_map_entry(n);
            return Map(parent);
        }

        template <typename... Args>
        Exporter::Tuple as_tuple(const Args &&... args) {
            vtss::meta::VAArgs<Args...> argpack(args...);
            const char *name = argpack.template get<tag::Name>().s;

            prepare_map_entry(name);
            return Exporter::Tuple(parent);
        }

        virtual void initialize() { serialize(*parent, map_start); }
        virtual void finalize() {
            if (!parent || !parent->ok_) return;
            serialize(*parent, map_end);
        }


        // Add an element to the map
        template <typename T, typename... Args>
        void add_leaf(T &&value, const Args &&... args) {
            if (!parent) return;

            // filter out attribute which are not available due to capabilities
            if (!tag::check_depend_on_capability(forward<const Args>(args)...))
                return;

            // get the name argument
            vtss::meta::VAArgs<Args...> argpack(args...);
            const char *name = argpack.template get<tag::Name>().s;

            // add the element to the map
            prepare_map_entry(name);
            expose::json::serialize_class(*parent, value);
        }

        template <typename T, typename... Args>
        void capability(const Args... args) {
            auto val = T::get();
            add_leaf(val, vtss::tag::Name(T::name),
                     vtss::tag::Description(T::desc),
                     forward<const Args>(args)...);
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

        // Connect a new place holder into the MAP.
        template <typename T, typename... Args>
        void add_placeholder(T &placeholder, const Args... args) {
            if (!ok()) return;

            // get the name argument
            vtss::meta::VAArgs<Args...> argpack(args...);
            const char *name = argpack.template get<tag::Name>().s;

            // hook in the new place-holder in the existing map
            prepare_map_entry(name);

            // connect the place holder to the exporter
            placeholder.connect(parent);
        }

      private:
        void prepare_map_entry(const char *name) {
            if (!ok()) return;

            if (first)
                first = false;
            else
                serialize(*parent, delimetor);

            serialize(*parent, str(name));
            serialize(*parent, map_assign);
        }

        bool first = true;
    };

    struct Tuple : public Placeholder {
      public:
        typedef Map Map_t;

        Tuple() : Placeholder() {}
        explicit Tuple(Exporter *p) : Placeholder(p) { initialize(); }
        Tuple(Tuple &&rhs)
            : Placeholder(vtss::move(rhs.parent)), first(rhs.first) {}
        ~Tuple() {
            finalize();
        }

        Placeholder &operator=(Tuple &&rhs) {
            Placeholder::operator=(vtss::move(rhs));
            first = rhs.first;
            return *this;
        }

        virtual void initialize() { serialize(*parent, array_start); }
        virtual void finalize() {
            if (!parent || !parent->ok_) return;
            serialize(*parent, array_end);
        }

        Ref as_ref() {
            prepare_tuple_entry();
            return Ref(parent);
        }

        template <typename... Args>
        Exporter::Map as_map(const Args &&... args) {
            prepare_tuple_entry();
            return Exporter::Map(parent);
        }

        Tuple as_tuple() {
            prepare_tuple_entry();
            return Tuple(parent);
        }

        template <typename T, typename... Args>
        void add_leaf(T &&value, const Args &&... args) {
            if (!ok()) return;

            prepare_tuple_entry();
            expose::json::serialize_class(*parent, value);
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

        bool ok() const { return parent->ok(); }

      private:
        void prepare_tuple_entry() {
            if (first) {
                first = false;
            } else {
                serialize(*parent, delimetor);
            }
        }

        bool first = true;
    };

    struct EncodedStream : public ostream, Placeholder {
        friend struct Exporter;

        EncodedStream() : Placeholder() {}
        explicit EncodedStream(Exporter *p) : Placeholder(p) { initialize(); }
        EncodedStream(EncodedStream &&rhs) : Placeholder(vtss::move(rhs)) {}

        ~EncodedStream() {
            finalize();
        }

        virtual void initialize() { parent->push_char('\"'); }
        virtual void finalize() {
            if (!parent || !parent->ok_) return;
            parent->push_char('\"');
        }

        bool ok() const { return parent->ok(); }

        bool push(char val);

        size_t write(const char *b, const char *e);
    };

    // request already connected data-structures
    template <typename... Args>
    Map as_map(const Args &&... args) {
        return Map(this);
    }
    Tuple as_tuple() { return Tuple(this); }
    EncodedStream encoded_stream() { return EncodedStream(this); }

    template <typename T, typename... Args>
    void add_leaf(T &&value, const Args &&... args) {
        if (!ok()) return;
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

    void flag_error() { ok_ = false; }
    bool push_char(char c) { return out->push(c); }
    bool ok() const { return ok_; }

    bool is_exporter() const { return true; }
    bool is_loader() const { return false; }

    ostream &os() { return *out; }

  private:
    bool ok_;
    ostream *out;
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_EXPORTER_HXX__
