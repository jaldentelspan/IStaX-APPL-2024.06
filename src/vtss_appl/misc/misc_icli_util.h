/*

 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _MISC_ICLI_UTIL_H_
#define _MISC_ICLI_UTIL_H_

#define MISC_CHIP_NO_NONE (VTSS_CHIP_NO_ALL - 1)

#ifdef __cplusplus
extern "C" {
#endif
/* ICLI request structure */

typedef struct {
    u32                     session_id;
    icli_stack_port_range_t *port_list;
    mesa_chip_no_t          chip_no;
    mesa_debug_info_t       debug_info;
    BOOL                    resume;
    BOOL                    dummy; /* Unused, for Lint */
    u32                     value;
} misc_icli_req_t;

void misc_icli_cmd_apc_restart (BOOL has_host, icli_stack_port_range_t *v_port_type_list);
void misc_icli_chip(misc_icli_req_t *req);
void misc_icli_debug_api(misc_icli_req_t *req);
void misc_icli_suspend_resume(misc_icli_req_t *req);
void phy_1g_token_ring_read( u32 session_id,icli_stack_port_range_t *v_port_type_list, u32 value);
icli_rc_t misc_icli_test(misc_icli_req_t *req);

mesa_rc phy_icli_1g_phy_extended_event_set(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_poll, BOOL has_set, BOOL has_clear, u32 phy_xtnd_event);

mesa_rc phy_fifo_sync(u32 session_id, icli_stack_port_range_t *v_port_type_list);
#if defined(VTSS_OPT_TS_SPI_FPGA)
mesa_rc misc_icli_spi_daisy_chaining_timestamp(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_output,
                                               BOOL has_output_enable, BOOL has_output_disable, BOOL has_input,
                                               BOOL has_input_enable, BOOL has_input_disable);
void misc_icli_cmd_debug_phy_1588_fifo_status(u32 session_id);
#if defined(VTSS_OPT_TS_SPI_FPGA) && defined(VTSS_SW_OPTION_SYNCE)
void misc_icli_cmd_debug_phy_1588_timestamp_show(u32 session_id, icli_stack_port_range_t *plist, u16 no_of_ts);
void misc_icli_cmd_debug_phy_1588_fifo_clear(u32 session_id);
#endif // VTSS_SW_OPTION_SYNCE
#endif //VTSS_OPT_TS_SPI_FPGA


void misc_icli_10g_kr_conf(u32 session_id, BOOL has_cm1, i32 cm_1, BOOL has_c0, i32 c_0, BOOL has_cp1, i32 c_1, 
                           BOOL has_ampl, u32 amp_val, BOOL has_ps25, BOOL has_ps35, BOOL has_ps55, BOOL has_ps70,
                           BOOL has_ps120, BOOL has_en_ob, BOOL has_dis_ob, BOOL has_ser_inv, BOOL has_ser_no_inv,
                           icli_stack_port_range_t *v_port_type_list, BOOL has_host);

mesa_rc phy_icli_10g_event_enable_set(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                      u32 phy_10g_events, BOOL has_enable, BOOL has_disable);
mesa_rc phy_icli_10g_event_enable_poll(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                       u32 phy_10g_events);
void misc_icli_10g_phy_clause37_abil(u32 session_id, BOOL has_host, BOOL has_line, BOOL has_aneg_enable, 
                                      BOOL has_aneg_disable,  icli_stack_port_range_t *v_port_type_list);
void misc_icli_10g_phy_pcs_sticky_poll(u32 session_id, icli_stack_port_range_t *v_port_type_list);
void misc_icli_10g_phy_kr_train_autoneg(u32 session_id, BOOL has_enable, BOOL has_disable,
        BOOL has_line, BOOL has_host,vtss_phy_10g_base_kr_ld_adv_abil_t *adv_abil,vtss_phy_10g_base_kr_training_t *training,
        icli_stack_port_range_t *v_port_type_list);
mesa_rc phy_icli_10g_extended_event_enable_set(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                               u32 xtnd_events, BOOL has_enable, BOOL has_disable);
mesa_rc phy_icli_10g_extended_event_enable_poll(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                                u32 xtnd_events);
mesa_rc misc_phy_10g_prbs_generator_conf ( u32 session_id, icli_stack_port_range_t *v_port_type_list, mesa_bool_t set, vtss_phy_10g_prbs_type_t type,  vtss_phy_10g_direction_t direction,  vtss_phy_10g_prbs_generator_conf_t *const conf);
mesa_rc misc_phy_10g_prbs_monitor_conf ( u32 session_id, icli_stack_port_range_t *v_port_type_list,  mesa_bool_t set, vtss_phy_10g_prbs_type_t type, vtss_phy_10g_direction_t direction, vtss_phy_10g_prbs_monitor_conf_t *const conf_prbs);
mesa_rc misc_phy_10g_monitor_status( u32 session_id, icli_stack_port_range_t *v_port_type_list, vtss_phy_10g_prbs_type_t type, vtss_phy_10g_direction_t direction, mesa_bool_t reset);
mesa_rc vtss_phy_10g_vscope_scan_icli(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_fast_scan, 
		BOOL has_fast_scan_plus, BOOL has_host, BOOL has_line, u32 v_0_to_10000, BOOL has_enable, BOOL has_disable, BOOL has_xy_scan , 
		BOOL has_host_1, BOOL has_line_1, BOOL has_enable_1, u32 v_0_to_127, u32 v_0_to_63, u32 v_0_to_127_1, u32 v_0_to_63_1, 
		u32 v_0_to_10, u32 v_0_to_10_1, u32 v_0_to_64,u32 v_0_to_10000_1, BOOL has_disable_1);
mesa_rc vtss_phy_10g_prbs_counters( u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_line);
mesa_rc vtss_phy_10g_prbs_generator ( u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_line,
                                              BOOL has_host, u32 prbsn, BOOL has_enable, BOOL has_disable);
mesa_rc vtss_phy_10g_prbs_monitor( u32 session_id, icli_stack_port_range_t*v_port_type_list, BOOL has_line,
        BOOL has_host, u16 prbsn, u16 max_bist_frames, u16 error_states, u16 des_interface_width,
        BOOL has_input_invert, u16 no_of_errors, u16 bist_mode, BOOL has_enable, BOOL has_disable);

mesa_rc misc_icli_10g_phy_pkt_generator(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_enable,
                                        BOOL has_ingress, BOOL has_egress, BOOL has_frames,
                                        BOOL has_ptp,u8 ts_sec, u8 ts_ns, u8 srate, BOOL has_stand, u16 ethtype, u8 pktlen, u32 intpktgap,mesa_mac_t srcmac,
                                        mesa_mac_t desmac, BOOL has_idle, BOOL has_disable);
mesa_rc misc_icli_10g_phy_pkt_monitor_conf(u32 session_id, icli_stack_port_range_t *plist, BOOL has_enable, BOOL has_update, BOOL has_reset, vtss_phy_10g_pkt_mon_rst_t mon_rst, BOOL has_disable, BOOL has_timestamp);

mesa_rc misc_icli_auto_failover(u32 session_id, icli_stack_port_range_t *plist, u16 des_ch, u16 source_ch, BOOL has_host, vtss_phy_10g_auto_failover_event_t event, BOOL has_enable, BOOL is_set, BOOL has_filter, u8 filter_type, u16 filter_val, u16 actual_pin, u16 virtual_pin);  

mesa_rc misc_icli_gpio_set_get(u32 session_id, icli_stack_port_range_t *plist, u16 gpio_no, BOOL has_mode, u8 mode, BOOL has_port_no, u32 port_no, u8 value, BOOL blink_time, BOOL has_pgpio_no, u16 int_gpio, BOOL has_internal_signal, u8 int_sign, BOOL has_source, u16 source, BOOL has_channel_interrupt, u8 c_intrp, BOOL has_gpio_port_interrupt, BOOL has_set, BOOL has_unset, BOOL has_aggr_interrupt, u32 aggr_mask,BOOL has_gpio_input, u8 input,BOOL use_for_GPIO_interrupt);

 
void misc_icli_10g_phy_loopback(u32 session_id, BOOL has_a, BOOL has_b, BOOL has_c, BOOL has_d, BOOL has_e, BOOL has_f, 
                                BOOL has_g, BOOL has_h, BOOL has_j, BOOL has_k, BOOL has_h2, BOOL has_h3, BOOL has_h4, 
                                BOOL has_h5, BOOL has_h6, BOOL has_l0, BOOL has_l1, BOOL has_l2, BOOL has_l3, BOOL has_l2c, 
                                BOOL has_enable, BOOL has_disable, 
                                icli_stack_port_range_t *v_port_type_list);

void misc_icli_10g_phy_mode(u32 session_id, BOOL has_mode,u8 mode, BOOL lane,u8 v_0_to_5, BOOL has_host_intf,u8 host_if,
                            u8 ddr_mode, BOOL has_host,BOOL has_clk_src,BOOL has_href,BOOL has_media_type,
                            u8 media_type,BOOL has_sgmii_pass_thru,BOOL has_enable,BOOL has_amp_tol,BOOL has_high,icli_stack_port_range_t *plist,
                            BOOL has_link_6g_distance, BOOL has_long_range, BOOL has_apc_line_ld_ctrl,
                            BOOL has_ld_lev_ini, u8 v_0_0xff, BOOL has_apc_offs_ctrl, BOOL has_eqz_offs_par_cfg, u32 v_0_0xffffffff);
void misc_icli_10g_phy_ib_conf(u32 session_id, vtss_phy_10g_ib_conf_t *ibconf, icli_stack_port_range_t *v_port_type_list,BOOL is_set,BOOL is_host, BOOL has_prbs);
void misc_icli_10g_phy_apc_conf(u32 session_id, vtss_phy_10g_apc_conf_t *apcconf, icli_stack_port_range_t *v_port_type_list,BOOL is_set,BOOL is_host);
void misc_icli_10g_phy_serdes_status(u32 session_id, vtss_phy_10g_serdes_status_t *serdes, icli_stack_port_range_t *v_port_type_list);
void misc_icli_phy_10g_jitter_conf(u32 session_id, BOOL set,BOOL has_host,u8 levn_val,BOOL has_yes,u8 vtail_val,icli_stack_port_range_t * port_list);
BOOL misc_icli_10g_phy_failover(u32 session_id, BOOL has_a, BOOL has_b, BOOL has_c, BOOL has_d, 
                                BOOL has_e, BOOL has_f, BOOL has_get,
                                icli_stack_port_range_t *v_port_type_list);

BOOL misc_icli_10g_phy_reset(u32 session_id, icli_stack_port_range_t *v_port_type_list);

BOOL misc_icli_10g_phy_status(u32 session_id, icli_stack_port_range_t *v_port_type_list);

BOOL misc_icli_10g_phy_fw_status(u32 session_id, icli_stack_port_range_t *v_port_type_list);
void misc_icli_phy_10g_fc_buffer_reset(u32 session_id, icli_stack_port_range_t *v_port_type_list);

BOOL misc_icli_10g_phy_gpio(u32 session_id, icli_stack_port_range_t *plist, BOOL has_mode_output, 
                            BOOL has_mode_input, BOOL has_mode_alternative, BOOL has_gpio_get, 
                            BOOL has_gpio_set, BOOL value, u8 gpio_no);

BOOL misc_icli_10g_phy_power(u32 session_id, BOOL has_enable, BOOL has_no,
                               BOOL has_show, icli_stack_port_range_t *v_port_type_list);

BOOL misc_icli_10g_phy_rd_wr_i2c(u32 session_id, icli_stack_port_range_t * plist, 
                                 u8 address, u16 address_1, BOOL has_read_i2c,
                                 BOOL has_write_i2c, u8 val);




void misc_icli_10g_phy_rxckout(u32 session_id, BOOL has_disable, BOOL has_rx_clock, BOOL has_tx_clock, 
                               BOOL has_pcs_fault_squelch, BOOL has_no_pcs_fault_squelch, 
                               BOOL has_lopc_squelch, BOOL has_no_lopc_squelch, 
                               icli_stack_port_range_t *v_port_type_list);

void misc_icli_10g_phy_txckout(u32 session_id, BOOL has_disable, BOOL has_rx_clock, BOOL has_tx_clock, 
                               icli_stack_port_range_t *v_port_type_list);

void misc_icli_10g_phy_ckout(u32 session_id, BOOL has_enable, BOOL has_disable, BOOL has_use_squelch_src_as_is, 
                               BOOL has_invert_squelch_src, BOOL has_Squelch_src, 
                               vtss_phy_10g_squelch_src_t Squelch_src, BOOL has_full_rate_clk, 
                               BOOL has_div_by_2_clk, BOOL has_clk_out_src, 
                               vtss_ckout_data_sel_t clk_out_src,
                               icli_stack_port_range_t *v_port_type_list);
void misc_icli_10g_phy_sckout(u32 session_id, BOOL has_enable, BOOL has_disable, BOOL has_frequency,
                               vtss_phy_10g_sckout_freq_t freq, BOOL has_use_squelch_src_as_is,
                               BOOL has_invert_squelch_src, BOOL has_Squelch_src, 
                               vtss_phy_10g_squelch_src_t Squelch_src, BOOL has_sckout_clkout_sel, 
                               vtss_phy_10g_clk_sel_t sckout_clkout_sel,
                               icli_stack_port_range_t *v_port_type_list);
void misc_icli_10g_phy_clk_sel(u32 session_id, BOOL has_line, BOOL has_host, BOOL has_clk_sel_no,
                               u8 clk_sel_no, BOOL has_clk_sel_val,
                               vtss_phy_10g_clk_sel_t clk_sel_val,
                               icli_stack_port_range_t *v_port_type_list);
void misc_icli_10g_phy_lane_sync(u32 session_id, BOOL has_enable, BOOL has_disable,
                                BOOL has_rx_macro, vtss_phy_10g_rx_macro_t rx_macro,
                                BOOL has_tx_macro, vtss_phy_10g_tx_macro_t tx_macro,
                                u8 rx_chid, u8 tx_chid,
                               icli_stack_port_range_t *v_port_type_list);
void misc_icli_spi_transaction(u32 session_id, u32 spi_cs, u32 cs_active_high, u32 gpio_mask, u32 gpio_value, u32 no_of_bytes, icli_hexval_t *v_hexval);

mesa_rc misc_icli_phy_aqr_mmd_dump_direct(u32 session_id, mesa_miim_controller_t miim_ctrl, u8 miim_addr);
mesa_rc misc_icli_phy_mmd_read_direct(u32 session_id, mesa_miim_controller_t miim_ctrl, u8 miim_addr,                                                   u16 dev_addr, u16 mmd_reg_addr);
mesa_rc misc_icli_phy_mmd_write_direct(u32 session_id, mesa_miim_controller_t miim_ctrl, u32 miim_addr,                                                  u32 dev_addr, u32 mmd_reg_addr, u16 value);
mesa_rc misc_icli_phy_csr_read_write(u32 session_id,icli_stack_port_range_t *plist,u8 dev,u16 addr,u32 value,BOOL is_read, icli_unsigned_range_t *addr_list);

BOOL misc_icli_10g_phy_synce_mode(u32 session_id, BOOL has_mode, u8 mode, BOOL has_clk_out, u8 clk_out, BOOL has_hitless, \
                               u8 hitless, BOOL has_rclk_div, u8 rclk_div, BOOL has_sref_div, u8 sref_div, BOOL has_wref_div, \
                               u8 wref_div, icli_stack_port_range_t *v_port_type_list);

BOOL misc_icli_10g_phy_srefclk(u32 session_id, BOOL has_enable, BOOL has_no,
                               BOOL has_show, icli_stack_port_range_t *v_port_type_list, BOOL has_freq, BOOL freq125, BOOL freq156_25, BOOL freq_155_52);

BOOL misc_icli_10g_phy_synce_clkout(u32 session_id, BOOL has_enable, BOOL has_disable, icli_stack_port_range_t *v_port_type_list);

BOOL misc_icli_10g_phy_xfp_clkout(u32 session_id, BOOL has_enable, BOOL has_disable, icli_stack_port_range_t *v_port_type_list);

mesa_rc misc_phy_icli_cmd_tod_time(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_time_sec, u8 value);
mesa_rc phy_icli_1588_event_enable_set(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_get,
                                       BOOL has_set, BOOL has_clear, u16 ts_event);
mesa_rc phy_icli_cmd_ltc_freq_synth(i32 session_id, icli_stack_port_range_t *plist, BOOL has_enable, BOOL has_disable,
                                    u8 High_duty_cycle, u8 Low_duty_cycle);
void misc_icli_deb_phy_ts_mmd_read(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 blk_id, u16 address);
void misc_icli_deb_phy_ts_mmd_write(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 blk_id, u16 address, u32 value);
void misc_icli_deb_phy_ts_engine_init(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id,BOOL ingress, u8 encap_type, u8 flow_st_index, u8 flow_end_index, u8 flow_match);
void misc_icli_deb_phy_ts_engine_clear(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress);
void misc_icli_deb_phy_ts_correction_field_clear(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 action_id, u8 msg_type);
void misc_icli_deb_phy_ts_engine_mode(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL enable);
void misc_icli_ts_engine_chan_map(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_flow, u8 flow_id, BOOL has_mask, u8 mask);
void misc_icli_ts_engine_eth1_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_pbb, BOOL pbb_en, BOOL has_etype, u16 etype, BOOL has_tpid, u16 tpid);
void misc_icli_deb_phy_ts_engine_eth1_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, BOOL has_mac_match_mode, u8 mac_match_mode, BOOL has_address, mesa_mac_t mac_addr, BOOL has_match_addr_types, u8 src_dest_match, BOOL has_vlan_chk, BOOL vlan_chk, BOOL has_tag_rng, u8 tag_rng, u8 has_num_tag, u8 num_tag, BOOL has_tag1_type, u8 tag1_type, BOOL has_tag2_type, u8 tag2_type, BOOL has_tag1_low, u16 tag1_low, BOOL has_tag1_up, u16 tag1_up, BOOL has_tag2_low, u16 tag2_low, BOOL has_tag2_up, u16 tag2_up);
void misc_icli_ts_engine_eth2_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_etype, u16 etype, BOOL has_tpid, u16 tpid);
void misc_icli_deb_phy_ts_engine_eth2_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, BOOL has_mac_match_mode, u8 mac_match_mode, BOOL has_address, mesa_mac_t mac_addr, BOOL has_match_addr_types, u8 src_dest_match, BOOL has_vlan_chk, BOOL vlan_chk, BOOL has_tag_rng, u8 tag_rng, u8 has_num_tag, u8 num_tag, BOOL has_tag1_type, u8 tag1_type, BOOL has_tag2_type, u8 tag2_type, BOOL has_tag1_low, u16 tag1_low, BOOL has_tag1_up, u16 tag1_up, BOOL has_tag2_low, u16 tag2_low, BOOL has_tag2_up, u16 tag2_up);
void misc_icli_deb_phy_ts_engine_ip1_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_ipmode, u8 ip_mode, u16 sport_val, u16 sport_mask, u16 dport_val, u16 dport_mask);
void misc_icli_deb_phy_ts_ip1_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, BOOL has_match_addr_types, u8 src_dest_match, BOOL has_ipv4, mesa_ipv4_t ipv4_addr, mesa_ipv4_t ipv4_mask, BOOL has_ipv6, mesa_ipv6_t ipv6_addr, mesa_ipv6_t ipv6_mask);
void misc_icli_deb_phy_ts_engine_ip2_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_ipmode, u8 ip_mode, u16 sport_val, u16 sport_mask, u16 dport_val, u16 dport_mask);
void misc_icli_deb_phy_ts_ip2_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, BOOL has_match_addr_types, u8 src_dest_match, BOOL has_ipv4, mesa_ipv4_t ipv4_addr, mesa_ipv4_t ipv4_mask, BOOL has_ipv6, mesa_ipv6_t ipv6_addr, mesa_ipv6_t ipv6_mask);
void misc_icli_deb_phy_ts_mpls_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL cw_en);
void misc_icli_deb_phy_ts_engine_mpls_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, u8 stk_depth, BOOL has_stk_ref, u8 stk_ref_point, BOOL has_stk_lvl_0, icli_unsigned_range_t *stk_lvl_0, BOOL has_stk_lvl_1, icli_unsigned_range_t *stk_lvl_1, BOOL has_stk_lvl_2, icli_unsigned_range_t *stk_lvl_2, BOOL has_stk_lvl_3, icli_unsigned_range_t *stk_lvl_3);
void misc_icli_deb_phy_ts_engine_ach_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u16 ach_ver, u16 channel_type, u16 proto_id);
void misc_icli_deb_phy_ts_engine_generic_comm_conf( u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u32 next_proto_offset, u8 flow_offset);
void misc_icli_deb_phy_ts_generic_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, u32 data_upper, u32 mask_upper, u32 data_upper_mid, u32 mask_upper_mid, u32 data_lower_mid, u32 mask_lower_mid, u32 data_lower, u32 mask_lower);
void misc_icli_ts_engine_add_action(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 action_id, u8 channel_map, BOOL has_ptp, u8 clk_mode, u8 delaym, u8 domain_lower, u8 domain_upper,BOOL has_delay_req_ts, BOOL has_y1731, BOOL has_ietf, u8 ietf_ds, BOOL has_generic, u8 flow_id, u32 gen_data_upper, u32 gen_data_lower, u32 gen_mask_upper, u32 gen_mask_lower, u8 ts_type, u8 ts_offset);
void misc_icli_deb_phy_ts_engine_action_del(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 action_id);
mesa_rc misc_icli_ts_latency(i32 session_id, BOOL has_ingress,BOOL has_egress,BOOL has_latency,u32 v_0_to_65535,icli_stack_port_range_t *plist);
mesa_rc misc_icli_ts_nphase(i32 session_id,BOOL has_nphase,u8 value,icli_stack_port_range_t *plist);
mesa_rc misc_icli_ts_asym_delay(i32 session_id,BOOL has_asym, i32 value, icli_stack_port_range_t *plist);
void misc_icli_deb_phy_ts_signature(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                    BOOL has_sig_mask, u16 sig_mask);
void misc_icli_deb_phy_ts_block_init(u32 session_id, icli_stack_port_range_t *v_port_type_list,BOOL has_clk_freq, u8 clk_freq, BOOL has_clk_src, u8 clk_src, BOOL has_rx_ts_pos, u8 rx_ts_pos, BOOL has_tx_fifo_mode, u8 tx_fifo_mode, BOOL has_tx_spi_conf, BOOL has_tx_fifo_hi_clk_cycs, u8 value, BOOL has_tx_fifo_lo_clk_cycs, u8 value_1, BOOL has_modify_frm, u8 modify_frm);
void misc_icli_deb_phy_ts_statistics(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 time_sec);
mesa_rc misc_icli_ts_path_delay(u32 session_id, BOOL has_delay,mesa_timeinterval_t value,icli_stack_port_range_t *plist);
void misc_icli_show_phy_ts_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_eng_init, u8 flow_id, BOOL has_comm_conf, BOOL has_eth1, BOOL has_eth2, BOOL has_ip1, BOOL has_ip2, BOOL has_mpls, BOOL has_ach, BOOL has_gen_ts, BOOL has_flow_conf, BOOL has_eth1_1, BOOL has_eth2_1, BOOL has_ip1_1, BOOL has_ip2_1, BOOL has_mpls_1, BOOL has_gen_ts_1, BOOL has_action, BOOL has_ptp, BOOL has_oam, BOOL has_generic);

mesa_rc misc_icli_debug_macsec_sd6g_csr_read_write(u32 session_id,icli_stack_port_range_t *plist,u16 target,u32 addr,u32 value,BOOL is_read);
mesa_rc misc_icli_wis_txtti(u32 session_id, BOOL has_set, u8 overhead, vtss_ewis_tti_mode_t tti_mode, const char *tti, icli_stack_port_range_t *plist);
mesa_rc misc_icli_wis_reset(u32 session_id, icli_stack_port_range_t *plist);
mesa_rc misc_icli_wis_atti(u32 session_id, BOOL has_overhead, u8 overhead_value, icli_stack_port_range_t *plist);
 
mesa_rc misc_icli_wis_force_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL set,
                                  u8 v_0_to_3, u8 v_0_to_3_1, u8 v_0_to_3_2);
mesa_rc misc_icli_wis_event_force_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, vtss_ewis_event_t ev_force,
                                        BOOL has_enable, BOOL  has_disable);
mesa_rc misc_icli_wis_conse_act(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_enable,
                                                      u8 wis_aisl, u8 wis_rdil, u16 wis_fault);
mesa_rc misc_icli_wis_status(u32 session_id, icli_stack_port_range_t *plist);
mesa_rc misc_icli_wis_mode(u32 session_id, BOOL enable , BOOL disable, icli_stack_port_range_t *plist);
void misc_icli_wis_counters(u32 session_id, icli_stack_port_range_t *v_port_type_list);
void misc_icli_wis_defects(u32 session_id, icli_stack_port_range_t *v_port_type_list);
mesa_rc misc_icli_wis_perf_counters_get(u32 session_id, icli_stack_port_range_t *plist);

mesa_rc misc_cli_wis_prbs31_err_inj_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                          BOOL has_single_erro, BOOL has_sat_erro);
mesa_rc misc_icli_wis_test_mode(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_loopback,
                                BOOL has_no_loopback, BOOL has_gen_dis, BOOL has_gen_sqr,
                                BOOL has_gen_prbs31, BOOL has_gen_mix, BOOL has_ana_dis, BOOL has_ana_prbs31,
                                BOOL has_ana_mix);
mesa_rc misc_icli_wis_test_status(u32 session_id, icli_stack_port_range_t *v_port_type_list);
mesa_rc misc_cli_wis_tx_perf_thr_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_set,
                                      u32 wis_n_ebc_thr_s, u32 wis_n_ebc_thr_l, u32 wis_f_ebc_thr_l,
                                      u32 wis_n_ebc_thr_p, u32 wis_f_ebc_thr_p);

mesa_rc misc_icli_wis_tx_overhead_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL  has_set,
                                       BOOL has_all, BOOL has_sec_oh, BOOL has_line_oh, BOOL has_path_oh, BOOL has_d1_d3,
                                       BOOL has_sec_ord, BOOL has_suc, BOOL  has_res_sg, BOOL has_d4_d12, BOOL has_line_ord,
                                       BOOL has_aps_rdil, BOOL has_sync, BOOL has_res_lg, BOOL has_c2pl, BOOL has_puc,
                                       BOOL has_ptcm, BOOL has_res_pg,  const char *oh_value_1);

mesa_rc misc_icli_pagemap_show(u32 session_id, char *section, int pid, const char *perm, BOOL details, BOOL changes_only);
mesa_rc misc_icli_vmstat_show(u32 session_id, BOOL changes_only);
#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
mesa_rc misc_icli_pgfaults_show(u32 session_id);
#endif
u32     misc_icli_thread_stack_size_get(int thread_id);
mesa_rc misc_icli_slabinfo_show(u32 session_id, BOOL changes_only);
mesa_rc misc_icli_debug_port_state(u32 session_id, icli_stack_port_range_t *plist, BOOL enable);
mesa_rc zl40251_initialization();
mesa_rc vtss_vscope_scan_icli(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_fast_scan,
                              u32 v_0_to_10000, BOOL has_enable, BOOL has_disable, BOOL has_xy_scan, BOOL has_enable_1,
                              u32 v_0_to_127, u32 v_0_to_63, u32 v_0_to_127_1, u32 v_0_to_63_1, u32 v_0_to_10,
                              u32 v_0_to_10_1, u32 v_0_to_64, u32 v_0_to_10000_1, BOOL has_disable_1);
#ifdef __cplusplus
}
#endif


#endif /* _MISC_ICLI_UTIL_H_ */
