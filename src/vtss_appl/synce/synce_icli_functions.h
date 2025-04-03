/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \brief Synce icli functions
 * \details This header file describes synce control functions
 */
#ifndef VTSS_ICLI_SYNCE_H
#define VTSS_ICLI_SYNCE_H

#include "icli_api.h"
#include "synce.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief Init function ICLI CFG
 * \return Error code.
 **/
mesa_rc synce_icfg_init(void);

/**
 * \brief Function for at runtime enabling EEC1 related options
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_synce_eec1_options(u32                session_id,
                                           icli_runtime_ask_t ask,
                                           icli_runtime_t     *runtime);

/**
 * \brief Function for at runtime enabling EEC2 related options
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_synce_eec2_options(u32                session_id,
                                           icli_runtime_ask_t ask,
                                           icli_runtime_t     *runtime);

/**
 * \brief Function for at runtime getting information about synce clock sources
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_synce_sources(u32                session_id,
                                      icli_runtime_ask_t ask,
                                      icli_runtime_t     *runtime);

/**
 * \brief Function for at runtime getting information about synce clock priorities
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_synce_priorities(u32                session_id,
                                         icli_runtime_ask_t ask,
                                         icli_runtime_t     *runtime);

/**
 * \brief Function for at runtime getting information about if 1544 kHz input frequency is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_input_1544khz(u32                session_id,
                                      icli_runtime_ask_t ask,
                                      icli_runtime_t     *runtime);

/** \brief Function for at runtime getting information about if 2058 kHz input frequency is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_input_2048khz(u32                session_id,
                                      icli_runtime_ask_t ask,
                                      icli_runtime_t     *runtime);

/** \brief Function for at runtime getting information about if any input frequency is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_any_input_freq(u32                session_id,
                                       icli_runtime_ask_t ask,
                                       icli_runtime_t     *runtime);

/** \brief Function for at runtime getting information about if 10MHZ input frequency is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_input_10MHz(u32                session_id,
                                    icli_runtime_ask_t ask,
                                    icli_runtime_t     *runtime);

/**
 * \brief Function for at runtime getting information about if 1544 kHz output frequency is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_output_1544khz(u32                session_id,
                                       icli_runtime_ask_t ask,
                                       icli_runtime_t     *runtime);

/** \brief Function for at runtime getting information about if 2058 kHz output frequency is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_output_2048khz(u32                session_id,
                                       icli_runtime_ask_t ask,
                                       icli_runtime_t     *runtime);

/** \brief Function for at runtime getting information about if any output frequency is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_any_output_freq(u32                session_id,
                                        icli_runtime_ask_t ask,
                                        icli_runtime_t     *runtime);

/** \brief Function for at runtime getting information about if 10MHZ output frequency is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [OUT]  Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL synce_icli_runtime_output_10MHz(u32                session_id,
                                     icli_runtime_ask_t ask,
                                     icli_runtime_t     *runtime);

/**
 * \brief Function for configuring station clock
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param input_source [IN] TRUE - Configure input clock. FALSE configure output clock.
 * \param has_1544 [IN]  Configure clock to 1544kHz
 * \param has_2048 [IN]  Configure clock to 2048kHz
 * \param has_10M [IN]  Configure clock to 10MHz
 * \param no [IN]  TRUE to configure to default value
 * \return Error code
 **/
mesa_rc synce_icli_station_clk(i32 session_id, BOOL input_source, BOOL has_1544, BOOL has_2048, BOOL has_10M, BOOL no);

/**
 * \brief Function for preferred auto negotiation
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param clk_list [IN] List of source clock to configure
 * \param has_master [IN] configure selected source clocks to preferred master
 * \param has_slave [IN] configure selected source clocks to preferred slave.
 * \param has_forced [IN] configure selected source clocks to forced slave.
 * \param no [IN]  TRUE to configure to default value
 * \return Error code
 **/
mesa_rc synce_icli_aneg(i32 session_id, icli_range_t *clk_list, BOOL has_master, BOOL has_slave, BOOL has_forced, BOOL no);

/**
 * \brief Function for configuring hold time
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param clk_list [IN] List of source clock to configure
 * \param v_3_to_18 [IN] New hold time value
 * \param no [IN]  TRUE to configure to default value
 * \return Error code
 **/
mesa_rc synce_icli_hold_time(i32 session_id, icli_range_t *clk_list, u8 v_3_to_18, BOOL no);

/**
 * \brief Function for showing SyncE clock selection config
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \return Error code
 **/
mesa_rc synce_icli_show_clock_selection_config(i32 session_id);

/**
 * \brief Function for showing SyncE station clock config
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \return Error code
 **/
mesa_rc synce_icli_show_station_clock_config(i32 session_id);

/**
 * \brief Function for showing SyncE source nomination config
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \return Error code
 **/
mesa_rc synce_icli_show_source_nomination_config(i32 session_id);

/**
 * \brief Function for showing SyncE clock synchronization config
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \return Error code
 **/
mesa_rc synce_icli_show_clock_synchronization_config(i32 session_id);

/**
 * \brief Function for showing synce state
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \return Error code
 **/
mesa_rc synce_icli_show(i32 session_id);

/**
 * \brief Function for showing synce state of PTP ports
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \return Error code
 **/
mesa_rc synce_icli_show_ptp_ports(i32 session_id);

/**
 * \brief Function for showing synce config of ports
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \return Error code
 **/
mesa_rc synce_icli_show_port_config(i32 session_id);

/**
 * \brief Function for showing synce state of ports
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \return Error code
 **/
mesa_rc synce_icli_show_port_status(i32 session_id);

/**
 * \brief Function for clearing WTR
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param clk_list [IN] List of source clock to clear
 * \return Error code
 **/
mesa_rc synce_icli_clear(i32 session_id, icli_range_t *clk_list);

/**
 * \brief Function for nominating clock sources
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param clk_list [IN] List of source clock to clear
 * \param clk_in [IN] TRUE if the Station clock in is nominated as source
 * \param port [IN] Port to a assign to the clock source
 * \param no [IN]  TRUE to configure to default value (dis-nominate) else nominate
 * \return Error code
 **/
mesa_rc synce_icli_nominate(i32 session_id, icli_range_t *clk_list, BOOL clk_in, BOOL has_ptp, int ptp_inst, icli_switch_port_range_t *port, BOOL no);

/**
 * \brief Function for configure selection
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param clk_src [IN] clock source number (Only used if has_manual is TRUE)
 * \param has_manual [IN] TRUE to configure selection to manual
 * \param has_selected [IN] TRUE to configure selection to selected
 * \param has_nonrevertive [IN] TRUE to configure selection to nonrevertive
 * \param has_revertive [IN] TRUE to configure selection to revertive
 * \param has_holdover [IN] TRUE to configure selection to holdover
 * \param has_freerun [IN] TRUE to configure selection to freerun
 * \param no [IN]  TRUE to configure to default value
 * \return Error code
 **/
mesa_rc synce_icli_selector(i32 session_id, u8 clk_src, BOOL has_manual, BOOL has_selected, BOOL has_nonrevertive, BOOL has_revertive, BOOL has_holdover, BOOL has_freerun, BOOL no);

/**
 * \brief Function for configuring priority
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param clk_list [IN] List of source clock to configure
 * \param prio [IN] New priority value
 * \param no [IN]  TRUE to configure to default value
 * \return Error code
 **/
mesa_rc synce_icli_prio(i32 session_id, icli_range_t *clk_list, u8 prio, BOOL no);

/**
 * \brief Function for configuring WTR
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param wtr_value [IN] New WTR value
 * \param no [IN]  TRUE to configure to default value
 * \return Error code
 **/
mesa_rc synce_icli_wtr(i32 session_id, u8 wtr_value, BOOL no);

/**
 * \brief Function for configuring ssm
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param clk_list [IN] List of source clock to configure
 * \param type [IN] Which tye of SSM to configure (HOLDOVER, FREERUN, OVERWRITE)
 * \param has_prc [IN] TRUE to configure SSM to PRC
 * \param has_ssua [IN] TRUE to configure SSM to SSUA
 * \param has_ssub [IN] TRUE to configure SSM to SSUB
 * \param has_eec2 [IN] TRUE to configure SSM to EEC2
 * \param has_eec1 [IN] TRUE to configure SSM to EEC1
 * \param has_dnu [IN] TRUE to configure SSM to DNU
 * \param has_inv [IN] TRUE to configure SSM to INV
 * \param has_prs [IN] TRUE to configure SSM to PRS
 * \param has_stu [IN] TRUE to configure SSM to STU
 * \param has_st2 [IN] TRUE to configure SSM to ST2
 * \param has_tnc [IN] TRUE to configure SSM to TNC
 * \param has_st3e [IN] TRUE to configure SSM to ST3E
 * \param has_smc [IN] TRUE to configure SSM to SMC
 * \param has_prov [IN] TRUE to configure SSM to PROV
 * \param has_dus [IN] TRUE to configure SSM to DUS
 * \param no [IN]  TRUE to configure to default value
 * \return Error code
 **/
mesa_rc synce_icli_ssm(i32 session_id, icli_range_t *clk_list, synce_icli_ssm_type_t type, BOOL has_prc, BOOL has_ssua, BOOL has_ssub, BOOL has_eec2, BOOL has_eec1, BOOL has_dnu, BOOL has_inv,
                       BOOL has_prs, BOOL has_stu, BOOL has_st2, BOOL has_tnc, BOOL has_st3e, BOOL has_smc, BOOL has_prov, BOOL has_dus, BOOL no);

/**
 * \brief Function for configuring option
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param has_eec1 [IN] TRUE to configure option to EEC1
 * \param has_eec2 [IN] TRUE to configure option to EEC2
 * \param no [IN]  TRUE to configure to default value
 * \return Error code
 **/
mesa_rc sycne_icli_option(i32 session_id, BOOL has_eec1, BOOL has_eec2, BOOL no);

/**
 * \brief Function for configuring enabling/disabling of SSM
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param plist [IN] Ports to configure
 * \param no [IN]  TRUE to configure to default value (disabled). FALSE to enable SSM
 * \return Error code
 **/
mesa_rc synce_icli_sync(i32 session_id, icli_stack_port_range_t *plist, BOOL no);

/**
 * \brief Debug Functions
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param reg [IN]      register address
 * \param cnt [out]   register count
 * \return Error code
 **/
mesa_rc synce_icli_debug_read(i32 session_id, u32 reg, u32 cnt);
mesa_rc synce_icli_debug_write(i32 session_id, u32 reg, u32 value);
mesa_rc synce_icli_debug_subject_write(i32 session_id, char *subjectname, u32 ix, char *value);

mesa_rc synce_icli_debug_test(i32 session_id);
mesa_rc synce_icli_debug_testing_script_synce_clock_type(i32 session_id);

mesa_rc synce_icli_debug_cpld_read(i32 session_id, u8 reg);
mesa_rc synce_icli_debug_cpld_write(i32 session_id, u8 reg, u8 value);
void synce_icli_debug_graph_dump(i32 session_id);
void synce_icli_path_for_clock(i32 session_id, uint32_t clk, uint32_t devid);

#ifdef __cplusplus
}
#endif
#endif /* VTSS_ICLI_SYNCE_H */

