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

#include "primitives.hxx"
#include "main.h"
#include "control_api.h"
#include "conf_api.h"
#include "os_file_api.h"
#include "vtss_api_if_api.h"
#include "port_api.h"
#include "critd_api.h"
#include "interrupt_api.h"
#include "led_api.h"
#include "misc_api.h"
#include "vtss_usb.h"
#include "msg_api.h"
#include "msg_test_api.h"
#include "topo_api.h"
#include "crashhandler.hxx"
#include <unistd.h>
#include <linux/unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <time.h>
#include <linux/random.h>
#include "vtss/basics/stream.hxx"
#include "vtss/basics/parser_impl.hxx"
#include "vtss/basics/string-utils.hxx"
#include <vtss/basics/set.hxx>
#include "board_misc.h"
#include "flash_mgmt_api.h"
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <mtd/ubi-user.h>
#include <mntent.h>
#include <chrono.hxx> /* For vtss::uptime_milliseconds() */
#include "backtrace.hxx"
#include "vtss_alloc.h"
#include "main_trace.h"

#ifdef VTSS_SW_OPTION_CLI
#include "cli_io_api.h"
#endif

#ifdef VTSS_SW_OPTION_ICLI
#include "icli_api.h"
#endif

#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#endif

#ifdef VTSS_SW_OPTION_CONSOLE
#include "console_api.h"
#endif

#ifdef VTSS_SW_OPTION_SYSUTIL
#include "sysutil_api.h"
#endif

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

#ifdef VTSS_SW_OPTION_FIRMWARE
#include "firmware_api.h"
#endif

#ifdef VTSS_SW_OPTION_AGGR
#include "aggr_api.h"
#endif

#ifdef VTSS_SW_OPTION_QOS
#include "qos_api.h"
#endif

#ifdef VTSS_SW_OPTION_VLAN
#include "vlan_api.h"
#endif

#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif

#ifdef VTSS_SW_OPTION_MAC
#include "mac_api.h"
#endif

#ifdef VTSS_SW_OPTION_LOOP_DETECT
#include "vtss_lb_api.h"
#endif

#ifdef VTSS_SW_OPTION_MIRROR
#include "mirror_basic_api.h"
#include "mirror_api.h"
#endif

#ifdef VTSS_SW_OPTION_ACL
#include "acl_api.h"
#endif

#ifdef VTSS_SW_OPTION_PACKET
#include "packet_api.h"
#endif

#if defined(VTSS_SW_OPTION_FRR)
#include "frr_api.hxx"
#endif

#if defined(VTSS_SW_OPTION_FRR_ROUTER)
#include "frr_router_api.hxx"
#endif

#if defined(VTSS_SW_OPTION_FRR_OSPF)
#include "frr_ospf_api.hxx"
#endif

#if defined(VTSS_SW_OPTION_FRR_OSPF6)
#include "frr_ospf6_api.hxx"
#endif

#if defined(VTSS_SW_OPTION_FRR_RIP)
#include "frr_rip_api.hxx"
#endif

#if defined(VTSS_SW_OPTION_IP)
#include "ping_api.h"
#include "traceroute_api.h"
#endif

#if defined(VTSS_SW_OPTION_IP)
#include "ip_api.h"
#endif

#if defined(VTSS_SW_OPTION_DHCP_CLIENT)
#include "dhcp_client_api.h"
#endif

#ifdef VTSS_SW_OPTION_DHCP6_CLIENT
#include "dhcp6_client_api.h"
#endif

#ifdef VTSS_SW_OPTION_DHCP6_RELAY
#include "dhcp6_relay_api.h"
#endif

#ifdef VTSS_SW_OPTION_SYNCE_DPLL
#include "synce_custom_clock_api.h"
#endif

#ifdef VTSS_SW_OPTION_SYNCE
#include "synce.h"
#endif

#ifdef VTSS_SW_OPTION_APS
#include "aps_api.h"
#endif

#ifdef VTSS_SW_OPTION_PVLAN
#include "pvlan_api.h"
#endif

#if defined(VTSS_SW_OPTION_DOT1X)
#include "dot1x_api.h"
#endif

#ifdef VTSS_SW_OPTION_L2PROTO
#include "l2proto_api.h"
#endif

#ifdef VTSS_SW_OPTION_LACP
#include "lacp_api.h"
#endif

#ifdef VTSS_SW_OPTION_LLDP
#include "lldp_api.h"
#endif

#if defined(VTSS_SW_OPTION_SNMP)
#include "vtss_snmp_api.h"
#endif

#ifdef VTSS_SW_OPTION_RMON
#include "rmon_api.h"
#endif

#ifdef VTSS_SW_OPTION_IGMPS
#include "igmps_api.h"
#endif

#ifdef VTSS_SW_OPTION_DNS
#include "ip_dns_api.h"
#endif

#ifdef VTSS_SW_OPTION_POE
#include "poe_api.h"
#endif

#ifdef VTSS_SW_OPTION_AUTH
#include "vtss_auth_api.h"
#endif

#ifdef VTSS_SW_OPTION_UPNP
#include "vtss_upnp_api.h"
#endif

#ifdef VTSS_SW_OPTION_RADIUS
#include "vtss_radius_api.h"
#endif

#ifdef VTSS_SW_OPTION_ACCESS_MGMT
#include "access_mgmt_api.h"
#endif

#if defined(VTSS_SW_OPTION_MSTP)
#include "mstp_api.h"
#endif

#if defined(VTSS_SW_OPTION_PTP)
#include "ptp_api.h"
#endif

#ifdef VTSS_SW_OPTION_DHCP_HELPER
#include "dhcp_helper_api.h"
#endif

#ifdef VTSS_SW_OPTION_DHCP_RELAY
#include "dhcp_relay_api.h"
#endif

#ifdef VTSS_SW_OPTION_DHCP_SNOOPING
#include "dhcp_snooping_api.h"
#endif

#ifdef VTSS_SW_OPTION_DHCP6_SNOOPING
#include "dhcp6_snooping_api.h"
#endif

#if defined(VTSS_SW_OPTION_NTP)
#include "vtss_ntp_api.h"
#endif

#if defined(VTSS_SW_OPTION_USERS)
#include "vtss_users_api.h"
#endif

#if defined(VTSS_SW_OPTION_PRIV_LVL)
#include "vtss_privilege_api.h"
#endif

#ifdef VTSS_SW_OPTION_ARP_INSPECTION
#include "arp_inspection_api.h"
#endif

#ifdef VTSS_SW_OPTION_IP_SOURCE_GUARD
#include "ip_source_guard_api.h"
#endif

#ifdef VTSS_SW_OPTION_IPV6_SOURCE_GUARD
#include "ipv6_source_guard_api.h"
#endif

#ifdef VTSS_SW_OPTION_PSEC
#include "psec_api.h"
#endif

#ifdef VTSS_SW_OPTION_PSEC_LIMIT
#include "psec_limit_api.h"
#endif

#ifdef VTSS_SW_OPTION_IGMP_HELPER
#include "igmp_rx_helper_api.h"
#endif

#ifdef VTSS_SW_OPTION_IPMC_LIB
#include "ipmc_lib_api.h"
#endif

#ifdef VTSS_SW_OPTION_MVR
#include "mvr_api.h"
#endif

#ifdef VTSS_SW_OPTION_VOICE_VLAN
#include "voice_vlan_api.h"
#endif

#ifdef VTSS_SW_OPTION_ERPS
#include "erps_api.h"
#endif

#ifdef VTSS_SW_OPTION_IEC_MRP
#include "iec_mrp_api.h"
#endif

#ifdef VTSS_SW_OPTION_REDBOX
#include "redbox_api.h"
#endif

#ifdef VTSS_SW_OPTION_EEE
#include "eee_api.h"
#endif

#ifdef VTSS_SW_OPTION_FAN
#include "fan_api.h"
#endif

#ifdef VTSS_SW_OPTION_THERMAL_PROTECT
#include "thermal_protect_api.h"
#endif

#ifdef VTSS_SW_OPTION_LED_POW_REDUC
#include "led_pow_reduc_api.h"
#endif

#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
#include "eth_link_oam_api.h"
#endif

#if defined(VTSS_SW_OPTION_TOD)
#include "tod_api.h"
#endif

#ifdef VTSS_SW_OPTION_VCL
#include "vcl_api.h"
#endif

#ifdef VTSS_SW_OPTION_MLDSNP
#include "mldsnp_api.h"
#endif

#ifdef VTSS_SW_OPTION_IPMC
#include "ipmc_api.h"
#endif

#ifdef VTSS_SW_OPTION_SFLOW
#include "sflow_api.h"
#endif

#ifdef VTSS_SW_OPTION_SYMREG
#include "symreg_api.h"
#endif

#ifdef VTSS_SW_OPTION_AFI
#include "afi_api.h"
#endif

#ifdef VTSS_SW_OPTION_THREAD_LOAD_MONITOR
#include "thread_load_monitor_api.hxx"
#endif

#ifdef VTSS_SW_OPTION_ALARM
#include "alarm_api.hxx"
#endif

#ifdef VTSS_SW_OPTION_CPUPORT
#include "cpuport_api.hxx"
#endif

#ifdef VTSS_SW_OPTION_VLAN_TRANSLATION
#include "vlan_translation_api.h"
#endif

#ifdef VTSS_SW_OPTION_XXRP
#include "xxrp_api.h"
#endif

#ifdef VTSS_SW_OPTION_LOOP_PROTECTION
#include "loop_protect_api.h"
#endif

#ifdef VTSS_SW_OPTION_TIMER
#include "vtss_timer_api.h"
#endif

#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
#include "daylight_saving_api.h"
#endif

#ifdef VTSS_SW_OPTION_PHY
#include "phy_api.h"
#endif

#ifdef VTSS_SW_OPTION_KR
#include "kr_api.h"
#endif

#if defined(VTSS_SW_OPTION_ZLS30387)
#include "zl_3038x_api_pdv_api.h"
#endif

#ifdef VTSS_SW_OPTION_DHCP_SERVER
#include "dhcp_server_api.h"
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
#include "vtss_json_rpc_api.h"
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
#include "json_rpc_notification_api.h"
#endif

#ifdef VTSS_SW_OPTION_SNMP_DEMO
#include "vtss_snmp_demo_api.h"
#endif





#if defined(VTSS_SW_OPTION_DDMI)
#include "ddmi_api.h"
#endif

#if defined(VTSS_SW_OPTION_SUBJECT)
#include "subject_api.h"
#endif

#if defined(VTSS_SW_OPTION_UDLD)
#include "udld_api.h"
#endif

#if defined(VTSS_SW_OPTION_FAST_CGI)
#include "fast_cgi_api.h"
#endif

#if defined(VTSS_SW_OPTION_JSON_IPC)
#include "json_ipc_api.h"
#endif





#ifdef VTSS_SW_OPTION_CFM
#include "cfm_api.h"
#endif

#ifdef VTSS_SW_OPTION_STREAM
#include "stream_api.h"
#endif

#ifdef VTSS_SW_OPTION_TSN
#include "tsn_api.h"
#endif

#ifdef VTSS_SW_OPTION_TSN_FRER
#include "frer_api.h"
#endif

#ifdef VTSS_SW_OPTION_TSN_PSFP
#include "psfp_api.h"
#endif

#if defined(VTSS_SW_OPTION_SSH)
#include "vtss_ssh_api.h"
#endif





#if defined(VTSS_SW_OPTION_OPTIONAL_MODULES)
#include "vtss_optional_modules_api.h"
#endif

#if defined(CYGPKG_IO_FILEIO) && defined(CYGPKG_FS_RAM) && defined(VTSS_SW_OPTION_ICFG)
#include <unistd.h>
#include <cyg/fileio/fileio.h>
#endif

#include "vtss_os_wrapper.h"

#if defined(CYGPKG_DEVS_DISK_MMC)
#include <pkgconf/devs_disk_mmc.h>
#endif

#define VTSS_RESTART_FILE "/switch/restart"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_MAIN

static vtss_trace_reg_t trace_reg =
{
    VTSS_TRACE_MODULE_ID, "main", "Main module"
};

static vtss_trace_grp_t trace_grps[] =
{
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MAIN_TRACE_GRP_INIT_MODULES] = {
        "initmods",
        "init_modules() calls",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MAIN_TRACE_GRP_BOARD] = {
        "board",
        "Board (meba) trace",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MAIN_TRACE_GRP_ALLOC] = {
        "alloc",
        "alloc/free calls",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define INIT_MODULES_CRIT_ENTER() critd_enter(&init_modules_crit, __FILE__, __LINE__)
#define INIT_MODULES_CRIT_EXIT()  critd_exit( &init_modules_crit, __FILE__, __LINE__)

static vtss_flag_t            control_flags;
static critd_t                init_modules_crit;
static critd_t                vtss_global_crit;
static vtss_recursive_mutex_t reset_mutex;

// Reset (shutdown) callbacks
typedef struct {
    control_system_reset_callback_t cb;
    control_system_reset_priority_t prio;
    vtss_module_id_t                module_id;
} control_system_reset_set_t;

static vtss::Set<control_system_reset_set_t> callback_list;

// Order reset callbacks after priority
static bool operator<(const control_system_reset_set_t &lhs, const control_system_reset_set_t &rhs)
{
    // We need to compare all elements, and not just the priority, because
    // otherwise vtss::Set will interpret two elements having the same priority
    // as identical and therefore not insert the new element (if lhs < rhs and
    // rhs < lhs, they are considered identical).

    if (lhs.prio != rhs.prio) {
        // The lower the priority, the later in the list must it come.
        return lhs.prio > rhs.prio;
    }

    if (lhs.module_id != rhs.module_id) {
        return lhs.module_id < rhs.module_id;
    }

    return lhs.cb < rhs.cb;
}

// Debugging stuff
static vtss_init_data_t dbg_latest_init_modules_data;
static ulong            dbg_latest_init_modules_init_fun_idx;

// Command flags to main thread
#define CTLFLAG_SYSTEM_RESET            (1 << 1)    // Reset (reboot) system)
#define CTLFLAG_CONFIG_RESTORE          (1 << 2)    // Restore configuration
#define CTLFLAG_CONFIG_RESET            (1 << 3)    // Reset configuration to default
#define CTLFLAG_CONFIG_RESET_KEEPIP     (1 << 4)    // Reset configuration to default but keep management IP

#define INITFUN(x)   {x, #x},

#ifdef VTSS_FEATURE_HEAP_WRAPPERS
mesa_rc vtss_alloc_init(vtss_init_data_t *data);
#endif

static struct {
    mesa_rc           (*func)(vtss_init_data_t *data);
    const char        *name;
    vtss_tick_count_t max_callback_ticks;
    init_cmd_t        max_callback_cmd;
} initfun[] = {
#ifdef VTSS_FEATURE_HEAP_WRAPPERS
    INITFUN(vtss_alloc_init)
#endif
    INITFUN(vtss_trace_init)
    INITFUN(critd_module_init)
#ifdef VTSS_SW_OPTION_TIMER
    INITFUN(vtss_timer_init)
#endif
#ifdef VTSS_SW_OPTION_THREAD_LOAD_MONITOR
    INITFUN(thread_load_monitor_init)
#endif
    INITFUN(conf_init)
    // TBD Why should irqs be initialized earlier for eCPU?
    INITFUN(interrupt_init)
    INITFUN(vtss_api_if_init)
#ifdef VTSS_SW_OPTION_ALARM
     INITFUN(alarm_init)
#endif
#ifdef VTSS_SW_OPTION_CPUPORT
     INITFUN(cpuport_init)
#endif
#ifdef VTSS_SW_OPTION_ICFG
    INITFUN(vtss_icfg_early_init)
#endif
    INITFUN(msg_init)
#ifdef VTSS_SW_OPTION_IEC_MRP
    INITFUN(iec_mrp_init) // Must come before port_init() to avoid startup loops on ring ports
#endif
#ifdef VTSS_SW_OPTION_REDBOX
    INITFUN(redbox_init)
#endif
    INITFUN(port_init)
    //INITFUN(interrupt_init)
#ifdef VTSS_SW_OPTION_CONSOLE
    INITFUN(console_start)
#endif
#if defined(CYGPKG_FS_RAM) && defined(VTSS_SW_OPTION_ICFG)
    INITFUN(os_file_init)
#endif
#ifdef VTSS_SW_OPTION_ICLI
    INITFUN(icli_init)
#endif
#ifdef VTSS_SW_OPTION_CLI
    INITFUN(cli_io_init)
#endif
#ifdef VTSS_SW_OPTION_KR
    INITFUN(kr_init)
#endif
#ifdef VTSS_SW_OPTION_PHY
    INITFUN(phy_init)
#endif
#if defined(VTSS_SW_OPTION_SYSLOG)
    INITFUN(syslog_init)
#endif
#ifdef VTSS_SW_OPTION_PSEC
    INITFUN(psec_init)
#endif
#ifdef VTSS_SW_OPTION_VLAN
    INITFUN(vlan_init)
#endif
    INITFUN(led_init)
    INITFUN(misc_init)
    INITFUN(standalone_init)
#ifdef VTSS_SW_OPTION_ACCESS_MGMT
    INITFUN(access_mgmt_init)
#endif
#ifdef VTSS_SW_OPTION_WEB
    INITFUN(web_init)
#endif
#ifdef VTSS_SW_OPTION_PACKET
    INITFUN(packet_init)
#endif
#ifdef VTSS_SW_OPTION_SYSUTIL
    INITFUN(system_init)
#endif
#ifdef VTSS_SW_OPTION_DHCP_CLIENT
    INITFUN(vtss_dhcp_client_init)
#endif
#if defined(VTSS_SW_OPTION_IP)
    INITFUN(ping_init)
    INITFUN(traceroute_init)
#endif
#ifdef VTSS_SW_OPTION_IP
    INITFUN(vtss_ip_init)
#ifndef VTSS_SW_OPTION_NTP
#endif
#endif
#ifdef VTSS_SW_OPTION_DHCP6_CLIENT
    INITFUN(dhcp6_client_init)
#endif
#ifdef VTSS_SW_OPTION_DHCP6_RELAY
    INITFUN(dhcp6_relay_init)
#endif
#ifdef VTSS_SW_OPTION_ACL
    INITFUN(acl_init)
#endif
#ifdef VTSS_SW_OPTION_MIRROR
    INITFUN(mirror_basic_init)
    INITFUN(mirror_init)
#endif
#ifdef VTSS_SW_OPTION_LOOP_DETECT
    INITFUN(vtss_lb_init)
#endif
#ifdef VTSS_SW_OPTION_MAC
    INITFUN(mac_init)
#endif
#ifdef VTSS_SW_OPTION_QOS
    INITFUN(qos_init)
#endif
#ifdef VTSS_SW_OPTION_AGGR
    INITFUN(aggr_init)
#endif
#ifdef VTSS_SW_OPTION_PVLAN
    INITFUN(pvlan_init)
#endif
#ifdef VTSS_SW_OPTION_FIRMWARE
    INITFUN(firmware_init)
#endif
#ifdef VTSS_SW_OPTION_SYNCE_DPLL
    INITFUN(synce_dpll_init)  // Depends upon misc module
#endif
#ifdef VTSS_SW_OPTION_SYNCE
    INITFUN(synce_init)  // Depends upon synce_dpll module
#endif
#if defined(VTSS_SW_OPTION_MSTP) || defined(VTSS_SW_OPTION_DOT1X) || defined(VTSS_SW_OPTION_LACP) || defined(VTSS_SW_OPTION_SNMP)
    INITFUN(l2_init)
#endif
#ifdef VTSS_SW_OPTION_CFM
    INITFUN(cfm_init)
#endif
#ifdef VTSS_SW_OPTION_APS
    INITFUN(aps_init) // Depends on misc and CFM module
#endif
#ifdef VTSS_SW_OPTION_ERPS
    INITFUN(erps_init) // Depends on misc and CFM module
#endif
#ifdef VTSS_SW_OPTION_MSTP
    INITFUN(mstp_init)
#endif
#ifdef VTSS_SW_OPTION_PTP
    INITFUN(ptp_init)
#endif
#ifdef VTSS_SW_OPTION_LACP
    INITFUN(lacp_init)
#endif
#ifdef VTSS_SW_OPTION_DOT1X
    INITFUN(dot1x_init)
#endif
#ifdef VTSS_SW_OPTION_LLDP
    INITFUN(lldp_init)
#endif
#ifdef VTSS_SW_OPTION_EEE
    INITFUN(eee_init)
#endif
#ifdef VTSS_SW_OPTION_FAN
    INITFUN(fan_init)
#endif
#ifdef VTSS_SW_OPTION_THERMAL_PROTECT
    INITFUN(thermal_protect_init)
#endif
#ifdef VTSS_SW_OPTION_LED_POW_REDUC
    INITFUN(led_pow_reduc_init)
#endif
#if defined(VTSS_SW_OPTION_SNMP)
    INITFUN(snmp_init)
#endif
#ifdef VTSS_SW_OPTION_RMON
    INITFUN(rmon_init)
#endif
#ifdef VTSS_SW_OPTION_DNS
    INITFUN(ip_dns_init)
#endif
#ifdef VTSS_SW_OPTION_POE
    INITFUN(poe_init)
#endif
#ifdef VTSS_SW_OPTION_AUTH
    INITFUN(vtss_auth_init)
#endif
#ifdef VTSS_SW_OPTION_UPNP
    INITFUN(upnp_init)
#endif
#ifdef VTSS_SW_OPTION_RADIUS
    INITFUN(vtss_radius_init)
#endif
#ifdef VTSS_SW_OPTION_DHCP_HELPER
    INITFUN(dhcp_helper_init)
#endif
#ifdef VTSS_SW_OPTION_DHCP_SNOOPING
    INITFUN(dhcp_snooping_init)
#endif
#ifdef VTSS_SW_OPTION_DHCP6_SNOOPING
    INITFUN(dhcp6_snooping_init)
#endif
#ifdef VTSS_SW_OPTION_DHCP_RELAY
    INITFUN(dhcp_relay_init)
#endif
#ifdef VTSS_SW_OPTION_NTP
    INITFUN(ntp_init)
#endif
#ifdef VTSS_SW_OPTION_USERS
    INITFUN(vtss_users_init)
#endif
#ifdef VTSS_SW_OPTION_PRIV_LVL
    INITFUN(vtss_priv_init)
#endif
#ifdef VTSS_SW_OPTION_ARP_INSPECTION
    INITFUN(arp_inspection_init)
#endif
#ifdef VTSS_SW_OPTION_IP_SOURCE_GUARD
    INITFUN(ip_source_guard_init)
#endif
#ifdef VTSS_SW_OPTION_IPV6_SOURCE_GUARD
    INITFUN(ipv6_source_guard_init)
#endif
#ifdef VTSS_SW_OPTION_PSEC_LIMIT
    INITFUN(psec_limit_init)
#endif
#ifdef VTSS_SW_OPTION_IPMC
    INITFUN(ipmc_init)
#endif
#ifdef VTSS_SW_OPTION_MVR
    INITFUN(mvr_init)
#endif
#ifdef VTSS_SW_OPTION_IPMC_LIB
    // Must be initialized after IPMC and MVR.
    INITFUN(ipmc_lib_init)
#endif
#ifdef VTSS_SW_OPTION_IGMP_HELPER
    INITFUN(vtss_igmp_helper_init)
#endif
#ifdef VTSS_SW_OPTION_IGMPS
    INITFUN(igmps_init)
#endif
#ifdef VTSS_SW_OPTION_MLDSNP
    INITFUN(mldsnp_init)
#endif
#ifdef VTSS_SW_OPTION_VOICE_VLAN
    INITFUN(voice_vlan_init)
#endif
#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
    INITFUN(eth_link_oam_init)
#endif
#ifdef VTSS_SW_OPTION_TOD
    INITFUN(tod_init)
#endif
#ifdef VTSS_SW_OPTION_SFLOW
    INITFUN(sflow_init)
#endif
#ifdef VTSS_SW_OPTION_VCL
    INITFUN(vcl_init)
#endif
#ifdef VTSS_SW_OPTION_SYMREG
    INITFUN(symreg_init)
#endif
#ifdef VTSS_SW_OPTION_AFI
    INITFUN(afi_init)
#endif
#ifdef VTSS_SW_OPTION_VLAN_TRANSLATION
    INITFUN(vlan_trans_init)
#endif
#ifdef VTSS_SW_OPTION_XXRP
    INITFUN(xxrp_init)
#endif
#ifdef VTSS_SW_OPTION_LOOP_PROTECTION
    INITFUN(loop_protect_init)
#endif
#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
    INITFUN(time_dst_init)
#endif
#if defined(VTSS_SW_OPTION_ZLS30387)
    INITFUN(zl_3038x_pdv_init)
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
    INITFUN(vtss_json_rpc_init)
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
    INITFUN(vtss_json_rpc_notification_init)
#endif
#ifdef VTSS_SW_OPTION_SNMP_DEMO
    INITFUN(vtss_snmp_demo_init)
#endif
#if defined(VTSS_SW_OPTION_DDMI)
    INITFUN(ddmi_init)
#endif
#if defined(VTSS_SW_OPTION_SUBJECT)
    INITFUN(vtss_subject_init)
#endif
#ifdef VTSS_SW_OPTION_FRR
    INITFUN(frr_init)
#endif
#ifdef VTSS_SW_OPTION_FRR_ROUTER
    INITFUN(frr_router_init)
#endif
#ifdef VTSS_SW_OPTION_FRR_OSPF
    INITFUN(frr_ospf_init)
#endif
#ifdef VTSS_SW_OPTION_FRR_OSPF6
    INITFUN(frr_ospf6_init)
#endif
#ifdef VTSS_SW_OPTION_FRR_RIP
    INITFUN(frr_rip_init)
#endif
#ifdef VTSS_SW_OPTION_DHCP_SERVER
    INITFUN(dhcp_server_init)
#endif
#if defined(VTSS_SW_OPTION_UDLD)
    INITFUN(udld_init)
#endif
#if defined(VTSS_SW_OPTION_FAST_CGI)
    INITFUN(vtss_fast_cgi_init)
#endif
#if defined(VTSS_SW_OPTION_JSON_IPC)
    INITFUN(json_ipc_init)
#endif



#if defined(VTSS_SW_OPTION_SSH)
    INITFUN(ssh_init)
#endif



#if defined(VTSS_SW_OPTION_OPTIONAL_MODULES)
    INITFUN(vtss_optional_modules_init)
#endif
#ifdef VTSS_SW_OPTION_STREAM
    INITFUN(stream_init)
#endif
#ifdef VTSS_SW_OPTION_TSN
    INITFUN(tsn_init)
#endif
#ifdef VTSS_SW_OPTION_TSN_FRER
    INITFUN(frer_init)
#endif
#ifdef VTSS_SW_OPTION_TSN_PSFP
    INITFUN(psfp_init)
#endif

// **** NOTE: vtss_icfg_late_init must be last in this list ****
#ifdef VTSS_SW_OPTION_ICFG
    INITFUN(vtss_icfg_late_init)
#endif

// **** NOTE: sw_push_button_init() is a special case need to process behind vtss_icfg_late_init()
// because the startup configuration will be reset to factory default when press this button and holding over 5 seconds. ****



};

#if defined(VTSS_SW_OPTION_SYNCE_DPLL)
static char synce_dpll_err_str[256];
#endif

/* API error text */
static const char *mesa_error_txt(mesa_rc rc)
{
#define MRCF(_rc_) case MESA_RC_##_rc_:     return vtss_xstr(MESA_RC_##_rc_);
#define MRC(_rc_)  case MESA_RC_ERR_##_rc_: return vtss_xstr(MESA_RC_ERR_##_rc_);

    switch (rc) {
    MRCF(OK);
    MRCF(ERROR);
    MRCF(INV_STATE);
    MRCF(INCOMPLETE);
    MRCF(NOT_IMPLEMENTED);
    MRC(PARM);
    MRC(NO_RES);

    // KR errors
    MRC(KR_CONF_NOT_SUPPORTED);
    MRC(KR_CONF_INVALID_PARAMETER);

    // 1G errors
    MRC(PHY_BASE_NO_NOT_FOUND);
    MRC(PHY_6G_MACRO_SETUP);
    MRC(PHY_MEDIA_IF_NOT_SUPPORTED);
    MRC(PHY_CLK_CONF_NOT_SUPPORTED);
    MRC(PHY_GPIO_ALT_MODE_NOT_SUPPORTED);
    MRC(PHY_GPIO_PIN_NOT_SUPPORTED);
    MRC(PHY_PORT_OUT_RANGE);
    MRC(PHY_PATCH_SETTING_NOT_SUPPORTED);
    MRC(PHY_LCPLL_NOT_SUPPORTED);
    MRC(PHY_RCPLL_NOT_SUPPORTED);

    // MACsec errors
    MRC(MACSEC_INVALID_SCI_MACADDR);
    MRC(MACSEC_NOT_ENABLED);
    MRC(MACSEC_SECY_ALREADY_IN_USE);
    MRC(MACSEC_NO_SECY_FOUND);
    MRC(MACSEC_NO_SECY_VACANCY);
    MRC(MACSEC_INVALID_VALIDATE_FRM);
    MRC(MACSEC_COULD_NOT_PRG_SA_MATCH);
    MRC(MACSEC_COULD_NOT_PRG_SA_FLOW);
    MRC(MACSEC_COULD_NOT_ENA_SA);
    MRC(MACSEC_COULD_NOT_SET_SA);
    MRC(MACSEC_INVALID_BYPASS_HDR_LEN);
    MRC(MACSEC_SC_NOT_FOUND);
    MRC(MACSEC_NO_CTRL_FRM_MATCH);
    MRC(MACSEC_COULD_NOT_SET_PATTERN);
    MRC(MACSEC_TIMEOUT_ISSUE);
    MRC(MACSEC_COULD_NOT_EMPTY_EGRESS);
    MRC(MACSEC_AN_NOT_CREATED);
    MRC(MACSEC_COULD_NOT_EMPTY_INGRESS);
    MRC(MACSEC_TX_SC_NOT_EXIST);
    MRC(MACSEC_COULD_NOT_DISABLE_SA);
    MRC(MACSEC_COULD_NOT_DEL_RX_SA);
    MRC(MACSEC_COULD_NOT_DEL_TX_SA);
    MRC(MACSEC_PATTERN_NOT_SET);
    MRC(MACSEC_HW_RESOURCE_EXHUSTED);
    MRC(MACSEC_SCI_ALREADY_EXISTS);
    MRC(MACSEC_SC_RESOURCE_NOT_FOUND);
    MRC(MACSEC_RX_AN_ALREADY_IN_USE);
    MRC(MACSEC_EMPTY_RECORD);
    MRC(MACSEC_COULD_NOT_PRG_XFORM);
    MRC(MACSEC_COULD_NOT_TOGGLE_SA);
    MRC(MACSEC_TX_AN_ALREADY_IN_USE);
    MRC(MACSEC_ALL_AVAILABLE_SA_IN_USE);
    MRC(MACSEC_MATCH_DISABLE);
    MRC(MACSEC_ALL_CP_RULES_IN_USE);
    MRC(MACSEC_PATTERN_PRIO_NOT_VALID);
    MRC(MACSEC_BUFFER_TOO_SMALL);
    MRC(MACSEC_FRAME_TOO_LONG);
    MRC(MACSEC_FRAME_TRUNCATED);
    MRC(MACSEC_PHY_POWERED_DOWN);
    MRC(MACSEC_PHY_NOT_MACSEC_CAPABLE);
    MRC(MACSEC_AN_NOT_EXIST);
    MRC(MACSEC_NO_PATTERN_CFG);
    MRC(MACSEC_MAX_MTU);
    MRC(MACSEC_UNEXPECT_CP_MODE);
    MRC(MACSEC_COULD_NOT_DISABLE_AN);
    MRC(MACSEC_RULE_OUT_OF_RANGE);
    MRC(MACSEC_RULE_NOT_EXIST);
    MRC(MACSEC_CSR_READ);
    MRC(MACSEC_CSR_WRITE);
    MRC(PHY_6G_RCPLL_ON_BASE_PORT_ONLY);

    // Misc errors
    MRC(INVALID_NULL_PTR);
    MRC(INV_PORT_BOARD);

    // Clause 37 errors
    MRC(PCS_BLOCK_NOT_SUPPORTED);

    // PoE errors
    MRC(POE_FIRMWARE_IS_UP_TO_DATE);
    MRC(POE_RX_BUF_EMPTY);
    MRC(POE_FIRM_UPDATE_NEEDED);
    MRC(POE_COMM_PROT_ERR);
    MRC(NOT_POE_PORT_ERR);

    default:
        return "MESA: Unknown error code";
    }

#undef MRC
#undef MRCF
}

/* Error code interpretation */
const char *error_txt(mesa_rc rc)
{
    const char  *txt = NULL;
    int         module_id;
    int         code;
    static char txt_default[100];

    if (rc > 0) {
        T_E("error_txt() invoked with a positive number (%d)", rc);
        txt = "No error";
        return "Unknown error (rc is positive)";
    }

    module_id = VTSS_RC_GET_MODULE_ID(rc);
    code      = VTSS_RC_GET_MODULE_CODE(rc);

    /* Default text if no module-specific decoding available */
    sprintf(txt_default, "module_id=%s, code=%d", vtss_module_names[module_id], code);
    T_D("%s",txt_default);
    if (rc <= VTSS_RC_OK) {
        switch (module_id) {
        case VTSS_MODULE_ID_API_IO:
        case VTSS_MODULE_ID_API_CI:
        case VTSS_MODULE_ID_API_AI:
            txt = mesa_error_txt(rc);
            break;
#ifdef VTSS_SW_OPTION_ACL
        case VTSS_MODULE_ID_ACL:
            txt = acl_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_MAC
        case VTSS_MODULE_ID_MAC:
            txt = mac_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_VLAN
        case VTSS_MODULE_ID_VLAN:
            txt = vlan_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_PACKET
        case VTSS_MODULE_ID_PACKET:
            txt = packet_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_QOS
        case VTSS_MODULE_ID_QOS:
            txt = vtss_appl_qos_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_FRR
        case VTSS_MODULE_ID_FRR:
            txt = frr_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_FRR_ROUTER
        case VTSS_MODULE_ID_FRR_ROUTER:
            txt = frr_router_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_FRR_OSPF
        case VTSS_MODULE_ID_FRR_OSPF:
            txt = frr_ospf_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_FRR_OSPF6
        case VTSS_MODULE_ID_FRR_OSPF6:
            txt = frr_ospf6_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_FRR_RIP
        case VTSS_MODULE_ID_FRR_RIP:
            txt = frr_rip_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_IP
        case VTSS_MODULE_ID_IP:
            txt = ip_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_DHCP_CLIENT
        case VTSS_MODULE_ID_DHCP_CLIENT:
            txt = dhcp_client_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_DHCP6_CLIENT
        case VTSS_MODULE_ID_DHCP6C:
            txt = dhcp6_client_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_IPMC_LIB
        case VTSS_MODULE_ID_IPMC_LIB:
            txt = ipmc_lib_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_PVLAN
        case VTSS_MODULE_ID_PVLAN:
            txt = pvlan_error_txt(rc);
            break;
#endif
#if defined(VTSS_SW_OPTION_DOT1X)
        case VTSS_MODULE_ID_DOT1X:
            txt = dot1x_error_txt(rc);
            break;
#endif
#if defined(VTSS_SW_OPTION_MIRROR)
        case VTSS_MODULE_ID_MIRROR:
            txt = mirror_error_txt(rc);
            break;
#endif
#if defined(VTSS_SW_OPTION_SYNCE)
        case VTSS_MODULE_ID_SYNCE:
            txt = synce_error_txt(rc);
            break;
#endif
#if defined(VTSS_SW_OPTION_POE)
        case VTSS_MODULE_ID_POE:
            txt = poe_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_FIRMWARE
        case VTSS_MODULE_ID_FIRMWARE:
            txt = firmware_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_AGGR
        case VTSS_MODULE_ID_AGGR:
          txt = aggr_error_txt(rc);
          break;
#endif
#ifdef VTSS_SW_OPTION_LACP
        case VTSS_MODULE_ID_LACP:
          txt = lacp_error_txt(rc);
          break;
#endif
#if defined(VTSS_SW_OPTION_SNMP)
        case VTSS_MODULE_ID_SNMP:
            txt = snmp_error_txt((snmp_error_t)rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_AUTH
        case VTSS_MODULE_ID_AUTH:
            txt = vtss_auth_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_ACCESS_SSH
        case VTSS_MODULE_ID_SSH:
            txt = ssh_error_txt(rc);
            break;
#endif
#if defined(VTSS_SW_OPTION_RADIUS)
        case VTSS_MODULE_ID_RADIUS:
            txt = vtss_radius_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_ACCESS_MGMT
        case VTSS_MODULE_ID_ACCESS_MGMT:
            txt = access_mgmt_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_UPNP
        case VTSS_MODULE_ID_UPNP:
            txt = upnp_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_DHCP_HELPER
        case VTSS_MODULE_ID_DHCP_HELPER:
            txt = dhcp_helper_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_DHCP_RELAY
        case VTSS_MODULE_ID_DHCP_RELAY:
            txt = dhcp_relay_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_DHCP_SNOOPING
        case VTSS_MODULE_ID_DHCP_SNOOPING:
            txt = dhcp_snooping_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_NTP
        case VTSS_MODULE_ID_NTP:
            txt = ntp_error_txt((ntp_error_t)rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_DAYLIGHT_SAVING
        case VTSS_MODULE_ID_DAYLIGHT_SAVING:
            txt = daylight_saving_error_txt((time_dst_error_t)rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_USERS
        case VTSS_MODULE_ID_USERS:
            txt = vtss_users_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_PRIV_LVL
        case VTSS_MODULE_ID_PRIV_LVL:
            txt = vtss_privilege_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_ARP_INSPECTION
        case VTSS_MODULE_ID_ARP_INSPECTION:
            txt = arp_inspection_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_IP_SOURCE_GUARD
        case VTSS_MODULE_ID_IP_SOURCE_GUARD:
            txt = ip_source_guard_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_IPV6_SOURCE_GUARD
        case VTSS_MODULE_ID_IPV6_SOURCE_GUARD:
            txt = ipv6_source_guard_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_PSEC
        case VTSS_MODULE_ID_PSEC:
            txt = psec_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_PSEC_LIMIT
        case VTSS_MODULE_ID_PSEC_LIMIT:
            txt = psec_limit_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_VOICE_VLAN
        case VTSS_MODULE_ID_VOICE_VLAN:
            txt = voice_vlan_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_PORT
        case VTSS_MODULE_ID_PORT:
            txt = port_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_LLDP
        case VTSS_MODULE_ID_LLDP:
            txt = lldp_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_EEE
        case VTSS_MODULE_ID_EEE:
            txt = eee_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_FAN
        case VTSS_MODULE_ID_FAN:
            txt = fan_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_LED_POW_REDUC
        case VTSS_MODULE_ID_LED_POW_REDUC:
            txt = led_pow_reduc_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_VCL
        case VTSS_MODULE_ID_VCL:
            txt = vcl_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_SYSLOG
        case VTSS_MODULE_ID_SYSLOG:
            txt = syslog_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_VLAN_TRANSLATION
        case VTSS_MODULE_ID_VLAN_TRANSLATION:
            txt = vlan_trans_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_XXRP
        case VTSS_MODULE_ID_XXRP:
            txt = xxrp_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_ICLI
        case VTSS_MODULE_ID_ICLI:
            txt = icli_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_SFLOW
        case VTSS_MODULE_ID_SFLOW:
            txt = sflow_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_CONSOLE
        case VTSS_MODULE_ID_CONSOLE:
            txt = console_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_DHCP_SERVER
        case VTSS_MODULE_ID_DHCP_SERVER:
            txt = dhcp_server_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_DDMI
        case VTSS_MODULE_ID_DDMI:
            txt = ddmi_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        case VTSS_MODULE_ID_JSON_RPC:
            txt = vtss_json_rpc_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
        case VTSS_MODULE_ID_JSON_RPC_NOTIFICATION:
            txt = vtss_json_rpc_notification_error_txt(rc);
            break;
#endif
#if defined(VTSS_SW_OPTION_PTP)
        case VTSS_MODULE_ID_PTP:
            txt = ptp_error_txt(rc);
            break;
#endif
#if defined(VTSS_SW_OPTION_APS)
        case VTSS_MODULE_ID_APS:
            txt = aps_error_txt(rc);
            break;
#endif
#if defined(VTSS_SW_OPTION_ERPS)
        case VTSS_MODULE_ID_ERPS:
            txt = erps_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_IEC_MRP
        case VTSS_MODULE_ID_IEC_MRP:
            txt = iec_mrp_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_REDBOX
        case VTSS_MODULE_ID_REDBOX:
            txt = redbox_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_SYMREG
        case VTSS_MODULE_ID_SYMREG:
            txt = symreg_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_AFI
        case VTSS_MODULE_ID_AFI:
            txt = afi_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_ALARM
        case VTSS_MODULE_ID_ALARM:
            txt = alarm_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_CPUPORT
        case VTSS_MODULE_ID_CPUPORT:
            txt = cpuport_error_txt(rc);
            break;
#endif





#ifdef VTSS_SW_OPTION_THREAD_LOAD_MONITOR
        case VTSS_MODULE_ID_THREAD_LOAD_MONITOR:
            txt = thread_load_monitor_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_CFM
        case VTSS_MODULE_ID_CFM:
            txt = cfm_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_TSN_PSFP
        case VTSS_MODULE_ID_PSFP:
            txt = psfp_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_STREAM
        case VTSS_MODULE_ID_STREAM:
            txt = stream_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_TSN
        case VTSS_MODULE_ID_TSN:
            txt = tsn_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_TSN_FRER
        case VTSS_MODULE_ID_FRER:
            txt = frer_error_txt(rc);
            break;
#endif
#ifdef VTSS_SW_OPTION_KR
        case VTSS_MODULE_ID_KR:
            txt = kr_error_txt(rc);
            break;
#endif





        default:
            txt = txt_default;
            break;
        }
    }
    return txt;
}

static mesa_restart_t __restart = MESA_RESTART_COLD;
static uint32_t       __restart_callback_wait_msec;

static void restart_reason_init()
{
    FILE *restart_file = fopen(VTSS_RESTART_FILE, "r");
    if(restart_file != NULL) {
        (void)fread(&__restart, sizeof(mesa_restart_t), 1, restart_file);
        (void)fclose(restart_file);
        (void)remove(VTSS_RESTART_FILE);
    }
}

static void restart_reason_set(mesa_restart_t restart_reason, uint32_t wait_before_callback_msec = 0)
{
    FILE *restart_file = fopen(VTSS_RESTART_FILE, "w");
    __restart = restart_reason;
    __restart_callback_wait_msec = wait_before_callback_msec;
    if(restart_file != NULL) {
        (void)fwrite(&restart_reason, sizeof(mesa_restart_t), 1, restart_file);
        (void)fclose(restart_file);
    }
}

mesa_rc control_system_restart_status_get(const mesa_inst_t inst,
                                          mesa_restart_status_t *const status)
{
#if defined(VTSS_DEVCPU_GCB_CHIP_REGS_GENERAL_PURPOSE) || defined(VTSS_DEVCPU_GCB_CHIP_REGS_GPR)
    return mesa_restart_status_get(inst, status);
#else
    (void) mesa_restart_status_get(inst, status);
    status->restart = __restart;
    return MESA_RC_OK;
#endif

}

const char *control_system_restart_to_str(mesa_restart_t restart)
{
    switch (restart) {
    case MESA_RESTART_COLD:
        return "cold";

    case MESA_RESTART_COOL:
        return "cool";

    case MESA_RESTART_WARM:
        return "warm";

    default:
        T_E("Unknown restart type: %d", restart);
        return "unknown";
    }
}

extern "C" int main_icli_cmd_register();

mesa_rc init_modules(vtss_init_data_t *data)
{
    int i;
    mesa_rc rc, rc_last = VTSS_RC_OK;
    vtss_tick_count_t start, total;
    INIT_MODULES_CRIT_ENTER();

    if (data->cmd == INIT_CMD_START) {
        if (strcmp(VTSS_PRODUCT_NAME, "IStaX") == 0 || strcmp(VTSS_PRODUCT_NAME, "IStaX38x") == 0) {
            // Verify the chip supports istax features
            switch (vtss_api_chipid()) {
            case 0x7423:
            case 0x7428:
            case 0x7429: // Luton 26
            case 0x7438:
            case 0x7442:
            case 0x7444:
            case 0x7448:
            case 0x7449:
            case 0x7464:
            case 0x7468: // Jr2
            case 0x7410:
            case 0x7415:
            case 0x7430:
            case 0x7435:
            case 0x7436:
            case 0x7437:
            case 0x7440: // Serval-T
            case 0x7513:
            case 0x7514: // Ocelot
            case 0x47546:
            case 0x47549:
            case 0x47552:
            case 0x47556:
            case 0x47558: // SparX-5
            case 0x9662:  
            case 0x9668:  // LAN-9668
            case 0x969c:  // LAN969x
                // No problem, just continue
                break;
            default:
                printf("****************\n");
                printf("* Warning:\n");
                printf("*\n");
                printf("* Software stack: %s not supported by chip %s.\n", VTSS_PRODUCT_NAME, misc_chip_id_txt());
                printf("*\n");
                printf("* Some features may not work.\n");
                printf("****************\n");
            }
        }
    }
    dbg_latest_init_modules_data = *data;
    for (i = 0; i < ARRSZ(initfun); i++) {
        dbg_latest_init_modules_init_fun_idx = i;
        T_IG(MAIN_TRACE_GRP_INIT_MODULES, "%s(%s, cmd: %d (%s), isid: %x, flags: 0x%x)", __FUNCTION__, initfun[i].name, data->cmd, control_init_cmd2str(data->cmd), data->isid, data->flags);

        start = vtss_current_time();
        if ((rc = initfun[i].func(data)) != VTSS_RC_OK) {
            rc_last = rc; /* Last error is returned */
        }
        total = vtss_current_time() - start;
        T_IG(MAIN_TRACE_GRP_INIT_MODULES, "%s: " VPRI64u " ms", initfun[i].name, VTSS_OS_TICK2MSEC(total));

        if (total > initfun[i].max_callback_ticks) {
            initfun[i].max_callback_ticks = total;
            initfun[i].max_callback_cmd   = data->cmd;
        }
    }

    if (data->cmd == INIT_CMD_ICFG_LOADING_PRE) {
        T_WG(MAIN_TRACE_GRP_INIT_MODULES, "INIT_CMD_ICFG_LOADING_PRE done. Time since boot = " VPRI64u "", vtss::uptime_milliseconds());
    }

    if (data->cmd == INIT_CMD_INIT) {
         // Tell message module that the INIT_CMD_INIT
         // message has been pumped through all modules.
         void msg_init_done(void); // Not published
         msg_init_done();
         main_icli_cmd_register();
    }

    INIT_MODULES_CRIT_EXIT();

#if defined(VTSS_SW_OPTION_SYNCE_DPLL)
    if (data->cmd == INIT_CMD_START && synce_dpll_err_str[0] != '\0') {
        // The DPLL F/W update failed. Issue a syslog message with that.
        S_I("%s", synce_dpll_err_str);
    }
#endif

    return rc_last;
}

#ifdef CYGPKG_CPULOAD
static u32           cpuload_calibrate;
static cyg_cpuload_t cpuload_obj;
static vtss_handle_t cpuload_handle;
#endif

vtss_bool_t control_sys_get_cpuload(u32 *average_point1s,
                                    u32 *average_1s,
                                    u32 *average_10s)
{
    vtss_cpuload_get(average_point1s, average_1s, average_10s);
    return true;
}

#ifdef __cplusplus
extern "C" void control_access_statistics_start(void);
#else
void control_access_statistics_start(void);
#endif

void vtss_global_lock(const char *file, unsigned int line)
{
    if (!vtss_global_crit.init_done) {
        // We do a lazy init, because this function may be called during
        // construction of objects, that is, prior to invocation of main().
        critd_init(&vtss_global_crit, "Global lock", VTSS_MODULE_ID_MAIN, CRITD_TYPE_MUTEX_RECURSIVE);
    }

    critd_enter(&vtss_global_crit, file, line);
}

void vtss_global_unlock(const char *file, unsigned int line)
{
    critd_exit(&vtss_global_crit, file, line);
}

static bool dir_exists(const char *dir)
{
    struct stat sb;
    return stat(dir, &sb) == 0 && S_ISDIR(sb.st_mode);
}

static bool mount_exists(const char *mnt_dir)
{
    struct mntent *ent;
    FILE *mounts;

    mounts = setmntent("/proc/mounts", "r");
    if (!mounts) {
        return false;
    }

    while ((ent = getmntent(mounts)) != nullptr) {
        if (strcmp(mnt_dir, ent->mnt_dir) == 0) {
            endmntent(mounts);
           return true;
        }
    }

    endmntent(mounts);
    return false;
}

static void basic_linux_system_init() {
    int fd = -1;
    int res, line;
    struct ifreq ifr = {};
    struct sockaddr_in *sa = (struct sockaddr_in *)&ifr.ifr_addr;

    // Very basic stuff
    chdir("/");
    setsid();
    putenv((char *) "HOME=/");
    putenv((char *) "TERM=linux");
    putenv((char *) "SHELL=/bin/sh");
    putenv((char *) "USER=root");

#define DO(X) res = X; if (res == -1) { line = __LINE__; goto ERROR; }
#define CHECK(X) if (X) { line = __LINE__; goto ERROR; }

    // Mount the various file systems

    // Shared Memory
    if (!dir_exists("/dev/shm")) {
        DO(mkdir("/dev/shm", 0755));
    }

    // Pseudo Terminal
    if (!dir_exists("/dev/pts")) {
        DO(mkdir("/dev/pts", 0755));
    }

    DO(mount("proc", "/proc", "proc", 0, 0));

    if (!mount_exists("/sys")) {
        DO(mount("sysfs", "/sys", "sysfs", 0, 0));
    }

    if (!mount_exists("/tmp")) {
        DO(mount("tmpfd", "/tmp", "tmpfs", 0, "size=8388608,nosuid,nodev,mode=1777"));
    }

    if (!mount_exists("/run")) {
        DO(mount("tmpfd", "/run", "tmpfs", 0, "size=2097152,nosuid,nodev,mode=1777"));
    }

    if (!mount_exists("/dev/pts")) {
        DO(mount("devpts", "/dev/pts", "devpts", 0, 0));
    }

    // We do not perform a pivot_root here, as we intend to continue running on
    // the read-only squashfs filesystem that is mounted as part of initrd.
    // This may change in near future as we could boot from NAND flash instead.

    if (misc_is_bbb()) {
        // Create a tmpfs for /switch/, since we don't have persistent memory at
        // hand
        if (!mount_exists("/switch/")) {
            DO(mount("tmpfd", "/switch", "tmpfs", 0, "size=2097152,mode=1777"));
        }
    }

    // Enable sys-requests - ignore errors as this is a nice-to-have
    fd = open("/proc/sys/kernel/sysrq", O_WRONLY);
    static const char *one = "1\n";
    if (fd != -1) {
        write(fd, one, strlen(one));
        close(fd);
    }

    // Assign address 127.0.0.1 to the loop-back device (lo) and bring it up
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK(fd == -1);
    strncpy(ifr.ifr_name, "lo", IFNAMSIZ);
    sa->sin_family = AF_INET;
    sa->sin_port = 0;
    sa->sin_addr.s_addr = inet_addr("127.0.0.1");
    DO(ioctl(fd, SIOCSIFADDR, &ifr));
    DO(ioctl(fd, SIOCGIFFLAGS, &ifr));
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    DO(ioctl(fd, SIOCSIFFLAGS, &ifr));

#undef DO
#undef CHECK
    if (fd != -1) close(fd);
    return;

ERROR:
    if (fd != -1) close(fd);
    printf("BASIC SYSTEM INIT FAILED!!!\n");
    printf("File: %s, Line: %d, code: %d\n", __FILE__, line, res);
    printf("errno: %d error: %s\n\n", errno, strerror(errno));
    fflush(stdout);
    fflush(stderr);

    ::abort();
}

static void basic_linux_system_init_urandom() {
    bool seed_ok = false;
    const size_t buf_size = 512;
    static const char *DEV = "/dev/urandom";
    static const char *SEED = VTSS_FS_FLASH_DIR "/random-seed";
    struct rand_pool_info *ent;
    u32 *buf;
    u32 *p;
    size_t s;
    int fd = -1;


    if ((ent = (struct rand_pool_info*) VTSS_MALLOC(sizeof(*ent)+buf_size)) == NULL) {
        T_E("malloc fails: %zd bytes", sizeof(*ent)+buf_size);
        return;
    }
    buf = &ent->buf[0];

    // Read the seed from last time (if it exists)
    seed_ok = false;
    fd = open(SEED, O_RDONLY);
    if (fd != -1) {
        p = buf;
        s = buf_size;

        while (s) {
            int res = read(fd, buf, s);
            if (res == -1) {
                printf("URANDOM: Failed to read from %s: %s", SEED,
                       strerror(errno));
                break;
            } else if (res == 0) {
                // zero indicates end of file
                break;
            } else {
                p += res;
                s -= res;
            }
        }

        if (s == 0) {
            seed_ok = true;
        }

        close(fd);
    }

    // Write the seed into the urandom device
    if (seed_ok) {
        fd = open(DEV, O_WRONLY);
        if (fd != -1) {
            ent->buf_size = buf_size;
            ent->entropy_count = ent->buf_size << 3;    // 8 bits entropy per byte (100%)
            if(ioctl(fd, RNDADDENTROPY, ent)) {
                T_E("ioctl(%s): %s", "RNDADDENTROPY", strerror(errno));
            } else {
                T_I("Added %d bytes of entropy to %s", ent->entropy_count, DEV);
            }
            close(fd);
        } else {
            T_E("open(%s) fails: %s", DEV, strerror(errno));
        }
    }

    // Read a new seed from urandom
    seed_ok = false;
    fd = open(DEV, O_RDONLY);
    if (fd != -1) {
        p = buf;
        s = buf_size;

        while (s) {
            int res = read(fd, buf, s);
            if (res == -1) {
                printf("URANDOM: Failed to read from %s: %s", DEV,
                       strerror(errno));
                break;
            } else if (res == 0) {
                // zero indicates end of file
                break;
            } else {
                p += res;
                s -= res;
            }
        }

        if (s == 0) {
            seed_ok = true;
        }

        close(fd);
    } else {
        printf("URANDOM: Failed to open %s: %s\n", DEV, strerror(errno));
    }

    // Write the seed such that it will be used on the next boot
    if (seed_ok) {
        fd = open(SEED, O_WRONLY | O_CREAT | O_TRUNC);
        if (fd != -1) {
            p = buf;
            s = buf_size;

            while (s) {
                int res = write(fd, buf, s);
                if (res == -1) {
                    printf("URANDOM: Failed to write to %s: %s", SEED,
                           strerror(errno));
                    break;
                } else if (res == 0) {
                    // zero indicates end of file
                    break;
                } else {
                    p += res;
                    s -= res;
                }
            }

            close(fd);

        } else {
            printf("URANDOM: Failed to open %s: %s\n", SEED, strerror(errno));
        }
    }

    if (ent) {
        VTSS_FREE(ent);
    }
}

namespace vtss {
namespace parser {
struct MtdLine : public ParserBase {
    typedef str val;

    bool operator()(const char *&b, const char *e) {
        // Expect something like. The first line is a header which will fail
        // dev:    size   erasesize  name
        // mtd0: 00040000 00040000 "RedBoot"
        // mtd1: 00040000 00040000 "conf"
        // mtd2: 00100000 00040000 "stackconf"
        // mtd3: 00040000 00040000 "syslog"
        // mtd4: 00380000 00040000 "managed"
        // mtd5: 00a00000 00040000 "linux"
        // mtd6: 00040000 00040000 "FIS_directory"
        // mtd7: 00001000 00040000 "RedBoot_config"
        // mtd8: 00040000 00040000 "Redundant_FIS"
        // mtd9: 10000000 00020000 "rootfs_data"
        Lit sep(":");
        Lit mtd("mtd");
        OneOrMore<group::Space> sp;
        return Group(b, e, mtd, dev_, sep, sp, size_, sp, erase_size_, sp,
                     name_);
    }
    bool operator()(str s) {
        const char *b = s.begin();
        const char *e = s.end();
        return operator()(b, e);
    }

    val get() const { return name_.get(); }
    str name() const { return name_.get(); }
    uint32_t dev() const { return dev_.get(); }
    uint32_t size() const { return size_.get(); }
    uint32_t erase_size() const { return erase_size_.get(); }

    Int<uint32_t, 10, 1, 3> dev_;
    Int<uint32_t, 16, 1, 8> size_;
    Int<uint32_t, 16, 1, 8> erase_size_;
    StringRef name_;
};
}  // namespace parser

static bool mtd_device_by_name(const char *name, Buf *dev) {
    const int buf_size = 1024 * 1024;
    char buf[buf_size];

    int fd = open("/proc/mtd", O_RDONLY);
    if (fd == -1) {
        printf("Failed to open /proc/mtd: %s\n", strerror(errno));
        return false;
    }

    char *p = buf;
    int s = buf_size;

    while (s) {
        int res = read(fd, p, s);
        if (res == -1 || res == 0) {
            break;
        }

        s -= res;
        p += res;
    }
    close(fd);
    fd = -1;

    str data(buf, p);

    for (str line : LineIterator(data)) {
        vtss::parser::MtdLine mtd_line;
        if (mtd_line(line)) {
            if (mtd_line.name() == str(name)) {
                BufPtrStream o(dev);
                o << mtd_line.dev();
                o.cstring();
                return o.ok();
            }
        }
    }

    return false;
}
}  // namespace vtss

static int ubi_attach (int mtd_dev)
{
    int fd, ret;
    struct ubi_attach_req uatt;

    fd = open("/dev/ubi_ctrl", O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    memset(&uatt, 0, sizeof(uatt));
    uatt.mtd_num = mtd_dev;
    uatt.ubi_num = 0;

    ret = ioctl(fd, UBI_IOCATT, &uatt);

    close(fd);

    return ret;
}

static void basic_linux_system_mount_rw(void) {
    int res;
    vtss::SBuf16 mtd_dev;

    if (access(VTSS_FS_FLASH_DIR "/", R_OK | W_OK | X_OK | F_OK) == 0) {
        // Flash FS already there and read/writeable (booted on eCPU).
        printf("Using existing mount point for " VTSS_FS_FLASH_DIR "\n");
        return;
    }

    // TBD_FA_ECPU
    // The mdt dev in the case of LS1046a is not called "rootfs_data"
    // by default, but "NAND (RW) UBIFS Root File System"
    // I believe we need to update its name in the BSP.
    if (misc_cpu_is_external()) {
        goto MOUNT_TMP;
    }
    if (mtd_device_by_name("rootfs_data", &mtd_dev) ||
	mtd_device_by_name("rootfs_nand_data", &mtd_dev)) {
        if (ubi_attach(atoi(mtd_dev.begin())) != 0) {
            printf("\n\n");
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            printf("!! FAILED TO ATTACH PERSISTENT STORAGE\n");
            printf("!! Attach(/dev/mtd%s) failed: %s\n", mtd_dev.begin(), strerror(errno));
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            printf("\n\n");
            goto MOUNT_TMP;
        }
        res = mount("ubi0:switch", VTSS_FS_FLASH_DIR, "ubifs", MS_SYNCHRONOUS, 0);
        if (res == -1) {
            printf("\n\n");
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            printf("!! FAILED TO MOUNT PERSISTENT STORAGE\n");
            printf("!! MOUNT-ERROR: %s (%d)\n", strerror(errno), errno);
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            printf("\n\n");

            // Mount a temp filesystem instead such that we can get the switch
            // booted
            goto MOUNT_TMP;
        }

    } else {
        printf("\n\n");
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("!! FAILED TO MOUNT PERSISTENT STORAGE\n");
        printf("!! ERROR: COULD NOT FIND 'rootfs_data'/'rootfs_nand_data' partition\n");
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("\n\n");
        goto MOUNT_TMP;
    }

    return;

MOUNT_TMP:
    res = mount("tmpfs", VTSS_FS_FLASH_DIR, "tmpfs", MS_NOSUID | MS_NODEV,
                "size=8388608,mode=1777");

    if (res == -1) {
        printf("\n\n");
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("!! FAILED TO MOUNT RAM FILESYSTEM AS PERSISTENT STORAGE\n");
        printf("!! ERROR: %s (%d)\n", strerror(errno), errno);
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("\n\n");
        ::abort();
    }
}

static void main_early_init(void)
{
    vtss_init_data_t data;

    // Pump an INIT_CMD_EARLY_INIT around as the very, very first thing we do.
    // This allows every module to do something before everything else starts.
    memset(&data, 0, sizeof(data));
    data.cmd = INIT_CMD_EARLY_INIT;
    (void)init_modules(&data);
}

static void print_time(void)
{
    time_t     rawtime;
    struct tm *timeinfo;
    char       buffer[80];
    struct tm  timeinfo_r;

    time(&rawtime);
    timeinfo = localtime_r(&rawtime, &timeinfo_r);
    strftime(buffer, sizeof(buffer), "%X", timeinfo);
    printf("%s ", buffer);
}

static void set_time(void)
{
    // Set initial time to build time
    // This initialization is important to make web pages load fast.
    // Whenever a browser tries to load a html page, Hiawatha will compare date of the
    // web page with the date of the zipped cpopy from the cache. If the web page is newer
    // than the zipped version in the cache, the webpage will be zipped and copied to the
    // cache. However, web pages are time-stamped with the time they are built, so if the
    // clock of the system is set to January 1. 1970, then (as the zipped files are
    // time-stamped with the system time) the zip files will alway be older than the html
    // files so the html files have to zipped every over and over again time they are fetched.

    struct tm tm_buildtime;
    time_t build_time;
    timeval timeval_buildtime;
    strptime(misc_software_date_txt(), "%Y-%m-%dT%T", &tm_buildtime);
    tm_buildtime.tm_isdst = 0; //strptime does not set the tm_isdst field

    build_time = mktime(&tm_buildtime);

    if (build_time >= 0) {
        timeval_buildtime.tv_sec = build_time;
        timeval_buildtime.tv_usec = 0;
        settimeofday(&timeval_buildtime, NULL);
    }
}

int main(int argc, char *argv[]) {
    int ac;
    int opt;
    char **av;
    int force_init = 0;

    print_time();
    printf("Starting application...\n");

    set_time();

    // Take over various signals to allow us to print thread info if e.g. an
    // abort or segmentation fault occurs.
    // If AddressSanitiser is enabled in the build, this has to be skipped.
#if !defined(VTSS_SW_OPTION_ASAN)
    vtss_crashhandler_setup();
#endif

    restart_reason_init();

    critd_init(&init_modules_crit, "Init Modules", VTSS_MODULE_ID_MAIN, CRITD_TYPE_MUTEX_RECURSIVE);

    // It might take some time to load a worst-case configuration, so allow
    // indefinite lock of this mutex.
    init_modules_crit.max_lock_time = -1;

    ac = argc;
    av = argv;
    while ((opt = getopt(ac, av, "t:is:h")) != -1) {
        switch (opt) {
        case 't':
            // Trace is handled later (after the trace system is initialized)
            break;

        case 'i':
            force_init = 1;
            break;

        case 's': {
            // Use a particular SPI device for UIO
            int  spi_freq = 5000000;
            int  spi_pad  = 1;
            char spi_dev[512];
            char *s_pad, *s_freq;

            s_pad = strchr(optarg, '@');

            if (s_pad && s_pad[1]) {
                *s_pad = '\0';
                s_pad++;
                spi_pad = atoi(s_pad);

                s_freq = strchr(s_pad, '@');
                if (s_freq) {
                    *s_freq = '\0';
                    s_freq++;
                    spi_freq = atoi(s_freq);
                }
            }

            spi_dev[sizeof(spi_dev) - 1] = '\0';
            strncpy(spi_dev, optarg, sizeof(spi_dev));

            vtss_api_if_spi_reg_io_set(spi_dev, spi_pad, spi_freq);
        }

        break;

        case '?':
        case 'h': /* Help */
        default:
            printf("%s [-h] [-t <module>:<group>:<level>] [-i] [-s <spidev[@pad[@freq]]]\n", argv[0]);
            printf("-h: Show help\n");
            printf("-s: Select DUT register interface SPI device along with optional padding bytes and frequency. This is in place of the normal UIO device.\n");
            printf("-t: Show or set trace level for a specific module and group (multiple occurrences allowed)\n");
            printf("-i: Force system init\n");
            exit(0);
        }
    }

    // If PID == 1 (we are the first process then) or the command line asks us
    // to initialize the system, do so.
    if ((int)getpid() == 1 || force_init) {
        basic_linux_system_init();
    }

    // Pump an early init event out before doing anything else.
    main_early_init();

#if defined(VTSS_SW_OPTION_SYNCE_DPLL)
    // Check to see if we need to update DPLL firmware. If so, we don't start
    // any modules, but reboot after it's done.
    if (clock_dpll_fw_update(synce_dpll_err_str, sizeof(synce_dpll_err_str) - 1)) {
        control_system_reset_no_cb(MESA_RESTART_COOL);
    }
#endif

#if defined(VTSS_SW_OPTION_ZLS30387)
    zl_3038x_api_version_check();
#endif

#if !defined(VTSS_SW_OPTION_IPV6)
    // Disable IPv6
    int fd = -1;
    static const char *one = "1\n";
    static const char *zero = "0\n";
    fd = open("/proc/sys/net/ipv6/conf/all/autoconf", O_WRONLY);
    if (fd != -1) {
        write(fd, zero, strlen(zero));
        close(fd);
    }
    fd = open("/proc/sys/net/ipv6/conf/all/accept_ra", O_WRONLY);
    if (fd != -1) {
        write(fd, zero, strlen(zero));
        close(fd);
    }
    fd = open("/proc/sys/net/ipv6/conf/all/disable_ipv6", O_WRONLY);
    if (fd != -1) {
        write(fd, one, strlen(one));
        close(fd);
    }
#endif //!VTSS_SW_OPTION_IPV6

    // Mount rw storage
    basic_linux_system_mount_rw();

    // Load the random seed (if we have such) - must run after we have a rw
    // filesystem.
    basic_linux_system_init_urandom();

    // Creating various folders needed by the application
    (void)mkdir(VTSS_FS_FILE_DIR, 0777);
    (void)mkdir(VTSS_FS_RUN_DIR, 0777);
    (void)mkdir(USB_DEVICE_DIR, 0777);

    // Time to parse optional trace command line arguments
    ac = argc;
    av = argv;
    optind = 1; // Restart scanning of arguments
    while ((opt = getopt(ac, av, "t:is:h")) != -1) {
        switch (opt) {
        case 't': {
            int module, group, level;
            const char *s_module, *s_group, *s_level;
            char *saveptr;

            s_module = strtok_r(optarg, ":", &saveptr);
            s_group = strtok_r(NULL, ":", &saveptr);
            s_level = strtok_r(NULL, ":", &saveptr);

            if (s_module == NULL || s_group == NULL || s_level == NULL) {
                printf("Error: Illegal trace arg: use <module>:<group>:<level>\n");
                continue;
            }

            if (vtss_trace_module_name_to_id(s_module, &module) != MESA_RC_OK) {
                printf("Error: Illegal trace module: %s\n", s_module);
                continue;
            }

            if (vtss_trace_grp_name_to_id(s_group, module, &group) != MESA_RC_OK) {
                printf("Error: Illegal trace group for %s: %s\n", s_module, s_group);
                continue;
            }

            if (vtss_trace_lvl_to_val(s_level, &level) != MESA_RC_OK) {
                printf("Error: Illegal trace level for: %s - using 'debug'\n", s_level);
                level = VTSS_TRACE_LVL_DEBUG;
            }

            vtss_trace_module_lvl_set(module, group, level);
            vtss_api_trace_update();

            break;
        }
        }
    }

    vtss_init_data_t data;
    memset(&data, 0, sizeof(data));
    data.cmd = INIT_CMD_INIT;
    data.isid = VTSS_ISID_LOCAL;
    data.switch_info[VTSS_ISID_LOCAL].configurable = TRUE;

    init_modules(&data);

    // Wait until INIT_CMD_INIT is complete.
    msg_wait(MSG_WAIT_UNTIL_INIT_DONE, VTSS_MODULE_ID_MAIN);

    vtss_flag_init(&control_flags);
    vtss_recursive_mutex_init(&reset_mutex);

    for (;;) {
        vtss_flag_value_t flags;
        flags = vtss_flag_wait(&control_flags, 0xFF, VTSS_FLAG_WAITMODE_OR_CLR);
        if (flags & CTLFLAG_SYSTEM_RESET) {
            control_system_reset_sync(__restart);
            /* NOTREACHED */
        }
        if (flags & CTLFLAG_CONFIG_RESET) {
            control_config_reset(VTSS_USID_ALL, 0);
        } else if (flags & CTLFLAG_CONFIG_RESET_KEEPIP) {
            control_config_reset(VTSS_USID_ALL, INIT_CMD_PARM2_FLAGS_IP);
        }
    }

    return 0;
}

/*
 * Leaching - control API functions
 */

static void control_config_change_detect_set(BOOL enable)
{
    conf_mgmt_conf_t conf;

    if (conf_mgmt_conf_get(&conf) == VTSS_RC_OK) {
        conf.change_detect = enable;
        (void)conf_mgmt_conf_set(&conf);
    }
}

extern "C"
void control_config_reset(vtss_usid_t usid, vtss_flag_value_t flags)
{
    vtss_init_data_t data;
    vtss_isid_t      isid;

    memset(&data, 0, sizeof(data));
    data.cmd = INIT_CMD_CONF_DEF;
    data.flags = flags;

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        // Pretend that all switches are configurable. Only the fact
        // that a switch is not configurable in INIT_CMD_CONF_DEF
        // events must be used for anything.
        data.switch_info[isid].configurable = TRUE;
    }

    // We want all the calls to init_modules()
    // below to be non-intervenable.
    // Notice that the semaphore is recursive, so that
    // the same thread may take it several times. This
    // is used in this case, because the init_modules()
    // function itself also takes it.
    INIT_MODULES_CRIT_ENTER();

    /* For improved performance, configuration change detection is disabled
       during default creation */
    control_config_change_detect_set(0);

    if(msg_switch_is_primary()) {
        if(usid == VTSS_USID_ALL) {
            T_W("Resetting global configuration, flags 0x%x", flags);
            data.isid = VTSS_ISID_GLOBAL;
            init_modules(&data);
        }
        if(vtss_switch_stackable()) { /* Reset selected switches individually */
            static control_msg_t cfg_reset_message = { .msg_id = CONTROL_MSG_ID_CONFIG_RESET };
            vtss_usid_t i;
            cfg_reset_message.flags = flags;
            for(i = VTSS_USID_START; i < VTSS_USID_END; i++) {
                if(usid == VTSS_USID_ALL || i == usid) {
                    vtss_isid_t isid = topo_usid2isid(i);
                    T_W("Reset switch %d configuration", isid);
                    data.isid = isid;
                    init_modules(&data);
                    if(msg_switch_exists(isid)) {
                        T_W("Requesting local config reset, sid %d (%d)", i, isid);
                        msg_tx_adv(NULL, NULL, MSG_TX_OPT_DONT_FREE,
                                   VTSS_MODULE_ID_MAIN, isid,
                                   (const void *)&cfg_reset_message, sizeof(cfg_reset_message));
                    }
                }
            }
        } else {
            T_W("Reset switch %d configuration", VTSS_ISID_START);
            data.isid = VTSS_ISID_START;
            init_modules(&data);
        }
    }

    /* Local config reset */
    if(!vtss_switch_stackable() || !msg_switch_is_primary())  {
        T_W("Resetting local configuration, flags 0x%x", flags);
        data.isid = VTSS_ISID_LOCAL;
        init_modules(&data);
    }

    /* Enable configuration change detection again */
    control_config_change_detect_set(1);

    INIT_MODULES_CRIT_EXIT();

    T_I("Reset configuration done");
}

void control_config_reset_async(vtss_usid_t usid, vtss_flag_value_t flags)
{
    if (flags & INIT_CMD_PARM2_FLAGS_IP) {
        vtss_flag_setbits(&control_flags, CTLFLAG_CONFIG_RESET_KEEPIP);
    } else {
        vtss_flag_setbits(&control_flags, CTLFLAG_CONFIG_RESET);
    }
}

static void control_system_reset_callback(mesa_restart_t restart)
{
    for (auto i = callback_list.begin(); i != callback_list.end(); ++i) {
        T_D("Reboot callback %s enter (@%p)", vtss_module_names[i->module_id], i->cb);
        i->cb(restart);
        T_D("Reboot callback %s return", vtss_module_names[i->module_id]);
    }
}

static void cool_restart(void)
{
#if defined(MSCC_BRSDK)
    // Ask the HAL layer to perform the cool restart.
    // It takes one parameter, which is the address of a register
    // in the switch core domain that must survive the reset.
#if defined(VTSS_DEVCPU_GCB_CHIP_REGS_GENERAL_PURPOSE)
    hal_cool_restart((u32 *)&VTSS_DEVCPU_GCB_CHIP_REGS_GENERAL_PURPOSE);
#elif defined(VTSS_DEVCPU_GCB_CHIP_REGS_GPR)
    hal_cool_restart((u32 *)&VTSS_DEVCPU_GCB_CHIP_REGS_GPR);
#else
    restart_reason_set(MESA_RESTART_COOL);
#endif
#endif
    sync();
    sync();
    umount("/switch");
    /*
     * AA-11113: On certain devices (Luton10 & Serval-T) the reboot below
     * would emit SPI error messages from the Flash device. Apparently it
     * helps to delay a few seconds here. Why this is so is unknown but
     * the Linux man pages for the sync() call says this:
     *
     * "On Linux, sync is only guaranteed to schedule the dirty blocks for writing;
     * it can actually take a short time before all the blocks are finally written.
     * The reboot(8) and halt(8) commands take this into account by sleeping for a
     * few seconds after calling sync(2)."
     */

    sleep(2);

    // Die...
    killpg(getpgrp(), SIGKILL);
    exit(0);
}

static void control_system_reset_do_sync(mesa_restart_t restart, BOOL bypass_callback)
{
    // Let those modules that are interested in this know about the upcoming restart.
    // In some situations, this is not possible and could cause a deadlock or
    // a recursive call to this function (through VTSS_ASSERT()). In such situations,
    // we bypass both callbacks and grabbing the reset lock (since that also might
    // cause a deadlock).
    if (!bypass_callback) {
        if (__restart_callback_wait_msec) {
            // Sleep this amount of milliseconds before starting the restart
            // callback sequence. This is useful if a module that invokes the
            // rebool function needs to e.g. send a frame before e.g. the port
            // module shuts down the ports.
            T_D("Sleeping %u msecs before calling back reboot handlers", __restart_callback_wait_msec);
            VTSS_MSLEEP(__restart_callback_wait_msec);
        } else {
            T_D("Skipping sleep prior to calling back reboot handlers");
        }

        control_system_reset_callback(restart);
         /* Boost current holder of lock - if any */
        vtss_thread_prio_set(vtss_thread_self(), VTSS_THREAD_PRIO_HIGHER);
        VTSS_OS_MSLEEP(1000);
    }

    // Tell the method to the API.
    mesa_restart_conf_set(NULL, restart);

    switch (restart) {
    case MESA_RESTART_COOL: {
        cool_restart();
        break;
      }

    default:
        // If we get here, the reboot method is not supported on this platform.
        T_E("Unsupported reboot method: %s", control_system_restart_to_str(restart));
        break;
    }

    T_E("Reset(%d) trying to return", restart);
    VTSS_ASSERT(0);             /* Don't return */
}

void control_system_reset_sync(mesa_restart_t restart)
{
    control_system_reset_do_sync(restart, FALSE);
    exit(1);
}

void control_system_reset_no_cb(mesa_restart_t restart)
{
    control_system_reset_do_sync(restart, TRUE);
    exit(1);
}

void control_system_assert_do_reset(void)
{
    vtss_backtrace(printf, 0);



    assert(0); // Causes a SIGABRT, handled by crashhandler.cxx#vtss_crashhandler_signal() and a subsequent reboot.

    while(1); /* Don't return */
}

int control_system_reset(BOOL local, vtss_usid_t usid, mesa_restart_t restart, uint32_t wait_before_callback_msec)
{
    /* Force imediate configuration flush */
    conf_flush();

    control_system_flash_lock();
    /* once lock, never return */
    restart_reason_set(restart, wait_before_callback_msec);
    printf("Rebooting system...\n");
    vtss_thread_yield();
    vtss_flag_setbits(&control_flags, CTLFLAG_SYSTEM_RESET);
    control_system_flash_unlock();

    return 0;
}

void control_system_flash_lock(void)
{
    vtss_recursive_mutex_lock(&reset_mutex);
}

void control_system_flash_unlock(void)
{
    (void)vtss_recursive_mutex_unlock(&reset_mutex);
}

void control_system_reset_register(control_system_reset_callback_t cb, vtss_module_id_t module_id, control_system_reset_priority_t prio)
{
    control_system_reset_set_t set;

    set.cb        = cb;
    set.prio      = prio;
    set.module_id = module_id;

    vtss_global_lock(__FILE__, __LINE__);
    // Insert using control_system_reset_set_t::operator<()
    callback_list.insert(set);
    T_D("Inserted %s (@%p) with priority %d in reset callback list. New number of registrants = %zu", vtss_module_names[module_id], cb, prio, callback_list.size());
    vtss_global_unlock(__FILE__, __LINE__);
}

/****************************************************************************/
/****************************************************************************/
int control_flash_erase(vtss_flashaddr_t base, size_t len)
{
  vtss_flashaddr_t err_address;
  int err;
  control_system_flash_lock(); // Prevent system from resetting and this function from being re-entrant
  err = vtss_flash_erase(base, len, &err_address);
  control_system_flash_unlock();
  if(err != VTSS_FLASH_ERR_OK) {
      T_E("An error occurred when attempting to erase " VPRIz " bytes at %p @ %p: %s", len, (void *)base, (void *)err_address, vtss_flash_errmsg(err));
  }
  return err;
}

/****************************************************************************/
/****************************************************************************/
int control_flash_program(vtss_flashaddr_t flash_base, const void *ram_base, size_t len)
{
  vtss_flashaddr_t err_address;
  int err;
  control_system_flash_lock(); // Prevent system from resetting and this function from being re-entrant
  err = vtss_flash_program(flash_base, ram_base, len, &err_address);
  control_system_flash_unlock();
  if(err != VTSS_FLASH_ERR_OK) {
      T_E("An error occurred when attempting to program " VPRIz " bytes to %p @ %p: %s", len, (void *)flash_base, (void *)err_address, vtss_flash_errmsg(err));
  }
  return err;
}

/****************************************************************************/
/****************************************************************************/
int control_flash_read(vtss_flashaddr_t flash_base, void *dest, size_t len)
{
  vtss_flashaddr_t err_address;
  int err;
  control_system_flash_lock(); // Prevent system from resetting and this function from being re-entrant
  err = vtss_flash_read(flash_base, dest, len, &err_address);
  control_system_flash_unlock();
  if(err != VTSS_FLASH_ERR_OK) {
      T_E("An error occurred when attempting to read " VPRIz " bytes from %p @ %p: %s", len, (void *)flash_base, (void *)err_address, vtss_flash_errmsg(err));
  }
  return err;
}


/****************************************************************************/
/****************************************************************************/
void control_dbg_latest_init_modules_get(vtss_init_data_t *data, const char **init_module_func_name)
{
    *data  = dbg_latest_init_modules_data;
    *init_module_func_name = initfun[dbg_latest_init_modules_init_fun_idx].name;
}

const char *control_init_cmd2str(init_cmd_t cmd)
{
    switch (cmd) {
    case INIT_CMD_EARLY_INIT:
        return "EARLY_INIT";
    case INIT_CMD_INIT:
        return "INIT";
    case INIT_CMD_START:
        return "START";
    case INIT_CMD_CONF_DEF:
        return "CONF DEFAULT";
    case INIT_CMD_ICFG_LOADING_PRE:
        return "ICFG Loading Pre";
    case INIT_CMD_ICFG_LOADING_POST:
        return "ICFG Loading Post";
    case INIT_CMD_ICFG_LOADED_POST:
        return "ICFG Loaded Post";
    case INIT_CMD_SUSPEND_RESUME:
        return "SUSPEND/RESUME";
    case INIT_CMD_WARMSTART_QUERY:
        return "WARMSTART QUERY";
    default:
        return "UNKNOWN COMMAND";
    }
}

/****************************************************************************/
/****************************************************************************/
void control_dbg_init_modules_callback_time_max_print(msg_dbg_printf_t dbg_printf, BOOL clear)
{
    int i;
    if (!clear) {
        dbg_printf("Init Modules Callback           Max time [ms] Callback type\n");
        dbg_printf("------------------------------- ------------- ---------------\n");
    }
    for (i = 0; i < sizeof(initfun) / sizeof(initfun[i]); i++) {
        if (clear) {
            initfun[i].max_callback_ticks = 0;
            initfun[i].max_callback_cmd   = (init_cmd_t)0;
        } else {
            dbg_printf("%-31s " VPRI64Fu("13") " %-15s\n", initfun[i].name, VTSS_OS_TICK2MSEC(initfun[i].max_callback_ticks), control_init_cmd2str(initfun[i].max_callback_cmd));
        }
    }

    if (clear) {
        dbg_printf("Init Modules callback statistics cleared\n");
    }
}

/* - Assertions ----------------------------------------------------- */

vtss_common_assert_cb_t vtss_common_assert_cb=NULL;

void vtss_common_assert_cb_set(vtss_common_assert_cb_t cb)
{
    VTSS_ASSERT(vtss_common_assert_cb == NULL);
    vtss_common_assert_cb = cb;
}

void cap_array_check_dim(size_t idx, size_t max) {
    if (idx > max) {
        printf("Array out of range! %zd > %zd\n", idx, max);
        VTSS_ASSERT(0);
    }
}

vtss::ostream& operator<<(vtss::ostream& o, init_cmd_t c) {
    switch (c) {
        case INIT_CMD_EARLY_INIT:
            o << "EARLY_INIT";
            return o;

        case INIT_CMD_INIT:
            o << "INIT";
            return o;

        case INIT_CMD_START:
            o << "START";
            return o;

        case INIT_CMD_CONF_DEF:
            o << "CONF_DEF";
            return o;

        case INIT_CMD_ICFG_LOADING_PRE:
            o << "ICFG_LOADING_PRE";
            return o;

        case INIT_CMD_OBSOLETE:
            o << "OBSOLETE";
            return o;

        case INIT_CMD_ICFG_LOADING_POST:
            o << "ICFG_LOADING_POST";
            return o;

        case INIT_CMD_ICFG_LOADED_POST:
            o << "ICFG_LOADED_POST";
            return o;

        case INIT_CMD_OBSOLETE_2:
            o << "OBSOLETE_2";
            return o;

        case INIT_CMD_SUSPEND_RESUME:
            o << "SUSPEND_RESUME";
            return o;

        case INIT_CMD_WARMSTART_QUERY:
            o << "WARMSTART_QUERY";
            return o;
    }

    o << "<UNKNOWN: " << (int)c << ">";
    return o;
}

uint32_t vtss_appl_port_cnt(mesa_inst_t inst) {
    return fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);
}

