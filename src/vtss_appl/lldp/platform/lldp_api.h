/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _LLDP_API_H_
#define _LLDP_API_H_

#include "msg_api.h"
#include "vtss_lldp.h"
#include "lldp_remote.h"


//
// Callback. Used to get called back when an entry is updated.
//
typedef void (*lldp_callback_t)(mesa_port_no_t port_no, vtss_appl_lldp_remote_entry_t *entry);


//
// LLDP configuration default values
//

// Default configuration values.
#define LLDPMED_ALTITUDE_DEFAULT 0
#define LLDPMED_ALTITUDE_TYPE_DEFAULT METERS

#define LLDPMED_LONGITUDE_DEFAULT 0
#define LLDPMED_LONGITUDE_DIR_DEFAULT EAST

#define LLDPMED_LATITUDE_DEFAULT 0
#define LLDPMED_LATITUDE_DIR_DEFAULT NORTH

#define LLDPMED_DATUM_DEFAULT WGS84

#define LLDPMED_LONG_LATI_TUDE_RES 34 // Fixed resolution for latitude and longitude to 34 bits (TIA1057, Figure 9)
#define LLDPMED_ALTITUDE_RES       30 // Fixed resolution to 30 bits (TIA1057, Figure 9)

// The switch is by default not CDP aware
#define LLDP_CDP_AWARE_DEFAULT FALSE

// Default operation mode
#define LLDPMED_DEVICE_TYPE_DEFAULT VTSS_APPL_LLDP_MED_CONNECTIVITY


const char *lldp_error_txt(mesa_rc rc);

// Define the size of the entry table.
#define LLDP_ENTRIES_TABLE_SIZE sizeof(vtss_appl_lldp_remote_entry_t) *  LLDP_REMOTE_ENTRIES

/**Max. string length for Country Code (including '\0'), Figure 10, TIA1057*/
#define VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN 3

/************************/
/* Configuration         */
/************************/
// Common configuration that is found at run-time.
typedef struct {
    vtss_common_macaddr_t mac_addr ; /* Mac address*/
} lldp_conf_run_time_t;

// Configuration that is controlled by the user.
typedef struct {
    vtss_appl_lldp_common_conf_t common;      // Configuration that is common for all switches in a stack
    CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port; // Configuration for each port.
#ifdef VTSS_SW_OPTION_LLDP_MED
    vtss_appl_lldp_med_policy_t  policies_table[LLDPMED_POLICIES_CNT];

    /** List of policies enabled for the ports*/
    CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT, VTSS_APPL_CAP_LLDPMED_POLICIES_CNT> port_policies;
#endif
} lldp_user_conf_t;

//
// Functions
//
// Function for getting an unique port id for each port in a stack.
// In : iport - Internal port number (starting from 0)
// Return : Unique port id
mesa_port_no_t       lldp_mgmt_get_unique_port_id(mesa_port_no_t iport);
mesa_rc              lldp_init(vtss_init_data_t *data);
mesa_rc              lldp_conf_get(vtss_appl_lldp_port_conf_t *conf);
mesa_rc              lldp_common_local_conf_get(vtss_appl_lldp_common_conf_t *conf);
void                 lldp_send_frame(lldp_port_t port_no, lldp_u8_t *frame, lldp_u16_t len);
void                 lldp_mgmt_last_change_ago_to_str(time_t last_change_ago, char *last_change_str);
void                 lldp_something_has_changed(void);

// Function that must be called when ever an entry is created, deleted or modified
void lldp_entry_changed(vtss_appl_lldp_remote_entry_t *entry);

int lldp_mgmt_get_notification_interval(BOOL crit_region_not_set);
mesa_rc lldp_mgmt_set_notification_interval(int notification_interval);
int lldp_mgmt_get_notification_ena(lldp_port_t port, BOOL crit_region_not_set);
mesa_rc lldp_mgmt_set_notification_ena(int notification_ena, lldp_port_t port);
int lldpmed_mgmt_get_notification_ena(lldp_port_t port);
mesa_rc lldpmed_mgmt_set_notification_ena(int notification_ena, lldp_port_t port);

void lldp_mgmt_entry_updated_callback_register(lldp_callback_t cb);
void lldp_mgmt_entry_updated_callback_unregister(lldp_callback_t cb);

mesa_ipv4_t lldp_ip_addr_get(mesa_port_no_t iport);

#ifdef VTSS_SW_OPTION_LLDP_MED
lldp_u8_t lldp_mgmt_lldpmed_coordinate_location_tlv_add(lldp_u8_t *buf);
lldp_u8_t lldp_mgmt_lldpmed_civic_location_tlv_add(lldp_u8_t *buf);
lldp_u8_t lldp_mgmt_lldpmed_ecs_location_tlv_add(lldp_u8_t *buf);
void lldpmed_mib_init(void);
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
void vtss_appl_lldp_json_init(void);
#endif

void lldp_system_name(char *sys_name, char get_name);

extern "C" void lldp_mib_init(void);

mesa_rc lldp_conf_port_policy_get(mesa_port_no_t iport, u8 if_policy_index, BOOL *enabled);

mesa_rc lldp_conf_policy_get(u8                          policy_index,
                             vtss_appl_lldp_med_policy_t *conf);

/**
 * Get pointer to the remote entries table
 *
 *  Purpose : To get the pointer to the remote neighbor entries table in order to read the entries.
 *            The table is quite big, so instead of copying the whole table over the stack, a pointer
 *            directly to the table is provided instead. Before accessing the table you MUST lock the mutex
 *            with lldp_mgmt_lock function, in order to protect for simultaneously access.
 *            Once done you MUST unlock the mutex with vtss_appl_lldp_mutex_unlock.
 *            If you need to work with the entries table for longer time (e.g. more than 1 second) you need to take copy of the table.
 *
 *  \return VTSS_RC_OK if pointer point to a valid entries table, else error code.
 */
vtss_appl_lldp_remote_entry_t *vtss_appl_lldp_entries_get();

/**
 * Locking LLDP mutex for avoiding simultaneously access
 */
void vtss_appl_lldp_mutex_lock(void);

/**
 * Un-Locking LLDP mutex
 */
void vtss_appl_lldp_mutex_unlock(void);

/**
* Assert LLDP mutex
*/
void vtss_appl_lldp_mutex_assert(const char *file, int line);


/** The standard does specify the how many remote (neighbors) entries a device shall be able to hold (IEEE 802.1AB-2005, Section 10.3.4). Per customers request we have set the number of entries to 4 times the number of device ports.*/
#define LLDP_REMOTE_ENTRIES (fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * 4)

/**
 * Get configuration that are specific for the interfaces/port.
 *
 *  Purpose : Getting array with the LLDP configuration that are specific interfaces/ports.
 *
 * \param conf [IN] Pointer to the where to put the configuration. The pointer shall point at this type "vtss_appl_lldp_port_conf_t conf[VTSS_PORTS]"
 *
 *  \return VTSS_RC_OK if configuration where applied correctly, else error code
 */
mesa_rc lldp_mgmt_conf_get(vtss_appl_lldp_port_conf_t *conf);

/**
 * Set configuration that are specific for interfaces/ports.
 *
 *  Purpose : To Set the LLDP configuration that are specific for interfaces/ports.
 *
 * \param conf [IN] Pointer to the new configuration. The pointer shall point at this type "vtss_appl_lldp_port_conf_t conf[VTSS_PORTS]"
 *
 *  \return VTSS_RC_OK if configuration were applied correctly, else error code
 */
mesa_rc lldp_mgmt_conf_set(vtss_appl_lldp_port_conf_t *conf);

/**
 * Getting the configuration tude parameters (alti-, lati, and longi-tude) as long which can be used for uses interface.
 *
 *  The tude values are floating point, but since we don't support floating point the are given as long. E.g. 3.4566 is giving as 34566.
 *  For latitude and longitude there are TUDE_DIGITS digits, and for altitude there are ALTITUDE_DIGITS
 *
 * \param conf [IN] Pointer to the configuration.
 *
 * \param altitude [IN] Altitude value
 * \param latitude [IN] Latitude value
 * \param longitude [IN] Longitude value
 *
 *  \return VTSS_RC_OK if convertion was done, and values are within valid range, else error code
 */
mesa_rc lldp_tudes_as_long(const vtss_appl_lldp_common_conf_t *conf, long *altitude, long *longitude, long *latitude);

/**
  * Getting the latitude direction based on the latitude 2s complement as specified in rfc3825
  * \param latitude [IN] Latitude in 2' complement
  *
  * return direction
  */
vtss_appl_lldp_med_latitude_dir_t get_latitude_dir(u64 latitude);

/**
  * Getting the longitude direction based on the longitude 2s complement as specified in rfc3825
  * \param longitude [IN] Longitude in 2' complement
  *
  * return direction
  */
vtss_appl_lldp_med_longitude_dir_t get_longitude_dir(u64 longitude);

#endif // _LLDP_API_H_

