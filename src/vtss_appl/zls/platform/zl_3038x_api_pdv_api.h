/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _ZL_30380_API_PDV_API_H_
#define _ZL_30380_API_PDV_API_H_

#include "main.h"
#include "main_types.h"
// #include "zl303xx_DataTypes.h"  Cannot be included here since this file (zl_3038x_api_pdv_api.h) is also included in ptp.c, but zl303xx_DataTypes.h is at the moment not released as a source file.
//
typedef unsigned short Uint16T;
#include "vtss_ptp_slave.h"
#include "synce_types.h"
#include "zl303xx_DeviceSpec.h"

#ifdef __cplusplus
extern "C" {
#endif

void zl_3038x_api_version_check();
vtss_zarlink_servo_t zl_3038x_module_type();

mesa_rc zl_3038x_pdv_init(vtss_init_data_t *data);

/* create timing server and add it to the CGU */
mesa_rc apr_stream_create(u32 domain, Uint16T serverId, u32 filter_type, bool hybrid_init, i8 ptp_rate);

/* Remove timing server from the CGU (and destroy it) */
mesa_rc apr_stream_remove(u32 domain, Uint16T serverId);

/* Set active timing server */
mesa_rc apr_set_active_timing_server(zl303xx_ParamsS *zl303xx_Params, Uint16T serverId);

/* Utility function to dump some of a timing servers 1Hz config to CLI */
void apr_show_server_1hz_config(zl303xx_ParamsS *zl303xx_Params, Uint16T serverId);

/**
 * \brief Process timestamps received in the PTP protocol.
 *
 * \param tx_ts [IN]  transmitted timestamp
 * \param rx_ts [IN]  received timestamp
 * \param corr  [IN]  received correction field
 * \param fwd_path [IN] true if forward path (Sync), false if reverse path (Delay_Req/Delat_Resp).
 * \param peer_delay [IN] true if using Peer delay Mechanism.
 * \return TRUE if success
 */
typedef struct {
    mesa_timestamp_t tx_ts;
    mesa_timestamp_t rx_ts;
    mesa_timeinterval_t corr;
    BOOL fwd_path;
    BOOL peer_delay;
    bool virtual_port;
} vtss_zl_30380_process_timestamp_t;

BOOL zl_30380_process_timestamp(u32 domain, u16 serverId, vtss_zl_30380_process_timestamp_t *ts);

/**
 * \brief Process packet rate indications received in the PTP protocol.
 *
 * \param serverId [IN]  serverId of the timing server for which the values shall be set
 * \param ptp_rate [IN]  ptp packet rate = 2**-ptp_rate packets pe sec, i.e. ptp_rate = -7 => 128 packets pr sec.
 * \param forward  [IN]  True if forward rate (Sync packets), False if reverse rate (Delay_Req)
  * \return TRUE if success
 */
BOOL  zl_30380_packet_rate_set(u32 domain, u16 serverId, i8 ptp_rate, BOOL forward, BOOL virtual_port);

// typedef struct
// {
//    /*!  Var: zl303xx_AprDeviceStatusIndicatorsS::bFreqLockFlag
//             Flag to indicate if the client clock is frequency locked with the reference clock. */
//    BOOL L1;
//    BOOL L2;
//    BOOL L3;
//    BOOL L;
//    BOOL gstL;
//    BOOL V;
//    BOOL gstV;
//    BOOL S;
//    BOOL U;
//    BOOL U1;
//    BOOL PE;
//    BOOL PA;
//    BOOL gstPA;
//    BOOL H;
//    BOOL gstH;
//    BOOL OOR;
//    BOOL ttErrDetected;
//    BOOL outageDetected;
//    BOOL outlierDetected;
//    BOOL rrDetected;
//    BOOL stepDetected;
//    BOOL pktLossDetectedFwd;
//    BOOL pktLossDetectedRev;
//    zl303xx_AprAlgPathFlagE algPathFlag;
//    zl303xx_AprAlgPathStateE algFwdState;
//    zl303xx_AprAlgPathStateE algRevState;
//
//    zl303xx_AprStateE state;
// } zl303xx_AprServerStatusFlagsS;

#define ZL_3034X_PDV_INITIAL           0
#define ZL_3034X_PDV_FREQ_LOCKING      1
#define ZL_3034X_PDV_PHASE_LOCKING     2
#define ZL_3034X_PDV_FREQ_LOCKED       3
#define ZL_3034X_PDV_PHASE_LOCKED      4
#define ZL_3034X_PDV_PHASE_HOLDOVER    5

BOOL zl_30380_pdv_status_get(u32 domain, u16 serverId, vtss_slave_clock_state_t *pdv_clock_state, i32 *freq_offset);
mesa_rc zl_30380_pdv_apr_server_status_get(u16 cguId, u16 serverId, char **s);

#define ZL_TOP_ISR_REF_TS_ENG         0x00000001  /* DCO Phase Word and Local System Time have been sampled */

typedef u32 zl_30380_event_t;

mesa_rc zl_30380_event_enable_set(zl_30380_event_t events, BOOL enable);

mesa_rc zl_30380_event_poll(zl_30380_event_t *const events);


mesa_rc zl_30380_apr_device_status_get(zl303xx_ParamsS *zl303xx_Params);
mesa_rc zl_30380_apr_server_config_get(zl303xx_ParamsS *zl303xx_Params);
mesa_rc zl_30380_apr_server_status_get(zl303xx_ParamsS *zl303xx_Params);
mesa_rc zl_30380_apr_force_holdover_set(u32 domain, BOOL enable);
mesa_rc zl_30380_apr_statistics_get(zl303xx_ParamsS *zl303xx_Params);
mesa_rc zl_30380_apr_statistics_reset(zl303xx_ParamsS *zl303xx_Params);
mesa_rc apr_server_notify_set(BOOL enable);
mesa_rc apr_server_notify_get(BOOL *enable);
mesa_rc zl_30380_apr_log_level_set(u8 level);
mesa_rc zl_30380_apr_log_level_get(u8 *level);
mesa_rc zl_30380_apr_ts_log_level_set(u8 level);
mesa_rc zl_30380_apr_ts_log_level_get(u8 *level);
mesa_rc apr_server_one_hz_set(zl303xx_ParamsS *zl303xx_Params, BOOL enable);
mesa_rc apr_server_one_hz_get(BOOL *enable);
mesa_rc zl_30380_apr_config_dump(u16 cguId);

mesa_rc zl_30380_apr_switch_to_packet_mode(u32 domain, int clock_inst);
mesa_rc zl_30380_apr_switch_to_hybrid_mode(u32 domain, int clock_inst);
mesa_rc zl_30380_apr_in_hybrid_mode(u32 domain, int clock_inst, BOOL *state);

mesa_rc zl_30380_apr_set_active_ref(u32 domain, int stream);
mesa_rc zl30380_apr_set_active_elec_ref(u32 domain, int input);

mesa_rc zl_30380_apr_set_hybrid_transient(u32 domain, int clock_inst, vtss_ptp_hybrid_transient transient);

mesa_rc zl_30380_apr_set_log_level(i32 level);
mesa_rc zl_30380_apr_show_psl_fcl_config(u16 cguId);
mesa_rc zl_30380_apr_show_statistics(u16 cguId, u32 stati);

mesa_rc zl_3038x_servo_dpll_config(int clock_option, bool generic);
bool zl_3038x_servo_dpll_config_generic(void);

mesa_rc zl_30380_holdover_set(u32 domain, int clock_inst, BOOL enable);

bool zl_3038x_get_type2b_enabled();
void zl_3038x_srvr_holdover_param_get(u32 cgu, u32 server, bool *holdover_ok, i64 *hold_val);
mesa_rc zl_30380_apr_dbg_cmds(u16 cguId, u16 serverId, u16 cmd, int val);

mesa_rc zl_30380_virtual_port_device_set(uint16_t instance, uint32_t domain, bool enable);
mesa_rc zl_30380_apr_switch_1pps_virtual_reference(uint16_t instance, uint32_t domain, bool enable, bool warm_start);
#ifdef __cplusplus
}
#endif
#endif // _ZL_30380_API_PDV_API_H_

