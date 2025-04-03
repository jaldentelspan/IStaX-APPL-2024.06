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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

/**
 * \file
 * \brief PTP icli functions
 * \details This header file describes ptp control functions
 */

#ifndef VTSS_ICLI_PTP_H
#define VTSS_ICLI_PTP_H

#include "icli_api.h"

//#define PTP_UNIT_TEST 1

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief Function for show div. ptp information
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \param has_xxx    [IN]  true if corresponding part of PTP data has to be displayed.
 * \return None.
 **/
icli_rc_t ptp_icli_show(i32 session_id, int clockinst, BOOL has_default, BOOL has_current, BOOL has_parent, BOOL has_time_property,
                        BOOL has_filter, BOOL has_servo, BOOL has_clk, BOOL has_ho, BOOL has_uni,
                        BOOL has_master_table_unicast, BOOL has_slave, BOOL has_details, BOOL has_port_state, BOOL has_port_statistics, BOOL has_port_ds, BOOL has_wireless,
                        BOOL has_foreign_master_record, BOOL has_interface, icli_stack_port_range_t *v_port_type_list, BOOL has_log_state);

/**
 * \brief Function for show external clock mode
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \return None.
 **/
icli_rc_t ptp_icli_ext_clock_mode_show(i32 session_id);

/**
 * \brief Function for show rs422 external clock mode
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \return None.
 **/
icli_rc_t ptp_icli_rs422_clock_mode_show(i32 session_id);

/**
 * \brief Function for show rs422 port configuration
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \return None.
 **/
icli_rc_t ptp_icli_rs422_clock_mode_show_baudrate(i32 session_id);

/**
 * \brief Function for calibrating the timing plane of a PTP port
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param iport      [IN]  The iport index of the port to be calibrated
 * \param has_ext    [IN]  Specifies that external loopback is to be used
 * \param has_int    [IN]  Specifies that internal loopback is to be used
 * \return None.
 **/
icli_rc_t ptp_icli_cal_t_plane(i32 session_id, u32 iport, BOOL has_ext, BOOL has_int);

/**
 * \brief Function for calibrating the timing variation between two ports
 *
 * \param session_id    [IN]  Needed for being able to print error messages
 * \param ref_iport     [IN]  The iport index of the ref port
 * \param other_iport   [IN]  The iport index of the port to be calibrated
 * \param cable_latency [IN]  Specifies the latency of the cable used for calibration
 * \return None.
 **/
icli_rc_t ptp_icli_cal_p2p(i32 session_id, u32 ref_iport, u32 other_iport, i32 cable_latency);

/**
 * \brief Function for calibrating port to external reference using 1PPS
 *
 * \param session_id    [IN]  Needed for being able to print error messages
 * \param iport         [IN]  The iport index of the port to be calibrated
 * \param has_synce     [IN]  True if the DPLL frequency shall be controlled by SyncE
 * \return None.
 **/
icli_rc_t ptp_icli_cal_port_start(i32 session_id, u32 iport, BOOL has_synce);

/**
 * \brief Function for calibrating offset of 1PPS output with port locked to external reference using 1PPS input
 *
 * \param session_id    [IN]  Needed for being able to print error messages
 * \param iport         [IN]  The iport index of the port to be calibrated
 * \param pps_offset    [IN]  Measured 1PPS offset from master to slave (positive value means slave outputs 1PPS after master)
 * \param cable_latency [IN]  Specifies the latency of the cable used for calibration
 * \return None.
 **/
icli_rc_t ptp_icli_cal_port_offset(i32 session_id, u32 iport, i32 pps_offset, i32 cable_latency);

/**
 * \brief Function for resetting the PTP calibration of a port
 *
 * \param session_id    [IN]  Needed for being able to print error messages
 * \param iport         [IN]  The iport index of the port to be calibrated
 * \param has_10m_cu    [IN]  True if specifically the calibration of the 10M (CU) mode shall be reset
 * \param has_100m_cu   [IN]  True if specifically the calibration of the 100M (CU) mode shall be reset
 * \param has_1g_cu     [IN]  True if specifically the calibration of the 1G (CU) mode shall be reset
 * \param has_1g        [IN]  True if specifically the calibration of the 1G (SFP) mode shall be reset
 * \param has_2g5       [IN]  True if specifically the calibration of the 2G5 mode shall be reset
 * \param has_5g        [IN]  True if specifically the calibration of the 5G mode shall be reset
 * \param has_10g       [IN]  True if specifically the calibration of the 10G mode shall be reset
 * \param has_25g       [IN]  True if specifically the calibration of the 25G mode shall be reset
 * \param has_25g_rsfec [IN]  True if specifically the calibration of the 25G mode with rs-fec enabled shall be reset
 * \param has_all       [IN]  True if the calibration of all modes shall be reset 
 * \return None.
 **/
icli_rc_t ptp_icli_cal_port_reset(i32 session_id, u32 iport, BOOL has_10m_cu, BOOL has_100m_cu, BOOL has_1g_cu,
                                  BOOL has_1g, BOOL has_2g5, BOOL has_5g, BOOL has_10g, BOOL has_25g, BOOL has_25g_rsfec, BOOL has_all);

/**
 * \brief Function for calibrating the 1PPS input
 *
 * \param session_id    [IN]  Needed for being able to print error messages
 * \param cable_latency [IN]  Specifies the latency of the cable used for calibration
 * \param sma_pps       [IN]  true if SMA connectors are used for delay measurement.
 * \param has_reset     [IN]  true if earlier 1pps delay calculations need to be reset.
 * \return None.
 **/
icli_rc_t ptp_icli_cal_1pps(i32 session_id, i32 cable_latency, bool sma_pps, bool has_reset);

/**
 * \brief Function for showing the PTP calibration
 *
 * \param session_id    [IN]  Needed for being able to print error messages
 * \return None.
 **/
icli_rc_t ptp_icli_ptp_cal_show(i32 session_id);

/**
 * \brief Function for set local clock time or ratio
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param has_update [IN]  true if ptp time has to be loaded with the OS time.
 * \param has_ratio  [IN]  true if ptp cloak ration has to be set.
 * \param ratio      [IN]  ratio in units of 0,1 ppb.
 * \return None.
 **/
icli_rc_t ptp_icli_local_clock_set(i32 session_id, int clockinst, BOOL has_update, BOOL has_ratio, i32 ratio);

icli_rc_t ptp_icli_show_filter_type(i32 session_id, int clockinst);

icli_rc_t ptp_icli_local_clock_show(i32 session_id, int clockinst);


/**
 * \brief Function for set slave lock state configuration
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \param has_xxx    [IN]  true if parameter is entered. If not entered, default value is used.
 * \param xxx        [IN]  parameter value.
 * \param ratio      [IN]  ratio in units of 0,1 ppb.
 * \return None.
 **/
icli_rc_t ptp_icli_slave_cfg_set(i32 session_id, int clockinst, BOOL has_stable_offset, u32 stable_offset, BOOL has_offset_ok, u32 offset_ok, BOOL has_offset_fail, u32 offset_fail);
icli_rc_t ptp_icli_slave_cfg_show(i32 session_id, int clockinst);

icli_rc_t ptp_icli_slave_table_unicast_show(i32 session_id, int clockinst);

icli_rc_t ptp_icli_deb_send_unicast_cancel(i32 session_id, int clockinst, int slave_idx, BOOL has_ann, BOOL has_sync, BOOL has_del);

icli_rc_t ptp_icli_virtual_port_show(i32 session_id, int clockinst);

/**
 * \brief Function for PTP wireless mode
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \param enable     [IN]  True if enable wireless mode.
 * \param priority1  [IN]  port list.
 * \return None.
 **/
icli_rc_t ptp_icli_wireless_mode_set(i32 session_id, int clockinst, BOOL enable, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_wireless_delay(i32 session_id, int clockinst, i32 base_delay, i32 incr_delay, icli_stack_port_range_t *v_port_type_list);

/**
 * \brief Function for PTP wireless pre notification
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \param priority1  [IN]  port list.
 * \return None.
 **/
icli_rc_t ptp_icli_wireless_pre_notification(i32 session_id, int clockinst, icli_stack_port_range_t *v_port_type_list);


icli_rc_t ptp_icli_mode(i32 session_id, int clockinst,
                        BOOL has_boundary, BOOL has_e2etransparent, BOOL has_p2ptransparent, BOOL has_master, BOOL has_slave, BOOL has_bcfrontend, BOOL has_aedgm, BOOL has_internal,
                        BOOL has_onestep, BOOL has_twostep,
                        BOOL has_ethernet, BOOL has_ethernet_mixed, BOOL has_ip4multi, BOOL has_ip4mixed, BOOL has_ip4unicast, BOOL has_ip6mixed, BOOL has_oam, BOOL has_onepps, BOOL has_any_ptp,
                        BOOL has_oneway, BOOL has_twoway,
                        BOOL has_id, icli_clock_id_t *v_clock_id,
                        BOOL has_vid, u32 vid, u32 prio, BOOL has_mep, u32 mep_id,
                        BOOL has_profile, BOOL has_ieee1588, BOOL has_g8265_1, BOOL has_g8275_1, BOOL has_g8275_2, BOOL has_802_1as,
                        BOOL has_802_1as_aed, BOOL has_clock_domain, u32 clock_domain, BOOL has_dscp, u32 dscp_id);

icli_rc_t ptp_icli_no_mode(i32 session_id, int clockinst,
                 BOOL has_boundary, BOOL has_e2etransparent, BOOL has_p2ptransparent, BOOL has_master, BOOL has_slave, BOOL has_bcfrontend, BOOL has_aedgm, BOOL has_internal);

/** \brief Function for at runtime getting information whether SyncE is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL ptp_icli_runtime_synce_present(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

/** \brief Function for at runtime getting information about if zls30380 servo is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL ptp_icli_runtime_zls30380_present(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

/** \brief Function for at runtime getting information about if zls30380 or zls30387 servo is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL ptp_icli_runtime_zls30380_or_zls30387_present(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

/** \brief Function for at runtime getting information about if 8275.1/8275.2/8265.1 PTP profile is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL ptp_icli_runtime_telecom_profile_present(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

/** \brief Function for at runtime getting information about if 802.1AS PTP profile is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL ptp_icli_runtime_802_1as_present(u32                session_id,
                                        icli_runtime_ask_t ask,
                                        icli_runtime_t     *runtime);

/** \brief Function for getting information at runtime whether PHY timestamping capability is present
 *
 * \param session_id [IN] The session id used by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL ptp_icli_runtime_has_phy_timestamp_capability(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

/** \brief Function for getting information at runtime whether single DPLL mode capability is present
 *
 * \param session_id [IN] The session id used by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL ptp_icli_runtime_has_single_dpll_mode_capability(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

/** \brief Function for getting information at runtime whether dual DPLL mode capability is present
 *
 * \param session_id [IN] The session id used by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL ptp_icli_runtime_has_dual_dpll_mode_capability(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

BOOL ptp_icli_runtime_has_multiple_clock_domains(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

// Function for getting information at runtime whether virtual port can be supported on the platform or not.
BOOL ptp_icli_runtime_has_virtual_port_support(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
/**
 * \brief Function for setting the class of a PTP clocks virtual port
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_class_set(i32 session_id, int clockinst, u8 ptpclass);

/**
 * \brief Function for setting the accuracy of a PTP clocks virtual port
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_accuracy_set(i32 session_id, int clockinst, u8 ptpaccuracy);

/**
 * \brief Function for setting the variance of a PTP clocks virtual port
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_variance_set(i32 session_id, int clockinst, u16 ptpvariance);

/**
 * \brief Function for setting the local priority of a PTP clocks virtual port
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_local_priority_set(i32 session_id, int clockinst, u8 localPriority);

/**
 * \brief Function for setting priority1 of a PTP clocks virtual port
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_priority1_set(i32 session_id, int clockinst, u8 priority1);

/**
 * \brief Function for setting priority2 of a PTP clocks virtual port
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_priority2_set(i32 session_id, int clockinst, u8 priority2);

/**
 * \brief Function for setting clock identity of a PTP clocks virtual port
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \param clock_identity [IN] virtual port clock idenity  or grand master identity
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_clock_identity_set(i32 session_id, int clockinst, char *clock_identity, bool enable);

/**
 * \brief Function for setting steps removed of a PTP clocks virtual port
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \param steps_removed[IN] no of clocks travelled from master to slave
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_steps_removed_set(i32 session_id, int clockinst, uint16_t steps_removed, bool enable);

/**
 * \brief Function for resetting the class of a PTP clocks virtual port to its default value
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_class_clear(i32 session_id, int clockinst);

/**
 * \brief Function for resetting the accuracy of a PTP clocks virtual port to its default value
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_accuracy_clear(i32 session_id, int clockinst);

/**
 * \brief Function for resetting the variance of a PTP clocks virtual port to its default value
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_variance_clear(i32 session_id, int clockinst);

/**
 * \brief Function for resetting the local priority of a PTP clocks virtual port to its default value
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_local_priority_clear(i32 session_id, int clockinst);

/**
 * \brief Function for resetting priority1 of a PTP clocks virtual port to its default value
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_priority1_clear(i32 session_id, int clockinst);

/**
 * \brief Function for resetting priority2 of a PTP clocks virtual port to its default value
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_virtual_port_priority2_clear(i32 session_id, int clockinst);

/**
 * \brief Function for setting alarm on a PTP clock's virtual port master.
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 * \param enable     [IN]  enable/disable the alarm.
 **/
icli_rc_t ptp_icli_virtual_port_alarm_conf_set(i32 session_id, int clock_inst, BOOL enable);

/**
 * \brief Function for PTP setting the PTP clock class
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_debug_class_set(i32 session_id, int clockinst, u8 ptpclass);

/**
 * \brief Function for PTP setting the PTP clock accuracy
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_debug_accuracy_set(i32 session_id, int clockinst, u8 ptpaccuracy);

/**
 * \brief Function for PTP setting the PTP clock variance
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_debug_variance_set(i32 session_id, int clockinst, u16 ptpvariance);

/**
 * \brief Function for PTP resetting the PTP clock class to its default value
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_debug_class_clear(i32 session_id, int clockinst);

/**
 * \brief Function for PTP resetting the PTP clock accuracy to its default value
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_debug_accuracy_clear(i32 session_id, int clockinst);

/**
 * \brief Function for PTP resetting the PTP clock variance to its default value
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \return None.
 **/
icli_rc_t ptp_icli_debug_variance_clear(i32 session_id, int clockinst);

/**
 * \brief Function for PTP priority1
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \param priority1  [IN]  PTP priority1.
 * \return None.
 **/
icli_rc_t ptp_icli_priority1_set(i32 session_id, int clockinst, u8 priority1);
icli_rc_t ptp_icli_priority2_set(i32 session_id, int clockinst, u8 priority2);
icli_rc_t ptp_icli_local_priority_set(i32 session_id, int clockinst, u8 localpriority);
icli_rc_t ptp_icli_domain_set(i32 session_id, int clockinst, u8 domain);

/**
 * \brief Function for PTP path trace enable
 *
 * \param session_id [IN]  Needed for being able to print error messages
 * \param clockinst  [IN]  PTP instance number 0..3.
 * \param priority1  [IN]  Enable/disable path trace option.
 * \return None.
 **/
icli_rc_t ptp_icli_path_trace_set(i32 session_id, int clockinst, BOOL enable);


icli_rc_t ptp_icli_time_property_set(i32 session_id, int clockinst, BOOL has_utc_offset, i32 utc_offset, BOOL has_valid,
                                     BOOL has_leapminus_59, BOOL has_leapminus_61, BOOL has_time_traceable,
                                     BOOL has_freq_traceable, BOOL has_ptptimescale,
                                     BOOL has_time_source, u8 time_source,
                                     BOOL has_leap_pending, char *date_string, BOOL has_leaptype_59, BOOL has_leaptype_61);

icli_rc_t ptp_icli_filter_type_set(i32 session_id, int clockinst, BOOL has_aci_default, BOOL has_aci_freq_xo,
                                   BOOL has_aci_phase_xo, BOOL has_aci_freq_tcxo, BOOL has_aci_phase_tcxo,
                                   BOOL has_aci_freq_ocxo_s3e, BOOL has_aci_phase_ocxo_s3e, BOOL has_aci_bc_partial_on_path_freq,
                                   BOOL has_aci_bc_partial_on_path_phase, BOOL has_aci_bc_full_on_path_freq, BOOL has_aci_bc_full_on_path_phase,
                                   BOOL has_aci_bc_full_on_path_phase_faster_lock_low_pkt_rate,
                                   BOOL has_aci_freq_accuracy_fdd, BOOL has_aci_freq_accuracy_xdsl, BOOL has_aci_elec_freq, BOOL has_aci_elec_phase,
                                   BOOL has_aci_phase_relaxed_c60w, BOOL has_aci_phase_relaxed_c150,
                                   BOOL has_aci_phase_relaxed_c180, BOOL has_aci_phase_relaxed_c240,
                                   BOOL has_aci_phase_ocxo_s3e_r4_6_1, BOOL has_aci_basic_phase, BOOL has_aci_basic_phase_low, BOOL has_basic);

icli_rc_t ptp_icli_servo_clear(i32 session_id, int clockinst);

icli_rc_t ptp_icli_servo_displaystate_set(i32 session_id, int clockinst, BOOL enable);

icli_rc_t ptp_icli_servo_ap_set(i32 session_id, int clockinst, BOOL enable, u32 ap);

icli_rc_t ptp_icli_servo_ai_set(i32 session_id, int clockinst, BOOL enable, u32 ai);

icli_rc_t ptp_icli_servo_ad_set(i32 session_id, int clockinst, BOOL enable, u32 ad);

icli_rc_t ptp_icli_servo_gain_set(i32 session_id, int clockinst, u32 gain);

icli_rc_t ptp_icli_clock_servo_options_set(i32 session_id, int clockinst, BOOL synce, u32 threshold, u32 ap);

icli_rc_t ptp_icli_filter_set(i32 session_id, int clockinst, BOOL has_delay, u32 delay, BOOL has_period, u32 period, BOOL has_dist, u32 dist);

icli_rc_t ptp_icli_clock_slave_holdover_set(i32 session_id, int clockinst, BOOL has_filter, u32 ho_filter, BOOL has_adj_threshold, u32 adj_threshold);

icli_rc_t ptp_icli_clock_unicast_conf_set(i32 session_id, int clockinst, int idx, BOOL has_duration, u32 duration, u32 ip);

/**
 * \brief Function for PTP setting ext clock
 *
 * \param session_id        [IN]  Needed for being able to print error messages
 * \param has_output        [IN]  true if 1PPS output is enabled.
 * \param has_input         [IN]  true if 1PPS input is enabled. If both has_output, has_input and has_output_input are false, the 1PPS is disabled.
 * \param has_output_input  [IN]  true if 1PPS output and input are enabled.
 * \param has_ext           [IN]  true if Clock frequency output is enabled.
 * \param clockfreq         [IN]  External clock output frequency in Hz.
 * \param has_ltc           [IN]  true if Select Local Time Counter (LTC) frequency control.
 * \param has_single        [IN]  true if Select SyncE DPLL frequency control, if allowed by SyncE, otherwise select phase control.
 * \param has_independent   [IN]  true if Select an oscillator independent of SyncE for frequency control, if supported by the HW.
 * \param has_common        [IN]  true if Use second DPLL for PTP, Both DPLL have the same (SyncE recovered) clock.
 * \param has_auto          [IN]  true Auto selection of the best method depending on HW sesources and PTP profile.
 * \param clk_domain        [IN]  clock domain number
 * \return None.
 **/
icli_rc_t ptp_icli_ext_clock_set(i32 session_id, BOOL has_output, BOOL has_ext, u32 clockfreq,
                                 BOOL has_ltc, BOOL has_single, BOOL has_independent, BOOL has_common, BOOL has_auto, u32 clk_domain);
icli_rc_t ptp_icli_preferred_adj_set(i32 session_id, BOOL has_ltc, BOOL has_single, BOOL has_independent, BOOL has_common, BOOL has_auto);

/**
 * \brief Function for PTP setting ext clock
 *
 * \param session_id    [IN]  Needed for being able to print error messages
 * \param has_main_auto [IN]  true if main auto mode.
 * \param has_main_man  [IN]  true if main manual mode.
 * \param has_sub       [IN]  true if sub mode.
 * \param has_pps_delay [IN]  true if 1PPS delay is entered.
 * \param pps_delay     [IN]  1pps delay used in main-man mode.
 * \param has_ser       [IN]  true if Serial protocol (UART2) is used.
 * \param has_pim       [IN]  true if PIM protocol is used.
 * \param pim_port      [IN]  true if Switch port used by the PIM protocol.
 * \return None.
 **/
icli_rc_t ptp_icli_rs422_clock_set(i32 session_id, BOOL has_main_auto, BOOL has_main_man, BOOL has_sub, BOOL has_calib, BOOL has_pps_delay, u32 pps_delay, BOOL has_ser, BOOL has_proto, BOOL has_polyt, BOOL has_zda, BOOL has_rmc, BOOL has_pim, u32 pim_port);

icli_rc_t ptp_icli_rs422_set_baudrate(i32 session_id, u32 baudrate, BOOL has_parity, BOOL has_none, BOOL has_even, BOOL has_odd, BOOL has_wordlength, u32 wordlength, BOOL has_stopbits, u32 stopbits, BOOL has_flowctrl, BOOL has_noflow, BOOL has_rtscts);

icli_rc_t ptp_icli_ho_spec_set(i32 session_id, BOOL has_cat1, u32 cat1, BOOL has_cat2, u32 cat2, BOOL has_cat3, u32 cat3);

icli_rc_t ptp_icli_debug_log_mode_set(i32 session_id, int clockinst, u32 debug_mode, BOOL has_log_to_file, BOOL has_control, BOOL has_max_time, u32 max_time);

icli_rc_t ptp_icli_log_delete(i32 session_id, int clockinst);

icli_rc_t ptp_icli_afi_mode_set(i32 session_id, int clockinst, bool ann, bool enable);

icli_rc_t ptp_icli_port_state_set(i32 session_id, int clockinst, BOOL enable, BOOL has_internal, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_announce_set(i32 session_id, int clockinst, BOOL has_interval, i8 interval, BOOL has_stop, BOOL has_default, BOOL has_timeout, u8 timeout, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_sync_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_gptp_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_mgtSettable_sync_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_mgtSettable_gptp_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_usemgtSettable_gptp_interval_set(i32 session_id, int clockinst, i8 usemgtSettableLogGptpInterval, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_usemgtSettable_sync_interval_set(i32 session_id, int clockinst, i8 usemgtSettableLogSyncInterval, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_mgtSettable_announce_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_usemgtSettable_announce_interval_set(i32 session_id, int clockinst, i8 usemgtSettableLogAnnounceInterval, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_mgtsettable_pdelay_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_usemgtSettable_pdelay_req_interval_set(i32 session_id, int clockinst, i8 usemgtSettableLogAnnounceInterval, icli_stack_port_range_t *v_port_type_list);


BOOL ptp_icli_log_sync_interval_check(u32                session_id,
                                      icli_runtime_ask_t ask,
                                      icli_runtime_t     *runtime);

icli_rc_t ptp_icli_port_delay_mechanism_set(i32 session_id, int clockinst, BOOL has_e2e, BOOL has_p2p, BOOL has_common_p2p, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_min_pdelay_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_delay_asymmetry_set(i32 session_id, int clockinst, i32 delay_asymmetry, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_ingress_latency_set(i32 session_id, int clockinst, i32 ingress_latency, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_egress_latency_set(i32 session_id, int clockinst, i32 egress_latency, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_local_priority_set(i32 session_id, int clockinst, u8 localpriority, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_master_only_set(i32 session_id, int clockinst, bool masterOnly, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_mcast_dest_adr_set(i32 session_id, int clockinst, bool has_default, bool has_link_local, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_two_step_set(i32 session_id, int clockinst, bool set_value, bool value, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_as2020_set(i32 session_id, int clockinst, icli_stack_port_range_t *port_list, BOOL as2020);

icli_rc_t ptp_icli_port_aed_port_set(i32 session_id, int clockinst, icli_stack_port_range_t *port_list, uint aedMaster);

icli_rc_t ptp_icli_aed_interval_set(i32 session_id, int clockinst, BOOL has_oper_pdelay, i8 oper_pdelay_val, BOOL has_init_sync, i8 init_sync_val, BOOL has_oper_sync, i8 oper_sync_val, icli_stack_port_range_t *port_list);

icli_rc_t ptp_icli_port_delay_thresh_set(i32 session_id, int clockinst, BOOL set, u32 delay_thresh, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_sync_rx_to_set(i32 session_id, int clockinst, BOOL set, u8 sync_rx_to, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_gptp_to_set(i32 session_id, int clockinst, BOOL set, u8 gptp_to, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_statistics_clear(i32 session_id, int clockinst, BOOL has_clear, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_allow_lost_resp_set(i32 session_id, int clockinst, BOOL set, u16 allow_lost_resp, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_allow_faults_set(i32 session_id, int clockinst, BOOL set, u16 allowed_faults, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_1pps_mode_set(i32 session_id, BOOL has_main_auto, BOOL has_main_man, BOOL has_sub, BOOL has_pps_phase, i32 pps_phase,
                                      BOOL has_cable_asy, i32 cable_asy, BOOL has_ser_man, BOOL has_ser_auto, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_1pps_delay_set(i32 session_id, BOOL has_auto, u32 master_port, BOOL has_man, u32 cable_delay, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_compute_nbr_rate_ratio_set(i32 session_id, int clockinst, BOOL compNbrRateRatio, BOOL useMgmtNbrRateRatio, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_compute_mean_link_delay_set(i32 session_id, int clockinst, BOOL compMeanLinkDelay, BOOL useMgmtCompMeanLinkDelay, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_port_not_master_set(i32 session_id, int clockinst, icli_stack_port_range_t *port_lis, BOOL not_master);

icli_rc_t ptp_icli_ms_pdv_show_config(i32 session_id);

icli_rc_t ptp_icli_ms_pdv_show_apr_server_status(i32 session_id, u16 cguId, u16 serverId);

icli_rc_t ptp_icli_ms_pdv_show_apr_config(i32 session_id, u16 cguId);

icli_rc_t ptp_icli_ms_pdv_set(i32 session_id, BOOL has_one_hz, BOOL has_min_phase, u32 min_phase, BOOL has_apr, u32 apr);

icli_rc_t ptp_icli_tc_internal_set(i32 session_id, BOOL has_mode, u32 mode);

icli_rc_t ptp_icli_phy_timestamp_dis_set(i32 session_id, BOOL phy_ts_dis);

icli_rc_t ptp_icli_system_time_sync_set(i32 session_id, BOOL has_get, BOOL has_set, int clockinst);

icli_rc_t ptp_icli_system_time_sync_show(i32 session_id);

icli_rc_t ptp_icli_ptp_ref_clock_set(i32 session_id, BOOL has_mhz125, BOOL has_mhz156p25, BOOL has_mhz250);

icli_rc_t ptp_icli_debug_pim_statistics(i32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL clear);

icli_rc_t ptp_icli_debug_egress_latency_statistics(i32 session_id, BOOL clear);

/**
 * \brief Function for enabling/disabling/showing statistics for the PTP slave function
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param has_enable [IN]  Enable statistics
 * \param has_disable [IN]  Disable statistics
 * \param has_clear [IN]  Clear statistics after reading
 * \return ICLI error code.
 **/
icli_rc_t ptp_icli_debug_slave_statistics(i32 session_id, int clockinst, BOOL has_enable, BOOL has_disable, BOOL has_clear);

/**
 * \brief Function for showing statistics for the 1-PPS/timeofday slave function
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param has_clear [IN]  Clear statistics after reading
 * \return ICLI error code.
 **/
icli_rc_t ptp_icli_debug_one_pps_tod_statistics(i32 session_id, BOOL has_clear);


/**
 * \brief Function for at runtime checking if the ref-clk command is visible
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return true if runtime contains valid information.
 **/
BOOL ptp_icli_ref_clk_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_icli_rs_422_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_inp_0_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_inp_1_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_inp_2_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_inp_3_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_inp_4_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_inp_5_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_out_0_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_out_1_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_out_2_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_out_3_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_out_4_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_out_5_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_pps_inp_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_pps_out_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL port_icli_runtime_tc_internal_range(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_icli_sync_ann_auto_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

BOOL ptp_icli_filter_type_basic_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL ptp_icli_wireless_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

icli_rc_t ptp_icli_debug_servo_best_master(i32 session_id, int clockinst);
icli_rc_t ptp_icli_show_servo_source(i32 session_id);
icli_rc_t ptp_icli_show_servo_mode_ref(i32 session_id);

icli_rc_t ptp_icli_debug_path_trace(i32 session_id, int clockinst);

icli_rc_t ptp_icli_debug_802_1as_status(i32 session_id, int clockinst);

icli_rc_t ptp_icli_debug_802_1as_port_status(i32 session_id, int clockinst, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_virtual_port_time_property_set(i32 session_id, int clockinst, BOOL has_utc_offset, i32 utc_offset, BOOL has_valid,
                                    BOOL has_leapminus_59, BOOL has_leapminus_61, BOOL has_time_traceable,
                                    BOOL has_freq_traceable, BOOL has_ptptimescale,
                                    BOOL has_time_source, u8 time_source,
                                    BOOL has_leap_pending, char *date_string, BOOL has_leaptype_59, BOOL has_leaptype_61);

icli_rc_t ptp_icli_virtual_port_default_time_property_set(i32 session_id, int clockinst);

#if defined(VTSS_SW_OPTION_ZLS30387)
/**
 * \brief Function for changing the log-level of the MS-PDV
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param loglevel [IN] The log level (a value from 0 to 2)
 * \return ICLI error code.
 **/
icli_rc_t ptp_icli_debug_mspdv_loglevel(i32 session_id, u32 loglevel);
#endif

#if defined(VTSS_SW_OPTION_ZLS30387)
icli_rc_t ptp_icli_show_psl_fcl_config(i32 session_id, u16 cguId);

icli_rc_t ptp_icli_show_apr_statistics(i32 session_id, u16 cguId, u32 stati);
#endif

icli_rc_t ptp_icli_debug_set_ptp_delta_time(i32 session_id, u32 domain, u16 sec_msb, u32 sec_lsb, u32 nanosec, BOOL has_neg);

icli_rc_t ptp_icli_virtual_port_mode_set(i32 session_id, uint instance, BOOL has_main_auto, BOOL has_main_man, BOOL has_sub, BOOL has_pps_delay, u32 pps_delay, BOOL has_pps_in, u32 input_pin, BOOL has_pps_out, u32 output_pin, BOOL has_freq_out, u32 freq_putput_pin, u32 freq);
icli_rc_t ptp_icli_virtual_port_tod_set(i32 session_id, uint instance, BOOL has_ser, BOOL has_proto, BOOL has_polyt, BOOL has_zda, BOOL has_rmc, BOOL has_pim, u32 pim_port);

icli_rc_t ptp_icli_cmlds_port_data_show(i32 session_id, BOOL has_port_status, BOOL has_port_conf, BOOL has_port_statistics, icli_stack_port_range_t *v_port_list);

icli_rc_t ptp_icli_cmlds_default_ds_show(i32 session_id);

icli_rc_t ptp_icli_cmlds_port_ds_conf(i32 session_id, bool enable, BOOL has_pdelay_thresh, u64 pdelay_thresh, BOOL has_use_compute_meanlinkdelay, BOOL has_compute_meanlinkdelay, BOOL has_use_pdelayreq_interval, BOOL has_pdelayreq_interval, uint pdelayreq_interval, BOOL has_allow_lost_resp, u8 v_0_to_10, BOOL has_allow_faults,u8 v_1_to_255, BOOL has_use_compute_neighbor_rate_ratio, BOOL has_compute_neighbor_rate_ratio, icli_stack_port_range_t *v_port_type_list);

icli_rc_t ptp_icli_cmlds_pdelay_thresh_set(i32 session_id, BOOL set, u32 pdelay_thresh, icli_stack_port_range_t *port_type_list_p);
icli_rc_t ptp_icli_cmlds_mean_link_delay(i32 session_id, BOOL mgtSettableComputeMeanLinkDelay, BOOL useMgtSettableComputeMeanLinkDelay, icli_stack_port_range_t *port_type_list_p);
icli_rc_t ptp_icli_cmlds_pdelay_req_int_set(i32 session_id, BOOL enable, i8 pdelay_req_int, BOOL has_stop, BOOL has_default, BOOL UseMgtSettableLogPdelayReqInterval, icli_stack_port_range_t *port_type_list_p);
icli_rc_t ptp_icli_cmlds_delay_asym_set(i32 session_id, i32 delay_asymmetry, icli_stack_port_range_t *port_type_list_p);
icli_rc_t ptp_icli_cmlds_nbr_rate_ratio(i32 session_id, BOOL compute_neighbor_rate_ratio, BOOL useMgtSettableComputeNeighborRateRatio, icli_stack_port_range_t *port_type_list_p);
icli_rc_t ptp_icli_cmlds_allow_faults_set(i32 session_id, BOOL enable, u8 allow_faults, icli_stack_port_range_t *port_type_list_p);
icli_rc_t ptp_icli_cmlds_allow_lost_resp_set(i32 session_id, BOOL enable, u8 allow_lost_resp, icli_stack_port_range_t *port_type_list_p);
icli_rc_t ptp_icli_cmlds_statistics_clear(i32 session_id, BOOL has_clear, icli_stack_port_range_t *port_type_list_p);
icli_rc_t ptp_icli_software_clock_pps_domain_set(int32_t session_id, uint32_t instance, uint32_t pps_domain, bool set);
void ptp_icli_software_clock_show_data(i32 session_id, uint32_t instance);
void ptp_icli_debug_basic_servo_hybrid_enable(i32 session_id, bool enable);
void ptp_icli_internal_mode_sync_rate_set(i32 session_id, uint32_t instance, int32_t sync_rate, bool enable);
#ifdef __cplusplus
}
#endif
#endif /* VTSS_ICLI_PTP_H */

