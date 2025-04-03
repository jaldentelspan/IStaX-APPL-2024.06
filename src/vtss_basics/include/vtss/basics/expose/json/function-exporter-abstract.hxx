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

#ifndef __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_ABSTRACT_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_ABSTRACT_HXX__

#include <vtss/basics/json-rpc.hxx>
#include <vtss/basics/expose/json/namespace-node.hxx>

namespace vtss {
namespace expose {
namespace json {

namespace priv {
enum E {
    NO_ACCESS_CONTROL,
    CONF_RO,
    CONF_RW,
    STATUS_RO,
    STATUS_RW,
    NO_ACCESS,
};
}  // namespace priv

struct FunctionExporterAbstract : public Node {
    FunctionExporterAbstract(NamespaceNode *parent, const char *name,
                             const char *description, bool priv_write,
                             int priv_type, int priv_module);

    size_t handle(int priv, const Request *req, Buf *result_buf) {
        BufPtrStream s(result_buf);
        handle(priv, req, s);
        return s.end() - s.begin();
    }

    virtual bool is_function_exporter_abstract() const { return true; }

    vtss::json::Result::Code handle(int priv, const Request *req,
                                    ostreamBuf &out) {
        return handle(priv, str(), req, out);
    }

    vtss::json::Result::Code handle(int priv, str method_name,
                                    const Request *req, ostreamBuf &out);

    virtual vtss::json::Result::Code exec(const Request *req,
                                          ostreamBuf *os) = 0;
    virtual ~FunctionExporterAbstract() {}

    mesa_rc error_code = MESA_RC_OK;
    void *failing_function = nullptr;
    unsigned error_argument_index = 0;
    unsigned arguments_got = 0;
    unsigned arguments_expect = 0;
    const char *argument_not_found = nullptr;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    const priv::E priv_type = priv::NO_ACCESS_CONTROL;
    const int priv_module_id = 0;
#endif
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_ABSTRACT_HXX__
