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

#include "critd_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#include "conf_api.h"
#include "misc_api.h"
#include "main.h"
#include "msg_api.h"
#include "control_api.h"
#include "main_types.h"
#include "board_if.h"
#if defined(VTSS_SW_OPTION_PTP)
#include "ptp_api.h"
#include "vtss_ptp_api.h"
#include "vtss_tod_api.h"
#endif

#include "synce_custom_clock_api.h"
#include "synce_dpll_base.h"
extern synce_dpll_base *synce_dpll;

#ifdef VTSS_SW_OPTION_PACKET
#include "packet_api.h"
#endif
#ifdef VTSS_SW_OPTION_PHY
#include "phy_api.h"
#endif

#ifdef VTSS_SW_OPTION_ICFG
#include "synce_icli_functions.h" // For synce_icfg_init
#endif

#include "interrupt_api.h"

#include "lock.hxx"
#include "synce_trace.h"

#include "microchip/ethernet/switch/api.h"

#include "synce_board.hxx"

#define VTSS_RC(expr) { mesa_rc __rc__ = (expr); if (__rc__ < VTSS_RC_OK) return __rc__; }

/****************************************************************************/
/*  Global variables                                                                                                                      */
/****************************************************************************/



#define SWITCH_AUTO_SQUELCH true              /* Enable auto squelching in the Switch (this was earlier disabled to avoid detection of
                                                 LOCS before Link down on Serdes ports. The problem was that the WTR timer was not
                                                 activated if Locs was detected before Link Down.
                                                 This problem has now been solved in func_thread in the PORT_CALL_BACK action. */

uint synce_my_nominated_max;                  /* actual max number of nominated ports */
uint synce_my_priority_max;                   /* actual max number of priorities */
uint synce_my_prio_disabled;


typedef struct
{
    vtss_appl_synce_clock_source_nomination_config_t clock_source_nomination_config[SYNCE_NOMINATED_MAX];
    vtss_appl_synce_clock_selection_mode_config_t    clock_selection_mode_config;
    vtss_appl_synce_station_clock_config_t           station_clock_config;
    CapArray<vtss_appl_synce_port_config_t, VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT> port_config;
} conf_blk_t;


#ifdef VTSS_SW_OPTION_PACKET
static u8  ssm_dmac[6] = {0x01,0x80,0xC2,0x00,0x00,0x02};
static u8  ssm_ethertype[2] = {0x88,0x09};
static u8  ssm_standard[6] = {0x0A,0x00,0x19,0xA7,0x00,0x01};
#endif

static vtss_handle_t func_thread_handle;
static vtss_thread_t func_thread_block;

/*lint -esym(457, crit) */
static critd_t          crit;

static vtss_trace_reg_t trace_reg =
{
    VTSS_TRACE_MODULE_ID, "SyncE", "SyncE module."
};

static vtss_trace_grp_t trace_grps[] =
{
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_PDU_RX] = {
        "rx",
        "Rx PDU print out ",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_PDU_TX] = {
        "tx",
        "Tx PDU print out ",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_API] = {
        "api",
        "Switch API printout",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_CLOCK] = {
        "clock",
        "Clock API printout ",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_CLI] = {
        "cli",
        "CLI",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_DEVELOP] = {
        "develop",
        "Develop",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_BOARD] = {
        "board",
        "Board",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define SYNCE_CRIT_ENTER() critd_enter(&crit, __FILE__, __LINE__)
#define SYNCE_CRIT_EXIT()  critd_exit( &crit, __FILE__, __LINE__)

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
    #ifdef __cplusplus
        extern "C" void vtss_synce_mib_init();
    #else
        void vtss_synce_mib_init();
    #endif
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC)
    #ifdef __cplusplus
        extern "C" void vtss_appl_synce_json_init();
    #else
        void vtss_appl_synce_json_init();
    #endif
#endif
vtss_zl_30380_dpll_type_t dpll_type = VTSS_ZL_30380_DPLL_GENERIC;
#define ZL3077X_PTP_REF_ID 8
#define ZL3073X_PTP_REF_ID 5

/****************************************************************************/
/*                                                                          */
/*  Local/static function decleration                                       */
/*                                                                          */
/****************************************************************************/
static void update_clk_in_selector(uint source);
static uint overwrite_conv(vtss_appl_synce_quality_level_t overwrite);
static void ssm_frame_tx(uint port, bool event_flag, uint ssm);
static void synce_set_clock_source_nomination_config_to_default(vtss_appl_synce_clock_source_nomination_config_t *config);
void synce_set_clock_selection_mode_config_to_default(vtss_appl_synce_clock_selection_mode_config_t *config);
void synce_set_station_clock_config_to_default(vtss_appl_synce_station_clock_config_t *config);
static void synce_set_port_config_to_default(vtss_appl_synce_port_config_t *config);

/****************************************************************************/
/*                                                                          */
/*  Eventhandler implementation.                                            */
/*                                                                          */
/****************************************************************************/
#include "notifications.hxx"
#include "subject.hxx"
#include "memcmp-operator.hxx"

VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_port_status_t);
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_synce_clock_source_nomination_config_t)
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_synce_clock_selection_mode_config_t)
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_synce_clock_selection_mode_status_t)

namespace vtss {

    /* 
       port_info , holds information of ethernet ports
     */
        static CapArray<notifications::Subject<vtss_appl_port_status_t>, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_info;
    /* 
       ci_ql_p , represents quality level deducted by ethernet,station and ptp clock inputs(esmc PDUs,PTP packets)
     */
        static CapArray<notifications::Subject<vtss_appl_synce_quality_level_t>, VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT> ci_ql_p;
    /* 
       ssf_p , represents signal failure deducted by ethernet,station and ptp clock inputs
     */
        static CapArray<notifications::Subject<bool>, VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT> ssf_p;
    /* 
       d_los_esmc , represents loss of esmc information on ethernet ports
     */
        static CapArray<notifications::Subject<bool>, MEBA_CAP_BOARD_PORT_MAP_COUNT> d_los_esmc;
     /* 
       is_aneg_master, represents aneg master or slave state of ethernet cu-ports
     */
        static CapArray<notifications::Subject<bool>, MEBA_CAP_BOARD_PORT_MAP_COUNT> is_aneg_master;
       
    /* 
       ssm_enabled , to enable or disable ssm TX,RX on ethernet ports
     */
        static CapArray<notifications::Subject<bool>, MEBA_CAP_BOARD_PORT_MAP_COUNT> ssm_enabled;
    /* 
       rx_ssm, recieved ssm information
     */
        static CapArray<notifications::Subject<u8>, MEBA_CAP_BOARD_PORT_MAP_COUNT> rx_ssm;
    /* 
       station_clock_in, station clock input frequency
     */
        static notifications::Subject<vtss_appl_synce_frequency_t> station_clock_in;
    /* 
       station_clock_out, station clock output frequency
     */
        static notifications::Subject<vtss_appl_synce_frequency_t> station_clock_out;
    /* 
       station_clock_source, represents clock source on which station clock selected 
     */
        static notifications::Subject<u8> station_clock_source;
    /* 
       nomination_config, Nomination configuration 
     */
        static CapArray<notifications::Subject<vtss_appl_synce_clock_source_nomination_config_t>, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> nomination_config;
    /* 
       ci_ql_n, represents quality level of the nominated clock sources
     */
        static CapArray<notifications::Subject<vtss_appl_synce_quality_level_t>, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> ci_ql_n;
    /* 
       ci_ssf_n, represents signal failure status of the nominated clock sources
     */
        static CapArray<notifications::Subject<bool>, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> ci_ssf_n;
    /* 
       ql, represents quality level of the sources that are available for selection(includes internal clock source) 
     */
        static CapArray<notifications::Subject<vtss_appl_synce_quality_level_t> ,VTSS_APPL_CAP_SYNCE_SELECTED_CNT> ql;
    /* 
       sf, represents signal failure status of the sources that are available for selection(includes internal clock source)
     */
        static CapArray<notifications::Subject<bool> ,VTSS_APPL_CAP_SYNCE_SELECTED_CNT> sf;
    /* 
       LOCS_n, represents loss of control signal from DPLL
     */
        static CapArray<notifications::Subject<bool>, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> LOCS_n;
    /* 
       ssm_status, represents ssm status of on a nominated clock source (applicable for eth and ptp inputs)
     */
        static CapArray<notifications::Subject<bool>, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> ssm_status;
    /* 
       wtr_status, represents timeout status of wtr timer
     */
        static CapArray<notifications::Subject<bool>, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> wtr_status;
    /* 
       clock_selection_config, represents clock selection configuration 
     */
        static notifications::Subject<vtss_appl_synce_clock_selection_mode_config_t> clock_selection_config;
    /* 
       selected_source, represents selected clock source by dpll
     */
        static notifications::Subject<uint> selected_source;
#if defined(VTSS_SW_OPTION_PTP)
    /* 
       best_master, represents selected PTP source by dpll
     */
        static notifications::Subject<uint> best_master;
#endif
    /* 
       selected_port, represents selected clock input on the selected_source
     */
        static notifications::Subject<uint> selected_port;
    /* 
       selected_ql, represents quality level of the input clock source using selected_port
     */
        static notifications::Subject<vtss_appl_synce_quality_level_t> selected_ql;
    /* 
       clear_wtr, clears wtr timer running
     */
        static CapArray<notifications::Subject<bool>, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> clear_wtr;
    /* 
       clock_input, represents actual clock input selected by dpll(different form selected_source)
     */
        static notifications::Subject<uint> clock_input;
    /* 
       LOL_dpll, represents Loss of link status of DPLL
     */
        static notifications::Subject<bool> LOL_dpll;
    /* 
       LOSX_dpll, represents LOSX status from DPLL
     */
        static notifications::Subject<bool> LOSX_dpll;
    /* 
       DHOLD_dpll, represents holdover status of DPLL
     */
        static notifications::Subject<bool> DHOLD_dpll;
    /* 
       ptp_hybrid_mode, represents ptp hybrid mode configuration
     */
        static notifications::Subject<bool> ptp_hybrid_mode;
    /* 
       selector_state, represents actual selector state from DPLL
     */
        static notifications::Subject<vtss_appl_synce_selector_state_t> selector_state;
    /* 
       saved_selector_state, saved selector state copy
     */
        static notifications::Subject<vtss_appl_synce_selector_state_t> saved_selector_state;
    /* 
       clock_selection_status, represents current status of clock selection
     */
        static notifications::Subject<vtss_appl_synce_clock_selection_mode_status_t> clock_selection_status;
#if defined(VTSS_SW_OPTION_PTP)
    /* 
       ptp_clock_class, represents PTP clock class which would be mapped to SYNCE clock
     */
        static CapArray<notifications::Subject<u8>, VTSS_APPL_CAP_PTP_CLOCK_CNT> ptp_clock_class;
    /* 
       ptp_clock_ptsf, represents signal failure of PTP clock instance
     */
        static CapArray<notifications::Subject<vtss_appl_synce_ptp_ptsf_state_t>, VTSS_APPL_CAP_PTP_CLOCK_CNT> ptp_clock_ptsf;
#endif
    /* 
       tx_ssm, represents SSM code to be transmitted
     */
        static CapArray<notifications::Subject<u8>, MEBA_CAP_BOARD_PORT_MAP_COUNT> tx_ssm;

static notifications::SubjectRunner *my_sr = &vtss::notifications::subject_main_thread;
//static notifications::SubjectRunner *my_sr = &notifications::subject_runner_get(VTSS_THREAD_PRIO_ABOVE_NORMAL, false);

static bool is_ssm_aligned(vtss_appl_synce_eec_option_t eec_option ,u8 ssm) {
    if (eec_option == VTSS_APPL_SYNCE_EEC_OPTION_1) {
        switch (ssm) {
            case SSM_QL_PRC:
            case SSM_QL_SSUA_TNC:
            case SSM_QL_SSUB:
            case SSM_QL_SEC:
            case SSM_QL_DNU_DUS:
                return true;
            default : 
                return false;
        }
    } else {
        switch(ssm) {
            case SSM_QL_STU:
            case SSM_QL_PRS_INV:
            case SSM_QL_ST2:
            case SSM_QL_SSUA_TNC:
            case SSM_QL_ST3E:
            case SSM_QL_ST3:
            case SSM_QL_SMC:
            case SSM_QL_PROV:
            case SSM_QL_DNU_DUS:
                return true;
            default:
                return false;
        }
    }
}

static void port_change_event(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    mepa_aneg_status_t phy_status;
    port_info[port_no].set(*status);

    // Only 1G copper PHY need to know about master/slave status of link
    if (status->link &&
        (status->static_caps & MEBA_PORT_CAP_1G_PHY) &&
        !status->fiber) {
        SYNCE_CRIT_ENTER();
        if (meba_phy_aneg_status_get(board_instance, port_no, &phy_status) != VTSS_RC_OK) {
            T_W("meba_phy_aneg_status_get() port %u failed", port_no);
        } else {
            is_aneg_master[port_no].set(phy_status.master);
        }
        SYNCE_CRIT_EXIT();
    } else {
        is_aneg_master[port_no].set(true);
    }
    T_IG(TRACE_GRP_DEVELOP,"port_change_event: port=%u, link=%u, phy=%u, fiber=%u, full_duplex=%u, state=%s",
         port_no, status->link, (status->static_caps & MEBA_PORT_CAP_1G_PHY) != 0,
         status->fiber, status->fdx, phy_status.master?"master":"slave");

}

static void port_interrupt_callback(meba_event_t source_id, u32 port_no)
{
    vtss_appl_port_status_t port_state = port_info[port_no].get();
    mesa_rc rc;

    port_state.link = false;
    port_info[port_no].set(port_state);

    // register for link-down events again.
    if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_SYNCE, vtss::port_interrupt_callback, source_id, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
        T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
    }
}

static u32 freq_khz(vtss_appl_synce_frequency_t f)
{
    switch(f)
    {
        case VTSS_APPL_SYNCE_STATION_CLK_DIS:      return 0;
        case VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ: return 1544;
        case VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ: return 2048;
        case VTSS_APPL_SYNCE_STATION_CLK_10_MHZ:   return 10000;
        default:                                   return 0;
    }
}
/*
 * EventHandler: eh_rx_ssm_processing:
 * replication: pr. ethernet port

 * Input:   rx_ssm, ssm_enabled, port_info.link, clock_selection_config.eec_option, t_ssm_timer
 * Output:  ci_ql_p, d_los_esmc
 * Description:
 *          Calculate ci_ql and d_los_esmc pr ethernet port.
 */
struct eh_rx_ssm_processing: public notifications::EventHandler {

    eh_rx_ssm_processing () :
        EventHandler(my_sr),
        e_rx_ssm(this), e_ssm_enabled(this), e_clock_selection_config(this), e_port_info(this),
        t_ssm_timer(this)
    {
    }
    
    void init(u32 port_no) {
        my_port = port_no;
        ssm_enabled[my_port].attach(e_ssm_enabled);
        rx_ssm[my_port].attach(e_rx_ssm);
        clock_selection_config.attach(e_clock_selection_config);
        port_info[my_port].attach(e_port_info);
        my_sr->timer_add(t_ssm_timer, (milliseconds) 5000);
        timeout = true;
    }

    void execute(notifications::Event *e) override {
        if (e == &e_rx_ssm) {
            T_NG(TRACE_GRP_DEVELOP,"eh_rx_ssm_processing[%u], recieved rx_ssm = %u",my_port, rx_ssm[my_port].get());
            timeout = false;
            my_sr->timer_add(t_ssm_timer, (milliseconds) 7000);
        }
        if (ssm_enabled[my_port].get(e_ssm_enabled)) {
            auto ssm = rx_ssm[my_port].get(e_rx_ssm);
            auto eec_option = clock_selection_config.get(e_clock_selection_config).eec_option;
            bool ssm_aligned = is_ssm_aligned(eec_option, ssm);
            bool link_up = port_info[my_port].get(e_port_info).link;
            if (link_up) {
                if (timeout) {
                    T_DG(TRACE_GRP_DEVELOP,
                         "eh_rx_ssm_processing[%u]: timeout setting ci_ql_p[%u]=QL_FAIL",
                         my_port, my_port);
                    ci_ql_p[my_port].set(VTSS_APPL_SYNCE_QL_FAIL);
                } else {
                    auto ql = (ssm_aligned) ? ssm_to_ql(ssm, eec_option) : VTSS_APPL_SYNCE_QL_INV;
                    if (ql != ci_ql_p[my_port].get()) {
                        T_DG(TRACE_GRP_DEVELOP,
                             "eh_rx_ssm_processing[%u]: setting ci_ql_p[%u]=%d",
                             my_port, my_port, ql);
                    }
                    ci_ql_p[my_port].set(ql);
                }
            } else {
                T_DG(TRACE_GRP_DEVELOP,
                     "eh_rx_ssm_processing[%u]: no link setting ci_ql_p[%u]=QL_LINK",
                     my_port, my_port);
                ci_ql_p[my_port].set(VTSS_APPL_SYNCE_QL_LINK);
                timeout = true;
            }
            d_los_esmc[my_port].set(timeout);
        } else {
            T_DG(TRACE_GRP_DEVELOP,"eh_rx_ssm_processing[%u], as ssm_enabled = false setting ci_ql_p[%u]=QL_DNU,d_los_esmc[%u]=false ",my_port ,my_port ,my_port);
            ci_ql_p[my_port].set(VTSS_APPL_SYNCE_QL_DNU);
            d_los_esmc[my_port].set(false);
        }
    }

    void execute(notifications::Timer *t) override {
        if (t == &t_ssm_timer) {
            timeout = true;
            execute(&e_ssm_enabled);
        } else {
            T_WG(TRACE_GRP_DEVELOP,"port %d unknown timer event %p", my_port, t);
        }
    }

    notifications::Event e_rx_ssm;
    notifications::Event e_ssm_enabled;
    notifications::Event e_clock_selection_config;
    notifications::Event e_port_info;
    notifications::Timer t_ssm_timer;
    u32 my_port;
    bool timeout;
};

/*
 * EventHandler: eh_1000base_t_master_slave:
 * replication: pr. ethernet port
 * Input:   port_info, aneg_mode, d_los_esmc
 * Output:  ci_ssf_p
 * Description:
 *          Calculate ci_ssf and make master slave negotiation for 1G Phy ports.
 */
struct eh_1000base_t_master_slave : public notifications::EventHandler {
    eh_1000base_t_master_slave () :
        EventHandler(my_sr),
        e_port_state(this), e_dlos_esmc(this), e_selected_port(this),
        t_aneg_timer(this)
    {
    }

    void init(uint32_t id) {
        my_id = id;
        T_DG(TRACE_GRP_DEVELOP,"eh_1000base_t_master_slave::init on nominated %d ", id);
        aneg_ongoing = false;
        e_port_state.signal();
    }

    void execute(notifications::Timer *t) override {
        aneg_ongoing = false;
        T_DG(TRACE_GRP_DEVELOP,"%s[%d]: check aneg state", __PRETTY_FUNCTION__, my_id);
        execute((notifications::Event *)nullptr);
    }
    void execute(notifications::Event *e) override {
        my_port_state = port_info[my_id].get(e_port_state);
        if (!(my_port_state.static_caps & MEBA_PORT_CAP_1G_PHY) || my_port_state.fiber) {
            T_DG(TRACE_GRP_DEVELOP,"%s[%d]: Not a 1G CU link: %d ", __PRETTY_FUNCTION__, my_id, my_id);
            return; // Aneg only applies to 1G CU ethernet ports
        }

        bool d_esmc = d_los_esmc[my_id].get(e_dlos_esmc);
        ssf_p[my_id].set(d_esmc || (!my_port_state.link && !aneg_ongoing));

        if (aneg_ongoing) {
            T_DG(TRACE_GRP_DEVELOP,"%s[%d]: Aneg is ongoing on port: %d ", __PRETTY_FUNCTION__, my_id, my_id);
            return; // If aneg already is on-going, do not restart it
        }

        if (!my_port_state.link) {
            // No reason to continue if there is no link
            T_DG(TRACE_GRP_DEVELOP,"%s[%d]: No link on port: %d ", __PRETTY_FUNCTION__, my_id, my_id);
            return;
        }

        uint32_t cur_selected_port = selected_port.get(e_selected_port);

        mepa_aneg_status_t phy_status;
        mepa_manual_neg_t  phy_man_neg;
        bool               re_auto_negotiate = false;

        SYNCE_CRIT_ENTER();
        if (meba_phy_aneg_status_get(board_instance, my_id, &phy_status) != VTSS_RC_OK) {
            T_WG(TRACE_GRP_DEVELOP,"%s[%d]: Could not read phy status for port: %d ", __PRETTY_FUNCTION__, my_id, my_id);
            SYNCE_CRIT_EXIT();
            return;
        }
        if (VTSS_RC_OK != port_man_neg_get(my_id, &phy_man_neg)) {
            T_E("vtss_appl_port_man_neg_get, port_no: %u", my_id);
            SYNCE_CRIT_EXIT();
            return;
        }
        if (my_id == cur_selected_port) {
            if (phy_status.master) {
                T_DG(TRACE_GRP_DEVELOP,"%s[%d]: Selected port (%d) is master, start aneg", __PRETTY_FUNCTION__, my_id, my_id);
                phy_man_neg = MEPA_MANUAL_NEG_CLIENT;
                re_auto_negotiate = true;
            }
        } else if (phy_man_neg != MEPA_MANUAL_NEG_DISABLED) {
            phy_man_neg = MEPA_MANUAL_NEG_DISABLED;
            re_auto_negotiate = true;
        }
        if (re_auto_negotiate) {
            if (port_man_neg_set(my_id, phy_man_neg) != VTSS_RC_OK) {
                T_EG(TRACE_GRP_DEVELOP,"%s[%d]: Could not write phy config for port: %d", __PRETTY_FUNCTION__, my_id, my_id);
            }
            aneg_ongoing = true;
            my_sr->timer_add(t_aneg_timer, (milliseconds) 6000);
            T_DG(TRACE_GRP_DEVELOP,"%s[%d]: started aneg for port: %d", __PRETTY_FUNCTION__, my_id, my_id);
        }

        SYNCE_CRIT_EXIT();
    }


    uint32_t my_id;
    notifications::Event e_port_state;
    notifications::Event e_dlos_esmc;
    notifications::Event e_selected_port;
    notifications::Timer t_aneg_timer;

    bool aneg_ongoing;
    vtss_appl_port_status_t my_port_state;
    vtss_appl_synce_clock_source_nomination_config_t nconfig;
};

/*
 * EventHandler: eh_port_monitor:
 * replication: pr. ethernet port
 * Input:   all synce port related data
 * Output:  trace changes in input data
 * Description:
 *          Make a trace output for each port related variables, when updated.
 */
struct eh_port_monitor: public notifications::EventHandler {

    eh_port_monitor () :
        EventHandler(my_sr),
        e_ssf(this), e_ci_ql(this), e_port_state(this),
        e_los_esmc(this), e_ssm_enabled(this), e_rx_ssm(this), e_tx_ssm(this), e_is_aneg_master(this)
    {
    }
    
    void init(u32 port_no) {
        my_port = port_no;
        port_info[my_port].attach(e_port_state);
        ci_ql_p[my_port].attach(e_ci_ql);
        ssf_p[my_port].attach(e_ssf);
        d_los_esmc[my_port].attach(e_los_esmc);
        ssm_enabled[my_port].attach(e_ssm_enabled);
        rx_ssm[my_port].attach(e_rx_ssm);
        tx_ssm[my_port].attach(e_tx_ssm);
        is_aneg_master[my_port].attach(e_is_aneg_master);
    }

    void execute(notifications::Event *e) override {
        if (e == &e_ssf) {
            auto info = ssf_p[my_port].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: port %d ssf %s", my_port, info ? "true" : "false");
        } else if (e == &e_ci_ql) {
            auto info = ci_ql_p[my_port].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: port %d ci_ql %s", my_port, vtss_appl_synce_quality_level_txt[info].valueName);
        } else if (e == &e_port_state) {
            auto info = port_info[my_port].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor port %d link %d, speed %d, phy %d", my_port, info.link, info.speed, (info.static_caps & MEBA_PORT_CAP_1G_PHY) != 0);
        } else if (e == &e_los_esmc) {
            auto info = d_los_esmc[my_port].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: port %d dlosesmc %s", my_port, info ? "true" : "false");
        } else if (e == &e_ssm_enabled) {
            auto info = ssm_enabled[my_port].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: port %d ssm_enabled %s", my_port, info ? "true" : "false");
        } else if (e == &e_rx_ssm) {
            auto info = rx_ssm[my_port].get(*e);
            T_DG(TRACE_GRP_CLOCK, "monitor: port %d rx_ssm %d", my_port, info);
        } else if (e == &e_tx_ssm) {
            auto info = tx_ssm[my_port].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: port %d tx_ssm %d", my_port, info);
        } else if (e == &e_is_aneg_master) {
            auto info = is_aneg_master[my_port].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: port %d is_aneg_master %s", my_port, info ? "true" : "false");
        }
    }

    notifications::Event e_ssf;
    notifications::Event e_ci_ql;
    notifications::Event e_port_state;
    notifications::Event e_los_esmc;
    notifications::Event e_ssm_enabled;
    notifications::Event e_rx_ssm;
    notifications::Event e_tx_ssm;
    notifications::Event e_is_aneg_master;
    u32 my_port;
};

#if defined(VTSS_SW_OPTION_PTP)
/*
 * EventHandler: eh_ptp_monitor:
 * replication: pr. ptp instance
 * Input:   all ptp interface related data
 * Output:  trace changes in input data
 * Description:
 *          Make a trace output for each port related variables, when updated.
 */
struct eh_ptp_monitor: public notifications::EventHandler {

    eh_ptp_monitor () :
        EventHandler(my_sr),
        e_ssf(this), e_ci_ql(this), e_ptp_clock_class(this),
        e_ptp_clock_ptsf(this)
    {
    }
    
    void init(u32 idx, u32 port_no) {
        my_idx = idx;
        my_port = port_no;
        ci_ql_p[my_port].attach(e_ci_ql);
        ssf_p[my_port].attach(e_ssf);
        ptp_clock_class[my_idx].attach(e_ptp_clock_class);
        ptp_clock_ptsf[my_idx].attach(e_ptp_clock_ptsf);
    }

    void execute(notifications::Event *e) override {
        if (e == &e_ssf) {
            auto info = ssf_p[my_port].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: ptp %d, port %d ssf %s", my_idx, my_port, info ? "true" : "false");
        } else if (e == &e_ci_ql) {
            auto info = ci_ql_p[my_port].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: ptp %d, port %d ci_ql %s", my_idx, my_port, vtss_appl_synce_quality_level_txt[info].valueName);
        } else if (e == &e_ptp_clock_class) {
            auto info = ptp_clock_class[my_idx].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor ptp %d class %d", my_idx, info);
        } else if (e == &e_ptp_clock_ptsf) {
            auto info = ptp_clock_ptsf[my_idx].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: ptp %d ptsf %s", my_idx, vtss_sync_ptsf_state_2_txt(info));
        }
    }

    notifications::Event e_ssf;
    notifications::Event e_ci_ql;
    notifications::Event e_ptp_clock_class;
    notifications::Event e_ptp_clock_ptsf;
    u32 my_idx;
    u32 my_port;
};
#endif
/*
 * EventHandler: eh_source_monitor:
 * replication: pr. nomination
 * Input:   all synce port related data
 * Output:  trace changes in input data
 * Description:
 *          Make a trace output for each port related variables, when updated.
 */
struct eh_source_monitor: public notifications::EventHandler {

    eh_source_monitor () :
        EventHandler(my_sr),
        e_ssf_n(this), e_ci_ql_n(this), e_nominated(this), e_locs_n(this), e_ssm_status_n(this), e_wtr_status_n(this)
    {
    }
    
    void init(u32 source) {
        my_source = source;
        ci_ssf_n[my_source].attach(e_ssf_n);
        ci_ql_n[my_source].attach(e_ci_ql_n);
        VTSS_ASSERT(my_source<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
        nomination_config[my_source].attach(e_nominated);
        LOCS_n[my_source].attach(e_locs_n);
        ssm_status[my_source].attach(e_ssm_status_n);
        wtr_status[my_source].attach(e_wtr_status_n);
    }

    void execute(notifications::Event *e) override {
        if (e == &e_ssf_n) {
            auto info = ci_ssf_n[my_source].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: source %d ci_ssf_n %s", my_source, info ? "true" : "false");
        } else if (e == &e_ci_ql_n) {
            auto info = ci_ql_n[my_source].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: source %d ci_ql_n %s", my_source, ssm_string(info));
        } else if (e == &e_nominated) {
            VTSS_ASSERT(my_source<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
            auto info = nomination_config[my_source].get(*e);
            u32 port=0;
            synce_network_port_clk_in_port_combo_to_port(info.network_port, info.clk_in_port, &port);
            T_WG(TRACE_GRP_CLOCK, "monitor: source %d nominated %s, port %d, priority %d, aneg_mode %s, ssm_overwrite %s, holdoff_time %d", 
                 my_source, 
                 info.nominated ? "true" : "false",
                 port, 
                 info.priority,
                 vtss_appl_synce_aneg_mode_txt[info.aneg_mode].valueName, 
                 ssm_string(info.ssm_overwrite),
                 info.holdoff_time);
        } else if (e == &e_locs_n) {
            auto info = LOCS_n[my_source].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: source %d LOCS_n %s", my_source, info ? "true" : "false");
        } else if (e == &e_ssm_status_n) {
            auto info = ssm_status[my_source].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: source %d ssm_status %s", my_source, info ? "true" : "false");
        } else if (e == &e_wtr_status_n) {
            auto info = wtr_status[my_source].get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: source %d wtr_status %s", my_source, info ? "true" : "false");
        }
    }

    notifications::Event e_ssf_n;
    notifications::Event e_ci_ql_n;
    notifications::Event e_nominated;
    notifications::Event e_locs_n;
    notifications::Event e_ssm_status_n;
    notifications::Event e_wtr_status_n;
    u32 my_source;
};

/*
 * EventHandler: eh_node_monitor:
 * replication: pr. ethernet port
 * Input:   all synce port related data
 * Output:  trace changes in input data
 * Description:
 *          Make a trace output for each port related variables, when updated.
 */
struct eh_node_monitor: public notifications::EventHandler {

    eh_node_monitor () :
        EventHandler(my_sr),
        e_selected_port(this), e_selected_source(this), e_station_clock_in(this), e_station_clock_out(this),  
        e_selected_ql(this), e_lol_dpll(this), e_dhold_dpll(this)
    {
    }
    
    void init() {
        selected_port.attach(e_selected_port);
        selected_source.attach(e_selected_source);
        station_clock_in.attach(e_station_clock_in);
        station_clock_out.attach(e_station_clock_out);
        selected_ql.attach(e_selected_ql);
        LOL_dpll.attach(e_lol_dpll);
        DHOLD_dpll.attach(e_dhold_dpll);
    }

    void execute(notifications::Event *e) override {
        if (e == &e_selected_port) {
            auto info = selected_port.get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: selected_port %u", info);
        } else if (e == &e_selected_source) {
            auto info = selected_source.get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: selected_source %u",info);
        } else if (e == &e_station_clock_in) {
            auto info = station_clock_in.get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: station clock in freq %u",info);
        } else if (e == &e_station_clock_out) {
            auto info = station_clock_out.get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: station clock out freq %u",info);
        } else if (e == &e_selected_ql) {
            auto info = selected_ql.get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: selected_ql %s",ssm_string(info));
        } else if (e == &e_lol_dpll) {
            auto info = LOL_dpll.get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: LOL_dpll %s",info ? "true" : "false");
        } else if (e == &e_dhold_dpll) {
            auto info = DHOLD_dpll.get(*e);
            T_WG(TRACE_GRP_CLOCK, "monitor: DHOLD_dpll %s",info ? "true" : "false");
        } else {
            T_WG(TRACE_GRP_CLOCK, "monitor: unknown event");
        }
    }

    notifications::Event e_selected_port;
    notifications::Event e_selected_source;
    notifications::Event e_station_clock_in;
    notifications::Event e_station_clock_out;
    notifications::Event e_selected_ql;
    notifications::Event e_lol_dpll;
    notifications::Event e_dhold_dpll;
};
/*
 * EventHandler: eh_station_clock_conf:
 * replication: pr. DPLL
 * Input:  station clock input and output frequency
 * Output: ci_ssf_p,ci_ql_p in DPLL
 * Description:
 *          Handles station clock input and output frequency configuration from management interfaces.
 */
struct eh_station_clock_conf: public notifications::EventHandler {

    eh_station_clock_conf() :
        EventHandler(my_sr),
        e_station_clock_in(this), e_station_clock_out(this),
        e_station_clock_source(this), e_locs_update(this)
    {
    }

    void init() {
        T_WG(TRACE_GRP_DEVELOP,"in station clock event handler constructor");
        station_clock_in.attach(e_station_clock_in);
        station_clock_out.attach(e_station_clock_out);
        station_clock_source.attach(e_station_clock_source);
        st_clk_src_svd = fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT);
        cur_locs = true;
        cur_stn_in_freq = VTSS_APPL_SYNCE_STATION_CLK_DIS;
        /* set default sf on station clock port to true */
        ssf_p[SYNCE_STATION_CLOCK_PORT].set(true);
    }
    void execute(notifications::Event *e) override {
        mesa_rc rc = SYNCE_RC_OK;
        u8 station_source_invalid = fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT);
        bool update_clk_in = false;
        if (e == &e_station_clock_in) {
            cur_stn_in_freq = station_clock_in.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"station clock in configuration is set to %u",cur_stn_in_freq);
            update_clk_in = true;
        } else if (e == &e_station_clock_out) {
            auto info = station_clock_out.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"station clock out configuration is set to %u",info);
            /* Apply station clock out configuration in DPLL */
            rc = clock_station_clk_out_freq_set(freq_khz(info));
            if (rc != SYNCE_RC_OK) {
                T_E("Station clock out freq = %u configuration failed",info);
            } else {
                T_WG(TRACE_GRP_DEVELOP,"Station clock-out freq = %u ,calc freq = %u configuration success",info ,freq_khz(info));
            }
        } else if (e == &e_station_clock_source) {
            T_WG(TRACE_GRP_DEVELOP,"event received is update of station clock source changed");
            update_clk_in = true;
            u8 src = station_clock_source.get(*e);
            if (src !=0 ) {
                st_clk_src_svd = src - 1;
                cur_locs = LOCS_n[st_clk_src_svd].get(e_locs_update);
                T_WG(TRACE_GRP_DEVELOP,"subscribing for update onf LOCS_n[%u]",st_clk_src_svd);
            } else if (st_clk_src_svd != station_source_invalid) {
                T_WG(TRACE_GRP_DEVELOP,"un-subscribing for update onf LOCS_n[%u]",st_clk_src_svd);
                LOCS_n[st_clk_src_svd].detach(e_locs_update);
                st_clk_src_svd = station_source_invalid;
            }
        } else if (e == &e_locs_update) {
            cur_locs = LOCS_n[st_clk_src_svd].get(*e);
            T_WG(TRACE_GRP_DEVELOP,"event received is update of LOCs on station clock source LOCS_n[%u]=%s",st_clk_src_svd ,cur_locs ? "TRUE" : "FALSE");
            update_clk_in = true;
        } else {
            T_WG(TRACE_GRP_DEVELOP,"event received is not of station clock type");
        }
        T_WG(TRACE_GRP_DEVELOP,"station clock is selected on nomination source %u \n",st_clk_src_svd);
        if (update_clk_in && (st_clk_src_svd != station_source_invalid)) {
            T_WG(TRACE_GRP_DEVELOP,"updating station clock input frequency in hardware on clock source %u",st_clk_src_svd);
            VTSS_ASSERT(st_clk_src_svd<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
            vtss_appl_synce_clock_source_nomination_config_t nomination = nomination_config[st_clk_src_svd].get();
            u32 port=0;
            synce_network_port_clk_in_port_combo_to_port(nomination.network_port, nomination.clk_in_port, &port);

            T_WG(TRACE_GRP_DEVELOP,"station input freq on nomination[%u] = %u, calc_freq = %u",st_clk_src_svd ,cur_stn_in_freq ,freq_khz(cur_stn_in_freq));
            clock_ref_clk_in_freq_set(st_clk_src_svd, freq_khz(cur_stn_in_freq));
            
            if (cur_stn_in_freq) {
                T_WG(TRACE_GRP_DEVELOP,"on nomination[%u] ci_ql_p[%u] = QL_DNU",st_clk_src_svd ,port);
                ci_ql_p[port].set(VTSS_APPL_SYNCE_QL_DNU);
            } else {
                T_WG(TRACE_GRP_DEVELOP,"on nomination[%u] ci_ql_p[%u] = QL_NONE",st_clk_src_svd ,port);
                ci_ql_p[port].set(VTSS_APPL_SYNCE_QL_NONE);
            }
            
            /* Get Station clock LOCS from DPLL to determine SSF */
            T_WG(TRACE_GRP_DEVELOP,"on nomination[%u] ci_ssf_p[%u] = %s",st_clk_src_svd ,port ,cur_locs ? "TRUE" : "FALSE");
            if (cur_stn_in_freq) {
                ssf_p[port].set(cur_locs);
            } else {
                ssf_p[port].set(true);
            }
        }
    }
    notifications::Event e_station_clock_in;
    notifications::Event e_station_clock_out;
    notifications::Event e_station_clock_source;
    notifications::Event e_locs_update;
    u8 st_clk_src_svd;
    bool cur_locs;
    vtss_appl_synce_frequency_t cur_stn_in_freq;
};

/*
 * EventHandler: eh_nomination_handler:
 * replication: pr. Nomination
 * Input:  Nomination configuration
 * Output: ci_ssf_p,ci_ql_p of nominated clock sources in DPLL
 * Description:
 *          Handles Nomination process with data given from management interfaces.
 */
struct eh_nomination_handler: public notifications::EventHandler {

    eh_nomination_handler() :
        EventHandler(my_sr),
        e_nomination_config(this), e_ssf_p(this), e_ci_ql(this), e_port_state(this)
    {
        nomination_prev.clk_in_port = 0;
        nomination_prev.priority = 0;
        nomination_prev.aneg_mode = VTSS_APPL_SYNCE_ANEG_NONE;
        nomination_prev.holdoff_time = 0;
        nomination_prev.ssm_overwrite = VTSS_APPL_SYNCE_QL_NONE;
        clk_in_source = 0;
    }

    void init(u8 id) {
        T_WG(TRACE_GRP_DEVELOP,"in nomination handler constructor");
        nomination_id = id;
        VTSS_ASSERT(nomination_id<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
        nomination_config[nomination_id].attach(e_nomination_config);
        ci_ssf_n[nomination_id].set(true);
        max_ports = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
    }
    void nomination_config_set(vtss_appl_synce_clock_source_nomination_config_t *config);
    void nomination_clock_source_set(u8 clk_in_source ,bool nominated);
    void execute(notifications::Event *e) override {
        bool new_ssf = true;
        bool do_rcvrd_set = false;
        vtss_appl_synce_quality_level_t new_ql = VTSS_APPL_SYNCE_QL_NONE;
        if (e == &e_nomination_config) {
            VTSS_ASSERT(nomination_id<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
            auto config_s = nomination_config[nomination_id].get(*e);
            u32 port;
            T_WG(TRACE_GRP_DEVELOP,"nomination configuration of id %u has been updated",nomination_id);
            /*
               nominated     -> to be handled here
               network_port  -> to be handled here
               clk_in_port   -> to be handled here
               priority      -> to be handled in selection_process()
               aneg_mode     -> to be handled in aneg_1000baset_master()
               ssm_overwrite -> to be handled here
               holdoff_time -> to be handled in hold_wtr()
             */
            synce_network_port_clk_in_port_combo_to_port(config_s.network_port, config_s.clk_in_port, &port);
            T_WG(TRACE_GRP_DEVELOP,"nominated %s, network_port %u, clk_in_port %u, priority %u, aneg_mode %u, ssm_overwrite %u, hold_off_time %u, port_cal %u",config_s.nominated ? "YES" : "NO", config_s.network_port, config_s.clk_in_port, config_s.priority, config_s.aneg_mode, config_s.ssm_overwrite, config_s.holdoff_time,port);

            if(!config_s.nominated) {
                if (nomination_prev.nominated) {
                    T_WG(TRACE_GRP_DEVELOP,"Un-subscribing for change of ssf,ci_ql events on port %u",clk_in_source);
                    /* handle de-nomination */
                    /* un-subscribe the change in ssf from the port which is de-nominated */
                    ssf_p[clk_in_source].detach(e_ssf_p);
                    /* un-subscribe the change in ql from the port which is de-nominated */
                    ci_ql_p[clk_in_source].detach(e_ci_ql);
                    /* un-subscribe the change in port events,which is denominated */
                    if (clk_in_source < max_ports) {
                        port_info[clk_in_source].detach(e_port_state);
                    }
                    // Clear any LOS associated with this nomination.
                    clock_link_state_set(nomination_id, true);
                }
            } else {
                if (!nomination_prev.nominated) {
                    T_WG(TRACE_GRP_DEVELOP,"Subscribing for change of ssf,ci_ql events on port %u",port);
                    /* subscribe the change in ssf from the port which is nominated */
                    ssf_p[port].attach(e_ssf_p);
                    /* subscribe the change in ql from the port which is nominated */
                    ci_ql_p[port].attach(e_ci_ql);
                    /* subscribe the change in port_state ,on the port which is nominated */
                    if (port < max_ports) {
                        my_port_state = port_info[port].get(e_port_state);
                    }
                } else if (nomination_prev.nominated && (port != clk_in_source)) {
                    /* if clock source is changed in nomination */
                    /* detach old subcription */
                    T_WG(TRACE_GRP_DEVELOP,"Un-subscribing for change of ssf,ci_ql events on port %u",clk_in_source);
                    ssf_p[clk_in_source].detach(e_ssf_p);
                    ci_ql_p[clk_in_source].detach(e_ci_ql);
                    if (clk_in_source < max_ports) {
                        port_info[clk_in_source].detach(e_port_state);
                    }
                    // Clear any LOS associated with this nomination due to previous port.
                    clock_link_state_set(nomination_id, true);

                    /* attach new subcription */
                    T_WG(TRACE_GRP_DEVELOP,"Subscribing for change of ssf,ci_ql events on port %u",port);
                    ssf_p[port].attach(e_ssf_p);
                    ci_ql_p[port].attach(e_ci_ql);
                    if (port < max_ports) {
                        my_port_state = port_info[port].get(e_port_state);
                    }

                    T_WG(TRACE_GRP_DEVELOP,"Modified nomination source from %u to %u \n",clk_in_source ,port);

                    /* set station clock source to 0 ,if previous nominated source is station clock */
                    if (clk_in_source == SYNCE_STATION_CLOCK_PORT) {
                        station_clock_source.set(0);
                    }
                }
            }
            /* If station clock is nominated or de-nominated inform station clock handler */
            if (port == SYNCE_STATION_CLOCK_PORT) {
                if (config_s.nominated) {
                    T_WG(TRACE_GRP_DEVELOP,"station clock being nominated on nomination id %u , clk source port %u",nomination_id ,port);
                    station_clock_source.set((nomination_id+1));
                } else {
                    T_WG(TRACE_GRP_DEVELOP,"station clock being de-nominated on nomination id %u , clk source port %u",nomination_id ,port);
                    station_clock_source.set(0);
                }
            }

            //nomination_config_set(&config_s); /* I dont belong here,To be moved out of nomination handler */
            if (config_s.nominated != nomination_prev.nominated || config_s.network_port != nomination_prev.network_port || config_s.clk_in_port != nomination_prev.clk_in_port || config_s.aneg_mode != nomination_prev.aneg_mode) {
                do_rcvrd_set = true;
            }

            /* saving current state */
            nomination_prev = config_s;
            clk_in_source = port;
        } else if (e == &e_ssf_p) {
            T_WG(TRACE_GRP_DEVELOP,"ssf received on nomination id %u , ci_ssf_p[%u] = %s",nomination_id ,clk_in_source ,ssf_p[clk_in_source].get()? "true" : "false");
            do_rcvrd_set = ssf_p[clk_in_source].get() ? false : true;
        } else if (e == &e_ci_ql) {
            T_WG(TRACE_GRP_DEVELOP,"ql/ssm received on nomination id %u , ci_ql_p[%u] = %u",nomination_id ,clk_in_source ,ci_ql_p[clk_in_source].get());
        } else if (e == &e_port_state) {
            auto info = port_info[clk_in_source].get(*e);
            if (info.link != my_port_state.link) {
                clock_link_state_set(nomination_id, info.link ? true : false);
            }
            T_WG(TRACE_GRP_DEVELOP,"port_state modified even received on nomination id %u , port[%u]",nomination_id ,clk_in_source);
            if (!ssf_p[clk_in_source].get() && info.link && (my_port_state.speed != info.speed || my_port_state.fiber != info.fiber || my_port_state.link != info.link) ) {
                do_rcvrd_set = true;
                T_WG(TRACE_GRP_DEVELOP,"port_speed[%u] = %u from %u under nomination[%u] ",clk_in_source ,info.speed ,my_port_state.speed ,nomination_id);
            }
            my_port_state = info;
        } else {
            T_WG(TRACE_GRP_DEVELOP,"expected event not received on nomination id %u ",nomination_id);
        }
        if (nomination_prev.nominated) {
            new_ssf = ssf_p[clk_in_source].get(e_ssf_p);
            new_ql = ci_ql_p[clk_in_source].get(e_ci_ql);
            /* update ssf */
            ci_ql_n[nomination_id].set((nomination_prev.ssm_overwrite != VTSS_APPL_SYNCE_QL_NONE) ? nomination_prev.ssm_overwrite : new_ql);
            /* update ssf */
            ci_ssf_n[nomination_id].set(new_ssf);
            ssm_status[nomination_id].set((new_ql == VTSS_APPL_SYNCE_QL_FAIL || new_ql == VTSS_APPL_SYNCE_QL_INV) ? true : false);
            T_WG(TRACE_GRP_DEVELOP,"SSM[%u] = %s, port %u",nomination_id, ssm_status[nomination_id].get() ? "BAD" : "OK" ,clk_in_source);
        } else {
            T_WG(TRACE_GRP_DEVELOP,"Source nomination[%u] = false , so setting *defaults* ci_ql_n = QL_NONE ci_ssf_n = true \n",nomination_id);
            ci_ql_n[nomination_id].set(VTSS_APPL_SYNCE_QL_NONE);
            /* update ssf */
            ci_ssf_n[nomination_id].set(true);
        }
        for (auto i=0;i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT);i++) {
            T_WG(TRACE_GRP_DEVELOP,"ci_ql_n[%u] = %u, ci_ssf_n[%u] = %s",i ,ci_ql_n[i].get() ,i ,ci_ssf_n[i].get()? "true" : "false") ;
        }
        if (do_rcvrd_set) {
            T_WG(TRACE_GRP_DEVELOP,"Do recovered clock setup on nomination_config[%u]",nomination_id);
            nomination_clock_source_set(clk_in_source ,nomination_prev.nominated);
        } else {
            T_WG(TRACE_GRP_DEVELOP,"Not doing recovered clock setup on nomination_config[%u]",nomination_id);
        }
    }
    notifications::Event e_nomination_config;
    notifications::Event e_ssf_p;
    notifications::Event e_ci_ql;
    notifications::Event e_port_state;
    u8 nomination_id;
    u8 clk_in_source;
    vtss_appl_synce_clock_source_nomination_config_t nomination_prev;
    vtss_appl_port_status_t my_port_state;
    u32 max_ports;
};
void eh_nomination_handler::nomination_clock_source_set(u8 port ,bool conf){ 

    vtss_appl_port_status_t port_state = port_info[0].get(); /* Just to satisfy Compiler */
    mepa_synce_clock_dst_t phy_clk_port = MEPA_SYNCE_CLOCK_DST_MAX;
    mesa_synce_clock_in_t  clk_in;
    mesa_synce_clock_out_t clk_out;
    mesa_synce_clk_port_t  switch_clk_port = fast_cap(MESA_CAP_SYNCE_CLK_CNT);
    mepa_synce_clock_conf_t phy_conf;
    meba_synce_clock_frequency_t dpll_input_frequency;
    int clk_sel_ref,ret_val = 0;
    bool ssf_n = true;
    bool is_eth_port = false;
    bool is_ptp_port = false;
    bool is_station_clk_port = false;
    uint max_synce_clk_inputs = switch_clk_port;

    ssf_n = vtss::ci_ssf_n[nomination_id].get();
    is_ptp_port = (port >= fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT)) ? true : false ;
    is_station_clk_port = (port == SYNCE_STATION_CLOCK_PORT) ? true : false ;
    is_eth_port = (!is_ptp_port && !is_station_clk_port )? true : false;

    T_WG(TRACE_GRP_DEVELOP,"port %u :: is_ptp %s is_station %s is_eth %s \n",port ,is_ptp_port? "true" : "false" ,is_station_clk_port ? "true" : "false" ,is_eth_port? "true" : "false");
    T_WG(TRACE_GRP_DEVELOP,"recovered clock setup is being %s on nomination[%u] with port %u \n", conf? "set" : "cleared" ,nomination_id ,port);
    u32 input = synce_get_mux_selector_w_attr(nomination_id, port);

    if (is_eth_port) {
        port_state = port_info[port].get();
        if (port_state.static_caps & MEBA_PORT_CAP_1G_PHY) {
            if (port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                /* Clock source is a PHY port */
                phy_clk_port = (mepa_synce_clock_dst_t) synce_get_phy_recovered_clock(nomination_id, port);
                T_WG(TRACE_GRP_DEVELOP,"phy_clk_port[%u] = %u with front port %u ",nomination_id ,phy_clk_port ,port);
                if (conf) {
                    phy_conf.src = (phy_clk_port == MEPA_SYNCE_CLOCK_DST_MAX) ?  MEPA_SYNCE_CLOCK_SRC_DISABLED : (port_state.fiber ? MEPA_SYNCE_CLOCK_SRC_SERDES_MEDIA :((input & MESA_SYNCE_TRI_STATE_FROM_SWITCH )? MEPA_SYNCE_CLOCK_SRC_DISABLED : MEPA_SYNCE_CLOCK_SRC_COPPER_MEDIA));
                    /* Configure clock selection in PHY */
                    meba_synce_clock_frequency_t phy_output_frequency = synce_get_phy_output_frequency(nomination_id, port, port_state.speed);
                    switch (phy_output_frequency) {
                        case MEBA_SYNCE_CLOCK_FREQ_25MHZ:
                            phy_conf.freq = MEPA_FREQ_25M;
                            break;
                        case MEBA_SYNCE_CLOCK_FREQ_125MHZ:
                            phy_conf.freq = MEPA_FREQ_125M;
                            break;
                        default:
                            phy_conf.freq = MEPA_FREQ_125M;
                            T_WG(TRACE_GRP_DEVELOP,"Unsupported frequency %d", (int) phy_output_frequency);
                    }
                } else {
                    phy_conf.src = MEPA_SYNCE_CLOCK_SRC_DISABLED;
                    phy_conf.freq = MEPA_FREQ_125M;
                }
                phy_conf.dst = phy_clk_port;
                T_WG(TRACE_GRP_DEVELOP,"phy_conf src %u , freq %u", phy_conf.src, phy_conf.freq);
                if (meba_phy_synce_clock_conf_set(board_instance, port, &phy_conf) != VTSS_RC_OK) {
                    T_WG(TRACE_GRP_DEVELOP,"meba_phy_synce_clock_conf_set() error returned port %u  ", port);
                }

            }

        } 
    }
    if (fast_cap(MESA_CAP_SYNCE)) {
        if (!is_ptp_port) {
            if((ret_val = synce_get_switch_recovered_clock(nomination_id, port)) >=0 ) {
                switch_clk_port = ret_val;
            }
        }
#if defined(VTSS_SW_OPTION_PTP)
        else if ((nomination_id >= fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT) - 1) || (port >= fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT))) {  // PTP 8265.1 ports exist for all but the last nomination_id associated with the station clock
            T_WG(TRACE_GRP_DEVELOP,"invalid clock nomination_id %u or port %d", nomination_id, port);
            return;
        }
        else {
            // In case of PTP clocks, assume port is 0 (also done below). The input is going to be squelshed anyway.
            if((ret_val = synce_get_switch_recovered_clock(nomination_id, 0)) >=0 ) {
                switch_clk_port = ret_val;
            }
        }
#else
        else {
            T_WG(TRACE_GRP_DEVELOP,"invalid clock nomination_id %u or port %d", nomination_id, port);
            return;
        }
#endif
    }
    if((ret_val = synce_get_switch_recovered_clock(nomination_id, port)) >=0 ) {
        switch_clk_port = ret_val;
    }
    T_WG(TRACE_GRP_DEVELOP,"ci_ssf_n[%u] = %s ,switch_clk_port %d,fast_cap(MESA_CAP_SYNCE_USE_SWITCH_SELECTOR_WITH_PHY) = %s",nomination_id ,ssf_n? "true" : "false",switch_clk_port ,fast_cap(MESA_CAP_SYNCE_USE_SWITCH_SELECTOR_WITH_PHY) ? "true" : "false"); 
   /* MI:nominated && (switch_clk_port >= 0) && !SF && !ptpPort &&(!port_info.phy || fast_cap(MESA_CAP_SYNCE_USE_SWITCH_SELECTOR_WITH_PHY)) */
    clk_in.enable = conf && (switch_clk_port != max_synce_clk_inputs) && !ssf_n && !(is_ptp_port) && (!(port_state.static_caps & MEBA_PORT_CAP_1G_PHY) || fast_cap(MESA_CAP_SYNCE_USE_SWITCH_SELECTOR_WITH_PHY)) ;
    clk_in.port_no = port;
    if (fast_cap(MESA_CAP_SYNCE_IN_TYPE)) {
        if (!is_station_clk_port) {
            clk_in.clk_in = MESA_SYNCE_CLOCK_INTERFACE;
        } else {
            clk_in.clk_in = MESA_SYNCE_CLOCK_STATION_CLK;
            clk_in.port_no = 0; // only support for one station clock input with index 0
            if ((mesa_synce_clock_in_set(NULL,  switch_clk_port ,  &clk_in)) != VTSS_RC_OK)    T_WG(TRACE_GRP_DEVELOP,"error returned port %u  ", port);
        }
    }

    if (is_eth_port) {
        /* !clk_in.enable ? false : ((100M port_info.fiber ? false : SWITCH_AUTO_SQUELCH)) */
        clk_in.squelsh = !clk_in.enable ? false :( ((port_state.speed == MESA_SPEED_100M) && port_state.fiber) ? false : SWITCH_AUTO_SQUELCH);
        T_WG(TRACE_GRP_DEVELOP,"mesa_synce_clock_in_set  switch_clk_port %d  clk_in.enable %s  clk_in.port_no %u clk_in.squelsh %u", switch_clk_port, clk_in.enable ? "true" : "false", clk_in.port_no ,clk_in.squelsh);
        if ((mesa_synce_clock_in_set(NULL,  switch_clk_port ,  &clk_in)) != VTSS_RC_OK)    T_WG(TRACE_GRP_DEVELOP,"error returned port %u  ", port);

        if (switch_clk_port  != max_synce_clk_inputs) {
            /* clock out set configuration */
            /* clk_out.enable = MI:nominated && (switch_clk_port >= 0) && !SF && !ptpPort &&(!port_info.phy || fast_cap(MESA_CAP_SYNCE_USE_SWITCH_SELECTOR_WITH_PHY)) */
            clk_out.enable = conf && !ssf_n && !(is_ptp_port) && (!(port_state.static_caps & MEBA_PORT_CAP_1G_PHY) || fast_cap(MESA_CAP_SYNCE_USE_SWITCH_SELECTOR_WITH_PHY)) ;
            clk_out.divider = synce_get_switch_clock_divider(nomination_id, port, port_state.speed);
            T_WG(TRACE_GRP_DEVELOP,"clk_out[%u] with port %u , clk_out.enable %s , clk_out.divider %u",nomination_id ,port ,clk_out.enable ? "true" : "false" ,clk_out.divider);
            if (input & MESA_SYNCE_TRI_STATE_FROM_PHY) {
                /* if the port is of internal PHY */
                if (mesa_synce_clock_out_set(NULL, phy_clk_port , &clk_out) != VTSS_RC_OK) {
                    T_WG(TRACE_GRP_DEVELOP,"mesa_synce_clock_out_set returned error on nomination id %u",nomination_id);
                }
            } else {
                if (mesa_synce_clock_out_set(NULL, switch_clk_port , &clk_out) != VTSS_RC_OK) {
                    T_WG(TRACE_GRP_DEVELOP,"mesa_synce_clock_out_set returned error on nomination id %u",nomination_id);
                }
            }
        } else {
            if ((input & MESA_SYNCE_TRI_STATE_FROM_PHY)) {                                                                                                

                clk_out.enable = true;     // Enable for clock output from switch                                                                                                  
                clk_out.divider = synce_get_switch_clock_divider(nomination_id, port, port_state.speed);                                                                            
                if (mesa_synce_clock_out_set(NULL, phy_clk_port, &clk_out) != VTSS_RC_OK)  T_WG(TRACE_GRP_DEVELOP,"mesa_synce_clock_out_set returned error");  
            }                                                                                                                                                                      

        }
    }
    if (is_eth_port || (is_station_clk_port && fast_cap(MEBA_CAP_SYNCE_STATION_CLOCK_MUX_SET))) {
        /* set up the multiplexer in the CPLD in cases where applicable */
        if (conf) {
            T_WG(TRACE_GRP_DEVELOP,"Set Mux for nomination %u with port %u",nomination_id ,port);
            synce_mux_set(nomination_id ,port);
        } else {
            T_WG(TRACE_GRP_DEVELOP,"No need to set Mux for nomination %u with port %u",nomination_id ,port);
        }
    }

    /* set clock selector map*/
    clk_sel_ref = synce_get_selector_ref_no(nomination_id, port);
    if (dpll_type == VTSS_ZL_30380_DPLL_ZLS3077X && clk_sel_ref == (uint)-1) {
        clk_sel_ref = ZL3077X_PTP_REF_ID;
    }
    if (dpll_type == VTSS_ZL_30380_DPLL_ZLS3073X && clk_sel_ref == (uint)-1) {
        clk_sel_ref = ZL3073X_PTP_REF_ID;
    }
    T_WG(TRACE_GRP_DEVELOP,"clock selector = %d for source %u with port = %u",clk_sel_ref ,nomination_id ,port);
    if (clock_selector_map_set(clk_sel_ref, nomination_id) != VTSS_RC_OK) {
        T_WG(TRACE_GRP_DEVELOP,"error returned clock_selector_map_set()");
    }

    if (is_eth_port  && conf) {
        dpll_input_frequency = synce_get_dpll_input_frequency(nomination_id ,port ,port_state.speed);
        /* Configure clock selection in PHY */
        u32 dpll_input_frequency_khz = 125000;
        switch (dpll_input_frequency) {
            case MEBA_SYNCE_CLOCK_FREQ_25MHZ:
                dpll_input_frequency_khz = 25000;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_31_25MHZ:
                dpll_input_frequency_khz = 31250;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_32_226MHZ:
                dpll_input_frequency_khz = 32226;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_39_062MHZ:
                dpll_input_frequency_khz = 39062;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_40_283MHZ:
                dpll_input_frequency_khz = 40283;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_60_606MHZ:
                dpll_input_frequency_khz = 60606;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_62_5MHZ:
                dpll_input_frequency_khz = 62500;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_78_125MHZ:
                dpll_input_frequency_khz = 78125;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_80_565MHZ:
                dpll_input_frequency_khz = 80566;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_125MHZ:
                dpll_input_frequency_khz = 125000;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_156_25MHZ:
                dpll_input_frequency_khz = 156250;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_161_13MHZ:
                dpll_input_frequency_khz = 161130;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_312_5MHZ:
                dpll_input_frequency_khz = 312500;
                break;
            case MEBA_SYNCE_CLOCK_FREQ_322_265MHZ:
                dpll_input_frequency_khz = 322265;
                break;
            default:
                T_D("Unsupported frequency %d, fall back to 125Mhz", (int) dpll_input_frequency);
        }
        T_WG(TRACE_GRP_DEVELOP,"nomination id %u , dpll_input_frequency %u ,dpll_input_frequency_khz %u",nomination_id ,dpll_input_frequency ,dpll_input_frequency_khz);

        if (clock_ref_clk_in_freq_set(nomination_id, dpll_input_frequency_khz) != MESA_RC_OK) {
            T_WG(TRACE_GRP_DEVELOP,"Could not set dpll input frequency Khz %d.", dpll_input_frequency_khz);
        }
    }
    update_clk_in_selector(nomination_id);

}

#if defined(VTSS_SW_OPTION_PTP)
/*
 * EventHandler: eh_ptp_config_handler:
 * replication: pr. ptp instance
 * Input:  ptp clock class, ptsf
 * Output: ci_ssf_p,ci_ql_p of nominated ptp instnaces in DPLL
 * Description:
 *         Receives ptp clock class and ptsf information from PTP module and set ssm,ssf accordingly.
 */
struct eh_ptp_config_handler: public notifications::EventHandler {

    eh_ptp_config_handler() :
        EventHandler(my_sr),
        e_ptp_clock_class(this), e_ptp_clock_ptsf(this) ,e_clock_selection_config(this)
    {
    }

    void init(u8 id) {
        T_WG(TRACE_GRP_DEVELOP,"in ptp configuration event handler");
        ptp_inst_id = id;
        ptp_clock_class[ptp_inst_id].attach(e_ptp_clock_class);
        ptp_clock_ptsf[ptp_inst_id].attach(e_ptp_clock_ptsf);
        ptp_src_port = ptp_inst_id + fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT);
        cur_eec_option = clock_selection_config.get(e_clock_selection_config).eec_option;
        /* By default set SF = true */
        ssf_p[ptp_src_port].set(true);
    }
    void execute(notifications::Event *e) override {
        if (e == &e_ptp_clock_class) {
            auto info = ptp_clock_class[ptp_inst_id].get(*e);
            bool ssm_aligned = false;
            u8 ssm_rx = 0;
            switch(info) {
                case 6 : ssm_rx = SSM_QL_PRC; break;
                case 80 : ssm_rx = SSM_QL_PRS_INV; break;
                case 82 : ssm_rx = SSM_QL_STU; break;
                case 84 : ssm_rx = SSM_QL_PRC; break;
                case 86 : ssm_rx = SSM_QL_ST2; break;
                case 90 : ssm_rx = SSM_QL_SSUA_TNC; break;
                case 96 : ssm_rx = SSM_QL_SSUB; break;
                case 100 : ssm_rx = SSM_QL_ST3E; break;
                case 102 : ssm_rx = SSM_QL_EEC2; break;
                case 104 : ssm_rx = SSM_QL_EEC1; break;
                case 106 : ssm_rx = SSM_QL_SMC; break;
                case 108 : ssm_rx = SSM_QL_PROV; break;
                case 110 : ssm_rx = SSM_QL_DNU_DUS; break;
                default: ssm_rx = SSM_QL_FAIL; break;
            }
            ssm_aligned = is_ssm_aligned(cur_eec_option ,ssm_rx);
            T_WG(TRACE_GRP_DEVELOP,"New ptp_clock_class[%u] = %u == ssm(%u) clock input %u",ptp_inst_id ,info ,ssm_rx ,ptp_src_port);
            ci_ql_p[ptp_src_port].set(ssm_aligned ? ssm_to_ql(ssm_rx ,cur_eec_option) : VTSS_APPL_SYNCE_QL_INV);
            cur_ssm = ssm_rx;
        } else if (e == &e_clock_selection_config) {
            auto info = clock_selection_config.get(*e).eec_option;
            if (info != cur_eec_option) {
                bool ssm_aligned = false;
                cur_eec_option = info;
                ssm_aligned = is_ssm_aligned(cur_eec_option ,cur_ssm);
                ci_ql_p[ptp_src_port].set(ssm_aligned ? ssm_to_ql(cur_ssm ,cur_eec_option) : VTSS_APPL_SYNCE_QL_INV);
                T_WG(TRACE_GRP_DEVELOP,"ptp_inst[%d] change in clock selection configuration  , eec_option = %u",ptp_src_port ,cur_eec_option);
            }
        } else if (e == &e_ptp_clock_ptsf) {
            T_WG(TRACE_GRP_DEVELOP,"New ptp_clock_ptsf[%u] = %s, port %u",ptp_inst_id ,vtss_sync_ptsf_state_2_txt(ptp_clock_ptsf[ptp_inst_id].get(*e)) ,ptp_src_port);
            if (ptp_clock_ptsf[ptp_inst_id].get(*e) > SYNCE_PTSF_UNUSABLE) {
                /* considering as a signal failure */
                ssf_p[ptp_src_port].set(true);
            } else {
                /* considering as valid clock source */
                ssf_p[ptp_src_port].set(false);
            }
        } else {
            T_WG(TRACE_GRP_DEVELOP,"un-expected event on ptp instance %u",ptp_inst_id);
        }
    }
    u8 ptp_inst_id;
    u32 ptp_src_port;
    uint cur_ssm;
    notifications::Event e_ptp_clock_class;
    notifications::Event e_ptp_clock_ptsf;
    notifications::Event e_clock_selection_config;
    vtss_appl_synce_eec_option_t cur_eec_option;
};

/*
 * EventHandler: eh_ptp_transient_processing:
 * replication: pr. None 
 * Input:  selected_ql ,selected port ,LOL ,dhold
 * Output: hybrid transient mode 
 * Description:
 *         sets hybrid transient mode by taking inputs into consideration .
 */
struct eh_ptp_transient_processing: public notifications::EventHandler {

    eh_ptp_transient_processing () :
        EventHandler(my_sr) ,e_selected_ql(this) ,e_selected_source(this) ,e_LOL_dpll(this) ,e_DHOLD_dpll(this), e_ptp_hybrid_mode(this)
    {
    }

    void init() {
        selected_source.attach(e_selected_source);
        selected_ql.attach(e_selected_ql);
        LOL_dpll.attach(e_LOL_dpll);
        DHOLD_dpll.attach(e_DHOLD_dpll);
        ptp_hybrid_mode.attach(e_ptp_hybrid_mode);

        cur_selected_source = selected_source.get();
        cur_selected_ql = selected_ql.get();
        cur_lol = LOL_dpll.get();
        cur_dhold = DHOLD_dpll.get();
        cur_selected_port = fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT);
        cur_hybrid = ptp_hybrid_mode.get();
        transient_set = true; //when synce is not started, it is considered lon term transient state.
    }

    void execute(notifications::Event *e) override {
        bool is_transient = false;

        if (e == &e_selected_ql) {
            cur_selected_ql = selected_ql.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_ptp_transient_processing :: received change in selected_ql = %u",cur_selected_ql);
        } else if (e == &e_selected_source) {
            cur_selected_source = selected_source.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_ptp_transient_processing :: received change in selected_source = %u",cur_selected_source);
            if(cur_selected_source) {
                VTSS_ASSERT(cur_selected_source-1<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
                auto config_s = nomination_config[cur_selected_source - 1].get();
                u32 port;
                synce_network_port_clk_in_port_combo_to_port(config_s.network_port, config_s.clk_in_port, &port);
                cur_selected_port = port;
            } else {
                cur_selected_port = fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT);
            }

        } else if (e == &e_LOL_dpll) {
            cur_lol = LOL_dpll.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_ptp_transient_processing :: received change in LOL_dpll %s",cur_lol ? "TRUE" : "FALSE");
        } else if (e == &e_DHOLD_dpll) {
            cur_dhold = DHOLD_dpll.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_ptp_transient_processing :: received change in DHOLD_dpll %s",cur_dhold ? "TRUE" : "FALSE");
        } else if (e == &e_ptp_hybrid_mode) {
            cur_hybrid = ptp_hybrid_mode.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_ptp_transient_processing :: received change in hybrid mode ");
        } else {
            T_WG(TRACE_GRP_DEVELOP,"eh_ptp_transient_processing :: received unexpected event");
        }

        /*
        (LOL || (CI_QL != PRC && CI_QL != INV)) && !dHold && (selected_port != internal source);
         */

        vtss_appl_synce_eec_option_t cur_eec_option = clock_selection_config.get().eec_option; 

        // if cur_selected_source is not available, then transient must be true.
        // When cur_dhold is true, synce should not be used and hence transient should be true.
        if (cur_eec_option == VTSS_APPL_SYNCE_EEC_OPTION_1) {
            is_transient = cur_lol || (cur_selected_ql != VTSS_APPL_SYNCE_QL_PRC && cur_selected_ql != VTSS_APPL_SYNCE_QL_INV) || cur_dhold || !cur_selected_source;
        } else {
            is_transient = cur_lol || (cur_selected_ql != VTSS_APPL_SYNCE_QL_PRS && cur_selected_ql != VTSS_APPL_SYNCE_QL_INV) || cur_dhold || !cur_selected_source;
        }
        T_WG(TRACE_GRP_DEVELOP,"LOL %s ,dhold %s ,sel_ql %u ,sel_source %u ,sel_port %u transient %s",cur_lol? "TRUE" : "FALSE" ,cur_dhold? "TRUE": "FALSE" ,cur_selected_ql ,cur_selected_source ,cur_selected_port ,is_transient? "TRUE" : "FALSE" );

        if (is_transient != transient_set) {
            if (is_transient) {
                if (cur_hybrid) {// transient must be detected only if current mode is hybrid.
                    if (vtss_ptp_set_hybrid_transient(VTSS_PTP_HYBRID_TRANSIENT_QUICK) != VTSS_RC_OK) {
                        T_D("Could not set hybrid transient to VTSS_PTP_HYBRID_TRANSIENT_QUICK");
                    }
                }
            } else {
                if (vtss_ptp_set_hybrid_transient(VTSS_PTP_HYBRID_TRANSIENT_NOT_ACTIVE) != VTSS_RC_OK) {
                    T_D("Could not set hybrid transient to VTSS_PTP_HYBRID_TRANSIENT_NOT_ACTIVE");
                }
            }
            transient_set = is_transient;
        }
    }
    bool transient_set;
    vtss_appl_synce_quality_level_t cur_selected_ql;
    u8 cur_selected_source;
    u8 cur_selected_port;
    bool cur_lol;
    bool cur_dhold;
    bool cur_hybrid;
    notifications::Event e_selected_ql;
    notifications::Event e_selected_source;
    notifications::Event e_LOL_dpll;
    notifications::Event e_DHOLD_dpll;
    notifications::Event e_ptp_hybrid_mode;
};
#endif
/*
 * EventHandler: eh_dpll_status_processor:
 * replication: 1 pr. synce application 
 * Input:   t_ssm_timer
 * Output:  LOCS,LOL,LOSX and DHOLD
 * Description:
 *         Checks DPLL status in timely manner .
 */
struct eh_dpll_status_processor: public notifications::EventHandler {

    eh_dpll_status_processor () :
        EventHandler(my_sr),
        t_ssm_timer(this)
    {
        dpll_status_interval = 1000;
    }

    void init() {
        my_sr->timer_add(t_ssm_timer, (milliseconds) dpll_status_interval);
    }

    void execute(notifications::Timer *t) override {
        if (t == &t_ssm_timer) {
            T_D("1s expired: time to check dpll status");
            /* DPLL health check */
            clock_event_type_t ev_mask = 0;
            clock_event_poll(false ,&ev_mask);
            T_D("clock event mask 0x%0x ",ev_mask);

            /* Get LOCs status */
            for (auto i=0;i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT);i++) {
                bool locs = true;
                VTSS_ASSERT(i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
                vtss_appl_synce_clock_source_nomination_config_t config_s = nomination_config[i].get();
                if (config_s.nominated) {
                    u32 port=0;
                    synce_network_port_clk_in_port_combo_to_port(config_s.network_port, config_s.clk_in_port, &port);
#if defined(VTSS_SW_OPTION_PTP)
                    /* check if clock source is a PTP instance or not */
                    if (port >= fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT)) {
                        /* If clock source nominated is a PTP instance get LOCS status from PTP_PTSF */
                        locs = ptp_clock_ptsf[port-fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT)].get() <= SYNCE_PTSF_UNUSABLE ? false : true;
                    } else 
#endif
                    {
                        /* If clock source nominated is ethernet port or station clock */
                        if (clock_locs_state_get(i, &locs) != VTSS_RC_OK) {
                            T_D("clock_locs_state_get: error returned on nomination source %u",i);
                        }
                    }
                }
                /* Not nominated , set to default value */
                LOCS_n[i].set(locs);
                T_D("nominated[%u]=%s LOCS_n[%u] = %s",i ,config_s.nominated? "true" : "false" ,i ,locs ? "true" : "false");
            }
            /* Get latest status of LOL from DPLL */

            bool state = true;
            clock_lol_state_get(&state);
            LOL_dpll.set(state);
            T_D("DPLL_LOL = %s ",state ? "true" : "false");

            state = true;
            clock_losx_state_get(&state);
            LOSX_dpll.set(state);
            T_D("DPLL_LOSX = %s ",state ? "true" : "false");

            state = true;
#if defined(VTSS_SW_OPTION_PTP)
            auto ptp_clk_inst_slctd = best_master.get();
            if (ptp_clk_inst_slctd != fast_cap(VTSS_APPL_CAP_PTP_CLOCK_CNT)) {
                state = vtss_ptp_servo_get_holdover_status(ptp_clk_inst_slctd);
                DHOLD_dpll.set(!state);
                T_D("PTP :: DPLL_DHOLD = %s ",!state ? "true" : "false");
            } else 
#endif                
            {
                clock_dhold_state_get(&state);
                DHOLD_dpll.set(!state);
                T_D("DPLL_DHOLD = %s ",!state ? "true" : "false");
            }

            vtss_appl_synce_selector_state_t selector_state_copy = VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN;
            uint clock_input_copy;
            if (clock_selector_state_get(&clock_input_copy, &selector_state_copy) == VTSS_RC_OK) {
                selector_state.set(selector_state_copy);
                clock_input.set(clock_input_copy);
                T_D("DPLL_selector state  = %u ",selector_state_copy);
            } else {
                T_D("error clock_selector_state_get ");
            }


            my_sr->timer_add(t_ssm_timer, (milliseconds) dpll_status_interval);
        }
    }
    notifications::Timer t_ssm_timer;
    uint dpll_status_interval;
};
/*
 * EventHandler: eh_ho_wtr_processor:
 * replication: 1 pr. nominated source 
 * Input:  hold_offtime,wtr_time and clearWtr
 * Output: final QL[0 - N], SF[0-N] including internal source 
 * Description:
 *         Handle hold off timer and wait to restore feature .
 */
struct eh_ho_wtr_processor: public notifications::EventHandler {

    eh_ho_wtr_processor () :
        EventHandler(my_sr) ,e_nomination_config(this) ,e_ci_ql_n(this) ,e_ci_ssf_n(this) ,e_clock_selection(this),
        e_clear_wtr(this) ,t_holdoff_timer(this) ,t_wtr_timer(this) ,t_wtr_helper_timer(this)
    {
        nomination_id = 0;
        nominated_prev = false;
        holdoff_prev = 0;
        wtr_time_prev = 0;
        holdoff_timeout = true;
        wtr_timeout = true;
        cur_ci_ssf_n = false;
        clear_wtr_ev = false;
        wtr_helper_timeout = false;
    }

    void init(u8 id) {
        T_WG(TRACE_GRP_DEVELOP,"in eh_ho_wtr_processor id %u",id);
        nomination_id = id;
        /* subscribe with required subjects */
        VTSS_ASSERT(nomination_id<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
        nomination_config[nomination_id].attach(e_nomination_config);
        ci_ssf_n[nomination_id].attach(e_ci_ssf_n);
        ci_ql_n[nomination_id].attach(e_ci_ql_n);
        clock_selection_config.attach(e_clock_selection);
        clear_wtr[nomination_id].attach(e_clear_wtr);

        nominated_prev = (nomination_config[nomination_id].get()).nominated;
        holdoff_prev = (nomination_config[nomination_id].get()).holdoff_time;
        cur_ci_ssf_n = ci_ssf_n[nomination_id].get();
        cur_ci_ql_n = ci_ql_n[nomination_id].get();
        wtr_time_prev = clock_selection_config.get().wtr_time;
        sf[nomination_id + 1].set(true);
    }

    void execute(notifications::Timer *t) override {
        u8 selection_index = nomination_id + 1;
        T_WG(TRACE_GRP_DEVELOP,"Holdoff_timer[%u] = %s ",nomination_id ,holdoff_timeout? "Timed out" : "running");
        T_WG(TRACE_GRP_DEVELOP,"WTRtimer[%u] = %s ",nomination_id ,wtr_timeout? "Timed out" : "running");
        if (t == &t_holdoff_timer) {
            T_WG(TRACE_GRP_DEVELOP,"received holdoff timeout event");
            sf[selection_index].set(cur_ci_ssf_n);
            ql[selection_index].set(cur_ci_ql_n);
            holdoff_timeout = true;
        } else if (t == &t_wtr_timer) {
            T_WG(TRACE_GRP_DEVELOP,"received wtr timeout event");
            wtr_timeout = true;
            sf[selection_index].set(cur_ci_ssf_n);
            ql[selection_index].set(cur_ci_ql_n);
            wtr_status[nomination_id].set(wtr_timeout? false : true);
            T_WG(TRACE_GRP_DEVELOP,"WTR_STATUS[%u] = %s",nomination_id ,wtr_status[nomination_id].get() ? "true" : "false");
        } else if (t == &t_wtr_helper_timer) {
            T_WG(TRACE_GRP_DEVELOP,"received wtr helper timeout event");
            wtr_helper_timeout = true;
        } else {
            T_E("Unexpected event occured ");
        }
        for (auto i=0 ; i<fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT); i++ ) {
            T_WG(TRACE_GRP_DEVELOP,"QL[%u] = %u , SF[%u]= %s",i,ql[i].get(),i, (sf[i].get()) ? "true" : "false");
        }
    }
    void execute(notifications::Event *e) override {
        bool nomination_update = false;
        bool wtr_time_update = false;
        u8 selection_index = nomination_id + 1;
        vtss_appl_synce_quality_level_t cur_ql = VTSS_APPL_SYNCE_QL_NONE; 
        bool cur_sf = true;

        if (e == &e_nomination_config) {
            VTSS_ASSERT(nomination_id<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
            auto info = nomination_config[nomination_id].get(*e);
            if (holdoff_prev != info.holdoff_time || nominated_prev != info.nominated) {
                /* change in nomination effects holdoff operation */
                T_WG(TRACE_GRP_DEVELOP,"change in nomination configuration of hold_offtime[%u] = %u",nomination_id ,info.holdoff_time);
                nomination_update = true;
            }
        } else if (e == &e_ci_ql_n) {
            cur_ci_ql_n = ci_ql_n[nomination_id].get(*e);
            T_WG(TRACE_GRP_DEVELOP,"change in quality level of source ci_ql_n[%u] = %u",nomination_id ,cur_ci_ql_n);
        } else if (e == &e_ci_ssf_n) {
            cur_ci_ssf_n = ci_ssf_n[nomination_id].get(*e);
            T_WG(TRACE_GRP_DEVELOP,"change in signal failure of  source ci_ssf_n[%u] = %s ",nomination_id ,cur_ci_ssf_n? "TRUE" : "FALSE");
        } else if (e == &e_clock_selection) {
            auto info = clock_selection_config.get(*e);
            if(info.wtr_time != wtr_time_prev) {
                /* change in wtr time effects wtr operation */
                T_WG(TRACE_GRP_DEVELOP,"change in wtr timer = %u is being handled for source %u",info.wtr_time ,nomination_id);
                wtr_time_update = true;
            }
        } else if (e == &e_clear_wtr) {
            auto info = clear_wtr[nomination_id].get();
            T_WG(TRACE_GRP_DEVELOP,"event to %s wtr timer on nomination_config[%u]",info ? "clear" : "set" ,nomination_id);
            clear_wtr_ev = info;
        }

        VTSS_ASSERT(nomination_id<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
        auto n_config = nomination_config[nomination_id].get();
        auto c_config = clock_selection_config.get();
        cur_ql = ql[selection_index].get();
        cur_sf = sf[selection_index].get();

        T_WG(TRACE_GRP_DEVELOP,"Holdoff_timer[%u] = %s ,holdoff_time %u00(milli seconds)",nomination_id ,holdoff_timeout? "Timed out" : "running" ,n_config.holdoff_time);
        T_WG(TRACE_GRP_DEVELOP,"WTRtimer[%u] = %s ,wtr_time %u(minutes)",nomination_id ,wtr_timeout? "Timed out" : "running" ,c_config.wtr_time);
        if (nominated_prev && !n_config.nominated ) {
            if (!holdoff_timeout) {
                /* if de-nominating source which has hold off timer active stop it */
                /* stop holdoff timer */
                T_WG(TRACE_GRP_DEVELOP,"stop holdoff timer, as denomination is attempted on source %u",nomination_id);
                holdoff_timeout = true;
                my_sr->timer_del(t_holdoff_timer);
                ql[selection_index].set(cur_ci_ql_n);
                sf[selection_index].set(cur_ci_ssf_n);
            }
            /* stop wtr timer */
            if (!wtr_timeout) {
                T_WG(TRACE_GRP_DEVELOP,"stop wtr timer, as denomination is attempted on source %u",nomination_id);
                wtr_timeout = true;
                my_sr->timer_del(t_wtr_timer);
                ql[selection_index].set(cur_ci_ql_n);
                sf[selection_index].set(cur_ci_ssf_n);
            }
        } else if (n_config.nominated) {

            /* start wtr helper timer if it is source is previously denominated */
            if (!nominated_prev) {
                T_WG(TRACE_GRP_DEVELOP,"start wtr helper timer on source %u,when source is firstly nominated",nomination_id);
                wtr_helper_timeout = false;
                my_sr->timer_add(t_wtr_helper_timer ,(seconds) 1);
            }
            /* check if holdoff time and wtr is set to default values */
            if(n_config.holdoff_time == 0 && c_config.wtr_time == 0) {
                T_WG(TRACE_GRP_DEVELOP,"no holdoff and wtr timer on source %u",nomination_id);
            }
            if (n_config.holdoff_time != 0) {
                if ( (cur_ci_ssf_n || (cur_ci_ql_n == VTSS_APPL_SYNCE_QL_FAIL) ) && (!cur_sf && (cur_ql != VTSS_APPL_SYNCE_QL_FAIL)) ) {
                    if (holdoff_timeout) {
                        T_WG(TRACE_GRP_DEVELOP,"start holdoff timer, as sf = true on source %u",nomination_id);
                        /* if there is a signal failure */
                        my_sr->timer_add(t_holdoff_timer ,(milliseconds) (n_config.holdoff_time * 100));
                        holdoff_timeout = false;
                    } else if (nomination_update) {
                        T_WG(TRACE_GRP_DEVELOP,"holdoff timer is already running,modification of holdoff time value is ignored");
                    }
                }

                /* check if there is recovery of signal,within holdoff time */
                if (!holdoff_timeout && !cur_ci_ssf_n && cur_ci_ql_n != VTSS_APPL_SYNCE_QL_FAIL) {
                    T_WG(TRACE_GRP_DEVELOP,"stop holdoff timer, as sf = false on source %u",nomination_id);
                    /* stop holdoff timer */
                    holdoff_timeout = true;
                    my_sr->timer_del(t_holdoff_timer);
                }
            } else {
                if (!holdoff_timeout) {
                    T_WG(TRACE_GRP_DEVELOP,"Hold off time is set to zero , stop holdoff timer if it is running\n");
                    holdoff_timeout = true;
                    my_sr->timer_del(t_holdoff_timer);

                }
            }
            cur_ql = ql[selection_index].get();
            cur_sf = sf[selection_index].get();
            if (c_config.wtr_time != 0) {

                if (wtr_timeout) { /* If wtr is running ignore the update in wtr value */
                    if ((cur_sf && !cur_ci_ssf_n ) || (cur_ql == VTSS_APPL_SYNCE_QL_FAIL && cur_ci_ql_n != VTSS_APPL_SYNCE_QL_FAIL  && !cur_ci_ssf_n)){
                        /* check if cur_ci_ssf_n = false and cur_ci_ql_n != FAIL with in 1sec of nomination( i.e. use wtr_helper_timeout = false 
                           then dont start wtr
                         */
                        if (!cur_ci_ssf_n && cur_ci_ql_n != VTSS_APPL_SYNCE_QL_FAIL && !wtr_helper_timeout) {
                            T_WG(TRACE_GRP_DEVELOP,"No need to start wtr if(!cur_ci_ssf_n && cur_ci_ql_n != VTSS_APPL_SYNCE_QL_FAIL) with in 1sec of nomination , source %u",nomination_id);
                        } else {
                            /* if sf = true && ci_ssf_n = false -> start timer*/
                            T_WG(TRACE_GRP_DEVELOP,"start wtr timer, as sf = true && ci_ssf_n = false on source %u",nomination_id);
                            my_sr->timer_add(t_wtr_timer ,(seconds) (c_config.wtr_time * 60));
                            wtr_timeout = false;
                        }
                    }
                } else if (wtr_time_update) {
                    T_WG(TRACE_GRP_DEVELOP,"WTR_timer[%u] is already running new value %u(min) doesn't effect the current timer",nomination_id ,c_config.wtr_time);
                }

                if (!wtr_timeout && (cur_ci_ssf_n || clear_wtr_ev)) {
                    /* if timer is running and there is a signal fail -> stop timer */
                    T_WG(TRACE_GRP_DEVELOP,"stop wtr timer, as sf = true && ci_ssf_n = true on source %u",nomination_id);
                    my_sr->timer_del(t_wtr_timer);
                    wtr_timeout = true;
                    if (clear_wtr_ev) {
                        clear_wtr_ev = false;
                        /* set the subject to false , to receive next clear_wtr event */
                        clear_wtr[nomination_id].set(false);
                        /* re-attach on clear_wtr */
                        clear_wtr[nomination_id].get(e_clear_wtr);
                    }
                }
                /* if clear_wtr = true -> stop timer, update sf */
            } else {
                /* update in wtr_time , set to 0 , check if timer is running*/
                if(!wtr_timeout) {
                    T_WG(TRACE_GRP_DEVELOP,"stop wtr timer, as wtr_timer = %u on source %u",nomination_id ,c_config.wtr_time);
                    my_sr->timer_del(t_wtr_timer);
                    wtr_timeout = true;
                }
                
            }
            if (holdoff_timeout && wtr_timeout) {
                T_WG(TRACE_GRP_DEVELOP,"No hold-off or wtr_timer running on source %u, so QL , SF are updated with ci_ql_n and ci_sf_n",nomination_id);
                ql[selection_index].set(cur_ci_ql_n);
                sf[selection_index].set(cur_ci_ssf_n);
            }
        } else if (!n_config.nominated) {
            T_WG(TRACE_GRP_DEVELOP,"nomination[%u] = false so QL,SF are updated with ci_ql_n,ci_ssf_n values",nomination_id);
            ql[selection_index].set(cur_ci_ql_n);
            sf[selection_index].set(cur_ci_ssf_n);
        }
        if (nomination_update) {
            holdoff_prev = n_config.holdoff_time;
            nominated_prev = n_config.nominated;
        }
        for (auto i=0 ; i<fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT); i++ ) {
            T_WG(TRACE_GRP_DEVELOP,"QL[%u] = %u , SF[%u]= %s",i,ql[i].get(),i, (sf[i].get()) ? "true" : "false");
        }
        wtr_status[nomination_id].set(wtr_timeout? false : true);
        T_WG(TRACE_GRP_DEVELOP,"WTR_STATUS[%u] = %s",nomination_id ,wtr_status[nomination_id].get() ? "true" : "false");

    }
    u8 nomination_id;
    bool nominated_prev;
    bool holdoff_timeout;
    bool cur_ci_ssf_n;
    vtss_appl_synce_quality_level_t cur_ci_ql_n;
    bool clear_wtr_ev;
    bool wtr_helper_timeout;
    uint holdoff_prev;
    uint wtr_time_prev;
    uint wtr_timeout;
    notifications::Event e_nomination_config;
    notifications::Event e_ci_ql_n;
    notifications::Event e_ci_ssf_n;
    notifications::Event e_clock_selection;
    notifications::Event e_clear_wtr;
    notifications::Timer t_holdoff_timer;
    notifications::Timer t_wtr_timer;
    notifications::Timer t_wtr_helper_timer;
};
/*
 * EventHandler: eh_selection_process:
 * replication: 1 pr. node 
 * Input:  Priority[1..N]i ,selection_mode ,source ,eec_option
 * Output: selected_port, selected_ql 
 * Description:
 *         Handle hold off timer and wait to restore feature .
 */
struct eh_selection_process: public notifications::EventHandler {

    eh_selection_process () :
        EventHandler(my_sr) ,e_clock_selection(this)
    {
    }

    void init() {
        T_WG(TRACE_GRP_DEVELOP,"in eh_selection_process ");
        for (auto i=0 ;i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT);i++) {
            T_WG(TRACE_GRP_DEVELOP,"subscribing for change in nomination_config[%u] ",i);
            new (&e_nomination_config[i]) notifications::Event(this);
            VTSS_ASSERT(i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
            nomination_config[i].attach(e_nomination_config[i]);
        }
        for (auto i=0 ;i<fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT);i++) {
            new (&e_ql[i]) notifications::Event(this);
            new (&e_sf[i]) notifications::Event(this);
            T_WG(TRACE_GRP_DEVELOP,"subscribing for change in ql[%u]",i);
            ql[i].attach(e_ql[i]);
            T_WG(TRACE_GRP_DEVELOP,"subscribing for change in sf[%u]",i);
            sf[i].attach(e_sf[i]);
        }
        clock_selection_config.attach(e_clock_selection);
        cur_priority[0] = 0; /* if nominated sources have same quality as internal source, internal source get picked by selection algorithm */
        cur_selection_mode_conf = clock_selection_config.get().selection_mode;

        /* source to be selected in manual selection mode */
        cur_selection_source = clock_selection_config.get().source + 1;
        cur_eec_option = clock_selection_config.get().eec_option; 
        cur_selected_source = fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT);
        for (auto i=0 ;i<fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT);i++) {
            if (i) {
                cur_priority[i] = nomination_config[i-1].get().priority; 
            }
            cur_sf[i] = sf[i].get();
            auto cur_ql_copy = ql[i].get();
            if (cur_eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) {
                if ((cur_ql_copy == VTSS_APPL_SYNCE_QL_NONE) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_PRS) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_STU) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_ST2) ||
                        (cur_ql_copy == VTSS_APPL_SYNCE_QL_TNC) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_ST3E) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_EEC2) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_SMC) ||
                        (cur_ql_copy == VTSS_APPL_SYNCE_QL_PROV) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_DUS)) {
                    cur_ql[i] = cur_ql_copy;
                }
                else {
                    T_WG(TRACE_GRP_DEVELOP,"ql[%u] = %u is not valid according to eec_option choosen %u, so treated as QL_DNU",i ,cur_ql_copy ,cur_eec_option);
                    cur_ql[i] = VTSS_APPL_SYNCE_QL_DUS;
                }
            }
            else {
                if ((cur_ql_copy == VTSS_APPL_SYNCE_QL_NONE) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_PRC) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_SSUA) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_SSUB) ||
                        (cur_ql_copy == VTSS_APPL_SYNCE_QL_EEC1) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_DNU) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_INV)) {
                    cur_ql[i] = cur_ql_copy;
                } else {
                    T_WG(TRACE_GRP_DEVELOP,"ql[%u] = %u is not valid according to eec_option choosen %u, so treated as QL_DNU",i ,cur_ql_copy ,cur_eec_option);
                    cur_ql[i] = VTSS_APPL_SYNCE_QL_DNU;
                }
            }
        }
        selected_port.set(fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT));
#if defined(VTSS_SW_OPTION_PTP)
        best_master.set(fast_cap(VTSS_APPL_CAP_PTP_CLOCK_CNT));
#endif
    }

    void execute(notifications::Event *e) override {
        bool do_selection = false;
        uint updated_src = 0;

        if (e == &e_clock_selection) {
            T_WG(TRACE_GRP_DEVELOP,"eh_selection_process:: received event of change in clock selection");
            auto info = clock_selection_config.get(*e);
            if ((info.selection_mode != cur_selection_mode_conf || (info.source != (cur_selection_source-1))) || (cur_eec_option != info.eec_option) ) {
                T_WG(TRACE_GRP_DEVELOP,"change in clock selection mode from %u to %u ,source = %u ,eec_option %u ",cur_selection_mode_conf ,info.selection_mode ,info.source ,info.eec_option);
                do_selection = true;
                cur_selection_mode_conf = info.selection_mode;
                cur_selection_source = info.source + 1;
                if (cur_eec_option != info.eec_option) {
                    cur_eec_option = info.eec_option;
                    /*update locat cur_ql[] array according to eec_option */
                    for (auto i=0 ;i<fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT);i++) {
                        auto cur_ql_copy = ql[i].get();
                        if (cur_eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) {
                            if ((cur_ql_copy == VTSS_APPL_SYNCE_QL_NONE) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_PRS) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_STU) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_ST2) ||
                                    (cur_ql_copy == VTSS_APPL_SYNCE_QL_TNC) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_ST3E) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_EEC2) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_SMC) ||
                                    (cur_ql_copy == VTSS_APPL_SYNCE_QL_PROV) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_DUS)) {
                                cur_ql[i] = cur_ql_copy;
                            }
                            else {
                                T_WG(TRACE_GRP_DEVELOP,"ql[%u] = %u is not valid according to eec_option choosen %u, so treated as QL_DNU",i ,cur_ql_copy ,cur_eec_option);
                                cur_ql[i] = VTSS_APPL_SYNCE_QL_DUS;
                            }
                        }
                        else {
                            if ((cur_ql_copy == VTSS_APPL_SYNCE_QL_NONE) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_PRC) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_SSUA) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_SSUB) ||
                                    (cur_ql_copy == VTSS_APPL_SYNCE_QL_EEC1) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_DNU) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_INV)) {
                                cur_ql[i] = cur_ql_copy;
                            } else {
                                T_WG(TRACE_GRP_DEVELOP,"ql[%u] = %u is not valid according to eec_option choosen %u, so treated as QL_DNU",i ,cur_ql_copy ,cur_eec_option);
                                cur_ql[i] = VTSS_APPL_SYNCE_QL_DNU;
                            }
                        }

                    }
                }
            }
        } else {
            bool priority_update = false;
            bool nomination_update = false;
            bool ql_update = false;
            bool sf_update = false;
            /* 
               if event is not clock_mode change, then there is three possibilities 
               1. change in priority @ nomination_config 
               2. change in quality level @ ql
               3. change in signal failure @ sf
             */
            /* check if event triggered from nomination_config */
            for (auto i=0 ;i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT);i++) {
                if (e == &e_nomination_config[i]) {
                    nomination_update = true;
                    updated_src = i;
                    /* re-attach for further events */
                    VTSS_ASSERT(i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
                    auto info = nomination_config[i].get(*e);
                    if (info.priority != cur_priority[i+1]) {
                        priority_update = true;
                        cur_priority[i+1] = info.priority;
                    }
                    break;
                }
            }
            if (nomination_update) {
                T_WG(TRACE_GRP_DEVELOP,"eh_selection_process:: update on nomination_config[%u] , priority[%u] = %u %s",updated_src ,updated_src+1 ,cur_priority[updated_src+1] ,priority_update ? "updated" : "unchanged");
            } else {
                /* check if event triggered from QL */
                for (auto i=0 ;i<fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT);i++) {
                    if (e == &e_ql[i]) {
                        ql_update = true;
                        updated_src = i;
                        /* re-attach for further events */
                        auto cur_ql_copy = ql[i].get(*e);
                        /* check if update ql is according to eec option */
                        if (cur_eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) {
                            if ((cur_ql_copy == VTSS_APPL_SYNCE_QL_NONE) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_PRS) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_STU) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_ST2) ||
                                    (cur_ql_copy == VTSS_APPL_SYNCE_QL_TNC) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_ST3E) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_EEC2) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_SMC) ||
                                    (cur_ql_copy == VTSS_APPL_SYNCE_QL_PROV) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_DUS)) {
                                cur_ql[i] = cur_ql_copy;
                            }
                            else {
                                T_WG(TRACE_GRP_DEVELOP,"ql[%u] = %u is not valid according to eec_option choosen %u, so treated as QL_DNU",i ,cur_ql_copy ,cur_eec_option);
                                cur_ql[i] = VTSS_APPL_SYNCE_QL_DUS;
                            }
                        }
                        else {
                            if ((cur_ql_copy == VTSS_APPL_SYNCE_QL_NONE) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_PRC) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_SSUA) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_SSUB) ||
                                    (cur_ql_copy == VTSS_APPL_SYNCE_QL_EEC1) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_DNU) || (cur_ql_copy == VTSS_APPL_SYNCE_QL_INV)) {
                                cur_ql[i] = cur_ql_copy;
                            } else {
                                T_WG(TRACE_GRP_DEVELOP,"ql[%u] = %u is not valid according to eec_option choosen %u, so treated as QL_DNU",i ,cur_ql_copy ,cur_eec_option);
                                cur_ql[i] = VTSS_APPL_SYNCE_QL_DNU;
                            }
                        }

                        break;
                    }
                }
                if (ql_update) {
                    T_WG(TRACE_GRP_DEVELOP,"eh_selection_process:: update on QL[%u] = %u, cur[%u] = %u",updated_src ,ql[updated_src].get() ,updated_src ,cur_ql[updated_src]);
                } else {
                    /* check if event triggered from SF */
                    for (auto i=0 ;i<fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT);i++) {
                        if (e == &e_sf[i]) {
                            sf_update = true;
                            updated_src = i;
                            /* re-attach for further events */
                            cur_sf[i] = sf[i].get(*e);
                            break;
                        }
                    }
                    if (sf_update) {
                        T_WG(TRACE_GRP_DEVELOP,"eh_selection_process:: update on SF[%u] = %s , cur_sf[%u] = %s",updated_src ,sf[updated_src].get() ? "true" : "false" ,updated_src ,cur_sf[updated_src] ? "true" : "false");
                    } else {
                        T_WG(TRACE_GRP_DEVELOP,"eh_selection_process:: received unexpected event");
                    }

                }
            }
            if (priority_update || ql_update || sf_update || nomination_update) {
                do_selection = true;
            }

        }
        if (do_selection) {
            for (auto i=0 ;i<fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT);i++) {
                T_WG(TRACE_GRP_DEVELOP,"eh_selection_process:: ql[%u] = %u , sf[%u] = %s, priority[%u] = %u",i ,cur_ql[i] ,i ,cur_sf[i]?"TRUE":"FALSE" ,i ,cur_priority[i]);
            }
            T_WG(TRACE_GRP_DEVELOP,"selection to be done ");
            switch(cur_selection_mode_conf) {
                case VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN:
                case VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER:
                    T_WG(TRACE_GRP_DEVELOP,"clock selection forced %s",cur_selection_mode_conf == VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN ? "FREERUN" : "HOLDOVER");
                    cur_selected_source = 0;
                    clock_selection_mode_set(cur_selection_mode_conf, cur_selected_source);
                    break;
                case VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL:
                    T_WG(TRACE_GRP_DEVELOP,"clock selection manual");
                    cur_selected_source = cur_selection_source;
                    clock_selection_mode_set(cur_selection_mode_conf, cur_selected_source - 1);
                    break;
                case VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL_TO_SELECTED:
                    // if no source is selected, this command has no effect.
                    T_WG(TRACE_GRP_DEVELOP,"clock selection mode manual to selected");
                    {
                        u8 cur_sel;
                        cur_sel = selected_source.get();
                        if (cur_sel) {
                            clock_selection_mode_set(VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL, cur_sel - 1);
                            cur_selected_source = cur_sel;
                        }
                    }
                    break;
                case VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE:
                    T_WG(TRACE_GRP_DEVELOP,"clock selection mode Automatic non-revertive");
                    {
                        u8 cur_sel;
                        cur_sel = selected_source.get();
                        // if a source is already selected, it will continue to be the 
                        // selected source as long as the source is ok, i.e. the normal
                        // selection process is on hold.
                        if (cur_sel && ql[cur_sel].get() != VTSS_APPL_SYNCE_QL_FAIL && !sf[cur_sel].get()) {
                            cur_selected_source = cur_sel;
                            break;
                        } else {
                            /* go to revertive */
                            T_WG(TRACE_GRP_DEVELOP,"Fall back to revertive as no source was selected ");
                        }

                    }
                case VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE:
                    {
                        vtss_appl_synce_quality_level_t ql_sel = cur_ql[0];
                        uint sel = 0;
                        uint priority_sel = cur_priority[0];
                        bool clk_src_exist = false; // indicates that there is atleast 1 clock source.
                        T_WG(TRACE_GRP_DEVELOP,"clock selection auto-revertive");
                        for (auto i=1 ;i <fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT);i++) {
                            if (cur_ql[i] || !cur_sf[i]) {
                                clk_src_exist = true;
                            }
                            if (!cur_sf[i] && (cur_ql[i] != VTSS_APPL_SYNCE_QL_NONE)) { //sources with no signal failure is only considered for selection
                                T_WG(TRACE_GRP_DEVELOP,"sel:: sf[%u] = false , ql[%u] = %u ql_sel %u , priority[%u] = %u, priority_sel =%u",i ,i ,cur_ql[i] ,ql_sel ,i ,cur_priority[i] ,priority_sel);
                                if ((cur_ql[i] < ql_sel) || ( cur_ql[i] == ql_sel && cur_priority[i] < priority_sel) ) {
                                    sel = i;
                                    ql_sel = cur_ql[i];
                                    priority_sel = cur_priority[i];
                                }
                            }
                        }
                        T_WG(TRACE_GRP_DEVELOP,"selected source is %u, with ql %u and priority %u",sel ,ql_sel ,priority_sel);
                        T_WG(TRACE_GRP_DEVELOP,"clock input changed to selected source is %u",sel);
                        if (sel == 0) {
                            cur_selected_source = sel;
                            /* if Internal clock source is selected */
                            /* If atleast 1 clock source exist, set dpll mode as holdover. Otherwise, set it to free-run state. */
                            if (clk_src_exist) {
                                clock_selection_mode_set(VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER, cur_selected_source);
                            } else {
                                clock_selection_mode_set(VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN, cur_selected_source);
                            }
                            T_WG(TRACE_GRP_DEVELOP,"Using internal clock source to drive ");
                        } else if (sel != cur_selected_source) {
                            cur_selected_source = sel;
                            /* clock source is either PTP clock or ethernet port or a station clock input */
                            T_WG(TRACE_GRP_DEVELOP,"clock_source[%u] is manually selected ",cur_selected_source);
                            clock_selection_mode_set(VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL, cur_selected_source - 1);
                        }
                    }

                    break;
            }
            vtss_appl_synce_selection_mode_t mode;

            if (clock_selection_mode_get(&mode) == VTSS_RC_OK) {
                T_WG(TRACE_GRP_DEVELOP,"hw selected mode is %u ",mode);
            }

            selected_source.set(cur_selected_source);
            /* populate slected quality */
            selected_ql.set(cur_ql[cur_selected_source]);

#if defined(VTSS_SW_OPTION_PTP)
            vtss_ptp_synce_src_t src = {VTSS_PTP_SYNCE_NONE, 0};
#endif
            if (cur_selected_source) {
                /* find selected port */
                VTSS_ASSERT(cur_selected_source-1<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
                auto config_s = nomination_config[cur_selected_source - 1].get();
                u32 port;

                synce_network_port_clk_in_port_combo_to_port(config_s.network_port, config_s.clk_in_port, &port);
                selected_port.set(port);
                T_WG(TRACE_GRP_DEVELOP,"selected port =%u, selected_source %u ",selected_port.get() ,cur_selected_source);
#if defined(VTSS_SW_OPTION_PTP)
                if (port < fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT)) {
                    src.type = VTSS_PTP_SYNCE_ELEC;
                    src.ref = synce_get_selector_ref_no(cur_selected_source-1, port);
                    T_WG(TRACE_GRP_DEVELOP,"Non-PTP port, calc. clock selector ref =%u ",src.ref);
                    best_master.set(fast_cap(VTSS_APPL_CAP_PTP_CLOCK_CNT));
                } else {
                    /* selected port is ptp instance */
                    src.type = VTSS_PTP_SYNCE_PAC;
                    src.ref  = port - fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT);
                    T_WG(TRACE_GRP_DEVELOP,"PTP port, calc. clock selector ref =%u ",src.ref);
                    best_master.set(port - fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT));
                }
                SYNCE_RC(ptp_set_selected_src(&src));
#endif
            } else {
#if defined(VTSS_SW_OPTION_PTP)
                if (clock_selection_status.get().selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_ACQUIRING && (saved_selector_state.get() != VTSS_APPL_SYNCE_SELECTOR_STATE_PTP) ) {
                    best_master.set(fast_cap(VTSS_APPL_CAP_PTP_CLOCK_CNT));
                    T_WG(TRACE_GRP_DEVELOP,"setting best_master to %u as selector state is != PTP ",best_master.get());
                }
                SYNCE_RC(ptp_set_selected_src(&src));
#endif
                selected_port.set(fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT));
                T_WG(TRACE_GRP_DEVELOP,"selected port =%u, selected_source %u ",selected_port.get() ,cur_selected_source);
            }

            T_WG(TRACE_GRP_DEVELOP,"using clock source with nomination %u(0->internal oscillator)  ql %u",cur_selected_source ,cur_ql[cur_selected_source]);
        }

    } /* end execute() */
    CapArray<notifications::Event, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> e_nomination_config;
    CapArray<notifications::Event, VTSS_APPL_CAP_SYNCE_SELECTED_CNT> e_ql;
    CapArray<notifications::Event, VTSS_APPL_CAP_SYNCE_SELECTED_CNT> e_sf;
    notifications::Event e_clock_selection;

    CapArray<vtss_appl_synce_quality_level_t ,VTSS_APPL_CAP_SYNCE_SELECTED_CNT> cur_ql;
    CapArray<bool ,VTSS_APPL_CAP_SYNCE_SELECTED_CNT> cur_sf;
    CapArray<uint, VTSS_APPL_CAP_SYNCE_SELECTED_CNT> cur_priority;
    vtss_appl_synce_selection_mode_t cur_selection_mode_conf;
    vtss_appl_synce_eec_option_t cur_eec_option;
    uint cur_selection_source; /* clock source to be used in manual selection mode */
    uint cur_selected_source;  /* clock source selected in revertive selection mode */

};
 /*
 * EventHandler: eh_internal_source:
 * replication: 1 pr. node 
 * Input:  selector state ,ssm_freerun ,ssm_holdover
 * Output: ql[0],sf[0] and priority[0] 
 * Description:
 *         Handle ql ,sf and priroity of the internal oscillator for the dpll to nominate.
 */
struct eh_internal_source: public notifications::EventHandler {

    eh_internal_source () :
        EventHandler(my_sr) ,e_clock_selection(this) ,e_selector_state(this)
    {
        internal_source_index = 0;
    }

    void init() {
        clock_selection_config.attach(e_clock_selection);
        selector_state.attach(e_selector_state);
        cur_ssm_freerun = clock_selection_config.get().ssm_freerun;
        cur_ssm_holdover = clock_selection_config.get().ssm_holdover;
        cur_selector_state = selector_state.get();
        sf[internal_source_index].set(false);
        /* set default quality level of internal source to QL_DNU */
        ql[internal_source_index].set(VTSS_APPL_SYNCE_QL_DNU);
    }
    void execute(notifications::Event *e) override {
        bool do_update = false;

        if (e == &e_clock_selection) {
            auto info = clock_selection_config.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_internal_source:: received event of change in clock selection, ssm_freerun %u , ssm_holdover %u",info.ssm_freerun ,info.ssm_holdover);
            if (cur_ssm_freerun != info.ssm_freerun || cur_ssm_holdover != info.ssm_holdover) {
                cur_ssm_freerun = info.ssm_freerun;
                cur_ssm_holdover = info.ssm_holdover;
                do_update = true;
            }
        } else if (e == &e_selector_state) {
            auto info = selector_state.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_internal_source:: received event of change in clock selector state = %u",info);
            do_update = true;
            cur_selector_state = info;
        } else {
            T_WG(TRACE_GRP_DEVELOP,"eh_internal_source:: received unexpected event ");
        }
        if (do_update) {
            T_WG(TRACE_GRP_DEVELOP,"eh_internal_source:: cur_ssm_freerun %u ,cur_ssm_holdover %u ",cur_ssm_freerun ,cur_ssm_holdover);
            vtss_appl_synce_quality_level_t cal_ql = (cur_selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN) ? cur_ssm_freerun : cur_ssm_holdover;
            ql[internal_source_index].set(cal_ql == VTSS_APPL_SYNCE_QL_NONE ? VTSS_APPL_SYNCE_QL_DNU : cal_ql);
            /* priority[0] is set in selection process handler */
        }
    }
    notifications::Event e_clock_selection;
    notifications::Event e_selector_state;
    vtss_appl_synce_quality_level_t cur_ssm_freerun;
    vtss_appl_synce_quality_level_t cur_ssm_holdover;
    vtss_appl_synce_selector_state_t cur_selector_state;
    uint internal_source_index;
};

/*
 * EventHandler: eh_tx_ssm:
 * replication: pr. port 
 * Input:  selected_ql, selected port ,ssm_enabled ,eec_option
 * Output: Tx_ssm 
 * Description:
 *         sends a esmc pdu on ports with ssm_enabled = true.
 */
struct eh_tx_ssm: public notifications::EventHandler {

    eh_tx_ssm () :
        EventHandler(my_sr) ,e_clock_selection(this) ,e_selected_port(this) ,e_selected_ql(this) ,e_ssm_enabled(this) ,e_port_state(this) ,t_tx_ssm_timer(this)
    {
        tx_ssm_interval = 1000;
    }

    void init(uint i) {
        cur_port = i;
        ssm_enabled[cur_port].attach(e_ssm_enabled);

        cur_eec_option = clock_selection_config.get().eec_option;
        cur_ssm_holdover = clock_selection_config.get().ssm_holdover;
        cur_ssm_freerun = clock_selection_config.get().ssm_freerun;
        cur_selected_port = selected_port.get();
        cur_selected_ql = selected_ql.get();
        cur_ssm_enabled = ssm_enabled[cur_port].get();
        cur_selector_state = selector_state.get(e_selector_state);
        is_port_up = port_info[cur_port].get(e_port_state).link;
        if (cur_ssm_enabled) {
            my_sr->timer_add(t_tx_ssm_timer, (milliseconds) tx_ssm_interval);
        }
        
        meba_port_cap_t port_cap = 0;
        VTSS_RC_ERR_PRINT(port_cap_get(cur_port, &port_cap));
        if ((port_cap & MEBA_PORT_CAP_10G_FDX) && fast_cap(MESA_CAP_SYNCE_10G_DNU)) {
            is_port_dnu = true;
        } else {
            is_port_dnu = false;
        }
        T_WG(TRACE_GRP_DEVELOP,"is_port_dnu[%u] = %s",cur_port ,is_port_dnu ? "TRUE" : "FALSE");
    }

    void execute(notifications::Timer *t) override {
        if (t == &t_tx_ssm_timer) {
            if (cur_ssm_enabled && is_port_up) {
                bool is_aligned = false;
                T_D("eh_tx_ssm[%u]:: received tx timer timeout, send packet on port %u with eec_option %u , selected_ql %u and selected_port %u",cur_port ,cur_port ,cur_eec_option ,cur_selected_ql ,cur_selected_port);
                if (!is_port_dnu) {
                    u8 tx_ssm_code;
                    cur_selector_state = selector_state.get(e_selector_state);
                    cur_ssm_holdover = clock_selection_config.get(e_selector_state).ssm_holdover;
                    cur_ssm_freerun = clock_selection_config.get(e_selector_state).ssm_freerun;

                    if (cur_selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED) {
                        tx_ssm_code = overwrite_conv((cur_port != cur_selected_port)? cur_selected_ql : VTSS_APPL_SYNCE_QL_DNU);
                    } else if ((cur_selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_HOLDOVER && cur_ssm_holdover) || (cur_selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_ACQUIRING && cur_ssm_holdover)) {
                        tx_ssm_code = overwrite_conv(cur_ssm_holdover);
                    } else if(cur_selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN && cur_ssm_freerun) {
                        tx_ssm_code = overwrite_conv(cur_ssm_freerun);
                    } else {
                        tx_ssm_code = overwrite_conv(VTSS_APPL_SYNCE_QL_DNU);
                    }
                    is_aligned = is_ssm_aligned(cur_eec_option ,tx_ssm_code);
                    ssm_frame_tx(cur_port ,false ,is_aligned ? tx_ssm_code : SSM_QL_DNU_DUS);
                    tx_ssm[cur_port].set(tx_ssm_code);
                } else {
                    ssm_frame_tx(cur_port ,false ,SSM_QL_DNU_DUS);
                    tx_ssm[cur_port].set(SSM_QL_DNU_DUS);
                }

            } else if(cur_ssm_enabled) {
                tx_ssm[cur_port].set(SSM_QL_LINK);
            }
            my_sr->timer_add(t_tx_ssm_timer, (milliseconds) tx_ssm_interval);
        } else {
            T_WG(TRACE_GRP_DEVELOP,"eh_tx_ssm[%u]:: received unexpected timer event on ",cur_port);
        }
    }

    void execute(notifications::Event *e) override {
        if (e == &e_port_state) {
            is_port_up = port_info[cur_port].get(*e).link;
            T_WG(TRACE_GRP_DEVELOP,"eh_tx_ssm[%u]:: received event of link %s",cur_port ,is_port_up ? "Up" : "Down");
        } else if (e == &e_clock_selection) {
            auto info = clock_selection_config.get(*e);
            if (info.eec_option != cur_eec_option) {
                T_WG(TRACE_GRP_DEVELOP,"eh_tx_ssm[%u]:: received event of clock selection mode change, cur_eec_option %u",cur_port ,cur_eec_option);
                cur_eec_option = info.eec_option;
            }
        } else if (e == &e_selected_port) {
            cur_selected_port = selected_port.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_tx_ssm[%u]:: received event of selected_port change, cur_selected_port = %u",cur_port ,cur_selected_port);
        } else if (e == &e_selected_ql) {
            cur_selected_ql = selected_ql.get(*e);
            auto selected_port_copy = selected_port.get();
            T_WG(TRACE_GRP_DEVELOP,"eh_tx_ssm[%u]:: received event of selected_ql change, cur_selected_ql = %u cur_selected_port %u",cur_port ,cur_selected_ql ,cur_selected_port);
            if (cur_ssm_enabled && selected_port_copy == cur_selected_port) {
                if (is_port_up) {
                    T_WG(TRACE_GRP_DEVELOP,"packet transmitted as change in quality to %u on port %u",cur_selected_ql ,cur_port);
                    if (!is_port_dnu) {
                        u8 tx_ssm_code = overwrite_conv((cur_port != cur_selected_port)? cur_selected_ql : VTSS_APPL_SYNCE_QL_DNU);
                        ssm_frame_tx(cur_port, true ,is_ssm_aligned(cur_eec_option ,tx_ssm_code) ? tx_ssm_code : SSM_QL_DNU_DUS);
                        tx_ssm[cur_port].set(tx_ssm_code) ;
                    } else {
                        ssm_frame_tx(cur_port, true ,SSM_QL_DNU_DUS);
                        tx_ssm[cur_port].set(SSM_QL_DNU_DUS);
                    }
                } else {
                    tx_ssm[cur_port].set(SSM_QL_LINK);
                }
            }
        } else if (e == &e_ssm_enabled) {
            cur_ssm_enabled = ssm_enabled[cur_port].get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_tx_ssm[%u]:: received event of ssm_enabled change, cur_ssm_enabled[%u] = %s",cur_port ,cur_port ,cur_ssm_enabled ? "enabled" : "disabled");
            if(cur_ssm_enabled) {
                T_WG(TRACE_GRP_DEVELOP,"eh_tx_ssm:: subscribing for change of events on subjects selected_port,selected_ql and eec_option, port %u",cur_port);
                cur_selected_port = selected_port.get(e_selected_port);
                cur_selected_ql   = selected_ql.get(e_selected_ql);
                cur_eec_option    = clock_selection_config.get(e_clock_selection).eec_option;
                /* start 5sec timer */
                T_WG(TRACE_GRP_DEVELOP,"tx_ssm[%u] timer started",cur_port);
                my_sr->timer_add(t_tx_ssm_timer, (milliseconds) tx_ssm_interval);
                tx_ssm[cur_port].set(is_port_up ? SSM_QL_DNU_DUS :SSM_QL_LINK);
            } else {
                /* stop 5sec timer */
                T_WG(TRACE_GRP_DEVELOP,"eh_tx_ssm:: de-subscribing for change of events on subjects selected_port,selected_ql and eec_option, port %u",cur_port);
                selected_port.detach(e_selected_port);
                selected_ql.detach(e_selected_ql);
                clock_selection_config.detach(e_clock_selection);
                T_WG(TRACE_GRP_DEVELOP,"tx_ssm[%u] timer stopped",cur_port);
                my_sr->timer_del(t_tx_ssm_timer);
                tx_ssm[cur_port].set(SSM_QL_FAIL);
            }
        } else {
            T_WG(TRACE_GRP_DEVELOP,"eh_tx_ssm[%u]:: received unexpected event",cur_port);
        }
    }
    notifications::Event e_clock_selection;
    notifications::Event e_selected_port;
    notifications::Event e_selected_ql;
    notifications::Event e_ssm_enabled;
    notifications::Event e_port_state;
    notifications::Event e_selector_state;
    notifications::Timer t_tx_ssm_timer;

    vtss_appl_synce_selector_state_t cur_selector_state;
    vtss_appl_synce_quality_level_t cur_ssm_holdover;
    vtss_appl_synce_quality_level_t cur_ssm_freerun;
    vtss_appl_synce_eec_option_t cur_eec_option;
    uint cur_selected_port;
    uint cur_port;
    vtss_appl_synce_quality_level_t cur_selected_ql;
    bool cur_ssm_enabled;
    uint tx_ssm_interval;
    bool is_port_dnu;
    bool is_port_up;
};

/*
 * EventHandler: eh_selector_state_monitor:
 * replication: pr. Node 
 * Input:  selector state ,hybrid_mode
 * Output: saved_selector_state 
 * Description:
 *         gives stable selector state.
 */
struct eh_selector_state_monitor: public notifications::EventHandler {

    eh_selector_state_monitor () :
        EventHandler(my_sr) ,e_selector_state(this) ,e_ptp_hybrid_mode(this) ,t_monitor_timer(this)
    {
        /* 10sec*/
        t_monitor_interval = 10000;
    }

    void init() {
        cur_selector_state = selector_state.get(e_selector_state);
        cur_hybrid_mode = ptp_hybrid_mode.get(e_ptp_hybrid_mode);

        t_monitor_timeout = true;
        saved_selector_state.set(VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN);
    }

    void execute(notifications::Timer *t) override {
        if (t == &t_monitor_timer) {
            T_WG(TRACE_GRP_DEVELOP,"t_monitor_timer timeout occured");
            T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_monitor::saved_selector_state = %u",cur_selector_state);
            t_monitor_timeout = true;
            saved_selector_state.set(cur_selector_state);
        }
    }
    void execute(notifications::Event *e) override {
        if (e == &e_selector_state) {
            cur_selector_state = selector_state.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_monitor:: recieved change of selector state event, selector_state %u",cur_selector_state);
        } else if (e == &e_ptp_hybrid_mode) {
            cur_hybrid_mode = ptp_hybrid_mode.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_monitor:: recieved change of hybrid mode event ,hybrid mode %s",cur_hybrid_mode ? "TRUE" : "FALSE");
        } else {
            T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_monitor::received unexpected event");
        }
        T_WG(TRACE_GRP_DEVELOP,"selector_state_monitor:: timer %s",t_monitor_timeout? "timedout" : "running");
        if(t_monitor_timeout) {
            if (cur_hybrid_mode && saved_selector_state.get() == VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED && cur_selector_state != VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED) {
                T_WG(TRACE_GRP_DEVELOP,"started t_monitor_timer ");
                t_monitor_timeout = false;
                my_sr->timer_add(t_monitor_timer, (milliseconds) t_monitor_interval);
            } else {
                T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_monitor::saved_selector_state = %u",cur_selector_state);
                saved_selector_state.set(cur_selector_state);
            }
        } else {
            /* timer is running */
            if (cur_selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED) {
                T_WG(TRACE_GRP_DEVELOP,"stopped t_monitor_timer ");
                my_sr->timer_del(t_monitor_timer);
                t_monitor_timeout = true;
                saved_selector_state.set(cur_selector_state);
                T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_monitor::saved_selector_state = %u",cur_selector_state);
            }
        }
    }
    notifications::Event e_selector_state;
    notifications::Event e_ptp_hybrid_mode;
    notifications::Timer t_monitor_timer;

    bool t_monitor_timeout;
    bool cur_hybrid_mode;
    uint t_monitor_interval;
    vtss_appl_synce_selector_state_t cur_selector_state;
};
 /*
 * EventHandler: eh_selector_state_combined:
 * replication: pr. Node 
 * Input:  selector state ,hybrid_mode
 * Output: saved_selector_state 
 * Description:
 *         gives stable selector state.
 */
struct eh_selector_state_combined: public notifications::EventHandler {

    eh_selector_state_combined () :
        EventHandler(my_sr) ,e_saved_selector_state(this) ,e_lol(this) ,e_losx(this) ,e_dhold(this) ,e_ptsf(this),
        e_selected_source(this) ,e_clock_input(this)
    {
    }

    void init() {
        cur_saved_selector_state = saved_selector_state.get(e_saved_selector_state);
        cur_lol = LOL_dpll.get(e_lol);
        cur_losx = LOSX_dpll.get(e_losx);
        cur_dhold = DHOLD_dpll.get(e_dhold);
        cur_selected_source = selected_source.get(e_selected_source);
        cur_clock_input = clock_input.get(e_clock_input);

        cur_selected_port = fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT);
        cur_ptsf = SYNCE_PTSF_LOSS_OF_ANNOUNCE;
        is_prev_selected_ptp = false;

    }

    void execute(notifications::Event *e) override {
        if (e == &e_saved_selector_state) {
            cur_saved_selector_state = saved_selector_state.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_combined :: recieved change in saved selector state event , = %u",cur_saved_selector_state);
        } else if (e == &e_selected_source) {
            cur_selected_source = selected_source.get(*e);
            if(cur_selected_source) {
                VTSS_ASSERT(cur_selected_source-1<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
                auto config_s = nomination_config[cur_selected_source - 1].get();
                u32 port;
                synce_network_port_clk_in_port_combo_to_port(config_s.network_port, config_s.clk_in_port, &port);
                cur_selected_port = port;
            } else {
                cur_selected_port = fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT);
            }
        } else if (e == &e_lol) {
            cur_lol = LOL_dpll.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_combined :: recieved change in e_lol");
        } else if (e == &e_losx) {
            cur_losx = LOSX_dpll.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_combined :: recieved change in e_losx");
        } else if (e == &e_dhold) {
            cur_dhold = DHOLD_dpll.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_combined :: recieved change in e_dhold");
        } else if (e == &e_ptsf) {
#if defined(VTSS_SW_OPTION_PTP)
            if (is_prev_selected_ptp) {
                u8 ptp_inst = prev_selected_port- fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT);
                cur_ptsf = ptp_clock_ptsf[ptp_inst].get(*e);
                T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_combined :: recieved change in ptsf on slected port =%u, selected source %u ptsf[%u] = %u",prev_selected_port ,cur_selected_source ,ptp_inst ,cur_ptsf);
            }
#endif
        } else if (e == &e_clock_input) {
            cur_clock_input = clock_input.get(*e);
            T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_combined :: recieved change in clockinput %u ",cur_clock_input);
        } else {
            T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_combined :: recieved unexpected ");
        }

        T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_combined :: cur_saved_selector_state %u ,cur_selected_port %u, prev_selected_port %u",cur_saved_selector_state ,cur_selected_port ,prev_selected_port);
        vtss_appl_synce_clock_selection_mode_status_t cur_clock_selection_status;

#if defined(VTSS_SW_OPTION_PTP)
        if (cur_selected_port >= fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT) && cur_selected_port < fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT)) {
            /* selected source is a ptp clock instance */
            u8 ptp_inst_id = cur_selected_port - fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT);
            T_WG(TRACE_GRP_DEVELOP,"Selected source %u, PTP_INST[%u] is selected ",cur_selected_source ,ptp_inst_id);

            /* attach to ptsf state of the ptp instance */
            if (is_prev_selected_ptp) {
                if(cur_selected_port != prev_selected_port) {
                    /* other ptp clock instance is selected now */
                    /* detach ptsf of the earlier ptp port */
                    T_WG(TRACE_GRP_DEVELOP,"un-subscribing for PTSF on ptp instance id %u",prev_selected_port- fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT));
                    ptp_clock_ptsf[ prev_selected_port- fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT) ].detach(e_ptsf);
                    /* attach to ptsf of the current ptp port */
                    T_WG(TRACE_GRP_DEVELOP,"subscribing for PTSF on ptp instance id %u",ptp_inst_id);
                    ptp_clock_ptsf[ptp_inst_id].attach(e_ptsf);
                }
                cur_ptsf = ptp_clock_ptsf[ptp_inst_id].get();
            } else {
                /* attach to ptsf of the current ptp port */
                T_WG(TRACE_GRP_DEVELOP,"subscribing for PTSF on ptp instance id %u",ptp_inst_id);
                ptp_clock_ptsf[ptp_inst_id].attach(e_ptsf);
                cur_ptsf = ptp_clock_ptsf[ptp_inst_id].get();
            }
            if (cur_saved_selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN || cur_saved_selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_HOLDOVER || cur_saved_selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_PTP) {
                T_WG(TRACE_GRP_DEVELOP,"ptsf[%u] = %u",ptp_inst_id ,cur_ptsf);
                if (cur_ptsf == SYNCE_PTSF_NONE) {
                    cur_clock_selection_status.selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_PTP;
                    cur_clock_selection_status.lol = VTSS_APPL_SYNCE_LOL_ALARM_STATE_FALSE;
                } else if (cur_ptsf == SYNCE_PTSF_LOSS_OF_ANNOUNCE) {
                    if (vtss_ptp_servo_get_holdover_status(ptp_inst_id)) {
                        cur_clock_selection_status.selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_HOLDOVER;
                    } else {
                        cur_clock_selection_status.selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN;
                    }
                    cur_clock_selection_status.lol = VTSS_APPL_SYNCE_LOL_ALARM_STATE_TRUE;
                } else {
                    cur_clock_selection_status.selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_ACQUIRING; 
                    cur_clock_selection_status.lol = VTSS_APPL_SYNCE_LOL_ALARM_STATE_TRUE;
                }
            } else {
                cur_clock_selection_status.selector_state = VTSS_APPL_SYNCE_SELECTOR_STATE_PTP;
                cur_clock_selection_status.lol = (cur_ptsf == SYNCE_PTSF_NONE) ? VTSS_APPL_SYNCE_LOL_ALARM_STATE_FALSE : VTSS_APPL_SYNCE_LOL_ALARM_STATE_TRUE ;
            }

            is_prev_selected_ptp = true;
            prev_selected_port = cur_selected_port;
        } else 
#endif            
        {
            T_WG(TRACE_GRP_DEVELOP,"Selected source %u, port[%u] is selected ",cur_selected_source ,cur_selected_port);
#if defined(VTSS_SW_OPTION_PTP)
            /* selected source is non-ptp instance */
            if (is_prev_selected_ptp) {
                /* detach earlier attached ptsf */
                T_WG(TRACE_GRP_DEVELOP,"un-subscribing for PTSF on ptp instance id %u,as ethernet port is selected ",prev_selected_port- fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT));
                ptp_clock_ptsf[ prev_selected_port- fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT) ].detach(e_ptsf);
            }
#endif            
            is_prev_selected_ptp = false;
            cur_clock_selection_status.selector_state = cur_saved_selector_state;
            cur_clock_selection_status.lol = cur_lol ? VTSS_APPL_SYNCE_LOL_ALARM_STATE_TRUE : VTSS_APPL_SYNCE_LOL_ALARM_STATE_FALSE;
#if defined(VTSS_SW_OPTION_PTP)
            // if selector state is PTP, but no PTP source is selected, then report holdover state and undefined lol
            if (cur_clock_selection_status.selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_PTP) {
                cur_clock_selection_status.selector_state =VTSS_APPL_SYNCE_SELECTOR_STATE_HOLDOVER;
                cur_clock_selection_status.lol = VTSS_APPL_SYNCE_LOL_ALARM_STATE_NA ;
            }
#endif
        }
        if (cur_selected_source > 0) {
            cur_clock_selection_status.clock_input = cur_selected_source - 1;
        } else {
            cur_clock_selection_status.clock_input = fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT);
        }
        cur_clock_selection_status.losx = cur_losx;
        cur_clock_selection_status.dhold = cur_dhold;
        clock_selection_status.set(cur_clock_selection_status);
        T_WG(TRACE_GRP_DEVELOP,"eh_selector_state_combined status:: clock_input %u ,selector_state %u ,lol %u ,losx %s ,dhold %s",cur_clock_input ,cur_clock_selection_status.selector_state ,cur_clock_selection_status.lol ,cur_losx? "TRUE":"FALSE" ,cur_dhold? "TRUE" : "FALSE");
    }
    notifications::Event e_saved_selector_state;
    notifications::Event e_lol;
    notifications::Event e_losx;
    notifications::Event e_dhold;
    notifications::Event e_ptsf;
    notifications::Event e_selected_source;
    notifications::Event e_clock_input;

    vtss_appl_synce_selector_state_t cur_saved_selector_state;
    vtss_appl_synce_ptp_ptsf_state_t cur_ptsf;
    bool cur_lol;
    bool cur_losx;
    bool cur_dhold;
    bool is_prev_selected_ptp;
    u8 cur_selected_port;
    u8 cur_clock_input;
    u8 cur_selected_source;
    u8 prev_selected_port;

};

void set_synce_config_to_default() {
    conf_blk_t save_blk;
    synce_set_clock_source_nomination_config_to_default(save_blk.clock_source_nomination_config);
    synce_set_clock_selection_mode_config_to_default(&save_blk.clock_selection_mode_config);
    synce_set_station_clock_config_to_default(&save_blk.station_clock_config);
    synce_set_port_config_to_default(save_blk.port_config.data());

    for (auto i=0; i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT); ++i) {
        VTSS_ASSERT(i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
        nomination_config[i].set(save_blk.clock_source_nomination_config[i]);
        T_D("setting nomination_config[%u] to defaults",i);
    }

    T_D("setting clock_selection_config to defaults");
    save_blk.clock_selection_mode_config.source = 0; /* as we are directly manipulating data sturcture */
    clock_selection_config.set(save_blk.clock_selection_mode_config);

    T_D("setting station_clock_in,station_clock_out to defaults");
    station_clock_in.set(save_blk.station_clock_config.station_clk_in);
    station_clock_out.set(save_blk.station_clock_config.station_clk_out);

    T_D("setting  ssm_enabled[%u-%u] to defaults",0,fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)-1);
    for (auto i=0; i<fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        ssm_enabled[i].set(save_blk.port_config[i].ssm_enabled);
    }
}

/*
 * Static array with eventhandler instances
 */
static CapArray<eh_1000base_t_master_slave, MEBA_CAP_BOARD_PORT_MAP_COUNT> base_t_master_slaves;
static CapArray<eh_port_monitor, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_monitors;
static CapArray<eh_rx_ssm_processing, MEBA_CAP_BOARD_PORT_MAP_COUNT> rx_ssm_processors;
static eh_station_clock_conf station_clock_handler;
static CapArray<eh_nomination_handler, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> nomination_handler;
static CapArray<eh_ho_wtr_processor, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> ho_wtr_processor;
static CapArray<eh_source_monitor, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> source_monitor;
static eh_selection_process selection_process;
struct eh_internal_source internal_source;
static eh_dpll_status_processor dpll_status_update;
static CapArray<eh_tx_ssm, MEBA_CAP_BOARD_PORT_MAP_COUNT> tx_ssm_processor;
#if defined(VTSS_SW_OPTION_PTP)
static CapArray<eh_ptp_config_handler, VTSS_APPL_CAP_PTP_CLOCK_CNT> ptp_config_handler;
static eh_ptp_transient_processing ptp_transient_processor;
static CapArray<eh_ptp_monitor, VTSS_APPL_CAP_PTP_CLOCK_CNT> ptp_monitor;
#endif
struct eh_selector_state_monitor selector_state_monitor;
struct eh_selector_state_combined selector_state_combined;
struct eh_node_monitor node_monitor;
};

static vtss_appl_synce_quality_level_t synce_parse_ql(char *ql)
{
    if (!strcmp(ql,"QL_NONE"))      return VTSS_APPL_SYNCE_QL_NONE;
    else if (!strcmp(ql,"QL_PRC"))  return VTSS_APPL_SYNCE_QL_PRC;
    else if (!strcmp(ql,"QL_SSUA")) return VTSS_APPL_SYNCE_QL_SSUA;
    else if (!strcmp(ql,"QL_SSUB")) return VTSS_APPL_SYNCE_QL_SSUB;
    else if (!strcmp(ql,"QL_DNU"))  return VTSS_APPL_SYNCE_QL_DNU;
    else if (!strcmp(ql,"QL_EEC2")) return VTSS_APPL_SYNCE_QL_EEC2;
    else if (!strcmp(ql,"QL_EEC1")) return VTSS_APPL_SYNCE_QL_EEC1;
    else if (!strcmp(ql,"QL_INV"))  return VTSS_APPL_SYNCE_QL_INV;
    else if (!strcmp(ql,"QL_FAIL")) return VTSS_APPL_SYNCE_QL_FAIL;
    else if (!strcmp(ql,"QL_LINK")) return VTSS_APPL_SYNCE_QL_LINK;
    else if (!strcmp(ql,"QL_PRS"))  return VTSS_APPL_SYNCE_QL_PRS;
    else if (!strcmp(ql,"QL_STU"))  return VTSS_APPL_SYNCE_QL_STU;
    else if (!strcmp(ql,"QL_ST2"))  return VTSS_APPL_SYNCE_QL_ST2;
    else if (!strcmp(ql,"QL_TNC"))  return VTSS_APPL_SYNCE_QL_TNC;
    else if (!strcmp(ql,"QL_ST3E")) return VTSS_APPL_SYNCE_QL_ST3E;
    else if (!strcmp(ql,"QL_SMC"))  return VTSS_APPL_SYNCE_QL_SMC;
    else if (!strcmp(ql,"QL_PROV")) return VTSS_APPL_SYNCE_QL_PROV;
    else if (!strcmp(ql,"QL_DUS"))  return VTSS_APPL_SYNCE_QL_DUS;
    else                            return VTSS_APPL_SYNCE_QL_NONE;
}

mesa_rc synce_subject_debug_write(char *subject_name, u32 index, char *value)
{
    T_D("name length %d subject = %s index %d, value %s",strlen(subject_name), subject_name, index, value);
    if (0 == strcmp(subject_name, "ql")) {
        if (index < fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT)) {
            vtss::ql[index].set((synce_parse_ql(value)));
        } else {
            return VTSS_RC_ERROR;
        }
    } else if (0 == strcmp(subject_name, "sf")) {
        if (index < fast_cap(VTSS_APPL_CAP_SYNCE_SELECTED_CNT)) {
            vtss::sf[index].set((0 == strcmp(value, "true")) ? true : false);
        } else {
            return VTSS_RC_ERROR;
        }
    }
    return VTSS_RC_OK;
}

/****************************************************************************/
/*  Various local functions                                                                                                          */
/****************************************************************************/

mesa_rc synce_network_port_clk_in_port_combo_to_port(vtss_ifindex_t network_port, u8 clk_in_port, u32 *v)
{
    if (network_port != 0) {
        vtss_ifindex_elm_t e;
        VTSS_RC(vtss_ifindex_decompose(network_port, &e));
        if (e.iftype != VTSS_IFINDEX_TYPE_PORT) {
            T_D("Interface %u is not a port interface", VTSS_IFINDEX_PRINTF_ARG(network_port));
            return VTSS_RC_ERROR;
        }
        *v = e.ordinal;
    }
    else if (clk_in_port == 0) {
        *v = SYNCE_STATION_CLOCK_PORT;
    }
#if defined(VTSS_SW_OPTION_PTP)
    else {
        *v = SYNCE_STATION_CLOCK_PORT + 1 + (clk_in_port - 128);
    }
#else
    else {
        *v = SYNCE_STATION_CLOCK_PORT;
        T_E("clk_in_port was != 0 but support for PTP ports in SyncE is not included.");
    }
#endif

    return VTSS_RC_OK;
}

static void system_reset(mesa_restart_t restart)
{
    if (clock_selection_mode_set(VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER, 0) != VTSS_RC_OK)    T_D("error returned");
}

static void synce_set_clock_source_nomination_config_to_default(vtss_appl_synce_clock_source_nomination_config_t *config)
{
    uint   i;
    vtss_ifindex_t ifindex;

    // Convert uport index 1 (with usid = 1) to vtss_ifindex_t type
    (void) vtss_ifindex_from_usid_uport(1, 1, &ifindex);

    for (i=0; i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT); ++i) {
        config[i].nominated = false;
        if ((i < fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT)-1) || !clock_zarlink()) {
            config[i].network_port = ifindex;  // vtss_ifindex_t value of port 0
        }
        else {
            config[i].network_port = VTSS_IFINDEX_NONE;        // SYNCE_STATION_CLOCK_PORT;
        }
        config[i].clk_in_port = 0;          // At present, clk_in_port is always 0 (or >= 128 for PTP sources) as only one station clock input is supported.
        config[i].priority = 0;
        config[i].aneg_mode = VTSS_APPL_SYNCE_ANEG_NONE;
        config[i].holdoff_time = 0;
        config[i].ssm_overwrite = VTSS_APPL_SYNCE_QL_NONE;
    }
}

void synce_mgmt_set_clock_source_nomination_config_to_default(vtss_appl_synce_clock_source_nomination_config_t *config)
{
    synce_set_clock_source_nomination_config_to_default(config);
}
void synce_get_clock_selection_mode_config_default(vtss_appl_synce_clock_selection_mode_config_t *config)
{
    config->selection_mode = VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE;
    config->source = 1;
    config->wtr_time = 5;  /* Default 5 min.*/
    config->ssm_holdover = VTSS_APPL_SYNCE_QL_NONE;
    config->ssm_freerun = VTSS_APPL_SYNCE_QL_NONE;
    config->eec_option = VTSS_APPL_SYNCE_EEC_OPTION_1;
    clock_eec_option_set(CLOCK_EEC_OPTION_1);
}

void synce_set_clock_selection_mode_config_to_default(vtss_appl_synce_clock_selection_mode_config_t *config)
{
    synce_get_clock_selection_mode_config_default(config);
}

void synce_set_station_clock_config_to_default(vtss_appl_synce_station_clock_config_t *config)
{
    config->station_clk_in  = VTSS_APPL_SYNCE_STATION_CLK_DIS;
    config->station_clk_out = VTSS_APPL_SYNCE_STATION_CLK_DIS;
}

void synce_mgmt_set_station_clock_config_to_default(vtss_appl_synce_station_clock_config_t *config)
{
    synce_set_station_clock_config_to_default(config);
}

static void synce_set_port_config_to_default(vtss_appl_synce_port_config_t *config)
{
    uint   i;

    for (i=0; i<SYNCE_PORT_COUNT; i++) {
        config[i].ssm_enabled = false;
    }
}

void synce_mgmt_set_port_config_to_default(vtss_appl_synce_port_config_t *config)
{
    synce_set_port_config_to_default(config);
}

static void apply_configuration(conf_blk_t *blk)
{
    uint  i;
    vtss_appl_synce_clock_selection_mode_config_t clock_selection_mode_config;

    for (i = 0; i < fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT); ++i) {
        if (vtss_appl_synce_clock_source_nomination_config_set((i + 1), &blk->clock_source_nomination_config[i]) != SYNCE_RC_OK)
            T_D("vtss_appl_synce_clock_source_nomination_config_set[%u] error returned",i);
    }

    clock_selection_mode_config = blk->clock_selection_mode_config;
    if (vtss_appl_synce_clock_selection_mode_config_set(&clock_selection_mode_config) != SYNCE_RC_OK)
        T_D("vtss_appl_synce_clock_selection_mode_config_set:: error returned");

    if (synce_mgmt_station_clock_out_set(blk->station_clock_config.station_clk_out) != SYNCE_RC_OK) {
        T_D("synce_mgmt_station_clock_out_set:: error returned");
    }
    if (synce_mgmt_station_clock_in_set(blk->station_clock_config.station_clk_in) != SYNCE_RC_OK) {
        T_D("synce_mgmt_station_clock_in_set:: error returned");
    }

    for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); ++i) {
        if (synce_mgmt_ssm_set(i + VTSS_PORT_NO_START, blk->port_config[i].ssm_enabled) != SYNCE_RC_OK) {
            T_D("synce_mgmt_ssm_set ::error returned");
        }
    }
}

static uint overwrite_conv(vtss_appl_synce_quality_level_t overwrite)
{
    switch (overwrite)
    {
        case VTSS_APPL_SYNCE_QL_NONE: return SSM_QL_PROV;   //This is not standard
        case VTSS_APPL_SYNCE_QL_PRC:  return SSM_QL_PRC;
        case VTSS_APPL_SYNCE_QL_SSUA: return SSM_QL_SSUA_TNC;
        case VTSS_APPL_SYNCE_QL_SSUB: return SSM_QL_SSUB;
        case VTSS_APPL_SYNCE_QL_EEC2: return SSM_QL_EEC2;
        case VTSS_APPL_SYNCE_QL_EEC1: return SSM_QL_EEC1;
        case VTSS_APPL_SYNCE_QL_DNU:  return SSM_QL_DNU_DUS;
        case VTSS_APPL_SYNCE_QL_INV:  return SSM_QL_PRS_INV;    // for test only
        case VTSS_APPL_SYNCE_QL_PRS:  return SSM_QL_PRS_INV;
        case VTSS_APPL_SYNCE_QL_STU:  return SSM_QL_STU;
        case VTSS_APPL_SYNCE_QL_ST2:  return SSM_QL_ST2;
        case VTSS_APPL_SYNCE_QL_TNC:  return SSM_QL_SSUA_TNC;
        case VTSS_APPL_SYNCE_QL_ST3E: return SSM_QL_ST3E;
        case VTSS_APPL_SYNCE_QL_SMC:  return SSM_QL_SMC;
        case VTSS_APPL_SYNCE_QL_PROV: return SSM_QL_PROV;
        case VTSS_APPL_SYNCE_QL_DUS:  return SSM_QL_DNU_DUS;
        case VTSS_APPL_SYNCE_QL_LINK: return SSM_QL_LINK;
        default:  return (SSM_QL_DNU_DUS);
    }
}

vtss_appl_synce_quality_level_t ssm_to_ql(uint ssm ,vtss_appl_synce_eec_option_t eec_option)
{
    switch (ssm)
    {
        case SSM_QL_PRS_INV:  if(eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) return VTSS_APPL_SYNCE_QL_PRS; else return (VTSS_APPL_SYNCE_QL_INV);
        case SSM_QL_PRC:      return (VTSS_APPL_SYNCE_QL_PRC);
        case SSM_QL_SSUA_TNC: if(eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) return (VTSS_APPL_SYNCE_QL_TNC); else return (VTSS_APPL_SYNCE_QL_SSUA);
        case SSM_QL_SSUB:     return (VTSS_APPL_SYNCE_QL_SSUB);
        case SSM_QL_EEC1:     return (VTSS_APPL_SYNCE_QL_EEC1);
        case SSM_QL_PROV:     return (VTSS_APPL_SYNCE_QL_PROV);
        case SSM_QL_DNU_DUS:  if(eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) return (VTSS_APPL_SYNCE_QL_DUS); else return (VTSS_APPL_SYNCE_QL_DNU);
        case SSM_QL_FAIL:     return (VTSS_APPL_SYNCE_QL_FAIL);
        case SSM_QL_STU:      if(eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) return (VTSS_APPL_SYNCE_QL_STU); else return (VTSS_APPL_SYNCE_QL_INV);                             
        case SSM_QL_ST2:      if(eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) return (VTSS_APPL_SYNCE_QL_ST2); else return (VTSS_APPL_SYNCE_QL_INV);                             
        case SSM_QL_ST3:      if(eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) return (VTSS_APPL_SYNCE_QL_EEC2); else return (VTSS_APPL_SYNCE_QL_INV);                             
        case SSM_QL_SMC:      if(eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) return (VTSS_APPL_SYNCE_QL_SMC); else return (VTSS_APPL_SYNCE_QL_INV);                             
        case SSM_QL_ST3E:     if(eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) return (VTSS_APPL_SYNCE_QL_ST3E); else return (VTSS_APPL_SYNCE_QL_INV);                            
        case SSM_QL_LINK:     return (VTSS_APPL_SYNCE_QL_LINK);
        default:              return (VTSS_APPL_SYNCE_QL_FAIL);
    }
}


#ifdef VTSS_SW_OPTION_PACKET
static void ssm_frame_tx(uint port, bool event_flag, uint ssm)
{
    static u8    reserved[3] = {0x00,0x00,0x00};
    static u8    tlv[3] = {0x01,0x00,0x04};
    u8           version = 0x10;
    u32          len = 64;
    u8           *buffer;
    conf_board_t conf;


    if (conf_mgmt_board_get(&conf) < 0)
        return;

    if ((buffer = packet_tx_alloc(len))) {
        packet_tx_props_t tx_props;

        T_D("port %d ssm %x", port, ssm);
        memset(buffer, 0xff, len);
        if (event_flag) version |= 0x08;
        ssm &= 0x0F;

        memcpy(buffer, ssm_dmac, 6);
        memcpy(buffer+6, conf.mac_address.addr, 6);
        memcpy(buffer+6+6, ssm_ethertype, 2);
        memcpy(buffer+6+6+2, ssm_standard, 6);
        memcpy(buffer+6+6+2+6, &version, 1);
        memcpy(buffer+6+6+2+6+1, reserved, 3);
        memcpy(buffer+6+6+2+6+1+3, tlv, 3);
        memcpy(buffer+6+6+2+6+1+3+3, &ssm, 1);
        packet_tx_props_init(&tx_props);
        tx_props.packet_info.modid     = VTSS_MODULE_ID_SYNCE;
        tx_props.packet_info.frm       = buffer;
        tx_props.packet_info.len       = len;
        tx_props.tx_info.dst_port_mask = VTSS_BIT64(port);

        T_DG(TRACE_GRP_PDU_TX, "port %u  length %u  dmac %X-%X-%X-%X-%X-%X  smac %X-%X-%X-%X-%X-%X  Ethertype %X-%X", port, len, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9], buffer[10], buffer[11], buffer[12], buffer[13]);
        T_DG(TRACE_GRP_PDU_TX, "frame[14-20] %X-%X-%X-%X-%X-%X-%X  ssm %X-%X-%X-%X", buffer[14], buffer[15], buffer[16], buffer[17], buffer[18], buffer[19], buffer[20], buffer[24], buffer[25], buffer[26], buffer[27]);
        if (packet_tx(&tx_props) != VTSS_RC_OK) {
            T_D("Packet tx failed");
        }
    }
}



static BOOL ssm_frame_rx(void *contxt, const unsigned char *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    uint port;

    T_DG(TRACE_GRP_PDU_RX, "port %u  length %u  dmac %X-%X-%X-%X-%X-%X  smac %X-%X-%X-%X-%X-%X  ethertype %X-%X", rx_info->port_no, rx_info->length, frm[0], frm[1], frm[2], frm[3], frm[4], frm[5], frm[6], frm[7], frm[8], frm[9], frm[10], frm[11], frm[12], frm[13]);
    T_DG(TRACE_GRP_PDU_RX, "frame[14-20] %X-%X-%X-%X-%X-%X-%X  ssm %X-%X-%X-%X", frm[14], frm[15], frm[16], frm[17], frm[18], frm[19], frm[20], frm[24], frm[25], frm[26], frm[27]);
    if ((rx_info->length < 28) || (memcmp(&frm[14], ssm_standard, 6) == 0))
    {
    /* this is a 'true' ESMC (SSM) slow protocal PDU */
        port = rx_info->port_no;
        if (port < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT))
        {
            vtss::rx_ssm[port].set(frm[27] & 0x0F, true);
        }
        return TRUE;
    }
    return FALSE;
}
#endif

void set_eec_option(vtss_appl_synce_eec_option_t eec_option)
{
    (void)clock_eec_option_set(eec_option == VTSS_APPL_SYNCE_EEC_OPTION_1 ? CLOCK_EEC_OPTION_1 : CLOCK_EEC_OPTION_2);
}

static void func_thread(vtss_addrword_t data)
{
    conf_blk_t                       save_blk;

    // This will wait until the PHYs are initialized.
    T_D("waiting for port PHY ready");
    port_phy_wait_until_ready();
    T_D("port PHY ready");

    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_SYNCE);

    // FIXME: This exception need to be handled in the board API somehow.
    //
    //    if (vtss::synce::dpll::pcb104_synce) {
    //        /* Enable Station clock input for PCB104 */
    //        synce_set_source_port(0, SYNCE_STATION_CLOCK_PORT, 1);
    //        synce_set_source_port(1, SYNCE_STATION_CLOCK_PORT, 1);
    //    }

    T_D("packet rx filter register");
    sleep(20);

#ifdef VTSS_SW_OPTION_PACKET

    /* hook up on SSM frame */
    packet_rx_filter_t filter;
    void *filter_id;
    mesa_rc rc;
    packet_rx_filter_init(&filter);
    filter.modid = VTSS_MODULE_ID_SYNCE;
    filter.match = PACKET_RX_FILTER_MATCH_DMAC | PACKET_RX_FILTER_MATCH_ETYPE;
    filter.cb    = ssm_frame_rx;
    filter.prio  = PACKET_RX_FILTER_PRIO_NORMAL;
    filter.etype = 0x8809; // slow protocol ethertype
    memcpy(filter.dmac, ssm_dmac, sizeof(filter.dmac));

    if ((rc = packet_rx_filter_register(&filter, &filter_id)) != VTSS_RC_OK) {
        T_E("error returned rc %u", rc);
    } else {
        T_D("packet rx filter register :: success");
    }
#endif
}

/****************************************************************************/
/*  MISC. Help functions                                                    */
/****************************************************************************/

const char* ssm_string(vtss_appl_synce_quality_level_t ssm)
{
    switch(ssm)
    {
        case VTSS_APPL_SYNCE_QL_NONE: return("QL_NONE");
        case VTSS_APPL_SYNCE_QL_PRC:  return("QL_PRC");
        case VTSS_APPL_SYNCE_QL_SSUA: return("QL_SSUA");
        case VTSS_APPL_SYNCE_QL_SSUB: return("QL_SSUB");
        case VTSS_APPL_SYNCE_QL_DNU:  return("QL_DNU");
        case VTSS_APPL_SYNCE_QL_EEC2: return("QL_EEC2");
        case VTSS_APPL_SYNCE_QL_EEC1: return("QL_EEC1");
        case VTSS_APPL_SYNCE_QL_INV:  return("QL_INV");
        case VTSS_APPL_SYNCE_QL_FAIL: return("QL_FAIL");
        case VTSS_APPL_SYNCE_QL_LINK: return("QL_LINK");
        case VTSS_APPL_SYNCE_QL_PRS:  return("QL_PRS");
        case VTSS_APPL_SYNCE_QL_STU:  return("QL_STU");
        case VTSS_APPL_SYNCE_QL_ST2:  return("QL_ST2");
        case VTSS_APPL_SYNCE_QL_TNC:  return("QL_TNC");
        case VTSS_APPL_SYNCE_QL_ST3E: return("QL_ST3E");
        case VTSS_APPL_SYNCE_QL_SMC:  return("QL_SMC");
        case VTSS_APPL_SYNCE_QL_PROV: return("QL_PROV");
        case VTSS_APPL_SYNCE_QL_DUS:  return("QL_DUS");

        default:                      return("QL_NONE");
    }
}

// Convert error code to text
// In : rc - error return code
const char *synce_error_txt(mesa_rc rc)
{
    switch (rc) {
        case SYNCE_RC_OK:                return("SYNCE_RC_OK");
        case SYNCE_RC_INVALID_PARAMETER: return("Invalid parameter error returned from SYNCE\n");
        case SYNCE_RC_NOM_PORT:          return("Port nominated to a clock source is already nominated\n");
        case SYNCE_RC_SELECTION:         return("NOT possible to make Manuel To Selected if not in locked mode\n");
        case SYNCE_RC_INVALID_PORT:      return("The selected port is not valid\n");
        case SYNCE_RC_NOT_SUPPORTED:     return("The selected feature is not available in current HW config.\n");
        default:                         return("Unknown error returned from SYNCE\n");
    }
}

// Function for converting from user clock source (starting from 1) to clock source index (starting from 0)
// In - uclk - user clock source
// Return - Clock source index
u8 synce_uclk2iclk(u8 uclk) {
    return uclk - 1;
}

// Function for converting from clock source index (starting from 0) to user clock source (starting from 1)
// In - iclk - Index clock source
// Return - Clock source index
u8 synce_iclk2uclk(u8 iclk) {
    return iclk + 1;
}

meba_synce_clock_frequency_t station_clock_2clockFreq(vtss_appl_synce_frequency_t clk_in_f)
{
    switch (clk_in_f) {
        case VTSS_APPL_SYNCE_STATION_CLK_DIS: return MEBA_SYNCE_CLOCK_FREQ_INVALID;
        case VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ: return MEBA_SYNCE_CLOCK_FREQ_1544_KHZ;
        case VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ: return MEBA_SYNCE_CLOCK_FREQ_2048_KHZ;
        case VTSS_APPL_SYNCE_STATION_CLK_10_MHZ: return MEBA_SYNCE_CLOCK_FREQ_10MHZ;;
        default: return MEBA_SYNCE_CLOCK_FREQ_INVALID;
    }
}

static void update_clk_in_selector(uint source)
{
    meba_synce_clock_frequency_t    frq;
    uint reference;
    VTSS_ASSERT(source<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
    vtss_appl_synce_clock_source_nomination_config_t clock_source_nomination_config = vtss::nomination_config[source].get();
    if (clock_source_nomination_config.nominated) {
        u32 port;
        (void) synce_network_port_clk_in_port_combo_to_port(clock_source_nomination_config.network_port, clock_source_nomination_config.clk_in_port, &port);

        reference = synce_get_selector_ref_no(source, port);
        frq = (clock_source_nomination_config.network_port == 0) ?    // data_port == 0 means that station clock input is used.
              station_clock_2clockFreq(vtss::station_clock_in.get()) : synce_get_rcvrd_clock_frequency(source, port, vtss::port_info[port].get().speed);
    } else {
        frq = MEBA_SYNCE_CLOCK_FREQ_INVALID;
        reference = CLOCK_REF_INVALID;
    }
    T_D("Set freq, source %d, freq %d", source,frq);
    if (clock_frequency_set(source, frq) != VTSS_RC_OK)    T_D("error returned");
    if (dpll_type == VTSS_ZL_30380_DPLL_ZLS3077X && reference == (uint)-1) {
        reference = ZL3077X_PTP_REF_ID;
    }
    if (dpll_type == VTSS_ZL_30380_DPLL_ZLS3073X && reference == (uint)-1) {
        reference = ZL3073X_PTP_REF_ID;
    }
    if (clock_selector_map_set(reference, source) != VTSS_RC_OK) T_D("error returned");
}



/****************************************************************************/
/*  SyncE interface                                                         */
/****************************************************************************/
mesa_rc vtss_appl_synce_clock_source_nomination_config_set(const uint sourceId, const vtss_appl_synce_clock_source_nomination_config_t *const config)
{
    uint  i ,nomination_id = sourceId - 1;

    SYNCE_CRIT_ENTER();
    VTSS_ASSERT(nomination_id<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
    (void)vtss::nomination_config[nomination_id].get();
    vtss_appl_synce_eec_option_t cur_eec_option = vtss::clock_selection_config.get().eec_option; 
    CapArray<vtss_appl_synce_clock_source_nomination_config_t, VTSS_APPL_CAP_SYNCE_NOMINATED_CNT> clock_source_nomination_config;
    u32 port;


    (void) synce_network_port_clk_in_port_combo_to_port(config->network_port, config->clk_in_port, &port);
    T_W("source = %d, port_no  = %d, aneg_mode = %d, ssm_overwrite %d, enable:%d, synce_get_source_port(source, port_no):%d",
            sourceId, port, config->aneg_mode, config->ssm_overwrite, config->nominated, synce_get_source_port(nomination_id, port));

    if ((sourceId < 1) || (sourceId > fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT))) {
        T_I("Invalid configuration attempted on nomination[%u]",nomination_id);
        SYNCE_CRIT_EXIT();
        return (SYNCE_RC_INVALID_PARAMETER);
    }

#if defined(VTSS_SW_OPTION_PTP)
    if (config->network_port == 0) {
        if ((config->clk_in_port != 0) && ((config->clk_in_port < 128) || (config->clk_in_port >= 128 + fast_cap(VTSS_APPL_CAP_PTP_CLOCK_CNT)))) {
            T_I("Invalid configuration attempted on nomination[%u]",nomination_id);
            SYNCE_CRIT_EXIT();
            return SYNCE_RC_INVALID_PARAMETER;
        }
    }
    else {
        if (config->clk_in_port != 0) {
            T_I("Invalid configuration attempted on nomination[%u]",nomination_id);
            SYNCE_CRIT_EXIT();
            return SYNCE_RC_INVALID_PARAMETER;
        }
        if (port >= SYNCE_PORT_COUNT - 1) {
            T_I("Invalid configuration attempted on nomination[%u]",nomination_id);
            SYNCE_CRIT_EXIT();
            return (SYNCE_RC_INVALID_PARAMETER);
        }
    }
#else
    if (config->clk_in_port != 0) {
        T_I("Invalid configuration attempted on nomination[%u]",nomination_id);
        SYNCE_CRIT_EXIT();
        return SYNCE_RC_INVALID_PARAMETER;
    }
#endif

    if (config->nominated && !synce_get_source_port(nomination_id, port)) {
        T_E("Invalid configuration attempted on nomination[%u]",nomination_id);
        SYNCE_CRIT_EXIT();
        return (SYNCE_RC_INVALID_PORT);
    }
    if (cur_eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) {
        if ((config->ssm_overwrite != VTSS_APPL_SYNCE_QL_NONE) && (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_PRS) && (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_STU) &&
                (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_ST2) && (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_TNC) && (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_ST3E) &&
                (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_EEC2) && (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_SMC) && (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_PROV) &&
                (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_DUS))
        {
            T_I("Invalid configuration attempted on nomination[%u]",nomination_id);
            SYNCE_CRIT_EXIT();
            return SYNCE_RC_INVALID_PARAMETER;
        }
    }
    else {
        if ((config->ssm_overwrite != VTSS_APPL_SYNCE_QL_NONE) && (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_PRC) && (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_SSUA) &&
                (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_SSUB) && (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_EEC1) && (config->ssm_overwrite != VTSS_APPL_SYNCE_QL_DNU))
        {
            T_I("Invalid configuration attempted on nomination[%u]",nomination_id);
            SYNCE_CRIT_EXIT();
            return SYNCE_RC_INVALID_PARAMETER;
        }
    }
    if (config->holdoff_time && (config->holdoff_time != SYNCE_HOLDOFF_TEST) && ((config->holdoff_time < SYNCE_HOLDOFF_MIN) || (config->holdoff_time > SYNCE_HOLDOFF_MAX))) {
        T_I("Invalid configuration attempted on nomination[%u]",nomination_id);
        SYNCE_CRIT_EXIT();
        return SYNCE_RC_INVALID_PARAMETER;
    }

    for (i = 0; i < fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT) ; i++ ) {
        VTSS_ASSERT(i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
        clock_source_nomination_config[i] = vtss::nomination_config[i].get();
    }
    /* check if other clock sources is nominated to the same port    */
    for (i = 0; i < fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT) ; i++ ) {
        if (clock_source_nomination_config[i].nominated && config->nominated &&
                (clock_source_nomination_config[i].network_port == config->network_port) &&
#if defined(VTSS_SW_OPTION_PTP)
                (clock_source_nomination_config[i].clk_in_port == config->clk_in_port) &&
#endif
                (i != (nomination_id)))
        {
            T_I("Invalid configuration attempted on nomination[%u], clock input already part of nomination_config[%u]",nomination_id ,i);
            SYNCE_CRIT_EXIT();
            return SYNCE_RC_NOM_PORT;
        }
    }

    /* save this normination in config block */
    VTSS_ASSERT(nomination_id<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
    vtss::nomination_config[nomination_id].set(*config);
    SYNCE_CRIT_EXIT();

    return SYNCE_RC_OK;
}

mesa_rc vtss_appl_synce_clock_selection_mode_config_set(const vtss_appl_synce_clock_selection_mode_config_t *const config)
{
    vtss_appl_synce_selection_mode_t selection_mode;
    vtss_appl_synce_clock_selection_mode_status_t status;
    uint                          clock_input;
    uint                          clock_type;
    vtss_appl_synce_eec_option_t  my_eec_option;
    vtss_appl_synce_clock_selection_mode_config_t clock_selection_mode_config = vtss::clock_selection_config.get();

    T_W("mode = %d, source = %d, wtr_time = %d", config->selection_mode, config->source, config->wtr_time);

    if ((config->source < 1) || (config->source > fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT))) return(SYNCE_RC_INVALID_PARAMETER);
    if ((clock_selection_mode_config.selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN) && (config->selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER))    return(SYNCE_RC_INVALID_PARAMETER);
    if ((config->selection_mode == VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL_TO_SELECTED) &&
        (clock_selection_mode_config.selection_mode != VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL) &&
        (clock_selection_mode_config.selection_mode != VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE) &&
        (clock_selection_mode_config.selection_mode != VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE))    return(SYNCE_RC_INVALID_PARAMETER);
    if (config->wtr_time > 12)    return(SYNCE_RC_INVALID_PARAMETER);

    if (config->eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) {
        if ((config->ssm_holdover != VTSS_APPL_SYNCE_QL_NONE) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_PRS) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_STU) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_ST2) &&
            (config->ssm_holdover != VTSS_APPL_SYNCE_QL_TNC) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_ST3E) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_EEC2) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_SMC) &&
            (config->ssm_holdover != VTSS_APPL_SYNCE_QL_PROV) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_DUS))
            return (SYNCE_RC_INVALID_PARAMETER);
    }
    else {
        if ((config->ssm_holdover != VTSS_APPL_SYNCE_QL_NONE) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_PRC) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_SSUA) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_SSUB) &&
            (config->ssm_holdover != VTSS_APPL_SYNCE_QL_EEC1) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_DNU) && (config->ssm_holdover != VTSS_APPL_SYNCE_QL_INV))
            return (SYNCE_RC_INVALID_PARAMETER);
    }

    if (config->eec_option == VTSS_APPL_SYNCE_EEC_OPTION_2) {
        if ((config->ssm_freerun != VTSS_APPL_SYNCE_QL_NONE) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_PRS) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_STU) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_ST2) &&
            (config->ssm_freerun != VTSS_APPL_SYNCE_QL_TNC) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_ST3E) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_EEC2) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_SMC) &&
            (config->ssm_freerun != VTSS_APPL_SYNCE_QL_PROV) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_DUS))
            return (SYNCE_RC_INVALID_PARAMETER);
    }
    else {
        if ((config->ssm_freerun != VTSS_APPL_SYNCE_QL_NONE) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_PRC) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_SSUA) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_SSUB) &&
            (config->ssm_freerun != VTSS_APPL_SYNCE_QL_EEC1) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_DNU) && (config->ssm_freerun != VTSS_APPL_SYNCE_QL_INV))
            return (SYNCE_RC_INVALID_PARAMETER);
    }

    if ((config->eec_option != VTSS_APPL_SYNCE_EEC_OPTION_1) && (config->eec_option != VTSS_APPL_SYNCE_EEC_OPTION_2))
        return (SYNCE_RC_INVALID_PARAMETER);
    (void)synce_mgmt_eec_option_type_get(&clock_type);
    my_eec_option = config->eec_option;
   
    if ((my_eec_option != VTSS_APPL_SYNCE_EEC_OPTION_1) && (clock_type != 0)) my_eec_option = VTSS_APPL_SYNCE_EEC_OPTION_1;

    clock_input = config->source - 1;
    if (vtss_appl_synce_clock_selection_mode_status_get(&status) != VTSS_RC_OK)    T_D("error returned");

    switch (config->selection_mode)
    {
        case VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL:
            selection_mode = VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL;
            break;
        case VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL_TO_SELECTED:
            if (status.selector_state != VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED)
                return (SYNCE_RC_SELECTION);   /* NOT possible to make Manuel To Selected if not in locked mode */

            clock_input = status.clock_input;
            selection_mode = VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL;
            break;
        case VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE:
            selection_mode = VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE;
            break;
        case VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE:
            selection_mode = VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE;
            break;
        case VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER:
            if (status.selector_state == VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN) {
                selection_mode = VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN;   /* NOT possible to make forced hold over in free run selector state */
            }
            else {
                selection_mode = VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER;
            }
            break;
        case VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN:
            selection_mode = VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN;
            break;
        default:
            return(SYNCE_RC_INVALID_PARAMETER);
    }


    clock_selection_mode_config.selection_mode = selection_mode;
    clock_selection_mode_config.source = clock_input;
    clock_selection_mode_config.wtr_time = config->wtr_time;

    clock_selection_mode_config.ssm_holdover = config->ssm_holdover;
    clock_selection_mode_config.ssm_freerun = config->ssm_freerun;
    clock_selection_mode_config.eec_option = my_eec_option;
    vtss::clock_selection_config.set(clock_selection_mode_config);
    
    set_eec_option(my_eec_option);


    return(SYNCE_RC_OK);
}

mesa_rc synce_mgmt_nominated_priority_set(const uint source, const uint priority)
{
    uint  rc=SYNCE_RC_OK;

    T_D("source = %d, priority = %d", source, priority);

    if ((source >= synce_my_nominated_max) || (priority >= synce_my_priority_max))    return (SYNCE_RC_INVALID_PARAMETER);

    SYNCE_CRIT_ENTER();
    VTSS_ASSERT(source<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
    auto nconfig = vtss::nomination_config[source].get();
    nconfig.priority = priority;
    vtss::nomination_config[source].set(nconfig);
    SYNCE_CRIT_EXIT();

    return(rc);
}

mesa_rc synce_mgmt_ssm_set(const uint port_no, const bool ssm_enabled_copy)
{

    T_W("port_no = %d, ssm_enabled = %d", port_no, ssm_enabled_copy);

    if (port_no >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT))    return (SYNCE_RC_INVALID_PARAMETER);
    vtss::ssm_enabled[port_no].set(ssm_enabled_copy);

    return(SYNCE_RC_OK);
}

mesa_rc synce_mgmt_wtr_clear_set(const uint     source)     /* nominated source - range is 0 - synce_my_nominated_max */
{
    if (source >= fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT))    return(SYNCE_RC_INVALID_PARAMETER);

    vtss::clear_wtr[source].set(true);

    return(SYNCE_RC_OK);
}

#if defined(VTSS_SW_OPTION_PTP)
int synce_mgmt_get_best_master()
{
    return vtss::best_master.get();
}
#endif

bool clock_out_range_check(const vtss_appl_synce_frequency_t freq)
{
    bool valid[4][VTSS_APPL_SYNCE_STATION_CLK_MAX] = {{true,true,true,true}, {true, false, true,true}, {true, false, false, false}, {true, false, false, true}};
    uint clock_type;

    if (clock_station_clock_type_get(&clock_type) == VTSS_RC_OK)
        return valid[clock_type][freq];
    else
        return true;
}

bool clock_in_range_check(const vtss_appl_synce_frequency_t freq)
{
    bool valid[4][VTSS_APPL_SYNCE_STATION_CLK_MAX] = {{true,true,true,true}, {true, false, false, true}, {true, false, false, false}, {true,true,true,true}};
    uint clock_type;

    if (clock_station_clock_type_get(&clock_type) == VTSS_RC_OK)
        return valid[clock_type][freq];
    else
        return true;
}

uint synce_mgmt_station_clock_out_set(const vtss_appl_synce_frequency_t freq) /* set the station clock output frequency */
{
    mesa_rc rc = SYNCE_RC_OK;
    if (clock_out_range_check(freq)) {
        /* save this parameter in config block */
        vtss::station_clock_out.set(freq);
    } else {
        rc = SYNCE_RC_INVALID_PARAMETER;
    }
    T_W("station clock out config set attempted ");
    return(rc);
}

uint synce_mgmt_station_clock_out_get(vtss_appl_synce_frequency_t *const freq)
{
    /* get this parameter from config block */
    *freq = vtss::station_clock_out.get();
    T_D("station clock out is %u ",*freq);
    return(SYNCE_RC_OK);
}

uint synce_mgmt_station_clock_in_set(const vtss_appl_synce_frequency_t freq) /* set the station clock input frequency */
{
    mesa_rc rc = SYNCE_RC_OK;
    if (clock_in_range_check(freq)) {
        /* save this parameter in config block */
        vtss::station_clock_in.set(freq);
    } else {
        rc = SYNCE_RC_INVALID_PARAMETER;
    }
    T_W("station clock in config set attempted ");
    return(rc);
}

uint synce_mgmt_station_clock_in_get(vtss_appl_synce_frequency_t *const freq)
{
    /* get this parameter from config block */
    *freq = vtss::station_clock_in.get();
    T_D("station clock in is %u ",*freq);
    return(SYNCE_RC_OK);
}

uint synce_mgmt_station_clock_type_get(uint *const clock_type)
{
    mesa_rc rc = SYNCE_RC_OK;
    SYNCE_CRIT_ENTER();
    rc = clock_station_clock_type_get(clock_type);
    SYNCE_CRIT_EXIT();
    return (rc);
}


uint synce_mgmt_eec_option_type_get(uint *const eec_type)
{
    mesa_rc rc = SYNCE_RC_OK;
    SYNCE_CRIT_ENTER();
    rc = clock_eec_option_type_get(eec_type);
    SYNCE_CRIT_EXIT();
    return (rc);
}

mesa_rc vtss_appl_synce_clock_selection_mode_status_get(vtss_appl_synce_clock_selection_mode_status_t *const status)
{
    *status = vtss::clock_selection_status.get();

    return SYNCE_RC_OK;
}

mesa_rc vtss_appl_synce_clock_source_nomination_status_get(const uint sourceId, vtss_appl_synce_clock_source_nomination_status_t *const status)
{
    mesa_rc rc = VTSS_RC_OK;
    uint nomination_id = sourceId - 1;
    if ((sourceId >= 1) && (sourceId <= fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT))) {
        status->locs = vtss::LOCS_n[nomination_id].get();
        //status->fos = clock_alarm_state.fos[sourceId - 1];
        status->ssm = vtss::ssm_status[nomination_id].get();
        status->wtr = vtss::wtr_status[nomination_id].get();
    }
    else rc = VTSS_RC_ERROR;
    return rc;
}

mesa_rc vtss_appl_synce_port_status_get(const vtss_ifindex_t portId, vtss_appl_synce_port_status_t *const status)
{
    u32 v;
    vtss_appl_synce_clock_selection_mode_config_t clock_selection_mode_config = vtss::clock_selection_config.get();

    VTSS_RC(synce_network_port_clk_in_port_combo_to_port(portId, 0, &v));

    uint tx_ssm_code;

    T_D("portId = %d", VTSS_IFINDEX_PRINTF_ARG(portId));

    /*lint -save -e685 -e568 */
    if (v >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) return (SYNCE_RC_INVALID_PARAMETER);
    /*lint -restore */

    SYNCE_CRIT_ENTER();
    tx_ssm_code = vtss::tx_ssm[v].get();

    status->ssm_rx = vtss::ci_ql_p[v].get();
    status->ssm_tx = ssm_to_ql(tx_ssm_code , clock_selection_mode_config.eec_option);

    status->master = vtss::is_aneg_master[v].get();
    SYNCE_CRIT_EXIT();

    return(SYNCE_RC_OK);
}

mesa_rc vtss_appl_synce_ptp_port_status_get(const uint sourceId, vtss_appl_synce_ptp_port_status_t *const status)
{
#if defined(VTSS_SW_OPTION_PTP)
    
    /*lint -save -e685 -e568 */
    if (sourceId >= fast_cap(VTSS_APPL_CAP_PTP_CLOCK_CNT)) return SYNCE_RC_INVALID_PARAMETER;
    /*lint -restore */
    
    status->ssm_rx = vtss::ci_ql_p[fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT) + sourceId].get();
    status->ptsf = vtss::ptp_clock_ptsf[sourceId].get();

    return SYNCE_RC_OK;
#else
    return SYNCE_RC_NOT_SUPPORTED;
#endif
}

mesa_rc vtss_appl_synce_clock_source_nomination_config_get(uint sourceId, vtss_appl_synce_clock_source_nomination_config_t *const config)
{
    /*lint -save -e685 -e568 */
    if ((sourceId >= 1) && (sourceId <= fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT))) {
        /*lint -restore */
        VTSS_ASSERT(sourceId-1<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
        *config = vtss::nomination_config[sourceId - 1].get();

        return VTSS_RC_OK;
    }
    else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc vtss_synce_clock_source_nomination_config_all_get(vtss_appl_synce_clock_source_nomination_config_t *const config)
{
    for (auto i = 0; i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT); i++) {
        VTSS_ASSERT(i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
        config[i] = vtss::nomination_config[i].get();;
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_synce_clock_source_ports_get(const uint sourceId, const uint index, vtss_appl_synce_clock_possible_source_t *possible_source)
{
    if (synce_get_source_port(sourceId-1, index)) {
        if (index == SYNCE_STATION_CLOCK_PORT) {
            possible_source->network_port = VTSS_IFINDEX_NONE;
            possible_source->clk_in_port = 0;
        }
        else if (index > SYNCE_STATION_CLOCK_PORT) {
            possible_source->network_port = VTSS_IFINDEX_NONE;
            possible_source->clk_in_port = 128 + index - (SYNCE_STATION_CLOCK_PORT + 1);
        }
        else {
            VTSS_RC(vtss_ifindex_from_port(VTSS_ISID_LOCAL, index, &(possible_source->network_port)));
            possible_source->clk_in_port = 0;
        }
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_synce_clock_source_ports_itr(const uint *prev_sourceId, uint *next_sourceId,
                                               const uint *prev_index, uint *next_index)
{
    if (!next_index || !next_sourceId) {
        return VTSS_RC_ERROR;
    }

    if (prev_index) {
        *next_index = *prev_index+1;
    } else {
        *next_index = 0;
    }
    if (prev_sourceId) {
        *next_sourceId = *prev_sourceId;
    } else {
        *next_sourceId = 1;
    }

    while (*next_sourceId <= synce_my_nominated_max) {
        while (*next_index < SYNCE_PORT_COUNT + PTP_CLOCK_INSTANCES) {
            if (synce_get_source_port((*next_sourceId)-1, *next_index)) {
                return VTSS_RC_OK;
            }
            (*next_index)++;
        }
        *next_index = 0;
        (*next_sourceId)++;
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_synce_clock_selection_mode_config_get(vtss_appl_synce_clock_selection_mode_config_t *const config)
{
    mesa_rc rc = VTSS_RC_OK;

    *config = vtss::clock_selection_config.get();
    config->source++;

    return rc;
}

mesa_rc vtss_appl_synce_station_clock_config_set(const vtss_appl_synce_station_clock_config_t *const config)
{
    T_W("station clock config set attempted ");
    if (synce_mgmt_station_clock_in_set(config->station_clk_in) != SYNCE_RC_OK ||
        synce_mgmt_station_clock_out_set(config->station_clk_out) != SYNCE_RC_OK)
    {
        return VTSS_RC_ERROR;
    }
    else {
        return VTSS_RC_OK;
    }
}

mesa_rc vtss_appl_synce_station_clock_config_get(vtss_appl_synce_station_clock_config_t *const config)
{
    config->station_clk_in  = vtss::station_clock_in.get();
    config->station_clk_out = vtss::station_clock_out.get();

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_synce_port_config_set(const vtss_ifindex_t portId, const vtss_appl_synce_port_config_t *const config)
{
    u32 v;
    VTSS_RC(synce_network_port_clk_in_port_combo_to_port(portId, 0, &v));

    T_W("port_no = %d, ssm_enabled = %d", v, config->ssm_enabled);
    /*lint -save -e685 -e568 */
    if (v <= SYNCE_PORT_COUNT-1) {
        /*lint -restore */
        vtss::ssm_enabled[v].set(config->ssm_enabled);
        return VTSS_RC_OK;
    }
    else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc vtss_appl_synce_port_config_get(const vtss_ifindex_t portId, vtss_appl_synce_port_config_t *const config)
{
    u32 v;
    VTSS_RC(synce_network_port_clk_in_port_combo_to_port(portId, 0, &v));

    /*lint -save -e685 -e568 */
    if (v <= SYNCE_PORT_COUNT-1) {
        /*lint -restore */
        config->ssm_enabled = vtss::ssm_enabled[v].get();
        return VTSS_RC_OK;
    }
    else {
        return VTSS_RC_ERROR;
    }
}

uint synce_mgmt_register_get(const uint reg, uint *const  value)
{
    if (clock_read(reg, value) != VTSS_RC_OK)    T_D("error returned");
    return(SYNCE_RC_OK);
}

uint synce_mgmt_register_set(const uint reg, const uint value)
{
    if (clock_write(reg, value) != VTSS_RC_OK)    T_D("error returned");
    return(SYNCE_RC_OK);
}

#if defined(VTSS_SW_OPTION_PTP)
mesa_rc vtss_synce_ptp_port_state_get(const uint idx, synce_ptp_port_state_t *state) 
{
    mesa_rc rc = VTSS_RC_ERROR;
    
    if (idx < fast_cap(VTSS_APPL_CAP_PTP_CLOCK_CNT)) {
        state->ssm_rx = vtss::ptp_config_handler[idx].cur_ssm;
        state->ptsf = vtss::ptp_clock_ptsf[idx].get();
        rc = VTSS_RC_OK;
    }
    return rc;
}
#endif

#if defined(VTSS_SW_OPTION_PTP)
mesa_rc vtss_synce_ptp_clock_ssm_ql_set(const uint idx, u8 clockClass) {
    vtss::ptp_clock_class[idx].set(clockClass);
    return VTSS_RC_OK;
}

const char *vtss_sync_ptsf_state_2_txt(vtss_appl_synce_ptp_ptsf_state_t s)
{
    switch (s) {
        case SYNCE_PTSF_NONE  : return "PTSF_NONE";
        case SYNCE_PTSF_UNUSABLE: return "PTSF_UNUSABLE";
        case SYNCE_PTSF_LOSS_OF_SYNC: return "PTSF_LOSS_OF_SYNC";
        case SYNCE_PTSF_LOSS_OF_ANNOUNCE: return "PTSF_LOSS_OF_ANNOUNCE";
        default: return "PTSF_UNKNOWN";
    }
}

mesa_rc vtss_synce_ptp_clock_ptsf_state_set(const uint idx, vtss_appl_synce_ptp_ptsf_state_t ptsfState) {

    vtss::ptp_clock_ptsf[idx].set(ptsfState);

    return VTSS_RC_OK;
}

mesa_rc vtss_synce_ptp_clock_hybrid_mode_set(BOOL hybrid)
{
    T_I("hybrid mode changed to %s", hybrid ? "TRUE" : "FALSE");
    vtss::ptp_hybrid_mode.set(hybrid);
    return VTSS_RC_OK;

}

#endif

/******************************************************************************
 * Description: Set frequency adjust value
 *
 ******************************************************************************/
mesa_rc synce_mgmt_clock_adjtimer_set(i64 adj)
{
    mesa_rc rc = SYNCE_RC_OK;
    rc = clock_adjtimer_set(adj);
    if (rc == VTSS_RC_ERROR) rc = SYNCE_RC_NOT_SUPPORTED;
    return rc;
}

mesa_rc synce_mgmt_clock_ql_get(vtss_appl_synce_quality_level_t *ql)
{
    mesa_rc rc = SYNCE_RC_OK;
    *ql = vtss::selected_ql.get();
    return rc;
}

extern "C" void vtss_synce_json_init();
extern "C" int synce_icli_cmd_register();

void vtss_appl_synce_reset_configuration_to_default()
{
    conf_blk_t save_blk;

    clock_reset_to_defaults();

    synce_set_clock_source_nomination_config_to_default(save_blk.clock_source_nomination_config);
    synce_set_clock_selection_mode_config_to_default(&save_blk.clock_selection_mode_config);
    synce_set_station_clock_config_to_default(&save_blk.station_clock_config);
    synce_set_port_config_to_default(save_blk.port_config.data());
    
    apply_configuration(&save_blk);

    vtss::LOCS_n[0].set(false);
}

/* Initialize module */
mesa_rc synce_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    port_iter_t pit;
    u32         i;
    mesa_rc     rc;

// Note: The code installing the ssm_frame_rx packet filter callback has been moved to func_thread and delayed 20 seconds from startup since the Linux system
//       seems to be occupying the SPI IF for extended periods during startup up. This causes SyncE / Zarlink API to wait for access while holding a lock
//       that prevents the ssm_frame_rx routine from running causing further problems.
//
// #ifdef VTSS_SW_OPTION_PACKET
//     packet_rx_filter_t filter;
//     void               *filter_id;
// #endif

    /*lint --e{454,456} */

    switch (data->cmd) {
    case INIT_CMD_EARLY_INIT:
        /* initialize critd */
        critd_init_legacy(&crit, "SyncE", VTSS_MODULE_ID_SYNCE, CRITD_TYPE_MUTEX);
        break;

    case INIT_CMD_INIT:
        T_D("INIT");
        if (fast_cap(MESA_CAP_CLOCK)) {
            vtss_synce_json_init();
        }

        if (synce_dpll) {
            synce_my_nominated_max = synce_dpll->clock_my_input_max;      // FIXME: Make sure that the synce_dpll clock_init function is called before this happens or the values will be undefined.
            synce_my_priority_max = synce_dpll->clock_my_input_max;       //        This means that in main.c synce_dpll needs not be initialized before synce
            synce_my_prio_disabled = synce_dpll->synce_my_prio_disabled;  //
            dpll_type = synce_dpll->dpll_type();
            T_I("Own dpll type is %d (3)", dpll_type);
        } else {
            synce_my_nominated_max = CLOCK_INPUT_MAX - 1;                 // FIXME: This is really a hack to prevent a seg-fault in case no DPLL is detected.
            synce_my_priority_max = CLOCK_INPUT_MAX - 1;                  //        We should handle this case in a better way since when no DPLL is present, synce is not supported at all.
            synce_my_prio_disabled = CLOCK_INPUT_MAX - 1;                 //
        }

        T_WG(TRACE_GRP_DEVELOP,"DPLL type detected:: %u",dpll_type);

        // Hook up for port link state changes.
        // The vtss::port_change_event() function will be called back once per
        // port within the next second, so no need to cache the current link
        // status.
        if ((rc = port_change_register(VTSS_MODULE_ID_SYNCE, vtss::port_change_event)) != VTSS_RC_OK) {
            T_D("error returned rc %u", rc);
        }

        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                          func_thread,
                          0,
                          "SYNCE Function",
                          nullptr,
                          0,
                          &func_thread_handle,
                          &func_thread_block);
        SYNCE_CRIT_EXIT();

#ifdef VTSS_SW_OPTION_ICFG
        // Initialize ICLI "show running" configuration
        VTSS_RC(synce_icfg_init());
#endif
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        vtss_synce_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_synce_json_init();
#endif
        synce_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_D("START");

        T_DG(TRACE_GRP_DEVELOP,"Max input clock sources, ethernet :: %u",fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT));
        T_DG(TRACE_GRP_DEVELOP,"Max input clock sources, PTP :: %u",fast_cap(VTSS_APPL_CAP_PTP_CLOCK_CNT));
        T_DG(TRACE_GRP_DEVELOP,"Max input clock sources,eth+ptp+station :: %u",fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_AND_PTP_CNT));
        T_DG(TRACE_GRP_DEVELOP,"Max clock sources , can be nominated :: %u",fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT));
        VTSS_RC(synce_board_init());

        vtss::set_synce_config_to_default();
        // initialize all eventhandlers
        for (i=0; i<fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); ++i) {
            vtss::base_t_master_slaves[i].init((u32)i);
            vtss::port_monitors[i].init((u32)i);
            vtss::rx_ssm_processors[i].init((u32)i);
            vtss::tx_ssm_processor[i].init((u32)i);
        }
        /* Initialize station clock configuration , event handler */
        vtss::station_clock_handler.init();
        /* Initialize nomination process event handler */
        for (i=0; i<fast_cap(VTSS_APPL_CAP_SYNCE_NOMINATED_CNT); i++) {
            vtss::nomination_handler[i].init((u8)i);
            vtss::ho_wtr_processor[i].init((u8)i);
            vtss::source_monitor[i].init((u8)i);
        }

        /* Initialize ptp configuration event handler */
#if defined(VTSS_SW_OPTION_PTP)
        for (i=0; i<fast_cap(VTSS_APPL_CAP_PTP_CLOCK_CNT); i++) {
            vtss::ptp_config_handler[i].init((u8)i);
            vtss::ptp_monitor[i].init(i, i + fast_cap(VTSS_APPL_CAP_SYNCE_PORT_AND_STATION_CNT));
        }
        vtss::ptp_transient_processor.init();
#endif

        /* Initialize DPLL status event handler */
        vtss::dpll_status_update.init();

        /* Initialize clock selection process handler*/
        vtss::selection_process.init();

        /* Initialize internal source event handler */
        vtss::internal_source.init();

        /* Initialize selector state status processors */
        vtss::selector_state_monitor.init();
        vtss::selector_state_combined.init();
        vtss::node_monitor.init();
        T_I("INIT_CMD_START synce");
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF");
        if (isid == VTSS_ISID_GLOBAL) {
            vtss_appl_synce_reset_configuration_to_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");

        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            i = pit.iport;
            T_I("port %d mux_selector[0] = %d, mux_selector[1] = %d", i, synce_get_mux_selector(0, i), synce_get_mux_selector(1, i));
        }

        T_I("port %d mux_selector[0] = %d, mux_selector[1] = %d", SYNCE_STATION_CLOCK_PORT, synce_get_mux_selector(0, SYNCE_STATION_CLOCK_PORT), synce_get_mux_selector(1, SYNCE_STATION_CLOCK_PORT));
        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            i = pit.iport;
            T_I("port %d source[0] = %d, source[1] = %d, source[2] = %d",i, synce_get_source_port(0, i), synce_get_source_port(1, i), synce_get_source_port(2, i));
        }

        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            i = pit.iport;
            T_I("port %d switch_rcvrd_clock[0] = %d, switch_rcvrd_clock[1] = %d, switch_rcvrd_clock[2] = %d",i, synce_get_switch_recovered_clock(0, i), synce_get_switch_recovered_clock(1, i), synce_get_switch_recovered_clock(2, i));
        }

        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            i = pit.iport;
            T_I("port %d phy_rcvrd_clock[0] = %d, phy_rcvrd_clock[1] = %d, phy_rcvrd_clock[2] = %d",i, synce_get_phy_recovered_clock(0, i), synce_get_phy_recovered_clock(1, i), synce_get_phy_recovered_clock(2, i));
        }

        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            mesa_port_speed_t p_speed = vtss::port_info[i].get().speed;
            i = pit.iport;
            T_I("port %d ref[0] = %d, ref[1] = %d, ref[2] = %d",i, synce_get_selector_ref_no(0, i), synce_get_selector_ref_no(1, i), synce_get_selector_ref_no(2, i));
            T_I("port %d, freq (via ref[0]) = %d, freq (via ref[0]) = %d, freq (via ref[0]) = %d", i, synce_get_rcvrd_clock_frequency(0, i, p_speed), synce_get_rcvrd_clock_frequency(1, i, p_speed), synce_get_rcvrd_clock_frequency(2, i, p_speed));
        }

        T_I("port %d ref[0] = %d, ref[1] = %d, ref[2] = %d", SYNCE_STATION_CLOCK_PORT, synce_get_selector_ref_no(0, SYNCE_STATION_CLOCK_PORT),
                                                                                       synce_get_selector_ref_no(1, SYNCE_STATION_CLOCK_PORT),
                                                                                       synce_get_selector_ref_no(2, SYNCE_STATION_CLOCK_PORT));

// Note: The code installing the ssm_frame_rx packet filter callback has been moved to func_thread and delayed 20 seconds from startup since the Linux system
//       seems to be occupying the SPI IF for extended periods during startup up. This causes SyncE / Zarlink API to wait for access while holding a lock
//       that prevents the ssm_frame_rx routine from running causing further problems.
//
// #ifdef VTSS_SW_OPTION_PACKET
//             /* hook up on SSM frame */
//             packet_rx_filter_init(&filter);
//             filter.modid = VTSS_MODULE_ID_SYNCE;
//             filter.match = PACKET_RX_FILTER_MATCH_DMAC | PACKET_RX_FILTER_MATCH_ETYPE;
//             filter.cb    = ssm_frame_rx;
//             filter.prio  = PACKET_RX_FILTER_PRIO_NORMAL;
//             filter.etype = 0x8809; // slow protocol ethertype
//             memcpy(filter.dmac, ssm_dmac, sizeof(filter.dmac));
//
//             if ((rc = packet_rx_filter_register(&filter, &filter_id)) != VTSS_RC_OK)    T_D("error returned rc %u", rc);
// #endif

        control_system_reset_register(system_reset, VTSS_MODULE_ID_SYNCE);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        // Subscribe to FLNK and LOS interrupts
        if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_SYNCE, vtss::port_interrupt_callback, MEBA_EVENT_FLNK, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
            T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
        }

        if ((rc = vtss_interrupt_source_hook_set(VTSS_MODULE_ID_SYNCE, vtss::port_interrupt_callback, MEBA_EVENT_LOS, INTERRUPT_PRIORITY_PROTECT)) != VTSS_RC_OK) {
            T_E("vtss_interrupt_source_hook_set() failed: %s", error_txt(rc));
        }
        break;
    default:
        break;
    }

    return VTSS_RC_OK;
}

