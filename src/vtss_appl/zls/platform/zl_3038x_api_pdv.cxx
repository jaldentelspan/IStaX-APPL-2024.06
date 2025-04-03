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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "assert.hxx"
#include "main.h"
#include "main_types.h"
#include "microchip/ethernet/switch/api.h"
#include "zl_3038x_api_pdv.h"
#include "zl_3038x_api_pdv_api.h"
#include "critd_api.h"
#include "unistd.h" // for sleep function

// zl30380 base interface
#include "zl303xx_DeviceIf.h"
#include "zl303xx_Error.h"
#include "zl303xx_Apr.h"
#include "zl303xx_LogToMsgQ.h"
#include "zl303xx_ExampleAprGlobals.h"
#include "zl303xx_ExampleUtils.h"
#include "zl303xx_ExampleAprBinding.h"
#include "zl303xx_DebugApr.h"
#include "zl303xx_ExampleApr.h"
#include "zl303xx_AprStatistics.h"
#include "zl303xx_Apr1Hz.h"
#include "zl303xx_Trace.h"
#include "interrupt_api.h"

#undef min
#undef max

/* interface to other components */
#include "vtss_tod_api.h"
#include "vtss_ptp_local_clock.h"
#include "vtss/appl/synce.h"
#include "synce_custom_clock_api.h"
#include "vtss_ptp_slave.h"
#if defined ZLS30771_INCLUDED
#include "zl303xx_Dpll77x.h"
#include "zl303xx_Dpll771.h"
#endif //ZLS30771_INCLUDED
#if defined ZLS30731_INCLUDED
#include "zl303xx_Dpll73xConfig.h"
#endif //ZLS30731_INCLUDED

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_ZL_3034X_PDV
#define PDV_NO_OF_DOMAINS 4
#define PTP_INSTANCES_NUM 4
zl303xx_ParamsS *zl303xx_Params_dpll;        // Pointers to a zl303xx_ParamsS structure associated with the primary SyncE DPLL chip.
                                             // Usually, this pointer is initialized by the driver for the zls30343, zls30363, silabs or ServalT DPLL.

zl303xx_ParamsS *zl303xx_Params_generic[PDV_NO_OF_DOMAINS];  // Index 0-3: These are pointers to zl303xx_ParamsS structures representing timing doimains that are not associated
                                             //            with a physical DPLL (or in case of index 0 a physical DPLL like the zls30343 being controlled in
                                             //            generic mode). The domains may be using a software emulation or a capability in hardware to set an
                                             //            offset relative to the domain controlled by the SyncE DPLL.
static zl303xx_ParamsS *zl303xx_Params[PDV_NO_OF_DOMAINS];          // Index 0-3: These are pointers to zl303xx_ParamsS of the actual domains.

// these two variables are needed because the APR code uses a global variable to store the actual DPLL type, therefore we need to save the
// two alternative DPLL types that we can switch between when we change the preferred adjustment method.
// this method only partly solves the problem, i.e. it is solved for domain 0, but if domain 0 uses an other DPLL type than other domains we
// still have the problem.
// The right solution is to avoid using the global variable, but there is 396 regerences to this vatiable in the APR code, provided by TIM.
static zl303xx_DevTypeE default_dpll_type;
static zl303xx_DevTypeE alternative_dpll_type;

static zl303xx_ParamsS *zl_virtual_param; // zl pointer to generic dpll used for virtual port.

#define CGUID_CHECK(expr) { if (expr >= PDV_NO_OF_DOMAINS) return VTSS_RC_ERROR; }

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */


static vtss_trace_reg_t trace_reg = {
    VTSS_MODULE_ID_ZL_3034X_PDV, "zl_3038x", "ZL3038x Filter algorithm Module."
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default (ZL30380 core)",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC
    },
    [TRACE_GRP_PTP_INTF] = {
        "ptp_intf",
        "ZL-PTP application interface functions",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [TRACE_GRP_OS_PORT] = {
        "os_port",
        "ZL-PTP application OS portability layer",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [TRACE_GRP_ZL_TRACE] = {
        "zl_trace",
        "ZL-PTP application zl trace",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#include "synce_spi_if.h" // Note: Must be included after the trace definitions as it depends on these.

static struct {
    BOOL ready;                 /* ZL3034X Initited  */
    critd_t datamutex;          /* Global data protection */
} zl30380_global;

/* ================================================================= *
 *  Persistent Configuration definitions
 * ================================================================= */

typedef struct zl30380_config_t {
    i8 version;             /* configuration version, to be changed if this structure is changed */
    zl303xx_AprAlgTypeModeE algTypeMode;
    zl303xx_AprOscillatorFilterTypesE oscillatorFilter;
    zl303xx_AprFilterTypesE filter;
    zl303xx_BooleanE enable1Hz;
    zl303xx_BooleanE bHoldover;
    BOOL apr_server_notify_flag;
    u32 mode;
    u32 adj_freq_min_phase;
} zl30380_config_t;

static zl30380_config_t config_data ;

static int my_clock_option;
static bool my_generic;
static vtss_ptp_hybrid_transient curTransient = VTSS_PTP_HYBRID_TRANSIENT_NOT_ACTIVE;
static int enable_ts = 1;
static bool g8275_profile = false; // needed to avoid transient response when no 8275 is configured.

/**
 * Read the PTP configuration.
 * \param create indicates a new default configuration block should be created.
 *
 */
static void zl30380_conf_read(BOOL create)
{
    ZL_3036X_DATA_LOCK();
    /* initialize run-time options to reasonable values */
    config_data.version = ZL30380_CONF_VERSION;
    config_data.algTypeMode = ZL303XX_NATIVE_PKT_FREQ;
    config_data.oscillatorFilter = ZL303XX_OCXO_S3E;
    config_data.filter = ZL303XX_BW_0_FILTER;
    config_data.enable1Hz = ZL303XX_FALSE;
    config_data.bHoldover = ZL303XX_FALSE;
    config_data.apr_server_notify_flag = FALSE;
    config_data.mode = 1;
    config_data.adj_freq_min_phase = 20; /* zl default value */
    ZL_3036X_DATA_UNLOCK();
}

/* ================================================================= *
 *  Configuration definitions
 * ================================================================= */

static zl303xx_AprAddServerS server[5];  // Support for 4 servers i.e. PTP streams + 1 virtual port.
static zl303xx_AprAddDeviceS dpll_dev, gen_dev[3];
u8 multi_inst = 0; // => clock domain 0
const uint16_t virt_server_id = 4;

// Instance configuration
typedef struct zls_instance {
    uint16_t id;
    bool vport_config;     // virtual port configured or not.
    bool vport_active;     // virtual port active or not.
    uint16_t active_server_id;
    exampleAprConfigIdentifiersE filter_mode;
    exampleAprConfigIdentifiersE virtual_port_filter;

    void init(uint16_t instance) {
        id               = instance;
        active_server_id = instance;
        filter_mode      = ACI_BASIC_PHASE_LOW;
    }

    void set_virtual_port(bool enable) {
        vport_active = enable;
    }
    // When virtual port server id 4 is used for stream creation, issues are seen with servo. To avoid them, for virtual port, either 0 or 1 stream-ids are used.
    // return virtual port server id
    uint32_t get_virtual_port_server() {
        return !id ? 1 : 0;
    }
    // returns active server-id. Chooses between virtual port and normal port.
    uint32_t active_server() {
        uint32_t stream_id = id;
        if (vport_active) {
            // This is the virtual port stream id used while registering with APR server.
            stream_id = !id ? 1 : 0;
        }
        return stream_id;
    }
    // returns server-id of packet stream. Same as instance id.
    uint32_t get_packet_server() {
        return id;
    }
    inline bool is_virtual_port_active() {
        return vport_active;
    }
    inline bool is_virtual_port_configured() {
        return vport_config;
    }
    inline void set_virtual_port_config(bool ena) {
        vport_config = ena;
    }
} vtss_zls_instance_cfg_t;
static vtss_zls_instance_cfg_t instance_cfg[4];  // Support for 4 PTP instances.
// Active PTP instance ids or server ids configured for clock domain.
static int active_instance[3] = {-1, -1, -1}; //utmost 3 clock domains possible.

static const char *apr_alg_type_2_txt(zl303xx_AprAlgTypeModeE value)
{
    switch (value) {
        case ZL303XX_NATIVE_PKT_FREQ                   : return "NATIVE_PKT_FREQ";
        case ZL303XX_NATIVE_PKT_FREQ_UNI               : return "NATIVE_PKT_FREQ_UNI";
        case ZL303XX_NATIVE_PKT_FREQ_CES               : return "NATIVE_PKT_FREQ_CES";
        case ZL303XX_NATIVE_PKT_FREQ_ACCURACY          : return "NATIVE_PKT_FREQ_ACCURACY";
        case ZL303XX_NATIVE_PKT_FREQ_ACCURACY_UNI      : return "NATIVE_PKT_FREQ_ACCURACY_UNI";
        case ZL303XX_NATIVE_PKT_FREQ_FLEX              : return "NATIVE_PKT_FREQ_FLEX";
        case ZL303XX_BOUNDARY_CLK                      : return "BOUNDARY_CLK";
        case ZL303XX_NATIVE_PKT_FREQ_ACCURACY_FDD      : return "NATIVE_PKT_FREQ_ACCURACY_FDD";
        case ZL303XX_XDSL_FREQ_ACCURACY                : return "XDSL_FREQ_ACCURACY";
        case ZL303XX_CUSTOM_FREQ_ACCURACY_200          : return "CUSTOM_FREQ_ACCURACY_200";
        case ZL303XX_CUSTOM_FREQ_ACCURACY_15           : return "CUSTOM_FREQ_ACCURACY_15";
    }
    return "INVALID";
}

static const char *apr_osc_filter_type_2_txt(zl303xx_AprOscillatorFilterTypesE value)
{
    switch (value) {
        case ZL303XX_TCXO                   : return "TCXO";
        case ZL303XX_TCXO_FAST              : return "TCXO_FAST";
        case ZL303XX_OCXO_S3E               : return "OCXO_S3E";
        case ZL303XX_OCXO_S3E_C             : return "OCXO_S3E_C";
        case ZL303XX_OCXO_S3E_DEPRECATED    : return "OCXO_S3E_DEP";
        case ZL303XX_OCXO_S3E_C_DEPRECATED  : return "OCXO_S3E_C_DEP";
        default                             : return "INVALID";
    }
}

static const char *apr_filter_type_2_txt(zl303xx_AprFilterTypesE value)
{
    switch (value) {
        case ZL303XX_BW_0_FILTER      : return "BW_0_FILTER";
        case ZL303XX_BW_1_FILTER      : return "BW_1_FILTER";
        case ZL303XX_BW_2_FILTER      : return "BW_2_FILTER";
        case ZL303XX_BW_3_FILTER      : return "BW_3_FILTER";
        case ZL303XX_BW_4_FILTER      : return "BW_4_FILTER";
        case ZL303XX_BW_5_FILTER      : return "BW_5_FILTER";
        case ZL303XX_BW_6_FILTER      : return "BW_6_FILTER";
        case ZL303XX_BW_7_FILTER      : return "BW_7_FILTER";
        case ZL303XX_BW_8_FILTER      : return "BW_8_FILTER";
        case ZL303XX_BW_700mHz        : return "BW_700mHz  ";
        case ZL303XX_BW_500mHz        : return "BW_500mHz  ";
        case ZL303XX_BW_300mHz        : return "BW_300mHz  ";
        case ZL303XX_BW_100mHz        : return "BW_100mHz  ";
        case ZL303XX_BW_90mHz         : return "BW_90mHz   ";
        case ZL303XX_BW_75mHz         : return "BW_75mHz   ";
        case ZL303XX_BW_30mHz         : return "BW_30mHz   ";
        case ZL303XX_BW_10mHz         : return "BW_10mHz   ";
        case ZL303XX_BW_3mHz          : return "BW_3mHz    ";
        case ZL303XX_BW_1mHz          : return "BW_1mHz    ";
        case ZL303XX_BW_300uHz        : return "BW_300uHz  ";
        case ZL303XX_BW_100uHz        : return "BW_100uHz  ";
        case ZL303XX_BW_33uHz         : return "BW_33uHz   ";
        case ZL303XX_BW_23uHz         : return "BW_23uHz   ";
        case ZL303XX_BW_700uHz        : return "BW_700uHz  ";
        default                       : return "INVALID";
    }
}

/****************************************************************************
 * Configuration API
 ****************************************************************************/
#ifdef VTSS_SW_OPTION_ZLS3077X
static zlStatusE zl3077x_HandleHybrid_Transient(zl303xx_ParamsS *ptp_dpll, zl303xx_BCHybridTransientType transient)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_ParamsS *nco_asst_dpll = NULL;
    static ZLS3077X_DpllHWModeE oldDpllMode = ZLS3077X_DPLL_MODE_REFLOCK;

    zl303xx_Dpll77xGetNCOAssistParamsSAssociation(ptp_dpll,&nco_asst_dpll);
    if (nco_asst_dpll != NULL) {
        if( transient == ZL303XX_BHTT_QUICK ) {
            zl303xx_Dpll77xMitigationEnabledSet(ptp_dpll,ZL303XX_TRUE);
            if( zl303xx_Dpll77xModeGet(nco_asst_dpll, &oldDpllMode) == ZL303XX_OK )
            {
                zl303xx_Dpll77xModeSet(nco_asst_dpll,ZLS3077X_DPLL_MODE_HOLDOVER);
            }
            zl303xx_Dpll77xBCHybridActionOutOfLock(ptp_dpll);
        } else if( transient == ZL303XX_BHTT_NOT_ACTIVE ) {
            status = zl303xx_Dpll77xModeSet(nco_asst_dpll, oldDpllMode);
            (void)zl303xx_Dpll77xMitigationEnabledSet(ptp_dpll, ZL303XX_FALSE);
        }
    } else {
        T_I("NCO assist has not yet configured ");
    }

    return status;
}
#endif //VTSS_SW_OPTION_ZLS3077X
static zlStatusE zl3xxAprHandleHybridTransient
         (
         zl303xx_ParamsS *ptp_dpll,
         zl303xx_BCHybridTransientType transient
         )
{
    zlStatusE status = ZL303XX_OK;
#ifdef VTSS_SW_OPTION_ZLS3077X
    if (ptp_dpll->deviceType == ZL3077X_DEVICETYPE) {
        status = zl3077x_HandleHybrid_Transient(ptp_dpll, transient);
    }
#endif //VTSS_SW_OPTION_ZLS3077X
    // for 361 dpll, when common mode(ptp dpll + synce dpll) is implemented, it needs to be added.
    return status;
}
static exampleAprConfigIdentifiersE filter_type_2_mode(u32 filter_type);

/*
 * Process timestamps received in the PTP protocol.
 */
BOOL zl_30380_process_timestamp(u32 domain, u16 instance, vtss_zl_30380_process_timestamp_t *ts)
{
    zl303xx_AprTimestampS aprTs;
    zlStatusE status = ZL303XX_OK;
    BOOL rc = TRUE;
    // Currently, apart from ptp packet server, only virtual port server is configured as alternate server.
    uint16_t serverId = ts->virtual_port ? instance_cfg[instance].get_virtual_port_server() : instance_cfg[instance].get_packet_server();

    memset(&aprTs, 0, sizeof(aprTs));
#if 0  // problem with Zarlink corrfield
    if (ts->corr != 0LL) {
        vtss_tod_add_TimeInterval(&ts->tx_ts, &ts->tx_ts, &ts->corr);
        ts->corr = 0LL;
    }
#endif
    T_IG(TRACE_GRP_PTP_INTF, "domain %d, serverId %d", domain, serverId);
    aprTs.serverId = serverId;
    aprTs.txTs.second.lo = ts->tx_ts.seconds;
    aprTs.txTs.second.hi = ts->tx_ts.sec_msb;
    aprTs.txTs.subSecond = ts->tx_ts.nanoseconds;
    aprTs.rxTs.second.lo = ts->rx_ts.seconds;
    aprTs.rxTs.second.hi = ts->rx_ts.sec_msb;
    aprTs.rxTs.subSecond = ts->rx_ts.nanoseconds;
    aprTs.bForwardPath = (zl303xx_BooleanE)ts->fwd_path;
    aprTs.corr.hi = ts->corr>>32;
    aprTs.corr.lo = ts->corr & 0xffffffff;
    aprTs.bPeerDelay = (zl303xx_BooleanE)ts->peer_delay;
    aprTs.peerMeanDelay.hi = 0;
    aprTs.peerMeanDelay.lo = 0;
    aprTs.offsetFromMaster.hi = 0;
    aprTs.offsetFromMaster.lo = 0;
    aprTs.offsetFromMasterValid = ZL303XX_FALSE;
    T_NG(TRACE_GRP_PTP_INTF, "aprTs txTs %u:%u, rxTs %u:%u, fwd %d, corr %u:%u (%lld ns), peer_delay %d", aprTs.txTs.second.lo, aprTs.txTs.subSecond,
         aprTs.rxTs.second.lo, aprTs.rxTs.subSecond, aprTs.bForwardPath,
         aprTs.corr.hi, aprTs.corr.lo, ts->corr>>16, aprTs.bPeerDelay);
    status = enable_ts ? zl303xx_AprProcessTimestamp(zl303xx_Params[domain], &aprTs): ZL303XX_OK;
    if (status != ZL303XX_OK) {
        T_WG(TRACE_GRP_PTP_INTF, "zl303xx_AprProcessTimestamp failed with status = %u", status);
        rc = FALSE;
    }
    return rc;
}

mesa_rc zl_30380_apr_device_status_get(zl303xx_ParamsS *zl303xx_Params)
{
    mesa_rc rc = VTSS_RC_OK;

    if (zl303xx_GetAprDeviceStatus(zl303xx_Params) != ZL303XX_OK) {
        T_D("Error during Zarlink APR device status get");
        rc = VTSS_RC_ERROR;
    }
    return(rc);
}

mesa_rc zl_30380_apr_server_config_get(zl303xx_ParamsS *zl303xx_Params, Uint16T serverId)
{
    mesa_rc rc = VTSS_RC_OK;

    if (zl303xx_GetAprServerConfigInfo(zl303xx_Params, serverId) != ZL303XX_OK) {
        T_D("Error during Zarlink APR server config get");
        rc = VTSS_RC_ERROR;
    }
    return(rc);
}

mesa_rc zl_30380_apr_server_status_get(zl303xx_ParamsS *zl303xx_Params, Uint16T serverId)
{
    mesa_rc rc = VTSS_RC_OK;

    if (zl303xx_GetAprServerStatus(zl303xx_Params, serverId) != ZL303XX_OK) {
        T_D("Error during Zarlink APR server status get");
        rc = VTSS_RC_ERROR;
    }
    return(rc);
}

mesa_rc zl_30380_apr_force_holdover_set(u32 domain, BOOL enable)
{
    mesa_rc rc = VTSS_RC_OK;
    zlStatusE aperr;
    ZL_3036X_DATA_LOCK();
    if ((aperr = zl303xx_AprSetHoldover(zl303xx_Params[domain], enable ? ZL303XX_TRUE : ZL303XX_FALSE)) != ZL303XX_OK) {
        T_D("Error %d during Zarlink APR force holdover set", aperr);
        rc = VTSS_RC_ERROR;
    }
    ZL_3036X_DATA_UNLOCK();
    return(rc);
}

mesa_rc zl_30380_apr_statistics_get(zl303xx_ParamsS *zl303xx_Params)
{
    mesa_rc rc = VTSS_RC_OK;

    if (zl303xx_DebugGetAllAprStatistics(zl303xx_Params) != ZL303XX_OK) {
        T_D("Error during Zarlink APR Statistics get");
        rc = VTSS_RC_ERROR;
    }
    return(rc);
}

mesa_rc zl_30380_apr_statistics_reset(zl303xx_ParamsS *zl303xx_Params)
{
    mesa_rc rc = VTSS_RC_OK;

//    if (zl303xx_AprResetPerfStatistics(zl303xx_Params) != ZL303XX_OK) {
        T_D("Error during Zarlink APR Statistics reset");
        rc = VTSS_RC_ERROR;
//    }
    return(rc);
}

mesa_rc apr_server_one_hz_set(zl303xx_ParamsS *zl303xx_Params, BOOL enable)
{
    mesa_rc rc = VTSS_RC_OK;

    ZL_3036X_DATA_LOCK();
    config_data.enable1Hz = (zl303xx_BooleanE)enable;
    T_D("Set 1 Hz mode if %p != NULL",zl303xx_Params);
    if (zl303xx_Params != NULL) {
        if (zl303xx_AprSetDevice1HzEnabled(zl303xx_Params, (zl303xx_BooleanE)enable) != ZL303XX_OK) {
            rc = VTSS_RC_ERROR;
        }
    }
    ZL_3036X_DATA_UNLOCK();
    return(rc);
}

mesa_rc apr_server_one_hz_get(BOOL *enable)
{
    ZL_3036X_DATA_LOCK();
    *enable = config_data.enable1Hz;
    ZL_3036X_DATA_UNLOCK();
    return(VTSS_RC_OK);
}

mesa_rc zl_30380_apr_config_dump(u16 cguId)
{
    mesa_rc rc = VTSS_RC_OK;
    CGUID_CHECK(cguId);

    /* Debugging Api Calls */

    printf("### zl303xx_GetAprDeviceConfigInfo\n");
    if (ZL303XX_OK != zl303xx_GetAprDeviceConfigInfo(zl303xx_Params[cguId])) rc = VTSS_RC_ERROR;
    printf("### zl303xx_GetAprServerConfigInfo\n");
    if (ZL303XX_OK != zl303xx_GetAprServerConfigInfo(zl303xx_Params[cguId], 0)) rc = VTSS_RC_ERROR;
    printf("### zl303xx_DebugAprGet1HzData FWD Path\n");
    if (ZL303XX_OK != zl303xx_DebugAprGet1HzData(zl303xx_Params[cguId], 0, 0)) rc = VTSS_RC_ERROR;
    printf("### 1Hz data FWD ####\n");
    if (ZL303XX_OK != zl303xx_AprPrint1HzData(zl303xx_Params[cguId], 0, 0)) rc = VTSS_RC_ERROR;
    printf("### zl303xx_DebugAprGet1HzData  Rev Path\n");
    if (ZL303XX_OK != zl303xx_DebugAprGet1HzData(zl303xx_Params[cguId], 1, 0)) rc = VTSS_RC_ERROR;
    printf("### 1Hz data REV ####\n");
    if (ZL303XX_OK != zl303xx_AprPrint1HzData(zl303xx_Params[cguId], 1, 0)) rc = VTSS_RC_ERROR;
    printf("### Other Params ####\n");
    if (ZL303XX_OK != zl303xx_DebugPrintAprByReferenceId(zl303xx_Params[cguId], 0)) rc = VTSS_RC_ERROR;
    return(rc);
}

mesa_rc zl_30380_apr_log_level_set(u8 level)
{
    mesa_rc rc = VTSS_RC_OK;

    if (zl303xx_SetAprLogLevel(level) != ZL303XX_OK) {
        rc = VTSS_RC_ERROR;
    }
    return(rc);
}

mesa_rc zl_30380_apr_log_level_get(u8 *level)
{
    mesa_rc rc = VTSS_RC_OK;

    *level = zl303xx_GetAprLogLevel();
    return(rc);
}

mesa_rc zl_30380_apr_ts_log_level_set(u8 level)
{
    mesa_rc rc = VTSS_RC_OK;

    if (level <= 2) {
        zl303xx_AprLogTimestampInputStart(level);
    } else {
        zl303xx_AprLogTimestampInputStop();
    }
    return(rc);
}

mesa_rc zl_30380_apr_ts_log_level_get(u8 *level)
{
    mesa_rc rc = VTSS_RC_OK;
    *level = 3; // tbd (saved persistent)
    return(rc);
}
/*
 * Process packet rate indications received in the PTP protocol.
 */

static  zl303xx_AprPktRateE rate_table [] = {
          ZL303XX_128_PPS,
          ZL303XX_64_PPS,
          ZL303XX_32_PPS,
          ZL303XX_16_PPS,
          ZL303XX_8_PPS,
          ZL303XX_4_PPS,
          ZL303XX_2_PPS,
          ZL303XX_1_PPS,
          ZL303XX_1_PP_2S,
          ZL303XX_1_PP_4S,
          ZL303XX_1_PP_8S,
          ZL303XX_1_PP_16S};

BOOL zl_30380_packet_rate_set(u32 domain, u16 instance, i8 ptp_rate, BOOL forward, BOOL virtual_port)
{
    BOOL rc = TRUE;
    zl303xx_AprPktRateE fwdPktRate;
    zl303xx_AprPktRateE revPktRate;
    zl303xx_AprPktRateE new_rate = ZL303XX_1_PPS;
    zlStatusE status;
    uint16_t serverId = virtual_port ? instance_cfg[instance].get_virtual_port_server() : instance_cfg[instance].get_packet_server();

    if (ptp_rate == 0x7f) {
        if (forward) ptp_rate = -6;
        else ptp_rate = -4;
    }

    if (ptp_rate < -7 || ptp_rate > 4) {
        ptp_rate = 0;
    }

    if ((status = zl303xx_AprGetServerPktRate(zl303xx_Params[domain], serverId, &fwdPktRate, &revPktRate)) == ZL303XX_OK) {
        new_rate = rate_table[ptp_rate + 7];

        if ((forward && new_rate != fwdPktRate) || (!forward && new_rate != revPktRate)) {
            status = zl303xx_AprNotifyServerPktRateChange(zl303xx_Params[domain], serverId, (zl303xx_BooleanE)forward, new_rate);
            if (status == ZL303XX_OK) {
                T_IG(TRACE_GRP_PTP_INTF, "packet rate updated, rate = %d , forw = %d serverId %d", new_rate, (zl303xx_BooleanE)forward, serverId);
            } else {
                T_WG(TRACE_GRP_PTP_INTF, "zl303xx_AprNotifyServerPktRateChange failed with status = %u", status);
                rc = FALSE;
            }
        }
    } else {
        T_WG(TRACE_GRP_PTP_INTF, "zl303xx_AprGetServerPktRate failed with status = %u", status);
    }

    return rc;
}

BOOL zl_30380_pdv_status_get(u32 domain, u16 instance, vtss_slave_clock_state_t *pdv_clock_state, i32 *freq_offset)
{
    zl303xx_AprServerStatusFlagsS status_flags;
    uint16_t serverId = instance_cfg[instance].active_server();

    T_IG(TRACE_GRP_PTP_INTF, "instance %d server %d", instance, serverId);
    zlStatusE status = zl303xx_AprGetServerStatusFlags(zl303xx_Params[domain], serverId, &status_flags);
    if (status == ZL303XX_OK) {
        switch (status_flags.state) {
            case ZL303XX_FREQ_LOCK_ACQUIRING:   *pdv_clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING; break;
            case ZL303XX_FREQ_LOCK_ACQUIRED:    *pdv_clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED; break;
            case ZL303XX_PHASE_LOCK_ACQUIRED:   *pdv_clock_state = VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED; break;
            case ZL303XX_HOLDOVER:              *pdv_clock_state = VTSS_PTP_SLAVE_CLOCK_HOLDOVER; break;
            case ZL303XX_REF_FAILED:            *pdv_clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING; break;  // Note: In this case and the cases below, *pdv_clock_state is set to
            case ZL303XX_UNKNOWN:               *pdv_clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING; break;  //       VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING and not ZL303XX_REF_FAILED,
            case ZL303XX_MANUAL_FREERUN:        *pdv_clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING; break;  //       ZL303XX_UNKNOWN, ZL303XX_MANUAL_FREERUN, etc. as one would
            case ZL303XX_MANUAL_HOLDOVER:       *pdv_clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING; break;  //       expect. This is made so since vtss_ptp_slave may otherwise
            case ZL303XX_MANUAL_SERVO_HOLDOVER: *pdv_clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING; break;  //       stop generating delay_requests.
            case ZL303XX_NO_ACTIVE_SERVER:      *pdv_clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING; break;  //
            default:                            *pdv_clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING; break;  //
        }

        status = zl303xx_AprGetServerFreqOffset(zl303xx_Params[domain], freq_offset);
        T_NG(TRACE_GRP_PTP_INTF, "zl303xx_AprGetServerFreqOffset status = %u flags %u zl-ptr %p", status, status_flags.state, zl303xx_Params[domain]);
    } else {
        T_WG(TRACE_GRP_PTP_INTF, "zl303xx_AprGetServerStatusFlags failed with status = %u", status);
    }

    return (status == ZL303XX_OK) ? TRUE : FALSE;

}

mesa_rc  zl_30380_pdv_apr_server_status_get(u16 cguId, u16 instance, char **s)
{
    static char buf[600];
    zl303xx_AprServerStatusFlagsS status_flags;
    zlStatusE status;
    uint16_t serverId = instance_cfg[instance].active_server();
    mesa_rc  rc = VTSS_RC_OK;
    CGUID_CHECK(cguId);

    status = zl303xx_AprGetServerStatusFlags(zl303xx_Params[cguId], serverId, &status_flags);
    if (status != ZL303XX_OK) {
        T_WG(TRACE_GRP_PTP_INTF, "zl303xx_AprGetServerStatusFlags failed with status = %u", status);
        sprintf(buf, "Error reading APR server status.\n");
        rc = VTSS_RC_ERROR;
    }
    else {
        int n = 0;
        n += sprintf(&buf[n], "L1 = %s\n", (status_flags.L1 ? "True" : "False"));
        n += sprintf(&buf[n], "L2 = %s\n", (status_flags.L2 ? "True" : "False"));
        n += sprintf(&buf[n], "L3 = %s\n", (status_flags.L3 ? "True" : "False"));
        n += sprintf(&buf[n], "L = %s\n", (status_flags.L ? "True" : "False"));
        n += sprintf(&buf[n], "gstL = %s\n", (status_flags.gstL ? "True" : "False"));
        n += sprintf(&buf[n], "V = %s\n", (status_flags.V ? "True" : "False"));
        n += sprintf(&buf[n], "gstV = %s\n", (status_flags.gstV ? "True" : "False"));
        n += sprintf(&buf[n], "S = %s\n", (status_flags.S ? "True" : "False"));
        n += sprintf(&buf[n], "U = %s\n", (status_flags.U ? "True" : "False"));
        n += sprintf(&buf[n], "U1 = %s\n", (status_flags.U1 ? "True" : "False"));
        n += sprintf(&buf[n], "PE = %s\n", (status_flags.PE ? "True" : "False"));
        n += sprintf(&buf[n], "PA = %s\n", (status_flags.PA ? "True" : "False"));
        n += sprintf(&buf[n], "gstPA = %s\n", (status_flags.gstPA ? "True" : "False"));
        n += sprintf(&buf[n], "H = %s\n", (status_flags.H ? "True" : "False"));
        n += sprintf(&buf[n], "gstH = %s\n", (status_flags.gstH ? "True" : "False"));
        n += sprintf(&buf[n], "OOR = %s\n", (status_flags.OOR ? "True" : "False"));
        n += sprintf(&buf[n], "ttErrDetected = %s\n", (status_flags.ttErrDetected ? "True" : "False"));
        n += sprintf(&buf[n], "outageDetected = %s\n", (status_flags.outageDetected ? "True" : "False"));
        n += sprintf(&buf[n], "outlierDetected = %s\n", (status_flags.outlierDetected ? "True" : "False"));
        n += sprintf(&buf[n], "frrDetected = %s\n", (status_flags.frrDetected ? "True" : "False"));
        n += sprintf(&buf[n], "rrrDetected = %s\n", (status_flags.rrrDetected ? "True" : "False"));
        n += sprintf(&buf[n], "stepDetected = %s\n", (status_flags.stepDetected ? "True" : "False"));
        n += sprintf(&buf[n], "pktLossDetectedFwd = %s\n", (status_flags.pktLossDetectedFwd ? "True" : "False"));
        n += sprintf(&buf[n], "pktLossDetectedRev = %s\n\n", (status_flags.pktLossDetectedRev ? "True" : "False"));

        n += sprintf(&buf[n], "algPathFlag = ");
        switch (status_flags.algPathFlag) {
            case ZL303XX_USING_FWD_PATH: n += sprintf(&buf[n], "USING_FWD_PATH\n"); break;
            case ZL303XX_USING_REV_PATH: n += sprintf(&buf[n], "USING_REV_PATH\n"); break;
            case ZL303XX_USING_FWD_REV_COMBINE: n += sprintf(&buf[n], "USING_FWD_REV_COMBINE\n\n"); break;
        }

        n += sprintf(&buf[n], "Algorithm forward state = S%d\n", status_flags.algFwdState);
        n += sprintf(&buf[n], "Algorithm reverse state = S%d\n", status_flags.algRevState);
        n += sprintf(&buf[n], "State is = ");

        switch (status_flags.state) {
            case ZL303XX_FREQ_LOCK_ACQUIRING: n += sprintf(&buf[n], "FREQ_LOCK_ACQUIRING\n"); break;
            case ZL303XX_FREQ_LOCK_ACQUIRED: n += sprintf(&buf[n], "FREQ_LOCK_ACQUIRED\n"); break;
            case ZL303XX_PHASE_LOCK_ACQUIRED: n += sprintf(&buf[n], "PHASE_LOCK_ACQUIRED\n"); break;
            case ZL303XX_HOLDOVER: n += sprintf(&buf[n], "HOLDOVER\n"); break;
            case ZL303XX_REF_FAILED: n += sprintf(&buf[n], "REF_FAILED\n"); break;
            case ZL303XX_UNKNOWN: n += sprintf(&buf[n], "UNKNOWN\n"); break;
            case ZL303XX_MANUAL_FREERUN: n += sprintf(&buf[n], "MANUAL_FREERUN\n"); break;
            case ZL303XX_MANUAL_HOLDOVER: n += sprintf(&buf[n], "MANUAL_HOLDOVER\n"); break;
            case ZL303XX_MANUAL_SERVO_HOLDOVER: n += sprintf(&buf[n], "MANUAL_SERVO_HOLDOVER\n"); break;
            case ZL303XX_NO_ACTIVE_SERVER: n += sprintf(&buf[n], "NO_ACTIVE_SERVER\n"); break;
        }
    }
    *s = buf;
    return rc;
}

static const char *state_2_txt(zl303xx_AprStateE value)
{
    switch (value) {
        case ZL303XX_FREQ_LOCK_ACQUIRING: return "FREQ_LOCK_ACQUIRING";
        case ZL303XX_FREQ_LOCK_ACQUIRED: return "FREQ_LOCK_ACQUIRED";
        case ZL303XX_PHASE_LOCK_ACQUIRED: return "PHASE_LOCK_ACQUIRED";
        case ZL303XX_HOLDOVER : return "HOLDOVER ";
        case ZL303XX_REF_FAILED: return "REF_FAILED";
        case ZL303XX_UNKNOWN: return "UNKNOWN";
        case ZL303XX_MANUAL_FREERUN: return "MANUAL_FREERUN";
        case ZL303XX_MANUAL_HOLDOVER: return "MANUAL_HOLDOVER";
        case ZL303XX_MANUAL_SERVO_HOLDOVER: return "MANUAL_SERVO_HOLDOVER";
        case ZL303XX_NO_ACTIVE_SERVER:  return "NO_ACTIVE_SERVER";
    }
    return "INVALID";
}

static void apr_cgu_notify(zl303xx_AprCGUNotifyS *msg)
{

    if(msg->type == ZL303XX_CGU_STATE) {
        T_I("dpll_addr 0x%08x STATE flag changed to: %s", msg->hwParams ,state_2_txt(msg->flags.state));
    }
    exampleAprCguNotify(msg);
}

mesa_rc zl_30380_holdover_set(u32 domain, int clock_inst, BOOL enable)
{
    mesa_rc rc = VTSS_RC_OK;

    T_I("holdover enable: %d", enable);
    ZL_3036X_DATA_LOCK();

    if (zl303xx_AprSetServerHoldover (zl303xx_Params[domain], clock_inst, enable ? ZL303XX_TRUE : ZL303XX_FALSE) != ZL303XX_OK) {
        T_D("Error during Zarlink APR force holdover set");
        rc = VTSS_RC_ERROR;
    }
    ZL_3036X_DATA_UNLOCK();
    return(rc);

}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/*
 * hwParams_2_domain
 *
 * Find the domain corresponding to a zl303xx_Params pointer (clkGenId).
 * If the domain is found then also check if the corresponding CGU is
 * active for the particular domain. This is needed for domain 0 since
 * that domain has two different CGUs associated (one corresponding to
 * a physical DPLL and another corresponding to a generic DPLL used when
 * adjustment method is LTC).
 *
 */
static mesa_rc hwParams_2_domain(void *clkGenId, u32 *domain, BOOL *active)
{
    mesa_rc rc = MESA_RC_ERROR;

    if (clkGenId == zl303xx_Params_dpll) {
        *domain = 0;
        rc = MESA_RC_OK;
    } else {
        for (u32 n = 0; n < PDV_NO_OF_DOMAINS; n++) {
            if (clkGenId == zl303xx_Params_generic[n]) {
                *domain = n;
                rc = MESA_RC_OK;
                break;
            }
        }
    }

    if (rc == MESA_RC_OK) {
        if (zl303xx_Params[*domain] == clkGenId) {
            *active = TRUE;
        } else {
            *active = FALSE;
        }
    } else {
        T_DG(TRACE_GRP_PTP_INTF, "Invalid CGU pointer: %p", clkGenId);
    }

    return rc;
}

static zlStatusE apr_set_time_tsu(void *clkGenId, Uint64S deltaTimeSec, Uint32T deltaTimeNanoSec, zl303xx_BooleanE negative)
{
    zlStatusE status = ZL303XX_OK;
    mesa_timestamp_t t;

    t.nanoseconds = deltaTimeNanoSec;
    t.seconds = deltaTimeSec.lo;
    t.sec_msb = deltaTimeSec.hi;

    u32 domain;
    BOOL active;
    if (hwParams_2_domain(clkGenId, &domain, &active) == MESA_RC_OK) {
        if (active) {
            T_IG(TRACE_GRP_PTP_INTF, "Delta time: %u.%u:%u, negative %d, domain %d", t.sec_msb, t.seconds, t.nanoseconds, negative, domain);
            vtss_local_clock_time_set_delta(&t, domain, negative);
        }
    } else {
        T_WG(TRACE_GRP_PTP_INTF, "clkGenid %p, invalid domain %d", clkGenId, domain);
        status = ZL303XX_INVALID_POINTER;
    }

    return status;
}

/* Below callback is added for fixing compilation errors. */
static Sint32T apr_set_time_callout(void *clkGenId, Uint64S deltaTimeSec, Uint32T deltaTimeNanoSec, zl303xx_BooleanE negative)
{
    zlStatusE status = ZL303XX_OK;

    status = apr_set_time_tsu(clkGenId, deltaTimeSec, deltaTimeNanoSec, negative);
    return (Sint32T)status;
}

static zlStatusE apr_step_time_tsu(void *clkGenId, Sint32T deltaTimeNs)
{
    zlStatusE status = ZL303XX_OK;

    u32 domain;
    BOOL active;
    if (hwParams_2_domain(clkGenId, &domain, &active) == MESA_RC_OK) {
        if (active) {
            T_IG(TRACE_GRP_PTP_INTF, "Delta nanosec: %d, domain %d", deltaTimeNs, domain);
            /* the vtss_local_clock_adj_offset subtracts the time from current time */
            vtss_local_clock_adj_offset(-deltaTimeNs, domain);
        }
    } else {
        T_WG(TRACE_GRP_PTP_INTF, "clkGenid %p, invalid domain %d", clkGenId, domain);
        status = ZL303XX_INVALID_POINTER;
    }

    return status;
}

/* Below callback is added for fixing compilation errors. */
static Sint32T apr_step_time_callout(void *clkGenId, Sint32T deltaTimeNs)
{
    zlStatusE status = ZL303XX_OK;

    status = apr_step_time_tsu(clkGenId, deltaTimeNs);
    return (Sint32T)status;
}

static zlStatusE apr_adj_time_tsu(void *clkGenId, Sint32T deltaTimeNs, Uint32T maxAdjTime)
{
    zlStatusE status = ZL303XX_OK;

    u32 domain;
    BOOL active;
    i64 delta_time_scaled_ns;

    if (hwParams_2_domain(clkGenId, &domain, &active) == MESA_RC_OK) {
        if (active) {
            T_IG(TRACE_GRP_PTP_INTF, "AdjustTime: %d ns within max %u secs time, domain %d", deltaTimeNs, maxAdjTime, domain);
            if (deltaTimeNs >= -32000 && deltaTimeNs <= 32000) {
                delta_time_scaled_ns = (i64)deltaTimeNs << 16;
                T_IG(TRACE_GRP_PTP_INTF, "Realignment pr packet scaled ns %" PRIi64 "", delta_time_scaled_ns);
                vtss_local_clock_fine_adj_offset(-delta_time_scaled_ns, domain);
            } else {
                vtss_local_clock_adj_offset(-deltaTimeNs, domain);
            }
        }
    } else {
        T_WG(TRACE_GRP_PTP_INTF, "clkGenid %p, invalid domain %d", clkGenId, domain);
        status = ZL303XX_INVALID_POINTER;
    }

    return status;
}
static Sint32T apr_adj_time_callout(void *clkGenId, Sint32T deltaTimeNs, Uint32T maxAdjTime)
{
    return (Sint32T)apr_adj_time_tsu(clkGenId, deltaTimeNs, maxAdjTime);
}

static zlStatusE apr_adj_freq_tsu(void *clkGenId, Sint32T deltaFreq)
{
    zlStatusE status = ZL303XX_OK;

    u32 domain;
    BOOL active;
    if (hwParams_2_domain(clkGenId, &domain, &active) == MESA_RC_OK) {
        if (active && (active_instance[domain] != -1)) {
            T_DG(TRACE_GRP_PTP_INTF, "AdjustFreq: %d ppt, domain %d", deltaFreq, domain);
            vtss_local_clock_ratio_set(((i64)deltaFreq << 16) / 1000, active_instance[domain]);
        }
    } else {
        T_WG(TRACE_GRP_PTP_INTF, "clkGenid %p, invalid domain %d", clkGenId, domain);
        status = ZL303XX_INVALID_POINTER;
    }

    return status;
}
static Sint32T apr_adj_freq_callout(void *clkGenId, Sint32T deltaFreq) {
    return (Sint32T)apr_adj_freq_tsu(clkGenId, deltaFreq);
}

static zlStatusE apr_dco_get_freq(void *clkGenId, Sint32T *freqOffsetInPartsPerTrillion)
{
    zlStatusE status = ZL303XX_OK;

    u32 domain;
    BOOL active;
    if (hwParams_2_domain(clkGenId, &domain, &active) == MESA_RC_OK) {
        if (active) {
            if (active_instance[domain] != -1) {
                *freqOffsetInPartsPerTrillion = (vtss_local_clock_ratio_get(active_instance[domain]) * 1000) / (1 << 16);
                T_NG(TRACE_GRP_PTP_INTF, "clkGenId %p, domain %d", clkGenId, domain);
            } else {
                *freqOffsetInPartsPerTrillion = 0;
            }
        }
    } else {
        T_DG(TRACE_GRP_PTP_INTF, "clkGenid %p, invalid domain %d", clkGenId, domain);
        status = ZL303XX_INVALID_POINTER;
    }

    return status;
}

static zlStatusE apr_get_hw_manual_holdover_status(void *clkGenId, Sint32T *holdover_status)
{
    zlStatusE status = ZL303XX_OK;

    u32 domain;
    BOOL active;
    if (hwParams_2_domain(clkGenId, &domain, &active) == MESA_RC_OK) {
        if (active) {
            if (my_clock_option == CLOCK_OPTION_SYNCE_DPLL && domain == 0) {
                vtss_appl_synce_selection_mode_t mode;

                if (clock_selection_mode_get(&mode) == VTSS_RC_OK) {
                    *holdover_status = (Sint32T)(mode == VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER);
                } else {
                    *holdover_status = (Sint32T)false;
                }
            } else {
                *holdover_status = (Sint32T)false;
            }
            T_DG(TRACE_GRP_PTP_INTF, "APR made a request for the hw manual holdover status. Value returned was %d", (int)*holdover_status);
        }
    } else {
        T_DG(TRACE_GRP_PTP_INTF, "clkGenid %p, invalid domain %d", clkGenId, domain);
        status = ZL303XX_INVALID_POINTER;
    }

    return status;
}

static zlStatusE apr_get_hw_manual_freerun_status(void *clkGenId, Sint32T *freerun_status)
{
    zlStatusE status = ZL303XX_OK;

    u32 domain;
    BOOL active;
    if (hwParams_2_domain(clkGenId, &domain, &active) == MESA_RC_OK) {
        if (active) {
            if (my_clock_option == CLOCK_OPTION_SYNCE_DPLL && domain == 0) {
                vtss_appl_synce_selection_mode_t mode;

                if (clock_selection_mode_get(&mode) == VTSS_RC_OK) {
                    *freerun_status = (Sint32T)(mode == VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN);
                } else {
                    *freerun_status = (Sint32T)false;
                }
            }
            T_DG(TRACE_GRP_PTP_INTF, "APR made a request for the hw manual freerun status. Actual value was %d. Value returned was 0", (int)*freerun_status);
            *freerun_status = (Sint32T)false;
        }
    } else {
        T_DG(TRACE_GRP_PTP_INTF, "clkGenid %p, invalid domain %d", clkGenId, domain);
        status = ZL303XX_INVALID_POINTER;
    }

    return status;
}

static zlStatusE apr_ref_switch_to_packet_ref(void *clkGenId)
{
    zlStatusE status = ZL303XX_OK;

    T_IG(TRACE_GRP_PTP_INTF, "APR requested a switch to packet mode.");
    u32 domain;
    BOOL active;
    if (hwParams_2_domain(clkGenId, &domain, &active) == MESA_RC_OK) {
        if (active) {
            if (my_clock_option == CLOCK_OPTION_SYNCE_DPLL && domain == 0) {
                if (clock_adjtimer_enable(true) != VTSS_RC_OK) {
                    status = ZL303XX_ERROR;
                }
            }
        }
    } else {
        T_DG(TRACE_GRP_PTP_INTF, "clkGenid %p, invalid domain %d", clkGenId, domain);
        status = ZL303XX_INVALID_POINTER;
    }

    return status;
}

static zlStatusE apr_ref_switch_to_electrical_ref(void *clkGenId)
{
    zlStatusE status = ZL303XX_OK;

    T_IG(TRACE_GRP_PTP_INTF, "APR requested a switch to electrical mode.");
    u32 domain;
    BOOL active;
    if (hwParams_2_domain(clkGenId, &domain, &active) == MESA_RC_OK) {
        if (active) {
            if (my_clock_option == CLOCK_OPTION_SYNCE_DPLL && domain == 0) {
                if (clock_adjtimer_enable(false) != VTSS_RC_OK) {
                    status = ZL303XX_ERROR;
                }
            }
        }
    } else {
        T_DG(TRACE_GRP_PTP_INTF, "clkGenid %p, invalid domain %d", clkGenId, domain);
        status = ZL303XX_INVALID_POINTER;
    }

    return status;
}

static bool apr_filter_valid(zl303xx_ParamsS *zl303xx_Params, u32 filter)
{
    bool ret = true;
    if (zl303xx_Params == nullptr) {
        return false;
    }
#ifdef VTSS_SW_OPTION_ZLS3077X
    if ((zl303xx_Params->deviceType == ZL3077X_DEVICETYPE) && (filter == PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_FREQ)) {
        ret = false;
    }
#endif //VTSS_SW_OPTION_ZLS3077X
    // With generic dpll on servo 5.5.0, full_on_path_phase filter stream addition is generating errors.
    if ((zl303xx_Params->deviceType == CUSTOM_DEVICETYPE) && (filter == PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE)) {
        T_W("Filter not supported with LTC.");
    }
    return ret;
}

zlStatusE apr_get_hw_lock_status(void *clkGenId, Sint32T *lock_status)
{
    zlStatusE status = ZL303XX_OK;

    u32 domain;
    BOOL active;
    if (hwParams_2_domain(clkGenId, &domain, &active) == MESA_RC_OK) {
        if (active) {
            if (my_clock_option == CLOCK_OPTION_SYNCE_DPLL && domain == 0) {
                vtss_appl_synce_selector_state_t selector_state;
                uint clock_input;

                if (clock_selector_state_get(&clock_input, &selector_state) == VTSS_RC_OK) {
                    *lock_status = (Sint32T)(selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED);
                } else {
                    *lock_status = (Sint32T)false;
                }
            }
            T_DG(TRACE_GRP_PTP_INTF, "APR made a request for the lock status. Value returned was %d", (int)*lock_status);
        }
    } else {
        T_DG(TRACE_GRP_PTP_INTF, "clkGenid %p, invalid domain %d", clkGenId, domain);
        status = ZL303XX_INVALID_POINTER;
    }

    return status;
}

zlStatusE apr_get_hw_sync_input_en_status(void *clkGenId, Sint32T *status)
{
    *status = (Sint32T)false;
    T_DG(TRACE_GRP_PTP_INTF, "APR made a request for the sync input enable status. Not implemented yet - simply returned FALSE");

    return ZL303XX_OK;
}

zlStatusE apr_get_hw_out_of_range_status(void *clkGenId, Sint32T *status)
{
    *status = (Sint32T)false;
    T_DG(TRACE_GRP_PTP_INTF, "APR made a request for the hw out of range status. Not implemented yet - simply returned FALSE");

    return ZL303XX_OK;
}

// Set profile specific configuration in stream.
static void apr_stream_profile_config_set(zl303xx_ParamsS *zl303xx_Params, uint16_t inst, uint16_t srvr, bool virt_port)
{
    // Eventhough useOFM setting is not useful for ACI_BC_PARTIAL_ON_PATH_PHASE filter, there is no disadvantage to use it for all filters.
    server[srvr].useOFM = ZL303XX_FALSE;

    // Currently, with zl30772 dpll, 8275.1, default 1588 profile settings modified.
    if ((zl303xx_Params->deviceType == ZL3077X_DEVICETYPE ||
         zl303xx_Params->deviceType == ZL3073X_DEVICETYPE) &&
        (instance_cfg[inst].filter_mode == ACI_BC_FULL_ON_PATH_PHASE ||
         instance_cfg[inst].filter_mode == ACI_BC_PARTIAL_ON_PATH_PHASE ||
         instance_cfg[inst].filter_mode == ACI_BASIC_PHASE_LOW)) {

        if (!virt_port) {
            // Reconfigure PSL.
            (void)exampleAprReConfigurePSLFCLWithGlobals(zl303xx_Params);
        }
    }
}

/* apr_stream_create */
/**
   An example of how to start a APR stream/server

*******************************************************************************/
mesa_rc apr_stream_create(u32 domain, Uint16T instance, u32 filter_type, bool hybrid_init, i8 ptp_rate)
{
    zlStatusE status = ZL303XX_OK;
    uint16_t serverId = instance_cfg[instance].active_server();
    zl303xx_AprPktRateE fwd_pps, rev_pps;

    T_I("creating new stream for instance %d serverId %d domain %d", instance, serverId, domain);
    /* Some devices do not support some filters. Verify them. */
    if (!apr_filter_valid(zl303xx_Params[domain], filter_type)) {
        return MESA_RC_ERROR;
    }
    /* Multiple instances can be configured in only domain 0. */
    if (!domain) {
        multi_inst++;
    }
    memset(&server[serverId], 0, sizeof(zl303xx_AprAddServerS));
    if ((status = zl303xx_AprAddServerStructInit(&server[serverId])) != ZL303XX_OK)
    {
        T_E("zl303xx_AprAddServerStructInit() failed with status = %u", status);
    }

    // exampleAprSetConfigParameters depends on default device type.
    // Clock domains 1,2 use custom device type. Adding server in these domains
    // with default device as zl3077x is causing errors.
    // Will restore default device at the end of this function again to dpll type.
    (void)zl303xx_SetDefaultDeviceType(zl303xx_Params[domain]->deviceType);

    instance_cfg[instance].filter_mode = filter_type_2_mode(filter_type);
    if (hybrid_init && !domain) {
        g8275_profile = true;
    }

    if (ptp_rate == 0x7f) {
        ptp_rate = -6;
    }

    if (ptp_rate < -7 || ptp_rate > 4) {
        ptp_rate = 0;
    }

    fwd_pps = rev_pps = rate_table[ptp_rate + 7];
    zl303xx_AprSetPktRate(fwd_pps, ZL303XX_TRUE);
    zl303xx_AprSetPktRate(rev_pps, ZL303XX_FALSE);

    zl303xx_AprSetUseReversePath(ZL303XX_TRUE);
    T_I("Calling exampleAprSetConfigParameters to put APR into mode %d", (int) instance_cfg[instance].filter_mode);
    if (exampleAprSetConfigParameters(instance_cfg[instance].filter_mode) != ZL303XX_OK) {
        T_W("Error could not update APR parameters.");
    }

    // Update Timer1.
    if (instance_cfg[instance].filter_mode == ACI_BC_FULL_ON_PATH_PHASE) {
        status = zl303xx_AprSetTimer1PeriodRuntime(zl303xx_Params[domain], 125);
    } else {
        status = zl303xx_AprSetTimer1PeriodRuntime(zl303xx_Params[domain], ZL303XX_APR_TIMER1_PERIOD_MS);
    }

    /* Overwrite defaults with values set through example globals. */
    if (status == ZL303XX_OK )
    {
        server[serverId].serverId = serverId;
        server[serverId].cid = exampleAprGetConfigParameters();

        server[serverId].algTypeMode = zl303xx_AprGetAlgTypeMode();
        server[serverId].oscillatorFilterType = zl303xx_AprGetOscillatorFilterType();
        server[serverId].osciHoldoverStability = zl303xx_AprGetHoldoverStability();
        server[serverId].sModeTimeout = zl303xx_AprGetSModeTimeout();
        server[serverId].sModeAgeout = zl303xx_AprGetSModeAgeOut();
        server[serverId].bXdslHpFlag = zl303xx_AprGetXdslHpFlag();
        server[serverId].filterType = zl303xx_AprGetFilterType();
        server[serverId].fwdPacketRateType = fwd_pps;
        server[serverId].revPacketRateType = rev_pps;
        server[serverId].tsFormat = zl303xx_AprGetTsFormat();
        server[serverId].b32BitTs = zl303xx_AprGet32BitTsFlag();
        server[serverId].bUseRevPath = zl303xx_AprGetUseReversePath();
        server[serverId].bHybridMode = zl303xx_AprGetHybridServerFlag();
//        server[serverId].packetDiscardDurationInSec = zl303xx_AprGetPacketDiscardDurationInSecFlag();   // FIXME: We need to determine if the value should be configurable or hardcoded as below:
        server[serverId].packetDiscardDurationInSec = 5;                                                  // 5 sec is selected because the PHY's needs 4 sec to become synchronized
        server[serverId].pullInRange = zl303xx_AprGetPullInRange();
        server[serverId].enterHoldoverGST = zl303xx_AprGetEnterHoldoverGST();
        server[serverId].exitVFlagGST = zl303xx_AprGetExitValidGST();
        server[serverId].exitLFlagGST = zl303xx_AprGetExitLockGST();
        server[serverId].lockFlagsMask = zl303xx_AprGetLockMasks();
        server[serverId].thresholdForFlagV = zl303xx_AprGetCustomerThresholdForFlagV();
        server[serverId].exitPAFlagGST = zl303xx_AprGetExitPhaseAlignGST();
        server[serverId].fastLockBW = zl303xx_AprGetFastLockBW();
        server[serverId].fastLockTime =   zl303xx_AprGetFastLockTotalTimeInSecs();
        server[serverId].fastLockWindow = zl303xx_AprGetFastLockPktSelWindowSize();
        server[serverId].bEnableLowBWFastLock = zl303xx_AprGetEnableLowBWFastLock();

        server[serverId].EnableXOCompensation = zl303xx_AprGetEnableXOCompensation();
        server[serverId].oorPeriodResetThr = zl303xx_AprGetoorPeriodResetThr();
        server[serverId].PathReselectTimerLimitIns = zl303xx_AprGetPathReselectTimerLimitIns();
        server[serverId].L4ThresholdValue = zl303xx_AprGetL4Threshold();
        server[serverId].useOFM = zl303xx_AprGetUseOFM();

        server[serverId].DFSeed = zl303xx_AprGetDFSeedValue();
        server[serverId].bCableFastLock = zl303xx_AprGetCableFastLock();
        server[serverId].CableFastLockTimer = zl303xx_AprGetCableFastLockTimer();
        server[serverId].L2phase_varLimit = zl303xx_AprGetL2phase_varLimitValue();
        server[serverId].OutlierTimer = zl303xx_AprGetOutlierTimerValue();
        server[serverId].ClkInvalidCntr = zl303xx_AprGetClkInvalidCntr();
        server[serverId].pllStepDetectCalc = zl303xx_AprGetpllStepDetectCalcValue();
#if defined(VTSS_SW_OPTION_ZLS30341) || defined(VTSS_SW_OPTION_ZLS30361) || defined(VTSS_SW_OPTION_ZLS3077X) || defined(VTSS_SW_OPTION_ZLS3073X)
        if (zl303xx_Params[domain]->deviceType != CUSTOM_DEVICETYPE) {
            server[serverId].bUseType2BPLL = zl303xx_AprGetUseType2BPLL();
            server[serverId].useNCOAssist = zl303xx_AprGetUseNCOAssist();
            server[serverId].Type2BPLLFastLock = zl303xx_AprGetUseType2BPLLFastLock();
            server[serverId].Type2bFastLockMinPhaseNs = zl303xx_AprGetType2bFastLockMinPhase();
            server[serverId].Type2BFastlockStartupIt = zl303xx_AprGetType2BFastlockStartupIt();  /* Set 0 to disable type2B fastlock */
            server[serverId].Type2BFastlockThreshold = zl303xx_AprGetType2BFastlockThreshold();
            server[serverId].Type2BfastLockBypassPSLFCL = zl303xx_AprGetType2BfastLockBypassPSLFCL();
            server[serverId].Type2BfastLockOnMonitorToActive = ZL303XX_TRUE;
            server[serverId].Type2BFastLockPSL = zl303xx_AprGetType2BFastLockPSL();
            server[serverId].Type2BFastLockFreqEstInterval = zl303xx_AprGetType2BFastLockFreqEstInterval();
            server[serverId].Type2BMinFastLockTargetTime = zl303xx_AprGetType2BMinFastLockTargetTime();
            server[serverId].Type2BMaxFastLockTargetTime = zl303xx_AprGetType2BMaxFastLockTargetTime();
            server[serverId].Type2BFastlockSecondaryTriggerPhaseThreshold = zl303xx_AprGetType2BFastlockSecondaryTriggerPhaseThreshold();
            server[serverId].Type2BFastlockSecondaryTriggerLimitSec = zl303xx_AprGetType2BFastlockSecondaryTriggerLimitSec();
            server[serverId].Type2BFastlockTypeRatio = zl303xx_AprGetType2BFastlockTypeRatio();
            server[serverId].Type2BFastLockSimpleCoef = zl303xx_AprGetType2BFastLockSimpleCoef();
            server[serverId].Type2BMinFastLockTargetTime = zl303xx_AprGetType2BMinFastLockTargetTime();
            server[serverId].Type2BMaxFastLockTargetTime = zl303xx_AprGetType2BMaxFastLockTargetTime();
            server[serverId].Type2BLockedPhaseSlopeLimit = zl303xx_AprGetType2BLockedPhaseSlopeLimit();
            server[serverId].NCOWritePeriod = zl303xx_AprGetAprTaskBasePeriodMs();

            status = exampleAprOverrideXParamApply(&server[serverId]);
        }
#endif // dpll defines
   }

    // Modify any profile specific parameters for UNG boards.
    apr_stream_profile_config_set(zl303xx_Params[domain], instance, serverId, false);

    T_I("server algorithm %s, oscillator %s, filter %s", apr_alg_type_2_txt(server[serverId].algTypeMode), apr_osc_filter_type_2_txt(server[serverId].oscillatorFilterType), apr_filter_type_2_txt(server[serverId].filterType));

    zl303xx_DisableLogToMsgQ();
    if ((status == ZL303XX_OK) &&
        (status = zl303xx_AprAddServer(zl303xx_Params[domain], &server[serverId])) != ZL303XX_OK)
    {
        T_E("zl303xx_AprAddServer(%p, %p) failed with status = %u\n",
            zl303xx_Params[domain], &server[serverId], status);
        zl303xx_ReEnableLogToMsgQ();
        return MESA_RC_ERROR;
    }
    zl303xx_ReEnableLogToMsgQ();
    T_D("Calling mesa_ts_domain_adjtimer_set to make sure LTC frequency offset is 0");
    vtss_tod_set_adjtimer(domain, 0LL);  // Make sure the frequency offset of the LTC is 0 so that it does not cause the phase to drift

    if (status == ZL303XX_OK)
    {
       T_D("APR STREAM created. handle=%u", server[serverId].serverId);
    }

    /* APR-1Hz settings */
    if (status == ZL303XX_OK)
    {
        status = exampleConfigure1HzWithGlobals(zl303xx_Params[domain], server[serverId].serverId);

        if (status != ZL303XX_OK)
        {
            T_D("exampleAprStreamCreate, error config 1Hz, status %d", status);
        }
    }

    if (status == ZL303XX_OK)
    {
       T_D("APR STREAM created. handle=%u", server[serverId].serverId);
        active_instance[domain] = instance;
    }

    if (domain || (!domain && (multi_inst <= 1))) {
        /* Enable/disable 1Hz depending on configuration. 1Hz disabled for frequency only filters. */
        if ((status = exampleAprSet1HzEnabled(zl303xx_Params[domain], zl303xx_AprGet1HzEnabled())) == ZL303XX_OK) {
            T_D("1Hz enabled after stream creation");
        }
    }

    zl303xx_AprSetServerHoldoverSettingsRuntime(zl303xx_Params[domain], server[serverId].serverId, ZL303XX_APR_HOLDOVER_SETTING_SMART, 0);

    // Restoring default device type setings changed for exampleAprSetConfigParameters.
    (void)zl303xx_SetDefaultDeviceType(default_dpll_type);
    return (status == ZL303XX_OK) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

mesa_rc apr_set_active_timing_server(zl303xx_ParamsS *zl303xx_Params, Uint16T instance)
{
   zlStatusE status = ZL303XX_OK;
   uint16_t serverId = instance_cfg[instance].active_server();

   T_I("Set active server %d for params %p", serverId, zl303xx_Params);
   if ((status = zl303xx_AprSetActiveRef(zl303xx_Params, serverId)) != ZL303XX_OK)
   {
       T_E("zl303xx_AprSetActiveRef(%p, %d) failed with status = %u", zl303xx_Params, serverId, status);
   }

   return (status == ZL303XX_OK) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

/* apr_stream_remove */
/**
   Remove a APR stream/server

*******************************************************************************/
mesa_rc apr_stream_remove(u32 domain, Uint16T instance)
{
   zlStatusE status;
   zl303xx_AprRemoveServerS aprServerInfo;
   int serverId = instance_cfg[instance].active_server();

   /* Multiple instances can be configured in only domain 0. */
   if (!domain) {
       multi_inst--;
   }
   status = zl303xx_AprRemoveServerStructInit(&aprServerInfo);

   if (status != ZL303XX_OK) {
      T_E("Call to zl303xx_AprRemoveServerStructInit() failure = %d\n", status);
   }

   if (status == ZL303XX_OK) {
      aprServerInfo.serverId = serverId;
      status = zl303xx_AprRemoveServer(zl303xx_Params[domain], &aprServerInfo);

      if(status != ZL303XX_OK) {
         T_E("Call to zl303xx_AprRemoveServer() failure = %d\n", status);
      } else {
        active_instance[domain] = -1;
      }
   }

   if (!domain && g8275_profile) {
       g8275_profile = false;
   }
   T_I("stream removed");
   /* Wait 1 second after removing stream to make sure shutdown has been completed before we continue with operations like removing the CGU etc. */
   //sleep(1);


   return (status == ZL303XX_OK) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

extern Uint32T defaultAdjSize1HzPSL[ZL303XX_MAX_NUM_PSL_LIMITS];
extern Uint32T defaultPSL_1Hz[ZL303XX_MAX_NUM_PSL_LIMITS];

#ifdef __cplusplus
extern "C" {
#endif
Sint32T zl303xx_Dpll34xMsgRouter(void *hwParams, void *inData, void *outData);
Sint32T zl303xx_Dpll36xMsgRouter(void *hwParams, void *inData, void *outData);
Sint32T zl303xx_Dpll77xMsgRouter(void *hwParams, void *inData, void *outData);
#ifdef __cplusplus
}
#endif

Sint32T zl303xx_UserMsgRouter(void *hwParams, void *inData, void *outData)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_UserMsgInDataS *in;
    zl303xx_UserMsgOutDataS *out;


    (void) out; /* warning removal */

    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(hwParams);
    }
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(inData);
    }
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(outData);
    }

    if (status == ZL303XX_OK)
    {
        in  = (zl303xx_UserMsgInDataS *)inData;
        out = (zl303xx_UserMsgOutDataS *)outData;
    }
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_UNSUPPORTED_MSG_ROUTER_OPERATION;
        T_IG(TRACE_GRP_PTP_INTF, "msg type %d", in->userMsgType);

        switch( in->userMsgType )
        {
            case ZL303XX_USER_MSG_SET_TIME_TSU:
            case ZL303XX_USER_MSG_STEP_TIME_TSU:
                /* defaultCGU structure is used to update setTime & stepTime. No need again.*/
            case ZL303XX_USER_MSG_JUMP_TIME_TSU:
            case ZL303XX_USER_MSG_JUMP_TIME_NOTIFICATION:
            case ZL303XX_USER_MSG_JUMP_STANDBY_CGU:
            case ZL303XX_USER_MSG_SEND_REDUNDANCY_DATA_TO_MONITOR:
            case ZL303XX_USER_MSG_APR_PHASE_ADJ_MODIFIER:
                break;
            default:
                break;
        }

    }

    return status;
}

Sint32T zl303xx_DpllDummyMsgRouter(void *hwParams, void *inData, void *outData)
{
    zlStatusE status = ZL303XX_OK;
    zl303xx_DriverMsgInDataS *in;
    zl303xx_DriverMsgOutDataS *out;

    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(hwParams);
    }
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(inData);
    }
    if (status == ZL303XX_OK)
    {
        status = ZL303XX_CHECK_POINTER(outData);
    }

    if (status == ZL303XX_OK)
    {
        in  = (zl303xx_DriverMsgInDataS *)inData;
        out = (zl303xx_DriverMsgOutDataS *)outData;
    }
    if (status == ZL303XX_OK)
    {
        T_NG(TRACE_GRP_PTP_INTF,"msgtype = %u",in->dpllMsgType);
        status = ZL303XX_UNSUPPORTED_OPERATION;
        switch( in->dpllMsgType )
        {
            case ZL303XX_DPLL_DRIVER_MSG_GET_DEVICE_INFO:
                status = ZL303XX_OK;
                out->d.getDeviceInfo.devType = CUSTOM_DEVICETYPE;
                out->d.getDeviceInfo.devId = (zl303xx_DeviceIdE)0;
                out->d.getDeviceInfo.devRev = 0;
                break;
            case ZL303XX_DPLL_DRIVER_MSG_GET_FREQ_I_OR_P:
                status = apr_dco_get_freq(hwParams,
                        &(out->d.getFreq.freqOffsetInPpt));
                break;
            case ZL303XX_DPLL_DRIVER_MSG_SET_FREQ:
                status = apr_adj_freq_tsu(hwParams,
                        in->d.setFreq.freqOffsetInPpt);
                break;
            case ZL303XX_DPLL_DRIVER_MSG_TAKE_HW_NCO_CONTROL:
                status = apr_ref_switch_to_packet_ref(hwParams);
                break;
            case ZL303XX_DPLL_DRIVER_MSG_RETURN_HW_NCO_CONTROL:
                status = apr_ref_switch_to_electrical_ref(hwParams);
                break;
            case ZL303XX_DPLL_DRIVER_MSG_SET_TIME:
                status = apr_set_time_tsu(hwParams,
                         in->d.setTime.seconds,
                         in->d.setTime.nanoSeconds,
                         in->d.setTime.bBackwardAdjust);
                break;
            case ZL303XX_DPLL_DRIVER_MSG_STEP_TIME:
                status = apr_step_time_tsu(hwParams, in->d.stepTime.deltaTime);
                break;
            case ZL303XX_DPLL_DRIVER_MSG_GET_HW_MANUAL_FREERUN_STATUS:
                status = apr_get_hw_manual_freerun_status(hwParams,
                        &(out->d.getHWManualFreerun.status));
                break;
            case ZL303XX_DPLL_DRIVER_MSG_GET_HW_MANUAL_HOLDOVER_STATUS:
                status = apr_get_hw_manual_holdover_status(hwParams,
                        &(out->d.getHWManualHoldover.status));
                break;
            case ZL303XX_DPLL_DRIVER_MSG_GET_HW_SYNC_INPUT_EN_STATUS:
                status = apr_get_hw_sync_input_en_status(hwParams,
                        &(out->d.getHWSyncInputEn.status));
                break;
            case ZL303XX_DPLL_DRIVER_MSG_GET_HW_OUT_OF_RANGE_STATUS:
                status = apr_get_hw_out_of_range_status(hwParams,
                        &(out->d.getHWOutOfRange.status));
                break;
            case ZL303XX_DPLL_DRIVER_MSG_GET_HW_LOCK_STATUS:
                status = apr_get_hw_lock_status(hwParams,
                        &(out->d.getHWLockStatus.lockStatus));
                break;
            case ZL303XX_DPLL_DRIVER_MSG_ADJUST_TIME:
                status = apr_adj_time_tsu(hwParams,
                        in->d.adjustTime.adjustment,
                        in->d.adjustTime.recomendedTime);
                break;

            case ZL303XX_DPLL_DRIVER_MSG_CONFIRM_HW_CNTRL:
                out->d.confirmHwCntrl.data = 0xAAAA;
                status = ZL303XX_OK;
                break;

            default:
                break;

        }
    }
    return ZL303XX_OK;
}

//warmstart between ethernet packet port and virtual port.
//if the current stream is phase_locked and new stream is not locked, then new stream is warm started with current stream's frequency.
static void apr_pkt_port_virtual_port_warm_start(uint16_t instance, uint32_t domain, bool pkt_to_virtual)
{
    uint32_t pkt_server = instance_cfg[instance].get_packet_server();
    uint32_t virt_server = instance_cfg[instance].get_virtual_port_server();
    zl303xx_AprServerStatusFlagsS pkt_flags, virt_flags;
    Sint32T freq;
    (void)zl303xx_AprGetServerStatusFlags(zl303xx_Params[domain], pkt_server, &pkt_flags);

    (void)zl303xx_AprGetServerStatusFlags(zl303xx_Params[domain], virt_server, &virt_flags);
    T_I("warm start pkt_flags.state %d virt_flags.state %d pkt_to_virtual %d\n", pkt_flags.state, virt_flags.state, pkt_to_virtual);
    if (pkt_to_virtual) { // switch from packet stream to virtual port.
        (void)zl303xx_AprGetCurrentDF(zl303xx_Params[domain], pkt_server, &freq);
        zl303xx_AprSetActiveRef(zl303xx_Params[domain], instance_cfg[instance].active_server());
        (void)zl303xx_AprWarmStartServer(zl303xx_Params[domain], virt_server, freq);
    } else { // switch from virtual port to packet stream.
        (void)zl303xx_AprGetCurrentDF(zl303xx_Params[domain], virt_server, &freq);
        zl303xx_AprSetActiveRef(zl303xx_Params[domain], instance_cfg[instance].active_server());
        (void)zl303xx_AprWarmStartServer(zl303xx_Params[domain], pkt_server, freq);
    }
}

mesa_rc zl_30380_apr_switch_1pps_virtual_reference(uint16_t instance, uint32_t domain, bool enable, bool warm_start)
{
    mesa_rc rc = VTSS_RC_OK;
    zlStatusE status;

    T_I("switching virtual reference %s virtual port %d", enable ? "true" : "false", instance_cfg[instance].is_virtual_port_active());
    if (enable) {
        if (!instance_cfg[instance].is_virtual_port_active()) {
            exampleAprSet1HzEnabled(zl303xx_Params[domain], ZL303XX_FALSE);
            instance_cfg[instance].set_virtual_port(true);
            if (warm_start) {
                apr_pkt_port_virtual_port_warm_start(instance, domain, true);
            }
            T_I("set virtual ref true ");
        }
    } else {
        if (instance_cfg[instance].is_virtual_port_active()) {
            exampleAprSet1HzEnabled(zl303xx_Params[domain], ZL303XX_FALSE);
            instance_cfg[instance].set_virtual_port(false);
            if (warm_start) {
                apr_pkt_port_virtual_port_warm_start(instance, domain, false);
            }
            T_I("set virtual ref as false");
        }
    }
    status = zl303xx_AprSetActiveRef(zl303xx_Params[domain], instance_cfg[instance].active_server());
    // After switching reference, NCO assist enable is getting set to FALSE.
    if ((instance_cfg[instance].is_virtual_port_active() &&
       (instance_cfg[instance].virtual_port_filter == ACI_BC_FULL_ON_PATH_PHASE_SYNCE_FASTER_LOCK_LOW_PKT_RATE ||
        instance_cfg[instance].virtual_port_filter == ACI_BASIC_PHASE_LOW_SYNCE ||
        instance_cfg[instance].virtual_port_filter == ACI_BC_PARTIAL_ON_PATH_PHASE_SYNCE)) ||
       (!instance_cfg[instance].is_virtual_port_active() &&
       (instance_cfg[instance].filter_mode == ACI_BC_FULL_ON_PATH_PHASE_SYNCE ||
        instance_cfg[instance].filter_mode == ACI_BASIC_PHASE_LOW_SYNCE ||
        instance_cfg[instance].filter_mode == ACI_BC_PARTIAL_ON_PATH_PHASE_SYNCE))) {
        if (zl303xx_Params[domain]->deviceType == ZL3077X_DEVICETYPE) {
#if defined(VTSS_SW_OPTION_ZLS3077X)
            zl303xx_Dpll77xSetNCOAssistEnable(zl303xx_Params[domain], ZL303XX_TRUE);
#endif //VTSS_SW_OPTION_ZLS3077X
        } else if (zl303xx_Params[domain]->deviceType == ZL3073X_DEVICETYPE) {
#if defined(VTSS_SW_OPTION_ZLS3073X)
            zl303xx_Dpll73xSetNCOAssistEnable(zl303xx_Params[domain], ZL303XX_TRUE);
#endif //VTSS_SW_OPTION_ZLS3073X
        }
    }
    exampleAprSet1HzEnabled(zl303xx_Params[domain], ZL303XX_TRUE);
    return rc;
}

// if servo version is 30380 and dpll exists for the clock domain, then
// returns ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE. Otherwise
// returns ACI_BASIC_PHASE_LOW.
exampleAprConfigIdentifiersE apr_get_virtual_port_filter(uint16_t instance, uint32_t domain)
{
    exampleAprConfigIdentifiersE filter;
    BOOL hybrid = FALSE;

    (void)zl_30380_apr_in_hybrid_mode(domain, instance, &hybrid);
    if ((zl_3038x_module_type() == VTSS_ZARLINK_SERVO_ZLS30387) ||
        (zl303xx_Params[domain]->deviceType == CUSTOM_DEVICETYPE)) {
        filter = hybrid ? ACI_BASIC_PHASE_LOW_SYNCE : ACI_BASIC_PHASE_LOW;
    } else if (instance_cfg[instance].filter_mode == ACI_BC_PARTIAL_ON_PATH_PHASE ||
               instance_cfg[instance].filter_mode == ACI_BC_PARTIAL_ON_PATH_PHASE_SYNCE) {
        filter = hybrid ? ACI_BC_PARTIAL_ON_PATH_PHASE_SYNCE : ACI_BC_PARTIAL_ON_PATH_PHASE;
    } else {
        filter = hybrid ? ACI_BC_FULL_ON_PATH_PHASE_SYNCE_FASTER_LOCK_LOW_PKT_RATE :
                          ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE;
    }
    return filter;
}

mesa_rc apr_virtual_port_stream_create(uint16_t instance, uint32_t domain)
{
    Uint16T serverId = virt_server_id;
    zlStatusE status;
    exampleAprConfigIdentifiersE virtual_filter_mode;
    zl303xx_AprPktRateE fwd_pps = ZL303XX_1_PPS, rev_pps = ZL303XX_1_PPS;

    memset(&server[serverId], 0, sizeof(zl303xx_AprAddServerS));
    if ((status = zl303xx_AprAddServerStructInit(&server[serverId])) != ZL303XX_OK)
    {
        T_E("zl303xx_AprAddServerStructInit() failed with status = %u", status);
    }

    // exampleAprSetConfigParameters depends on default device type.
    // Clock domains 1,2 use custom device type. Adding server in these domains
    // with default device as zl3077x is causing errors.
    // Will restore default device at the end of this function again to dpll type.
    (void)zl303xx_SetDefaultDeviceType(zl303xx_Params[domain]->deviceType);

    zl303xx_AprSetPktRate(fwd_pps, ZL303XX_TRUE);
    zl303xx_AprSetPktRate(rev_pps, ZL303XX_FALSE); //0 pps is generating stream addition errors.
    virtual_filter_mode = apr_get_virtual_port_filter(instance, domain);

    if (exampleAprSetConfigParameters(virtual_filter_mode) != ZL303XX_OK) {
        T_W("Error could not update APR parameters.");
    }
    zl303xx_AprSetUseReversePath(ZL303XX_FALSE);
    instance_cfg[instance].virtual_port_filter = virtual_filter_mode;
    /* Overwrite defaults with values set through example globals. */
    if (status == ZL303XX_OK )
    {
        server[serverId].serverId = instance_cfg[instance].get_virtual_port_server();
        server[serverId].cid = exampleAprGetConfigParameters();

        server[serverId].algTypeMode = zl303xx_AprGetAlgTypeMode();
        server[serverId].oscillatorFilterType = zl303xx_AprGetOscillatorFilterType();
        server[serverId].osciHoldoverStability = zl303xx_AprGetHoldoverStability();
        server[serverId].sModeTimeout = zl303xx_AprGetSModeTimeout();
        server[serverId].sModeAgeout = zl303xx_AprGetSModeAgeOut();
        server[serverId].bXdslHpFlag = zl303xx_AprGetXdslHpFlag();
        server[serverId].filterType = zl303xx_AprGetFilterType();
        server[serverId].fwdPacketRateType = fwd_pps;
        server[serverId].revPacketRateType = rev_pps;
        server[serverId].bUseRevPath = ZL303XX_FALSE;
        server[serverId].tsFormat = zl303xx_AprGetTsFormat();
        server[serverId].b32BitTs = zl303xx_AprGet32BitTsFlag();
        server[serverId].packetDiscardDurationInSec = 5;                                                  // 5 sec is selected because the PHY's needs 4 sec to become synchronized
        server[serverId].pullInRange = zl303xx_AprGetPullInRange();
        server[serverId].enterHoldoverGST = zl303xx_AprGetEnterHoldoverGST();
        server[serverId].exitVFlagGST = zl303xx_AprGetExitValidGST();
        server[serverId].exitLFlagGST = zl303xx_AprGetExitLockGST();
        server[serverId].lockFlagsMask = zl303xx_AprGetLockMasks();
        server[serverId].thresholdForFlagV = zl303xx_AprGetCustomerThresholdForFlagV();
        server[serverId].exitPAFlagGST = zl303xx_AprGetExitPhaseAlignGST();
        server[serverId].fastLockBW = zl303xx_AprGetFastLockBW();
        server[serverId].fastLockTime =   zl303xx_AprGetFastLockTotalTimeInSecs();
        server[serverId].fastLockWindow = zl303xx_AprGetFastLockPktSelWindowSize();
        server[serverId].bEnableLowBWFastLock = zl303xx_AprGetEnableLowBWFastLock();

        server[serverId].EnableXOCompensation = zl303xx_AprGetEnableXOCompensation();
        server[serverId].oorPeriodResetThr = zl303xx_AprGetoorPeriodResetThr();
        server[serverId].PathReselectTimerLimitIns = zl303xx_AprGetPathReselectTimerLimitIns();
        server[serverId].L4ThresholdValue = zl303xx_AprGetL4Threshold();
        server[serverId].useOFM = zl303xx_AprGetUseOFM();

        server[serverId].DFSeed = zl303xx_AprGetDFSeedValue();
        server[serverId].bCableFastLock = zl303xx_AprGetCableFastLock();
        server[serverId].CableFastLockTimer = zl303xx_AprGetCableFastLockTimer();
        server[serverId].L2phase_varLimit = zl303xx_AprGetL2phase_varLimitValue();
        server[serverId].OutlierTimer = zl303xx_AprGetOutlierTimerValue();
        server[serverId].ClkInvalidCntr = zl303xx_AprGetClkInvalidCntr();
        server[serverId].enableFastNetOutageDetection = zl303xx_AprGetEnableFastNetOutageDetection();
#if defined(VTSS_SW_OPTION_ZLS30341) || defined(VTSS_SW_OPTION_ZLS30361) || defined(VTSS_SW_OPTION_ZLS3077X) || defined(VTSS_SW_OPTION_ZLS3073X)
        if (zl_virtual_param->deviceType != CUSTOM_DEVICETYPE) {
            server[serverId].bUseType2BPLL = zl303xx_AprGetUseType2BPLL();
            server[serverId].useNCOAssist = zl303xx_AprGetUseNCOAssist();
            server[serverId].Type2BPLLFastLock = zl303xx_AprGetUseType2BPLLFastLock();
            server[serverId].Type2bFastLockMinPhaseNs = zl303xx_AprGetType2bFastLockMinPhase();
            server[serverId].Type2BFastlockStartupIt = zl303xx_AprGetType2BFastlockStartupIt();  /* Set 0 to disable type2B fastlock */
            server[serverId].Type2BFastlockThreshold = zl303xx_AprGetType2BFastlockThreshold();
            server[serverId].Type2BFastlockTypeRatio = zl303xx_AprGetType2BFastlockTypeRatio();
            server[serverId].Type2BFastLockSimpleCoef = zl303xx_AprGetType2BFastLockSimpleCoef();
            server[serverId].Type2BfastLockBypassPSLFCL = zl303xx_AprGetType2BfastLockBypassPSLFCL();
            server[serverId].Type2BfastLockOnMonitorToActive = ZL303XX_TRUE;
            server[serverId].Type2BFastLockPSL = zl303xx_AprGetType2BFastLockPSL();
            server[serverId].Type2BFastLockFreqEstInterval = zl303xx_AprGetType2BFastLockFreqEstInterval();
            server[serverId].Type2BMinFastLockTargetTime = zl303xx_AprGetType2BMinFastLockTargetTime();
            server[serverId].Type2BMaxFastLockTargetTime = zl303xx_AprGetType2BMaxFastLockTargetTime();
            server[serverId].Type2BLockedPhaseSlopeLimit = zl303xx_AprGetType2BLockedPhaseSlopeLimit();
            server[serverId].NCOWritePeriod = zl303xx_AprGetAprTaskBasePeriodMs();

            status = exampleAprOverrideXParamApply(&server[serverId]);
        }
#endif // dpll defines
    }
    // Modify any profile specific parameters for UNG boards.
    apr_stream_profile_config_set(zl_virtual_param, instance, serverId, true);
    T_I("server algorithm %s, oscillator %s, filter %s", apr_alg_type_2_txt(server[serverId].algTypeMode), apr_osc_filter_type_2_txt(server[serverId].oscillatorFilterType), apr_filter_type_2_txt(server[serverId].filterType));

    zl303xx_DisableLogToMsgQ();
    if ((status == ZL303XX_OK) &&
        (status = zl303xx_AprAddServer(zl_virtual_param, &server[serverId])) != ZL303XX_OK)
    {
        T_E("zl303xx_AprAddServer(%p, %p) failed with status = %u\n",
            zl_virtual_param, &server[serverId], status);
        zl303xx_ReEnableLogToMsgQ();
        return MESA_RC_ERROR;
    }
    zl303xx_ReEnableLogToMsgQ();
    T_I("zl303xx_AprAddServer(%p, %p) done with status = %u\n",
            zl_virtual_param, &server[serverId], status);
    /* APR-1Hz settings */
    if (status == ZL303XX_OK)
    {
        zl303xx_DisableLogToMsgQ();
        status = exampleConfigure1HzWithGlobals(zl_virtual_param, instance_cfg[instance].get_virtual_port_server());
        zl303xx_ReEnableLogToMsgQ();

        if (status != ZL303XX_OK)
        {
            T_D("exampleAprStreamCreate, error config 1Hz, status %d", status);
        }
    }

    if (status == ZL303XX_OK)
    {
       T_D("APR STREAM created. handle=%u", server[serverId].serverId);
    }

    if ((status = exampleAprSet1HzEnabled(zl_virtual_param, zl303xx_AprGet1HzEnabled())) == ZL303XX_OK) {
        T_D("1Hz enabled after stream creation");
    }

    // Set holdover settings same as done during ptp packet stream creation in apr_stream_create().
    zl303xx_AprSetServerHoldoverSettingsRuntime(zl_virtual_param, instance_cfg[instance].get_virtual_port_server(), ZL303XX_APR_HOLDOVER_SETTING_SMART, 0);

    // Restoring default device type setings changed for exampleAprSetConfigParameters.
    (void)zl303xx_SetDefaultDeviceType(default_dpll_type);
    return (status == ZL303XX_OK) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

mesa_rc zl_30380_virtual_port_device_set(uint16_t instance, uint32_t domain, bool enable)
{
    mesa_rc rc = VTSS_RC_OK;

    zl_virtual_param = zl303xx_Params[domain];
    if (enable) {
        // Register new server to servo
        rc = apr_virtual_port_stream_create(instance, domain);
        if (rc != VTSS_RC_OK) {
            T_W("virtual port stream addition failed");
        } else {
            instance_cfg[instance].set_virtual_port_config(true);
        }
    } else {
        zlStatusE status;
        zl303xx_AprRemoveServerS aprServerInfo;

        instance_cfg[instance].set_virtual_port(false);
        status = zl303xx_AprSetActiveRef(zl_virtual_param, instance_cfg[instance].active_server());

        status = zl303xx_AprRemoveServerStructInit(&aprServerInfo);

        if (status != ZL303XX_OK) {
           T_E("Call to zl303xx_AprRemoveServerStructInit() failure = %d\n", status);
        }

        T_I("Removing virtual port stream");
        if (status == ZL303XX_OK) {
           aprServerInfo.serverId = instance_cfg[instance].get_virtual_port_server();
           status = zl303xx_AprRemoveServer(zl_virtual_param, &aprServerInfo);

           if(status != ZL303XX_OK) {
              T_E("Call to zl303xx_AprRemoveServer() failure = %d\n", status);
           }
        }

        /* Wait 1 second after removing stream to make sure shutdown has been completed before we continue with operations like removing the CGU etc. */
        sleep(1);
        T_I("stream removed");

        zl_virtual_param = NULL;
        instance_cfg[instance].set_virtual_port_config(false);
    }

    return rc;
}
/* Board specific initialization.
   On Multi domain board using dual Dpll, PTP and synce use different synthesizers to drive clock. So, PTP dpll
   can adjust frequency of PTP clock using dpll synthesizer output.
   On Single domain board, there is only one clock available to be driven from Dpll and Synce dpll uses it. So,
   PTP clock need to be adjusted through internal timer. */
static void meba_zls30380_device_init(zl303xx_ParamsS *zl303xx_Params, vtss_zl_30380_dpll_type_t dpll_type, zl303xx_AprAddDeviceS *device)
{
    device->PFConfig.bUseAdjustTimePacket = ZL303XX_FALSE;
    if (dpll_type == VTSS_ZL_30380_DPLL_GENERIC) {
        /* Based on configuration from past releases for servalT. */
        device->PFConfig.bUseAdjustTimeHybrid = ZL303XX_TRUE;
        device->devHybridAdjMode = ZL303XX_HYBRID_ADJ_PHASE;
    } else {
        device->PFConfig.bUseAdjustTimeHybrid = ZL303XX_FALSE;
        if (zl_3038x_get_type2b_enabled()) {
            device->devHybridAdjMode = ZL303XX_HYBRID_ADJ_FREQ;
        } else {
            device->devHybridAdjMode = zl303xx_AprGetDeviceHybridAdjMode();// use default for 343 and currently for 363.
        }
        /* Single domain or Single dpll board. */
        /* Currently enabled for redwood zls30772 dpll. */
        if (zl_3038x_get_type2b_enabled() && !fast_cap(MESA_CAP_SYNCE_SEPARATE_TIMING_DOMAINS)) {
            /* Internal timer should be called by PTP since the single clock is controlled by Synce. */
            device->defaultCGU[ZL303XX_ADCGU_ADJUST_FREQ] = ZL303XX_FALSE;
            device->defaultCGU[ZL303XX_ADCGU_ADJUST_TIME] = ZL303XX_FALSE;
        }
    }
}

static zlStatusE apr_clock_create(zl303xx_ParamsS *zl303xx_Params, vtss_zl_30380_dpll_type_t dpll_type, zl303xx_AprAddDeviceS *device)
{
   zlStatusE status = ZL303XX_OK;

    T_I("device instance pointer %p", zl303xx_Params);

   /* Initialize addDevice struct */
   memset(device, 0, sizeof(zl303xx_AprAddDeviceS));
   if ((status = zl303xx_AprAddDeviceStructInit(zl303xx_Params, device)) != ZL303XX_OK)
   {
       T_E("zl303xx_AprAddDeviceStructInit() failed with status = %u", status);
   }
   /* Overwrite defaults with values set through example globals. */
   if (status == ZL303XX_OK)
   {
       (void)exampleAprSetConfigParameters(ACI_DEFAULT);
       (void)zl303xx_AprSetDeviceOptMode(ZL303XX_ELECTRIC_MODE); // start it in the default mode (Electrical mode).
       device->hwDcoResolutionInPpt = zl303xx_AprGetHwDcoResolution();
       device->enterPhaseLockStatusThreshold = zl303xx_AprGetEnterPhaseLockStatusThreshold();
       device->exitPhaseLockStatusThreshold = zl303xx_AprGetExitPhaseLockStatusThreshold();
       device->bWarmStart = zl303xx_AprGetWarmStart();
       device->warmStartInitialFreqOffset = zl303xx_AprGetWsInitialOffset();
       device->clkStabDelayLimit = zl303xx_AprGetClkStabDelayIterations();

       device->cguNotify = apr_cgu_notify;  /* Hook the default notify handlers */
       device->elecNotify = exampleAprElecNotify;
       device->serverNotify = exampleAprServerNotify;
       device->oneHzNotify = exampleAprOneHzNotify;

       device->setTime = apr_set_time_callout;
       device->stepTime = apr_step_time_callout;
       device->adjustTime = apr_adj_time_callout;
       device->adjustFreq = apr_adj_freq_callout;

       device->defaultCGU[ZL303XX_ADCGU_SET_TIME] = ZL303XX_FALSE;   // Disable built-in function for set_time (this is needed when DPLL type is 30343 or 30363)
       device->useDriverMsgRouter = ZL303XX_TRUE;
       if (0) {
       }







#ifdef VTSS_SW_OPTION_ZLS30361
       else if (dpll_type == VTSS_ZL_30380_DPLL_ZLS3036X) {
           T_D("30363 TYPE DPLL");
           device->defaultCGU[ZL303XX_ADCGU_STEP_TIME] = ZL303XX_FALSE;  // Disable built-in function for step_time
           device->driverMsgRouter = zl303xx_Dpll36xMsgRouter;
           (void)zl303xx_SetDefaultDeviceType(ZL3036X_DEVICETYPE);
       }
#endif
#ifdef VTSS_SW_OPTION_ZLS3077X
       else if (dpll_type == VTSS_ZL_30380_DPLL_ZLS3077X) {
           T_D("3077X TYPE DPLL");
           device->defaultCGU[ZL303XX_ADCGU_STEP_TIME] = ZL303XX_FALSE;  // Disable built-in function for step_time
           device->driverMsgRouter = zl303xx_Dpll77xMsgRouter;
           (void)zl303xx_SetDefaultDeviceType(ZL3077X_DEVICETYPE);
       }
#endif //VTSS_SW_OPTION_ZLS3077X
#ifdef VTSS_SW_OPTION_ZLS3073X
       else if (dpll_type == VTSS_ZL_30380_DPLL_ZLS3073X) {
           T_D("3073X TYPE DPLL");
           device->defaultCGU[ZL303XX_ADCGU_STEP_TIME] = ZL303XX_FALSE;  // Disable built-in function for step_time
           device->driverMsgRouter = zl303xx_Dpll73xMsgRouter;
           (void)zl303xx_SetDefaultDeviceType(ZL3073X_DEVICETYPE);
       }
#endif //VTSS_SW_OPTION_ZLS3073X
       else {
           T_D("GENERIC TYPE DPLL");
           device->driverMsgRouter = zl303xx_DpllDummyMsgRouter;
           device->defaultCGU[ZL303XX_ADCGU_STEP_TIME] = ZL303XX_FALSE;  // Disable built-in function for step_time (this is needed when DPLL type is 30343 or 30363)
           (void)zl303xx_SetDefaultDeviceType(CUSTOM_DEVICETYPE);
           ZL_30380_CHECK(zl303xx_SetUseAdjustTimeHybrid(ZL303XX_TRUE));  // Only enable adjust time in hybrid mode when running on Generic type DPLL e.g. ServalT
           ZL_30380_CHECK(zl303xx_AprSetDeviceHybridAdjMode(ZL303XX_HYBRID_ADJ_PHASE));
       }
       device->devMode = zl303xx_AprGetDeviceOptMode(); /* PKT, ELEC, HYB */

       device->PFConfig.setTimeResolution = 1;
       device->PFConfig.setTimeRoundingZone = 0;
       device->PFConfig.stepTimeResolution = 1;
       device->PFConfig.setTimeStepTimeThreshold = 500000000; // 1sec/2 = 500000000ns
       device->PFConfig.stepTimeMaxTimePerAdjustment = 500000000; //equal to setTimeStepTimeThreshold
       device->PFConfig.stepTimeAdjustTimeThreshold = 2000;
       device->PFConfig.adjustFreqMinPhase = 0;
       device->PFConfig.adjustTimeMinThreshold = 0;
       zl303xx_AprSetPSL(0, 1000, 100);
       zl303xx_AprGetPSL(0, &(device->PFConfig.adjSize1HzPSL[0]), &(device->PFConfig.PSL_1Hz[0]));

       device->PFConfig.stepTimeExecutionTime = 4000;
       device->PFConfig.adjustTimeExecutionTime = 1000;

       device->legacyTreatment = zl303xx_AprGetLegacyTreatment();
       meba_zls30380_device_init(zl303xx_Params, dpll_type, device);
        /* Using user message router */
       device->useUserMsgRouter  = ZL303XX_TRUE;
       device->userMsgRouter = zl303xx_UserMsgRouter; 
   }
   /* Register a hardware device with APR. */
   if ((status == ZL303XX_OK) && (status = zl303xx_AprAddDevice(zl303xx_Params, device)) != ZL303XX_OK)
   {
      T_E("zl303xx_AprAddDevice() failed with status = %u", status);
   }
   if (status == ZL303XX_OK)
   {
       T_D("APR Device added. cguId=%p", zl303xx_Params);
   }
   return status;
}

/* apr_env_init */
/**
   Initializes the APR environment and the global variables used in our
   PTP code.

   Dpll argument to this function points to the default dpll on board.
*******************************************************************************/
static zlStatusE apr_env_init(vtss_zl_30380_dpll_type_t dpll_type)
{
   zlStatusE status = ZL303XX_OK;

   static zl303xx_AprInitS aprInit;

   OS_MEMSET(&aprInit, 0, sizeof(aprInit));
   if ((status == ZL303XX_OK) &&
       ((status = zl303xx_AprInitStructInit(&aprInit)) != ZL303XX_OK))
   {
       T_E("zl303xx_AprInitStructInit() failed with status = %u", status );
   }

   T_D("zl303xx_AprInitStructInit() succeded with status = %u", status );
   if (status == ZL303XX_OK)
   {
      /* Change any APR init defaults here: */
      /*
       * ZL303XX_APR_MAX_NUM_DEVICES = 1                         - Maximum number of clock generation device in APR;
       * ZL303XX_APR_MAX_NUM_MASTERS = 8                         - Maximum number of packet/hybrid server clock for each clock device;
       * aprInit.logLevel = 0                                  - APR log level

       DEPRACATED in SERVO 5.0.0 -
       aprInit.PFInitParams.useHardwareTimer = ZL303XX_TRUE;  - Use PSLFCL and Sample delay binding (see aprAddDevice) 
       aprInit.PFInitParams.userDelayFunc = (swFuncPtrUserDelay)user_delay_func;  - Hook PSLFCL delay binding to handler
       aprInit.AprSampleInitParams.userDelayFunc = (swFuncPtrUserDelay)user_delay_func;  - Hook Sample delay binding to handler */
       aprInit.PFInitParams.useHardwareTimer = ZL303XX_TRUE; /* - Use PSLFCL and Sample delay binding (see aprAddDevice) */
        if (dpll_type == VTSS_ZL_30380_DPLL_ZLS3077X) {
            aprInit.aprTimer1PeriodMs = 125;
            zl303xx_AprSetOutlierTimerValue(3*3600*1000/125); // from example code global variables definition.
        } else {
            aprInit.aprTimer1PeriodMs = ZL303XX_APR_TIMER1_PERIOD_MS;
        }
        aprInit.aprTimer2PeriodMs = ZL303XX_APR_TIMER2_PERIOD_MS;
        aprInit.logLevel = 0;  /* least detailed log level, can be set from CLI */
        aprInit.PFInitParams.logLevel = 0;  /* least detailed log level, can be set from CLI */
        //aprInit.userDelayFunc = (swFuncPtrUserDelay)&exampleUserDelayFunc;
        if ((status == ZL303XX_OK) &&                                     // This code has been inserted to resolve problems
            ((status = zl303xx_SetAprQIFQLength(128)) !=  ZL303XX_OK))    // with "ANMS: error sending=-1" messages on the console.
        {                                                                 //
            T_W("Failed execute zl303xx_SetAprQIFQLength(128)");          //
        }                                                                 //
        T_D("APR task perid = %d ms  PSLFCL task period = %d ms.", zl303xx_AprGetAprTaskBasePeriodMs(), zl303xx_AprGetPslFclTaskBasePeriodMs());
   }
   //zl303xx_DebugAprPrintStructAprInit(&aprInit,NULL);
   if ((status == ZL303XX_OK) &&
       (status = zl303xx_AprInit(&aprInit)) != ZL303XX_OK)
   {
       T_E("zl303xx_AprInit() failed with status = %u", status );
   }

   return status;
}

void apr_show_server_1hz_config(zl303xx_ParamsS *zl303xx_Params, Uint16T serverId)
{
    Uint16T server_id;
    zl303xx_AprConfigure1HzS fwd_1hz;
    zl303xx_AprConfigure1HzS rev_1hz;

    ZL_30380_CHECK(zl303xx_AprGetCurrent1HzConfigData(zl303xx_Params, &server_id, &fwd_1hz, &rev_1hz));
    T_N("1Hz configuration, serverId %d", server_id);
    T_N("1Hz fwd configuration, dis %d, realign %d, interval %d", fwd_1hz.disabled, fwd_1hz.realignmentType, fwd_1hz.realignmentInterval);
    T_N("1Hz rev configuration, dis %d, realign %d, interval %d", rev_1hz.disabled, rev_1hz.realignmentType, rev_1hz.realignmentInterval);
}

static exampleAprConfigIdentifiersE filter_type_2_mode(u32 filter_type)
{
    switch(filter_type) {
        case PTP_FILTERTYPE_ACI_DEFAULT:                                        return ACI_DEFAULT;
        case PTP_FILTERTYPE_ACI_FREQ_XO:                                        return ACI_FREQ_XO;
        case PTP_FILTERTYPE_ACI_PHASE_XO:                                       return ACI_PHASE_XO;
        case PTP_FILTERTYPE_ACI_FREQ_TCXO:                                      return ACI_FREQ_TCXO;
        case PTP_FILTERTYPE_ACI_PHASE_TCXO:                                     return ACI_PHASE_TCXO;
        case PTP_FILTERTYPE_ACI_FREQ_OCXO_S3E:                                  return ACI_FREQ_OCXO_S3E;
        case PTP_FILTERTYPE_ACI_PHASE_OCXO_S3E:                                 return ACI_PHASE_OCXO_S3E;
        case PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_FREQ:                        return ACI_BC_PARTIAL_ON_PATH_FREQ;
        case PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_PHASE:                       return ACI_BC_PARTIAL_ON_PATH_PHASE;
        case PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_FREQ:                           return ACI_BC_FULL_ON_PATH_FREQ;
        case PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE:                          return ACI_BC_FULL_ON_PATH_PHASE;
        case PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE: return ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE;
        case PTP_FILTERTYPE_ACI_FREQ_ACCURACY_FDD:                              return ACI_FREQ_ACCURACY_FDD;
        case PTP_FILTERTYPE_ACI_FREQ_ACCURACY_XDSL:                             return ACI_FREQ_ACCURACY_XDSL;
        case PTP_FILTERTYPE_ACI_ELEC_FREQ:                                      return ACI_ELEC_FREQ;
        case PTP_FILTERTYPE_ACI_ELEC_PHASE:                                     return ACI_ELEC_PHASE;
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C60W:                             return ACI_PHASE_RELAXED_C60W;
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C150:                             return ACI_PHASE_RELAXED_C150;
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C180:                             return ACI_PHASE_RELAXED_C180;
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C240:                             return ACI_PHASE_RELAXED_C240;
        case PTP_FILTERTYPE_ACI_BASIC_PHASE:                                    return ACI_BASIC_PHASE;
        case PTP_FILTERTYPE_ACI_BASIC_PHASE_LOW:                                return ACI_BASIC_PHASE_LOW;
        default:                                                                return ACI_DEFAULT;
    }
}

static mesa_rc zl_3038x_pdv_create(zl303xx_ParamsS *zl303xx_Params, vtss_zl_30380_dpll_type_t dpll_type, zl303xx_AprAddDeviceS *device)
{
    /* create the PDV instance */
    T_D("create the MS-PDV instance");


    ZL_30380_CHECK(zl303xx_SetAprLogLevel(0));
    /* Create a CGU */
    ZL_30380_CHECK(apr_clock_create(zl303xx_Params, dpll_type, device));

    return VTSS_RC_OK;
}

mesa_rc zl_3038x_servo_dpll_config(int clock_option, bool generic)
{
    // if generic, domain 0 uses zl303xx_Params_generic[0] otherwise is uses zl303xx_Params_dpll which is configured for the actual DPLL
    // this function shall only be called if there are no active PTP instances using domain 0.
    my_generic = generic;
    my_clock_option = clock_option;
    if (zl_3038x_get_type2b_enabled() && (clock_option == CLOCK_OPTION_PTP_DPLL)) {
        /* Timing BU servo expects both dpll device and ptp stream go through same states of freq_locked, phase_locked.
           With generic dpll using internal timer, the state of generic dpll in freq_locked or phase_locked cannot be found. So, needed to use
           actual dpll wherever possible. */
        zl303xx_Params[0] = zl303xx_Params_dpll;
        (void)zl303xx_SetDefaultDeviceType(default_dpll_type);
        my_generic = false;
    } else if (generic) {
        vtss_zl_30380_dpll_type_t dpll_type;

        zl303xx_Params[0] = zl303xx_Params_generic[0];
        (void)zl303xx_SetDefaultDeviceType(alternative_dpll_type);
        zl303xx_AprSetType2BEnabled(-1);
        /* If generic and clock option is PTP Dpll, then set PTP dpll in NCO mode. */
        if (clock_option == CLOCK_OPTION_PTP_DPLL) {
            if (clock_dpll_type_get(&dpll_type) == VTSS_RC_OK) {
                if (dpll_type == VTSS_ZL_30380_DPLL_ZLS3077X) {
#if defined(VTSS_SW_OPTION_ZLS3077X)
                    if ((zl303xx_Dpll77xModeSet(zl303xx_Params_dpll, ZLS3077X_DPLL_MODE_NCO)) != ZL303XX_OK) {
                        T_W("NCO mode set failed");
                    }
#endif //VTSS_SW_OPTION_ZLS3077X
                } else if (dpll_type == VTSS_ZL_30380_DPLL_ZLS3036X) {
                    /* Currently in 363, PTP dpll stays in NCO mode. */
                }
            }
        }
        T_IG(TRACE_GRP_PTP_INTF, "Generic DPLL %p selected", zl303xx_Params[0]);
    } else {
        zl303xx_Params[0] = zl303xx_Params_dpll;
        (void)zl303xx_SetDefaultDeviceType(default_dpll_type);
        T_IG(TRACE_GRP_PTP_INTF, "Zl303xx DPLL %p selected", zl303xx_Params[0]);
    }
    return VTSS_RC_OK;
}

bool zl_3038x_servo_dpll_config_generic(void)
{
    return (my_generic);
}

/* Type 2b method implies PTP dpll is registered with servo.
   Earlier, synce dpll used to be registered with servo. Currently, only 
   redwood dpll zls30772 uses this servo method. */
bool zl_3038x_get_type2b_enabled()
{
    bool ret = false;
#if defined(VTSS_SW_OPTION_ZLS3077X) || defined(VTSS_SW_OPTION_ZLS3073X)
    if (zl303xx_Params_dpll && (zl303xx_Params_dpll->deviceType == ZL3077X_DEVICETYPE ||
                                zl303xx_Params_dpll->deviceType == ZL3073X_DEVICETYPE)) {
        ret = true;
    }
#endif //VTSS_SW_OPTION_ZLS3077X
    return ret;
}

void zl_3038x_api_version_check()
{
    if (strcmp(zl303xx_ApiReleaseVersion, zl303xx_AprReleaseVersion) != 0) {
        T_E("Expected zl303xx_ApiReleaseVersion = %s", (const char*)zl303xx_ApiReleaseVersion );
        T_E("Actual zl303xx_AprReleaseVersion = %s", (const char*)zl303xx_AprReleaseVersion );
        VTSS_ASSERT(0);
    }
}

/* This function is used for setting default values which are specific to 30380 code */
static void zl_30380_defaults()
{
    if (zl_3038x_module_type() != VTSS_ZARLINK_SERVO_ZLS30387) {
        zl303xx_AprSetPFLockedPhaseOutlierThreshold(600);
    }
}

vtss_zarlink_servo_t zl_3038x_module_type()
{
    if (strcmp(zl303xx_AprReleaseSwId, "ZLS30387") == 0) {
        return VTSS_ZARLINK_SERVO_ZLS30387;
    }
    if (strcmp(zl303xx_AprReleaseSwId, "ZLS30380") == 0) {
        return VTSS_ZARLINK_SERVO_ZLS30380;
    }
    return VTSS_ZARLINK_SERVO_NONE;
}

mesa_rc zl_3038x_pdv_init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_INIT:
        critd_init(&zl30380_global.datamutex, "zl_30380_data", VTSS_MODULE_ID_ZL_3034X_PDV, CRITD_TYPE_MUTEX);
        T_D("INIT_CMD_INIT ZLS30380" );
        break;

    case INIT_CMD_START:
        T_D("INIT_CMD_START ZLS30380");
        T_D("Expected zl303xx_ApiReleaseSwId = %s", (const char*)zl303xx_ApiReleaseSwId);
        T_D("Actual zl303xx_AprReleaseSwId = %s", (const char*)zl303xx_AprReleaseSwId);
        T_D("Expected zl303xx_ApiReleaseVersion = %s", (const char*)zl303xx_ApiReleaseVersion );
        T_D("Actual zl303xx_AprReleaseVersion = %s", (const char*)zl303xx_AprReleaseVersion );
        break;

    case INIT_CMD_CONF_DEF:
        zl30380_conf_read(TRUE);
        T_D("INIT_CMD_CONF_DEF ZLS30380" );
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        zl30380_conf_read(FALSE);
        /* Setup logging */
        ZL_30380_CHECK(zl303xx_SetupLogToMsgQ(stdout));
        zl303xx_TraceSetLevelAll(0);

        // Set any defaults specific to 30380
        zl_30380_defaults();
        // create a zl_3038x pdv (device) instance
        vtss_zl_30380_dpll_type_t dpll_type;
        if (clock_dpll_type_get(&dpll_type) != VTSS_RC_OK) {
            T_I("Could not get DPLL type - assuming GENERIC");
            dpll_type = VTSS_ZL_30380_DPLL_GENERIC;
        }
        zl303xx_AprSetResetReady(ZL303XX_TRUE);
        /* Launch APR application */
        ZL_30380_CHECK(apr_env_init(dpll_type));
        T_I("apr_env_init finished");
        zl_3038x_pdv_create(zl303xx_Params_dpll, dpll_type, &dpll_dev);
        default_dpll_type = zl303xx_GetDefaultDeviceType();
        zl303xx_Params[0] = zl303xx_Params_dpll;
        zl_3038x_pdv_create(zl303xx_Params_generic[0], VTSS_ZL_30380_DPLL_GENERIC, &gen_dev[0]);
        zl_3038x_pdv_create(zl303xx_Params_generic[1], VTSS_ZL_30380_DPLL_GENERIC, &gen_dev[1]);
        zl303xx_Params[1] = zl303xx_Params_generic[1];
        zl_3038x_pdv_create(zl303xx_Params_generic[2], VTSS_ZL_30380_DPLL_GENERIC, &gen_dev[2]);
        zl303xx_Params[2] = zl303xx_Params_generic[2];
#if defined(VTSS_SW_OPTION_ZLS30341) || defined(VTSS_SW_OPTION_ZLS30361) || defined(VTSS_SW_OPTION_ZLS3077X) || defined(VTSS_SW_OPTION_ZLS3073X)
        zl303xx_Params_generic[0]->deviceType = CUSTOM_DEVICETYPE;
        zl303xx_Params_generic[1]->deviceType = CUSTOM_DEVICETYPE;
        zl303xx_Params_generic[2]->deviceType = CUSTOM_DEVICETYPE;
#endif // dpll defines for using deviceType parameter

        free(zl303xx_Params_generic[3]);
        zl303xx_Params_generic[3] = 0;
        zl303xx_Params[3] = zl303xx_Params_generic[3];
        alternative_dpll_type = zl303xx_GetDefaultDeviceType();
        T_IG(TRACE_GRP_PTP_INTF, "zl303xx_Params_dpll = %p", zl303xx_Params_dpll);
        int idx;
        for (idx = 0; idx < PDV_NO_OF_DOMAINS; idx++) {
            T_IG(TRACE_GRP_PTP_INTF, "zl303xx_Params_generic[%d] = %p", idx, zl303xx_Params_generic[idx]);
        }

        T_I("pdv_create, dpll_type = %d finished", dpll_type);
        T_D("INIT_CMD_ICFG_LOADING_PRE ");
        for (idx = 0 ; idx < PTP_INSTANCES_NUM; idx++) {
            instance_cfg[idx].init(idx);
        }
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

mesa_rc zl_30380_apr_set_server_mode(zl303xx_ParamsS *zl303xx_Params, int mode, u32 serverId)
{
    if (mode == 1) {  // FIXME: For now only 1 == packet is defined
        if (zl303xx_AprSetServerMode(zl303xx_Params, serverId, ZL303XX_PACKET_MODE) == ZL303XX_OK) return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

// switch virtual port to packet mode from hybrid mode.
mesa_rc zl_30380_virtual_port_switch_to_packet_mode(u32 domain, int clock_inst)
{
    bool changed_mode = false;

    if (instance_cfg[clock_inst].virtual_port_filter == ACI_BC_FULL_ON_PATH_PHASE_SYNCE_FASTER_LOCK_LOW_PKT_RATE) {
        instance_cfg[clock_inst].virtual_port_filter = ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE;
        changed_mode = true;
    }
    if (instance_cfg[clock_inst].virtual_port_filter == ACI_BASIC_PHASE_LOW_SYNCE) {
        instance_cfg[clock_inst].virtual_port_filter = ACI_BASIC_PHASE_LOW;
        changed_mode = true;
    }
    if (instance_cfg[clock_inst].virtual_port_filter == ACI_BC_PARTIAL_ON_PATH_PHASE_SYNCE) {
        instance_cfg[clock_inst].virtual_port_filter = ACI_BC_PARTIAL_ON_PATH_PHASE;
        changed_mode = true;
    }

    if (changed_mode) {
        zl303xx_AprSetServerHoldoverSettingsRuntime(zl303xx_Params[domain], instance_cfg[clock_inst].get_virtual_port_server(), ZL303XX_APR_HOLDOVER_SETTING_SMART, 0);
        (void)exampleAprSetConfigParameters(instance_cfg[clock_inst].virtual_port_filter);
        if (exampleAprModeSwitching(zl303xx_Params[domain], instance_cfg[clock_inst].get_virtual_port_server(), instance_cfg[clock_inst].virtual_port_filter) != ZL303XX_OK) {
            zl303xx_ReEnableLogToMsgQ();
            return VTSS_RC_ERROR;
        }
        if (zl303xx_Params[domain]->deviceType == ZL3077X_DEVICETYPE) {
#if defined(VTSS_SW_OPTION_ZLS3077X)
            zl303xx_Dpll77xSetNCOAssistEnable(zl303xx_Params[domain], ZL303XX_FALSE);
#endif //VTSS_SW_OPTION_ZLS3077X
        } else if (zl303xx_Params[domain]->deviceType == ZL3073X_DEVICETYPE) {
#if defined(VTSS_SW_OPTION_ZLS3073X)
            zl303xx_Dpll73xSetNCOAssistEnable(zl303xx_Params[domain], ZL303XX_FALSE);
#endif //VTSS_SW_OPTION_ZLS3073X
        }
        clock_nco_assist_set(false);
        T_I("virtual port domain %u, clock_inst %d, mode switching to %d", domain, clock_inst, instance_cfg[clock_inst].virtual_port_filter);
    }
    return MESA_RC_OK;
}

// switch virtual port to hybrid mode from packet mode.
mesa_rc zl_30380_virtual_port_switch_to_hybrid_mode(u32 domain, int clock_inst)
{
    bool changed_mode = false;

    if (instance_cfg[clock_inst].virtual_port_filter == ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE) {
        instance_cfg[clock_inst].virtual_port_filter = ACI_BC_FULL_ON_PATH_PHASE_SYNCE_FASTER_LOCK_LOW_PKT_RATE;
        changed_mode = true;
    }
    if (instance_cfg[clock_inst].virtual_port_filter == ACI_BASIC_PHASE_LOW) {
        instance_cfg[clock_inst].virtual_port_filter = ACI_BASIC_PHASE_LOW_SYNCE;
        changed_mode = true;
    }
    if (instance_cfg[clock_inst].virtual_port_filter == ACI_BC_PARTIAL_ON_PATH_PHASE) {
        instance_cfg[clock_inst].virtual_port_filter = ACI_BC_PARTIAL_ON_PATH_PHASE_SYNCE;
        changed_mode = true;
    }

    if (changed_mode) {
        clock_nco_assist_set(true);
        zl303xx_AprSetServerHoldoverSettingsRuntime(zl303xx_Params[domain], instance_cfg[clock_inst].get_virtual_port_server(), ZL303XX_APR_HOLDOVER_SETTING_USER_SET, 0);
        (void)exampleAprSetConfigParameters(instance_cfg[clock_inst].virtual_port_filter);
        if (zl303xx_Params[domain]->deviceType == ZL3077X_DEVICETYPE) {
#if defined(VTSS_SW_OPTION_ZLS3077X)
            zl303xx_Dpll77xSetNCOAssistEnable(zl303xx_Params[domain], ZL303XX_TRUE);
#endif //VTSS_SW_OPTION_ZLS3077X
        } else if (zl303xx_Params[domain]->deviceType == ZL3073X_DEVICETYPE) {
#if defined(VTSS_SW_OPTION_ZLS3073X)
            zl303xx_Dpll73xSetNCOAssistEnable(zl303xx_Params[domain], ZL303XX_TRUE);
#endif //VTSS_SW_OPTION_ZLS3073X
        }
        if (exampleAprModeSwitching(zl303xx_Params[domain], instance_cfg[clock_inst].get_virtual_port_server(), instance_cfg[clock_inst].virtual_port_filter) != ZL303XX_OK) {
            zl303xx_ReEnableLogToMsgQ();
            return VTSS_RC_ERROR;
        }
        T_I("virtual port domain %u, clock_inst %d, mode switching to %d", domain, clock_inst, instance_cfg[clock_inst].virtual_port_filter);
    }
    return MESA_RC_OK;
}

// While switching the mode from hybrid to  packet mode or viceversa,
// 1) if virtual port is active, virtual port's mode is switched.
// 2) ptp ethernet port's mode is switched next.
// 3) if virtual port is configured but not active, it's mode is switched next.
// Idea is to switch first the mode of the stream which is currently active. Whether virtual port exists or not, ptp ethernet port's mode must be switched.
mesa_rc zl_30380_apr_switch_to_packet_mode(u32 domain, int clock_inst)
{
    bool changed_mode = false;
    T_I("domain %u, clock_inst %d vport active:%s", domain, clock_inst, instance_cfg[clock_inst].is_virtual_port_active() ? "true" : "false");

    // Active virtual port mode switch
    if (instance_cfg[clock_inst].is_virtual_port_active()) {
        zl_30380_virtual_port_switch_to_packet_mode(domain, clock_inst);
    }

    // ptp port mode switch
    zl303xx_AprSetServerHoldoverSettingsRuntime(zl303xx_Params[domain], clock_inst, ZL303XX_APR_HOLDOVER_SETTING_SMART, 0);
    if (instance_cfg[clock_inst].filter_mode == ACI_BC_PARTIAL_ON_PATH_PHASE_SYNCE) {
        instance_cfg[clock_inst].filter_mode = ACI_BC_PARTIAL_ON_PATH_PHASE;
        changed_mode = true;
    }
    if (instance_cfg[clock_inst].filter_mode == ACI_BC_FULL_ON_PATH_PHASE_SYNCE) {
        instance_cfg[clock_inst].filter_mode = ACI_BC_FULL_ON_PATH_PHASE;
        changed_mode = true;
    }
    if (instance_cfg[clock_inst].filter_mode == ACI_BASIC_PHASE_SYNCE) {
        instance_cfg[clock_inst].filter_mode = ACI_BASIC_PHASE;
        changed_mode = true;
    }
    if (instance_cfg[clock_inst].filter_mode == ACI_BASIC_PHASE_LOW_SYNCE) {
        instance_cfg[clock_inst].filter_mode = ACI_BASIC_PHASE_LOW;
        changed_mode = true;
    }

    if (changed_mode) {
        (void)exampleAprSetConfigParameters(instance_cfg[clock_inst].filter_mode);
        zl303xx_DisableLogToMsgQ();
        if (exampleAprModeSwitching(zl303xx_Params[domain], instance_cfg[clock_inst].get_packet_server(), instance_cfg[clock_inst].filter_mode) != ZL303XX_OK) {
            zl303xx_ReEnableLogToMsgQ();
            return VTSS_RC_ERROR;
        }
        if (zl303xx_Params[domain]->deviceType == ZL3077X_DEVICETYPE) {
#if defined(VTSS_SW_OPTION_ZLS3077X)
            zl303xx_Dpll77xSetNCOAssistEnable(zl303xx_Params[domain], ZL303XX_FALSE);
#endif //VTSS_SW_OPTION_ZLS3077X
        } else if (zl303xx_Params[domain]->deviceType == ZL3073X_DEVICETYPE) {
#if defined(VTSS_SW_OPTION_ZLS3073X)
            zl303xx_Dpll73xSetNCOAssistEnable(zl303xx_Params[domain], ZL303XX_FALSE);
#endif //VTSS_SW_OPTION_ZLS3073X
        }
        zl303xx_ReEnableLogToMsgQ();
        T_I("domain %u, clock_inst %d, mode switching to %d", domain, clock_inst, instance_cfg[clock_inst].filter_mode);
        clock_nco_assist_set(false);

        if (zl_3038x_get_type2b_enabled()) {
            // Currently, there is no differentiation between long term transient and short term transient states.
            // So, whenever we are in packet mode, we note it as VTSS_PTP_HYBRID_TRANSIENT_QUICK.
            curTransient = VTSS_PTP_HYBRID_TRANSIENT_QUICK;
        }
    }

    // configured but not active virtual port mode switch
    if (instance_cfg[clock_inst].is_virtual_port_configured() && !instance_cfg[clock_inst].is_virtual_port_active()) {
        zl_30380_virtual_port_switch_to_packet_mode(domain, clock_inst);
    }
    return VTSS_RC_OK;
}

// While switching the mode from packet to hybrid mode,
// 1) if virtual port is active, virtual port's mode is switched.
// 2) ptp ethernet port's mode is switched next.
// 3) if virtual port is configured but not active, it's mode is switched next.
// Idea is to switch first the mode of the stream which is currently active. Whether virtual port exists or not, ptp ethernet port's mode must be switched.
mesa_rc zl_30380_apr_switch_to_hybrid_mode(u32 domain, int clock_inst)
{
    bool changed_mode = false;
    T_I("domain %u, clock_inst %d vport active:%s", domain, clock_inst, instance_cfg[clock_inst].is_virtual_port_active() ? "true" : "false");

    // active virtual port mode switch
    if (instance_cfg[clock_inst].is_virtual_port_active()) {
        zl_30380_virtual_port_switch_to_hybrid_mode(domain, clock_inst);
    }

    // ptp ethernet port mode switch.
    zl303xx_AprSetServerHoldoverSettingsRuntime(zl303xx_Params[domain], clock_inst, ZL303XX_APR_HOLDOVER_SETTING_USER_SET, 0);
    if (instance_cfg[clock_inst].filter_mode == ACI_BC_PARTIAL_ON_PATH_PHASE) {
        instance_cfg[clock_inst].filter_mode = ACI_BC_PARTIAL_ON_PATH_PHASE_SYNCE;
        changed_mode = true;
    }
    if (instance_cfg[clock_inst].filter_mode == ACI_BC_FULL_ON_PATH_PHASE) {
        instance_cfg[clock_inst].filter_mode = ACI_BC_FULL_ON_PATH_PHASE_SYNCE;
        changed_mode = true;
    }
    if (instance_cfg[clock_inst].filter_mode == ACI_BASIC_PHASE) {
        instance_cfg[clock_inst].filter_mode = ACI_BASIC_PHASE_SYNCE;
        changed_mode = true;
    }
    if (instance_cfg[clock_inst].filter_mode == ACI_BASIC_PHASE_LOW) {
        instance_cfg[clock_inst].filter_mode = ACI_BASIC_PHASE_LOW_SYNCE;
        changed_mode = true;
    }
    if (changed_mode) {
        clock_nco_assist_set(true);
        (void)exampleAprSetConfigParameters(instance_cfg[clock_inst].filter_mode);
        zl303xx_DisableLogToMsgQ();
        if (zl303xx_Params[domain]->deviceType == ZL3077X_DEVICETYPE) {
#if defined(VTSS_SW_OPTION_ZLS3077X)
            zl303xx_Dpll77xSetNCOAssistEnable(zl303xx_Params[domain], ZL303XX_TRUE);
#endif //VTSS_SW_OPTION_ZLS3077X
        } else if (zl303xx_Params[domain]->deviceType == ZL3073X_DEVICETYPE) {
#if defined(VTSS_SW_OPTION_ZLS3073X)
            zl303xx_Dpll73xSetNCOAssistEnable(zl303xx_Params[domain], ZL303XX_TRUE);
#endif //VTSS_SW_OPTION_ZLS3073X
        }
        if (exampleAprModeSwitching(zl303xx_Params[domain], instance_cfg[clock_inst].get_packet_server(), instance_cfg[clock_inst].filter_mode) != ZL303XX_OK) {
            zl303xx_ReEnableLogToMsgQ();
            return VTSS_RC_ERROR;
        }
        T_I("domain %u, clock_inst %d, mode switching to %d", domain, clock_inst, instance_cfg[clock_inst].filter_mode);
        zl303xx_ReEnableLogToMsgQ();

        if (zl_3038x_get_type2b_enabled()) {
            // Currently, there is no differentiation between long term transient and short term transient states.
            // So, whenever we are in hybrid mode, we note it as VTSS_PTP_HYBRID_TRANSIENT_NOT_ACTIVE.
            curTransient = VTSS_PTP_HYBRID_TRANSIENT_NOT_ACTIVE;
        }
    }

    // configured but not active virtual port mode switch
    if (instance_cfg[clock_inst].is_virtual_port_configured() && !instance_cfg[clock_inst].is_virtual_port_active()) {
        zl_30380_virtual_port_switch_to_hybrid_mode(domain, clock_inst);
    }
    return VTSS_RC_OK;
}

mesa_rc zl_30380_apr_in_hybrid_mode(u32 domain, int clock_inst, BOOL *state)
{
    zl303xx_BooleanE hybridMode;

    if (zl303xx_AprIsServerInHybridMode(zl303xx_Params[domain], clock_inst, &hybridMode) == ZL303XX_OK) {
        T_IG(TRACE_GRP_PTP_INTF,"domain %u, clock_inst %d, state %d", domain, clock_inst, hybridMode);
        if (hybridMode == ZL303XX_TRUE)
            *state = TRUE;
        else
            *state = FALSE;

        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc zl_30380_apr_set_active_ref(u32 domain, int instance)
{
    zlStatusE status;
    uint16_t stream = instance_cfg[instance].active_server();

    T_IG(TRACE_GRP_PTP_INTF, "Params %p, stream %d", zl303xx_Params[domain], stream);
//    if ((status = zl303xx_AprGetDeviceCurrActiveRef(zl303xx_Params, &serverId)) == ZL303XX_OK) {
//        if (serverId != stream) {
            if ((status = zl303xx_AprSetActiveRef(zl303xx_Params[domain], stream)) == ZL303XX_OK)
                return VTSS_RC_OK;
            else {
                T_WG(TRACE_GRP_PTP_INTF, "Could not set active reference. Status was %d", status);
                return VTSS_RC_ERROR;
            }
//        }
//        return VTSS_RC_OK;
//    } else {
//        T_WG(TRACE_GRP_PTP_INTF, "Could not get current active ref. Status was %d", (int) status);
//    }
//    return VTSS_RC_ERROR;
}

mesa_rc zl30380_apr_set_active_elec_ref(u32 domain, int input)
{
    zlStatusE status;

    T_IG(TRACE_GRP_PTP_INTF,"domain %u, input %d", domain, input);
//    if ((status = zl303xx_AprGetDeviceCurrActiveRef(zl303xx_Params, &serverId)) == ZL303XX_OK) {
//        if (serverId != 0xFFFF) {
            if ((status = zl303xx_AprSetActiveElecRef(zl303xx_Params[domain], input)) == ZL303XX_OK)
                return VTSS_RC_OK;
            else {
                T_WG(TRACE_GRP_PTP_INTF, "Could not set active electrical ref. mode. Status was %d", status);
                return VTSS_RC_ERROR;
            }
//        }
//        return VTSS_RC_OK;
//    } else {
//        T_WG(TRACE_GRP_PTP_INTF, "Could not get current active ref. Status was %d", (int) status);
//    }
//    return VTSS_RC_ERROR;
}

mesa_rc zl_30380_apr_set_hybrid_transient(u32 domain, int clock_inst, vtss_ptp_hybrid_transient transient)
{
    mesa_rc rc = VTSS_RC_OK;
    BOOL is_in_hybrid_mode;
    i64 adj;
    zl303xx_AprConfigure1HzS par;
    zl303xx_DevTypeE devType;
   
    devType = zl303xx_GetDefaultDeviceType();

    /* 30343 dpll does not support hybrid transient function. There are examples about 30343 dpll behaviour related to hybrid transient in Example code. */
    if (devType == ZL3034X_DEVICETYPE) {
        return MESA_RC_OK;
    }

    // transient response should be considered only for 8275 profile.
    if (!g8275_profile) {
        return MESA_RC_OK;
    }

    if (curTransient == transient) {
        return MESA_RC_OK;
    }
    T_I("curTransient %d, transient %d.", curTransient, transient);
    if (rc == VTSS_RC_OK) {
        if (instance_cfg[clock_inst].is_virtual_port_active()) {
            rc = zl_30380_apr_in_hybrid_mode(domain, instance_cfg[clock_inst].get_virtual_port_server(), &is_in_hybrid_mode);
        } else {
            rc = zl_30380_apr_in_hybrid_mode(domain, instance_cfg[clock_inst].get_packet_server(), &is_in_hybrid_mode);
        }
        if (rc == VTSS_RC_OK) {
            if (CUSTOM_DEVICETYPE == devType) {
                if (is_in_hybrid_mode) {
                    //ZL_30380_CHECK(zl303xx_AprConfigure1HzStructInit(zl303xx_Params[domain], ZL303XX_TRUE, &par));
                    par.disabled = ZL303XX_TRUE;
                    par.realignmentType = ZL303XX_1HZ_REALIGNMENT_PERIODIC;
                    par.realignmentInterval = 120;
                    /* Servo does not support transient mitigation for generic DPLL's (i.e. non zls303x3 dplls) */
                    switch(transient) {
                        case VTSS_PTP_HYBRID_TRANSIENT_QUICK:
                            // get current holdover value from synce DPLL
                            rc = clock_ho_frequency_offset_get(&adj);
                            
                            // set offset in the PTP dpll
                            rc = clock_output_adjtimer_set(adj);
                            // switch PTP to holdover
                            rc = clock_ptp_timer_source_set(PTP_CLOCK_SOURCE_INDEP);
                            par.disabled = ZL303XX_FALSE;
                            par.realignmentType = ZL303XX_1HZ_REALIGNMENT_PERIODIC;
                            par.realignmentInterval = 1;
                            //ZL_30380_CHECK(zl303xx_AprConfigServer1Hz(zl303xx_Params[domain], clock_inst, ZL303XX_TRUE, &par));
                            //ZL_30380_CHECK(zl303xx_AprConfigServer1Hz(zl303xx_Params[domain], clock_inst, ZL303XX_FALSE, &par));
                            T_WG(TRACE_GRP_PTP_INTF, "set transient state, offset %d ppb", (i32)(adj>>16));
                            break;
                        case VTSS_PTP_HYBRID_TRANSIENT_OPTIONAL:
                            T_WG(TRACE_GRP_PTP_INTF, "HYBRID_TRANSIENT_OPTIONAL not supported");
                            rc = VTSS_RC_ERROR;
                            break;
                        case VTSS_PTP_HYBRID_TRANSIENT_NOT_ACTIVE:
                            par.disabled = ZL303XX_FALSE;
                            par.realignmentType = ZL303XX_1HZ_REALIGNMENT_PERIODIC;
                            par.realignmentInterval = 1;
                            //ZL_30380_CHECK(zl303xx_AprConfigServer1Hz(zl303xx_Params[domain], clock_inst, ZL303XX_TRUE, &par));
                            //ZL_30380_CHECK(zl303xx_AprConfigServer1Hz(zl303xx_Params[domain], clock_inst, ZL303XX_FALSE, &par));
                            // switch back to Synce
                            rc = clock_ptp_timer_source_set(PTP_CLOCK_SOURCE_SYNCE);
                            // set offset in the PTP dpll
                            rc = clock_output_adjtimer_set(0LL);
                            T_WG(TRACE_GRP_PTP_INTF, "clear transient state");
                            break;
                        default: rc = VTSS_RC_ERROR;
                    }
                } else {
                    T_WG(TRACE_GRP_PTP_INTF, "HYBRID_TRANSIENT detected but servo is not in hybrid mode");
                }
            } else {
                zlStatusE status = ZL303XX_OK;

                switch(transient) {
                    case VTSS_PTP_HYBRID_TRANSIENT_QUICK:
                        if (zl_3038x_get_type2b_enabled()) {
                            // For type 2B enabled dplls, during transient period, dpll can switch to packet mode.
                            zl_30380_apr_switch_to_packet_mode(domain, clock_inst);
                        } else {
                            // In legacy method, during transient period, zl303xx_AprSetHybridTransient is used.
                            status = zl303xx_AprSetHybridTransient(zl303xx_Params[domain], ZL303XX_BHTT_QUICK);
                            zl3xxAprHandleHybridTransient(zl303xx_Params[domain], ZL303XX_BHTT_QUICK);
                            T_IG(TRACE_GRP_PTP_INTF, "Call to zl303xx_AprSetHybridTransient to set transient state to ZL303XX_BHTT_QUICK returned %d", status);
                        }
                        break;
                    case VTSS_PTP_HYBRID_TRANSIENT_OPTIONAL:
                        status = zl303xx_AprSetHybridTransient(zl303xx_Params[domain], ZL303XX_BHTT_OPTIONAL);
                        T_IG(TRACE_GRP_PTP_INTF, "Call to zl303xx_AprSetHybridTransient to set transient state to ZL303XX_BHTT_OPTIONAL returned %d", status);
                        break;
                    case VTSS_PTP_HYBRID_TRANSIENT_NOT_ACTIVE:
                        if (zl_3038x_get_type2b_enabled()) {
                            // For type 2B enabled dplls, after transient period, dpll can switch directly to hybrid mode.
                            zl_30380_apr_switch_to_hybrid_mode(domain, clock_inst);
                        } else {
                            // In legacy method, after transient period, zl303xx_AprSetHybridTransient is used for setting transient_not_active state.
                            status = zl303xx_AprSetHybridTransient(zl303xx_Params[domain], ZL303XX_BHTT_NOT_ACTIVE);
                            zl3xxAprHandleHybridTransient(zl303xx_Params[domain], ZL303XX_BHTT_NOT_ACTIVE);
                            T_IG(TRACE_GRP_PTP_INTF, "Call to zl303xx_AprSetHybridTransient to set transient state to ZL303XX_BHTT_NOT_ACTIVE returned %d", status);
                        }
                        break;
                    default: status = ZL303XX_ERROR;
                }

                if (status != ZL303XX_OK) {
                    rc = VTSS_RC_ERROR;
                }
            }
        }
    }
    if (rc == VTSS_RC_OK) {
        curTransient = transient;
    }
    return rc;
}

// To avoid some of the logs displayed on console, they are disabled in logging function by matching specific strings. Refer to zl_ung_strs_no_logging function for logs disabled.
mesa_rc zl_30380_apr_set_log_level(i32 level)
{
    if (level >= 0) {
        ZL_30380_CHECK(zl303xx_SetAprLogLevel(level));
    } else {
        ZL_30380_CHECK(zl303xx_SetAprLogLevel(0));
    }

    return VTSS_RC_OK;
}

mesa_rc zl_30380_apr_show_psl_fcl_config(u16 cguId)
{
    CGUID_CHECK(cguId);
    ZL_30380_CHECK(zl303xx_DebugAprPrintPSLFCLData(zl303xx_Params[cguId]));

    return VTSS_RC_OK;
}

mesa_rc zl_30380_apr_show_statistics(u16 cguId, u32 stati)
{
    CGUID_CHECK(cguId);
    if (stati == 0) {
        ZL_30380_CHECK(zl303xx_DebugGetAprPathStatistics(zl303xx_Params[cguId]));
    }
    if (stati == 1) {
        ZL_30380_CHECK(zl303xx_AprGetDeviceCurrentPathDelays(zl303xx_Params[cguId]));
    }
    if (stati == 2) {
        ZL_30380_CHECK(zl303xx_DebugGetAllAprStatistics(zl303xx_Params[cguId]));
    }
    return VTSS_RC_OK;
}

void zl_3038x_srvr_holdover_param_get(u32 cgu, u32 instance, bool *holdover_ok, i64 *hold_val)
{
    Sint32T holdoverValue = 0;
    zl303xx_BooleanE holdoverValid = ZL303XX_FALSE;
    BOOL hybridMode;
    if (zl303xx_AprGetServerHoldoverValue(zl303xx_Params[cgu], instance_cfg[instance].active_server(), &holdoverValue, &holdoverValid) == ZL303XX_OK) {
        *holdover_ok = (holdoverValid == ZL303XX_TRUE) ? true : false;
        zl_30380_apr_in_hybrid_mode(cgu, instance, &hybridMode);
        if(hybridMode) {
            // if hybrid use the dpll 3 offset
            *hold_val = zl303xx_Params[cgu]->pllParams.syncFreq;
        } else {
            *hold_val = (i64)holdoverValue;
        }
    } else {
        *holdover_ok = false;
    }
}
mesa_rc zl_30380_apr_dbg_cmds(u16 cguId, u16 serverId, u16 cmd, int val)
{
    Sint32T df=0;

    T_I("cgu %d ser %d cmd %d val %d", cguId, serverId, cmd, val);
    switch(cmd) {
    case 1: // Set server holdover gst
           (void)zl303xx_AprSetServerFlagGST(zl303xx_Params[cguId], serverId, ZL303XX_SERVER_CLK_GST_V_FLAG, val);
           (void)zl303xx_AprSetServerFlagGST(zl303xx_Params[cguId], serverId, ZL303XX_SERVER_CLK_GST_L_FLAG, val);
           (void)zl303xx_AprSetServerFlagGST(zl303xx_Params[cguId], serverId, ZL303XX_SERVER_CLK_GST_PA_FLAG, val);
           (void)zl303xx_AprSetServerEnterHoldoverGST(zl303xx_Params[cguId], serverId, val);
            break;
    case 2: // Enable/disable timestamp processing
            enable_ts = val;
            break;
    case 3: //Current DF
            (void)exampleGetHoldoverAndWarmStartServer(zl303xx_Params[cguId], serverId);
            break;
    case 4: // Holdover quality
            (void)zl303xx_DebugAprHoldoverQualityParams(zl303xx_Params[cguId]);
            break;
    case 5: // Device Status
            (void)zl303xx_GetAprDeviceStatus(zl303xx_Params[cguId]);
            break;
    case 6: // Server Status
            (void)zl303xx_GetAprServerStatus(zl303xx_Params[cguId], serverId);
            break;
    case 7: // Timestamp logging
            (void)zl_30380_apr_ts_log_level_set(val);
            break;
    case 8: // Version information
        printf("APR %s version: %s patch level: %s\n", zl303xx_AprReleaseSwId, zl303xx_AprReleaseVersion, zl303xx_AprPatchLevel);
            break;
    case 9: // Apr server status
            (void)zl303xx_DebugGetServerStatus(zl303xx_Params[cguId], serverId);
            break;
    case 10: // Get active ref
            (void)zl303xx_AprGetDeviceCurrActiveRef(zl303xx_Params[cguId], &serverId);
            printf("Apr Reference server %d\n", serverId);
            break;
    case 11: // Set Server Holdover settings
            T_I("set server holdover val %d\n", val);
            zl303xx_AprSetServerHoldoverSettingsRuntime(zl303xx_Params[cguId], serverId, ZL303XX_APR_HOLDOVER_SETTING_USER_SET, val);
            break;
    case 12:
            (void)zl303xx_AprGetCurrentDF(zl303xx_Params[cguId], serverId, &df);
            printf("cguId %d serverId %d currentDF %d\n", cguId, serverId, df);
            break;
    default:
            break;
    }
    return VTSS_RC_OK;
}

