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

#ifndef _SERIALIZE_PHY_IMPL_H_
#define _SERIALIZE_PHY_IMPL_H_

#include <microchip/ethernet/switch/api.h>
#include "vtss/basics/formatting_tags.hxx"
#include "serialize_phy.hxx"
#include "vtss_appl_expose_json_array.hxx"

template<class A>
void serialize(A &a, vtss_ib_par_cfg_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.value, vtss::tag::Name("value"));
    map.add_leaf(s.min, vtss::tag::Name("min"));
    map.add_leaf(s.max, vtss::tag::Name("max"));
}

template<class A>
void serialize(A &a, vtss_phy_10g_ib_conf_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.offs, vtss::tag::Name("offs"));
    map.add_leaf(s.gain, vtss::tag::Name("gain"));
    map.add_leaf(s.gainadj, vtss::tag::Name("gainadj"));
    map.add_leaf(s.l, vtss::tag::Name("l"));
    map.add_leaf(s.c, vtss::tag::Name("c"));
    map.add_leaf(s.agc, vtss::tag::Name("agc"));
    map.add_leaf(s.dfe1, vtss::tag::Name("dfe1"));
    map.add_leaf(s.dfe2, vtss::tag::Name("dfe2"));
    map.add_leaf(s.dfe3, vtss::tag::Name("dfe3"));
    map.add_leaf(s.dfe4, vtss::tag::Name("dfe4"));
    map.add_leaf(s.ld, vtss::tag::Name("ld"));
    map.add_leaf(s.prbs, vtss::tag::Name("prbs"));
    map.add_leaf(s.prbs_inv, vtss::tag::Name("prbs_inv"));
    map.add_leaf(s.apc_bit_mask, vtss::tag::Name("apc_bit_mask"));
    map.add_leaf(s.freeze_bit_mask, vtss::tag::Name("freeze_bit_mask"));
    map.add_leaf(s.config_bit_mask, vtss::tag::Name("config_bit_mask"));
    map.add_leaf(s.is_host, vtss::tag::Name("is_host"));
}

template<class A>
void serialize(A &a, vtss_phy_10g_ib_status_t &s) {
    typename A::Map map = a.as_map();

    map.add_leaf(s.ib_conf, vtss::tag::Name("ib_conf"));
    map.add_leaf(s.sig_det, vtss::tag::Name("sig_det"));
    map.add_leaf(s.bit_errors, vtss::tag::Name("bit_errors"));
}

template<class A>
void serialize(A &a, vtss_phy_10g_apc_conf_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.op_mode, vtss::tag::Name("op_mode"));
    map.add_leaf(s.op_mode_flag, vtss::tag::Name("op_mode_flag"));
}
template<class A>
void serialize(A &a, vtss_phy_10g_apc_status_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.reset, vtss::tag::Name("reset"));
    map.add_leaf(s.freeze, vtss::tag::Name("freeze"));
}
template<class A>
void serialize(A &a, vtss_phy_10g_base_kr_conf_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.cm1, vtss::tag::Name("cm1"));
    map.add_leaf(s.c0, vtss::tag::Name("c0"));
    map.add_leaf(s.c1, vtss::tag::Name("c1"));
    map.add_leaf(s.ampl, vtss::tag::Name("ampl"));
    map.add_leaf(s.slewrate, vtss::tag::Name("slewrate"));
    map.add_leaf(s.en_ob, vtss::tag::Name("en_ob"));
    map.add_leaf(s.ser_inv, vtss::tag::Name("ser_inv"));
}
template<class A>
void serialize(A &a, vtss_phy_10g_ob_status_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.r_ctrl, vtss::tag::Name("r_ctrl"));
    map.add_leaf(s.c_ctrl, vtss::tag::Name("c_ctrl"));
    map.add_leaf(s.slew, vtss::tag::Name("slew"));
    map.add_leaf(s.levn, vtss::tag::Name("levn"));
    map.add_leaf(s.d_fltr, vtss::tag::Name("d_fltr"));
    map.add_leaf(s.v3, vtss::tag::Name("v3"));
    map.add_leaf(s.vp, vtss::tag::Name("vp"));
    map.add_leaf(s.v4, vtss::tag::Name("v4"));
    map.add_leaf(s.v5, vtss::tag::Name("v5"));
    map.add_leaf(s.is_host, vtss::tag::Name("is_host"));
}
template<class A>
void serialize(A &a, vtss_phy_10g_vscope_conf_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.scan_type, vtss::tag::Name("scan_type"));
    map.add_leaf(s.line, vtss::tag::Name("line"));
    map.add_leaf(s.enable, vtss::tag::Name("enable"));
    map.add_leaf(s.error_thres, vtss::tag::Name("error_thres"));
}
template<class A>
void serialize(A &a, vtss_phy_10g_vscope_scan_conf_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.line, vtss::tag::Name("line"));
    map.add_leaf(s.x_start, vtss::tag::Name("x_start"));
    map.add_leaf(s.y_start, vtss::tag::Name("y_start"));
    map.add_leaf(s.x_incr, vtss::tag::Name("x_incr"));
    map.add_leaf(s.y_incr, vtss::tag::Name("y_incr"));
    map.add_leaf(s.x_count, vtss::tag::Name("x_count"));
    map.add_leaf(s.y_count, vtss::tag::Name("y_count"));
    map.add_leaf(s.ber, vtss::tag::Name("ber"));
}
template<class A>
void serialize(A &a, vtss_phy_10g_vscope_scan_status_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.scan_conf, vtss::tag::Name("scan_conf"));
    map.add_leaf(s.error_free_x, vtss::tag::Name("error_free_x"));
    map.add_leaf(s.error_free_y, vtss::tag::Name("error_free_y"));
    map.add_leaf(s.amp_range, vtss::tag::Name("amp_range"));
    map.add_leaf(vtss::AsArrayDouble<u32>(&s.errors[0][0], PHASE_POINTS, AMPLITUDE_POINTS), vtss::tag::Name("errors"));
}
template<class A>
void serialize(A &a, vtss_phy_10g_serdes_status_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.rcomp, vtss::tag::Name("rcomp"));
    map.add_leaf(s.h_pll5g_lock_status, vtss::tag::Name("h_pll5g_lock_status"));
    map.add_leaf(s.h_pll5g_fsm_lock, vtss::tag::Name("h_pll5g_fsm_lock"));
    map.add_leaf(s.h_pll5g_fsm_stat, vtss::tag::Name("h_pll5g_fsm_stat"));
    map.add_leaf(s.h_pll5g_gain, vtss::tag::Name("h_pll5g_gain"));
    map.add_leaf(s.l_pll5g_lock_status, vtss::tag::Name("l_pll5g_lock_status"));
    map.add_leaf(s.l_pll5g_fsm_lock, vtss::tag::Name("l_pll5g_fsm_lock"));
    map.add_leaf(s.l_pll5g_fsm_stat, vtss::tag::Name("l_pll5g_fsm_stat"));
    map.add_leaf(s.l_pll5g_gain, vtss::tag::Name("l_pll5g_gain"));
    map.add_leaf(s.h_rx_rcpll_lock_status, vtss::tag::Name("h_rx_rcpll_lock_status"));
    map.add_leaf(s.h_rx_rcpll_range, vtss::tag::Name("h_rx_rcpll_range"));
    map.add_leaf(s.h_rx_rcpll_vco_load, vtss::tag::Name("h_rx_rcpll_vco_load"));
    map.add_leaf(s.h_rx_rcpll_fsm_status, vtss::tag::Name("h_rx_rcpll_fsm_status"));
    map.add_leaf(s.l_rx_rcpll_lock_status, vtss::tag::Name("l_rx_rcpll_lock_status"));
    map.add_leaf(s.l_rx_rcpll_range, vtss::tag::Name("l_rx_rcpll_range"));
    map.add_leaf(s.l_rx_rcpll_vco_load, vtss::tag::Name("l_rx_rcpll_vco_load"));
    map.add_leaf(s.l_rx_rcpll_fsm_status, vtss::tag::Name("l_rx_rcpll_fsm_status"));
    map.add_leaf(s.h_tx_rcpll_lock_status, vtss::tag::Name("h_tx_rcpll_lock_status"));
    map.add_leaf(s.h_tx_rcpll_range, vtss::tag::Name("h_tx_rcpll_range"));
    map.add_leaf(s.h_tx_rcpll_vco_load, vtss::tag::Name("h_tx_rcpll_vco_load"));
    map.add_leaf(s.h_tx_rcpll_fsm_status, vtss::tag::Name("h_tx_rcpll_fsm_status"));
    map.add_leaf(s.l_tx_rcpll_lock_status, vtss::tag::Name("l_tx_rcpll_lock_status"));
    map.add_leaf(s.l_tx_rcpll_range, vtss::tag::Name("l_tx_rcpll_range"));
    map.add_leaf(s.l_tx_rcpll_vco_load, vtss::tag::Name("l_tx_rcpll_vco_load"));
    map.add_leaf(s.l_tx_rcpll_fsm_status, vtss::tag::Name("l_tx_rcpll_fsm_status"));
    map.add_leaf(s.h_pma, vtss::tag::Name("h_pma"));
    map.add_leaf(s.h_pcs, vtss::tag::Name("h_pcs"));
    map.add_leaf(s.l_pma, vtss::tag::Name("l_pma"));
    map.add_leaf(s.l_pcs, vtss::tag::Name("l_pcs"));
    map.add_leaf(s.wis, vtss::tag::Name("wis"));
}
template<class A>
void serialize(A &a, vtss_phy_10g_prbs_mon_conf_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.enable, vtss::tag::Name("enable"));
    map.add_leaf(s.line, vtss::tag::Name("line"));
    map.add_leaf(s.max_bist_frames, vtss::tag::Name("max_bist_frames"));
    map.add_leaf(s.error_states, vtss::tag::Name("error_states"));
    map.add_leaf(s.des_interface_width, vtss::tag::Name("des_interface_width"));
    map.add_leaf(s.prbsn_sel, vtss::tag::Name("prbsn_sel"));
    map.add_leaf(s.prbs_check_input_invert, vtss::tag::Name("prbs_check_input_invert"));
    map.add_leaf(s.no_of_errors, vtss::tag::Name("no_of_errors"));
    map.add_leaf(s.bist_mode, vtss::tag::Name("bist_mode"));
    map.add_leaf(s.error_status, vtss::tag::Name("error_status"));
    map.add_leaf(s.PRBS_status, vtss::tag::Name("PRBS_status"));
    map.add_leaf(s.main_status, vtss::tag::Name("main_status"));
    map.add_leaf(s.stuck_at_par, vtss::tag::Name("stuck_at_par"));
    map.add_leaf(s.stuck_at_01, vtss::tag::Name("stuck_at_01"));
    map.add_leaf(s.no_sync, vtss::tag::Name("no_sync"));
    map.add_leaf(s.instable, vtss::tag::Name("instable"));
    map.add_leaf(s.incomplete, vtss::tag::Name("incomplete"));
    map.add_leaf(s.active, vtss::tag::Name("active"));
}
template<class A>
void serialize(A &a, vtss_sublayer_status_t &s) {
    typename A::Map map = a.as_map();
    map.add_leaf(s.rx_link, vtss::tag::Name("rx_link"));
    map.add_leaf(s.link_down, vtss::tag::Name("link_down"));
    map.add_leaf(s.rx_fault, vtss::tag::Name("rx_fault"));
    map.add_leaf(s.tx_fault, vtss::tag::Name("tx_fault"));
}
#endif //_SERIALIZE_PHY_IMPL_H_
