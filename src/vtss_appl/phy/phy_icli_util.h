/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _PHY_ICLI_UTIL_H_
#define _PHY_ICLI_UTIL_H_


#ifdef __cplusplus
extern "C" {
#endif
/* ICLI request structure */
typedef struct {
    u32                     session_id;
    icli_stack_port_range_t *port_list;
    icli_unsigned_range_t   *addr_list;
    u32                     count;
    u32                     page;
    u32                     value;
    BOOL                    addr_sort;
    BOOL                    write;
    BOOL                    header;
    BOOL                    dummy;  /* Unused, for Lint */
} phy_icli_req_t;

typedef struct {
    BOOL near;
    BOOL far;
    BOOL connector;
    BOOL mac_serdes_input;
    BOOL mac_serdes_facility;
    BOOL mac_serdes_equipment;
    BOOL media_serdes_input;
    BOOL media_serdes_facility;
    BOOL media_serdes_equipment;
    BOOL qsgmii_tbi;
    BOOL qsgmii_gmi;
    BOOL qsgmii_serdes;
} phy_icli_loopback_t;

void phy_icli_debug_phy(phy_icli_req_t *req);

// Debug function used for testing PHY for simulating cu SFP setup in forced mode.
mesa_rc phy_icli_debug_phy_pass_through_speed(i32 session_id, icli_stack_port_range_t *plist, BOOL has_1g, BOOL has_100M, BOOL has_10M);
mesa_rc phy_icli_debug_phy_mode_set(i32 session_id, icli_stack_port_range_t *plist, BOOL has_media_if, u8 media_if, BOOL has_conf_mode, u8 conf_mode, BOOL has_speed, BOOL has_1g, BOOL has_100M, BOOL has_10M);

// Debug function for enabling/disabling phy page checking. PHY page checking is a mode where the page register is checked for is we are at the right page for all phy registers accesses (This will double the amount of registers access)
mesa_rc phy_icli_debug_do_page_chk(i32 session_id, BOOL has_enable, BOOL has_disable);

/**
 * \brief Debug function for resetting a PHY 
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param plist      [IN] Port list with ports to configure (Not Stack-aware).
 * \return VTSS_RC_OK if PHY were reset correct else error code.
 **/
mesa_rc phy_icli_debug_phy_reset(i32 session_id, icli_stack_port_range_t *plist);

/**
 * \brief Debug function for accessing/configuring PHY GPIOs
 *
 * \param session_id           [IN] The session id use by iCLI print.
 * \param plist                [IN] Port list with ports to configure (Not Stack-aware).
 * \param has_mode_output      [IN] Set to TRUE in order to configure GPIO as output.
 * \param has_mode_input       [IN] Set to TRUE in order to configure GPIO as input.
 * \param has_mode_alternative [IN] Set to TRUE in order to configure GPIO to alternative mode (See datasheet).
 * \param has_gpio_get         [IN] Set to TRUE in order to print the current state for the GPIO.
 * \param has_gpio_set         [IN] Set to TRUE in order to set the current state of the GPIO.
 * \param value                [IN] Set to TRUE in order to set the current state of the GPIO to high (FALSE = set low). 
 * \param gpio_no              [IN] the gpio number to access/configure.
 * \return VTSS_RC_OK if PHY GPIO were accessed/configured correct else error code.
 **/
mesa_rc phy_icli_debug_phy_gpio(i32 session_id, icli_stack_port_range_t *plist, BOOL has_mode_output, BOOL has_mode_input, BOOL has_mode_alternative, BOOL has_gpio_get, BOOL has_gpio_set, BOOL value, u8 gpio_no);

/**
 * \brief Debug function for setting a PHY into local loopback mode 
 *
 * \param session_id                   [IN] The session id use by iCLI print.
 * \param plist                        [IN] Port list with ports to configure (Not Stack-aware).
 * \param lb                           [IN] CLI parameters indicating presence of corresponding loopbacks.
 * \return VTSS_RC_OK if PHY were reset correct else error code.
 **/
mesa_rc phy_icli_debug_phy_loop(i32 session_id, icli_stack_port_range_t *plist, phy_icli_loopback_t lb, BOOL no);
/**
 * \brief Debug function for setting a PHY into ring resiliency mode
 *
 * \param session_id  [IN] The session id use by iCLI print.
 * \param plist       [IN] Port list with ports to configure (Not Stack-aware).
 * \param has_enable  [IN] Set to TRUE to enable ring resiliency
 * \param has_disable [IN] Set to TRUE to disable ring resiliency
 * \param has_get     [IN] Set to TRUE to get ring resiliency state
 * \return VTSS_RC_OK if PHY were reset correct else error code.
 **/
mesa_rc phy_icli_debug_phy_ring_resiliency_conf(i32 session_id, icli_stack_port_range_t *plist, BOOL has_enable,
                                                BOOL has_disable, BOOL has_get);

/**
 * \brief Debug function for setting a PHY LED configuration
 *
 * \param session_id       [IN] The session id use by iCLI print.
 * \param plist            [IN] Port list with ports to configure (Not Stack-aware).
 * \param has_led_num      [IN] Set to TRUE to configure particular LED
 * \param led_num          [IN] Set mode configuration to LED
 * \param has_led_mode     [IN] Set to TRUE to configure LED mode
 * \param led_mode         [IN] Set LED mode configuration
 * \return VTSS_RC_OK if PHY were reset correct else error code.
 **/
mesa_rc phy_icli_debug_phy_led_set(i32 session_id, icli_stack_port_range_t *plist, BOOL has_led_num, u8 led_num, BOOL has_led_mode, u8 led_mode);

mesa_rc phy_icli_dbg_phy_clause45_access(i32 session_id, icli_stack_port_range_t *plist, uint16_t page, uint16_t address, BOOL read, uint16_t value, uint16_t count);

mesa_rc phy_icli_dbg_phy_qsgmii_aneg_set(i32 session_id, icli_stack_port_range_t *plist, BOOL enable);

mepa_rc phy_icli_dbg_event_conf_status(i32 session_id, icli_stack_port_range_t *plist, BOOL has_set, BOOL enable, uint32_t events, BOOL has_get, BOOL has_poll);

mesa_rc phy_ts_mepa_eth_class_set(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t flow_id, mepa_bool_t pbb_en, bool has_etype, uint16_t etype, bool has_tpid, uint16_t tpid, mepa_ts_mac_match_select_t mac_match_select, mepa_ts_mac_match_mode_t mac_match_mode, bool has_mac, mesa_mac_t mac_addr, mepa_bool_t vlan_chk, bool has_tag_cnt, uint8_t num_tag, bool has_out_tag, bool has_out_rng, uint16_t out_rng_low, uint16_t out_rng_hi, bool has_out_val, uint16_t out_val, uint16_t out_mask, bool has_in_tag, bool has_in_rng, uint16_t in_rng_low, uint16_t in_rng_hi, bool has_in_val, uint16_t in_val, uint16_t in_mask);

mesa_rc phy_ts_mepa_ip_class_set(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t flow_id, bool has_udp_src_port, uint16_t udp_src_port, bool has_udp_dst_port, uint16_t udp_dst_port, mepa_ts_ip_match_select_t ip_match_mode, bool has_ipv4, mesa_ipv4_t ipv4_addr,  mesa_ipv4_t ipv4_mask, BOOL has_ipv6, mesa_ipv6_t ipv6_addr, mesa_ipv6_t ipv6_mask);

mesa_rc phy_ts_mepa_clock_set(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t clock_id, BOOL enable, bool has_clk_type, mepa_ts_ptp_clock_mode_t clk_mode, bool has_delay_type, mepa_ts_ptp_delaym_type_t delaym_type, bool has_cf_update, mepa_bool_t cf_update, bool has_version_rng, uint8_t ver_low, uint8_t ver_hi, bool has_minor_ver, uint8_t min_ver_low, uint8_t min_ver_hi, bool has_domain, bool has_domain_range, uint8_t domain_rng_low, uint8_t domain_rng_hi, bool has_domain_val, uint8_t domain_val, uint8_t domain_mask, bool has_sdoid, bool has_sdoid_rng, uint8_t sdoid_rng_lo, uint8_t sdoid_rng_hi, bool has_sdoid_val, uint8_t sdoid_val, uint8_t sdoid_mask);

mesa_rc phy_ts_mepa_class_comm_set(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t flow_id, mepa_ts_pkt_encap_t encap, uint8_t clock_id);

mepa_rc phy_ts_mepa_class_get(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t flow_id, bool has_eth, bool has_ip);

mepa_rc phy_ts_mepa_clock_get(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t clock_id);

mepa_rc phy_ts_mepa_port_mode_set(i32 session_id, icli_stack_port_range_t *plist, BOOL enable);

mepa_rc phy_ts_mepa_fifo_empty(i32 session_id, icli_stack_port_range_t *plist, BOOL fifo_read);

mepa_rc phy_icli_dbg_phy_ts_mepa_init_set( i32 session_id, icli_stack_port_range_t *plist, bool has_clk_freq, bool  has_clk_src, bool has_rx_ts_pos, bool has_rx_ts_len, bool has_tx_ts_len, bool has_ls_clear, bool has_tc_mode, bool has_dly_req_rcv_10, mepa_ts_clock_freq_t clk_freq, mepa_ts_clock_src_t clk_src, mepa_ts_rx_timestamp_pos_t rx_ts_pos, mepa_ts_rx_timestamp_len_t rx_ts_len, mepa_ts_fifo_timestamp_len_t  tx_ts_len, mepa_bool_t clear, mepa_ts_tc_op_mode_t tc_op_mode, mepa_bool_t dly_req_recv_10byte_ts, mepa_bool_t auto_ts);

mepa_rc phy_icli_show_sqi_value( i32 session_id, icli_stack_port_range_t *plist );

mesa_rc phy_icli_dbg_phy_mdi(i32 session_id, icli_stack_port_range_t *plist, bool has_mdi, bool has_mdix, bool has_auto);

mepa_rc  phy_icli_dbg_phy_start_of_frame_pulses(i32 session_id, icli_stack_port_range_t *plist, mepa_bool_t ingress, bool has_dir, bool has_sofno, uint8_t sofno, bool has_start_of_frame_preemption_val, mepa_preemption_mode_t preemption);

mesa_rc phy_icli_dbg_phy_frame_preemption(i32 session_id, icli_stack_port_range_t *plist, bool has_enable, bool has_disable);

mesa_rc phy_icli_dbg_phy_selftest(i32 session_id, icli_stack_port_range_t *plist, mepa_port_speed_t speed, mepa_media_mode_t mdi, uint32_t frames, bool has_start, bool has_read);

mesa_rc phy_icli_crc_gen(i32 session_id, char *input_clk, uint16_t port_id);

mepa_rc phy_icli_dbg_phy_prbs(i32 session_id, icli_stack_port_range_t *plist, bool enable, mepa_phy_prbs_type_t type, mepa_phy_prbs_direction_t direction, mepa_prbs_pattern_t prbsn_sel, bool is_set, mepa_prbs_clock_t clk, mepa_prbs_loopback_t loopback);

mepa_rc phy_icli_show_prbs_checker_value(i32 session_id, icli_stack_port_range_t *plist, mepa_prbs_pattern_t prbsn_sel, bool has_inject, bool has_value);

#ifdef __cplusplus
}
#endif

#endif /* _PHY_ICLI_UTIL_H_ */
