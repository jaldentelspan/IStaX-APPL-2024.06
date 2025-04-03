/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __THREAD_LOAD_MONITOR_API_H__
#define __THREAD_LOAD_MONITOR_API_H__

#include "main_types.h"
#include <vtss/basics/map.hxx>

/**
 * Thread Load Monitor module error codes (mesa_rc)
 */
enum {
    THREAD_LOAD_MONITOR_RC_TIMER_START = MODULE_ERROR_START(VTSS_MODULE_ID_THREAD_LOAD_MONITOR), /**< Unable to start timer (Linux) */
    THREAD_LOAD_MONITOR_RC_TIMER_STOP,                                                           /**< Unable to stop timer (Linux) */
}; // Leave it anonymous

/**
 * \file Thread Load Monitor
 *
 * Module that allows for monitoring the 1-sec and 10-sec
 * thread load measured in percent of the total CPU time.
 */

/**
 * Start monitoring the thread load.
 *
 * The thread load monitor puts additional load on the CPU,
 * which is why its startable and stoppable.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt() to
 * get a textual representation.
 */
mesa_rc thread_load_monitor_start(void);

/**
 * Stop monitoring the thread load.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt() to
 * get a textual representation.
 */
mesa_rc thread_load_monitor_stop(void);

/**
 * Structure for holding 1 and 10 second loads.
 * The two load members must be interpreted as follows:
 * They can be a number in range ]0; 10000], where
 * 10000 indicates 100.0% load.
 */
typedef struct {
    /**
     * CPU load during last one second
     */
    u16 one_sec_load;

    /**
     * CPU load during last ten seconds
     */
    u16 ten_sec_load;
} thread_load_monitor_load_t;

/**
 * Get the one- and ten-sec loads.
 *
 * The map is keyed by thread ID. Use map::find() to get the load
 * for a given thread ID. If ::find() returns map::end(), it's because the load
 * has been 0% during the monitored period for that thread ID.
 * Thread ID -1 indicates time spent in the Idle task
 * Thread ID 0 indicates time spent by other processes (hereunder interrupts).
 * These special ones are always present.
 *
 * \param load          [OUT] Map of thread IDs to thread loads monitored over the last 1 and 10 seconds.
 * \param started       [OUT] Is true if thread load monitoring is currently started, false otherwise.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt() to
 * get a textual representation.
 */
mesa_rc thread_load_monitor_load_get(vtss::Map<int, thread_load_monitor_load_t> &load, bool &started);

/**
 * Structure for holding 1 and 10 second page faults.
 */
typedef struct {
    /**
     * Average number of page faults during last one second
     */
    u32 one_sec;

    /**
     * Average number of page faults during last 10 seconds
     */
    u32 ten_sec;

    /**
     * Total number of page faults since boot
     */
    u32 total;
} thread_load_monitor_page_faults_t;

/**
 * Get the one- and ten-sec page faults.
 *
 * \param page_faults   [OUT] Number of page faults
 * \param started       [OUT] Is true if thread load monitoring is currently started, false otherwise.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt() to
 * get a textual representation.
 */
mesa_rc thread_load_monitor_page_faults_get(thread_load_monitor_page_faults_t &page_faults, bool &started);

/**
 * Structure for holding 1 and 10 second context switches.
 */
typedef struct {
    /**
     * Average number of context switches during last one second
     */
    u64 one_sec;

    /**
     * Average number of context switches during last 10 seconds
     */
    u64 ten_sec;

    /**
     * Total number of context switch since boot
     */
    u64 total;
} thread_load_monitor_context_switches_t;

/**
 * Get the one- and ten-sec context switches.
 *
 * The map is keyed by thread ID. Use map::find() to get the number of context
 * switches for a given thread ID.
 * Thread ID 0 contains the total number of context switches and is always
 * present.
 *
 * \param context_switches [OUT] Map of thread IDs to context switches monitored over the last 1 and 10 seconds.
 * \param started          [OUT] Is true if thread load monitoring is currently started, false otherwise.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt() to
 * get a textual representation.
 */
mesa_rc thread_load_monitor_context_switches_get(vtss::Map<int, thread_load_monitor_context_switches_t> &context_switches, bool &started);

/**
 * Internal function to initialize the module.
 *
 * \param data [IN] Pointer to state
 *
 * \return VTSS_RC_OK unless something serious is wrong.
 */
mesa_rc thread_load_monitor_init(vtss_init_data_t *data);

/**
 * Function for converting a thread load monitor module error
 * (see THREAD_LOAD_MONITOR_RC_xxx above) to a textual string.
 * Only errors in the Thread Load Monitor module's range can
 * be converted.
 *
 * \param rc [IN] Binary form of error
 *
 * \return Static string containing textual representation of #rc.
 */
const char *thread_load_monitor_error_txt(mesa_rc rc);

#endif /* !defined(__THREAD_LOAD_MONITOR_API_H__) */

