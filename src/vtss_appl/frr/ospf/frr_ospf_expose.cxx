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

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr_ospf_serializer.hxx"
#include "vtss/appl/ospf.h"

/******************************************************************************/
/** Enum descriptor text mapping                                              */
/******************************************************************************/
/* Notice that the naming rule of enumeration should refer to
 * RFC 2578 - Structure of Management Information Version 2 (SMIv2).
 * Section 7.1.4.  The BITS construct
 *      A label for a named-number enumeration must consist of one
 *      or more letters or digits, up to a maximum of 64 characters, and the
 *      initial character must be a lower-case letter.  (However, labels
 *      longer than 32 characters are not recommended.)  Note that hyphens
 *      are not allowed by this specification.
 */

vtss_enum_descriptor_t vtss_appl_ospf_auth_type_txt[] {
    {VTSS_APPL_OSPF_AUTH_TYPE_NULL, "nullAuth"},
    {VTSS_APPL_OSPF_AUTH_TYPE_SIMPLE_PASSWORD, "simplePasswordAuth"},
    {VTSS_APPL_OSPF_AUTH_TYPE_MD5, "md5Auth"},
    {VTSS_APPL_OSPF_AUTH_TYPE_AREA_CFG, "areaConfigAuth"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_ospf_interface_state_txt[] {
    {VTSS_APPL_OSPF_INTERFACE_DOWN, "down"},
    {VTSS_APPL_OSPF_INTERFACE_LOOPBACK, "loopback"},
    {VTSS_APPL_OSPF_INTERFACE_WAITING, "waiting"},
    {VTSS_APPL_OSPF_INTERFACE_POINT2POINT, "pointToPoint"},
    {VTSS_APPL_OSPF_INTERFACE_DR_OTHER, "drOther"},
    {VTSS_APPL_OSPF_INTERFACE_BDR, "bdr"},
    {VTSS_APPL_OSPF_INTERFACE_DR, "dr"},
    {VTSS_APPL_OSPF_INTERFACE_UNKNOWN, "unknown"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_ospf_neighbor_state_txt[] {
    {VTSS_APPL_OSPF_NEIGHBOR_STATE_DEPENDUPON, "dependupon"},
    {VTSS_APPL_OSPF_NEIGHBOR_STATE_DELETED, "deleted"},
    {VTSS_APPL_OSPF_NEIGHBOR_STATE_DOWN, "down"},
    {VTSS_APPL_OSPF_NEIGHBOR_STATE_ATTEMPT, "attempt"},
    {VTSS_APPL_OSPF_NEIGHBOR_STATE_INIT, "init"},
    {VTSS_APPL_OSPF_NEIGHBOR_STATE_2WAY, "twoWay"},
    {VTSS_APPL_OSPF_NEIGHBOR_STATE_EXSTART, "exstart"},
    {VTSS_APPL_OSPF_NEIGHBOR_STATE_EXCHANGE, "exchange"},
    {VTSS_APPL_OSPF_NEIGHBOR_STATE_LOADING, "loading"},
    {VTSS_APPL_OSPF_NEIGHBOR_STATE_FULL, "full"},
    {VTSS_APPL_OSPF_NEIGHBOR_STATE_UNKNOWN, "unknown"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_ospf_area_type_txt[] {
    {VTSS_APPL_OSPF_AREA_NORMAL, "normalArea"},
    {VTSS_APPL_OSPF_AREA_STUB, "stubArea"},
    {VTSS_APPL_OSPF_AREA_TOTALLY_STUB, "totallyStubArea"},
    {VTSS_APPL_OSPF_AREA_NSSA, "nssa"},
    {VTSS_APPL_OSPF_AREA_COUNT, "areaUnknown"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_ospf_redist_metric_type_txt[] {
    {VTSS_APPL_OSPF_REDIST_METRIC_TYPE_NONE, "metricTypeNone"},
    {VTSS_APPL_OSPF_REDIST_METRIC_TYPE_1, "metricType1"},
    {VTSS_APPL_OSPF_REDIST_METRIC_TYPE_2, "metricType2"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_ospf_nssa_translator_role_text[] {
    {VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_CANDIDATE, "candidate"},
    {VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_NEVER, "never"},
    {VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_ALWAYS, "always"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_ospf_nssa_translator_state_text[] {
    {VTSS_APPL_OSPF_NSSA_TRANSLATOR_STATE_DISABLED, "disabled"},
    {VTSS_APPL_OSPF_NSSA_TRANSLATOR_STATE_ELECTED, "elected"},
    {VTSS_APPL_OSPF_NSSA_TRANSLATOR_STATE_ENABLED, "enabled"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_ospf_route_type_txt[] {
    {VTSS_APPL_OSPF_ROUTE_TYPE_INTRA_AREA, "intraArea"},
    {VTSS_APPL_OSPF_ROUTE_TYPE_INTER_AREA, "interArea"},
    {VTSS_APPL_OSPF_ROUTE_TYPE_BORDER_ROUTER, "borderRouter"},
    {VTSS_APPL_OSPF_ROUTE_TYPE_EXTERNAL_TYPE_1, "externalType1"},
    {VTSS_APPL_OSPF_ROUTE_TYPE_EXTERNAL_TYPE_2, "externalType2"},
    {VTSS_APPL_OSPF_ROUTE_TYPE_UNKNOWN, "unknown"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_ospf_route_border_router_type_txt[] {
    {VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_ABR, "abr"},
    {VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_INTRA_AREA_ASBR, "asbrIntra"},
    {VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_INTER_AREA_ASBR, "asbrInter"},
    {VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_ABR_ASBR, "abrAsbr"},
    {VTSS_APPL_OSPF_ROUTE_BORDER_ROUTER_TYPE_NONE, "none"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_ospf_lsdb_type_txt[] {
    {VTSS_APPL_OSPF_LSDB_TYPE_NONE, "none"},
    {VTSS_APPL_OSPF_LSDB_TYPE_ROUTER, "router"},
    {VTSS_APPL_OSPF_LSDB_TYPE_NETWORK, "network"},
    {VTSS_APPL_OSPF_LSDB_TYPE_SUMMARY, "summary"},
    {VTSS_APPL_OSPF_LSDB_TYPE_ASBR_SUMMARY, "asbrSummary"},
    {VTSS_APPL_OSPF_LSDB_TYPE_EXTERNAL, "external"},
    {VTSS_APPL_OSPF_LSDB_TYPE_NSSA_EXTERNAL, "nssaExternal"},
    {0, 0}
};

