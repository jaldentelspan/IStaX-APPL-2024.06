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

/* NOTE: FOR DEBUGGING THE lu26 TRANSPARENT CLOCK FORWARDING */
//#define PTP_LOG_TRANSPARENT_ONESTEP_FORWARDING

//#define TIMEOFDAY_TEST
//#define PHY_DATA_DUMP
// use custom wireless delay filter

#define SW_OPTION_PACKET_TX_DONE_CALLED_FROM_PACKET_TX
#include "main.h"
#include "main_types.h"
#include "ip_api.h"
#include "ip_utils.hxx"        /* For vtss_ip_checksum() and friends */
#include <vtss/appl/port.h>
#include "port_api.h"          /* For port_count_max() */
#include "port_iter.hxx"
#include "ptp_api.h"           /* Our module API */
#include "ptp.h"               /* Our private definitions */
#include "ptp_local_clock.h"   /* platform part of local_clock if */
#include "vtss_ptp_os.h"
#include "ptp_1pps_serial.h"
#include "ptp_pim_api.h"
#if defined(VTSS_SW_OPTION_MEP)
#include "mep_api.h"
#endif
#ifdef VTSS_SW_OPTION_ICFG
#include "ptp_icfg.h"
#endif
#if defined(VTSS_SW_OPTION_SYNCE)
#include "synce_api.h"
#include "synce_ptp_if.h"
#endif
#include "ptp_afi.h"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_PTP

// base PTP
#include "vtss_ptp_types.h"
#include "vtss_ptp_api.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_ptp_offset_filter.h"
#include "vtss_ptp_packet_callout.h"
#include "vtss_ptp_sys_timer.h"
#include "vtss_ptp_unicast.hxx"

/* Used APIs */
#include "l2proto_api.h"
#include "critd_api.h"
#include "packet_api.h"
#include "conf_api.h"
#include "misc_api.h"           /* instantiate MAC */
#include "acl_api.h"            /* set up access rule */
#include "interrupt_api.h"      /* interrupt handling */
#include "msg_api.h"            /* message module */
#include "vtss_timer_api.h"             /* timer system with no better resolution than the OS timer, but it runs on Linux */
#include "control_api.h"

#ifdef VTSS_SW_OPTION_PHY
#include "phy_api.h"
#endif
//  TOD
#include "vtss_tod_api.h"
#include "icli_porting_util.h"
#include "misc_icli_util.h"
#if defined(VTSS_SW_OPTION_NTP)
#include "vtss_ntp_api.h"
#endif

#include "vtss/basics/new.hxx"
#include "vtss/basics/trace.hxx"

#if defined(VTSS_SW_OPTION_ZLS30387)
#include "zl_3038x_api_pdv_api.h"
#include "synce_custom_clock_api.h"
#endif

#include "microchip/ethernet/switch/api.h"
#include "vtss_tod_mod_man.h"
#include "tod_api.h"
#include "ptp_1pps_sync.h"     /* platform part: 1pps synchronization */
#include "ptp_1pps_closed_loop.h"     /* platform part: 1pps closed loop delay measurement */

#include "vtss_os_wrapper.h"
#include "vtss_os_wrapper_network.h"

#include <vector>

//#define PTP_OFFSET_FILTER_CUSTOM
//#define PTP_DELAYFILTER_CUSTOM

#define API_INST_DEFAULT PHY_INST

static const u16 ptp_ether_type = 0x88f7;

// Cached capabilities
static uint32_t meba_cap_synce_dpll_mode_dual;
static uint32_t meba_cap_synce_dpll_mode_single;
static uint32_t mesa_cap_misc_chip_family;
#if defined(VTSS_SW_OPTION_MEP)
static uint32_t mesa_cap_oam_used_as_ptp_protocol;
#endif /* defined(VTSS_SW_OPTION_MEP) */
static uint32_t mesa_cap_packet_auto_tagging;
static uint32_t mesa_cap_packet_inj_encap;
static uint32_t mesa_cap_port_cnt;
static uint32_t mesa_cap_synce_ann_auto_transmit;
static uint32_t mesa_cap_ts;
static uint32_t mesa_cap_ts_asymmetry_comp;
static uint32_t mesa_cap_ts_bc_ts_combo_is_special;
static uint32_t mesa_cap_ts_c_dtc_supported;
static uint32_t mesa_cap_ts_domain_cnt;
static uint32_t mesa_cap_ts_has_alt_pin;
static uint32_t mesa_cap_ts_has_ptp_io_pin;
static uint32_t mesa_cap_ts_hw_fwd_e2e_1step_internal;
static uint32_t mesa_cap_ts_hw_fwd_p2p_1step;
static uint32_t mesa_cap_ts_io_cnt;
static uint32_t mesa_cap_ts_vir_io_cnt;
static uint32_t mesa_cap_ts_internal_mode_supported;
static uint32_t mesa_cap_ts_internal_ports_req_twostep;
static uint32_t mesa_cap_ts_missing_tx_interrupt;
static uint32_t mesa_cap_ts_org_time;
static uint32_t mesa_cap_ts_p2p_delay_comp;
static uint32_t mesa_cap_ts_pps_via_configurable_io_pins;
static uint32_t mesa_cap_ts_ptp_rs422;
static uint32_t mesa_cap_ts_twostep_always_required;
static uint32_t mesa_cap_ts_use_external_input_servo;
static uint32_t vtss_appl_cap_max_acl_rules_pr_ptp_clock;
static uint32_t vtss_appl_cap_ptp_clock_cnt;
static uint32_t vtss_appl_cap_zarlink_servo_type;
static uint32_t mesa_cap_ts_pps_in_via_io_pin;
 uint32_t mesa_cap_ts_twostep_use_ptp_id;
static bool     vtss_appl_ptp_sub_nano_second;
static bool     vtss_appl_ptp_combined_phy_switch_ts; // true when both phy and switch timestamping are used simultaneously.
static bool     pim_active = false;
static CapArray<bool, MESA_CAP_TS_DOMAIN_CNT> mesa_cap_ts_delay_req_auto_resp;

#define PTP_FLAG_DEFCONFIG   0x1
#define PTP_FLAG_TIMER_START 0x2

#define SW_OPTION_BASIC_PTP_SERVO // Basic Servo needs to be encluded for the purpose of calibration

#ifdef SW_OPTION_BASIC_PTP_SERVO
static CapArray<ptp_basic_servo *, VTSS_APPL_CAP_PTP_CLOCK_CNT> basic_servo;
#endif // SW_OPTION_BASIC_PTP_SERVO
#if defined(VTSS_SW_OPTION_ZLS30387)
static CapArray<ptp_ms_servo *, VTSS_APPL_CAP_PTP_CLOCK_CNT> advanced_servo;
#endif

/**
 * \brief PTP Clock Config Data Set structure
 */
typedef struct ptp_instance_config_t {
    ptp_init_clock_ds_t                                                                 clock_init;
    vtss_appl_ptp_clock_timeproperties_ds_t                                             time_prop;
    CapArray<vtss_appl_ptp_config_port_ds_t, VTSS_APPL_CAP_PORT_CNT_PTP_PHYS_AND_VIRT>  port_config;
    vtss_appl_ptp_clock_filter_config_t                                                 filter_params; /* default offset filter config */
    vtss_appl_ptp_clock_servo_config_t                                                  servo_params;  /* default servo config */
    vtss_appl_ptp_unicast_slave_config_t                                                unicast_slave[MAX_UNICAST_MASTERS_PR_SLAVE]; /* Unicast slave config, i.e. requested master(s) */
    vtss_appl_ptp_clock_slave_config_t                                                  slave_cfg;
    vtss_appl_ptp_virtual_port_config_t                                                 virtual_port_cfg;
} ptp_instance_config_t;

typedef struct ptp_config_t {
    CapArray<ptp_instance_config_t, VTSS_APPL_CAP_PTP_CLOCK_CNT> conf;
    vtss_appl_ptp_ext_clock_mode_t init_ext_clock_mode; /* luton26/Jaguar/Serval(Synce) external clock mode */
    vtss_ptp_rs422_conf_t init_ext_clock_mode_rs422; /* Serval(RS422) external clock mode */
    vtss_ho_spec_conf_t init_ho_spec;
    CapArray<vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> cmlds_port_conf;
} ptp_config_t;

static ptp_config_t config_data;
/* indicates if a transparent clock exists (only one transparent clock may exist in a node)*/
static bool transparent_clock_exists = false;
/* indicates if the wireless mode is enabled*/
typedef struct {
    bool                mode_enabled; /* true is fireless mode enabled */
    bool                remote_pre;   /* remote pre_notification => true, remote delay => false */
    bool                local_pre;    /* local pre_notification => true, local delay => false */
    mesa_timeinterval_t remote_delay;
    mesa_timeinterval_t local_delay;
    int                 pre_cnt;
} ptp_wireless_status_t;
//static ptp_wireless_status_t wireless_status = {false, false, false, 0LL, 0LL};

static mesa_packet_internal_tc_mode_t phy_ts_mode = MESA_PACKET_INTERNAL_TC_MODE_30BIT;
static uint32_t pps_calib_delay;

/* IPV4 protocol definitions */
#define PTP_EVENT_PORT 319
#define PTP_GENERAL_PORT 320
#define IP_HEADER_SIZE 20
#define UDP_HEADER_SIZE 8

static const u16 ip_ether_type = 0x0800;
static const mesa_mac_t ptp_eth_mcast_adr[2] = {{{0x01, 0x1b, 0x19, 0x00, 0x00, 0x00}}, {{0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e}}};
static const mesa_mac_t ptp_ip_mcast_adr[2]  = {{{0x01, 0x00, 0x5e, 0x00, 0x01, 0x81}}, {{0x01, 0x00, 0x5e, 0x00, 0x00, 0x6b}}};

static const mesa_mac_t ptp_ipv6_mcast_adr[2] = {{{0x33, 0x33, 0x0, 0x0, 0x01, 0x81}}, {{0x33, 0x33, 0x0, 0x0, 0x0, 0x6B}}};
/****************************************************************************/
/*  Reserved ACEs functions and MAC table setup                             */
/****************************************************************************/
/* If all ports have 1588 PHY, no ACL rules are needed for timestamping */
/* instead the forwarding is set up in the MAC table */
#define PTP_MAC_ENTRIES (PTP_CLOCK_INSTANCES*2)
static mesa_mac_table_entry_t mac_entry[PTP_MAC_ENTRIES];
static u32 mac_used = 0;

typedef struct {
    u8 domain;
    u8 deviceType;
    u8 protocol;
    mesa_port_list_t port_list;
    mesa_port_list_t external_port_list;
    mesa_port_list_t internal_port_list;
    CapArray<mesa_ace_id_t, VTSS_APPL_CAP_MAX_ACL_RULES_PR_PTP_CLOCK> id;
    bool      internal_port_exists;
} ptp_ace_rule_t;

static CapArray<ptp_ace_rule_t, VTSS_APPL_CAP_PTP_CLOCK_CNT> rules;
/*
    {.domain = 0, .deviceType = VTSS_APPL_PTP_DEVICE_NONE, .protocol = VTSS_APPL_PTP_PROTOCOL_ETHERNET,
     .id = {ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE,ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE,ACL_MGMT_ACE_ID_NONE}},
 {.domain = 0, .deviceType = VTSS_APPL_PTP_DEVICE_NONE, .protocol = VTSS_APPL_PTP_PROTOCOL_ETHERNET,
     .id = {ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE,ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE,ACL_MGMT_ACE_ID_NONE}},
 {.domain = 0, .deviceType = VTSS_APPL_PTP_DEVICE_NONE, .protocol = VTSS_APPL_PTP_PROTOCOL_ETHERNET,
     .id = {ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE,ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE,ACL_MGMT_ACE_ID_NONE}},
 {.domain = 0, .deviceType = VTSS_APPL_PTP_DEVICE_NONE, .protocol = VTSS_APPL_PTP_PROTOCOL_ETHERNET,
     .id = {ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE,ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE, ACL_MGMT_ACE_ID_NONE,ACL_MGMT_ACE_ID_NONE}},
};
*/

/****************************************************************************/
/*  PHY Timestamp data */
/****************************************************************************/
/*
 *  This variable is set to true if all external PTP ports have timestamp PHY.
 *  In this case we use MAC table instead og ACL rulse, because the switch does not need to do any timestamping.
 */
static bool ptp_all_external_port_phy_ts = false;

#define VTSS_PHY_TS_MAX_ENGINES 4

#define VTSS_PHY_PTP_ACTION_INVALID 0xffffffff

#define VTSS_PHY_PTP_INVALID_FLOW 0xFF

typedef struct {
    bool  phy_ts_port;  /* TRUE if this port is a PTP port and has the PHY timestamp feature */
    u8 deviceType;
    vtss_appl_ptp_protocol_t protocol; /* protocol configured from user interface */
    uint8_t flow[2];  // 0 - egress, 1 - ingress
    bool flow_used[2];// 0 - egress, 1 - ingress
    uint8_t clock[2]; // 0 - egress, 1 - ingress
    bool    sw_clk;   // software clock.
} ptp_inst_phy_ts_rule_config_t;

/**
 * \brief PHY timestamp configuration for each PTP instance
 */
static CapArray<ptp_inst_phy_ts_rule_config_t, VTSS_APPL_CAP_PTP_CLOCK_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> _ptp_inst_phy_rules;

#if defined (VTSS_SW_OPTION_P802_1_AS)
typedef struct {
    vtss_appl_ptp_802_1as_cmlds_default_ds_t cmlds_defaults;
    CapArray<ptp_cmlds_port_ds_t *, MEBA_CAP_BOARD_PORT_MAP_COUNT> cmlds_port_ds;
    CapArray<uint16_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> crcSrcPort;
} ptpCommonMeanLinkDelayService;
#endif

// Timer for switching to hybrid mode
static vtss::Timer hybrid_switch_timer;
// Time during which transient considered active
static vtss::Timer transient_timer;
static bool short_transient = false;
/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "ptp", "Precision Time Protocol"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default (PTP core)",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC
    },
    [VTSS_TRACE_GRP_PTP_BASE_TIMER] = {
        "timer",
        "PTP Internal timer",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC
    },
    [VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK] = {
        "pack",
        "PTP Pack/Unpack functions",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_BASE_MASTER] = {
        "master",
        "PTP Master Clock Trace",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_BASE_SLAVE] = {
        "slave",
        "PTP Slave Clock Trace",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_BASE_STATE] = {
        "state",
        "PTP Port State Trace",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_BASE_FILTER] = {
        "filter",
        "PTP filter module Trace",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY] = {
        "peer_delay",
        "PTP peer delay module Trace",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_BASE_TC] = {
        "tc",
        "PTP Transparent clock module Trace",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_BASE_BMCA] = {
        "bmca",
        "PTP BMCA module Trace",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_BASE_CLK_STATE] = {
        "clk_state",
        "slave clock state",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_BASE_802_1AS] = {
        "802_1AS",
        "802.1AS specific trace",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_SERVO] = {
        "servo",
        "PTP Local Clock Servo",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_INTERFACE] = {
        "interface",
        "PTP Core interfaces",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_CLOCK] = {
        "clock",
        "PTP Local clock functions",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_1_PPS] = {
        "1_pps",
        "PTP 1 pps input functions",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_EGR_LAT] = {
        "egr_lat",
        "Jaguar 1 step egress latency",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PHY_TS] = {
        "phy_ts",
        "PHY Timestamp feature",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_SER] = {
        "serial_1pps",
        "PTP Serial 1pps interface feature",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_PIM] = {
        "pim_proto",
        "PTP PIM protocol for 1pps and Modem modulation",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PTP_ICLI] = {
        "icli",
        "PTP ICLI log function",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PHY_1PPS] = {
        "phy_1pps",
        "PTP PHY 1pps Synchronization",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_SYS_TIME] = {
        "sys_time",
        "PTP and system time Synchronization",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_ACE] = {
        "ace",
        "PTP switch ACE configuration",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_MS_SERVO] = {
        "ms_servo",
        "PTP - MS Servo interface",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/* Thread variables */
static vtss_handle_t ptp_thread_handle;
static vtss_thread_t ptp_thread_block;

/* PTP global data */
/*lint -esym(457, ptp_flags) */
static vtss_flag_t ptp_flags;
static struct {
    bool ready;                                                   /* PTP Initited  */
    critd_t coremutex;                                            /* PTP core library serialization */
    mesa_mac_t sysmac;                                            /* Switch system MAC address */
    CapArray<struct in_addr, VTSS_APPL_CAP_PTP_CLOCK_CNT> my_ip;  /* one ip pr PTP instance */
    CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> ptpi;    /* PTP instance handle */
#if defined (VTSS_SW_OPTION_P802_1_AS)
    ptpCommonMeanLinkDelayService ptp_cm;
#endif
} ptp_global;

struct Lock_PTP {
    Lock_PTP(int line) {
        critd_enter(&ptp_global.coremutex, __FILE__, line);
    }
    ~Lock_PTP() {
        critd_exit( &ptp_global.coremutex, __FILE__, 0);
    }
};

#define PTP_CORE_LOCK_SCOPE() Lock_PTP __lock_guard__(__LINE__)

static const int header_size_ip4 = sizeof(mesa_mac_t) + sizeof(ptp_global.sysmac) + sizeof(ptp_ether_type) + IP_HEADER_SIZE + UDP_HEADER_SIZE;
static uint32_t hybrid_inst;

static mesa_rc ptp_phy_ts_update(int inst, int port);

static size_t pack_ip_udp_header(uchar * buf, u32 dest_ip, u32 src_ip, u16 port, u16 len, u16 dscp);

static mesa_rc ptp_servo_instance_set(uint instance);

static int find_active_reference(int inst, bool create);
static void vtss_ptp_process_non_active_virtual_port(int instance, vtss_ptp_timestamps_t *ts);
static void hybrid_switch_timer_init(void);
static void transient_timer_init(void);
static mesa_rc ptp_port_dis( uint portnum, uint instance);
static mesa_rc ptp_port_ena(bool internal, uint portnum, uint instance);

#if defined (VTSS_SW_OPTION_P802_1_AS)
static void ptp_cmlds_default_ds_default_get(vtss_appl_ptp_802_1as_cmlds_default_ds_t *cmlds_def_ds);
#endif

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
    #ifdef __cplusplus
        extern "C" void vtss_ptp_mib_init();
    #else
        void vtss_ptp_mib_init();
    #endif
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC)
    #ifdef __cplusplus
        extern "C" void vtss_appl_ptp_json_init();
    #else
        void vtss_appl_ptp_json_init();
    #endif
#endif

#ifdef PHY_DATA_DUMP
static void phy_ts_dump(void);
#endif

/* port ingress and egress data
 * this datastructure is used to hold the port data which depends on the HW
 * architecture.
 */
typedef struct {
    vtss_tod_ts_phy_topo_t topo;                                               /* Phy timestamp topology info */
    bool port_ts_in_sync_tmp;                                                  /* false if the port has PHY timestamper, and timestamper is not in sync with master timer, otherwise TRUE */
    u64 delay_cnt;                                                             /* peer delay value */
    u64 asymmetry_cnt;                                                         /* link asymmetry */
    mesa_timeinterval_t ingress_latency;
    mesa_timeinterval_t egress_latency;
    mesa_timeinterval_t asymmetry;
    bool link_state;                                                           /* false if link is down, otherwise TRUE */
    CapArray<mesa_packet_filter_t, VTSS_APPL_CAP_PTP_CLOCK_CNT> vlan_forward;  /* packet forwarding filter obtained from mesa_packet_port_filter_get */
    CapArray<mesa_etype_t, VTSS_APPL_CAP_PTP_CLOCK_CNT>         vlan_tpid;     /* packet forwarding filter tpid obtained from mesa_packet_port_filter_get */
    CapArray<bool, VTSS_APPL_CAP_PTP_CLOCK_CNT>                 vlan_forw;     /* packet forwarding filter tpid obtained from mesa_packet_port_filter_get */
    bool      backplane_port;                                                  /* True if the port is a backplane port, i.e no timestamping is done on the port */
    u32       port_domain;                                                     /* The timing domain number assigned to this port */
    u32       inst_count;                                                      /* The number of BC, MA, SL instances assigned to this port */
    vtss_ptp_phy_corr_type_t phy_corr_type;                                    /* PHY correction field update type. */
} port_data_t;

CapArray<port_data_t, VTSS_APPL_CAP_PORT_CNT_PTP_PHYS_AND_VIRT> port_data;

static CapArray<bool, VTSS_APPL_CAP_PTP_CLOCK_CNT, VTSS_APPL_CAP_PORT_CNT_PTP_PHYS_AND_VIRT> ports_org_time;

static CapArray<uint16_t, VTSS_APPL_CAP_PTP_CLOCK_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> crcSrcPortId;

/*
 * If a TC exists, that forwards all packets in HW, then my_tc_encapsulation is set to the TC encapsulation type
 * If this type of TC exists, and a SlaveOnly instance also exists, this slave is used for syntonization of the TC,
 * and the packets forwarded to the CPU must also be forwarded, i.e. the ACL rules for this slaveOnly instance depends
 * on the TC configuration.
 */
static u8 my_tc_encapsulation = VTSS_APPL_PTP_PROTOCOL_MAX_TYPE;   /* indicates that no forwarding in the SlaveOnly instance is needed */
static int my_tc_instance = -1;
static int my_2step_tc_instance = -1;

static CapArray<vtss_ptp_servo_mode_ref_t, VTSS_APPL_CAP_PTP_CLOCK_CNT> current_servo_mode_ref;

/*
 * Statistics for the 1PPS/ timeofday slave function
 */
static vtss_ptp_one_pps_tod_statistics_t pim_tod_statistics;

static void one_pps_tod_statistics_clear(void)
{
    pim_tod_statistics.tod_cnt          = 0;
    pim_tod_statistics.one_pps_cnt          = 0;
    pim_tod_statistics.missed_one_pps_cnt   = 0;
    pim_tod_statistics.missed_tod_rx_cnt    = 0;
}

// actual RS422 board configuration
static meba_ptp_rs422_conf_t rs422_conf;

mesa_rc ptp_clock_one_pps_tod_statistics_get(vtss_ptp_one_pps_tod_statistics_t *statistics, bool clear)
{
    PTP_CORE_LOCK_SCOPE();

    if (clear) {
        one_pps_tod_statistics_clear();
    }
    *statistics = pim_tod_statistics;

    return VTSS_RC_OK;
}

#define DEFAULT_1PPS_LATENCY        (12LL<<16)

static void ptp_in_sync_callback(mesa_port_no_t port_no, BOOL in_sync);

#define PTP_VIRTUAL_PORT_T1_TIMEOUT 3
#define PTP_VIRTUAL_PORT_T2_TIMEOUT 3
#define PTP_VIRTUAL_PORT_ALARM_RX_TIMEOUT 60 // 60 seconds after which alarm is set to false
typedef struct {
    mesa_port_no_t              one_pps_slave_port_no;  // actual port used as a slave port in a 1PPS slave (if PIM protocol used, this is the port)
    vtss_ptp_timestamps_t       one_pps_t1_t2;
    bool                        enable_t1;  // set to true if timeofday is expected via a serial interface or PIM (t1).
    bool                        new_t1;     // set to true when t1 (serial/PIM data)is received, set to false when it is used
    bool                        new_t2;     // set to true when t2 (1PPS) is received, set to false when it is used
    u32                         t1_to_count;   // check if t1 timestamps are missing
    u32                         t2_to_count;   // check if t2 timestamps are missing
    bool                        first;
    mesa_bool_t                 rx_alarm;
    uint8_t                     rx_alarm_timeout;
} ptp_one_pps_slave_data_t;

/* forward declaration */
static void ptp_virtual_port_monitor(vtss_ptp_sys_timer_t *timer, void *m);
static void ptp_virtual_port_alarm_generator(vtss_ptp_sys_timer_t *timer, void *m);
static mesa_rc ptp_clock_config_virtual_port_config_set(uint instance, const vtss_appl_ptp_virtual_port_config_t *const config);

typedef struct {
    ptp_one_pps_slave_data_t      slave_data;
    vtss_ptp_sys_timer_t          virtual_port_monitor_timer;
    vtss_ptp_sys_timer_t          virtual_port_alarm_timer;
} ptp_virtual_port_status_t;


CapArray<ptp_io_pin_mode_t, MESA_CAP_TS_IO_CNT> ptp_io_pin;
static ptp_virtual_port_status_t ptp_virtual_port[MAX_VTSS_PTP_INSTANCES];

static mesa_rc ptp_pin_init(void);
static mesa_rc io_pin_instance_set(int instance, u32 io_pin, BOOL enable);

static void ptp_port_filter_change_callback(int instance, mesa_port_no_t port_no, bool forward);
static bool ptp_port_domain_check(bool internal, bool enable, mesa_port_no_t port_no, uint instance);
static mesa_rc vtss_ptp_update_selected_src(void);

// PTP Internal mode
static CapArray<ptp_internal_mode_config_t, VTSS_APPL_CAP_PTP_CLOCK_CNT> ptp_internal_cfg;
static void ptp_internal_mode_init(int inst);
void ptp_internal_slave_t1_t2_rx(vtss_ptp_sys_timer_t *timer, void *m);
bool ptp_internal_mode_config_set_priv(uint32_t instance, ptp_internal_mode_config_t in_cfg);

static void port_data_conf_set(mesa_port_no_t port_no, bool link_up)
{
    mesa_port_no_t shared_port;
    if (link_up) {
        /* is this port a PHY TS port ? */

        tod_mod_man_port_data_get(port_no, &port_data[port_no].topo);

        T_DG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, ts_feature %d, channel %d, shared_port %d, in_sync %d", port_no,
             port_data[port_no].topo.ts_feature,
             port_data[port_no].topo.channel_id,
             port_data[port_no].topo.shared_port_no,
             port_data[port_no].topo.port_ts_in_sync);
        if (port_data[port_no].topo.port_shared) {
            shared_port = port_data[port_no].topo.shared_port_no;
            tod_mod_man_port_data_get(shared_port, &port_data[shared_port].topo);
            T_DG(VTSS_TRACE_GRP_PHY_TS, "Shared Port_no = %d, ts_feature %d, channel %d, shared_port %d, in_sync %d", shared_port,
             port_data[shared_port].topo.ts_feature,
             port_data[shared_port].topo.channel_id,
             port_data[shared_port].topo.shared_port_no,
             port_data[shared_port].topo.port_ts_in_sync);
        }
    }
}

//Calculate crc-12 used in Gen-3 phys(Lan8814).
static uint16_t crcCalculate(vtss_appl_ptp_port_identity *portIdentity)
{
    char crc[12] = {};
    char crc_input[10] = {};
    char byte, bit, invert;
    int i;
    uint16_t ret = 0;

    memcpy(crc_input, &portIdentity->clockIdentity, CLOCK_IDENTITY_LENGTH);
    crc_input[8] = (portIdentity->portNumber & 0xFF00) >> 8;
    crc_input[9] = portIdentity->portNumber & 0xFF;

    //Initialise crc.
    for (i = 0; i < 12; i++) {
        crc[i] = 1;
    }

    //calculate crc.
    for (i = 0; i < 10; i++) {
        byte = crc_input[i];

        // X^12 + X^11 + X^3 + X^2 + X^1 + 1
        for (int j = 0; j < 8; j++) {
            bit = byte & 0x1; //lowest order bit first processed
            byte = byte >> 1;
            invert = bit ^ crc[11];

            crc[11] = crc[10] ^ invert;
            crc[10] = crc[9];
            crc[9]  = crc[8];
            crc[8]  = crc[7];
            crc[7]  = crc[6];
            crc[6]  = crc[5];
            crc[5]  = crc[4];
            crc[4]  = crc[3];
            crc[3]  = crc[2] ^ invert;
            crc[2]  = crc[1] ^ invert;
            crc[1]  = crc[0] ^ invert;
            crc[0]  = invert;
        }
    }

    // Convert binary crc into number
    for (int j = 11; j > 0; j--) {
        ret |= crc[j];
        ret = ret << 1;
    }
    ret |= crc[0];

    T_IG(VTSS_TRACE_GRP_PHY_TS, "crc-12 calculated : 0x%x", ret);
    return ret;
}

static void port_data_init(void)
{
    int i;
    port_iter_t       pit;
    mepa_phy_info_t   phy_info;
    mesa_packet_internal_tc_mode_t tc_mode = MESA_PACKET_INTERNAL_TC_MODE_30BIT;

    if (mepa_phy_ts_cap()) {
        /* add in-sync callback function */
        PTP_RC(tod_mod_man_tx_ts_in_sync_cb_set(ptp_in_sync_callback));
        (void)tod_tc_mode_get(&tc_mode);

        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            i = pit.iport;

            tod_mod_man_port_data_get(i, &port_data[i].topo);

            T_IG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, ts_feature(gen) %d(%d), channel %d, shared_port %d, in_sync %d",
                                        i,
                                        port_data[i].topo.ts_feature,
                                        port_data[i].topo.ts_gen,
                                        port_data[i].topo.channel_id,
                                        port_data[i].topo.shared_port_no,
                                        port_data[i].topo.port_ts_in_sync);
            if (port_data[i].topo.ts_gen != VTSS_PTP_TS_GEN_NONE &&
                meba_phy_info_get(board_instance, i, &phy_info) == MESA_RC_OK) {
                if (tc_mode == MESA_PACKET_INTERNAL_TC_MODE_30BIT) {
                    // phys not using switch timestamping combination
                    if (!vtss_appl_ptp_combined_phy_switch_ts) {
                        port_data[i].phy_corr_type = PHY_CORR_GEN_2A;
                    }
                } else if (tc_mode == MESA_PACKET_INTERNAL_TC_MODE_48BIT) {
                    port_data[i].phy_corr_type = PHY_CORR_GEN_3C;
                }
                // Lan-8814 supports TC mode C only.
                if (phy_info.part_number == 8814) {
                    if (phy_info.revision == 1) {
                        port_data[i].phy_corr_type = PHY_CORR_GEN_3;
                    } else {
                        port_data[i].phy_corr_type = PHY_CORR_GEN_3C;
                    }
                }
                // Initialise 2-step phy memory for any start-up config
                for (int inst = 0; inst < PTP_CLOCK_INSTANCES; inst++) {
                    if ((config_data.conf[inst].clock_init.cfg.twoStepFlag ||
                         config_data.conf[inst].port_config[i].twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE) &&
                         config_data.conf[inst].port_config[i].enabled) {
                        if (tod_mod_man_tx_ts_queue_init(i) == VTSS_RC_ERROR) {
                            T_E("Could not create 2-step queue for port %d", i);
                        }
                    }
                }
                // Calculate crc for gen-3 phys cmlds port.
                if (port_data[i].topo.ts_gen == VTSS_PTP_TS_GEN_3) {
                    ptp_global.ptp_cm.crcSrcPort[i] = crcCalculate(&ptp_global.ptp_cm.cmlds_port_ds[i]->status.portIdentity);
                }
            }
        }
    }
}

static void port_data_pre_initialize(void)
{
    int i,j;
    port_iter_t       pit;
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        i = pit.iport;

        port_data[i].topo.port_ts_in_sync = TRUE;
        port_data[i].port_ts_in_sync_tmp = TRUE;
        port_data[i].topo.ts_feature = VTSS_PTP_TS_NONE;
        port_data[i].delay_cnt = 0;
        port_data[i].asymmetry_cnt = 0;
        port_data[i].ingress_latency = DEFAULT_INGRESS_LATENCY<<16;
        port_data[i].egress_latency = DEFAULT_EGRESS_LATENCY<<16;
        port_data[i].link_state = false;
        for (j = 0; j < PTP_CLOCK_INSTANCES; j++) {
            port_data[i].vlan_forward[j] =  MESA_PACKET_FILTER_DISCARD;
            port_data[i].vlan_tpid[j] = 0;
            port_data[i].vlan_forw[j] = false;
            if (mesa_cap_ts_org_time) {
                ports_org_time[j][i] = true;
            } else {
                ports_org_time[j][i] = false;
            }
        }
        port_data[i].backplane_port = false;
        port_data[i].port_domain = 0;
        port_data[i].inst_count = 0;
        port_data[i].phy_corr_type = PHY_CORR_GEN_2;
    }
}

/* latency observed in onestep tx timestamping */
static observed_egr_lat_t observed_egr_lat = {0,0,0,0};

/*
 * Forward defs
 */
static void
ptp_port_link_state_initial(int instance);
static mesa_rc ptp_ace_update(int i);
static void ext_clock_out_set(const vtss_appl_ptp_ext_clock_mode_t *mode);

//static mesa_rc ext_clock_rs422_conf_set(const vtss_ptp_rs422_conf_t *mode);
static void clock_default_timeproperties_ds_get(vtss_appl_ptp_clock_timeproperties_ds_t *timeproperties_ds);

/*
 *  Extract date from string. Result is represented as the number of days since 1970-01-01
 */
int extract_date(const char* s, u16* date) {
    int d, m, y;
    struct tm timeinfo;

    // Check that string pointed to by s is exactly 10 characters long and that the delimiters are correct
    if (strlen(s) != 10 || s[4] != '-' || s[7] != '-') return 1;

    // Scan the string
    if (sscanf(s, "%04d-%02d-%02d", &y, &m, &d) != 3) return 1;

    // Initialize a tm struct with the values parsed from the string
    struct tm t = {0};
    t.tm_mday = d;
    t.tm_mon = m - 1;
    t.tm_year = y - 1900;
    t.tm_isdst = -1;

    // Convert to a time_t structure and then back to a tm structure to normalize the date.
    time_t when = mktime(&t);
    const struct tm *norm = localtime_r(&when, &timeinfo);

    // validate (is the normalized date still the same?)
    if ((norm->tm_mday == d) && (norm->tm_mon == m - 1) && (norm->tm_year == y - 1900)) {
        *date = when / 86400;
        return 0;
    }
    else {
        return 1;
    }
}

/*
 * Conversion between l2port numbers and PTP port numbers.
 */
/* Convert from l2port (0-based) to PTP API port (1-based) */
static uint ptp_l2port_to_api(l2_port_no_t l2port)
{
    return (l2port + 1);
}

/* Convert vtss_ifindex_t to PTP API port (0-based) */
mesa_rc ptp_ifindex_to_port(vtss_ifindex_t i, u32 *v)
{
    vtss_ifindex_elm_t e;
    VTSS_RC(vtss_ifindex_decompose(i, &e));
    if (e.iftype != VTSS_IFINDEX_TYPE_PORT) {
        T_D("Interface %u is not a port interface", VTSS_IFINDEX_PRINTF_ARG(i));
        return VTSS_RC_ERROR;
    }
    *v = e.ordinal;

    return VTSS_RC_OK;
}

/* Convert from pTP API port (1-based) to l2port (0-based) */
static l2_port_no_t ptp_api_to_l2port(uint port)
{
    return (port - 1);
}

/* Convert from l2port (0-based) to pTP API port (1-based) */
static uint l2port_to_ptp_api(l2_port_no_t port)
{
    return (port + 1);
}

#if defined(VTSS_SW_OPTION_SYNCE)
static u8 g8275_ql_to_class(vtss_appl_synce_quality_level_t ql, vtss_appl_ptp_device_type_t device_type)
{
    switch (device_type) {
        case VTSS_APPL_PTP_DEVICE_ORD_BOUND:
            switch(ql) {
                case VTSS_APPL_SYNCE_QL_PRC:  return G8275PRTC_BC_HO_CLOCK_CLASS;
                case VTSS_APPL_SYNCE_QL_SSUA: return G8275PRTC_BC_OUT_OF_HO_CLOCK_CLASS;
                case VTSS_APPL_SYNCE_QL_SSUB: return G8275PRTC_BC_OUT_OF_HO_CLOCK_CLASS;
                default: return DEFAULT_CLOCK_CLASS;
            }
        case VTSS_APPL_PTP_DEVICE_MASTER_ONLY:
            switch(ql) {
                case VTSS_APPL_SYNCE_QL_PRC:  return G8275PRTC_GM_OUT_OF_HO_CLOCK_CLASS_CAT1;
                case VTSS_APPL_SYNCE_QL_SSUA: return G8275PRTC_GM_OUT_OF_HO_CLOCK_CLASS_CAT2;
                case VTSS_APPL_SYNCE_QL_SSUB: return G8275PRTC_GM_OUT_OF_HO_CLOCK_CLASS_CAT3;
                default: return DEFAULT_CLOCK_CLASS;
            }
        case VTSS_APPL_PTP_DEVICE_SLAVE_ONLY:
            return G8275PRTC_TSC_CLOCK_CLASS;
        default: return DEFAULT_CLOCK_CLASS;
    }
}

static u32 g8275_ho_spec(vtss_appl_synce_quality_level_t ql)
{
    switch(ql) {
        case VTSS_APPL_SYNCE_QL_PRC:  return config_data.init_ho_spec.cat1;
        case VTSS_APPL_SYNCE_QL_SSUA: return config_data.init_ho_spec.cat2;
        case VTSS_APPL_SYNCE_QL_SSUB: return config_data.init_ho_spec.cat3;
        default: return  0;
    }

}
#endif //defined(VTSS_SW_OPTION_SYNCE)
/****************************************************************************/
/*  Time adjustment period handling                                         */
/****************************************************************************/
static vtss_ptp_sys_timer_t ptp_time_settle_timer;
static bool ptp_time_settling = false;
                                /*
 * PTP system time to PTP time synchronization and vice versa timer function
 */
/*lint -esym(459, ptp_time_settle_function) */
/* to avoid lint warnings for the PTP_CORE_UNLOCK(); PTP_CORE_LOCK when calling out from the ptp_time_settle_function */
/*lint -e{455} */
/*lint -e{454} */

static void ptp_time_settle_function(vtss_ptp_sys_timer_t *timer, void *m)
{
    ptp_time_settling = false;

    PTP_CORE_UNLOCK(); /* unlock because ptp_in_sync_callback takes the lock */

    if (mepa_phy_ts_cap()) {
        port_iter_t pit;
        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            ptp_in_sync_callback(pit.iport, port_data[pit.iport].port_ts_in_sync_tmp);
        }
    }

    PTP_CORE_LOCK();
}

void ptp_time_setting_start(void)
{
    int32_t ptp_time_settle_init_value = 0; /**< Timer value used by the settle timer */
    if (mepa_phy_ts_cap()) {
        ptp_time_settle_init_value = 128*8;
    } else {
        ptp_time_settle_init_value = 128*2;
    }

    vtss_ptp_timer_start(&ptp_time_settle_timer, ptp_time_settle_init_value, false);
    ptp_time_settling = true;
    T_I("start settling timer");
}

static void ptp_time_settling_init(void)
{
    vtss_ptp_timer_init(&ptp_time_settle_timer, "ptp_time_settle", -1, ptp_time_settle_function, &ptp_time_settling);
    ptp_time_settling = false;
}

#if defined(VTSS_SW_OPTION_MEP)
static vtss_ptp_sys_timer_t oam_timer;
#define OAM_TIMER_INIT_VALUE 128
/*
 * PTP OAM slave timer
 */
/*lint -esym(459, ptp_oam_slave_timer) */
static void ptp_oam_slave_timer(vtss_ptp_sys_timer_t *timer, void *m)
{
    mep_dm_timestamp_t far_to_near;
    mep_dm_timestamp_t near_to_far;
    vtss_ptp_timestamps_t ts;
    int instance = 0;
    mesa_rc rc;
    int clock_inst = (u64)m;
    ptp_clock_t *ptp = ptp_global.ptpi[clock_inst];
    vtss_appl_mep_dm_conf_t   config;
    mep_conf_t  conf;
    static i32 oam_timer_value = OAM_TIMER_INIT_VALUE;
    u32 mep_inst = 0, res_port;

    T_N("ptp %p", ptp);
    if (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY && config_data.conf[instance].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM) {
        if (vtss_non_ptp_slave_check_port(ptp) == NULL) {
            /* check if OAM instance is active */
            oam_timer_value = OAM_TIMER_INIT_VALUE;
            if (mep_instance_conf_get(mep_inst, &conf) != VTSS_RC_OK) {
                T_W("MISSING MEP instance");
            } else {
                res_port = conf.port;

                if (conf.enable && conf.mode == VTSS_APPL_MEP_MEP && conf.direction == VTSS_APPL_MEP_DOWN && conf.domain == VTSS_APPL_MEP_PORT && conf.voe) {
                    rc = vtss_appl_mep_dm_conf_get(mep_inst, &config);
                    if (rc == VTSS_APPL_MEP_RC_INVALID_PARAMETER) {
                        T_W("MISSING OAM monitor");
                    } else {
                        if (!config.enable || config.calcway != VTSS_APPL_MEP_FLOW || config.tunit != VTSS_APPL_MEP_NS || !config.synchronized) {
                            T_I("OAM monitor not set up correctly");
                            config.enable = true;
                            config.calcway = VTSS_APPL_MEP_FLOW;
                            config.tunit = VTSS_APPL_MEP_NS;
                            config.synchronized = true;
                            rc = vtss_appl_mep_dm_conf_set(mep_inst, &config);
                            if (rc != VTSS_RC_OK) {
                                T_W("OAM monitor configuration failed");
                            }
                        } else {
                            oam_timer_value = (config.interval*128)/100;
                            T_N("OAM timer rate %d msec pr packet, timer valus %d", config.interval*10, oam_timer_value);

                            (void) vtss_ptp_port_ena(ptp_global.ptpi[instance], l2port_to_ptp_api(res_port));
                            ptp_port_filter_change_callback(instance, res_port, true);  /* ignore VLAN discarding function */
                            ptp_port_link_state_initial(instance);
                            vtss_non_ptp_slave_init(ptp_global.ptpi[instance], l2port_to_ptp_api(res_port));
                        }
                    }
                } else {
                    T_W("Wrong mep configuration");
                }
            }
        } else {
            if (wireless_status.remote_pre || wireless_status.local_pre) {
                T_I("Inst %d, remote_pre %d, local_pre %d", instance, wireless_status.remote_pre, wireless_status.local_pre);
                    if (++wireless_status.pre_cnt > 3) {
                        wireless_status.remote_pre = false;
                        wireless_status.local_pre = false;
                        wireless_status.pre_cnt = 0;
                    }
            } else {
                wireless_status.pre_cnt = 0;
                rc = mep_dm_timestamp_get(instance, &far_to_near, &near_to_far);
                if (rc == VTSS_RC_OK) {
                    T_D("Inst %d, far_to_near, tx_time  %12d:%9d, rx_time %12d:%9d", instance, far_to_near.tx_time.seconds,
                        far_to_near.tx_time.nanoseconds, far_to_near.rx_time.seconds, far_to_near.rx_time.nanoseconds);
                    ts.tx_ts.sec_msb = far_to_near.tx_time.sec_msb;
                    ts.tx_ts.seconds = far_to_near.tx_time.seconds;
                    ts.tx_ts.nanoseconds = far_to_near.tx_time.nanoseconds;
                    ts.rx_ts.sec_msb = far_to_near.rx_time.sec_msb;
                    ts.rx_ts.seconds = far_to_near.rx_time.seconds;
                    ts.rx_ts.nanoseconds = far_to_near.rx_time.nanoseconds;
                    ts.corr = wireless_status.remote_delay;
                    vtss_non_ptp_slave_t1_t2_rx(ptp_global.ptpi, clock_inst, &ts, DEFAULT_CLOCK_CLASS, 0, false);

                    T_D("Inst %d, near_to_far, tx_time  %12d:%9d, rx_time %12d:%9d", instance, near_to_far.tx_time.seconds,
                        near_to_far.tx_time.nanoseconds, near_to_far.rx_time.seconds, near_to_far.rx_time.nanoseconds);
                    ts.tx_ts.sec_msb = near_to_far.tx_time.sec_msb;
                    ts.tx_ts.seconds = near_to_far.tx_time.seconds;
                    ts.tx_ts.nanoseconds = near_to_far.tx_time.nanoseconds;
                    ts.rx_ts.sec_msb = near_to_far.rx_time.sec_msb;
                    ts.rx_ts.seconds = near_to_far.rx_time.seconds;
                    ts.rx_ts.nanoseconds = near_to_far.rx_time.nanoseconds;
                    ts.corr = wireless_status.local_delay;
                    vtss_non_ptp_slave_t3_t4_rx(ptp_global.ptpi, clock_inst, &ts);
                } else if (rc == VTSS_APPL_MEP_RC_NO_TIMESTAMP_DATA) {
                    T_I("Inst %d, no valid timestamp data", instance);
                } else {
                    T_I("Inst %d, OAM monitoring not active", instance);
                }
                if (rc != VTSS_RC_OK) {
                    vtss_non_ptp_slave_timeout_rx(ptp);
                }
            }
        }
        vtss_ptp_timer_start(&oam_timer, oam_timer_value, false);
    }
}
#endif

/*
 * PTP one_pps_synchronization slave function
 */
void ptp_1pps_ptp_slave_t1_t2_rx(int instance, mesa_port_no_t port_no, vtss_ptp_timestamps_t *ts)
{
    uint8_t clock_class = config_data.conf[instance].virtual_port_cfg.clockQuality.clockClass;

    T_IG(VTSS_TRACE_GRP_1_PPS, "Instance %d virtual port clk-class %d", instance, clock_class);

    // Do not process timestamps further if virtual port is not selected.
    if (!ptp_global.ptpi[instance]->ssm.virtual_port_select) {
        vtss_ptp_process_non_active_virtual_port(instance, ts);
        return;
    }

    vtss_non_ptp_slave_t1_t2_rx(ptp_global.ptpi, instance, ts, clock_class, 0, true);
}

/*!
*	Description: new_t1 is for tod interrupt and new_t2 is for pps signal. Both should be received in 1sec.
*	if any one is received twice consequtively, increment miss count for the other timestamp.
*	If both received in one sec set to true, after using set new_t1 and new_t2 to false for next cycle.
*/
static void ptp_external_input_slave_function(ptp_one_pps_slave_data_t *external_input_data, int inst, const mesa_timestamp_t  *ts, bool t1, mesa_timeinterval_t corr)
{
    char str1 [50];
    char str2 [50];

    T_DG(VTSS_TRACE_GRP_1_PPS,"DUMP: enable_t1[%d ]new_t1 [%d], new_t2[%d]", \
         external_input_data->enable_t1,external_input_data->new_t1 ,external_input_data->new_t2 );

    if (inst >= 0 && external_input_data->one_pps_slave_port_no != VTSS_PORT_NO_NONE) {
        if (t1) {
			++pim_tod_statistics.tod_cnt;
            external_input_data->one_pps_t1_t2.tx_ts = *ts;
            external_input_data->new_t1 = true;
            external_input_data->t1_to_count = 0;
            T_DG(VTSS_TRACE_GRP_1_PPS,"t1 %s", TimeStampToString(&external_input_data->one_pps_t1_t2.tx_ts, str1));
        } else {
            ++pim_tod_statistics.one_pps_cnt;
            external_input_data->one_pps_t1_t2.rx_ts = *ts;
            external_input_data->one_pps_t1_t2.corr = corr;
            external_input_data->new_t2 = true;
            external_input_data->t2_to_count = 0;
            T_DG(VTSS_TRACE_GRP_1_PPS,"t2 %s", TimeStampToString(&external_input_data->one_pps_t1_t2.rx_ts, str2));
            if (!external_input_data->enable_t1) {
                external_input_data->one_pps_t1_t2.tx_ts.sec_msb = ts->sec_msb;
                external_input_data->one_pps_t1_t2.tx_ts.seconds = ts->seconds;
                external_input_data->one_pps_t1_t2.tx_ts.nanoseconds = 0;
                if (ts->nanoseconds >= 500000000L) {
                    if (++external_input_data->one_pps_t1_t2.tx_ts.seconds == 0) {
                        ++external_input_data->one_pps_t1_t2.tx_ts.sec_msb;
                    }
                }
                external_input_data->new_t1 = true;
            } else { 
				T_DG(VTSS_TRACE_GRP_1_PPS,"set new_t1 to false");
                external_input_data->new_t1 = false;
            }
        }
        if (external_input_data->new_t1 && external_input_data->new_t2) {
            if (!external_input_data->first) {
                external_input_data->first = true;
            }
            T_DG(VTSS_TRACE_GRP_1_PPS,"call t1_t2_rx with: t1 %s and t2 %s", TimeStampToString(&external_input_data->one_pps_t1_t2.tx_ts, str1), TimeStampToString(&external_input_data->one_pps_t1_t2.rx_ts, str2));
            ptp_1pps_ptp_slave_t1_t2_rx(inst, external_input_data->one_pps_slave_port_no, &external_input_data->one_pps_t1_t2);
            external_input_data->new_t1 = false;
            external_input_data->new_t2 = false;
        }
    }
}

static vtss_ptp_sys_timer_t sys_time_sync_timer;
#define SYS_TIME_SYNC_INIT_VALUE 128
#define MAX_RTD 60  /* max accepted turn around time between two gettimeofday calls */
static vtss_appl_ptp_system_time_sync_conf_t system_time_sync_conf = {VTSS_APPL_PTP_SYSTEM_TIME_NO_SYNC};

/*
 * PTP system time to PTP time synchronization and vice versa timer function
 * Only PTP HW domain 0 can be synchronized to System Time
 */
/*lint -esym(459, ptp_sys_time_sync_function) */
static void ptp_sys_time_sync_function(vtss_ptp_sys_timer_t *timer, void *m)
{
    long clock_inst = (long) m;

    struct timeval tv;
    struct timespec ts_sys;
    struct timespec t4_sys;
    u64 tc;
    i32 rtd;        /* round trip relay in us */
    i32 offset;     /* current offset in ns */
    vtss_ptp_timestamps_t t1_t2;
    vtss_ptp_timestamps_t t3_t4;

    switch (system_time_sync_conf.mode) {
        case VTSS_APPL_PTP_SYSTEM_TIME_SYNC_SET:
            vtss_ptp_timer_start(&sys_time_sync_timer, SYS_TIME_SYNC_INIT_VALUE , true);
            T_DG(VTSS_TRACE_GRP_SYS_TIME, "SET SYSTEM TIME");
            (void)gettimeofday(&tv, NULL);
            vtss_tod_gettimeofday(0, &t1_t2.rx_ts, &tc); // Only domain 0
            TIMEVAL_TO_TIMESPEC(&tv, &ts_sys)
            if (ts_sys.tv_sec == t1_t2.rx_ts.seconds+1) {
                offset = -VTSS_ONE_MIA + t1_t2.rx_ts.nanoseconds - ts_sys.tv_nsec;
            } else if (ts_sys.tv_sec == t1_t2.rx_ts.seconds-1) {
                offset = VTSS_ONE_MIA + t1_t2.rx_ts.nanoseconds - ts_sys.tv_nsec;
            } else if (ts_sys.tv_sec == t1_t2.rx_ts.seconds) {
                offset = t1_t2.rx_ts.nanoseconds - ts_sys.tv_nsec;
            } else {offset = 0x7fffffff;}
            if (labs(offset) > 20*VTSS_ONE_MILL) {
#if defined(VTSS_SW_OPTION_NTP)
                ntp_conf_t ntp_conf;
                if ((VTSS_RC_OK == ntp_mgmt_conf_get(&ntp_conf)) && ntp_conf.mode_enabled) {
                    T_WG(VTSS_TRACE_GRP_SYS_TIME, "cannot set system time if ntp enabled");
                    system_time_sync_conf.mode = VTSS_APPL_PTP_SYSTEM_TIME_NO_SYNC;
                }
#endif
                if (system_time_sync_conf.mode == VTSS_APPL_PTP_SYSTEM_TIME_SYNC_SET) {
                    tv.tv_sec = t1_t2.rx_ts.seconds;
                    tv.tv_usec = t1_t2.rx_ts.nanoseconds/1000;
                    (void)settimeofday(&tv, NULL);
                    T_IG(VTSS_TRACE_GRP_SYS_TIME, "system time %lu s : %lu us", tv.tv_sec, tv.tv_usec);
                }
            }
            break;
        case VTSS_APPL_PTP_SYSTEM_TIME_SYNC_GET:
            if (config_data.conf[0].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {

                clock_gettime(CLOCK_REALTIME, &ts_sys);
                vtss_tod_gettimeofday(0, &t1_t2.rx_ts, &tc); // only domain 0
                clock_gettime(CLOCK_REALTIME, &t4_sys);

                /* calculate round trip delay in us */
                rtd = (t4_sys.tv_sec - ts_sys.tv_sec)*1000000 + (t4_sys.tv_nsec - ts_sys.tv_nsec) / 1000;
                T_DG(VTSS_TRACE_GRP_SYS_TIME, "rtd: %d", rtd);
                if (rtd > MAX_RTD || rtd < 0) {
                    T_IG(VTSS_TRACE_GRP_SYS_TIME, "big rtd: %d ignored", rtd);
                } else {
                    t1_t2.tx_ts.sec_msb = 0;
                    t1_t2.tx_ts.seconds = ts_sys.tv_sec;
                    t1_t2.tx_ts.nanoseconds = ts_sys.tv_nsec;
                    t1_t2.corr = 0LL;
                    vtss_non_ptp_slave_t1_t2_rx(ptp_global.ptpi, clock_inst, &t1_t2, DEFAULT_CLOCK_CLASS, 0, false);

                    t3_t4.tx_ts = t1_t2.rx_ts;
                    t3_t4.rx_ts.sec_msb = 0;
                    t3_t4.rx_ts.seconds = t4_sys.tv_sec;
                    t3_t4.rx_ts.nanoseconds = t4_sys.tv_nsec;
                    t3_t4.corr = 0LL;
                    vtss_non_ptp_slave_t3_t4_rx(ptp_global.ptpi, clock_inst, &t3_t4);
                    T_DG(VTSS_TRACE_GRP_SYS_TIME, "t1: %u:%u", t1_t2.tx_ts.seconds, t1_t2.tx_ts.nanoseconds);
                    T_DG(VTSS_TRACE_GRP_SYS_TIME, "t2: %u:%u", t1_t2.rx_ts.seconds, t1_t2.rx_ts.nanoseconds);
                    T_DG(VTSS_TRACE_GRP_SYS_TIME, "t3: %u:%u", t3_t4.tx_ts.seconds, t3_t4.tx_ts.nanoseconds);
                    T_DG(VTSS_TRACE_GRP_SYS_TIME, "t4: %u:%u", t3_t4.rx_ts.seconds, t3_t4.rx_ts.nanoseconds);
                }
            } else {
                vtss_ptp_timer_stop(timer);
                system_time_sync_conf.mode = VTSS_APPL_PTP_SYSTEM_TIME_NO_SYNC;
            }
            break;
        default:
            vtss_ptp_timer_stop(timer);
            break;
    }
}

static void poll_port_filter(int instance)
{
    mesa_packet_port_info_t   info;
    u16 port;
    CapArray<mesa_packet_port_filter_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> filter;
    static CapArray<bool, VTSS_APPL_CAP_PTP_CLOCK_CNT> done_first_time;  // Note: All elements are initialize to 0 i.e. FALSE by constructor
    port_iter_t       pit;
    bool              filter_change = false;

    mesa_vid_t vid  = config_data.conf[instance].clock_init.cfg.configured_vid;
    PTP_RC(mesa_packet_port_info_init(&info));
    info.vid = vid; /* Tx VLAN ID */
    PTP_RC(mesa_packet_port_filter_get(NULL, &info, filter.size(), filter.data()));
    /* dump filter info */
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        port = pit.iport;

        // Update vlan tag information if required.
        if (config_data.conf[instance].clock_init.cfg.profile != VTSS_APPL_PTP_PROFILE_IEEE_802_1AS && config_data.conf[instance].clock_init.cfg.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            if ((filter[port].filter == MESA_PACKET_FILTER_TAGGED && port_data[port].vlan_forward[instance] != MESA_PACKET_FILTER_TAGGED) ||
                (filter[port].filter != MESA_PACKET_FILTER_TAGGED && port_data[port].vlan_forward[instance] == MESA_PACKET_FILTER_TAGGED) ||
                (filter[port].tpid != port_data[port].vlan_tpid[instance])) {
                filter_change = true;
            }
        }

        T_I("Inst %d, Port %d, VID %d, filter %d, tpid 0x%x port data forward %d tpid 0x%x", instance, port, vid, filter[port].filter, filter[port].tpid, port_data[port].vlan_forward[instance], port_data[port].vlan_tpid[instance]);
        if (!done_first_time[instance]) {
            ptp_port_filter_change_callback(instance, port, filter[port].filter != MESA_PACKET_FILTER_DISCARD);
        } else {
            if (config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
#if defined (VTSS_SW_OPTION_P802_1_AS)
                /* Link up status combined with 802.1AS capable status used to determine forwarding status below. This is to ignore spanning tree discard status. */
                /* If peer delay mechanism is cmlds, then cmlds status is used for forwarding status. This is because incase of cmlds, instance specific asCapable state
                   is dependent on asCapableAcrossDomains and NeighborGptpCapable states. But, NeighborGptpCapable state is dependent on SIGNALLING messages
                   which are ignored when the PTP port state is disabled. */
                BOOL asCapable;
                if (config_data.conf[instance].port_config[port].delayMechanism == VTSS_APPL_PTP_DELAY_MECH_COMMON_P2P) {
                    asCapable = ptp_global.ptp_cm.cmlds_port_ds[port]->status.asCapableAcrossDomains;
                } else {
                    asCapable = ptp_global.ptpi[instance]->ptpPort[port].portDS.status.s_802_1as.asCapable;
                }
                /* IEEE 802.1AS 2011 11.3.3 : Vlan Tag : A frame that carries an IEEE 802.1AS message shall not have a VLAN tag nor a priority tag. */
                /* So, There should not be need for comparing tpid value. Spanning tree state changes notify change in tpid value and hence ignored.*/
                bool fwd_status = (port_data[port].link_state && asCapable) || (filter[port].filter != MESA_PACKET_FILTER_DISCARD);
                if (fwd_status != port_data[port].vlan_forw[instance]) {
                    port_data[port].vlan_forward[instance] = filter[port].filter;
                    port_data[port].vlan_tpid[instance] = filter[port].tpid;
                    ptp_port_filter_change_callback(instance, port, fwd_status);
                }
#endif //VTSS_SW_OPTION_P802_1_AS
            } else if (filter[port].filter != port_data[port].vlan_forward[instance] || filter[port].tpid != port_data[port].vlan_tpid[instance]) {
                port_data[port].vlan_forward[instance] = filter[port].filter;
                port_data[port].vlan_tpid[instance] = filter[port].tpid;
                ptp_port_filter_change_callback(instance, port, filter[port].filter != MESA_PACKET_FILTER_DISCARD);
            }
        }
        if (filter_change) {
            // phy config update with vlan id
            if (_ptp_inst_phy_rules[instance][port].phy_ts_port) {
                T_I("Modify phy timestamping configuration due to change in vlan filter");
                ptp_phy_ts_update(instance, port);
            }
            // update vid in peer delay pkt buffer.
            vtss_ptp_update_vid_pkt_buf(ptp_global.ptpi[instance], port);
            filter_change = false;
        }
    }
    done_first_time[instance] = true;
}

static void initial_port_filter_get(int instance)
{
    mesa_packet_port_info_t info;
    u16                     port;
    port_iter_t             pit;
    CapArray<mesa_packet_port_filter_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> filter;
    mesa_vid_t              vid = config_data.conf[instance].clock_init.cfg.configured_vid;

    PTP_RC(mesa_packet_port_info_init(&info));

    info.vid = vid; /* Tx VLAN ID */
    PTP_RC(mesa_packet_port_filter_get(NULL, &info, filter.size(), filter.data()));

    // Dump filter info
    port_iter_init_local(&pit);

    while (port_iter_getnext(&pit)) {
        port = pit.iport;
        T_I("Inst %d, Port %d, VID %d, filter %d, tpid %x", instance, port, vid, filter[port].filter, filter[port].tpid);
        port_data[port].vlan_forward[instance] = filter[port].filter;
        port_data[port].vlan_tpid[instance] = filter[port].tpid;
    }
}

static size_t pack_ethernet_header(uchar * buf, const mesa_mac_t dest, const mesa_mac_t src, u16 ether_type, int instance)
{
    int i = 0;
    /* DA MAC */
    memcpy(&buf[i], &dest, sizeof(mesa_mac_t));
    i += sizeof(mesa_mac_t);
    /* SA MAC */
    memcpy(&buf[i], &src, sizeof(mesa_mac_t));
    i += sizeof(mesa_mac_t);

    /* ethertype  */
    buf[i++] = (ether_type>>8) & 0xff;
    buf[i++] = ether_type & 0xff;

    return i;
}

static size_t unpack_ethernet_header(const uchar *const buf, mesa_mac_t *src, u16 *ether_type)
{

    int i = sizeof(mesa_mac_t);

    /* SA MAC */
    memcpy(src, &buf[i], sizeof(mesa_mac_t));
    i += sizeof(mesa_mac_t);

    /* ethertype  */
    *ether_type = (buf[i]<<8) + buf[i+1];
    i += 2;

    /* skip vlan tag if present */
    if (*ether_type != ptp_ether_type && *ether_type != ip_ether_type) {
        T_W("skipping tag  %d", *ether_type);
        i+=2;
        *ether_type = (buf[i]<<8) + buf[i+1];
        i += 2;
    }

    return i;
}

void vtss_1588_update_encap_header(u8 *buf, bool uni, bool event, u16 len, int instance)
{

    int i;
    u16 ether_type;
    mesa_mac_t src;
    u32 dest_ip;
    u32 src_ip;
    u16 port;
    i = sizeof(mesa_mac_t);
    if (uni) {  /* swap addresses */
        /* SA MAC */
        memcpy(&src, &buf[i], sizeof(mesa_mac_t));
        memcpy(&buf[i], &buf[0], sizeof(mesa_mac_t));
        memcpy(&buf[0], &src, sizeof(mesa_mac_t));
    } else {  /* update src addres */
        memcpy(&buf[i], &ptp_global.sysmac, sizeof(mesa_mac_t));
    }

    i += sizeof(mesa_mac_t);
    /* ethertype  */
    ether_type = (buf[i]<<8) + buf[i+1];
    i += 2;
    if (ether_type != ptp_ether_type && ether_type != ip_ether_type) {
        T_W("skipping tag  %d", ether_type);
        i+=2;
        ether_type = (buf[i]<<8) + buf[i+1];
        i += 2;
    }
    if (ether_type == ip_ether_type) {
        if (uni) {  /* SWAP IP adresses */
            dest_ip = vtss_tod_unpack32(buf+i+12);
            src_ip = vtss_tod_unpack32(buf+i+16);
        } else {
            dest_ip = vtss_tod_unpack32(buf+i+16);
            src_ip = ptp_global.my_ip[instance].s_addr;
        }
        port = event ? PTP_EVENT_PORT : PTP_GENERAL_PORT;
        (void)pack_ip_udp_header(buf+i, dest_ip, src_ip, port, len, config_data.conf[instance].clock_init.cfg.dscp);
    }
}

static void update_ptp_ip_address(int instance, mesa_vid_t vid)
{
    vtss_appl_ip_if_status_t status;
    int                      port;
    struct in_addr           ip;
    bool                     err = false;
    mesa_vid_t               conf_vid = vid;
    port_iter_t              pit;

    ip.s_addr = 0;

    vtss_ifindex_t ifidx;
    (void)vtss_ifindex_from_vlan(conf_vid, &ifidx);
    if (vtss_appl_ip_if_status_get(ifidx, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 1, nullptr, &status) == VTSS_RC_OK) {
        if(status.type != VTSS_APPL_IP_IF_STATUS_TYPE_IPV4) {
            err = true;
        } else {
            ip.s_addr = status.u.ipv4.net.address;
            err = false;
        }

        T_D("my IP address= %x", ip.s_addr);
        if (!err) {
            if (ptp_global.my_ip[instance].s_addr != ip.s_addr) {
                ptp_global.my_ip[instance].s_addr = ip.s_addr;
                if (ptp_ace_update(instance) != VTSS_RC_OK) {
                    T_I("ACE update error");
                }
            }
        } else {
            T_I("No IPV4 address defined for VID %d", conf_vid);
        }
    } else {
        T_I("Failed to get my IP address for VID %d", conf_vid);
    }

    /* we also must reinitialize the ports, if the IP address has changed */
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        port = pit.iport;
        ptp_port_filter_change_callback(instance, port, port_data[port].vlan_forw[instance]);
    }

    T_I("instance: %d, IP address= %x, err %d", instance, ip.s_addr, err);
}

static void notify_ptp_ip_address(vtss_ifindex_t ifidx)
{
    if (!vtss_ifindex_is_vlan(ifidx)) {
        VTSS_TRACE(DEBUG) << "ptp called with unsupported interface: " << ifidx;
        return;
    }

    vtss_ifindex_elm_t ife;
    if (vtss_ifindex_decompose(ifidx, &ife) != MESA_RC_OK) {
        return;
    }
    mesa_vid_t if_id = ife.ordinal;

    PTP_CORE_LOCK_SCOPE();

    T_D("if_id: %d", if_id);
    for (int i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        if (config_data.conf[i].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE &&
           (config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI || //When there is change in IP address, notify only the instances using
            config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED || //ptp over IP protocol because ptp state will get reset due to this
            config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI)) {  //notification.
            if (config_data.conf[i].clock_init.cfg.configured_vid == if_id) {
                update_ptp_ip_address(i, if_id);
            }
        }
    }
}

/*
 * buf = address of data buffer
 *
 */
static void update_udp_checksum(uchar *buf)
{
    int            ip_header_len, i;
    unsigned char  *ip_buf, *udp_buf;
    uint16_t       ether_type, uh_sum, len;
    mesa_ip_addr_t sip, dip;

    /* check if IPv4 encapsulation */
    i = 2 * sizeof(mesa_mac_t);

    /* ethertype  */
    ether_type = (buf[i] << 8) + buf[i + 1];
    i += 2;

    if (ether_type != ptp_ether_type && ether_type != ip_ether_type) {
        T_N("skipping tag  %x", ether_type);
        i += 2;
        ether_type = (buf[i] << 8) + buf[i + 1];
        i += 2;
    }

    if (ether_type != ip_ether_type) {
        // Not an IPv4 packet.
        return;
    }

    ip_buf = buf + i;
    ip_header_len = (ip_buf[0] & 0x0f) << 2;
    if (ip_buf[9] != IPPROTO_UDP) {
        // Not an UDP packet.
        return;
    }

    // Advance to start of UDP protocol header
    i += ip_header_len;
    udp_buf = buf + i;

    /* if checksum != 0 then recalculate the checksum */
    uh_sum = vtss_tod_unpack16(udp_buf + 6);
    if (uh_sum == 0) {
        // UDP checksum is 0, so leave it like that without recalculating a new
        return;
    }

    // Find length of UDP data, SIP, and DIP, which is used in UDP checksum
    // calculations.
    len = vtss_tod_unpack16(udp_buf +  4);
    sip.type = MESA_IP_TYPE_IPV4;
    sip.addr.ipv4 = vtss_tod_unpack32(ip_buf + 12);
    dip.type = MESA_IP_TYPE_IPV4;
    dip.addr.ipv4 = vtss_tod_unpack32(ip_buf + 16);

    // Clear UDP checksum field first
    vtss_tod_pack16(0, udp_buf + 6);
    uh_sum = vtss_ip_pseudo_header_checksum(udp_buf, len, sip, dip, IP_PROTO_UDP);

    T_R("uh_sum after: 0x%04x, len = %u", uh_sum, len);
    vtss_tod_pack16(uh_sum, udp_buf + 6);
}

static size_t pack_ip_udp_header(uchar * buf, u32 dest_ip, u32 src_ip, u16 port, u16 len, u16 dscp)
{
    u16 ip_sum;
    /* Fill out the IP header */
    buf[0] = (4<<4) | (IP_HEADER_SIZE>>2);
    buf[1] = dscp<<2;
    vtss_tod_pack16(IP_HEADER_SIZE + UDP_HEADER_SIZE + len, buf+2);
    vtss_tod_pack16(0, buf+4); // identification = 0
    vtss_tod_pack16(0, buf+6); // fragmentation = 0
    buf[8] = 128; // time to live
    // For messages sent to PTP pdelay address TTL shall be set to 1 (IEEE 1588)
    if (dest_ip == PTP_PDELAY_DEST_IP) {
        buf[8] = 1; // time to live
    }
    buf[9] = IPPROTO_UDP; // protocol
    vtss_tod_pack16(0, buf+10); // header checksum
    //update_ip_address();
    vtss_tod_pack32(src_ip, buf+12);
    vtss_tod_pack32(dest_ip, buf+16);
    /* Checksum the IP header... */
    ip_sum = vtss_ip_checksum(buf, IP_HEADER_SIZE);
    vtss_tod_pack16(ip_sum, buf + 10);

    /* Fill out the UDP header */
    vtss_tod_pack16(port, buf+IP_HEADER_SIZE);  /* source port same as dest port */
    vtss_tod_pack16(port, buf+IP_HEADER_SIZE+2);  /* dest port */
    vtss_tod_pack16(UDP_HEADER_SIZE + len, buf+IP_HEADER_SIZE+4);  /* message length */
    vtss_tod_pack16(0, buf+IP_HEADER_SIZE+6); /* 0 checksum is allowed */

    return IP_HEADER_SIZE + UDP_HEADER_SIZE;
}

static size_t unpack_ip_udp_header(const uchar * buf, u32 *src_ip, u32 *dest_ip, u16 *port, u16 *len)
{

    int             ip_header_len = (buf[0] & 0x0f) << 2;

    /* Decode the IP header */
    *len = vtss_tod_unpack16(buf+2) - ip_header_len - UDP_HEADER_SIZE;
    *src_ip = vtss_tod_unpack32(buf+12);
    *dest_ip = vtss_tod_unpack32(buf+16);

    /* Decode the UDP header */
    if (buf[9] == IPPROTO_UDP) { // protocol
        *port = vtss_tod_unpack16(buf+IP_HEADER_SIZE+2);  /* dest port */
        return ip_header_len + UDP_HEADER_SIZE;
    } else {
        *port = 0;
        return ip_header_len;
    }
}

/*
 * pack the Encapsulation protocol into a frame buffer.
 *
 */
size_t vtss_1588_pack_encap_header(u8 * buf, vtss_appl_ptp_protocol_adr_t *sender, vtss_appl_ptp_protocol_adr_t *receiver, u16 data_size, bool event, int instance)
{
    size_t i;
    vtss_appl_ptp_protocol_adr_t my_sender;
    if (sender != 0) {
        memcpy(&my_sender, sender, sizeof(my_sender));
    } else {
        my_sender.ip = ptp_global.my_ip[instance].s_addr;
        memcpy(&my_sender.mac, &ptp_global.sysmac, sizeof(my_sender.mac));
    }

    if (receiver->ip == 0) {
        i = pack_ethernet_header(buf, receiver->mac, my_sender.mac, ptp_ether_type, instance);
    } else {
        i = pack_ethernet_header(buf, receiver->mac, my_sender.mac, ip_ether_type, instance);
        i += pack_ip_udp_header(buf+i,receiver->ip, my_sender.ip, event ? PTP_EVENT_PORT : PTP_GENERAL_PORT, data_size, config_data.conf[instance].clock_init.cfg.dscp);
    }
    return i;
}

void vtss_1588_pack_eth_header(u8 * buf, mesa_mac_t receiver)
{
    /* DA MAC */
    memcpy(&buf[0], &receiver, sizeof(mesa_mac_t));

    /* SA MAC */
    memcpy(&buf[sizeof(mesa_mac_t)], &ptp_global.sysmac, sizeof(mesa_mac_t));
}

void vtss_1588_tag_get(ptp_tag_conf_t *tag_conf, int instance, vtss_ptp_tag_t *tag)
{
    if ((port_data[ptp_api_to_l2port(tag_conf->port)].vlan_forward[instance] == MESA_PACKET_FILTER_TAGGED)) {
        tag->tpid = port_data[ptp_api_to_l2port(tag_conf->port)].vlan_tpid[instance];
        tag->vid = tag_conf->vid;
        tag->pcp = tag_conf->pcp;
    } else {
        tag->tpid = 0;
        tag->vid = 0;
        tag->pcp = 0;
    }
}
#if defined (VTSS_SW_OPTION_P802_1_AS)
static void poll_cmlds_conf_updates()
{
    bool cmlds_link_ena;
    for (uint i = 0; i < mesa_cap_port_cnt; i++) {
        cmlds_link_ena = false;
        for (int j = 0; j < PTP_CLOCK_INSTANCES; j++) {
            if (ptp_global.ptp_cm.cmlds_port_ds[i]->cmlds_in_use[j]) {
                vtss_ptp_cmlds_clock_inst_status_notify(ptp_global.ptpi[j], i, &ptp_global.ptp_cm.cmlds_port_ds[i]->status);
                cmlds_link_ena = true;
            }
        }
        if ((cmlds_link_ena != ptp_global.ptp_cm.cmlds_port_ds[i]->status.cmldsLinkPortEnabled) ||
            (ptp_global.ptp_cm.cmlds_port_ds[i]->conf_modified)) {
            ptp_cmlds_default_ds_default_get(&ptp_global.ptp_cm.cmlds_defaults);
            vtss_ptp_cmlds_peer_delay_update(ptp_global.ptp_cm.cmlds_port_ds[i], ptp_global.ptp_cm.cmlds_defaults.clockIdentity, &config_data.cmlds_port_conf[i], i, cmlds_link_ena, ptp_global.ptp_cm.cmlds_port_ds[i]->conf_modified);
        }
    }
}
#endif
/*
 * Propagate the PTP (module) configuration to the PTP core
 * library.
 */
static void ptp_conf_propagate(void)
{
    int i, j;
    /* Make effective in PTP core */
    for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        vtss_ptp_clock_create(ptp_global.ptpi[i]);
        if (config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT || config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT) {
            transparent_clock_exists = true;
        }
        for (j = 0; j < MAX_UNICAST_MASTERS_PR_SLAVE; j++) {
            vtss_ptp_uni_slave_conf_set(ptp_global.ptpi[i], j, &config_data.conf[i].unicast_slave[j]);
        }
        ptp_port_link_state_initial(i);
        vtss_ptp_clock_slave_config_set(ptp_global.ptpi[i]->ssm.servo, &config_data.conf[i].slave_cfg);
        if (config_data.conf[i].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            update_ptp_ip_address(i, config_data.conf[i].clock_init.cfg.configured_vid);
            if (VTSS_RC_OK != ptp_servo_instance_set(i)) {
                T_W("Failed to set servo instance");
            }
        }

#if defined(VTSS_SW_OPTION_MEP)
        if (mesa_cap_oam_used_as_ptp_protocol) {
            if (i == 0 && config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY && config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM) {
                vtss_ptp_timer_start(&oam_timer, OAM_TIMER_INIT_VALUE, false);
            }
        }
#endif
        for (j = 0; j < MAX_UNICAST_MASTERS_PR_SLAVE; j++) {
            vtss_ptp_uni_slave_conf_set(ptp_global.ptpi[i], j, &config_data.conf[i].unicast_slave[j]);
        }

        if (ptp_ace_update(i) != VTSS_RC_OK) {
            T_I("ACE del error");
        }
    }
    /* initial setup of external clock output */
    ext_clock_out_set(&config_data.init_ext_clock_mode);
#if 0 // outdated
    if (MESA_CAP(MESA_CAP_TS_PTP_RS422)) {
        if (VTSS_RC_OK != ext_clock_rs422_conf_set(&config_data.init_ext_clock_mode_rs422) && config_data.init_ext_clock_mode_rs422.mode != VTSS_PTP_RS422_DISABLE) {
            T_W("Failed to set rs422 conf");
        }
    }
#endif // outdated
}

/**
 * initialize run-time options to reasonable values
 * \param create indicates a new default configuration block should be created.
 *
 */
static void ptp_conf_init(ptp_instance_config_t *conf, u8 instance)
{
    int i, j;
    port_iter_t       pit;

    /* use the Encapsulated MAC-48 value as default clockIdentity
     * according to IEEE Guidelines for 64-bit Global Identifier (EUI-64) registration authority
     */
    conf->clock_init.cfg.deviceType = VTSS_APPL_PTP_DEVICE_NONE;
    conf->clock_init.cfg.twoStepFlag = CLOCK_FOLLOWUP;
    conf->clock_init.cfg.protocol = VTSS_APPL_PTP_PROTOCOL_ETHERNET;
    conf->clock_init.cfg.oneWay = false;
    conf->clock_init.cfg.configured_vid = 1;
    conf->clock_init.cfg.configured_pcp = 0;
    conf->clock_init.cfg.clock_domain = 0;
    conf->clock_init.cfg.dscp = 0; // the recommended DSCP for the default PHB
    conf->clock_init.numberPorts         = port_count_max() + (mesa_cap_ts_io_cnt ? 1 : 0);
    conf->clock_init.numberEtherPorts    = port_count_max();
    conf->clock_init.max_foreign_records = DEFAULT_MAX_FOREIGN_RECORDS;
    conf->clock_init.max_outstanding_records = DEFAULT_MAX_OUTSTANDING_RECORDS;
    memcpy(conf->clock_init.clockIdentity, ptp_global.sysmac.addr, 3);
    conf->clock_init.clockIdentity[3] = 0xff;
    conf->clock_init.clockIdentity[4] = 0xfe;
    memcpy(conf->clock_init.clockIdentity+5, ptp_global.sysmac.addr+3, 3);
    conf->clock_init.clockIdentity[7] += instance;
    if (mesa_cap_synce_ann_auto_transmit) {
        conf->clock_init.afi_announce_enable = true;
        conf->clock_init.afi_sync_enable = true;

    } else {
        conf->clock_init.afi_announce_enable = false;
        conf->clock_init.afi_sync_enable = false;
    }
    conf->clock_init.cfg.priority1 = 128;
    conf->clock_init.cfg.priority2 = 128;
    conf->clock_init.cfg.domainNumber = instance; // default domain is the same as instance number
    conf->clock_init.cfg.localPriority = 128;

    clock_default_timeproperties_ds_get(&conf->time_prop);

    vtss_appl_ptp_clock_config_default_virtual_port_config_get(&conf->virtual_port_cfg);

    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        i = pit.iport;
        conf->port_config[i].enabled = false;
        conf->port_config[i].logSyncInterval = DEFAULT_SYNC_INTERVAL;
        conf->port_config[i].delayMechanism = VTSS_APPL_PTP_DELAY_MECH_E2E;
        conf->port_config[i].logAnnounceInterval = DEFAULT_ANNOUNCE_INTERVAL;
        conf->port_config[i].logMinPdelayReqInterval = DEFAULT_DELAY_REQ_INTERVAL;
        conf->port_config[i].announceReceiptTimeout = DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT;
        conf->port_config[i].delayAsymmetry = DEFAULT_DELAY_ASYMMETRY<<16;
        conf->port_config[i].versionNumber = VERSION_PTP;
        conf->port_config[i].portInternal = false;
        conf->port_config[i].ingressLatency = DEFAULT_INGRESS_LATENCY<<16;
        conf->port_config[i].egressLatency = DEFAULT_EGRESS_LATENCY<<16;
        conf->port_config[i].localPriority = 128;
        conf->port_config[i].dest_adr_type = VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT;
        conf->port_config[i].masterOnly = FALSE;
        conf->port_config[i].notMaster = FALSE;
        conf->port_config[i].twoStepOverride = VTSS_APPL_PTP_TWO_STEP_OVERRIDE_NONE;
        conf->port_config[i].c_802_1as.peer_d.allowedLostResponses = DEFAULT_MAX_CONTIGOUS_MISSED_PDELAY_RESPONSE;
        conf->port_config[i].c_802_1as.peer_d.meanLinkDelayThresh = VTSS_MAX_TIMEINTERVAL; // max value set as default.
        conf->port_config[i].c_802_1as.syncReceiptTimeout = 3;
        conf->port_config[i].c_802_1as.as2020 = TRUE;
        conf->port_config[i].c_802_1as.peer_d.allowedFaults = DEFAULT_MAX_PDELAY_ALLOWED_FAULTS;
        conf->port_config[i].c_802_1as.peer_d.initialComputeMeanLinkDelay = true;
        conf->port_config[i].c_802_1as.peer_d.mgtSettableComputeMeanLinkDelay = true;
        conf->port_config[i].c_802_1as.peer_d.useMgtSettableComputeMeanLinkDelay = false;
        conf->port_config[i].c_802_1as.peer_d.initialComputeNeighborRateRatio = true;
        conf->port_config[i].c_802_1as.peer_d.mgtSettableComputeNeighborRateRatio = true;
        conf->port_config[i].c_802_1as.peer_d.useMgtSettableComputeNeighborRateRatio = false;
        conf->port_config[i].c_802_1as.peer_d.operLogPdelayReqInterval = 0;
        conf->port_config[i].c_802_1as.initialLogSyncInterval = -3;
        conf->port_config[i].c_802_1as.operLogSyncInterval = -3;
        conf->port_config[i].c_802_1as.peer_d.operLogPdelayReqInterval = 0;
        conf->port_config[i].c_802_1as.initialLogSyncInterval = -3;
        conf->port_config[i].c_802_1as.operLogSyncInterval = -3;
        conf->port_config[i].aedPortState = VTSS_APPL_PTP_PORT_STATE_AED_MASTER;
    }
    /* default offset filter config */
    vtss_appl_ptp_filter_default_parameters_get(&conf->filter_params, VTSS_APPL_PTP_PROFILE_NO_PROFILE);

    /* default servo config */
    vtss_appl_ptp_clock_servo_default_parameters_get(&conf->servo_params, VTSS_APPL_PTP_PROFILE_NO_PROFILE);

    /* initialize unicast slave table */
    for (j = 0; j < MAX_UNICAST_MASTERS_PR_SLAVE; j++) {
        conf->unicast_slave[j].duration = 100;
        conf->unicast_slave[j].ip_addr = 0;
    }
    vtss_appl_ptp_clock_slave_default_config_get(&conf->slave_cfg);

}

/**
 * Initialise PTP CMLDS values
 */
static void ptp_cmlds_conf_init()
{
    for (int i = 0; i < mesa_cap_port_cnt; i++) {
        vtss_appl_ptp_cmlds_conf_defaults_get(&config_data.cmlds_port_conf[i]);
    }
}

/**
 * Read the PTP configuration.
 * \param create indicates a new default configuration block should be created.
 *
 */
static void ptp_conf_read(bool create)
{
    int i;
    /* initialize run-time options to reasonable values */
    T_I("conf create");

    for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        if (config_data.conf[i].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            ptp_global.ptpi[i]->ssm.servo->deactivate(config_data.conf[i].clock_init.cfg.clock_domain);  // Deactivate the servo associated with the clock before the clock is deleted
        }
        ptp_conf_init(&config_data.conf[i], i);
    }
    ptp_cmlds_conf_init();
    vtss_ext_clock_out_default_get(&config_data.init_ext_clock_mode);
    system_time_sync_conf.mode = VTSS_APPL_PTP_SYSTEM_TIME_NO_SYNC;
    config_data.init_ext_clock_mode_rs422.mode = VTSS_PTP_RS422_DISABLE;
    config_data.init_ext_clock_mode_rs422.delay = 0;
}

static bool ptp_tc_sw_forwarding (int inst)
{
    bool rv = false;

    if  (((config_data.conf[inst].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT ||
           config_data.conf[inst].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT) &&
           config_data.conf[inst].clock_init.cfg.twoStepFlag)
      || (config_data.conf[inst].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT &&
          !config_data.conf[inst].clock_init.cfg.twoStepFlag && rules[inst].internal_port_exists &&
          !mesa_cap_ts_hw_fwd_e2e_1step_internal)
      || (config_data.conf[inst].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT &&
          !config_data.conf[inst].clock_init.cfg.twoStepFlag && !mesa_cap_ts_hw_fwd_p2p_1step))
    {
        rv = true;
    }

    T_D("rv = %d", rv);
    return rv;
}

/** \brief ptp protocol level */
typedef enum
{
    VTSS_PTP_PROTOCOL_LEVEL_NONE,     /**< Any frame type */
    VTSS_PTP_PROTOCOL_LEVEL_ETYPE,   /**< Ethernet Type */
    VTSS_PTP_PROTOCOL_LEVEL_IPV4,    /**< IPv4 */
    VTSS_PTP_PROTOCOL_LEVEL_IPV6     /**< IPv6 */
} vtss_ptp_protocol_level_t;

static vtss_ptp_protocol_level_t protocol_level(int protocol)
{
    if ((protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI) || (protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED) || (protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI))
        return VTSS_PTP_PROTOCOL_LEVEL_IPV4;
    if (protocol == VTSS_APPL_PTP_PROTOCOL_IP6MIXED)
        return VTSS_PTP_PROTOCOL_LEVEL_IPV6;
    if (protocol == VTSS_APPL_PTP_PROTOCOL_MAX_TYPE) return VTSS_PTP_PROTOCOL_LEVEL_NONE;
    return VTSS_PTP_PROTOCOL_LEVEL_ETYPE;
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

/*
 * Local port packet receive indication
 */
static BOOL packet_rx(void *contxt,
                      const u8 *const frm,
                      const mesa_packet_rx_info_t *const rx_info)
{
    int i;
    char str1[20];
    char str2[20];
    u16 etype;
    u16 udp_port = 0;
    u16 len;
    uint64_t rxTime = 0;
    mesa_bool_t timestamp_ok = false;
    vtss_appl_ptp_protocol_adr_t sender;
    bool release_fdma_buffers = true;
    ptp_tx_buffer_handle_t buf_handle;

    BOOL ongoing_adjustment = false;

    if (mesa_cap_ts_missing_tx_interrupt) {
        /* No timestamp interrupt, therefore do the timestamp update here */
        T_DG(_I,"update timestamps");
        PTP_RC(mesa_tx_timestamp_update(nullptr));
    }

    ulong j = unpack_ethernet_header(frm, &sender.mac, &etype);
    T_RG(_I, "RX_ptp, etype = %04x, port = %d, da= %s, sa = %s", etype, rx_info->port_no, misc_mac_txt(&frm[0],str1), misc_mac_txt(&frm[6], str2));
    T_RG_HEX(_I, frm, rx_info->length);
    sender.ip = 0;

    u32 dest_ip = 0;
    if (etype == ip_ether_type) {
        j += unpack_ip_udp_header(frm + j, &sender.ip, &dest_ip, &udp_port, &len);
        T_NG(_I, "RX_udp, udp_port = %d, port = %d dest_ip = 0x%x sender ip = 0x%x", udp_port, rx_info->port_no, dest_ip, sender.ip);
        if (udp_port != PTP_GENERAL_PORT && udp_port != PTP_EVENT_PORT) {
            T_WG(_I, "This UDP packet is not intended for me, udp_port = %d, port = %d", udp_port, rx_info->port_no);
            return false; /* this IP packet was not intended for me: i.e forward to other subscribers. */
        }
    }
    if (((etype == ptp_ether_type) && (rx_info->length > j)) ||
            ((etype == ip_ether_type) && (rx_info->length > j) && (udp_port == PTP_GENERAL_PORT || udp_port == PTP_EVENT_PORT))) {
        u8 message_type = frm[j] & 0xf;

        u8 domain_number = frm[j + 4];
        bool cmldsSdoId = ((frm[j] >> 4) & MAJOR_SDOID_CMLDS_802_1AS) ? true : false;
        uint rx_port = ptp_l2port_to_api(rx_info->port_no);
        T_DG(_I, "message_type: %d, domain %d, rx_port %d", message_type, domain_number, rx_port);
        PTP_CORE_LOCK();
        if (message_type <= 3) {
            mesa_packet_ptp_message_type_t ptp_message_type;
            if (message_type == PTP_MESSAGE_TYPE_SYNC) {
                ptp_message_type = MESA_PACKET_PTP_MESSAGE_TYPE_SYNC;
            } else if (message_type == PTP_MESSAGE_TYPE_P_DELAY_RESP) {
                ptp_message_type = MESA_PACKET_PTP_MESSAGE_TYPE_P_DELAY_RESP;
            } else {
                ptp_message_type = MESA_PACKET_PTP_MESSAGE_TYPE_GENERAL;
            }

            mesa_packet_timestamp_props_t ts_props;
            ts_props.ts_feature_is_PTS = (port_data[rx_info->port_no].topo.ts_feature == VTSS_PTP_TS_PTS &&
                                         !port_data[rx_info->port_no].port_domain);
            ts_props.phy_ts_mode = phy_ts_mode;
            ts_props.backplane_port = port_data[rx_info->port_no].backplane_port;
            ts_props.delay_comp.delay_cnt = port_data[rx_info->port_no].delay_cnt;
            ts_props.delay_comp.asymmetry_cnt = port_data[rx_info->port_no].asymmetry_cnt;

            (void)mesa_ptp_get_timestamp(NULL,
                                         frm + j + PTP_MESSAGE_RESERVED_FOR_TS_OFFSET,
                                         rx_info,
                                         ptp_message_type,
                                         ts_props,
                                         &rxTime,
                                         &timestamp_ok);
        }
        (void)mesa_ts_ongoing_adjustment(NULL, &ongoing_adjustment);
#if defined (VTSS_SW_OPTION_P802_1_AS)
        T_DG(_I, "port %d cmlds enabled %d", rx_info->port_no, ptp_global.ptp_cm.cmlds_port_ds[rx_info->port_no]->status.cmldsLinkPortEnabled);
        if (ptp_global.ptp_cm.cmlds_port_ds[rx_info->port_no]->status.cmldsLinkPortEnabled &&
           (message_type == PTP_MESSAGE_TYPE_P_DELAY_REQ || message_type == PTP_MESSAGE_TYPE_P_DELAY_RESP ||
            message_type == PTP_MESSAGE_TYPE_P_DELAY_RESP_FOLLOWUP)) {
            if (cmldsSdoId) { //Compare SdoId
                /* fill in the buffer structure */
                vtss_1588_tx_handle_init(&buf_handle);
                buf_handle.handle = 0;
                buf_handle.frame = (u8 *)frm;                         /* pointer to frame buffer */
                buf_handle.size = rx_info->length;                    /* length of data received, including encapsulation. */
                buf_handle.header_length = j;                    /* length of encapsulation header */
                buf_handle.hw_time = rxTime;                     /* internal HW time value used for correction field update. */
                buf_handle.tag.vid = 0;
                buf_handle.tag.pcp = 0;
                buf_handle.tag.tpid = 0;
                if (!ongoing_adjustment) {
                    release_fdma_buffers = !vtss_ptp_cmlds_messages(ptp_global.ptp_cm.cmlds_port_ds[rx_info->port_no], &buf_handle);
                }
            } else {// Discard packet.
                ptp_cmlds_port_ds_s *cmlds_ds = ptp_global.ptp_cm.cmlds_port_ds[rx_info->port_no];
                cmlds_ds->statistics.rxPTPPacketDiscardCount++;
            }
        } else if (ptp_global.ptp_cm.cmlds_port_ds[rx_info->port_no]->status.cmldsLinkPortEnabled &&
                   message_type == PTP_MESSAGE_TYPE_SIGNALLING && cmldsSdoId) {
            /* fill in the buffer structure */
            vtss_1588_tx_handle_init(&buf_handle);
            buf_handle.handle = 0;
            buf_handle.frame = (u8 *)frm;                         /* pointer to frame buffer */
            buf_handle.size = rx_info->length;                    /* length of data received, including encapsulation. */
            buf_handle.header_length = j;                    /* length of encapsulation header */
            buf_handle.hw_time = rxTime;                     /* internal HW time value used for correction field update. */
            buf_handle.tag.vid = 0;
            buf_handle.tag.pcp = 0;
            buf_handle.tag.tpid = 0;
            if (!ongoing_adjustment) {
                release_fdma_buffers = !vtss_ptp_cmlds_messages(ptp_global.ptp_cm.cmlds_port_ds[rx_info->port_no], &buf_handle);
            }
        } else {
#endif //VTSS_SW_OPTION_P802_1_AS
        for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
            /* ignore packets if protocol encapsulation does not match the configuration */
            if ((etype == ptp_ether_type && (config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET ||
                                             config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED)) ||
                (etype == ip_ether_type && (config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI ||
                                            config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED ||
                                            config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI))) {
                if (config_data.conf[i].port_config[rx_info->port_no].enabled &&
                               ((config_data.conf[i].clock_init.cfg.domainNumber == domain_number  || (message_type == PTP_MESSAGE_TYPE_P_DELAY_REQ ||
                                message_type == PTP_MESSAGE_TYPE_P_DELAY_RESP || message_type == PTP_MESSAGE_TYPE_P_DELAY_RESP_FOLLOWUP)) ||
                                (((config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT ||
                                config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT) &&
                                ptp_tc_sw_forwarding(i)) &&
                                 (message_type != PTP_MESSAGE_TYPE_P_DELAY_REQ && message_type != PTP_MESSAGE_TYPE_P_DELAY_RESP && message_type != PTP_MESSAGE_TYPE_P_DELAY_RESP_FOLLOWUP)
                                 )))
                {
                    if (!release_fdma_buffers) {
                        T_DG(_I, "inst %d, message_type %d, buffer from domain %d has already been forwarded", i, message_type, domain_number);
                    } else if (!cmldsSdoId) {
                        if (mesa_cap_ts_bc_ts_combo_is_special) {
                            if (timestamp_ok) {
                                /* special case for combined BC and TC  */
                                /* in L26, the rxtime is added to the correctionField in this case, because L26 does both onestep and twostep on the sync packet */
                                if (protocol_level(my_tc_encapsulation) == protocol_level(config_data.conf[i].clock_init.cfg.protocol) && my_tc_instance > 0 && message_type == PTP_MESSAGE_TYPE_SYNC) {
                                    mesa_timeinterval_t corr = vtss_tod_unpack64(frm + j + PTP_MESSAGE_CORRECTION_FIELD_OFFSET);
                                    corr -= 4*(((mesa_timeinterval_t)rxTime)<<16);
                                    /*Update correction Field */
                                    vtss_tod_pack64(corr, (u8 *)(frm + j + PTP_MESSAGE_CORRECTION_FIELD_OFFSET));
                                }
                            }
                        }

                        T_NG(_I, "clock instance: %d, domain %d", i, domain_number);
                        if (rx_info->tag.vid == config_data.conf[i].clock_init.cfg.configured_vid) {
                            T_IG(_I,"Accepted Vlan tag: tag %x vid %d,", rx_info->tag_type, rx_info->tag.vid);
                            /* fill in the buffer structure */
                            vtss_1588_tx_handle_init(&buf_handle);
                            buf_handle.handle = 0;
                            buf_handle.frame = (u8 *)frm;                          /* pointer to frame buffer */
                            buf_handle.size = rx_info->length;                    /* length of data received, including encapsulation. */
                            buf_handle.header_length = j;                    /* length of encapsulation header */
                            buf_handle.hw_time = rxTime;                     /* internal HW time value used for correction field update. */
                            buf_handle.tag.vid = config_data.conf[i].clock_init.cfg.configured_vid; /* use configured tag for forwarding*/
                            buf_handle.tag.pcp = config_data.conf[i].clock_init.cfg.configured_pcp; /* use configured tag for forwarding*/
                            buf_handle.tag.tpid = port_data[rx_info->port_no].vlan_tpid[i];
                            T_IG(_I,"Handle Vlan tpid %x vid %d,", buf_handle.tag.tpid, buf_handle.tag.vid);

                            if (message_type <= 3) {
                                // EVENT_PDU:
                                T_IG(_I, "EVENT_PDU: %u bytes, port %u, msg_type %d, timestamp_ok %d", rx_info->length, rx_port, message_type, timestamp_ok);
                                /* if the port has PHY 1588 support, the timestamp is read from the reserved field */
                                if (timestamp_ok && port_data[rx_info->port_no].topo.port_ts_in_sync && !ongoing_adjustment) {
                                    release_fdma_buffers = !vtss_ptp_event_rx(ptp_global.ptpi, i, rx_port, &buf_handle, &sender);
                                }
                            } else if (message_type >=8 && message_type <=0xd) {
                                // GENERAL_PDU:
                                T_NG(_I, "GENERAL_PDU: %u bytes, port %u", rx_info->length, rx_port);
                                release_fdma_buffers = !vtss_ptp_general_rx(ptp_global.ptpi, i, rx_port, &buf_handle, &sender);
                            } else {
                                T_EG(_I, "Invalid message type %d received", message_type);
                            }
                        } else {
                            T_IG(_I,"Not Accepted Vlan tag: tag %x vid %d,", rx_info->tag_type, rx_info->tag.vid);
                        }
                    } else {
                        T_IG(_I, "Cmlds not enabled on port %d.", rx_port);
                    }
                }
            }
        }
#if defined (VTSS_SW_OPTION_P802_1_AS)
        }
#endif
        PTP_CORE_UNLOCK();
    } else {
        T_EG(_I, "Invalid message length %u or etype %04x received", rx_info->length, etype);
    }

    return true; // Don't allow other subscribers to receive the packet
}

/****************************************************************************
 * PTP callouts - vtss_ptp_packet_callout.h
 ****************************************************************************/
// This macro must *not* evaluate to an empty macro, since it will be called an expecting to do useful stuff.
#define PARAMETER_CHECK(x, y) do {if (!(x)) {T_E("Assertion failed: " #x); y}} while (0)

/* If 'tc' is true, then packet is meant to be forwarded like transparent clock and hence, any additional TLVs in packet also will be forwarded. Otherwise,
   while sending response, TLVs in the received packet will not be copied into response. Currently, apart from unicast transmission related TLVs and 802.1as
   related organization extension, path trace TLVs, we do not support any other TLVs. */
size_t vtss_1588_prepare_tx_buffer(ptp_tx_buffer_handle_t *tx_buf, u32 length, bool tc)
{
    u8 * frame;
    size_t fr_size = length + tx_buf->header_length + 4;

    if (tx_buf->handle == 0) {
        if (tc && (tx_buf->size > fr_size)) {
            fr_size = tx_buf->size + 4;
        }
        frame = packet_tx_alloc(fr_size); // received packet length  may be up to 4 bytes longer than expected //
        if (frame) {
            if (tc || (tx_buf->size <= fr_size)) {
                memcpy(frame, tx_buf->frame, tx_buf->size);
            } else {
                memcpy(frame, tx_buf->frame, fr_size);
            }
            tx_buf->frame = frame;
            T_I("allocated a new buffer of size %u for transmission", fr_size - 4);
            return fr_size - 4;
        } else {
            tx_buf->frame = 0;
            T_E("new buffer allocation adr %p, old size " VPRIz, frame, tx_buf->size);
            if (frame) packet_tx_free(frame);
            return 0;
        }
    } else {
        return tx_buf->size;
    }
}

size_t vtss_1588_prepare_general_packet(u8 **frame, vtss_appl_ptp_protocol_adr_t *receiver, size_t size, size_t *header_size, int instance)
{
    // Check parameters
    PARAMETER_CHECK(frame != NULL, return 0;);
    *frame = packet_tx_alloc(size + header_size_ip4);
    if (*frame) {
        *header_size = vtss_1588_pack_encap_header(*frame, NULL, receiver,  size, false, instance);
        return size + *header_size;
    } else {
        return 0;
    }
}

size_t vtss_1588_prepare_general_packet_2(u8 **frame, vtss_appl_ptp_protocol_adr_t *sender, vtss_appl_ptp_protocol_adr_t *receiver, size_t size, size_t *header_size, int instance)
{
    // Check parameters
    PARAMETER_CHECK(frame != NULL, return 0;);
    *frame = packet_tx_alloc(size + header_size_ip4);
    if (*frame) {
        *header_size = vtss_1588_pack_encap_header(*frame, sender, receiver,  size, false, instance);
        return size + *header_size;
    } else {
        return 0;
    }
}

void vtss_1588_release_general_packet(u8 **handle)
{
    // Check parameters
    PARAMETER_CHECK(handle != NULL, return;);
    PARAMETER_CHECK(*handle != NULL, return;); /* no buffer allocated */
    packet_tx_free(*handle);
    *handle = NULL;
}

size_t vtss_1588_packet_tx_alloc(void **handle, u8 **frame, size_t size)
{
    // Check parameters
    PARAMETER_CHECK(handle != NULL, return 0;);
    PARAMETER_CHECK(frame != NULL, return 0;);
    PARAMETER_CHECK(*handle == NULL, return 0;); /* buffer already allocated */

    *frame  = packet_tx_alloc(size);
    *handle = *frame;

    if (*frame) {
        return size;
    } else {
        return 0;
    }
}

void vtss_1588_packet_tx_free(void **handle)
{
    // Check parameters
    PARAMETER_CHECK(handle != NULL, return;);
    PARAMETER_CHECK(*handle != NULL, return;); /* no buffer allocated */
    packet_tx_free((u8 *)*handle);
    *handle = NULL;
}

/**
 * vtss_1588_tx_general - Transmit a general message.
 */
size_t vtss_1588_tx_general(u64 port_mask, u8 *frame, size_t size, vtss_ptp_tag_t *tag)
{
    packet_tx_props_t tx_props;

    T_RG_HEX(_I, frame, size);
    T_NG(_I, "Portmask " VPRI64x " , tx %zd bytes", port_mask, size);
    if (port_mask) {
        packet_tx_props_init(&tx_props);
        tx_props.packet_info.modid     = VTSS_MODULE_ID_PTP;
        tx_props.packet_info.frm       = frame;
        tx_props.packet_info.len       = size;
        tx_props.tx_info.dst_port_mask = port_mask;
        tx_props.tx_info.tag.vid       = tag->vid;
        tx_props.tx_info.tag.pcp       = tag->pcp;
        tx_props.tx_info.tag.tpid      = mesa_cap_packet_auto_tagging ? 0 : tag->tpid;

        T_IG(_I, "Tag tpid: %x , vid %d , pcp %d", tx_props.tx_info.tag.tpid, tx_props.tx_info.tag.vid, tx_props.tx_info.tag.pcp);

        update_udp_checksum(frame);
        if (packet_tx(&tx_props) == VTSS_RC_OK) {
            return size;
        } else {
            T_EG(_I,"Transmit general on non-port (" VPRI64x ")?", port_mask);
        }
    }
    return 0;
}

/**
 * Send a general or event PTP message.
 *
 * \param port_mask switch port mask.
 * \param ptp_buf_handle ptp transmit buffer handle
 *
 */

static void timestamp_cbx (void *context, u32 port_no, mesa_ts_timestamp_t *ts)
{
    T_DG(_I, "timestamp: port_no %d, ts %u", port_no, ts->ts);
    if (ts->ts_valid) {
        PTP_CORE_LOCK_SCOPE();

        ptp_tx_timestamp_context_t *h = (ptp_tx_timestamp_context_t *)context;
        if (h->cb_ts) {
            /* Converting 32-bit time counter to 64-bit below. */
            h->cb_ts(h->context, l2port_to_ptp_api(port_no), ts->id, ts->ts);
        } else {
            T_WG(_I, "Missing callback: port_no %d", port_no);
        }
    } else {
        T_IG(_I, "Missed tx time: port_no %d", port_no);
    }
}

static mesa_rc allocate_phy_signaturex(u8 *ptp_frm, void *context, u64 port_mask, int inst, bool cmlds)
{
    mesa_rc rc = VTSS_RC_OK;
    if (mepa_phy_ts_cap()) {
        mesa_ts_timestamp_alloc_t alloc_parm;
        mepa_ts_fifo_sig_t        ts_sig = {.crc_src_port = 0, .has_crc_src = false};
        uint16_t port = 0;

        ts_sig.msg_type = ptp_frm[PTP_MESSAGE_MESSAGE_TYPE_OFFSET] & 0xF;
        ts_sig.domain_num = ptp_frm[PTP_MESSAGE_DOMAIN_OFFSET];
        memcpy(ts_sig.src_port_identity, ptp_frm+PTP_MESSAGE_SOURCE_PORT_ID_OFFSET, 10);
        port = vtss_tod_unpack16(&ts_sig.src_port_identity[CLOCK_IDENTITY_LENGTH]) - 1;
        if (cmlds) {
            if (ptp_global.ptp_cm.crcSrcPort[port]) {
                ts_sig.has_crc_src = true;
                ts_sig.crc_src_port = ptp_global.ptp_cm.crcSrcPort[port];
            }
        } else {
            if (crcSrcPortId[inst][port]) {
                ts_sig.has_crc_src = true;
                ts_sig.crc_src_port = crcSrcPortId[inst][port];
            }
        }
        ts_sig.sequence_id = vtss_tod_unpack16(&ptp_frm[PTP_MESSAGE_SEQUENCE_ID_OFFSET]);

        alloc_parm.port_mask = port_mask;
        alloc_parm.context = context;
        alloc_parm.cb = timestamp_cbx;
        rc = tod_mod_man_tx_ts_allocate(&alloc_parm, &ts_sig);
        T_DG(_I,"Timestamp Signature allocated: msg-type %d, domain %d, seq: %d, crc %d port = %d cmlds %d rc = %d",
             ts_sig.msg_type, ts_sig.domain_num, ts_sig.sequence_id, ts_sig.crc_src_port, port, cmlds, rc);
    }
    return rc;
}

void vtss_1588_tx_handle_init(ptp_tx_buffer_handle_t *ptp_buf_handle)
{
    memset(ptp_buf_handle, 0, sizeof(ptp_tx_buffer_handle_t));
}

/**
 * vtss_1588_forw_one_step_event.
 */
typedef struct {
    u8 corr [8];
    u8 reserved[4];
} onestep_forw_t;

mesa_rc vtss_1588_afi(void ** afi, u64 port_mask,
                        ptp_tx_buffer_handle_t *ptp_buf_handle, i8 log_sync_interval, BOOL resume)
{
    if (mesa_cap_synce_ann_auto_transmit) {
        ptp_afi_conf_s* my_afi = (ptp_afi_conf_s*)*afi;
        ptp_afi_setup_t afi_conf;
        mesa_port_no_t port_no = VTSS_PORT_NO_NONE;
        afi_conf.log_sync_interval = log_sync_interval;
        mesa_rc rc = VTSS_RC_OK;
        afi_conf.switch_port = TRUE;
        port_iter_t       pit;
        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit) && port_no == VTSS_PORT_NO_NONE) {
            if (port_mask & (1LL<<pit.iport)) {
                port_no = pit.iport;
                if (port_data[pit.iport].topo.ts_feature == VTSS_PTP_TS_PTS) {
                    afi_conf.switch_port = FALSE;
                }
            }
        }
        if (port_no != VTSS_PORT_NO_NONE) {
            afi_conf.clk_domain = port_data[port_no].port_domain;
            if (resume) {
                if (my_afi == 0) {
                    PTP_RETURN(ptp_afi_alloc(&my_afi, port_no+1));
                }
                *afi = my_afi;
                T_IG(_I, "port_no %d, my_afi %p AFI started", port_no, my_afi);
                return my_afi->ptp_afi_sync_conf_start(port_no, ptp_buf_handle, &afi_conf);
            } else {
                if (my_afi != 0) {
                    T_IG(_I, "AFI stopped");
                    rc = my_afi->ptp_afi_sync_conf_stop();
                    PTP_RC(ptp_afi_free(&my_afi));
                }
                *afi = my_afi;
                return rc;
            }
        } else {
            return VTSS_RC_ERROR;
        }
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc vtss_1588_afi_packet_tx(void ** afi, u32* cnt_delta)
{
    if (mesa_cap_synce_ann_auto_transmit) {
        ptp_afi_conf_s* my_afi = (ptp_afi_conf_s*)*afi;
        PTP_RC(my_afi->ptp_afi_packet_tx(cnt_delta));
        return VTSS_RC_OK;
    } else {
        *cnt_delta = 0;
        return VTSS_RC_ERROR;
    }
}

bool vtss_1588_check_transmit_resources(int instance)
{
    bool available = true;
    if (mesa_cap_synce_ann_auto_transmit) {
        u32 afi_request = 0;

        if (config_data.conf[instance].clock_init.afi_announce_enable) {
            afi_request++;
        }
        if (config_data.conf[instance].clock_init.afi_sync_enable) {
            afi_request++;
        }

        PTP_RC(ptp_afi_available(afi_request, &available));
        T_IG(_I, "instance %d, afi_request %d, available %d", instance, afi_request, available);
    }

    return available;
}

// buf_tsid is assigned with switch timestamping id.
size_t vtss_1588_tx_msg(u64 port_mask,
                        ptp_tx_buffer_handle_t *ptp_buf_handle,
                        int inst, bool cmlds,
                        uint32_t *buf_tsid)
{
    packet_tx_props_t tx_props;
    mesa_rc           rc = VTSS_RC_OK;
    u64               pts_port_mask = 0;
    int               portnum = 0, last_port = 0, port_count = 0;
    u8                *my_buf = (u8 *)ptp_buf_handle->frame + ptp_buf_handle->header_length;
    u8                message_type = my_buf[PTP_MESSAGE_MESSAGE_TYPE_OFFSET] & 0xF;

    T_RG_HEX(_I, ptp_buf_handle->frame, ptp_buf_handle->size);
    T_IG(_I, "Portmask " VPRI64x " , tx %zd bytes", port_mask, ptp_buf_handle->size);
    if (port_mask) {
        packet_tx_props_init(&tx_props);
        tx_props.packet_info.modid     = VTSS_MODULE_ID_PTP;
        tx_props.packet_info.frm       = ptp_buf_handle->frame;
        tx_props.packet_info.len       = ptp_buf_handle->size;
        tx_props.tx_info.dst_port_mask = port_mask;
        tx_props.packet_info.no_free   = ptp_buf_handle->handle != NULL;
        tx_props.tx_info.tag.vid       = ptp_buf_handle->tag.vid;
        tx_props.tx_info.tag.pcp       = ptp_buf_handle->tag.pcp;
        if (mesa_cap_packet_inj_encap) {
            tx_props.tx_info.inj_encap = ptp_buf_handle->inj_encap;
            // Based on type of injection encapsulation, chip would take care of identifying
            // ptp header offset on top of ethernet header. So, pdu_offset must be ethernet header length.
            tx_props.tx_info.pdu_offset = ETH_HDR_LEN;
        } else {
            tx_props.tx_info.inj_encap.type = MESA_PACKET_ENCAP_TYPE_NONE;
            /* Jaguar 2 needs to know the PTP header offset within the packet */
            // Chip does not know offset of PTP header within packet. So, ptp header offset from
            // the beginning of the packet must be given as pdu_offset.
            tx_props.tx_info.pdu_offset = ptp_buf_handle->header_length;
        }
        tx_props.tx_info.tag.tpid      = mesa_cap_packet_auto_tagging ? 0 : ptp_buf_handle->tag.tpid; /* In Serval or Jaguar2: must be 0 otherwise the FDMA inserts a tag, and the so do the Switch */

        T_IG(_I, "Tag tpid: %x , vid %d , pcp %d, offset %u", tx_props.tx_info.tag.tpid, tx_props.tx_info.tag.vid, tx_props.tx_info.tag.pcp, tx_props.tx_info.pdu_offset);

        /* split port mask into PHY TS ports and non PHY TS ports */
        // phy timestamping exists in clock domain 0 only.
        // software clock can run in hardware clock domain 0 only currently.
        if (cmlds || config_data.conf[inst].clock_init.cfg.clock_domain == 0 ||
            config_data.conf[inst].clock_init.cfg.clock_domain >= mesa_cap_ts_domain_cnt) {
            port_iter_t       pit;
            port_iter_init_local(&pit);
            while (port_iter_getnext(&pit)) {
                portnum = pit.iport;
                if (port_mask & (1LL<<portnum)) {
                    port_count++;
                    last_port = portnum;
                    if (port_data[portnum].topo.ts_feature == VTSS_PTP_TS_PTS) {
                        pts_port_mask |= 1LL<<portnum;
                        port_mask &= ~(1LL<<portnum);
                        tx_props.tx_info.inj_encap.type = MESA_PACKET_ENCAP_TYPE_NONE;
                    }
                }
            }
            T_DG(_I,"port_count %d, last port is %d phy_port_mask %d", port_count, last_port, pts_port_mask);
        } else if (config_data.conf[inst].clock_init.cfg.clock_domain) {
            tx_props.tx_info.ptp_domain = config_data.conf[inst].clock_init.cfg.clock_domain;
        }
        switch (ptp_buf_handle->msg_type) {
            case VTSS_PTP_MSG_TYPE_GENERAL:
                T_IG(_I, "general packet");
                break;

            case VTSS_PTP_MSG_TYPE_2_STEP:
                T_IG(_I, "2-step packet Clk-domain %d", tx_props.tx_info.ptp_domain);

                if (pts_port_mask) {
                    /* packet transmitted to PTS ports */
                    rc = allocate_phy_signaturex(ptp_buf_handle->frame + ptp_buf_handle->header_length, ptp_buf_handle->ts_done, pts_port_mask, inst, cmlds);
                }
                if (port_mask) {
                    if (mesa_cap_ts) {
                        mesa_ts_timestamp_alloc_t alloc_parm;
                        alloc_parm.port_mask = port_mask;
                        alloc_parm.context = ptp_buf_handle->ts_done;
                        alloc_parm.cb = timestamp_cbx;
                        mesa_ts_id_t ts_id;
                        rc = mesa_tx_timestamp_idx_alloc(0, &alloc_parm, &ts_id); /* allocate id for transmission*/
                        T_DG(_I,"Timestamp Id (%u)allocated", ts_id.ts_id);
                        tx_props.tx_info.ptp_action    = MESA_PACKET_PTP_ACTION_TWO_STEP; /* twostep action */
                        if (mesa_cap_ts_twostep_use_ptp_id) {
                            tx_props.tx_info.ptp_id        = ts_id.ts_id;
                        } else {
                            tx_props.tx_info.ptp_timestamp = ts_id.ts_id << 16;
                        }
                        // Assign back ts_id for only switch timestamping.
                        if (!pts_port_mask) {
                            *buf_tsid = ts_id.ts_id;
                        }
                    }
                }
                break;

            case VTSS_PTP_MSG_TYPE_CORR_FIELD:
                T_IG(_I, "corr field packet");
                if (!pts_port_mask) {
                    /* link asymmetry compensation for DelayReq and PDelayReq is done in SW for switch ports */
                    portnum = 0;
                    while ((port_mask & (1LL<<portnum)) == 0 && portnum < mesa_cap_port_cnt) {
                        portnum++;
                    }
                    if (portnum >= mesa_cap_port_cnt)  {
                        portnum = mesa_cap_port_cnt - 1;
                        T_WG(_I, "invalid portnum");
                    }
                    if ((message_type == PTP_MESSAGE_TYPE_DELAY_REQ || message_type == PTP_MESSAGE_TYPE_P_DELAY_REQ) &&
                    port_data[portnum].asymmetry_cnt != 0) {
                        vtss_1588_ts_cnt_add(&ptp_buf_handle->hw_time, ptp_buf_handle->hw_time, port_data[portnum].asymmetry_cnt);
                    }
                }
                T_DG(_I, "one step packet, hw_time " VPRI64d, ptp_buf_handle->hw_time);
                // Use MESA_PACKET_PTP_ACTION_ONE_STEP when switch timestamping is enabled or
                // when phy timestamping is used in combination with switch timestamping.
                if (!(pts_port_mask && !vtss_appl_ptp_combined_phy_switch_ts)) {
                    tx_props.tx_info.ptp_action = MESA_PACKET_PTP_ACTION_ONE_STEP; /* onestep action */
                }
                tx_props.tx_info.ptp_timestamp      = ptp_buf_handle->hw_time; /* used for correction field update */
                break;

            case VTSS_PTP_MSG_TYPE_ORG_TIME:
                T_IG(_I, "org time packet");
                if (!pts_port_mask) {
                    if (mesa_cap_ts_org_time) {
                        tx_props.tx_info.ptp_action = MESA_PACKET_PTP_ACTION_ORIGIN_TIMESTAMP;  /* origin PTP action */
                        T_IG(_I, "HW sets preciseOriginTimestamp in the packet");
                    } else {
                        T_EG(_I, "org_time is not supported");
                        rc = VTSS_RC_ERROR;
                    }
                }
                break;

            default:
                T_EG(_I, "invalid message type");
                break;
        }

        if (rc == VTSS_RC_OK) {
            update_udp_checksum(ptp_buf_handle->frame);
            T_IG(_I,"packet tx, ptp_action = %d",tx_props.tx_info.ptp_action);

            if (packet_tx(&tx_props) == VTSS_RC_OK) {
                // Check to see if there is a timestamp ready for us - if needed.
                if (ptp_buf_handle->msg_type == VTSS_PTP_MSG_TYPE_2_STEP && mesa_cap_ts_missing_tx_interrupt) {
                    // Two-step transmission w/o timestamp interrupt, so do the
                    // timestamp update here.
                    T_DG(_I,"update timestamps");
                    PTP_CORE_UNLOCK();  // Temporarily unlock PTP core - locking again below.
                    rc = mesa_tx_timestamp_update(nullptr);
                    PTP_CORE_LOCK();
                    PTP_RC(rc);
                }

                return ptp_buf_handle->size;
            } else {
                T_EG(_I,"Transmit message on non-port (" VPRI64x ")?", port_mask);
            }
        } else {
            T_WG(_I,"Could not get a timestamp ID msg_type %d ", message_type);
        }
    }

    return 0;
}

static int ptp_sock = 0;

static void ptp_socket_init(void)
{
    struct sockaddr_in sender;
    u16 port = PTP_GENERAL_PORT;
    int length, retry = 0;

    if (ptp_sock == 0)  {
        // this command tells the IP stack to drop PTP over IP messages. This is done to avoid ICMP 'unreachable' messages.
        // this operation takes more than 100 ms, therefore it shall only be done once
        system("iptables -A INPUT -p udp -m multiport --dports 319,320 -j DROP");
    }
    if (ptp_sock > 0) close(ptp_sock);

    ptp_sock = vtss_socket(AF_INET, SOCK_DGRAM, 0);
    if (ptp_sock < 0) {
        T_EG(_I, "socket returned %d",ptp_sock);
        return;
    }
    length = sizeof(sender);
    bzero(&sender,length);
    sender.sin_family=AF_INET;
    sender.sin_addr.s_addr=INADDR_ANY;
    vtss_tod_pack16(port,(u8*)&sender.sin_port);
    T_IG(_I, "binding socket");
    while (bind(ptp_sock, (struct sockaddr *)&sender, length) < 0 ) {
        if (++retry > 30) {
            T_EG(_I, "binding error, err_no %d",errno);
            break;
        }
        T_IG(_I, "binding problem, err_no %d",errno);
        VTSS_OS_MSLEEP(1000); /* 1 sec */
    }
}

static void ptp_socket_close(void)
{
    if (ptp_sock > 0) {
        if (close(ptp_sock) == 0) ptp_sock = 0;
    }
}

size_t vtss_1588_tx_unicast_request(u32 dest_ip, const void *buffer, size_t size, int instance)
{
    struct sockaddr_in receiver;
    char buf1[20];
    int n;
    int length;
    struct sockaddr_in my_addr;
    socklen_t my_addr_len;
    u16 port = PTP_GENERAL_PORT;

    if (ptp_global.my_ip[instance].s_addr == 0) {
        T_IG(_I,"cannot send before my ip address is defined");
        return 0;
    }

    T_IG(_I, "Dest_ip %s, size " VPRIz, misc_ipv4_txt(dest_ip, buf1), size);
    //packet_dump(buffer, size);

    memset(&receiver, 0, sizeof(struct sockaddr_in));
    receiver.sin_family = AF_INET;
    vtss_tod_pack32(dest_ip,(u8*)&receiver.sin_addr);
    vtss_tod_pack16(port,(u8*)&receiver.sin_port);
    length = sizeof(struct sockaddr_in);

    /* Connect socket */
    if (connect(ptp_sock, (struct sockaddr *)&receiver, length) != 0) {
        T_IG(_I, "Failed to connect: %d - %s", errno, strerror(errno));
        return 0;
    }

    /* Get my address */
    my_addr_len = sizeof(my_addr);
    if (getsockname(ptp_sock, (struct sockaddr *)&my_addr, &my_addr_len) != 0) {
        T_WG(_I, "getsockname failed: %d - %s", errno, strerror(errno));
        return 0;
    }
    if (my_addr.sin_family != AF_INET) {
        T_WG(_I, "Got something else than as an IPv4 address: %d", my_addr.sin_family);
        return 0;
    }
    T_IG(_I, "Source_ip %s, size " VPRIz, misc_ipv4_txt(my_addr.sin_addr.s_addr, buf1), size);

    n = write(ptp_sock,buffer,size);
    if (n < 0) {
        T_IG(_I, "write returned %d", n);
        if (errno == EHOSTUNREACH) {
            T_I("Error no: %d, error message: %s", errno, strerror(errno));
        } else {
            T_W("Error no: %d, error message: %s", errno, strerror(errno));
        }
        return 0;
    }
    return n;
}

/****************************************************************************/
/*  Reserved ACEs functions and MAC table setup                             */
/****************************************************************************/
/* If all ports have 1588 PHY, no ACL rules are needed for timestamping */
/* instead the forwarding is set up in the MAC table */
static bool ptp_external_phys_get(void)
{
    bool ext_phy = true;
    int i, j;
    port_iter_t       pit;
    T_IG(VTSS_TRACE_GRP_ACE,"ext_phy before %d", ptp_all_external_port_phy_ts);
    for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        if (config_data.conf[i].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            /* check if PHY timetsamper esists */
            port_iter_init_local(&pit);
            while (port_iter_getnext(&pit)) {
                j = pit.iport;
                if (config_data.conf[i].port_config[j].enabled && port_data[j].topo.ts_feature != VTSS_PTP_TS_PTS &&
                    (!config_data.conf[i].port_config[j].portInternal || mesa_cap_ts_c_dtc_supported))
                {
                    ext_phy = false;
                    T_DG(VTSS_TRACE_GRP_ACE,"port %d, enabled %d, internal %d, ts_feature %d", pit.uport,
                         config_data.conf[i].port_config[j].enabled,
                         config_data.conf[i].port_config[j].portInternal,
                         port_data[j].topo.ts_feature);
                }
            }
        }
    }
    T_IG(VTSS_TRACE_GRP_ACE,"ext_phy %d", ext_phy);
    return ext_phy;
}

/* To accomodate multi protocol encapsulation, all the protocols related to encapsulation are returned in the array ace_types. */
bool conf_type(int protocol, mesa_ace_type_t ace_types[], int size, int *proto_num)
{
    int i = 0;
    if ((protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI) || (protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED) || (protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) || (protocol == VTSS_APPL_PTP_PROTOCOL_ANY))
        ace_types[(i < size) ? i++ : i] = MESA_ACE_TYPE_IPV4;

    if ((protocol == VTSS_APPL_PTP_PROTOCOL_IP6MIXED) || (protocol == VTSS_APPL_PTP_PROTOCOL_ANY))
        ace_types[(i < size) ? i++ : i] = MESA_ACE_TYPE_IPV6;

    /* For any other encapsulation type, set it as ethernet */
    if ((protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET) || (protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED) ||
        (protocol == VTSS_APPL_PTP_PROTOCOL_OAM) || (protocol == VTSS_APPL_PTP_PROTOCOL_ONE_PPS) || (protocol == VTSS_APPL_PTP_PROTOCOL_ANY))
        ace_types[(i < size) ? i++ : i] = MESA_ACE_TYPE_ETYPE;

    *proto_num = i;

    return i > 1;
}

static void acl_rules_init(void)
{
    int i,j;
    for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        rules[i].domain = 0;
        rules[i].deviceType = VTSS_APPL_PTP_DEVICE_NONE;
        rules[i].protocol = VTSS_APPL_PTP_PROTOCOL_ETHERNET;
        for (j = 0; j < vtss_appl_cap_max_acl_rules_pr_ptp_clock; j++) {
            rules[i].id[j] = ACL_MGMT_ACE_ID_NONE;
        }
    }
}

/*
 * ACL rule_id's are allocated this way:
 * if no TC instance exists, then the rules are inserted as last entries
 * if a tc exists, then the rules are inserted before the TC's rules.
 * the first TC rule id is saved in my_next_id.
 * This is because the BC's are more specific than the TC rules.
 * If a TC exists, that forwards all packets in HW, then my_tc_encapsulation is set to the TC encapsulation type
 * If this type of TC exists, and a SlaveOnly instance also exists, this slave is used for syntonization of the TC,
 * and the packets forwarded to the CPU must also be forwarded, i.e. the ACL rules for this slaveOnly instance depends
 * on the TC configuration. (my_tc_encapsulation is defined and described earlier in this file)
 */
static mesa_ace_id_t my_next_id = ACL_MGMT_ACE_ID_NONE;                  /* indicates that no TC exists */

static mesa_rc acl_rule_apply(acl_entry_conf_t    *conf, int instance, int rule_id)
{
    if (rule_id >= vtss_appl_cap_max_acl_rules_pr_ptp_clock) {
        T_WG(VTSS_TRACE_GRP_ACE,"too many ACL rules");
        return VTSS_RC_ERROR;
    }
    conf->id = rules[instance].id[rule_id];/* if id == ACL_MGMT_ACE_ID_NONE, it is auto assigned, otherwise it is reused */
    T_DG(VTSS_TRACE_GRP_ACE,"acl_mgmt_ace_add instance: %d, rule idx %d, conf->id %d, my_next_id %d", instance, rule_id, conf->id, my_next_id);
    PTP_RETURN(acl_mgmt_ace_add(ACL_USER_PTP, my_next_id, conf));
    if (conf->conflict) {
        T_WG(VTSS_TRACE_GRP_ACE,"acl_mgmt_ace_add conflict instance %d, rule %d", instance, rule_id);
        return PTP_RC_MISSING_ACL_RESOURCES;
    } else {
        rules[instance].id[rule_id] = conf->id;
    }
    T_DG(VTSS_TRACE_GRP_ACE,"ACL rule_id %d, conf->id %d", rule_id, conf->id);
    return VTSS_RC_OK;
}

static mesa_rc ptp_auto_resp_setup(int i, acl_entry_conf_t *pconf, int *rule_no) {
    port_iter_t       pit;
    int j;
    mesa_ace_ptp_t *pptp;
    uint8_t         clock_domain = config_data.conf[i].clock_init.cfg.clock_domain;

    // software clock domains use chip clock domain 0
    if (clock_domain >= mesa_cap_ts_domain_cnt) {
        clock_domain = 0;
    }

    // in Jr2 the DelayReq/DelayResp is handled in HW
    // set up the clockId for the domain
    acl_entry_conf_t my_conf = *pconf;      //make my own copy og the configuration before modifying it
    mesa_ts_autoresp_dom_cfg_t autoresp_cfg;
    autoresp_cfg.ptp_port_individual = TRUE;
    autoresp_cfg.ptp_port_msb = 7;       /* to be fixed */
    memcpy(autoresp_cfg.clock_identity, config_data.conf[i].clock_init.clockIdentity, VTSS_CLOCK_IDENTITY_LENGTH);
    autoresp_cfg.flag_field_update.value = 0x00;  /*clear flags */
    autoresp_cfg.flag_field_update.mask = 0x0b;  /*except unicast flag which is kept unchanged */
    PTP_RC(mesa_ts_autoresp_dom_cfg_set(0, clock_domain, &autoresp_cfg));

    mesa_port_list_t rsp_port_list;

    if (my_conf.type == MESA_ACE_TYPE_ETYPE) {
        pptp = &my_conf.frame.etype.ptp;
    } else if (my_conf.type == MESA_ACE_TYPE_IPV4) {
        pptp = &my_conf.frame.ipv4.ptp;
    } else {
        T_WG(VTSS_TRACE_GRP_ACE,"unsupported ACL frame type %d", my_conf.type);
        return PTP_RC_UNSUPPORTED_ACL_FRAME_TYPE;
    }
    // set up Source IP address used in auto response
    mesa_acl_sip_conf_t sip;
    sip.sip.type = MESA_IP_TYPE_IPV4;
    sip.sip.addr.ipv4 = ptp_global.my_ip[i].s_addr;

    PTP_RC(mesa_acl_sip_conf_set(0, i, &sip));
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        j = pit.iport;
        rsp_port_list[j] = false;
    }
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        j = pit.iport;
        if (rules[i].port_list[j]) {
            rsp_port_list[j] = true;
            my_conf.action.force_cpu = false;
            my_conf.action.port_action = MESA_ACL_PORT_ACTION_REDIR;
            my_conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP;
            if (port_data[j].topo.ts_feature == VTSS_PTP_TS_PTS &&
                port_data[j].port_domain == 0) {
                my_conf.action.ptp.response = MESA_ACL_PTP_RSP_DLY_REQ_RSP_NO_TS;
                my_conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
            } else {
                my_conf.action.ptp.response = MESA_ACL_PTP_RSP_DLY_REQ_RSP_TS_UPD;
            }
            my_conf.action.ptp.log_message_interval = config_data.conf[i].port_config[j].logMinPdelayReqInterval; // message interval returned in the dely_resp
            my_conf.action.ptp.copy_smac_to_dmac = 0;  //SMAC/DMAC handling is controlled by my_conf.action.addr.update
            my_conf.action.ptp.set_smac_to_port_mac = 0;
            my_conf.action.ptp.dom_sel = clock_domain;   // HW domain
            my_conf.action.addr.sip_idx = i;
            my_conf.action.ptp.sport = 320;
            my_conf.action.ptp.dport = 320;
            my_conf.action.addr.update = MESA_ACL_ADDR_UPDATE_MAC_IP_SWAP_UC;   //TBD
            memcpy(&my_conf.action.addr.mac, &ptp_global.sysmac, sizeof(mesa_mac_t));
            pptp->header.mask[0] = 0x0f;
            pptp->header.value[0] = 0x01; /* messageType = [1] */
            my_conf.port_list = rsp_port_list;
            my_conf.action.port_list = rsp_port_list;
            my_conf.vid.value = config_data.conf[i].clock_init.cfg.configured_vid;
            my_conf.vid.mask = 0xfff;

            rsp_port_list[j] = false;
            // If multicast address is link-local use the application response instead
            if (config_data.conf[i].port_config[j].dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT) {
                T_IG(VTSS_TRACE_GRP_ACE,"inst %d: autoresponse applied for port %d, vid %d", i, j, my_conf.vid.value);
                PTP_RETURN(acl_rule_apply(&my_conf, i, (*rule_no)++));
            }
        }
    }
    return VTSS_RC_OK;
}

static mesa_rc _ptp_ace_update(int i, CapArray<bool, VTSS_APPL_CAP_PTP_CLOCK_CNT>& ptp_inst_updated)
{
    mesa_rc             rc = VTSS_RC_OK;
    acl_entry_conf_t    conf = {};
    int rule_no = ACL_MGMT_ACE_ID_NONE;
    port_iter_t       pit;
    int portlist_size;
    mesa_ace_ptp_t *pptp = 0;
    u32 mac_idx;
    u32 inst, idx;
    char dest_txt[100];
    int j;
    CapArray<bool, VTSS_APPL_CAP_PTP_CLOCK_CNT> refresh_inst;  // Note: All elements initialize to 0 i.e. FALSE by constructor
    uint8_t clock_domain = config_data.conf[i].clock_init.cfg.clock_domain;

    // software clock domains use clock domain 0 for timestamping.
    if (clock_domain >=  mesa_cap_ts_domain_cnt) {
        clock_domain = 0;
    }

    // Check if PTP instance has already been updated. If this is the case simply return with VTSS_RC_OK
    if (ptp_inst_updated[i]) return VTSS_RC_OK;
    ptp_inst_updated[i] = true;  // Make sure this PTP instance will only be updated once.

    if ((config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY ||
         config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) &&
         config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM) {
        return rc;
    }
    if ((config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY) &&
            config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_ONE_PPS) {
        return rc;
    }
    // If the clock is a boundary clock then:  ptp_all_external_port_phy_ts is set to true if all external PTP ports have timestamp PHY and none of them are using "two step mode".
    // else:                                   ptp_all_external_port_phy_ts is set to true if all external PTP ports have timestamp PHY and clock is not configured to use "two step mode" (ignoring setting at port level).
    if ((rules[i].deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) || (rules[i].deviceType == VTSS_APPL_PTP_DEVICE_AED_GM) || (rules[i].deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY)) {
        bool a_port_uses_two_step = false;
        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            if (config_data.conf[i].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE &&
                ((config_data.conf[i].clock_init.cfg.twoStepFlag && !(config_data.conf[i].port_config[pit.iport].twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE)) ||
                (config_data.conf[i].port_config[pit.iport].twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE)))
            {
                a_port_uses_two_step = true;
            }
        }
        ptp_all_external_port_phy_ts = ptp_external_phys_get() && !a_port_uses_two_step && !clock_domain;
    }
    else {
        ptp_all_external_port_phy_ts = ptp_external_phys_get() && !config_data.conf[i].clock_init.cfg.twoStepFlag && !clock_domain;
    }
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        PTP_RETURN(ptp_phy_ts_update(i, pit.iport));
    }
#ifdef PHY_DATA_DUMP
    phy_ts_dump();
#endif
    if (rules[i].deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
        /* Remove rules for this clock instance */
        /* If All ports have 1588 PHY, forwarding is set up in the MAC table */
        for (mac_idx = 0; mac_idx < PTP_MAC_ENTRIES; mac_idx++) {
            if (mac_idx < mac_used) {
                PTP_RC(mesa_mac_table_del(NULL,&mac_entry[mac_idx].vid_mac));
                T_IG(VTSS_TRACE_GRP_ACE,"deleting mac table entry: mac %s, vid %d, copy to cpu %d", misc_mac2str(mac_entry[mac_idx].vid_mac.mac.addr),
                    mac_entry[mac_idx].vid_mac.vid, mac_entry[mac_idx].copy_to_cpu);
            }
            vtss_clear(mac_entry[mac_idx]);
        }
        if (mac_used > 0 && !ptp_all_external_port_phy_ts) {
            // mode is changed from mac table to ACL table, therefore other instances must be refreshed
            for (int ii = 0; ii < PTP_CLOCK_INSTANCES; ii++) {
                if (i != ii && rules[ii].deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
                    refresh_inst[ii] = true;
                    T_IG(VTSS_TRACE_GRP_ACE,"deleting mac table entry, therefore refresh instance %d", ii);
                }
            }
        }
        mac_used = 0;
        if (rules[i].deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT || rules[i].deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT) {
            my_next_id = ACL_MGMT_ACE_ID_NONE;
            if (i == my_tc_instance || i == my_2step_tc_instance) {
                for (int ii = 0; ii < PTP_CLOCK_INSTANCES; ii++) {
                    if (i != ii && protocol_level(rules[i].protocol) == protocol_level(rules[ii].protocol)) {
                        refresh_inst[ii] = true;
                    }
                }
            }
            my_tc_encapsulation = VTSS_APPL_PTP_PROTOCOL_MAX_TYPE;   /* indicates that no forwarding in the SlaveOnly instance is needed */
            my_tc_instance = -1;
            my_2step_tc_instance = -1;
        }
        rules[i].deviceType = VTSS_APPL_PTP_DEVICE_NONE;
        for (j = 0; j < vtss_appl_cap_max_acl_rules_pr_ptp_clock; j++) {
            if (rules[i].id[j] != ACL_MGMT_ACE_ID_NONE) {
                PTP_RC(acl_mgmt_ace_del(ACL_USER_PTP, rules[i].id[j]));
                rules[i].id[j] = ACL_MGMT_ACE_ID_NONE;
            }
            T_IG(VTSS_TRACE_GRP_ACE,"clear ACL rules for inst %d", i);
        }
    }

    if (config_data.conf[i].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
        /* Set up rules for this instance */
        rules[i].deviceType = config_data.conf[i].clock_init.cfg.deviceType;
        rules[i].protocol = config_data.conf[i].clock_init.cfg.protocol;
        rules[i].domain = config_data.conf[i].clock_init.cfg.domainNumber;
        if (rules[i].protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI && (ptp_global.my_ip[i].s_addr == 0)) {
            T_IG(VTSS_TRACE_GRP_ACE,"cannot set up unicast ACL before my ip address is defined");
            return PTP_RC_MISSING_IP_ADDRESS;
        }
        rules[i].internal_port_exists = 0;
        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            j = pit.iport;
            rules[i].port_list[j] = config_data.conf[i].port_config[j].enabled;
            rules[i].external_port_list[j] = config_data.conf[i].port_config[j].enabled && !config_data.conf[i].port_config[j].portInternal;
            rules[i].internal_port_list[j] = config_data.conf[i].port_config[j].portInternal;
            rules[i].internal_port_exists |= config_data.conf[i].port_config[j].portInternal;
            T_DG(VTSS_TRACE_GRP_ACE,"inst %d, port %d, list %d, xlist %d, ilist %d", i, j, rules[i].port_list.get(j), rules[i].external_port_list.get(j), rules[i].internal_port_list.get(j));
        }
        T_DG(VTSS_TRACE_GRP_ACE,"inst %d, internal port exists %d", i, rules[i].internal_port_exists);

        T_IG(_I, "ace_init protocol: %d", rules[i].protocol);
        mesa_ace_type_t  ace_types[4];
        memset(ace_types, 0, sizeof(ace_types));
        int proto_type = 0, proto_num = 0;
        bool multi_proto = conf_type(rules[i].protocol, ace_types, sizeof(ace_types), &proto_num);
        do {
            /* For 'VTSS_APPL_PTP_PROTOCOL_ANY' encapsulation, we need to configure ACL rules for Eth, IPv4 and IPv6. So, loop is added here so that all the protocols
               related to encapsulation are configured in ACL rules. */
            if ((rc = acl_mgmt_ace_init(ace_types[proto_type], &conf)) != VTSS_RC_OK) {
                return rc;
            }
            conf.action.cpu_queue = PACKET_XTR_QU_BPDU;  /* increase extraction priority to the same as bpdu packets */
            VTSS_BF_SET(conf.flags.value, ACE_FLAG_IP_FRAGMENT, 0);  /* ignore IPV4 fragmented packets */
            VTSS_BF_SET(conf.flags.mask, ACE_FLAG_IP_FRAGMENT, 1);
            conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
            memset(conf.action.port_list, 0, sizeof(conf.action.port_list));
            conf.action.logging = false;
            conf.action.shutdown = false;
            conf.action.cpu_once = false;
            conf.isid = VTSS_ISID_LOCAL;
            if (conf.type == MESA_ACE_TYPE_ETYPE) {
                conf.frame.etype.etype.value[0] = (ptp_ether_type >> 8) & 0xff;
                conf.frame.etype.etype.value[1] = ptp_ether_type & 0xff;
                conf.frame.etype.etype.mask[0] = 0xff;
                conf.frame.etype.etype.mask[1] = 0xff;
            } else if (conf.type == MESA_ACE_TYPE_IPV4) {
                conf.frame.ipv4.proto.value = 17; //UDP
                conf.frame.ipv4.proto.mask = 0xFF;
                conf.frame.ipv4.sport.in_range = conf.frame.ipv4.dport.in_range = true;
                conf.frame.ipv4.sport.low = 0;
                conf.frame.ipv4.sport.high = 65535;
                conf.frame.ipv4.dport.low = 319;
                conf.frame.ipv4.dport.high = 320;
                if ((VTSS_APPL_PTP_DEVICE_ORD_BOUND == rules[i].deviceType || VTSS_APPL_PTP_DEVICE_AED_GM == rules[i].deviceType || VTSS_APPL_PTP_DEVICE_MASTER_ONLY  == rules[i].deviceType ||
                        VTSS_APPL_PTP_DEVICE_SLAVE_ONLY == rules[i].deviceType) && (rules[i].protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI)) {
                    /* in unicast mode only terminate PTP messages with my address */
                    conf.frame.ipv4.dip.value = ptp_global.my_ip[i].s_addr;
                    conf.frame.ipv4.dip.mask = 0xffffffff;
                }
            } else if (conf.type == MESA_ACE_TYPE_IPV6) {
                conf.frame.ipv6.proto.value = 17; //UDP
                conf.frame.ipv6.proto.mask = 0xff;
                /* Currently, only one step E2E transparent clock is supported over ipv6. */
                memset(&conf.frame.ipv6.sip, 0, sizeof(conf.frame.ipv6.sip));
                conf.frame.ipv6.sport.in_range = conf.frame.ipv6.dport.in_range = true;
                conf.frame.ipv6.sport.low = 0;
                conf.frame.ipv6.sport.high = 65535;
                conf.frame.ipv6.dport.low = 319;
                conf.frame.ipv6.dport.high = 319; //sufficient for only one step clock.
                conf.frame.ipv6.tcp_fin = MESA_ACE_BIT_0;
                conf.frame.ipv6.tcp_syn = MESA_ACE_BIT_0;
                conf.frame.ipv6.tcp_rst = MESA_ACE_BIT_0;
                conf.frame.ipv6.tcp_psh = MESA_ACE_BIT_0;
                conf.frame.ipv6.tcp_ack = MESA_ACE_BIT_0;
                conf.frame.ipv6.tcp_urg = MESA_ACE_BIT_0;
            }
            if (!ptp_all_external_port_phy_ts) {
                portlist_size = sizeof(conf.port_list);
                memcpy(conf.port_list, rules[i].port_list, portlist_size);
                conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
                if (conf.type == MESA_ACE_TYPE_ETYPE) {
                    pptp = &conf.frame.etype.ptp;
                } else if (conf.type == MESA_ACE_TYPE_IPV4) {
                    pptp = &conf.frame.ipv4.ptp;
                } else if (conf.type == MESA_ACE_TYPE_IPV6) {
                    pptp = &conf.frame.ipv6.ptp;
                } else {
                    T_WG(VTSS_TRACE_GRP_ACE,"unsupported ACL frame type %d", conf.type);
                    return PTP_RC_UNSUPPORTED_ACL_FRAME_TYPE;
                }

                switch (rules[i].deviceType) {
                    case VTSS_APPL_PTP_DEVICE_ORD_BOUND:
                    case VTSS_APPL_PTP_DEVICE_MASTER_ONLY:
                    case VTSS_APPL_PTP_DEVICE_SLAVE_ONLY:
                    case VTSS_APPL_PTP_DEVICE_AED_GM:
                        memset(conf.action.port_list, 0, sizeof(conf.action.port_list));
                        conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                        conf.action.force_cpu = true;
                        pptp->enable = true;
                        pptp->header.mask[0] = 0x0f;
                        pptp->header.value[0] = 0x0a; /* messageType = 10 Pdelay_Resp_Follow_Up*/
                        pptp->header.mask[1] = 0x0f;
                        pptp->header.value[1] = 0x02; /* versionPTP = 2 */
                        pptp->header.mask[2] = 0xff;
                        pptp->header.value[2] = rules[i].domain; /* domainNumber */
                        pptp->header.mask[3] = 0x00;
                        pptp->header.value[3] = 0x00; /* flagField[0] = don't care */
                        if (rules[i].protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
                            /* always terminate PTP peer delay messages */
                            conf.frame.ipv4.dip.mask = 0x0;
                        }
                        /* ACL rule for Pdelay_Resp_Follow_Up (never forwarded) */
                        T_DG(VTSS_TRACE_GRP_ACE,"apply rule %d, domain %d", rule_no, rules[i].domain);
                        PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                        if (protocol_level(my_tc_encapsulation) == protocol_level(rules[i].protocol) && my_tc_instance > 0  && my_tc_instance != i) {
                            /* the other packets for this BC device are also forwarded to the TC ports */
                            /* only if the TC is a 1-step */
                            memcpy(conf.action.port_list,  rules[my_tc_instance].port_list, portlist_size);
                            T_DG(VTSS_TRACE_GRP_ACE,"inst %d: packets are forwarded to instance %d TC ports", i, my_tc_instance);
                        }
                        if (rules[i].protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
                            /* in unicast mode only terminate PTP messages with my address */
                            conf.frame.ipv4.dip.mask = 0xffffffff;
                        }
                        conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                        conf.action.force_cpu = true;
                        pptp->enable = true;
                        pptp->header.mask[0] = 0x0e;
                        pptp->header.value[0] = 0x08; /* messageType = [8..9]  */
                        pptp->header.mask[1] = 0x0f;
                        pptp->header.value[1] = 0x02; /* versionPTP = 2 */
                        pptp->header.mask[2] = 0xff;
                        pptp->header.value[2] = rules[i].domain; /* domainNumber */
                        pptp->header.mask[3] = 0x00;
                        pptp->header.value[3] = 0x00; /* flagField[0] = don't care */

                        /* ACL rule for Follow_up, DelayResp */
                        T_DG(VTSS_TRACE_GRP_ACE,"apply rule %d, domain %d", rule_no, rules[i].domain);
                        PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                        if (protocol_level(my_tc_encapsulation) == protocol_level(rules[i].protocol) && my_2step_tc_instance > 0  && my_2step_tc_instance != i) {
                            /* the other packets for this BC device are also forwarded to the TC ports */
                            /* also if the TC is a 2-step */
                            memcpy(conf.action.port_list, rules[my_2step_tc_instance].port_list, portlist_size);
                            T_DG(VTSS_TRACE_GRP_ACE,"inst %d: packets are forwarded to instance %d TC ports", i, my_2step_tc_instance);
                        }
                        conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                        conf.action.force_cpu = true;
                        pptp->enable = true;
                        pptp->header.mask[0] = 0x08;
                        pptp->header.value[0] = 0x08; /* messageType = [8..15] except 8,9,10, which are hit by the rules above */
                        pptp->header.mask[1] = 0x0f;
                        pptp->header.value[1] = 0x02; /* versionPTP = 2 */
                        pptp->header.mask[2] = 0xff;
                        pptp->header.value[2] = rules[i].domain; /* domainNumber */
                        pptp->header.mask[3] = 0x00;
                        pptp->header.value[3] = 0x00; /* flagField[0] = don't care */

                        /* ACL rule for Announce, Signalling and Management */
                        T_DG(VTSS_TRACE_GRP_ACE,"apply rule %d, domain %d", rule_no, rules[i].domain);
                        PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                        if (protocol_level(my_tc_encapsulation) == protocol_level(rules[i].protocol) && my_tc_instance > 0  && my_tc_instance != i) {
                            /* the other packets for this BC device are also forwarded to the TC ports */
                            /* only if the TC is a 1-step */
                            memcpy(conf.action.port_list,  rules[my_tc_instance].port_list, portlist_size);
                            T_DG(VTSS_TRACE_GRP_ACE,"inst %d: packets are forwarded to instance %d TC ports", i, my_tc_instance);
                        } else {
                            memset(conf.action.port_list, 0, sizeof(conf.action.port_list));
                        }

                        /* ACL rule for Sync events */
                        if (mesa_cap_ts_p2p_delay_comp && mesa_cap_ts_asymmetry_comp) {
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_2; /* Asymmetry and p2p delay compensation */
                            conf.action.ptp.dom_sel = clock_domain;
                            pptp->header.mask[0] = 0x0f;
                            pptp->header.value[0] = 0x00; /* messageType = [0] */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                        } else if (mesa_cap_ts_twostep_always_required) {
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_TWO_STEP; /* always twostep action on received packets */
                            conf.action.ptp.dom_sel = clock_domain;
                            if (protocol_level(my_tc_encapsulation) == protocol_level(rules[i].protocol) && my_tc_instance > 0  && my_tc_instance != i) {
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_AND_TWO_STEP; /* always twostep action on received packets, */
                                conf.action.ptp.dom_sel = clock_domain;
                                                                                               /* and also do 1-step because the packet is 'TC' forwarded  */
                            }
                            pptp->header.mask[0] = 0x0f;
                            pptp->header.value[0] = 0x00; /* messageType = [0] */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                        } else {
                            T_EG(VTSS_TRACE_GRP_ACE, "Not supported!");
                        }

                        /* ACL rule for DelayReq events */
                        if (mesa_cap_ts_twostep_always_required) {
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_TWO_STEP; /* always twostep action on received packets */
                            conf.action.ptp.dom_sel = clock_domain;
                            pptp->header.mask[0] = 0x0f;
                            pptp->header.value[0] = 0x01; /* messageType = [1]*/
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                        } else {
                            if (mesa_cap_ts_delay_req_auto_resp[clock_domain] &&
                                rules[i].deviceType != VTSS_APPL_PTP_DEVICE_SLAVE_ONLY) {
                                if (config_data.conf[i].clock_init.cfg.clock_domain < mesa_cap_ts_domain_cnt) {
                                    //Auto response is only supported in HW clock domains
                                    PTP_RC(ptp_auto_resp_setup(i, &conf, &rule_no));
                                }
                            } else {
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP; /* Rx timestamp no compensation */
                                conf.action.ptp.dom_sel = clock_domain;
                                pptp->header.mask[0] = 0x0f;
                                pptp->header.value[0] = 0x01; /* messageType = [1] */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                            }
                        }

                        /* ACL rule for Pdelayxxx events (never forwarded) */
                        if (rules[i].protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
                            /* always terminate PTP peer delay messages */
                            conf.frame.ipv4.dip.mask = 0x0;
                        }
                        if (mesa_cap_ts_asymmetry_comp) {
                            /* ACL rule for PdelayReq events */
                            conf.action.force_cpu = true;
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP; /* Rx timestamp no compensation */
                            conf.action.ptp.dom_sel = clock_domain;
                            pptp->header.mask[0] = 0x0f;
                            pptp->header.value[0] = 0x02; /* messageType = [2] */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                            memcpy(conf.port_list,  rules[i].port_list, portlist_size);
                            memset(conf.action.port_list, 0, sizeof(conf.action.port_list));

                            /* ACL rule for PdelayResp events */
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_1; /* Asymmetry delay compensation */
                            conf.action.ptp.dom_sel = clock_domain;
                            pptp->header.mask[0] = 0x0f;
                            pptp->header.value[0] = 0x03; /* messageType = [3] */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                        } else {
                            /* ACL rule for PdelayReq and for PdelayResp events */
                            memset(conf.action.port_list, 0, sizeof(conf.action.port_list));
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_TWO_STEP; /* always twostep action on received packets */
                            conf.action.ptp.dom_sel = clock_domain;
                            pptp->header.mask[0] = 0x0e;
                            pptp->header.value[0] = 0x02; /* messageType = [2..3]*/
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                        }
                        break;
                    case VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT:
                        if (config_data.conf[i].clock_init.cfg.twoStepFlag) {
                            /* two-step E2E transparent clock */
                            /**********************************/
                            memset(conf.action.port_list, 0x00, sizeof(conf.action.port_list));
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                            conf.action.force_cpu = true;
                            pptp->enable = true;
                            pptp->header.mask[0] = 0x0e;
                            pptp->header.value[0] = 0x08; /* messageType = [8..9] */
                            pptp->header.mask[1] = 0x0f;
                            pptp->header.value[1] = 0x02; /* versionPTP = 2 */
                            pptp->header.mask[2] = 0x00;
                            pptp->header.value[2] = 0x00; /* domainNumber = d.c */
                            pptp->header.mask[3] = 0x00;
                            pptp->header.value[3] = 0x00; /* flagField[0] = don't care */

                            /* ACL rule for Follow_up, Delay_resp */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                            /* ACL rule for Pdelay_resp_follow_up */
                            pptp->header.mask[0] = 0x0f;
                            pptp->header.value[0] = 0x0a; /* messageType = [10] */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                            /* ACL rule for OneStep Sync Events */
                            if (mesa_cap_ts_twostep_always_required) {
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_TWO_STEP;
                                conf.action.ptp.dom_sel = clock_domain;
                            } else {
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                            }

                            pptp->header.mask[0] = 0x0f;
                            pptp->header.value[0] = 0x00; /* messageType = [0] */
                            pptp->header.mask[3] = 0x02;
                            pptp->header.value[3] = 0x00; /* flagField[0] = twostep = 0 */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                            /* ACL rule for 2step Sync, Delay_req and Pdelay_xx */
                            /* forwarded, and 2-step action, i.e. a timestamp id is reserved, and transmit time is saved in fifo */
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_TWO_STEP;
                            conf.action.ptp.dom_sel = clock_domain;
                            pptp->header.mask[0] = 0x0c;
                            pptp->header.value[0] = 0x00; /* messageType = [0..3] */
                            pptp->header.mask[3] = 0x00;
                            pptp->header.value[3] = 0x00; /* flagField[0] = don't care */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                            /* ACL rule for All other PTP messages (forward) */
                            memcpy(conf.action.port_list,  rules[i].port_list, portlist_size);
                            conf.action.force_cpu = false;   /* forwarded for TC. If needed for a BC, then the BC rules must hit first */
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                            pptp->header.mask[0] = 0x00;
                            pptp->header.value[0] = 0x00; /* messageType = d.c. */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                        } else {
                            /* one-step E2E transparent clock */
                            /**********************************/
                            if (mesa_cap_ts_asymmetry_comp) {
                                memcpy(conf.port_list,  rules[i].port_list, portlist_size);
                                memcpy(conf.action.port_list,  rules[i].port_list, portlist_size);
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_2;
                                conf.action.ptp.dom_sel = clock_domain;
                                conf.action.force_cpu = false;
                                pptp->enable = true;
                                pptp->header.mask[0] = 0x0f;
                                pptp->header.value[0] = 0x00; /* messageType = [0] */
                                pptp->header.mask[1] = 0x0f;
                                pptp->header.value[1] = 0x02; /* versionPTP = 2 */
                                pptp->header.mask[2] = 0x00;
                                pptp->header.value[2] = 0x00; /* domainNumber = d.c */
                                pptp->header.mask[3] = 0x00;
                                pptp->header.value[3] = 0x00; /* flagField[0] = don't care */
                                /* ACL rule for Sync Events:  */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                                /* ACL rule for PDelayResp Events:  */
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_1;
                                conf.action.ptp.dom_sel = clock_domain;
                                pptp->header.mask[0] = 0x0f;
                                pptp->header.value[0] = 0x03; /* messageType = [3] */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                                /* ACL rule for DelayReq, PDelayReq Events:  */
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP_ADD_DELAY;
                                conf.action.ptp.dom_sel = clock_domain;
                                pptp->header.mask[0] = 0x0c;
                                pptp->header.value[0] = 0x00; /* messageType = [1,2]  (because 0 and 3 are hit by the previous rules */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                            } else if (mesa_cap_ts_internal_ports_req_twostep) {
                                memcpy(conf.port_list,  rules[i].external_port_list, portlist_size);
    #ifdef PTP_LOG_TRANSPARENT_ONESTEP_FORWARDING
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_TWO_STEP;
                                conf.action.ptp.dom_sel = clock_domain;
                                conf.action.force_cpu = true;
    #else
                                /* if no internal ports: forward to other external ports, if internal ports exista: copy to CPU */
                                if (rules[i].internal_port_exists) {
                                    memset(conf.action.port_list, 0x00, sizeof(conf.action.port_list));
                                    conf.action.ptp_action = MESA_ACL_PTP_ACTION_TWO_STEP;
                                    conf.action.ptp.dom_sel = clock_domain;
                                    conf.action.force_cpu = true;
                                } else {
                                    memcpy(conf.action.port_list,  rules[i].external_port_list, portlist_size);
                                    conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP;
                                    conf.action.ptp.dom_sel = clock_domain;
                                    conf.action.force_cpu = false;
                                }
    #endif
                                pptp->enable = true;
                                pptp->header.mask[0] = 0x0c;
                                pptp->header.value[0] = 0x00; /* messageType = [0..3] */
                                pptp->header.mask[1] = 0x0f;
                                pptp->header.value[1] = 0x02; /* versionPTP = 2 */
                                pptp->header.mask[2] = 0x00;
                                pptp->header.value[2] = 0x00; /* domainNumber = d.c */
                                pptp->header.mask[3] = 0x00;
                                pptp->header.value[3] = 0x00; /* flagField[0] = don't care */

                                /* ACL rule for External port Events:  */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                                if (rules[i].internal_port_exists) {
                                    /* ACL rule for Internal port Events: Send to CPU, SW expects a timestamp on all events, though it is not used */
                                    memcpy(conf.port_list,  rules[i].internal_port_list, portlist_size);
                                    memset(conf.action.port_list, 0x00, sizeof(conf.action.port_list));
                                    conf.action.ptp_action = MESA_ACL_PTP_ACTION_TWO_STEP;
                                    conf.action.ptp.dom_sel = clock_domain;
                                    conf.action.force_cpu = true;
                                    PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                                    /* ACL rule for follow_Up and Delay_Resp messages. Forwarded in SW like Sync and DelayReq, otherwise the packet order may change */
                                    memcpy(conf.port_list,  rules[i].port_list, portlist_size);
                                    memset(conf.action.port_list, 0x00, sizeof(conf.action.port_list));
                                    conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                                    conf.action.force_cpu = true;
                                    pptp->enable = true;
                                    pptp->header.mask[0] = 0x0e;
                                    pptp->header.value[0] = 0x08; /* messageType = [8..9] */
                                    PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                                }
                            } else {
                                T_EG(VTSS_TRACE_GRP_ACE, "Not supported!");
                            }
                            /* ACL rule for other general messages */
                            memcpy(conf.port_list,  rules[i].port_list, portlist_size);
                            memcpy(conf.action.port_list,  rules[i].port_list, portlist_size);
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                            conf.action.force_cpu = false;
                            pptp->header.mask[0] = 0x08;
                            pptp->header.value[0] = 0x08; /* messageType = [8..15] */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                        }

                        break;
                    case VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT:
                        if (config_data.conf[i].clock_init.cfg.twoStepFlag) {
                            /* two-step P2P transparent clock */
                            /**********************************/
                            memset(conf.action.port_list, 0x00, sizeof(conf.action.port_list));
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                            conf.action.force_cpu = true;
                            pptp->enable = true;
                            pptp->header.mask[0] = 0x0d;
                            pptp->header.value[0] = 0x08; /* messageType = [8,10] */
                            pptp->header.mask[1] = 0x0f;
                            pptp->header.value[1] = 0x02; /* versionPTP = 2 */
                            pptp->header.mask[2] = 0x00;
                            pptp->header.value[2] = 0x00; /* domainNumber = d.c */
                            pptp->header.mask[3] = 0x00;
                            pptp->header.value[3] = 0x00; /* flagField[0] = don't care */

                            /* ACL rule for Follow_up, Pdelay_resp_follow_up */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                            /* ACL rule for Delay_Req, Delay_resp */
                            if (mesa_cap_ts_p2p_delay_comp) {
                                conf.action.force_cpu = false;
                            }
                            pptp->header.mask[0] = 0x07;
                            pptp->header.value[0] = 0x01; /* messageType = [1,9] */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                            if (mesa_cap_ts_twostep_always_required) {
                                /* ACL rule for Sync, Pdelay_req, Pdelay_resp */
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_TWO_STEP;
                                conf.action.ptp.dom_sel = clock_domain;
                                pptp->header.mask[0] = 0x0c;
                                pptp->header.value[0] = 0x00; /* messageType = [0..3] */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                            } else {
                                /* ACL rule for TwoStep Sync Events */
                                memset(conf.action.port_list, 0x00, sizeof(conf.action.port_list));
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                                conf.action.ptp.dom_sel = clock_domain;
                                conf.action.force_cpu = true;
                                pptp->header.mask[0] = 0x0f;
                                pptp->header.value[0] = 0x00; /* messageType = [0] */
                                pptp->header.mask[3] = 0x02;
                                pptp->header.value[3] = 0x02; /* flagField[0] = twostep = 1 */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                                /* ACL rule for PdelayResp events */
                                memset(conf.action.port_list, 0x00, sizeof(conf.action.port_list));
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_1; /* Asymmetry delay compensation */
                                conf.action.ptp.dom_sel = clock_domain;
                                conf.action.force_cpu = true;
                                pptp->header.mask[0] = 0x0f;
                                pptp->header.value[0] = 0x03; /* messageType = [3] */
                                pptp->header.mask[3] = 0x00;
                                pptp->header.value[3] = 0x00; /* flagField[0] = don't care */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                                /* ACL rule for OnesStep Sync, Pdelay_req */
                                memset(conf.action.port_list, 0x00, sizeof(conf.action.port_list));
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                                conf.action.force_cpu = true;
                                pptp->header.mask[0] = 0x0d;
                                pptp->header.value[0] = 0x00; /* messageType = [0,2] */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                            }

                            /* ACL rule for All other PTP messages */
                            memcpy(conf.action.port_list,  rules[i].port_list, portlist_size);
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                            pptp->header.mask[0] = 0x00;
                            pptp->header.value[0] = 0x00; /* messageType = d.c. */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                        } else {
                            /* one-step P2P transparent clock */
                            /**********************************/
                            memcpy(conf.action.port_list,  rules[i].port_list, portlist_size);
                            if (mesa_cap_ts_p2p_delay_comp && mesa_cap_ts_asymmetry_comp) {
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_2;
                                conf.action.ptp.dom_sel = clock_domain;
                            } else {
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP;
                                conf.action.ptp.dom_sel = clock_domain;
                            }
                            conf.action.force_cpu = false;
                            pptp->enable = true;
                            pptp->header.mask[0] = 0x0f;
                            pptp->header.value[0] = 0x00; /* messageType = [0] */
                            pptp->header.mask[1] = 0x0f;
                            pptp->header.value[1] = 0x02; /* versionPTP = 2 */
                            pptp->header.mask[2] = 0x00;
                            pptp->header.value[2] = 0x00; /* domainNumber = d.c */
                            pptp->header.mask[3] = 0x00;
                            pptp->header.value[3] = 0x00; /* flagField[0] = don't care */

                            /* ACL rule for Sync Events */
                            if (!mesa_cap_ts_p2p_delay_comp) {
                                /* as Luton26 does not support path delay correction, the sync is handled in the CPU */
                                memset(conf.action.port_list, 0x00, sizeof(conf.action.port_list));
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_TWO_STEP;
                                conf.action.ptp.dom_sel = clock_domain;
                                conf.action.force_cpu = true;
                            }
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                            if (mesa_cap_ts_twostep_always_required) {
                                /* ACL rule for Pdelay Events */
                                memset(conf.action.port_list, 0x00, sizeof(conf.action.port_list));
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_TWO_STEP;
                                conf.action.ptp.dom_sel = clock_domain;
                                conf.action.force_cpu = true;
                                pptp->header.mask[0] = 0x0e;
                                pptp->header.value[0] = 0x02; /* messageType = [2..3] */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                            } else {
                                /* ACL rule for PdelayReq Events */
                                memset(conf.action.port_list, 0x00, sizeof(conf.action.port_list));
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                                conf.action.ptp.dom_sel = clock_domain;
                                conf.action.force_cpu = true;
                                pptp->header.mask[0] = 0x0f;
                                pptp->header.value[0] = 0x02; /* messageType = [2] */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                                /* ACL rule for PdelayResp Events */
                                conf.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_1;
                                conf.action.ptp.dom_sel = clock_domain;
                                conf.action.force_cpu = true;
                                pptp->header.mask[0] = 0x0f;
                                pptp->header.value[0] = 0x03; /* messageType = [3] */
                                PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                            }

                            /* ACL rule for Pdelay_Resp_follow_Up messages */
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                            if (mesa_cap_ts_p2p_delay_comp) {
                                pptp->header.mask[0] = 0x0f;
                                pptp->header.value[0] = 0x0a; /* messageType = [10] */
                            } else {
                                /* in Luton26 the sync is handled in the CPU, and also the Followup, otherwise the Followup may arrive before the Sync */
                                pptp->header.mask[0] = 0x0d;
                                pptp->header.value[0] = 0x08; /* messageType = [8, 10] */
                            }
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                            /* ACL rule for Delay_Req and Delay_Resp messages */
                            conf.action.ptp_action = MESA_ACL_PTP_ACTION_NONE;
                            conf.action.force_cpu = false;
                            pptp->header.mask[0] = 0x07;
                            pptp->header.value[0] = 0x01; /* messageType = [1,9] */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));

                            /* ACL rule for All other PTP messages */
                            memcpy(conf.action.port_list,  rules[i].port_list, portlist_size);
                            pptp->header.mask[0] = 0x00;
                            pptp->header.value[0] = 0x00; /* messageType = d.c. */
                            PTP_RETURN(acl_rule_apply(&conf, i, rule_no++));
                        }
                        break;
                }

                if (rules[i].deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT || rules[i].deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT) {
                    my_next_id = rules[i].id[0];
                    if (i != 0) {
                        my_tc_encapsulation = rules[i].protocol;   /* indicates that forwarding in the BC instance is needed */

                        if (mesa_cap_ts_p2p_delay_comp) {
                            if (config_data.conf[i].clock_init.cfg.twoStepFlag) {
                                my_2step_tc_instance = i;
                            } else {
                                my_tc_instance = i;
                            }
                        } else {
                            if ((rules[i].deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT) || config_data.conf[i].clock_init.cfg.twoStepFlag) {
                                my_2step_tc_instance = i;
                            } else {
                                my_tc_instance = i;
                            }
                        }
                        T_DG(VTSS_TRACE_GRP_ACE,"my_tc_instance = %d", i);
                        for (int ii = 0; ii < PTP_CLOCK_INSTANCES; ii++) {
                            if (i != ii && protocol_level(my_tc_encapsulation) == protocol_level(rules[ii].protocol)) {
                                /* recalculate instance 0 ACL rules */
                                refresh_inst[ii] = true;
                            }
                        }
                    } else {
                        my_tc_encapsulation = VTSS_APPL_PTP_PROTOCOL_MAX_TYPE;   /* indicates that no forwarding in the SlaveOnly instance is needed */
                        my_tc_instance = i;
                        my_2step_tc_instance = i;
                        T_DG(VTSS_TRACE_GRP_ACE,"my_tc_instance = %d", i);
                        for (int ii = 0; ii < PTP_CLOCK_INSTANCES; ii++) {
                            if (i != ii && protocol_level(rules[i].protocol) == protocol_level(rules[ii].protocol)) {
                                /* recalculate instance 0 ACL rules */
                                refresh_inst[ii] = true;
                            }
                        }
                    }
                }
            }

        } while (multi_proto && (++proto_type < proto_num)); // loop for multi protocol encapsulation

        if (rules[i].deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT || rules[i].deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT) {
            my_next_id = rules[i].id[0];
        }

    }
    if (ptp_all_external_port_phy_ts) {
        /* All ports have 1588 PHY, therefore no ACL rules are needed for timestamping */
        /* instead the forwarding is set up in the MAC table */
        mac_used = 0;
        my_tc_instance = -1;
        for (inst = 0; inst < PTP_CLOCK_INSTANCES; inst++) {
            if (rules[inst].deviceType != VTSS_APPL_PTP_DEVICE_NONE &&
                    (rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET || rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED
                     || rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI || rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED
                     || rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP6MIXED)) {
                /* check if the configuration matches previous instances */
                for (mac_idx = 0; mac_idx < mac_used; mac_idx++) {
                    if ((0 == memcmp(&mac_entry[mac_idx].vid_mac.mac,  /* match 1588 multicast Dest address */
                                     (rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP6MIXED) ? &ptp_ipv6_mcast_adr[0] :
                                     (rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI || rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED)
                                       ? &ptp_ip_mcast_adr[0] : &ptp_eth_mcast_adr[0],
                                     sizeof(mesa_mac_t))) && mac_entry[mac_idx].vid_mac.vid == config_data.conf[inst].clock_init.cfg.configured_vid) {
                        break;
                    }
                }
                if (mac_idx >= mac_used) {
                    ++mac_used;
                }
                T_IG(VTSS_TRACE_GRP_ACE,"mac_used %d, mac_idx %d", mac_used, mac_idx);
                memcpy(&mac_entry[mac_idx].vid_mac.mac,  /* match 1588 multicast Dest address */
                       (rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP6MIXED) ? &ptp_ipv6_mcast_adr[0] :
                       (rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI || rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED)
                         ? &ptp_ip_mcast_adr[0] : &ptp_eth_mcast_adr[0],
                       sizeof(mesa_mac_t));
                mac_entry[mac_idx].vid_mac.vid = config_data.conf[inst].clock_init.cfg.configured_vid;
                if (rules[inst].deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND || rules[inst].deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY || rules[inst].deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY) {
                    mac_entry[mac_idx].copy_to_cpu = true;
                }
                if (rules[inst].deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT || rules[inst].deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT || rules[inst].deviceType == VTSS_APPL_PTP_DEVICE_BC_FRONTEND) {
                    port_iter_init_local(&pit);
                    my_tc_instance = inst;
                    while (port_iter_getnext(&pit)) {
                        mac_entry[mac_idx].destination[pit.iport] |= rules[inst].port_list[pit.iport];
                    }
                }
            }
            if (config_data.conf[i].clock_init.cfg.clock_domain < mesa_cap_ts_domain_cnt &&
                mesa_cap_ts_delay_req_auto_resp[config_data.conf[i].clock_init.cfg.clock_domain]) {
                // the auto response rule shall also be set up even if all ports are PHY TS ports
                if (rules[i].deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND || rules[i].deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {
                    portlist_size = sizeof(conf.port_list);
                    memcpy(conf.port_list,  rules[i].port_list, portlist_size);
                    conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
                    if (conf.type == MESA_ACE_TYPE_ETYPE) {
                        pptp = &conf.frame.etype.ptp;
                    } else if (conf.type == MESA_ACE_TYPE_IPV4) {
                        pptp = &conf.frame.ipv4.ptp;
                    } else {
                        T_WG(VTSS_TRACE_GRP_ACE,"unsupported ACL frame type %d", conf.type);
                        return PTP_RC_UNSUPPORTED_ACL_FRAME_TYPE;
                    }
                    pptp->enable = true;
                    pptp->header.mask[1] = 0x0f;
                    pptp->header.value[1] = 0x02; /* versionPTP = 2 */
                    pptp->header.mask[2] = 0xff;
                    pptp->header.value[2] = rules[i].domain; /* domainNumber */
                    pptp->header.mask[3] = 0x00;
                    pptp->header.value[3] = 0x00; /* flagField[0] = don't care */
                    if (rules[i].protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
                        /* always terminate PTP peer delay messages */
                        conf.frame.ipv4.dip.mask = 0x0;
                    }
                        //Auto response is only supported in HW clock domains
                    PTP_RC(ptp_auto_resp_setup(i, &conf, &rule_no));
                }
            }
        }
        /* repeat the same process for the Peer delay multicast addresses (always to sent to CPU) */
        for (inst = 0; inst < PTP_CLOCK_INSTANCES; inst++) {
            if (rules[inst].deviceType != VTSS_APPL_PTP_DEVICE_NONE &&
                    (rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET || rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED
                     || rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI || rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED)) {
                /* check if the configuration matches previous instances */
                for (mac_idx = 0; mac_idx < mac_used; mac_idx++) {
                    if ((0 == memcmp(&mac_entry[mac_idx].vid_mac.mac,  /* match 1588 multicast Dest address */
                                     (rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI || rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED)
                                       ? &ptp_ip_mcast_adr[1] : &ptp_eth_mcast_adr[1],
                                     sizeof(mesa_mac_t))) && mac_entry[mac_idx].vid_mac.vid == config_data.conf[inst].clock_init.cfg.configured_vid) {
                        break;
                    }
                }
                if (mac_idx >= mac_used) {
                    ++mac_used;
                }
                T_IG(VTSS_TRACE_GRP_ACE,"mac_used %d, mac_idx %d", mac_used, mac_idx);
                memcpy(&mac_entry[mac_idx].vid_mac.mac,  /* match 1588 multicast Dest address */
                       (rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MULTI || rules[inst].protocol == VTSS_APPL_PTP_PROTOCOL_IP4MIXED)
                         ? &ptp_ip_mcast_adr[1] : &ptp_eth_mcast_adr[1],
                       sizeof(mesa_mac_t));
                mac_entry[mac_idx].vid_mac.vid = config_data.conf[inst].clock_init.cfg.configured_vid;

                mac_entry[mac_idx].copy_to_cpu = true;
            }
        }
        /* now apply the calculated mac teble entries */
        for (mac_idx = 0; mac_idx < mac_used; mac_idx++) {
            port_iter_init_local(&pit);
            idx = 0;
            dest_txt[0] = 0;
            while (port_iter_getnext(&pit)) {
                if (mac_entry[mac_idx].destination[pit.iport]) {
                    idx += snprintf(&dest_txt[idx], sizeof(dest_txt)-idx, "%d, ", pit.uport);
                }
            }
            T_IG(VTSS_TRACE_GRP_ACE,"Adding mac table entry: mac %s, vid %d, copy to cpu %d, destinations %s", misc_mac2str(mac_entry[mac_idx].vid_mac.mac.addr),
                mac_entry[mac_idx].vid_mac.vid, mac_entry[mac_idx].copy_to_cpu, dest_txt);
            mac_entry[mac_idx].locked = true;
            mac_entry[mac_idx].aged = false;
            mac_entry[mac_idx].cpu_queue = PACKET_XTR_QU_BPDU;
            PTP_RC(mesa_mac_table_add(NULL, &mac_entry[mac_idx]));
        }
    }

    for (int ii = 0; ii < PTP_CLOCK_INSTANCES; ii++) {
        if (refresh_inst[ii]) {
            T_IG(VTSS_TRACE_GRP_ACE,"ptp_ace_update %d", ii);
            PTP_RC(_ptp_ace_update(ii, ptp_inst_updated));
        }
    }

    return rc;
}

static mesa_rc ptp_ace_update(int i)
{
    CapArray<bool, VTSS_APPL_CAP_PTP_CLOCK_CNT> ptp_inst_updated;  // Note: All elements initialized to 0 i.e. FALSE by constructor

    return _ptp_ace_update(i, ptp_inst_updated);
}

/****************************************************************************/
/*  Allocated PHY Timestamp functions                                       */
/****************************************************************************/
static void phy_ts_rules_init(void)
{
    int inst;
    mesa_port_no_t j;
    port_iter_t       pit;
    for (inst = 0; inst < PTP_CLOCK_INSTANCES; inst++) {
        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            j = pit.iport;

            _ptp_inst_phy_rules[inst][j].deviceType = VTSS_APPL_PTP_DEVICE_NONE;
            _ptp_inst_phy_rules[inst][j].protocol = VTSS_APPL_PTP_PROTOCOL_ETHERNET;
            _ptp_inst_phy_rules[inst][j].phy_ts_port = false;
            _ptp_inst_phy_rules[inst][j].flow[0] = 0;
            _ptp_inst_phy_rules[inst][j].flow[1] = 0;
            _ptp_inst_phy_rules[inst][j].flow_used[0] = false;
            _ptp_inst_phy_rules[inst][j].flow_used[1] = false;
            _ptp_inst_phy_rules[inst][j].clock[0] = 0;
            _ptp_inst_phy_rules[inst][j].clock[1] = 0;
            _ptp_inst_phy_rules[inst][j].sw_clk = false;

        }
    }
}

#ifdef PHY_DATA_DUMP
static void phy_ts_dump(void)
{
    int i,p;
    port_iter_t       pit;
    port_iter_init_local(&pit);

    for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            p = pit.iport;
            if (_ptp_inst_phy_rules[i][p].phy_ts_port) {
                for (int j = 0; j < VTSS_PHY_TS_MAX_ENGINES; j++) {
                    printf(" instance %d  port %d: eng_id %d, used %d \n", i, p, j,
                           _ptp_inst_phy_rules[i][p].eng_used[j]);
                }
            }
        }

    }
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        p = pit.iport;
        for (int j = 0; j < VTSS_PHY_TS_MAX_ENGINES; j++) {
            printf(" port %d: eng : %d protocol %d action %d \n", p, j, _phy_port_rules[p].eng[j].protocol,
                     _phy_port_rules[p].eng[j].act);
        }
    }

}

#endif

static mepa_ts_pkt_encap_t ptp_to_mepa_encap(vtss_appl_ptp_protocol_t proto)
{
    mepa_ts_pkt_encap_t encap = MEPA_TS_ENCAP_NONE;

    switch(proto) {
        case VTSS_APPL_PTP_PROTOCOL_ETHERNET:
        case VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED:
            encap = MEPA_TS_ENCAP_ETH_PTP;
            break;
        case VTSS_APPL_PTP_PROTOCOL_IP4MULTI:
        case VTSS_APPL_PTP_PROTOCOL_IP4MIXED:
        case VTSS_APPL_PTP_PROTOCOL_IP4UNI:
        case VTSS_APPL_PTP_PROTOCOL_IP6MIXED:
            encap = MEPA_TS_ENCAP_ETH_IP_PTP;
            break;
        default:
            encap = MEPA_TS_ENCAP_NONE;
            break;
    }
    return encap;
}

#define VSC_CLOCKS_PER_ENGINE 2
#define VSC_FLOWS_PER_ENGINE 8

static mesa_rc phy_ts_clock_conf_get(int port, bool ingress, uint16_t clock_id, mepa_ts_ptp_clock_conf_t *const clk_conf)
{
    mesa_rc rc = MESA_RC_OK;

    if (ingress) {
        rc = meba_phy_ts_rx_clock_conf_get(board_instance, port, clock_id, clk_conf);
    } else {
        rc = meba_phy_ts_tx_clock_conf_get(board_instance, port, clock_id, clk_conf);
    }
    return rc;
}
static mesa_rc phy_ts_flow_conf_get(int port, bool ingress, uint16_t flow_id, mepa_ts_classifier_t *const flow_conf)
{
    mesa_rc rc = MESA_RC_OK;

    if (ingress) {
        rc = meba_phy_ts_rx_classifier_conf_get(board_instance, port, flow_id, flow_conf);
    } else {
        rc = meba_phy_ts_tx_classifier_conf_get(board_instance, port, flow_id, flow_conf);
    }
    return rc;
}
static mesa_rc phy_ts_clock_conf_set(int port, BOOL ingress, uint16_t clock_id, const mepa_ts_ptp_clock_conf_t *clk_conf)
{
    mesa_rc rc = MESA_RC_OK;

    if (ingress) {
        rc = meba_phy_ts_rx_clock_conf_set(board_instance, port, clock_id, clk_conf);
    } else {
        rc = meba_phy_ts_tx_clock_conf_set(board_instance, port, clock_id, clk_conf);
    }
    return rc;
}
static mesa_rc phy_ts_flow_conf_set(int port, BOOL ingress, uint16_t flow_id, const mepa_ts_classifier_t *flow_conf)
{
    mesa_rc rc = MESA_RC_OK;

    if (ingress) {
        rc = meba_phy_ts_rx_classifier_conf_set(board_instance, port, flow_id, flow_conf);
    } else {
        rc = meba_phy_ts_tx_classifier_conf_set(board_instance, port, flow_id, flow_conf);
    }
    return rc;
}
static bool phy_flow_clk_id_get(int ptp_inst, int port, BOOL ingress, uint16_t *flow_id, uint16_t *clk_id, mepa_ts_ptp_clock_mode_t clk_mode, mepa_ts_ptp_delaym_type_t delaym)
{
    bool ret = false;
    mepa_ts_ptp_clock_conf_t clk_conf;
    mepa_ts_pkt_encap_t encap;

    T_IG(VTSS_TRACE_GRP_PHY_TS, "Get free flow & clock on phy for port %d", port);
    encap = ptp_to_mepa_encap(config_data.conf[ptp_inst].clock_init.cfg.protocol);
    if (port_data[port].topo.ts_gen == VTSS_PTP_TS_GEN_3) {
        *flow_id = *clk_id = 0;
        if (phy_ts_clock_conf_get(port, ingress, *clk_id, &clk_conf) == MESA_RC_OK) {
            T_IG(VTSS_TRACE_GRP_PHY_TS, "clock enable %d\n", clk_conf.enable);
            if (!clk_conf.enable) {
                ret = true;
            }
        }
    } else {
        // VSC phys
        uint8_t st_clk = 0, end_clk = 3, i, st_flow;

        if (port_data[port].topo.ts_gen == VTSS_PTP_TS_GEN_2) {
            end_clk = 5;
        }
        do {
            for (i = st_clk; i < end_clk; i++) {
                if (phy_ts_clock_conf_get(port, ingress, i, &clk_conf) == MESA_RC_OK) {
                    if (clk_conf.clk_mode == MEPA_TS_PTP_CLOCK_MODE_NONE) {
                        *clk_id = i;
                        break;
                    }
                    T_DG(VTSS_TRACE_GRP_PHY_TS, "clk_mode %d delaym %d clk %d", clk_conf.clk_mode, clk_conf.delaym_type, i);
                    // shared port
                    if (!clk_conf.enable && ((clk_conf.clk_mode == clk_mode) &&
                        (clk_conf.delaym_type == delaym))) {
                        *clk_id = i;
                        break;
                    }
                }
            }
            T_IG(VTSS_TRACE_GRP_PHY_TS, "Using clock %d on phy for port %d, i %d", *clk_id, port, i);
            if (i != end_clk) {
                mepa_ts_classifier_t flow_conf;
                // check the available flow
                st_flow = (*clk_id/VSC_CLOCKS_PER_ENGINE) * VSC_FLOWS_PER_ENGINE; // 2 clocks & 8 flows per engine
                for (int j = st_flow; j < (st_flow + VSC_FLOWS_PER_ENGINE); j++) {
                    T_DG(VTSS_TRACE_GRP_PHY_TS, "Get flow %d on phy for port %d, i %d", j, port, i);
                    if (phy_ts_flow_conf_get(port, ingress, j, &flow_conf) == MESA_RC_OK) {
                        T_IG(VTSS_TRACE_GRP_PHY_TS, "flow encap %d", flow_conf.pkt_encap_type);
                        if (flow_conf.pkt_encap_type == MEPA_TS_ENCAP_NONE) {
                            *flow_id = j;
                            ret = true;
                            break;
                        }
                        if (encap != flow_conf.pkt_encap_type) {
                            break;
                        }
                        // if encap is same then common conf need to be verified.
                        if (encap == MEPA_TS_ENCAP_ETH_IP_PTP) {
                            if (flow_conf.ip_class_conf.ip_ver == MEPA_TS_IP_VER_6 &&
                                config_data.conf[ptp_inst].clock_init.cfg.protocol != VTSS_APPL_PTP_PROTOCOL_IP6MIXED) {
                                break; // Engine has ipv6 but application needs to configure ipv4.
                            }
                        }
                        if (!flow_conf.enable) {
                            *flow_id = j;
                            ret = true;
                            break;
                        }
                    }
                }
            }
            if (i != end_clk && !ret) {
                // Go to next engine's first clock.
                st_clk = (*clk_id/VSC_CLOCKS_PER_ENGINE + 1) * VSC_CLOCKS_PER_ENGINE;
            } else if (i == end_clk) {
                break;
            }
            T_NG(VTSS_TRACE_GRP_PHY_TS, "Next engine clock %d on phy for port %d", st_clk, port);
        } while (!ret && st_clk < end_clk);
    }
    if (ret) {
        T_IG(VTSS_TRACE_GRP_PHY_TS, "free flow %d free clock %d for port %d", *flow_id, *clk_id, port);
    } else {
        T_IG(VTSS_TRACE_GRP_PHY_TS, "Could not find free flow and clock");
    }
    return ret;
}

static void phy_get_eth_class_conf(mepa_ts_classifier_eth_t *const eth_cfg)
{
    eth_cfg->mac_match_mode = MEPA_TS_ETH_ADDR_MATCH_ANY;
    eth_cfg->mac_match_select = MEPA_TS_ETH_MATCH_DEST_ADDR;
    eth_cfg->vlan_check = FALSE;
    eth_cfg->vlan_conf.pbb_en = FALSE;
    eth_cfg->vlan_conf.num_tag = 0;
    eth_cfg->vlan_conf.etype = 0x88f7;
}

static void phy_get_ip_class_conf(mepa_ts_classifier_ip_t *const ip_cfg, bool ipv6_en)
{
    ip_cfg->udp_dport_en = TRUE;
    ip_cfg->udp_dport    = PTP_EVENT_PORT;
    ip_cfg->udp_sport_en = FALSE;
    ip_cfg->udp_sport    = 0;

    if (ipv6_en) {
        ip_cfg->ip_ver = MEPA_TS_IP_VER_6;
        ip_cfg->ip_match_mode = MEPA_TS_IP_MATCH_DEST;
        memset(ip_cfg->ip_addr.ipv6.addr, 0, sizeof(ip_cfg->ip_addr.ipv6.addr));
        memset(ip_cfg->ip_addr.ipv6.mask, 0, sizeof(ip_cfg->ip_addr.ipv6.mask));
    } else {
        ip_cfg->ip_ver = MEPA_TS_IP_VER_4;
        ip_cfg->ip_match_mode = MEPA_TS_IP_MATCH_DEST;
        memset(&ip_cfg->ip_addr.ipv4.addr, 0, sizeof(ip_cfg->ip_addr.ipv4.addr));
        memset(&ip_cfg->ip_addr.ipv4.mask, 0, sizeof(ip_cfg->ip_addr.ipv4.mask));
    }
}
static bool phy_to_appl_clk_mode(vtss_appl_ptp_device_type_t device, bool two_step, mepa_ts_ptp_clock_mode_t *const clk_mode)
{
    bool ret = true;;
    switch(device) {
        case VTSS_APPL_PTP_DEVICE_ORD_BOUND:
        case VTSS_APPL_PTP_DEVICE_MASTER_ONLY:
        case VTSS_APPL_PTP_DEVICE_SLAVE_ONLY:
        case VTSS_APPL_PTP_DEVICE_BC_FRONTEND:
        case VTSS_APPL_PTP_DEVICE_AED_GM:
            *clk_mode = two_step ? MEPA_TS_PTP_CLOCK_MODE_BC2STEP : MEPA_TS_PTP_CLOCK_MODE_BC1STEP;
            break;
        case VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT:
        case VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT:
            *clk_mode = two_step ? MEPA_TS_PTP_CLOCK_MODE_TC2STEP : MEPA_TS_PTP_CLOCK_MODE_TC1STEP;
            break;
        default:
            ret = false;
            break;
    }
    T_IG(VTSS_TRACE_GRP_PHY_TS, "clk_mode %d device %d", *clk_mode, device);
    return ret;
}

static void phy_get_clock_conf(mepa_ts_ptp_clock_conf_t *const clk_conf)
{
    clk_conf->enable = TRUE;
    clk_conf->cf_update = false;
    clk_conf->ptp_class_conf.version.lower = VERSION_PTP;
    clk_conf->ptp_class_conf.version.upper = VERSION_PTP;
    clk_conf->ptp_class_conf.minor_version.lower = 0;
    clk_conf->ptp_class_conf.minor_version.upper = 1;
    clk_conf->ptp_class_conf.domain.mode = MEPA_TS_MATCH_MODE_RANGE;
    clk_conf->ptp_class_conf.domain.match.range.upper = 0xff;
    clk_conf->ptp_class_conf.domain.match.range.lower = 0;
    clk_conf->ptp_class_conf.sdoid.mode = MEPA_TS_MATCH_MODE_RANGE;
    clk_conf->ptp_class_conf.sdoid.match.range.upper = 0xff;
    clk_conf->ptp_class_conf.sdoid.match.range.lower = 0;
}

// clear phy instance rule state
static void phy_inst_rule_clear(ptp_inst_phy_ts_rule_config_t *inst_rule)
{
    // Clear the state
    inst_rule->deviceType = VTSS_APPL_PTP_DEVICE_NONE;
    inst_rule->flow_used[0] = false;
    inst_rule->flow_used[1] = false;
    inst_rule->phy_ts_port  = false;
    inst_rule->sw_clk       = false;
}

static mesa_rc ptp_phy_ts_update(int ptp_inst, int port)
{
    vtss_appl_ptp_clock_config_default_ds_t *init_cfg = &config_data.conf[ptp_inst].clock_init.cfg;
    vtss_appl_ptp_config_port_ds_t *port_cfg = &config_data.conf[ptp_inst].port_config[port];
    ptp_inst_phy_ts_rule_config_t *inst_rule = &_ptp_inst_phy_rules[ptp_inst][port];
    mepa_ts_classifier_t cls_cf;
    mesa_rc rc = MESA_RC_OK;
    mepa_ts_ptp_clock_conf_t clk_conf = {};
    mepa_ts_ptp_clock_mode_t clk_mode;
    mepa_ts_ptp_delaym_type_t delaym;
    bool two_step = false, remove_conf = false, add_conf = false;

    T_IG(VTSS_TRACE_GRP_PHY_TS, "instance %d port %d port-enabled %s device-type %d", ptp_inst, port, port_cfg->enabled?"true":"false", inst_rule->deviceType);
    // Error checking
    if (init_cfg->protocol == VTSS_APPL_PTP_PROTOCOL_OAM || init_cfg->protocol == VTSS_APPL_PTP_PROTOCOL_ONE_PPS) {
        return MESA_RC_OK;
    }
    if (port_data[port].topo.ts_feature != VTSS_PTP_TS_PTS) {
        /* Not PHY timestamping port*/
        return MESA_RC_OK;
    }
    if ((init_cfg->deviceType == VTSS_APPL_PTP_DEVICE_NONE) && (inst_rule->deviceType == VTSS_APPL_PTP_DEVICE_NONE)) {
        T_DG(VTSS_TRACE_GRP_PHY_TS, " no protocol configured\n");
        /* No protocol configured for this instance */
        return MESA_RC_OK;
    }

    if (!port_cfg->enabled && inst_rule->deviceType == VTSS_APPL_PTP_DEVICE_NONE) {
        // On this port, protocol not configured.
        return MESA_RC_OK;
    }

    // PHY exists in clock domain 0 or software clock domains.
    if (init_cfg->clock_domain && init_cfg->clock_domain < mesa_cap_ts_domain_cnt) {
        return MESA_RC_OK;
    }

    if (((init_cfg->deviceType == VTSS_APPL_PTP_DEVICE_NONE) || !port_cfg->enabled) && (inst_rule->deviceType != VTSS_APPL_PTP_DEVICE_NONE)) {
        remove_conf = true;
    } else if (port_cfg->enabled && (inst_rule->deviceType == VTSS_APPL_PTP_DEVICE_NONE)) {
        add_conf = true;
    } else { // reconfigure
        remove_conf = true;
        add_conf    = true;
    }

    T_IG(VTSS_TRACE_GRP_PHY_TS, "clk-domain %d sw_clk %d add_conf %d remove_conf %d", init_cfg->clock_domain, inst_rule->sw_clk, add_conf, remove_conf);
    // Configure only phy data structures but not in chip
    // if another instance was already configured on same port.
    if (init_cfg->clock_domain >= mesa_cap_ts_domain_cnt || inst_rule->sw_clk) {
        for (int i = 0; i < PTP_CLOCK_INSTANCES; i++) {
            if (i == ptp_inst) {
                continue;
            }
            T_IG(VTSS_TRACE_GRP_PHY_TS, "i %d port enabled %d deviceType %d", i, config_data.conf[i].port_config[port].enabled, _ptp_inst_phy_rules[i][port].deviceType);
            if (config_data.conf[i].port_config[port].enabled &&
                _ptp_inst_phy_rules[i][port].deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
                if (add_conf && !remove_conf) {
                    if (config_data.conf[i].clock_init.cfg.protocol == init_cfg->protocol) {
                        *inst_rule = _ptp_inst_phy_rules[i][port];
                    } else {
                        // Only one encapsulation can be configured on phy port.
                        return MESA_RC_ERROR;
                    }
                } else if (!add_conf && remove_conf) {
                    phy_inst_rule_clear(inst_rule);
                }
                return MESA_RC_OK;
            }
        }
    }

    if (remove_conf) {
        T_IG(VTSS_TRACE_GRP_PHY_TS, "Delete config for port %d", port);
        // Delete the configuration.
        // Delete egress clock
        if (inst_rule->flow_used[0]) {
            rc = meba_phy_ts_tx_clock_conf_get(board_instance, port, inst_rule->clock[0], &clk_conf);
            clk_conf.enable = FALSE;
            rc = meba_phy_ts_tx_clock_conf_set(board_instance, port, inst_rule->clock[0], &clk_conf);
        }
        // Delete ingress clock
        if (inst_rule->flow_used[1]) {
            rc = meba_phy_ts_rx_clock_conf_get(board_instance, port, inst_rule->clock[1], &clk_conf);
            clk_conf.enable = FALSE;
            rc = meba_phy_ts_rx_clock_conf_set(board_instance, port, inst_rule->clock[1], &clk_conf);
        }

        // Delete egress flow
        if (inst_rule->flow_used[0]) {
            rc = meba_phy_ts_tx_classifier_conf_get(board_instance, port, inst_rule->flow[0], &cls_cf);
            cls_cf.pkt_encap_type = MEPA_TS_ENCAP_NONE;
            cls_cf.enable = FALSE;
            rc = meba_phy_ts_tx_classifier_conf_set(board_instance, port, inst_rule->flow[0], &cls_cf);
            if (rc != MESA_RC_OK) {
                T_IG(VTSS_TRACE_GRP_PHY_TS, "PHY flow configuration on egress could not be deleted rc=%d", rc);
            }
        }
        // Delete ingress flow
        if (inst_rule->flow_used[1]) {
            rc = meba_phy_ts_rx_classifier_conf_get(board_instance, port, inst_rule->flow[1], &cls_cf);
            cls_cf.pkt_encap_type = MEPA_TS_ENCAP_NONE;
            cls_cf.enable = FALSE;
            rc = meba_phy_ts_rx_classifier_conf_set(board_instance, port, inst_rule->flow[1], &cls_cf);
            if (rc != MESA_RC_OK) {
                T_IG(VTSS_TRACE_GRP_PHY_TS, "PHY flow configuration on ingress could not be deleted rc=%d", rc);
            }
        }
        // Clear the state
        inst_rule->deviceType = VTSS_APPL_PTP_DEVICE_NONE;
        inst_rule->flow_used[0] = false;
        inst_rule->flow_used[1] = false;
        inst_rule->phy_ts_port = false;
        inst_rule->sw_clk = false;
    }
    if (add_conf) {
        T_IG(VTSS_TRACE_GRP_PHY_TS, "Add config on phy for port %d", port);
        if ((init_cfg->deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) || (init_cfg->deviceType == VTSS_APPL_PTP_DEVICE_AED_GM) || (init_cfg->deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY)) {
            two_step = ((init_cfg->twoStepFlag && !(port_cfg->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE))
                                  || (port_cfg->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE));
        } else {
            two_step = init_cfg->twoStepFlag;
        }
        if (!phy_to_appl_clk_mode(init_cfg->deviceType, two_step, &clk_mode)) {
            // TODO: delete the flow conf
            return MESA_RC_ERROR;
        }
        delaym = (port_cfg->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_E2E) ? MEPA_TS_PTP_DELAYM_E2E : MEPA_TS_PTP_DELAYM_P2P;
        // Create the configuration.
        // Identify flow and clock id for egress.
        for (int ingr = 0; ingr < 2; ingr++) {
            uint16_t flow_id=0, clk_id=0;
            mepa_ts_pkt_encap_t encap = ptp_to_mepa_encap(config_data.conf[ptp_inst].clock_init.cfg.protocol);

            if (encap == MEPA_TS_ENCAP_NONE) {
                T_IG(VTSS_TRACE_GRP_PHY_TS, "Could not find suitable phy encapsulation");
                return MESA_RC_ERROR;
            }
            if (phy_flow_clk_id_get(ptp_inst, port, ingr, &flow_id, &clk_id, clk_mode, delaym) == true) {
                T_IG(VTSS_TRACE_GRP_PHY_TS, "Get classifier config for flow %d on phy for port %d", flow_id, port);
                // Create the flow on egress
                rc = phy_ts_flow_conf_get(port, ingr, flow_id, &cls_cf);
                cls_cf.pkt_encap_type = ptp_to_mepa_encap(config_data.conf[ptp_inst].clock_init.cfg.protocol);
                cls_cf.enable = TRUE;
                cls_cf.clock_id = clk_id;
                phy_get_eth_class_conf(&cls_cf.eth_class_conf);
                // Vlan tag configuration
                if (config_data.conf[ptp_inst].clock_init.cfg.configured_vid &&
                    port_data[port].vlan_forward[ptp_inst] == MESA_PACKET_FILTER_TAGGED) {
                    cls_cf.eth_class_conf.vlan_check = TRUE;
                    cls_cf.eth_class_conf.vlan_conf.num_tag = 1;
                    cls_cf.eth_class_conf.vlan_conf.inner_tag.mode = MEPA_TS_MATCH_MODE_VALUE;
                    cls_cf.eth_class_conf.vlan_conf.inner_tag.match.value.val = config_data.conf[ptp_inst].clock_init.cfg.configured_vid;
                    cls_cf.eth_class_conf.vlan_conf.tpid = port_data[port].vlan_tpid[ptp_inst];
                }

                T_IG(VTSS_TRACE_GRP_PHY_TS, "mepa encap %d pbb_en %d", cls_cf.pkt_encap_type, cls_cf.eth_class_conf.vlan_conf.pbb_en);
                if (cls_cf.pkt_encap_type == MEPA_TS_ENCAP_ETH_IP_PTP) {
                    bool ipv6_en = false;
                    cls_cf.eth_class_conf.vlan_conf.etype = 0x800;
                    if (config_data.conf[ptp_inst].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP6MIXED) {
                        ipv6_en = true;
                        cls_cf.eth_class_conf.vlan_conf.etype = 0x86DD;
                    }
                    phy_get_ip_class_conf(&cls_cf.ip_class_conf, ipv6_en);
                }
                T_IG(VTSS_TRACE_GRP_PHY_TS, "Set class config for flow %d on phy for port %d", flow_id, port);
                rc = phy_ts_flow_conf_set(port, ingr, flow_id, &cls_cf);
                if (rc != MESA_RC_OK) {
                    T_IG(VTSS_TRACE_GRP_PHY_TS, "PHY flow configuration on egress could not be configured for port %d rc %d", port, rc);
                    return MESA_RC_ERROR;
                }

                T_IG(VTSS_TRACE_GRP_PHY_TS, "Get clock config for clock %d on phy for port %d", clk_id, port);
                // Create the clock
                rc = phy_ts_clock_conf_get(port, ingr, clk_id, &clk_conf);
                /* Get the current config of PTP instance called with this function. */
                clk_conf.enable = TRUE;
                clk_conf.clk_mode = clk_mode;
                clk_conf.delaym_type = delaym;
                phy_get_clock_conf(&clk_conf);
                T_IG(VTSS_TRACE_GRP_PHY_TS, "Set clock config for clock %d on phy for port %d clk_mode %d delaym %d", clk_id, port, clk_conf.clk_mode, clk_conf.delaym_type);
                rc = phy_ts_clock_conf_set(port, ingr, clk_id, &clk_conf);
                if (rc != MESA_RC_OK) {
                    T_IG(VTSS_TRACE_GRP_PHY_TS, "PHY clock configuration could not be set rc %d", rc);
                    return MESA_RC_ERROR;
                }
                inst_rule->flow_used[ingr] = true;
                inst_rule->flow[ingr] = flow_id;
                inst_rule->clock[ingr] = clk_id;
                inst_rule->phy_ts_port = true;
                if (init_cfg->clock_domain >= mesa_cap_ts_domain_cnt) {
                    inst_rule->sw_clk = true;
                }
            } else {
                T_IG(VTSS_TRACE_GRP_PHY_TS, "Could not find free flow and clock for %s port %d", ingr ? "ingress" : "egress", port);
                    return MESA_RC_ERROR;
            } 
        }
        inst_rule->deviceType = init_cfg->deviceType;
    }
    return MESA_RC_OK;
}

void vtss_1588_org_time_option_get(int instance, u16 portnum, bool *org_time_option)
{
    *org_time_option = false;
    if (instance < PTP_CLOCK_INSTANCES) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            // correction field update does not work for clock domain 1 in sparx-v currently. Hence, origin timestamping needed for clk domain 1 also.
            *org_time_option = ports_org_time[instance][portnum -1];
        }
        T_DG(_I, "portnum: %d inst %d, org_time %d", portnum, instance, *org_time_option);
    }
}

// Returns true if delay request to be sent with correction field updated in application.
vtss_ptp_phy_corr_type_t vtss_port_phy_delay_corr_upd(uint port)
{
    return port_data[uport2iport(port)].phy_corr_type; // converted to internal port
}
// Returns true for lan-8814 phy which has limits on maximum LTC frequency adjustment configuration to avoid switch and phy LTC going out of sync.
bool vtss_port_has_lan8814_phy(uint port)
{
    return port_data[uport2iport(port)].topo.ts_gen == VTSS_PTP_TS_GEN_3;
}

// Update crc used in 2-step clock for Gen-3 phys.
void vtss_ptp_port_crc_update(uint32_t inst, uint32_t port)
{
    if (port_data[port].topo.ts_gen == VTSS_PTP_TS_GEN_3) {
        crcSrcPortId[inst][port] = crcCalculate(&ptp_global.ptpi[inst]->ptpPort[port].portDS.status.portIdentity);
    }
}
/****************************************************************************/
/*  port state functions                                                    */
/****************************************************************************/

/*
 * Port in-sync state change indication
 */
static void ptp_in_sync_callback(mesa_port_no_t port_no, BOOL in_sync)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY()) {
        port_data[port_no].port_ts_in_sync_tmp = in_sync;
        if (port_data[port_no].topo.port_ts_in_sync != (in_sync || ptp_time_settling)) {
            port_data[port_no].topo.port_ts_in_sync = in_sync || ptp_time_settling;
            for (int i = 0; i < PTP_CLOCK_INSTANCES; i++) {
                if (vtss_ptp_port_linkstate(ptp_global.ptpi[i], ptp_l2port_to_api(port_no),
                                            port_data[port_no].link_state && port_data[port_no].topo.port_ts_in_sync && port_data[port_no].vlan_forw[i]) == VTSS_RC_OK)
                {
                    T_IG(_I, "port_no: %d %s, settling %s", ptp_l2port_to_api(port_no), in_sync ? "in_sync" : "out_of_sync", ptp_time_settling ? "true" : "false");
                }
                else {
                    T_WG(_I, "invalid port_no: %d", ptp_l2port_to_api(port_no));
                }
                if (vtss_ptp_p2p_state(ptp_global.ptpi[i], ptp_l2port_to_api(port_no),
                                            port_data[port_no].link_state && port_data[port_no].topo.port_ts_in_sync) != VTSS_RC_OK) {
                    T_WG(_I, "invalid port_no: %d", ptp_l2port_to_api(port_no));
                }
            }
        }
    } else {
        T_WG(_I, "PTP not ready");
    }
}

/*
 * Port filter change indication
 */
static void ptp_port_filter_change_callback(int instance, mesa_port_no_t port_no, bool forward)
{
    port_data[port_no].vlan_forw[instance] = forward;
    if (vtss_ptp_port_linkstate(ptp_global.ptpi[instance], ptp_l2port_to_api(port_no),
                                port_data[port_no].link_state && port_data[port_no].topo.port_ts_in_sync && port_data[port_no].vlan_forw[instance]) == VTSS_RC_OK)
    {
        T_IG(_I, "port_no: %d filter %s", ptp_l2port_to_api(port_no), port_data[port_no].vlan_forw[instance] ? "Forward" : "Discard");
    } else {
        T_WG(_I, "invalid port_no: %d", ptp_l2port_to_api(port_no));
    }
}

/*
 * Port state change indication
 */
static void ptp_port_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY()) {
        port_data[port_no].link_state = status->link;
        if (mepa_phy_ts_cap()) {
            port_data_conf_set(port_no, status->link); /* check if the port has a 1588 PHY */
        }
        T_IG(_I, "port_no: %d link %s, speed %d", ptp_l2port_to_api(port_no), status->link ? "up" : "down", status->speed);
        for (int i = 0; i < PTP_CLOCK_INSTANCES; i++) {
            if (vtss_ptp_port_linkstate(ptp_global.ptpi[i], ptp_l2port_to_api(port_no),
                                        port_data[port_no].link_state && port_data[port_no].topo.port_ts_in_sync && port_data[port_no].vlan_forw[i]) == VTSS_RC_OK)
            {
                T_IG(_I, "port_no: %d link %s", ptp_l2port_to_api(port_no), status->link ? "up" : "down");
            } else {
                T_WG(_I, "invalid port_no: %d", ptp_l2port_to_api(port_no));
            }
            if (vtss_ptp_p2p_state(ptp_global.ptpi[i], ptp_l2port_to_api(port_no), port_data[port_no].link_state && port_data[port_no].topo.port_ts_in_sync) != VTSS_RC_OK) {
                T_WG(_I, "invalid port_no: %d", ptp_l2port_to_api(port_no));
            }

            // Load latencies again after every link up.
            if (ptp_global.ptpi[i]->ptpPort[port_no].designatedEnabled) {
                vtss_appl_ptp_config_port_ds_t *port_ds = &config_data.conf[i].port_config[port_no];
                (void)vtss_1588_ingress_latency_set(l2port_to_ptp_api(port_no), port_ds->ingressLatency);
                (void)vtss_1588_egress_latency_set(l2port_to_ptp_api(port_no), port_ds->egressLatency);
            }
        }
#if defined (VTSS_SW_OPTION_P802_1_AS)
        /* Update CMLDS port info */
        ptp_global.ptp_cm.cmlds_port_ds[port_no]->p2p_state = port_data[port_no].link_state && port_data[port_no].topo.port_ts_in_sync;
#endif
    } else {
        /* port state may change before PTP process is ready */
        T_IG(_I, "PTP not ready");
    }
}

/*
 * Set initial port link down state
 */
static void ptp_port_link_state_initial(int instance)
{
    mesa_port_no_t          portidx;
    vtss_ifindex_t          ifindex;
    vtss_appl_port_status_t ps;
    bool                    virtual_port;

    bool link_state, p2p_state;
    if (!msg_switch_exists(VTSS_ISID_START)) {
        T_NG(_I, "switch %d does not exist", VTSS_ISID_START);
        return;
    }
    for (portidx = 0; portidx < config_data.conf[instance].clock_init.numberPorts; portidx++) {
        virtual_port = false;
        if (vtss_ifindex_from_port(VTSS_ISID_START, portidx, &ifindex) == VTSS_RC_OK &&  vtss_appl_port_status_get(ifindex, &ps) == VTSS_RC_OK) {
            if (port_data[portidx].link_state != ps.link) {
                port_data[portidx].link_state = ps.link;
                if (mepa_phy_ts_cap()) {
                    port_data_conf_set(portidx, ps.link); /* check if the port has a 1588 PHY */
                }
            }
            link_state = port_data[portidx].link_state && port_data[portidx].topo.port_ts_in_sync && port_data[portidx].vlan_forw[instance];
            p2p_state = port_data[portidx].link_state && port_data[portidx].topo.port_ts_in_sync;
        } else {
            // virtual port
            ps.link = true;
            link_state = true;
            p2p_state = true;
            virtual_port = true;
        }
        if (vtss_ptp_port_linkstate(ptp_global.ptpi[instance], ptp_l2port_to_api(portidx),
                                    link_state) == VTSS_RC_ERROR)
        {
            T_EG(_I, "invalid port_no: %d", ptp_l2port_to_api(portidx));
        }
        if (!virtual_port) {
            if (vtss_ptp_p2p_state(ptp_global.ptpi[instance], ptp_l2port_to_api(portidx),
                        p2p_state) != VTSS_RC_OK) {
                T_WG(_I, "invalid port_no: %d,numberPoets %d", ptp_l2port_to_api(portidx), ptp_global.ptpi[instance]->clock_init->numberPorts);
            }
        }
    }
}

#if defined (VTSS_SW_OPTION_P802_1_AS)
static void ptp_cmlds_default_ds_default_get(vtss_appl_ptp_802_1as_cmlds_default_ds_t *cmlds_def_ds)
{
    memcpy(cmlds_def_ds->clockIdentity, ptp_global.sysmac.addr, 3);
    cmlds_def_ds->clockIdentity[3] = 0xff;
    cmlds_def_ds->clockIdentity[4] = 0xfe;
    memcpy(cmlds_def_ds->clockIdentity+5, ptp_global.sysmac.addr+3, 3);
    //virtual port uses clock-id[7] as PTP_CLOCK_INSTANCES. Hence, cmlds must use PTP_CLOCK_INSTANCES + 1 to have unique clock-id.
    cmlds_def_ds->clockIdentity[7] += PTP_CLOCK_INSTANCES + 1;
    cmlds_def_ds->numberLinkPorts = mesa_cap_port_cnt;
    cmlds_def_ds->sdoId = 0x200;
}

static void ptp_cmlds_add()
{
    ptp_cmlds_default_ds_default_get(&ptp_global.ptp_cm.cmlds_defaults);
    for (uint portnum = 0; portnum < mesa_cap_port_cnt; portnum++) {
        ptp_global.ptp_cm.cmlds_port_ds[portnum] = vtss_ptp_cmlds_port_inst_add(portnum);
        ptp_global.ptp_cm.cmlds_port_ds[portnum]->conf = &config_data.cmlds_port_conf[portnum];
        memcpy(ptp_global.ptp_cm.cmlds_port_ds[portnum]->status.portIdentity.clockIdentity, ptp_global.ptp_cm.cmlds_defaults.clockIdentity, CLOCK_IDENTITY_LENGTH);
        ptp_global.ptp_cm.cmlds_port_ds[portnum]->status.portIdentity.portNumber = portnum + 1;
        ptp_global.ptp_cm.cmlds_port_ds[portnum]->pDelay.port_cmlds = ptp_global.ptp_cm.cmlds_port_ds[portnum];
    }
}

static void ptp_clock_cmlds_init(ptp_clock_t *ptp_clk)
{
    for (uint portnum = 0; portnum < mesa_cap_port_cnt; portnum++) {
        ptp_clk->ptpPort[portnum].pDelay.port_cmlds = ptp_global.ptp_cm.cmlds_port_ds[portnum];
    }
}

static void ptp_cmlds_link_state_update()
{
    int port_cnt = port_count_max();

    for (int portnum = 0; portnum < port_cnt; portnum++)
        ptp_global.ptp_cm.cmlds_port_ds[portnum]->p2p_state = port_data[portnum].link_state && port_data[portnum].topo.port_ts_in_sync;
}
#endif //VTSS_SW_OPTION_P802_1_AS
static vtss_ptp_clock_mode_t my_mode = VTSS_PTP_CLOCK_FREERUN;

void vtss_local_clock_mode_set(vtss_ptp_clock_mode_t mode)
{
    int i;
    mesa_port_no_t portidx;
    if (mode != my_mode) {
        my_mode = mode;

        for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
            for (portidx = 0; portidx < config_data.conf[i].clock_init.numberPorts; portidx++) {
                if (vtss_ptp_port_internal_linkstate(ptp_global.ptpi[i], ptp_l2port_to_api(portidx)) == VTSS_RC_ERROR) {
                    T_EG(_I, "invalid port_no: %d", ptp_l2port_to_api(portidx));
                }
            }
        }
        T_IG(_C, "Clock mode %d", my_mode);
    }
}
void vtss_local_clock_mode_get(vtss_ptp_clock_mode_t *mode)
{
    *mode = my_mode;
}

bool calib_1pps_initiate = false;
bool calib_1pps_enabled = false;

static vtss::Timer pps_msg_timer;
static vtss::Timer alarm_msg_timer;

ptp_rs422_protocol_t ptp_rs422_proto_appl_to_internal_converter(vtss_ptp_appl_rs422_protocol_t appl_rs422)
{
    ptp_rs422_protocol_t ret;
    switch(appl_rs422) {
        case VTSS_PTP_APPL_RS422_PROTOCOL_SER_ZDA:
            ret = VTSS_PTP_RS422_PROTOCOL_SER_ZDA;
            break;
        case VTSS_PTP_APPL_RS422_PROTOCOL_SER_GGA:
            ret = VTSS_PTP_RS422_PROTOCOL_SER_GGA;
            break;
        case VTSS_PTP_APPL_RS422_PROTOCOL_SER_RMC:
            ret = VTSS_PTP_RS422_PROTOCOL_SER_RMC;
            break;
        case VTSS_PTP_APPL_RS422_PROTOCOL_PIM:
            ret = VTSS_PTP_RS422_PROTOCOL_PIM;
            break;
        case VTSS_PTP_APPL_RS422_PROTOCOL_SER_POLYT:
        default:
            ret = VTSS_PTP_RS422_PROTOCOL_SER_POLYT;
            break;
    }
    return ret;
}

ptp_rs422_mode_t ptp_virtual_to_rs422_mode_convert(vtss_appl_virtual_port_mode_t mode)
{
    ptp_rs422_mode_t ret;

    switch (mode) {
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO:
            ret = VTSS_PTP_RS422_MAIN_AUTO;
            break;
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN:
            ret = VTSS_PTP_RS422_MAIN_MAN;
            break;
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE:
        default:
            ret = VTSS_PTP_RS422_DISABLE;
            break;
    }
    return ret;
}

static void one_pps_msg_timer_expired(vtss::Timer *timer)
{
    PTP_CORE_LOCK_SCOPE();
    int instance_id = 0;
    mesa_timestamp_t t_next_pps;
    vtss_appl_virtual_port_mode_t port_mode = config_data.conf[instance_id].virtual_port_cfg.virtual_port_mode;

    T_DG(VTSS_TRACE_GRP_1_PPS, "One sec message timeout ");

    PTP_RC(mesa_ts_timeofday_prev_pps_get(NULL, &t_next_pps));
    t_next_pps.nanoseconds = config_data.init_ext_clock_mode_rs422.delay; /* Delay is manually configured or calculated */

    if (port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN || port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO ||
        port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT) {
        if (config_data.conf[instance_id].virtual_port_cfg.proto >= VTSS_PTP_APPL_RS422_PROTOCOL_SER_POLYT &&
            config_data.conf[instance_id].virtual_port_cfg.proto <= VTSS_PTP_APPL_RS422_PROTOCOL_SER_RMC) {
            ptp_1pps_msg_send(&t_next_pps, config_data.init_ext_clock_mode_rs422.proto);
        } else if (config_data.conf[instance_id].virtual_port_cfg.proto == VTSS_PTP_APPL_RS422_PROTOCOL_PIM) {
            ptp_pim_1pps_msg_send(config_data.conf[instance_id].virtual_port_cfg.portnum, &t_next_pps);
        }
    }
}
/*
 * timer used for calling Time stamp age function
 */
static void pps_msg_timer_init(void) {
    if (mesa_cap_ts_ptp_rs422 || mesa_cap_ts_has_ptp_io_pin) {
        pps_msg_timer.set_repeat(FALSE);
        pps_msg_timer.set_period(vtss::milliseconds(40));
        pps_msg_timer.callback    = one_pps_msg_timer_expired;
        pps_msg_timer.modid       = VTSS_MODULE_ID_PTP;
        T_IG(VTSS_TRACE_GRP_CLOCK, "pps_msg_timer initialized");
    }
}

// Forward declaration of mesa_rc udp_rx_filter_manage
static mesa_rc udp_rx_filter_manage(bool init, uint clockId, bool usesUDP);

// Function for updating the UDP rx filter (i.e. installing/deinstalling depending on whether one or more PTP clocks use UDP or not).
/*lint -sem(udp_rx_filter_update, thread_protected) */
static mesa_rc udp_rx_filter_update(uint clockId, bool usesUDP)
{
    mesa_rc rc;
    bool init = 0;

    rc = udp_rx_filter_manage(init, clockId, usesUDP);
    return rc;
}

static mesa_rc udp_rx_filter_manage(bool init, uint clockId, bool usesUDP)
{
    static packet_rx_filter_t udp_rx_filter;
    static void *filter_id_udp = NULL;
    static CapArray<bool, VTSS_APPL_CAP_PTP_CLOCK_CNT> clock_uses_udp;  // Note: All elements intialized to 0 i.e. FALSE by constructor.
    bool some_clock_uses_udp;
    mesa_rc rc = VTSS_RC_ERROR;
    int n;

    if (init == 1) {
        // Setup udp_rx_filter for use later
        packet_rx_filter_init(&udp_rx_filter);
        udp_rx_filter.modid = VTSS_MODULE_ID_PTP;
        udp_rx_filter.match = PACKET_RX_FILTER_MATCH_UDP_DST_PORT; // ethertype, IP protocol and UDP port match filter
        udp_rx_filter.udp_dst_port_min = PTP_EVENT_PORT;
        udp_rx_filter.udp_dst_port_max = PTP_GENERAL_PORT;
        udp_rx_filter.prio  = PACKET_RX_FILTER_PRIO_HIGH;
        udp_rx_filter.contxt = &ptp_global.ptpi;
        udp_rx_filter.cb     = packet_rx;
        udp_rx_filter.etype = ip_ether_type;
        udp_rx_filter.ip_proto = IPPROTO_UDP;
        udp_rx_filter.thread_prio = VTSS_THREAD_PRIO_HIGHER;

        // Install call back function to be used by vtss_ptp_port_ena and vtss_ptp_port_dis to update the UDP rx filter.
        vtss_ptp_install_udp_rx_filter_update_cb(udp_rx_filter_update);

        rc = VTSS_RC_OK;
    }
    else {
        if ((usesUDP == 1) && (clock_uses_udp[clockId] == 0)) {
            clock_uses_udp[clockId] = 1;
            if (filter_id_udp != NULL) {
                rc = packet_rx_filter_change(&udp_rx_filter, &filter_id_udp);
            }
            else {
                rc = packet_rx_filter_register(&udp_rx_filter, &filter_id_udp);
            }
            ptp_socket_init();
        }
        else if ((usesUDP == 0) && (clock_uses_udp[clockId] == 1)) {
            clock_uses_udp[clockId] = 0;
            some_clock_uses_udp = 0;
            for (n = 0; n < PTP_CLOCK_INSTANCES; n++) some_clock_uses_udp = some_clock_uses_udp || (clock_uses_udp[n] != 0);
            if (filter_id_udp != NULL) {
                if (some_clock_uses_udp != 0) {
                    rc = packet_rx_filter_change(&udp_rx_filter, &filter_id_udp);
                }
                else {
                    rc = packet_rx_filter_unregister(filter_id_udp);
                    if (rc == VTSS_RC_OK) filter_id_udp = NULL;
                }
            }
            else {
                T_E("Trying to change or unregister udp_rx_filter when no one is registered.");
                rc = VTSS_RC_ERROR;
            }
            ptp_socket_close();
        }
        else rc = VTSS_RC_OK;  // Nothing to do as port was already enabled / diabled.
    }
    return rc;
}

static mesa_rc ptp_ts_operation_mode_set(mesa_port_no_t port_no)
{
    mesa_rc rc = VTSS_RC_OK;
    if (mesa_cap_ts && mesa_cap_ts_internal_mode_supported) {
        mesa_ts_operation_mode_t mode;
        int i;
        // PHY exists only in clock domain 0.
        if (port_data[port_no].topo.ts_feature != VTSS_PTP_TS_PTS ||
            port_data[port_no].port_domain) {
            mode.mode = MESA_TS_MODE_EXTERNAL;
            for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
                if (config_data.conf[i].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE && config_data.conf[i].port_config[port_no].portInternal) {
                    mode.mode = MESA_TS_MODE_INTERNAL;
                }
            }
            if (mesa_cap_ts_domain_cnt > 1) {
                mode.domain = port_data[port_no].port_domain;
                T_I("set ts_operation mode to %d, domain %d for port %d", mode.mode, mode.domain, iport2uport(port_no));
            } else {
                T_I("set ts_operation mode to %d, for port %d", mode.mode, iport2uport(port_no));
            }
            PTP_RC(mesa_ts_operation_mode_set(NULL, port_no, &mode));
            port_data[port_no].backplane_port = (mode.mode == MESA_TS_MODE_INTERNAL) ? true : false;
        } else {
            T_D("ts_operation mode unchanged for port %d", iport2uport(port_no));
        }
    }
    return rc;
}

static void ptp_phy_ts_init(void)
{
    if (mepa_phy_ts_cap()) {
        while (!tod_ready()) {
            T_I("wait until TOD module is ready");
            VTSS_OS_MSLEEP(100);
        }
    }
}

static void ptp_packet_rx_register(void)
{
    void               *filter_id;
    packet_rx_filter_t rx_filter;
    mesa_rc            rc;

    // Register for PTP frames (frames with ethertype == ptp_ether_type)
    packet_rx_filter_init(&rx_filter);
    rx_filter.modid       = VTSS_MODULE_ID_PTP;
    rx_filter.match       = PACKET_RX_FILTER_MATCH_ETYPE; // only ethertype filter
    rx_filter.prio        = PACKET_RX_FILTER_PRIO_HIGH;
    rx_filter.contxt      = &ptp_global.ptpi;
    rx_filter.cb          = packet_rx;
    rx_filter.etype       = ptp_ether_type;
    rx_filter.thread_prio = VTSS_THREAD_PRIO_HIGHER;
    rx_filter.mtu = 10000; // Accept frames up to 10000 bytes to avoid filtering ptp announce packets with a large path trace - AVNU test case 223.1b

    if ((rc = packet_rx_filter_register(&rx_filter, &filter_id)) != MESA_RC_OK) {
        T_E("Unable to register for PTP packets. rc = %s", error_txt(rc));
    }

    // Also register for UDP PTP frames
    if ((rc = udp_rx_filter_manage(true, 0, false)) != MESA_RC_OK) {
         T_E("Unable to register for UDP PTP frames. rc = %s", error_txt(rc));
    }
}


static void ptp_do_init(void)
{
    int                                 i, j;
    mesa_rc                             rc;
    vtss_appl_ptp_virtual_port_config_t cfg;

    // Wait until time-of-day is ready
    ptp_phy_ts_init();

    PTP_CORE_LOCK();

    // Initialize the pulse-per-second timer, which gets kicked by a PPS and
    // causes the one_pps_msg_timer_expired() function to be invoked 40 ms later
    pps_msg_timer_init();

    // Initialise timers used as waiting time for switching to hybrid mode and short term transient state.
    hybrid_switch_timer_init();
    transient_timer_init();

    if (mepa_phy_ts_cap()) {
        /* Initialize PHY ts rules */
        phy_ts_rules_init();
    }

    // Initialize port data
    port_data_init();

    // Local clock initialization
    vtss_local_clock_initialize();

    // Register for VLAN interface changes (IP addresses)
    PTP_RC(vtss_ip_if_callback_add(notify_ptp_ip_address));

#if defined(VTSS_SW_OPTION_MEP)
    // Note: The value 0 indicates first ptp clock instance.
    vtss_ptp_timer_init(&oam_timer, "oam", -1, ptp_oam_slave_timer, 0);
#endif

    for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        vtss_ptp_timer_init(&ptp_virtual_port[i].virtual_port_monitor_timer, "virtual port monitor", i, ptp_virtual_port_monitor, ptp_global.ptpi[i]);
        vtss_ptp_timer_init(&ptp_virtual_port[i].virtual_port_alarm_timer, "virtual port alarm", i, ptp_virtual_port_alarm_generator, ptp_global.ptpi[i]);
    }

    if (mepa_phy_ts_cap()) {
        if (!tod_tc_mode_get(&phy_ts_mode)) {
            T_W("Missed tc_mode_get");
        }
    }

    // Note: The value 0 indicates first ptp clock instance.
    vtss_ptp_timer_init(&sys_time_sync_timer, "sys_time_sync", -1, ptp_sys_time_sync_function, 0);
    ptp_time_settling_init();

    ptp_packet_rx_register();

    // Port link state change callback
    if ((rc = port_change_register(VTSS_MODULE_ID_PTP, ptp_port_state_change_callback)) != MESA_RC_OK) {
        T_E("Unable to register for link state changes. Error = %s", error_txt(rc));
    }

    // Wait 500 ms or the ACL setup may fail
    VTSS_OS_MSLEEP(500);

    // Initialize the clock instances
    for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        port_iter_t pit;

        initial_port_filter_get(i);

        // Update serval backplane mode
        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            j = pit.iport;
            PTP_RC(ptp_ts_operation_mode_set(j));
        }

#if defined(VTSS_SW_OPTION_MEP)
        if (mesa_cap_oam_used_as_ptp_protocol) {
            if (i == 0 && config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY && config_data.conf[i].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM) {
                vtss_ptp_timer_start(&oam_timer, OAM_TIMER_INIT_VALUE, false);
            }
        }
#endif
    }

    ptp_conf_propagate();

    ptp_global.ready = true; /* Ready to rock */
    T_I("Clock initialized");

    // Register Initialport state (portstate changes before setting
    // ptp_global.ready are ignored).
    for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        ptp_port_link_state_initial(i);
	if((rc = vtss_appl_ptp_clock_config_virtual_port_config_get(i, &cfg)) == VTSS_RC_OK){
		if(cfg.virtual_port_mode != VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE){
			rc = ptp_clock_config_virtual_port_config_set(i, &cfg);
			if(rc != VTSS_RC_OK)
				T_W("Error in setting virtual_port_config, rc=%d", rc);
		}
	}
    }

#if defined (VTSS_SW_OPTION_P802_1_AS)
    ptp_cmlds_link_state_update();
#endif

    PTP_CORE_UNLOCK();

    /* Start system-time <-> PTP time synchronization */
    if (system_time_sync_conf.mode == VTSS_APPL_PTP_SYSTEM_TIME_SYNC_GET) {
        // If startup config contains a "ptp system-time get 0", the command is called before
        // PTP has been initialized. In that case, set time now.
        mesa_timestamp_t t;
        t.sec_msb = 0;
        t.seconds = time(NULL);
        t.nanoseconds = 0;
        ptp_local_clock_time_set(&t, ptp_instance_2_timing_domain(system_time_sync_conf.clockinst));
    }
    if (system_time_sync_conf.mode != VTSS_APPL_PTP_SYSTEM_TIME_NO_SYNC) {
        PTP_RC(vtss_appl_ptp_system_time_sync_mode_set(&system_time_sync_conf));
    }

    T_I("Set ACL rules");
    for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        if ((rc = ptp_ace_update(i)) != MESA_RC_OK) {
            if (rc != PTP_RC_MISSING_IP_ADDRESS) {
                T_EG(VTSS_TRACE_GRP_ACE, "ACE update error = %s", error_txt(rc));
            } else {
                // Not a problem, ACE will be configured when IP address is configured.
                // In case of DHCP it may take a while before IP address is configured.
            }
        }
    }
}

static inline void ptp_reload_defaults(void)
{
    // No existing transparent clock after reset
    transparent_clock_exists = false;

    // Clear configuration
    ptp_conf_read(true);

    // Make PTP configuration effective in PTP core
    ptp_conf_propagate();
}

static inline void ptp_1sec_timeout(void)
{
    static int next_instance_to_poll;
    int i;

    for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        if (config_data.conf[i].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            vtss_ptp_tick(ptp_global.ptpi[i]);
            if (i == next_instance_to_poll                                                &&
                config_data.conf[i].clock_init.cfg.protocol != VTSS_APPL_PTP_PROTOCOL_OAM &&
                config_data.conf[i].clock_init.cfg.protocol != VTSS_APPL_PTP_PROTOCOL_ONE_PPS) {
                T_I("Poll port filter (instance = %d)", i);
                poll_port_filter(i);
            }

#if defined(VTSS_SW_OPTION_SYNCE)
            if (config_data.conf[i].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_1 ||
                config_data.conf[i].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_2) {
                // Update clock class from current oscillator quality.
                T_I("Update clock class (instance = %d)", i);
                vtss_appl_synce_quality_level_t ql;
                PTP_RC(synce_mgmt_clock_ql_get(&ql));
                u8 clock_class = g8275_ql_to_class(ql, config_data.conf[i].clock_init.cfg.deviceType);
                if (ptp_global.ptpi[i]->local_clock_class != clock_class || ptp_global.ptpi[i]->holdover_timeout_spec != g8275_ho_spec(ql)) {
                    T_I("Change local clock class to %u", clock_class);
                    ptp_global.ptpi[i]->local_clock_class = clock_class;
                    ptp_global.ptpi[i]->holdover_timeout_spec = g8275_ho_spec(ql);
                    defaultDSChanged(ptp_global.ptpi[i]);
                }
            }
#endif /* defined(VTSS_SW_OPTION_SYNCE) */
        }
    }

    // Update servo mode according to Synce selector state, adjustment method
    // filter type and profile.
    T_NG(VTSS_TRACE_GRP_MS_SERVO, "Update selected source");
    PTP_RC(vtss_ptp_update_selected_src());

#if defined (VTSS_SW_OPTION_P802_1_AS)
    poll_cmlds_conf_updates();
#endif

    // Run the port filter every PTP_CLOCK_INSTANCES seconds for a particular
    // instance.
    next_instance_to_poll++;
    if (next_instance_to_poll >= PTP_CLOCK_INSTANCES) {
        next_instance_to_poll = 0;
    }
}

static inline void ptp_21sec_timeout(void)
{
    // Re-initialize socket each 21 sec, to reconnect if the vlan has been down
    T_I("21sec timeout");
    if (ptp_sock > 0) {
        T_I("socket_init");
        ptp_socket_init();
    }
}

// Length of one tick in microseconds and nanoseconds
u64 tick_duration_us;
u64 tick_duration_ns;

static void ptp_tick_init(void)
{
    // PTP_TICK_FACTOR defines how many ticks to invoke vtss_ptp_timer_tick()
    // with for each callback from this thread.
    // The value is normally 1 indicating a period og 1/128 sec.
    // In slow CPU systems, the value can be incremented to reduce the CPU load.
    // The result of this is that the max packet rate decreases.
#if defined(VTSS_MAX_LOG_MESSAGE_RATE) && VTSS_MAX_LOG_MESSAGE_RATE >= -7 && VTSS_MAX_LOG_MESSAGE_RATE <= 0
    #define PTP_TICK_FACTOR (1 << (VTSS_MAX_LOG_MESSAGE_RATE + 7))
#else
    #define PTP_TICK_FACTOR 1
#endif

    // Tick duration = X / 128 second = 7,812,500 ns => 7,812 us
    tick_duration_ns = (u64)PTP_TICK_FACTOR * TickTimeNs;
    tick_duration_us = tick_duration_ns / 1000LLU;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER, "PTP_TICK_FACTOR = %u => tick_duration_ns = " VPRI64u ", tick_duration_us " VPRI64u, PTP_TICK_FACTOR, tick_duration_ns, tick_duration_us);
}

static inline u64 ptp_tick_align(u64 timeout_us)
{
    // Compute the next timeout of vtss_ptp_timer_tick() given we want a timeout
    // to happen at timeout_us, and given that we don't want to invoke the
    // function more than 1000000 / tick_duration_us, which (normally) equals
    // 128 times per second.
    // The thing is that suppose one timer wants to time out at 12000 us and
    // another timer wants to time out at 13000 us. If tick duration is 7812 us,
    // we do not want vtss_ptp_timer_tick() to time out at both 12000 and 13000
    // us, but only once, namely at 2 * 7812 = 15624 us, where both timers will
    // fire.

    // Compute the next timeout so that it's an integral number of ticks.
    // Example:
    //   timeout_us = 12000, tick_duration_us = 7812.
    //   result_us = 7812 * PTP_DIV_ROUND_UP(12000, 7812) = 7812 * 2 = 15624 us.
    u64 next_timeout_us = tick_duration_us * PTP_DIV_ROUND_UP(timeout_us, tick_duration_us);

    // Since the thread timeouts are measured in milliseconds, we need to do
    // another rounding.
    u64 next_timeout_ms = PTP_DIV_ROUND_UP(next_timeout_us, 1000LLU);

    T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER, "Requested timeout_us = " VPRI64u " => next_timeout_ms = " VPRI64u, timeout_us, next_timeout_ms);

    return next_timeout_ms;
}

static inline u64 ptp_tick_timeout(u64 now_us)
{
    // Call the tick function and get back the absolute time (in microseconds)
    // that the next timer times out.
    u64 next_timeout_us = vtss_ptp_timer_tick(now_us);

    // Convert that to an integral number of ticks and further to milliseconds.
    if (next_timeout_us == (u64)-1) {
        return (u64)-1;
    } else {
        return ptp_tick_align(next_timeout_us);
    }
}

static inline u64 timeout_calc(u64 to0, u64 to1, u64 to2)
{
    u64 to;
    to = MIN(to0, to1);
    to = MIN(to,  to2);
    return to;
}

#define ENABLE_SEGFAULT_HANDLER 0
#if ENABLE_SEGFAULT_HANDLER
void (*signal(int signum, void (*sighandler)(int)))(int);
void seg_signal_handler(int sig_num)
{
        T_EG(VTSS_TRACE_GRP_1_PPS,"\n ------   coredump %d handled ------------ ",sig_num);
        signal(SIGSEGV, seg_signal_handler);
}
#endif

// Needs to be kickstarted
static volatile u64 next_tick_timeout_ms = (u64)-1;

/****************************************************************************
 * Module thread
 ****************************************************************************/
static void ptp_thread(vtss_addrword_t data)
{
    vtss_flag_value_t flags;
    u64               next_1sec_timeout_ms  = 0; // Time out at first given occassion
    u64               next_21sec_timeout_ms = 0; // Time out at first given occassion
    u64               next_timeout_ms;
    u32               invocation_cnt = 0, flag_invocation_cnt = 0;

#if ENABLE_SEGFAULT_HANDLER
    signal(SIGSEGV, seg_signal_handler);
#endif

    ptp_do_init();

    // Time out one second from now.
    next_timeout_ms = vtss::uptime_milliseconds() + 1000;

    while (1) {
        u64 now_us, now_ms;

        flags = vtss_flag_timed_wait(&ptp_flags, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR, VTSS_OS_MSEC2TICK(next_timeout_ms));
        invocation_cnt++;

        if (flags) {
            flag_invocation_cnt++;
        }

        now_us = vtss::uptime_microseconds();
        now_ms = now_us / 1000LLU;

        PTP_CORE_LOCK();

        if (flags & PTP_FLAG_DEFCONFIG) {
            ptp_reload_defaults();
        }

        if (flags & PTP_FLAG_TIMER_START) {
            // We need a new tick timeout.
            next_timeout_ms = timeout_calc(next_1sec_timeout_ms, next_21sec_timeout_ms, next_tick_timeout_ms);

            // Just compute the next timeout and use it, but don't invoke any
            // timeout functions (that will happen in just a split second if
            // next_timeout_ms <= now_ms.
            PTP_CORE_UNLOCK();
            continue;
        }

        if (next_tick_timeout_ms <= now_ms) {
            u64 old_next_tick_timeout_ms = next_tick_timeout_ms;
            next_tick_timeout_ms = ptp_tick_timeout(now_us);
            T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER, "Tick timeout. now = " VPRI64u " ms, old-tick-timeout = " VPRI64u " ms, new-tick-timeout = " VPRI64u " ms",
                 now_ms,
                 old_next_tick_timeout_ms,
                 next_tick_timeout_ms);
        }

        if (next_1sec_timeout_ms <= now_ms) {
            ptp_1sec_timeout();
            next_1sec_timeout_ms = now_ms + 1000;
            T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER, "1sec timeout. next_1sec_timeout @ " VPRI64u " ms", next_1sec_timeout_ms);

            // Debug
            T_IG(VTSS_TRACE_GRP_PTP_BASE_TIMER, "Invoked %u times the last second, due to flags = %u times", invocation_cnt, flag_invocation_cnt);
            invocation_cnt = 0;
            flag_invocation_cnt = 0;
        }

        if (next_21sec_timeout_ms <= now_ms) {
            ptp_21sec_timeout();
            next_21sec_timeout_ms = now_ms + 21000;
        }

        next_timeout_ms = timeout_calc(next_1sec_timeout_ms, next_21sec_timeout_ms, next_tick_timeout_ms);

        T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER, "now_ms = " VPRI64u ", next_timeout_ms = " VPRI64u ", i.e. in " VPRI64u " ms",
             now_ms,
             next_timeout_ms,
             next_timeout_ms - now_ms);

        PTP_CORE_UNLOCK();
    }
}

/****************************************************************************/
/*  PTP callout functions                                                  */
/****************************************************************************/

void ptp_timer_tick_start(const vtss_ptp_sys_timer_t *const timer)
{
    // Avoid kick-starting the thread if not necessary.
    u64 next_timeout_ms = ptp_tick_align(timer->timeout_us);

    if (next_timeout_ms < next_tick_timeout_ms) {
        // The new tick timeout is before the current, so we need to signal the
        // thread to get it to timeout sooner.
        T_IG(VTSS_TRACE_GRP_PTP_BASE_TIMER, "%s#%d: Kickstarting timer. Requested timeout_us = " VPRI64u ", next_tick_timeout_ms was " VPRI64u ", next_timeout_ms = " VPRI64u,
             timer->name,
             timer->instance,
             timer->timeout_us,
             next_tick_timeout_ms,
             next_timeout_ms);

        next_tick_timeout_ms = next_timeout_ms;
        vtss_flag_setbits(&ptp_flags, PTP_FLAG_TIMER_START);
    }
}

/*
 * calculate difference between two ts counters.
 */
void vtss_1588_ts_cnt_sub(u64 *r, u64 x, u64 y)
{
    vtss_tod_ts_cnt_sub(r, x, y);
}

/*
 * calculate sum of two ts counters.
 */
void vtss_1588_ts_cnt_add(u64 *r, u64 x, u64 y)
{
    vtss_tod_ts_cnt_add(r, x, y);
}

void vtss_1588_timeinterval_to_ts_cnt(u64 *r, mesa_timeinterval_t x)
{
    vtss_tod_timeinterval_to_ts_cnt(r, x);
}

void vtss_1588_ts_cnt_to_timeinterval(mesa_timeinterval_t *r, u64 x)
{
    vtss_tod_ts_cnt_to_timeinterval(r, x);
}

mesa_rc vtss_1588_ingress_latency_set(u16 portnum, mesa_timeinterval_t ingress_latency)
{
    mesa_rc rc = VTSS_RC_OK;
    T_DG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, ingress_latency " VPRI64d, portnum, ingress_latency);

    // Read ingress calibration and compensate ingress latency accordingly
    mesa_timeinterval_t ingress_calib_value = 0;
    {
        // Determine port mode
        vtss_appl_port_status_t port_status;
        vtss_ifindex_t ifindex;
        (void)vtss_ifindex_from_port(0, uport2iport(portnum), &ifindex);
        if (vtss_appl_port_status_get(ifindex, &port_status) == MESA_RC_OK) {

            BOOL fiber = port_status.fiber;

            switch (port_status.speed) {
                case MESA_SPEED_10M:   {
                                           ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_10m_cu.ingress_latency;
                                           break;
                                       }
                case MESA_SPEED_100M:  {
                                           ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_100m_cu.ingress_latency;
                                           break;
                                       }
                case MESA_SPEED_1G:    {
                                           if (fiber) {
                                               ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_1g.ingress_latency;
                                           } else {
                                               ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_1g_cu.ingress_latency;
                                           }
                                           break;
                                       }
                case MESA_SPEED_2500M: {
                                           ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_2g5.ingress_latency;
                                           break;
                                       }
                case MESA_SPEED_5G:    {
                                           ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_5g.ingress_latency;
                                           break;
                                       }
                case MESA_SPEED_10G:   {
                                           ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_10g.ingress_latency;
                                           break;
                                       }
                case MESA_SPEED_25G:   if ((port_status.fec_mode == VTSS_APPL_PORT_FEC_MODE_RS_FEC) || (port_status.fec_mode == VTSS_APPL_PORT_FEC_MODE_AUTO))
                                       {
                                           ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_25g_rsfec.ingress_latency;
                                       } else {
                                           ingress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_25g_nofec.ingress_latency;
                                       }
                                       break;
                default: {
                             ingress_calib_value = 0;
                         }
            }
            ingress_latency += ingress_calib_value;
        }
    }

    if (port_data[ptp_api_to_l2port(portnum)].topo.ts_feature == VTSS_PTP_TS_PTS &&
        port_data[ptp_api_to_l2port(portnum)].port_domain == 0) {
        T_IG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, ingress_latency = " VPRI64d " ns", portnum, ingress_latency>>16);
        rc = meba_phy_ts_ingress_latency_set(board_instance, ptp_api_to_l2port(portnum), &ingress_latency);
        if (rc == MESA_RC_NOT_IMPLEMENTED) {
            T_WG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, PHY timestamping not supported", portnum);
        }
    } else {
        rc = mesa_ts_ingress_latency_set(0, ptp_api_to_l2port(portnum), &ingress_latency);
    }
    if (rc == VTSS_RC_OK) {
        port_data[ptp_api_to_l2port(portnum)].ingress_latency = ingress_latency - ingress_calib_value;
    } else {
        T_WG(VTSS_TRACE_GRP_PHY_TS, "Could not set ingress latency value in HW. The calibration for port %d is likely invalid. You may want to reset the calibration and/or recalibrate.", portnum);
    }
    return rc;
}

void vtss_1588_ingress_latency_get(u16 portnum, mesa_timeinterval_t *ingress_latency)
{
     *ingress_latency = port_data[ptp_api_to_l2port(portnum)].ingress_latency;
     T_DG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, ingress_latency " VPRI64d, portnum, *ingress_latency);
}

mesa_rc vtss_1588_egress_latency_set(u16 portnum, mesa_timeinterval_t egress_latency)
{
    mesa_rc rc = VTSS_RC_OK;

    // Read egress calibration and compensate egress latency accordingly
    mesa_timeinterval_t egress_calib_value = 0;
    {
        // Determine port mode
        vtss_appl_port_status_t port_status;
        vtss_ifindex_t ifindex;
        (void)vtss_ifindex_from_port(0, uport2iport(portnum), &ifindex);

        if (vtss_appl_port_status_get(ifindex, &port_status) == MESA_RC_OK) {

            BOOL fiber = port_status.fiber;

            switch (port_status.speed) {
                case MESA_SPEED_10M:   {
                                           egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_10m_cu.egress_latency;
                                           break;
                                       }
                case MESA_SPEED_100M:  {
                                           egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_100m_cu.egress_latency;
                                           break;
                                       }
                case MESA_SPEED_1G:    {
                                           if (fiber) {
                                               egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_1g.egress_latency;
                                           } else {
                                               egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_1g_cu.egress_latency;
                                           }
                                           break;
                                       }
                case MESA_SPEED_2500M: {
                                           egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_2g5.egress_latency;
                                           break;
                                       }
                case MESA_SPEED_5G:    {
                                           egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_5g.egress_latency;
                                           break;
                                       }
                case MESA_SPEED_10G:   {
                                           egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_10g.egress_latency;
                                           break;
                                       }
                case MESA_SPEED_25G:   if ((port_status.fec_mode == VTSS_APPL_PORT_FEC_MODE_RS_FEC) || (port_status.fec_mode == VTSS_APPL_PORT_FEC_MODE_AUTO))
                                       {
                                           egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_25g_rsfec.egress_latency;
                                       } else {
                                           egress_calib_value = ptp_port_calibration.port_calibrations[uport2iport(portnum)].port_latencies_25g_nofec.egress_latency;
                                       }
                                       break;
                default: {
                             egress_calib_value = 0;
                         }
            }
            egress_latency += egress_calib_value;
        }
    }

    if (port_data[ptp_api_to_l2port(portnum)].topo.ts_feature == VTSS_PTP_TS_PTS &&
        port_data[ptp_api_to_l2port(portnum)].port_domain == 0) {
        T_DG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, egress_latency = " VPRI64d " ns", portnum, egress_latency>>16);
        rc = meba_phy_ts_egress_latency_set(board_instance, ptp_api_to_l2port(portnum), &egress_latency);
        if (rc == MESA_RC_NOT_IMPLEMENTED) {
            T_WG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, PHY timestamping not supported", portnum);
        }
    } else {
        rc = mesa_ts_egress_latency_set(0, ptp_api_to_l2port(portnum), &egress_latency);
    }
    if (rc == VTSS_RC_OK) {
        port_data[ptp_api_to_l2port(portnum)].egress_latency = egress_latency - egress_calib_value;
    } else {
        T_WG(VTSS_TRACE_GRP_PHY_TS, "Could not set egress latency value in HW. The calibration for port %d is likely invalid. You may want to reset the calibration and/or recalibrate.", portnum);
    }
    return rc;
}

void vtss_1588_egress_latency_get(u16 portnum, mesa_timeinterval_t *egress_latency)
{
    *egress_latency = port_data[ptp_api_to_l2port(portnum)].egress_latency;
}

void vtss_1588_p2p_delay_set(u16 portnum,
                             mesa_timeinterval_t p2p_delay)
{
    mepa_timeinterval_t phy_dly = p2p_delay < 0 ? 0 : p2p_delay;
    mesa_rc rc;
    if (0 < portnum && portnum <= config_data.conf[0].clock_init.numberEtherPorts) {
        if (port_data[ptp_api_to_l2port(portnum)].topo.ts_feature == VTSS_PTP_TS_PTS &&
            port_data[ptp_api_to_l2port(portnum)].port_domain == 0) {
            rc = meba_phy_ts_path_delay_set(board_instance, ptp_api_to_l2port(portnum), &phy_dly);
            if (rc != MESA_RC_OK) {
                T_WG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, PHY ts path delay not set rc %d", portnum, rc);
            }
        } else {
            u64 delay_cnt;
            vtss_1588_timeinterval_to_ts_cnt(&delay_cnt, p2p_delay);
            port_data[ptp_api_to_l2port(portnum)].delay_cnt = delay_cnt;
            T_DG(_I, "port %d, delay_cnt = %d, peer_delay = " VPRI64d " ns", portnum, delay_cnt, p2p_delay>>16);

            PTP_RC(mesa_ts_p2p_delay_set(0, ptp_api_to_l2port(portnum), &p2p_delay));
        }
    } else {
        T_IG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, is a non ethernet port", portnum);
    }
}

mesa_rc vtss_1588_asymmetry_set(u16 portnum,
                             mesa_timeinterval_t asymmetry)
{
    mesa_rc rc = VTSS_RC_OK;

    if (port_data[ptp_api_to_l2port(portnum)].topo.ts_feature == VTSS_PTP_TS_PTS &&
       (port_data[ptp_api_to_l2port(portnum)].port_domain == 0)) {
        rc = meba_phy_ts_delay_asymmetry_set(board_instance, ptp_api_to_l2port(portnum), &asymmetry);
        if (rc != MESA_RC_OK) {
            T_WG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, PHY ts asymmetry not set rc %d", portnum, rc);
        }
    } else {
        u64 asy_cnt;
        vtss_1588_timeinterval_to_ts_cnt(&asy_cnt, asymmetry);
        port_data[ptp_api_to_l2port(portnum)].asymmetry_cnt = asy_cnt;
        T_IG(_I, "port %d, asymmetry = %d", portnum, asy_cnt);

        if (mesa_cap_ts_asymmetry_comp) {
            rc = mesa_ts_delay_asymmetry_set(0, ptp_api_to_l2port(portnum), &asymmetry);
        }
    }
    if (rc == VTSS_RC_OK) {
        port_data[ptp_api_to_l2port(portnum)].asymmetry = asymmetry;
    }
    return rc;
}

void vtss_1588_asymmetry_get(u16 portnum,
                             mesa_timeinterval_t *asymmetry)
{
    *asymmetry = port_data[ptp_api_to_l2port(portnum)].asymmetry;
}

u16 vtss_ptp_get_rand(u32 *seed)
{
    return rand_r((unsigned int*)seed);
}

void *vtss_ptp_calloc(size_t nmemb, size_t sz)
{
    return VTSS_CALLOC(nmemb, sz);
}

void vtss_ptp_free(void *ptr)
{
    VTSS_FREE(ptr);
}

/****************************************************************************/
/*  API functions                                                           */
/****************************************************************************/

static mesa_rc ptp_servo_instance_set(uint instance)
{
    bool basic_servo_used = false;
#ifdef SW_OPTION_BASIC_PTP_SERVO
    if (config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_BASIC) {
        (void) vtss_ptp_clock_servo_set(ptp_global.ptpi[instance], basic_servo[instance]);
        basic_servo_used = true;
    } else
#endif // SW_OPTION_BASIC_PTP_SERVO
#if defined(VTSS_SW_OPTION_ZLS30387)
    if (config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_DEFAULT ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_XO ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_XO ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_TCXO ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_TCXO ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_OCXO_S3E ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_FREQ ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_PHASE ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_FREQ ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_FDD ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_XDSL ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_FREQ ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_PHASE ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C60W ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C150 ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C180 ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C240 ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E_R4_6_1 ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE ||
            config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE_LOW
       )
    {
        (void) vtss_ptp_clock_servo_set(ptp_global.ptpi[instance], advanced_servo[instance]);
    } else
#endif
    {
        T_E("No PTP servo available.");
        return VTSS_RC_ERROR;
    }

    if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
        if (VTSS_RC_OK != vtss_local_clock_adj_method((uint32_t)instance, config_data.init_ext_clock_mode.adj_method, config_data.conf[instance].clock_init.cfg.profile,
                                                      config_data.conf[instance].clock_init.cfg.clock_domain, basic_servo_used)) {
            T_W("vtss_local_clock_adj_method returned error");
        }
    }
    
    PTP_RETURN(ptp_global.ptpi[instance]->ssm.servo->activate(config_data.conf[instance].clock_init.cfg.clock_domain,
                                                              config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_1 || config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_2));
    int ref;
    if ((ref = find_active_reference(instance, true)) != -1) {
        (void)vtss_ptp_set_active_ref(ref);
    }
    return VTSS_RC_OK;
}

static mesa_rc ptp_clock_create(const ptp_init_clock_ds_t *initData, uint instance, bool re_init)
{
    mesa_rc rc = VTSS_RC_ERROR;
    int j;

    if ((initData->cfg.configured_vid < 1) || (initData->cfg.configured_vid > 4095)) {
        T_E("Cannot create clock with illegal VLAN ID");
        return VTSS_RC_ERROR;
    }
    /* after change to icfg, this function is initially called before the PTP application is ready */
    /* in this case we only save the configuration, and does not update the HW */
    T_I("PTP_READY %d, deviceType %d, transparent_clock_exists %d", PTP_READY(), initData->cfg.deviceType, transparent_clock_exists);
    if (instance < PTP_CLOCK_INSTANCES) {
        // apply default filter and servo parameters
        vtss_appl_ptp_filter_default_parameters_get(&config_data.conf[instance].filter_params, initData->cfg.profile);
        vtss_appl_ptp_clock_servo_default_parameters_get(&config_data.conf[instance].servo_params, initData->cfg.profile);

        // virtual port config is initialized only when new PTP configuration is created.
        // If there is any change in existing configuration, virtual port config is not modified.
        if (!re_init) {
            // apply default virtual port config
            vtss_appl_ptp_clock_config_default_virtual_port_config_get(&config_data.conf[instance].virtual_port_cfg);
        }

        // Check if other clocks than this one exist in the same clock domain.
        // If so then update the configuration of the new clock so that it uses the same servo type as the already existing clocks in the same domain.
        for (int i = 0; i < PTP_CLOCK_INSTANCES; i++) {
            if ((i != instance) &&
                (ptp_global.ptpi[i]->clock_init->cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) &&
                (ptp_global.ptpi[i]->clock_init->cfg.clock_domain == ptp_global.ptpi[instance]->clock_init->cfg.clock_domain))
            {
                config_data.conf[instance].clock_init.cfg.filter_type = config_data.conf[i].clock_init.cfg.filter_type;
                break;
            }
        }

        if (initData->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY && initData->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM) {
            /* check if OAM instance is active */
            if (instance == 0) {
                config_data.conf[instance].clock_init.cfg.mep_instance = initData->cfg.mep_instance;
                config_data.conf[instance].clock_init.cfg.priority1 = 255;
            }
        }
        if (PTP_READY()) {
            if (initData->cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT || initData->cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT) {
                if (transparent_clock_exists) {
                    config_data.conf[instance].clock_init.cfg.deviceType = VTSS_APPL_PTP_DEVICE_NONE;
                    return PTP_RC_MULTIPLE_TC;
                } else
                    transparent_clock_exists = true;
            }
            T_I("clock init set: portcount = %d, deviceType = %d",config_data.conf[instance].clock_init.numberPorts, config_data.conf[instance].clock_init.cfg.deviceType);
            (void)update_ptp_ip_address(instance, initData->cfg.configured_vid);
            if (initData->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY && initData->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM) {
#if defined(VTSS_SW_OPTION_MEP)
                if (mesa_cap_oam_used_as_ptp_protocol) {
                    /* check if OAM instance is active */
                    if (instance == 0) {
                        vtss_ptp_clock_create(ptp_global.ptpi[instance]);
                        /* start timer for reading OAM timestamps */
                        vtss_ptp_timer_start(&oam_timer, 128, false);
                        rc = VTSS_RC_OK;
                    }
                } else {
                    T_W("OAM protocol not supported.");
                }
#else
                T_W("OAM protocol not supported.");
#endif
            } else if (initData->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY && initData->cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM) {
#if defined(VTSS_SW_OPTION_MEP)
                if (mesa_cap_oam_used_as_ptp_protocol) {
                    /* check if OAM instance is active */
                    if (instance == 0) {
                        config_data.conf[instance].clock_init.cfg.mep_instance = initData->cfg.mep_instance;
                        vtss_ptp_clock_create(ptp_global.ptpi[instance]);
                        rc = VTSS_RC_OK;
                    }
                } else {
                    T_W("OAM protocol not supported.");
                }
#else
                T_W("OAM protocol not supported.");
#endif
            } else {
                if ((rc = ptp_ace_update(instance)) != VTSS_RC_OK) {
                    T_W("ACE add error");
                    config_data.conf[instance].clock_init.cfg.deviceType = VTSS_APPL_PTP_DEVICE_NONE;
                    return rc;
                }
                if (initData->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY) {
                    if (initData->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_1) {
                        config_data.conf[instance].clock_init.cfg.priority1 = 128;
                    } else {
                        config_data.conf[instance].clock_init.cfg.priority1 = 255;
                    }
                }
                vtss_ptp_clock_create(ptp_global.ptpi[instance]);
                for (j = 0; j < MAX_UNICAST_MASTERS_PR_SLAVE; j++) {
                    vtss_ptp_uni_slave_conf_set(ptp_global.ptpi[instance], j, &config_data.conf[instance].unicast_slave[j]);
                }
                ptp_port_link_state_initial(instance);
            }

            vtss_ptp_clock_slave_config_set(ptp_global.ptpi[instance]->ssm.servo, &config_data.conf[instance].slave_cfg);

            rc = ptp_servo_instance_set(instance);

            // virtual port config cannot be accessed in announce to timer callbacks. Hence, setting the state during re-initialization.
            if (re_init &&
               (config_data.conf[instance].virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN ||
                config_data.conf[instance].virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT)) {
                // Virtual port is the succeeding number of the last ethernet port. So, same number will be used by virtual port.
                vtss_ptp_virtual_port_master_state_init(ptp_global.ptpi[instance], mesa_cap_port_cnt);
            }
        } else {
            rc = VTSS_RC_OK;
        }
    }
    return rc;
}
/* This function finds active reference among existing instances when a instance 'inst'
   is created or deleted. priority is given to lowest numbered instance. */
/* Currently, this function is used for only 8275 profile. For 8265 profile, synce updates
   active instance through vtss_ptp_update_selected_src. */
static int find_active_reference(int inst, bool create)
{
    if (config_data.conf[inst].clock_init.cfg.profile != VTSS_APPL_PTP_PROFILE_G_8275_1) {
        return -1;
    }
    for (int i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        if (!create && i == inst) {
            continue;
        }
        /* Multiple instances are supported in clock domain 0. */
        if (config_data.conf[i].clock_init.cfg.clock_domain != 0) {
            continue;
        }
        if (config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY ||
            config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) {
            return i;
        }
    }
    return -1; // no instance found
}
mesa_rc vtss_appl_ptp_clock_config_default_ds_del(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    int j;
    port_iter_t       pit;
    vtss_appl_ptp_virtual_port_config_t virtual_port_cfg;
    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES) && (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE)) {
        int ref;
        if ((ref = find_active_reference(instance, false)) != -1) {
            (void)vtss_ptp_set_active_ref(ref);
        }

        // Delete internal mode srcClkDomain.
        if (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_INTERNAL) {
            ptp_internal_mode_config_t intr_cfg = ptp_internal_cfg[instance];
            intr_cfg.srcClkDomain = VTSS_PTP_SRC_CLK_DOMAIN_NONE;
            intr_cfg.sync_rate = DEFAULT_INTERNAL_MODE_SYNC_RATE;
            ptp_internal_mode_config_set_priv(instance, intr_cfg);
        } else {
            // Virtal port cannot be configured for internal mode.
            (void)vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &virtual_port_cfg);
            virtual_port_cfg.enable = FALSE;
            virtual_port_cfg.virtual_port_mode = VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE;
            (void)ptp_clock_config_virtual_port_config_set(instance, &virtual_port_cfg);
        }
        ptp_global.ptpi[instance]->ssm.servo->deactivate(config_data.conf[instance].clock_init.cfg.clock_domain);  // Deactivate the servo associated with the clock before the clock is deleted
        // Unconfigure software clocks.
        vtssLocalClockReset(instance);

        if (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT ||
            config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT) {
            transparent_clock_exists = false;
        }

        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            PTP_RC(ptp_port_domain_check(config_data.conf[instance].port_config[pit.iport].portInternal, FALSE, pit.iport, instance));
        }

        //clear src ip address used earlier
        ptp_global.my_ip[instance].s_addr = 0;

        for (j = 0; j < MAX_UNICAST_MASTERS_PR_SLAVE; j++) {
            T_I("conf[%d].unicast_slave[%d].ip_addr= 0x%x", instance, j, config_data.conf[instance].unicast_slave[j].ip_addr);
            if (config_data.conf[instance].unicast_slave[j].ip_addr != 0) {
                config_data.conf[instance].unicast_slave[j].ip_addr = 0;
                vtss_ptp_uni_slave_conf_set(ptp_global.ptpi[instance], j, &config_data.conf[instance].unicast_slave[j]);
            }
        }
        ptp_conf_init(&config_data.conf[instance], instance);
        T_D("clock delete");

        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            PTP_RC(ptp_ts_operation_mode_set(pit.iport));
        }
        vtss_ptp_clock_create(ptp_global.ptpi[instance]);
        if (ptp_ace_update(instance) != VTSS_RC_OK) {
            T_I("ACE update error");
        }

        rc = VTSS_RC_OK;
    }
    return rc;
}

static bool ptp_port_domain_check(bool internal, bool enable, mesa_port_no_t port_no, uint instance)
{
    BOOL rc = TRUE;
    uint32_t clk_domain = config_data.conf[instance].clock_init.cfg.clock_domain;
    if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE && port_no < port_count_max()) {
        if (!internal) {
            if (enable) {
                // Software clock uses port domain 0
                if (config_data.conf[instance].port_config[port_no].enabled == FALSE) {
                    if (port_data[port_no].inst_count == 0) {
                        port_data[port_no].inst_count++;
                        port_data[port_no].port_domain = clk_domain < mesa_cap_ts_domain_cnt ? clk_domain : 0;
                    } else {
                        if (port_data[port_no].port_domain == clk_domain ||
                           (port_data[port_no].port_domain == 0 && clk_domain >= mesa_cap_ts_domain_cnt)) {
                            port_data[port_no].inst_count++;
                        } else {
                            rc = FALSE;
                        }
                    }
                }
            } else { //disable
                if (config_data.conf[instance].port_config[port_no].enabled == TRUE) {
                    if (port_data[port_no].inst_count != 0) {
                        port_data[port_no].inst_count--;
                        //if (port_data[port_no].inst_count == 0) {
                        //    port_data[port_no].port_domain = 0;
                        //}
                    } else {
                        T_EG(_I,"instance %d, port_no %d, domain %d, inst_count %d mesh", instance, iport2uport(port_no), port_data[port_no].port_domain, port_data[port_no].inst_count);
                    }
                }
            }
        }
    }
    T_IG(_I,"instance %d, port_no %d, domain %d, inst_count %d ", instance, iport2uport(port_no), port_data[port_no].port_domain, port_data[port_no].inst_count);
    return rc;
}

static mesa_rc ptp_port_ena(bool internal, uint portnum, uint instance)
{
    mesa_rc ok = PTP_ERROR_INV_PARAM;
    if (instance < PTP_CLOCK_INSTANCES) {
        if ((0 < portnum) && (portnum <= config_data.conf[instance].clock_init.numberPorts)) {
            if (ptp_port_domain_check(internal, TRUE, ptp_api_to_l2port(portnum), instance)) {
                if ((config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].portInternal != internal) || (config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].enabled != TRUE)) {
                    config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].portInternal = internal;
                    config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].enabled = TRUE;

                    if (PTP_READY()) {
                        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
                            if (0 < portnum && portnum <= config_data.conf[instance].clock_init.numberEtherPorts) {
                                vtss_1588_asymmetry_get(portnum, &config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].delayAsymmetry);
                                PTP_RC(ptp_ts_operation_mode_set(ptp_api_to_l2port(portnum)));
                            }
                            //if (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT ||
                            //        config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT ||
                            //    !internal) {
                            config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].portInternal = internal;
                            ok = (vtss_ptp_port_ena(ptp_global.ptpi[instance], portnum) == VTSS_RC_OK) ? VTSS_RC_OK : (mesa_rc) PTP_RC_INVALID_PORT_NUMBER;
                            config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].enabled = TRUE;
                            //}
                        }
                        T_I("port enabled, instance %d, port %d, ok %x", instance, portnum, ok);
                    } else {
                        ok = VTSS_RC_OK;
                    }
                } else {
                    ok = VTSS_RC_OK;
                }
            } else {
                ok = PTP_RC_CLOCK_DOMAIN_CONFLICT;
            }
        }
    }
    return ok;
}

static mesa_rc ptp_port_dis( uint portnum, uint instance)
{
    mesa_rc rc = VTSS_RC_ERROR;
    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            if (0 < portnum && portnum <= config_data.conf[instance].clock_init.numberPorts) {
                if (ptp_port_domain_check(config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].portInternal, FALSE, ptp_api_to_l2port(portnum), instance)) {
                    if ((config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].enabled != FALSE) || (config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].portInternal != FALSE)) {
                        rc = vtss_ptp_port_dis(ptp_global.ptpi[instance], portnum);
                        config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].enabled = FALSE;
                        config_data.conf[instance].port_config[ptp_api_to_l2port(portnum)].portInternal = FALSE;
                        if (0 < portnum && portnum <= config_data.conf[instance].clock_init.numberEtherPorts) {
                            PTP_RC(ptp_ts_operation_mode_set(ptp_api_to_l2port(portnum)));
                        }
                    }
                    else {
                        rc = VTSS_RC_OK;
                    }
                }
            }
        }
    }
    return rc;
}

mesa_rc vtss_appl_ptp_clock_config_default_ds_get(uint instance, vtss_appl_ptp_clock_config_default_ds_t *const default_ds_cfg)
{
    PTP_CORE_LOCK_SCOPE();

    if (instance < PTP_CLOCK_INSTANCES) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            *default_ds_cfg = config_data.conf[instance].clock_init.cfg;
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_clock_status_default_ds_get(uint instance, vtss_appl_ptp_clock_status_default_ds_t *const default_ds_status)
{
    PTP_CORE_LOCK_SCOPE();

    if (instance < PTP_CLOCK_INSTANCES) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            (void) vtss_ptp_get_clock_default_ds_status(ptp_global.ptpi[instance], default_ds_status);
            // now the numberPorts includes some virtual ports (for 1PPS synchronization), but we only make the ethernet ports visible for the user
            default_ds_status->numberPorts = config_data.conf[instance].clock_init.numberEtherPorts;
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

void ptp_apply_profile_defaults_to_default_ds(vtss_appl_ptp_clock_config_default_ds_t *default_ds_cfg, vtss_appl_ptp_profile_t profile)
{
    vtss_zarlink_servo_t servo_type = (vtss_zarlink_servo_t)vtss_appl_cap_zarlink_servo_type;
    default_ds_cfg->profile = profile;

    default_ds_cfg->path_trace_enable = FALSE;
    if (profile == VTSS_APPL_PTP_PROFILE_G_8265_1 || profile == VTSS_APPL_PTP_PROFILE_G_8275_2) {
        default_ds_cfg->protocol = VTSS_APPL_PTP_PROTOCOL_IP4UNI;
        default_ds_cfg->oneWay = false;  // Note: oneWay should actually be true but MS-PDV does not support uni-directional modes.
    } else {
        default_ds_cfg->protocol = VTSS_APPL_PTP_PROTOCOL_ETHERNET;
        default_ds_cfg->oneWay = false;
    }

    if (profile == VTSS_APPL_PTP_PROFILE_G_8265_1) {
        default_ds_cfg->domainNumber = 4;
        if (servo_type == VTSS_ZARLINK_SERVO_ZLS30380) {
            default_ds_cfg->filter_type = VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_FDD;
        } else if (servo_type == VTSS_ZARLINK_SERVO_ZLS30387) {
            default_ds_cfg->filter_type = VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_FREQ;
        }
    }
    else if (profile == VTSS_APPL_PTP_PROFILE_G_8275_1) {
        default_ds_cfg->domainNumber = 24;
        default_ds_cfg->localPriority = 128;
        if (servo_type == VTSS_ZARLINK_SERVO_ZLS30380) {
            default_ds_cfg->filter_type = VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE;
        }
    }
    else if (profile == VTSS_APPL_PTP_PROFILE_G_8275_2) {
        default_ds_cfg->domainNumber = 44; // default is 44. applicable range {44-63}
        default_ds_cfg->priority1 = 128;   // must not be changed.
        default_ds_cfg->priority2 = 128;
        if (servo_type == VTSS_ZARLINK_SERVO_ZLS30380) {
            default_ds_cfg->filter_type = VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_PHASE;
        }
    }
    else if (profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
        default_ds_cfg->domainNumber = 0;
        default_ds_cfg->localPriority = 128;
        default_ds_cfg->twoStepFlag = TRUE;
        default_ds_cfg->path_trace_enable = TRUE;
        default_ds_cfg->priority1 = 246; // IEEE 802_1as Table 8-2 : network infrastructure time aware system.
        default_ds_cfg->priority2 = 248; // AVNU 5.1, IEEE 802_1as 8.6.2.5.
        default_ds_cfg->filter_type = VTSS_APPL_PTP_FILTER_TYPE_BASIC;
    }
    else {
        default_ds_cfg->domainNumber = 0;
        default_ds_cfg->filter_type = fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_LAN966X ? VTSS_APPL_PTP_FILTER_TYPE_BASIC : VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE_LOW;
    }
}

void ptp_get_default_clock_default_ds(vtss_appl_ptp_clock_status_default_ds_t *default_ds_status,
                                      vtss_appl_ptp_clock_config_default_ds_t *default_ds_cfg)
{
    vtss_zarlink_servo_t servo_type = (vtss_zarlink_servo_t)vtss_appl_cap_zarlink_servo_type;
    default_ds_cfg->deviceType= VTSS_APPL_PTP_DEVICE_NONE;
    default_ds_cfg->twoStepFlag = false;
    default_ds_cfg->oneWay = false;
    default_ds_cfg->protocol = VTSS_APPL_PTP_PROTOCOL_ETHERNET;
    if (servo_type == VTSS_ZARLINK_SERVO_ZLS30380 || servo_type == VTSS_ZARLINK_SERVO_ZLS30387) {
        default_ds_cfg->filter_type = VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE_LOW;
    }

    {
        PTP_CORE_LOCK_SCOPE();

        memcpy(default_ds_status->clockIdentity, ptp_global.sysmac.addr, 3);
        default_ds_status->clockIdentity[3] = 0xff;
        default_ds_status->clockIdentity[4] = 0xfe;
        memcpy(default_ds_status->clockIdentity+5, ptp_global.sysmac.addr+3, 3);
    }

    default_ds_cfg->configured_vid = 1;
    /* in IPv4 encapsulation mode, the management VLAN id is used */
    default_ds_cfg->configured_pcp = 0;
    default_ds_cfg->mep_instance = 1;
    default_ds_cfg->clock_domain = 0;
    default_ds_cfg->dscp = 0;

    default_ds_status->numberPorts = port_count_max() + (mesa_cap_ts_io_cnt ? 1 : 0);
    /* dynamic */
    default_ds_status->clockQuality.clockAccuracy = 0;
    default_ds_status->clockQuality.clockClass = 0;
    default_ds_status->clockQuality.offsetScaledLogVariance = 0;

    /* configurable */
    default_ds_cfg->priority1 = 128;
    default_ds_cfg->priority2 = 128;
    default_ds_cfg->domainNumber = 0;
    default_ds_cfg->localPriority = 128;
    default_ds_cfg->path_trace_enable = FALSE;
}

void ptp_virtual_port_selection_enable(uint32_t inst, bool enable)
{
    ptp_global.ptpi[inst]->ssm.virtual_port_select = enable;
}

mesa_rc ptp_set_virtual_port_clock_class(uint instance, u8 ptp_class)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.clockQuality.clockClass = ptp_class;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_set_virtual_port_clock_accuracy(uint instance, u8 ptp_accuracy)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.clockQuality.clockAccuracy = ptp_accuracy;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_set_virtual_port_clock_variance(uint instance, u16 ptp_variance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.clockQuality.offsetScaledLogVariance = ptp_variance;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_set_virtual_port_local_priority(uint instance, u8 local_priority)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.localPriority = local_priority;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_set_virtual_port_priority1(uint instance, u8 priority1)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.priority1 = priority1;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_set_virtual_port_priority2(uint instance, u8 priority2)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.priority2 = priority2;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_set_virtual_port_clock_identity(uint instance, char *ci, bool enable)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            if(enable){
                int i=0,id[8]={0}, len =0;
                if (sscanf(ci,"%x:%x:%x:%x:%x:%x:%x:%x",&id[0],&id[1],&id[2],&id[3],&id[4],&id[5],&id[6],&id[7]) != 8) {
                    return rc;
                }
                len = sizeof(vtss_appl_clock_identity) + 7; //7 for colons
                for(i=0 ; i<len ; i++){ //validate hex string
                    if (( ('A' <= ci[i] && ci[i] <= 'F') || ('a' <= ci[i] && ci[i] <= 'f') || ('0' <=  ci[i] && ci[i] <= '9') )&&
                            ( ('A' <= ci[i+1] && ci[i+1] <= 'F') || ('a' <= ci[i+1] && ci[i+1] <= 'f') || ('0' <=  ci[i+1] && ci[i+1] <= '9') ) ){
                        i +=2;
                        continue;
                    } else {
                        T_W("Invalid clock identity value entered\n");
                        return rc;
                    }
                }
                for(i=0 ; i<sizeof(vtss_appl_clock_identity); i++)
                    c.clock_identity[i] = (uint8_t ) id[i];
            }else { // in case no command, set default values
                memcpy(c.clock_identity,  config_data.conf[0].clock_init.clockIdentity, sizeof(vtss_appl_clock_identity));
                c.clock_identity[7] += PTP_CLOCK_INSTANCES;
            }
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_set_virtual_port_steps_removed(uint instance, uint16_t steps_removed, bool enable)
{

    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;

    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.steps_removed = enable ? steps_removed : 0 ;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}
mesa_rc ptp_set_virtual_port_io_pin(uint instance, u16 io_pin, BOOL enable)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.enable = enable;
            // io-pin is only valid when it is enabled, in disable, use the already enabled io_pin number
            if (enable) {
                c.input_pin = io_pin;
                //rc = vtss_ptp_port_ena_virtual(ptp_global.ptpi[instance], l2port_to_ptp_api(MESA_CAP(MESA_CAP_PORT_CNT)));
            }
            T_I("inst %d, io_pin %d enable %d", instance, io_pin, enable);
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
            if (MESA_CAP(MESA_CAP_TS_HAS_PTP_IO_PIN)) {
                rc = io_pin_instance_set(instance, c.input_pin, enable);
#if 0 // outdated
                T_I("set hook: source_id %d", ptp_io_pin[io_pin].source_id);
                if (VTSS_RC_OK != vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PTP, io_pin_interrupt_handler, ptp_io_pin[io_pin].source_id, INTERRUPT_PRIORITY_NORMAL)) {
                    T_I("source already hooked");
                }
#endif // outdated
            }
        }
    }
    return rc;
}

mesa_rc ptp_set_virtual_port_time_property(uint instance, const vtss_appl_ptp_clock_timeproperties_ds_t *prop)
{
    mesa_rc rc = VTSS_RC_ERROR;

    PTP_CORE_LOCK_SCOPE();

    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.timeproperties = *prop;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_get_virtual_port_time_property(uint instance, vtss_appl_ptp_clock_timeproperties_ds_t *const prop)
{
    vtss_appl_ptp_virtual_port_config_t c;
    mesa_rc rc = VTSS_RC_ERROR;

    PTP_CORE_LOCK_SCOPE();
    if (instance < PTP_CLOCK_INSTANCES) {
        if ((rc = vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c)) == VTSS_RC_OK) {
            *prop = c.timeproperties;
        }
    }
    return rc;
}

mesa_rc ptp_clear_virtual_port_clock_class(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_clear_virtual_port_clock_accuracy(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.clockQuality.clockAccuracy = 0xFE;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_clear_virtual_port_clock_variance(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.clockQuality.offsetScaledLogVariance = 0xffff;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_clear_virtual_port_local_priority(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.localPriority = 128;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_clear_virtual_port_priority1(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.priority1 = 128;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_clear_virtual_port_priority2(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.priority2 = 128;
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_virtual_port_alarm_set(uint instance, mesa_bool_t enable)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;
        if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &c) == VTSS_RC_OK) {
            c.alarm = enable;
            if (c.enable) {
                if (c.proto != VTSS_PTP_APPL_RS422_PROTOCOL_PIM && c.proto != VTSS_PTP_APPL_RS422_PROTOCOL_NONE) {
                    ptp_rs422_alarm_send(c.alarm);
                } else if (c.proto == VTSS_PTP_APPL_RS422_PROTOCOL_PIM) {
                    ptp_pim_alarm_msg_send(config_data.conf[instance].virtual_port_cfg.portnum, c.alarm);
                }
            }
            T_IG(VTSS_TRACE_GRP_1_PPS, "alarm %d", c.alarm);
            rc = ptp_clock_config_virtual_port_config_set(instance, &c);
        }
    }
    return rc;
}

mesa_rc ptp_set_clock_class(uint instance, u8 ptp_class)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        ptp_global.ptpi[instance]->announced_clock_quality.clockClass = ptp_class;
        ptp_global.ptpi[instance]->defaultDS.status.clockQuality.clockClass = ptp_class;
        defaultDSChanged(ptp_global.ptpi[instance]);
        rc = VTSS_RC_OK;
    }
    return rc;
}

mesa_rc ptp_set_clock_accuracy(uint instance, u8 ptp_accuracy)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        ptp_global.ptpi[instance]->announced_clock_quality.clockAccuracy = ptp_accuracy;
        ptp_global.ptpi[instance]->defaultDS.status.clockQuality.clockAccuracy = ptp_accuracy;
        defaultDSChanged(ptp_global.ptpi[instance]);
        rc = VTSS_RC_OK;
    }
    return rc;
}

mesa_rc ptp_set_clock_variance(uint instance, u16 ptp_variance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        ptp_global.ptpi[instance]->announced_clock_quality.offsetScaledLogVariance = ptp_variance;
        ptp_global.ptpi[instance]->defaultDS.status.clockQuality.offsetScaledLogVariance = ptp_variance;
        defaultDSChanged(ptp_global.ptpi[instance]);
        rc = VTSS_RC_OK;
    }
    return rc;
}

mesa_rc ptp_clear_clock_class(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        ptp_global.ptpi[instance]->announced_clock_quality.clockClass = DEFAULT_CLOCK_CLASS;
        ptp_global.ptpi[instance]->defaultDS.status.clockQuality.clockClass = DEFAULT_CLOCK_CLASS;
        defaultDSChanged(ptp_global.ptpi[instance]);
        rc = VTSS_RC_OK;
    }
    return rc;
}

mesa_rc ptp_clear_clock_accuracy(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        ptp_global.ptpi[instance]->announced_clock_quality.clockAccuracy = 0xFE;
        ptp_global.ptpi[instance]->defaultDS.status.clockQuality.clockAccuracy = 0xFE;
        defaultDSChanged(ptp_global.ptpi[instance]);
        rc = VTSS_RC_OK;
    }
    return rc;
}

mesa_rc ptp_clear_clock_variance(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (instance < PTP_CLOCK_INSTANCES) {
        ptp_global.ptpi[instance]->announced_clock_quality.offsetScaledLogVariance = 0xffff;
        ptp_global.ptpi[instance]->defaultDS.status.clockQuality.offsetScaledLogVariance = 0xffff;
        defaultDSChanged(ptp_global.ptpi[instance]);
        rc = VTSS_RC_OK;
    }
    return rc;
}

mesa_rc ptp_clock_clockidentity_set(uint instance, vtss_appl_clock_identity *clockIdentity)
{
    PTP_CORE_LOCK_SCOPE();
    memcpy(&config_data.conf[instance].clock_init.clockIdentity, clockIdentity, sizeof(*clockIdentity));
    return VTSS_RC_OK;
}

// Compare default_ds_cfg with existing instance.
//
mesa_rc ptp_1as_profile_allow_config(const vtss_appl_ptp_clock_config_default_ds_t *default_ds_cfg, uint32_t inst)
{
    if ((default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS ||
         default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) &&
       ((!default_ds_cfg->clock_domain && config_data.conf[inst].clock_init.cfg.clock_domain >= mesa_cap_ts_domain_cnt) ||
        (!config_data.conf[inst].clock_init.cfg.clock_domain && default_ds_cfg->clock_domain >= mesa_cap_ts_domain_cnt))) {
        T_W("Software clock domains and clock domain 0 cannot be configured together.");
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

#define HAS_SERVO_380(expr) ((servo_type == VTSS_ZARLINK_SERVO_ZLS30380 && (expr)) || false)
#define HAS_ANY_SERVO(expr) (((servo_type == VTSS_ZARLINK_SERVO_ZLS30380 || servo_type == VTSS_ZARLINK_SERVO_ZLS30387) && (expr)) || false)

mesa_rc vtss_appl_ptp_clock_config_default_ds_set(uint instance, const vtss_appl_ptp_clock_config_default_ds_t *default_ds_cfg)
{
    vtss_zarlink_servo_t servo_type = (vtss_zarlink_servo_t)vtss_appl_cap_zarlink_servo_type;
    bool basic_servo_used = false;
    /*lint -save -e685 -e568 */
    if (((default_ds_cfg->deviceType >= VTSS_APPL_PTP_DEVICE_NONE) && (default_ds_cfg->deviceType < VTSS_APPL_PTP_DEVICE_MAX_TYPE)) &&
        ((default_ds_cfg->twoStepFlag == 0) || (default_ds_cfg->twoStepFlag == 1)) &&
        ((default_ds_cfg->oneWay == 0) || (default_ds_cfg->oneWay == 1)) &&
        (default_ds_cfg->domainNumber <= 127) &&
#if defined(VTSS_SW_OPTION_MEP)
        ((mesa_cap_oam_used_as_ptp_protocol && (default_ds_cfg->protocol >= VTSS_APPL_PTP_PROTOCOL_ETHERNET) && (default_ds_cfg->protocol < VTSS_APPL_PTP_PROTOCOL_MAX_TYPE)) ||
         (!mesa_cap_oam_used_as_ptp_protocol && (default_ds_cfg->protocol >= VTSS_APPL_PTP_PROTOCOL_ETHERNET) && (default_ds_cfg->protocol < VTSS_APPL_PTP_PROTOCOL_MAX_TYPE) && (default_ds_cfg->protocol != VTSS_APPL_PTP_PROTOCOL_OAM))) &&
#else
        ((default_ds_cfg->protocol >= VTSS_APPL_PTP_PROTOCOL_ETHERNET) && (default_ds_cfg->protocol < VTSS_APPL_PTP_PROTOCOL_MAX_TYPE) && (default_ds_cfg->protocol != VTSS_APPL_PTP_PROTOCOL_OAM)) &&
#endif
        ((default_ds_cfg->configured_vid >= 1) && (default_ds_cfg->configured_vid <= 4095)) &&
        (default_ds_cfg->configured_pcp <= 7) && (default_ds_cfg->dscp <= 0x3f) &&
        ((default_ds_cfg->mep_instance >= 1) && (default_ds_cfg->mep_instance <= 100)) &&
        (default_ds_cfg->clock_domain <= VTSS_PTP_SW_CLK_DOMAIN_CNT + mesa_cap_ts_domain_cnt) &&
        (((default_ds_cfg->profile >= VTSS_APPL_PTP_PROFILE_NO_PROFILE) && (default_ds_cfg->profile <= VTSS_APPL_PTP_PROFILE_G_8265_1)) ||
         (default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8275_1) ||
         (default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8275_2 && default_ds_cfg->protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) ||
#if defined (VTSS_SW_OPTION_P802_1_AS)
         (default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) || (default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) ||
#endif
         false) &&
        (
         HAS_ANY_SERVO(default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE || \
            default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE_LOW) ||
         HAS_SERVO_380(default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_DEFAULT || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_XO || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_XO || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_TCXO || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_TCXO || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_OCXO_S3E || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_FREQ || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_PHASE) ||
         HAS_ANY_SERVO(default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_FREQ) ||
         HAS_SERVO_380(default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_FDD || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_XDSL || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_FREQ || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_PHASE || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C60W || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C150 || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C180 || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C240 || \
             default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E_R4_6_1) ||
#ifdef SW_OPTION_BASIC_PTP_SERVO
         (default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_BASIC) ||
#endif
         false
        ))
    {
        /*lint -restore */
        mesa_rc rc = VTSS_RC_ERROR;
        if (fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_LAN966X &&
           (default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8275_1 || default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8275_2 ||
            default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8265_1)) {
            T_W("On Lan9668 platform, telecom profiles are not supported.");
            return rc;
        }
        if ((default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8275_2) && (default_ds_cfg->deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT || default_ds_cfg->deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT)) {
            T_W("802.1AS, 8275.2 profiles do not support Transparent clock");
            return rc;
        }
        if (default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS && (default_ds_cfg->deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY || default_ds_cfg->deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY)) {
            T_W("802.1AS profile does not support Master/Slave only");
            return rc;
        }
        if (default_ds_cfg->deviceType == VTSS_APPL_PTP_DEVICE_AED_GM && default_ds_cfg->profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            T_W("Device type AED-GM is only compatible with AED 802.1AS profile");
            return rc;
        }
        if (default_ds_cfg->deviceType == VTSS_APPL_PTP_DEVICE_INTERNAL && (default_ds_cfg->clock_domain == 0 ||
            default_ds_cfg->clock_domain >= mesa_cap_ts_domain_cnt)) {
            T_W("Chip clock domains 1,2 are configurable in internal mode.");
            return rc;
        }
#if defined(__MIPSEL__)
        if (default_ds_cfg->twoStepFlag) {
            port_iter_t pit;
            port_iter_init_local(&pit);
            while (port_iter_getnext(&pit)) {
                if ((config_data.conf[instance].port_config[pit.iport].logSyncInterval < -3 || config_data.conf[instance].port_config[pit.iport].logMinPdelayReqInterval < 0)
                    && config_data.conf[instance].port_config[pit.iport].enabled){
                    T_W("This device does only support twoStep with a maximum sync interval of 125ms and delay request interval of 1 sec as per 802.1AS standard");
                    return rc;
                }
            }
        }
#endif
        if (instance < PTP_CLOCK_INSTANCES) {
            PTP_CORE_LOCK_SCOPE();
            if (PTP_READY()) {
                if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
                    rc = VTSS_RC_OK;
                    if ((config_data.conf[instance].clock_init.cfg.deviceType == default_ds_cfg->deviceType) &&          // Check that deviceType is not modified.
                        (config_data.conf[instance].clock_init.cfg.mep_instance == default_ds_cfg->mep_instance) &&      // Check that mep_instance is not modified (req. may be relaxed later).
                        (config_data.conf[instance].clock_init.cfg.clock_domain == default_ds_cfg->clock_domain))        // Check that clock_id is not modified (req. may be relaxed later).
                    {
                        if ((config_data.conf[instance].clock_init.cfg.domainNumber != default_ds_cfg->domainNumber) ||      // If the domainNumber is changed the clock needs to be recreated
                            (config_data.conf[instance].clock_init.cfg.twoStepFlag != default_ds_cfg->twoStepFlag) ||        // If the twoStepFlag flag is changed the clock needs to be recreated
                            (config_data.conf[instance].clock_init.cfg.oneWay != default_ds_cfg->oneWay) ||                  // If the oneWay flag is changed the clock needs to be recreated
                            (config_data.conf[instance].clock_init.cfg.protocol != default_ds_cfg->protocol) ||              // If the protocol is changed the clock needs to be recreated
                            (config_data.conf[instance].clock_init.cfg.configured_vid != default_ds_cfg->configured_vid) ||  // If the configured VID is changed the clock needs to be recreated
                            (config_data.conf[instance].clock_init.cfg.configured_pcp != default_ds_cfg->configured_pcp) ||  // If the configured PCP is changed the clock needs to be recreated
                            (config_data.conf[instance].clock_init.cfg.dscp != default_ds_cfg->dscp) ||                      // If the dscp is changed the clock needs to be recreated
                            (config_data.conf[instance].clock_init.cfg.mep_instance != default_ds_cfg->mep_instance))        // If the mep instance is changed the clock needs to be recreated
                        {
                            config_data.conf[instance].clock_init.cfg = *default_ds_cfg;
                            //update_ptp_ip_address(instance, default_ds_cfg->configured_vid);
                            //vtss_ptp_clock_create(ptp_global.ptpi[instance]);
                            ptp_global.ptpi[instance]->ssm.servo->deactivate(config_data.conf[instance].clock_init.cfg.clock_domain);  // ptp_clock_create is going to create this PTP clock again. Before doing this, de-activate the filter/servo instance.
                            current_servo_mode_ref[instance].mode = VTSS_PTP_SERVO_NONE;
                            // Re-create clock instance. If it is a transparent clock, we need to simulate that the clock doesn't exist
                            if (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT || config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT) {
                                transparent_clock_exists = false;
                            }
                            rc = ptp_clock_create(&config_data.conf[instance].clock_init, instance, true);
                            if (rc != VTSS_RC_OK) {
                                config_data.conf[instance].clock_init.cfg.deviceType = VTSS_APPL_PTP_DEVICE_NONE;
                            }
                        }
                        else {
                            // If other clocks exist in the same hardware clock domain, the value filter_type must not be changed.
                            for (int i = 0; i < PTP_CLOCK_INSTANCES; i++) {
                                if ((i != instance) &&
                                    (config_data.conf[i].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) &&
                                    (config_data.conf[i].clock_init.cfg.clock_domain == config_data.conf[instance].clock_init.cfg.clock_domain) &&
                                    (default_ds_cfg->clock_domain < mesa_cap_ts_domain_cnt) &&
                                    (default_ds_cfg->filter_type != config_data.conf[i].clock_init.cfg.filter_type))
                                {
                                    T_W("Another clock exists in the same hardware clock domain. Filter type cannot be changed.");
                                    return VTSS_RC_ERROR;
                                }
                            }
                            if (default_ds_cfg->filter_type != config_data.conf[instance].clock_init.cfg.filter_type) {
                                ptp_global.ptpi[instance]->ssm.servo->deactivate(config_data.conf[instance].clock_init.cfg.clock_domain);
                                current_servo_mode_ref[instance].mode = VTSS_PTP_SERVO_NONE;
#ifdef SW_OPTION_BASIC_PTP_SERVO
                                if (default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_BASIC) {
                                    (void) vtss_ptp_clock_servo_set(ptp_global.ptpi[instance], basic_servo[instance]);
                                    basic_servo_used = true;
                                } else
#endif // SW_OPTION_BASIC_PTP_SERVO
#if defined(VTSS_SW_OPTION_ZLS30387)
                                if (default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_DEFAULT ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_XO ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_XO ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_TCXO ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_TCXO ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_OCXO_S3E ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_FREQ ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_PHASE ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_FREQ ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_FDD ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_XDSL ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_FREQ ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_PHASE ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C60W ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C150 ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C180 ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C240 ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E_R4_6_1 ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE ||
                                    default_ds_cfg->filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE_LOW )
                                {
                                    (void) vtss_ptp_clock_servo_set(ptp_global.ptpi[instance], advanced_servo[instance]);
                                } else
#endif
                                {
                                    return VTSS_RC_ERROR;
                                }
                                config_data.conf[instance].clock_init.cfg.filter_type = default_ds_cfg->filter_type;
                                // Re-configure clock adjustment as basic servo uses internal timer.
                                if (VTSS_RC_OK != vtss_local_clock_adj_method((uint32_t)instance, config_data.init_ext_clock_mode.adj_method, config_data.conf[instance].clock_init.cfg.profile,
                                                                              config_data.conf[instance].clock_init.cfg.clock_domain, basic_servo_used)) {
                                    T_W("vtss_local_clock_adj_method returned error");
                                }
                                ptp_global.ptpi[instance]->ssm.servo->activate(config_data.conf[instance].clock_init.cfg.clock_domain,
                                                                            default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8275_1 ||
                                                                            default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8275_2);
                                int ref;
                                if ((ref = find_active_reference(instance, true)) != -1) {
                                    (void)vtss_ptp_set_active_ref(ref);
                                }
                            }
                            config_data.conf[instance].clock_init.cfg = *default_ds_cfg;
                            rc = vtss_ptp_set_clock_default_ds_cfg(ptp_global.ptpi[instance], default_ds_cfg);
                        }
                        if (rc == VTSS_RC_OK) {
                            if (VTSS_RC_OK != ptp_ace_update(instance)) {
                                T_IG(_I,"ptp_ace_update failed");
                            }
                        }
                    }
                }
                else {
                    /* Dpll is in clock domain 0. So, dpll dependent profiles must be configured in clock domain 0. */
                    if ((default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8265_1 || default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8275_1) &&
                        (default_ds_cfg->clock_domain != 0)) {
                        T_W(" 8275 and 8265 profiles dependent on dpll can be configured only in clock domain 0");
                        return VTSS_RC_ERROR;
                    }
                    if (default_ds_cfg->deviceType != VTSS_APPL_PTP_DEVICE_NONE) {  // Do not allow a clock of deviceType == VTSS_APPL_PTP_DEVICE_NONE to be created.
                        // forbid more than one BC/ Slave using the same HW domain
                        // 8275 allowed for accuracy testing only but not useful in real network.
                        // TODO: validate multiple 8265 instance purpose.
                        if ((default_ds_cfg->deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) || (default_ds_cfg->deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY)) {
                            for (int i = 0; i < PTP_CLOCK_INSTANCES; i++) {
                                if ((i != instance) &&
                                   ((config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) ||
                                    (config_data.conf[i].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY))) {
                                    if (default_ds_cfg->clock_domain == config_data.conf[i].clock_init.cfg.clock_domain) {
                                        if (((default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8265_1) &&
                                            (config_data.conf[i].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_G_8265_1)) ||
                                            ((default_ds_cfg->profile == VTSS_APPL_PTP_PROFILE_G_8275_1) &&
                                            (config_data.conf[i].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_1))) {
                                            continue;
                                        }
                                        T_W("more than one slave/boundary clock cannot be configured in same clock domain [%d] ", default_ds_cfg->clock_domain);
                                        return VTSS_RC_ERROR;
                                    } else if (ptp_1as_profile_allow_config(default_ds_cfg, i) != VTSS_RC_OK) {
                                        return VTSS_RC_ERROR;
                                    }
                                }
                            }
                        }
                        config_data.conf[instance].clock_init.cfg = *default_ds_cfg;
                        //if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE && config_data.conf[instance].clock_init.cfg.clock_domain == 0) {
                        //    rc = vtss_local_clock_adj_method(config_data.init_ext_clock_mode.adj_method, config_data.conf[instance].clock_init.cfg.profile);
                        //}
                        //ptp_global.ptpi[instance]->ssm.servo->activate(config_data.conf[instance].clock_init.cfg.clock_domain);
                        T_IG(_I,"activate new instance: filter type = %d, clock_domain = %d", default_ds_cfg->filter_type, config_data.conf[instance].clock_init.cfg.clock_domain);
                        rc = ptp_clock_create(&config_data.conf[instance].clock_init, instance, false);
                        if (rc != VTSS_RC_OK) {
                            config_data.conf[instance].clock_init.cfg.deviceType = VTSS_APPL_PTP_DEVICE_NONE;
                        }
                    }
                }
            } else {
                config_data.conf[instance].clock_init.cfg = *default_ds_cfg;
                rc = VTSS_RC_OK;
            }
        }
        return rc;
    }
    else {
        T_W("Invalid parameter");
        return VTSS_RC_ERROR;  // One or more paramters were invalid.
    }
}

mesa_rc vtss_appl_ptp_clock_slave_config_get(uint instance, vtss_appl_ptp_clock_slave_config_t *const cfg)
{
    PTP_CORE_LOCK_SCOPE();

    if (instance < PTP_CLOCK_INSTANCES) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            *cfg = config_data.conf[instance].slave_cfg;
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_clock_slave_config_set(uint instance, const vtss_appl_ptp_clock_slave_config_t *const cfg)
{
    PTP_CORE_LOCK_SCOPE();

    /*lint -save -e685 -e568 */
    if ((cfg->stable_offset <= 1000000) && (cfg->offset_ok <= 1000000) && (cfg->offset_fail <= 1000000))
    {
        /*lint -restore */
        if (instance < PTP_CLOCK_INSTANCES) {
            if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
                config_data.conf[instance].slave_cfg = *cfg;
                vtss_ptp_clock_slave_config_set(ptp_global.ptpi[instance]->ssm.servo, cfg);
                return VTSS_RC_OK;
            }
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc ptp_clock_send_unicast_cancel(uint instance, uint slave_index, u8 msg_type)
{
    PTP_CORE_LOCK_SCOPE();
    T_DG(_I,"cancel unicast: instance = %d, index = %d, msg_type %d", instance, slave_index, msg_type);
    if (instance < PTP_CLOCK_INSTANCES && slave_index < MAX_UNICAST_SLAVES_PR_MASTER) {
        debugIssueCancelUnicast(ptp_global.ptpi[instance], slave_index, msg_type);
        return MESA_RC_OK;
    } else {
        return MESA_RC_ERROR;
    }
}

void vtss_appl_ptp_clock_slave_default_config_get(vtss_appl_ptp_clock_slave_config_t *cfg)
{
    cfg->offset_fail = 3000;
    cfg->offset_ok = 1000;
    cfg->stable_offset = 1000;
}

mesa_rc vtss_appl_ptp_clock_status_current_ds_get(uint instance, vtss_appl_ptp_clock_current_ds_t *const status)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            vtss_ptp_get_clock_current_ds(ptp_global.ptpi[instance], status);
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_clock_status_parent_ds_get(uint instance, vtss_appl_ptp_clock_parent_ds_t *const status)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            vtss_ptp_get_clock_parent_ds(ptp_global.ptpi[instance], status);
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_clock_status_timeproperties_ds_get(uint instance, vtss_appl_ptp_clock_timeproperties_ds_t *const timeproperties_ds)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            vtss_ptp_get_clock_timeproperties_ds(ptp_global.ptpi[instance], timeproperties_ds);
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_clock_config_timeproperties_ds_get(uint instance, vtss_appl_ptp_clock_timeproperties_ds_t *const timeproperties_ds)
{
    PTP_CORE_LOCK_SCOPE();

    if (instance < PTP_CLOCK_INSTANCES) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            *timeproperties_ds = config_data.conf[instance].time_prop;
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

static void clock_default_timeproperties_ds_get(vtss_appl_ptp_clock_timeproperties_ds_t *timeproperties_ds)
{
    timeproperties_ds->currentUtcOffset = DEFAULT_UTC_OFFSET;
    timeproperties_ds->currentUtcOffsetValid = false;
    timeproperties_ds->leap59 = false;
    timeproperties_ds->leap61 = false;
    timeproperties_ds->timeTraceable = false;
    timeproperties_ds->frequencyTraceable = false;
    timeproperties_ds->ptpTimescale = true; /* default in ITU profile */
    timeproperties_ds->timeSource = 0xa0; /* indicates internal oscillator */
    timeproperties_ds->pendingLeap = false;
    timeproperties_ds->leapDate = 0;
    timeproperties_ds->leapType = VTSS_APPL_PTP_LEAP_SECOND_61;
}

void ptp_clock_default_timeproperties_ds_get(vtss_appl_ptp_clock_timeproperties_ds_t *timeproperties_ds)
{
    PTP_CORE_LOCK_SCOPE();
    clock_default_timeproperties_ds_get(timeproperties_ds);
    return;
}

mesa_rc vtss_appl_ptp_clock_config_timeproperties_ds_set(uint instance, const vtss_appl_ptp_clock_timeproperties_ds_t *const timeproperties_ds)
{
    PTP_CORE_LOCK_SCOPE();

    if (((timeproperties_ds->currentUtcOffsetValid == 0) || (timeproperties_ds->currentUtcOffsetValid == 1)) &&
        ((timeproperties_ds->leap59 == 0) || (timeproperties_ds->leap59 == 1)) &&
        ((timeproperties_ds->leap61 == 0) || (timeproperties_ds->leap61 == 1)) &&
        ((timeproperties_ds->timeTraceable == 0) || (timeproperties_ds->timeTraceable == 1)) &&
        ((timeproperties_ds->frequencyTraceable == 0) || (timeproperties_ds->frequencyTraceable == 1)) &&
        ((timeproperties_ds->ptpTimescale == 0) || (timeproperties_ds->ptpTimescale == 1)) &&
        ((timeproperties_ds->pendingLeap == 0) || (timeproperties_ds->pendingLeap == 1)) &&
        ((timeproperties_ds->leapType == VTSS_APPL_PTP_LEAP_SECOND_59) || (timeproperties_ds->leapType == VTSS_APPL_PTP_LEAP_SECOND_61)))
    {
        if (instance < PTP_CLOCK_INSTANCES) {
            config_data.conf[instance].time_prop = *timeproperties_ds;
            if (PTP_READY()) {
                if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
                    vtss_ptp_set_clock_timeproperties_ds(ptp_global.ptpi[instance], timeproperties_ds);
                    return VTSS_RC_OK;
                }
            } else return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_status_clocks_port_ds_get(uint instance, vtss_ifindex_t portnum, vtss_appl_ptp_status_port_ds_t *const port_ds)
{
    PTP_CORE_LOCK_SCOPE();

    u32 v;
    VTSS_RC(ptp_ifindex_to_port(portnum, &v));
    if ((instance < PTP_CLOCK_INSTANCES) && (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE)) {
        if (vtss_ptp_get_port_status(ptp_global.ptpi[instance], iport2uport(v), port_ds) == VTSS_RC_OK) {
            /* special handling for port state in a OAM Master */
            if (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY &&
                config_data.conf[instance].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM)
            {
                if (port_data[v].link_state) {
                    port_ds->portState = VTSS_APPL_PTP_MASTER;
                } else {
                    port_ds->portState = VTSS_APPL_PTP_DISABLED;
                }
            }
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_status_clocks_port_statistics_get(uint instance, vtss_ifindex_t portnum, vtss_appl_ptp_status_port_statistics_t *const port_statistics)
{
    PTP_CORE_LOCK_SCOPE();
    u32 v;
    VTSS_RC(ptp_ifindex_to_port(portnum, &v));
    if ((instance < PTP_CLOCK_INSTANCES) && (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE)) {
        if (vtss_ptp_get_port_statistics(ptp_global.ptpi[instance], iport2uport(v), port_statistics, FALSE) == VTSS_RC_OK) {
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_status_clocks_port_statistics_get_clear(uint instance, vtss_ifindex_t portnum, vtss_appl_ptp_status_port_statistics_t *const port_statistics)
{
    PTP_CORE_LOCK_SCOPE();
    u32 v;
    VTSS_RC(ptp_ifindex_to_port(portnum, &v));
    if ((instance < PTP_CLOCK_INSTANCES) && (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE)) {
        if (vtss_ptp_get_port_statistics(ptp_global.ptpi[instance], iport2uport(v), port_statistics, TRUE) == VTSS_RC_OK) {
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_status_clocks_virtual_port_ds_get(uint instance, u32 portnum, vtss_appl_ptp_status_port_ds_t *const port_ds)
{
    PTP_CORE_LOCK_SCOPE();

    if ((instance < PTP_CLOCK_INSTANCES) && (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE)) {
        if (vtss_ptp_get_port_status(ptp_global.ptpi[instance], iport2uport(portnum), port_ds) == VTSS_RC_OK) {
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

void vtss_appl_ptp_clock_config_default_virtual_port_config_get(vtss_appl_ptp_virtual_port_config_t *const c)
{
    vtss_appl_ptp_clock_timeproperties_ds_t *prop = &c->timeproperties;

    memcpy(c->clock_identity,  config_data.conf[0].clock_init.clockIdentity, sizeof(vtss_appl_clock_identity));
    T_IG(VTSS_TRACE_GRP_PTP_BASE_BMCA, "Clock Identity = %s \n",config_data.conf[0].clock_init.clockIdentity);
	c->clock_identity[7] += PTP_CLOCK_INSTANCES;
    c->clockQuality.clockClass = G8275PRTC_GM_CLOCK_CLASS;
    c->clockQuality.clockAccuracy = G8275PRTC_GM_ACCURACY;
    c->clockQuality.offsetScaledLogVariance = 0xFFFF;
    c->localPriority = 128;
    c->priority1 = 128;
    c->priority2 = 128;
	c->steps_removed = 0;
    c->enable = FALSE;
    c->input_pin = PTP_IO_PIN_UNUSED;
    c->output_pin = PTP_IO_PIN_UNUSED;
    c->virtual_port_mode = VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE;
    c->proto = VTSS_PTP_APPL_RS422_PROTOCOL_NONE;
    c->alarm = FALSE;
    // timeproperties
    prop->currentUtcOffset = DEFAULT_UTC_OFFSET;
    prop->currentUtcOffsetValid = false;
    prop->leap59 = false;
    prop->leap61 = false;
    prop->timeTraceable = true;
    prop->frequencyTraceable = true;
    prop->ptpTimescale = true; /* default in ITU profile */
    prop->timeSource = 0x20; /* indicates GNSS */
    prop->pendingLeap = false;
    prop->leapDate = 0;
    prop->leapType = VTSS_APPL_PTP_LEAP_SECOND_61;
}

mesa_rc vtss_appl_ptp_clock_config_virtual_port_config_get(uint instance, vtss_appl_ptp_virtual_port_config_t *const c)
{
    if (instance < PTP_CLOCK_INSTANCES) {
        *c = config_data.conf[instance].virtual_port_cfg;
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc ptp_clock_config_virtual_port_config_get(uint instance, vtss_appl_ptp_virtual_port_config_t *const c)
{
    PTP_CORE_LOCK_SCOPE();
    return vtss_appl_ptp_clock_config_virtual_port_config_get(instance, c);
}

mesa_rc vtss_appl_ptp_clock_config_virtual_port_config_set(uint instance, const vtss_appl_ptp_virtual_port_config_t *const c)
{
    PTP_CORE_LOCK_SCOPE();
    return ptp_clock_config_virtual_port_config_set(instance, c);

}

static bool has_mgmt_conf_changed(vtss_appl_ptp_config_port_ds_t prev_port_ds, const  vtss_appl_ptp_config_port_ds_t *port_ds)
{
    return ((prev_port_ds.c_802_1as.useMgtSettableLogSyncInterval != port_ds->c_802_1as.useMgtSettableLogSyncInterval) ||
            (prev_port_ds.c_802_1as.useMgtSettableLogAnnounceInterval != port_ds->c_802_1as.useMgtSettableLogAnnounceInterval) ||
            (prev_port_ds.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval != port_ds->c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval) ||
            (prev_port_ds.c_802_1as.mgtSettableLogSyncInterval != port_ds->c_802_1as.mgtSettableLogSyncInterval) ||
            (prev_port_ds.c_802_1as.mgtSettableLogAnnounceInterval != port_ds->c_802_1as.mgtSettableLogAnnounceInterval) ||
            (prev_port_ds.c_802_1as.peer_d.mgtSettableLogPdelayReqInterval != port_ds->c_802_1as.peer_d.mgtSettableLogPdelayReqInterval)||
            (prev_port_ds.c_802_1as.mgtSettableLogGptpCapableMessageInterval != port_ds->c_802_1as.mgtSettableLogGptpCapableMessageInterval) ||
            (prev_port_ds.c_802_1as.useMgtSettableLogGptpCapableMessageInterval != port_ds->c_802_1as.useMgtSettableLogGptpCapableMessageInterval));

}

static bool ptp_state_init_required(vtss_appl_ptp_config_port_ds_t prev_port_ds, const  vtss_appl_ptp_config_port_ds_t *port_ds)
{
     return ((prev_port_ds.enabled != port_ds->enabled) ||
            (prev_port_ds.logAnnounceInterval != port_ds->logAnnounceInterval) ||
            (prev_port_ds.announceReceiptTimeout != port_ds->announceReceiptTimeout) ||
            (prev_port_ds.logSyncInterval != port_ds->logSyncInterval) ||
            (prev_port_ds.delayMechanism != port_ds->delayMechanism) ||
            (prev_port_ds.logMinPdelayReqInterval != port_ds->logMinPdelayReqInterval)||
            (prev_port_ds.portInternal != port_ds->portInternal) ||
            (prev_port_ds.dest_adr_type != port_ds->dest_adr_type) ||
            (prev_port_ds.localPriority != port_ds->localPriority) ||
            (prev_port_ds.twoStepOverride != port_ds->twoStepOverride)||
            (prev_port_ds.notMaster != port_ds->notMaster)||
            (prev_port_ds.masterOnly != port_ds->masterOnly)||
             (prev_port_ds.c_802_1as.syncReceiptTimeout != port_ds->c_802_1as.syncReceiptTimeout));

}
mesa_rc vtss_appl_ptp_mgmt_config_port_ds_set(uint instance, vtss_ifindex_t portnum, vtss_appl_ptp_config_port_ds_t prev_port_ds)
{
     u32 v;
     i8 temp;
     VTSS_RC(ptp_ifindex_to_port(portnum, &v));
     PtpPort_t *ptpPort = &(ptp_global.ptpi[instance])->ptpPort[(iport2uport(v))-1];
     vtss_appl_ptp_config_port_ds_t init_port_ds;
     vtss_ptp_apply_profile_defaults_to_port_ds(&init_port_ds, ptpPort->parent->clock_init->cfg.profile);

     if (ptpPort->port_config->c_802_1as.useMgtSettableLogSyncInterval == TRUE) {
         if(ptpPort->port_config->c_802_1as.mgtSettableLogSyncInterval > ptpPort->portDS.status.s_802_1as.currentLogSyncInterval)     {
             ptpPort->msm.syncSlowdown = TRUE;
         }
         ptpPort->portDS.status.s_802_1as.currentLogSyncInterval = ptpPort->port_config->c_802_1as.mgtSettableLogSyncInterval;
         if(ptpPort->msm.syncSlowdown != TRUE){
             ptpPort->msm.sync_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogSyncInterval;
         }

     } else if(ptpPort->port_config->c_802_1as.useMgtSettableLogSyncInterval == FALSE && prev_port_ds.c_802_1as.useMgtSettableLogSyncInterval == TRUE){

         temp = (ptpPort->port_config->logSyncInterval == 126 || ptpPort->port_config->logSyncInterval == 127) ? init_port_ds.logSyncInterval : ptpPort->port_config->logSyncInterval;
         if(temp > ptpPort->portDS.status.s_802_1as.currentLogSyncInterval)     {
             ptpPort->msm.syncSlowdown = TRUE;
         }
         ptpPort->portDS.status.s_802_1as.currentLogSyncInterval = temp;
         if(ptpPort->msm.syncSlowdown != TRUE){
             ptpPort->msm.sync_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogSyncInterval;
         }
     }
     if (ptpPort->port_config->c_802_1as.useMgtSettableLogAnnounceInterval == TRUE) {
         if ( ptpPort->port_config->c_802_1as.mgtSettableLogAnnounceInterval > ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval) {
             ptpPort->ansm.announceSlowdown = TRUE;
         }
         ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval = ptpPort->port_config->c_802_1as.mgtSettableLogAnnounceInterval;
         if ( ptpPort->ansm.announceSlowdown != TRUE){
             ptpPort->ansm.ann_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval;
         }
     } else if(ptpPort->port_config->c_802_1as.useMgtSettableLogAnnounceInterval == FALSE && prev_port_ds.c_802_1as.useMgtSettableLogAnnounceInterval == TRUE){
         temp = (ptpPort->port_config->logAnnounceInterval == 126 || ptpPort->port_config->logAnnounceInterval == 127) ? init_port_ds.logAnnounceInterval : ptpPort->port_config->logAnnounceInterval;
         if ( temp > ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval) {
             ptpPort->ansm.announceSlowdown = TRUE;
         }
         ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval = temp;
         if ( ptpPort->ansm.announceSlowdown != TRUE){
             ptpPort->ansm.ann_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogAnnounceInterval;
         }
     }
     if (ptpPort->port_config->c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval == TRUE) {
         ptpPort->portDS.status.s_802_1as.peer_d.currentLogPDelayReqInterval = ptpPort->port_config->c_802_1as.peer_d.mgtSettableLogPdelayReqInterval == 126 ? init_port_ds.logMinPdelayReqInterval : ptpPort->port_config->c_802_1as.peer_d.mgtSettableLogPdelayReqInterval;
     } else if(ptpPort->port_config->c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval == FALSE && prev_port_ds.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval == TRUE) {
         ptpPort->portDS.status.s_802_1as.peer_d.currentLogPDelayReqInterval = (ptpPort->port_config->logMinPdelayReqInterval == 126 || ptpPort->port_config->logMinPdelayReqInterval == 127) ? init_port_ds.logMinPdelayReqInterval : ptpPort->port_config->logMinPdelayReqInterval;
     }
     if (ptpPort->port_config->c_802_1as.useMgtSettableLogGptpCapableMessageInterval == TRUE) {
         if(ptpPort->port_config->c_802_1as.mgtSettableLogGptpCapableMessageInterval > ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval){
             ptpPort->gptpsm.gPtpCapableMessageSlowdown = TRUE;
         }
         ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval = ptpPort->port_config->c_802_1as.mgtSettableLogGptpCapableMessageInterval == 126 ? ptpPort->port_config->c_802_1as.initialLogGptpCapableMessageInterval : ptpPort->port_config->c_802_1as.mgtSettableLogGptpCapableMessageInterval;
         if(ptpPort->gptpsm.gPtpCapableMessageSlowdown != TRUE){
             ptpPort->gptpsm.gptp_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval;
         }
        T_DG(_I,"useMgtSettableLogGptpCapableMessageInterval is Enabled. mgtSettableLogGptpCapableMessageInterval=%u, currentLogGptpCapableMessageInterval=%u, gptp_log_msg_period=%d, gptp_log_msg_period=%u", ptpPort->port_config->c_802_1as.mgtSettableLogGptpCapableMessageInterval, ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval, ptpPort->gptpsm.gPtpCapableMessageSlowdown, ptpPort->gptpsm.gptp_log_msg_period);

     } else if(ptpPort->port_config->c_802_1as.useMgtSettableLogGptpCapableMessageInterval == FALSE && prev_port_ds.c_802_1as.useMgtSettableLogGptpCapableMessageInterval == TRUE){
         temp = (ptpPort->port_config->c_802_1as.initialLogGptpCapableMessageInterval == 126 || ptpPort->port_config->c_802_1as.initialLogGptpCapableMessageInterval == 127) ? init_port_ds.c_802_1as.initialLogGptpCapableMessageInterval : ptpPort->port_config->c_802_1as.initialLogGptpCapableMessageInterval;
         if(temp > ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval){
             ptpPort->gptpsm.gPtpCapableMessageSlowdown = TRUE;
         }
         ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval = temp;
         if(ptpPort->gptpsm.gPtpCapableMessageSlowdown != TRUE){
             ptpPort->gptpsm.gptp_log_msg_period = ptpPort->portDS.status.s_802_1as.currentLogGptpCapableMessageInterval;
         }
     }
     return VTSS_RC_OK;
}

mesa_rc vtss_appl_ptp_config_clocks_port_ds_set(uint instance, vtss_ifindex_t portnum, const vtss_appl_ptp_config_port_ds_t *port_ds)
{
    PTP_CORE_LOCK_SCOPE();

    u32 v;
    VTSS_RC(ptp_ifindex_to_port(portnum, &v));
    if (((port_ds->enabled == 0) || (port_ds->enabled == 1)) &&
        (((port_ds->logAnnounceInterval >= -3) && (port_ds->logAnnounceInterval <= 4)) || ((config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) && 
        (port_ds->logAnnounceInterval == 126 || port_ds->logAnnounceInterval ==127))) && ((port_ds->announceReceiptTimeout >= 1) && (port_ds->announceReceiptTimeout <= 10)) &&
        (((port_ds->logSyncInterval >= -7) && (port_ds->logSyncInterval <= 4)) || ((config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) && 
        (port_ds->logSyncInterval == 126 || port_ds->logSyncInterval ==127)) || ((port_ds->c_802_1as.mgtSettableLogSyncInterval >= -7) && (port_ds->c_802_1as.mgtSettableLogSyncInterval <= 4))) &&
        ((port_ds->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_E2E) || (port_ds->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_P2P) || (port_ds->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_COMMON_P2P) || (port_ds->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_DISABLED)) &&
        (((port_ds->logMinPdelayReqInterval >= -7) && (port_ds->logMinPdelayReqInterval <= 5)) || ((config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) && 
        (port_ds->logMinPdelayReqInterval == 126 || port_ds->logMinPdelayReqInterval ==127))) &&
        ((port_ds->delayAsymmetry >= (-100000LL<<16)) && (port_ds->delayAsymmetry <= (100000LL<<16))) &&
        ((port_ds->portInternal == 0) || (port_ds->portInternal == 1)) &&
        ((port_ds->ingressLatency >= (-100000LL<<16)) && (port_ds->ingressLatency <= (100000LL<<16))) &&
        ((port_ds->egressLatency >= (-100000LL<<16)) && (port_ds->egressLatency <= (100000LL<<16))) &&
        (port_ds->versionNumber ==  2) &&
        (!(port_ds->masterOnly && port_ds->notMaster)) &&
        ((port_ds->twoStepOverride >= VTSS_APPL_PTP_TWO_STEP_OVERRIDE_NONE) && (port_ds->twoStepOverride <= VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE)))
    {
        mesa_rc rc = VTSS_RC_ERROR;
        bool temp_int = false;
        u8 temp;
        bool ena = false, disa = false;
        T_DG(_I,"useMgtSettableLogGptpCapableMessageInterval=%s, mgtSettableLogGptpCapableMessageInterval:%u",
            (port_ds->c_802_1as.useMgtSettableLogGptpCapableMessageInterval? "Enabled" : "Disabled"), port_ds->c_802_1as.mgtSettableLogGptpCapableMessageInterval);

        // Enabling 'masterOnly' option on slaveOnly instance is invalid.
        if (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY && port_ds->masterOnly) {
            T_W("cannot enable master only option on slave only clock");
            return rc;
        } else if (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY && port_ds->notMaster) {
            T_W("cannot enable not master option on master only clock");
            return rc;
        } else if ((port_ds->masterOnly || port_ds->notMaster) && (config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_G_8265_1)) {
            T_W("masterOnly or notMaster option cannot be enabled for 8265 profile.");
            return rc;
        } else if (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_AED_GM && port_ds->aedPortState == VTSS_APPL_PTP_PORT_STATE_AED_SLAVE) {
            T_W("AED port role can only be master for a Grandmaster device");
            return rc;
        } else if (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_INTERNAL) {
            if (port_ds->masterOnly) {
                T_W("cannot enable master only option on Internal mode clock.");
                return rc;
            } else if (port_ds->enabled) {
                T_W("Cannot enable internal mode on ports.");
                return rc;
            }
        }
        /* Before doing anything else, check if port needs to be enabled or disabled */
        T_DG(_I,"instance %d, port %d, new ena %d, old ena %d, domain %d", instance, v, port_ds->enabled, config_data.conf[instance].port_config[v].enabled,  port_data[v].port_domain);
        if ((port_ds->enabled == 1) && config_data.conf[instance].port_config[v].enabled == 0) {
            ena = true;
        }
        else if ((port_ds->enabled == 0) && config_data.conf[instance].port_config[v].enabled == 1) {
            disa= true;
        }
        // For 2-step clocks on phy ports, create FIFO in TOD module
        if (port_data[v].topo.ts_feature == VTSS_PTP_TS_PTS &&
           (port_ds->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE ||
            config_data.conf[instance].clock_init.cfg.twoStepFlag)) {
            if (ena) {
                if (tod_mod_man_tx_ts_queue_init(v) == VTSS_RC_ERROR) {
                    return rc;
                }
            }
        }

        if (config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS ||
            config_data.conf[instance].clock_init.cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
            for (int inst = 0; inst < PTP_CLOCK_INSTANCES; inst++) {
                if (inst != instance) {
                    if (config_data.conf[inst].port_config[v].enabled && port_ds->enabled) {
                        if ((config_data.conf[inst].clock_init.cfg.clock_domain < mesa_cap_ts_domain_cnt) ||
                            (config_data.conf[inst].clock_init.cfg.clock_domain >= mesa_cap_ts_domain_cnt &&
                             config_data.conf[instance].clock_init.cfg.clock_domain < mesa_cap_ts_domain_cnt)) {
                            T_W("Multiple instances can be enabled only with software clocks.");
                            return rc;
                        }

                        if ((config_data.conf[inst].port_config[v].delayMechanism != VTSS_APPL_PTP_DELAY_MECH_COMMON_P2P) ||
                            (config_data.conf[instance].port_config[v].enabled && port_ds->delayMechanism != VTSS_APPL_PTP_DELAY_MECH_COMMON_P2P)) {
                            T_W("Multiple instances can be enabled only using common mean link delay mechanism on all instances of port.");
                            return rc;
                        }
                    }
                }
            }
        }

        if (ena) {
            if (ptp_port_ena(port_ds->portInternal, iport2uport(v), instance) == VTSS_RC_ERROR) return rc;
        }
        else if (disa) {
            if (ptp_port_dis(iport2uport(v), instance) == VTSS_RC_ERROR) return rc;
        }

        if (port_data[v].topo.ts_feature == VTSS_PTP_TS_PTS) {
            if (disa) {
                // No need to return error if it is already deleted.
                (void)tod_mod_man_tx_ts_queue_deinit(v);
            }
        }

        if (port_ds->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_P2P && config_data.conf[instance].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_IP4UNI) {
            T_W("Combined Ipv4 Unicast and PeerToPeer is not supported");
            return rc;
        }
#if defined (__MIPSEL__)
        if ((port_ds->logSyncInterval < -3 || port_ds->logMinPdelayReqInterval < 0) && ((config_data.conf[instance].clock_init.cfg.twoStepFlag && port_ds->twoStepOverride != VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE) || port_ds->twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE)){
            T_W("This device does only support twoStep with a maximum sync interval of 125ms and delay request interval of 1 sec as per 802.1AS standard");
            return rc;
        }
#endif
        if (instance < PTP_CLOCK_INSTANCES) {
            /* preserve initportState, and portInternal state */
            temp_int = config_data.conf[instance].port_config[v].portInternal;
            temp = config_data.conf[instance].port_config[v].enabled;
            if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
                PTP_RETURN(vtss_1588_ingress_latency_set(l2port_to_ptp_api(v), port_ds->ingressLatency));
                PTP_RETURN(vtss_1588_egress_latency_set(l2port_to_ptp_api(v), port_ds->egressLatency));
                PTP_RETURN(vtss_1588_asymmetry_set(l2port_to_ptp_api(v), port_ds->delayAsymmetry));
            }
            vtss_appl_ptp_config_port_ds_t prev_port_ds;
            prev_port_ds = config_data.conf[instance].port_config[v];
            config_data.conf[instance].port_config[v] = *port_ds;
            config_data.conf[instance].port_config[v].enabled = temp;
            config_data.conf[instance].port_config[v].portInternal = temp_int;
            if (PTP_READY()) {
                if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
                    vtss_appl_ptp_mgmt_config_port_ds_set(instance, portnum, prev_port_ds);
                    if(has_mgmt_conf_changed(prev_port_ds, port_ds) && !ptp_state_init_required(prev_port_ds, port_ds)){
                        return VTSS_RC_OK;
                    }
                    rc = vtss_ptp_set_port_cfg(ptp_global.ptpi[instance], iport2uport(v), port_ds);
                }
                if (rc == VTSS_RC_OK) {
                    if ((rc = ptp_ace_update(instance)) != VTSS_RC_OK) {
                        if (rc != PTP_RC_MISSING_IP_ADDRESS) {
                            if (ptp_port_dis(iport2uport(v), instance) != VTSS_RC_OK) return rc;
                        }
                        T_IG(_I,"ptp_ace_update failed");
                    }
                }
            } else {
                rc = VTSS_RC_OK;
            }
        }
        return rc;
    }
    else return VTSS_RC_ERROR;
}

void ptp_apply_profile_defaults_to_port_ds(vtss_appl_ptp_config_port_ds_t *port_ds, vtss_appl_ptp_profile_t profile)
{
    vtss_ptp_apply_profile_defaults_to_port_ds(port_ds, profile);
}

mesa_rc vtss_appl_ptp_config_clocks_port_ds_get(uint instance, vtss_ifindex_t portnum, vtss_appl_ptp_config_port_ds_t *port_ds)
{
    PTP_CORE_LOCK_SCOPE();

    u32 v;
    VTSS_RC(ptp_ifindex_to_port(portnum, &v));
    if ((instance < PTP_CLOCK_INSTANCES) /* && (0 <= v) */ && (v < config_data.conf[instance].clock_init.numberPorts)) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            if (PTP_READY()) {
                vtss_1588_ingress_latency_get(l2port_to_ptp_api(v), &config_data.conf[instance].port_config[v].ingressLatency);
                vtss_1588_egress_latency_get(l2port_to_ptp_api(v), &config_data.conf[instance].port_config[v].egressLatency);
                vtss_1588_asymmetry_get(l2port_to_ptp_api(v), &config_data.conf[instance].port_config[v].delayAsymmetry);
            }
            *port_ds = config_data.conf[instance].port_config[v];
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

bool ptp_get_port_foreign_ds(ptp_foreign_ds_t *f_ds, int portnum, i16 ix, uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            return vtss_ptp_get_port_foreign_ds(ptp_global.ptpi[instance], portnum, ix, f_ds);
        }
    }
    return false;
}

/**
 * \brief Set filter parameters for a Default PTP filter instance.
 *
 */
mesa_rc vtss_appl_ptp_clock_filter_parameters_set(uint instance, const vtss_appl_ptp_clock_filter_config_t *const c)
{
    PTP_CORE_LOCK_SCOPE();

    /*lint -save -e685 -e568 */
    if ((c->delay_filter <= 6) && ((c->period >= 1) && (c->period <= 10000)) && (c->dist <= 10))
    {
        /*lint -restore */
        if (PTP_READY() && (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_NONE)) return VTSS_RC_ERROR;

        if (instance < PTP_CLOCK_INSTANCES) {
            config_data.conf[instance].filter_params = *c;
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

/**
 * \brief Get filter parameters for a PTP filter instance.
 *
 */
mesa_rc vtss_appl_ptp_clock_filter_parameters_get(uint instance, vtss_appl_ptp_clock_filter_config_t *const c)
{
    PTP_CORE_LOCK_SCOPE();

    if (instance < PTP_CLOCK_INSTANCES) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            *c = config_data.conf[instance].filter_params;
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

/**
 * \brief Get default filter parameters for a Default PTP filter instance.
 *
 */
void vtss_appl_ptp_filter_default_parameters_get(vtss_appl_ptp_clock_filter_config_t *c, vtss_appl_ptp_profile_t profile)
{
    switch (profile) {
        case VTSS_APPL_PTP_PROFILE_G_8275_1:
            c->delay_filter = DEFAULT_DELAY_S;
            c->period = 1;
            c->dist = 0;
            break;
        case  VTSS_APPL_PTP_PROFILE_IEEE_802_1AS:
        case  VTSS_APPL_PTP_PROFILE_AED_802_1AS:
        case VTSS_APPL_PTP_PROFILE_NO_PROFILE:
        case  VTSS_APPL_PTP_PROFILE_1588:
        case  VTSS_APPL_PTP_PROFILE_G_8265_1:
        default:
            c->delay_filter = DEFAULT_DELAY_S;
            c->period = 1;
            c->dist = 2;
            break;
    }
}

/**
 * \brief Set filter parameters for a Default PTP servo instance.
 *
 */
mesa_rc vtss_appl_ptp_clock_servo_parameters_set(uint instance, const vtss_appl_ptp_clock_servo_config_t *const c)
{
    PTP_CORE_LOCK_SCOPE();

    if (((c->display_stats == 0) || (c->display_stats == 1)) &&
        ((c->p_reg == 0) || (c->p_reg == 1)) &&
        ((c->i_reg == 0) || (c->i_reg == 1)) &&
        ((c->d_reg == 0) || (c->d_reg == 1)) &&
        ((c->ap >= 1) && (c->ap <= 1000)) &&
        ((c->ai >= 1) && (c->ai <= 10000)) &&
        ((c->ad >= 1) && (c->ad <= 10000)) &&
        ((c->gain >= 1) && (c->gain <= 10000)) &&
        ((c->srv_option == VTSS_APPL_PTP_CLOCK_FREE) || (c->srv_option == VTSS_APPL_PTP_CLOCK_SYNCE)) &&
        ((c->synce_threshold >= 1) && (c->synce_threshold <= 1000)) &&
        ((c->synce_ap >= 1) && (c->synce_ap <= 40)) &&
        ((c->ho_filter >= 10) && (c->ho_filter <= 86400)) &&
        ((c->stable_adj_threshold >= 1) && (c->stable_adj_threshold <= 1000)))
    {
        if (instance < PTP_CLOCK_INSTANCES) {
            config_data.conf[instance].servo_params = *c;
            if (PTP_READY()) {
                if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) return VTSS_RC_OK;
            }
            else return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

/**
 * \brief Clear (i.e. reset) the servo of a PTP instance.
 *
 */
mesa_rc vtss_appl_ptp_clock_servo_clear(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    if (instance < PTP_CLOCK_INSTANCES) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
#ifdef SW_OPTION_BASIC_PTP_SERVO
            if (config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_BASIC) {
                basic_servo[instance]->clock_servo_reset(SET_VCXO_FREQ);
            }
#endif // SW_OPTION_BASIC_PTP_SERVO
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

/**
 * \brief Get filter parameters for a PTP servo instance.
 *
 */
mesa_rc vtss_appl_ptp_clock_servo_parameters_get(uint instance, vtss_appl_ptp_clock_servo_config_t *const c)
{
    PTP_CORE_LOCK_SCOPE();

    if (instance < PTP_CLOCK_INSTANCES) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            *c = config_data.conf[instance].servo_params;
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

/**
 * \brief Get default filter parameters for a Default PTP servo
 *
 */
void vtss_appl_ptp_clock_servo_default_parameters_get(vtss_appl_ptp_clock_servo_config_t *c, vtss_appl_ptp_profile_t profile)
{
    switch (profile) {
        case VTSS_APPL_PTP_PROFILE_G_8275_1:
            c->display_stats = false;
            c->p_reg = true;
            c->i_reg = true;
            c->d_reg = true;
            c->ap = 12;
            c->ai = 512;
            c->ad = 7;
            c->gain = 16;
            c->srv_option = VTSS_APPL_PTP_CLOCK_FREE;
            c->synce_threshold = 1000;
            c->synce_ap = 2;
            c->ho_filter = 960;
            c->stable_adj_threshold = 1000;  /* = 100 ppb */
            break;
        case  VTSS_APPL_PTP_PROFILE_1588:
        case  VTSS_APPL_PTP_PROFILE_IEEE_802_1AS:
        case  VTSS_APPL_PTP_PROFILE_AED_802_1AS:
            c->display_stats = false;
            c->p_reg = true;
            c->i_reg = true;
            c->d_reg = true;
            c->ap = DEFAULT_AP;
            c->ai = DEFAULT_AI;
            c->ad = DEFAULT_AD;
            c->gain = fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_LAN966X ? 5 : 1;
            c->srv_option = VTSS_APPL_PTP_CLOCK_FREE;
            c->synce_threshold = 1000;
            c->synce_ap = 2;
            c->ho_filter = 60;
            c->stable_adj_threshold = 300;  /* = 30 ppb */
            break;
        case VTSS_APPL_PTP_PROFILE_NO_PROFILE:
        case  VTSS_APPL_PTP_PROFILE_G_8265_1:
        case  VTSS_APPL_PTP_PROFILE_G_8275_2:
        default:
            c->display_stats = false;
            c->p_reg = true;
            c->i_reg = true;
            c->d_reg = true;
            c->ap = DEFAULT_AP;
            c->ai = DEFAULT_AI;
            c->ad = DEFAULT_AD;
            c->gain = 1;
            c->srv_option = VTSS_APPL_PTP_CLOCK_FREE;
            c->synce_threshold = 1000;
            c->synce_ap = 2;
            c->ho_filter = 60;
            c->stable_adj_threshold = 300;  /* = 30 ppb */
    }
}

mesa_rc vtss_appl_ptp_clock_servo_status_get(uint instance, vtss_ptp_servo_status_t *s)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
#if defined(VTSS_SW_OPTION_ZLS30387)
            if (config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_DEFAULT ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_XO ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_XO ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_TCXO ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_TCXO ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_OCXO_S3E ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_FREQ ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_PHASE ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_FREQ ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_FDD ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_XDSL ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_FREQ ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_PHASE ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C60W ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C150 ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C180 ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C240 ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E_R4_6_1 ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE ||
                config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE_LOW
                )
            {
                advanced_servo[instance]->clock_servo_status(config_data.conf[instance].clock_init.cfg.clock_domain, s);
            }
            else
#endif
#ifdef SW_OPTION_BASIC_PTP_SERVO
            if (config_data.conf[instance].clock_init.cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_BASIC) {
                basic_servo[instance]->clock_servo_status(config_data.conf[instance].clock_init.cfg.clock_domain, s);
            }
            else
#else
            {
                return VTSS_RC_ERROR;
            }
#endif // SW_OPTION_BASIC_PTP_SERVO
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

void vtss_ptp_clock_slave_config_set(ptp_servo *servo, const vtss_appl_ptp_clock_slave_config_t *cfg)
{
    servo->stable_offset_threshold = ((i64)cfg->stable_offset)<<16;
    servo->offset_ok_threshold = ((i64)cfg->offset_ok)<<16;
    servo->offset_fail_threshold = ((i64)cfg->offset_fail)<<16;
}

mesa_rc vtss_appl_ptp_clock_status_slave_ds_get(uint instance, vtss_appl_ptp_clock_slave_ds_t *const status)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            vtss_ptp_get_clock_slave_ds(ptp_global.ptpi[instance], status);
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

/**
 * \brief Set unicast slave configuration parameters.
 *
 */
mesa_rc vtss_appl_ptp_clock_config_unicast_slave_config_set(uint instance, uint idx, const vtss_appl_ptp_unicast_slave_config_t *const c)
{
    PTP_CORE_LOCK_SCOPE();

    if (((c->duration >= 10) && (c->duration <= 1000)) &&
        (((c->ip_addr & 0xff000000U) != 0) || (c->ip_addr == 0)) &&
        ((c->ip_addr & 0xff000000U) != 0x7f000000U) &&
        ((c->ip_addr & 0xff000000U) <= 0xdf000000U))
    {
        if ((instance < PTP_CLOCK_INSTANCES) && (idx < MAX_UNICAST_MASTERS_PR_SLAVE)) {
            config_data.conf[instance].unicast_slave[idx] = *c;
            if (PTP_READY()) {
                if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
                    vtss_ptp_uni_slave_conf_set(ptp_global.ptpi[instance], idx, c);
                    return VTSS_RC_OK;
                }
            } else return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

/**
 * \brief Get unicast slave configuration parameters.
 *
 */
mesa_rc vtss_appl_ptp_clock_config_unicast_slave_config_get(uint instance, uint idx, vtss_appl_ptp_unicast_slave_config_t *const c)
{
    PTP_CORE_LOCK_SCOPE();

    if ((instance < PTP_CLOCK_INSTANCES) && idx < MAX_UNICAST_MASTERS_PR_SLAVE) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            vtss_ptp_uni_slave_conf_get(ptp_global.ptpi[instance], idx, c);
//            vtss_ptp_uni_slave_conf_state_get(ptp_global.ptpi[instance], idx, c);
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}

/**
 * \brief Get unicast slave table data.
 *
 */
mesa_rc vtss_appl_ptp_clock_status_unicast_slave_table_get(uint instance, uint ix, vtss_appl_ptp_unicast_slave_table_t *const uni_slave_table)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
            return vtss_ptp_clock_unicast_table_get(ptp_global.ptpi[instance], uni_slave_table, ix);
        }
    }
    return VTSS_RC_ERROR;
}

/**
* \brief Get unicast master table data.
*
*/
mesa_rc vtss_appl_ptp_clock_slave_itr(const uint *const clock_prev, uint *const clock_next, const uint *const slave_prev, uint *const slave_next)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    vtss_ptp_master_table_key_t prev_key, next_key;
    if (clock_prev == 0) {
        rc = vtss_appl_ptp_clock_slave_itr_get(0, &next_key);
    }
    else if (slave_prev == 0) {
        prev_key.ip = 0;
        prev_key.inst = *clock_prev;
        rc = vtss_appl_ptp_clock_slave_itr_get(&prev_key, &next_key);
    }
    else {
        prev_key.ip = *slave_prev;
        prev_key.inst = *clock_prev;
        rc = vtss_appl_ptp_clock_slave_itr_get(&prev_key, &next_key);
    }
    if (rc == VTSS_RC_OK) {
        *slave_next = next_key.ip;
        *clock_next = next_key.inst;
    }
    return rc;
}

mesa_rc vtss_appl_ptp_clock_status_unicast_master_table_get(uint instance, u32 ip, vtss_appl_ptp_unicast_master_table_t *uni_master_table)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        vtss_ptp_master_table_key_t key;
        key.ip = ip;
        key.inst = instance;
        return vtss_ptp_clock_status_unicast_master_table_get(key, uni_master_table);
    }
    return VTSS_RC_ERROR;
}

void ptp_local_clock_time_set(mesa_timestamp_t *t, u32 domain)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY()) {
        // When called from startup-config, PTP is not yet ready. Postpone setting of
        // clock till PTP is started
        vtss_local_clock_time_set(t, domain);
    }
}

mesa_rc vtss_appl_ptp_clock_control_get(uint instance, vtss_appl_ptp_clock_control_t *const port_control)
{
    if (instance < PTP_CLOCK_INSTANCES) {
        memset(port_control, 0, sizeof(vtss_appl_ptp_clock_control_t));
        return VTSS_RC_OK;
    }
    else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc vtss_appl_ptp_clock_control_set(uint instance, const vtss_appl_ptp_clock_control_t *const port_control)
{
    mesa_timestamp_t t;

    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (port_control->syncToSystemClock == true) {
            t.sec_msb = 0;
            t.seconds = time(NULL);
            t.nanoseconds = 0;
            ptp_local_clock_time_set(&t, ptp_instance_2_timing_domain(instance));
        }
        return VTSS_RC_OK;
    }
    else {
        return VTSS_RC_ERROR;
    }
}

/**
 * \brief Get observed egress latency.
 *
 */
void ptp_clock_egress_latency_get(observed_egr_lat_t *lat)
{
    PTP_CORE_LOCK_SCOPE();

    *lat = observed_egr_lat;
}

/**
 * \brief Clear observer egress latency.
 *
 */
void ptp_clock_egress_latency_clear(void)
{
    PTP_CORE_LOCK_SCOPE();

    observed_egr_lat.cnt = 0;
    observed_egr_lat.min = 0;
    observed_egr_lat.mean = 0;
    observed_egr_lat.max = 0;
}

/* Get external clock output configuration. */
mesa_rc vtss_appl_ptp_ext_clock_out_get(vtss_appl_ptp_ext_clock_mode_t *mode)
{
    *mode = config_data.init_ext_clock_mode;
    return VTSS_RC_OK;
}

/* Get default external clock output configuration. */
void vtss_ext_clock_out_default_get(vtss_appl_ptp_ext_clock_mode_t *mode)
{
    mode->one_pps_mode = VTSS_APPL_PTP_ONE_PPS_OUTPUT;
    mode->clock_out_enable = false;
    mode->adj_method = VTSS_APPL_PTP_PREFERRED_ADJ_AUTO;
    mode->freq = 1;
    mode->clk_domain = 0;
}

/* Set debug_mode. */
bool ptp_debug_mode_set(int debug_mode, uint instance, BOOL has_log_to_file, BOOL has_control, u32 log_time)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        return vtss_ptp_debug_mode_set(ptp_global.ptpi[instance], debug_mode, has_log_to_file, has_control, log_time);
    }
    return false;
}

/* Get debug_mode. */
bool ptp_debug_mode_get(uint instance, vtss_ptp_logmode_t *log_mode)
{
    PTP_CORE_LOCK_SCOPE();

    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        return vtss_ptp_debug_mode_get(ptp_global.ptpi[instance], log_mode);
    }
    return false;
}

/* Delete PTP log file */
bool ptp_log_delete(uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    return vtss_ptp_log_delete(ptp_global.ptpi[instance]);
}

/* Get afi mode. */
mesa_rc ptp_afi_mode_get(uint instance, bool ann, bool* enable)
{
    PTP_CORE_LOCK_SCOPE();
    if (instance < PTP_CLOCK_INSTANCES) {
        if (ann) {
            *enable = config_data.conf[instance].clock_init.afi_announce_enable;
        } else {
            *enable = config_data.conf[instance].clock_init.afi_sync_enable;
        }
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

/* Set afi_mode. */
mesa_rc ptp_afi_mode_set(uint instance, bool ann, bool enable)
{
    PTP_CORE_LOCK_SCOPE();
    if (instance < PTP_CLOCK_INSTANCES) {
        if (ann) {
            config_data.conf[instance].clock_init.afi_announce_enable = enable;
        } else {
            config_data.conf[instance].clock_init.afi_sync_enable = enable;
        }
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

void vtss_appl_ptp_cmlds_conf_defaults_get(vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t *const conf)
{
    conf->peer_d.meanLinkDelayThresh = 800LL<<16;
    conf->delayAsymmetry = DEFAULT_DELAY_ASYMMETRY<<16;
    conf->peer_d.initialLogPdelayReqInterval = DEFAULT_DELAY_REQ_INTERVAL;
    conf->peer_d.useMgtSettableLogPdelayReqInterval = false;
    conf->peer_d.mgtSettableLogPdelayReqInterval = DEFAULT_DELAY_REQ_INTERVAL;
    conf->peer_d.initialComputeNeighborRateRatio = true;
    conf->peer_d.useMgtSettableComputeNeighborRateRatio = false;
    conf->peer_d.mgtSettableComputeNeighborRateRatio = true;
    conf->peer_d.initialComputeMeanLinkDelay = true;
    conf->peer_d.useMgtSettableComputeMeanLinkDelay = false;
    conf->peer_d.mgtSettableComputeMeanLinkDelay = true;
    conf->peer_d.allowedLostResponses = DEFAULT_MAX_CONTIGOUS_MISSED_PDELAY_RESPONSE;
    conf->peer_d.allowedFaults = DEFAULT_MAX_PDELAY_ALLOWED_FAULTS;
}

mesa_rc vtss_appl_ptp_cmlds_port_status_get(vtss_uport_no_t portnum, vtss_appl_ptp_802_1as_cmlds_status_port_ds_t *const status)
{
#if defined (VTSS_SW_OPTION_P802_1_AS)
    uint iport = uport2iport(portnum);
    PTP_CORE_LOCK_SCOPE();
    if (iport < mesa_cap_port_cnt)
    {
        *status = ptp_global.ptp_cm.cmlds_port_ds[iport]->status;
        return VTSS_RC_OK;
    }
#endif
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_cmlds_port_statistics_get(vtss_uport_no_t portnum, vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t *const statistics)
{
#if defined (VTSS_SW_OPTION_P802_1_AS)
    uint iport = uport2iport(portnum);
    PTP_CORE_LOCK_SCOPE();
    if (iport < mesa_cap_port_cnt)
    {
        *statistics = ptp_global.ptp_cm.cmlds_port_ds[iport]->statistics;
        return VTSS_RC_OK;
    }
#endif
    return VTSS_RC_ERROR;

}

mesa_rc vtss_appl_ptp_cmlds_port_statistics_get_clear(vtss_uport_no_t portnum, vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t *const statistics)
{
#if defined (VTSS_SW_OPTION_P802_1_AS)
    uint iport = uport2iport(portnum);
    PTP_CORE_LOCK_SCOPE();
    if (iport < mesa_cap_port_cnt)
    {
        *statistics = ptp_global.ptp_cm.cmlds_port_ds[iport]->statistics;
        memset(&ptp_global.ptp_cm.cmlds_port_ds[iport]->statistics, 0, sizeof(vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t));
        return VTSS_RC_OK;
    }
#endif
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_cmlds_port_conf_get(vtss_uport_no_t portnum, vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t *const conf)
{
#if defined (VTSS_SW_OPTION_P802_1_AS)
    uint iport = uport2iport(portnum);
    PTP_CORE_LOCK_SCOPE();
    if (iport < mesa_cap_port_cnt) {
        *conf = config_data.cmlds_port_conf[iport];
        return VTSS_RC_OK;
    }
#endif
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_cmlds_port_conf_set(vtss_uport_no_t portnum, const vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t *const conf)
{
#if defined (VTSS_SW_OPTION_P802_1_AS)
    uint iport = uport2iport(portnum);
    PTP_CORE_LOCK_SCOPE();
    if (iport < mesa_cap_port_cnt) {
        config_data.cmlds_port_conf[iport] = *conf;
        ptp_global.ptp_cm.cmlds_port_ds[iport]->conf_modified = true;
        return VTSS_RC_OK;
    }
#endif
    return VTSS_RC_ERROR;
}

mesa_rc vtss_appl_ptp_cmlds_default_ds_get(vtss_appl_ptp_802_1as_cmlds_default_ds_t *const def_ds)
{
#if defined (VTSS_SW_OPTION_P802_1_AS)
    PTP_CORE_LOCK_SCOPE();
    *def_ds = ptp_global.ptp_cm.cmlds_defaults;

    return VTSS_RC_OK;
#else
    return VTSS_RC_ERROR;
#endif
}
/* Set external clock output configuration. */
static void ext_clock_out_set(const vtss_appl_ptp_ext_clock_mode_t *mode)
{
    if (mesa_cap_ts) {
        mesa_ts_ext_clock_one_pps_mode_t m;
        mesa_ts_ext_clock_mode_t clock_mode;

        m = mode->one_pps_mode == VTSS_APPL_PTP_ONE_PPS_OUTPUT ? MESA_TS_EXT_CLOCK_MODE_ONE_PPS_OUTPUT : MESA_TS_EXT_CLOCK_MODE_ONE_PPS_DISABLE;
        clock_mode.one_pps_mode = m;
        clock_mode.enable = mode->clock_out_enable;
        clock_mode.freq = mode->freq;
        clock_mode.domain = mode->clk_domain;

        PTP_RC(mesa_ts_external_clock_mode_set( NULL, &clock_mode));
    } else {
        T_D("Clock mode set not supported, mode = %d, freq = %u",mode->clock_out_enable, mode->freq);
    }
}
/* Set external clock output configuration. */
mesa_rc vtss_appl_ptp_ext_clock_out_set(const vtss_appl_ptp_ext_clock_mode_t *mode)
{
    int instance;
    /*lint -save -e568 */
    if (((mode->one_pps_mode >= VTSS_APPL_PTP_ONE_PPS_DISABLE) && (mode->one_pps_mode <= VTSS_APPL_PTP_ONE_PPS_OUTPUT_INPUT)) &&
        ((mode->clock_out_enable == 0) || (mode->clock_out_enable == 1)) &&
        (
         (mode->adj_method == VTSS_APPL_PTP_PREFERRED_ADJ_LTC)                                            ||
         (mode->adj_method == VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE      && meba_cap_synce_dpll_mode_single) ||
         (mode->adj_method == VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT && meba_cap_synce_dpll_mode_dual)   ||
         (mode->adj_method == VTSS_APPL_PTP_PREFERRED_ADJ_COMMON      && meba_cap_synce_dpll_mode_single) ||
         (mode->adj_method == VTSS_APPL_PTP_PREFERRED_ADJ_AUTO)
        ) &&
        ((mode->freq >= 1) && (mode->freq <= 25000000)) &&
        (mode->clk_domain < mesa_cap_ts_domain_cnt))
    {
        T_IG(VTSS_TRACE_GRP_MS_SERVO,"mode: one_pps_mode %d, clock_out_enable %d, adj_method %d, freq %d domain %d", mode->one_pps_mode, mode->clock_out_enable, mode->adj_method, mode->freq, mode->clk_domain);
        /*lint -restore */
        mesa_rc rc = VTSS_RC_OK;
        /* update clk options is only allowed if no active PTP instances are using clock_domain 0 */
        if (mode->adj_method != config_data.init_ext_clock_mode.adj_method) {
            for (instance = 0; instance < PTP_CLOCK_INSTANCES; instance++) {
                if (mode->clk_domain != config_data.conf[instance].clock_init.cfg.clock_domain) {
                    continue;
                }

                if (config_data.conf[instance].clock_init.cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE && config_data.conf[instance].clock_init.cfg.clock_domain == 0) {
                    rc = PTP_RC_ADJ_METHOD_CHANGE_NOT_ALLOWED;
                }

                // Clock domains 1,2 support only ltc adjustment.
                if (mode->clk_domain != 0 &&
                   (mode->adj_method == VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE ||
                    mode->adj_method == VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT ||
                    mode->adj_method == VTSS_APPL_PTP_PREFERRED_ADJ_COMMON)) {
                    rc = PTP_RC_ADJ_METHOD_CHANGE_NOT_ALLOWED;
                }
            }
        }
        if (rc != VTSS_RC_OK) {
            T_WG(VTSS_TRACE_GRP_MS_SERVO,"Error code: %s", error_txt(rc));
            return rc;
        }
        {
            PTP_CORE_LOCK_SCOPE();

            if (rc == VTSS_RC_OK) {
                config_data.init_ext_clock_mode = *mode;
                if (PTP_READY()) {
                    ext_clock_out_set(mode);

                }
            }
        }
        return rc;
    }
    else return PTP_ERROR_INV_PARAM;  // One or more paramters were invalid.
}

// Store RS-422 config from an instance in global config
static void ptp_rs422_conf_set(vtss_ptp_rs422_conf_t *conf)
{
    vtss_ptp_rs422_conf_t *rs422 = &config_data.init_ext_clock_mode_rs422;

    rs422->mode     = conf->mode;
    rs422->delay    = conf->delay;
    rs422->proto    = conf->proto;
    rs422->instance = conf->instance;
    rs422->port     = conf->port;
}

/* Get serval rs422 external clock configuration. */
void vtss_ext_clock_rs422_conf_get(vtss_ptp_rs422_conf_t *mode)
{
    PTP_CORE_LOCK_SCOPE();

    *mode = config_data.init_ext_clock_mode_rs422;
}

/* Get serval rs422 external clock protocol configuration. */
void vtss_ext_clock_rs422_protocol_get(ptp_rs422_protocol_t *proto)
{
    *proto = config_data.init_ext_clock_mode_rs422.proto;
}

/* Get serval default rs422 external clock configuration. */
void vtss_ext_clock_rs422_default_conf_get(vtss_ptp_rs422_conf_t *mode)
{
    mode->mode = VTSS_PTP_RS422_DISABLE;
    mode->delay = 0;
    mode->proto = VTSS_PTP_RS422_PROTOCOL_SER_POLYT;
    mode->port = 0;
    mode->instance = -1;
}

static mesa_rc sgpio_bit_set(int port, int bit, bool set)
{
    mesa_sgpio_conf_t conf;

    T_I("set SDPIO pin port %d, bit %d to %d", port, bit, set);

    // mesa_sgpio_conf_get()/mesa_sgpio_conf_set() must be called without
    // interference.
    VTSS_APPL_API_LOCK_SCOPE();

    VTSS_RC(mesa_sgpio_conf_get(NULL, 0, 0, &conf));

    conf.port_conf[port].mode[bit] = set ? MESA_SGPIO_MODE_OFF : MESA_SGPIO_MODE_ON;
    conf.port_conf[port].enabled = TRUE;
    return mesa_sgpio_conf_set(NULL, 0, 0, &conf);
}

/* Get holdover spec configuration. */
void vtss_ho_spec_conf_get(vtss_ho_spec_conf_t *spec)
{

    PTP_CORE_LOCK();
    *spec = config_data.init_ho_spec;
    PTP_CORE_UNLOCK();
}

/* Set holdover spec configuration. */
void vtss_ho_spec_conf_set(const vtss_ho_spec_conf_t *spec)
{

    PTP_CORE_LOCK();
    config_data.init_ho_spec = *spec;
    PTP_CORE_UNLOCK();
}

mesa_rc ptp_get_port_link_state(uint instance, int portnum, vtss_ptp_port_link_state_t *ds)
{
    PTP_CORE_LOCK_SCOPE();

    mesa_rc rc = VTSS_RC_ERROR;
    if (PTP_READY()) {
        ds->link_state = port_data[portnum - 1].link_state;
        ds->in_sync_state = port_data[portnum - 1].topo.port_ts_in_sync;
        ds->forw_state = port_data[portnum - 1].vlan_forw[instance];
        if (mepa_phy_ts_cap()) {
            if (port_data[portnum - 1].topo.ts_feature == VTSS_PTP_TS_PTS) {
                ds->phy_timestamper = _ptp_inst_phy_rules[instance][portnum - 1].phy_ts_port;
            } else {
                ds->phy_timestamper = FALSE;
            }
        } else {
            ds->phy_timestamper = false;
        }
        rc = VTSS_RC_OK;
    }
    return rc;
}

/*
 * Enable/disable the wireless variable tx delay feature for a port.
 */
bool ptp_port_wireless_delay_mode_set(bool enable, int portnum, uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    bool ok = false;
    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (((config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) &&
              config_data.conf[instance].clock_init.cfg.twoStepFlag) ||
            (config_data.conf[instance].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM)) {
            ok = vtss_ptp_port_wireless_delay_mode_set(ptp_global.ptpi[instance], enable, portnum);
        }
    }
    return ok;
}

/*
 * Get the Enable/disable mode for the wireless variable tx delay feature for a port.
 */
bool ptp_port_wireless_delay_mode_get(bool *enable, int portnum, uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    bool ok = false;
    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (((config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) &&
                config_data.conf[instance].clock_init.cfg.twoStepFlag) ||
                (config_data.conf[instance].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM)) {
            ok = vtss_ptp_port_wireless_delay_mode_get(ptp_global.ptpi[instance], enable, portnum);
        }
    }
    return ok;
}

/*
 * Pre notification sent from the wireless modem transmitter before the delay is changed.
 */
bool ptp_port_wireless_delay_pre_notif(int portnum, uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    bool ok = false;
    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if ((config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) &&
                config_data.conf[instance].clock_init.cfg.twoStepFlag) {
            ok = vtss_ptp_port_wireless_delay_pre_notif(ptp_global.ptpi[instance], portnum);
        } else if ((config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) &&
                 config_data.conf[instance].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM) {
            ok = true;
        } else if ((config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY) &&
                    config_data.conf[instance].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM) {
            ok = true;
        }
    }
    return ok;
}

/*
 * Set the delay configuration, sent from the wireless modem transmitter whenever the delay is changed.
 */
bool ptp_port_wireless_delay_set(const vtss_ptp_delay_cfg_t *delay_cfg, int portnum, uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    bool ok = false;
    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (((config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) &&
                config_data.conf[instance].clock_init.cfg.twoStepFlag) ||
        (config_data.conf[instance].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM)) {
            ok = vtss_ptp_port_wireless_delay_set(ptp_global.ptpi[instance], delay_cfg, portnum);
        }
    }
    return ok;
}

/*
 * Get the delay configuration.
 */
bool ptp_port_wireless_delay_get(vtss_ptp_delay_cfg_t *delay_cfg, int portnum, uint instance)
{
    PTP_CORE_LOCK_SCOPE();

    bool ok = false;
    if (PTP_READY() && (instance < PTP_CLOCK_INSTANCES)) {
        if (((config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND) &&
                config_data.conf[instance].clock_init.cfg.twoStepFlag) ||
                (config_data.conf[instance].clock_init.cfg.protocol == VTSS_APPL_PTP_PROTOCOL_OAM)) {
            ok = vtss_ptp_port_wireless_delay_get(ptp_global.ptpi[instance], delay_cfg, portnum);
        }
    }
    return ok;
}

mesa_rc vtss_appl_ptp_system_time_sync_mode_set(const vtss_appl_ptp_system_time_sync_conf_t *const conf)
{
    PTP_CORE_LOCK_SCOPE();

    /*lint -save -e568 */
    if ((conf->mode >= VTSS_APPL_PTP_SYSTEM_TIME_NO_SYNC) && (conf->mode <= VTSS_APPL_PTP_SYSTEM_TIME_SYNC_SET)) {
        /*lint -restore */
        mesa_rc rc = VTSS_RC_OK;
        u32 sys_time_default_period = 10;   // default filter period in system_time_sync mode
        u32 sys_time_default_dist = 1;      // default filter dist in system_time_sync mode
        static const vtss_appl_ptp_clock_servo_config_t sys_time_servo = {false, true, true, true, 25, 1000, 500, 1, VTSS_APPL_PTP_CLOCK_FREE, 0,0,60,300};
        static const vtss_appl_ptp_clock_slave_config_t slave_cfg = {20000, 2000, 20000};
        if (PTP_READY() && config_data.conf[conf->clockinst].clock_init.cfg.profile != VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
            switch (conf->mode) {
                case VTSS_APPL_PTP_SYSTEM_TIME_SYNC_GET:
                    if (config_data.conf[conf->clockinst].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {
                        config_data.conf[conf->clockinst].filter_params.period = sys_time_default_period;
                        config_data.conf[conf->clockinst].filter_params.dist = sys_time_default_dist;
                        config_data.conf[conf->clockinst].servo_params = sys_time_servo;
                        config_data.conf[conf->clockinst].slave_cfg = slave_cfg;
                        vtss_ptp_clock_slave_config_set(ptp_global.ptpi[conf->clockinst]->ssm.servo, &slave_cfg);
                        vtss_ptp_timer_start(&sys_time_sync_timer, SYS_TIME_SYNC_INIT_VALUE , true);
                    } else {
                        T_IG(VTSS_TRACE_GRP_SYS_TIME, "%s", ptp_error_txt(PTP_RC_CONFLICT_PTP_ENABLED));
                        rc = PTP_RC_CONFLICT_PTP_ENABLED;
                    }
                    break;
                case VTSS_APPL_PTP_SYSTEM_TIME_SYNC_SET:
#if defined(VTSS_SW_OPTION_NTP)
                    ntp_conf_t ntp_conf;
                    if ((VTSS_RC_OK == ntp_mgmt_conf_get(&ntp_conf)) && ntp_conf.mode_enabled) {
                        T_IG(VTSS_TRACE_GRP_SYS_TIME, "%s", ptp_error_txt(PTP_RC_CONFLICT_NTP_ENABLED));
                        rc = PTP_RC_CONFLICT_NTP_ENABLED;
                    }
#endif
                    if (rc == VTSS_RC_OK) {
                        vtss_ptp_timer_start(&sys_time_sync_timer, SYS_TIME_SYNC_INIT_VALUE , true);
                    }
                    break;
                default:
                    vtss_ptp_timer_stop(&sys_time_sync_timer);
                    break;
            }
        }
        if (rc == VTSS_RC_OK) {
            system_time_sync_conf = *conf;
        }
        return rc;
    }
    else return VTSS_RC_ERROR;  // One or more parameters were illegal.
}

mesa_rc vtss_appl_ptp_system_time_sync_mode_get(vtss_appl_ptp_system_time_sync_conf_t *const conf)
{
    PTP_CORE_LOCK_SCOPE();

    *conf = system_time_sync_conf;
    return VTSS_RC_OK;
}

mesa_rc ptp_clock_slave_statistics_enable(int instance, bool enable)
{
    PTP_CORE_LOCK_SCOPE();

    return vtss_ptp_clock_slave_statistics_enable(ptp_global.ptpi[instance], enable);
}

mesa_rc ptp_clock_slave_statistics_get(int instance, vtss_ptp_slave_statistics_t *statistics, bool clear)
{
    PTP_CORE_LOCK_SCOPE();

    return vtss_ptp_clock_slave_statistics_get(ptp_global.ptpi[instance], statistics, clear);
}

mesa_rc ptp_clock_path_trace_get(int instance, ptp_path_trace_t *trace)
{
    PTP_CORE_LOCK_SCOPE();
    if (instance < PTP_CLOCK_INSTANCES) {
        memcpy(trace, &ptp_global.ptpi[instance]->path_trace, sizeof(ptp_global.ptpi[instance]->path_trace));
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc ptp_clock_path_802_1as_status_get(int instance, vtss_ptp_clock_802_1as_bmca_t *status)
{
    PTP_CORE_LOCK_SCOPE();
    if (instance < PTP_CLOCK_INSTANCES) {
        *status = ptp_global.ptpi[instance]->bmca_802_1as;
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc ptp_port_path_802_1as_status_get(int instance, mesa_port_no_t port_no, vtss_ptp_port_802_1as_bmca_t *status)
{
    PTP_CORE_LOCK_SCOPE();
    if (instance < PTP_CLOCK_INSTANCES && port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        *status = ptp_global.ptpi[instance]->ptpPort[port_no].bmca_802_1as;
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

u32 ptp_instance_2_timing_domain(int instance)
{
    if (instance < PTP_CLOCK_INSTANCES) {
        return config_data.conf[instance].clock_init.cfg.clock_domain;
    } else {
        return 0;
    }
}

// Convert error code to text
// In : rc - error return code
const char *ptp_error_txt(mesa_rc rc)
{
    switch (rc)
    {
    case PTP_ERROR_INV_PARAM:                       return "Invalid parameter error returned from PTP";
    case PTP_RC_INVALID_PORT_NUMBER:                return "Invalid port number";
    case PTP_RC_INTERNAL_PORT_NOT_ALLOWED:          return "Enabling internal mode is only valid for a Transparent clock";
    case PTP_RC_MISSING_PHY_TIMESTAMP_RESOURCE:     return "No timestamp engine is available in the PHY";
    case PTP_RC_MISSING_IP_ADDRESS:                 return "cannot set up unicast ACL before my ip address is defined";
    case PTP_RC_UNSUPPORTED_ACL_FRAME_TYPE:         return "unsupported ACL frame type";
    case PTP_RC_UNSUPPORTED_PTP_ENCAPSULATION_TYPE: return "unsupported PTP ancapsulation type";
    case PTP_RC_UNSUPPORTED_1PPS_OPERATION_MODE:    return "unsupported 1PPS operation mode";
    case PTP_RC_CONFLICT_NTP_ENABLED:               return "cannot set system time if ntp is enabled";
    case PTP_RC_CONFLICT_PTP_ENABLED:               return "cannot get system time if ptp BC/Slave is enabled";
    case PTP_RC_MULTIPLE_SLAVES:                    return "cannot create multiple slave clocks within the same clock domain";
    case PTP_RC_MULTIPLE_TC:                        return "cannot create multiple transparent clocks";
    case PTP_RC_CLOCK_DOMAIN_CONFLICT:              return "Clock domain conflict: a port can only be active in one clock domain";
    case PTP_RC_MISSING_ACL_RESOURCES:              return "Cannot add ACL resource";
    case PTP_RC_ADJ_METHOD_CHANGE_NOT_ALLOWED:      return "Cannot change preferred adj method if active PTP instances are using clock domain 0";

    default:                                        return "Unknown error returned from PTP";

    }
}

const char *sync_src_type_2_txt(vtss_ptp_synce_src_type_t s)
{
    switch (s) {
        case VTSS_PTP_SYNCE_NONE: return "NONE";
        case VTSS_PTP_SYNCE_ELEC: return "ELEC";
        case VTSS_PTP_SYNCE_PAC : return "PAC";
        default: return "UNKNOWN";
    }
}

static vtss_ptp_synce_src_t current_src = {VTSS_PTP_SYNCE_NONE, 0};

mesa_rc ptp_set_selected_src(vtss_ptp_synce_src_t *src)
{
    PTP_CORE_LOCK_SCOPE();
    if (src->type != current_src.type || src->ref != current_src.ref) {
        current_src = *src;
        T_IG(VTSS_TRACE_GRP_MS_SERVO, "current_src changed to type %s, ref %d", sync_src_type_2_txt(current_src.type), current_src.ref);
    }
    return VTSS_RC_OK;
}

static mesa_rc get_selected_src(vtss_ptp_synce_src_t *src)
{
    *src = current_src;
    T_NG(VTSS_TRACE_GRP_MS_SERVO, "current_src type %s, ref %d", sync_src_type_2_txt(current_src.type), current_src.ref);
    return VTSS_RC_OK;
}

mesa_rc ptp_get_selected_src(vtss_ptp_synce_src_t *src)
{
    PTP_CORE_LOCK_SCOPE();
    return get_selected_src(src);
}

const char *sync_servo_mode_2_txt(vtss_ptp_servo_mode_t s)
{
    switch (s) {
        case VTSS_PTP_SERVO_NONE     : return "NONE";
        case VTSS_PTP_SERVO_HYBRID   : return "HYBRID";
        case VTSS_PTP_SERVO_ELEC     : return "ELEC";
        case VTSS_PTP_SERVO_PAC      : return "PACKET";
        case VTSS_PTP_SERVO_HOLDOVER : return "HOLDOVER";
        default: return "UNKNOWN";
    }
}

mesa_rc vtss_ptp_get_servo_mode_ref(int inst, vtss_ptp_servo_mode_ref_t *mode_ref)
{
    if (inst >= 0 && inst < PTP_CLOCK_INSTANCES) {
        *mode_ref = current_servo_mode_ref[inst];
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

static bool vtss_ptp_aci_sync_filter_type_get(int instance)
{
    u32 ft = ptp_global.ptpi[instance]->clock_init->cfg.filter_type;
    bool rv = (ft == PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_PHASE ||
                ft == PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE ||
                ft == PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE ||
                ft == PTP_FILTERTYPE_ACI_BASIC_PHASE ||
                ft == PTP_FILTERTYPE_ACI_BASIC_PHASE_LOW);
    T_NG(VTSS_TRACE_GRP_MS_SERVO, "instance %d sync_filter type %s", instance, rv ? "true" : "false");
    return rv;
}

static mesa_rc vtss_ptp_update_selected_src(void)
{
    mesa_rc rc = VTSS_RC_OK;
    int instance;
    vtss_ptp_synce_src_t new_src;
    vtss_ptp_servo_mode_t servo_mode;
    int active_ref;
    bool instance_deleted = false;
    PTP_RC(get_selected_src(&new_src));
    for (instance = 0; instance < PTP_CLOCK_INSTANCES; instance++) {
        servo_mode = current_servo_mode_ref[instance].mode;
        active_ref = -1;
        if (ptp_global.ptpi[instance] != 0) {
            int clock_option = vtss_ptp_adjustment_method(instance);
            if (ptp_global.ptpi[instance]->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_ORD_BOUND ||
                ptp_global.ptpi[instance]->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_AED_GM ||
                ptp_global.ptpi[instance]->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_SLAVE_ONLY ||
                ptp_global.ptpi[instance]->clock_init->cfg.deviceType == VTSS_APPL_PTP_DEVICE_MASTER_ONLY) {
                //'active_domain' data removed in 2023.12 release.
                if (ptp_global.ptpi[instance]->slavePort != 0) {
                    active_ref = instance;
                }
                if (ptp_global.ptpi[instance]->clock_init->cfg.clock_domain == 0) {
                    vtss_appl_ptp_profile_t profile = ptp_global.ptpi[instance]->clock_init->cfg.profile;
                    switch (new_src.type) {
                        case VTSS_PTP_SYNCE_NONE:
                            if (clock_option == CLOCK_OPTION_SYNCE_DPLL) {
                                // Put PTP into packet mode unless PTP is using the 8265.1 profile. In that case PTP has to be selected by SyncE and that is covered by the VTSS_PTP_SYNCE_PAC case below.
                                if (ptp_global.ptpi[instance]->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_G_8265_1) {
                                    if (ptp_global.ptpi[instance]->slavePort) {
                                        servo_mode = VTSS_PTP_SERVO_PAC;
                                    }
                                } else {
                                    // Servo shall be put into holdover mode (or freerun mode, if holdover is not possible).
                                    if (current_servo_mode_ref[instance].mode == VTSS_PTP_SERVO_PAC || current_servo_mode_ref[instance].mode == VTSS_PTP_SERVO_HYBRID) {
                                        servo_mode = VTSS_PTP_SERVO_HOLDOVER;
                                    } else if (current_servo_mode_ref[instance].mode == VTSS_PTP_SERVO_HOLDOVER) {
                                        servo_mode = VTSS_PTP_SERVO_HOLDOVER;
                                    } else if (current_servo_mode_ref[instance].mode == VTSS_PTP_SERVO_ELEC) {
                                        servo_mode = VTSS_PTP_SERVO_ELEC;
                                    }
                                }
                            } else {
                                // If slave port exists, set servo in packet mode or else holdover
                                if (ptp_global.ptpi[instance]->slavePort) {
                                    servo_mode = VTSS_PTP_SERVO_PAC;
                                }
                            }
                            T_NG(VTSS_TRACE_GRP_MS_SERVO, "servo mode is %s, ref %d", sync_servo_mode_2_txt(servo_mode), active_ref);
                            break;
                        case VTSS_PTP_SYNCE_ELEC:
                            if (clock_option == CLOCK_OPTION_SYNCE_DPLL) {
                                bool aci_sync_mode = vtss_ptp_aci_sync_filter_type_get(instance);
                                if (profile == VTSS_APPL_PTP_PROFILE_G_8265_1 || !aci_sync_mode) {
                                    //put PTP into Electrical mode
                                    servo_mode = VTSS_PTP_SERVO_ELEC;
                                    active_ref = new_src.ref;
                                } else {
                                    //put PTP into Hybrid mode
                                    servo_mode = VTSS_PTP_SERVO_HYBRID;
                                }
                            } else {
                                // Always in packet mode
                                servo_mode = VTSS_PTP_SERVO_PAC;
                            }
                            T_NG(VTSS_TRACE_GRP_MS_SERVO, "servo mode is %s, ref %d", sync_servo_mode_2_txt(servo_mode), active_ref);
                            break;
                        case VTSS_PTP_SYNCE_PAC:
                            if (clock_option == CLOCK_OPTION_SYNCE_DPLL) {
                                if (ptp_global.ptpi[instance]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8265_1) {
                                    //put PTP into Packet mode
                                    // Set active ref
                                    servo_mode = VTSS_PTP_SERVO_PAC;
                                    active_ref = new_src.ref;
                                } else {
                                    //put PTP into Packet mode
                                    servo_mode = VTSS_PTP_SERVO_PAC;
                                }
                            } else {
                                if (ptp_global.ptpi[instance]->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_G_8265_1) {
                                    //put PTP into Packet mode
                                    servo_mode = VTSS_PTP_SERVO_PAC;
                                    active_ref = new_src.ref;
                                    T_I("Instance %d unsupported configuration", instance);
                                } else {
                                    //put PTP into Packet mode
                                    servo_mode = VTSS_PTP_SERVO_PAC;
                                }
                            }
                            T_NG(VTSS_TRACE_GRP_MS_SERVO, "instance %d, servo mode is %s, ref %d", instance, sync_servo_mode_2_txt(servo_mode), active_ref);
                            break;
                        default:
                            T_WG(VTSS_TRACE_GRP_MS_SERVO, "Instance %d Unknown src type", instance);
                            break;
                    }
                } else {
                    // domain != 0, i.e. no synce dependency
                    servo_mode = VTSS_PTP_SERVO_PAC;
                    T_NG(VTSS_TRACE_GRP_MS_SERVO, "servo mode is %s, ref %d", sync_servo_mode_2_txt(servo_mode), active_ref);
                }
            }

            // update servo state
            if (servo_mode != current_servo_mode_ref[instance].mode) {
                T_IG(VTSS_TRACE_GRP_MS_SERVO, "instance %d, servo mode changed to %s mode, ref %d", instance, sync_servo_mode_2_txt(servo_mode), active_ref);
                switch (servo_mode) {
                    case VTSS_PTP_SERVO_HYBRID:
#ifdef SW_OPTION_SYNCE
                        clock_nco_assist_set(true);
#endif                        
                        hybrid_inst = instance;
                        hybrid_switch_timer.user_data = &hybrid_inst;
                        (void)vtss_timer_start(&hybrid_switch_timer);
                        break;
                    case VTSS_PTP_SERVO_ELEC:
                        vtss_ptp_set_active_electrical_ref(active_ref);
                        T_IG(VTSS_TRACE_GRP_MS_SERVO, "change servo mode to 'electrical mode'.");
                        break;
                    case VTSS_PTP_SERVO_PAC:
                        BOOL enable;
                        vtss_timer_cancel(&hybrid_switch_timer);
                        if (vtss_ptp_force_holdover_get(instance, &enable) == VTSS_RC_OK) {
                            if (enable == TRUE) {
                                if (vtss_ptp_force_holdover_set(instance, FALSE) == VTSS_RC_OK) {
                                    T_IG(VTSS_TRACE_GRP_MS_SERVO, "Servo mode 'holdover' cleared.");
                                } else {
                                    T_WG(VTSS_TRACE_GRP_MS_SERVO, "Could not clear servo mode 'holdover'.");
                                }
                            }
                        } else {
                            T_WG(VTSS_TRACE_GRP_MS_SERVO, "Could not get servo mode 'holdover' state.");
                        }
                        if (instance == active_ref) {
                            (void)vtss_ptp_set_active_ref(active_ref);
                        }
                        if (vtss_ptp_switch_to_packet_mode(instance) == VTSS_RC_OK) {
                            if ((clock_option == CLOCK_OPTION_SYNCE_DPLL) || (clock_option == CLOCK_OPTION_PTP_DPLL)) {
                                vtss_tod_set_adjtimer(0, 0LL);  // Make sure the frequency offset of the LTC is 0 so that it does not cause the phase to drift
                            }
                            T_IG(VTSS_TRACE_GRP_MS_SERVO, "Servo mode changed to 'packet mode'.");
                        } else {
                            T_WG(VTSS_TRACE_GRP_MS_SERVO, "Could not change servo mode to 'packet mode'.");
                        }
                        break;
                    case VTSS_PTP_SERVO_HOLDOVER:
                        if (vtss_ptp_force_holdover_set(instance, TRUE) == VTSS_RC_OK) {
                            T_IG(VTSS_TRACE_GRP_MS_SERVO, "Servo mode 'holdover' set.");
                        } else {
                            T_WG(VTSS_TRACE_GRP_MS_SERVO, "Could not set servo mode 'holdover'.");
                        }
                        break;
                    default:
                        instance_deleted = true;
                        T_IG(VTSS_TRACE_GRP_MS_SERVO, "Instance %d is not active", instance);
                        break;
                }
            } else if (active_ref != current_servo_mode_ref[instance].ref) {
                //Servo mode cot changed, but reference is changed
                T_IG(VTSS_TRACE_GRP_MS_SERVO, "instance %d, servo mode not changed %s mode, but ref changed to %d", instance, sync_servo_mode_2_txt(servo_mode), active_ref);
                switch (servo_mode) {
                    case VTSS_PTP_SERVO_HYBRID:
                    case VTSS_PTP_SERVO_PAC:
                        if (instance == active_ref) {
                            (void)vtss_ptp_set_active_ref(active_ref);
                        }
                        break;
                    case VTSS_PTP_SERVO_ELEC:
                        vtss_ptp_set_active_electrical_ref(active_ref);
                        break;
                    default:
                        T_IG(VTSS_TRACE_GRP_MS_SERVO, "Instance %d is not active", instance);
                        break;
                }

            }
            current_servo_mode_ref[instance].mode = servo_mode;
            current_servo_mode_ref[instance].ref = active_ref;
            T_DG(VTSS_TRACE_GRP_MS_SERVO, "instance %d, servo mode %s ref %d", instance, sync_servo_mode_2_txt(servo_mode), active_ref);
        }
    }
    if (instance_deleted) {
        bool change = true;
        //If no PTP instances uses the Servo, electrical mode must be set
        for (instance = 0; instance < PTP_CLOCK_INSTANCES; instance++) {
            T_DG(VTSS_TRACE_GRP_MS_SERVO, "Instance %d servo_mode %s", instance, sync_servo_mode_2_txt(current_servo_mode_ref[instance].mode));
            if (current_servo_mode_ref[instance].mode != VTSS_PTP_SERVO_NONE) {
                change = false;
            }
        }
        if (change) {
            T_IG(VTSS_TRACE_GRP_MS_SERVO, "No Instance use the servo");
            vtss_ptp_set_active_electrical_ref(new_src.ref);
        }
    }
    return rc;
}

mesa_rc vtss_ptp_switch_to_packet_mode(int instance)
{
    mesa_rc rc = VTSS_RC_ERROR;

    T_IG(VTSS_TRACE_GRP_MS_SERVO, "Instance %d", instance);
    if (instance >= 0 && instance < PTP_CLOCK_INSTANCES) {
        if (ptp_global.ptpi[instance] != 0) {
#if defined(VTSS_SW_OPTION_SYNCE)
            if (config_data.conf[instance].clock_init.cfg.clock_domain == 0) {
                if (vtss_synce_ptp_clock_hybrid_mode_set(FALSE) != VTSS_RC_OK) {
                    T_W("vtss_synce_ptp_clock_hybrid_mode_set(FALSE) returned error.");
                }
            }
#endif
            rc = ptp_global.ptpi[instance]->ssm.servo->switch_to_packet_mode(config_data.conf[instance].clock_init.cfg.clock_domain);
        }
    }

    return rc;
}

mesa_rc vtss_ptp_force_holdover_set(int instance, BOOL enable)
{
    if (instance >= 0 && instance < PTP_CLOCK_INSTANCES) {
        if (ptp_global.ptpi[instance] != 0) {
            mesa_rc rc = ptp_global.ptpi[instance]->ssm.servo->force_holdover_set(enable);
            return rc;
        } else {
            return VTSS_RC_ERROR;
        }
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc vtss_ptp_force_holdover_get(int instance, BOOL *enable)
{
    if (instance >= 0 && instance < PTP_CLOCK_INSTANCES) {
        if (ptp_global.ptpi[instance] != 0) {
            mesa_rc rc = ptp_global.ptpi[instance]->ssm.servo->force_holdover_get(enable);
            return rc;
        } else {
            return VTSS_RC_ERROR;
        }
    } else {
        return VTSS_RC_ERROR;
    }
}

static void hybrid_switch_timer_expired(vtss::Timer *timer)
{
    uint32_t instance = *((uint32_t *)timer->user_data);
    PTP_CORE_LOCK_SCOPE();
    if (vtss_ptp_switch_to_hybrid_mode(instance) == VTSS_RC_OK) {
        int clock_option = vtss_ptp_adjustment_method(instance);
        if (clock_option == CLOCK_OPTION_SYNCE_DPLL) {
            mesa_ts_domain_adjtimer_set(NULL, 0, 0);  // Make sure the frequency offset of the LTC is 0 so that it does not cause the phase to drift
        }
        T_IG(VTSS_TRACE_GRP_MS_SERVO, "Servo mode changed to 'hybrid mode'.");
    } else {
        T_WG(VTSS_TRACE_GRP_MS_SERVO, "Could not change servo mode to 'hybrid mode'.");
    }

}

static void hybrid_switch_timer_init(void) {
    hybrid_switch_timer.set_repeat(FALSE);
    hybrid_switch_timer.set_period(vtss::seconds(80));
    hybrid_switch_timer.callback    = hybrid_switch_timer_expired;
    hybrid_switch_timer.modid       = VTSS_MODULE_ID_PTP;
    T_IG(VTSS_TRACE_GRP_CLOCK, "hybrid_switch_timer initialized");
}

static void transient_timer_expired(vtss::Timer *timer)
{
    PTP_CORE_LOCK_SCOPE();
    T_IG(VTSS_TRACE_GRP_MS_SERVO, "short term transient considered complete after 15 seconds.");
    short_transient = false;
}

static void transient_timer_init(void) {
    transient_timer.set_repeat(FALSE);
    //15 seconds is short term transient time from 8273.2 annex B. For moving from aquiring to locked state in zl30772, synce is taking 10 seconds.
    //So total time for short term transient is 15 + 10 = 25 seconds.
    transient_timer.set_period(vtss::seconds(25));
    transient_timer.callback    = transient_timer_expired;
    transient_timer.modid       = VTSS_MODULE_ID_PTP;
    T_IG(VTSS_TRACE_GRP_CLOCK, "transient_timer initialized");
}

mesa_rc vtss_ptp_switch_to_hybrid_mode(int instance)
{
    mesa_rc rc = VTSS_RC_ERROR;

    T_IG(VTSS_TRACE_GRP_MS_SERVO, "Instance %d", instance);
    if (instance >= 0 && instance < PTP_CLOCK_INSTANCES) {
        if (ptp_global.ptpi[instance] != 0) {
#if defined(VTSS_SW_OPTION_SYNCE)
            if (config_data.conf[instance].clock_init.cfg.clock_domain == 0) {
                if (vtss_synce_ptp_clock_hybrid_mode_set(TRUE) != VTSS_RC_OK) {
                    T_W("vtss_synce_ptp_clock_hybrid_mode_set(TRUE) returned error.");
                }
            }
#endif
            rc = ptp_global.ptpi[instance]->ssm.servo->switch_to_hybrid_mode(config_data.conf[instance].clock_init.cfg.clock_domain);
        }
    }

    return rc;
}

mesa_rc vtss_ptp_get_hybrid_mode_state(int instance, bool *hybrid_mode)
{
    if (instance >= 0 && instance < PTP_CLOCK_INSTANCES) {
        if (ptp_global.ptpi[instance] != 0) {
            *hybrid_mode = ptp_global.ptpi[instance]->ssm.servo->mode_is_hybrid_mode(config_data.conf[instance].clock_init.cfg.clock_domain);
            return VTSS_RC_OK;
        } else {
            return VTSS_RC_ERROR;
        }
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc vtss_ptp_set_active_ref(int stream)
{
    mesa_rc rc = VTSS_RC_ERROR;

    if (stream >= 0 && stream < vtss_appl_cap_ptp_clock_cnt) {
        T_IG(VTSS_TRACE_GRP_MS_SERVO, "Instance %d, domain %d", stream, config_data.conf[stream].clock_init.cfg.clock_domain);
        if (ptp_global.ptpi[stream] != 0) {
            rc = ptp_global.ptpi[stream]->ssm.servo->set_active_ref(config_data.conf[stream].clock_init.cfg.clock_domain, stream);
        }
    }
    return rc;
}

mesa_rc vtss_ptp_set_active_electrical_ref(int input)
{
    mesa_rc rc = VTSS_RC_ERROR;

    T_IG(VTSS_TRACE_GRP_MS_SERVO, "input %d", input);
#if defined(VTSS_SW_OPTION_SYNCE)
#if defined(VTSS_SW_OPTION_ZLS30387)
    // electrical references are only relevant for clock domain 0
    if ((rc = zl30380_apr_set_active_elec_ref(0, input)) != VTSS_RC_OK) {
        T_WG(VTSS_TRACE_GRP_MS_SERVO, "Could not set electrical reference mode");
    } else {
        T_IG(VTSS_TRACE_GRP_MS_SERVO, "APR set electrical reference mode");
    }
#endif //VTSS_SW_OPTION_ZLS30387
    rc = vtss_synce_ptp_clock_hybrid_mode_set(FALSE);
#endif
    return rc;
}

// In hybrid mode, if transient_quick is received then 15 seconds is the maximum time that short term transient state is considered active.
// In long term transient state, for switching to hybrid mode from packet mode, currently waiting time is around 80 seconds to ensure NCO assist dpll attains same frequency as synce.
mesa_rc vtss_ptp_set_hybrid_transient(vtss_ptp_hybrid_transient state)
{
    int instance = -1;

    PTP_CORE_LOCK_SCOPE();
    if ((short_transient == false && state == VTSS_PTP_HYBRID_TRANSIENT_QUICK) ||
        (short_transient == true  && state == VTSS_PTP_HYBRID_TRANSIENT_NOT_ACTIVE)) {
        // find the instance whose clock domain is 0 and has a slv port.
        // only clock domain 0 is used by dpll and hence synce influence.
        for (int inst = 0; inst < PTP_CLOCK_INSTANCES; inst++) {
            if (config_data.conf[inst].clock_init.cfg.clock_domain == 0 &&
                ptp_global.ptpi[inst]->slavePort) {
                instance = inst;
                break;
            }
        }
        // Not useful in switching between hybrid or packet modes if ptp does not have any synchronising port.
        if ((instance < 0) || !ptp_global.ptpi[instance]->slavePort ||
            !port_data[uport2iport(ptp_global.ptpi[instance]->slavePort)].link_state) {
            return VTSS_RC_OK;
        }

        T_IG(VTSS_TRACE_GRP_MS_SERVO, "new transient state %d", state);
        if (state == VTSS_PTP_HYBRID_TRANSIENT_QUICK) {
            T_IG(VTSS_TRACE_GRP_MS_SERVO, "starting transient timer");
            (void)vtss_timer_start(&transient_timer);
            short_transient = true;
        } else {
            T_IG(VTSS_TRACE_GRP_MS_SERVO, "cancelling transient timer");
            vtss_timer_cancel(&transient_timer);
            short_transient = false;
        }
        return ptp_global.ptpi[instance]->ssm.servo->set_hybrid_transient(config_data.conf[instance].clock_init.cfg.clock_domain, state);
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_ptp_set_1pps_virtual_reference(int inst, bool enable, bool warm_start)
{
    return ptp_global.ptpi[inst]->ssm.servo->switch_1pps_virtual_ref(ptp_global.ptpi[inst], inst, config_data.conf[inst].clock_init.cfg.clock_domain, enable, warm_start);
}

static void vtss_ptp_process_non_active_virtual_port(int instance, vtss_ptp_timestamps_t *ts)
{
    ptp_global.ptpi[instance]->ssm.servo->process_alternate_timestamp(ptp_global.ptpi[instance], config_data.conf[instance].clock_init.cfg.clock_domain, instance, &ts->tx_ts, &ts->rx_ts, ts->corr, 0, true);
}

mesa_rc vtss_ptp_set_servo_device_1pps_virtual_port(int inst, bool enable)
{
    return ptp_global.ptpi[inst]->ssm.servo->set_device_1pps_virtual_ref(ptp_global.ptpi[inst], inst, config_data.conf[inst].clock_init.cfg.clock_domain, enable);
}

bool vtss_ptp_servo_get_holdover_status(int inst)
{
    PTP_CORE_LOCK_SCOPE();
    return ptp_global.ptpi[inst]->ssm.servo->holdover_ok;
}

#if defined(VTSS_SW_OPTION_SYNCE)
void vtss_ptp_ptsf_state_set(const int instance) {
    mesa_rc rc;
    vtss_appl_synce_ptp_ptsf_state_t ptsf_state;
    T_N("vtss_ptp_ptsf_state_set %u ", instance);
    if (ptp_global.ptpi[instance] != 0) {
        if (ptp_global.ptpi[instance]->ssm.ptsf_loss_of_announce) {
            ptsf_state = SYNCE_PTSF_LOSS_OF_ANNOUNCE;
        } else if (ptp_global.ptpi[instance]->ssm.ptsf_loss_of_sync) {
            ptsf_state = SYNCE_PTSF_LOSS_OF_SYNC;
        } else if (ptp_global.ptpi[instance]->ssm.ptsf_unusable) {
            ptsf_state = SYNCE_PTSF_UNUSABLE;
        } else {
            ptsf_state = SYNCE_PTSF_NONE;
        }
        if (ptsf_state != ptp_global.ptpi[instance]->ptsf_state) {
            // only send to SyncE if changed
            T_D("instance %d vtss_ptp_ptsf_state_set %s", instance, vtss_sync_ptsf_state_2_txt(ptsf_state));
            rc = vtss_synce_ptp_clock_ptsf_state_set(instance, ptsf_state);
            ptp_global.ptpi[instance]->ptsf_state = ptsf_state;
            if (rc != VTSS_RC_OK) T_W("vtss_synce_ptp_clock_ptsf_state_set returned error code %s", error_txt(rc));
        }
    } else {
        T_W("vtss_ptp_ptsf_state_set not ready %u ", instance);
    }
    u8 clock_class;
    if (ptp_global.ptpi[instance]->slavePort != 0)
        clock_class = ptp_global.ptpi[instance]->parentDS.grandmasterClockQuality.clockClass;
    else {
        clock_class = 255;
    }
    (void) vtss_synce_ptp_clock_ssm_ql_set(ptp_global.ptpi[instance]->localClockId, clock_class);
    ptp_global.ptpi[instance]->synce_clock_class = clock_class;
    return;
}
#endif
/* Return capabilities stored locally. */
uint32_t ptp_meba_cap_synce_dpll_mode_dual()
{
    return meba_cap_synce_dpll_mode_dual;
}
uint32_t ptp_meba_cap_synce_dpll_mode_single()
{
    return meba_cap_synce_dpll_mode_single;
}
uint32_t ptp_mesa_cap_ts_separate_domain()
{
    return (mesa_cap_ts_domain_cnt > 1) ? 1 : 0;
}
bool ptp_cap_sub_nano_second() {
    return vtss_appl_ptp_sub_nano_second;
}

/****************************************************************************/
/*                                                                          */
/*  Virtual port start.                                                     */
/*                                                                          */
/****************************************************************************/

static vtss_appl_ptp_ext_io_pin_cfg_t io_pin[MAX_VTSS_TS_IO_ARRAY_SIZE] = {VTSS_APPL_PTP_IO_MODE_ONE_PPS_DISABLE, VTSS_APPL_PTP_IO_MODE_ONE_PPS_DISABLE, VTSS_APPL_PTP_IO_MODE_ONE_PPS_DISABLE, VTSS_APPL_PTP_IO_MODE_ONE_PPS_DISABLE,
                                                            VTSS_APPL_PTP_IO_MODE_ONE_PPS_DISABLE, VTSS_APPL_PTP_IO_MODE_ONE_PPS_DISABLE, VTSS_APPL_PTP_IO_MODE_ONE_PPS_DISABLE, VTSS_APPL_PTP_IO_MODE_ONE_PPS_DISABLE};
static u32 io_pin_domain[MAX_VTSS_TS_IO_ARRAY_SIZE] = {0,0,0,0,0,0,0,0};
// the first two pins are used by the PTP application
static ptp_io_pin_usage_t io_pin_default_usage[MAX_VTSS_TS_IO_ARRAY_SIZE] = {PTP_IO_PIN_USAGE_NONE, PTP_IO_PIN_USAGE_NONE, PTP_IO_PIN_USAGE_NONE, PTP_IO_PIN_USAGE_NONE, PTP_IO_PIN_USAGE_NONE, PTP_IO_PIN_USAGE_NONE, PTP_IO_PIN_USAGE_NONE, PTP_IO_PIN_USAGE_NONE};

static u32 source_id_to_pin(meba_event_t source_id)
{
    u32 i;
    for (i = 0; i < mesa_cap_ts_vir_io_cnt; i++) {
        if (ptp_io_pin[i].source_id == source_id) {
            return i;
        }
    }
    T_WG(VTSS_TRACE_GRP_1_PPS, "unknown source_id %d", source_id);
    return 0;
}

static mesa_rc ptp_pin_init(void)
{
    uint32_t pin_idx;
    mesa_ts_ext_io_mode_t io_mode;
    mesa_rc rc = VTSS_RC_OK;
    T_I("initialize IO PIN mode");
    for (pin_idx = 0; pin_idx < mesa_cap_ts_vir_io_cnt; pin_idx++) {
        ptp_io_pin[pin_idx].io_pin.pin = io_pin[pin_idx];
        ptp_io_pin[pin_idx].io_pin.domain = io_pin_domain[pin_idx];
        ptp_io_pin[pin_idx].io_pin.freq = 1;
        ptp_io_pin[pin_idx].ptp_inst = -1;
        ptp_io_pin[pin_idx].usage = io_pin_default_usage[pin_idx];
        meba_ptp_external_io_conf_get(board_instance, pin_idx, &(ptp_io_pin[pin_idx].board_assignments), &(ptp_io_pin[pin_idx].source_id));
        T_I("IO PIN %d init", pin_idx);
        if (VTSS_IS_PHY_SYNC(ptp_io_pin[pin_idx].board_assignments)) {
            io_mode.freq = 1;
            io_mode.domain = 0;
            io_mode.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_OUTPUT;
            rc = mesa_ts_external_io_mode_set(NULL, pin_idx, &io_mode);
        }
    }
    return rc;
}

static mesa_rc io_pin_instance_set(int instance, u32 io_pin, BOOL enable)
{
    mesa_rc rc = VTSS_RC_OK;
    if (instance < PTP_CLOCK_INSTANCES && io_pin < mesa_cap_ts_io_cnt) {
        T_I("enable %d, instance %d, io_pin %d", enable, instance, io_pin);
        if (enable) {
            ptp_io_pin[io_pin].ptp_inst = instance;
        } else if (ptp_io_pin[io_pin].ptp_inst == instance) {
            ptp_io_pin[io_pin].ptp_inst = -1;
        }
    } else {
        T_W("Invalid instance %d or io_pin %d", instance, io_pin);
        rc = VTSS_RC_ERROR;
    }
    return rc;
}

/*
 * IO pin interrupt handler
 */
static void io_pin_main_auto_handler(meba_event_t     source_id,
                                     u32                         instance_id)
{
    mesa_timestamp_t                ts;
    u64                             tc;
    u32 io_pin = source_id_to_pin(source_id);
    uint inst = ptp_io_pin[io_pin].ptp_inst;
    if (inst > vtss_appl_cap_ptp_clock_cnt) {
        T_WG(VTSS_TRACE_GRP_1_PPS, "Invalid ptp instance %u", inst);
        return;
    }        
    PTP_CORE_LOCK_SCOPE();
    vtss_appl_ptp_virtual_port_config_t *current_vp_cfg = &config_data.conf[inst].virtual_port_cfg;
    
    T_DG(VTSS_TRACE_GRP_1_PPS, "I/O pin event: source_id %d, instance_id %u", source_id, instance_id);
    T_DG(VTSS_TRACE_GRP_1_PPS, "I/O pin: %u, pin mode %d", io_pin, ptp_io_pin[io_pin].usage);
    if (current_vp_cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO) {
        u32 turnaround_latency;

        PTP_RC(mesa_ts_saved_timeofday_get(NULL, io_pin, &ts, &tc));
        turnaround_latency = ts.nanoseconds;
        T_IG(VTSS_TRACE_GRP_1_PPS, "Saved turn around or onesec counter value: %u", turnaround_latency);
        // save calculated cable delay
        config_data.init_ext_clock_mode_rs422.delay = turnaround_latency/2 + PTP_VIRTUAL_PORT_SUB_DELAY; /* 19 ns is the additional delay in the path to the SUB module */
        // wait 40 msec before sending timing message
        if (vtss_timer_start(&pps_msg_timer) != VTSS_RC_OK) {
            T_EG(VTSS_TRACE_GRP_CLOCK, "Unable to start pps_msg_timer");
        }
        PTP_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PTP, io_pin_main_auto_handler, source_id, INTERRUPT_PRIORITY_NORMAL));
    } else {
        T_WG(VTSS_TRACE_GRP_1_PPS, "Instance %d Not in auto mode", inst);
    }
}

/*
 * TOD Synchronization 1 pps pulse update handler
 */
static void io_pin_pps_out_handler(meba_event_t source_id,
                                     u32 instance_id)
{    
    u32 io_pin = source_id_to_pin(source_id);
    uint inst = ptp_io_pin[io_pin].ptp_inst;
    if (inst > vtss_appl_cap_ptp_clock_cnt) {
        T_WG(VTSS_TRACE_GRP_1_PPS, "Invalid ptp instance %u", inst);
        return;
    }        
    PTP_CORE_LOCK();

    T_IG(VTSS_TRACE_GRP_1_PPS, "One sec internal clock event: source_id %d, instance_id %u", source_id, inst);
    if (config_data.conf[inst].virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN || config_data.conf[inst].virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT) {
        T_IG(VTSS_TRACE_GRP_1_PPS, "MODE_MAIN_MAN/pps-out");
        // wait 40 msec before sending timing message
        if (vtss_timer_start(&pps_msg_timer) != VTSS_RC_OK) {
            T_EG(VTSS_TRACE_GRP_CLOCK, "Unable to start pps_msg_timer");
        }
    }
    PTP_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PTP,
                                          io_pin_pps_out_handler,
                                          source_id,
                                          INTERRUPT_PRIORITY_NORMAL));
    PTP_CORE_UNLOCK();
}

/*
 * IO pin interrupt handler
 */
static void io_pin_pps_in_slave_handler(meba_event_t     source_id,
                                     u32                         instance_id)
{

    mesa_timestamp_t                ts;
    u64                             tc;
    char str1[50];
    u32 io_pin = source_id_to_pin(source_id);
    uint inst = ptp_io_pin[io_pin].ptp_inst;

    if (ptp_io_pin[io_pin].usage == PTP_IO_PIN_USAGE_NONE) {
        T_IG(VTSS_TRACE_GRP_1_PPS, "IO Pin[%d] usage is NONE", io_pin);
        return;
    }
    if (inst > vtss_appl_cap_ptp_clock_cnt) {
        T_WG(VTSS_TRACE_GRP_1_PPS, "Invalid ptp instance %u", inst);
        return;
    }
    PTP_CORE_LOCK();
    vtss_appl_ptp_virtual_port_config_t *current_vp_cfg = &config_data.conf[inst].virtual_port_cfg;
    
    T_DG(VTSS_TRACE_GRP_1_PPS, "I/O pin event: source_id %d, instance_id %u", source_id, inst);
    T_DG(VTSS_TRACE_GRP_1_PPS, "I/O pin: %u, pin mode %d", io_pin, ptp_io_pin[io_pin].io_pin.pin);
    PTP_RC(mesa_ts_saved_timeofday_get(0, io_pin, &ts, &tc));
    if (current_vp_cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB || current_vp_cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN) {
        T_IG(VTSS_TRACE_GRP_1_PPS, "inst: %u, devicetype %d, protocol %d", inst, config_data.conf[ptp_io_pin[io_pin].ptp_inst].clock_init.cfg.deviceType, config_data.conf[ptp_io_pin[io_pin].ptp_inst].clock_init.cfg.protocol);
        T_DG(VTSS_TRACE_GRP_1_PPS, "io_pin %u, time %s", io_pin, TimeStampToString (&ts, str1));
        ptp_external_input_slave_function(&ptp_virtual_port[inst].slave_data, inst, &ts, false, ((mesa_timeinterval_t)current_vp_cfg->delay) << 16);
        PTP_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PTP, io_pin_pps_in_slave_handler, source_id, INTERRUPT_PRIORITY_NORMAL));
    } else {
        T_IG(VTSS_TRACE_GRP_1_PPS, "Instance %d Not in pps input or sub mode", inst);
    }
    PTP_CORE_UNLOCK();
}

/* if mode is VTSS_PTP_RS422_SUB, then GPIO mstoen  must be set active high, and slvoen must be set low.
 * if mode is PPS, then GPIO mstoen and slvoen must be high ( use cases - refer APPL-4618, mail attachment)
 * this is done to tristate the driver connected to the TX connector feedback signal.
 * in VTSS_PTP_RS422_DISABLE mode the same setting is done  as in VTSS_PTP_RS422_SUB mode, i.e. the PPS input can also be used in VTSS_PTP_RS422_DISABLE mode 
 * If both ser and sma connected and pps is configured, t1(from tod) and t2(from pps) are used.
 */
static mesa_rc rs422_gpio_setup(vtss_appl_virtual_port_mode_t port_mode)
{
    mesa_rc rc = MESA_RC_OK;
    mesa_bool_t slave_input_disable = (port_mode != VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB) && (port_mode != VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO);
    mesa_bool_t master_input_disable = (port_mode != VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO);

    T_I("RS422 gpio mstoen %d, slvoen %d", master_input_disable, slave_input_disable);
    T_I("gpio mstoen %d, slvoen %d", rs422_conf.gpio_rs422_1588_mstoen, rs422_conf.gpio_rs422_1588_slvoen);

    //Laguna board does not follow RS422 gpio pattern of earlier boards.
    if (vtss_board_type() == VTSS_BOARD_LAN9694_PCB8398) {
        T_IG(VTSS_TRACE_GRP_1_PPS, "virtual port mode %d", port_mode);
        switch (port_mode) {
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB:
            PTP_RETURN(sgpio_bit_set(1, 2, true));
            PTP_RETURN(sgpio_bit_set(1, 3, false));
            break;

        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN:
            PTP_RETURN(sgpio_bit_set(1, 2, false));
            PTP_RETURN(sgpio_bit_set(1, 3, false));
            break;

        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN:
            PTP_RETURN(sgpio_bit_set(1, 2, false));
            PTP_RETURN(sgpio_bit_set(1, 3, true));
            break;

        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT:
            PTP_RETURN(sgpio_bit_set(1, 3, false));
            PTP_RETURN(sgpio_bit_set(1, 2, false));
            break;

        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO:
            PTP_RETURN(sgpio_bit_set(1, 3, true));
            PTP_RETURN(sgpio_bit_set(1, 2, true));
            break;

        default:
            T_I("Not expected to come here");
        }
    } else {
        if (rs422_conf.gpio_rs422_1588_mstoen != -1 && rs422_conf.gpio_rs422_1588_mstoen < 64) {
            PTP_RETURN(mesa_gpio_mode_set(NULL, 0,   rs422_conf.gpio_rs422_1588_mstoen, MESA_GPIO_OUT));
            PTP_RETURN(mesa_gpio_write(NULL, 0, rs422_conf.gpio_rs422_1588_mstoen, master_input_disable));
        } else if (rs422_conf.gpio_rs422_1588_mstoen != -1) {
            // use SGPIO pin
            PTP_RETURN(sgpio_bit_set(rs422_conf.gpio_rs422_1588_mstoen>>8,  rs422_conf.gpio_rs422_1588_mstoen &0x3, master_input_disable));
        }
        if (rs422_conf.gpio_rs422_1588_slvoen != -1 && rs422_conf.gpio_rs422_1588_slvoen < 64) {
            PTP_RETURN(mesa_gpio_mode_set(NULL, 0,   rs422_conf.gpio_rs422_1588_slvoen, MESA_GPIO_OUT));
            PTP_RETURN(mesa_gpio_write(NULL, 0, rs422_conf.gpio_rs422_1588_slvoen,slave_input_disable));
        } else if (rs422_conf.gpio_rs422_1588_slvoen != -1) {
            // use SGPIO pin
            PTP_RETURN(sgpio_bit_set(rs422_conf.gpio_rs422_1588_slvoen>>8,  rs422_conf.gpio_rs422_1588_slvoen &0x3, slave_input_disable));
        }
    }
    return rc;
}

/*
 * PTP virtual port monitor timer function
 *      check if requested interrupts occurs, and if the requested t1 time is received
 *      if the state is ok then enable simulated announce info from the virtual port based on the configured parameters
 */
/*lint -esym(459, ptp_one_pps_slave_timer) */
static void ptp_virtual_port_monitor(vtss_ptp_sys_timer_t *timer, void *m)
{
    // check if virtual port is running, and simulate announce messages.
    ptp_clock_t * ptp = (ptp_clock_t *)m;
    ptp_virtual_port_status_t *vp = &ptp_virtual_port[ptp->localClockId];
    vtss_appl_ptp_virtual_port_config_t *cfg = &config_data.conf[ptp->localClockId].virtual_port_cfg;
    PtpPort_t *ptpPort = &ptp->ptpPort[vp->slave_data.one_pps_slave_port_no];
    bool port_enabled = true;

    T_DG(VTSS_TRACE_GRP_1_PPS, "inst %d slave_port %d", ptp->localClockId, vp->slave_data.one_pps_slave_port_no);

    if (vp->slave_data.enable_t1) {
        // missing T1, virtual Port status disabled
        if (vp->slave_data.t1_to_count) {
            ++pim_tod_statistics.missed_tod_rx_cnt;
            T_WG(VTSS_TRACE_GRP_1_PPS, "inst %d, slave port %u missed tod timestamp, tod miss count[%d]",
                ptp->localClockId, vp->slave_data.one_pps_slave_port_no, pim_tod_statistics.missed_tod_rx_cnt);
        }
        if (vp->slave_data.t1_to_count++ > PTP_VIRTUAL_PORT_T1_TIMEOUT) {
            port_enabled = false;
        }
    }

    if (vp->slave_data.t2_to_count) {
            ++pim_tod_statistics.missed_one_pps_cnt;
            T_WG(VTSS_TRACE_GRP_1_PPS, "inst %d, slave port %u missed 1PPS timestamp, miss count[%d]",
                   ptp->localClockId, vp->slave_data.one_pps_slave_port_no,pim_tod_statistics.missed_one_pps_cnt);
    }
    if (vp->slave_data.t2_to_count++ > PTP_VIRTUAL_PORT_T2_TIMEOUT) {
        // missing T2, virtual Port status disabled
        port_enabled = false;
    }

    // if the state is ok then enable simulated announce info from the virtual port based on the configured parameters
    T_DG(VTSS_TRACE_GRP_1_PPS, "port_enabled %d rx_alarm %d", port_enabled, vp->slave_data.rx_alarm);
    if (port_enabled) {
        //simulate Announce
        ClockDataSet clock_ds;
        vtss_appl_ptp_clock_timeproperties_ds_t time_prop = cfg->timeproperties;

        memcpy(clock_ds.grandmasterIdentity, cfg->clock_identity, sizeof(vtss_appl_clock_identity));
        clock_ds.priority1 = cfg->priority1;
        clock_ds.clockQuality = cfg->clockQuality;
        clock_ds.priority2 = cfg->priority2;
        clock_ds.stepsRemoved = cfg->steps_removed;
        clock_ds.sourcePortIdentity = ptpPort->portDS.status.portIdentity;    /* current parent sourcePort identity */
        clock_ds.localPriority = cfg->localPriority;
        ptpPort->port_config->localPriority = cfg->localPriority;
        if (vp->slave_data.rx_alarm) {
            // If next alarm is not received by 60 seconds, set it to false.
            if (++vp->slave_data.rx_alarm_timeout >= PTP_VIRTUAL_PORT_ALARM_RX_TIMEOUT) {
                vp->slave_data.rx_alarm = FALSE;
                vp->slave_data.rx_alarm_timeout = 0;
            } else {
                time_prop.timeTraceable = false;
                time_prop.frequencyTraceable = false;
                clock_ds.clockQuality.clockAccuracy = 0xFE; // When alarm is set, accuracy is same as when not locked to GNSS.
            }
        }

        vtss_virtual_ptp_announce_rx(ptp_global.ptpi, ptp->localClockId, l2port_to_ptp_api(vp->slave_data.one_pps_slave_port_no), &clock_ds, &time_prop);
        T_DG(VTSS_TRACE_GRP_1_PPS, "inst %d, slave port %d got both 1PPS and timestamp", ptp->localClockId, vp->slave_data.one_pps_slave_port_no);
    } else {
        vp->slave_data.rx_alarm = FALSE; // reset
        vp->slave_data.rx_alarm_timeout = 0;
        // virtual port does not participate in asCapable state transitions. Hence, needed to set disabled state here.
        if (ptp->clock_init->cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS) {
            ptpPort->bmca_802_1as.infoIs = VTSS_PTP_INFOIS_DISABLED;
            if (ptpPort->portDS.status.portState != VTSS_APPL_PTP_DISABLED) {
                ptpPort->portDS.status.s_802_1as.portRole = VTSS_APPL_PTP_PORT_ROLE_DISABLED_PORT;
                vtss_ptp_state_set(VTSS_APPL_PTP_DISABLED, ptp, ptpPort);
            }
        }
    }
    PTP_RC(vtss_ptp_port_linkstate(ptp, vp->slave_data.one_pps_slave_port_no, port_enabled));

    if (cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE) {
        vtss_ptp_timer_stop(timer);
    }
}

static void ptp_virtual_port_alarm_generator(vtss_ptp_sys_timer_t *timer, void *m)
{
    // check if virtual port is running, and simulate announce messages.
    ptp_clock_t * ptp = (ptp_clock_t *)m;
    vtss_appl_ptp_virtual_port_config_t *cfg = &config_data.conf[ptp->localClockId].virtual_port_cfg;

    if (cfg->enable) {
        if (cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN ||
            cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO ||
            cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT) {
            if (cfg->proto != VTSS_PTP_APPL_RS422_PROTOCOL_PIM && cfg->proto != VTSS_PTP_APPL_RS422_PROTOCOL_NONE) {
                ptp_rs422_alarm_send(cfg->alarm);
            } else if (cfg->proto == VTSS_PTP_APPL_RS422_PROTOCOL_PIM) {
                ptp_pim_alarm_msg_send(config_data.conf[ptp->localClockId].virtual_port_cfg.portnum, cfg->alarm);
            }
        }
    }
}

// Receives alarm on RS422 interface.
void ptp_virtual_port_alarm_rx(mesa_bool_t alarm)
{
    int inst;
    ptp_virtual_port_status_t *vp;
    T_DG(VTSS_TRACE_GRP_1_PPS, "alarm received");
    PTP_CORE_LOCK();
    inst = config_data.init_ext_clock_mode_rs422.instance;
    if (inst >= 0 && inst < vtss_appl_cap_ptp_clock_cnt) {
        vp = &ptp_virtual_port[inst];
        vp->slave_data.rx_alarm = alarm;
        vp->slave_data.rx_alarm_timeout = 0;
    }
    PTP_CORE_UNLOCK();
}

static void _my_virtual_port_timestamp_rx(uint inst, const mesa_timestamp_t *ts)
{
    char str[50];

    const mesa_timeinterval_t io_pin_input_latency = (8LL<<16);
    mesa_timestamp_t my_ts;
    vtss_tod_sub_TimeStamp(&my_ts, ts, &io_pin_input_latency);
    T_DG(VTSS_TRACE_GRP_1_PPS, "got 1pps: %s from instance %u", TimeStampToString (&my_ts, str), inst);
    if (inst < vtss_appl_cap_ptp_clock_cnt) {
        ptp_external_input_slave_function(&ptp_virtual_port[inst].slave_data, inst, &my_ts, true, 0);

    } else {
        T_IG(VTSS_TRACE_GRP_1_PPS, "1pps from invalid instance %u", inst);
    }
}
static u32 ptp_1pps_pim_port_2_inst(mesa_port_no_t port_no)
{
    u32 i;
    for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
        if (config_data.conf[i].virtual_port_cfg.portnum == port_no) {
            return i;
        }
    }
    T_WG(VTSS_TRACE_GRP_1_PPS, "unknown port_no %d", port_no);
    return i;
}

// Receives timestamp from RS-422 serial port.
void ptp_virtual_port_timestamp_rx(const mesa_timestamp_t *ts)
{
    int inst;
    T_DG(VTSS_TRACE_GRP_1_PPS, "ts recv from mas");
    PTP_CORE_LOCK();
    inst = config_data.init_ext_clock_mode_rs422.instance;

    _my_virtual_port_timestamp_rx(inst, ts);
    PTP_CORE_UNLOCK();
}


static void my_virtual_port_timestamp_rx(mesa_port_no_t port_no, const mesa_timestamp_t *ts)
{
    PTP_CORE_LOCK_SCOPE();
    
    _my_virtual_port_timestamp_rx(ptp_1pps_pim_port_2_inst(port_no), ts);
}

// Receive alarm using pim protocol.
static void ptp_virtual_port_pim_alarm_rx(mesa_port_no_t port_no, mesa_bool_t alarm)
{
    int inst;
    ptp_virtual_port_status_t *vp;
    T_DG(VTSS_TRACE_GRP_1_PPS, "alarm received on pim interface");
    PTP_CORE_LOCK();
    inst = ptp_1pps_pim_port_2_inst(port_no);
    if (inst >= 0 && inst < vtss_appl_cap_ptp_clock_cnt) {
        vp = &ptp_virtual_port[inst];
        vp->slave_data.rx_alarm = alarm;
        vp->slave_data.rx_alarm_timeout = 0;
    }
    PTP_CORE_UNLOCK();
}

// return port number used by virtual port.
mesa_port_no_t ptp_get_virtual_port_number()
{
    return mesa_cap_port_cnt;
}

/* 
 * Set virtual port configuration:
 * if requested resources are available then continue otherwise return error code
 * If port mode has changed then release currently used resources (io_pins)
 * If protocol has changed, enable/disable PIM.
 * If TOD protocol requested (either serial or PIM): enable t1 handling, otherwise t1 is not used
 * Configure GPIO pins used for RS422 control (mstoen, slvoen)
 * Set up io_pin(s) for the current configurations, and enable requested interrupt handlers
 * Enable monitor function for monitoring the virtual port state 
 *      check if requested interrupts occurs, and if the requested t1 time is received
 *      if the state is ok then enable simulated announce info from the virtual port based on the configured parameters
 * 
 */
static mesa_rc ptp_clock_config_virtual_port_config_set(uint instance, const vtss_appl_ptp_virtual_port_config_t *const config)
{
    mesa_rc rc = VTSS_RC_OK;
	static bool init_done = false; //To differentiate init sequence and CLI configurations.
    vtss_appl_ptp_virtual_port_config_t *current_vp_cfg = &config_data.conf[instance].virtual_port_cfg;
    mesa_ts_ext_io_mode_t io_mode_i = {.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_DISABLE, .domain = 0, .freq = 1};
    mesa_ts_ext_io_mode_t io_mode_o = {.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_DISABLE, .domain = 0, .freq = 1};
    u16 input_pin = config->input_pin;
    u16 output_pin = config->output_pin;
    vtss_appl_ptp_profile_t profile = config_data.conf[instance].clock_init.cfg.profile;

    if (mesa_cap_misc_chip_family == MESA_CHIP_FAMILY_CARACAL) {
        if (config->virtual_port_mode != VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE) {
            T_WG(VTSS_TRACE_GRP_1_PPS, "Virtual port not supported on caracal platform");
            return VTSS_RC_ERROR;
        }
        return VTSS_RC_OK;
    }

    if (config_data.conf[instance].clock_init.cfg.deviceType == VTSS_APPL_PTP_DEVICE_INTERNAL) {
        T_WG(VTSS_TRACE_GRP_1_PPS, "virtual port cannot be configured for Internal mode");
        return VTSS_RC_ERROR;
    }

    if (profile == VTSS_APPL_PTP_PROFILE_G_8265_1) {
        T_WG(VTSS_TRACE_GRP_1_PPS, "Profile is not supported to configure virtual port");
        return VTSS_RC_ERROR;     
    }   

    if(!PTP_READY()){
        config_data.conf[instance].virtual_port_cfg = *config;
        return VTSS_RC_OK;
    }
    if (instance >= PTP_CLOCK_INSTANCES ||
       ((profile == VTSS_APPL_PTP_PROFILE_G_8275_1 || profile == VTSS_APPL_PTP_PROFILE_G_8275_2) && (config->priority1 != 128))) {
        T_WG(VTSS_TRACE_GRP_1_PPS, "Virtual port, invalid instance %d or parameter value", instance);
        return VTSS_RC_ERROR;
        
    }
    // check if requested resources are available 
    if (config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO || config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB || config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN) {
        if (rs422_conf.gpio_rs422_1588_mstoen == -1 && rs422_conf.gpio_rs422_1588_slvoen == -1) {
            T_WG(VTSS_TRACE_GRP_1_PPS, "RS422 not supported on board type: %d", vtss_board_type());
            return VTSS_RC_ERROR;
        } else {
            input_pin = rs422_conf.ptp_pin_ldst;
            output_pin = rs422_conf.ptp_pin_ppso;
        }
        if(config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO || config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB){
            if (ptp_io_pin[input_pin].usage != PTP_IO_PIN_USAGE_NONE && ptp_io_pin[input_pin].ptp_inst != instance) {
                T_WG(VTSS_TRACE_GRP_1_PPS, "Pin is already in use ");
            return VTSS_RC_ERROR;
            }
        }
        if(config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO || config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN){
            if (ptp_io_pin[output_pin].usage != PTP_IO_PIN_USAGE_NONE && ptp_io_pin[output_pin].ptp_inst != instance) {
                T_WG(VTSS_TRACE_GRP_1_PPS, "Pin is already in use ");
            return VTSS_RC_ERROR;
            }
        }
                
    } else if (config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN) {
        if (ptp_io_pin[input_pin].usage != PTP_IO_PIN_USAGE_NONE && ptp_io_pin[input_pin].ptp_inst != instance) {
            T_WG(VTSS_TRACE_GRP_1_PPS, "Pin is already in use ");
            return VTSS_RC_ERROR;
        }
    } else if (config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT || config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_FREQ_OUT) {
        if (ptp_io_pin[output_pin].usage != PTP_IO_PIN_USAGE_NONE && ptp_io_pin[output_pin].ptp_inst != instance) {
            T_WG(VTSS_TRACE_GRP_1_PPS, "Pin is already in use ");
            return VTSS_RC_ERROR;
        }
    }

    // If port mode has changed then release currently used resources (io_pins)
    if (config->virtual_port_mode != current_vp_cfg->virtual_port_mode) {
        // Stop the current alarm timer which sends alarm from master.
        if (current_vp_cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN ||
            current_vp_cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO ||
            current_vp_cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT) {
            vtss_ptp_timer_stop(&ptp_virtual_port[instance].virtual_port_alarm_timer);
        }
        // Reset the rx_alarm data
        if (current_vp_cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB ||
            current_vp_cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO ||
            current_vp_cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN) {
            ptp_virtual_port[instance].slave_data.rx_alarm = FALSE;
            ptp_virtual_port[instance].slave_data.rx_alarm_timeout = 0;
        }

        // port mode has changed, therefore release currently used resources
        switch (current_vp_cfg->virtual_port_mode) {
            case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE:
                break;
            case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO:
            case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB:
            case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN:
                // Set RS422 into disabled mode
                PTP_RC(rs422_gpio_setup(VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE));
                if (current_vp_cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB) {
                    vtss_ptp_timer_stop(&ptp_virtual_port[instance].virtual_port_monitor_timer);
                }
                // mark pins as unused
                PTP_RC(mesa_ts_external_io_mode_set(NULL, current_vp_cfg->input_pin,  &io_mode_i));
                PTP_RC(mesa_ts_external_io_mode_set(NULL, current_vp_cfg->output_pin, &io_mode_o));
                ptp_io_pin[current_vp_cfg->input_pin].usage = PTP_IO_PIN_USAGE_NONE;
                ptp_io_pin[current_vp_cfg->output_pin].usage = PTP_IO_PIN_USAGE_NONE;
                current_vp_cfg->input_pin = PTP_IO_PIN_UNUSED;
                current_vp_cfg->output_pin = PTP_IO_PIN_UNUSED;
                break;
            case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN:
            case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT:
            case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_FREQ_OUT:
                if (current_vp_cfg->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN) {
                    vtss_ptp_timer_stop(&ptp_virtual_port[instance].virtual_port_monitor_timer);
                }

                if (current_vp_cfg->input_pin != PTP_IO_PIN_UNUSED) {
                    PTP_RC(mesa_ts_external_io_mode_set(NULL, current_vp_cfg->input_pin,  &io_mode_i));
                    ptp_io_pin[current_vp_cfg->input_pin].usage = PTP_IO_PIN_USAGE_NONE;
                    current_vp_cfg->input_pin = PTP_IO_PIN_UNUSED;
                }
                if (current_vp_cfg->output_pin != PTP_IO_PIN_UNUSED) {
                    PTP_RC(mesa_ts_external_io_mode_set(NULL, current_vp_cfg->output_pin, &io_mode_o));
                    ptp_io_pin[current_vp_cfg->output_pin].usage = PTP_IO_PIN_USAGE_NONE;
                    current_vp_cfg->output_pin = PTP_IO_PIN_UNUSED;
                }
                break;
        }
        if (input_pin != PTP_IO_PIN_UNUSED) {
            ptp_io_pin[input_pin].ptp_inst = -1;
        } else if (output_pin != PTP_IO_PIN_UNUSED) {
            ptp_io_pin[output_pin].ptp_inst = -1;
        }
    }
    ptp_pim_init_t pim_ini = {};

    PTP_RC(rs422_gpio_setup(config->virtual_port_mode));
    if (config->virtual_port_mode != VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE) {
        vtss_ptp_rs422_conf_t rs422 = {};

        rs422.mode = ptp_virtual_to_rs422_mode_convert(config->virtual_port_mode);
        rs422.delay = config->delay;
        rs422.proto = ptp_rs422_proto_appl_to_internal_converter(config->proto);
        rs422.instance = instance;
        rs422.port = (config->proto == VTSS_PTP_APPL_RS422_PROTOCOL_PIM) ? config->portnum : 0;
        ptp_rs422_conf_set(&rs422);
    } else if (config_data.init_ext_clock_mode_rs422.instance == instance) {
        vtss_ptp_rs422_conf_t rs422;
        vtss_ext_clock_rs422_default_conf_get(&rs422);
        ptp_rs422_conf_set(&rs422);
    }

    //if ((ptp_io_pin[input_pin].usage == PTP_IO_PIN_USAGE_NONE || ptp_io_pin[input_pin].usage == PTP_IO_PIN_USAGE_RS422) &&
    //    (ptp_io_pin[output_pin].usage == PTP_IO_PIN_USAGE_NONE || ptp_io_pin[output_pin].usage == PTP_IO_PIN_USAGE_RS422)) {
    //    if (MESA_CAP(MESA_CAP_PHY_TS)) {
    //        mesa_timeinterval_t one_pps_latency = (mesa_timeinterval_t)8LL<<16; /* default 8 ns including GPIO delay */
    //        if (config->virtual_port_mode== VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB) {
    //            one_pps_latency += (((mesa_timeinterval_t)mode->delay)<<16); /* default one clock cycle */
    //        }
    //        PTP_RC(vtss_module_man_master_1pps_latency(one_pps_latency));
    //    }
    if (config->proto == VTSS_PTP_APPL_RS422_PROTOCOL_PIM) {
        pim_ini.co_1pps = my_virtual_port_timestamp_rx;
        pim_ini.co_delay = 0;
        pim_ini.co_pre_delay = 0;
        pim_ini.co_alarm = ptp_virtual_port_pim_alarm_rx;
        pim_ini.tg = VTSS_TRACE_GRP_PTP_PIM;
        if (!pim_active) {
            PTP_CORE_UNLOCK();      // to avoid deadlock because pim may call back to PTP
            pim_active = true;
            ptp_pim_init(&pim_ini, true);
            PTP_CORE_LOCK();        // to avoid deadlock because pim may call back to PTP
            one_pps_tod_statistics_clear();
        }
    } else {
        if (pim_active) {
            PTP_CORE_UNLOCK();      // to avoid deadlock because pim may call back to PTP
            pim_active = false;
            ptp_pim_init(&pim_ini, false);
            PTP_CORE_LOCK();        // to avoid deadlock because pim may call back to PTP
        }
    }
    // If TOD protocol requested (either serial or PIM): enable t1 handling, otherwise t1 is not used
    if (config->proto == VTSS_PTP_APPL_RS422_PROTOCOL_NONE) {
        ptp_virtual_port[instance].slave_data.enable_t1 = false;
    } else {
        ptp_virtual_port[instance].slave_data.enable_t1 = true;
    }
    // Set up io_pin(s) for the current configurations, and enable requested interrupt handlers
    ptp_virtual_port[instance].slave_data.one_pps_slave_port_no = mesa_cap_port_cnt;
    // apply default values to the virtual port configuration
    ptp_apply_profile_defaults_to_port_ds(&config_data.conf[instance].port_config[ptp_virtual_port[instance].slave_data.one_pps_slave_port_no], 
                                          config_data.conf[instance].clock_init.cfg.profile);

    switch (config->virtual_port_mode) {
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB:
            T_IG(VTSS_TRACE_GRP_1_PPS, "initialize IO PIN mode for pin 2");
            io_mode_i.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_SAVE;
            ptp_io_pin[input_pin].ptp_inst = instance;
            ptp_io_pin[input_pin].usage = PTP_IO_PIN_USAGE_RS422;
            if (VTSS_RC_OK != vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PTP, io_pin_pps_in_slave_handler,
                    ptp_io_pin[input_pin].source_id,
                    INTERRUPT_PRIORITY_NORMAL)) {
                T_IG(VTSS_TRACE_GRP_1_PPS, "source already hooked");
            }
            T_IG(VTSS_TRACE_GRP_1_PPS, "source %d", ptp_io_pin[input_pin].source_id);
            break;
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO:
            T_IG(VTSS_TRACE_GRP_1_PPS, "initialize IO PIN mode for input pin %d", input_pin);
            io_mode_i.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_SAVE;
            io_mode_o.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_OUTPUT;
            ptp_io_pin[input_pin].ptp_inst = instance;
            ptp_io_pin[input_pin].usage = PTP_IO_PIN_USAGE_RS422;
            ptp_io_pin[output_pin].ptp_inst = instance;
            ptp_io_pin[output_pin].usage = PTP_IO_PIN_USAGE_RS422;
            if (VTSS_RC_OK != vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PTP, io_pin_main_auto_handler,
                           ptp_io_pin[input_pin].source_id,
                           INTERRUPT_PRIORITY_NORMAL)) {
               T_IG(VTSS_TRACE_GRP_1_PPS, "source already hooked");
            }
            if (config->alarm) {
                vtss_ptp_timer_start(&ptp_virtual_port[instance].virtual_port_alarm_timer, PTP_VIRTUAL_PORT_ALARM_TIME, true);
            } else {
                vtss_ptp_timer_stop(&ptp_virtual_port[instance].virtual_port_alarm_timer);
            }
            T_IG(VTSS_TRACE_GRP_1_PPS, "source %d", ptp_io_pin[input_pin].source_id);
            break;

        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN:
            T_IG(VTSS_TRACE_GRP_1_PPS, "initialize IO PIN mode for pin output pin %d",output_pin);
            io_mode_o.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_OUTPUT;
            ptp_io_pin[output_pin].ptp_inst = instance;
            ptp_io_pin[output_pin].usage = PTP_IO_PIN_USAGE_RS422;
            if (VTSS_RC_OK != vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PTP, io_pin_pps_out_handler,
                       ptp_io_pin[output_pin].source_id,
                       INTERRUPT_PRIORITY_NORMAL)) {
               T_IG(VTSS_TRACE_GRP_1_PPS, "source already hooked");
            }
            if (config->alarm) {
                vtss_ptp_timer_start(&ptp_virtual_port[instance].virtual_port_alarm_timer, PTP_VIRTUAL_PORT_ALARM_TIME, true);
            } else {
                vtss_ptp_timer_stop(&ptp_virtual_port[instance].virtual_port_alarm_timer);
            }
            T_IG(VTSS_TRACE_GRP_1_PPS, "source %d", ptp_io_pin[output_pin].source_id);
            break;
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN:
            T_IG(VTSS_TRACE_GRP_1_PPS, "initialize PPS_IN IO PIN mode for pin %d", input_pin);
            io_mode_i.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_SAVE;
            ptp_io_pin[input_pin].ptp_inst = instance;
            ptp_io_pin[input_pin].usage = PTP_IO_PIN_USAGE_IO_PIN;
            if (VTSS_RC_OK != vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PTP, io_pin_pps_in_slave_handler,
                    ptp_io_pin[input_pin].source_id,
                    INTERRUPT_PRIORITY_NORMAL)) {
                T_IG(VTSS_TRACE_GRP_1_PPS, "source already hooked");
            }
            T_IG(VTSS_TRACE_GRP_1_PPS, "source %d",  ptp_io_pin[input_pin].source_id);
            break;
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT:
            T_IG(VTSS_TRACE_GRP_1_PPS, "initialize PPS_OUT IO PIN mode for pin %d", output_pin);
            io_mode_o.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_OUTPUT;
            ptp_io_pin[output_pin].ptp_inst = instance;
            ptp_io_pin[output_pin].usage = PTP_IO_PIN_USAGE_IO_PIN;
            // only if TOD transfer is enabled
            if (VTSS_RC_OK != vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PTP, io_pin_pps_out_handler,
                    ptp_io_pin[output_pin].source_id,
                    INTERRUPT_PRIORITY_NORMAL)) {
                T_IG(VTSS_TRACE_GRP_1_PPS, "source already hooked");
            }
            if (config->alarm) {
                vtss_ptp_timer_start(&ptp_virtual_port[instance].virtual_port_alarm_timer, PTP_VIRTUAL_PORT_ALARM_TIME, true);
            } else {
                vtss_ptp_timer_stop(&ptp_virtual_port[instance].virtual_port_alarm_timer);
            }
            T_IG(VTSS_TRACE_GRP_1_PPS, "source %d", ptp_io_pin[output_pin].source_id);
            break;
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_FREQ_OUT:
            T_IG(VTSS_TRACE_GRP_1_PPS, "initialize PPS_OUT IO PIN mode for pin %d", output_pin);
            io_mode_o.pin = MESA_TS_EXT_IO_MODE_WAVEFORM_OUTPUT;
            io_mode_o.freq = config->freq;
            ptp_io_pin[output_pin].ptp_inst = instance;
            ptp_io_pin[output_pin].usage = PTP_IO_PIN_USAGE_IO_PIN;
            break;
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE:
            vtss_interrupt_function_hook_t func_hook;
            meba_event_t source_id;
            bool mode_valid = true;
            switch (current_vp_cfg->virtual_port_mode) {
                case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB:
                case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN:
                     func_hook = io_pin_pps_in_slave_handler;
                     source_id = ptp_io_pin[input_pin].source_id;
                     break;
                case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO:
                    func_hook = io_pin_main_auto_handler;
                    source_id = ptp_io_pin[input_pin].source_id;
                    break;
                case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN:
                case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT:
                    func_hook = io_pin_pps_out_handler;
                    source_id = ptp_io_pin[output_pin].source_id;
                    break;
                default:
                    T_IG(VTSS_TRACE_GRP_1_PPS, "invalid virtual port mode");
                    mode_valid = false;
            }
            T_IG(VTSS_TRACE_GRP_1_PPS, "clear func handler :%s\n", func_hook);
            if (mode_valid) {
                vtss_interrupt_source_hook_clear(func_hook, source_id);
            }
            // stop any slave timers used
            T_IG(VTSS_TRACE_GRP_1_PPS, "Disable any 1pps slave timers");
            ptp_virtual_port_selection_enable(instance, false);
            break;
    }
    io_mode_i.domain = config_data.conf[instance].clock_init.cfg.clock_domain;
    io_mode_o.domain = config_data.conf[instance].clock_init.cfg.clock_domain;
    T_IG(VTSS_TRACE_GRP_1_PPS, "input pin %d, output_pin %d, domain %d mode:%d", input_pin, output_pin, io_mode_i.domain, config->virtual_port_mode);
    if (input_pin != PTP_IO_PIN_UNUSED && config->virtual_port_mode != VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE) {
        PTP_RC(mesa_ts_external_io_mode_set(NULL, input_pin, &io_mode_i));
    }
    if (output_pin != PTP_IO_PIN_UNUSED && config->virtual_port_mode != VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE) {
        PTP_RC(mesa_ts_external_io_mode_set(NULL, output_pin, &io_mode_o));
    }

    if (rc == VTSS_RC_OK) {
	//init_done is to differentiate init sequence and any other CLI configurations.
	//&&with config->enable case is to avoid disabl again when port is already disabled during the startup time.
        if ((!init_done && config->enable) || (config_data.conf[instance].virtual_port_cfg.enable != config->enable)) {
		   vtss_ptp_set_servo_device_1pps_virtual_port(instance, config->enable);
        }
        config_data.conf[instance].virtual_port_cfg = *config;
        config_data.conf[instance].virtual_port_cfg.input_pin = input_pin;
        config_data.conf[instance].virtual_port_cfg.output_pin = output_pin;
        // Enable monitor function for monitoring the virtual port state 
        T_IG(VTSS_TRACE_GRP_1_PPS, "input pin %d, output_pin %d, domain %d", input_pin, output_pin, io_mode_i.domain);
        if (config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN || config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB){
            vtss_ptp_timer_start(&ptp_virtual_port[instance].virtual_port_monitor_timer, ONE_SEC_TIMER_INIT_VALUE,true);
        }
        if (config->virtual_port_mode != VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE) {
            PTP_RC(ptp_port_ena(false, iport2uport(ptp_virtual_port[instance].slave_data.one_pps_slave_port_no), instance));
            // Set master port state for MAIN_MAN or PPS_OUT configuration
            if (config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN ||
                config->virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT) {
                // Virtual port is the succeeding number of the last ethernet port. So, same number will be used by virtual port.
                vtss_ptp_virtual_port_master_state_init(ptp_global.ptpi[instance], mesa_cap_port_cnt);
            }
        } else {
            PTP_RC(ptp_port_dis(iport2uport(ptp_virtual_port[instance].slave_data.one_pps_slave_port_no), instance));
        }
    }
	init_done=true;
    return rc;
}

static void io_pin_pps_in_calibrate_handler(meba_event_t source_id, u32 instance_id)
{
    mesa_timestamp_t                ts;
    u64                             tc;
    char str1[50];
    u32 io_pin = source_id_to_pin(source_id);
    uint32_t inst;

    if (ptp_io_pin[io_pin].usage == PTP_IO_PIN_USAGE_NONE) {
        T_IG(VTSS_TRACE_GRP_1_PPS, "IO Pin[%d] usage is NONE", io_pin);
        return;
    }
    if (inst > vtss_appl_cap_ptp_clock_cnt) {
        T_WG(VTSS_TRACE_GRP_1_PPS, "Invalid ptp instance %u", inst);
        return;
    }
    PTP_CORE_LOCK();
    T_DG(VTSS_TRACE_GRP_1_PPS, "I/O pin event: source_id %d, instance_id %u", source_id, inst);
    T_DG(VTSS_TRACE_GRP_1_PPS, "I/O pin: %u, pin mode %d", io_pin, ptp_io_pin[io_pin].io_pin.pin);
    PTP_RC(mesa_ts_saved_timeofday_get(0, io_pin, &ts, &tc));
    T_IG(VTSS_TRACE_GRP_1_PPS, "io_pin %u, time %s", io_pin, TimeStampToString (&ts, str1));
    pps_calib_delay = ts.nanoseconds;
    PTP_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PTP, io_pin_pps_in_calibrate_handler, source_id, INTERRUPT_PRIORITY_NORMAL));
    PTP_CORE_UNLOCK();
}

uint32_t ptp_1pps_calibrated_delay_get(void)
{
    return pps_calib_delay;
}

mesa_rc ptp_1pps_sma_calibrate_virtual_port(uint32_t instance, bool enable)
{
    mesa_ts_ext_io_mode_t io_mode_i = {.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_DISABLE, .domain = 0, .freq = 1};
    mesa_ts_ext_io_mode_t io_mode_o = {.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_DISABLE, .domain = 0, .freq = 1};
    int in_pin = -1;
    int out_pin = -1;

    if (VTSS_IS_IO_PIN_IN(ptp_io_pin[0].board_assignments)) {
        in_pin = 0;
    } else if (VTSS_IS_IO_PIN_IN(ptp_io_pin[1].board_assignments)) {
        in_pin = 1;
    } else if (VTSS_IS_IO_PIN_IN(ptp_io_pin[2].board_assignments)) {
        in_pin = 2;
    } else if (VTSS_IS_IO_PIN_IN(ptp_io_pin[3].board_assignments)){
        in_pin = 3;
    }

    if (VTSS_IS_IO_PIN_OUT(ptp_io_pin[0].board_assignments)) {
        out_pin = 0;
    } else if (VTSS_IS_IO_PIN_OUT(ptp_io_pin[1].board_assignments)) {
        out_pin = 1;
    } else if (VTSS_IS_IO_PIN_OUT(ptp_io_pin[2].board_assignments)) {
        out_pin = 2;
    } else if (VTSS_IS_IO_PIN_OUT(ptp_io_pin[3].board_assignments)) {
        out_pin = 3;
    }

    if (in_pin == -1 || out_pin == -1) {
        T_IG(VTSS_TRACE_GRP_1_PPS, "could not find input/output pin");
        return MESA_RC_ERROR;
    }

    PTP_CORE_LOCK_SCOPE();
    if (enable) {
        pps_calib_delay = 0;
        io_mode_i.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_SAVE;
        io_mode_i.domain = config_data.conf[instance].clock_init.cfg.clock_domain;
        io_mode_o.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_OUTPUT;
        io_mode_o.domain = config_data.conf[instance].clock_init.cfg.clock_domain;
        ptp_io_pin[in_pin].ptp_inst = instance;
        ptp_io_pin[in_pin].usage = PTP_IO_PIN_USAGE_IO_PIN;
        ptp_io_pin[out_pin].ptp_inst = instance;
        ptp_io_pin[out_pin].usage = PTP_IO_PIN_USAGE_IO_PIN;

        if (VTSS_RC_OK != vtss_interrupt_source_hook_set(VTSS_MODULE_ID_PTP, io_pin_pps_in_calibrate_handler,
                ptp_io_pin[in_pin].source_id,
                INTERRUPT_PRIORITY_NORMAL)) {
            T_IG(VTSS_TRACE_GRP_1_PPS, "source already hooked");
        }
        T_IG(VTSS_TRACE_GRP_1_PPS, "source %d",  ptp_io_pin[in_pin].source_id);

    } else {
        ptp_io_pin[in_pin].ptp_inst = -1;
        ptp_io_pin[in_pin].usage = PTP_IO_PIN_USAGE_NONE;
        ptp_io_pin[out_pin].ptp_inst = -1;
        ptp_io_pin[out_pin].usage = PTP_IO_PIN_USAGE_NONE;

        io_mode_i.domain = config_data.conf[instance].clock_init.cfg.clock_domain;
        io_mode_o.domain = config_data.conf[instance].clock_init.cfg.clock_domain;
        vtss_interrupt_source_hook_clear(io_pin_pps_in_calibrate_handler, ptp_io_pin[in_pin].source_id);
    }

    PTP_RC(mesa_ts_external_io_mode_set(NULL, in_pin, &io_mode_i));
    PTP_RC(mesa_ts_external_io_mode_set(NULL, out_pin, &io_mode_o));
    return VTSS_RC_OK;
}
/****************************************************************************/
/*                                                                          */
/*  Virtual port end.                                                       */
/*                                                                          */
/****************************************************************************/

bool ptp_auto_delay_resp_set(uint32_t clock_domain, bool set, bool enable)
{
    if (set && clock_domain < mesa_cap_ts_domain_cnt &&
        fast_cap(MESA_CAP_TS_DELAY_REQ_AUTO_RESP)) {
        mesa_cap_ts_delay_req_auto_resp[clock_domain] = enable;
    }
    return mesa_cap_ts_delay_req_auto_resp[clock_domain];
}

/****************************************************************************/
/*                                                                          */
/*  PTP Internal mode.                                                       */
/*                                                                          */
/****************************************************************************/
static void ptp_internal_mode_init(int inst)
{
    ptp_internal_cfg[inst].sync_rate = DEFAULT_INTERNAL_MODE_SYNC_RATE;
    ptp_internal_cfg[inst].srcClkDomain = VTSS_PTP_SRC_CLK_DOMAIN_NONE;
    vtss_ptp_timer_init(&ptp_internal_cfg[inst].internal_sync_timer, "Internal sync", inst, ptp_internal_slave_t1_t2_rx, &ptp_global.ptpi[inst]->localClockId);
}

static void ptp_internal_state_set(int inst, int srcClkDomain)
{
    ptp_clock_t *ptpClock = ptp_global.ptpi[inst];
    PtpPort_t *ptpPort = &ptpClock->ptpPort[mesa_cap_port_cnt]; //using virtual port.
    if (srcClkDomain == VTSS_PTP_SRC_CLK_DOMAIN_NONE) {
        if (ptpClock->slavePort != 0) {
            T_I("Internal mode disabled");
            PTP_RC(ptp_port_dis(iport2uport(mesa_cap_port_cnt), inst));
            vtss_ptp_state_set(VTSS_APPL_PTP_DISABLED, ptpClock, ptpPort);
        }
    } else {
        if (ptpClock->slavePort == 0) {
            T_I("Internal mode slave port");
            PTP_RC(ptp_port_ena(false, iport2uport(mesa_cap_port_cnt), inst));
            vtss_non_ptp_slave_init(ptpClock, iport2uport(mesa_cap_port_cnt));
            vtss_ptp_state_set(VTSS_APPL_PTP_SLAVE, ptpClock, ptpPort);
        }
    }
}

void ptp_internal_slave_t1_t2_rx(vtss_ptp_sys_timer_t *timer, void *m)
{
    int inst = *((int *)m);
    int srcClkDomain = vtss_local_clock_src_clk_domain_get(inst);
    int clkDomain = config_data.conf[inst].clock_init.cfg.clock_domain;
    int hwSrcClkDomain = srcClkDomain >= mesa_cap_ts_domain_cnt ? SOFTWARE_CLK_DOMAIN : srcClkDomain;
    mesa_timestamp_t t1, t2;
    vtss_ptp_timestamps_t t1_t2;
    static vtss_ptp_timestamps_t prev_t1_t2;
    uint64_t curTime;
    char str1[50], str2[50];
    mesa_timeinterval_t t2Diff, t1Diff;
    int64_t freq_offset = 0;

    if (srcClkDomain == VTSS_PTP_SRC_CLK_DOMAIN_NONE ||
        clkDomain    >= mesa_cap_ts_domain_cnt) {
        return;
    }

    ptp_internal_state_set(inst, srcClkDomain);
    mesa_ts_multi_domain_timeofday_get(NULL, hwSrcClkDomain, clkDomain, &t1, &t2);
    curTime = (((uint64_t)t1.nanoseconds) << 16) | t1.nanosecondsfrac;
    T_IG(VTSS_TRACE_GRP_1_PPS, " curTime t1 %s t2 %s", TimeStampToString(&t1, str1), TimeStampToString(&t2, str2));
    vtss_domain_clock_convert_to_time(curTime, &t1, srcClkDomain);
    T_IG(VTSS_TRACE_GRP_1_PPS, "t1 %s t2 %s", TimeStampToString(&t1, str1), TimeStampToString(&t2, str2));

    t1_t2.tx_ts = t1;
    t1_t2.rx_ts = t2;
    t1_t2.corr = 0;
    if (prev_t1_t2.tx_ts.seconds != 0) {
        vtss_tod_sub_TimeInterval(&t1Diff, &t1, &prev_t1_t2.tx_ts);
        vtss_tod_sub_TimeInterval(&t2Diff, &t2, &prev_t1_t2.rx_ts);
        freq_offset = (t1Diff - t2Diff) * VTSS_ONE_MIA / t2Diff;
        T_IG(VTSS_TRACE_GRP_1_PPS, "freq offset " VPRI64d, freq_offset);
    }
    vtss_non_ptp_slave_t1_t2_rx(ptp_global.ptpi, inst, &t1_t2, DEFAULT_CLOCK_CLASS, 0, true);
    prev_t1_t2 = t1_t2;
}

bool ptp_internal_mode_config_set_priv(uint32_t instance, ptp_internal_mode_config_t in_cfg)
{
    bool ret = true, restart = false;
    int ticks = PTP_LOG_TIMEOUT(DEFAULT_INTERNAL_MODE_SYNC_RATE);

    if (instance < PTP_CLOCK_INSTANCES) {
        if (in_cfg.sync_rate != ptp_internal_cfg[instance].sync_rate) {
            ptp_internal_cfg[instance].sync_rate = in_cfg.sync_rate;
            ticks = PTP_LOG_TIMEOUT(in_cfg.sync_rate);
            vtss_ptp_timer_stop(&ptp_internal_cfg[instance].internal_sync_timer);
            restart = true;
        }

        if (in_cfg.srcClkDomain != ptp_internal_cfg[instance].srcClkDomain) {
            ptp_internal_cfg[instance].srcClkDomain = in_cfg.srcClkDomain;
            vtss_ptp_timer_stop(&ptp_internal_cfg[instance].internal_sync_timer);
            if (in_cfg.srcClkDomain != VTSS_PTP_SRC_CLK_DOMAIN_NONE) {
                restart = true;
            }
        }

        if (restart) {
            //restart the timer.
            vtss_ptp_timer_start(&ptp_internal_cfg[instance].internal_sync_timer, ticks, true);
        }
    } else {
        ret = false;
        T_W("internal mode set failed");
    }
    return ret;

}

bool ptp_internal_mode_config_set(uint32_t instance, ptp_internal_mode_config_t in_cfg)
{
    PTP_CORE_LOCK_SCOPE();
    return ptp_internal_mode_config_set_priv(instance, in_cfg);
}

bool ptp_internal_mode_config_get(uint32_t instance, ptp_internal_mode_config_t *const in_cfg)
{
    bool ret = true;

    PTP_CORE_LOCK_SCOPE();
    if (instance < PTP_CLOCK_INSTANCES) {
        *in_cfg = ptp_internal_cfg[instance];
    } else {
        ret = false;
    }
    return ret;

}

bool vtss_local_clock_src_clk_domain_set(uint32_t inst, int32_t srcClkDomain)
{
    int ticks;
    if (inst >= PTP_CLOCK_INSTANCES || srcClkDomain >= VTSS_PTP_MAX_CLOCK_DOMAINS) {
        return false;
    }

    PTP_CORE_LOCK_SCOPE();
    ticks = PTP_LOG_TIMEOUT(ptp_internal_cfg[inst].sync_rate);
    if (ptp_internal_cfg[inst].srcClkDomain != srcClkDomain) {
        ptp_internal_cfg[inst].srcClkDomain = srcClkDomain;
    }
    if (srcClkDomain != VTSS_PTP_SRC_CLK_DOMAIN_NONE) {
        vtss_ptp_timer_start(&ptp_internal_cfg[inst].internal_sync_timer, ticks, true);
    } else {
        vtss_ptp_timer_stop(&ptp_internal_cfg[inst].internal_sync_timer);
    }

    return true;
}

int vtss_local_clock_src_clk_domain_get(uint32_t inst)
{
    if (inst >= PTP_CLOCK_INSTANCES) {
        return VTSS_PTP_SRC_CLK_DOMAIN_NONE;
    }

    return ptp_internal_cfg[inst].srcClkDomain;
}


#if defined TIMEOFDAY_TEST
static bool first_time = true;
static void tod_test(void)
{
    mesa_timestamp_t ts;
    mesa_timestamp_t prev_ts;

    mesa_timeinterval_t diff;
    u64 tc;
    u64 prev_tc = 0;
    u32 diff_tc;
    int i;
    char str1[50];
    char str2[50];

    vtss_tod_gettimeofday(0, &ts, &tc);
    T_IG(_C,"Testing TOD: Now= %s, tc = " VPRI64u, TimeStampToString (&ts, str1),tc);
    for (i = 0; i < 250; i++) {
        vtss_tod_gettimeofday(0, &ts, &tc);
        if (!first_time) {
            first_time = false;
            subTimeInterval(&diff, &ts, &prev_ts);
            if (diff < 0) {
                T_WG(_C,"Bad time reading: Now= %s, Prev = %s, diff = " VPRI64d,
                     TimeStampToString (&ts, str1),
                     TimeStampToString (&prev_ts, str2), diff);
            }
            prev_ts = ts;
            diff_tc = (u32)((tc >> 16) - (prev_tc >> 16)); // consider nano seconds part only
            if (tc < prev_tc) { /* time counter has wrapped */
                diff_tc += VTSS_HW_TIME_WRAP_LIMIT;
                T_WG(_C,"counter wrapped: tc = " VPRI64u ",  hw_time = " VPRI64u ", diff = %lu", tc, prev_tc, diff_tc);
            }
            if (diff_tc > VTSS_HW_TIME_NSEC_PR_CNT) {
                T_WG(_C,"Bad TC reading: tc = " VPRI64u ",  prev_tc = " VPRI64u ", diff = %lu", tc, prev_tc, diff_tc);
            }
            prev_tc = tc;
        }
    }
    vtss_tod_gettimeofday(0, &ts, &tc);
    T_IG(_C,"End Testing TOD: Now= %s, tc = " VPRI64u, TimeStampToString (&ts, str1),tc);

}
#endif

extern "C" int ptp_icli_cmd_register();

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
mesa_rc ptp_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    int i;
    mesa_rc rc;
    switch (data->cmd) {
    case INIT_CMD_INIT:
        ptp_global.ready = false;
        critd_init(&ptp_global.coremutex, "ptp.core", VTSS_MODULE_ID_PTP, CRITD_TYPE_MUTEX);
        ptp_local_clock_critd_init();

        // Cache various MESA capabilities for fast access.
        meba_cap_synce_dpll_mode_dual            = fast_cap(MEBA_CAP_SYNCE_DPLL_MODE_DUAL);
        meba_cap_synce_dpll_mode_single          = fast_cap(MEBA_CAP_SYNCE_DPLL_MODE_SINGLE);
        mesa_cap_misc_chip_family                = fast_cap(MESA_CAP_MISC_CHIP_FAMILY);
#if defined(VTSS_SW_OPTION_MEP)
        mesa_cap_oam_used_as_ptp_protocol        = fast_cap(MESA_CAP_OAM_USED_AS_PTP_PROTOCOL);
#endif /* defined(VTSS_SW_OPTION_MEP) */
        mesa_cap_packet_auto_tagging             = fast_cap(MESA_CAP_PACKET_AUTO_TAGGING);
        mesa_cap_packet_inj_encap                = fast_cap(MESA_CAP_PACKET_INJ_ENCAP);
        mesa_cap_port_cnt                        = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
        mesa_cap_synce_ann_auto_transmit         = fast_cap(MESA_CAP_SYNCE_ANN_AUTO_TRANSMIT);
        mesa_cap_ts                              = fast_cap(MESA_CAP_TS);
        mesa_cap_ts_asymmetry_comp               = fast_cap(MESA_CAP_TS_ASYMMETRY_COMP);
        mesa_cap_ts_bc_ts_combo_is_special       = fast_cap(MESA_CAP_TS_BC_TS_COMBO_IS_SPECIAL);
        mesa_cap_ts_c_dtc_supported              = fast_cap(MESA_CAP_TS_C_DTC_SUPPORTED);
        mesa_cap_ts_domain_cnt                   = fast_cap(MESA_CAP_TS_DOMAIN_CNT);
        mesa_cap_ts_has_alt_pin                  = fast_cap(MESA_CAP_TS_HAS_ALT_PIN);
        mesa_cap_ts_has_ptp_io_pin               = fast_cap(MESA_CAP_TS_HAS_PTP_IO_PIN);
        mesa_cap_ts_hw_fwd_e2e_1step_internal    = fast_cap(MESA_CAP_TS_HW_FWD_E2E_1STEP_INTERNAL);
        mesa_cap_ts_hw_fwd_p2p_1step             = fast_cap(MESA_CAP_TS_HW_FWD_P2P_1STEP);
        mesa_cap_ts_io_cnt                       = fast_cap(MESA_CAP_TS_IO_CNT);
        mesa_cap_ts_internal_mode_supported      = fast_cap(MESA_CAP_TS_INTERNAL_MODE_SUPPORTED);
        mesa_cap_ts_internal_ports_req_twostep   = fast_cap(MESA_CAP_TS_INTERNAL_PORTS_REQ_TWOSTEP);
        mesa_cap_ts_missing_tx_interrupt         = fast_cap(MESA_CAP_TS_MISSING_TX_INTERRUPT);
        mesa_cap_ts_org_time                     = fast_cap(MESA_CAP_TS_ORG_TIME);
        mesa_cap_ts_p2p_delay_comp               = fast_cap(MESA_CAP_TS_P2P_DELAY_COMP);
        mesa_cap_ts_pps_via_configurable_io_pins = fast_cap(MESA_CAP_TS_PPS_VIA_CONFIGURABLE_IO_PINS);
        mesa_cap_ts_ptp_rs422                    = fast_cap(MESA_CAP_TS_PTP_RS422);
        mesa_cap_ts_twostep_always_required      = fast_cap(MESA_CAP_TS_TWOSTEP_ALWAYS_REQUIRED);
        mesa_cap_ts_use_external_input_servo     = fast_cap(MESA_CAP_TS_USE_EXTERNAL_INPUT_SERVO);
        vtss_appl_cap_max_acl_rules_pr_ptp_clock = fast_cap(VTSS_APPL_CAP_MAX_ACL_RULES_PR_PTP_CLOCK);
        vtss_appl_cap_ptp_clock_cnt              = fast_cap(VTSS_APPL_CAP_PTP_CLOCK_CNT);
        vtss_appl_cap_zarlink_servo_type         = fast_cap(VTSS_APPL_CAP_ZARLINK_SERVO_TYPE);
        mesa_cap_ts_pps_in_via_io_pin            = fast_cap(MESA_CAP_TS_PPS_IN_VIA_IO_PIN);
        mesa_cap_ts_twostep_use_ptp_id           = fast_cap(MESA_CAP_TS_TWOSTEP_USE_PTP_ID);
        vtss_appl_ptp_sub_nano_second            = ((MESA_CHIP_FAMILY_SPARX5  == fast_cap(MESA_CAP_MISC_CHIP_FAMILY)) ||
                                                    (MESA_CHIP_FAMILY_LAN969X == fast_cap(MESA_CAP_MISC_CHIP_FAMILY)) ||
                                                    (MESA_CHIP_FAMILY_LAN966X == fast_cap(MESA_CAP_MISC_CHIP_FAMILY))) ? true : false;
        mesa_cap_ts_vir_io_cnt                   = fast_cap(MESA_CAP_TS_IO_CNT); // number of io pins determine number of virtul ports possible.
        vtss_appl_ptp_combined_phy_switch_ts     = (MESA_CHIP_FAMILY_JAGUAR2 == fast_cap(MESA_CAP_MISC_CHIP_FAMILY)); // use both switch and phy timestamping simultaneously.

        if (fast_cap(MESA_CAP_TS_DELAY_REQ_AUTO_RESP)) {
            for (auto i = 0; i < mesa_cap_ts_domain_cnt; i++) {
                mesa_cap_ts_delay_req_auto_resp[i] = true;
            }
        }
        ptp_tick_init();
        vtss_ptp_timer_initialize();

        vtss_flag_init(&ptp_flags);
        acl_rules_init();
        if (mesa_cap_ts_ptp_rs422) {
            PTP_RC(ptp_1pps_serial_init());
        }

#ifdef VTSS_SW_OPTION_ICFG
        PTP_RC(ptp_icfg_init());
#endif
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        vtss_ptp_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_ptp_json_init();
#endif

        ptp_icli_cmd_register();
        T_I("INIT_CMD_INIT PTP" );
        break;

    case INIT_CMD_CONF_DEF:
        T_I("INIT_CMD_CONF_DEF PTP" );
        if (isid == VTSS_ISID_GLOBAL)
            vtss_flag_setbits(&ptp_flags, PTP_FLAG_DEFCONFIG);
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        // Get our MAC address
        (void)conf_mgmt_mac_addr_get(ptp_global.sysmac.addr, 0);
        {
            char macbuf[20];
            T_I("System MAC address: %s", misc_mac_txt(ptp_global.sysmac.addr, macbuf));
        }

        // phy devices need to be loaded into board_instance to correctly detect phy ts capability.
#ifdef VTSS_SW_OPTION_TOD
        // TGO 23.06.22: taken out due to wear on ZL40251 chip. Implementation must be re-thinked; covered by APPL-5325.
        if (vtss_board_type() == VTSS_BOARD_FIREANT_PCB135_REF &&
            !meba_capability(board_instance, MEBA_CAP_BOARD_HAS_PCB135_CPLD)) {
            // This is a PCB135 rev >4 where the two zl40251 needs to be programmed
            zl40251_initialization();
        }
        // Initialise PHY timestamping data structures in TOD module
        tod_phy_ts_init();
#endif

        if (mepa_phy_ts_cap()) {
            PTP_RC(ptp_1pps_sync_init());
            PTP_RC(ptp_1pps_closed_loop_init());
        }

        if (mesa_cap_ts_ptp_rs422) {
            if ((rc = meba_ptp_rs422_conf_get(board_instance, &rs422_conf)) != VTSS_RC_OK) {
                T_E("RS422 configuration not supported: %s", error_txt(rc));
                return rc;
            }

            T_I("RS422 configuration: gpio_rs422_1588_mstoen %d, gpio_rs422_1588_slvoen %d, \nptp_pin_ldst %d, ptp_pin_ppso %d, \nptp_rs422_pps_int_id %d, ptp_rs422_ldsv_int_id %d",
                rs422_conf.gpio_rs422_1588_mstoen,
                rs422_conf.gpio_rs422_1588_slvoen,
                rs422_conf.ptp_pin_ldst,
                rs422_conf.ptp_pin_ppso,
                rs422_conf.ptp_rs422_pps_int_id,
                rs422_conf.ptp_rs422_ldsv_int_id);
        }

        // Read PTP port calibration from file on Linux filesystem
        {
            int ptp_port_calib_file = open(PTP_CALIB_FILE_NAME, O_RDONLY);
            if (ptp_port_calib_file != -1) {
                u32 dummy;

                ssize_t numread = read(ptp_port_calib_file, &ptp_port_calibration.version, sizeof(u32));
                numread += read(ptp_port_calib_file, &ptp_port_calibration.crc32, sizeof(u32));
                numread += read(ptp_port_calib_file, &ptp_port_calibration.rs422_pps_delay, sizeof(u32));
                numread += read(ptp_port_calib_file, &ptp_port_calibration.sma_pps_delay, sizeof(u32));
                numread += read(ptp_port_calib_file, ptp_port_calibration.port_calibrations.data(), mesa_cap_port_cnt * sizeof(port_calibrations_s));
                numread += read(ptp_port_calib_file, &dummy, sizeof(u32));

                if (numread == 4 * sizeof(u32) + mesa_cap_port_cnt * sizeof(port_calibrations_s)) {
                    if (ptp_port_calibration.version != PTP_CURRENT_CALIB_FILE_VER) {   // File was read. Check version of file is current.
                        T_I("PTP port calibration data file has incorrect version - using default values.");
                        memset(ptp_port_calibration.port_calibrations.data(), 0, mesa_cap_port_cnt * sizeof(port_calibrations_s));
                    } else {  // File was read and version was OK. Check the CRC32.
                        u32 crc32 = vtss_crc32((const unsigned char*)&ptp_port_calibration.version, sizeof(u32));
                        crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)&ptp_port_calibration.rs422_pps_delay, sizeof(u32));
                        crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)&ptp_port_calibration.sma_pps_delay, sizeof(u32));
                        crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)ptp_port_calibration.port_calibrations.data(), mesa_cap_port_cnt * sizeof(port_calibrations_s));

                        if (crc32 != ptp_port_calibration.crc32) {
                            T_W("PTP port calibration data file is corrupt (incorrect CRC32 %u) expected %u - using default values.", ptp_port_calibration.crc32, crc32);
                            memset(ptp_port_calibration.port_calibrations.data(), 0, mesa_cap_port_cnt * sizeof(port_calibrations_s));
                        }
                    }
                } else {
                    T_I("PTP port calibration data file is corrupt (incorrect length) - using default values.");
                    memset(ptp_port_calibration.port_calibrations.data(), 0, mesa_cap_port_cnt * sizeof(port_calibrations_s));
                }

                if (close(ptp_port_calib_file) == -1) {
                    T_W("Could not close PTP port calibration data file.");
                }
            } else {
                T_I("No PTP port calibration data file - using default values.");
                memset(ptp_port_calibration.port_calibrations.data(), 0, mesa_cap_port_cnt * sizeof(port_calibrations_s));
            }
        }

        ptp_conf_read(false);
        port_data_pre_initialize();

#if defined (VTSS_SW_OPTION_P802_1_AS)
        ptp_cmlds_add();
#endif
        for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
            /* Create a clock servo instance */
#ifdef SW_OPTION_BASIC_PTP_SERVO
            basic_servo[i] = vtss::create<VTSS_MODULE_ID_PTP, ptp_basic_servo>(i, &config_data.conf[i].filter_params, &config_data.conf[i].servo_params, config_data.conf[i].clock_init.numberPorts);
#endif // SW_OPTION_BASIC_PTP_SERVO
#if defined(VTSS_SW_OPTION_ZLS30387)
            advanced_servo[i] = vtss::create<VTSS_MODULE_ID_PTP, ptp_ms_servo>(i, &config_data.conf[i].servo_params, &config_data.conf[i].clock_init.cfg, config_data.conf[i].clock_init.numberPorts);
#endif

            /* Create a PTP engine */
            {
                ptp_servo *servo;
#if defined(VTSS_SW_OPTION_ZLS30387)
                servo = advanced_servo[i];
#elif defined(SW_OPTION_BASIC_PTP_SERVO)//  defined(VTSS_SW_OPTION_ZLS30387)
                servo = basic_servo[i];
#endif

                ptp_global.ptpi[i] = vtss_ptp_clock_add(&config_data.conf[i].clock_init,
                                                        &config_data.conf[i].time_prop,
                                                        &config_data.conf[i].port_config[0],
                                                        servo,
                                                        i);
            }
            VTSS_ASSERT(ptp_global.ptpi[i] != NULL);
            if (mesa_cap_synce_ann_auto_transmit) {
                for (int j = 0; j < mesa_cap_port_cnt; j++) {
                    T_I("set afi pointer for inst %d, port %d", i,j);
                    ptp_global.ptpi[i]->ptpPort[j].msm.afi = 0;
                    ptp_global.ptpi[i]->ptpPort[j].ansm.afi = 0;
                }
            }
#if defined (VTSS_SW_OPTION_P802_1_AS)
            ptp_clock_cmlds_init(ptp_global.ptpi[i]);
#endif
        }
        if (mesa_cap_ts_pps_via_configurable_io_pins) {
            PTP_RC(ptp_pin_init());
        }

        if (fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_LAN966X) {
            mesa_ts_ext_io_mode_t io_mode;
            io_mode.freq = 1;
            io_mode.domain = 0;
            io_mode.pin = MESA_TS_EXT_IO_MODE_ONE_PPS_OUTPUT;
            //io pin 4 is used for enabling 1pps to phy on lan-9668
            rc = mesa_ts_external_io_mode_set(NULL, 4, &io_mode);
        }
        // Internal mode initialisation.
        for (i = 0; i < PTP_CLOCK_INSTANCES; i++) {
            ptp_internal_mode_init(i);
        }
        T_I("INIT_CMD_ICFG_LOADING_PRE");
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_I("Resuming thread (ISID = %u)", isid);
        if (isid == VTSS_ISID_START) {
            vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                               ptp_thread,
                               0,
                               "PTP",
                               nullptr,
                               0,
                               &ptp_thread_handle,
                               &ptp_thread_block);
        } else {
            T_E("INIT_CMD_ICFG_LOADING_POST - unknown ISID %u", isid);
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}


