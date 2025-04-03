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

/*!
 * \file
 *
 * \brief Public QoS API.
 *
 * This header file describes QoS control functions and types.\n
 * This API enables configuration of various kind of QoS functionality, such as:
 * \li Basic and advanced traffic classification.
 * \li Policers and shapers.
 * \li Storm control.
 * \li Scheduler mode.
 * \li Ingress and egress translation.
 * \li IEEE 802.1Qbv (Enhancements for scheduled traffic).
 *
 */


#ifndef _VTSS_APPL_QOS_H_
#define _VTSS_APPL_QOS_H_

#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/vcap_types.h>
#include <vtss/appl/module_id.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * QoS Error Codes
 ****************************************************************************/

/*! \brief QoS error codes (mesa_rc). */
enum {
    VTSS_APPL_QOS_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_QOS), /*!< Generic error code. */
    VTSS_APPL_QOS_ERROR_PARM,                                         /*!< Illegal parameter. */
    VTSS_APPL_QOS_ERROR_FEATURE,                                      /*!< Feature not present. */
    VTSS_APPL_QOS_ERROR_QCE_NOT_FOUND,                                /*!< QCE not found. */
    VTSS_APPL_QOS_ERROR_QCE_TABLE_FULL,                               /*!< QCE table full. */
    VTSS_APPL_QOS_ERROR_QCL_USER_NOT_FOUND,                           /*!< User not found. */
    VTSS_APPL_QOS_ERROR_STACK_STATE,                                  /*!< Invalid switch id. */
    VTSS_APPL_QOS_ERROR_UNHANDLED_QUEUES,                             /*!< Unhandled queues in Gate Control List. */
    VTSS_APPL_QOS_ERROR_INVALID_CYCLE_TIME,                           /*!< Invalid CycleTime configuration. */
    VTSS_APPL_QOS_ERROR_SFI_SAME_PORT_OR_DIFFERENT_PRIO,              /*!< Using same port or different priorities and SID in different SFIs. */
    VTSS_APPL_QOS_ERROR_SID_IN_USE_OR_INVALID_OR_TOO_MANY,            /*!< Reserved SID is in use, invalid or has too many references. */
};

/****************************************************************************
 * QoS Capabilities
 ****************************************************************************/

/*! \brief QoS capabilities.
 *
 *  Contains platform specific QoS definitions.
 */
typedef struct {
    mesa_prio_t        class_min;                      /*!< Minimum allowed QoS class value. */
    mesa_prio_t        class_max;                      /*!< Maximum allowed QoS class value. */

    mesa_dp_level_t    dpl_min;                        /*!< Minimum allowed DPL value. */
    mesa_dp_level_t    dpl_max;                        /*!< Maximum allowed DPL value. */

    mesa_wred_group_t  wred_group_min;                 /*!< Minimum allowed WRED group value. */
    mesa_wred_group_t  wred_group_max;                 /*!< Maximum allowed WRED group value. */
    mesa_dp_level_t    wred_dpl_min;                   /*!< Minimum allowed WRED DPL value. */
    mesa_dp_level_t    wred_dpl_max;                   /*!< Maximum allowed WRED DPL value. */
    mesa_cosid_t       cosid_min;                      /*!< Minimum allowed COSID value. */
    mesa_cosid_t       cosid_max;                      /*!< Maximum allowed COSID value. */

    mesa_bitrate_t     port_policer_bit_rate_min;      /*!< Minimum allowed bit rate in kbps. */
    mesa_bitrate_t     port_policer_bit_rate_max;      /*!< Maximum allowed bit rate in kbps. Zero means 'bit rate not supported'. */
    mesa_burst_level_t port_policer_bit_burst_min;     /*!< Minimum allowed burst in bytes when using bit rate. */
    mesa_burst_level_t port_policer_bit_burst_max;     /*!< Maximum allowed burst in bytes when using bit rate. */
    mesa_bitrate_t     port_policer_frame_rate_min;    /*!< Minimum allowed frame rate in fps. */
    mesa_bitrate_t     port_policer_frame_rate_max;    /*!< Maximum allowed frame rate in fps. Zero means 'frame rate not supported'. */
    mesa_burst_level_t port_policer_frame_burst_min;   /*!< Minimum allowed burst in frames when using frame rate. */
    mesa_burst_level_t port_policer_frame_burst_max;   /*!< Maximum allowed burst in frames when using frame rate. */

    mesa_bitrate_t     queue_policer_bit_rate_min;     /*!< Minimum allowed bit rate in kbps. */
    mesa_bitrate_t     queue_policer_bit_rate_max;     /*!< Maximum allowed bit rate in kbps. Zero means 'bit rate not supported'. */
    mesa_burst_level_t queue_policer_bit_burst_min;    /*!< Minimum allowed burst in bytes when using bit rate. */
    mesa_burst_level_t queue_policer_bit_burst_max;    /*!< Maximum allowed burst in bytes when using bit rate. */
    mesa_bitrate_t     queue_policer_frame_rate_min;   /*!< Minimum allowed frame rate in fps. */
    mesa_bitrate_t     queue_policer_frame_rate_max;   /*!< Maximum allowed frame rate in fps. Zero means 'frame rate not supported'. */
    mesa_burst_level_t queue_policer_frame_burst_min;  /*!< Minimum allowed burst in frames when using frame rate. */
    mesa_burst_level_t queue_policer_frame_burst_max;  /*!< Maximum allowed burst in frames when using frame rate. */

    mesa_bitrate_t     port_shaper_bit_rate_min;       /*!< Minimum allowed bit rate in kbps. */
    mesa_bitrate_t     port_shaper_bit_rate_max;       /*!< Maximum allowed bit rate in kbps. Zero means 'bit rate not supported'. */
    mesa_burst_level_t port_shaper_bit_burst_min;      /*!< Minimum allowed burst in bytes when using bit rate. */
    mesa_burst_level_t port_shaper_bit_burst_max;      /*!< Maximum allowed burst in bytes when using bit rate. */
    mesa_bitrate_t     port_shaper_frame_rate_min;     /*!< Minimum allowed frame rate in fps. */
    mesa_bitrate_t     port_shaper_frame_rate_max;     /*!< Maximum allowed frame rate in fps. Zero means 'frame rate not supported'. */
    mesa_burst_level_t port_shaper_frame_burst_min;    /*!< Minimum allowed burst in frames when using frame rate. */
    mesa_burst_level_t port_shaper_frame_burst_max;    /*!< Maximum allowed burst in frames when using frame rate. */

    mesa_bitrate_t     queue_shaper_bit_rate_min;      /*!< Minimum allowed bit rate in kbps. */
    mesa_bitrate_t     queue_shaper_bit_rate_max;      /*!< Maximum allowed bit rate in kbps. Zero means 'bit rate not supported'. */
    mesa_burst_level_t queue_shaper_bit_burst_min;     /*!< Minimum allowed burst in bytes when using bit rate. */
    mesa_burst_level_t queue_shaper_bit_burst_max;     /*!< Maximum allowed burst in bytes when using bit rate. */
    mesa_bitrate_t     queue_shaper_frame_rate_min;    /*!< Minimum allowed frame rate in fps. */
    mesa_bitrate_t     queue_shaper_frame_rate_max;    /*!< Maximum allowed frame rate in fps. Zero means 'frame rate not supported'. */
    mesa_burst_level_t queue_shaper_frame_burst_min;   /*!< Minimum allowed burst in frames when using frame rate. */
    mesa_burst_level_t queue_shaper_frame_burst_max;   /*!< Maximum allowed burst in frames when using frame rate. */

    mesa_bitrate_t     global_storm_bit_rate_min;      /*!< Minimum allowed bit rate in kbps. */
    mesa_bitrate_t     global_storm_bit_rate_max;      /*!< Maximum allowed bit rate in kbps. Zero means 'bit rate not supported'. */
    mesa_burst_level_t global_storm_bit_burst_min;     /*!< Minimum allowed burst in bytes when using bit rate. */
    mesa_burst_level_t global_storm_bit_burst_max;     /*!< Maximum allowed burst in bytes when using bit rate. */
    mesa_bitrate_t     global_storm_frame_rate_min;    /*!< Minimum allowed frame rate in fps. */
    mesa_bitrate_t     global_storm_frame_rate_max;    /*!< Maximum allowed frame rate in fps. Zero means 'frame rate not supported'. */
    mesa_burst_level_t global_storm_frame_burst_min;   /*!< Minimum allowed burst in frames when using frame rate. */
    mesa_burst_level_t global_storm_frame_burst_max;   /*!< Maximum allowed burst in frames when using frame rate. */

    mesa_bitrate_t     port_storm_bit_rate_min;        /*!< Minimum allowed bit rate in kbps. */
    mesa_bitrate_t     port_storm_bit_rate_max;        /*!< Maximum allowed bit rate in kbps. Zero means 'bit rate not supported'. */
    mesa_burst_level_t port_storm_bit_burst_min;       /*!< Minimum allowed burst in bytes when using bit rate. */
    mesa_burst_level_t port_storm_bit_burst_max;       /*!< Maximum allowed burst in bytes when using bit rate. */
    mesa_bitrate_t     port_storm_frame_rate_min;      /*!< Minimum allowed frame rate in fps. */
    mesa_bitrate_t     port_storm_frame_rate_max;      /*!< Maximum allowed frame rate in fps. Zero means 'frame rate not supported'. */
    mesa_burst_level_t port_storm_frame_burst_min;     /*!< Minimum allowed burst in frames when using frame rate. */
    mesa_burst_level_t port_storm_frame_burst_max;     /*!< Maximum allowed burst in frames when using frame rate. */

    mesa_qce_id_t      qce_id_min;                     /*!< Minimum allowed QCE id. */
    mesa_qce_id_t      qce_id_max;                     /*!< Maximum allowed QCE id. */

    mesa_qos_ingress_map_id_t ingress_map_id_min;      /*!< Minimum allowed ingress map id. */
    mesa_qos_ingress_map_id_t ingress_map_id_max;      /*!< Maximum allowed ingress map id. */
    mesa_qos_egress_map_id_t  egress_map_id_min;       /*!< Minimum allowed egress map id. */
    mesa_qos_egress_map_id_t  egress_map_id_max;       /*!< Maximum allowed egress map id. */

    uint32_t                  dwrr_cnt_mask;           /*!< Bitmask of allowed values of dwrr_cnt (zero is always allowed).\n
                                                         Example:\n
                                                         00100000 (0x20): Allowed values are 0 and 6.\n
                                                         11111110 (0xFE): Allowed values are 0 and 2..8. */

    mesa_bool_t               has_global_storm_policers;      /*!< Platform supports global storm policers. */
    mesa_bool_t               has_port_storm_policers;        /*!< Platform supports port storm policers. */
    mesa_bool_t               has_port_queue_policers;        /*!< Platform supports port queue policers. */
    mesa_bool_t               has_wred_v1;                    /*!< Platform supports WRED version 1. */
    mesa_bool_t               has_wred_v2;                    /*!< Platform supports WRED version 2. */
    mesa_bool_t               has_wred_v3;                    /*!< Platform supports WRED version 3. */
    mesa_bool_t               has_fixed_tag_cos_map;          /*!< Platform supports fixed tag to cos mapping only. */
    mesa_bool_t               has_tag_classification;         /*!< Platform supports using tag for classification. */
    mesa_bool_t               has_tag_remarking;              /*!< Platform supports tag remarking. */
    mesa_bool_t               has_dscp;                       /*!< Platform supports DSCP based classification, translation and remarking. */
    mesa_bool_t               has_dscp_dpl_class;             /*!< Platform supports DPL based DSCP classification. */
    mesa_bool_t               has_dscp_dpl_remark;            /*!< Platform supports DPL based DSCP remarking. */
    mesa_bool_t               has_cosid_classification;       /*!< Platform supports per port classification of COSID. */
    mesa_bool_t               has_port_policers_fc;           /*!< Platform supports flow control in port policers. */
    mesa_bool_t               has_queue_policers_fc;          /*!< Platform supports flow control in queue policers. */
    mesa_bool_t               has_port_shapers_dlb;           /*!< Platform supports dual leaky bucket egress port shapers. */
    mesa_bool_t               has_queue_shapers_dlb;          /*!< Platform supports dual leaky bucket egress queue shapers. */
    mesa_bool_t               has_queue_shapers_frame_dlb;    /*!< Platform supports dual leaky bucket egress frame based queue shapers. */
    mesa_bool_t               has_queue_shapers_eb;           /*!< Platform supports excess bandwidth in queue shapers. */
    mesa_bool_t               has_queue_shapers_crb;          /*!< Platform supports credit based in queue shapers. */
    mesa_bool_t               has_queue_cut_through;          /*!< Platform supports cut through feature. */
    mesa_bool_t               has_queue_frame_preemption;     /*!< Platform supports frame preemption feature. */
    mesa_bool_t               has_ingress_map;                /*!< Platform supports ingress mapping. */
    mesa_bool_t               has_egress_map;                 /*!< Platform supports egress mapping. */
    mesa_bool_t               has_qce;                        /*!< Platform supports QCEs. */
    mesa_bool_t               has_qce_address_mode;           /*!< Platform supports QCE address mode. */
    mesa_bool_t               has_qce_key_type;               /*!< Platform supports QCE key type. */
    mesa_bool_t               has_qce_mac_oui;                /*!< Platform supports QCE MAC OUI part only (24 most significant bits). */
    mesa_bool_t               has_qce_dmac;                   /*!< Platform supports QCE destination MAC key. */
    mesa_bool_t               has_qce_dip;                    /*!< Platform supports QCE destination IP key. */
    mesa_bool_t               has_qce_ctag;                   /*!< Platform supports QCE VLAN C-TAG key. */
    mesa_bool_t               has_qce_stag;                   /*!< Platform supports QCE VLAN S-TAG key. */
    mesa_bool_t               has_qce_inner_tag;              /*!< Platform supports QCE VLAN inner tag key. */
    mesa_bool_t               has_qce_action_pcp_dei;         /*!< Platform supports QCE PCP and DEI action. */
    mesa_bool_t               has_qce_action_policy;          /*!< Platform supports QCE policy action. */
    mesa_bool_t               has_qce_action_map;             /*!< Platform supports QCE ingress map action. */
    mesa_bool_t               has_shapers_rt;                 /*!< Platform supports rate type selection in QoS egress shapers. */
    mesa_bool_t               has_wred2_or_wred3;             /*!< Platform supports WRED version 2. or WRED version 3. */
    mesa_bool_t               has_dscp_dp2;                   /*!< Platform supports DPL based DSCP classification and maximum allowed DPL value is greater than 1. */
    mesa_bool_t               has_dscp_dp3;                   /*!< Platform supports DPL based DSCP classification and maximum allowed DPL value is greater than 2. */
    mesa_bool_t               has_default_pcp_and_dei;        /*!< Platform supports default pcp and dei. */
    mesa_bool_t               has_trust_tag;                  /*!< Platform does not supports fixed tag to cos mapping only and supports using tag for classification. */
    mesa_bool_t               has_qbv;                        /*!< Platform supports 802.1Qbv (TAS) */
    mesa_bool_t               has_psfp;                       /*!< Platform supports 802.1Qci (Per-Stream Filtering and Policing) */
    mesa_bool_t               has_mpls_tc_obsolete;           /*!< Platform supports MPLS TC mapping (obsolete. Always false) */
} vtss_appl_qos_capabilities_t;

/*!
 * \brief Get QoS capabilities.
 *
 * \param c [OUT] The QoS capabilities.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_capabilities_get(vtss_appl_qos_capabilities_t *c);

/****************************************************************************
 * QoS Global Storm Policer Configuration
 ****************************************************************************/

/*! \brief QoS global storm policer configuration. */
typedef struct {
    mesa_bool_t        enable;     /*!< Enable storm policer */
    mesa_packet_rate_t rate;       /*!< Rate. Unit: kbps (or fps if frame_rate == TRUE). */
    mesa_bool_t        frame_rate; /*!< Measure rate in fps instead of kbps.
                                     fps  (frame_rate == TRUE)  is only valid if capabilities.global_storm_frame_rate_max is non-zero.
                                     kbps (frame_rate == FALSE) is only valid if capabilities.global_storm_bit_rate_max is non-zero. */
} vtss_appl_qos_global_storm_policer_t;

/*!
 * \brief Get QoS global unicast storm policer configuration.
 *
 * Only implemented if capabilities.has_global_storm_policers is TRUE
 *
 * \param conf [OUT] The global unicast storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_global_uc_policer_get(vtss_appl_qos_global_storm_policer_t *conf);

/*!
 * \brief Set QoS global unicast storm policer configuration.
 *
 * Only implemented if capabilities.has_global_storm_policers is TRUE
 *
 * \param conf [IN] The global unicast storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_global_uc_policer_set(const vtss_appl_qos_global_storm_policer_t *conf);

/*!
 * \brief Get QoS global multicast storm policer configuration.
 *
 * Only implemented if capabilities.has_global_storm_policers is TRUE
 *
 * \param conf [OUT] The global multicast storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_global_mc_policer_get(vtss_appl_qos_global_storm_policer_t *conf);

/*!
 * \brief Set QoS global multicast storm policer configuration.
 *
 * Only implemented if capabilities.has_global_storm_policers is TRUE
 *
 * \param conf [IN] The global multicast storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_global_mc_policer_set(const vtss_appl_qos_global_storm_policer_t *conf);

/*!
 * \brief Get QoS global broadcast storm policer configuration.
 *
 * Only implemented if capabilities.has_global_storm_policers is TRUE
 *
 * \param conf [OUT] The global broadcast storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_global_bc_policer_get(vtss_appl_qos_global_storm_policer_t *conf);

/*!
 * \brief Set QoS global broadcast storm policer configuration.
 *
 * Only implemented if capabilities.has_global_storm_policers is TRUE
 *
 * \param conf [IN] The global broadcast storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_global_bc_policer_set(const vtss_appl_qos_global_storm_policer_t *conf);

/****************************************************************************
* QoS Global Status
****************************************************************************/

/*! \brief QoS global status. */
typedef struct {
    mesa_bool_t storm_active;  /*!< TRUE if global storm policing was active. */
} vtss_appl_qos_status_t;


/*!
* \brief Get QoS global status.
*
* \param status  [OUT] The global status.
*
* \return Return code.
*/
mesa_rc vtss_appl_qos_status_get(vtss_appl_qos_status_t *status);

/****************************************************************************
 * QoS Weighted Random Early Detection (WRED) Configuration
 ****************************************************************************/

/*! \brief WRED max selector.
 *
 * Selects if 'max' means 'max drop probability' or 'max fill level' */
typedef enum {
    VTSS_APPL_QOS_WRED_MAX_DP, /*!< Unit for max is drop probability. */
    VTSS_APPL_QOS_WRED_MAX_FL  /*!< Unit for max is fill level. */
} vtss_appl_qos_wred_max_t;

/*! \brief Weighted Random Early Detection configuration. */
typedef struct {
    mesa_bool_t              enable;     /*!< Enable/disable WRED. */
    mesa_pct_t               min;        /*!< Minimum fill level. */
    mesa_pct_t               max;        /*!< Maximum fill level (or in V2/V3: drop probability - selected by max_unit). */
    mesa_pct_t               max_prob_1; /*!< Drop probability for DPL 1 at max fill level.
                                           Only valid if capabilities.has_wred_v1 is TRUE. */
    mesa_pct_t               max_prob_2; /*!< Drop probability for DPL 2 at max fill level.
                                           Only valid if capabilities.has_wred_v1 is TRUE. */
    mesa_pct_t               max_prob_3; /*!< Drop probability for DPL 3 at max fill level.
                                           Only valid if capabilities.has_wred_v1 is TRUE. */
    vtss_appl_qos_wred_max_t max_unit;   /*!< Selects the unit for max.
                                           Only valid if capabilities.has_wred_v2 or capabilities.has_wred_v3 is TRUE. */
} vtss_appl_qos_wred_t;

/*!
 * \brief Iterator for retrieving QoS WRED configuration key/index
 *
 * Only implemented if capabilities.has_wred_v1, capabilities.has_wred_v2 or capabilities.has_wred_v3 is TRUE
 *
 * \param prev_group [IN]    Group to be used for indexing determination.
 * \param prev_queue [IN]    Queue to be used for indexing determination.
 * \param prev_dpl   [IN]    Dpl to be used for indexing determination.
 *
 * \param next_group [OUT]   The key/index of group should be used for the GET operation.
 * \param next_queue [OUT]   The key/index of queue should be used for the GET operation.
 * \param next_dpl   [OUT]   The key/index of dpl should be used for the GET operation.
 *                           When IN is NULL, assign the first index.
 *                           When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_wred_itr(const mesa_wred_group_t *prev_group, mesa_wred_group_t *next_group,
                               const mesa_prio_t       *prev_queue, mesa_prio_t       *next_queue,
                               const mesa_dp_level_t   *prev_dpl,   mesa_dp_level_t   *next_dpl);

/*!
 * \brief Get QoS global WRED configuration.
 *
 * Only implemented if capabilities.has_wred_v1, capabilities.has_wred_v2 or capabilities.has_wred_v3 is TRUE
 *
 * \param group [IN]  The group for which to get the configuration.
 * \param queue [IN]  The queue for which to get the configuration.
 * \param dpl   [IN]  The dpl for which to get the configuration.
 * \param conf  [OUT] The global WRED configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_wred_get(mesa_wred_group_t group, mesa_prio_t queue, mesa_dp_level_t dpl, vtss_appl_qos_wred_t *conf);

/*!
 * \brief Set QoS global WRED configuration.
 *
 * Only implemented if capabilities.has_wred_v1, capabilities.has_wred_v2 or capabilities.has_wred_v3 is TRUE
 *
 * \param group [IN] The group for which to set the configuration.
 * \param queue [IN] The queue for which to set the configuration.
 * \param dpl   [IN] The dpl for which to set the configuration.
 * \param conf  [IN] The global WRED configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_wred_set(mesa_wred_group_t group, mesa_prio_t queue, mesa_dp_level_t dpl, const vtss_appl_qos_wred_t *conf);

/****************************************************************************
 * QoS Global DSCP Configuration
 ****************************************************************************/

/*! \brief QoS global DSCP configuration. */
typedef struct {
    mesa_bool_t     trust;          /*!< Ingress: Only trusted DSCP values are used for CoS and DPL classification. */
    mesa_prio_t     cos;            /*!< Ingress: Mapping from DSCP value to CoS. */
    mesa_dp_level_t dpl;            /*!< Ingress: Mapping from DSCP value to DPL. */
    mesa_dscp_t     dscp;           /*!< Ingress: Translated DSCP value. Used when port.dscp_translate = TRUE. */
    mesa_bool_t     remark;         /*!< Ingress: Enable remarking for this DSCP. Used when port.dscp_mode = MESA_DSCP_MODE_SEL. */
    mesa_dscp_t     dscp_remap;     /*!< Egress:  Remap DSCP to another (DP unaware or DP level = 0). */
    mesa_dscp_t     dscp_remap_dp1; /*!< Egress:  Remap DSCP to another (DP aware and DP level = 1).
                                      Only valid if capabilities.has_dscp_dpl_remark is TRUE. */
} vtss_appl_qos_dscp_entry_t;

/*!
 * \brief Iterator for retrieving QoS DSCP configuration key/index
 *
 * Only implemented if capabilities.has_dscp is TRUE
 *
 * \param prev      [IN]    DSCP to be used for indexing determination.
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_dscp_itr(const mesa_dscp_t *prev, mesa_dscp_t *next);

/*!
 * \brief Get QoS global DSCP configuration.
 *
 * Only implemented if capabilities.has_dscp is TRUE
 *
 * \param dscp [IN]  The DSCP value for which to get the configuration.
 * \param conf [OUT] The global DSCP configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_dscp_get(mesa_dscp_t dscp, vtss_appl_qos_dscp_entry_t *conf);

/*!
 * \brief Set QoS global DSCP configuration.
 *
 * Only implemented if capabilities.has_dscp is TRUE
 *
 * \param dscp [IN] The DSCP value for which to set the configuration.
 * \param conf [IN] The global DSCP configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_dscp_set(mesa_dscp_t dscp, const vtss_appl_qos_dscp_entry_t *conf);

/****************************************************************************
 * QoS Global COS-DSCP Configuration
 ****************************************************************************/

/*! \brief QoS global COS-DSCP configuration. */
typedef struct {
    mesa_dscp_t dscp;     /*!< Ingress: Mapping from CoS to DSCP (DP unaware or DP level = 0). */
    mesa_dscp_t dscp_dp1; /*!< Ingress: Mapping from CoS to DSCP (DP aware and DP level = 1).
                            Only valid if capabilities.has_dscp_dpl_class is TRUE. */
    mesa_dscp_t dscp_dp2; /*!< Ingress: Mapping from CoS to DSCP (DP aware and DP level = 2).
                            Only valid if capabilities.has_dscp_dpl_class is TRUE and capabilities.dpl_max >= 2. */
    mesa_dscp_t dscp_dp3; /*!< Ingress: Mapping from CoS to DSCP (DP aware and DP level = 3).
                            Only valid if capabilities.has_dscp_dpl_class is TRUE and capabilities.dpl_max >= 3. */
} vtss_appl_qos_cos_dscp_entry_t;

/*!
 * \brief Iterator for retrieving QoS CsS-DSCP configuration key/index
 *
 * Only implemented if capabilities.has_dscp is TRUE
 *
 * \param prev      [IN]    CoS to be used for indexing determination.
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_cos_dscp_itr(const mesa_prio_t *prev, mesa_prio_t *next);

/*!
 * \brief Get QoS global CoS-DSCP configuration.
 *
 * Only implemented if capabilities.has_dscp is TRUE
 *
 * \param queue [IN]  The CoS value for which to get the configuration.
 * \param conf  [OUT] The global CoS-DSCP configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_cos_dscp_get(mesa_prio_t queue, vtss_appl_qos_cos_dscp_entry_t *conf);

/*!
 * \brief Set QoS global CoS-DSCP configuration.
 *
 * Only implemented if capabilities.has_dscp is TRUE
 *
 * \param queue [IN] The CoS value for which to set the configuration.
 * \param conf  [IN] The global CoS-DSCP configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_cos_dscp_set(mesa_prio_t queue, const vtss_appl_qos_cos_dscp_entry_t *conf);

/****************************************************************************
 * QoS Interface Configuration
 ****************************************************************************/

/*! \brief Tag Remark Mode. */
typedef enum {
    VTSS_APPL_QOS_TAG_REMARK_MODE_CLASSIFIED = 0, /*!< Use classified PCP/DEI values. */
    VTSS_APPL_QOS_TAG_REMARK_MODE_DEFAULT    = 2, /*!< Use default (configured) PCP/DEI values. */
    VTSS_APPL_QOS_TAG_REMARK_MODE_MAPPED     = 3  /*!< Use mapped versions of classified QOS class and DP level .*/
} vtss_appl_qos_tag_remark_mode_t;

/*! \brief DSCP mode for ingress port. */
typedef enum {
    VTSS_APPL_QOS_DSCP_MODE_NONE, /*!< DSCP not remarked. */
    VTSS_APPL_QOS_DSCP_MODE_ZERO, /*!< DSCP value zero remarked. */
    VTSS_APPL_QOS_DSCP_MODE_SEL,  /*!< DSCP values selected above (dscp_remark) are remarked. */
    VTSS_APPL_QOS_DSCP_MODE_ALL   /*!< DSCP remarked for all values. */
} vtss_appl_qos_dscp_mode_t;

/*! \brief DSCP mode for egress port. */
typedef enum {
    VTSS_APPL_QOS_DSCP_EMODE_DISABLE,  /*!< DSCP not remarked. */
    VTSS_APPL_QOS_DSCP_EMODE_REMARK,   /*!< DSCP remarked with DSCP value from analyzer. */
    VTSS_APPL_QOS_DSCP_EMODE_REMAP,    /*!< DSCP remarked with DSCP value from analyzer remapped through global remap table. */
    VTSS_APPL_QOS_DSCP_EMODE_REMAP_DPA /*!< DSCP remarked with DSCP value from analyzer remapped through global remap dp aware tables.
                                         Only valid if capabilities.has_dscp_dpl_remark is TRUE. */
} vtss_appl_qos_dscp_emode_t;

/*! \brief QoS interface configuration. */
typedef struct {
    mesa_prio_t                     default_cos;     /*!< Default classified CoS. */
    mesa_dp_level_t                 default_dpl;     /*!< Default classified DPL. */
    mesa_tagprio_t                  default_pcp;     /*!< Default classified PCP.
                                                       Only valid if capabilities.has_fixed_tag_cos_map is FALSE. */
    mesa_dei_t                      default_dei;     /*!< Default classified DEI.
                                                       Only valid if capabilities.has_fixed_tag_cos_map is FALSE. */
    mesa_bool_t                     trust_tag;       /*!< Ingress classification based on vlan tag (PCP and DEI).
                                                       Only valid if capabilities.has_fixed_tag_cos_map is FALSE and capabilities.has_tag_classification is TRUE. */
    mesa_bool_t                     trust_dscp;      /*!< Ingress classification of CoS and DPL based on DSCP.
                                                       Only valid if capabilities.has_dscp is TRUE. */

    uint8_t                         dwrr_cnt;        /*!< Number of queues running Deficit Weighted Round Robin scheduling.
                                                      This value is restricted to 0 or one of the values defined by capabilities.dwrr_cnt_mask.*/

    vtss_appl_qos_tag_remark_mode_t tag_remark_mode; /*!< Egress tag remark mode.
                                                       Only valid if capabilities.has_tag_remarking is TRUE. */
    mesa_tagprio_t                  tag_default_pcp; /*!< Default PCP value for egress tag remarking.
                                                       Only valid if capabilities.has_tag_remarking is TRUE. */
    mesa_dei_t                      tag_default_dei; /*!< Default DEI value for egress tag remarking.
                                                       Only valid if capabilities.has_tag_remarking is TRUE. */

    mesa_bool_t                     dscp_translate;  /*!< Ingress DSCP translation.
                                                       Only valid if capabilities.has_dscp is TRUE. */
    vtss_appl_qos_dscp_mode_t       dscp_imode;      /*!< Ingress DSCP mode.
                                                       Only valid if capabilities.has_dscp is TRUE. */
    vtss_appl_qos_dscp_emode_t      dscp_emode;      /*!< Egress DSCP mode.
                                                       Only valid if capabilities.has_dscp is TRUE. */

    mesa_bool_t                     dmac_dip;        /*!< Enable DMAC/DIP matching in QCLs (default SMAC/SIP).
                                                       Only valid if capabilities.has_qce_address_mode is TRUE. */

    mesa_vcap_key_type_t            key_type;        /*!< Key type for received frames.
                                                       Only valid if capabilities.has_qce_key_type is TRUE. */

    mesa_wred_group_t               wred_group;      /*!< WRED group number.
                                                       Only valid if capabilities.has_wred_v3 is TRUE. */

    mesa_cosid_t                    default_cosid;   /*!< Default classified CoS ID.
                                                       Only valid if capabilities.has_cosid_classification is TRUE. */

    mesa_qos_ingress_map_id_t       ingress_map;     /*!< Ingress map to use for classification. Default is none.
                                                       Only valid if capabilities.has_ingress_map is TRUE. */

    mesa_qos_egress_map_id_t        egress_map;      /*!< Egress map to use for remarking. Default is none.
                                                       Only valid if capabilities.has_egress_map is TRUE. */

} vtss_appl_qos_if_conf_t;

/*!
 * \brief Iterator for retrieving interface configuration
 *
 * \param prev [IN]  Previous interface index. May be given as a NULL,
 *                   in which case the first interface will be returned.
 * \param next [OUT] The next valid interface numerically larger than the input.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_interface_conf_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next);

/*!
 * \brief Get QoS interface configuration.
 *
 * \param ifindex [IN]  The interface index for which to get the configuration.
 * \param conf    [OUT] The interface configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_interface_conf_get(vtss_ifindex_t ifindex, vtss_appl_qos_if_conf_t *conf);

/*!
 * \brief Set QoS interface configuration.
 *
 * \param ifindex [IN] The interface index for which to get the configuration.
 * \param conf    [IN] The interface configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_interface_conf_set(vtss_ifindex_t ifindex, const vtss_appl_qos_if_conf_t *conf);

/****************************************************************************
 * QoS Interface Status
 ****************************************************************************/

/*! \brief QoS interface status. */
typedef struct {
    mesa_prio_t default_cos; /*!< Currently active default CoS. */
} vtss_appl_qos_if_status_t;

/*!
 * \brief Iterator for retrieving interface status
 *
 * \param prev [IN]  Previous interface index. May be given as a NULL,
 *                   in which case the first interface will be returned.
 * \param next [OUT] The next valid interface numerically larger than the input.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_interface_status_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next);

/*!
 * \brief Get QoS interface status.
 *
 * \param ifindex [IN]  The interface index for which to get the status.
 * \param status  [OUT] The interface status.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_interface_status_get(vtss_ifindex_t ifindex, vtss_appl_qos_if_status_t *status);

/****************************************************************************
 * QoS TAG-COS Configuration
 ****************************************************************************/

/*! \brief QoS PCP,DEI to CoS,DPL configuration. */
typedef struct {
    mesa_prio_t     cos; /*!< Mapping from PCP and DEI to CoS. */
    mesa_dp_level_t dpl; /*!< Mapping from PCP and DEI to DPL. */
} vtss_appl_qos_tag_cos_entry_t;

/*!
 * \brief Iterator for retrieving QoS tag-cos configuration key/index
 *
 * \param prev_ifindex [IN]    Interface index to be used for indexing determination.
 * \param prev_pcp     [IN]    PCP to be used for indexing determination.
 * \param prev_dei     [IN]    DEI to be used for indexing determination.
 *
 * \param next_ifindex [OUT]   The key/index of Interface index should be used for the GET operation.
 * \param next_pcp     [OUT]   The key/index of PCP should be used for the GET operation.
 * \param next_dei     [OUT]   The key/index of DEI should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_tag_cos_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                  const mesa_tagprio_t *prev_pcp,     mesa_tagprio_t *next_pcp,
                                  const mesa_dei_t     *prev_dei,     mesa_dei_t     *next_dei);

/*!
 * \brief Get QoS tag-cos configuration.
 *
 * \param ifindex [IN]  The interface index for which to get the configuration.
 * \param pcp     [IN]  The PCP for which to get the configuration.
 * \param dei     [IN]  The DEI for which to get the configuration.
 * \param conf    [OUT] The tag-cos configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_tag_cos_get(vtss_ifindex_t ifindex, mesa_tagprio_t pcp, mesa_dei_t dei, vtss_appl_qos_tag_cos_entry_t *conf);

/*!
 * \brief Set QoS tag-cos configuration.
 *
 * \param ifindex [IN] The interface index for which to set the configuration.
 * \param pcp     [IN] The PCP for which to set the configuration.
 * \param dei     [IN] The DEI for which to set the configuration.
 * \param conf    [IN] The tag-cos configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_tag_cos_set(vtss_ifindex_t ifindex, mesa_tagprio_t pcp, mesa_dei_t dei, const vtss_appl_qos_tag_cos_entry_t *conf);

/****************************************************************************
 * QoS COS-TAG Configuration
 ****************************************************************************/

/*! \brief QoS CoS,DPL to PCP,DEI configuration. */
typedef struct {
    mesa_tagprio_t  pcp; /*!< Mapping from CoS and DPL to PCP. */
    mesa_dei_t      dei; /*!< Mapping from CoS and DPL to DEI. */
} vtss_appl_qos_cos_tag_entry_t;

/*!
 * \brief Iterator for retrieving QoS cos-tag configuration key/index
 *
 * \param prev_ifindex [IN]    Interface index to be used for indexing determination.
 * \param prev_cos     [IN]    CoS to be used for indexing determination.
 * \param prev_dpl     [IN]    DPL to be used for indexing determination.
 *
 * \param next_ifindex [OUT]   The key/index of Interface index should be used for the GET operation.
 * \param next_cos     [OUT]   The key/index of CoS should be used for the GET operation.
 * \param next_dpl     [OUT]   The key/index of DPL should be used for the GET operation.
 *                             When IN is NULL, assign the first index.
 *                             When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_cos_tag_itr(const vtss_ifindex_t  *prev_ifindex, vtss_ifindex_t  *next_ifindex,
                                  const mesa_prio_t     *prev_cos,     mesa_prio_t     *next_cos,
                                  const mesa_dp_level_t *prev_dpl,     mesa_dp_level_t *next_dpl);

/*!
 * \brief Get QoS cos-tag configuration.
 *
 * \param ifindex [IN]  The interface index for which to get the configuration.
 * \param cos     [IN]  The CoS for which to get the configuration.
 * \param dpl     [IN]  The DPL for which to get the configuration.
 * \param conf    [OUT] The cos-tag configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_cos_tag_get(vtss_ifindex_t ifindex, mesa_prio_t cos, mesa_dp_level_t dpl, vtss_appl_qos_cos_tag_entry_t *conf);

/*!
 * \brief Set QoS cos-tag configuration.
 *
 * \param ifindex [IN] The interface index for which to set the configuration.
 * \param cos     [IN] The CoS for which to set the configuration.
 * \param dpl     [IN] The DPL for which to set the configuration.
 * \param conf    [IN] The cos-tag configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_cos_tag_set(vtss_ifindex_t ifindex, mesa_prio_t cos, mesa_dp_level_t dpl, const vtss_appl_qos_cos_tag_entry_t *conf);

/****************************************************************************
 * QoS Port Policer Configuration
 ****************************************************************************/

/*! \brief QoS port policer configuration. */
typedef struct {
    mesa_bool_t        enable;       /*!< Enable policer. */
    mesa_bool_t        frame_rate;   /*!< Measure rate in fps instead of kbps. */
    mesa_bool_t        flow_control; /*!< Enable flow control.
                                       Only valid if capabilities.has_port_policers_fc is TRUE. */
    mesa_bitrate_t     cir;          /*!< CIR (Committed Information Rate). Unit: kbps (or fps if frame_rate == TRUE). */
    mesa_burst_level_t cbs;          /*!< CBS (Committed Burst Size).       Unit: bytes. */
} vtss_appl_qos_port_policer_t;

/*!
 * \brief Iterator for retrieving port policer configuration
 *
 * \param prev [IN]  Previous interface index. May be given as a NULL,
 *                   in which case the first interface will be returned.
 * \param next [OUT] The next valid interface numerically larger than the input.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_policer_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next);

/*!
 * \brief Get QoS port policer configuration.
 *
 * \param ifindex [IN]  The interface index for which to get the configuration.
 * \param conf    [OUT] The port policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_policer_get(vtss_ifindex_t ifindex, vtss_appl_qos_port_policer_t *conf);

/*!
 * \brief Set QoS port policer configuration.
 *
 * \param ifindex [IN] The interface index for which to set the configuration.
 * \param conf    [IN] The port policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_policer_set(vtss_ifindex_t ifindex, const vtss_appl_qos_port_policer_t *conf);

/****************************************************************************
 * QoS Queue Policer Configuration
 ****************************************************************************/

/*! \brief QoS queue policer configuration. */
typedef struct {
    mesa_bool_t        enable; /*!< Enable policer. */
    mesa_bitrate_t     cir;    /*!< CIR (Committed Information Rate). Unit: kbps. */
    mesa_burst_level_t cbs;    /*!< CBS (Committed Burst Size).       Unit: bytes. */
} vtss_appl_qos_queue_policer_t;

/*!
 * \brief Iterator for retrieving QoS queue policer configuration by ifindex,queue
 *
 * \param prev_ifindex [IN]    Interface index to be used for indexing determination.
 * \param prev_queue   [IN]    Queue to be used for indexing determination.
 *
 * \param next_ifindex [OUT]   The key/index of Interface index should be used for the GET operation.
 * \param next_queue   [OUT]   The key/index of Queue should be used for the GET operation.
 *                             When IN is NULL, assign the first index.
 *                             When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_queue_policer_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                        const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue);

/*!
 * \brief Get QoS queue policer configuration.
 *
 * Only implemented if capabilities.has_port_queue_policers is TRUE
 *
 * \param ifindex [IN]  The interface index for which to get the configuration.
 * \param queue   [IN]  The queue for which to get the configuration.
 * \param conf    [OUT] The queue policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_queue_policer_get(vtss_ifindex_t ifindex, mesa_prio_t queue, vtss_appl_qos_queue_policer_t *conf);

/*!
 * \brief Set QoS queue policer configuration.
 *
 * Only implemented if capabilities.has_port_queue_policers is TRUE
 *
 * \param ifindex [IN] The interface index for which to set the configuration.
 * \param queue   [IN] The queue for which to set the configuration.
 * \param conf    [IN] The queue policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_queue_policer_set(vtss_ifindex_t ifindex, mesa_prio_t queue, const vtss_appl_qos_queue_policer_t *conf);

/****************************************************************************
 * QoS Port Storm Policer Configuration
 ****************************************************************************/

/*! \brief QoS port storm policer configuration. */
typedef struct {
    mesa_bool_t        enable;     /*!< Enable policer. */
    mesa_bool_t        frame_rate; /*!< Measure rate in fps instead of kbps. */
    mesa_bitrate_t     cir;        /*!< CIR (Committed Information Rate). Unit: kbps (or fps if frame == TRUE). */
    mesa_burst_level_t cbs;        /*!< CBS (Committed Burst Size).       Unit: bytes. */
} vtss_appl_qos_port_storm_policer_t;

/*!
 * \brief Iterator for retrieving port storm policer configuration
 *
 * \param prev [IN]  Previous interface index. May be given as a NULL,
 *                   in which case the first interface will be returned.
 * \param next [OUT] The next valid interface numerically larger than the input.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_storm_policer_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next);

/*!
 * \brief Get QoS port unicast storm policer configuration.
 *
 * Only implemented if capabilities.has_port_storm_policers is TRUE
 *
 * \param ifindex [IN]  The interface index for which to get the configuration.
 * \param conf    [OUT] The port unicast storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_uc_policer_get(vtss_ifindex_t ifindex, vtss_appl_qos_port_storm_policer_t *conf);

/*!
 * \brief Set QoS port unicast storm policer configuration.
 *
 * Only implemented if capabilities.has_port_storm_policers is TRUE
 *
 * \param ifindex [IN] The interface index for which to set the configuration.
 * \param conf    [IN] The port unicast storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_uc_policer_set(vtss_ifindex_t ifindex, const vtss_appl_qos_port_storm_policer_t *conf);

/*!
 * \brief Get QoS port broadcast storm policer configuration.
 *
 * Only implemented if capabilities.has_port_storm_policers is TRUE
 *
 * \param ifindex [IN]  The interface index for which to get the configuration.
 * \param conf    [OUT] The port broadcast storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_bc_policer_get(vtss_ifindex_t ifindex, vtss_appl_qos_port_storm_policer_t *conf);

/*!
 * \brief Set QoS port broadcast storm policer configuration.
 *
 * Only implemented if capabilities.has_port_storm_policers is TRUE
 *
 * \param ifindex [IN] The interface index for which to set the configuration.
 * \param conf    [IN] The port broadcast storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_bc_policer_set(vtss_ifindex_t ifindex, const vtss_appl_qos_port_storm_policer_t *conf);

/*!
 * \brief Get QoS port unknown (flooded) storm policer configuration.
 *
 * Only implemented if capabilities.has_port_storm_policers is TRUE
 *
 * \param ifindex [IN]  The interface index for which to get the configuration.
 * \param conf    [OUT] The port unknown (flooded) storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_un_policer_get(vtss_ifindex_t ifindex, vtss_appl_qos_port_storm_policer_t *conf);

/*!
 * \brief Set QoS port unknown (flooded) storm policer configuration.
 *
 * Only implemented if capabilities.has_port_storm_policers is TRUE
 *
 * \param ifindex [IN] The interface index for which to set the configuration.
 * \param conf    [IN] The port unknown (flooded) storm policer configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_un_policer_set(vtss_ifindex_t ifindex, const vtss_appl_qos_port_storm_policer_t *conf);

/****************************************************************************
 * QoS Port Shaper Configuration
 ****************************************************************************/

/*!
 * Enumeration of Shaper Accounting modes used in the shaper configuration
 */
typedef enum {
    VTSS_APPL_QOS_SHAPER_MODE_LINE, /*!< Shaper line-rate. */
    VTSS_APPL_QOS_SHAPER_MODE_DATA, /*!< Shaper date-rate. */
    VTSS_APPL_QOS_SHAPER_MODE_FRAME /*!< Shaper frame-rate. */
} vtss_appl_qos_shaper_mode_t;

/*! \brief QoS port shaper configuration. */
typedef struct {
    mesa_bool_t                 enable; /*!< Enable shaper. */
    vtss_appl_qos_shaper_mode_t mode;   /*!< Shaper mode. Line or data rate.
                                          Only valid if capabilities.has_shapers_rt is TRUE. */
    mesa_bitrate_t              cir;    /*!< CIR (Committed Information Rate). Unit: kbps. */
    mesa_burst_level_t          cbs;    /*!< CBS (Committed Burst Size).       Unit: bytes. */
    mesa_bool_t                 dlb;    /*!< Enable dual leaky bucket and use EIR and EBS.
                                          Only valid if capabilities.has_port_shapers_dlb is TRUE. */
    mesa_bitrate_t              eir;    /*!< EIR (Excess Information Rate).    Unit: kbps.
                                          Only valid if capabilities.has_port_shapers_dlb is TRUE. */
    mesa_burst_level_t          ebs;    /*!< EBS (Excess Burst Size).          Unit: bytes.
                                          Only valid if capabilities.has_port_shapers_dlb is TRUE. */
} vtss_appl_qos_port_shaper_t;

/*!
 * \brief Iterator for retrieving port shaper configuration
 *
 * \param prev [IN]  Previous interface index. May be given as a NULL,
 *                   in which case the first interface will be returned.
 * \param next [OUT] The next valid interface numerically larger than the input.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_shaper_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next);

/*!
 * \brief Get QoS port shaper configuration.
 *
 * \param ifindex [IN]  The interface index for which to get the configuration.
 * \param conf    [OUT] The port shaper configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_shaper_get(vtss_ifindex_t ifindex, vtss_appl_qos_port_shaper_t *conf);

/*!
 * \brief Set QoS port shaper configuration.
 *
 * \param ifindex [IN] The interface index for which to set the configuration.
 * \param conf    [IN] The port shaper configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_shaper_set(vtss_ifindex_t ifindex, const vtss_appl_qos_port_shaper_t *conf);

/****************************************************************************
 * QoS Queue Shaper Configuration
 ****************************************************************************/

/*! \brief QoS queue shaper configuration. */
typedef struct {
    mesa_bool_t                 enable; /*!< Enable shaper. */
    vtss_appl_qos_shaper_mode_t mode;   /*!< Shaper mode. Line, data or frame rate.
                                          Only valid if capabilities.has_shapers_rt is TRUE. */
    mesa_bool_t                 excess; /*!< Allow this queue to use excess bandwidth.
                                          Only valid if capabilities.has_queue_shapers_eb is TRUE. */
    mesa_bool_t                 credit; /*!< Enable the credit based shaper.
                                          Only valid if capabilities.has_queue_shapers_crb is TRUE. */
    mesa_bitrate_t              cir;    /*!< CIR (Committed Information Rate). Unit: kbps.  */
    mesa_burst_level_t          cbs;    /*!< CBS (Committed Burst Size).       Unit: bytes. */
    mesa_bool_t                 dlb;    /*!< Enable dual leaky bucket and use EIR and EBS.
                                          Only valid if capabilities.has_queue_shapers_dlb is TRUE. */
    mesa_bitrate_t              eir;    /*!< EIR (Excess Information Rate).    Unit: kbps.
                                          Only valid if capabilities.has_queue_shapers_dlb is TRUE. */
    mesa_burst_level_t          ebs;    /*!< EBS (Excess Burst Size).          Unit: bytes.
                                          Only valid if capabilities.has_queue_shapers_dlb is TRUE. */
} vtss_appl_qos_queue_shaper_t;

/*!
 * \brief Iterator for retrieving QoS queue shaper configuration by ifindex,queue
 *
 * \param prev_ifindex [IN]    Interface index to be used for indexing determination.
 * \param prev_queue   [IN]    Queue to be used for indexing determination.
 *
 * \param next_ifindex [OUT]   The key/index of Interface index should be used for the GET operation.
 * \param next_queue   [OUT]   The key/index of Queue should be used for the GET operation.
 *                             When IN is NULL, assign the first index.
 *                             When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_queue_shaper_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                       const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue);

/*!
 * \brief Get QoS queue shaper configuration.
 *
 * \param ifindex [IN]  The interface index for which to get the configuration.
 * \param queue   [IN]  The queue for which to get the configuration.
 * \param conf    [OUT] The queue shaper configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_queue_shaper_get(vtss_ifindex_t ifindex, mesa_prio_t queue, vtss_appl_qos_queue_shaper_t *conf);

/*!
 * \brief Set QoS queue shaper configuration.
 *
 * \param ifindex [IN] The interface index for which to set the configuration.
 * \param queue   [IN] The queue for which to set the configuration.
 * \param conf    [IN] The queue shaper configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_queue_shaper_set(vtss_ifindex_t ifindex, mesa_prio_t queue, const vtss_appl_qos_queue_shaper_t *conf);

/****************************************************************************
 * QoS Scheduler Configuration
 ****************************************************************************/

/*! \brief QoS scheduler configuration. */
typedef struct {
    mesa_pct_t  weight;           /*!< Queue weight. */
    mesa_bool_t cut_through;      /*!< Enable the cut through feature.
                                    Only valid if capabilities.has_queue_cut_through is TRUE. */
    mesa_bool_t frame_preemption; /*!< Enable the frame preemption feature.
                                    Only valid if capabilities.has_queue_frame_preemption is TRUE. */
} vtss_appl_qos_scheduler_t;

/*!
 * \brief Iterator for retrieving QoS scheduler configuration key/index
 *
 * \param prev_ifindex [IN]    Interface index to be used for indexing determination.
 * \param prev_queue   [IN]    Queue to be used for indexing determination.
 *
 * \param next_ifindex [OUT]   The key/index of Interface index should be used for the GET operation.
 * \param next_queue   [OUT]   The key/index of Queue should be used for the GET operation.
 *                             When IN is NULL, assign the first index.
 *                             When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_scheduler_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                    const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue);

/*!
 * \brief Get QoS scheduler configuration.
 *
 * \param ifindex [IN]  The interface index for which to get the configuration.
 * \param queue   [IN]  The queue for which to get the configuration.
 * \param conf    [OUT] The scheduler configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_scheduler_get(vtss_ifindex_t ifindex, mesa_prio_t queue, vtss_appl_qos_scheduler_t *conf);

/*!
 * \brief Set QoS scheduler configuration.
 *
 * \param ifindex [IN] The interface index for which to set the configuration.
 * \param queue   [IN] The queue for which to set the configuration.
 * \param conf    [IN] The scheduler configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_scheduler_set(vtss_ifindex_t ifindex, mesa_prio_t queue, const vtss_appl_qos_scheduler_t *conf);

/****************************************************************************
 * QoS Scheduler Status
 ****************************************************************************/

/*! \brief QoS scheduler status. */
typedef struct {
    mesa_pct_t weight; /*!< Queue weight. */
} vtss_appl_qos_scheduler_status_t;

/*!
 * \brief Iterator for retrieving QoS scheduler status key/index
 *
 * \param prev_ifindex [IN]    Interface index to be used for indexing determination.
 * \param prev_queue   [IN]    Queue to be used for indexing determination.
 *
 * \param next_ifindex [OUT]   The key/index of Interface index should be used for the GET operation.
 * \param next_queue   [OUT]   The key/index of Queue should be used for the GET operation.
 *                             When IN is NULL, assign the first index.
 *                             When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_scheduler_status_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                           const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue);

/*!
 * \brief Get QoS scheduler status.
 *
 * \param ifindex [IN]  The interface index for which to get the status.
 * \param queue   [IN]  The queue for which to get the status.
 * \param status  [OUT] The scheduler status.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_scheduler_status_get(vtss_ifindex_t ifindex, mesa_prio_t queue, vtss_appl_qos_scheduler_status_t *status);

/****************************************************************************
 * QoS Ingress Map Configuration
 ****************************************************************************/
/*! \brief Key that determines what to match */
typedef enum {
    VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP,             /*!< Use PCP for tagged frames and none for the rest */
    VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP_DEI,         /*!< Use PCP/DEI for tagged frames and none for the rest */
    VTSS_APPL_QOS_INGRESS_MAP_KEY_DSCP,            /*!< Use DSCP as key for IP frames and none for the rest */
    VTSS_APPL_QOS_INGRESS_MAP_KEY_DSCP_PCP_DEI,    /*!< Use DSCP as key for IP frames, PCP/DEI for tagged frames and none for the rest */
} vtss_appl_qos_ingress_map_key_t;

/*! \brief Actions that can be applied to classified values it the entry is matched */
typedef struct {
    mesa_bool_t cos;              /*!< If TRUE, then replace the classified COS */
    mesa_bool_t dpl;              /*!< If TRUE, then replace the classified DPL */
    mesa_bool_t pcp;              /*!< If TRUE, then replace the classified PCP */
    mesa_bool_t dei;              /*!< If TRUE, then replace the classified DEI */
    mesa_bool_t dscp;             /*!< If TRUE, then replace the classified DSCP */
    mesa_bool_t cosid;            /*!< If TRUE, then replace the classified COS ID */
} vtss_appl_qos_ingress_map_action_t;

/*! \brief Key/action configuration. There is one of these per map id */
typedef struct {
    vtss_appl_qos_ingress_map_key_t    key;    /*!< Lookup key */
    vtss_appl_qos_ingress_map_action_t action; /*!< Action enable/disable */
} vtss_appl_qos_ingress_map_conf_t;

/*! \brief Mapped values that can be applied to classified values it the entry is matched */
typedef struct {
    mesa_cos_t     cos;        /*!< The classified COS is set to cos if action.cos is TRUE */
    mesa_dpl_t     dpl;        /*!< The classified DPL is set to dpl if action.dpl is TRUE */
    mesa_pcp_t     pcp;        /*!< The classified PCP is set to pcp if action.pcp is TRUE */
    mesa_dei_t     dei;        /*!< The classified DEI is set to dei if action.dei is TRUE */
    mesa_dscp_t    dscp;       /*!< The classified DSCP is set to dscp if action.dscp is TRUE */
    mesa_cosid_t   cosid;      /*!< The classified COS ID is set to cosid if action.cosid is TRUE */
} vtss_appl_qos_ingress_map_values_t;

/*!
 * \brief Preset an ingress map configuration to default mappings.
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param id          [IN]  Map ID.
 * \param classes     [IN]  Number of traffic classes.
 * \param color_aware [IN]  If TRUE, then mappings are color aware.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_preset(mesa_qos_ingress_map_id_t id,
                                         uint8_t                   classes,
                                         mesa_bool_t               color_aware);

/*!
 * \brief Iterator for retrieving QoS ingress map configuration in ID order
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param prev_id [IN]    Map Id to be used for indexing determination.
 * \param next_id [OUT]   The key/index of Map Id should be used for the GET operation.
 *                        When IN is NULL, assign the first index.
 *                        When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_conf_itr(const mesa_qos_ingress_map_id_t *prev_id, mesa_qos_ingress_map_id_t *next_id);

/*!
 * \brief Add an ingress map configuration.
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param id   [IN]  Map ID.
 * \param conf [IN]  Ingress map configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_conf_add(mesa_qos_ingress_map_id_t              id,
                                           const vtss_appl_qos_ingress_map_conf_t *conf);

/*!
 * \brief Delete an ingress map configuration.
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param id   [IN]  Map ID.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_conf_del(const mesa_qos_ingress_map_id_t id);

/*!
 * \brief Get an ingress map configuration.
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param id   [IN]  Map ID.
 * \param conf [OUT] Ingress map structure.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_conf_get(mesa_qos_ingress_map_id_t        id,
                                           vtss_appl_qos_ingress_map_conf_t *conf);

/*!
 * \brief Set an ingress map configuration.
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param id   [IN]  Map ID.
 * \param conf [IN]  Ingress map structure.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_conf_set(mesa_qos_ingress_map_id_t              id,
                                           const vtss_appl_qos_ingress_map_conf_t *conf);

/*!
 * \brief Iterator for retrieving QoS ingress map PCP/DEI configuration key/index
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param prev_id  [IN]    Map Id to be used for indexing determination.
 * \param prev_pcp [IN]    PCP to be used for indexing determination.
 * \param prev_dei [IN]    DEI to be used for indexing determination.
 *
 * \param next_id  [OUT]   Map Id of Interface index should be used for the GET operation.
 * \param next_pcp [OUT]   The key/index of PCP should be used for the GET operation.
 * \param next_dei [OUT]   The key/index of DEI should be used for the GET operation.
 *                         When IN is NULL, assign the first index.
 *                         When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_pcp_dei_conf_itr(const mesa_qos_ingress_map_id_t *prev_id,  mesa_qos_ingress_map_id_t *next_id,
                                                   const mesa_tagprio_t            *prev_pcp, mesa_tagprio_t            *next_pcp,
                                                   const mesa_dei_t                *prev_dei, mesa_dei_t                *next_dei);

/*!
 * \brief Get an ingress map PCP/DEI configuration.
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param id   [IN]  The Map ID for which to get the configuration.
 * \param pcp  [IN]  The PCP for which to get the configuration.
 * \param dei  [IN]  The DEI for which to get the configuration.
 * \param conf [OUT] The PCP/DEI configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_pcp_dei_conf_get(mesa_qos_ingress_map_id_t          id,
                                                   mesa_tagprio_t                     pcp,
                                                   mesa_dei_t                         dei,
                                                   vtss_appl_qos_ingress_map_values_t *conf);

/*!
 * \brief Set an ingress map PCP/DEI configuration.
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param id   [IN]  The Map ID for which to set the configuration.
 * \param pcp  [IN]  The PCP for which to set the configuration.
 * \param dei  [IN]  The DEI for which to set the configuration.
 * \param conf [IN]  The PCP/DEI configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_pcp_dei_conf_set(mesa_qos_ingress_map_id_t                id,
                                                   mesa_tagprio_t                           pcp,
                                                   mesa_dei_t                               dei,
                                                   const vtss_appl_qos_ingress_map_values_t *conf);

/*!
 * \brief Iterator for retrieving QoS ingress map DSCP configuration key/index
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param prev_id   [IN]    Map Id to be used for indexing determination.
 * \param prev_dscp [IN]    DSCP to be used for indexing determination.
 *
 * \param next_id   [OUT]   Map Id of Interface index should be used for the GET operation.
 * \param next_dscp [OUT]   The key/index of DSCP should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_dscp_conf_itr(const mesa_qos_ingress_map_id_t *prev_id,   mesa_qos_ingress_map_id_t *next_id,
                                                const mesa_dscp_t               *prev_dscp, mesa_dscp_t               *next_dscp);

/*!
 * \brief Get an ingress map DSCP configuration.
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param id   [IN]  The Map ID for which to get the configuration.
 * \param dscp [IN]  The DSCP for which to get the configuration.
 * \param conf [OUT] The DSCP configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_dscp_conf_get(mesa_qos_ingress_map_id_t          id,
                                                mesa_dscp_t                        dscp,
                                                vtss_appl_qos_ingress_map_values_t *conf);

/*!
 * \brief Set an ingress map DSCP configuration.
 *
 * Only implemented if capabilities.has_ingress_map is TRUE
 *
 * \param id   [IN]  The Map ID for which to set the configuration.
 * \param dscp [IN]  The DSCP for which to set the configuration.
 * \param conf [IN]  The DSCP configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_ingress_map_dscp_conf_set(mesa_qos_ingress_map_id_t                id,
                                                mesa_dscp_t                              dscp,
                                                const vtss_appl_qos_ingress_map_values_t *conf);

/****************************************************************************
 * QoS Egress Map Configuration
 ****************************************************************************/
/*! \brief Key that determines what to match */
typedef enum {
    VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID,       /*!< Use classified COS ID */
    VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID_DPL,   /*!< Use classified COS ID and DPL */
    VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP,        /*!< Use classified DSCP  */
    VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP_DPL,    /*!< Use classified DSCP and DPL */
} vtss_appl_qos_egress_map_key_t;

/** \brief Actions that can be applied to the frame if the entry is matched */
typedef struct {
    mesa_bool_t pcp;     /*!< If TRUE, then replace PCP in frame */
    mesa_bool_t dei;     /*!< If TRUE, then replace DEI in frame */
    mesa_bool_t dscp;    /*!< If TRUE, then replace DSCP in frame */
} vtss_appl_qos_egress_map_action_t;

/*! \brief Key/action configuration. There is one of these per map id */
typedef struct {
    vtss_appl_qos_egress_map_key_t    key;    /*!< Lookup key */
    vtss_appl_qos_egress_map_action_t action; /*!< Action enable/disable */
} vtss_appl_qos_egress_map_conf_t;

/** \brief Mapped values that can be applied to the frame if the entry is matched */
typedef struct {
    mesa_pcp_t     pcp;        /*!< The frame PCP is set to pcp if action.pcp is TRUE */
    mesa_dei_t     dei;        /*!< The frame DEI is set to dei if action.dei is TRUE */
    mesa_dscp_t    dscp;       /*!< The frame DSCP is set to dscp if action.dscp is TRUE */
} vtss_appl_qos_egress_map_values_t;

/*!
 * \brief Preset an egress map configuration to default mappings.
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param id          [IN]  Map ID.
 * \param classes     [IN]  Number of traffic classes.
 * \param color_aware [IN]  If TRUE, then mappings are color aware.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_preset(mesa_qos_egress_map_id_t id,
                                        uint8_t                  classes,
                                        mesa_bool_t              color_aware);

/*!
 * \brief Iterator for retrieving QoS egress map configuration in ID order
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param prev_id [IN]    Map Id to be used for indexing determination.
 * \param next_id [OUT]   The key/index of Map Id should be used for the GET operation.
 *                        When IN is NULL, assign the first index.
 *                        When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_conf_itr(const mesa_qos_egress_map_id_t *prev_id, mesa_qos_egress_map_id_t *next_id);

/*!
 * \brief Add an egress map configuration.
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param id   [IN]  Map ID.
 * \param conf [IN]  Egress map configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_conf_add(mesa_qos_egress_map_id_t              id,
                                          const vtss_appl_qos_egress_map_conf_t *conf);

/*!
 * \brief Delete an egress map configuration.
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param id   [IN]  Map ID.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_conf_del(const mesa_qos_egress_map_id_t id);

/*!
 * \brief Get an egress map configuration.
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param id   [IN]  Map ID.
 * \param conf [OUT] Egress map structure.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_conf_get(mesa_qos_egress_map_id_t        id,
                                          vtss_appl_qos_egress_map_conf_t *conf);

/*!
 * \brief Set an egress map configuration.
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param id   [IN]  Map ID.
 * \param conf [IN]  Egress map structure.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_conf_set(mesa_qos_egress_map_id_t              id,
                                          const vtss_appl_qos_egress_map_conf_t *conf);

/*!
 * \brief Iterator for retrieving QoS egress map COSID/DPL configuration key/index
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param prev_id    [IN]    Map Id to be used for indexing determination.
 * \param prev_cosid [IN]    COSID to be used for indexing determination.
 * \param prev_dpl   [IN]    DPL to be used for indexing determination.
 *
 * \param next_id    [OUT]   Map Id of Interface index should be used for the GET operation.
 * \param next_cosid [OUT]   The key/index of COSID should be used for the GET operation.
 * \param next_dpl   [OUT]   The key/index of DPL should be used for the GET operation.
 *                           When IN is NULL, assign the first index.
 *                           When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_cosid_dpl_conf_itr(const mesa_qos_egress_map_id_t *prev_id,    mesa_qos_egress_map_id_t *next_id,
                                                    const mesa_cosid_t             *prev_cosid, mesa_cosid_t             *next_cosid,
                                                    const mesa_dpl_t               *prev_dpl,   mesa_dpl_t               *next_dpl);

/*!
 * \brief Get an egress map COSID/DPL configuration.
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param id    [IN]  The Map ID for which to get the configuration.
 * \param cosid [IN]  The COSID for which to get the configuration.
 * \param dpl   [IN]  The DPL for which to get the configuration.
 * \param conf  [OUT] The COSID/DPL configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_cosid_dpl_conf_get(mesa_qos_egress_map_id_t          id,
                                                    mesa_cosid_t                      cosid,
                                                    mesa_dpl_t                        dpl,
                                                    vtss_appl_qos_egress_map_values_t *conf);

/*!
 * \brief Set an egress map COSID/DPL configuration.
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param id    [IN]  The Map ID for which to set the configuration.
 * \param cosid [IN]  The COSID for which to set the configuration.
 * \param dpl   [IN]  The DPL for which to set the configuration.
 * \param conf  [IN]  The COSID/DPL configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_cosid_dpl_conf_set(mesa_qos_egress_map_id_t                id,
                                                    mesa_cosid_t                            cosid,
                                                    mesa_dpl_t                              dpl,
                                                    const vtss_appl_qos_egress_map_values_t *conf);

/*!
 * \brief Iterator for retrieving QoS egress map DSCP/DPL configuration key/index
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param prev_id   [IN]    Map Id to be used for indexing determination.
 * \param prev_dscp [IN]    DSCP to be used for indexing determination.
 * \param prev_dpl  [IN]    DPL to be used for indexing determination.
 *
 * \param next_id   [OUT]   Map Id of Interface index should be used for the GET operation.
 * \param next_dscp [OUT]   The key/index of DSCP should be used for the GET operation.
 * \param next_dpl  [OUT]   The key/index of DPL should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_dscp_dpl_conf_itr(const mesa_qos_egress_map_id_t *prev_id,   mesa_qos_egress_map_id_t *next_id,
                                                   const mesa_dscp_t              *prev_dscp, mesa_dscp_t              *next_dscp,
                                                   const mesa_dpl_t               *prev_dpl,  mesa_dpl_t               *next_dpl);

/*!
 * \brief Get an egress map DSCP/DPL configuration.
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param id   [IN]  The Map ID for which to get the configuration.
 * \param dscp [IN]  The DSCP for which to get the configuration.
 * \param dpl  [IN]  The DPL for which to set the configuration.
 * \param conf [OUT] The DSCP/DPL configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_dscp_dpl_conf_get(mesa_qos_egress_map_id_t          id,
                                                   mesa_dscp_t                       dscp,
                                                   mesa_dpl_t                        dpl,
                                                   vtss_appl_qos_egress_map_values_t *conf);

/*!
 * \brief Set an egress map DSCP/DPL configuration.
 *
 * Only implemented if capabilities.has_egress_map is TRUE
 *
 * \param id   [IN]  The Map ID for which to set the configuration.
 * \param dscp [IN]  The DSCP for which to set the configuration.
 * \param dpl  [IN]  The DPL for which to set the configuration.
 * \param conf [IN]  The DSCP/DPL configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_egress_map_dscp_dpl_conf_set(mesa_qos_egress_map_id_t                id,
                                                   mesa_dscp_t                             dscp,
                                                   mesa_dpl_t                              dpl,
                                                   const vtss_appl_qos_egress_map_values_t *conf);

/****************************************************************************
 * QoS QCE Configuration, Status and Control
 ****************************************************************************/

/*! \brief QoS QCL users declaration. */
typedef enum {
    VTSS_APPL_QOS_QCL_USER_STATIC,     /*!< Static configuration. */
    VTSS_APPL_QOS_QCL_USER_VOICE_VLAN, /*!< Dynamic configuration made by voice_vlan module. */
    VTSS_APPL_QOS_QCL_USER_DHCP6_SNOOP, /*!< DHCPv6 snooping module. */
    VTSS_APPL_QOS_QCL_USER_IPV6_SOURCE_GUARD,   /*!< Ipv6 source guard module. */
    VTSS_APPL_QOS_QCL_USER_CNT         /*!< Counting number of entries in enum. */
} vtss_appl_qos_qcl_user_t;

/*! \brief QoS Control Entry type. */
typedef enum {
    VTSS_APPL_QOS_QCE_TYPE_ANY,   /*!< Any frame type. */
    VTSS_APPL_QOS_QCE_TYPE_ETYPE, /*!< Ethernet Type. */
    VTSS_APPL_QOS_QCE_TYPE_LLC,   /*!< LLC. */
    VTSS_APPL_QOS_QCE_TYPE_SNAP,  /*!< SNAP. */
    VTSS_APPL_QOS_QCE_TYPE_IPV4,  /*!< IPv4. */
    VTSS_APPL_QOS_QCE_TYPE_IPV6   /*!< IPv6. */
} vtss_appl_qos_qce_type_t;

/*! \brief VCAP MAC key information. */
typedef struct {
    vtss_appl_vcap_dmac_type_t dmac_type; /*!< DMAC match type. */
    vtss_appl_vcap_mac_t       dmac;      /*!< DMAC.
                                            Only valid if capabilities.has_qce_dmac is TRUE. */
    vtss_appl_vcap_mac_t       smac;      /*!< SMAC.
                                            If capabilities.has_qce_mac_oui is TRUE then only the OUI part of the MAC address (24 most significant bits) is used.*/
} vtss_appl_qos_qce_mac_t;

/*! \brief VCAP tag key information. */
typedef struct {
    vtss_appl_vcap_vlan_tag_type_t tag_type; /*!< Tag match type. */
    vtss_appl_vcap_asr_t           vid;      /*!< VLAN ID. */
    vtss_appl_vcap_vlan_pri_type_t pcp;      /*!< PCP. */
    mesa_vcap_bit_t                dei;      /*!< DEI. */
} vtss_appl_qos_qce_tag_t;

/*! \brief Frame fields for MESA_QCE_TYPE_ETYPE. */
typedef struct {
    uint16_t etype; /*!< EtherType value - 0 is any. */
} vtss_appl_qos_qce_frame_etype_t;

/*! \brief Frame fields for MESA_QCE_TYPE_LLC. */
typedef struct {
    mesa_vcap_u8_t dsap;    /*!< DSAP. */
    mesa_vcap_u8_t ssap;    /*!< SSAP. */
    mesa_vcap_u8_t control; /*!< LLC Control. */
} vtss_appl_qos_qce_frame_llc_t;

/*! \brief Frame fields for MESA_QCE_TYPE_SNAP. */
typedef struct {
    vtss_appl_vcap_uint16_t pid; /*!< Protocol ID -> EtherType value. */
} vtss_appl_qos_qce_frame_snap_t;

/*! \brief Frame fields for MESA_QCE_TYPE_IPV4. */
typedef struct {
    mesa_vcap_bit_t      fragment; /*!< Fragment. */
    vtss_appl_vcap_asr_t dscp;     /*!< DSCP field (6 bit). */
    mesa_vcap_u8_t       proto;    /*!< IP protocol. */
    mesa_vcap_ip_t       sip;      /*!< Source IP address - Serval: key_type = normal, ip_addr and mac_ip_addr. */
    mesa_vcap_ip_t       dip;      /*!< Destination IP address - Serval: key_type = ip_addr and mac_ip_addr.
                                     Only valid if capabilities.has_qce_dmac is TRUE. */
    vtss_appl_vcap_asr_t sport;    /*!< UDP/TCP: Source port. */
    vtss_appl_vcap_asr_t dport;    /*!< UDP/TCP: Destination port. */
} vtss_appl_qos_qce_frame_ipv4_t;

/*! \brief Frame fields for MESA_QCE_TYPE_IPV6. */
typedef struct {
    vtss_appl_vcap_asr_t  dscp;     /*!< DSCP field (6 bit). */
    mesa_vcap_u8_t        proto;    /*!< IP protocol. */
    vtss_appl_vcap_ipv6_t sip;      /*!< Source IP address (32 LSB on L26 and J1, 64 LSB on Serval when key_type = mac_ip_addr). */
    vtss_appl_vcap_ipv6_t dip;      /*!< Destination IP address - 64 LSB on Serval when key_type = mac_ip_addr.
                                     Only valid if capabilities.has_qce_dmac is TRUE. */
    vtss_appl_vcap_asr_t  sport;    /*!< UDP/TCP: Source port. */
    vtss_appl_vcap_asr_t  dport;    /*!< UDP/TCP: Destination port. */
} vtss_appl_qos_qce_frame_ipv6_t;

/*! \brief QoS QCE frame fields. */
typedef struct {
    vtss_appl_qos_qce_frame_etype_t etype; /*!< MESA_QCE_TYPE_ETYPE. */
    vtss_appl_qos_qce_frame_llc_t   llc;   /*!< MESA_QCE_TYPE_LLC. */
    vtss_appl_qos_qce_frame_snap_t  snap;  /*!< MESA_QCE_TYPE_SNAP. */
    vtss_appl_qos_qce_frame_ipv4_t  ipv4;  /*!< MESA_QCE_TYPE_IPV4. */
    vtss_appl_qos_qce_frame_ipv6_t  ipv6;  /*!< MESA_QCE_TYPE_IPV6. */
} vtss_appl_qos_qce_frame_t;

/*! \brief QoS QCE key. */
typedef struct {
    vtss_port_list_stackable_t port_list;  /*!< Port list. */
    vtss_appl_qos_qce_mac_t    mac;        /*!< MAC. */
    vtss_appl_qos_qce_tag_t    tag;        /*!< Tag. */
    vtss_appl_qos_qce_tag_t    inner_tag;  /*!< Inner tag.
                                             Only valid if capabilities.has_qce_inner_tag is TRUE. */
    vtss_appl_qos_qce_type_t   type;       /*!< Frame type. */
    vtss_appl_qos_qce_frame_t  frame;      /*!< Frame type specific data. */
} vtss_appl_qos_qce_key_t;

/*! \brief QoS QCE action. */
typedef struct {
    mesa_bool_t               prio_enable;      /*!< Enable priority classification. */
    mesa_prio_t               prio;             /*!< Priority value. */
    mesa_bool_t               dp_enable;        /*!< Enable DP classification. */
    mesa_dp_level_t           dp;               /*!< DP value. */
    mesa_bool_t               dscp_enable;      /*!< Enable DSCP classification. */
    mesa_dscp_t               dscp;             /*!< DSCP value. */
    mesa_bool_t               pcp_dei_enable;   /*!< Enable PCP and DEI classification.
                                                  Only valid if capabilities.has_qce_action_pcp_dei is TRUE. */
    mesa_tagprio_t            pcp;              /*!< PCP value.
                                                  Only valid if capabilities.has_qce_action_pcp_dei is TRUE. */
    mesa_dei_t                dei;              /*!< DEI value.
                                                  Only valid if capabilities.has_qce_action_pcp_dei is TRUE. */
    mesa_bool_t               policy_no_enable; /*!< Enable ACL policy classification.
                                                  Only valid if capabilities.has_qce_action_policy is TRUE. */
    mesa_acl_policy_no_t      policy_no;        /*!< ACL policy number.
                                                  Only valid if capabilities.has_qce_action_policy is TRUE. */
    mesa_bool_t               map_id_enable;    /*!< Enable classification via ingress map.
                                                  Only valid if capabilities.has_qce_action_map is TRUE. */
    mesa_qos_ingress_map_id_t map_id;           /*!< Ingress map to use for classification.
                                                  Only valid if capabilities.has_qce_action_map is TRUE. */
} vtss_appl_qos_qce_action_t;

/*! \brief QoS QCE conf. */
typedef struct {
    mesa_qce_id_t              qce_id;      /*!< QCE id for this entry. */
    mesa_qce_id_t              next_qce_id; /*!< Id of next QCE in list. QCE_ID_NONE if this is (or must be) the last QCE in list. */
    vtss_usid_t                usid;        /*!< Switch ID. */
    vtss_appl_qos_qcl_user_t   user_id;     /*!< QCE user id. */
    vtss_appl_qos_qce_key_t    key;         /*!< QCE key. */
    vtss_appl_qos_qce_action_t action;      /*!< QCE action. */
    mesa_bool_t                conflict;    /*!< QCE conflict flag. */
} vtss_appl_qos_qce_conf_t;

/*!
 * \brief Iterator for retrieving QoS QCE configuration in ID order
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param prev_id [IN]    Qce Id to be used for indexing determination.
 * \param next_id [OUT]   The key/index of Qce Id should be used for the GET operation.
 *                        When IN is NULL, assign the first index.
 *                        When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_qce_conf_itr(const mesa_qce_id_t *const prev_id,
                                   mesa_qce_id_t       *const next_id);
/*!
 * \brief Set QCE.
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param qce_id [IN]  QCE ID.
 * \param conf   [IN]  The QCE to be updated.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_qce_conf_set(mesa_qce_id_t                  qce_id,
                                   const vtss_appl_qos_qce_conf_t *conf);

/*!
 * \brief Add QCE.
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param qce_id [IN]  QCE ID. The QCE will be added before conf->next_qce_id or last if conf->next_qce_id == QCE_ID_NONE.
 * \param conf   [IN]  The QCE to be added.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_qce_conf_add(mesa_qce_id_t                  qce_id,
                                   const vtss_appl_qos_qce_conf_t *conf);

/*!
 * \brief Delete QCE.
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param qce_id  [IN]  QCE ID to be deleted.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_qce_conf_del(mesa_qce_id_t qce_id);

/*!
 * \brief Get QCE.
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param qce_id [IN]  QCE ID.
 * \param conf   [OUT] The QCE.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_qce_conf_get(mesa_qce_id_t            qce_id,
                                   vtss_appl_qos_qce_conf_t *conf);

/*!
 * \brief Get default QCE
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param conf [OUT] The default QCE.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_qce_conf_get_default(vtss_appl_qos_qce_conf_t *conf);

/*!
 * \brief Iterator for retrieving QoS QCE precedence key/index
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param prev_index [IN]    Precedence Index to be used for indexing determination.
 * \param next_index [OUT]   The key/index of Precedence Index should be used for the GET operation.
 *                           When IN is NULL, assign the first index.
 *                           When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_qos_qce_precedence_itr(const uint32_t *prev_index, uint32_t *next_index);

/*!
 * \brief Get QoS QCE precedence.
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param index [IN]  The Precedece Index for which to get the configuration.
 * \param conf  [OUT] The QCE configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_qce_precedence_get(uint32_t index, vtss_appl_qos_qce_conf_t *conf);

/*!
 * \brief Iterator for retrieving QoS QCE status in switch/precedence order
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param prev_usid  [IN]    Switch Id to be used for indexing determination.
 * \param prev_index [IN]    Precedence Index to be used for indexing determination.
 *
 * \param next_usid  [OUT]   The key/index of Switch Id should be used for the GET operation (Stacking only).
 * \param next_index [OUT]   The key/index of Precedence Index should be used for the GET operation.
 *                           When IN is NULL, assign the first index.
 *                           When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Return code.
 */

mesa_rc vtss_appl_qos_qce_status_itr(const vtss_usid_t *prev_usid,  vtss_usid_t *next_usid,
                                     const uint32_t    *prev_index, uint32_t    *next_index);
/*!
 * \brief Get QoS QCE status.
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param usid    [IN]  The Switch Id for which to get the status.
 * \param index   [IN]  The Precedece Index for which to get the status.
 * \param status  [OUT] The QCE status.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_qce_status_get(vtss_usid_t usid, uint32_t index, vtss_appl_qos_qce_conf_t *status);

/*!
 * \brief Get QCE conflict resolve action.
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param conf    [OUT] Always false.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_qce_conflict_resolve_get(mesa_bool_t *conf);

/*!
 * \brief Set QCE conflict resolve action.
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \param conf    [IN] If true then initiate resolve conflict action.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_qce_conflict_resolve_set(const mesa_bool_t *conf);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_APPL_QOS_H_ */
