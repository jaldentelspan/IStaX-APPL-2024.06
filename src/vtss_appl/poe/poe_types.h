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

#ifndef _VTSS_APPL_POE_TYPES_H_
#define _VTSS_APPL_POE_TYPES_H_

typedef enum {
    POE_CAP_DETECT_DISABLED,
    POE_CAP_DETECT_ENABLED
} poe_cap_detect_t; // Capacitor detection enabled (or not)

/* typedef enum { */
/*     UNKNOWN, */
/*     PRIMARY, */
/*     BACKUP, */
/*     RESERVED */
/* } poe_power_source_t; // power source, see TIA-1057 table 16 or IEEE 802.3at table 33-22 (bits 5:4) */


/* PoE info for the switch */
typedef struct 
{
    char Firmware_version[33];
    //char Product_name[33];
    //char SN[33];
} poe_info_t;


/**********************************************************************
* Struct for defining PoE capabilites for each port
**********************************************************************/
typedef struct {
    BOOL available;    /* True is PoE is available for the port  */
    u8   pse_pairs_control_ability;  // pethPsePortPowerPairsControlAbility, rfc3621
    u8   pse_power_pair;             // pethPsePortPowerPairs, rfc3621
} poe_entry_t;


/**********************************************************************
 ** Status structs
 **********************************************************************/

/* PoE status for lldp enabled device */
typedef struct {
    u16 pse_allocated_power_dw;
    u16 pd_requested_power_dw;
    u8  pse_power_type;
    u8  power_class;
    u8  pse_power_pair;
    u8  mdi_power_status;
    u8  cable_len;
    u16 power_indicator;
    meba_poe_port_pse_prebt_port_type_t port_type_prebt_af_at_poh;
    uint8_t  layer2_execution_status;

    // elements for TLV type 3 and type 4 extensions
    u16  requested_power_mode_a_dw;
    u16  requested_power_mode_b_dw;
    u16  pse_alloc_power_alt_a_dw;
    u16  pse_alloc_power_alt_b_dw;
    u16  power_status;
    u8   system_setup;
    u16  pse_maximum_avail_power_dw;
    u8   auto_class;
} poe_pse_data_t;


/* PoE status for the switch */
typedef struct {

    poe_individual_mask_info_t  tPoe_individual_mask_info;

    // Port status
    CapArray<vtss_appl_poe_port_status_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_status;
    // List with the poe chip found for corresponding port.
    CapArray<meba_poe_chip_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_chip_found;
    // List with the lldp configured parameters for corresponding port.
    CapArray<poe_pse_data_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_pse_data;

    mesa_bool_t pwr_in_status1;                  /**< first power supply status */
    mesa_bool_t pwr_in_status2;                  /**< second power supply status */
    mesa_poe_milliwatt_t   calculated_total_power_used_mw;     /**< Total Power in DeciWatt currently used */
    uint32_t               calculated_total_power_reserved_dw; /**< Total amount of power reserved by the PDs */
    mesa_poe_milliampere_t calculated_total_current_used_ma;   /**< Total amount of current in mA currently used */

    mesa_bool_t       is_chip_state_ok;

    // in case of static parameters:  max_number_of_poe_ports = max number of ports
    // in case of dynamic parameters:  max_number_of_poe_ports as given from appl
    uint8_t           max_number_of_poe_ports;

    // counter the number of PoE i2c driver tx error
    uint32_t i2c_tx_error_counter;

    // poe firmware info
    uint8_t  prod_number_detected;
    uint16_t sw_version_detected;
    uint8_t  sw_version_high_detected;
    uint8_t  sw_version_low_detected;
    uint8_t  param_number_detected;
    uint8_t  prod_number_from_file;
    uint16_t sw_version_from_file;
    uint8_t  param_number_from_file;

    uint8_t  build_number;
    uint16_t internal_sw_number;
    uint16_t asic_patch_number;

    vtss_appl_poe_controller_type_t  ePoE_Controller_Type;

    uint8_t vmain_out_of_range;

    mesa_bool_t is_bt;

    //--- telemetry info from PoE MCU ---//

    // The sum of measured consumed power (Iport x Vmain), from all logical ports that are
    // delivering power. If the value exceeds Power Budget limit, ports are disconnected. The units
    // are in Watts.
    uint16_t power_consumption_w;

    // The sum of all logical ports reflected power, that are delivering power, based on the PM1 and
    // PM2 settings (combination of TPPL_BT values and measured port power).
    // If the value exceeds Power Budget limit, ports are disconnected. The units are in Watts.
    uint16_t calculated_power_w;

    // How much calculated power is available in the system till it reaches to the power limit.
    // Available power = (Power limit ? Calculated power consumption). The units are in Watts.
    uint16_t available_power_w;

    // The disconnection power level of a specific power bank. If system power consumption
    // exceeds this value and the power bank is active, ports are disconnected due to over power.
    uint16_t power_limit_w;

    // The current active power bank index that was read from the first PD69208 device.
    uint8_t  power_bank;

    // Actual momentary measured system main voltage in 0.1 V step.
    uint16_t vmain_voltage_dv;

    // Actual momentary current in mA step.
    uint16_t imain_current_ma;

    char     version[MEBA_POE_VERSION_STRING_SIZE];
} poe_status_t;

/**********************************************************************
 ** Configuration structs
 **********************************************************************/
#define POE_LLDP_EXECUTE                    (1 << 0)
#define POE_LLDP_DISABLE                    (1 << 1)
#define POE_LLDP_POWER_RESERVE_MODE_ENABLE  (1 << 2)

/* Struct for configuration through lldp */
typedef struct {
    u8  type;
    u16 pd_requested_power_dw;
    u16 pse_allocated_power_dw;
} poe_pd_data_t;

/* Struct for configuration through lldp with tlv type 3 & 4*/
typedef struct {
    u8  type;
    u16 pd_requested_power_dw;
    u16 pse_allocated_power_dw;
    u16 requested_power_mode_a_dw;
    u16 requested_power_mode_b_dw;
    u16 pse_alloc_power_alt_a_dw;
    u16 pse_alloc_power_alt_b_dw;
    u16 power_status;
    u8  system_setup;
    u16 pse_maximum_avail_power;
    u8  auto_class;
    u32 power_down;
} poe_pd_bt_data_t;

/* Configuration for the local switch */
typedef struct {

    /* The power that the power supply can give [W] */
    int power_supply_max_power_w;

    /* Some customers requires some spare power for powering the system. E.g. they like to configure a power supply of 180W, but only give 170 W for PoE. */
    int system_power_usage_w;

    /** Signal if the end user is allowed to configure the PSU capacity. If set
     * to false, then the 'def - system_consumed' will be used, and the user can
     * not overwrite that. */
    mesa_bool_t power_supply_user_configurable;

    /* priority for each port */
    CapArray<vtss_appl_poe_port_power_priority_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> priority ;

    /* type for each port */
    CapArray<vtss_appl_poebt_port_type_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> bt_pse_port_type ;

    /* It is possible to select mode (dis, std , plus) for each port individually.*/
    CapArray<vtss_appl_poe_port_mode_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> poe_mode;

    /* It is possible to select bt_port_pm_mode globally for all ports .*/
    CapArray<vtss_appl_poebt_port_pm_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> bt_port_pm_mode;

    /* It is possible to enable/disable mode for each port individually.*/
    CapArray<vtss_appl_poe_port_lldp_disable_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldp_disable;

    /* cable length for each port */
    CapArray<vtss_appl_poe_port_cable_length_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> cable_length ;

    poe_cap_detect_t                      cap_detect;           // Capacitor detection enabled (or not)
    mesa_bool_t                           interruptible_power;
    mesa_bool_t                           pd_auto_class_request;
    vtss_appl_poe_legacy_pd_class_mode_t  global_legacy_pd_class_mode;

    //    vtss_appl_poe_management_mode_t power_mgmt_mode; // Power management mode
    char firmware[64];
    
} poe_conf_t;

inline bool operator==(const poe_conf_t &a, const poe_conf_t &b)
{
#define TRY_CMP(X) if (a.X != b.X) return false
    TRY_CMP(power_supply_max_power_w);
    TRY_CMP(system_power_usage_w);
    TRY_CMP(priority);
    TRY_CMP(poe_mode);
    TRY_CMP(cap_detect);
    TRY_CMP(pd_auto_class_request);
    TRY_CMP(global_legacy_pd_class_mode);
    //    TRY_CMP(power_mgmt_mode);
#undef TRY_CMP
    return true;
}

inline bool operator!=(const poe_conf_t &a, const poe_conf_t &b)
{
    return !(a == b);
}

#endif  /* _VTSS_APPL_POE_TYPES_H_ */
