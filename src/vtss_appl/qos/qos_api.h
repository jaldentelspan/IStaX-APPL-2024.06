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

#ifndef _VTSS_QOS_APPL_API_H_
#define _VTSS_QOS_APPL_API_H_

#include "vtss/appl/qos.h"

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_SW_OPTION_BUILD_ISTAX)
#define VTSS_SW_OPTION_QOS_ADV /* Advanced QoS features */
#endif

/* Attention!!. Be aware that many of the following capability defines are used in                             */
/* vtss_appl/qos.cxx/vtss_appl_qos_capabilities_get() to initialise the vtss_appl_qos_capabilities_t stucture. */

/* QoS constants. */
#define VTSS_APPL_QOS_BITRATE_DEF         500  /*!< Default rate for policer/shaper (kbps) */
#define VTSS_APPL_QOS_BURSTSIZE_DEF  (4096*5)  /*!< Default burst size for policer/shaper (bytes) */

#define VTSS_APPL_QOS_CLASS_CNT             8  /*!< Maximum number of QoS classes */
#define VTSS_APPL_QOS_CLASS_MIN             0  /*!< Minimum value for QoS class */
#define VTSS_APPL_QOS_CLASS_MAX             7  /*!< Maximum value for QoS class */

#define VTSS_APPL_QOS_PORT_PRIO_CNT            MESA_PRIO_ARRAY_SIZE       /*!< Number of priorities per port */
#define VTSS_APPL_QOS_PORT_QUEUE_CNT           MESA_QUEUE_ARRAY_SIZE      /*!< Number of queues per port */

#define VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_CNT (MESA_QUEUE_ARRAY_SIZE - (fast_cap(MESA_CAP_QOS_SCHEDULER_CNT_DWRR) ? 0 : 2))     /*!< Number of weighted queues per port */
#define VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_MAX (VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_CNT - 1) /*!< Maximum weighted queue number */

/* Port storm policers require at least 4 port policers */
#define VTSS_APPL_QOS_PORT_STORM_POLICER (fast_cap(MESA_CAP_QOS_PORT_POLICER_CNT) >= 4 ? 1 : 0)

# ifdef VTSS_SW_OPTION_VOICE_VLAN
#  define VTSS_APPL_QOS_RESERVED_QCE_CNT  1             /*!< Reserved number of QCEs */
# else
#  define VTSS_APPL_QOS_RESERVED_QCE_CNT  0             /*!< Reserved number of QCEs */
# endif /* VTSS_SW_OPTION_VOICE_VLAN */

#define VTSS_APPL_QOS_QCE_MAX     CAPA->qce_id_max
#define VTSS_APPL_QOS_QCE_ID_NONE  0                                       /*!< Reserved */
#define VTSS_APPL_QOS_QCE_ID_START 1                                       /*!< First QCE ID */
#define VTSS_APPL_QOS_QCE_ID_END   (VTSS_ISID_CNT * VTSS_APPL_QOS_QCE_MAX) /*!< Last QCE ID */

# if defined(VTSS_SW_OPTION_QOS_QCL_INCLUDE)
#  if VTSS_SW_OPTION_QOS_QCL_INCLUDE == 1
#   define VTSS_APPL_QOS_QCL_INCLUDE                    /*!< Force inclusion of QCL functionality */
#  endif
# else
#  define VTSS_APPL_QOS_QCL_INCLUDE                     /*!< Use default inclusion of QCL functionality */
# endif /* defined(VTSS_SW_OPTION_QOS_QCL_INCLUDE) */

#define SECONDS_BETWEEN_STATUS_UPDATE 10               /* Determines how often the qos status structure is updated. */

/**
 * \brief Global read-only access to QoS capabilities.
 **/
extern const vtss_appl_qos_capabilities_t *const vtss_appl_qos_capabilities;

/**
 * \brief QoS error text from error code.
 *
 * \param rc [IN] Error code.
 * \return Error text.
 **/
const char *vtss_appl_qos_error_txt(mesa_rc rc);

/*!
 * \brief QoS shaper mode (line/data) to text conversion.
 *
 * \param mode       [IN]  Shaper mode.
 * \param buf        [OUT] Buffer for storing result.
 * \return Shaper mode text.
 **/
const char *vtss_appl_qos_shaper_mode2txt(vtss_appl_qos_shaper_mode_t mode, char *buf);

/*!
 * \brief QoS QCL port list to text conversion.
 *
 * \param port_list [IN]  Port list.
 * \param buf       [OUT] Buffer for storing result.
 * \param upper     [IN]  If true then return upper case format, else return lower case format
 * \return Tag type text.
 **/
const char *vtss_appl_qos_qcl_port_list2txt(mesa_port_list_t &port_list, char *buf, BOOL upper);

/****************************************************************************
 * Global configuration
 ****************************************************************************/
/*! \brief QoS Priority Levels configuration */
typedef enum {
    VTSS_APPL_QOS_PRIO_LEVELS_1,   /*!< Use 1 priority level  - map level 0..7 to 0 */
    VTSS_APPL_QOS_PRIO_LEVELS_2,   /*!< Use 2 priority levels - map level 0..3 to 0 and 4..7 to 1 */
    VTSS_APPL_QOS_PRIO_LEVELS_4,   /*!< Use 4 priority levels - map level 0..1 to 0, 2..3 to 1, 4..5 to 2 and 6..7 to 3 */
    VTSS_APPL_QOS_PRIO_LEVELS_8,   /*!< Use 8 priority levels - map level 0..7 to 0..7 */
    VTSS_APPL_QOS_PRIO_LEVELS_LAST /*!< End of enum */
} vtss_appl_qos_prio_levels_t;

/*! \brief QoS global configuration */
typedef struct {
    vtss_appl_qos_prio_levels_t prio_levels;  /*!< Number of supported priority levels (CoS values) */
} vtss_appl_qos_global_conf_t;

/*! \brief QoS global switch configuration */
typedef struct {
    vtss_appl_qos_global_conf_t global; /*!< Global parameters */

    vtss_appl_qos_global_storm_policer_t uc_policer; /*!< Unicast storm policer */
    vtss_appl_qos_global_storm_policer_t mc_policer; /*!< Multicast storm policer */
    vtss_appl_qos_global_storm_policer_t bc_policer; /*!< Broadcast storm policer */

#if 1
    vtss_appl_qos_wred_t wred[VTSS_APPL_QOS_PORT_QUEUE_CNT][3][3]; /*!< Weighted Random Early Detection. Per queue (0..7) per DPL (1..3) per group (0..2) */
#else
    CapArray<vtss_appl_qos_wred_t,  VTSS_APPL_CAP_QOS_PORT_QUEUE_CNT, VTSS_APPL_CAP_QOS_WRED_DPL_CNT, VTSS_APPL_CAP_QOS_WRED_GRP_CNT> wred; /*!< Weighted Random Early Detection. Per queue (0..7) per DPL (1..3) per group (0..2) */
#if defined(VTSS_FEATURE_QOS_WRED)
    vtss_appl_qos_wred_t wred[VTSS_APPL_QOS_PORT_QUEUE_CNT]; /*!< Weighted Random Early Detection. Per queue (0..7) */
#elif defined(VTSS_FEATURE_QOS_WRED_V2)
    vtss_appl_qos_wred_t wred[VTSS_APPL_QOS_PORT_QUEUE_CNT][2]; /*!< Weighted Random Early Detection. Per queue (0..7) per DPL (0..1) */
#elif defined(VTSS_FEATURE_QOS_WRED_V3)
    vtss_appl_qos_wred_t wred[VTSS_APPL_QOS_PORT_QUEUE_CNT][VTSS_APPL_QOS_WRED_DPL_CNT][VTSS_APPL_QOS_WRED_GROUP_CNT]; /*!< Weighted Random Early Detection. Per queue (0..7) per DPL (1..3) per group (0..2) */
#endif /* VTSS_FEATURE_QOS_WRED_V3 */
#endif

#if defined(VTSS_SW_OPTION_QOS_ADV)
    vtss_appl_qos_dscp_entry_t     dscp_map[64];                       /*!< Global DSCP mappings */
    vtss_appl_qos_cos_dscp_entry_t cos_dscp_map[VTSS_PRIO_ARRAY_SIZE]; /*!< Global COS to DSCP mappings */
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
} vtss_appl_qos_conf_t;

/*!
 * \brief Set QoS global switch configuration.
 *
 * \param conf [IN]  The global switch configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_conf_set(const vtss_appl_qos_conf_t *conf);

/*!
 * \brief Get QoS global switch configuration.
 *
 * \param conf [OUT] The global switch configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_conf_get(vtss_appl_qos_conf_t *conf);

/*!
 * \brief Get QoS default global switch configuration.
 *
 * \param conf [OUT] The default global switch configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_conf_get_default(vtss_appl_qos_conf_t *conf);

/****************************************************************************
 * Per port configuration and status
 ****************************************************************************/

/*! \brief QoS port configuration */
typedef struct {
    vtss_appl_qos_if_conf_t            port;                                                  /*!< Interface configuration */

    vtss_appl_qos_tag_cos_entry_t      tag_cos_map[VTSS_PCP_ARRAY_SIZE][VTSS_DEI_ARRAY_SIZE]; /*!< Ingress mapping from PCP,DEI to CoS,DPL */

    vtss_appl_qos_port_policer_t       port_policer;                                          /*!< Port policer configuration */
#if defined(VTSS_SW_OPTION_QOS_ADV)
    vtss_appl_qos_queue_policer_t      queue_policer[VTSS_APPL_QOS_PORT_QUEUE_CNT];           /*!< Queue policers configuration */
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
    vtss_appl_qos_port_storm_policer_t uc_policer;                                            /*!< Storm policer for unicast frames */
    vtss_appl_qos_port_storm_policer_t bc_policer;                                            /*!< Storm policer for broadcast frames */
    vtss_appl_qos_port_storm_policer_t un_policer;                                            /*!< Storm policer for unknown (flooded) frames (TTM) or multicast frames (TTM_V2) */

    vtss_appl_qos_port_shaper_t        port_shaper;                                           /*!< Port shaper configuration */
    vtss_appl_qos_queue_shaper_t       queue_shaper[VTSS_APPL_QOS_PORT_QUEUE_CNT];            /*!< Queue shapers configuration */

    vtss_appl_qos_scheduler_t          scheduler[VTSS_APPL_QOS_PORT_QUEUE_CNT];               /*!< Scheduler configuration */

    vtss_appl_qos_cos_tag_entry_t      cos_tag_map[VTSS_APPL_QOS_PORT_PRIO_CNT][2];           /*!< Egress mapping from CoS,DPL to PCP,DEI */

} vtss_appl_qos_port_conf_t;

inline int vtss_memcmp(const vtss_appl_qos_port_conf_t &a, const vtss_appl_qos_port_conf_t &b)
{
    VTSS_MEMCMP_ELEMENT(a, b, port);
    VTSS_MEMCMP_ELEMENT_ARRAY(a, b, tag_cos_map);
    VTSS_MEMCMP_ELEMENT(a, b, port_policer);
#if defined(VTSS_SW_OPTION_QOS_ADV)
    VTSS_MEMCMP_ELEMENT_ARRAY(a, b, queue_policer);
#endif
    VTSS_MEMCMP_ELEMENT(a, b, uc_policer);
    VTSS_MEMCMP_ELEMENT(a, b, bc_policer);
    VTSS_MEMCMP_ELEMENT(a, b, un_policer);
    VTSS_MEMCMP_ELEMENT(a, b, port_shaper);
    VTSS_MEMCMP_ELEMENT_ARRAY(a, b, queue_shaper);
    VTSS_MEMCMP_ELEMENT_ARRAY(a, b, scheduler);
    VTSS_MEMCMP_ELEMENT_ARRAY(a, b, cos_tag_map);
    return 0;
}

/*! \brief QoS port status */
typedef struct {
    /* Ingress classification */
    mesa_prio_t                      default_cos;                             /*!< Currently active default CoS */

    /* Scheduler */
    vtss_appl_qos_scheduler_status_t scheduler[VTSS_APPL_QOS_PORT_QUEUE_CNT]; /*!< Currently active scheduler configuration */

} vtss_appl_qos_port_status_t;

/*!
 * \brief Set QoS port configuration.
 *
 * \param isid    [IN]  The switch ID for which to set the configuration.
 * \param port_no [IN]  The port number for which to set the configuration.
 * \param conf    [IN]  The port configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_conf_set(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_appl_qos_port_conf_t *conf);

/*!
 * \brief Get QoS port configuration.
 *
 * \param isid    [IN]  The switch ID for which to get the configuration.
 * \param port_no [IN]  The port number for which to get the configuration.
 * \param conf    [OUT] The port configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_conf_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_qos_port_conf_t *conf);

/*!
 * \brief Get QoS default port configuration.
 *
 * \param conf [OUT] The default port configuration.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_conf_get_default(vtss_appl_qos_port_conf_t *conf);

/*!
 * \brief Get QoS port status.
 *
 * \param isid    [IN]  The switch ID for which to get the status.
 * \param port_no [IN]  The port number for which to get the status.
 * \param status  [OUT] The port status.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_qos_port_status_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_appl_qos_port_status_t *status);

/* Set port QoS volatile default priority. Use VTSS_PRIO_NO_NONE to disable */
mesa_rc qos_port_volatile_default_prio_set(vtss_isid_t isid, mesa_port_no_t port_no, mesa_prio_t default_prio);

/* ================================================================= *
 *  QoS port configuration change events
 * ================================================================= */

/**
 * \brief QoS port configuration change callback.
 *
 * Global callbacks are executed in management thread context (e.g Console, Telnet, SSH or Web) and are not time sensitive.
 * Local callbacks are executed in msg rx thread context and must NOT contain lengthy operations.
 *
 * \param isid     [IN]  Switch ID on global registrations, otherwise VTSS_ISID_LOCAL.
 * \param iport    [IN]  Port number.
 * \param conf     [OUT] New configuration.
 *
 * \return Nothing.
 */
typedef void (*qos_port_conf_change_cb_t)(const vtss_isid_t isid, const mesa_port_no_t iport, const vtss_appl_qos_port_conf_t *const conf);

/**
 * \brief QoS port configuration change callback registration.
 *
 * \param global    [IN]  FALSE: Callback is called on local switch only (primary or secondary switch) with isid == VTSS_ISID_LOCAL.
 *                        TRUE:  Callback is called on primary switch only and contains actual isid.
 *                               Use FALSE if your module is distributed among all switches.
 *                               Use TRUE if your module is centralized on the primary switch.
 * \param module_id [IN]  Callers module ID.
 * \param callback  [IN]  Callback function to be called on QoS port configuration changes.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc qos_port_conf_change_register(BOOL global, vtss_module_id_t module_id, qos_port_conf_change_cb_t callback);

/* QoS port configuration change registration info - for debug only */
typedef struct {
    BOOL                      global;    /* Local or global */
    vtss_module_id_t          module_id; /* Module ID */
    qos_port_conf_change_cb_t callback;  /* User callback function */
    vtss_tick_count_t          max_ticks; /* Maximum ticks */
} qos_port_conf_change_reg_t;

/* Get/clear QoS port configuration change registration info - for debug only  */
mesa_rc qos_port_conf_change_reg_get(qos_port_conf_change_reg_t *entry, BOOL clear);

/* ================================================================= *
 *  QoS Ingress Map Configuration
 * ================================================================= */

typedef struct {
    BOOL                               used;          /*!< TRUE if used */
    vtss_appl_qos_ingress_map_conf_t   conf;          /*!< Key and action parameters */
    vtss_appl_qos_ingress_map_values_t pcp_dei[8][2]; /*!< Mapped values for each PCP/DEI value */
    vtss_appl_qos_ingress_map_values_t dscp[64];      /*!< Mapped values for each DSCP value */
} vtss_appl_qos_imap_entry_t;

/*!
 * \brief Get an ingress map entry.
 *
 * \param id    [IN]  Map ID.
 * \param entry [OUT] Ingress map entry.
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_qos_imap_entry_get(mesa_qos_ingress_map_id_t  id,
                                     vtss_appl_qos_imap_entry_t *entry);

/*!
 * \brief Get a default ingress map entry.
 *
 * \param id    [IN]  Map ID.
 * \param entry [OUT] Default ingress map entry.
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_qos_imap_entry_get_default(mesa_qos_ingress_map_id_t  id,
                                             vtss_appl_qos_imap_entry_t *entry);

/* ================================================================= *
 *  QoS Egress Map Configuration
 * ================================================================= */

typedef struct {
    BOOL                              used;            /*!< TRUE if used */
    vtss_appl_qos_egress_map_conf_t   conf;            /*!< Key and action parameters */
    vtss_appl_qos_egress_map_values_t cosid_dpl[8][4]; /*!< Mapped values for each COSID/DPL value */
    vtss_appl_qos_egress_map_values_t dscp_dpl[64][4]; /*!< Mapped values for each DSCP/DPL value */
} vtss_appl_qos_emap_entry_t;

/*!
 * \brief Get an egress map entry.
 *
 * \param id    [IN]  Map ID.
 * \param entry [OUT] Egress map entry.
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_qos_emap_entry_get(mesa_qos_egress_map_id_t   id,
                                     vtss_appl_qos_emap_entry_t *entry);

/*!
 * \brief Get a default egress map entry.
 *
 * \param id    [IN]  Map ID.
 * \param entry [OUT] Default egress map entry.
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_qos_emap_entry_get_default(mesa_qos_egress_map_id_t   id,
                                             vtss_appl_qos_emap_entry_t *entry);


/* ================================================================= *
 *  QoS QCE configuration
 * ================================================================= */

/*! \brief QoS QCE entry configuration */
typedef struct {
    vtss_isid_t              isid;     /*!< Switch ID */
    vtss_appl_qos_qcl_user_t user_id;  /*!< QCE user id */
    BOOL                     conflict; /*!< QCE conflict flag */
    mesa_qce_t               qce;      /*!< QCE configuration */
} vtss_appl_qos_qce_intern_conf_t;

/*!
 * \brief Add QCE.
 *
 * \param next_qce_id [IN]  Next QCE ID. The QCE will be added before next_qce_id or last if next_qce_id == QCE_ID_NONE.
 * \param conf        [IN]  The QCE to be added.
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_qos_qce_intern_add(mesa_qce_id_t                   next_qce_id,
                                     vtss_appl_qos_qce_intern_conf_t *conf);

/*!
 * \brief Delete QCE.
 *
 * \param isid    [IN]  Internal switch ID
 * \param user_id [IN]  User ID. The user of this entry.
 * \param qce_id  [IN]  QCE ID to be deleted.
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_qos_qce_intern_del(vtss_isid_t              isid,
                                     vtss_appl_qos_qcl_user_t user_id,
                                     mesa_qce_id_t            qce_id);

/*!
 * \brief Get specific QCE or get next QCE.
 *
 * \param isid    [IN]  Internal switch ID.
 * \param user_id [IN]  User ID. The user of this entry.
 * \param qce_id  [IN]  QCE ID. If QCE_ID_NONE, the first existing entry is returned and next is ignored.
 * \param conf    [OUT] The QCE.
 * \param next    [IN]  Next. If TRUE, get next QCE after qce_id. Otherwise get specific QCE
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_qos_qce_intern_get(vtss_isid_t                     isid,
                                     vtss_appl_qos_qcl_user_t        user_id,
                                     mesa_qce_id_t                   qce_id,
                                     vtss_appl_qos_qce_intern_conf_t *conf,
                                     BOOL                            next);

/*!
 * \brief Get default QCE
 *
 * \param conf [OUT] The default QCE.
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_qos_qce_intern_get_default(vtss_appl_qos_qce_intern_conf_t *conf);

/*!
* \brief Get specific QCE configuration/status based on index.
*
* \param isid  [IN]  Internal switch ID.
* \param index [IN]  Index in list (1..n).
* \param conf  [OUT] The QCE.
*
* \return Return code.
**/
mesa_rc vtss_appl_qos_qce_intern_get_nth(vtss_isid_t                     isid,
                                         u32                             index,
                                         vtss_appl_qos_qce_intern_conf_t *conf);

/*!
 * \brief Resolve QCE conflict in hardware.
 *
 * Only implemented if capabilities.has_qce is TRUE
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_qos_qce_conflict_resolve(void);

/****************************************************************************
 * Enum Descriptors
 ****************************************************************************/

extern const vtss_enum_descriptor_t vtss_appl_qos_wred_max_txt[];         /*!< Enum descriptor text */
extern const vtss_enum_descriptor_t vtss_appl_qos_tag_remark_mode_txt[];  /*!< Enum descriptor text */
extern const vtss_enum_descriptor_t vtss_appl_qos_dscp_mode_txt[];        /*!< Enum descriptor text */
extern const vtss_enum_descriptor_t vtss_appl_qos_dscp_emode_txt[];       /*!< Enum descriptor text */
extern const vtss_enum_descriptor_t vtss_appl_qos_qcl_user_txt[];         /*!< Enum descriptor text */
extern const vtss_enum_descriptor_t vtss_appl_qos_qce_type_txt[];         /*!< Enum descriptor text */
extern const vtss_enum_descriptor_t vtss_appl_qos_shaper_mode_txt[];      /*!< Enum descriptor text */
extern const vtss_enum_descriptor_t vtss_appl_qos_ingress_map_key_txt[];  /*!< Enum descriptor text */
extern const vtss_enum_descriptor_t vtss_appl_qos_egress_map_key_txt[];   /*!< Enum descriptor text */

/****************************************************************************
 * Helper funtions for management interfaces, such as CLI and Web
 ****************************************************************************/

/*!
 * \brief QoS minimum rate function.
 *
 * If one of the rates is zero, it is ignored and the other rate is returned.
 * Both rates must not be zero!
 *
 * \param r1 [IN] First rate to compare
 * \param r2 [IN] Second rate to compare
 *
 * \return The smallest of the two rates.
 */

uint32_t vtss_appl_qos_rate_min(uint32_t r1, uint32_t r2);

/*!
 * \brief QoS maximum rate function.
 *
 * If one of the rates is zero, it is ignored and the other rate is returned.
 * Both rates must not be zero!
 *
 * \param r1 [IN] First rate to compare
 * \param r2 [IN] Second rate to compare
 *
 * \return The largest of the two rates.
 */

uint32_t vtss_appl_qos_rate_max(uint32_t r1, uint32_t r2);

/*!
 * \brief QoS general policer and shaper rate to text conversion.
 *
 * \param rate       [IN]  Rate.
 * \param frame_rate [IN]  If true then rate unit is fps.
 * \param buf        [OUT] Buffer for storing result.
 * \return Rate text.
 **/
const char *vtss_appl_qos_rate2txt(uint32_t rate, mesa_bool_t frame_rate, char *buf);

/*!
 * \brief QoS Ingress Map key to text conversion.
 *
 * \param key   [IN] Key.
 * \param upper [IN] If true then return upper case format, else return lower case format
 * \return Key text.
 **/
const char *vtss_appl_qos_ingress_map_key2txt(vtss_appl_qos_ingress_map_key_t key, mesa_bool_t upper);

/*!
 * \brief QoS Egress Map key to text conversion.
 *
 * \param key   [IN] Key.
 * \param upper [IN] If true then return upper case format, else return lower case format
 * \return Key text.
 **/
const char *vtss_appl_qos_egress_map_key2txt(vtss_appl_qos_egress_map_key_t key, mesa_bool_t upper);

/*!
 * \brief QoS QCL key type to text conversion.
 *
 * \param key_type [IN] Key type.
 * \param upper    [IN] If true then return upper case format, else return lower case format
 * \return Key type text.
 **/
const char *vtss_appl_qos_qcl_key_type2txt(mesa_vcap_key_type_t key_type, mesa_bool_t upper);

/*!
 * \brief QoS DSCP value to text conversion.
 *
 * \param dscp [IN] DSCP value.
 * \return DSCP text.
 **/
const char *vtss_appl_qos_dscp2str(mesa_dscp_t dscp);

/*!
 * \brief QoS QCL user to text conversion.
 *
 * \param user  [IN] QCL User.
 * \param upper [IN] If true then return upper case format, else return lower case format
 * \return DSCP text.
 **/
const char *vtss_appl_qos_qcl_user2txt(vtss_appl_qos_qcl_user_t user, mesa_bool_t upper);

/*!
 * \brief QoS QCL tag type to text conversion.
 *
 * \param tagged [IN] Value of 'tagged' bit.
 * \param s_tag  [IN] Value of 's-tag' bit.
 * \param upper [IN] If true then return upper case format, else return lower case format
 * \return Tag type text.
 **/
const char *vtss_appl_qos_qcl_tag_type2txt(mesa_vcap_bit_t tagged, mesa_vcap_bit_t s_tag, mesa_bool_t upper);

/*!
 * \brief QoS QCL DMAC type to text conversion.
 *
 * \param bc    [IN] Value of 'bc' bit.
 * \param mc    [IN] Value of 'mc' bit.
 * \param upper [IN] If true then return upper case format, else return lower case format
 * \return DMAC type text.
 **/
const char *vtss_appl_qos_qcl_dmactype2txt(mesa_vcap_bit_t bc, mesa_vcap_bit_t mc, mesa_bool_t upper);

/*!
 * \brief QoS QCL protocol to text conversion.
 *
 * \param proto [IN]  Protocol.
 * \param buf   [OUT] Buffer for storing result.
 * \return Protocol text.
 **/
const char *vtss_appl_qos_qcl_proto2txt(mesa_vcap_u8_t *proto, char *buf);

/*!
 * \brief Convert an ipv4 address to the 32 LSB in an ipv6 address.
 *
 * \param ipv4 [IN]  IPv4 value.
 * \param ipv6 [OUT] IPv6 value.
 * \return Void.
 **/
void vtss_appl_qos_qcl_ipv42ipv6(mesa_vcap_ip_t *ipv4, mesa_vcap_u128_t *ipv6);

/*!
 * \brief Convert the 32 LSB in an ipv6 address to an ipv4 address.
 *
 * \param ipv6 [IN]  IPv6 value.
 * \param ipv4 [OUT] IPv4 value.
 * \return Void.
 **/
void vtss_appl_qos_qcl_ipv62ipv4(mesa_vcap_u128_t *ipv6, mesa_vcap_ip_t *ipv4);

/*!
 * \brief Convert an ipv4 address to text.
 *
 * \param ipv4  [IN]  IPv4 value.
 * \param buf   [OUT] Buffer for storing result.
 * \param upper [IN]  If true then return upper case format, else return lower case format
 * \return IPv4 text.
 **/
const char *vtss_appl_qos_qcl_ipv42txt(mesa_vcap_ip_t *ipv4, char *buf, mesa_bool_t upper);

/*!
 * \brief Convert an ipv6 address to text.
 *
 * \param ipv6  [IN]  IPv6 value.
 * \param buf   [OUT] Buffer for storing result.
 * \param upper [IN]  If true then return upper case format, else return lower case format
 * \return IPv6 text.
 **/
const char *vtss_appl_qos_qcl_ipv62txt(mesa_vcap_u128_t *ipv6, char *buf, mesa_bool_t upper);

/*!
 * \brief Convert a range to text.
 *
 * \param range [IN]  The range.
 * \param buf   [OUT] Buffer for storing result.
 * \param upper [IN]  If true then return upper case format, else return lower case format
 * \return Range text.
 **/
const char *vtss_appl_qos_qcl_range2txt(mesa_vcap_vr_t *range, char *buf, mesa_bool_t upper);

/*!
 * \brief Convert specific values to a range.
 *
 * If min, max and mask is 0 range is 'any'.
 * If min >= max it's specific value/mask else it's range.
 *
 * \param dest [OUT] The range.
 * \param min  [IN]  Minimum value.
 * \param max  [IN]  Maximum value.
 * \param mask [IN]  Mask.
 * \return Void.
 **/
void vtss_appl_qos_qcl_range_set(mesa_vcap_vr_t *dest, uint16_t min, uint16_t max, uint16_t mask);

/* Initialize module */
mesa_rc qos_init(vtss_init_data_t *data);

// Start shaper calibration timer
void qos_shaper_calibration_timer_start(void);

#endif /* _VTSS_QOS_APPL_API_H_ */
