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

#ifndef _VTSS_APPL_POE_API_H_
#define _VTSS_APPL_POE_API_H_
#include "vtss/appl/poe.h"
#include "poe_types.h"


typedef enum {
    ACTUAL,
    REQUESTED
} poe_mgmt_mode_t;

typedef enum {
    NOT_FOUND,
    FOUND,
    DETECTION_NEEDED
} poe_chip_found_t;

/**********************************************************************
 ** Configuration structs
 **********************************************************************/


/**********************************************************************
** Wrapper-methods for hardware-access (For LLDP-Poe control).
**********************************************************************/
BOOL pd69200bt_poe_info_get(poe_info_t *poe_info);
poe_entry_t poe_hw_config_get(mesa_port_no_t port_idx, poe_entry_t *hw_conf);
void poe_pse_data_get(mesa_port_no_t port_index, poe_pse_data_t *pse_data);
void poe_pd_data_set(mesa_port_no_t port_index, poe_pd_data_t *pd_data);
void poe_pd_bt_data_set(mesa_port_no_t port_index, poe_pd_bt_data_t *pd_data);
void poe_pd_data_clear(mesa_port_no_t port_index);
BOOL poe_new_pd_detected_get(mesa_port_no_t port_index, BOOL clear);
BOOL poe_port_lldp_disabled(mesa_port_no_t port_index);
char *poe_firmware_info_get(uint32_t max_size, char *info);

/**********************************************************************
 ** PoE functions
 **********************************************************************/
extern BOOL poe_init_done;

const char *poe_error_txt(mesa_rc rc); // Return printable string for the given return code

/* Initialize module */
mesa_rc poe_init(vtss_init_data_t *data);

// Function that returns the current configuration.
void poe_config_get(poe_conf_t *conf);

// Function that can set the current configuration.
mesa_rc poe_config_set(poe_conf_t *new_conf);
bool poe_config_updated();

// Function for getting current status for all ports
void poe_mgmt_get_status(poe_status_t *status);

// Function Converting a integer to a string with one digit. E.g. 102 becomes 10.2.
char *one_digi_float2str(int val, char *string_ptr);

const char *poe_icli_port_status2str(vtss_appl_poe_status_t status);

char *poe_class2str(const poe_status_t *status, u8 assigned_pd_class_a, u8 assigned_pd_class_b ,mesa_port_no_t port_index, char *class_str);

BOOL poe_mgmt_is_backup_power_supported(void);
meba_poe_power_source_t poe_mgmt_get_power_source(void); // Function that returns which power source the host is using right now,

// Function for getting a list with which PoE chipset there is associated to a given port.
// In/out : poe_chip_found - pointer to array to return the list.
void poe_mgmt_is_chip_found(meba_poe_chip_state_t *poe_chip_found);

// Debug function for when debugging i2c issues
void poe_halt_at_i2c_err_ena(); // Enabling halting at i2c errors.
void poe_halt_at_i2c_err(BOOL i2c_err); // Do the halting if an i2c error happens - Input - TRUE is an i2c error has been detected.

// Debug function
void poe_mgmt_capacitor_detection_set(mesa_port_no_t iport, BOOL enable);
BOOL poe_mgmt_capacitor_detection_get(mesa_port_no_t iport);
mesa_rc poe_mgmt_firmware_update(const char *firmware, size_t firmware_size);

void poe_debug_access(mesa_port_no_t iport, char* var, uint32_t str_len ,char* title ,char* tx_str ,char* rx_str ,char* msg ,int max_msg_len);
void poe_restore_factory_default(mesa_port_no_t iport);
void poe_reset_command(void);
void poe_save_command(void);

// Function for getting which PoE chipset is found from outside the poe.c file (In order to do semaphore protection)
meba_poe_chip_state_t poe_is_chip_found(mesa_port_no_t iport, const char *file, u32 line);

// Returns TRUE if the PoE chips are detected and initialized, else FALSE.
BOOL is_poe_ready(void);

BOOL is_port_shutdown(mesa_port_no_t iport);

BOOL poe_is_any_chip_found(void);

void poe_status_polling_disable(BOOL disable);
#define POE_FIRMWARE_SIZE_MAX 2000000 // Maximum size of the PoE firmware in bytes (file mscc_firmware.s19)
mesa_rc poe_do_firmware_upgrade(const char *url, int &tftp_err, BOOL has_built_in, BOOL has_brick);

#endif  /* _VTSS_APPL_POE_API_H_ */

