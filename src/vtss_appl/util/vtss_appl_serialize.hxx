/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_APPL_SERIALIZE_HXX__
#define __VTSS_APPL_SERIALIZE_HXX__

#include "vtss/basics/snmp.hxx"
#include "vtss/basics/expose.hxx"

#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#ifdef VTSS_SW_OPTION_WEB
#include "web_api_linux.h"
#endif // VTSS_SW_OPTION_WEB
#include "vtss/basics/stream.hxx"
#endif

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#ifdef VTSS_SW_OPTION_WEB
#include "vtss_privilege_web_api.h"
#endif
#endif

#if defined(VTSS_SW_OPTION_PRIVATE_MIB) && !defined(VTSS_SW_OPTION_JSON_RPC)
#define VTSS_XXXX_SERIALIZE_ENUM(A, B, C, D) \
    VTSS_SNMP_SERIALIZE_ENUM(A, B, C, D);

#define VTSS_XXXX_SERIALIZE_ENUM_SHARED(A, B, C, D) \
    VTSS_SNMP_SERIALIZE_ENUM_SHARED(A, B, C, D);

#elif !defined(VTSS_SW_OPTION_PRIVATE_MIB) && defined(VTSS_SW_OPTION_JSON_RPC)
#define VTSS_XXXX_SERIALIZE_ENUM(A, B, C, D) \
    VTSS_JSON_SERIALIZE_ENUM(A, B, C, D)

#define VTSS_XXXX_SERIALIZE_ENUM_SHARED(A, B, C, D) \
    VTSS_JSON_SERIALIZE_ENUM(A, B, C, D)

#elif defined(VTSS_SW_OPTION_PRIVATE_MIB) && defined(VTSS_SW_OPTION_JSON_RPC)
#define VTSS_XXXX_SERIALIZE_ENUM(A, B, C, D) \
    VTSS_SNMP_SERIALIZE_ENUM(A, B, C, D);    \
    VTSS_JSON_SERIALIZE_ENUM(A, B, C, D)

#define VTSS_XXXX_SERIALIZE_ENUM_SHARED(A, B, C, D) \
    VTSS_SNMP_SERIALIZE_ENUM_SHARED(A, B, C, D);    \
    VTSS_JSON_SERIALIZE_ENUM(A, B, C, D)

#else
#define VTSS_XXXX_SERIALIZE_ENUM(A, B, C, D)
#define VTSS_XXXX_SERIALIZE_ENUM_SHARED(A, B, C, D)

#endif

typedef void (*vtss_appl_mib_build_specific_t)(vtss::expose::snmp::MibGenerator &);

#if defined(VTSS_SW_OPTION_PRIVATE_MIB_GEN)
int vtss_appl_mibgenerator_common(CYG_HTTPD_STATE *p,
                                  ::vtss::expose::snmp::NamespaceNode &ns,
                                  const char *name1, const char *name2,
                                  uint32_t module_id,
                                  vtss_appl_mib_build_specific_t f);
#endif

#define VTSS_MIB_MODULE_BASE(NAME1, NAME2, INIT, MODULE_ID, ROOT, HANDLER) \
    static ::vtss::expose::snmp::NamespaceNode ROOT(NAME1, MODULE_ID);             \
    static ::vtss::expose::snmp::reg_ns root_reg(NAME1, NAME2);                    \
    extern "C" void INIT() { ROOT.init(); root_reg.regMib(ROOT); }

#define VTSS_MIB_MODULE_WEB(NAME1, NAME2, INIT, MODULE_ID, ROOT, HANDLER)     \
    static i32 build_mib_web(CYG_HTTPD_STATE *p) {                            \
        return vtss_appl_mibgenerator_common(p, ROOT, NAME1, NAME2 "-MIB",    \
                                             MODULE_ID, &__build_mib__);      \
    }                                                                         \
    CYG_HTTPD_HANDLER_TABLE_ENTRY(INIT##web_entry,                            \
                                  "/" TO_STR(MIBPREFIX) "-" NAME2 "-MIB.mib", \
                                  build_mib_web);

#if defined(VTSS_SW_OPTION_PRIVATE_MIB_GEN)
#define VTSS_MIB_MODULE(NAME1, NAME2, INIT, MODULE_ID, ROOT, HANDLER)   \
    static void __build_mib__(::vtss::expose::snmp::MibGenerator &HANDLER);     \
    VTSS_MIB_MODULE_BASE(NAME1, NAME2, INIT, MODULE_ID, ROOT, HANDLER); \
    VTSS_MIB_MODULE_WEB(NAME1, NAME2, INIT, MODULE_ID, ROOT, HANDLER);  \
    static void __build_mib__(::vtss::expose::snmp::MibGenerator &HANDLER)
#else
#define VTSS_MIB_MODULE(NAME1, NAME2, INIT, MODULE_ID, ROOT, HANDLER)   \
    static void __build_mib__(::vtss::expose::snmp::MibGenerator &HANDLER)      \
            __attribute__((unused));                                    \
    VTSS_MIB_MODULE_BASE(NAME1, NAME2, INIT, MODULE_ID, ROOT, HANDLER); \
    static void __build_mib__(::vtss::expose::snmp::MibGenerator &HANDLER)
#endif


#ifndef VTSS_REGISTER_MIB_NO_WEB
#define VTSS_REGISTER_MIB_NO_WEB(T, I, N)             \
    static T root;                                    \
    static vtss::expose::snmp::vtss_snmp_reg_mib<T> root_reg; \
    extern "C" void I() { root_reg.regMib(root); }
#endif

#if !defined(VTSS_REGISTER_MIB) && defined(VTSS_SW_OPTION_PRIVATE_MIB_GEN)
#define VTSS_REGISTER_MIB(T, I, N)                                        \
    VTSS_REGISTER_MIB_NO_WEB(T, I, N);                                    \
    static i32 root_mib_gen(CYG_HTTPD_STATE *p) {                         \
        vtss::httpstream output(p);                                       \
        vtss::expose::snmp::MibGenerator gen(&output);                            \
                                                                          \
        serialize(gen, root);                                             \
        return -1;                                                        \
    }                                                                     \
    CYG_HTTPD_HANDLER_TABLE_ENTRY(T##web_entry,                           \
                                  "/" TO_STR(MIBPREFIX) "-" N "-MIB.mib", \
                                  root_mib_gen);

#elif !defined(VTSS_REGISTER_MIB)
#define VTSS_REGISTER_MIB(T, I, N) VTSS_REGISTER_MIB_NO_WEB(T, I, N)
#endif

namespace vtss {
void json_node_add(expose::json::Node *node);
}  // namespace vtss

#include "main_types.h"
#include "vtss/appl/types.h"
#include "vtss/appl/port.h"
#include "vtss/appl/interface.h"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss_common_iterator.hxx"
#include "vtss_vcap_serializer.hxx"

/****************************************************************************
 * Shared serializers for API types
 ****************************************************************************/

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
void serialize(vtss::expose::snmp::GetHandler &h, vtss_inet_address_t &a);
void serialize(vtss::expose::snmp::SetHandler &h, vtss_inet_address_t &a);
void serialize(vtss::expose::snmp::Reflector  &h, vtss_inet_address_t &a);

void serialize(vtss::expose::snmp::GetHandler &h, mesa_ip_addr_t &a);
void serialize(vtss::expose::snmp::SetHandler &h, mesa_ip_addr_t &a);
void serialize(vtss::expose::snmp::Reflector  &h, mesa_ip_addr_t &a);
#endif  // VTSS_SW_OPTION_PRIVATE_MIB

#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(vtss::expose::json::Exporter &e, vtss_inet_address_t &s);
void serialize(vtss::expose::json::Loader &e, vtss_inet_address_t &s);
void serialize(vtss::expose::json::HandlerReflector &e, vtss_inet_address_t &s);
#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
void serialize(vtss::expose::snmp::GetHandler &h, mesa_timestamp_t &t);
void serialize(vtss::expose::snmp::SetHandler &h, mesa_timestamp_t &t);
void serialize(vtss::expose::snmp::Reflector &h, mesa_timestamp_t &t);
#endif  // VTSS_SW_OPTION_PRIVATE_MIB

#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(vtss::expose::json::Exporter &e, mesa_timestamp_t &t);
void serialize(vtss::expose::json::Loader &e, mesa_timestamp_t &t);
void serialize(vtss::expose::json::HandlerReflector &e, mesa_timestamp_t &t);
#endif  // VTSS_SW_OPTION_JSON_RPC

#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(vtss::expose::json::Exporter &e, const vtss_appl_vcap_mac_t &s);
void serialize(vtss::expose::json::Loader &e, vtss_appl_vcap_mac_t &s);
void serialize(vtss::expose::json::HandlerReflector &h, vtss_appl_vcap_mac_t &s);
#endif  // VTSS_SW_OPTION_JSON_RPC

/****************************************************************************
 * Shared serializers for PORT types
 ****************************************************************************/
#if defined(VTSS_SW_OPTION_PRIVATE_MIB) || defined(VTSS_SW_OPTION_JSON_RPC)
extern const vtss_enum_descriptor_t vtss_sfp_transceiver_txt[];
#endif
VTSS_XXXX_SERIALIZE_ENUM_SHARED(
        meba_sfp_transreceiver_t, "SfpTransceiver", vtss_sfp_transceiver_txt,
        "This enumeration shows the SFP transceiver type.");

#endif  // __VTSS_APPL_SERIALIZE_HXX__
