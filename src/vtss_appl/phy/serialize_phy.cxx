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

#include "vtss_json_rpc_server_api.hxx"
#include "vtss/basics/ringbuf.hxx"
#include "vtss/basics/expose/json/parse-and-compare.hxx"
#include "serialize_phy_impl.hxx"
#if defined(VTSS_SW_OPTION_PHY)
#include "phy_api.h"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;

#define PP_SERIALIZE_JSON_ENUM_EXPORTER_CASE(X)     \
    case PP_TUPLE_N(0, X) :                         \
        serialize(ar, vtss::str(PP_TUPLE_N(1, X))); \
        break;

#define PP_SERIALIZE_JSON_ENUM_EXPORTER(N, ...)                 \
    void serialize(::vtss::expose::json::Exporter &ar, const N &b) {    \
        switch (b) {                                            \
            PP_FOR_EACH(PP_SERIALIZE_JSON_ENUM_EXPORTER_CASE,   \
                        __VA_ARGS__) default : ar.flag_error(); \
            break;                                              \
        }                                                       \
    }

#define PP_SERIALIZE_JSON_ENUM_LOADER_CASE(X)                               \
    if (parse_and_compare(ar.pos_, ar.end_, vtss::str(PP_TUPLE_N(1, X)))) { \
        b = PP_TUPLE_N(0, X);                                               \
        return;                                                             \
    }

#define PP_SERIALIZE_JSON_ENUM_LOADER(N, ...)           \
    void serialize(::vtss::expose::json::Loader &ar, N &b) {    \
        using ::vtss::expose::json::parse_and_compare;          \
        PP_FOR_EACH(PP_SERIALIZE_JSON_ENUM_LOADER_CASE, \
                    __VA_ARGS__) ar.flag_error();       \
    }

#define PP_SERIALIZE_JSON_ENUM(N, ...)              \
    PP_SERIALIZE_JSON_ENUM_EXPORTER(N, __VA_ARGS__) \
            PP_SERIALIZE_JSON_ENUM_LOADER(N, __VA_ARGS__)

#define PP_SERIALIZE_JSON_ENUM_DECLARE(N)      \
    PP_SERIALIZE_JSON_ENUM_EXPORTER_DECLARE(N) \
            PP_SERIALIZE_JSON_ENUM_LOADER_DECLARE(N)

#define DECL_SERIALIZE_REF(X)                             \
    void serialize(::vtss::expose::json::Exporter &e, const X &); \
    void serialize(::vtss::expose::json::Loader &e, X &);

#define DECL_SERIALIZE_TAG(X)                     \
    void serialize(::vtss::expose::json::Exporter &e, X); \
    void serialize(::vtss::expose::json::Loader &e, X);

PP_SERIALIZE_JSON_ENUM(vtss_phy_10g_ib_apc_op_mode_t,
    (VTSS_IB_APC_AUTO,   "IB_APC_AUTO"),
    (VTSS_IB_APC_MANUAL, "IB_APC_MANUAL"),
    (VTSS_IB_APC_FREEZE, "IB_APC_FREEZE"),
    (VTSS_IB_APC_RESET,  "IB_APC_RESET"),
    (VTSS_IB_APC_RESTART,"IB_APC_RESTART"),
    (VTSS_IB_APC_NONE,   "IB_APC_NONE")
);

PP_SERIALIZE_JSON_ENUM(vtss_phy_10g_vscope_scan_t,
    (VTSS_PHY_10G_FAST_SCAN,      "FAST_SCAN"),
    (VTSS_PHY_10G_FAST_SCAN_PLUS, "FAST_SCAN_PLUS"),
    (VTSS_PHY_10G_QUICK_SCAN,     "QUICK_SCAN"),
    (VTSS_PHY_10G_FULL_SCAN,      "FULL_SCAN")
);

namespace vtss {
static expose::json::NamespaceNode ns("phy10g");

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<    const mesa_port_no_t>,
    json::PARAM_AUTO<    const vtss_phy_10g_ib_conf_t *const>,
    json::PARAM_IN<      const BOOL>
> export_phy10g_ib_conf_set(&ns, "ib_conf_set", &vtss_phy_10g_ib_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<    const mesa_port_no_t>,
    json::PARAM_IN<      const BOOL>,
    json::PARAM_AUTO<    vtss_phy_10g_ib_conf_t *const>
> export_phy10g_ib_conf_get(&ns, "ib_conf_get", &vtss_phy_10g_ib_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<    const mesa_port_no_t>,
    json::PARAM_IN_OUT<  vtss_phy_10g_ib_status_t *const>
> export_phy10g_ib_status_get(&ns, "ib_status_get", &vtss_phy_10g_ib_status_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_AUTO<   const vtss_phy_10g_apc_conf_t *const>,
    json::PARAM_IN<     const BOOL>
> export_phy10g_apc_conf_set(&ns, "apc_conf_set", &vtss_phy_10g_apc_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_AUTO<   const BOOL>,
    json::PARAM_AUTO<   vtss_phy_10g_apc_conf_t *const>
> export_phy10g_apc_conf_get(&ns, "apc_conf_get", &vtss_phy_10g_apc_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_AUTO<   const BOOL>,
    json::PARAM_AUTO<   vtss_phy_10g_apc_status_t *const>
> export_phy10g_apc_status_get(&ns, "apc_status_get", &vtss_phy_10g_apc_status_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_AUTO<   const vtss_phy_10g_base_kr_conf_t *const>
> export_phy10g_kr_conf_set(&ns, "kr_conf_set", &vtss_phy_10g_base_kr_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_AUTO<   vtss_phy_10g_base_kr_conf_t *const>
> export_phy10g_kr_conf_get(&ns, "kr_conf_get", &vtss_phy_10g_base_kr_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_AUTO<   const vtss_phy_10g_base_kr_conf_t *const>
> export_phy10g_kr_host_conf_set(&ns, "kr_host_conf_set", &vtss_phy_10g_base_kr_host_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_AUTO<   vtss_phy_10g_base_kr_conf_t *const>
> export_phy10g_kr_host_conf_get(&ns, "kr_host_conf_get", &vtss_phy_10g_base_kr_host_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_IN_OUT< vtss_phy_10g_ob_status_t *const>
> export_phy10g_ob_status_get(&ns, "ob_status_get", &vtss_phy_10g_ob_status_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_AUTO<   const vtss_phy_10g_vscope_conf_t *const>
> export_phy10g_vscope_conf_set(&ns, "vscope_conf_set", &vtss_phy_10g_vscope_conf_set);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_IN_OUT< vtss_phy_10g_vscope_conf_t *const>
> export_phy10g_vscope_conf_get(&ns, "vscope_conf_get", &vtss_phy_10g_vscope_conf_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_IN_OUT< vtss_phy_10g_vscope_scan_status_t *const>
> export_phy10g_vscope_scan_status_get(&ns, "vscope_scan_status_get", &vtss_phy_10g_vscope_scan_status_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_AUTO<   vtss_phy_10g_serdes_status_t *const>
> export_phy10g_serdes_status_get(&ns, "serdes_status_get", &vtss_phy_10g_serdes_status_get);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_IN<     vtss_phy_10g_prbs_mon_conf_t *const>
>export_phy10g_prbs_mon_conf(&ns, "prbs_mon_conf", &vtss_phy_10g_prbs_mon_conf);

static json::FunctionExporter<
    json::PARAM_INST_PTR< const vtss_inst_t>,
    json::PARAM_AUTO<   const mesa_port_no_t>,
    json::PARAM_IN_OUT< vtss_phy_10g_prbs_mon_conf_t *const>
>export_phy10g_prbs_mon_conf_get(&ns, "prbs_mon_conf_get", &vtss_phy_10g_prbs_mon_conf_get);
}

extern "C" void vtss_appl_phy_json_init() { 
    vtss::json_node_add(&vtss::ns);
}
#endif
