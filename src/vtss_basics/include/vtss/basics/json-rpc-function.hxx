/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __JSON_RPC_FUNCTION_HXX_
#define __JSON_RPC_FUNCTION_HXX_

#include <stdlib.h>  // TODO(anielsen) get this removed!
#include <unistd.h>  // TODO(anielsen) get this removed!
#include "vtss/basics/utility.hxx"
#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/json-rpc.hxx"
#include "vtss/basics/intrusive_list.hxx"
#include "vtss/basics/http-too-simple-client.hxx"
#include "vtss/basics/expose/json/node.hxx"
#include "vtss/basics/expose/json/namespace-node.hxx"

#include <vtss/basics/expose/json/loader.hxx>
#include <vtss/basics/expose/json/literal.hxx>
#include <vtss/basics/expose/json/exporter.hxx>
#include <vtss/basics/expose/json/response.hxx>
#include <vtss/basics/expose/json/function-exporter-abstract.hxx>

namespace vtss {
namespace json {

template <class T>
static bool parse_input_param(T &data, const vtss::expose::json::Request *req,
                              uint32_t &i, const char **not_found) {
    if (req->params.size() <= i) {
        return false;
    }

    if (!req->params[i].copy_to(data, not_found)) {
        return false;
    }

    ++i;
    return true;
}

template <class T>
static bool parse_output_param(T &data,
                               const vtss::expose::json::JsonArrayPtr &res,
                               uint32_t &i) {
    if (res.size() <= i) {
        return false;
    }

    if (!res[i].copy_to(data)) {
        return false;
    }

    ++i;
    return true;
}

// TODO(anielsen) add support for a formating tag
// TODO(anielsen) some functions allows to pass a null pointer, we must find a
// way to handle this

template <class T>
struct CalcArgType {
    typedef typename T::type type;
};

template <class T>
struct PARAM_INST_PTR {
    typedef T type;
    typedef T param_type;
    param_type as_param() { return NULL; }
    bool init(const vtss::expose::json::Request * /*req*/, uint32_t & /*i*/, const char ** /* not_found */) {
        return true;
    }

    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}
    static void serialize_in(const T &, vtss::expose::json::Exporter::Tuple &) {
    }
    static bool parse_output(const T &,
                             const vtss::expose::json::JsonArrayPtr &,
                             uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_IN {
    typedef T type;
    typedef T &param_type;
    T data;
    param_type as_param() { return data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T &t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(const_cast<T &>(t));
        ;
    }
    static bool parse_out(const T &,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_IN<const T> {
    typedef const T type;
    typedef T &param_type;
    T data;
    param_type as_param() { return data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T &t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(const_cast<T &>(t));
    }
    static bool parse_out(const T &,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_IN<T *> {
    typedef T *type;
    typedef T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T *const t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(*const_cast<T *>(t));
    }
    static bool parse_out(const T *,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_IN<const T *> {
    typedef const T *type;
    typedef T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T *const t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(*const_cast<T *>(t));
    }
    static bool parse_out(const T *,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_IN<T *const> {
    typedef T *const type;
    typedef T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T *const t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(*const_cast<T *>(t));
    }
    static bool parse_out(T *const, const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_IN<const T *const> {
    typedef const T *const type;
    typedef const T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T *const t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(*const_cast<T *>(t));
    }
    static bool parse_out(const T *const,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_IN<T &> {
    typedef T &type;
    typedef T &param_type;
    T data;
    param_type as_param() { return data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T &t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(const_cast<T &>(t));
    }
    static bool parse_out(const T &,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_IN<const T &> {
    typedef const T &type;
    typedef T &param_type;
    T data;
    param_type as_param() { return data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T &t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(const_cast<T &>(t));
    }
    static bool parse_out(const T &t,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &c) {
        return true;
    }
};

template <class T>
struct PARAM_OUT;

template <class T>
struct PARAM_OUT<T *> {
    typedef T *type;
    typedef T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request * /*req*/, uint32_t & /*i*/, const char ** /* not_found */) {
        return true;
    }
    void latch_out(vtss::expose::json::Exporter::Tuple &tuple) { tuple.add_leaf(data); }

    static void serialize_in(const T *const,
                             vtss::expose::json::Exporter::Tuple &) {}
    static bool parse_out(T *t, const vtss::expose::json::JsonArrayPtr &r,
                          uint32_t &c) {
        return parse_output_param(*t, r, c);
    }
};

template <class T>
struct PARAM_OUT<T *const> {
    typedef T *const type;
    typedef T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request * /*req*/, uint32_t & /*i*/, const char ** /* not_found */) {
        return true;
    }
    void latch_out(vtss::expose::json::Exporter::Tuple &tuple) { tuple.add_leaf(data); }

    static void serialize_in(const T *const,
                             vtss::expose::json::Exporter::Tuple &) {}
    static bool parse_out(T *const t,
                          const vtss::expose::json::JsonArrayPtr &r,
                          uint32_t &c) {
        return parse_output_param(*t, r, c);
    }
};

template <class T>
struct PARAM_OUT<T &> {
    typedef T &type;
    typedef T &param_type;
    T data;
    param_type as_param() { return data; }
    bool init(const vtss::expose::json::Request * /*req*/, uint32_t & /*i*/, const char ** /* not_found */) {
        return true;
    }
    void latch_out(vtss::expose::json::Exporter::Tuple &tuple) { tuple.add_leaf(data); }

    static void serialize_in(const T &, vtss::expose::json::Exporter::Tuple &) {
    }
    static bool parse_out(T &t, const vtss::expose::json::JsonArrayPtr &r,
                          uint32_t &c) {
        return parse_output_param(t, r, c);
    }
};

template <class T>
struct PARAM_IN_OUT;

template <class T>
struct PARAM_IN_OUT<T *> {
    typedef T *type;
    typedef T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple &tuple) { tuple.add_leaf(data); }

    static void serialize_in(const T *const t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(*const_cast<T *>(t));
    }
    static bool parse_out(T *t, const vtss::expose::json::JsonArrayPtr &r,
                          uint32_t &c) {
        return parse_output_param(*t, r, c);
    }
};

template <class T>
struct PARAM_IN_OUT<T *const> {
    typedef T *const type;
    typedef T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple &tuple) { tuple.add_leaf(data); }

    static void serialize_in(const T *const t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(*const_cast<T *>(t));
    }
    static bool parse_out(T *const t,
                          const vtss::expose::json::JsonArrayPtr &r,
                          uint32_t &c) {
        return parse_output_param(*t, r, c);
    }
};

template <class T>
struct PARAM_IN_OUT<T &> {
    typedef T &type;
    typedef T &param_type;
    T data;
    param_type as_param() { return data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple &tuple) { tuple.add_leaf(data); }

    static void serialize_in(const T &t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(const_cast<T &>(t));
    }
    static bool parse_out(T &t, const vtss::expose::json::JsonArrayPtr &r,
                          uint32_t &c) {
        return parse_output_param(t, r, c);
    }
};

template <class T>
struct PARAM_AUTO;
template <class T>
struct PARAM_AUTO {  // This is an input
    typedef T type;
    typedef T &param_type;
    T data;
    param_type as_param() { return data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T &t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(const_cast<T &>(t));
    }
    static bool parse_out(const T &t,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_AUTO<const T> {  // This is an input
    typedef const T type;
    typedef T &param_type;
    T data;
    param_type as_param() { return data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T &t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(const_cast<T &>(t));
    }
    static bool parse_out(const T &,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_AUTO<T *> {  // This is an output
    typedef T *type;
    typedef T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request * /*req*/, uint32_t & /*i*/, const char ** /* not_found */) {
        return true;
    }
    void latch_out(vtss::expose::json::Exporter::Tuple &tuple) { tuple.add_leaf(data); }

    static void serialize_in(const T *, vtss::expose::json::Exporter::Tuple &) {
    }
    static bool parse_out(T *t, const vtss::expose::json::JsonArrayPtr &r,
                          uint32_t &c) {
        return parse_output_param(*t, r, c);
    }
};

template <class T>
struct PARAM_AUTO<const T *> {  // This is an input
    typedef const T *type;
    typedef T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T *const t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(*const_cast<T *>(t));
    }
    static bool parse_out(const T *,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_AUTO<T *const> {  // This is an output
    typedef T *const type;
    typedef T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request * /*req*/, uint32_t & /*i*/, const char ** /* not_found */) {
        return true;
    }
    void latch_out(vtss::expose::json::Exporter::Tuple &tuple) { tuple.add_leaf(data); }

    static void serialize_in(T *const, vtss::expose::json::Exporter::Tuple &) {}
    static bool parse_out(T *const t,
                          const vtss::expose::json::JsonArrayPtr &r,
                          uint32_t &c) {
        return parse_output_param(*t, r, c);
    }
};

template <class T>
struct PARAM_AUTO<const T *const> {  // This is an input
    typedef const T *const type;
    typedef T *param_type;
    T data;
    param_type as_param() { return &data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T *const t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(*const_cast<T *>(t));
    }
    static bool parse_out(const T *const,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &) {
        return true;
    }
};

template <class T>
struct PARAM_AUTO<T &> {  // This is an output
    typedef T &type;
    typedef T &param_type;
    T data;
    param_type as_param() { return data; }
    bool init(const vtss::expose::json::Request * /*req*/, uint32_t & /*i*/, const char ** /* not_found */) {
        return true;
    }
    void latch_out(vtss::expose::json::Exporter::Tuple &tuple) { tuple.add_leaf(data); }

    static void serialize_in(const T &, vtss::expose::json::Exporter::Tuple &) {
    }
    static bool parse_out(T &t, const vtss::expose::json::JsonArrayPtr &r,
                          uint32_t &c) {
        return parse_output_param(t, r, c);
    }
};

template <class T>
struct PARAM_AUTO<const T &> {  // This is an input
    typedef const T &type;
    typedef T &param_type;
    T data;
    param_type as_param() { return data; }
    bool init(const vtss::expose::json::Request *r, uint32_t &c, const char **not_found) {
        return parse_input_param(data, r, c, not_found);
    }
    void latch_out(vtss::expose::json::Exporter::Tuple & /*tuple*/) {}

    static void serialize_in(const T &t,
                             vtss::expose::json::Exporter::Tuple &tuple) {
        tuple.add_leaf(const_cast<T &>(t));
    }
    static bool parse_out(const T &t,
                          const vtss::expose::json::JsonArrayPtr &,
                          uint32_t &c) {
        return true;
    }
};

namespace {
template <class... Args>
struct tuple_;
template <>
struct tuple_<> {
    bool parse_input(const vtss::expose::json::Request *req, uint32_t &cnt, const char **not_found) {
        return true;
    }

    template <typename F, class... Args>
    int call_(F f, Args... args) {
        return (*f)(args...);
    }

    template <typename F>
    int call(F f) {
        return (*f)();
    }

    bool serialize_output(vtss::expose::json::Exporter::Tuple &o) { return true; }
};

template <class Head, class... Tail>
struct tuple_<Head, Tail...> {
    Head data;
    tuple_<Tail...> rest;

    bool parse_input(const vtss::expose::json::Request *req, uint32_t &cnt, const char **not_found) {
        if (!data.init(req, cnt, not_found)) {
            return false;
        }

        return rest.parse_input(req, cnt, not_found);
    }

    template <typename F, class... Args>
    int call_(F f, Args... args) {
        typedef typename Head::param_type data_type;
        return rest.template call_<F, Args..., data_type>(f, args...,
                                                          data.as_param());
    }

    template <typename F>
    int call(F f) {
        typedef typename Head::param_type data_type;
        return rest.template call_<F, data_type>(f, data.as_param());
    }

    bool serialize_output(vtss::expose::json::Exporter::Tuple &o) {
        data.latch_out(o);

        if (!o.ok()) {
            //VTSS_BASICS_TRACE(ERROR) << "JSON Encoding error";
            return false;
        }

        return rest.serialize_output(o);
    }
};
}  // namespace

template <class... Args>
struct FunctionExporter : public vtss::expose::json::FunctionExporterAbstract {
    typedef int (*function_ptr_t)(typename Args::type...);

    FunctionExporter(vtss::expose::json::NamespaceNode *ns, const char *n,
                     function_ptr_t p)
        : vtss::expose::json::FunctionExporterAbstract(ns, n, nullptr, false, 0,
                                                       -1),
          ptr(p) {}

    bool is_function_exporter() const {
        return true;
    }
    
    Result::Code exec(const vtss::expose::json::Request *req, ostreamBuf *os) {
        uint32_t cnt = 0;
        tuple_<Args...> args;

        arguments_got = req->params.size();
        if (!args.parse_input(req, cnt, &argument_not_found)) {
            error_argument_index = cnt;
            arguments_expect = arguments_got;
            return Result::INVALID_PARAMS;
        }

        arguments_expect = cnt;
        if (arguments_got != arguments_expect) {
            return Result::INVALID_PARAMS;
        }

        // Invoke the actual function
        if (args.call(ptr) != 0) return Result::INTERNAL_ERROR;

        // Create an exporter which the output parameters is written to
        vtss::expose::json::Exporter e(os);

        // Build the result message as a map
        vtss::expose::json::Exporter::Map m = e.as_map();
        m.add_leaf(req->id, vtss::tag::Name("id"));
        m.add_leaf(vtss::expose::json::lit_null, vtss::tag::Name("error"));

        // Prepare a result vector
        vtss::expose::json::Exporter::Tuple result_vector =
                m.as_tuple(vtss::tag::Name("result"));
        if (!args.serialize_output(result_vector)) {
            return Result::INTERNAL_ERROR;
        }

        return Result::OK;
    }

    void handle_reflection(vtss::expose::json::Reflection*) {}

    function_ptr_t ptr;
};


namespace {
template <class... Args>
void serialize_inputs(vtss::expose::json::Exporter::Tuple &param_vector, Args... args);

template <class... Args>
int parse_outputs(vtss::expose::json::JsonArrayPtr &, uint32_t &, Args...);

template <>
inline void serialize_inputs(vtss::expose::json::Exporter::Tuple &param_vector) {}
template <class Head, class... Tail>
void serialize_inputs(vtss::expose::json::Exporter::Tuple &param_vector,
                      typename CalcArgType<Head>::type head,
                      typename CalcArgType<Tail>::type... tail) {
    Head::serialize_in(head, param_vector);
    serialize_inputs<Tail...>(param_vector, tail...);
}

template <>
inline int parse_outputs(vtss::expose::json::JsonArrayPtr &params, uint32_t &cnt) {
    cnt = 0;
    return 0;
}

template <class Head, class... Tail>
int parse_outputs(vtss::expose::json::JsonArrayPtr &params, uint32_t &cnt,
                  typename CalcArgType<Head>::type head,
                  typename CalcArgType<Tail>::type... tail) {
    if (Head::parse_out(head, params, cnt))
        return parse_outputs<Tail...>(params, cnt, tail...);
    else
        return -1;
}
}  // namespace

struct NodeImport {
    explicit NodeImport(const char *n) : name_(n) {}
    NodeImport(NodeImport *ns, const char *n) : parent_(ns), name_(n) {}

    void attach(NodeImport *leaf) const { leaf->parent_ = this; }

    const NodeImport *parent_ = 0;
    const str name_;
};

struct NamespaceImport : public NodeImport {
    explicit NamespaceImport(const char *n) : NodeImport(n) {}
    NamespaceImport(NamespaceImport *ns, const char *n) : NodeImport(ns, n) {}
};

void serialize(vtss::expose::json::Exporter &e, const NodeImport &n);

template <class... Args>
struct FunctionImporter : public NodeImport {
    explicit FunctionImporter(const char *n) : NodeImport(n) {}
    FunctionImporter(NamespaceImport *ns, const char *n) : NodeImport(ns, n) {}

    int operator()(typename CalcArgType<Args>::type... args) {
        // TODO(anielsen) should not be constructed every time
        http::TooSimpleHttpClient client(4096);

        {  // Build a request /////////////////////////////////////////////////
            int id = 42;  // TODO(anielsen) should not be magic
            auto bufstream = client.post_data();
            vtss::expose::json::Exporter e(&bufstream);

            // Build request
            vtss::expose::json::Exporter::Map m = e.as_map();
            m.add_leaf(id, vtss::tag::Name("id"));
            m.add_leaf(*static_cast<NodeImport *>(this),
                       vtss::tag::Name("method"));

            vtss::expose::json::Exporter::Tuple param_vector =
                    m.as_tuple(vtss::tag::Name("params"));
            serialize_inputs<Args...>(param_vector, args...);
        }

        // hack
        // This is a really bad idea... Only one environment variable should be
        // used and it should be called VTSS_RPC_SERVER_URI
        const char *rpc_server_ip = getenv("VTSS_RPC_SERVER_IP");
        const char *rpc_server_port = getenv("VTSS_RPC_SERVER_PORT");
        const char *rpc_server_path = getenv("VTSS_RPC_SERVER_PATH");
        if (!rpc_server_ip) rpc_server_ip = "127.0.0.1";
        if (!rpc_server_port) rpc_server_port = "5000";
        if (!rpc_server_path) rpc_server_path = "/json_rpc";
        int port = atoi(rpc_server_port);

        // Send request and wait for response /////////////////////////////////

        int post_cnt = 0;
        bool post_ok = false;
        do {
            if (post_cnt) {
                sleep(1);
            }

            ++post_cnt;
            post_ok = client.post(rpc_server_ip, port, str(rpc_server_path));
        } while (!post_ok && post_cnt < 10);

        if (!post_ok) {
            //VTSS_BASICS_TRACE(DEBUG) << "Failed posting data";
            return -1;
        }

        //VTSS_BASICS_TRACE(NOISE) << "Got result: " << client.msg();

        vtss::expose::json::Response res;
        {
            vtss::expose::json::Loader loader(client.msg().begin(),
                                              client.msg().end());
            if (!loader.load(res)) {
                //VTSS_BASICS_TRACE(DEBUG)
                //        << "Failed at position "
                //        << static_cast<unsigned int>(loader.pos_ -
                //                                     loader.begin_)
                //        << " to parse response message as a JSON response:\n"
                //        << client.msg() << "\n"
                //        << WhiteSpace(static_cast<unsigned int>(
                //                   loader.pos_ - loader.begin_)) << "^";
                return -1;
            }
        }

        if (res.result.type() != vtss::expose::json::ValueType::Array) {
            //VTSS_BASICS_TRACE(DEBUG) << "Could not parse result.\n"
            //                         << client.msg();
            return -1;
        }

        vtss::expose::json::JsonArrayPtr params;
        if (!res.result.copy_to(params)) {
            //VTSS_BASICS_TRACE(DEBUG) << "Failed to parse results as array:\n"
            //                         << client.msg();
            return -1;
        }

        // Parse and copy all input parameters ////////////////////////////////
        uint32_t cnt = 0;
        if (parse_outputs<Args...>(params, cnt, args...) != 0) {
            //VTSS_BASICS_TRACE(DEBUG) << "Failed to parse argument\n"
            //                         << client.msg();
            return -1;
        }
        return 0;
    }
};

}  // namespace json
}  // namespace vtss

#endif  // __JSON_RPC_FUNCTION_HXX_
