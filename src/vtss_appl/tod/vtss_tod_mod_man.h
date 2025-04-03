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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef _VTSS_TOD_MOD_MAN_H_
#define _VTSS_TOD_MOD_MAN_H_
#include "main_types.h"
#include "microchip/ethernet/switch/api.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief Module manager thread trigger upon the 1 PPS from the master timecounter.
 *
 * \param ongoing_adj [IN]  TRUE if the master counter setting is ongoing
 *
 * \return Return code.
 **/
mesa_rc tod_mod_man_trig(BOOL ongoing_adj);

/**
 * \brief Module manager initialization, i.e. create task and initialize internal tables.
 *
 * \return Return code.
 **/
mesa_rc tod_mod_man_pre_init(void);

/**
 * \brief Module manager resume, i.e. resume task.
 *
 * \return Return code.
 **/
mesa_rc tod_mod_man_init(void);

/**
 * \brief Module manager update the internal tables for a 8488 PHY.
 *
 * \return Return code.
 **/
mesa_rc ptp_module_man_port_data_update(mesa_port_no_t port_no);

/**
 * \brief Module manager rate adjustment, i.e. adjust the clock rate for all slave timecounters.
 * \note this function is used when all slave timecounters have the same reference clock.
 *
 * \param adj [IN]  Clock ratio frequency offset in units of scaled ppb
 *                         (parts pr billion). ratio > 0 => clock runs faster. 
 *
 * \return Return code.
 **/
mesa_rc tod_mod_man_slave_freq_adjust(mepa_ts_scaled_ppb_t *adj);

/* forward declaration */
typedef enum  {
    VTSS_TS_TIMECOUNTER_PHY,
    VTSS_TS_TIMECOUNTER_JR1,
    VTSS_TS_TIMECOUNTER_L26,
} vtss_ts_timecounter_type_t;

/**
 * \brief Module manager enable/disable a slave timecounter.
 *
 * \param port_no [IN]  Port number to Enable or Disable.
 * \param enable  [IN]  TRUE => enable, FALSE => disable 
 *
 * \return Return code.
 **/
mesa_rc tod_mod_man_force_slave_state(mesa_port_no_t port_no, BOOL enable);


/**
 * \brief Module manager add an signature entry.
 *
 * \param alloc_parm [IN]  Timestamping allocation parameters.
 * \param ts_sig     [IN]  Timestamping signature 
 *
 * \return Return code.
 **/
mesa_rc tod_mod_man_tx_ts_allocate(const mesa_ts_timestamp_alloc_t *const alloc_parm,
                                               const mepa_ts_fifo_sig_t    *const ts_sig);

/*
 * in_sync state callback
 */
typedef void (*vtss_module_man_in_sync_cb_t)(mesa_port_no_t port_no, /* Port */
                                            BOOL in_sync);  /* true if port is in sync   */

/**
 * \brief Module manager add a in-sync state callback.
 *
 * \param in_sync_cb [IN]  function pointer to a callback function.
 *
 * \return Return code.
 **/
mesa_rc tod_mod_man_tx_ts_in_sync_cb_set(const vtss_module_man_in_sync_cb_t in_sync_cb);

/*
 * Timestamp PHY analyzer topology.
 */
typedef enum {
    VTSS_PTP_TS_NONE,      /* port has no timestamp feature  */
    VTSS_PTP_TS_PTS,       /* port has PHY timestamp feature */
} vtss_ptp_timestamp_feature_t;

/*
 * Timestamp PHY analyzer Generation.
 */
typedef enum {
    VTSS_PTP_TS_GEN_NONE,  /* port has no timestamp feature  */
    VTSS_PTP_TS_GEN_1,     /* port has PHY timestamp feature generation 1*/
    VTSS_PTP_TS_GEN_2,     /* port has PHY timestamp feature generation 2*/
    VTSS_PTP_TS_GEN_3,     /* port has PHY timestamp feature of lan8814(Indy) type */
} vtss_ptp_timestamp_generation_t;

typedef struct vtss_tod_ts_phy_topo_t  {
    vtss_ptp_timestamp_feature_t ts_feature;    /* indicates pr port which timestamp feature is available */
    vtss_ptp_timestamp_generation_t ts_gen;     /* indicates which generation timestamp feature is available */
    u16 channel_id;                             /* identifies the channel id in the PHY
                                                   (needed to access the timestamping feature) */
    BOOL port_shared;                           /* port shares engine with another port */
    mesa_port_no_t shared_port_no;              /* the port this engine is shared with. */
    BOOL port_ts_in_sync;                       /* false if the port has PHY timestamper, 
                                                    and timestamper is not in sync with master timer and no time adjustment is ongoing,otherwise TRUE */
} vtss_tod_ts_phy_topo_t ;

/* Internal slave LTC state */
typedef enum {MOD_MAN_NOT_STARTED, MOD_MAN_DISABLED, MOD_MAN_READ_RTC_FROM_SWITCH, MOD_MAN_SET_ABS_TOD, MOD_MAN_SET_ABS_TOD_DONE, MOD_MAN_READ_RTC_FROM_PHY_AND_COMPARE } mod_man_state_t;

typedef enum {MOD_MAN_NOT_INSTALLED, MOD_MAN_INSTALLED_BUT_NOT_RUNNING, MOD_MAN_INSTALLED_AND_RUNNING } mod_man_installed_t;

void tod_mod_man_port_data_get(mesa_port_no_t port_no,
                         vtss_tod_ts_phy_topo_t *phy);

/**
 * \brief Module manager set 1PPS output latency.
 *
 * \param latency [IN]  The latency of the master 1PPS output compared to the start of a one sec period.
 * \return Return code.
 **/
mesa_rc vtss_module_man_master_1pps_latency(const mesa_timeinterval_t latency);

// Create queue for two-step clock's PHY FIFO timestamps.
mesa_rc tod_mod_man_tx_ts_queue_init(const mesa_port_no_t port);
// Delete queue used for storing PHY's FIFO timestamps.
mesa_rc tod_mod_man_tx_ts_queue_deinit(const mesa_port_no_t port);

mesa_rc tod_mod_man_port_status_get(mesa_port_no_t port_no, mepa_timestamp_t *slave_time, mepa_timestamp_t *next_pps, i64 *skew, bool *in_sync, mod_man_state_t *state, uint32_t *oos_cnt, int64_t *oos_skew);
mesa_rc tod_mod_man_gen_status_get(int *running, int *remain_time_until_start, int *max_skew);

mesa_rc tod_mod_man_time_set(const mesa_timestamp_t *ts);
mesa_rc tod_mod_man_time_step(const mesa_timestamp_t *in, BOOL negative);
bool tod_mod_man_time_settling();
mesa_rc tod_mod_man_settling_time_ts_proc(uint64_t hw_time, mesa_timestamp_t *ts);
mesa_rc tod_mod_man_time_fine_adj(int offset, BOOL neg);

#ifdef __cplusplus
}
#endif
#endif // _VTSS_TOD_MOD_MAN_H_

