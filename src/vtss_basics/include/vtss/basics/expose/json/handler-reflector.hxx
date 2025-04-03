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

#ifndef __VTSS_BASICS_EXPOSE_JSON_HANDLER_REFLECTOR_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_HANDLER_REFLECTOR_HXX__

#include <vtss/basics/tags.hxx>
#include <vtss/basics/meta-data-packet.hxx>
#include <vtss/basics/depend-on-capability.hxx>
#include <vtss/basics/expose/json/serialize.hxx>
#include <vtss/basics/expose/json/reflection.hxx>
#include <vtss/basics/expose/json/serialize-class.hxx>

namespace vtss {
namespace expose {
namespace json {

struct HandlerReflector {
  public:
    friend class Map;

    struct Map {
        Map(HandlerReflector *p, const char *t, const char *d) : parent(p) {
            parent->reflector->type_begin(t, d);
        }

        ~Map() { parent->reflector->type_end(); }

        template <typename T, typename... Args>
        void add_leaf(T &&value, const Args &&... args) {
            // get the name argument
            vtss::meta::VAArgs<Args...> argpack(args...);
            const char *name = argpack.template get<tag::Name>().s;
            const char *desc = argpack.template get<tag::Description>().s;
            const char *dep =
                    depend_on_capability_name(forward<const Args>(args)...);
            const char *dep_group =
                    depend_on_capability_json_ref(forward<const Args>(args)...);

            // if we are generating a target specific specification, then check
            // the capabilities
            if (parent->target_specific_ &&
                !check_depend_on_capability(forward<const Args>(args)...)) {
                return;
            }

            parent->reflector->variable_begin(name, desc, dep, dep_group);
            serialize_class(*parent, value);
            parent->reflector->variable_end();
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

        bool ok() const { return true; }

      private:
        HandlerReflector *const parent;
    };

    typedef Map Map_t;

    HandlerReflector(Reflection *const r) : reflector(r) {
        target_specific_ = r->target_specific();
    }

    bool ok() const { return ok_; }

    void increment_argument_index() {}

    template <typename T, typename... Args>
    void add_leaf(T &&value, const Args &&... args) {
        vtss::meta::VAArgs<Args...> argpack(args...);
        const char *name = argpack.template get<tag::Name>().s;
        const char *desc = argpack.template get<tag::Description>().s;
        const char *dep =
                depend_on_capability_name(forward<const Args>(args)...);
        const char *dep_group =
                depend_on_capability_json_ref(forward<const Args>(args)...);

        // if we are generating a target specific specification, then check
        // the capabilities
        if (target_specific_ &&
            !check_depend_on_capability(forward<const Args>(args)...)) {
            return;
        }

        reflector->variable_begin(name, desc, dep, dep_group);
        serialize_class(*this, value);
        reflector->variable_end();
    }

    template <typename T, typename... Args>
    void capability(const Args... args) {
        auto val = T::get();
        add_leaf(val, vtss::tag::Name(T::name), vtss::tag::Description(T::desc),
                 forward<const Args>(args)...);
    }


    template <typename T, typename... Args>
    void add_snmp_leaf(T &&value, const Args &&... args) {}

    template <typename T, typename... Args>
    void add_rpc_leaf(T &&value, const Args &&... args) {
        add_leaf(forward<T>(value), forward<const Args>(args)...);
    }

    void type_terminal(JsonCoreType::E type, const char *c_type_name,
                       const char *description) {
        reflector->type_terminal(type, c_type_name, c_type_name, description);
    }

    template <typename T>
    void type_terminal_alias(T &&t, JsonCoreType::E type,
                             const char *c_type_name,
                             const char *c_type_alias_of,
                             const char *description) {
        reflector->type_terminal(type, c_type_name, c_type_alias_of,
                                 description);

        reflector->alias_begin();
        // Ensure that we know how the typedef is defined
        serialize(*this, t);
        reflector->alias_end();
    }

    void type_terminal_enum(const char *c_type_name, const char *desc,
                            const vtss_enum_descriptor_t *d) {
        reflector->type_terminal_enum(c_type_name, desc, d);
    }

    template <typename... Args>
    void argument_properties(const Args &&... args) {
        vtss::meta::VAArgs<Args...> argpack(args...);

        auto a = argpack.template get_optional<tag::ArgumentIndex>();
        if (a) reflector->argument_begin(a->i);

        auto name = argpack.template get_optional<tag::Name>();
        auto desc = argpack.template get_optional<tag::Description>();
        reflector->argument_name(name ? name->s : nullptr,
                                 desc ? desc->s : nullptr);
    }

    void argument_properties_clear() { reflector->argument_end(); }

    template <typename... Args>
    Map_t as_map(const Args &&... args) {
        vtss::meta::VAArgs<Args...> argpack(args...);
        const char *t = argpack.template get<tag::Typename>().s;
        auto *d = argpack.template get_optional<tag::Description>();

        if (d)
            return Map(this, t, d->s);
        else
            return Map(this, t, "");
    }

  private:
    bool ok_ = true;
    Reflection *const reflector;
    bool target_specific_;


#if 0
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

        Placeholder(Placeholder &&rhs) {
            parent = rhs.parent;
            rhs.parent = nullptr;
        }

        Placeholder() : parent(nullptr) {}
        explicit Placeholder(Exporter *p) : parent(p) {}

        void connect(Exporter *p) {
            // If we are already connected, start by disconnect!
            if (parent) close();

            // Can not continue with a non-working parent! leave this
            // unconnected
            if (!parent || !parent->ok_) return;

            // store the new parent
            parent = p;

            initialize();
        };

        void close() {
            if (!parent || !parent->ok_) return;
            finialize();
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
        virtual void finialize() = 0;
    };

    struct Ref : public Placeholder {
        typedef Map Map_t;

        Ref() : Placeholder() {}
        explicit Ref(Exporter *p) : Placeholder(p) { initialize(); }
        Ref(Ref &&rhs) : Placeholder(vtss::move(rhs)) {}
        ~Ref() { close(); }

        template <typename T, typename... Args>
        void add_leaf(T &&value, const Args &&... args) {
            if (!parent) return;
            expose::json::serialize_class(*parent, value);
        }

        template <typename... Args>
        void argument_properties(const Args... args) {}
        void argument_properties_clear() {}

        Ref as_ref() { return Ref(parent); }

        Exporter::Map as_map() { return Exporter::Map(parent); }

        Tuple as_tuple() { return Tuple(parent); }


      protected:
        void initialize() {};
        void finialize() {};
    };

    struct Map : public Placeholder {
      public:
        typedef Map Map_t;

        Map() : Placeholder() {}

        explicit Map(Exporter *p) : Placeholder(p) { initialize(); }

        Map(Map &&rhs) : Placeholder(vtss::move(rhs)), first(rhs.first) {}

        ~Map() { close(); }

        Ref as_ref(const char *name) {
            prepare_map_entry(name);
            return Ref(parent);
        }

        Map as_map(const char *name) {
            prepare_map_entry(name);
            return Map(parent);
        }

        Exporter::Tuple as_tuple(const char *name) {
            prepare_map_entry(name);
            return Exporter::Tuple(parent);
        }

        virtual void initialize() { serialize(*parent, map_start); }
        virtual void finialize() { serialize(*parent, map_end); }

        // Add an element to the map
        template <typename T, typename... Args>
        void add_leaf(T &&value, const Args &&... args) {
            if (!parent) return;

            // get the name argument
            vtss::meta::VAArgs<Args...> argpack(args...);
            const char *name = argpack.template get<tag::Name>().s;

            // add the element to the map
            prepare_map_entry(name);
            expose::json::serialize_class(*parent, value);
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
        Tuple(const Tuple &&rhs)
            : Placeholder(vtss::move(rhs.parent)), first(rhs.first) {}
        ~Tuple() { close(); }

        virtual void initialize() { serialize(*parent, array_start); }
        virtual void finialize() { serialize(*parent, array_end); }

        Ref as_ref() {
            prepare_tuple_entry();
            return Ref(parent);
        }

        Exporter::Map as_map() {
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

        ~EncodedStream() { close(); }

        virtual void initialize() { parent->push_char('\"'); }
        virtual void finialize() { parent->push_char('\"'); }

        bool ok() const { return ok(); }

        bool push(char val);

        size_t write(const char *b, const char *e);
    };

    // request already connected data-structures
    Map as_map() { return Map(this); }
    Tuple as_tuple() { return Tuple(this); }
    EncodedStream encoded_stream() { return EncodedStream(this); }

    template <typename T, typename... Args>
    void add_leaf(T &&value, const Args &&... args) {
        if (!ok()) return;
        expose::json::serialize_class(*this, value);
    }

    template <typename... Args>
    void argument_properties(const Args... args) {}
    void argument_properties_clear() {}

    void flag_error() { ok_ = false; }
    bool push_char(char c) { return out->push(c); }

    bool is_exporter() const { return true; }
    bool is_loader() const { return false; }

  private:
    ostream *out;
#endif
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_HANDLER_REFLECTOR_HXX__
