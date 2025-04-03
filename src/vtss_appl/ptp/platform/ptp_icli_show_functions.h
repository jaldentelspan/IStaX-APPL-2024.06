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
 * \brief PTP icli functions
 * \details This header file describes ptp control functions
 */

#ifndef PTP_ICLI_SHOW_FUNCTIONS_H
#define PTP_ICLI_SHOW_FUNCTIONS_H

#include "ptp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ICLI printout functions, can also be called from vcli.
 */
typedef int vtss_ptp_cli_pr(        const char                  *fmt,
                                    ...);

void ptp_cli_table_header(const char *txt, vtss_ptp_cli_pr *pr);
void ptp_show_clock_default_ds(int inst, vtss_ptp_cli_pr *pr);
void ptp_show_clock_current_ds(int inst, vtss_ptp_cli_pr *pr);
void ptp_show_clock_parent_ds(int inst, vtss_ptp_cli_pr *pr);
void ptp_show_clock_time_property_ds(int inst, vtss_ptp_cli_pr *pr);
void ptp_show_clock_filter_ds(int inst, vtss_ptp_cli_pr *pr);

void ptp_show_clock_servo_ds(int inst, vtss_ptp_cli_pr *pr);

void ptp_show_clock_clk_ds(int inst, vtss_ptp_cli_pr *pr);

void ptp_show_clock_ho_ds(int inst, vtss_ptp_cli_pr *pr);

void ptp_show_clock_uni_ds(int inst, vtss_ptp_cli_pr *pr);

void ptp_show_clock_master_table_unicast_ds(uint inst, vtss_ptp_cli_pr *pr);

void ptp_show_clock_slave_ds(int inst, vtss_ptp_cli_pr *pr, bool details);

void ptp_show_log_mode(int inst, vtss_ptp_cli_pr *pr);

void ptp_show_clock_port_state_ds(int inst, mesa_port_no_t uport, bool first, vtss_ptp_cli_pr *pr);

void ptp_show_clock_port_statistics(int inst, mesa_port_no_t uport, bool first, vtss_ptp_cli_pr *pr, bool clear);

#if defined (VTSS_SW_OPTION_P802_1_AS)
void ptp_show_clock_port_802_1as_cfg(int inst, vtss_uport_no_t uport, bool first, vtss_ptp_cli_pr *pr);

void ptp_show_clock_port_802_1as_status(int inst, vtss_uport_no_t uport, bool first, vtss_ptp_cli_pr *pr);
#endif //defined (VTSS_SW_OPTION_P802_1_AS)

void ptp_show_clock_virtual_port_state_ds(int inst, u32 virtual_port, bool first, vtss_ptp_cli_pr *pr);

void ptp_show_clock_port_ds(int inst, mesa_port_no_t uport, bool first, vtss_ptp_cli_pr *pr);

void ptp_show_clock_wireless_ds(int inst, mesa_port_no_t uport, bool first, vtss_ptp_cli_pr *pr);

void ptp_show_clock_foreign_master_record_ds(int inst, mesa_port_no_t uport, bool first, vtss_ptp_cli_pr *pr);

void ptp_show_ext_clock_mode(vtss_ptp_cli_pr *pr);

void ptp_show_local_clock(int inst, vtss_ptp_cli_pr *pr);

void ptp_show_slave_cfg(int inst, vtss_ptp_cli_pr *pr);

void ptp_show_clock_slave_table_unicast_ds(int inst, vtss_ptp_cli_pr *pr);

void ptp_show_rs422_clock_mode(vtss_ptp_cli_pr *pr);

void ptp_show_cmlds_port_status(vtss_uport_no_t uport, bool first, vtss_ptp_cli_pr *pr);

void ptp_show_cmlds_port_conf(vtss_uport_no_t uport, bool first, vtss_ptp_cli_pr *pr);

void ptp_show_cmlds_port_statistics(vtss_uport_no_t uport, bool first, vtss_ptp_cli_pr *pr, bool clear);

#ifdef __cplusplus
}
#endif
#endif /* PTP_ICLI_SHOW_FUNCTIONS_H */

