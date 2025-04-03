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

#include "xxrp_trace.h"
#include "xxrp_api.h"
#include "vtss_xxrp_callout.h"
#include "critd_api.h"
#include "misc_api.h"
#include "conf_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#include "packet_api.h"
#include "l2proto_api.h"
#include "vlan_api.h"
#include "msg_api.h"
#include "vtss_bip_buffer_api.h"
#include "aggr_api.h"
#ifdef VTSS_SW_OPTION_MVRP
#endif
#include "mstp_api.h"
#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#include "xxrp_icfg.h"
#endif
#ifdef VTSS_SW_OPTION_GVRP
#include "../base/vtss_garp.h"
#include "../base/vtss_gvrp.h"
#endif
#include "../base/vtss_xxrp_callout.h"
#include "vtss_mrp.hxx"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_XXRP

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "xxrp", "XXRP module (base)"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_XXRP_GRP_DEFAULT] = {
        "default",
        "Default (XXRP base)",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_XXRP_GRP_PLATFORM] = {
        "platform",
        "Platform calls",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_XXRP_GRP_TIMER] = {
        "timer",
        "Timer calls",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_XXRP_GRP_FSM] = {
        "fsm",
        "State machines' calls",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_XXRP_GRP_RX] = {
        "rx",
        "Rx calls",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_XXRP_GRP_TX] = {
        "tx",
        "Tx calls",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_XXRP_GRP_GVRP] = {
        "gvrp",
        "GVRP operations",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_XXRP_GRP_ICLI] = {
        "icli",
        "ICLI calls",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_XXRP_GRP_WEB] = {
        "web",
        "WEB calls",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_XXRP_GRP_MIB] = {
        "mib",
        "MIB calls",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/***************************************************************************************************
 * Macros for accessing semaphore functions
 **************************************************************************************************/
#define XXRP_BASE_CRIT_ENTER()                  critd_enter(        &XXRP_base_crit,          __FILE__, __LINE__)
#define XXRP_BASE_CRIT_EXIT()                   critd_exit(         &XXRP_base_crit,          __FILE__, __LINE__)
#define XXRP_BASE_CRIT_ASSERT()                 critd_assert_locked(&XXRP_base_crit,          __FILE__, __LINE__)
#define XXRP_PLATFORM_CRIT_ENTER()              critd_enter(        &XXRP_platform_crit,      __FILE__, __LINE__)
#define XXRP_PLATFORM_CRIT_EXIT()               critd_exit(         &XXRP_platform_crit,      __FILE__, __LINE__)
#define XXRP_PLATFORM_CRIT_ASSERT_LOCKED()      critd_assert_locked(&XXRP_platform_crit,      __FILE__, __LINE__)
/* XXRP_mstp_platform_crit is required as we see conflicts (deadlocks) btw platform and mstp mutexes */
#define XXRP_MSTP_PLATFORM_CRIT_ENTER()         critd_enter(        &XXRP_mstp_platform_crit, __FILE__, __LINE__)
#define XXRP_MSTP_PLATFORM_CRIT_EXIT()          critd_exit(         &XXRP_mstp_platform_crit, __FILE__, __LINE__)
#define XXRP_MSTP_PLATFORM_CRIT_ASSERT_LOCKED() critd_assert_locked(&XXRP_mstp_platform_crit, __FILE__, __LINE__)

// Generic configuration structure (primary switch only)
// The primary switch  contains an instance of this for each application (MVRP, GVRP, etc)
// Remember to subtract VTSS_ISID_START and VTSS_PORT_NO_START from indices
typedef struct {
    bool                  global_enable;
    u8                    port_enable[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE]; // Using bit field saves memory
    CapArray<vtss_mrp_timer_conf_t, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> timers;
    vtss_mrp_timer_conf_t global_timers;
    vtss::VlanList        managedVlans;
    u8                    periodic_tx[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE]; // Using bit field saves memory
    CapArray<u32, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> applicant_adm;      // Bit field - up to 32 attribute types where attribute type 1 is LSB and FALSE == normal participant
} xxrp_stack_conf_t;

// Generic structure used for local enable/disable configuration on each switch
// Each switch contains an instance of this for each application (MVRP, GVRP, etc)
// The purpose of this is to avoid sending MRPDUs to the primary switch for a disabled port
typedef struct {
    BOOL global_enable;
    u8   port_enable[VTSS_PORT_BF_SIZE]; // Using bit field saves memory
} xxrp_local_conf_t;

#define XXRP_MSTP_PORT_STATE_FORWARDING     1
#define XXRP_MSTP_PORT_STATE_DISCARDING     0

enum {
    XXRP_MSTP_PORT_NOTHING = 0,
    XXRP_MSTP_PORT_FLUSH,
    XXRP_MSTP_PORT_REDECLARE
};

typedef struct {
    u8 port_state_changed;    //Flag to check if port state has changed
    u8 port_state;            //MSTP port state
    u8 port_role;             //MSTP port role
} xxrp_mstp_port_conf_t;

/***************************************************************************************************
 * Msg definitions
 **************************************************************************************************/
typedef enum {
    XXRP_MSG_ID_LOCAL_CONF_SET,   // Local configuration set request (no reply)
} xxrp_msg_id_t;

// Message for configuration sent by primary switch.
typedef struct {
    xxrp_msg_id_t     msg_id;      // Message ID
    vtss_mrp_appl_t   appl;        // Application for which this configuration applies
    xxrp_local_conf_t switch_conf; // Configuration that is local for a switch
} xxrp_msg_local_conf_t;

/***************************************************************************************************
 * Event flags
 **************************************************************************************************/
#define XXRP_EVENT_FLAG_MRPDU                       (1 << 0) /* MRPDU has been received */
#define XXRP_EVENT_FLAG_KICK                        (1 << 1) /* Wake up the thread imediately */
#define XXRP_EVENT_FLAG_MSTP_PORT_STATE_CHANGE      (1 << 2) /* MSTP port state change */
#define XXRP_EVENT_FLAG_MSTP_PORT_ROLE_CHANGE       (1 << 3) /* MSTP port role change */
#define XXRP_EVENT_FLAG_VLAN_2_MSTI_MAP_CHANGE      (1 << 4) /* VLAN->MSTI mapping change */
#define XXRP_EVENT_FLAG_VLAN_MEMBER_CHANGE          (1 << 5) /* GVRP->VLAN member change */

// A little helper macro. No timeout if w == 0
#define XXRP_FLAG_WAIT(f, w) ((w) ?                                     \
                              vtss_flag_timed_wait(f, 0xfffffff, VTSS_FLAG_WAITMODE_OR_CLR, w) : \
                              vtss_flag_wait(      f, 0xfffffff, VTSS_FLAG_WAITMODE_OR_CLR))

/***************************************************************************************************
 * RX buffer
 **************************************************************************************************/
#define XXRP_RX_BUFFER_SIZE_BYTES   40000

/***************************************************************************************************
 * Application specific parameter table
 **************************************************************************************************/
#define XXRP_MVRP_ETH_TYPE   (0x88f5)
#define XXRP_MVRP_MAC_ADDR   {0x01, 0x80, 0xc2, 0x00, 0x00, 0x21}
#define XXRP_GARP_LLC_HEADER {0x42, 0x42, 0x03}

static const uchar llc_header[] = XXRP_GARP_LLC_HEADER;

typedef struct {
    mesa_mac_addr_t dmac;                 // Application specific destination MAC address
    u16             etype;                // Application specific ether-type. Set to 0 when using LLC header (GARP only)
    u8              attr_type_cnt;        // Maximum number of attribute types supported in this application
} xxrp_mrp_appl_parm_t;

static const xxrp_mrp_appl_parm_t XXRP_mrp_appl_parm[VTSS_MRP_APPL_MAX] = {
#ifdef VTSS_SW_OPTION_MVRP
    [VTSS_MRP_APPL_MVRP] = {
        XXRP_MVRP_MAC_ADDR,
        XXRP_MVRP_ETH_TYPE,
        1                               // Only one: VID Vector
    },
#endif

#ifdef VTSS_SW_OPTION_GVRP
    [VTSS_GARP_APPL_GVRP] = {
        XXRP_MVRP_MAC_ADDR,
        0, // len/type indicate LLC
        1                               // Only one: VID Vector
    },
#endif
};

/***************************************************************************************************
 * Global variables
 **************************************************************************************************/
static critd_t                  XXRP_base_crit;                     // Critical section for base
static critd_t                  XXRP_platform_crit;                 // Critical section for platform
static critd_t                  XXRP_mstp_platform_crit;            // Critical section for platform code related to mstp
critd_t xxrp_appl_crit;

static xxrp_local_conf_t        XXRP_local_conf[VTSS_MRP_APPL_MAX]; // One instance per application (all switches)
static xxrp_stack_conf_t        XXRP_stack_conf[VTSS_MRP_APPL_MAX]; // One instance per application (primary switch only)

static vtss_flag_t              XXRP_tm_thread_flag;                // Flag for signalling events to timer thread
static vtss_handle_t            XXRP_tm_thread_handle;
static vtss_thread_t            XXRP_tm_thread_state;

static vtss_flag_t              XXRP_rx_thread_flag;                // Flag for signalling events to rx thread
static vtss_handle_t            XXRP_rx_thread_handle;
static vtss_thread_t            XXRP_rx_thread_state;

static vtss_bip_buffer_t        XXRP_rx_buffer;                     // Buffer for received MRPDUs (primary switch only)

static u8                       XXRP_p2p_cache[VTSS_ISID_CNT][VTSS_PORT_BF_SIZE]; // Bit == TRUE means p2p
static CapArray<xxrp_mstp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT, VTSS_APPL_CAP_MSTI_CNT> xxrp_mstp_port_conf;

/***************************************************************************************************
 * Private functions
 **************************************************************************************************/

/***************************************************************************************************
 * XXRP_base_port_timer_set_specific()
 * Update base module with port timer configuration for specific application.
 * If check == TRUE it verifies the ports enabled state.
 **************************************************************************************************/
static void XXRP_base_port_timer_set_specific(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, BOOL check)
{
    BOOL enabled = TRUE;

    XXRP_PLATFORM_CRIT_ASSERT_LOCKED();
    if (check) {
        enabled = (XXRP_stack_conf[appl].global_enable && VTSS_PORT_BF_GET(XXRP_stack_conf[appl].port_enable[isid - VTSS_ISID_START], iport));
    }

    if (enabled) {
        mesa_rc brc;
        u32 l2port = L2PORT2PORT(isid, iport);
        if ((brc = vtss_xxrp_timers_conf_set(appl, l2port, &XXRP_stack_conf[appl].global_timers)) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Unable to update timer "
                    << " conf for appl =  "
                    << appl << ", port = "
                    << l2port2str(l2port)
                    << " - " << error_txt(brc);
        }
    }
}

/***************************************************************************************************
 * Update base module with port periodix tx configuration for specific application.
 * If check == TRUE it verifies the ports enabled state.
 **************************************************************************************************/
static void XXRP_base_port_periodic_tx_set_specific(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, BOOL check)
{
    BOOL enabled = TRUE;

    XXRP_PLATFORM_CRIT_ASSERT_LOCKED();

    if (check) {
        enabled = (XXRP_stack_conf[appl].global_enable && VTSS_PORT_BF_GET(XXRP_stack_conf[appl].port_enable[isid - VTSS_ISID_START], iport));
    }

    if (enabled) {
        mesa_rc brc;
        u32     l2port = L2PORT2PORT(isid, iport);
        bool periodic_status = VTSS_PORT_BF_GET(XXRP_stack_conf[appl].periodic_tx[isid - VTSS_ISID_START], iport);
        if ((brc = vtss_xxrp_periodic_transmission_control_conf_set(appl, l2port, periodic_status)) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Error while updating periodic tx configuration - "
                    << error_txt(brc);
            return;
        }
    }
}

/***************************************************************************************************
 * Update base module with port applicant adm configuration for specific application.
 * If check == TRUE it verifies the ports enabled state.
 **************************************************************************************************/
static void XXRP_base_port_applicant_adm_set_specific(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, BOOL check)
{
    BOOL enabled = TRUE;

    XXRP_PLATFORM_CRIT_ASSERT_LOCKED();

    if (check) {
        enabled = (XXRP_stack_conf[appl].global_enable && VTSS_PORT_BF_GET(XXRP_stack_conf[appl].port_enable[isid - VTSS_ISID_START], iport));
    }

    if (enabled) {
        int adm_ix;
        u32 brc;
        u32 l2port = L2PORT2PORT(isid, iport);
        u32 applicant_adm = XXRP_stack_conf[appl].applicant_adm[isid - VTSS_ISID_START][iport - VTSS_PORT_NO_START];

        for (adm_ix = 0; adm_ix < 32; adm_ix++) { // Loop through all applicant_adm bits
            BOOL                      participant;
            vtss_mrp_attribute_type_t attr_type;

            if (adm_ix >= XXRP_mrp_appl_parm[appl].attr_type_cnt) {
                break; // No more attribute types defined for this MRP application
            }

            participant = !(applicant_adm & (1 << adm_ix)); // bit == 1 in applicant_adm means non-participant - base module expects the inverse
            attr_type.dummy = adm_ix + 1;
#ifdef VTSS_SW_OPTION_MVRP
            if (appl == VTSS_MRP_APPL_MVRP) {
                attr_type.mvrp_attr_type = VTSS_MVRP_VLAN_ATTRIBUTE;
            }
#endif /* VTSS_SW_OPTION_MVRP */
            if ((brc = vtss_xxrp_applicant_admin_control_conf_set(appl, l2port, attr_type, participant))) {
                T_EG(VTSS_TRACE_XXRP_GRP_PLATFORM, "Unable to update applicant adm conf for appl %u port %u (brc %u)", appl, l2port, brc);
                return;
            }
        }
    }
}

/***************************************************************************************************
 * Synchronize base module with specific port configuration and state for a given application.
 * This function expects that the application is globally enabled.
 **************************************************************************************************/
static void XXRP_base_port_sync_specific(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl)
{
    mesa_rc brc;
    BOOL enabled_base = FALSE;
    BOOL enabled_conf = VTSS_PORT_BF_GET(XXRP_stack_conf[appl].port_enable[isid - VTSS_ISID_START], iport);
    u32  l2port = L2PORT2PORT(isid, iport);
    vtss::VlanList vls = XXRP_stack_conf[appl].managedVlans;

    XXRP_PLATFORM_CRIT_ASSERT_LOCKED();

    if (appl == VTSS_GARP_APPL_GVRP) {
        if ((brc = vtss_xxrp_port_control_conf_get(appl, l2port, &enabled_base)) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Unable to get port conf for appl = "
                    << appl << ", port = " << l2port2str(l2port) << " - "
                    << error_txt(brc);
            return;
        }
    }
    if (appl == VTSS_MRP_APPL_MVRP) {
        (void)vtss::mrp::port_state_get(l2port, enabled_base);
    }

    if (enabled_conf != enabled_base) { // Synchronize port enabled

        if (appl == VTSS_GARP_APPL_GVRP) {
            if ((brc = vtss_xxrp_port_control_conf_set(appl, l2port, enabled_conf, vls)) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Unable to sync port conf for appl = "
                        << appl << ", port = " << l2port2str(l2port)
                        << " - " << error_txt(brc);
                return;
            }
        }
        if (appl == VTSS_MRP_APPL_MVRP) {
            (void)vtss::mrp::port_state_set(l2port, enabled_conf);
        }
    }

    if (enabled_conf) { // Update all other port configuration and state
        if (appl == VTSS_GARP_APPL_GVRP) {
            XXRP_base_port_timer_set_specific(isid, iport, appl, FALSE);
            XXRP_base_port_periodic_tx_set_specific(isid, iport, appl, FALSE);
            XXRP_base_port_applicant_adm_set_specific(isid, iport, appl, FALSE);
        }
        if (appl == VTSS_MRP_APPL_MVRP) {
            u32 port_no = L2PORT2PORT(isid, iport);
            vtss::mrp::MrpTimeouts t;
            vtss_mrp_timer_conf_t *timers = &XXRP_stack_conf[VTSS_MRP_APPL_MVRP].timers[isid - VTSS_ISID_START][iport - VTSS_PORT_NO_START];
            t.join = (vtss::milliseconds) (10 * timers->join_timer);
            t.leave = (vtss::milliseconds) (10 * timers->leave_timer);
            t.leaveAll = (vtss::milliseconds) (10 * timers->leave_all_timer);
            (void)vtss::mrp::port_timers_set(port_no, t);
        }
    }
}

/***************************************************************************************************
 * XXRP_base_switch_sync_specific()
 * Called on one of the following events:
 * 1: A switch has been added (via XXRP_base_switch_sync())
 * 2: Configuration reset to default (whole stack) (via XXRP_base_switch_sync())
 * 3: An MRP application has been globally enabled or disabled
 * Synchronize base module with actual configuration.
 **************************************************************************************************/
static void XXRP_base_switch_sync_specific(vtss_isid_t isid, vtss_mrp_appl_t appl)
{
    u32  brc;
    BOOL global_enabled_base = FALSE;
    BOOL global_enabled_conf = XXRP_stack_conf[appl].global_enable;

    XXRP_PLATFORM_CRIT_ASSERT_LOCKED();
    if (appl == VTSS_GARP_APPL_GVRP) {
        if ((brc = vtss_mrp_global_control_conf_get(appl, &global_enabled_base)) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Unable to get global conf for appl " << appl
                    << " - " << error_txt(brc);
            return;
        }
    }
    if (appl == VTSS_MRP_APPL_MVRP) {
        (void)vtss::mrp::global_state_get(global_enabled_base);
    }

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, NOISE) << "Sync appl = " << appl << " global status = "
            << global_enabled_conf << " into base's status = "
            << global_enabled_base;
    if (global_enabled_conf != global_enabled_base) {
        if (appl == VTSS_GARP_APPL_GVRP) {
            if ((brc = vtss_mrp_global_control_conf_set(appl, global_enabled_conf)) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Unable to sync global conf for appl = "
                        << appl << " - " << error_txt(brc);
                return;
            }
        }
        if (appl == VTSS_MRP_APPL_MVRP) {
            vtss::VlanList vls = XXRP_stack_conf[appl].managedVlans;
            if (global_enabled_conf) {
                (void)vtss::mrp::vlan_list_set(vls);
                (void)vtss::mrp::global_state_set(global_enabled_conf);
            } else {
                (void)vtss::mrp::global_state_set(global_enabled_conf);
                (void)vtss::mrp::vlan_list_set(vls);
            }
        }
    }
    /* Then sync the ports as well */
    if (global_enabled_conf) {
        switch_iter_t sit;
        port_iter_t   pit;

        (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID);
        (void)port_iter_init(&pit, &sit, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            XXRP_base_port_sync_specific(sit.isid, pit.iport, appl);
        }
    }
}

/***************************************************************************************************
 **************************************************************************************************/
static void XXRP_base_switch_sync(vtss_isid_t isid)
{
    vtss_mrp_appl_t appl;

    for (appl = (vtss_mrp_appl_t)0; appl < VTSS_MRP_APPL_MAX; appl++) {
        XXRP_base_switch_sync_specific(isid, appl);
    }
}

/***************************************************************************************************
 * Setup forwarding or copy to CPU for all MRPDUs on local switch
 **************************************************************************************************/
void XXRP_forwarding_control(void)
{
    mesa_rc               rc;
    vtss_mrp_appl_t       appl;
    mesa_packet_rx_conf_t conf;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, NOISE) << "Configure MRPDU forwarding control";
    XXRP_PLATFORM_CRIT_ASSERT_LOCKED();
    memset(&conf, 0, sizeof(mesa_packet_rx_conf_t));

    // mesa_packet_rx_conf_get()/set() must be called without interference.
    VTSS_APPL_API_LOCK_SCOPE();

    if ((rc = mesa_packet_rx_conf_get(NULL, &conf)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Failed to get packet_rx_conf - " << error_txt(rc);
        return;
    }

    // First run: Disable copying to CPU for all included MRP applications
    for (appl = (vtss_mrp_appl_t)0; appl < VTSS_MRP_APPL_MAX; appl++) {
        int dmac_ix = XXRP_mrp_appl_parm[appl].dmac[5] & 0x0f; // Get the last nibble in the dmac
        conf.reg.garp_cpu_only[dmac_ix] = FALSE;
    }

    // Second run: Enable copying to CPU for all enabled and included MRP applications
    // This is neccessary as DMAC addresses can be shared by MRP application
    for (appl = (vtss_mrp_appl_t)0; appl < VTSS_MRP_APPL_MAX; appl++) {
        int dmac_ix = XXRP_mrp_appl_parm[appl].dmac[5] & 0x0f; // Get the last nibble in the dmac
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, NOISE) << "appl = " << appl
                << ", global status = " << XXRP_local_conf[appl].global_enable;
        if (XXRP_local_conf[appl].global_enable) {
            conf.reg.garp_cpu_only[dmac_ix] = TRUE;
        }
    }

    if ((rc = mesa_packet_rx_conf_set(NULL, &conf)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Failed to set packet_rx_conf - " << error_txt(rc);
    }
}

/***************************************************************************************************
 * Initialize generic configuration to default.
 **************************************************************************************************/
static void XXRP_conf_default(xxrp_stack_conf_t *cfg)
{
    switch_iter_t       sit;
    port_iter_t         pit;

    vtss_clear(*cfg);
    cfg->global_timers.join_timer = VTSS_MRP_JOIN_TIMER_DEF;
    cfg->global_timers.leave_timer = VTSS_MRP_LEAVE_TIMER_DEF;
    cfg->global_timers.leave_all_timer = VTSS_MRP_LEAVE_ALL_TIMER_DEF;
    cfg->managedVlans.clear_all();
    for (int v = VTSS_APPL_MVRP_VLAN_ID_MIN; v <= VTSS_APPL_MVRP_VLAN_ID_MAX; ++v) {
        cfg->managedVlans.set(v);
    }
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
    (void)port_iter_init(&pit, &sit, VTSS_ISID_GLOBAL, PORT_ITER_SORT_ORDER_IPORT_ALL, PORT_ITER_FLAGS_ALL);
    while (port_iter_getnext(&pit)) {
        vtss_mrp_timer_conf_t *timers = &cfg->timers[sit.isid - VTSS_ISID_START][pit.iport - VTSS_PORT_NO_START];
        timers->join_timer      = VTSS_MRP_JOIN_TIMER_DEF;
        timers->leave_timer     = VTSS_MRP_LEAVE_TIMER_DEF;
        timers->leave_all_timer = VTSS_MRP_LEAVE_ALL_TIMER_DEF;
    }

#ifdef VTSS_SW_OPTION_GVRP
    GVRP_CRIT_ENTER();
    vtss_gvrp_destruct(TRUE);
    GVRP_CRIT_EXIT();
#endif
}

static void mrp_conf_reset()
{
    xxrp_stack_conf_t     *cfg = &XXRP_stack_conf[VTSS_MRP_APPL_MVRP];
    vtss_mrp_timer_conf_t timers;
    switch_iter_t         sit;
    port_iter_t           pit;

    vtss_clear(*cfg);

    cfg->managedVlans.clear_all();
    for (int v = VTSS_APPL_MVRP_VLAN_ID_MIN; v <= VTSS_APPL_MVRP_VLAN_ID_MAX; ++v) {
        cfg->managedVlans.set(v);
    }

    timers.join_timer      = VTSS_MRP_JOIN_TIMER_DEF;
    timers.leave_timer     = VTSS_MRP_LEAVE_TIMER_DEF;
    timers.leave_all_timer = VTSS_MRP_LEAVE_ALL_TIMER_DEF;
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
    (void)port_iter_init(&pit, &sit, VTSS_ISID_GLOBAL, PORT_ITER_SORT_ORDER_IPORT_ALL, PORT_ITER_FLAGS_ALL);
    while (port_iter_getnext(&pit)) {
        XXRP_stack_conf[VTSS_MRP_APPL_MVRP].timers[sit.isid - VTSS_ISID_START][pit.iport - VTSS_PORT_NO_START] = timers;
        vtss::mrp::mgmt_periodic_state_set(sit.isid, pit.iport, false);
    }
}

static void xxrp_conf_clear(void)
{
    vtss_mrp_appl_t appl;

    XXRP_PLATFORM_CRIT_ASSERT_LOCKED();
    for (appl = (vtss_mrp_appl_t)0; appl < VTSS_MRP_APPL_MAX; appl++) {
        if (appl == VTSS_MRP_APPL_MVRP) {
            mrp_conf_reset();
        } else {
            XXRP_conf_default(&XXRP_stack_conf[appl]);
        }
    }
}

/***************************************************************************************************
 * XXRP_msg_tx_local_conf_specific()
 * Transmit application specific local configuration to one or all switches.
 **************************************************************************************************/
static void XXRP_msg_tx_local_conf_specific(vtss_isid_t isid, vtss_mrp_appl_t appl)
{
    switch_iter_t sit;

    if (appl >=  VTSS_MRP_APPL_MAX) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Invalid appl = " << appl;
        return;
    }
    (void)switch_iter_init(&sit, isid, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        if (sit.isid - VTSS_ISID_START >= VTSS_ISID_CNT) {
            /* break out. Should not be necessary, but coverity check 
            fails and claims (correctly) that sit.isid might be too big 
            when used as index in port_enable-array.*/
            break;
        }
        xxrp_msg_local_conf_t *msg = (xxrp_msg_local_conf_t *)VTSS_MALLOC(sizeof(xxrp_msg_local_conf_t));
        if (msg == NULL) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Msg allocation failed";
            return;
        }
        msg->msg_id = XXRP_MSG_ID_LOCAL_CONF_SET;
        msg->appl   = appl;
        XXRP_PLATFORM_CRIT_ENTER();
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, NOISE) << "Sent conf for appl = " << appl
                                                        << ", global status = " << XXRP_stack_conf[msg->appl].global_enable;
        msg->switch_conf.global_enable = XXRP_stack_conf[msg->appl].global_enable;
        memcpy(msg->switch_conf.port_enable, XXRP_stack_conf[msg->appl].port_enable[sit.isid - VTSS_ISID_START], sizeof(msg->switch_conf.port_enable));
        XXRP_PLATFORM_CRIT_EXIT();
        msg_tx(VTSS_MODULE_ID_XXRP, sit.isid, msg, sizeof(*msg));
    }
}

/***************************************************************************************************
 * XXRP_msg_tx_local_conf()
 * Transmit all application specific local configurations to one or all switches.
 **************************************************************************************************/
static void XXRP_msg_tx_local_conf(vtss_isid_t isid)
{
    vtss_mrp_appl_t       appl;

    for (appl = (vtss_mrp_appl_t)0; appl < VTSS_MRP_APPL_MAX; appl++) {
        XXRP_msg_tx_local_conf_specific(isid, appl);
    }
}

/***************************************************************************************************
 * XXRP_msg_rx()
 **************************************************************************************************/
static BOOL XXRP_msg_rx(void *contxt, const void *const rx_msg, const size_t len, const vtss_module_id_t modid, const u32 isid)
{
    xxrp_msg_id_t msg_id = *(xxrp_msg_id_t *)rx_msg;

    switch (msg_id) {
    case XXRP_MSG_ID_LOCAL_CONF_SET: {
        xxrp_msg_local_conf_t *msg = (xxrp_msg_local_conf_t *)rx_msg;

        VTSS_ASSERT(msg->appl < VTSS_MRP_APPL_MAX);
        XXRP_PLATFORM_CRIT_ENTER();
        XXRP_local_conf[msg->appl].global_enable = msg->switch_conf.global_enable;
        memcpy(XXRP_local_conf[msg->appl].port_enable, msg->switch_conf.port_enable, VTSS_PORT_BF_SIZE);
        XXRP_forwarding_control();
        XXRP_PLATFORM_CRIT_EXIT();
        break;
    }

    default:
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, WARNING) << "Msg rx with unknown message ID = " << msg_id;
        break;
    }
    return TRUE;
}

/***************************************************************************************************
 * Register for XXRP messages
 **************************************************************************************************/
static void XXRP_msg_register(void)
{
    msg_rx_filter_t filter;
    mesa_rc         rc;

    memset(&filter, 0, sizeof(filter));
    filter.cb = XXRP_msg_rx;
    filter.modid = VTSS_MODULE_ID_XXRP;
    if ((rc = msg_rx_filter_register(&filter)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Failed to register for XXRP messages - " << error_txt(rc);
    }
}

/***************************************************************************************************
 * XXRP_packet_rx()
 * MRPDU packet reception on local switch.
 * Send it to the current primary switch via l2proto if MRP app is global enabled and enabled on port.
 **************************************************************************************************/
static BOOL XXRP_packet_rx(void *contxt, const uchar *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    vtss_mrp_appl_t appl;
    BOOL            send_to_primary_switch = FALSE, rc = FALSE; // Allow other subscribers to receive the packet

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "MRPDU rx " << (contxt ? "MRP" : "GARP")
            << " on port " << l2port2str(rx_info->port_no)
            << " from " << misc_mac2str(&frm[6]);
    if (rx_info->tag_type != MESA_TAG_TYPE_UNTAGGED) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, DEBUG) << "Tagged MRPDU received on port " << l2port2str(rx_info->port_no);
        return rc;
    }

    /* Lookup for the MRP application */
    for (appl = (vtss_mrp_appl_t)0; appl < VTSS_MRP_APPL_MAX; appl++) {
        if ((!contxt && XXRP_mrp_appl_parm[appl].etype) ||
                (contxt && !XXRP_mrp_appl_parm[appl].etype) ) {
            continue;
        }
        if (memcmp(&frm[0], XXRP_mrp_appl_parm[appl].dmac, 6)) {
            continue; // No match on DMAC
        }
        if (contxt) {
            if (((frm[12] << 8) + frm[13]) != XXRP_mrp_appl_parm[appl].etype) {
                continue; // No match on ETYPE
            }
        } else {
            if (memcmp(&frm[14], llc_header, sizeof(llc_header))) {
                continue; // No match on LLC header
            }
        }
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "Matching application found: " << appl;
        break;
    }
    if (appl >= VTSS_MRP_APPL_MAX) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, DEBUG) << "No matching application was found";
        return rc;
    }

    XXRP_PLATFORM_CRIT_ENTER();
    /* Check if app is enabled globally and on port */
    send_to_primary_switch = (XXRP_local_conf[appl].global_enable &&
                      VTSS_PORT_BF_GET(XXRP_local_conf[appl].port_enable, rx_info->port_no));
    XXRP_PLATFORM_CRIT_EXIT();

    if (send_to_primary_switch) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "MRPDU forward to primary_switch";
        l2_receive_indication(VTSS_MODULE_ID_XXRP, frm, rx_info->length, rx_info->port_no, rx_info->tag.vid, VTSS_GLAG_NO_NONE);
        return rc;
    }
    /* App is disabled - discard frame */
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "MRPDU discarded";
    return rc;
}

/***************************************************************************************************
 * XXRP_packet_register()
 * Register with the packet module for receiving XXRP packets (MRPDUs).
 * There are two different kind of registrations:
 * 1: Match on DMAC and ETYPE. Used for MRP apps which are always associated with an ETYPE.
 * 2: Match on DMAC only. Used for GARP apps which are always identified by a LLC header (42-42-03).
 * Registrations with an ETYPE has higher priority than registrations with a LLC header.
 * The same cb is used for both types of registrations and the context parameter is used to
 * distinguish them from each other (*contxt != 0 means with ETYPE).
 * Unregistration is not neccessary as MRPDUs are only delivered to the CPU if the specific MRP
 * application is globally enabled.
 **************************************************************************************************/
static void XXRP_packet_register(void)
{
    packet_rx_filter_t rx_filter;
    void               *rx_filter_id;
    vtss_mrp_appl_t    appl;
    mesa_rc            rc;

    packet_rx_filter_init(&rx_filter);
    rx_filter.modid = VTSS_MODULE_ID_XXRP;
    rx_filter.cb    = XXRP_packet_rx;

    for (appl = (vtss_mrp_appl_t)0; appl < VTSS_MRP_APPL_MAX; appl++) {
        memcpy(rx_filter.dmac, XXRP_mrp_appl_parm[appl].dmac, sizeof(rx_filter.dmac));
        if (XXRP_mrp_appl_parm[appl].etype) { // MRP application
            rx_filter.match  = PACKET_RX_FILTER_MATCH_DMAC | PACKET_RX_FILTER_MATCH_ETYPE;
            rx_filter.prio   = PACKET_RX_FILTER_PRIO_NORMAL;
            rx_filter.etype  = XXRP_mrp_appl_parm[appl].etype;
            rx_filter.contxt = (void *)1; // Any non NULL value will be ok here
        } else { // GARP application
            rx_filter.match  = PACKET_RX_FILTER_MATCH_DMAC;
            rx_filter.prio   = PACKET_RX_FILTER_PRIO_LOW;
            rx_filter.etype  = 0;
            rx_filter.contxt = NULL;
        }
        if ((rc = packet_rx_filter_register(&rx_filter, &rx_filter_id)) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Failed to register packet rx for XXRP packets - "
                    << error_txt(rc);
        }
    }
}

/*
 * XXRP_l2_rx()
 * MRPDU packet reception on primary switch from all the secondary switches
 * (including the primary switch itself).
 * Store it to the rx buffer, the rx_thread will fetch it later.
 */
static void XXRP_l2_rx(const void *packet, size_t len, mesa_vid_t vid,
                       l2_port_no_t l2port)
{
    u32 *buf;
    /* We reserve two extra u32 for total length and l2 port */
    size_t total_len = len + (3 * sizeof(u32));
    size_t delta;

    T_NG(VTSS_TRACE_XXRP_GRP_RX, "MRPDU rx on l2 port %u", l2port);
    /* Ensure that buffers, i.e. 'buf' will always be 4 byte aligned. */
    if ((delta = total_len % 4)) {
        total_len += 4 - delta;
    }

    XXRP_PLATFORM_CRIT_ENTER();
    if ((buf = (u32 *)vtss_bip_buffer_reserve(&XXRP_rx_buffer, total_len)) != NULL) {
        buf[0] = total_len;  // Save the total length first (in bytes)
        buf[1] = l2port;     // Then the l2 port it came from
        buf[2] = len;
        memcpy(&buf[3], packet, len);            // Then the data
        vtss_bip_buffer_commit(&XXRP_rx_buffer); // Tell it to the BIP buffer.
    }
    XXRP_PLATFORM_CRIT_EXIT();

    if (buf) {
        /* Set the frame reception flag in order to awake the rx thread. */
        vtss_flag_setbits(&XXRP_rx_thread_flag, XXRP_EVENT_FLAG_MRPDU);
    }
}

static const char *xxrp_mstp_port_state2txt(vtss_common_stpstate_t state)
{
    switch (state) {
    case VTSS_COMMON_STPSTATE_DISCARDING:
        return "Discarding";
    case VTSS_COMMON_STPSTATE_LEARNING:
        return "Learning";
    case VTSS_COMMON_STPSTATE_FORWARDING:
        return "Forwarding";
    default:
        return "Invalid";
    }
}

/***************************************************************************************************
 * Callback function for mstp port state changes.
 **************************************************************************************************/
static void XXRP_mstp_state_change(vtss_common_port_t l2port, uchar msti, vtss_common_stpstate_t new_state)
{
    xxrp_mstp_port_conf_t *conf;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MSTP state of l2 port =" << l2port
            << ", msti =" << msti
            << " changed to " << xxrp_mstp_port_state2txt(new_state);
    if (!l2port_is_port(l2port)) {
        /* XXRP does not work with aggregated ports at the moment, so no need to update port states of aggregated ports */
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MSTP port state change on aggregation, notification is ignored";
        return;
    }

    if ((new_state == MESA_STP_STATE_FORWARDING) || (new_state == MESA_STP_STATE_DISCARDING)) {
        XXRP_MSTP_PLATFORM_CRIT_ENTER();
        conf = &xxrp_mstp_port_conf[l2port][msti];
        conf->port_state_changed = TRUE;
        conf->port_state = ((new_state == MESA_STP_STATE_FORWARDING) ? XXRP_MSTP_PORT_STATE_FORWARDING : XXRP_MSTP_PORT_STATE_DISCARDING);
        vtss_flag_setbits(&XXRP_rx_thread_flag, XXRP_EVENT_FLAG_MSTP_PORT_STATE_CHANGE);
        XXRP_MSTP_PLATFORM_CRIT_EXIT();
    }
}

/***************************************************************************************************
 * XXRP_mstp_state_change_handler()
 * Callback function for mstp state changes.
 **************************************************************************************************/
static void XXRP_mstp_state_change_handler(void)
{
    mesa_rc                                brc;
    vtss_mrp_mstp_port_state_change_type_t new_state;
    vtss_common_port_t                     l2port;
    u8                                     msti;
    BOOL                                   state_changed = FALSE;

    for (l2port = 0; l2port < L2_MAX_PORTS_; l2port++) {
        for (msti = 0; msti < N_L2_MSTI_MAX; msti++) {
            XXRP_MSTP_PLATFORM_CRIT_ENTER();

            state_changed = xxrp_mstp_port_conf[l2port][msti].port_state_changed;
            if (state_changed) {
                xxrp_mstp_port_conf[l2port][msti].port_state_changed = FALSE;
            }

            new_state = ((xxrp_mstp_port_conf[l2port][msti].port_state == XXRP_MSTP_PORT_STATE_FORWARDING) ?
                         VTSS_MRP_MSTP_PORT_ADD : VTSS_MRP_MSTP_PORT_DELETE);

            XXRP_MSTP_PLATFORM_CRIT_EXIT();

            if (state_changed == TRUE) {
                if ((brc = vtss_xxrp_mstp_port_state_change_handler(l2port, msti, new_state)) != VTSS_RC_OK) {
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Unable to set MSTP state for port "
                            << l2port << " - " << error_txt(brc);
                }
            }
        }
    }
}

static const char *const xxrp_mstp_port_role2txt(vtss_mstp_portrole_t role)
{
    switch (role) {
    case VTSS_MSTP_PORTROLE_DESIGNATEDPORT:
        return "Designated";
    case VTSS_MSTP_PORTROLE_ROOTPORT:
        return "Root";
    case VTSS_MSTP_PORTROLE_ALTERNATEPORT:
        return "Alternate";
    default:
        return "Invalid";
    }
}

/* Callback function for mstp port role changes */
/* Called by MSTP with lock taken, so don't process it right away
   Leave that to the RX_thread later on, just set up a flag for it */
#ifdef VTSS_SW_OPTION_MRP
void vtss_mstp_port_setrole(uint portnum,
                            u8 msti,
                            vtss_mstp_portrole_t old_role,
                            vtss_mstp_portrole_t new_role)
{
    xxrp_mstp_port_conf_t *conf;

    if (!l2port_is_port(portnum)) {
        /* XXRP does not work with aggregated ports at the moment, so no need to update port states of aggregated ports */
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, INFO) << "MSTP port role change on aggregation, notification is ignored";
        return;
    }

    /* IEEE 802.1Q-2014 10.7.5.2
       If port goes from Root or Alternate to Designated
       then a Flush! event must be triggered */
    if ((new_role == VTSS_MSTP_PORTROLE_DESIGNATEDPORT) &&
       ((old_role == VTSS_MSTP_PORTROLE_ROOTPORT) ||
       (old_role == VTSS_MSTP_PORTROLE_ALTERNATEPORT))) {
        XXRP_MSTP_PLATFORM_CRIT_ENTER();
        conf = &xxrp_mstp_port_conf[portnum][msti];
        conf->port_role = XXRP_MSTP_PORT_FLUSH;
        vtss_flag_setbits(&XXRP_rx_thread_flag, XXRP_EVENT_FLAG_MSTP_PORT_ROLE_CHANGE);
        XXRP_MSTP_PLATFORM_CRIT_EXIT();
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MSTP role of port "
                                                    << portnum << " changed to "
                                                    << xxrp_mstp_port_role2txt(new_role)
                                                    << " for MSTI " << msti;
        return;
    }

    /* IEEE 802.1Q-2014 10.7.5.3
       If port goes from Designated to either Root or Alternate
       then a Re-Declare! event must be triggered */
    if ((old_role == VTSS_MSTP_PORTROLE_DESIGNATEDPORT) &&
       ((new_role == VTSS_MSTP_PORTROLE_ROOTPORT) ||
       (new_role == VTSS_MSTP_PORTROLE_ALTERNATEPORT))) {
        XXRP_MSTP_PLATFORM_CRIT_ENTER();
        conf = &xxrp_mstp_port_conf[portnum][msti];
        conf->port_role = XXRP_MSTP_PORT_REDECLARE;
        vtss_flag_setbits(&XXRP_rx_thread_flag, XXRP_EVENT_FLAG_MSTP_PORT_ROLE_CHANGE);
        XXRP_MSTP_PLATFORM_CRIT_EXIT();
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MSTP role of port "
                                                    << portnum << " changed to "
                                                    << xxrp_mstp_port_role2txt(new_role)
                                                    << " for MSTI " << msti;
        return;
    }
}
#endif /* VTSS_SW_OPTION_MRP */

/* XXRP_mstp_port_role_change_handler()
   Handler for mstp port role changes.
   Called by the RX_thread after been awaken by the proper flag */
static void XXRP_mstp_port_role_change_handler(void)
{
    u32 l2port = 0;
    u8  msti = 0;

    for (l2port = 0; l2port < L2_MAX_PORTS_; l2port++) {
        for (msti = 0; msti < N_L2_MSTI_MAX; msti++) {
            XXRP_MSTP_PLATFORM_CRIT_ENTER();
            switch (xxrp_mstp_port_conf[l2port][msti].port_role) {
            case XXRP_MSTP_PORT_NOTHING:
                XXRP_MSTP_PLATFORM_CRIT_EXIT();
                continue;
            case XXRP_MSTP_PORT_FLUSH:
                xxrp_mstp_port_conf[l2port][msti].port_role = XXRP_MSTP_PORT_NOTHING;
                XXRP_MSTP_PLATFORM_CRIT_EXIT();
                vtss::mrp::handle_port_role_change(l2port, msti, vtss::mrp::MRP_DESIGNATED);
                break;
            case XXRP_MSTP_PORT_REDECLARE:
                xxrp_mstp_port_conf[l2port][msti].port_role = XXRP_MSTP_PORT_NOTHING;
                XXRP_MSTP_PLATFORM_CRIT_EXIT();
                vtss::mrp::handle_port_role_change(l2port, msti,
                                                   vtss::mrp::MRP_ROOT_ALTERNATE);
                break;
            default:
                XXRP_MSTP_PLATFORM_CRIT_EXIT();
                break;
            }
        }
    }
}

static mesa_rc XXRP_vlan_port_membership_add(u32 port, mesa_vid_t vid, vtss_appl_vlan_user_t vlan_user)
{
    vtss_isid_t            isid;
    mesa_port_no_t         iport_no;
    vtss_appl_vlan_entry_t vlan_mgmt_entry;

    vtss_clear(vlan_mgmt_entry);
    if (l2port2port(port, &isid, &iport_no) != TRUE) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "l2port2port conversion failed";
        return VTSS_RC_ERROR;
    }
    if (vtss_appl_vlan_get(isid, vid, &vlan_mgmt_entry, FALSE, vlan_user) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Failed to fetch conf from VLAN module";
    }
    vlan_mgmt_entry.vid = vid;
    vlan_mgmt_entry.ports[iport_no] = 1;
    if (vlan_mgmt_vlan_add(isid, &vlan_mgmt_entry, vlan_user) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Failed to add conf through VLAN module";
        return VTSS_RC_OK;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Added VLAN conf successfully";
    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_MVRP
mesa_rc XXRP_mvrp_vlan_port_membership_add(u32 port, mesa_vid_t vid)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MVRP registering VID " << vid
            << " on l2port " << l2port2str(port);
    return XXRP_vlan_port_membership_add(port, vid, VTSS_APPL_VLAN_USER_MVRP);
}
#endif

static u32  XXRP_vlan_port_membership_del(u32 port, mesa_vid_t vid, vtss_appl_vlan_user_t vlan_user)
{
    vtss_isid_t            isid;
    mesa_port_no_t         iport_no;
    vtss_appl_vlan_entry_t vlan_mgmt_entry;
    static const mesa_port_list_t empty_ports = {0}; /* This will initialize empty_ports array to zeros */

    vtss_clear(vlan_mgmt_entry);
    if (l2port2port(port, &isid, &iport_no) != TRUE) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "l2port2port conversion failed";
        return FALSE;
    }
    if (vtss_appl_vlan_get(isid, vid, &vlan_mgmt_entry, FALSE, vlan_user) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Failed to fetch conf from VLAN module";
    }
    vlan_mgmt_entry.vid = vid;              /* VLAN ID   */
    vlan_mgmt_entry.ports[iport_no] = 0;    /* Port mask */
    if (!memcmp(vlan_mgmt_entry.ports, empty_ports, sizeof(empty_ports))) {
        if (vlan_mgmt_vlan_del(isid, vid, vlan_user) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Failed to remove conf through VLAN module";
        }
    } else {
        if (vlan_mgmt_vlan_add(isid, &vlan_mgmt_entry, vlan_user) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Failed to add conf through VLAN module";
        }
    }
    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_MVRP
mesa_rc XXRP_mvrp_vlan_port_membership_del(u32 port, mesa_vid_t vid)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MVRP de-registering VID " << vid
            << " on l2port " << l2port2str(port);
    return XXRP_vlan_port_membership_del(port, vid, VTSS_APPL_VLAN_USER_MVRP);
}
#endif

#ifdef VTSS_SW_OPTION_GVRP
void XXRP_gvrp_vlan_port_membership_change(void)
{
    vtss_flag_setbits(&XXRP_rx_thread_flag, XXRP_EVENT_FLAG_VLAN_MEMBER_CHANGE);
}
#endif

/* Callback function for vlan module */
static void XXRP_vlan_membership_change_callback(vtss_isid_t isid, mesa_vid_t vid, vlan_membership_change_t *changes)
{
    port_iter_t pit;

    XXRP_BASE_CRIT_ENTER();

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        vlan_registration_type_t t;
        u32                      l2port = L2PORT2PORT(isid, pit.iport);

        if (!changes->changed_ports.ports[pit.iport]) {
            continue;
        }

        if (changes->forbidden_ports.ports[pit.iport]) {
            t = VLAN_REGISTRATION_TYPE_FORBIDDEN;
        } else {
            t = changes->static_ports.ports[pit.iport] ? VLAN_REGISTRATION_TYPE_FIXED : VLAN_REGISTRATION_TYPE_NORMAL;
        }

#ifdef VTSS_SW_OPTION_MVRP
        (void)vtss::mrp::mvrp_handle_vlan_change(l2port, vid, t);
#endif
#ifdef VTSS_SW_OPTION_GVRP
        (void)vtss_gvrp_registrar_administrative_control(l2port, vid, t);
#endif
    }

    XXRP_BASE_CRIT_EXIT();
}

BOOL XXRP_mvrp_is_vlan_registered(u32 l2port, mesa_vid_t vid)
{
    vtss_isid_t            isid;
    mesa_port_no_t         iport;
    vtss_appl_vlan_entry_t vlan_conf;
    u8                     access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    mesa_rc                rc;

    if (l2port2port(l2port, &isid, &iport) != TRUE) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "l2port2port conversion failed";
        return FALSE;
    }
    if ((rc = vtss_appl_vlan_access_vids_get(access_vids)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Huh?" << error_txt(rc);
        return FALSE;
    }
    if (!VTSS_BF_GET(access_vids, vid)) {
        return FALSE;
    }
    if (vtss_appl_vlan_get(isid, vid, &vlan_conf, FALSE, VTSS_APPL_VLAN_USER_STATIC) == VTSS_RC_OK) {
        if (vlan_conf.ports[iport] == 1) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, NOISE) << "VID " << vid << " is registered on switch = "
                    << isid << " and port = " << iport;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL XXRP_is_port_point2point(u32 l2port)
{
    vtss_isid_t           isid;
    mesa_port_no_t        iport_no;
    vtss_appl_port_conf_t conf;
    vtss_ifindex_t        ifindex;

    if (l2port2port(l2port, &isid, &iport_no) != TRUE) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "l2port2port conversion failed";
        return FALSE;
    }
    if (vtss_ifindex_from_port(isid, iport_no, &ifindex) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "port to ifindex conversion failed";
        return FALSE;
    };
    if (vtss_appl_port_conf_get(ifindex, &conf) == VTSS_RC_OK) {
        if (conf.fdx == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

/***************************************************************************************************
 * XXRP_port_state_change()
 * Callback function for port state changes.
 * This is how we get the point-to-point (full duplex) state of a port.
 * Update the base module if the port is enabled.
 **************************************************************************************************/
static void XXRP_port_state_change(mesa_port_no_t iport, const vtss_appl_port_status_t *status)
{
    vtss_isid_t isid = VTSS_ISID_START;

    u32 l2port = L2PORT2PORT(isid, iport);

    if (status->link) {
        XXRP_PLATFORM_CRIT_ENTER();
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Port state of l2 port "
                << l2port << " changed to "
                << (status->fdx ? "p2p" : "shared");
        VTSS_PORT_BF_SET(XXRP_p2p_cache[isid - VTSS_ISID_START], iport, status->fdx);
        XXRP_PLATFORM_CRIT_EXIT();
    }
}

/***************************************************************************************************
 * XXRP_tm_thread()
 **************************************************************************************************/
static void XXRP_tm_thread(vtss_addrword_t data)
{
    for (;;) {
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_XXRP);
        if (msg_switch_is_primary()) {
            // Initialize primary switch state
            vtss_tick_count_t wakeup_prev = 0;
            vtss_tick_count_t wakeup_now  = 0;
            vtss_tick_count_t wakeup_next = 0; // 0 == no timeout
            uint              delay = 0;
            vtss_flag_value_t flags;

            while (msg_switch_is_primary()) {
                // Process while being primary switch
                if ((flags = XXRP_FLAG_WAIT(&XXRP_tm_thread_flag, wakeup_next))) {
                    if (flags & XXRP_EVENT_FLAG_KICK) {
                        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Received XXRP_EVENT_FLAG_KICK";
                        wakeup_now = vtss_current_time();
                        delay = wakeup_next ?  wakeup_now - wakeup_prev : 0;
                        if ((delay = vtss_xxrp_timer_tick(delay))) {
                            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Timer restarted, delay = " << delay;
                            wakeup_prev = wakeup_now;
                            wakeup_next = vtss_current_time() + delay; // Restart timer with new value
                        } else {
                            wakeup_next = 0; // Stop timer
                        }
                    }
                } else { // Timer expired
                    if ((delay = vtss_xxrp_timer_tick(delay))) {
                        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, NOISE) << "Timer restarted, delay = " << delay;
                        wakeup_prev = wakeup_next;
                        wakeup_next = vtss_current_time() + delay; // Restart timer with new value
                    } else {
                        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, NOISE) << "Timer stopped";
                        wakeup_next = 0; // Stop timer
                    }
                }
            }
        }
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Suspending XXRP timer thread (became secondary switch)";
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_XXRP);
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Restarting XXRP timer thread (became primary switch)";
    }
}

/***************************************************************************************************
 * XXRP_rx_thread()
 * Process received MRPDUs.
 **************************************************************************************************/
static void XXRP_rx_thread(vtss_addrword_t data)
{
    for (;;) {
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_XXRP);
        if (msg_switch_is_primary()) {
            // Initialize primary switch state
            vtss_tick_count_t wakeup_next = 0; // 0 == no timeout
            XXRP_PLATFORM_CRIT_ENTER();
            vtss_bip_buffer_clear(&XXRP_rx_buffer); // Start with a cleared buffer
            XXRP_PLATFORM_CRIT_EXIT();

            while (msg_switch_is_primary()) {
                vtss_flag_value_t flags;
                // Process while being primary switch
                if ((flags = XXRP_FLAG_WAIT(&XXRP_rx_thread_flag, wakeup_next))) {
                    if (flags & XXRP_EVENT_FLAG_MRPDU) {
                        u32 *buf;
                        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Received XXRP_EVENT_FLAG_MRPDU";
                        do { // Process all received MRPDUs
                            int contiguous_block_size;
                            XXRP_PLATFORM_CRIT_ENTER();
                            buf = (u32 *)vtss_bip_buffer_get_contiguous_block(&XXRP_rx_buffer, &contiguous_block_size);
                            XXRP_PLATFORM_CRIT_EXIT();
                            if (buf) {
                                u32 total_len = buf[0]; // First dword is the buffer length in bytes including the length and the l2 port fields themselves.
                                u32 l2port    = buf[1]; // Second dword is the l2 port and the MRPDU starts in 4th dword
                                u32 pdu_len   = buf[2];
                                if ((total_len <= (3 * sizeof(u32))) || (total_len > (u32)contiguous_block_size)) {
                                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR)
                                            << "Invalid RX buffer entry total_len="
                                            << total_len << ", contiguous_block_size="
                                            << contiguous_block_size;
                                }
                                (void)vtss_mrp_mrpdu_rx(l2port, (u8 *)&buf[3], pdu_len);
                                XXRP_PLATFORM_CRIT_ENTER();
                                vtss_bip_buffer_decommit_block(&XXRP_rx_buffer, total_len);
                                XXRP_PLATFORM_CRIT_EXIT();
                            }
                        } while (buf);
                    }

                    if (flags & XXRP_EVENT_FLAG_MSTP_PORT_STATE_CHANGE) {
                        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Received XXRP_EVENT_FLAG_MSTP_PORT_STATE_CHANGE";
                        XXRP_mstp_state_change_handler();
                    }

                    if (flags & XXRP_EVENT_FLAG_MSTP_PORT_ROLE_CHANGE) {
                        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Received XXRP_EVENT_FLAG_MSTP_PORT_ROLE_CHANGE";
                        XXRP_mstp_port_role_change_handler();
                    }

                    if (flags & XXRP_EVENT_FLAG_VLAN_2_MSTI_MAP_CHANGE) {
#ifdef VTSS_SW_OPTION_GVRP
                        vtss_gvrp_update_vlan_to_msti_mapping();
#endif
                        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Received XXRP_EVENT_FLAG_VLAN_2_MSTI_MAP_CHANGE";
                    }


                    if (flags & XXRP_EVENT_FLAG_VLAN_MEMBER_CHANGE) {
#ifdef VTSS_SW_OPTION_GVRP
                        u32 port;
                        mesa_vid_t vid;
                        int rc;

                        for (;;) {
                            XXRP_BASE_CRIT_ENTER();
                            rc = vtss_gvrp_vlan_membership_getnext_add(&port, &vid);
                            XXRP_BASE_CRIT_EXIT();

                            if (rc) {
                                break;
                            }
                            T_N("Port %d: VID %d was removed from the add list", port, vid);
                            XXRP_vlan_port_membership_add(port, vid, VTSS_APPL_VLAN_USER_GVRP);
                        }

                        for (;;) {
                            XXRP_BASE_CRIT_ENTER();
                            rc = vtss_gvrp_vlan_membership_getnext_del(&port, &vid);
                            XXRP_BASE_CRIT_EXIT();

                            if (rc) {
                                break;
                            }
                            T_N("Port %d: VID %d was removed from the del list", port, vid);
                            XXRP_vlan_port_membership_del(port, vid, VTSS_APPL_VLAN_USER_GVRP);
                        }
#endif
                    }
                }
            }
        }
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Suspending XXRP rx thread (became secondary switch)";
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_XXRP);
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Restarting XXRP rx thread (became primary switch)";
    }
}

static mesa_rc XXRP_check_isid_port(vtss_isid_t isid, mesa_port_no_t port, BOOL allow_local)
{
    if ((isid != VTSS_ISID_LOCAL && !VTSS_ISID_LEGAL(isid)) || (isid == VTSS_ISID_LOCAL && !allow_local)) {
        return XXRP_ERROR_ISID;
    }

    if (isid != VTSS_ISID_LOCAL && !msg_switch_is_primary()) {
        return XXRP_ERROR_NOT_PRIMARY_SWITCH;
    }

    if (port >= port_count_max()) {
        return XXRP_ERROR_PORT;
    }

    return VTSS_RC_OK;
}

/***************************************************************************************************
 *
 * PUBLIC FUNCTIONS
 *
 **************************************************************************************************/

/***************************************************************************************************
 * Callout function implementations
 **************************************************************************************************/

/* Called when the base module needs to (re)start the timer. */
void vtss_mrp_timer_kick(void)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Kick timer";
    vtss_flag_setbits(&XXRP_tm_thread_flag, XXRP_EVENT_FLAG_KICK);
}

void vtss_mrp_crit_enter()
{
    XXRP_BASE_CRIT_ENTER();
}

void vtss_mrp_crit_exit()
{
    XXRP_BASE_CRIT_EXIT();
}

void vtss_mrp_crit_assert()
{
    XXRP_BASE_CRIT_ASSERT();
}

/***************************************************************************************************
 * Called when the base module wants to get the port msti status.
 **************************************************************************************************/
mesa_rc mrp_mstp_port_status_get(u8 msti, u32 l2port)
{
    mstp_port_mgmt_status_t ps;
    mesa_rc                 rc;

    memset(&ps, 0, sizeof(mstp_port_mgmt_status_t));
    if (mstp_get_port_status(msti, l2port, &ps)
            && ps.active
            && (strncmp(ps.core.statestr, "Forwarding", sizeof("Forwarding")) == 0)) {
        rc = VTSS_RC_OK;
    } else {
        rc = VTSS_RC_ERROR;
    }

    return rc;
}

/* attr_index    :  attribute index. Actually VID.  */
/* msti     :  msti information                     */
/* Retrieves the msti information for a given VLAN. */
mesa_rc mrp_mstp_index_msti_mapping_get(u32 attr_index, u8 *msti)
{
    mstp_msti_config_t msti_config;
    mesa_rc            rc;

    if ((rc = vtss_appl_mstp_msti_config_get(&msti_config, NULL)) == VTSS_RC_OK) {
        *msti = msti_config.map.map[attr_index];
    } else {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Unable to retrieve vlan -> msti mapping";
    }
    return rc;
}

static mesa_rc vtss_xxrp_port_mac_conf_get(u32 l2port, u8 *port_mac_addr)
{
    mesa_rc        rc = VTSS_RC_OK;
    vtss_isid_t    isid;
    mesa_port_no_t iport;
    u8             sys_mac_addr[VTSS_XXRP_MAC_ADDR_LEN];

    if (port_mac_addr == NULL) {
        return XXRP_ERROR_VALUE;
    }
    if (l2port2port(l2port, &isid, &iport) == FALSE) {
        return XXRP_ERROR_VALUE;
    }
    //Get the system MAC address
    (void)conf_mgmt_mac_addr_get(sys_mac_addr, 0);
    misc_instantiate_mac(port_mac_addr, sys_mac_addr, iport + 1 - VTSS_PORT_NO_START);

    return rc;
}

/***************************************************************************************************
 * vtss_mrp_mrpdu_tx_alloc()
 * Called when the base module wants to allocate a transmit buffer.
 **************************************************************************************************/
void *vtss_mrp_mrpdu_tx_alloc(u32 port_no, u32 length)
{
    void *p;
    p = vtss_os_alloc_xmit(port_no, length);
    return p;
}

void dump_packet(int len, const u8 *p)
{
    int i;
#define BLINE 14
    for (i = 0; i < (len < BLINE ? len : BLINE); ++i) {
        printf("%2.2x ", p[i]);
    }

    for (i = BLINE; i < len; ++i) {
        if ( (i - BLINE) % 16 == 0 ) {
            printf("\n");
        }
        printf("%2.2x ", p[i]);
    }
    printf("\n");
}

/***************************************************************************************************
 * vtss_mrp_mrpdu_tx()
 * Called when the base module wants to transmit an MRPDU.
 * The SMAC address is inserted.
 **************************************************************************************************/
mesa_rc vtss_mrp_mrpdu_tx(u32 l2port, void *mrpdu, u32 length)
{
    u8 port_mac[VTSS_XXRP_MAC_ADDR_LEN];

    /* fill the source MAC address in the MVRPDU */
    (void)vtss_xxrp_port_mac_conf_get(l2port, port_mac);
    memcpy(((u8 *)mrpdu + VTSS_XXRP_MAC_ADDR_LEN), port_mac, sizeof(port_mac));
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, DEBUG) << "Transmitting a " << length << " byte long MVRPDU on port " << l2port2str(l2port);
    return vtss_os_xmit(l2port, mrpdu, length);
}

const char *xxrp_error_txt(mesa_rc rc)
{
    switch (rc) {
    case XXRP_ERROR_INVALID_PARAMETER:
        return "Invalid parameter";
    case XXRP_ERROR_INVALID_APPL:
        return "XXRP Error - Invalid application id provided";
    case XXRP_ERROR_INVALID_L2PORT:
        return "XXRP Error - Invalid l2port number provided";
    case XXRP_ERROR_INVALID_ATTR:
        return "XXRP Error - Invalid MRP attribute type provided";
    case XXRP_ERROR_NOT_ENABLED:
        return "XXRP Error - MRP/GARP application is not enabled globally";
    case XXRP_ERROR_NOT_ENABLED_PORT:
        return "XXRP Error - MRP/GARP application is not enabled on this port";
    case XXRP_ERROR_NULL_PTR:
        return "XXRP Error - Function called with null ptr";
    case XXRP_ERROR_ALREADY_CONFIGURED:
        return "Already configured";
    case XXRP_ERROR_NO_MEMORY:
        return "No memory";
    case XXRP_ERROR_NOT_SUPPORTED:
        return "Not supported";
    case XXRP_ERROR_NO_SUFFIFIENT_MEMORY:
        return "No available memory";
    case XXRP_ERROR_NOT_FOUND:
        return "Not found";
    case XXRP_ERROR_UNKNOWN:
        return "Unknown";
    case XXRP_ERROR_ISID:
        return "Invalid Switch ID";
    case XXRP_ERROR_PORT:
        return "Invalid port number";
    case XXRP_ERROR_FLASH:
        return "Could not store configuration in flash";
    case XXRP_ERROR_NOT_PRIMARY_SWITCH:
        return "Switch must be primary switch";
    case XXRP_ERROR_VALUE:
        return "Invalid value";
    case XXRP_ERROR_APPL_OVERLAP:
        return "Another MRP/GARP application is currently"
               " enabled - disable it first";
    case XXRP_ERROR_APPL_ENABLED_ALREADY:
        return "The GARP application is currently"
               " enabled - disable it in order to configure"
               " its parameters.";
    case XXRP_ERROR_INVALID_IF_TYPE:
        return "XXRP Error - The provided interface type was invalid"
               " - expected switch port";
    case XXRP_ERROR_INVALID_TIMER:
        return "XXRP Error - One of the provided timer values was invalid";
    case XXRP_ERROR_INVALID_VID:
        return "XXRP Error - The provided VLAN ID was invalid"
               " - valid range: [1, 4094]";
    default:
        return "";
    }
}

mesa_rc xxrp_mgmt_appl_exclusion(vtss_mrp_appl_t appl)
{
    int i;

    for (i = 0; i < VTSS_MRP_APPL_MAX; ++i) {
        if (i == appl) {
            continue;
        } else {
            if (XXRP_stack_conf[i].global_enable) {
                return XXRP_ERROR_APPL_OVERLAP;
            }
        }
    }
    return VTSS_RC_OK;
}

mesa_rc xxrp_mgmt_global_enabled_get(vtss_mrp_appl_t appl, BOOL *enable)
{
    if (appl >= VTSS_MRP_APPL_MAX) {
        return XXRP_ERROR_INVALID_APPL;
    }

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MGMT API fetching global status of MRP application";
    XXRP_PLATFORM_CRIT_ENTER();
    if (msg_switch_is_primary()) {
        *enable = XXRP_stack_conf[appl].global_enable;
    } else {
        *enable = XXRP_local_conf[appl].global_enable;
    }
    XXRP_PLATFORM_CRIT_EXIT();

    return VTSS_RC_OK;
}

mesa_rc xxrp_mgmt_global_enabled_set(vtss_mrp_appl_t appl, BOOL enable)
{
    BOOL changed;

    if (appl >= VTSS_MRP_APPL_MAX || appl < 0) {
        return XXRP_ERROR_INVALID_APPL;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MGMT API "
            << (enable ? "enabling" : "disabling")
            << " MRP application globally";
    XXRP_PLATFORM_CRIT_ENTER();
    if ((changed = (XXRP_stack_conf[appl].global_enable != enable))) {
        XXRP_stack_conf[appl].global_enable = enable;
        XXRP_base_switch_sync_specific(VTSS_ISID_GLOBAL, appl);
    }
    XXRP_PLATFORM_CRIT_EXIT();

    if (changed) {
        XXRP_msg_tx_local_conf_specific(VTSS_ISID_GLOBAL, appl); // Send configuration to all existing switches
    }

    return VTSS_RC_OK;
}

mesa_rc xxrp_mgmt_enabled_get(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, BOOL *enable)
{
    VTSS_RC(XXRP_check_isid_port(isid, iport, TRUE));
    if (appl >= VTSS_MRP_APPL_MAX) {
        return XXRP_ERROR_INVALID_APPL;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MGMT API fetching status of MRP application on port = " << iport2uport(iport);
    XXRP_PLATFORM_CRIT_ENTER();
    if (isid != VTSS_ISID_LOCAL) {
        *enable = VTSS_PORT_BF_GET(XXRP_stack_conf[appl].port_enable[isid - VTSS_ISID_START], iport);
    } else {
        *enable = VTSS_PORT_BF_GET(XXRP_local_conf[appl].port_enable, iport);
    }
    XXRP_PLATFORM_CRIT_EXIT();

    return VTSS_RC_OK;
}

mesa_rc xxrp_mgmt_enabled_set(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, BOOL enable)
{
    BOOL changed;

    VTSS_RC(XXRP_check_isid_port(isid, iport, FALSE));
    if (appl >= VTSS_MRP_APPL_MAX) {
        return XXRP_ERROR_INVALID_APPL;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MGMT API enabling MRP application on port  = " << iport2uport(iport);
    XXRP_PLATFORM_CRIT_ENTER();
    if ((changed = (VTSS_PORT_BF_GET(XXRP_stack_conf[appl].port_enable[isid - VTSS_ISID_START], iport) != enable))) {
        VTSS_PORT_BF_SET(XXRP_stack_conf[appl].port_enable[isid - VTSS_ISID_START], iport, enable);
        if (XXRP_stack_conf[appl].global_enable) {
            XXRP_base_port_sync_specific(isid, iport, appl);
        }
    }
    XXRP_PLATFORM_CRIT_EXIT();
    if (changed) {
        XXRP_msg_tx_local_conf_specific(VTSS_ISID_GLOBAL, appl); // Send configuration to all existing switches
    }

    return VTSS_RC_OK;
}

#if 0
/* Not used at the moment */
mesa_rc xxrp_mgmt_periodic_tx_get(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, BOOL *enable)
{
    VTSS_RC(XXRP_check_isid_port(isid, iport, FALSE));
    if (appl >= VTSS_MRP_APPL_MAX) {
        return XXRP_ERROR_INVALID_APPL;
    }
    XXRP_PLATFORM_CRIT_ENTER();
    *enable = VTSS_PORT_BF_GET(XXRP_stack_conf[appl].periodic_tx[isid - VTSS_ISID_START], iport);
    XXRP_PLATFORM_CRIT_EXIT();

    return VTSS_RC_OK;
}

/* Not used at the moment */
mesa_rc xxrp_mgmt_periodic_tx_set(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, BOOL enable)
{
    VTSS_RC(XXRP_check_isid_port(isid, iport, FALSE));

    if (appl >= VTSS_MRP_APPL_MAX) {
        return XXRP_ERROR_INVLALID_APPL;
    }
    XXRP_PLATFORM_CRIT_ENTER();
    if (VTSS_PORT_BF_GET(XXRP_stack_conf[appl].periodic_tx[isid - VTSS_ISID_START], iport) != enable) {
        VTSS_PORT_BF_SET(XXRP_stack_conf[appl].periodic_tx[isid - VTSS_ISID_START], iport, enable);
        XXRP_base_port_periodic_tx_set_specific(isid, iport, appl, TRUE);
    }
    XXRP_PLATFORM_CRIT_EXIT();

    return VTSS_RC_OK;
}
#endif

mesa_rc xxrp_mgmt_global_timers_get(vtss_mrp_appl_t appl, vtss_mrp_timer_conf_t *timers)
{
    if (appl >= VTSS_MRP_APPL_MAX) {
        return XXRP_ERROR_INVALID_APPL;
    }
    XXRP_PLATFORM_CRIT_ENTER();
    *timers = XXRP_stack_conf[appl].global_timers;
    XXRP_PLATFORM_CRIT_EXIT();

    return VTSS_RC_OK;
}

mesa_rc xxrp_mgmt_global_timers_set(vtss_mrp_appl_t appl, const vtss_mrp_timer_conf_t *const timers)
{
    if (appl >= VTSS_MRP_APPL_MAX) {
        return XXRP_ERROR_INVALID_APPL;
    }
    XXRP_PLATFORM_CRIT_ENTER();
    XXRP_stack_conf[appl].global_timers = *timers;
    /*(void)vtss::mrp::global_timers_set((vtss::milliseconds) (timers->join_timer * 10),
                                       (vtss::milliseconds) (timers->leave_timer * 10),
                                       (vtss::milliseconds) (timers->leave_all_timer * 10));
                                       */
    XXRP_PLATFORM_CRIT_EXIT();

    return VTSS_RC_OK;
}

mesa_rc xxrp_mgmt_timers_check(vtss_mrp_appl_t appl, const vtss_mrp_timer_conf_t *const timers)
{
    if (appl >= VTSS_MRP_APPL_MAX) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if ((timers->join_timer < VTSS_MRP_JOIN_TIMER_MIN) || (timers->join_timer > VTSS_MRP_JOIN_TIMER_MAX)) {
        return XXRP_ERROR_INVALID_TIMER;
    }
    if ((timers->leave_timer < VTSS_MRP_LEAVE_TIMER_MIN) || (timers->leave_timer > VTSS_MRP_LEAVE_TIMER_MAX)) {
        return XXRP_ERROR_INVALID_TIMER;
    }
    if ((timers->leave_all_timer < VTSS_MRP_LEAVE_ALL_TIMER_MIN) || (timers->leave_all_timer > VTSS_MRP_LEAVE_ALL_TIMER_MAX)) {
        return XXRP_ERROR_INVALID_TIMER;
    }
    return VTSS_RC_OK;
}

/* Taken from vlan_mgmt_vid_bitmask_to_txt() */
char *xxrp_mgmt_vlan_list_to_txt(vtss::VlanList &vls, char *txt)
{
    mesa_vid_t vid;
    BOOL       member, first = TRUE;
    u32        count = 0;
    char       *p = txt;

    txt[0] = '\0';

    for (vid = VTSS_APPL_MVRP_VLAN_ID_MIN; vid <= VTSS_APPL_MVRP_VLAN_ID_MAX; vid++) {
        member = vls.get(vid);

        if ((member && (count == 0 || vid == VTSS_APPL_MVRP_VLAN_ID_MAX)) || (!member && count > 1)) {
            p += sprintf(p, "%s%d",
                         first ? "" : count > (member ? 1 : 2) ? "-" : ",",
                         member ? vid : vid - 1);
            first = FALSE;
        }

        count = member ? count + 1 : 0;
    }

    return txt;
}

/* Convert text to list or bit field */
mesa_rc xxrp_mgmt_txt_to_vlan_list(char *txt, vtss::VlanList &vls,
                                   ulong min, ulong max)
{
    ulong i = 0, start = 0, n;
    char  *p, *end;
    bool  error, range = false, comma = false;

    /* Clear list by default */
    vls.clear_all();

    p = txt;
    error = (p == NULL);
    while (p != NULL && *p != '\0') {
        /* Read integer */
        n = strtoul(p, &end, 0);
        if (end == p) {
            error = true;
            break;
        }
        p = end;

        /* Check legal range */
        if (n < min || n > max) {
            error = true;
            break;
        }

        if (range) {
            /* End of range has been read */
            if (n < start) {
                error = true;
                break;
            }
            for (i = start ; i <= n; i++) {
                vls.set(i);
            }
            range = false;
        } else if (*p == '-') {
            /* Start of range has been read */
            start = n;
            range = true;
            p++;
        } else {
            /* Single value has been read */
            vls.set(n);
        }
        comma = false;
        if (!range && *p == ',') {
            comma = true;
            p++;
        }
    }

    /* Check for trailing comma/dash */
    if (comma || range)
        error = true;

    /* Restore defaults if error */
    if (error) {
        vls.clear_all();
    }
    return (error ? VTSS_UNSPECIFIED_ERROR : VTSS_RC_OK);
}

mesa_rc xxrp_mgmt_global_managed_vids_get(vtss_mrp_appl_t appl, vtss::VlanList &vls)
{
    XXRP_PLATFORM_CRIT_ENTER();
    vls = XXRP_stack_conf[appl].managedVlans;
    XXRP_PLATFORM_CRIT_EXIT();
    return VTSS_RC_OK;
}

mesa_rc xxrp_mgmt_global_managed_vids_set(vtss_mrp_appl_t appl, vtss::VlanList &vls)
{
    XXRP_PLATFORM_CRIT_ENTER();
    XXRP_stack_conf[appl].managedVlans = vls;
    (void)vtss::mrp::vlan_list_set(vls);
    XXRP_PLATFORM_CRIT_EXIT();
    return VTSS_RC_OK;
}

mesa_rc mrp_mgmt_timers_get(vtss_isid_t isid,
                             mesa_port_no_t iport,
                             vtss_mrp_timer_conf_t *timers)
{
    mesa_rc rc = VTSS_RC_OK;

    if ((rc = XXRP_check_isid_port(isid, iport, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    XXRP_PLATFORM_CRIT_ENTER();
    *timers = XXRP_stack_conf[VTSS_MRP_APPL_MVRP].timers[isid - VTSS_ISID_START][iport - VTSS_PORT_NO_START];
    XXRP_PLATFORM_CRIT_EXIT();

    return rc;
}

mesa_rc mrp_mgmt_timers_set(vtss_isid_t isid, mesa_port_no_t iport,
                            const vtss_mrp_timer_conf_t *timers)
{
    mesa_rc rc = VTSS_RC_OK;
    u32 port_no = L2PORT2PORT(isid, iport);
    vtss::mrp::MrpTimeouts t;

    if ((rc = XXRP_check_isid_port(isid, iport, FALSE)) != VTSS_RC_OK) {
        return rc;
    }

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MGMT API setting timers for switch " << isid << ", iport " << iport;
    XXRP_PLATFORM_CRIT_ENTER();
    XXRP_stack_conf[VTSS_MRP_APPL_MVRP].timers[isid - VTSS_ISID_START][iport - VTSS_PORT_NO_START] = *timers;
    t.join = (vtss::milliseconds) (10 * timers->join_timer);
    t.leave = (vtss::milliseconds) (10 * timers->leave_timer);
    t.leaveAll = (vtss::milliseconds) (10 * timers->leave_all_timer);
    (void)vtss::mrp::port_timers_set(port_no, t);
    XXRP_PLATFORM_CRIT_EXIT();
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "MGMT API set the above timers";

    return rc;
}

#if 0
/* Not used at the moment */
mesa_rc xxrp_mgmt_applicant_adm_get(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, vtss_mrp_attribute_type_t attr_type, BOOL *participant)
{
    return VTSS_RC_OK;
}

/* Not used at the moment */
mesa_rc xxrp_mgmt_applicant_adm_set(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, vtss_mrp_attribute_type_t attr_type, BOOL participant)
{
    XXRP_base_port_applicant_adm_set_specific(isid, iport, appl, TRUE);
    return VTSS_RC_OK;
}
#endif

mesa_rc xxrp_mgmt_port_stats_get(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, vtss_mrp_stats_t *stats)
{
    u32 l2port = L2PORT2PORT(isid, iport);

    VTSS_RC(XXRP_check_isid_port(isid, iport, FALSE));
    if (appl >= VTSS_MRP_APPL_MAX) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (!stats) {
        return XXRP_ERROR_NULL_PTR;
    }
    VTSS_RC(vtss_mrp_stats_get(appl, l2port, stats));

    return VTSS_RC_OK;
}

mesa_rc xxrp_mgmt_port_stats_clear(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl)
{
    u32 l2port = L2PORT2PORT(isid, iport);

    VTSS_RC(XXRP_check_isid_port(isid, iport, FALSE))
    if (appl >= VTSS_MRP_APPL_MAX) {
        return XXRP_ERROR_INVALID_APPL;
    }
    VTSS_RC(vtss_mrp_stats_clear(appl, l2port));
    return VTSS_RC_OK;
}

mesa_rc xxrp_mgmt_vlan_state(u32 l2port, vlan_registration_type_t *array /* with VTSS_APPL_VLAN_ID_MAX + 1 elements */)
{
    vtss_isid_t  isid;
    mesa_port_no_t port;
    mesa_rc rc = VTSS_RC_ERROR;

    if (l2port2port(l2port, &isid, &port)) {
        rc = vlan_mgmt_registration_per_port_get(isid, port, array);
    }

    if (rc != VTSS_RC_OK) {
        // Better clear it in case of errors if caller should happen to
        // attempt to interpret it despite the error return code.
        memset(array, 0, (VTSS_APPL_VLAN_ID_MAX + 1) * sizeof(vlan_registration_type_t));
    }

    return rc;
}

mesa_rc xxrp_mgmt_map_get(vtss_mrp_appl_t appl, vtss_mrp_map_t ***map_ports)
{
    VTSS_RC(vtss_mrp_map_get(appl, map_ports));
    return VTSS_RC_OK;
}

/* Not used at the moment */
mesa_rc xxrp_mgmt_print_connected_ring(u8 msti)
{
#ifdef VTSS_MRP_APPL_MVRP
    VTSS_RC(vtss_mrp_port_ring_print(VTSS_MRP_APPL_MVRP, msti));
#endif
    return VTSS_RC_OK;
}

/* Not used at the moment */
#ifdef VTSS_MRP_APPL_MVRP
mesa_rc xxrp_mgmt_pkt_dump_set(BOOL pkt_control)
{
    xxrp_pkt_dump_set(pkt_control);
    return VTSS_RC_OK;
}
#endif

/* Not used at the moment */
#ifdef VTSS_MRP_APPL_MVRP
mesa_rc xxrp_mgmt_mad_port_print(vtss_isid_t isid, mesa_port_no_t iport, u32 machine_index)
{
    u32 l2port = L2PORT2PORT(isid, iport);
    vtss_mrp_port_mad_print(l2port, machine_index);
    return VTSS_RC_OK;
}
#endif

const char *const registrar_state2txt(u8 S)
{
    static const char *const N[] = {"IN", "LV", "MT"};
    return N[S];
}

const char *const registrar_admin_state2txt(u8 S)
{
    static const char *const N[] = {"Normal", "Fixed", "Forbidden"};
    return N[S];
}

const char *const bool_state2txt(u8 S)
{
    static const char *const N[] = {"Passive", "Active"};
    return N[S];
}

const char *const applicant_state2txt(u8 S)
{
    static const char *const N[] = {"VO", "VP", "VN", "AN", "AA", "QA", "LA", "AO", "QO", "AP", "QP", "LO"};
    return N[S];
}

/* Called locked */
mesa_rc xxrp_mgmt_reg_admin_status_get(vtss_mrp_appl_t appl, vtss_isid_t isid, mesa_port_no_t iport, u32 mad_fsm_index, u8 *t)
{
    u32 port = L2PORT2PORT(isid, iport);

    vtss_mrp_crit_assert();
    return vtss_mrp_reg_admin_status_get(appl, port, mad_fsm_index, t);
}

#ifdef VTSS_SW_OPTION_MVRP
mesa_rc xxrp_mgmt_mad_get(vtss_mrp_appl_t appl, vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_mad_t **mad)
{
    u32 l2port = L2PORT2PORT(isid, iport);

    return vtss_mrp_port_mad_get(appl, l2port, mad);
}
#endif

static void XXRP_mstp_register_config_change_cb(void)
{
    vtss_flag_setbits(&XXRP_rx_thread_flag, XXRP_EVENT_FLAG_VLAN_2_MSTI_MAP_CHANGE);
}

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
#ifdef __cplusplus
#ifdef VTSS_SW_OPTION_GVRP
extern "C" void vtss_gvrp_mib_init();
#endif /* VTSS_SW_OPTION_GVRP */
#endif /* __cplusplus */
/* MRP Private MIB */
#ifdef VTSS_SW_OPTION_MRP
namespace vtss {
    namespace mrp {
VTSS_PRE_DECLS void vtss_appl_mrp_mib_init();
}
}
#endif /* VTSS_SW_OPTION_MRP */
/* MVRP Private MIB */
#ifdef VTSS_SW_OPTION_MVRP
namespace vtss {
    namespace mvrp {
VTSS_PRE_DECLS void vtss_appl_mvrp_mib_init();
}
}
#endif /* VTSS_SW_OPTION_MVRP */
#endif /* defined(VTSS_SW_OPTION_PRIVATE_MIB) */

#if defined(VTSS_SW_OPTION_JSON_RPC)
#ifdef VTSS_SW_OPTION_GVRP
void vtss_appl_gvrp_json_init();
#endif /* VTSS_SW_OPTION_GVRP */
/* MRP JSON */
#ifdef VTSS_SW_OPTION_MRP
VTSS_PRE_DECLS void vtss_appl_mrp_json_init(void);
#endif /* VTSS_SW_OPTION_MRP */
/* MVRP JSON */
#ifdef VTSS_SW_OPTION_MVRP
VTSS_PRE_DECLS void vtss_appl_mvrp_json_init(void);
#endif /* VTSS_SW_OPTION_MVRP */
#endif /* defined(VTSS_SW_OPTION_JSON_RPC) */
extern "C" int mvrp_icli_cmd_register();
extern "C" int gvrp_icli_cmd_register();

mesa_rc xxrp_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "INIT_CMD_INIT";

        // Create the rx buffer for received MRPDUs (primary switch only)
        if (!vtss_bip_buffer_init(&XXRP_rx_buffer, XXRP_RX_BUFFER_SIZE_BYTES)) {
            T_EG(VTSS_TRACE_XXRP_GRP_PLATFORM, "Unable to create rx buffer");
        }
        // Create and release base crit
        /* If the 'max_lock_time' attribute of the critd struct is ever changed
         * to something other that the default 'CRITD_MAX_LOCK_TIME_DEFAULT'
         * then we need to also update the 'vtss_garp_timer_tick()'. The reason is
         * that this function assumes that the default value is used for the following
         * mutex.
         */
        critd_init(&XXRP_base_crit, "xxrp.base", VTSS_MODULE_ID_XXRP, CRITD_TYPE_MUTEX);

        // Create and release platform crit
        critd_init(&XXRP_platform_crit, "xxrp.platform", VTSS_MODULE_ID_XXRP, CRITD_TYPE_MUTEX);

        // Create and release mstp platform crit
        critd_init(&XXRP_mstp_platform_crit, "xxrp.mstp_platform", VTSS_MODULE_ID_XXRP, CRITD_TYPE_MUTEX);

        // Create and release application crit
        critd_init(&xxrp_appl_crit, "xxrp.appl", VTSS_MODULE_ID_XXRP, CRITD_TYPE_MUTEX);

        //Initialize the data structures.
        vtss_mrp_init();

        // Create timer thread related stuff
        vtss_flag_init(&XXRP_tm_thread_flag);
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           XXRP_tm_thread,
                           0,
                           "XXRP Timer",
                           nullptr,
                           0,
                           &XXRP_tm_thread_handle,
                           &XXRP_tm_thread_state);

        // Create rx thread related stuff
        vtss_flag_init(&XXRP_rx_thread_flag);
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           XXRP_rx_thread,
                           0,
                           "XXRP RX",
                           nullptr,
                           0,
                           &XXRP_rx_thread_handle,
                           &XXRP_rx_thread_state);

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
#ifdef VTSS_SW_OPTION_GVRP
        vtss_gvrp_mib_init();
#endif /* VTSS_SW_OPTION_GVRP */
#ifdef VTSS_SW_OPTION_MRP
        vtss::mrp::vtss_appl_mrp_mib_init();
#endif /* VTSS_SW_OPTION_MRP */
#ifdef VTSS_SW_OPTION_MVRP
        vtss::mvrp::vtss_appl_mvrp_mib_init();
#endif /* VTSS_SW_OPTION_MVRP */
#endif /* VTSS_SW_OPTION_PRIVATE_MIB */

#ifdef VTSS_SW_OPTION_JSON_RPC
#ifdef VTSS_SW_OPTION_GVRP
        vtss_appl_gvrp_json_init();
#endif /* VTSS_SW_OPTION_GVRP */
#ifdef VTSS_SW_OPTION_MRP
        vtss_appl_mrp_json_init();
#endif /* VTSS_SW_OPTION_MRP */
#ifdef VTSS_SW_OPTION_MVRP
        vtss_appl_mvrp_json_init();
#endif /* VTSS_SW_OPTION_MVRP */
#endif /* VTSS_SW_OPTION_JSON_RPC */

#ifdef VTSS_SW_OPTION_GVRP
        gvrp_icli_cmd_register();
#endif /* VTSS_SW_OPTION_GVRP */
#ifdef VTSS_SW_OPTION_MRP
#endif /* VTSS_SW_OPTION_MRP */
#ifdef VTSS_SW_OPTION_MVRP
        mvrp_icli_cmd_register();
#endif /* VTSS_SW_OPTION_MVRP */

        break;
    case INIT_CMD_START:
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "INIT_CMD_START";
        if (l2_stp_msti_state_change_register(XXRP_mstp_state_change) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Registration of MSTP port state change callback failed";
        }
        if (port_change_register(VTSS_MODULE_ID_XXRP, XXRP_port_state_change) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Registration of port state change callback failed";
        }

        XXRP_msg_register();
        l2_receive_register(VTSS_MODULE_ID_XXRP, XXRP_l2_rx);
        XXRP_packet_register();

        /* VLAN config change register */
        vlan_membership_change_register(VTSS_MODULE_ID_XXRP, XXRP_vlan_membership_change_callback);

#ifdef VTSS_SW_OPTION_ICFG
        VTSS_RC(xxrp_icfg_init());
#endif /* VTSS_SW_OPTION_ICFG */
        break;

    case INIT_CMD_CONF_DEF:
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "INIT_CMD_CONF_DEF, isid = " << isid;
        if (isid == VTSS_ISID_GLOBAL) { // Reset global configuration (no local or per switch configuration here)
            XXRP_PLATFORM_CRIT_ENTER();
            xxrp_conf_clear();
            XXRP_base_switch_sync(VTSS_ISID_GLOBAL); // Synchronize base module with actual configuration for all existing switches
            XXRP_PLATFORM_CRIT_EXIT();
            XXRP_msg_tx_local_conf(VTSS_ISID_GLOBAL); // Send configurations to all existing switches
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "INIT_CMD_ICFG_LOADING_PRE";
        XXRP_PLATFORM_CRIT_ENTER();
        xxrp_conf_clear();
        XXRP_PLATFORM_CRIT_EXIT();

        if (!mstp_register_config_change_cb(XXRP_mstp_register_config_change_cb)) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, ERROR) << "Registration of MSTP VLAN_2_MSTI change callback failed";
        }

        break;

    case INIT_CMD_ICFG_LOADING_POST:
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "INIT_CMD_ICFG_LOADING_POST, isid = " << isid;
        XXRP_msg_tx_local_conf(isid); // Send configurations to new switch
        XXRP_PLATFORM_CRIT_ENTER();
        XXRP_base_switch_sync(isid); // Synchronize base module with actual configuration for this switch
        XXRP_PLATFORM_CRIT_EXIT();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}
