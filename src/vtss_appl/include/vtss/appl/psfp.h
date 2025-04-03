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
 * \file psfp.h
 *
 * \brief This API provides typedefs and functions for the PSFP (Per-Stream
 * Filtering and Policing) module
 */

#ifndef _VTSS_APPL_PSFP_H_
#define _VTSS_APPL_PSFP_H_

#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/stream.h>

/**
 * PSFP error codes (mesa_rc)
 */
enum {
    VTSS_APPL_PSFP_RC_INVALID_PARAMETER = MODULE_ERROR_START(VTSS_MODULE_ID_PSFP),      /**< Invalid argument (typically a NULL pointer) passed to a function */
    VTSS_APPL_PSFP_RC_NOT_SUPPORTED,                                                    /**< PSFP is not supported on this platform                           */
    VTSS_APPL_PSFP_RC_OUT_OF_MEMORY,                                                    /**< Out of memory                                                    */
    VTSS_APPL_PSFP_RC_INTERNAL_ERROR,                                                   /**< Internal error: Check console or crashlog for details            */
    VTSS_APPL_PSFP_RC_INVALID_TIME_UNIT,                                                /**< Invalid time units                                               */
    VTSS_APPL_PSFP_RC_STREAM_ID_OUT_OF_RANGE,                                           /**< Stream ID out of range                                           */
    VTSS_APPL_PSFP_RC_STREAM_COLLECTION_ID_OUT_OF_RANGE,                                /**< Stream collection ID out of range                                */
    VTSS_APPL_PSFP_RC_FLOW_METER_ID_OUT_OF_RANGE,                                       /**< Flow meter ID out of range                                       */
    VTSS_APPL_PSFP_RC_FLOW_METER_NO_SUCH_INSTANCE,                                      /**< No such flow meter instance                                      */
    VTSS_APPL_PSFP_RC_FLOW_METER_OUT_OF_HW_RESOURCES,                                   /**< Out of hardware resources                                        */
    VTSS_APPL_PSFP_RC_GATE_ID_OUT_OF_RANGE,                                             /**< Stream gate ID out of range                                      */
    VTSS_APPL_PSFP_RC_GATE_NO_SUCH_INSTANCE,                                            /**< No such stream gate ID                                           */
    VTSS_APPL_PSFP_RC_GATE_INVALID_GATE_STATE,                                          /**< Invalid administrative gate state                                */
    VTSS_APPL_PSFP_RC_GATE_INVALID_IPV_VALUE,                                           /**< Invalid priority value. Valid range is 0-7                       */
    VTSS_APPL_PSFP_RC_GATE_INVALID_CYCLE_TIME,                                          /**< Invalid cycle time                                               */
    VTSS_APPL_PSFP_RC_GATE_INVALID_TIME_EXTENSION,                                      /**< Invalid extension time                                           */
    VTSS_APPL_PSFP_RC_GATE_INVALID_GCL_LEN,                                             /**< Invalid Gate Control List length                                 */
    VTSS_APPL_PSFP_RC_GATE_INVALID_GCE_GATE_STATE,                                      /**< Invalid Gate Control Entry gate state                            */
    VTSS_APPL_PSFP_RC_GATE_INVALID_GCE_IPV_VALUE,                                       /**< Invalid Gate Control Entry IPV value                             */
    VTSS_APPL_PSFP_RC_GATE_INVALID_GCE_TIME_INTERVAL,                                   /**< Invalid Gate Control Entry time interval                         */
    VTSS_APPL_PSFP_RC_GATE_CYCLETIME_EXCEEDED,                                          /**< Cycle-time is exceeded                                           */
    VTSS_APPL_PSFP_RC_FILTER_ID_OUT_OF_RANGE,                                           /**< Stream filter ID out of range                                    */
    VTSS_APPL_PSFP_RC_FILTER_NO_SUCH_INSTANCE,                                          /**< No such filter instance                                          */
    VTSS_APPL_PSFP_RC_FILTER_STREAM_ID_AND_COLLECTION_ID_CANNOT_BE_USED_SIMULTANEOUSLY, /**< Cannot use stream and stream collections at the same time        */
    VTSS_APPL_PSFP_RC_FILTER_ANOTHER_FILTER_USING_SAME_STREAM_ID,                       /**< Another stream filter is using the same stream ID                */
    VTSS_APPL_PSFP_RC_FILTER_ANOTHER_FILTER_USING_SAME_STREAM_COLLECTION_ID,            /**< Another stream filter is using the same stream collection ID     */
};

/**
 * PSFP capabilities
 */
typedef struct  {
    /**
     * Maximum number of PSFP stream filter instances, numbered [0; Max[.
     */
    uint32_t max_filter_instances;

    /**
     * Maximum number of PSFP stream gate instances, numbered [0; Max[.
     */
    uint32_t max_gate_instances;

    /**
     * Maximum number of flow meter instances, numbers [0; Max[.
     */
    uint32_t max_flow_meter_instances;

    /**
     * Maximum length of Gate Control List
     */
    uint32_t gate_control_list_length_max;

    /**
     * Maximum value of a stream ID.
     */
    uint32_t stream_id_max;

    /**
     * Maximum value of a stream collection ID.
     */
    uint32_t stream_collection_id_max;

    /**
     * Boolean indicating whether PSFP is supported on this platform or not.
     */
    mesa_bool_t psfp_supported;
} vtss_appl_psfp_capabilities_t;

/**
 * Get capabilities of the PSFP module.
 *
 * \param cap [IN] Pointer to structure receiving capabilities.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_capabilities_get(vtss_appl_psfp_capabilities_t *cap);

/**
 * Function to figure out whether PSFP is supported or not.
 *
 * \return True if PSFP is supported on this platform, false otherwise.
 */
mesa_bool_t vtss_appl_psfp_supported(void);

//------------------------------------------------------------------------------
// PSFP Flow Meter Functions
//------------------------------------------------------------------------------

/**
 * Flow Meter ID.
 */
typedef uint32_t vtss_appl_psfp_flow_meter_id_t;

/**
 * May be used to indicate the beginning of a flow meter iteration.
 */
#define VTSS_APPL_PSFP_FLOW_METER_ID_NONE 0xffffffff

/**
 * Flow meter color mode
 */
typedef enum {
    VTSS_APPL_PSFP_FLOW_METER_CM_BLIND, /**< Flow meter is color blind */
    VTSS_APPL_PSFP_FLOW_METER_CM_AWARE  /**< Flow meter is color aware */
} vtss_appl_psfp_flow_meter_cm_t;

/**
 * Flow meter configuration structure
 */
struct vtss_appl_psfp_flow_meter_conf_t {
    /**
     * Committed Information Rate measured in kbps.
     *
     * The value will be adjusted to the closest value supported by hardware.
     */
    uint32_t cir;

    /**
     * Committed Burst Size measured in bytes.
     *
     * The value will be adjusted to the closest value supported by hardware.
     */
    uint32_t cbs;

    /**
     * Excess Information Rate measured in kbps.
     *
     * The value will be adjusted to the closest value supported by hardware.
     */
    uint32_t eir;

    /**
     * Excess Burst Size measured in bytes.
     *
     * The value will be adjusted to the closest value supported by hardware.
     */
    uint32_t ebs;

    /**
     * Coupling Flag.
     *
     * When true, frames that would overflow the committed bucket will be added
     * to the excess bucket unless it's full. Mostly relevant when the Color
     * Mode is Color Aware.
     */
    mesa_bool_t cf;

    /**
     * Color mode.
     *
     * If set to Color Blind, each frame starts green and is then marked
     * according to the policer's operation.
     *
     * If set to Color Aware, the frame starts at the classified color (green or
     * yellow), and is then marked according to the policer's operation.
     */
    vtss_appl_psfp_flow_meter_cm_t cm;

    /**
     * If true, frames marked yellow are discarded. If unchecked, frames will
     * have their DEI value set to 1.
     */
    mesa_bool_t drop_on_yellow;

    /**
     * If true, discard all subsequent frames if red frame is seen.
     */
    mesa_bool_t mark_all_frames_red_enable;
};

/**
 * Get a default PSFP Flow Meter configuration.
 *
 * \param conf [IN] Pointer to structure receiving the default flow meter configurat.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_flow_meter_conf_default_get(vtss_appl_psfp_flow_meter_conf_t *conf);

/**
 * Get a PSFP Flow Meter configuration for a given Flow Meter ID.
 *
 * \param flow_meter_id [IN]  Flow meter ID to get flow meter configuration for.
 * \param conf          [OUT] Pointer to structure receiving flow meter configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_flow_meter_conf_get(vtss_appl_psfp_flow_meter_id_t flow_meter_id, vtss_appl_psfp_flow_meter_conf_t *conf);

/**
 * Set a PSFP flow meter configuration for a given flow meter ID.
 *
 * \param flow_meter_id [IN] Flow meter ID to set flow meter configuration for.
 * \param conf          [IN] New configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_flow_meter_conf_set(vtss_appl_psfp_flow_meter_id_t flow_meter_id, const vtss_appl_psfp_flow_meter_conf_t *conf);

/**
 * Delete a PSFP Flow Meter configuration
 *
 * \param flow_meter_id [IN] Flow meter ID of configuration to remove.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_flow_meter_conf_del(vtss_appl_psfp_flow_meter_id_t flow_meter_id);

/**
 * Iterate across all created flow meter IDs.
 *
 * \param prev [IN]  Pointer to the previous flow meter ID. Set to nullptr or VTSS_APPL_PSFP_FLOW_METER_ID_NONE to start.
 * \param next [OUT] If function succeeds, this one will hold the ID of the next Flow Meter instance.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_flow_meter_itr(const vtss_appl_psfp_flow_meter_id_t *prev, vtss_appl_psfp_flow_meter_id_t *next);

/**
 * Flow Meter status
 */
typedef struct  {
    /**
     * If true, all frames are being marked red, and therefore discarded.
     */
    mesa_bool_t mark_all_frames_red;
} vtss_appl_psfp_flow_meter_status_t;

/**
 * Get flow meter status for a given Flow Meter ID.
 *
 * \param flow_meter_id [IN]  The Flow Meter ID to get status for.
 * \param status        [OUT] Pointer to structure receiving the flow meter status.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_flow_meter_status_get(vtss_appl_psfp_flow_meter_id_t flow_meter_id, vtss_appl_psfp_flow_meter_status_t *status);

/**
 * PSFP flow meter control
 */
typedef struct {
    /**
     * If a flow has started marking all frames red, it may be cleared by
     * setting this to true.
     */
    mesa_bool_t clear_mark_all_frames_red;
} vtss_appl_psfp_flow_meter_control_t;

/**
 * Control a Flow Meter instance.
 *
 * \param flow_meter_id [IN] The Flow Meter ID to control
 * \param ctrl          [IN] Pointer to structure containing what to control.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_psfp_flow_meter_control_set(vtss_appl_psfp_flow_meter_id_t flow_meter_id, const vtss_appl_psfp_flow_meter_control_t *ctrl);

//------------------------------------------------------------------------------
// PSFP Stream Gate Functions
//------------------------------------------------------------------------------

/**
 * Stream gate ID.
 */
typedef uint32_t vtss_appl_psfp_gate_id_t;

/**
 * May be used to indicate the beginning of a stream gate iteration.
 */
#define VTSS_APPL_PSFP_GATE_ID_NONE 0xffffffff

/**
 * Is the gate closed or open?
 */
typedef enum {
    VTSS_APPL_PSFP_GATE_STATE_CLOSED, /**< Gate is closed */
    VTSS_APPL_PSFP_GATE_STATE_OPEN    /**< Gate is open   */
} vtss_appl_psfp_gate_state_t;

/**
 * The maximum number of entries in a stream gate list. This is an upper bound,
 * and may not be supported on all platforms. The actual value can be found in
 * vtss_appl_psfp_capabilities::gate_control_list_length_max.
 */
#define VTSS_APPL_PSFP_GATE_LIST_LENGTH_MAX 4

/**
 * Stream Gate Gate Control List configuration entries (GCEs).
 */
typedef struct {
    /**
     * The GCE's gate state. Default is closed.
     */
    vtss_appl_psfp_gate_state_t gate_state;

    /**
     * The gate entry's value of the IPV parameter for the gate.
     * Valid values are in the range [-1; 7].
     * Set to -1 to disable mapping to a particular egress queue.
     */
    int32_t ipv;

    /**
     * The time in nanoseconds that this entry is active.
     *
     * Valid values are 1 to 999,999,999 nanoseconds, but the sum of all
     * active entries cannot exceed the cycle time (cycle_time_ns).
     */
    uint32_t time_interval_ns;

    /**
     * If a frame is larger than this value, it will be discarded by the GCE.
     *
     * Set to 0 to disable this check and let any frame size pass through.
     */
    uint32_t interval_octet_max;
} vtss_appl_psfp_gate_gce_conf_t;

/**
 * Stream Gate configuration.
 */
typedef struct {
    /**
     * Initial gate state.
     */
    vtss_appl_psfp_gate_state_t gate_state;

    /**
     * The initial internal priority value upon frame arrival. May be changed
     * later by a control list entry.
     * Valid values are in the range [-1; 7].
     * Set to -1 to disable mapping to a particular egress queue.
     */
    int32_t ipv;

    /**
     * Cycle time of the stream gate measured in nanoseconds.
     *
     * Valid values are [0; 1,000,000,000] nanoseconds.
     * 0 disables the gate control list.
     */
    uint32_t cycle_time_ns;

    /**
     * If true, a stream gate gets permanently closed if receiving a frame
     * during a closed gate state.
     */
    mesa_bool_t close_gate_due_to_invalid_rx_enable;

    /**
     * If true, a stream gate gets permanently closed if receiving a frame that
     * exceeds the configured Octet Max.
     */
    mesa_bool_t close_gate_due_to_octets_exceeded_enable;

    /**
     * The administrative value of the CycleTimeExtension parameter for the
     * gate, measured in nanoseconds.
     *
     * Valid values are [0; 1,000,000,000] nanoseconds.
     */
    uint32_t cycle_time_extension_ns;

    /**
     * The administrative value of the BaseTime parameter for the gate.
     *
     * Only base_time.seconds and base_time.nanoseconds are used.
     * The remaining fields are implicitly set to 0.
     */
    mesa_timestamp_t base_time;

    /**
     * Number of active items in the gcl_list.
     *
     * VTSS_APPL_PSFP_GATE_LIST_LENGTH_MAX is an upper bound and should
     * not be used when iterating across the individual gate control entries.
     *
     * Instead use vtss_appl_psfp_capabilities::gate_control_list_length_max.
     *
     * The gcl_length must therefore be a value between 0 and
     * gate_control_list_length_max.
     */
    uint32_t gcl_length;

    /**
     * Gate Control List (GCL) entries (GCEs).
     * See \p gcl_length for how to find platform-dependent upper bound of the
     * list.
     */
    vtss_appl_psfp_gate_gce_conf_t gcl[VTSS_APPL_PSFP_GATE_LIST_LENGTH_MAX];

    /**
     * Set to true to enable the stream gate.
     */
    mesa_bool_t gate_enabled;

    /**
     * The parameter signals the start of a configuration change for the gate
     * when set to true. This should only be done when the various
     * administrative parameters are all set to appropriate values.
     */
    mesa_bool_t config_change;
} vtss_appl_psfp_gate_conf_t;

/**
 * Get a default stream gate configuration
 *
 * \param conf [OUT] Pointer to structure receiving a default stream gate configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_gate_conf_default_get(vtss_appl_psfp_gate_conf_t *conf);

/**
 * Get a Stream Gate configuration for a given Stream Gate ID.
 *
 * \param gate_id [IN]  Stream gate ID to get stream gate configuration for.
 * \param conf    [OUT] Pointer to structure receiving stream gate configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_gate_conf_get(vtss_appl_psfp_gate_id_t gate_id, vtss_appl_psfp_gate_conf_t *conf);

/**
 * Set a PSFP stream gate configuration for a given stream gate ID.
 *
 * \param gate_id [IN] Stream gate ID to set stream gate configuration for.
 * \param conf    [IN] New configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_gate_conf_set(vtss_appl_psfp_gate_id_t gate_id, const vtss_appl_psfp_gate_conf_t *conf);

/**
 * Delete a PSFP Stream Gate configuration
 *
 * \param gate_id [IN] Stream gate ID of configuration to remove.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_gate_conf_del(vtss_appl_psfp_gate_id_t gate_id);

/**
 * Iterate across all created stream gate IDs.
 *
 * \param prev [IN]  Pointer to the previous stream gate ID. Set to nullptr or VTSS_APPL_PSFP_GATE_ID_NONE to start.
 * \param next [OUT] If function succeeds, this one will hold the ID of the next Stream Gate instance.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_gate_itr(const vtss_appl_psfp_gate_id_t *prev, vtss_appl_psfp_gate_id_t *next);

/**
 * Stream gate status
 */
typedef struct {
    /**
     * The operational configuration (\p oper_conf) can only be trusted when
     * it's been applied to hardware the very first time.
     */
    mesa_bool_t oper_conf_valid;

    /**
     * Operational configuration. This is the configuration that currently is
     * in effect. It is only valid if \p oper_conf_valid is true.
     */
    vtss_appl_psfp_gate_conf_t oper_conf;

    /**
     * The value of the ConfigPending state machine variable.
     */
    mesa_bool_t config_pending;

    /**
     * Pending configuration. This is the configuration that will take effect
     * at the configured base_time when the config_change parameter was
     * set. Only valid if \p config_pending is true.
     */
    vtss_appl_psfp_gate_conf_t pend_conf;

    /**
     * The operational gate state.
     */
    vtss_appl_psfp_gate_state_t oper_gate_state;

    /**
     * The operational IPV.
     * Valid values are in the range [-1; 7].
     * -1 indicates that use of an IPV is disabled.
     */
    int32_t oper_ipv;

    /**
     * The PTPtime at which the next config change is scheduled to occur or has
     * occurred.
     */
    mesa_timestamp_t config_change_time;

    /**
     * The current time, in PTPtime, as maintained by the local system.
     */
    mesa_timestamp_t current_time;

    /**
     * The granularity of the cycle time clock, represented as an unsigned
     * number of tenths of nanoseconds.
     */
    uint32_t tick_granularity;

    /**
     * A counter of the number of times that a re-configuration of the traffic
     * schedule has been requested with the old schedule still running and the
     * requested base time was in the past.
     */
    uint32_t config_change_errors;

    /**
     * The PSFPGateClosedDueToInvalidRx object contains a boolean value that
     * indicates whether, if the PSFPGateClosedDueToInvalidRx function is
     * enabled, all frames are to be discarded (true) or not (false).
     */
    mesa_bool_t gate_closed_due_to_invalid_rx;

    /**
     * The PSFPGateClosedDueToOctectsExceeded parameter contains a boolean value
     * that indicates whether, if the PSFPGateClosedDueToOctetsExceeded function
     * is enabled, all frames are to be discarded (true) or not (false).
     */
    mesa_bool_t gate_closed_due_to_octets_exceeded;
} vtss_appl_psfp_gate_status_t;

/**
 * Get stream gate status for a given stream gate ID.
 *
 * \param gate_id [IN]  The stream gate ID to get status for.
 * \param status  [OUT] Pointer to structure receiving the stream gate status.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_gate_status_get(vtss_appl_psfp_gate_id_t gate_id, vtss_appl_psfp_gate_status_t *status);

/**
 * PSFP stream gate control structure
 */
typedef struct {
    /**
     * If a stream gate is closed due to invalid Rx, it may be set open again by
     * setting this to true.
     */
    mesa_bool_t clear_gate_closed_due_to_invalid_rx;

    /**
     * If a stream gate is closed due to octets exceeded, it may be set open
     * again by setting this to true.
     */
    mesa_bool_t clear_gate_closed_due_to_octets_exceeded;
} vtss_appl_psfp_gate_control_t;

/**
 * Control a Stream Gate instance.
 *
 * \param gate_id [IN] The Stream Gate ID to control
 * \param ctrl    [IN] Pointer to structure containing what to control.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_psfp_gate_control_set(vtss_appl_psfp_gate_id_t gate_id, const vtss_appl_psfp_gate_control_t *ctrl);

//------------------------------------------------------------------------------
// PSFP Stream Filte Functions
//------------------------------------------------------------------------------

/**
 * Stream filter ID.
 */
typedef uint32_t vtss_appl_psfp_filter_id_t;

/**
 * May be used to indicate the beginning of a stream filter iteration.
 */
#define VTSS_APPL_PSFP_FILTER_ID_NONE 0xffffffff

/**
 * Configuration structure of a stream filter.
 */
typedef struct {
    /**
     * ID of the stream that this PSFP filter works on.
     * If not yet assigned, it contains the value VTSS_APPL_STREAM_ID_NONE.
     * Unlike the standard, a value of VTSS_APPL_STREAM_ID_NONE causes the
     * stream filter NOT to match anything. The standard states that it matches
     * all streams. That's not possible with our chip architectures.
     *
     * Either this one of \p stream_collection_id can be specified, not both.
     */
    vtss_appl_stream_id_t stream_id;

    /**
     * ID of the stream collection that this PSFP filter works on.
     * If not yet assigned, it contains the value
     * VTSS_APPL_STREAM_COLLECTION_ID_NONE.
     * Unlike the standard, a value of VTSS_APPL_STREAM_COLLECTION_ID_NONE
     * causes the stream filter NOT to match anything. The standard states that
     * it matches all streams. That's not possible with our chip architectures.
     *
     * Either this one of \p stream_id can be specified, not both.
     */
    vtss_appl_stream_collection_id_t stream_collection_id;

    /**
     * ID of a flow meter.
     * If not yet assigned, it contains the value
     * VTSS_APPL_PSFP_FLOW_METER_ID_NONE.
     *
     * A filter does not need to have a flow meter attached.
     */
    vtss_appl_psfp_flow_meter_id_t flow_meter_id;

    /**
     * ID of a stream gate.
     * If not yet assigned, it contains the value VTSS_APPL_PSFP_GATE_ID_NONE.
     */
    vtss_appl_psfp_gate_id_t gate_id;

    /**
     * This maximum SDU size, measured in octets.
     *
     * If the SDU size of a frame exceeds the value of this one, the frame is
     * discarded.
     *
     * A value of 0 disables this check.
     *
     * See also \p block_due_to_oversize_frame_enable.
     */
    uint32_t max_sdu_size;

    /**
     * Whenever a frame gets discarded because its SDU size is greater than
     * \p max_sdu_size, this one controls what shall happen with subsequent
     * frames that go through the stream filter.
     * If set to true, subsequent frames will also be discarded whether they are
     * larger or smaller than the configured \p max_sdu_size. Otherwise they
     * will remain being subject to only the max SDU size check.
     *
     * An administrative action is required to reset the discarding. See
     * vtss_appl_psfp_filter_control_set().
     */
    mesa_bool_t block_due_to_oversize_frame_enable;
} vtss_appl_psfp_filter_conf_t;

/**
 * Get a default stream filter configuration
 *
 * \param conf [OUT] Pointer to structure receiving a default stream filter configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_filter_conf_default_get(vtss_appl_psfp_filter_conf_t *conf);

/**
 * Get a Stream Filter configuration for a given Stream Filter ID.
 *
 * \param filter_id [IN]  Stream filter ID to get stream filter configuration for.
 * \param conf      [OUT] Pointer to structure receiving stream filter configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_filter_conf_get(vtss_appl_psfp_filter_id_t filter_id, vtss_appl_psfp_filter_conf_t *conf);

/**
 * Set a PSFP stream filter configuration for a given stream filter ID.
 *
 * \param filter_id [IN] Stream filter ID to set stream filter configuration for.
 * \param conf      [IN] New configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_filter_conf_set(vtss_appl_psfp_filter_id_t filter_id, const vtss_appl_psfp_filter_conf_t *conf);

/**
 * Delete a PSFP Stream Filter configuration
 *
 * \param filter_id [IN] Stream filter ID of configuration to remove.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_filter_conf_del(vtss_appl_psfp_filter_id_t filter_id);

/**
 * Iterate across all created stream filter IDs.
 *
 * \param prev [IN]  Pointer to the previous stream filter ID. Set to nullptr or VTSS_APPL_PSFP_FILTER_ID_NONE to start.
 * \param next [OUT] If function succeeds, this one will hold the ID of the next Stream Filter instance.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_filter_itr(const vtss_appl_psfp_filter_id_t *prev, vtss_appl_psfp_filter_id_t *next);

/**
 * Bitmask of configurational warnings of a Stream Filter instance.
 */
typedef enum {
    VTSS_APPL_PSFP_FILTER_OPER_WARNING_NONE                                       = 0x000, /**< No warnings                                          */
    VTSS_APPL_PSFP_FILTER_OPER_WARNING_NO_STREAM_OR_STREAM_COLLECTION             = 0x001, /**< Neither a stream or a stream collection is specified */
    VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_NOT_FOUND                           = 0x002, /**< The specified stream ID does not exist               */
    VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_NOT_FOUND                = 0x004, /**< The specified stream collection ID does not exist    */
    VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_ATTACH_FAIL                         = 0x008, /**< Unable to attach to the specified stream             */
    VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_ATTACH_FAIL              = 0x010, /**< Unable to attach to the specified stream collection  */
    VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_HAS_OPERATIONAL_WARNINGS            = 0x020, /**< Stream has operational warnings                      */
    VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_HAS_OPERATIONAL_WARNINGS = 0x040, /**< Stream collection has operational warnings           */
    VTSS_APPL_PSFP_FILTER_OPER_WARNING_FLOW_METER_NOT_FOUND                       = 0x080, /**< The specified flow meter ID does not exist           */
    VTSS_APPL_PSFP_FILTER_OPER_WARNING_GATE_NOT_FOUND                             = 0x100, /**< The specified stream gate ID does not exist          */
    VTSS_APPL_PSFP_FILTER_OPER_WARNING_GATE_NOT_ENABLED                           = 0x200, /**< The specified stream gate is not enabled             */
} vtss_appl_psfp_filter_oper_warnings_t;

/**
 * Operators for vtss_appl_psfp_filter_oper_warnings_t flags.
 */
VTSS_ENUM_BITWISE(vtss_appl_psfp_filter_oper_warnings_t);

/**
 * Status of a stream filter instance.
 */
typedef struct {
    /**
     * Configurational warnings of this Stream Filter Instance.
     */
    vtss_appl_psfp_filter_oper_warnings_t oper_warnings;

    /**
     * This is true is an oversized frame (exceeding max_sdu_size) has been
     * received and block_due_to_oversize_frame_enable is true, false otherwise.
     *
     * The stream can be re-opened with a call to
     * vtss_appl_psfp_filter_control_set() with
     * clear_blocked_due_to_oversize_frame set to true.
     */
    mesa_bool_t stream_blocked_due_to_oversize_frame;
} vtss_appl_psfp_filter_status_t;

/**
 * Get stream filter status for a given stream filter ID.
 *
 * \param filter_id [IN]  The stream filter ID to get status for.
 * \param status    [OUT] Pointer to structure receiving the stream filter status.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_filter_status_get(vtss_appl_psfp_filter_id_t filter_id, vtss_appl_psfp_filter_status_t *status);

/**
 * PSFP stream filter control structure
 */
typedef struct {
    /**
     * If a stream is closed due to reception of oversized frame (see
     * stream_blocked_due_to_oversize_frame), setting this to true will clear
     * the blocking.
     */
    mesa_bool_t clear_blocked_due_to_oversize_frame;
} vtss_appl_psfp_filter_control_t;

/**
 * Control a Stream filter instance.
 *
 * \param filter_id [IN] The stream filter ID to control
 * \param ctrl      [IN] Pointer to structure containing what to control.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_psfp_filter_control_set(vtss_appl_psfp_filter_id_t filter_id, const vtss_appl_psfp_filter_control_t *ctrl);

/**
 * PSFP stream filter statistics
 */
typedef struct {
    /**
     * Counts received frames that match this stream filter.
     */
    uint64_t matching;

    /**
     * Counts received frames that pass the gate associated with this stream
     * filter.
     */
    uint64_t passing;

    /**
     * Counts received frames that do not pass the gate associated with this
     * stream filter.
     */
    uint64_t not_passing;

    /**
     * Counts received frames that pass the SDU size filter specification
     * associated with this stream filter.
     */
    uint64_t passing_sdu;

    /**
     * Counts received frames that do not pass the SDU size filter specification
     * associated with this stream filter.
     */
    uint64_t not_passing_sdu;

    /**
     * Counts received frames that were discarded as a result of the flow meter
     * associated with this stream filter.
     */
    uint64_t red;
} vtss_appl_psfp_filter_statistics_t;

/**
 * Get statistics of a PSFP stream filter.
 *
 * \param filter_id  [IN]  The stream filter ID to get statistics for.
 * \param statistics [OUT] Pointer to structure receiving the stream filter statistics.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_filter_statistics_get(vtss_appl_psfp_filter_id_t filter_id, vtss_appl_psfp_filter_statistics_t *statistics);

/**
 * Clear statistics of a PSFP stream filter.
 *
 * \param filter_id  [IN]  The stream filter ID to clear statistics for.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psfp_filter_statistics_clear(vtss_appl_psfp_filter_id_t filter_id);
#endif

