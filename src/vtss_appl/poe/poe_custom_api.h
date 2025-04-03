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

#ifndef _VTSS_POE_CUSTOM_API_H_
#define _VTSS_POE_CUSTOM_API_H_
#include <vtss/appl/types.h>
#include <vtss/appl/poe.h>
#include "main.h"
#include "poe_types.h"

/**********************************************************************
* Struct for defining PoE capabilites for each port
**********************************************************************/
typedef struct {
    BOOL available;    /* True is PoE is available for the port  */
    u8   pse_pairs_control_ability;  // pethPsePortPowerPairsControlAbility, rfc3621
    u8   pse_power_pair;             // pethPsePortPowerPairs, rfc3621
    char i2c_device_name[10][100];    // i2c Linux device number
    // PALLE: To be removed
} poe_custom_entry_t;

/**********************************************************************
** Generic function for controlling the PoE chipsets
**********************************************************************/
VTSS_BEGIN_HDR

mesa_rc poe_custom_cfg_set(poe_conf_t *conf);

void poe_custom_set_port_priority(mesa_port_no_t iport, vtss_appl_poe_port_power_priority_t priority) ; // Configuration of port priority
mesa_rc poe_custom_get_status(poe_status_t *poe_status); // Updates the poe_status structure.
void poe_custom_port_status_get(mesa_port_no_t iport, vtss_appl_poe_status_t *port_status); // Getting status for a single port
mesa_rc poe_custom_init(meba_poe_init_params_t *tPoe_init_params); // Initialise the PoE "API"
void poe_custom_init_chips(mesa_bool_t interruptible_power, int16_t restart_cause); // Initialise the PoE chip
void poe_custom_get_all_ports_classes(char *classes);// updates the power reserved fields according to the PD's detected class

u16  poe_custom_get_port_power_max(mesa_port_no_t iport);// Returns Maximum power supported for a port (in DeciWatts)
BOOL poe_custom_is_backup_power_supported (void); // Returns TRUE if backup power supply is supported by the HW.


meba_poe_power_source_t poe_custom_get_power_source(void); // Function that returns which power source the host is using right now,
poe_custom_entry_t poe_custom_get_hw_config(mesa_port_no_t port_idx, poe_custom_entry_t *hw_conf);

void poe_custom_pd_data_set(mesa_port_no_t iport, poe_pd_data_t *pd_data);
void poe_custom_pd_bt_data_set(mesa_port_no_t iport, poe_pd_bt_data_t *pd_data);
BOOL poe_custom_new_pd_detected_get(mesa_port_no_t iport, BOOL clear);
BOOL poe_custom_new_pd_detected_set(mesa_port_no_t iport);
mesa_rc poe_custom_powerin_status_get(vtss_appl_poe_powerin_status_t *const powerin_status);


// Debug function for converting PoE chip_state to a printable text string.
// In : poe_chipset - The PoE chipset type.
//      buf        - Pointer to a buffer that can contain the printable Poe chip_state string.
char *poe_chipset2txt(meba_poe_chip_state_t poe_chip_state, char *buf);
char *poe_custom_firmware_info_get(uint32_t max_size, char *info);
VTSS_END_HDR

/**********************************************************************
** Misc.
**********************************************************************/
// Each PD69200 controller controls a number of ports. This number depends on the board design.
#define POE_PORTS_PER_CONTROLLER 24

// Defines the maximum number of retries that we shall try an re-transmit
// I2C accesses before giving up.
#define I2C_RETRIES_MAX 2

#endif // _VTSS_POE_CUSTOM_API_H_

