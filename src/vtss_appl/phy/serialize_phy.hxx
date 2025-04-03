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

#ifndef _SERIALIZE_PHY_H_
#define _SERIALIZE_PHY_H_

#include <microchip/ethernet/switch/api.h>
#include "vtss/basics/expose/json.hxx"

#define PP_SERIALIZE_JSON_ENUM_EXPORTER_DECLARE(N) \
    void serialize(::vtss::expose::json::Exporter &ar, const N &b);

#define PP_SERIALIZE_JSON_ENUM_LOADER_DECLARE(N) \
    void serialize(::vtss::expose::json::Loader &ar, N &b);

#define PP_SERIALIZE_JSON_ENUM_DECLARE(N)      \
    PP_SERIALIZE_JSON_ENUM_EXPORTER_DECLARE(N) \
    PP_SERIALIZE_JSON_ENUM_LOADER_DECLARE(N)

PP_SERIALIZE_JSON_ENUM_DECLARE(vtss_phy_10g_ib_apc_op_mode_t);
PP_SERIALIZE_JSON_ENUM_DECLARE(vtss_phy_10g_vscope_scan_t);

template<class A> void serialize(A &a, vtss_ib_par_cfg_t &s);
template<class A> void serialize(A &a, vtss_phy_10g_ib_conf_t &s);
template<class A> void serialize(A &a, vtss_phy_10g_ib_status_t &s);
template<class A> void serialize(A &a, vtss_phy_10g_apc_conf_t &s);
template<class A> void serialize(A &a, vtss_phy_10g_apc_status_t &s);
template<class A> void serialize(A &a, vtss_phy_10g_base_kr_conf_t &s);
template<class A> void serialize(A &a, vtss_phy_10g_ob_status_t &s);
template<class A> void serialize(A &a, vtss_phy_10g_vscope_conf_t &s);
template<class A> void serialize(A &a, vtss_phy_10g_vscope_scan_conf_t &s);
template<class A> void serialize(A &a, vtss_phy_10g_vscope_scan_status_t &s);
template<class A> void serialize(A &a, vtss_phy_10g_serdes_status_t &s);
template<class A> void serialize(A &a, vtss_phy_10g_prbs_mon_conf_t &s);
template<class A> void serialize(A &a, vtss_sublayer_status_t &s);

#endif //_SERIALIZE_PHY_H_
