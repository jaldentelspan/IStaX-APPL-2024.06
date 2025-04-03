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

#ifndef _TSN_API_H_
#define _TSN_API_H_

#include <vtss/appl/tsn.h>
#include <vtss/appl/port.h> /* For vtss_appl_port_status_t */
#if defined(VTSS_SW_OPTION_LLDP)
#include <vtss/appl/lldp.h>
#endif

extern const vtss_appl_tsn_capabilities_t *const vtss_appl_tsn_capabilities;

/**
 * Utility function for registering a callback function that will be invoked
 * after any TSN configuration change has occured.
 * The callback function will be invoked without any mutexes held.
 */
mesa_rc tsn_util_conf_change_callback_register(vtss_module_id_t module_id, void (*callback)(void));

/**
 * Function for converting a TSN error code to a text string
 */
const char *tsn_error_txt(mesa_rc rc);

/**
 * Debug function to manipulate the local value of per port add_frag_size which will be forwarded by lldp message.
 */
mesa_rc tsn_util_debug_set_add_frag_size(vtss_ifindex_t ifindex, int add_frag_size);

void TSN_tas_data_init(void);

void TSN_fp_default(void);
void TSN_tas_default(void);
void TSN_psfp_default(void);

void TSN_port_change_cb(mesa_port_no_t port_no, const vtss_appl_port_status_t *status);

mesa_rc TSN_ptr_check(const void *ptr);
mesa_rc TSN_ifindex_to_port(const vtss_ifindex_t ifindex, uint32_t *port);

mesa_rc TSN_tas_config_change();
mesa_rc TSN_psfp_config_change();

void TSN_tas_init();

bool TSN_is_started();
void TSN_fp_port_state_set_all();

#if defined(VTSS_SW_OPTION_LLDP)
void TSN_lldp_change_cb(mesa_port_no_t port_no, vtss_appl_lldp_remote_entry_t *entry);
#endif

/**
 * Function for initializing the TSN module
 */
mesa_rc tsn_init(vtss_init_data_t *data);

mesa_timestamp_t tsn_util_calculate_chip_base_time(mesa_timestamp_t admin_base_time, uint32_t chip_cycle_time);
uint32_t tsn_util_calc_cycle_time_nsec(uint32_t numerator, uint32_t denominator);
mesa_rc tsn_util_current_time_get(mesa_timestamp_t &tod);

// Returns a string on the form [SSSSSSSSSSSSSSS.nnnnnnnnn]. sz must be at least
// 26 chars.
char *tsn_util_timestamp_to_str(char *buf, size_t sz, mesa_timestamp_t &ts);

// Convert a mesa_timestamp_t to an ISO 8601 string.
// The sz parameter should be at least the following:
#define TSN_UTIL_ISO8601_STRING_SIZE 25 /* including terminating null */
char *tsn_util_timestamp_to_iso8601(char *buf, size_t sz, mesa_timestamp_t &ts);

#endif /* _TSN_API_H_ */

