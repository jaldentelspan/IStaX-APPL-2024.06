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

/**
 * \file
 * \brief RMON API
 * \details This header file describes public functions applicable to RMON management.
 */

#ifndef _VTSS_APPL_RMON_H_
#define _VTSS_APPL_RMON_H_

#include <vtss/appl/interface.h>
/* {--------------------- Data Types & macro defined --------------------------- */

/**
 * Length of log and event description string buffers
 */
#define VTSS_APPL_RMON_LOG_STRING_LENGTH        128

/**
 * Maximum number of RMON statistics entries supported
 */
#define VTSS_APPL_RMON_STATISTICS_ENTRIES_MAX   128

/**
 * Maximum number of RMON history entries supported
 */
#define VTSS_APPL_RMON_HISTORY_ENTRIES_MAX      256

/**
 * Maximum number RMON alarm entries supported
 */
#define VTSS_APPL_RMON_ALARM_ENTRIES_MAX        256

/**
 * Maximum number RMON event entries supported
 */
#define VTSS_APPL_RMON_EVENT_ENTRIES_MAX        128

/**
 * Maximum number of log/data entries to iterate through for all types of RMON entries
 */
#define VTSS_APPL_RMON_DATA_INDEX_ITR_MAX       255

/**
 * Start index number for all types of RMON entries' key/index
 */
#define VTSS_APPL_RMON_ENTRIES_START_INDEX      1

/**
 * Maximum number of history buckets.
 */
#define RMON_BUCKET_CNT_MAX 50

/**
 * Default number of history buckets.
 */
#define RMON_BUCKET_CNT_DEF RMON_BUCKET_CNT_MAX

/**
 * \brief RMON event log entry.
 */
typedef struct {

    /**
     * Time when this log entry was created
     */
    uint64_t     log_time;

    /**
     * Log description string
     */
    char    log_description[VTSS_APPL_RMON_LOG_STRING_LENGTH];
} vtss_appl_event_data_t;

/**
 * \brief RMON event stored type.
 */
typedef enum {
    VTSS_APPL_RMON_EVENT_NONE = 1,         /**< none store event */
    VTSS_APPL_RMON_EVENT_LOG,              /**< store event in log table   */
    VTSS_APPL_RMON_EVENT_TRAP,             /**< send event to trap destination */
    VTSS_APPL_RMON_EVENT_LOG_AND_TRAP,     /**< store event in log table and send it to trap destination   */
    VTSS_APPL_RMON_EVENT_END               /**< last event enum value, is used to denote total number of member enums */
} vtss_appl_event_t;

/**
 * \brief RMON alarm sampling type
 */
typedef enum {
    VTSS_APPL_RMON_SAMPLE_TYPE_ABSOLUTE =  1,      /**< absolute sampling  */
    VTSS_APPL_RMON_SAMPLE_TYPE_DELTA,              /**< dalta sampling     */
    VTSS_APPL_RMON_SAMPLE_TYPE_END                 /**< total enums */
} vtss_appl_alarm_sample_t;

/**
 * \brief RMON alarm trigger type
 */
typedef enum {
    VTSS_APPL_RMON_ALARM_NOTHING = 0,              /**< none trigger   */
    VTSS_APPL_RMON_ALARM_RISING,                   /**< rising trigger     */
    VTSS_APPL_RMON_ALARM_FALLING,                  /**< falling trigger    */
    VTSS_APPL_RMON_ALARM_BOTH,                     /**< rising or falling trigger  */
    VTSS_APPL_RMON_ALARM_END                       /**< total enums */
} vtss_appl_alarm_t;

/**
 * \brief data type to specify variable/counter type to monitor for a give alarm entry
 */
typedef enum {

    /**
     * Octets statistics counter of ingress side
     */
    VTSS_APPL_RMON_IF_IN_OCTETS = 10,

    /**
     * Unicast packets statistics counter of ingress side
     */
    VTSS_APPL_RMON_IF_IN_UCAST_PKTS,

    /**
     * Non-unicast (i.e. Multicast and Broadcast) statistics counter of ingress side
     */
    VTSS_APPL_RMON_IF_IN_NON_UCAST_PKTS,

    /**
     * Discard statistics counter of ingress side
     */
    VTSS_APPL_RMON_IF_IN_DISCARDS,

    /**
     * Error statistics counter of ingress side
     */
    VTSS_APPL_RMON_IF_IN_ERRORS,

    /**
     * Unknown protocols statistics counter of ingress side
     */
    VTSS_APPL_RMON_IF_IN_UNKNOWN_PROTOCOLS,

    /**
     * Octets statistics counter of egress side for an interface
     */
    VTSS_APPL_RMON_IF_OUT_OCTETS,

    /**
     * Unicast statistics counter of egress side for an interface
     */
    VTSS_APPL_RMON_IF_OUT_UCAST_PKTS,

    /**
     * Non-Unicast statistics counter of egress side for an interface
     */
    VTSS_APPL_RMON_IF_OUT_NON_UCAST_PKTS,

    /**
     * Discard statistics counter of egress side for an interface
     */
    VTSS_APPL_RMON_IF_OUT_DISCARDS,

    /**
     * Error statistics counter of egress side for an interface
     */
    VTSS_APPL_RMON_IF_OUT_ERRORS,

    /**
     * length of the output packet queue
     */
    VTSS_APPL_RMON_IF_OUT_QLEN

} vtss_appl_rmon_alarm_if_t;

/**
 * \brief RMON ethernet statistics datatype
 */
typedef struct {
    vtss_ifindex_t  ifIndex;    /**< Interface index */
    uint32_t          drop_events;   /**< Dropped frames counter */
    uint32_t          octets;        /**< Number of octets/bytes */
    uint32_t          packets;       /**< Packets counter */
    uint32_t          bcast_pkts;    /**< Broadcoast packets count */
    uint32_t          mcast_pkts;    /**< Multicast packets count */
    uint32_t          crc_align;     /**< CRC error count */
    uint32_t          undersize;     /**< Undersized packets count */
    uint32_t          oversize;      /**< Oversized packets count */
    uint32_t          fragments;     /**< Fragments count */
    uint32_t          jabbers;       /**< Jabber frame count */
    uint32_t          collisions;    /**< Collision error count */
    uint32_t          pkts_64;       /**< pkts sized less than 64 bytes */
    uint32_t          pkts_65_127;   /**< pkts sized from 64 to 128 */
    uint32_t          pkts_128_255;  /**< pkts sized from 128 to 256 */
    uint32_t          pkts_256_511;  /**< pkts sized from 256 to 512 */
    uint32_t          pkts_512_1023; /**< pkts sized from 512 to 1024 */
    uint32_t          pkts_1024_1518;/**< pkts sized from 1024 to 1518 */
} vtss_appl_eth_stats_t;

/**
 * \brief RMON history data entry
 */
typedef struct {
    uint64_t                     start_interval;     /**< Data entry startup time */
    uint64_t                     utilization;        /**< The untilization   */
    vtss_appl_eth_stats_t   EthData;            /**< The statistics data    */
} vtss_appl_history_statistics_t;

/**
 * \brief RMON statistics data entry
 */
typedef struct {
    uint32_t                     etherStatsCreateTime;   /**< the time when the entry is activated. */
    vtss_appl_eth_stats_t   eth;                    /**< the statistics data    */
} vtss_appl_stats_statistics_entry_t;

/**
 * \brief RMON statistics control entry
 */
typedef struct {
    vtss_ifindex_t      ifIndex;                /**< Interface index */
} vtss_appl_stats_ctrl_entry_t;

/**
 * \brief RMON history control entry
 */
typedef struct {
    /**
     * Interface index
     */
    vtss_ifindex_t ifIndex;

    /**
     * Sampling interval
     */
    uint32_t interval;

    /**
     * Requested data bucket size.
     * [1; RMON_BUCKET_CNT_MAX], default is RMON_BUCKET_CNT_DEF
     */
    uint32_t data_requested;
} vtss_appl_history_entry_t;

/**
 * \brief RMON history entry's status
 */
typedef struct {
    /**
     * Size of data bucket granted in added history entry
     */
    uint32_t     data_granted;
} vtss_appl_history_status_t; 

/**
 * \brief RMON alarm control entry
 */
typedef struct {
    vtss_appl_rmon_alarm_if_t   var_name;       /**< The source data to be compared */
    vtss_appl_alarm_sample_t    sample_type;    /**< The source data to be computed with absolute value or delta value */
    vtss_appl_alarm_t           startup_type;   /**< Trigger rising or falling alarm when startup */

    uint32_t                 interval;               /**< Sampling interval */
    vtss_ifindex_t      ifIndex;                /**< Interface index */
    int32_t                 rising_threshold;       /**< Rising threshold */
    int32_t                 falling_threshold;      /**< Falling threshold */
    uint32_t                 rising_event_index;     /**< Rising event index*/
    uint32_t                 falling_event_index;    /**< Falling event index */
} vtss_appl_alarm_ctrl_entry_t;

/**
 * \brief Data type of hold the RMON alarm count
 */
typedef struct {

    /**
     * RMON alarm trigger count value
     */
    int32_t value;
} vtss_appl_alarm_value_t;

/**
 * \brief RMON event entry.
 */
typedef struct {
    char                event_description[VTSS_APPL_RMON_LOG_STRING_LENGTH];      /**< Event description */
    vtss_appl_event_t   event_type;           /**< RMON event stored type */
} vtss_appl_event_entry_t;

/* }---------------------------------------------------------------------------- */

/* {--------------------------- Public API functions -----------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Set/Add RMON statistics entry
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_STATISTICS_ENTRIES_MAX)
 * \param entry [IN] Entry set/add flag (set to TRUE) to save entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_statistics_entry_add(uint32_t index, const vtss_appl_stats_ctrl_entry_t *const entry);

/**
 * \brief Delete RMON statistics entry
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_STATISTICS_ENTRIES_MAX)
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_statistics_entry_del(uint32_t index);

/**
 * \brief Get RMON statistics entry
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_STATISTICS_ENTRIES_MAX)
 * \param entry [OUT] Entry status flag, turned to TRUE, if entry is present.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_statistics_entry_get(uint32_t index, vtss_appl_stats_ctrl_entry_t *const entry);

/**
 * \brief Get RMON statistics entry's status
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_STATISTICS_ENTRIES_MAX)
 * \param entry [OUT] Entry datatype which holds the current status attributes for an entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_statistics_stats_get(uint32_t index, vtss_appl_stats_statistics_entry_t *const entry);

/**
 * \brief RMON statistics entries' index iterator
 *
 * Notice that the indices are 1-based
 *
 * \param prev_idx [IN]  Previous entry's index number, if NULL then next_idx will return 1 (i.e. 1st index).
 * \param next_idx [OUT] Next entry's index number, which depends on value of prev_idx.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_statistics_itr(const uint32_t *const prev_idx,
                                      uint32_t *const next_idx);

/**
 * \brief Add/Set RMON history entry
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_HISTORY_ENTRIES_MAX)
 * \param entry [IN] Pointer to Entry structure, which hold configuration to be set for an entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_history_entry_add(uint32_t index, const vtss_appl_history_entry_t *const entry);

/**
 * \brief Delete RMON history entry
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_HISTORY_ENTRIES_MAX)
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_history_entry_del(uint32_t index);

/**
 * \brief Get RMON history entry
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_HISTORY_ENTRIES_MAX)
 * \param entry [OUT] Pointer to Entry structure, which will hold configuration returned for an entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_history_entry_get(uint32_t index, vtss_appl_history_entry_t *const entry);

/**
 * \brief Get RMON history entry status
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_HISTORY_ENTRIES_MAX)
 * \param entry [OUT] Pointer to Entry structure, which will hold status attributes of an entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_history_status_get(uint32_t index, vtss_appl_history_status_t *const entry);

/**
 * \brief RMON history entries' index iterator
 *
 * Notice that the indices are 1-based
 *
 * \param prev_idx [IN]  Previous entry's index number, if NULL then next_idx will contain 1 (i.e. 1st index).
 * \param next_idx [OUT] Next entry's index number, which depends on value of prev_idx.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_history_entry_itr(const uint32_t *const prev_idx,
                                         uint32_t *const next_idx);

/**
 * \brief Get RMON history entry statistics
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number (1..VTSS_APPL_RMON_HISTORY_ENTRIES_MAX)
 * \param data_index [IN] Data Index number (1..VTSS_APPL_RMON_DATA_INDEX_ITR_MAX)
 * \param entry [OUT] Pointer to Entry structure, which will hold status attributes of an entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_history_stats_get(uint32_t index,
                                         uint32_t data_index,
                                         vtss_appl_history_statistics_t *const entry);

/**
 * \brief RMON history entries iterator
 *
 * Notice that the indices are 1-based
 *
 * \param prev_idx [IN]  Pointer to Index number (1..VTSS_APPL_RMON_HISTORY_ENTRIES_MAX)
 * \param next_idx [OUT] Pointer to Index number (1..VTSS_APPL_RMON_HISTORY_ENTRIES_MAX).
 * \param prev_data_idx [IN]  Pointer to Data Index number (1..VTSS_APPL_RMON_DATA_INDEX_ITR_MAX)
 * \param next_data_idx [OUT] Pointer to Data Index number (1..VTSS_APPL_RMON_DATA_INDEX_ITR_MAX)
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_history_stats_itr(const uint32_t *const prev_idx,
                                         uint32_t *const next_idx,
                                         const uint32_t *const prev_data_idx,
                                         uint32_t *const next_data_idx);

/**
 * \brief Add/Set RMON alarm entry
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_ALARM_ENTRIES_MAX)
 * \param entry [IN] Pointer to Entry structure, which hold configuration to be set for an entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_alarm_entry_add(uint32_t index, const vtss_appl_alarm_ctrl_entry_t *const entry);

/**
 * \brief Delete RMON alarm entry
 *
 * Notice that the indices are 1-based for entry index key
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_ALARM_ENTRIES_MAX)
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_alarm_entry_del(uint32_t index);

/**
 * \brief Get RMON alarm entry
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN]  Index number value (1..VTSS_APPL_RMON_ALARM_ENTRIES_MAX)
 * \param entry [OUT] Pointer to Entry structure, which will hold configuration returned for an entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_alarm_entry_get(uint32_t index, vtss_appl_alarm_ctrl_entry_t *const entry);

/**
 * \brief Get RMON alarm entry status
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN]  Index number value (1..VTSS_APPL_RMON_ALARM_ENTRIES_MAX)
 * \param value [OUT] Pointer to Entry structure, which will hold status attributes of an entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_alarm_status_get(uint32_t index, vtss_appl_alarm_value_t *const value);

/**
 * \brief RMON alarm entries' index iterator
 *
 * Notice that the indices are 1-based
 *
 * \param prev_idx [IN]  Previous entry's index number, if NULL then next_idx will return 1 (i.e. 1st index).
 * \param next_idx [OUT] Next entry's index number, which depends on value of prev_idx.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_alarm_itr(const uint32_t *const prev_idx,
                                 uint32_t *const next_idx);

/**
 * \brief Add/Set RMON event entry
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_EVENT_ENTRIES_MAX)
 * \param entry [IN] Pointer to Entry structure, which hold configuration to be set for an entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_event_entry_add(uint32_t index, const vtss_appl_event_entry_t *const entry);

/**
 * \brief Delete RMON event entry
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_EVENT_ENTRIES_MAX)
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_event_entry_del(uint32_t index);

/**
 * \brief Get RMON event entry
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_EVENT_ENTRIES_MAX)
 * \param entry [OUT] Pointer to Entry structure, which will hold configuration returned for an entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_event_entry_get(uint32_t index, vtss_appl_event_entry_t *const entry);

/**
 * \brief RMON event entries' index iterator
 *
 * Notice that the indices are 1-based
 *
 * \param prev_idx [IN]  Previous entry's index number, if NULL then next_idx will return 1 (i.e. 1st index).
 * \param next_idx [OUT] Next entry's index number, which depends on value of prev_idx.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_event_entry_itr(const uint32_t *const prev_idx, uint32_t *const next_idx);

/**
 * \brief Get RMON event entry
 *
 * Notice that the indices are 1-based
 *
 * \param index      [IN]  Index number value (1..VTSS_APPL_RMON_EVENT_ENTRIES_MAX)
 * \param data_index [IN]  Index of the event's data.
 * \param entry      [OUT] Pointer to Entry structure, which will hold returned configuration this entry.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_event_status_get(uint32_t index, uint32_t data_index, vtss_appl_event_data_t *const entry);

/**
 * \brief RMON event entries iterator
 *
 * Notice that the indices are 1-based
 *
 * \param prev_idx [IN]  Pointer to Index number (1..VTSS_APPL_RMON_EVENT_ENTRIES_MAX)
 * \param next_idx [OUT] Pointer to Index number (1..VTSS_APPL_RMON_EVENT_ENTRIES_MAX).
 * \param prev_data_idx [IN]  Pointer to Data Index number (1..VTSS_APPL_RMON_DATA_INDEX_ITR_MAX)
 * \param next_data_idx [OUT] Pointer to Data Index number (1..VTSS_APPL_RMON_DATA_INDEX_ITR_MAX)
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_event_status_itr(const uint32_t *const prev_idx,
                                        uint32_t *const next_idx,
                                        const uint32_t *const prev_data_idx,
                                        uint32_t *const next_data_idx);
/**
 * \brief Get RMON event last time sent
 *
 * Notice that the indices are 1-based
 *
 * \param index [IN] Index number value (1..VTSS_APPL_RMON_EVENT_ENTRIES_MAX)
 * \param event_last_time_sent [OUT] Pointer to unsigned integer which holds the time when event was last sent.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_rmon_event_status_last_sent(uint32_t index, uint32_t *const event_last_time_sent);
#ifdef __cplusplus
}
#endif
/* }-------------------------------------------------------------------------- */
#endif /* _VTSS_APPL_RMON_H_ */
