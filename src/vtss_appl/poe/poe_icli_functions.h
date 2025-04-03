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

#ifndef _VTSS_ICLI_POE_H_
#define _VTSS_ICLI_POE_H_

#include "icli_api.h" // for icli_stack_port_range_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file
 * \brief PoE iCLI functions
 * \details This header file describes PoE iCLI functions
 */



/**
 * \brief Function for displaying PoE status
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param has_interface [IN] TRUE if the user want to display a specific interface (port)
 * \param list [IN] port list of which interfaces to display PoE status.
 * \return None.
 **/
void poe_icli_show(i32 session_id, BOOL has_interface, icli_stack_port_range_t *list);

/**
 * \brief Function for configuring PoE mode
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param has_poe [IN] TRUE if the user want to set PoE to PoE mode (15.4W).
 * \param has_poe_plus [IN] TRUE if the user want to set PoE to PoE mode (30W).
 * \param plist [IN] List of interfaces to configure.
 * \param no [IN] TRUE is user want to set mode to default value
 * \return VTSS_RC_OK if mode was set correctly, else error code.
 **/
mesa_rc poe_icli_mode(i32 session_id, BOOL has_poe, BOOL has_poe_plus, icli_stack_port_range_t *plist, BOOL no);

/**
 * \brief Function for displaying PoE status
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param has_interface [IN] TRUE if the user want to display a specific interface (port)
 * \param list [IN] port list of which interfaces to display PoE debug status.
 * \return None.
 **/
void poe_icli_debug_show(i32 session_id, BOOL has_interface, icli_stack_port_range_t *list);


/**
 * \brief Function for displaying PoE counters
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \return None.
 **/
void poe_icli_debug_show_poe_error_counters(i32 session_id, BOOL has_interface, icli_stack_port_range_t *list);


/**
 * \brief Function for displaying PoE individual masks status
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \return None.
 **/
void poe_icli_individual_masks_show(i32 session_id);



/**
 * \brief Function for displaying PoE i2c status
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \return None.
 **/
void poe_icli_i2c_status_show(i32 session_id);


/**
 * \brief Function for configuring PoE priority
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param has_low [IN] TRUE if the user want to priority to low.
 * \param has_high [IN] TRUE if the user want to priority to high.
 * \param has_critical [IN] TRUE if the user want to priority to critical.
 * \param plist [IN] List of interfaces to configure.
 * \param no [IN] TRUE is user want to set mode to default value
 * \return None.
 **/
void poe_icli_priority(i32 session_id, BOOL has_low, BOOL has_high, BOOL has_critical, icli_stack_port_range_t *plist, BOOL no);




/**
 * \brief Function for configuring PoE power management
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param has_dynamic [IN] TRUE if the user want pm mode dynamic.
 * \param has_static [IN] TRUE if the user want pm mode static.
 * \param has_hybrid [IN] TRUE if the user want pm mode hybrid.
 * \param plist [IN] List of interfaces to configure.
 * \param no [IN] TRUE is user want to set pm mode to default value
 * \return None.
 **/
void poe_icli_power_management(i32 session_id, BOOL has_dynamic, BOOL has_static, BOOL has_hybrid, icli_stack_port_range_t *plist, BOOL no);

/**
 * \brief Function for configuring PoE type
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param has_type3Pwr15w [IN] TRUE if the user want to set type 1.
 * \param has_type3Pwr30w [IN] TRUE if the user want to set type 2.
 * \param has_type3Pwr60w [IN] TRUE if the user want to set type 3.
 * \param has_type4Pwr90w [IN] TRUE if the user want to set type 4.
 * \param plist [IN] List of interfaces to configure.
 * \param no [IN] TRUE is user want to set mode to default value
 * \return None.
 **/
void poe_icli_poe_type(i32 session_id, BOOL has_type3Pwr15w, BOOL has_type3Pwr30w, BOOL has_type3Pwr60w, BOOL has_type4Pwr90w, icli_stack_port_range_t *plist, BOOL no);



/**
 * \brief Function for configuring PoE lldp
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param plist [IN] List of interfaces to configure.
 * \param no [IN] FALSE if the user want to Enable poe lldp.
 * \return None.
 **/
void poe_icli_lldp(i32 session_id, icli_stack_port_range_t *plist, BOOL no);




/**
 * \brief Function for configuring PoE max cable length
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param plist [IN] List of interfaces to configure.
 * \param no [IN] FALSE if the user want to set max cable
 *        length.
 * \return None.
 **/
void poe_icli_poe_cable_length(i32 session_id, BOOL has_max10, BOOL has_max30, BOOL has_max60, BOOL has_max100, icli_stack_port_range_t *plist, BOOL no);


/**
 * \brief Function for configuring PoE capacitor detection
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param no [IN] TRUE is user want to set mode to default value
 * \return None.
 **/
void poe_icli_cap_detect_set(i32 session_id, BOOL no);

/**
 * \brief Function for configuring PoE interruptible power
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param val [IN] TRUE to enable interruptible power, FALSE to disable
 * \return None.
 **/
void poe_icli_interruptible_power_set(i32 session_id, BOOL val);


/**
 * \brief Function for sending poe debug messgae
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param iport [IN] is the PoE MCU index
 * \param char* var [IN] is the command string
 * \param str_len [IN] is the len of the command string
 * \return None.
 **/
void poe_icli_debug_access(i32 session_id, u32 iport, char *var, u32 str_len);



/**
 * \brief Function for configuring PoE interruptible power
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param val [IN] TRUE to enable interruptible power, FALSE to disable
 * \return None.
 **/
void poe_icli_pd_auto_class_request_set(i32 session_id, BOOL val);


/**
 * \brief Function for configuring PoE poe support
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param val [IN] TRUE to enable interruptible power, FALSE to disable
 * \return None.
 **/

void poe_icli_legacy_pd_class_mode_set(i32 session_id, BOOL has_standard, BOOL has_poh, BOOL has_ignore_pd_class, BOOL no);


/** Selecting which power to configure.*/
typedef enum {VTSS_POE_ICLI_POWER_SUPPLY,
              VTSS_POE_ICLI_SYSTEM_RESERVE_POWER
             }
vtss_poe_icli_power_conf_t;

/**
 * \brief Function for configuring maximum power for the power supply
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param has_sid[IN] TRUE if user has specified a specific sid
 * \param usid[IN] User switch ID.
 * \param no [IN] TRUE is user want to set mode to default value
 * \return None.
 **/
void poe_icli_power_supply(i32 session_id, u32 value, BOOL no, vtss_poe_icli_power_conf_t power_type);

/**
 * \brief Function for getting the current power supply status
 *
 * \param session_id [IN] The session id used by iCLI print.
 *
 * \return None.
 **/
void poe_icli_get_power_in_status(i32 session_id);

#if 0
}
#endif
/**
 * \brief Function for getting the current led color of power supplies
 *
 * \param session_id [IN] The session id used by iCLI print.
 *
 * \return None.
 */
void poe_icli_get_power_in_led(i32 session_id);

/**
 * \brief Function for getting the current led color indicating PoE status
 *
 * \param session_id [IN] The session id used by iCLI print.
 *
 * \return None.
 */
void poe_icli_get_status_led(i32 session_id);

/**
 * \brief Function for initializing ICFG
 **/
mesa_rc poe_icfg_init(void);

/**
 * \brief Function for at runtime getting information if PoE AT
 *        is supported for the currently selected interfaces
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_at_icli_interface_runtime_supported(u32                session_id,
                                             icli_runtime_ask_t ask,
                                             icli_runtime_t     *runtime);


/**
 * \brief Function for at runtime getting information if PoE BT
 *        is supported for the currently selected interfaces
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_bt_icli_interface_runtime_supported(u32                session_id,
                                             icli_runtime_ask_t ask,
                                             icli_runtime_t     *runtime);


/**
 * \brief Function for at runtime getting information if PoE
 *        AT+BT are supported for the currently selected
 *        interfaces
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_at_bt_icli_interface_runtime_supported(u32                session_id,
                                                icli_runtime_ask_t ask,
                                                icli_runtime_t     *runtime);


/**
 * \brief Function for at runtime getting information if PoE is supported at all.
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_icli_runtime_supported(u32                session_id,
                                icli_runtime_ask_t ask,
                                icli_runtime_t     *runtime);


/**
 * \brief Function for at runtime determine whether startup config is
 *  being loaded during initialization
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_icli_loading_startup_config(uint32_t session_id,
                                     icli_runtime_ask_t ask,
                                     icli_runtime_t     *runtime);
/**
 * \brief Function for at runtime getting information if PoE power supply is external
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_icli_external_power_supply(u32                session_id,
                                    icli_runtime_ask_t ask,
                                    icli_runtime_t     *runtime);


/**
 * \brief Function for at runtime getting information if PoE capacitor detect is supportes
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_icli_capacitor_detect(u32                session_id,
                               icli_runtime_ask_t ask,
                               icli_runtime_t     *runtime);


/**
 * \brief Function for at runtime getting information if PoE interruptible power is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_icli_interruptible_power(u32                session_id,
                                  icli_runtime_ask_t ask,
                                  icli_runtime_t     *runtime);


/**
 * \brief Function for at runtime getting information if PoE
 *        interruptible power is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_icli_pd_auto_class_request(u32                session_id,
                                    icli_runtime_ask_t ask,
                                    icli_runtime_t     *runtime);



/**
 * \brief Function for at runtime getting information if PoE
 *        interruptible power is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_icli_legacy_pd_class_mode(u32                session_id,
                                   icli_runtime_ask_t ask,
                                   icli_runtime_t     *runtime);

/**
 * \brief Function for at runtime getting the range of configurable power supply value.
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask [IN]  Asking
 * \param runtime [IN]  Pointer to where to put the "answer"
 * \return TRUE if runtime contains valid information.
 **/
BOOL poe_icli_runtime_power_range(u32             session_id,
                                  icli_runtime_ask_t ask,
                                  icli_runtime_t     *runtime);



mesa_rc poe_icli_firmware_upgrade(u32 session_id, mesa_port_no_t iport, const char *path, BOOL has_built_in, BOOL has_brick);

#ifdef __cplusplus
}
#endif

#endif // _VTSS_ICLI_POE_H_
