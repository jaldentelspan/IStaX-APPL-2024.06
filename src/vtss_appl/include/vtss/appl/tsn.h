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
 * \file tsn.h
 *
 * \brief This API provides typedefs and functions for the
 * TSN module.
 */


#ifndef _VTSS_APPL_TSN_HXX_
#define _VTSS_APPL_TSN_HXX_

#include <vtss/appl/interface.h>  /* For vtss_ifindex_t        */
#include <vtss/appl/module_id.h>  /* For MODULER_ERROR_START() */

/**
 * TSN error codes (mesa_rc)
 */
enum {
    VTSS_APPL_TSN_INVALID_ARGUMENT = MODULE_ERROR_START(VTSS_MODULE_ID_TSN),      /**< Invalid argument (typically a NULL pointer) passed to a function          */
    VTSS_APPL_FP_FEATURE,                                                         /**< Frame preemption not present.                                             */
    VTSS_APPL_TAS_FEATURE,                                                        /**< Time Aware Shaper feature not present.                                    */
    /* TSN general module related errors */
    VTSS_APPL_TSN_PTP_TIME_NOT_READY,                                             /**< PTP Time not ready. Typically PTP time has not yet sychronized            */
    VTSS_APPL_TSN_WAIT_TIME_TOO_SMALL,
    VTSS_APPL_TSN_WAIT_TIME_TOO_BIG,
    VTSS_APPL_TSN_PTP_PORT_TOO_BIG,
    VTSS_APPL_TSN_INVALID_PROCEDURE,
    /* Frame Preemption related errors */
    VTSS_APPL_FP_INVALID_IFINDEX,                                                 /**< Invalid ifindex. It must represent a port                                 */
    VTSS_APPL_FP_INVALID_VERIFY_TIME,                                             /**< Invalid verify time. It must be in range [1,128]                          */
    VTSS_APPL_FP_INVALID_FRAG_SIZE,                                               /**< Invalid add_frag_size. It must be in range [0,3]                          */
    VTSS_APPL_FP_CUT_THROUGH,                                                     /**< It is not possible to enable both cut-through and frame preemption at the same time */
    VTSS_APPL_FP_10G_COPPER_PORT_NOT_SUPPORTED,
    /* Frame TAS related errors */
    VTSS_APPL_TAS_INVALID_IFINDEX,
    VTSS_APPL_TAS_INVALID_CYCLE_TIME,
    VTSS_APPL_TAS_INVALID_CYCLE_TIME_EXTENSION,
    VTSS_APPL_TAS_UNHANDLED_QUEUES,
    VTSS_APPL_TAS_ADMIN_CTRL_LIST_TOO_SHORT,
    VTSS_APPL_TAS_ADMIN_CTRL_LIST_TOO_LONG,
    VTSS_APPL_TAS_INVALID_ADMIN_BASE_TIME_SEC_MSB,
    VTSS_APPL_TAS_ADMIN_BASE_TIME_SEC_NANO_TOO_BIG,
    VTSS_APPL_TAS_ADMIN_CYCLE_TIME_NUMERATOR_ZERO,
    VTSS_APPL_TAS_ADMIN_CYCLE_TIME_DENOMINATOR_ZERO,
    VTSS_APPL_TAS_ADMIN_CYCLE_TIME_DENOMINATOR_NOT_VALID,
    VTSS_APPL_TAS_CYCLE_TIME_TOO_SMALL,
    VTSS_APPL_TAS_CYCLE_TIME_TOO_BIG,
    VTSS_APPL_TAS_CYCLE_TIME_EXTENSION_TOO_BIG,
    VTSS_APPL_TAS_CYCLE_TIME_EXTENSION_TOO_SMALL,
    VTSS_APPL_TAS_INVALID_GATE_OPERATION,
    VTSS_APPL_TAS_MAX_SDU_OUT_OF_RANGE,
    VTSS_APPL_TAS_INVALID_QUEUE,
    VTSS_APPL_TAS_INVALID_GCE,
    VTSS_APPL_TAS_GCE_CONFIGURATION,
    VTSS_APPL_TAS_TAS_CONFIGURATION,
    VTSS_APPL_TAS_TAS_CONFIGURATION_BASETIME_NOT_FUTURE,
    VTSS_APPL_TSN_INVALID_CLOCK_DOMAIN,
};

/**
 * CAPABILITIES on this particular platform.
 */
typedef struct {
    mesa_bool_t has_queue_frame_preemption; /*!< Platform supports frame preemption feature. */
    mesa_bool_t has_tas;            /*!< Platform supports time aware shaper */
    mesa_bool_t has_psfp;           /*!< Platform supports time aware shaper */
    mesa_bool_t has_frer;           /*!< Platform supports time aware shaper */
    uint8_t     min_add_frag_size;  /*!< The value of the 802.3br LocAddFragSize
                                     * parameter for the port on this platform.
                                     * The minimum size of non-final fragments supported by the
                                     * receiver on the local port. This value is expressed in units
                                     * of 64 octets of additional fragment length.
                                     * The minimum non-final fragment size is:
                                     * (LocAddFragSize + 1) * 64 octets.
                                     */
    mesa_bool_t tas_mac_restrict;   /*!< Hold and Release MAC operations are restricted.
                                     * All opened Hold queues must be express. All opened release queues must be preemptable. */
    uint32_t   tas_max_gce_cnt;     /*!< The maximum number of Gate Control Elements in a TAS ControlList. */
    uint32_t   tas_max_sdu_min;     /*!< Minimum value of TAS maxSDU. */
    uint32_t   tas_max_sdu_max;     /*!< Maximum value of TAS maxSDU. */
    uint32_t   tas_ct_min;          /*!< Minimum TAS Cycle time in nano seconds supported. */
    uint32_t   tas_ct_max;          /*!< Maximum TAS Cycle time in nano seconds supported. */
} vtss_appl_tsn_capabilities_t;

/**
 * Get the capabilities for this platform.
 *
 * \param cap [OUT] This module's capabilities.
 * \return Error code.
 */
mesa_rc vtss_appl_tsn_capabilities_get(vtss_appl_tsn_capabilities_t *cap);


/**
 * The procedure to use when starting PTP depending modules (TAS and PSFP)
 */
typedef enum {
    VTSS_APPL_TSN_PROCEDURE_NONE,           /* Start immediately. */
    VTSS_APPL_TSN_PROCEDURE_TIME_ONLY,      /* Wait seconds speficied by timeout parameter and then start. */
    VTSS_APPL_TSN_PROCEDURE_TIME_AND_PTP    /* Start sensing PTP state, and start id state is LOCKED or LOCKING.
                                             * If this takes more than time specified by timeout parameter,
                                             * then start even if good PTP state is not reached.
                                             */
} appl_tsn_time_start_procedure_t;

/**
 * Global configuration
 */
typedef struct {
    appl_tsn_time_start_procedure_t procedure;      /*!< Selects which procedure shall be used to reach a state where TAS and PSFP can issue config_change.*/
    uint32_t                        timeout;        /*!< Maximum number of seconds to wait before TAS and PSFP can issue config_change.                    */
    uint32_t                        ptp_port;       /*!< The PTP port to use when sensing wether PTP time is locked.                                       */
    uint8_t                         clock_domain;   /*!< The clock domain used for TSN                                                                     */
} vtss_appl_tsn_global_conf_t;

/**
 * Get default global configuration.
 *
 * \param conf [OUT] Pointer to structure receiving default global configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_tsn_global_conf_default_get(vtss_appl_tsn_global_conf_t *conf);

/**
 * Get the global configuration.
 *
 * \param conf [OUT] Pointer to structure receiving the global configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_tsn_global_conf_get(vtss_appl_tsn_global_conf_t *conf);

/**
 * Change the global configuration.
 *
 * \param conf [IN] Pointer to structure with new global configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_tsn_global_conf_set(const vtss_appl_tsn_global_conf_t *conf);

/**
 * The state of the procedure when starting PTP depending modules (TAS and PSFP)
 */
typedef enum {
    VTSS_APPL_TSN_INITIAL,
    VTSS_APPL_TSN_START_IMMEDIATELY,
    VTSS_APPL_TSN_WAITING_FOR_TIMEOUT,
    VTSS_APPL_TSN_TIMED_OUT,
    VTSS_APPL_TSN_PTP_WAITING_FOR_LOCK,
    VTSS_APPL_TSN_PTP_LOCKING,
    VTSS_APPL_TSN_PTP_LOCKED,
    VTSS_APPL_TSN_PTP_TIMED_OUT
} appl_tsn_start_state_t;

/**
 * Global state
 */
typedef struct {
    appl_tsn_start_state_t  start_state;    /*!< The state of the process of getting a synchronised ptp time      */
    bool                    start;          /*!< True, it modules like TAS and PSFP can issue config_change       */
    uint32_t                time_passed;    /*!< The number of seconds it took to reach the current start_state   */
} vtss_appl_tsn_global_state_t;

/**
 * Get the global state.
 *
 * \param state [OUT] Pointer to structure receiving the global state.
 * \return Error code.
 */
mesa_rc vtss_appl_tsn_global_state_get(vtss_appl_tsn_global_state_t *state);


/****************************************************************************
 * QoS 802.3br (Frame Preemption) Configuration and Status
 ****************************************************************************/

/*! \brief 802.3br (Frame Preemption) port configuration */
typedef struct {
    mesa_bool_t admin_status[MESA_QUEUE_ARRAY_SIZE];    /*!< IEEE802.1Qbu: framePreemptionStatusTable */
    mesa_bool_t enable_tx;                              /*!< The value of the 802.3br aMACMergeEnableTx parameter for the port.
                                                         * This value determines whether frame preemption is enabled (true) or
                                                         * disabled (false) in the MAC Merge sublayer in the transmit direction.
                                                         */
    mesa_bool_t ignore_lldp_tx;                         /*!< If this value is enabled (true) the enable_tx parameter will determine if preemption is enabled (false)
                                                         *  without considering the state of potentially received lldp information. Default value false.
                                                         */
    mesa_bool_t verify_disable_tx;                      /*!< The value of the 802.3br aMACMergeVerifyDisableTx parameter for the port.
                                                         * This value determines whether the verifiy function is disabled (true) or
                                                         * enabled (false) in the MAC Merge sublayer in the transmit direction.
                                                         */
    uint8_t     verify_time;                            /*!< IEEE802.3br: aMACMergeVerifyTime [msec]
                                                         */
    uint8_t     add_frag_size;                          /*!< IEEE802.3br: aMACMergeAddFragSize
                                                         */
} vtss_appl_tsn_fp_cfg_t;

/*!
 * \brief Get configuration.
 *
 * Only implemented if capabilities.has_queue_frame_preemption is TRUE
 *
 * \param ifindex   [IN]  The interface index for which to get the configuration.
 * \param cfg       [OUT] Configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_fp_cfg_get(
    vtss_ifindex_t ifindex,
    vtss_appl_tsn_fp_cfg_t *const cfg
);

/*!
 * \brief Set configuration.
 *
 * Only implemented if capabilities.has_queue_frame_preemption is TRUE
 *
 * \param ifindex   [IN]  The interface index for which to set the configuration.
 * \param cfg       [IN]  Configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_fp_cfg_set(
    vtss_ifindex_t ifindex,
    const vtss_appl_tsn_fp_cfg_t *const cfg
);

/*!
 * \brief Iterator for retrieving TSN Frame Preemption configuration key/index
 *
 * \param prev_ifindex [IN]  Interface index to be used for indexing determination.
 *
 * \param next_ifindex [OUT] The key/index of Interface index should be used for the GET operation.
 *                           When IN is NULL, assign the first index.
 *                           When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_fp_cfg_itr(
    const vtss_ifindex_t *const prev_ifindex,
    vtss_ifindex_t       *const next_ifindex
);

/*!
 * \brief Get default configuration.
 *
 * Only implemented if capabilities.has_queue_frame_preemption is TRUE
 *
 * \param cfg       [OUT] Configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_fp_cfg_get_default(
    vtss_appl_tsn_fp_cfg_t *const cfg
);

/*! \brief MAC Merge Status Verify (aMACMergeStatusVerify in 802.3br) */
typedef enum {
    VTSS_APPL_TSN_MM_STATUS_VERIFY_INITIAL,   // INIT_VERIFICATION
    VTSS_APPL_TSN_MM_STATUS_VERIFY_IDLE,      // VERIFICATION_IDLE
    VTSS_APPL_TSN_MM_STATUS_VERIFY_SEND,      // SEND_VERIFY
    VTSS_APPL_TSN_MM_STATUS_VERIFY_WAIT,      // WAIT_FOR_RESPONSE.
    VTSS_APPL_TSN_MM_STATUS_VERIFY_SUCCEEDED, // VERIFIED
    VTSS_APPL_TSN_MM_STATUS_VERIFY_FAILED,    // VERIFY_FAIL
    VTSS_APPL_TSN_MM_STATUS_VERIFY_DISABLED,  // Verification process is disabled
    VTSS_APPL_TSN_MM_STATUS_VERIFY_UNKNOWN    // Unknown
} vtss_appl_tsn_mm_status_verify_t;

/*! \brief 802.1Qbu and 802.3br (Frame Preemption) port status */
typedef struct {
    uint32_t                         hold_advance;          /*!< The value of the 802.1Qbu holdAdvance
                                                             * parameter for the port in nanoseconds.
                                                             * There is no default value; the holdAdvance is
                                                             * a property of the underlying MAC.
                                                             */
    uint32_t                         release_advance;       /*!< The value of the 802.1Qbu releaseAdvance
                                                             * parameter for the port in nanoseconds.
                                                             * There is no default value; the releaseAdvance is
                                                             * a property of the underlying MAC.
                                                             */
    mesa_bool_t                      preemption_active;     /*!< The value of the 802.1Qbu preemptionActive.
                                                             * The value is active (TRUE) when preemption is operationally
                                                             * active for the port, and idle (FALSE) otherwise.
                                                             */
    mesa_bool_t                      hold_request;          /*!< The value of the 802.1Qbu holdRequest
                                                             * parameter for the port.
                                                             * The value is hold (TRUE) when the sequence of gate operations
                                                             * for the port has executed a Set-And-Hold-MAC operation,
                                                             * and release (FALSE) when the sequence of gate operations has
                                                             * executed a Set-And-Release-MAC operation. The
                                                             * value of this object is release (FALSE) on system
                                                             * initialization.
                                                             */
    vtss_appl_tsn_mm_status_verify_t status_verify;         /*!< Status of MAC Merge sublayer verification.
                                                             * This parameter corresponds to the aMACMergeStatusVerify
                                                             * attribute in 802.3br.
                                                             */
    mesa_bool_t                      loc_preempt_supported; /*!< The value of the 802.3br LocPreemptSupported
                                                             * parameter for the port.
                                                             * The value is TRUE when preemption is supported
                                                             * on the port, and FALSE otherwise.
                                                             */
    mesa_bool_t                      loc_preempt_enabled;   /*!< The value of the 802.3br LocPreemptEnabled
                                                             * parameter for the port.
                                                             * The value is TRUE when preemption is enabled
                                                             * on the port, and FALSE otherwise.
                                                             */
    mesa_bool_t                      loc_preempt_active;    /*!< The value of the 802.3br LocPreemptActive
                                                             * parameter for the port.
                                                             * The value is TRUE when preemption is operationally
                                                             * active on the port, and FALSE otherwise.
                                                             */
    uint8_t                          loc_add_frag_size;     /*!< The value of the 802.3br LocAddFragSize
                                                             * parameter for the port.
                                                             * The minimum size of non-final fragments supported by the
                                                             * receiver on the local port. This value is expressed in units
                                                             * of 64 octets of additional fragment length.
                                                             * The minimum non-final fragment size is:
                                                             * (LocAddFragSize + 1) * 64 octets.
                                                             */
} vtss_appl_tsn_fp_status_t;

/*!
 * \brief Get status.
 *
 * Only implemented if capabilities.has_queue_frame_preemption is TRUE
 *
 * \param ifindex   [IN]  The interface index for which to get the status.
 * \param status    [OUT] Status.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_fp_status_get(
    vtss_ifindex_t ifindex,
    vtss_appl_tsn_fp_status_t *const status
);

/*!
 * \brief Iterator for retrieving TSN frame preemption status's key/index
 *
 * \param prev_ifindex   [IN]  Interface index to be used for indexing determination.
 *
 * \param next_ifindex   [OUT] The key/index of Interface index should be used for the GET operation.
 *                             When IN is NULL, assign the first index.
 *                             When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_fp_status_itr(
    const vtss_ifindex_t *const prev_ifindex,
    vtss_ifindex_t       *const next_ifindex
);

/****************************************************************************
 * TSN TAS Configuration and Status
 ****************************************************************************/

/*! \brief QoS Qbv (TAS) maximum SDU size conf. */
typedef struct {
    uint16_t max_sdu; /*!< Maximum SDU size. */
} vtss_appl_tsn_tas_max_sdu_t;

/** The default value of max_sdu */
#define MAX_SDU_DEFAULT 24 * 64

/*!
 * \brief Get per port per queue maximum SDU size.
 *
 * \param ifindex [IN] The interface index for which to get the configuration.
 * \param queue   [IN] The queue for which to get the configuration.
 * \param max_sdu [OUT] Max SDU size value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_per_q_max_sdu_get(
    vtss_ifindex_t ifindex,
    mesa_prio_t    queue,
    vtss_appl_tsn_tas_max_sdu_t *const max_sdu
);

/*!
 * \brief Set per port per queue maximum SDU size.
 *
 * Only implemented if capabilities.has_qbv is TRUE
 *
 * \param ifindex [IN] The interface index for which to set the configuration.
 * \param queue   [IN] The queue for which to set the configuration.
 * \param max_sdu [IN] Max SDU size value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_per_q_max_sdu_set(
    vtss_ifindex_t ifindex,
    mesa_prio_t    queue,
    const vtss_appl_tsn_tas_max_sdu_t *const max_sdu
);

/*!
 * \brief Iterator for retrieving TSN TAS's maximum SDU size key/index
 *
 * \param prev_ifindex [IN]    Interface index to be used for indexing determination.
 * \param prev_queue   [IN]    Queue to be used for indexing determination.
 *
 * \param next_ifindex [OUT]   The key/index of Interface index should be used for the GET operation.
 * \param next_queue   [OUT]   The key/index of Queue should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_per_q_max_sdu_itr(
    const vtss_ifindex_t *const prev_ifindex,   vtss_ifindex_t  *const  next_ifindex,
    const mesa_prio_t    *const prev_queue,     mesa_prio_t     *const  next_queue
);

/*! \brief Gate Control List index. */
typedef uint32_t vtss_gcl_index_t;

/*! \brief Struct to hold an entry/TLV of the Gate Control List. */
typedef struct {
    mesa_qos_tas_gco_t gate_operation; /*!< Stream Gate Control Operation. */
    uint8_t            gate_state;     /*!< Octet represent the gate states for the
                                        * corresponding traffic classes;
                                        * The MS bit corresponds to traffic class 7.
                                        * The LS bit to traffic class 0.
                                        * A bit value of 0 indicates closed;
                                        * A bit value of 1 indicates open. */
    uint32_t           time_interval;  /*!< A TimeInterval is encoded in 4 octets as a 32-bit
                                        * unsigned integer, representing a number of nanoseconds.
                                        * The first octet encodes the most significant 8 bits of the
                                        * integer, and the fourth octet encodes the least
                                        * significant 8 bits. */
} vtss_appl_tsn_tas_gcl_t;

/*!
 * \brief Get Operational configuration of a single GCL entry.
 * *
 * \param ifindex   [IN]  The interface index for which to get the configuration.
 * \param gcl_index [IN]  The GCL index for which to get the configuration.
 * \param cfg       [OUT] Configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_gcl_oper_get(
    vtss_ifindex_t   ifindex,
    vtss_gcl_index_t gcl_index,
    vtss_appl_tsn_tas_gcl_t *const cfg
);

/*!
 * \brief Get Admin configuration of a single GCL entry.
 * *
 * \param ifindex   [IN]  The interface index for which to get the configuration.
 * \param gcl_index [IN]  The GCL index for which to get the configuration.
 * \param cfg       [OUT] Configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_gcl_admin_get(
    vtss_ifindex_t   ifindex,
    vtss_gcl_index_t gcl_index,
    vtss_appl_tsn_tas_gcl_t *const cfg
);

/*!
 * \brief Set Admin configuration of a single GCL entry.
 *
 * \param ifindex   [IN]  The interface index for which to set the configuration.
 * \param gcl_index [IN]  The GCL index for which to set the configuration.
 * \param cfg       [IN]  Configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_gcl_admin_set(
    vtss_ifindex_t   ifindex,
    vtss_gcl_index_t gcl_index,
    const vtss_appl_tsn_tas_gcl_t *const cfg
);

/*!
 * \brief Iterator for retrieving TSN TAS's Gate Control admin entry key/index
 *
 * \param prev_ifindex   [IN]  Interface index to be used for indexing determination.
 * \param prev_gcl_index [IN]  GCL entry index to be used for indexing determination.
 *
 * \param next_ifindex   [OUT] The key/index of Interface index should be used for the GET operation.
 * \param next_gcl_index [OUT] The key/index of GCL entry should be used for the GET operation.
 *                             When IN is NULL, assign the first index.
 *                             When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_gcl_admin_itr(
    const vtss_ifindex_t    *const prev_ifindex,    vtss_ifindex_t   *const next_ifindex,
    const vtss_gcl_index_t  *const prev_gcl_index,  vtss_gcl_index_t *const next_gcl_index
);

/*!
 * \brief TAS's Gate Control entry default configuration.
 *
 * \param cfg       [OUT] Default Gate Control entry
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_gcl_admin_get_default(
    vtss_appl_tsn_tas_gcl_t *const cfg
);

/*!
 * \brief Iterator for retrieving TSN TAS's Gate Control oper entry key/index
 *
 * \param prev_ifindex   [IN]  Interface index to be used for indexing determination.
 * \param prev_gcl_index [IN]  GCL entry index to be used for indexing determination.
 *
 * \param next_ifindex   [OUT] The key/index of Interface index should be used for the GET operation.
 * \param next_gcl_index [OUT] The key/index of GCL entry should be used for the GET operation.
 *                             When IN is NULL, assign the first index.
 *                             When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_gcl_oper_itr(
    const vtss_ifindex_t    *const prev_ifindex,    vtss_ifindex_t   *const next_ifindex,
    const vtss_gcl_index_t  *const prev_gcl_index,  vtss_gcl_index_t *const next_gcl_index
);

/*! \brief miscellaneous TAS configuration,
 * as defined in IEEE8021-ST-MIB standard MIB */
typedef struct {
    mesa_bool_t      gate_enabled;                /*!< The GateEnabled parameter determines
                                                   * whether traffic scheduling
                                                   * is active (true) or inactive (false).
                                                   * The value of this object MUST be retained across
                                                   * reinitializations of the management system.
                                                   */
    uint8_t          admin_gate_states;           /*!< The administrative value of the GateStates parameter
                                                   * for the Port. The bits of the octet represent the gate
                                                   * states for the corresponding traffic classes; the MS bit
                                                   * corresponds to traffic class 7, the LS bit to traffic
                                                   * class 0. A bit value of 0 indicates closed; a bit value
                                                   * of 1 indicates open. The value of this object MUST be
                                                   * retained across reinitializations of the management system.
                                                   */
    uint32_t         admin_control_list_length;   /*!< The administrative value of the ListMax parameter for the
                                                   * port. The integer value indicates the number of entries (TLVs)
                                                   * in the AdminControlList. The value of this object MUST be
                                                   * retained across reinitializations of the management system.
                                                   */
    uint32_t         admin_cycle_time_numerator;  /*!< The administrative value of the numerator of the CycleTime
                                                   * parameter for the Port. The numerator and denominator together
                                                   * represent the cycle time as a rational number of seconds.
                                                   * The value of this object MUST be retained across
                                                   * reinitializations of the management system.
                                                   */
    uint32_t         admin_cycle_time_denominator; /*!< The administrative value of the denominator of the
                                                    * CycleTime parameter for the Port. The numerator and denominator
                                                    * together represent the cycle time as a rational number of seconds.
                                                    * The value of this object MUST be retained across
                                                    * reinitializations of the management system.
                                                    */
    uint32_t         admin_cycle_time_extension;   /*!< The administrative value of the CycleTimeExtension
                                                    * parameter for the Port. The value is an unsigned integer number
                                                    * of nanoseconds. The value of this object MUST be retained across
                                                    * reinitializations of the management system.
                                                    */
    mesa_timestamp_t admin_base_time;              /*!< The administrative value of the BaseTime parameter for the Port.
                                                    * The value is a representation of a PTPtime value, consisting of a
                                                    * 48-bit integer number of seconds and a 32-bit integer number of
                                                    * nanoseconds. The value of this object MUST be retained across
                                                    * reinitializations of the management system.
                                                    */
    mesa_bool_t      config_change;                /*!< The ConfigChange parameter signals the start of a configuration
                                                    * change when it is set to TRUE. This should only be done when the
                                                    * various administrative parameters are all set to appropriate
                                                    * values.
                                                    */
} vtss_appl_tsn_tas_cfg_t;

/*!
 * \brief Get configuration.
 *
 * Only implemented if capabilities.has_qbv is TRUE
 *
 * \param ifindex   [IN]  The interface index for which to get the configuration.
 * \param cfg       [OUT] Configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_cfg_get(
    vtss_ifindex_t ifindex,
    vtss_appl_tsn_tas_cfg_t *const cfg
);

/*!
 * \brief Set configuration.
 *
 * Only implemented if capabilities.has_qbv is TRUE
 *
 * \param ifindex   [IN]  The interface index for which to set the configuration.
 * \param cfg       [IN]  Configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_cfg_set(
    vtss_ifindex_t ifindex,
    const vtss_appl_tsn_tas_cfg_t *const cfg
);

/*!
 * \brief Iterator for retrieving TSN TAS's miscellaneous confingurations' key/index
 *
 * \param prev_ifindex [IN]  Interface index to be used for indexing determination.
 *
 * \param next_ifindex [OUT] The key/index of Interface index should be used for the GET operation.
 *                        When IN is NULL, assign the first index.
 *                        When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_cfg_itr(
    const vtss_ifindex_t *const prev_ifindex,
    vtss_ifindex_t       *const next_ifindex
);

/*!
 * \brief Get default configuration.
 *
 * \param cfg       [OUT] Default Configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_cfg_get_default(
    vtss_appl_tsn_tas_cfg_t *const cfg
);

/*! \brief Global TAS configuration. */
typedef struct {
    mesa_bool_t always_guard_band;      /*!< When set a guard band is implemented even for scheduled queues
                                         * to scheduled queue transition.
                                         * false: Guard band is implemented for non-scheduled queues to scheduled
                                         * queues transition.
                                         * true: Guard band is implemented for any queue to scheduled
                                         * queues transition.
                                         */
} vtss_appl_tsn_tas_cfg_global_t;

/*!
 * \brief Get TAS global configuration.
 *
 * \param cfg       [OUT] Global Configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_cfg_get_global(
    vtss_appl_tsn_tas_cfg_global_t *const cfg
);

/*!
 * \brief Set TAS global configuration.
 *
 * \param cfg       [IN] Global Configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_cfg_set_global(
    const vtss_appl_tsn_tas_cfg_global_t *const cfg
);

/*! \brief Operational status parameters */
typedef struct {
    uint8_t          oper_gate_states;            /*!< The operational value of the GateStates parameter for
                                                   * the Port. The bits of the octet represent the gate states
                                                   * for the corresponding traffic classes;the MS bit corresponds
                                                   * to traffic class 7, the LS bit to traffic class 0. A bit
                                                   * value of 0 indicates closed; a bit value of 1 indicates open.
                                                   */
    uint32_t         oper_control_list_length;    /*!< The operational value of the ListMax parameter for the
                                                   * Port. The integer value indicates the number of entries
                                                   * (TLVs) in the OperControlList.
                                                   */
    uint32_t         oper_cycle_time_numerator;   /*!< The operational value of the numerator of the
                                                   * CycleTime parameter for the Port. The numerator
                                                   * and denominator together represent the cycle
                                                   * time as a rational number of seconds.
                                                   */
    uint32_t         oper_cycle_time_denominator; /*!< The operational value of the denominator of the
                                                   * CycleTime parameter for the Port. The numerator and
                                                   * denominator together represent the cycle time as a rational
                                                   * number of seconds.
                                                   */
    uint32_t         oper_cycle_time_extension;   /*!< The operational value of the CycleTimeExtension parameter
                                                   * for the Port. The value is an unsigned integer number of
                                                   * nanoseconds.
                                                   */
    mesa_timestamp_t oper_base_time;              /*!< The operationsl value of the BaseTime parameter for the Port.
                                                   * The value is a representation of a PTPtime value,
                                                   * consisting of a 48-bit integer number of seconds and a 32-bit
                                                   * integer number of nanoseconds.
                                                   */
    mesa_timestamp_t config_change_time;          /*!< The PTPtime at which the next config change is scheduled to occur.
                                                   * The value is a representation of a PTPtime value,
                                                   * consisting of a 48-bit integer
                                                   * number of seconds and a 32-bit integer number of nanoseconds.
                                                   * The value of this object MUST be retained across
                                                   * reinitializations of the management system.
                                                   */
    uint32_t         tick_granularity;            /*!< The granularity of the cycle time clock, represented as an
                                                   * unsigned number of tenths of nanoseconds.
                                                   * The value of this object MUST be retained across
                                                   * reinitializations of the management system.
                                                   */
    mesa_timestamp_t current_time;                /*!< The current time, in PTPtime, as maintained by the local system.
                                                   * The value is a representation of a PTPtime value,
                                                   * consisting of a 48-bit integer
                                                   * number of seconds and a 32-bit integer number of nanoseconds.
                                                   */
    mesa_bool_t      config_pending;              /*!< The value of the ConfigPending state machine variable.
                                                   * The value is TRUE if a configuration change is in progress
                                                   * but has not yet completed.
                                                   */
    uint64_t         config_change_error;         /*!< A counter of the number of times that a re-configuration
                                                   * of the traffic schedule has been requested with the old
                                                   * schedule still running and the requested base time was
                                                   * in the past.
                                                   */
    uint32_t         supported_list_max;          /*!< The maximum value supported by this Port of the
                                                   * AdminControlListLength and OperControlListLength
                                                   * parameters.
                                                   */
} vtss_appl_tsn_tas_oper_state_t;

/*!
 * \brief Get status.
 * *
 * \param ifindex   [IN]  The interface index for which to get the configuration.
 * \param status    [OUT] Status.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_status_get(
    vtss_ifindex_t ifindex,
    vtss_appl_tsn_tas_oper_state_t *const status
);

/*!
 * \brief Iterator for retrieving TSN TAS status's key/index
 *
 * \param prev_ifindex   [IN]  Interface index to be used for indexing determination.
 *
 * \param next_ifindex   [OUT] The key/index of Interface index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_tsn_tas_status_itr(
    const vtss_ifindex_t *const prev_ifindex,
    vtss_ifindex_t       *const next_ifindex
);

#endif /* _VTSS_APPL_TSN_HXX_ */

