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
 * \brief Public Frame Replication and Elimination for Reliability API
 * \details This header file describes FRER control/status functions and types.
 * Unless otherwise stated, references - mostly stated in parentheses - are to
 * clauses in IEEE802.1CB-2017.
 */

#ifndef _VTSS_APPL_FRER_HXX_
#define _VTSS_APPL_FRER_HXX_

#include <vtss/appl/types.hxx>
#include <vtss/appl/stream.h>
#include <vtss/appl/interface.h>       /* For vtss_ifindex_t      */
#include <vtss/basics/enum_macros.hxx> /* For VTSS_ENUM_BITWISE() */

/**
 * FRER error codes (mesa_rc)
 */
enum {
    VTSS_APPL_FRER_RC_INVALID_PARAMETER = MODULE_ERROR_START(VTSS_MODULE_ID_FRER), /**< Invalid argument (typically a NULL pointer) passed to a function          */
    VTSS_APPL_FRER_RC_NOT_SUPPORTED,                                               /**< FRER is not supported on this platform                                    */
    VTSS_APPL_FRER_RC_INTERNAL_ERROR,                                              /**< Internal error. Requires code update                                      */
    VTSS_APPL_FRER_RC_NO_SUCH_INSTANCE,                                            /**< FRER instance is not created                                              */
    VTSS_APPL_FRER_RC_HW_RESOURCES,                                                /**< Out of H/W resources                                                      */
    VTSS_APPL_FRER_RC_OUT_OF_MEMORY,                                               /**< Out of memory                                                             */
    VTSS_APPL_FRER_RC_INVALID_MODE,                                                /**< Invalid mode                                                              */
    VTSS_APPL_FRER_RC_INVALID_STREAM_ID_LIST,                                      /**< At least one of the Stream IDs is invalid                                 */
    VTSS_APPL_FRER_RC_STREAM_ID_AND_COLLECTION_ID_CANNOT_BE_USED_SIMULTANEOUSLY,   /**< Cannot use stream-id-list and stream-collection at the same time          */
    VTSS_APPL_FRER_RC_INVALID_STREAM_COLLECTION_ID,                                /**< Invalid stream collection ID                                              */
    VTSS_APPL_FRER_RC_INVALID_VLAN,                                                /**< Invalid FRER VLAN                                                         */
    VTSS_APPL_FRER_RC_INVALID_STREAM_ID,                                           /**< Invalid Stream ID                                                         */
    VTSS_APPL_FRER_RC_STREAM_CNT_ZERO_OR_NO_STREAM_COLLECTION,                     /**< At least one stream or stream collection must be specified                */
    VTSS_APPL_FRER_RC_STREAM_CNT_MUST_BE_ONE,                                      /**< In generation mode, the number of streams must be exactly one.            */
    VTSS_APPL_FRER_RC_STREAM_CNT_EXCEEDED,                                         /**< The maximum number of streams is exeeded                                  */
    VTSS_APPL_FRER_RC_INDIVIDUAL_RECOVERY_WITH_STREAM_COLLECTIONS_NOT_POSSIBLE,    /**< Individual recovery with stream collections not possible                  */
    VTSS_APPL_FRER_RC_EGRESS_PORT_CNT_EXCEEDED,                                    /**< The maximum number of egress ports is exceeded                            */
    VTSS_APPL_FRER_RC_EGRESS_PORT_CNT_ZERO,                                        /**< At least one egress port must be specified                                */
    VTSS_APPL_FRER_RC_INVALID_ALGORITHM,                                           /**< Invalid recovery algorithm                                                */
    VTSS_APPL_FRER_RC_INVALID_HISTORY_LEN,                                         /**< Invalid history length                                                    */
    VTSS_APPL_FRER_RC_INVALID_RESET_TIMEOUT,                                       /**< Invalid recovery reset timeout                                            */
    VTSS_APPL_FRER_RC_INVALID_LATENT_ERROR_DIFF,                                   /**< Invalid recovery latent error difference                                  */
    VTSS_APPL_FRER_RC_INVALID_LATENT_ERROR_PERIOD,                                 /**< Invalid recovery latent error period                                      */
    VTSS_APPL_FRER_RC_INVALID_LATENT_ERROR_PATHS,                                  /**< Invalid recovery latent error paths                                       */
    VTSS_APPL_FRER_RC_INVALID_LATENT_RESET_PERIOD,                                 /**< Invalid recovery latent reset period                                      */
    VTSS_APPL_FRER_RC_LIMIT_REACHED,                                               /**< The maximum number of FRER instances is reached                           */
    VTSS_APPL_FRER_RC_ANOTHER_FRER_INSTANCE_USING_SAME_STREAM_ID,                  /**< Another active FRER instance is using at least one of the same stream IDs */
    VTSS_APPL_FRER_RC_ANOTHER_FRER_INSTANCE_USING_SAME_STREAM_COLLECTION_ID,       /**< Another active FRER instance is using the same stream collection          */
    VTSS_APPL_FRER_RC_IFINDEX_MUST_BE_NONE_IN_GENERATION_MODE,                     /**< Ifindex must be none in generation mode                                   */
    VTSS_APPL_FRER_RC_STREAM_ID_MUST_BE_NONE_IN_GENERATION_MODE,                   /**< Stream ID must be none in generation mode                                 */
    VTSS_APPL_FRER_RC_IFINDEX_MUST_NOT_BE_NONE_IN_RECOVERY_MODE,                   /**< Ifindex must not be none in recovery mode                                 */
    VTSS_APPL_FRER_RC_INVALID_IFINDEX,                                             /**< Ifindex is not of type port                                               */
    VTSS_APPL_FRER_RC_NOT_PART_OF_EGRESS_PORTS,                                    /**< Specified interface is not part of configured egress interfaces           */
    VTSS_APPL_FRER_RC_STREAM_ID_CANNOT_BE_SPECIFIED_IN_THIS_MODE,                  /**< A stream ID cannot be specified in this mode                              */
    VTSS_APPL_FRER_RC_STREAM_ID_NOT_FOUND,                                         /**< Stream ID not found                                                       */
};

/**
 * Capabilities on this particular platform.
 */
typedef struct {
    /**
     * Maximum number of creatable FRER instances.
     * This value is 0 if FRER is not supported on this platform.
     */
    uint32_t inst_cnt_max;

    /**
     * Maximum number of creatable member streams.
     */
    uint32_t rcvy_mstream_cnt_max;

    /**
     * Maximum number of creatable compound streams.
     */
    uint32_t rcvy_cstream_cnt_max;

    /**
     * Minimum value to configure for reset timer (in milliseconds).
     */
    uint32_t rcvy_reset_timeout_ms_min;

    /**
     * Maximum value to configure for reset timer (in milliseconds).
     */
    uint32_t rcvy_reset_timeout_ms_max;

    /**
     * Minimum history length (when using vector recovery algorithm).
     */
    uint32_t rcvy_history_len_min;

    /**
     * Maximum history length (when using vector recovery algorithm).
     */
    uint32_t rcvy_history_len_max;

    /**
     * The minimum allowed sequence number distance used in latent error
     * detection.
     */
    int32_t rcvy_latent_error_difference_min;

    /**
     * The maximum allowed sequence number distance used in latent error
     * detection.
     */
    int32_t rcvy_latent_error_difference_max;

    /**
     * Minimum configurable latent error period in milliseconds.
     */
    uint32_t rcvy_latent_error_period_ms_min;

    /**
     * Maximum configurable latent error period in milliseconds.
     */
    uint32_t rcvy_latent_error_period_ms_max;

    /**
     * The minimum allowed number of paths used in latent error detection.
     */
    int32_t rcvy_latent_error_paths_min;

    /**
     * The maximum allowed number of paths used in latent error detection.
     */
    int32_t rcvy_latent_error_paths_max;

    /**
     * Minimum configurable latent error reset period in milliseconds.
     */
    uint32_t rcvy_latent_reset_period_ms_min;

    /**
     * Maximum configurable latent error reset period in milliseconds.
     */
    uint32_t rcvy_latent_reset_period_ms_max;

    /**
     * Maximum number of egress ports per FRER instance.
     */
    uint32_t egress_port_cnt_max;

    /**
     * Maximum value of a stream ID.
     */
    uint32_t stream_id_max;

    /**
     * This is the maximum value of a stream collection ID (copy from stream
     * module).
     */
    uint32_t stream_collection_id_max;
} vtss_appl_frer_capabilities_t;

/**
 * Get FRER capabilities.
 *
 * \param cap [OUT] FRER capabilities
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_frer_capabilities_get(vtss_appl_frer_capabilities_t *cap);

/**
 * Indicates whether FRER is supported on this platform.
 * This is a shorthand for vtss_appl_frer_capabilities_t::inst_cnt_max > 0.
 *
 * \return true if FRER is supported, false if not.
 */
mesa_bool_t vtss_appl_frer_supported(void);

/**
 * FRER modes
 */
typedef enum {
    VTSS_APPL_FRER_MODE_GENERATION, /**< FRER instance is in generation mode */
    VTSS_APPL_FRER_MODE_RECOVERY,   /**< FRER instance is in recovery mode   */
} vtss_appl_frer_mode_t;

/**
 * The maximum number of supported egress ports limited by H/W when doing
 * recovery.
 */
#define VTSS_APPL_FRER_EGRESS_PORT_CNT_MAX 8

/**
 * In recovery mode, it is possible to enable latent error detection.
 * This structure defines the operation of that function.
 */
typedef struct {
    /**
     * Controls latent error detection enabledness.
     * The latent error detection function monitors the recovery function in
     * order to detect the condition that relatively few packets are being
     * discarded by that function.
     * When the latent error detection function observes a violation, it raises
     * a notification (SIGNAL_LATENT_ERROR, see
     * vtss_appl_frer_notification_status_t).
     *
     * Only used when \p mode is VTSS_APPL_FRER_MODE_RECOVERY.
     *
     * Corresponds to frerSeqRcvyLatentErrorDetection in 802.1CB (10.4.1.11).
     * See 802.1CB, clause 7.4.4.
     *
     * Corresponds to sequence-recovery-entry.latent-error-detection in  YANG
     * model
     */
    mesa_bool_t enable;

    /**
     * Specifies the maximum distance between the number of discarded packets
     * and the number of member streams (paths) multiplied by the number of
     * passed packets. Any larger difference will trigger the detection of a
     * latent error by the latent error test function.
     *
     * Valid range is
     * [vtss_appl_frer_capabilities_t.:rcvy_latent_error_difference_min;
     * vtss_appl_frer_capabilities_t::rcvy_latent_error_difference_max].
     *
     * Only used when \p mode is VTSS_APPL_FRER_MODE_RECOVERY and
     * \p enable is true.
     *
     * Corresponds to frerSeqRcvyLatentErrorDifference in 802.1CB (10.4.1.12.1).
     *
     * Even though the standard calls this a signed integer, we define it as an
     * unsigned integer, because it is used as an absolute difference, so it
     * doesn't make sense to specify it as a negative number.
     *
     * Corresponds to latent-error-detection-parameters.difference in YANG
     * model.
     */
    uint32_t difference;

    /**
     * The number of milliseconds between running the latent error test
     * function.
     *
     * Valid range is
     * [vtss_appl_frer_capabilities_t::rcvy_latent_error_period_ms_min;
     * vtss_appl_frer_capabilities_t::rcvy_latent_error_period_ms_max].
     *
     * Default is 2000 ms.
     *
     * Only used when \p mode is VTSS_APPL_FRER_MODE_RECOVERY and
     * \p enable is true.
     *
     * Corresponds to frerSeqRcvyLatentErrorPeriod in 802.1CB (10.4.1.12.2).
     *
     * Corresponds to latent-error-detection-parameters.period in YANG
     * model.
     */
    uint32_t period_ms;

    /**
     * Specifies the number of paths that the latent error detection function
     * operates on.
     *
     * If this FRER instance receives R-tagged frames from N member streams,
     * then this value should be set to N.
     *
     * Valid range is
     * [vtss_appl_frer_capabilities_t.:rcvy_latent_error_paths_min;
     * vtss_appl_frer_capabilities_t::rcvy_latent_error_paths_max].
     *
     * Only used when \p mode is VTSS_APPL_FRER_MODE_RECOVERY and
     * \p enable is true.
     *
     * Corresponds to frerSeqRcvyLatentErrorPaths in 802.1CB (10.4.1.12.3).
     *
     * Corresponds to latent-error-detection-parameters.paths in YANG model.
     */
    uint32_t paths;

    /**
     * The number of milliseconds between running the latent error reset
     * function.
     *
     * Valid range is
     * [vtss_appl_frer_capabilities_t::rcvy_latent_reset_period_ms_min;
     * vtss_appl_frer_capabilities_t::rcvy_latent_reset_period_ms_max].
     *
     * Default is 30000 ms.
     *
     * Only used when \p mode is VTSS_APPL_FRER_MODE_RECOVERY and
     * \p enable is true.
     *
     * Corresponds to frerSeqRcvyLatentResetPeriod in 802.1CB (10.4.1.12.4).
     *
     * Corresponds to latent-error-detection-parameters.reset-period in YANG
     * model.
     */
    uint32_t reset_period_ms;
} vtss_appl_frer_latent_error_detection_t;

/**
 * Configuration of one FRER instance
 */
typedef struct {
    /**
     *  Mode that this instance is running in.
     */
    vtss_appl_frer_mode_t mode;

    /**
     * The VLAN ID that the streams identified by \p stream_ids[] or \p
     * stream_collection_id get classified to.
     */
    mesa_vid_t frer_vlan;

    /**
     * Stream identity list, coming from the stream (VCL) module.
     *
     * A given FRER instance can match up to
     * vtss_appl_frer_capabilities_t::egress_port_cnt_max different streams,
     * each instantiated on any given set of ingress ports.
     *
     * This number is derived from the fact that H/W can do compound recovery on
     * this many egress ports at a time, and if individual recovery is enabled,
     * we must also be able to recognize this many member streams.
     *
     * The use of the stream list is primarily useful when doing individual
     * recovery.
     *
     * In generation mode, the number of valid streams in \p stream_ids[] must
     * be exactly one! If more are needed, use \p stream_collection_id.
     *
     * In recovery mode more than one stream ID may be specified, but this is
     * normally only interesting when using individual recovery in order to
     * separate the individual streams.
     *
     * In non-individual recovery, it's normally good enough to specify one
     * stream, which is instantiated on multiple ingress ports.
     *
     * If utilizing individual recovery, you may want to specify the same stream
     * matching rules in separate stream IDs and let one ingress port match its
     * own stream ID.
     *
     * A value of VTSS_APPL_STREAM_ID_NONE in an entry indicates that this entry
     * is not used.
     *
     * Another property of this one is that the order of the IDs can be any when
     * setting, whereas when retrieving the configuration, it is guaranteed that
     * they are in numerical order with duplicates removed and possible
     * VTSS_APPL_STREAM_ID_NONE entries come last.
     *
     * Corresponds to sequence-recovery-entry.stream-list in YANG model in
     * recovery mode.
     *
     * VTSS_APPL_FRER_EGRESS_PORT_CNT_MAX is an upper limit. Use
     * vtss_appl_frer_capabilities_t::egress_port_cnt_max to find the actual
     * number.
     */
    vtss_appl_stream_id_t stream_ids[VTSS_APPL_FRER_EGRESS_PORT_CNT_MAX];

    /**
     * An alternative to using \p stream_ids is to use a stream collection. As
     * the name implies, a stream collection is a compilation of multiple
     * streams.
     *
     * Stream collections are particularly useful in generation mode, where it
     * can be used to specify multiple ingress streams that all must be mapped
     * to the same FRER instance and use the same sequence number generator.
     * This is not possible with \p stream_ids, where only one stream can be
     * used in generation mode.
     *
     * Stream collections can also be used in non-individual recovery mode, but
     * it is not possible in individual recovery mode. In individual recovery
     * mode, separate streams must be specified.
     */
    vtss_appl_stream_collection_id_t stream_collection_id;

    /**
     * List of egress ports.
     * At most vtss_appl_frer_capabilities_t::egress_port_cnt_max ports can be
     * specified in this list.
     *
     * In generation mode, this tells which egress ports will have an R-Tag
     * pushed to the frames. The egress ports must all be members of the FRER
     * VLAN or they won't get out.
     *
     * If a port is member of the FRER VLAN but isn't included in this list of
     * egress ports, the frame - if transmitted on that port - will not include
     * an R-Tag.
     *
     * Corresponds to sequence-recovery-entry.port-list in YANG model in
     * recovery mode, but has no corresponding entry in the YANG model in
     * generation mode.
     */
    mesa_port_list_t egress_ports;

    /**
     * Controls whether to pop an outer VLAN tag from frames matching the
     * stream or stream collection in the generator end.
     *
     * If the frames don't contain a VLAN tag and \p outer_tag_pop is set to
     * true, nothing is popped.
     *
     * Not used if the instance is in recovery mode.
     */
    mesa_bool_t outer_tag_pop;

    /**
     * The chosen recovery algorithm.
     * Only used when \p mode is VTSS_APPL_FRER_MODE_RECOVERY.
     *
     * Default is MESA_FRER_RECOVERY_ALG_VECTOR.
     *
     * Corresponds to frerSeqRcvyAlgorithm in 802.1CB.
     *
     * Corresponds to sequence-recovery-entry.algorithm in YANG model.
     */
    mesa_frer_recovery_alg_t rcvy_algorithm;

    /**
     * The vector recovery algorithm's history length.
     *
     * Only used when \p mode is VTSS_APPL_FRER_MODE_RECOVERY and
     * \p rcvy_algorithm is MESA_FRER_RECOVERY_ALG_VECTOR.
     *
     * Valid range is [vtss_appl_frer_capabilities_t::rcvy_history_len_min;
     * vtss_appl_frer_capabilities_t::rcvy_history_len_max].
     *
     * Default is 2.
     *
     * Corresponds to sequence-recovery-entry.history-length in YANG model.
     */
    uint32_t rcvy_history_len;

    /**
     * The reset time in milliseconds of the recovery algorithm.
     *
     * Only used when \p mode is VTSS_APPL_FRER_MODE_RECOVERY.
     * Valid values are [vtss_appl_frer_capabilities_t::rcvy_reset_timeout_ms_min;
     * vtss_appl_frer_capabilities_t::rcvy_reset_timeout_ms_max]
     *
     * See 802.1CB, clause 10.4.1.7.
     *
     * Corresponds to sequence-recovery-entry.reset-timeout in YANG model.
     */
    uint32_t rcvy_reset_timeout_ms;

    /**
     * Indicates whether the vector recovery algorithm is allowed to pass frames
     * that don't include an R-Tag.
     *
     * Only used when \p mode is VTSS_APPL_FRER_MODE_RECOVERY. Even though the
     * standard specifies that this is only used when \p rcvy_algorithm is
     * MESA_FRER_RECOVERY_ALG_VECTOR, we also support it when \p rcvy_algorithm
     * is MESA_FRER_RECOVERY_ALG_MATCH.
     *
     * Corresponds to sequence-recovery-entry.take-no-sequence in YANG model.
     */
    mesa_bool_t rcvy_take_no_sequence;

    /**
     * Indicates whether individual recovery is enabled on this FRER instance.
     * When individual recovery is enabled, each member flow will run the
     * selected recovery algorithm before passing it to the compound recovery,
     * allowing for detecting a sender that sends the same R-Tag sequence number
     * over and over again (see e.g. 802.1CB, Annex C.10).
     *
     * Moreover, when individual recovery is enabled, individual counters for
     * each individual member flow are made available.
     *
     * Only used when \p mode is VTSS_APPL_FRER_MODE_RECOVERY.
     *
     * Corresponds to sequence-recovery-entry.individual-recovery in YANG
     * model.
     */
    mesa_bool_t rcvy_individual;

    /**
     * Indicates whether we are an end system that terminates recovery and
     * removes the R-tag before the frames are egressing.
     *
     * Only used when \p mode is VTSS_APPL_FRER_MODE_RECOVERY.
     *
     * This doesn't have a corresponding entry in the YANG model, so I don't
     * know how to model this through YANG.
     */
    mesa_bool_t rcvy_terminate;

    /**
     * The members of this structure control the operation of the latent error
     * detection function.
     * It is used only when \p mode is VTSS_APPL_FRER_MODE_RECOVERY.
     */
    vtss_appl_frer_latent_error_detection_t rcvy_latent_error_detection;

    /**
     * Controls whether this instance is active or not.
     *
     * Default is false.
     */
    mesa_bool_t admin_active;
} vtss_appl_frer_conf_t;

/**
 * Get a default FRER configuration.
 *
 * \param *conf [OUT] Default FRER configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_frer_conf_default_get(vtss_appl_frer_conf_t *conf);

/**
 * Get FRER instance configuration.
 *
 * \param instance [IN]  FRER instance
 * \param conf     [OUT] Current configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_frer_conf_get(uint32_t instance, vtss_appl_frer_conf_t *conf);

/**
 * Set FRER instance configuration.
 *
 * \param instance [IN] FRER instance
 * \param conf     [IN] FRER new configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_frer_conf_set(uint32_t instance, const vtss_appl_frer_conf_t *conf);

/**
 * Delete a FRER instance.
 *
 * \param instance [IN] Instance to be deleted
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_frer_conf_del(uint32_t instance);

/**
 * Instance iterator.
 *
 * This function returns the next defined FRER instance number. The end is
 * reached when VTSS_RC_ERROR is returned.
 *
 * The iterator can be used both for configuration and status.
 *
 * The search for an enabled instance will start with 'prev_instance' + 1.
 * If 'prev_instance' pointer is NULL, the search start with the lowest possible
 * instance number.
 *
 * \param prev_instance [IN]  Instance number
 * \param next_instance [OUT] Next instance
 *
 * \return VTSS_RC_OK as long as next is valid.
 */
mesa_rc vtss_appl_frer_itr(const uint32_t *prev_instance, uint32_t *next_instance);

/**
 * Controls the FRER functions.
 */
typedef struct  {
    /**
     * If this FRER instance is in generation mode (vtss_appl_frer_conf_t::mode
     * is VTSS_APPL_FRER_MODE_GENERATION), this member is used to reset the
     * sequence number of the sequence generator.
     *
     * If this FRER instance is in recovery mode (vtss_appl_frer_conf_t::mode
     * is VTSS_APPL_FRER_MODE_RECOVERY), this member is used to reset the
     * recovery function. It resets both possible individual recovery functions
     * and the compound recovery functions.
     *
     * A value of false has no effect.
     */
    mesa_bool_t reset;

    /**
     * Clear a sticky latent error, that is, get
     * vtss_appl_frer_notification_status_t::latent_error back to false.
     *
     * If this FRER instance is in generation mode, this member has no effect.
     * If this FRER instance is in recovery mode, but latent error detection is
     * disabled, this member has no effect.
     *
     * A value of false has no effect.
     */
    mesa_bool_t latent_error_clear;
} vtss_appl_frer_control_t;

/**
 * Control a single FRER instance.
 *
 * This function controls both a FRER instance in generation and in recovery
 * mode as described in vtss_appl_frer_control_t.
 *
 * \param instance [IN] Instance number
 * \param ctrl     [IN] Pointer to structure containing what to control.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_frer_control_set(uint32_t instance, const vtss_appl_frer_control_t *ctrl);

/**
 * Notification status of a given FRER instance (recovery mode, only).
 */
typedef struct {
    /**
     * If vtss_appl_frer_rcvy_latent_error_detection_t::enable is true and a
     * SIGNAL_LATENT_ERROR is raised, this member will become true.
     *
     * In SNMP this may be used to generate a trap and in JSON, it may be used
     * to generate a JSON notification.
     *
     * The error is sticky in the sense that once it is set, it requires the
     * user to do a reset of the error to get it back to false.
     */
    mesa_bool_t latent_error;
} vtss_appl_frer_notification_status_t;

/**
 * Get the notification status of a FRER instance.
 *
 * \param instance     [IN]  FRER instance.
 * \param notif_status [OUT] Pointer to structure receiving FRER instance's notification status.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_frer_notification_status_get(uint32_t instance, vtss_appl_frer_notification_status_t *const notif_status);

/**
 * The operational state of a FRER instance.
 * There are a few ways of not having the instance active. Each of them has its
 * own enumeration. Only when the state is VTSS_APPL_FRER_OPER_STATE_ACTIVE,
 * will the FRER instance be active and up and running.
 * However, there may still be operational warnings that may cause the instance
 * not to run optimally. See vtss_appl_frer_oper_warnings_t for a list of
 * possible warnings.
 */
typedef enum {
    VTSS_APPL_FRER_OPER_STATE_ADMIN_DISABLED, /**< Instance is inactive, because it is administratively disabled. */
    VTSS_APPL_FRER_OPER_STATE_ACTIVE,         /**< The instance is active and up and running.                     */
    VTSS_APPL_FRER_OPER_STATE_INTERNAL_ERROR, /**< Instance is inactive, because an internal error has occurred.  */
} vtss_appl_frer_oper_state_t;

/**
 * This enum holds flags that indicate various (configuration) warnings.
 *
 * If the operational state is VTSS_APPL_FRER_OPER_STATE_ACTIVE, the FRER
 * instance is indeed active, but it may be that it doesn't run as the
 * administrator thinks, because of configuration errors, which are reflected in
 * the warnings below.
 */
typedef enum {
    VTSS_APPL_FRER_OPER_WARNING_NONE                                       = 0x0000, /**< No warnings found                                               */
    VTSS_APPL_FRER_OPER_WARNING_STREAM_NOT_FOUND                           = 0x0001, /**< At least one of the matching streams doesn't exist              */
    VTSS_APPL_FRER_OPER_WARNING_STREAM_ATTACH_FAIL                         = 0x0002, /**< Failed to attach to at least one of the streams                 */
    VTSS_APPL_FRER_OPER_WARNING_STREAM_HAS_OPERATIONAL_WARNINGS            = 0x0004, /**< At least one of the streams has operational warnings            */
    VTSS_APPL_FRER_OPER_WARNING_INGRESS_EGRESS_OVERLAP                     = 0x0008, /**< There is an overlap between ingress and egress ports            */
    VTSS_APPL_FRER_OPER_WARNING_EGRESS_PORT_CNT                            = 0x0010, /**< In generation mode, only one egress port is specified           */
    VTSS_APPL_FRER_OPER_WARNING_INGRESS_NO_LINK                            = 0x0020, /**< At least one of the ingress ports doesn't have link             */
    VTSS_APPL_FRER_OPER_WARNING_EGRESS_NO_LINK                             = 0x0040, /**< At least one of the egress ports doesn't have link              */
    VTSS_APPL_FRER_OPER_WARNING_VLAN_MEMBERSHIP                            = 0x0080, /**< At least one of the egress ports is not member of the FRER VLAN */
    VTSS_APPL_FRER_OPER_WARNING_STP_BLOCKED                                = 0x0100, /**< At least one of the egress ports is blocked by STP              */
    VTSS_APPL_FRER_OPER_WARNING_MSTP_BLOCKED                               = 0x0200, /**< At least one of the egress ports is blocked by MSTP             */
    VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_NOT_FOUND                = 0x0400, /**< Stream collection doesn't exist                                 */
    VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_ATTACH_FAIL              = 0x0800, /**< Failed to attach to stream collection                           */
    VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_HAS_OPERATIONAL_WARNINGS = 0x1000, /**< The stream collection has operational warnings                  */
} vtss_appl_frer_oper_warnings_t;

/**
 * Operators for vtss_appl_frer_oper_warnings_t flags.
 */
VTSS_ENUM_BITWISE(vtss_appl_frer_oper_warnings_t);

/**
 * Status of a single FRER instance.
 */
typedef struct {
    /**
     * Operational state of this FRER instance.
     *
     * The FRER instance is inactive unless oper_state is
     * VTSS_APPL_FRER_OPER_STATE_ACTIVE.
     *
     * When inactive, non of the remaining members of this struct are valid.
     * When active, the FRER instance may, however, still have warnings. See
     * \p warnings for a list of possible warnings.
     */
    vtss_appl_frer_oper_state_t oper_state;

    /**
     * Operational warnings of this FRER instance.
     *
     * The FRER instance is error and warning free if \p oper_state is
     * VTSS_APPL_FRER_OPER_STATE_ACTIVE and \p oper_warnings is 0.
     * VTSS_APPL_FRER_OPER_WARNING_NONE.
     */
    vtss_appl_frer_oper_warnings_t oper_warnings;

    /**
     * This is the same information as can be retrieved with
     * vtss_appl_frer_notification_status_get(), but included here for
     * completeness.
     */
    vtss_appl_frer_notification_status_t notif_status;
} vtss_appl_frer_status_t;

/**
 * Get the status of a FRER instance.
 *
 * \param instance [IN]  FRER instance
 * \param status   [OUT] Pointer to structure receiving FRER instance's status
 * \return Error code.
 */
mesa_rc vtss_appl_frer_status_get(uint32_t instance, vtss_appl_frer_status_t *status);

/**
 * FRER counters.
 *
 * Counters starting with "rcvy_" are only valid when the FRER instance is in
 * recovery mode.
 * Likewise, counters starting with "gen_" are only valid when the FRER instance
 * is in generation mode.
 */
typedef struct {
    uint64_t rcvy_out_of_order_packets; /**< frerCpsSeqRcvyOutOfOrderPackets                */
    uint64_t rcvy_rogue_packets;        /**< frerCpsSeqRcvyRoguePackets                     */
    uint64_t rcvy_passed_packets;       /**< frerCpsSeqRcvyPassedPackets                    */
    uint64_t rcvy_discarded_packets;    /**< frerCpsSeqRcvyDiscardedPackets                 */
    uint64_t rcvy_lost_packets;         /**< frerCpsSeqRcvyLostPackets                      */
    uint64_t rcvy_tagless_packets;      /**< frerCpsSeqRcvyTaglessPackets                   */
    uint64_t rcvy_resets;               /**< frerCpsSeqRcvyResets                           */
    uint64_t rcvy_latent_error_resets;  /**< frerCpsSeqRcvyLatentErrorResets  (S/W counted) */
    uint64_t gen_resets;                /**< frerCpsSeqGenResets (S/W counted)              */
    uint64_t gen_matches;               /**< Number of frames that matched this stream      */
} vtss_appl_frer_statistics_t;

/**
 * Get FRER statistics.
 *
 * This function can get counters for a FRER instance whether in generation or
 * recovery mode.
 *
 * In generation mode, \p stream_id must be set to VTSS_APPL_STREAM_ID_NONE and
 * \p ifindex must be set to VTSS_IFINDEX_NONE.
 *
 * In recovery mode, \p ifindex must be set to the ifindex of one of the egress
 * interfaces marked in the configuration (vtss_appl_frer_conf_t::egress_ports).
 *
 * To obtain counters for a compound recovery function, use
 * VTSS_APPL_STREAM_ID_NONE as \p stream_id.
 *
 * To obtain counters for an invidiual recovery function, use one of the stream
 * IDs used in vtss_appl_frer_conf_t::stream_ids. If the FRER instance is not
 * in invidiual recovery mode, an error will be returned.
 *
 * In recovery mode, <ifindex, stream_id> can be set to <VTSS_IFINDEX_NONE,
 * VTSS_APPL_STREAM_ID_NONE> only in one case, namely when there are no
 * egress ports yet configured. The function will return all-zeros statistics.
 *
 * \param instance   [IN]  Instance number
 * \param ifindex    [IN]  ifindex of the egress port to get compound or individual statistics for.
 * \param stream_id  [IN]  Ingress stream ID. Set to VTSS_APPL_STREAM_ID_NONE to get compound statistics or one of the ingress stream IDs to get individual counters.
 * \param statistics [OUT] Pointer to structure containing counters to obtain.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_frer_statistics_get(uint32_t instance, vtss_ifindex_t ifindex, vtss_appl_stream_id_t stream_id, vtss_appl_frer_statistics_t *statistics);

/**
 * \brief Clear FRER instance statistics.
 *
 * In case of recovery, both the individual and the compound recovery statistics
 * are cleared.
 *
 * \param instance [IN] FRER instance
 *
 * \return VTSS_RC_OK if operation succeeds.
 **/
mesa_rc vtss_appl_frer_statistics_clear(uint32_t instance);

/**
 * Statistics iterator.
 *
 * Since the number of counters for a given FRER instance varies depending on
 * mode (generation/recovery) and whether individual recovery is enabled or not,
 * it is desirable to be able to iterate across all instances and their
 * counters. This is what this function is meant for.
 *
 * This function returns the next defined FRER instance number and its port
 * number, and stream ID. The end is reached when VTSS_RC_ERROR is returned.
 *
 * For generation instances, one iteration occurs with next_ifindex set to
 * VTSS_IFINDEX_NONE and next_stream_id set to VTSS_APPL_STREAM_ID_NONE.
 * The following iterations occur:
 * <IFINDEX_NONE, STREAM_ID_NONE>
 *
 * For recovery instances in non-individual recovery mode, one iteration per
 * egress port occurs, with next_ifindex set to the egress port and
 * next_stream_id is set to VTSS_APPL_STREAM_ID_NONE.
 * <egress_port_1, STREAM_ID_NONE>, <egress_port_2, STREAM_ID_NONE>, ..., <egress_port_M, STREAM_ID_NONE>
 *
 * For recovery instances in individual recovery mode, the following iterations
 * occur:
 * <egress_port_1, stream_id_1>, <egress_port_1, stream_id_2>, ..., <egress_port_1, stream_id_N>, <egress_port_1, STREAM_ID_NONE>
 * <egress_port_2, stream_id_1>, <egress_port_2, stream_id_2>, ..., <egress_port_2, stream_id_N>, <egress_port_2, STREAM_ID_NONE>
 * ...
 * <egress_port_M, stream_id_1>, <egress_port_M, stream_id_2>, ..., <egress_port_M, stream_id_N>, <egress_port_M, STREAM_ID_NONE>
 *
 * If no egress ports are configured, the function returns one entry
 * (VTSS_IFINDEX_NONE, VTSS_APPL_STREAM_ID_NONE>, which will always return
 * all-zeros statistics in the call to vtss_appl_frer_statistics_get().
 *
 * \param prev_instance  [IN]  Previous instance number
 * \param next_instance  [OUT] Next instance
 * \param prev_ifindex   [IN]  Previous ifindex
 * \param next_ifindex   [OUT] Next ifindex
 * \param prev_stream_id [IN]  Previous stream ID
 * \param next_stream_id [OUT] Next stream ID
 *
 * \return VTSS_RC_OK as long as *_next are valid.
 */
mesa_rc vtss_appl_frer_statistics_itr(const uint32_t *prev_instance, uint32_t *next_instance, const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex, const vtss_appl_stream_id_t *prev_stream_id, vtss_appl_stream_id_t *next_stream_id);

#endif /* _VTSS_APPL_FRER_HXX_ */
