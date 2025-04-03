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

#include "rmon_serializer.hxx"
#include "vtss/appl/rmon.h"
#include "rmon_api.h"
#include "rmon_trace.h"

/* {------------ Enum to text definitions --------------*/
vtss_enum_descriptor_t rmon_event_log_trap_type_txt[] {
    {VTSS_APPL_RMON_EVENT_NONE,        "NONE"},
    {VTSS_APPL_RMON_EVENT_LOG,         "LOG"},
    {VTSS_APPL_RMON_EVENT_TRAP,        "TRAP"},
    {VTSS_APPL_RMON_EVENT_LOG_AND_TRAP,"LOG_AND_TRAP"},
    {VTSS_APPL_RMON_EVENT_END,         "END"},
    {0, 0},
};

vtss_enum_descriptor_t rmon_alarm_startup_type_txt[] {
    {VTSS_APPL_RMON_ALARM_NOTHING, "NOTHING"},
    {VTSS_APPL_RMON_ALARM_RISING,  "RISING"},
    {VTSS_APPL_RMON_ALARM_FALLING, "FALLING"},
    {VTSS_APPL_RMON_ALARM_BOTH,    "BOTH"},
    {VTSS_APPL_RMON_ALARM_END,     "END"},
    {0, 0},
};

vtss_enum_descriptor_t rmon_alarm_sample_type_txt[] {
    {VTSS_APPL_RMON_SAMPLE_TYPE_ABSOLUTE,  "ABSOLUTE"},
    {VTSS_APPL_RMON_SAMPLE_TYPE_DELTA,     "DELTA"},
    {VTSS_APPL_RMON_SAMPLE_TYPE_END,       "END"},
    {0, 0},
};

vtss_enum_descriptor_t rmon_if_counter_type_txt[] {
    {VTSS_APPL_RMON_IF_IN_OCTETS,           "ifInOctets"},
    {VTSS_APPL_RMON_IF_IN_UCAST_PKTS,       "ifInUcastPkts"},
    {VTSS_APPL_RMON_IF_IN_NON_UCAST_PKTS,   "ifInNUcastPkts"},
    {VTSS_APPL_RMON_IF_IN_DISCARDS,         "ifInDiscards"},
    {VTSS_APPL_RMON_IF_IN_ERRORS,           "ifInErrors"},
    {VTSS_APPL_RMON_IF_IN_UNKNOWN_PROTOCOLS,"ifInUnknownProtos"},
    {VTSS_APPL_RMON_IF_OUT_OCTETS,          "ifOutOctets"},
    {VTSS_APPL_RMON_IF_OUT_UCAST_PKTS,      "ifOutUcastPkts"},
    {VTSS_APPL_RMON_IF_OUT_NON_UCAST_PKTS,  "ifOutNUcastPkts"},
    {VTSS_APPL_RMON_IF_OUT_DISCARDS,        "ifOutDiscards"},
    {VTSS_APPL_RMON_IF_OUT_ERRORS,          "ifOutErrors"},
    {VTSS_APPL_RMON_IF_OUT_QLEN,            "ifOutQLen"},
    {0, 0},
};

/* }---------------------***********------------------------*/

/* {--------------------------- Table iterators ---------------------------*/
mesa_rc vtss_appl_rmon_statistics_itr(const u32 *const prev_idx,
                                      u32 *const next_idx)
{
    vtss_stat_ctrl_entry_t entry;

    memset(&entry, 0, sizeof(vtss_stat_ctrl_entry_t));
    entry.id = prev_idx ? *prev_idx : 0;

    if(rmon_mgmt_statistics_entry_get(&entry, TRUE) != VTSS_RC_OK) {
        return RMON_ERROR_STAT_ENTRY_NOT_FOUND;
    }
    *next_idx = entry.id;
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_rmon_history_entry_itr(const u32 *const prev_idx,
                                         u32 *const next_idx)
{
    vtss_history_ctrl_entry_t   conf;

    memset(&conf, 0, sizeof(vtss_history_ctrl_entry_t));
    conf.id = prev_idx ? *prev_idx : 0;

    if (rmon_mgmt_history_entry_get(&conf, TRUE) != VTSS_RC_OK){
        return RMON_ERROR_HISTORY_ENTRY_NOT_FOUND;
    }
    *next_idx = conf.id;
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_rmon_history_stats_itr(const u32 *const prev_idx,
                                         u32 *const next_idx,
                                         const u32 *const prev_data_idx,
                                         u32 *const next_data_idx)
{
    vtss_history_data_entry_t data;
    uint32_t                  prv_idx = prev_idx ? *prev_idx : 0;

    data.data_index = prev_data_idx ? *prev_data_idx : 0;
    if (rmon_mgmt_history_data_get(prv_idx, &data, TRUE) == VTSS_RC_OK) {
       *next_idx            = prv_idx;
       *next_data_idx       = data.data_index;
    } else if (vtss_appl_rmon_history_entry_itr(prev_idx, next_idx) == VTSS_RC_OK) {
        data.data_index = 0;
        if (rmon_mgmt_history_data_get(*next_idx, &data, TRUE) == VTSS_RC_OK) {
            *next_data_idx = data.data_index;
        } else {
            return RMON_ERROR_HISTORY_ENTRY_NOT_FOUND;
        }
    } else {
        return RMON_ERROR_HISTORY_ENTRY_NOT_FOUND;
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_rmon_alarm_itr(const u32 *const prev_idx,
                                 u32 *const next_idx)
{
    vtss_alarm_ctrl_entry_t entry;

    memset(&entry, 0, sizeof(vtss_alarm_ctrl_entry_t));

    entry.id = prev_idx ? *prev_idx : 0;

    if (rmon_mgmt_alarm_entry_get(&entry, TRUE) != VTSS_RC_OK) {
        return RMON_ERROR_ALARM_ENTRY_NOT_FOUND;
    }

    *next_idx = entry.id;
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_rmon_event_entry_itr(const u32 *const prev_idx, u32 *const next_idx)
{
    vtss_event_ctrl_entry_t entry;

    memset(&entry, 0, sizeof(vtss_event_ctrl_entry_t));

    entry.id = prev_idx ? *prev_idx : 0;
    T_D("prev_idx = %d", entry.id);
    if (rmon_mgmt_event_entry_get(&entry, TRUE) != VTSS_RC_OK) {
        T_D("RMON_ERROR_EVENT_ENTRY_NOT_FOUND");
        return RMON_ERROR_EVENT_ENTRY_NOT_FOUND;
    }

    *next_idx = entry.id;
    T_D("next_idx = %d", *next_idx);
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_rmon_event_status_itr(const u32 *const prev_idx,
                                        u32 *const next_idx,
                                        const u32 *const prev_data_idx,
                                        u32 *const next_data_idx)
{
    vtss_event_data_entry_t data;
    uint32_t                prv_idx = prev_idx ? *prev_idx : 0;

    data.data_index = prev_data_idx ? *prev_data_idx : 0;
    if (rmon_mgmt_event_data_get(prv_idx, &data, TRUE) == VTSS_RC_OK) {
       *next_idx            = prv_idx;
       *next_data_idx       = data.data_index;
       T_D("next_idx = %d, next_data_idx = %d", *next_idx, *next_data_idx);
    } else if (vtss_appl_rmon_event_entry_itr(prev_idx, next_idx) == VTSS_RC_OK) {
        data.data_index = 0;
        T_D("next_idx = %d", *next_idx);
        if (rmon_mgmt_event_data_get(*next_idx, &data, TRUE) == VTSS_RC_OK) {
            *next_data_idx = data.data_index;
            T_D("next_data_idx = %d", *next_data_idx);
        } else {
            return RMON_ERROR_EVENT_ENTRY_NOT_FOUND;
        }
    } else {
        return RMON_ERROR_EVENT_ENTRY_NOT_FOUND;
    }
    T_D("VTSS_RC_OK");
    return VTSS_RC_OK;

}
/* }---------------------***********------------------------*/
