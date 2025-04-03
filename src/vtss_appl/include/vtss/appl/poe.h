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

/**
 * \file
 * \brief Public Power Over Ethernet(PoE) APIs.
 * \details This header file describes public power over Ethernet APIs.
 *          PoE is used to provide electrical power over standard Ethernet cable.
 *          This allows a single cable to provide both data connection and electrical power to devices.
 *          Device which supply power known as power sourcing equipment(PSE)
 *          and device which consume power known as powered device(PD).
 */

#ifndef __VTSS_APPL_POE_H__
#define __VTSS_APPL_POE_H__

#include <vtss/appl/types.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/interface.h>
#include <vtss/basics/enum-descriptor.h>    // For vtss_enum_descriptor_t

#ifdef __cplusplus
extern "C" {
#endif


/** Definition of error return codes return by the function in case of malfunctioning behavior*/
enum {
    /** Invalid switch id*/
    VTSS_APPL_POE_ERROR_ISID =  MODULE_ERROR_START(VTSS_MODULE_ID_POE),
    VTSS_APPL_POE_ERROR_NULL_POINTER,                /**< Unexpected reference to NULL pointer.*/
    VTSS_APPL_POE_ERROR_UNKNOWN_BOARD,               /**< The board type is unknown.*/
    VTSS_APPL_POE_ERROR_PRIM_SUPPLY_RANGE,           /**< Primary power supply value out of range.*/
    VTSS_APPL_POE_ERROR_CONF_ERROR,                  /**< Internal error - Configuration could not be done.*/
    VTSS_APPL_POE_ERROR_PORT_POWER_EXCEEDED,         /**< Maximum power for a port is exceeded.*/
    VTSS_APPL_POE_ERROR_NOT_SUPPORTED,               /**< Port is not supporting PoE.*/
    VTSS_APPL_POE_ERROR_FIRMWARE,                    /**< PoE firmware download failed.*/
    VTSS_APPL_POE_ERROR_FIRMWARE_VER,                /**< PoE firmware version not found.*/
    VTSS_APPL_POE_ERROR_FIRMWARE_VER_NOT_NEW,        /**< PoE firmware version not a new version.*/
    VTSS_APPL_POE_ERROR_DETECT,                      /**< PoE chip detection still in progress.*/
};

/**
 * \brief PoE Port status type
 */
typedef enum {
    /** Unknown state. should be placed first (because the web access to poe status before poe thread was initiated) */
    VTSS_APPL_POE_UNKNOWN_STATE = 0,
    /** PoE is turned OFF due to power budget exceeded on PSE. */
    VTSS_APPL_POE_POWER_BUDGET_EXCEEDED,
    /** No PD detected. */
    VTSS_APPL_POE_NO_PD_DETECTED,
    /** PSE supplying power to PD through PoE. */
    VTSS_APPL_POE_PD_ON,
    /** PD consumes more power than the maximum limit configured on the PSE port. */
    VTSS_APPL_POE_PD_OVERLOAD,
    /** PoE feature is not supported. */
    VTSS_APPL_POE_NOT_SUPPORTED,
    /** PoE feature is disabled on PSE. */
    VTSS_APPL_POE_DISABLED,
    /** PoE disabled due to interface shutdown */
    VTSS_APPL_POE_DISABLED_INTERFACE_SHUTDOWN,
    /** PoE PD fault */
    VTSS_APPL_POE_PD_FAULT,
    /** PoE pse fault */
    VTSS_APPL_POE_PSE_FAULT
} vtss_appl_poe_status_t;

/**
 * \brief PoE PD structure
 */
typedef enum {
    /** No device detected */
    VTSS_APPL_POE_PD_STRUCTURE_NOT_PERFORMED,
    VTSS_APPL_POE_PD_STRUCTURE_OPEN,

    /** No valid signature detected */
    VTSS_APPL_POE_PD_STRUCTURE_INVALID_SIGNATURE,

    /** Single signature PD over 4 pairs according to IEEE802.3bt */
    VTSS_APPL_POE_PD_STRUCTURE_4P_SINGLE_IEEE,

    /** Single signature PD over 4 pairs, Microsemi legacy protocol */
    VTSS_APPL_POE_PD_STRUCTURE_4P_SINGLE_LEGACY,

    /** Dual signature PD over 4 pairs according to IEEE802.3bt */
    VTSS_APPL_POE_PD_STRUCTURE_4P_DUAL_IEEE,

    VTSS_APPL_POE_PD_STRUCTURE_2P_DUAL_4P_CANDIDATE_FALSE,

    /** Single signature PD over 2 pairs according to IEEE802.3af */
    VTSS_APPL_POE_PD_STRUCTURE_2P_IEEE,

    /** Single signature PD over 2 pairs, Microsemi legacy protocol */
    VTSS_APPL_POE_PD_STRUCTURE_2P_LEGACY
} vtss_appl_poe_pd_structure_t;
/**
 * \brief PoE Port status
 */


typedef enum {
    VTSS_APPL_PD692X0_CONTROLLER_TYPE_AUTO_DETECTION = 0,
    VTSS_APPL_PD69200_CONTROLLER_TYPE ,
    VTSS_APPL_PD69210_CONTROLLER_TYPE ,
    VTSS_APPL_PD69220_CONTROLLER_TYPE ,
    VTSS_APPL_PD69200M_CONTROLLER_TYPE
} vtss_appl_poe_controller_type_t;


/**
 * \brief Types of port PoE mode
 */
typedef enum {
    /** PoE functionality is disabled. */
    VTSS_APPL_POE_MODE_DISABLED = 0,
    /** Enables PoE based on IEEE 802.3af standard,
        and provides power up to 15.4W(or 154 deciwatt) of DC power to powered device.
     */
    VTSS_APPL_POE_MODE_POE,
    /** Enabled PoE based on IEEE 802.3at standard,
        and provides power up to 30W(or 300 deciwatt) of DC power to powered device.
     */
    VTSS_APPL_POE_MODE_POE_PLUS
} vtss_appl_poe_port_mode_t;


/**
 * \brief Types of port PoE mode
 */
typedef enum {
    /** The port power that is used for power management purposes is dynamic (Iport x Vmain). */
    VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC = 0,
    /** The port power that is used for power management purposes is port TPPL_BT. */
    VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_STATIC,
    /** The port power that is used for power management purposes is dynamic for non LLDP/CDP/Autoclass ports and TPPL_BT for LLDP/CDP/Autoclass ports. */
    VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_HYBRID
} vtss_appl_poebt_port_pm_t;


/**
 * \brief Types of port power priority.
 * \details The port power priority, which determines the order in which the port will receive power.
 *          Ports with a higher priority will receive power before ports with a lower priority.
 */
typedef enum {
    /** Least port power priority. */
    VTSS_APPL_POE_PORT_POWER_PRIORITY_LOW = 0,
    /** Medium port power priority. */
    VTSS_APPL_POE_PORT_POWER_PRIORITY_HIGH,
    /** Highest port power priority. */
    VTSS_APPL_POE_PORT_POWER_PRIORITY_CRITICAL
} vtss_appl_poe_port_power_priority_t;


/**
 * \brief Types of port power priority.
 * \details The port power priority, which determines the order in which the port will receive power.
 *          Ports with a higher priority will receive power before ports with a lower priority.
 */
typedef enum {
    /** cable length till 10 meters. */
    VTSS_APPL_POE_PORT_CABLE_LENGTH_10 = 1,
    /** cable length till 30 meters. */
    VTSS_APPL_POE_PORT_CABLE_LENGTH_30 = 3,
    /** cable length till 60 meters. */
    VTSS_APPL_POE_PORT_CABLE_LENGTH_60 = 6,
    /** cable length till 100 meters. */
    VTSS_APPL_POE_PORT_CABLE_LENGTH_100 = 10
} vtss_appl_poe_port_cable_length_t;


/**
 * \brief Types of port power priority.
 * \details The port power priority, which determines the order in which the port will receive power.
 *          Ports with a higher priority will receive power before ports with a lower priority.
 */
typedef enum {
    /** Least port power priority. */
    VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_STANDARD = 0,
    /** Medium port power priority. */
    VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_POH,
    /** Highest port power priority. */
    VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_IGNORE_PD_CLASS
} vtss_appl_poe_legacy_pd_class_mode_t;


/**
 * \brief Types of port types.
 * \details The port type ,which determines the max class that
 *          will be accepted by the PSE.
 */
typedef enum {
    /** limit to type 3 15W . */
    VTSS_APPL_POE_PSE_PORT_TYPE3_15W = 0,
    /** limit to type 3 30W . */
    VTSS_APPL_POE_PSE_PORT_TYPE3_30W,
    /** limit to type 3 . */
    VTSS_APPL_POE_PSE_PORT_TYPE3_60W,
    /** limit to type 4 . */
    VTSS_APPL_POE_PSE_PORT_TYPE4_90W
} vtss_appl_poebt_port_type_t;


/**
 * \brief Types of port types.
 * \details The port type ,which determines the max class that 
 *          will be accepted by the PSE.
 */
typedef enum {
    VTSS_APPL_POE_PORT_PSE_IEEE802_3AF_operation = 0,
    VTSS_APPL_POE_PORT_PSE_IEEE802_3AF_AT_operation,
    VTSS_APPL_POE_PORT_PSE_POH_operation
} vtss_appl_poe_port_pse_prebt_port_type_t;


/** PoE port configuration */
typedef struct {
    /** Power requested by PD on alt_a */
    mesa_poe_milliwatt_t  alt_a_mw;
    /** Power requested by PD on alt_b */
    mesa_poe_milliwatt_t  alt_b_mw;
    /** Power requested by PD in single pair */
    mesa_poe_milliwatt_t  single_mw;
} vtss_appl_pd_request_power_t;


/** PoE port error counters */
typedef struct 
{
   /** When port turns off due to under-load */
   uint32_t udl_count;
   /** When port is overloaded */
   uint32_t ovl_count;
   /** When port turns off due to short circuit */
   uint32_t sc_count;
   /** When port failed in connection check or detection */
   uint32_t invalid_signature_count;
   /** When port turns off due to power management or port was
    *  not turned on due to any power limit */
   uint32_t power_denied_count;
}bt_port_counters_t;


/** PoE port status */
typedef struct {
    /** Port PoE status - as mapped by UNG */
    vtss_appl_poe_status_t  pd_status;

    /** remember last port error helps us to prevent sending too many syslog error messages on the same error */
    vtss_appl_poe_status_t  port_status_last_error;

    /** poe port has fault status - pwr budget exceed or pd fault or pse fault */
    bool      bISPoEStatusFault;

    /** internal Port PoE status - as reported by PoE MCU. */
    uint8_t  pd_status_internal;

    /** 0 - not valid , 1 - sspd , 2 - dspd. */
    uint8_t pd_type_sspd_dspd;

    /** description correlate to the internal Port PoE status. */
    char     pd_status_internal_description[100];

    /** port operation mode as configured by PoE MCU. */
    uint8_t  cfg_bt_port_operation_mode;  // bt std = 0,1,2,3 legacy 0x10,0x11,0x12,0x13

    /** port bt port pm mode as configured by PoE MCU. */
    vtss_appl_poebt_port_pm_t  cfg_bt_port_pm_mode;

    /** PoE Port PSE type according to 802.3bt. A PSE type 1 supports class 1-3 PDs,
        a PSE type 2 supports class 1-4 PDs, a PSE type 3 supports class 1-6 PDs, a PSE
        type 4 supports class 1-8 PDs.
     */
    vtss_appl_poebt_port_type_t  cfg_pse_port_type;

    /** configured port type for PREBT firmware - AF/AT/PoH */
    vtss_appl_poe_port_pse_prebt_port_type_t  cfg_pse_prebt_port_type ;

    /** assigned class a :Powered device(PD) negotiates a power class with sourcing equipment(PSE)
        during the time of initial connection, each class have a maximum supported power.
        Class assigned to PD is based on PD electrical characteristics.
        Value -1 means the PD attached to the port cannot advertise its power class.
    */
    
    u8   assigned_pd_class_a;

    /** assigned class b :In Poe BT using 4 pairs, powered device(PD) and sourcing equipment(PSE) negotiates
        power class is individually for the first set of pairs and the second set of pairs.
        Class assigned to PD is based on PD electrical characteristics.
        Value -1 means the PD attached to the port cannot advertise its power class.
    */
    u8   assigned_pd_class_b;
   
    /** PoE port PD class - The measured classification result of each pair set in a PSE logical port
        The measured class of the Primary alternative
    */
    u8    measured_pd_class_a;

    /** PoE port PD measured class of the Secondary alternative
        (second pair - used for poe-bt) The measured classification 
        result of each pair set in a PSE logical port The measured 
        class of the Secondary alternative. Classification values 
        range from 0 to 5. In case of SSPD or if class was not 
        performed, this field returns 0xC.
    */
    u8    measured_pd_class_b;

    /** PoE port PD class - The requested port class value is
        determined by the measured class result and internal logic
        configuration of the port.
        The requested class of the Primary alternative.
    */
    u8    requested_pd_class_a;

    /** PoE port PD requested class of the Secondary alternative (second pair - used for poe-bt)
        The requested port class value is determined by the measured
        class result and internal logic configuration of the port. 
        Classification values range from 0 to 5. In case of SSPD or 
        if class is not performed, this field returns 0xC. 
    */
    u8    requested_pd_class_b;

    /** PD structure describes single signature vs dual signature, 4 pair vs. 2 pair,
        IEEE vs legacy.
    */
    vtss_appl_poe_pd_structure_t pd_structure;

    /** The power (in miliwatt) requested by the PD. The value is only meaningful when
        the PD is on */
    mesa_poe_milliwatt_t   power_requested_mw;
   
    /** The power  (in miliwatt) limit of a working port referring to a specific PD, during ongoing power
        delivery. If port power exceeds the assigned power level,
	the port is disconnected
    */
    mesa_poe_milliwatt_t   power_assigned_mw;

    /** The power reserved for the PD. When power is allocated on basis of PD class, this
        number will be equal to the consumed power. When LLDP is used to allocated power, this
        will be the amount of power reserved through LLDP. The value is only meaningful
        when the PD is on. */
    uint16_t  power_reserved_dw;

    /** The power (in deciwatt) that the PD is consuming right now. The value is only
        meaningful when the PD is on.  */
    mesa_poe_milliwatt_t   power_consume_mw;

    /** The current(in mA) that the PD is consuming right now. The value is only
        meaningful when the PD is on.  */
    mesa_poe_milliampere_t   current_consume_ma;

    /** Power previously requested by PD  */
    vtss_appl_pd_request_power_t  previous_pd_request_power;

    /** port counters */
    bt_port_counters_t bt_port_counters;

} vtss_appl_poe_port_status_t;

/**
 * \brief Types of PoE power management mode
 */
typedef enum {
    /** Maximum port power determined by class,
        and power is managed according to power consumption.
     */
    VTSS_APPL_POE_CLASS_RESERVED,
    /** Maximum port power determined by class,
        and power is managed according to reserved power.
     */
    VTSS_APPL_POE_CLASS_CONSUMP,
    /** Maximum port power determined by allocated,
        and power is managed according to power consumption.
     */
    VTSS_APPL_POE_ALLOCATED_RESERVED,
    /** Maximum port power determined by allocated,
        and power is managed according to reserved power.
     */
    VTSS_APPL_POE_ALLOCATED_CONSUMP,
    /** Maximum port power determined by LLDP Media protocol,
        and power is managed according to reserved power.
    */
    VTSS_APPL_POE_LLDPMED_RESERVED,
    /** Maximum port power determined by LLDP Media protocol,
        and power is managed according to power consumption.
     */
    VTSS_APPL_POE_LLDPMED_CONSUMP
} vtss_appl_poe_management_mode_t;

/**
 * \brief PoE switch configuration.
 */
typedef struct {
    /** The max power that the power supply can give [W] */
    int power_supply_max_power_w;

    /** power consumed by system itself [W] */
    int system_power_usage_w;

    /** Used to control capacitor detection feature. */
    mesa_bool_t capacitor_detect;

    /** Used to control interruptible power feature. */
    mesa_bool_t interruptible_power;

    /** Used to control pd auto class request feature. */                                  
    mesa_bool_t pd_auto_class_request;

    /** Used to control legacy-pd-class-mode. */
    int global_legacy_pd_class_mode;

    /** Name of firmware file to be used for poe controller. When empty, use built-in
        default controller firmware */
    char firmware[63];
} vtss_appl_poe_conf_t;


/**
 * \brief Types of port LLDP PDU configuration.
 * \details This parameter is DISABLED, the poe part of received LLDP PDU shall be be disregarded,
 *          and transmitted LLDP PDUs shall not contain poe related data.
 */
typedef enum {
    /** Enable PoE data in LLDP PDUs. */
    VTSS_APPL_POE_PORT_LLDP_ENABLED = 0,
    /** Disable PoE data in LLDP PDUs. */
    VTSS_APPL_POE_PORT_LLDP_DISABLED = 1
} vtss_appl_poe_port_lldp_disable_t;

/**
 *  \brief PoE port configurable parameters.
 */
typedef struct {

    /** PoE Port PSE type according to 802.3bt. A PSE type 1 supports class 1-3 PDs,
        a PSE type 2 supports class 1-4 PDs, a PSE type 3 supports class 1-6 PDs, a PSE
        type 4 supports class 1-8 PDs.
     */
    vtss_appl_poebt_port_type_t           bt_pse_port_type;

    /** Indicate whether PoE is configured as VTSS_APPL_POE_MODE_POE or
        VTSS_APPL_POE_MODE_POE_PLUS on a port.
    */
    vtss_appl_poe_port_mode_t             poe_port_mode;

    /** Configuration of port power management mode. */
    vtss_appl_poebt_port_pm_t             bt_port_pm;

    /** Indicate port power priority. */
    vtss_appl_poe_port_power_priority_t   priority;

    /** Indicate port port operation mode. */
    vtss_appl_poe_port_lldp_disable_t     lldp_disable;

    /** Indicate port lldp cable length. */
    vtss_appl_poe_port_cable_length_t     cable_length;
} vtss_appl_poe_port_conf_t;

/**
 *   \brief PoE platform specific port capabilities.
 */
typedef struct {
    /** Indicate whether port is PoE capable or not. */
    mesa_bool_t  poe_capable;
} vtss_appl_poe_port_capabilities_t;

/**
 *   \brief PoE platform specific psu capabilities.
 */
typedef struct {

    /** Indicates whether PoE MCU firmware is BT or AT
    */
    mesa_bool_t is_bt;
    
    /** Indicates whether the user can and need to specify the amount 
        of power the PSU can deliver */
    mesa_bool_t user_configurable;

    /** Indicates the max power in DeciWatt the PoE board can handle. No
        reason to use a bigger PSU than this. For systems with internal PSU, this
        is the size of the built-in PSU. */
    uint32_t    max_power_w;

    /** For systems where the switch itself is powered by the same PSU as used for
        the PoE functionality, this shall reflect the amount of power required to
        keep the switch operating. */
    uint32_t    system_reserved_power_w;

    /** Indicates whether the PSE supports detection of legacy PD devices, and that
        the user can configure this feature.
    */
    mesa_bool_t legacy_mode_configurable;

    /** Indicates whether switch can reset software without affecting PoE powered
        devices connected to the switch.
    */
    mesa_bool_t interruptible_power_supported;

    /** Indicates whether the PSE supports pd autoclass request and that
        the user can configure this feature.
    */
    mesa_bool_t pd_auto_class_request;

    /** Indicates whether the PSE supports legacy-pd-class-mode
    */
    mesa_bool_t legacy_pd_class_mode;

} vtss_appl_poe_psu_capabilities_t;

/**
 * \brief PoE power supply status
 */
typedef struct {
    mesa_bool_t pwr_in_status1;                   /**< first power supply status */
    mesa_bool_t pwr_in_status2;                   /**< second power supply status */
    mesa_poe_milliwatt_t   calculated_total_power_used_mw;      /**< Total Power in DeciWatt currently used */
    uint32_t               calculated_total_power_reserved_dw;  /**< Total amount of power reserved by the PDs */
    mesa_poe_milliampere_t calculated_total_current_used_ma;    /**< Total amount of current in mA currently used */

    //--- telemetry info from PoE MCU ---//

    /** The sum of measured consumed power (Iport x Vmain), from all logical ports that are
        delivering power. If the value exceeds Power Budget limit, ports are disconnected. The units
        are in Watts.
    */
    uint16_t power_consumption_w;

    /** The sum of all logical ports reflected power, that are delivering power, based on the PM1 and
        PM2 settings (combination of TPPL_BT values and measured port power).
        If the value exceeds Power Budget limit, ports are disconnected. The units are in Watts.
    */
    uint16_t calculated_power_w;

    /** How much calculated power is available in the system till it reaches to the power limit.
        Available power = (Power limit ? Calculated power consumption). The units are in Watts.
    */
    uint16_t available_power_w;

    /** The disconnection power level of a specific power bank. If system power consumption
        exceeds this value and the power bank is active, ports are disconnected due to over power.
    */
    uint16_t power_limit_w;

    /** The current active power bank that was read from the first PD69208 device.
     */
    uint8_t  power_bank;

    /** Actual momentary measured system main voltage in 0.1 V step.
     */
    uint16_t vmain_voltage_dv;

    /** Actual momentary current in mA step.
     */
    uint16_t imain_current_ma;
} vtss_appl_poe_powerin_status_t;

/**
 * \brief PoE power led status
 */
typedef enum {
    VTSS_APPL_POE_LED_NULL,        /* Unknown status*/
    VTSS_APPL_POE_LED_OFF,         /* LED Off */
    VTSS_APPL_POE_LED_GREEN,       /* LED Green */
    VTSS_APPL_POE_LED_RED,         /* LED Red */
    VTSS_APPL_POE_LED_BLINK_RED,   /* LED Blink Red */
} vtss_appl_poe_led_t;

/**
 * \brief PoE led status of two power supplies
 */
typedef struct {
    vtss_appl_poe_led_t pwr_led1;  /**< led color for the 1st power supply */
    vtss_appl_poe_led_t pwr_led2;  /**< led color for the 2nd power supply */
} vtss_appl_poe_powerin_led_t;

/**
 * \brief PoE status led
 */
typedef struct {
    vtss_appl_poe_led_t status_led;  /**< led color indicating PoE status */
} vtss_appl_poe_status_led_t;

/**
 * \brief Get PoE maximum power supply.
 *
 * \return Maximum power supply.
 */
 uint32_t vtss_appl_poe_supply_max_get(void);

/**
 * \brief Get PoE minimum power supply.
 *
 * \return Minimum power supply.
 */
 uint32_t vtss_appl_poe_supply_min_get(void);

/**
 * \brief Get PoE default power supply.
 *
 * \return Default power supply.
 */
 uint32_t vtss_appl_poe_supply_default_get(void);

/**
 * \brief Get power consumed by system itself (when system and PoE are powered by same PSU).
 *
 * \return Power cusumed by system.
 */
uint32_t vtss_appl_poe_system_power_usage_get(void);

/**
 * \brief True when PSU is external to PSE and size therefore must be configured by user
 *
 * \return Whether PSU is user configurable
 */
mesa_bool_t vtss_appl_poe_psu_user_configurable_get(void);

/**
 * \brief True if the mode of PD detection can be configured to legacy
 *
 * \return Whether PD legacy mode can be configured
 */
mesa_bool_t vtss_appl_poe_pd_legacy_mode_configurable_get(void);

/**
 * \brief True is the controller supports interruptible power supply, i.e
 *        continue power delivery while overall system do software reset.
 *
 * \return Whether interruptible power supply is supported.
 */
mesa_bool_t vtss_appl_poe_pd_interruptible_power_get(void);


/**
 * \brief True is the pd auto class requset is enabled.
 *
 * \return pd auto class requset feature state.
 */
mesa_bool_t vtss_appl_poe_pd_auto_class_request_get(void); 


/**
 * \brief True is the poe has legacy pd class mode support.
 *
 * \return poe poe_legacy pd class mode.
 */
mesa_bool_t vtss_appl_poe_legacy_pd_class_mode_get(void);


 /**
 * \brief Set PoE switch configuration.
 *
 * \param usid  [IN]: Switch id
 * \param conf  [IN]: PoE configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_poe_conf_set(vtss_usid_t                 usid,
                                const vtss_appl_poe_conf_t  *const conf);

/**
 * \brief Get PoE switch configuration.
 *
 * \param usid   [IN]:  Switch id
 * \param conf  [OUT]: PoE switch configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_poe_conf_get(vtss_usid_t           usid,
                                vtss_appl_poe_conf_t  *const conf);

/**
 * \brief Set PoE port configuration.
 *
 * \param ifIndex  [IN]: Interface index
 * \param conf     [IN]: PoE port configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_poe_port_conf_set(vtss_ifindex_t                   ifIndex,
                                     const vtss_appl_poe_port_conf_t  *const conf);

/**
 * \brief Get PoE port configuration.
 *
 * \param ifIndex   [IN]: Interface index
 * \param conf     [OUT]: PoE port configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_poe_port_conf_get(vtss_ifindex_t            ifIndex,
                                     vtss_appl_poe_port_conf_t *const conf);


/**
 * \brief Get PoE port status
 *
 * \param ifIndex     [IN]: Interface index
 * \param status     [OUT]: Port status
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_poe_port_status_get(vtss_ifindex_t              ifIndex,
                                       vtss_appl_poe_port_status_t *const status);

/**
 * \brief Get PoE port capabilities
 *
 * \param ifIndex       [IN]: Interface index
 * \param capabilities [OUT]: PoE platform specific port capabilities
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_poe_port_capabilities_get(vtss_ifindex_t                    ifIndex,
                                             vtss_appl_poe_port_capabilities_t *const capabilities);


/**
 * \brief Get PSU capabilities
 *
 * \param capabilities [OUT]: PoE platform specific PSU capabilities
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_poe_psu_capabilities_get(
     vtss_appl_poe_psu_capabilities_t *const capabilities);

/**
 * \brief Function for getting the current power supply status
 *
 * \param powerin_status [IN,OUT] The status of the power supply.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
 mesa_rc vtss_appl_poe_powerin_status_get(vtss_appl_poe_powerin_status_t *const powerin_status);

/**
 * \brief Function for getting the status of the 2 power leds on the PoE board.
 *
 * \param pwr_led [IN/OUT] Led status for power supply.
 *
 * \return VTSS_RC_OK  if operation is succeeded.
 **/
mesa_rc vtss_appl_poe_powerin_led_get(vtss_appl_poe_powerin_led_t *const pwr_led);

/**
 * \brief Function for getting PoE status led color
 *
 * \param status_led [IN/OUT] The current color of the PoE status led.
 *
 * \return VTSS_RC_OK if operation succeeded.
 **/
mesa_rc vtss_appl_poe_status_led_get(vtss_appl_poe_status_led_t *status_led);


/**
 * \brief Function for setting flag to enable poe thread to resume its operation using prod thread with its db parameters
 **/
void prod_db_file_read_done(void);


#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // __VTSS_APPL_POE_H__
