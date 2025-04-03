/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \file aps.h
 * \brief Public APS (Automatic Protection Switching) API
 * \details This header file describes the APS (Automatic Protection Switching)
 * control functions and types.
 */

#ifndef _VTSS_APPL_APS_H_
#define _VTSS_APPL_APS_H_

#include <vtss/appl/interface.h>
#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/cfm.hxx>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Definition of error return codes.
 * See also aps_error_txt() in aps.cxx.
 */
enum {
    VTSS_APPL_APS_RC_INVALID_PARAMETER = MODULE_ERROR_START(VTSS_MODULE_ID_APS), /**< Invalid parameter                                                */
    VTSS_APPL_APS_RC_INTERNAL_ERROR,                                             /**< Internal error. Requires code update                             */
    VTSS_APPL_APS_RC_NO_SUCH_INSTANCE,                                           /**< APS instance is not created                                      */
    VTSS_APPL_APS_RC_MODE,                                                       /**< Invalid mode                                                     */
    VTSS_APPL_APS_RC_WTR,                                                        /**< Invalid wait-to-restore value                                    */
    VTSS_APPL_APS_RC_HOLD_OFF,                                                   /**< Invalid hold-off-time value                                      */
    VTSS_APPL_APS_RC_INVALID_LEVEL,                                              /**< Invalid MEG level                                                */
    VTSS_APPL_APS_RC_INVALID_VLAN,                                               /**< Invalid VLAN                                                     */
    VTSS_APPL_APS_RC_INVALID_PCP,                                                /**< Invalid PCP                                                      */
    VTSS_APPL_APS_RC_INVALID_SMAC,                                               /**< SMAC not a unicast SMAC                                          */
    VTSS_APPL_APS_RC_INVALID_W_IFINDEX,                                          /**< Invalid working ifindex                                          */
    VTSS_APPL_APS_RC_INVALID_P_IFINDEX,                                          /**< Invalid protect ifindex                                          */
    VTSS_APPL_APS_RC_INVALID_SF_TRIGGER_W,                                       /**< Invalid SF trigger for W-port                                    */
    VTSS_APPL_APS_RC_INVALID_SF_TRIGGER_P,                                       /**< Invalid SF trigger for P-port                                    */
    VTSS_APPL_APS_RC_WMEP_MUST_BE_SPECIFIED,                                     /**< Working MEP must be specified when SF-trigger is MEP             */
    VTSS_APPL_APS_RC_PMEP_MUST_BE_SPECIFIED,                                     /**< Protect MEP must be specified when SF-trigger is MEP             */
    VTSS_APPL_APS_RC_W_P_MEP_IDENTICAL,                                          /**< Working and protecting MEP are the same                          */
    VTSS_APPL_APS_RC_W_P_IFINDEX_IDENTICAL,                                      /**< Working and protect ports are identical                          */
    VTSS_APPL_APS_RC_ANOTHER_INST_USES_SAME_W_PORT,                              /**< Another APS instance uses the same W port                        */
    VTSS_APPL_APS_RC_ANOTHER_INST_USES_SAME_P_PORT_AND_VLAN,                     /**< Another APS instance uses the same P port and VLAN               */
    VTSS_APPL_APS_RC_LIMIT_REACHED,                                              /**< The maximum number of APS instances is reached                   */
    VTSS_APPL_APS_RC_OUT_OF_MEMORY,                                              /**< Out of memory                                                    */
    VTSS_APPL_APS_RC_NOT_ACTIVE,                                                 /**< Instance is not active                                           */
    VTSS_APPL_APS_RC_COMMAND,                                                    /**< Invalid command                                                  */
    VTSS_APPL_APS_RC_COMMAND_FREEZE,                                             /**< In freeze mode, only way out is to clear the freeze              */
    VTSS_APPL_APS_RC_COMMAND_EXERCISE_UNIDIRECTIONAL,                            /**< EXER cannot be performed in unidirectional mode                  */
    VTSS_APPL_APS_RC_NOT_READY_TRY_AGAIN_IN_A_FEW_SECONDS,                       /**< APS is not yet ready. Try again in a few seconds                 */
    VTSS_APPL_APS_RC_HW_RESOURCES,                                               /**< Out of H/W resources                                             */
};

/**
 * Instance capability structure.
 */
typedef struct {
    uint32_t inst_cnt_max;       /**< Maximum number of creatable APS instances                                  */
    uint32_t wtr_secs_max;       /**< The maximum number of seconds vtss_appl_aps_conf_t::wtr_secs can be set to */
    uint32_t hold_off_msecs_max; /**< The maximum number of milliseconds vtss_appl_aps_conf_t::hold_off_msecs    */
} vtss_appl_aps_capabilities_t;

/**
 * \brief Get APS capabilities
 *
 * \param cap [OUT] Capabilities.
 *
 * \return VTSS_RC_OK
 */
mesa_rc vtss_appl_aps_capabilities_get(vtss_appl_aps_capabilities_t *cap);

/**
 * Signal fail can either come from the physical link on a given port or from a
 * Down-MEP.
 */
typedef enum {
    VTSS_APPL_APS_SF_TRIGGER_LINK, /**< Signal Fail comes from the physical port's link state */
    VTSS_APPL_APS_SF_TRIGGER_MEP,  /**< Signal Fail comes from a down-MEP                     */
} vtss_appl_aps_sf_trigger_t;

/**
 * Configuration for a particular protection switching port
 */
typedef struct {
    /**
     * Interface index of port.
     * Must always be filled in.
     */
    vtss_ifindex_t ifindex;

    /**
     * Selects whether Signal Fail (SF) comes from the link state of a given
     * interface, or from a Down-MEP.
     */
    vtss_appl_aps_sf_trigger_t sf_trigger;

    /**
     * Reference to a down-MEP that provides SF for the ring port that this
     * structure instance represents.
     *
     * The MEP's residence port must be the same as \p ifindex or the instance
     * will get an operational warning and use link as SF trigger until the
     * problem is remedied.
     *
     * Only used when sf_trigger is VTSS_APPL_APS_SF_TRIGGER_MEP.
     */
    vtss_appl_cfm_mep_key_t mep;
} vtss_appl_aps_port_conf_t;

/**
 * An APS instance can be in one of these three modes.
 */
typedef enum {
    VTSS_APPL_APS_MODE_ONE_FOR_ONE,                 /**< Bidirectional  1:1 architecture */
    VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL, /**< Unidirectional 1+1 architecture */
    VTSS_APPL_APS_MODE_ONE_PLUS_ONE_BIDIRECTIONAL,  /**< Bidirectional  1+1 architecture */
} vtss_appl_aps_mode_t;

/**
 * APS configuration structure.
 */
typedef struct {
    /**
     * Reference to the working port
     */
    vtss_appl_aps_port_conf_t w_port_conf;

    /**
     * Reference to the protect port
     */
    vtss_appl_aps_port_conf_t p_port_conf;

    /**
     * MD/MEG Level of L-APS PDUs we transmit.
     *
     * A Down-MEP on the protect port on the same level and VID as this one may
     * or may not exist and may or may not provide signal fail for the protect
     * port.
     *
     * Valid range is [0; 7], with 0 as default.
     */
    uint8_t level;

    /**
     * The VLAN on which L-APS PDUs are transmitted and received on the protect
     * port.
     *
     * APS instances on the same protect port must use different VLANs.
     *
     * Untagged L-APS PDUs must have \p vlan set to 0.
     */
    mesa_vid_t vlan;

    /**
     * The PCP value used in the VLAN tag of the L-APS PDUs.
     *
     * Valid range is [0; 7], with 7 as default.
     *
     * Only used if \p vlan is non-zero.
     */
    mesa_pcp_t pcp;

    /**
     * Source MAC address (must be unicast) used in L-APS PDUs sent on the
     * protect port.
     *
     * If all-zeros, the port's native MAC address will be used.
     */
    mesa_mac_t smac;

    /**
     * This selects the architecture and direction of the APS instance.
     *
     * In bidirectional mode, both ends attempt to set their selectors
     * identically using a.o. APS PDU information.
     *
     * In unidirectional mode, the sink exclusively determines the selector
     * settings. However, transmission of APS PDUs can still be enabled in this
     * mode (see #tx_aps).
     */
    vtss_appl_aps_mode_t mode;

    /**
     * Choose whether this end transmits APS PDUs.
     * This member is only used when #mode is
     * VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL.
     *
     * In this mode, reception of APS PDUs are only used for informational
     * purposes and the contents are not used in choosing the sink's selector
     * setting.
     */
    mesa_bool_t tx_aps;

    /**
     * When set, the port recovery mode is revertive, that is, traffic switches
     * back to the working port after the condition(s) causing a switch has
     * cleared. In the case of clearing a command (e.g. forced switch), this
     * happens immediately. In the case of clearing of a defect, this generally
     * happens after the expiry of the WTR (Wait-To-Restore) timer.
     *
     * When cleared, the port recovery mode is non-revertive and traffic is
     * allowed to remain on the protect port after a switch reason has cleared.
     * This is generally accomplished by replacing the previous switch request
     * with a do not revert (DNR) request, which is low priority.
     */
    mesa_bool_t revertive;

    /**
     * When #revertive is true, this member tells how many seconds to wait
     * before restoring to the working port after a fault condition has cleared.
     *
     * Valid values are in range [1; vtss_appl_aps_capabilities_t::wtr_secs_max]
     * seconds with a default of 300 seconds.
     */
    uint32_t wtr_secs;

    /**
     * Hold-off timer value.
     *
     * When a new defect or more severe defect occurs, this event will not be
     * reported until a hold-off timer expires. When the hold-off timer expires,
     * it will be checked whether a defect still exists. If it does, that defect
     * will be reported to protection switching.
     *
     * The hold-off timer is measured in milliseconds, and valid values are in
     * the range [0; vtss_appl_aps_capabilities_t::hold_off_msecs_max] in steps
     * of 100 milliseconds.
     * Default is 0, which means immediate reporting of the defect.
     */
    uint32_t hold_off_msecs;

    /**
     * The administrative state of this APS instance (default false).
     * Set to true to make it function normally and false to make it cease
     * functioning.
     * When false, no LAPS PDUs are sent and no MEPs are snooped.
     */
    mesa_bool_t admin_active;
} vtss_appl_aps_conf_t;

/**
 * \brief Get default APS instance configuration
 *
 * \param conf [OUT] Default configuration
 *
 * \return Error code.
 */
mesa_rc vtss_appl_aps_conf_default_get(vtss_appl_aps_conf_t *conf);

/**
 * \brief Get APS instance configuration
 *
 * \param instance [IN]  Instance number
 * \param conf     [OUT] Configuration
 *
 * \return Error code.
 */
mesa_rc vtss_appl_aps_conf_get(uint32_t instance, vtss_appl_aps_conf_t *conf);

/**
 * \brief Set APS instance configuration
 *
 * \param instance [IN] Instance number.
 * \param conf [IN]     Configuration.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_aps_conf_set(uint32_t instance, const vtss_appl_aps_conf_t *conf);

/**
 * \brief Delete APS instance
 *
 * \param instance [IN] Instance number.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_aps_conf_del(uint32_t instance);

/**
 * \brief Instance iterator.
 *
 * This function returns the next defined APS instance number. The end is
 * reached when VTSS_RC_ERROR is returned.
 * The search for an enabled instance will start with 'prev_instance' + 1.
 * If 'prev_instance' pointer is NULL, the search start with the lowest
 * possible instance number.
 *
 * \param prev_instance [IN]  Instance number
 * \param next_instance [OUT] Next instance
 *
 * \return VTSS_RC_OK as long as next is valid.
 */
mesa_rc vtss_appl_aps_itr(const uint32_t *prev_instance, uint32_t *next_instance);

/**
 * The possible protection group commands
 */
typedef enum {
    VTSS_APPL_APS_COMMAND_NR,           /**< No request (not a command, only a status)  */
    VTSS_APPL_APS_COMMAND_LO,           /**< Lockout of protection (go to working)      */
    VTSS_APPL_APS_COMMAND_FS,           /**< Forced switch to protection                */
    VTSS_APPL_APS_COMMAND_MS_TO_W,      /**< Manual switch to working                   */
    VTSS_APPL_APS_COMMAND_MS_TO_P,      /**< Manual switch to protection                */
    VTSS_APPL_APS_COMMAND_EXER,         /**< Exercise of APS protocol                   */
    VTSS_APPL_APS_COMMAND_CLEAR,        /**< Clears LO, FS, MS, EXER or a WTR condition */
    VTSS_APPL_APS_COMMAND_FREEZE,       /**< Local Freeze of protection group           */
    VTSS_APPL_APS_COMMAND_FREEZE_CLEAR, /**< Only way to get out of freeze              */
} vtss_appl_aps_command_t;

/**
 * \brief Instance command structure.
 *
 * This structure is used to set or get the protection group command.
 */
typedef struct {
    /**
     * See vtss_appl_aps_command_t enum for possible requests (commands).
     */
    vtss_appl_aps_command_t command;
} vtss_appl_aps_control_t;

/**
 * \brief Get APS instance protection group control
 *
 * \param instance [IN]  Instance number
 * \param ctrl     [OUT] Protection group control
 *
 * \return Error code
 */
mesa_rc vtss_appl_aps_control_get(uint32_t instance, vtss_appl_aps_control_t *ctrl);

/**
 * \brief Set APS Instance protection group control.
 *
 * If current command is FREEZE, the next command must be FREEZE_CLEAR.
 * You may go from one command directly to another (e.g. LO to FS), but if you
 * want to undo the current command, you must issue a CLEAR.
 * You may not use NR in the call to set(). NR can only be returned by get().
 *
 * \param instance [IN] Instance number
 * \param ctrl     [IN] Protection group control
 *
 * \return Error code
 */
mesa_rc vtss_appl_aps_control_set(uint32_t instance, const vtss_appl_aps_control_t *ctrl);

/**
 * The operational state of an APS instance.
 * There are a few ways to not have the instance active. Each of them has its
 * own enumeration. Only when the state is VTSS_APPL_APS_OPER_STATE_ACTIVE, will
 * the APS instance be active and up and running.
 * However, there may still be operational warnings that may cause the instance
 * not to run optimally. See vtss_appl_aps_oper_warnings_t for a list of
 * possible warnings.
 *
 * The reason for having operational warnings rather than just an operational
 * state is that we only want an APS instance to be inactive if a true error
 * has occurred, or if the user has selected it to be inactive, because
 * if one of the warnings also resulted in making the APS instance inactive, we
 * might get loops in our network
 */
typedef enum {
    VTSS_APPL_APS_OPER_STATE_ADMIN_DISABLED,      /**< Instance is inactive, because it is administratively disabled. */
    VTSS_APPL_APS_OPER_STATE_ACTIVE,              /**< The instance is active and up and running.                     */
    VTSS_APPL_APS_OPER_STATE_INTERNAL_ERROR,      /**< Instance is inactive, because an internal error has occurred.  */
} vtss_appl_aps_oper_state_t;

/**
 * Operational warnings of an APS instance.
 *
 * If the operational state is VTSS_APPL_APS_OPER_STATE_ACTIVE, the APS instance
 * is indeed active, but it may be that it doesn't run as the administrator
 * thinks, because of configuration errors, which are reflected in the warnings
 * below.
 */
typedef enum {
    VTSS_APPL_APS_OPER_WARNING_NONE,                         /**< No warnings. If oper. state is VTSS_APPL_APS_OPER_STATE_ACTIVE, everything is fine.            */
    VTSS_APPL_APS_OPER_WARNING_WMEP_NOT_FOUND,               /**< Working MEP is not found. Using link-state for SF instead.                                     */
    VTSS_APPL_APS_OPER_WARNING_PMEP_NOT_FOUND,               /**< Protect MEP is not found. Using link-state for SF instead.                                     */
    VTSS_APPL_APS_OPER_WARNING_WMEP_ADMIN_DISABLED,          /**< Working MEP is administratively disabled. Using link-state for SF instead.                     */
    VTSS_APPL_APS_OPER_WARNING_PMEP_ADMIN_DISABLED,          /**< Protect MEP is administratively disabled. Using link-state for SF instead.                     */
    VTSS_APPL_APS_OPER_WARNING_WMEP_NOT_DOWN_MEP,            /**< Working MEP is not a Down-MEP. Using link-state for SF instead.                                */
    VTSS_APPL_APS_OPER_WARNING_PMEP_NOT_DOWN_MEP,            /**< Protect MEP is not a Down-MEP. Using link-state for SF instead.                                */
    VTSS_APPL_APS_OPER_WARNING_WMEP_AND_PORT_IFINDEX_DIFFER, /**< Working MEP's residence port is not that of the working port. Using link-state for SF instead. */
    VTSS_APPL_APS_OPER_WARNING_PMEP_AND_PORT_IFINDEX_DIFFER, /**< Protect MEP's residence port is not that of the protect port. Using link-state for SF instead. */
} vtss_appl_aps_oper_warning_t;

/**
 * The possible protection group states as described in G.8031 Annex A.
 * The letters that is used in the description of each state refer to the state
 * letters also described in Annex A of G.8031.
 */
typedef enum {
    VTSS_APPL_APS_PROT_STATE_NR_W,    /**< A: No Request Working       */
    VTSS_APPL_APS_PROT_STATE_NR_P,    /**< B: No Request Protect       */
    VTSS_APPL_APS_PROT_STATE_LO,      /**< C: Lockout                  */
    VTSS_APPL_APS_PROT_STATE_FS,      /**< D: Forced Switch            */
    VTSS_APPL_APS_PROT_STATE_SF_W,    /**< E: Signal Fail Working      */
    VTSS_APPL_APS_PROT_STATE_SF_P,    /**< F: Signal Fail Protect      */
    VTSS_APPL_APS_PROT_STATE_MS_TO_P, /**< G: Manual Switch to Protect */
    VTSS_APPL_APS_PROT_STATE_MS_TO_W, /**< H: Manual Switch to Working */
    VTSS_APPL_APS_PROT_STATE_WTR,     /**< I: Wait to Restore          */
    VTSS_APPL_APS_PROT_STATE_DNR,     /**< J: Do Not Revert            */
    VTSS_APPL_APS_PROT_STATE_EXER_W,  /**< K: Exercise Working         */
    VTSS_APPL_APS_PROT_STATE_EXER_P,  /**< L: Exercise Protect         */
    VTSS_APPL_APS_PROT_STATE_RR_W,    /**< M: Reverse Request Working  */
    VTSS_APPL_APS_PROT_STATE_RR_P,    /**< N: Reverse Request Protect  */
    VTSS_APPL_APS_PROT_STATE_SD_W,    /**< P: Signal Degrade Working   */
    VTSS_APPL_APS_PROT_STATE_SD_P,    /**< Q: Signal Degrade Protect   */
} vtss_appl_aps_prot_state_t;

/**
 *  The possible working or protecting port defect state.
 */
typedef enum {
    VTSS_APPL_APS_DEFECT_STATE_OK, /**< The Working/protecting port defect state is OK - not active       */
    VTSS_APPL_APS_DEFECT_STATE_SD, /**< The Working/protecting port defect state is Signal Degrade active */
    VTSS_APPL_APS_DEFECT_STATE_SF  /**< The Working/protecting port defect state is Signal Fail active    */
} vtss_appl_aps_defect_state_t;

/**
 * The possible transmitted or received APS requests according to G.8031,
 * Table 11-1. The enumeration values are selected according to that table.
 * The higher value, the higher priority.
 */
typedef enum {
    VTSS_APPL_APS_REQUEST_NR   = 0x0, /**< No Request              */
    VTSS_APPL_APS_REQUEST_DNR  = 0x1, /**< Do Not Revert           */
    VTSS_APPL_APS_REQUEST_RR   = 0x2, /**< Reverse Request         */
    VTSS_APPL_APS_REQUEST_EXER = 0x4, /**< Exercise                */
    VTSS_APPL_APS_REQUEST_WTR  = 0x5, /**< Wait-To-Restore         */
    VTSS_APPL_APS_REQUEST_MS   = 0x7, /**< Manual Switch           */
    VTSS_APPL_APS_REQUEST_SD   = 0x9, /**< Signal Degrade          */
    VTSS_APPL_APS_REQUEST_SF_W = 0xB, /**< Signal Fail for Working */
    VTSS_APPL_APS_REQUEST_FS   = 0xD, /**< Forced Switch           */
    VTSS_APPL_APS_REQUEST_SF_P = 0xE, /**< Signal Fail for Protect */
    VTSS_APPL_APS_REQUEST_LO   = 0xF  /**< Lockout                 */
} vtss_appl_aps_request_t;

/**
 * \brief APS information structure.
 *
 * This structure is used to contain transmitted or received APS information.
 *
 */
typedef struct {
    vtss_appl_aps_request_t request;   /**< Transmitted/Received request type according to G.8031 table 11-1      */
    uint8_t                 re_signal; /**< Transmitted/Received requested signal according to G.8031 figure 11-2 */
    uint8_t                 br_signal; /**< Transmitted/Received bridged signal according to G.8031 figure 11-2   */
} vtss_appl_aps_aps_info_t;

/**
 * Instance status data structure.
 */
typedef struct {
    /**
     * Operational state of this APS instance.
     *
     * The APS instance is inactive unless oper_state is
     * VTSS_APPL_APS_OPER_STATE_ACTIVE.
     *
     * When inactive, non of the remaining members of this struct are valid.
     * When active, the APS instance may, however, still have warnings. See
     * \p warnings_state for a list of possible warnings.
     */
    vtss_appl_aps_oper_state_t oper_state;

    /**
     * Operational warnings of this APS instance.
     *
     * The APS instance is error and warning free if \p oper_state is
     * VTSS_APPL_APS_OPER_STATE_ACTIVE and \p oper_warning is
     * VTSS_APPL_APS_OPER_WARNING_NONE.
     */
    vtss_appl_aps_oper_warning_t oper_warning;

    /**
     * Current protection state.
     */
    vtss_appl_aps_prot_state_t prot_state;

    /**
     * State of the working port after hold-off
     */
    vtss_appl_aps_defect_state_t w_state;

    /**
     * State of the protect port after hold-off
     */
    vtss_appl_aps_defect_state_t p_state;

    /**
     * Transmitted L-APS request
     */
    vtss_appl_aps_aps_info_t tx_aps;

    /**
     * Received L-APS request
     */
    vtss_appl_aps_aps_info_t rx_aps;

    /**
     * This applies to all modes.
     * If a LAPS PDU is received on the working interface, this member will be
     * set to true. It will be cleared 17.5 seconds after the last such PDU was
     * received.
     * See G.8021, Table 6-2, dFOP-CM (Configuration Mismatch)
     * See G.8031, 11.2.4 and 11.15.b.
     */
    mesa_bool_t dFOP_CM;

    /**
     * This applies to bidirectional switching, only.
     * If a LAPS PDU is received on the protect interface and at least one of
     * the following criteria is true:
     *  1) the B-bit differs from the expected B-bit (0 for 1+1 and 1 for 1:1
     *    architectures) or
     *  2) we are running bidirectional switching, and at least one of the A-
     *     or D-bits is cleared.
     * this member will be set. Once a valid LAPS PDU where these conditions are
     * When this happens, the selector will be released.
     * See G.8021, Table 6-2, dFOP-PM (Provisioning Mismatch).
     * See G.8031, 11.4 and 11.15.a.
     */
    mesa_bool_t dFOP_PM;

    /**
     * This applies to bidirectional switching, only.
     * If there is a "Requested Signal" mismatch in PDUs we transmit and PDUs
     * we receive, a 50 millisecond timer will be started. If that timer times
     * out, before the received and transmitted "Requested Signals" are
     * identical, this member will be set. It will be cleared as soon as a PDU
     * is received with no mismatch.
     * See G.8021, Table 6-2, dFOP-NR (No Response)
     * See G.8031, 11.15.c.
     */
    mesa_bool_t dFOP_NR;

    /**
     * This applies to bidirectional switching,  only.
     * If the protect interface does not have signal fail, but no LAPS PDUs are
     * received on that interface within the last 17.5 seconds, this member will
     * be set. The last received LAPS PDU's info will be used unless there is a
     * signal fail condition on the protect interface.
     * See G.8021, Table 6-2, dFOP-TO (TimeOut)
     * See G.8031, 11.2.4 and 11.15.d.
     *
     * Note that whenever the APS instance activates, this member will be
     * set to true (if in bidirectional mode).
     */
    mesa_bool_t dFOP_TO;

    /**
     * Source MAC address of last received LAPS PDU or all-zeros if no PDU has
     * been received.
     */
    mesa_mac_t smac;

    /**
     * Number of APS PDU frames transmitted.
     * Cleared by vtss_appl_aps_statistics_clear()
     */
    uint64_t tx_cnt;

    /**
     * Number of valid APS PDU frames received on the protect port. It may still
     * be that the APS info embedded inside the PDU doesn't match the currently
     * configured APS mode, but it is nonetheless, still counted as valid.
     * Cleared by vtss_appl_aps_statistics_clear()
     */
    uint64_t rx_valid_cnt;

    /**
     * Number of invalid APS PDU frames received on the protect port.
     * Cleared by vtss_appl_aps_statistics_clear()
     */
    uint64_t rx_invalid_cnt;
} vtss_appl_aps_status_t;

/**
 * \brief Get APS instance status
 *
 * \param instance [IN]  Instance number
 * \param status   [OUT] Instance status
 *
 * \return Error code.
 */
mesa_rc vtss_appl_aps_status_get(uint32_t instance, vtss_appl_aps_status_t *status);

/**
 * \brief Clear APS instance counters
 *
 * This will clear the three counters in vtss_appl_aps_status_t.
 *
 * \param instance [IN] Instance number
 *
 * \return Error code.
 */
mesa_rc vtss_appl_aps_statistics_clear(uint32_t instance);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_APPL_APS_H_ */

