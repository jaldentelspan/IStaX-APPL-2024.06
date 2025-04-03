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

// These are used unprotected in calls to vtss_flag_maskbits() and vtss_flag_timed_wait(), only
/*lint -esym(459, lldp_if_crit)                      */
#include "vtss_common_os.h"
#include "critd_api.h"
#include "main.h"
#include "msg_api.h"
#include "vtss_lldp.h"
#ifdef VTSS_SW_OPTION_IP
#include "ip_utils.hxx"
#endif
#include "microchip/ethernet/switch/api.h"
#include "led_api.h"
#include "lldp_api.h"
#include "control_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#include "misc_api.h"
#include "sysutil_api.h"
#include "lldp_trace.h"
#include "lldp_remote.h"  // For lldp_remote_table_init
#include "qos_api.h"

#ifdef VTSS_SW_OPTION_PACKET
#include "packet_api.h"
#endif
#ifdef VTSS_SW_OPTION_IP
#include "ip_api.h"
#endif

#include "vlan_api.h" // For VTSS_APPL_VLAN_ID_Mxx

#ifdef VTSS_SW_OPTION_CDP
#include "cdp_analyse.h"
#endif

#if defined(VTSS_SW_OPTION_SNMP)
#include "dot1ab_lldp_api.h"
#endif /* VTSS_SW_OPTION_SNMP */

#include "vtss/appl/interface.h"
#ifdef VTSS_SW_OPTION_LLDP_MED
#include "lldpmed_rx.h"
#include "lldpmed_tx.h"

#ifdef VTSS_SW_OPTION_SECURE_SNMP
#include "lldpXMedMIB_api.h"
#endif
#endif

#ifdef VTSS_SW_OPTION_VOICE_VLAN
#include "voice_vlan_api.h"
#endif

#ifdef VTSS_SW_OPTION_ICFG
#include "lldp_icli_functions.h" // For lldp_icfg_init
#ifdef VTSS_SW_OPTION_LLDP_MED
#include "lldpmed_icli_functions.h" // For lldpmed_icfg_init
#endif
#endif /* VTSS_SW_OPTION_ICFG */

#ifdef VTSS_SW_OPTION_TSN
#include "vtss/appl/tsn.h"
#endif


#include "mgmt_api.h"

/* Critical region protection protecting the following block of variables */
static critd_t lldp_crit;
static critd_t lldp_if_crit;

//****************************************
// TRACE
//****************************************
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "lldp", "Link Layer Discovery Protocol"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_STAT] = {
        "stat",
        "Statistics",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_CONF] = {
        "conf",
        "LLDP configuration",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_SNMP] = {
        "snmp",
        "LLDP SNMP",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_TX] = {
        "tx",
        "LLDP Transmit",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_RX] = {
        "rx",
        "LLDP RX",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_POE] = {
        "poe",
        "LLDP PoE",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_CDP] = {
        "cdp",
        "Cisco Discovery Protocol",
        VTSS_TRACE_LVL_ERROR
    },
    [TRACE_GRP_CLI] = {
        "cli",
        "Command line interface",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_EEE] = {
        "EEE",
        "Energy Efficient Ethernet",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_MED_TX] = {
        "med_tx",
        "LLDP-MED Transmit",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_MED_RX] = {
        "med_rx",
        "LLDP-MED Rx",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_FP] = {
        "FP",
        "Frame Preemption",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_FP_TX] = {
        "fp_tx",
        "LLDP-FP Tx",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_FP_RX] = {
        "fp_rx",
        "LLDP-FP Rx",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define LLDP_CRIT_ENTER()            critd_enter(        &lldp_crit,    __FILE__, __LINE__)
#define LLDP_CRIT_EXIT()             critd_exit(         &lldp_crit,    __FILE__, __LINE__)
#define LLDP_CRIT_ASSERT_LOCKED()    critd_assert_locked(&lldp_crit,    __FILE__, __LINE__)
#define LLDP_IF_CRIT_ENTER()         critd_enter(        &lldp_if_crit, __FILE__, __LINE__)
#define LLDP_IF_CRIT_EXIT()          critd_exit(         &lldp_if_crit, __FILE__, __LINE__)
#define LLDP_IF_CRIT_ASSERT_LOCKED() critd_assert_locked(&lldp_if_crit, __FILE__, __LINE__)

// Define timeout for secondary switch to reply upon requests to 15 secs
#define LLDP_REQ_TIMEOUT 15

/* Configuration */
typedef struct {
    lldp_user_conf_t        user;     // Configuration that is user controlled
    lldp_conf_run_time_t    run_time; // Settings that are found at runtime
} lldp_conf_t;

//***********************************************
// MISC
//***********************************************
/* Thread variables */
static vtss_handle_t lldp_thread_handle;
static vtss_thread_t lldp_thread_block;

static lldp_conf_t  lldp_conf;                         // Current configuration for this switch

//************************************************
// Variables
//************************************************
static lldp_bool_t system_conf_has_changed = 1; // Set by callback function if systemname or ip address changes.
static time_t last_entry_change_time_this_switch  = 0;

//
// Converts error to printable text
//
// In : rc - The error type
//
// Retrun : Error text
//
const char *lldp_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_LLDP_ERROR_VOICE_VLAN_CONFIGURATION_MISMATCH:
        return "Cannot set LLDP port mode to disabled or TX only when Voice-VLAN supports LLDP discovery protocol.";

    case VTSS_APPL_LLDP_ERROR_NOTIFICATION_VALUE:
        return "Notification enable must be either 1 (enable) or 2 (disable)";

    case VTSS_APPL_LLDP_ERROR_NOTIFICATION_INTERVAL_VALUE:
        return "Notification interval value must be between 5 and 3600";

    case VTSS_APPL_LLDP_ERROR_MSG:
        return "Internal message issue";

    case VTSS_APPL_LLDP_ERROR_TX_DELAY:
        return "txDelay must not be larger than 0.25 * TxInterval - IEEE 802.1AB-clause 10.5.4.2 - Configuration ignored";

    case VTSS_APPL_LLDP_ERROR_NULL_POINTER:
        return "Unexpected reference to NULL pointer";

    case VTSS_APPL_LLDP_ERROR_REINIT_VALUE:
        return "Re-init is out of range";

    case VTSS_APPL_LLDP_ERROR_TX_HOLD_VALUE:
        return "TX-Hold is out of range";

    case VTSS_APPL_LLDP_ERROR_TX_INTERVAL_VALUE:
        return "TX-Interval is out of range";

    case VTSS_APPL_LLDP_ERROR_TX_DELAY_VALUE:
        return "TX-Delay is out of range";

    case VTSS_APPL_LLDP_ERROR_FAST_START_REPEAT_COUNT:
        return "Fast Start Repeat Count is out of range";

    case VTSS_APPL_LLDP_ERROR_LATITUDE_OUT_OF_RANGE:
        return "Latitude is out of range. Must be in the range 0.0000 to 90.0000";

    case VTSS_APPL_LLDP_ERROR_LONGITUDE_OUT_OF_RANGE:
        return "Longitude is out of range. Must be in the range 0.0000 to 180.0000";

    case VTSS_APPL_LLDP_ERROR_ALTITUDE_OUT_OF_RANGE:
        return "Altitude is out of range. Must be in the range Must be in the range -2097151.9 to 2097151.9";

    case VTSS_APPL_LLDP_ERROR_COUNTRY_CODE_LETTER_SIZE:
        return "Country code MUST be two letters."; // From TIA1057, ANNEX B. - country code: The two-letter ISO 3166 country code in capital ASCII letters, e.g., DE or US*;

    case VTSS_APPL_LLDP_ERROR_UNSUPPORTED_OPTIONAL_TLVS_BITS:
        return "Trying to set optional TLVs bit(s) which is not supported";

    case VTSS_APPL_LLDP_ERROR_COUNTRY_CODE_LETTER:
        return "Country code contains non-valid letter(s) or is not capitalized."; // From TIA1057, ANNEX B. - country code: The two-letter ISO 3166 country code in capital ASCII letters, e.g., DE or US*;

    case VTSS_APPL_LLDP_ERROR_ELIN_SIZE:
        return "ECS ELIN must maximum be 25 character long."; // TIA1057, Figure 11

    case VTSS_APPL_LLDP_ERROR_ELIN:
        return "ECS ELIN must be a numerical string"; // TIA1057, Figure 11

    case VTSS_APPL_LLDP_ERROR_ENTRY_INDEX:
        return "Entry with the specific index/interface is not in use";

    case VTSS_APPL_LLDP_ERROR_CIVIC_EXCEED:
        return "The amount of CIVIC information is exceeded";

    case VTSS_APPL_LLDP_ERROR_CIVIC_TYPE:
        return "Invalid Civic address type";

    case VTSS_APPL_LLDP_ERROR_IFINDEX:
        return "Interface index (ifindex) is not a port index";

    case VTSS_APPL_LLDP_ERROR_POLICY_OUT_OF_RANGE:
        return "Policy index is out of range";

    case VTSS_APPL_LLDP_ERROR_POE_NOT_VALID:
        return "PoE Information is not valid";

    case VTSS_APPL_LLDP_ERROR_POLICY_VLAN_OUT_OF_RANGE:
        return "Invalid VLAN value for the policy";

    case VTSS_APPL_LLDP_ERROR_POLICY_PRIO_OUT_OF_RANGE:
        return "Invalid priority value for the policy";

    case VTSS_APPL_LLDP_ERROR_POLICY_DSCP_OUT_OF_RANGE:
        return "Invalid DSCP value for the policy";

    case VTSS_APPL_LLDP_ERROR_POLICY_NOT_DEFINED:
        return "The policy requested is not created";

    case VTSS_APPL_LLDP_ERROR_MGMT_ADDR_NOT_FOUND:
        return "No management address with the given index found in the entry table";

    case VTSS_APPL_LLDP_ERROR_MGMT_ADDR_INDEX_INVALID:
        return "The management address index is invalid";

    case VTSS_RC_OK:
        return "";
    }

    T_I("rc:%d", rc);
    return "Unknown LLDP error";
}

#ifdef VTSS_SW_OPTION_CDP
// Because the CDP frame is a special multicast frame that the chip doesn't know
// how to handle, we need to add the MAC address to the MAC table manually.
// The hash entry to the MAC table consists of both the DMAC and VLAN VID.
// Since the port VLAN ID can be changed by the user this function MUST be called
// every time the port VLAN VID changes.
static void cdp_add_to_mac_table(vtss_isid_t isid /* unused */, mesa_port_no_t port_no, const vtss_appl_vlan_port_detailed_conf_t *vlan_conf)
{
    static CapArray<mesa_mac_table_entry_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> table;
    const mesa_mac_t              vtss_cdp_slowmac = {{0x01, 0x00, 0x0C, 0xCC, 0xCC, 0xCC}};
    mesa_mac_table_entry_t        *entry;
    vtss_appl_vlan_port_conf_t    conf;
    port_iter_t                   pit;
    mesa_port_no_t                port_index = port_no - VTSS_PORT_NO_START;

    if (port_index < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
        LLDP_CRIT_ENTER();
        mesa_vid_t  pvid;   // Port VLAN ID for the port that has changed configuration
        lldp_bool_t remove_entry_from_mac_table = TRUE;

        // Get the PVID for the port that has changed configuration
        (void)vlan_mgmt_port_conf_get(VTSS_ISID_LOCAL, port_no, &conf, VTSS_APPL_VLAN_USER_ALL, TRUE);
        pvid = conf.hybrid.pvid;

        // We need to make sure that no other ports uses the same PVID before
        // we can remove the entry from the mac table.
        entry = &table[port_index];
        (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);
        while (port_iter_getnext(&pit)) {
            (void)vlan_mgmt_port_conf_get(VTSS_ISID_LOCAL, pit.iport, &conf, VTSS_APPL_VLAN_USER_ALL, TRUE);
            if (conf.hybrid.pvid == entry->vid_mac.vid && lldp_conf.user.port[pit.iport - VTSS_PORT_NO_START].cdp_aware) {
                T_RG(TRACE_GRP_CDP, "Another port uses the same PVID (%u) - Leaving entry in mac table", entry->vid_mac.vid);
                remove_entry_from_mac_table = FALSE; // There is another port that uses the same PVID, so we can not remove the entry.
            }
        }

        // Do the remove of the entry.
        if (remove_entry_from_mac_table) {
            T_NG(TRACE_GRP_CDP, "removing mac entry");
            (void)mesa_mac_table_del(NULL, &entry->vid_mac);
        }

        if (lldp_conf.user.port[port_index].cdp_aware) {
            // Add the new entry to the mac table
            T_DG_PORT(TRACE_GRP_CDP, port_index, "Adding CDP to MAC table, pvid = %u", pvid);
            vtss_clear(*entry);
            entry->vid_mac.vid = pvid;
            entry->vid_mac.mac = vtss_cdp_slowmac;
            entry->copy_to_cpu = 1;
            entry->cpu_queue = PACKET_XTR_QU_MAC;
            entry->locked = 1;
            (void)mesa_mac_table_add(NULL, entry);
        }
        LLDP_CRIT_EXIT();
    }
}
#endif /* VTSS_SW_OPTION_CDP */

#if defined(VTSS_SW_OPTION_IP)
// Function for getting the IP address for a specific port.
// NOTE: No such function exists. You get a IP address on a specific VLAN.
// IN : iport - Port in question
// Return - IPv4 address.
mesa_ipv4_t lldp_ip_addr_get(mesa_port_no_t iport)
{
    vtss_appl_ip_if_status_t ipv4;
    vtss_ifindex_t           prev_ifindex, ifindex;

    LLDP_CRIT_ASSERT_LOCKED();

    // This is a little bit clumsy, because this function is always called with
    // a mutex taken, and we don't want to be within the mutex lock when
    // accessing the IP module (in order not to get involved with any deadlock
    // within that module), so we exit the mutex and re-enter afterwards.
    LLDP_CRIT_EXIT();

    prev_ifindex = VTSS_IFINDEX_NONE;
    while (vtss_appl_ip_if_itr(&prev_ifindex, &ifindex) == VTSS_RC_OK) {
        prev_ifindex = ifindex;

        if (vtss_appl_ip_if_status_get(ifindex, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 1, nullptr, &ipv4) == VTSS_RC_OK) {
            T_D_PORT(iport, "Got IP:0x%X", ipv4.u.ipv4.net.address);
            LLDP_CRIT_ENTER();
            return ipv4.u.ipv4.net.address;
        }
    }

    T_I_PORT(iport, "Setting IP to %u", 0);
    LLDP_CRIT_ENTER();

    return 0;
}
#else
mesa_ipv4_t lldp_ip_addr_get(mesa_port_no_t iport)
{
    return 0;
}
#endif // defined(VTSS_SW_OPTION_IP)

void lldp_something_has_changed(void)
{
    system_conf_has_changed = 1;
}

// Geting the primary switch's MAC address
vtss_common_macaddr_t lldp_os_get_primary_switch_mac(void)
{
    return lldp_conf.run_time.mac_addr;
}


// Function that checks if configuration has changed and call for an update of all switches. This function shall only be called on a primary switch.
static void chk_for_conf_change(void)
{
    T_RG(TRACE_GRP_CONF, "Enter");

    // Update all switches id configuration has changed
    vtss_common_macaddr_t current_mac_addr;
    system_conf_t         sys_conf;

    if (system_conf_has_changed) {
        // Updated MAC address
        vtss_os_get_systemmac(&current_mac_addr);
        T_D("MAC %02x-%02x-%02x-%02x-%02x-%02x ", current_mac_addr.macaddr[0], current_mac_addr.macaddr[1], current_mac_addr.macaddr[2], current_mac_addr.macaddr[3], current_mac_addr.macaddr[4], current_mac_addr.macaddr[5]);

        LLDP_CRIT_ENTER(); // Protect lldp_conf
        lldp_conf.run_time.mac_addr = current_mac_addr; // update the primary switch mac address

        // Update system name to the secondary switches (we don't know, but it could have been)
        if (system_get_config(&sys_conf) != VTSS_RC_OK) {
            T_E("Didn't get the system configuration");
            static char sys_unkn[] = "Unknown system name";
            lldp_system_name(sys_unkn, 0); // Set a default system name (should never happen).
        } else {
            lldp_system_name(&sys_conf.sys_name[0], 0); // Copy the primary switch's system name to a local copy
            lldp_something_changed_local();
            T_D(" Setting system name to %s", &sys_conf.sys_name[0]);
        }
        system_conf_has_changed = 0;
        LLDP_CRIT_EXIT();
    }
}

void lldp_system_name(char *sys_name, char get_name)
{
    static char local_system_name[VTSS_SYS_STRING_LEN]; // Local copy of the system name, sent by primary switch to secondary switches.

    if (get_name) {
        strcpy(sys_name, local_system_name);
    } else {
        // Got new name from primary switch - update the local copy.
        misc_strncpyz(local_system_name, sys_name, VTSS_SYS_STRING_LEN);
    }
    T_N("set_name = %d, local_system_name = %s, sys_name  = %s", get_name, local_system_name, sys_name);
}

static BOOL lldp_port_is_authorized(lldp_port_t port_idx)
{
    mesa_auth_state_t auth_state;
    (void)mesa_auth_port_state_get(NULL, port_idx, &auth_state);
    return auth_state == MESA_AUTH_STATE_BOTH;
}

//
// LLDP call back function that is called when a lldp frame is received
//
static lldp_bool_t lldp_rx_pkt(void  *contxt,
                               const lldp_u8_t *const frm_p,
                               const mesa_packet_rx_info_t *const rx_info)
{
    T_RG(TRACE_GRP_RX, "src_port:%u, len:%u", rx_info->port_no, rx_info->length);
    if (lldp_port_is_authorized(rx_info->port_no)) {
        T_RG(TRACE_GRP_RX, "lldp_port_is_authorized");
        LLDP_CRIT_ENTER();
        lldp_frame_received(rx_info->port_no - VTSS_PORT_NO_START, frm_p, rx_info->length, VTSS_GLAG_NO_NONE); // Calls same procedure as lldp_1sec_timer_tick
        T_RG(TRACE_GRP_RX, "Done src_port:%u, len:%u", rx_info->port_no, rx_info->length);
        LLDP_CRIT_EXIT();
    }
    T_RG(TRACE_GRP_RX, "Done src_port:%u, len:%u", rx_info->port_no, rx_info->length);
    return 0; // Allow other subscribers to receive the packet
}

//
// Call back function that is called when a CDP frame is received
//
#ifdef VTSS_SW_OPTION_CDP
static lldp_bool_t cdp_rx_pkt(void  *contxt,
                              const uchar *const frm_p,
                              const mesa_packet_rx_info_t *const rx_info)
{
    lldp_bool_t eat_frm = 0; // Allow other subscribers to receive the packet

    if (!lldp_port_is_authorized(rx_info->port_no)) {
        return eat_frm;
    }

    LLDP_CRIT_ENTER();
    T_NG(TRACE_GRP_CDP, "Got CDP frame");

    // PID shall be 0x2000 for CDP frame - See http://wiki.wireshark.org/CDP
    if (*(frm_p + 20) == 0x20 && *(frm_p + 21) == 0x00) {
        if (lldp_conf.user.port[rx_info->port_no - VTSS_PORT_NO_START].cdp_aware) {
            // Calls same procedure as lldp_1sec_timer_tick
            if (cdp_frame_decode(rx_info->port_no - VTSS_PORT_NO_START, frm_p, rx_info->length) != VTSS_RC_OK) {
                lldp_bad_lldpdu (rx_info->port_no);
            };
        }
    }
    LLDP_CRIT_EXIT();
    return eat_frm;
}
#endif // VTSS_SW_OPTION_CDP

//
// Procedure for subscribing for LLDP packets.
//
static void lldp_pkt_subscribe(void)
{
    packet_rx_filter_t rx_filter;
    static void *filter_id = NULL;
    const vtss_common_macaddr_t vtss_lldp_slowmac = {{0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E}};

    /* LLDP frames registration */
    packet_rx_filter_init(&rx_filter);
    rx_filter.modid = VTSS_MODULE_ID_LLDP;
    rx_filter.match = PACKET_RX_FILTER_MATCH_DMAC | PACKET_RX_FILTER_MATCH_ETYPE;
    rx_filter.cb    = lldp_rx_pkt;

    memcpy(rx_filter.dmac, vtss_lldp_slowmac.macaddr, sizeof(rx_filter.dmac));
    rx_filter.etype       = 0x88CC;
    rx_filter.prio        = PACKET_RX_FILTER_PRIO_NORMAL;
    rx_filter.thread_prio = VTSS_THREAD_PRIO_BELOW_NORMAL;

    if (packet_rx_filter_register(&rx_filter, &filter_id) != VTSS_RC_OK) {
        T_E("Not possible to register for LLDP packets. LLDP will not work !");
    }
}

#ifdef VTSS_SW_OPTION_CDP
// Subscribe for CDP frames from the packet module (the CDP frame must be added top
// to the MAC table for this to work, see the cdp_add_to_mac_table function).
static void cdp_pkt_subscribe(void)
{
    packet_rx_filter_t rx_filter;
    static void *filter_id = NULL;

    /* CDP frames registration */
    const mesa_mac_t vtss_cdp_slowmac = {{0x01, 0x00, 0x0C, 0xCC, 0xCC, 0xCC}};
    T_DG(TRACE_GRP_CDP, "Registering CDP frames");
    packet_rx_filter_init(&rx_filter);
    rx_filter.modid = VTSS_MODULE_ID_LLDP;
    rx_filter.match = PACKET_RX_FILTER_MATCH_DMAC;
    rx_filter.cb    = cdp_rx_pkt; // Setup callback function

    memcpy(rx_filter.dmac, vtss_cdp_slowmac.addr, sizeof(rx_filter.dmac));
    rx_filter.prio        = PACKET_RX_FILTER_PRIO_NORMAL;
    rx_filter.thread_prio = VTSS_THREAD_PRIO_BELOW_NORMAL;

    if (packet_rx_filter_register(&rx_filter, &filter_id) != VTSS_RC_OK) {
        T_E("Not possible to register for CDP packets. CDP will not work !");
    }

    // For getting the CDP frames to the CPU we need to add the CDP DMAC to the MAC table. The MAC
    // table needs to be updated every time the PVID changes, so we registers a callback function
    // for getting informed every time the VLAN port configuration changes.
    vlan_port_conf_change_register(VTSS_MODULE_ID_LLDP, cdp_add_to_mac_table, FALSE /* get called back on local switch after the change has occurred */);
}
#endif // VTSS_SW_OPTION_CDP

//
// Function called when booting
//
static void lldp_start(void)
{
    T_R("Entering lldp_start");
    lldp_pkt_subscribe(); // Subscribe for dicovery packets (LLDP)
#ifdef VTSS_SW_OPTION_CDP
    cdp_pkt_subscribe(); // Subscribe for dicovery packets (CDP)
#endif // VTSS_SW_OPTION_CDP
    T_R("Exiting lldp_start");
}

/****************************************************************************
* return true in case platform supports frame preemption feature
****************************************************************************/
BOOL lldp_is_frame_preemption_supported()
{
#ifdef VTSS_SW_OPTION_TSN
    mesa_rc                      rc;
    vtss_appl_tsn_fp_status_t    tsn_fp_status;
    vtss_ifindex_t               if_index;

    /* get Frame Preemption support status */
    rc = vtss_ifindex_from_port(VTSS_ISID_LOCAL, VTSS_PORT_NO_START, &if_index);
    if (rc != VTSS_RC_OK) {
        T_W("vtss_ifindex_from_port failed.");
        return LLDP_FALSE;
    }
    rc = vtss_appl_tsn_fp_status_get(if_index, &tsn_fp_status);
    if ((rc == VTSS_RC_OK) && tsn_fp_status.loc_preempt_supported) {
        return LLDP_TRUE;
    }
    return LLDP_FALSE;
#else
    return LLDP_FALSE;
#endif
}

/****************************************************************************
* Module thread
****************************************************************************/
static void lldp_thread(vtss_addrword_t data)
{
    vtss_flag_t       one_sec_timer_flag;
    vtss_tick_count_t time_tick;
    port_iter_t       pit;

    // Wait until INIT_CMD_INIT is complete.
    msg_wait(MSG_WAIT_UNTIL_INIT_DONE, VTSS_MODULE_ID_LLDP);

    vtss_flag_init(&one_sec_timer_flag);
    time_tick = vtss_current_time();

    // Initialize the entry table containing all the remote neighbors information
    if (lldp_remote_table_init() != VTSS_RC_OK) {
        T_E("Could not initialize the remote entry table");
    }

    lldp_start(); // Register for LLDP frames

    // ***** Go into loop **** //
    T_I("Entering lldp_thread");
    while (1) {
        time_tick += VTSS_OS_MSEC2TICK(1000); // Setup time to wait 1 sec..
        (void)vtss_flag_timed_wait(&one_sec_timer_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR, time_tick);

        chk_for_conf_change();

        // Do the one sec timer tick - for front ports only
        (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);

        LLDP_CRIT_ENTER();
        while (port_iter_getnext(&pit)) {
            lldp_1sec_timer_tick(pit.iport);
        }

        lldp_remote_1sec_timer();
        LLDP_CRIT_EXIT();
    }
}

//************************************************
// Configuration
//************************************************
static void lldp_conf_default_set()
{
    vtss_common_macaddr_t current_mac_addr;
    lldp_16_t port_index;
    // Set default configuration

#ifdef VTSS_SW_OPTION_LLDP_MED
    lldp_16_t policy_index;
#endif

    LLDP_CRIT_ENTER();
    // Set everything to 0. Non-zero default values will be set below.
    vtss_clear(lldp_conf);
    T_IG(TRACE_GRP_CONF, "Setting to default");
    for (port_index = 0; port_index < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_index++) {
        lldp_conf.user.port[port_index].optional_tlvs_mask    = VTSS_APPL_LLDP_TLV_OPTIONAL_ALL_BITS;
        lldp_conf.user.port[port_index].snmp_notification_ena = FALSE; // Setting notification to disabled.
        lldp_conf.user.port[port_index].admin_states          = LLDP_ADMIN_STATE_DEFAULT;
#ifdef VTSS_SW_OPTION_CDP
        lldp_conf.user.port[port_index].cdp_aware             = LLDP_CDP_AWARE_DEFAULT;
#endif

#ifdef VTSS_SW_OPTION_LLDP_MED
        // Just for information : When we went from vCLI to iCLI we had to change to the default value for optional LLDP-MED
        // from enabled to disabled in order to be able to make a logical iCLI "no command".
        lldp_conf.user.port[port_index].lldpmed_optional_tlvs_mask =
            VTSS_APPL_LLDP_MED_OPTIONAL_TLV_CAPABILITIES_BIT |
            VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POLICY_BIT |
#ifdef VTSS_SW_OPTION_POE
            VTSS_APPL_LLDP_MED_OPTIONAL_TLV_POE_BIT |
#endif
            VTSS_APPL_LLDP_MED_OPTIONAL_TLV_LOCATION_BIT;

        lldp_conf.user.port[port_index].lldpmed_snmp_notification_ena = FALSE; // Setting notification to disabled.
#ifdef VTSS_SW_OPTION_LLDP_MED_TYPE
        lldp_conf.user.port[port_index].lldpmed_device_type = LLDPMED_DEVICE_TYPE_DEFAULT;
#endif
#endif
    }

#ifdef VTSS_SW_OPTION_LLDP_MED
    lldp_conf.user.common.coordinate_location.altitude_type = LLDPMED_ALTITUDE_TYPE_DEFAULT;
    lldp_conf.user.common.coordinate_location.altitude      = LLDPMED_ALTITUDE_DEFAULT;
    lldp_conf.user.common.coordinate_location.longitude     = LLDPMED_LONGITUDE_DEFAULT;
    lldp_conf.user.common.coordinate_location.latitude      = LLDPMED_LATITUDE_DEFAULT;

    lldp_conf.user.common.medFastStartRepeatCount     = VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_DEFAULT;
    lldp_conf.user.common.coordinate_location.datum         = LLDPMED_DATUM_DEFAULT;

    for (policy_index = LLDPMED_POLICY_MIN; policy_index <= LLDPMED_POLICY_MAX; policy_index++) {
        lldp_conf.user.policies_table[policy_index].network_policy.application_type = VOICE;
        lldp_conf.user.policies_table[policy_index].network_policy.vlan_id = VTSS_APPL_VLAN_ID_MIN;
    }

#endif
    lldp_conf.user.common.tx_sm.reInitDelay           = VTSS_APPL_LLDP_REINIT_DEFAULT;
    lldp_conf.user.common.tx_sm.msgTxHold             = VTSS_APPL_LLDP_TX_HOLD_DEFAULT;
    lldp_conf.user.common.tx_sm.txDelay               = VTSS_APPL_LLDP_TX_DELAY_DEFAULT;
    lldp_conf.user.common.tx_sm.msgTxInterval         = VTSS_APPL_LLDP_TX_INTERVAL_DEFAULT;
    lldp_conf.user.common.snmp_notification_interval  = 5; // Suggested value by the IEEE standard.
    vtss_os_get_systemmac(&current_mac_addr);
    lldp_conf.run_time.mac_addr                       = current_mac_addr; // update the primary switch's mac address

    system_conf_has_changed = 1; // Signal that new configuration has been done (making sure that system name is updated as well).
    LLDP_CRIT_EXIT();
}

#ifdef VTSS_SW_OPTION_VOICE_VLAN
// Check for Voice VLAN cross-reference for misconfiguration
static mesa_rc lldp_voice_vlan_chk(const vtss_appl_lldp_port_conf_t *port_conf)
{
    lldp_port_t iport;
    for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {

        T_DG_PORT(TRACE_GRP_CONF, iport, "%s", "Check for Voice Vlan configuration mismatch");
        if (voice_vlan_is_supported_LLDP_discovery(VTSS_ISID_START, iport)) {
            T_DG_PORT(TRACE_GRP_CONF, iport, "%s", "voice_vlan_is_supported_LLDP_discovery");
            if (port_conf[iport].admin_states == VTSS_APPL_LLDP_ENABLED_TX_ONLY || port_conf[iport].admin_states == VTSS_APPL_LLDP_DISABLED) {
                T_DG_PORT(TRACE_GRP_CONF, iport, "Voice Vlan configuration mismatch %d", VTSS_APPL_LLDP_ERROR_VOICE_VLAN_CONFIGURATION_MISMATCH);
                return VTSS_APPL_LLDP_ERROR_VOICE_VLAN_CONFIGURATION_MISMATCH;
            }
        }
    }

    return VTSS_RC_OK;
}
#endif

// Check if it is OK to do a configuration for a switch
static mesa_rc lldp_is_conf_set_valid()
{
    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_LLDP_MED
// Return error code if the policy configuration isn't within valid ranges.
static mesa_rc lldp_med_policy_conf_valid(const vtss_appl_lldp_med_policy_t *conf, u8 policy_index)
{
    T_RG(TRACE_GRP_CONF, "policy_index:%d", policy_index);
    if (policy_index > LLDPMED_POLICY_MAX) {
        return VTSS_APPL_LLDP_ERROR_POLICY_OUT_OF_RANGE;
    }

    if (conf->network_policy.vlan_id < VTSS_APPL_VLAN_ID_MIN || conf->network_policy.vlan_id > VTSS_APPL_VLAN_ID_MAX) {
        return VTSS_APPL_LLDP_ERROR_POLICY_VLAN_OUT_OF_RANGE;
    }

    /* As l2_priority and dscp_value both are unsigned we only check for the max value */
    if (conf->network_policy.l2_priority > VTSS_APPL_LLDP_MED_L2_PRIORITY_MAX) {
        return VTSS_APPL_LLDP_ERROR_POLICY_PRIO_OUT_OF_RANGE;
    }

    if (conf->network_policy.dscp_value > VTSS_APPL_LLDP_MED_DSCP_MAX) {
        return VTSS_APPL_LLDP_ERROR_POLICY_DSCP_OUT_OF_RANGE;
    }

    return VTSS_RC_OK;
}
#endif
/****************************************************************************/
/*  API functions                                                           */
/****************************************************************************/
// Because we want to get the whole LLDP entry table/statistics from the switch, and since the entry table can be quite large (280K) at
// the time of writing, we give the applications the possibility to take the semaphore, and work directly with the entry table in the LLDP module,
// instead of having to copy the table between the different modules.
void vtss_appl_lldp_mutex_lock(void)
{
    // Avoid Lint Warning 454: A thread mutex has been locked but not unlocked
    /*lint --e{454} */
    LLDP_IF_CRIT_ENTER();
}

void vtss_appl_lldp_mutex_unlock(void)
{
    // Avoid Lint Warning 455: A thread mutex that had not been locked is being unlocked
    /*lint -e(455) */
    LLDP_IF_CRIT_EXIT();
}

void vtss_appl_lldp_mutex_assert(const char *file, int line)
{
    critd_assert_locked(&lldp_if_crit, file, line);
}

void lldp_mgmt_last_change_ago_to_str(time_t last_change_ago, char *last_change_str)
{
    time_t last_change_time = msg_uptime_get(VTSS_ISID_LOCAL) - last_change_ago;
    sprintf(last_change_str, "%s  (" VPRIlu" secs. ago)", misc_time2str(msg_abstime_get(VTSS_ISID_LOCAL, last_change_time)), last_change_ago);
}

// Returns global statistics counters.
mesa_rc vtss_appl_lldp_stat_global_get(vtss_appl_lldp_global_counters_t *statistics)
{
    if (statistics == NULL) {
        return VTSS_APPL_LLDP_ERROR_NULL_POINTER;
    }

    memset(statistics, 0, sizeof(vtss_appl_lldp_global_counters_t));

    lldp_get_mib_stats(statistics);
    statistics->last_change_ago = msg_uptime_get(VTSS_ISID_LOCAL) - last_entry_change_time_this_switch; // MUST be after lldp_get_mib_stats().
    return VTSS_RC_OK;
}

// Returns statistics for a switch, and global counters and last change for the stack as a whole.
mesa_rc vtss_appl_lldp_stat_if_get(vtss_ifindex_t ifindex, vtss_appl_lldp_port_counters_t *statistics)
{
    mesa_rc rc = VTSS_RC_OK;
    VTSS_RC(port_ifindex_valid(ifindex));

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    if (statistics == NULL) {
        return VTSS_APPL_LLDP_ERROR_NULL_POINTER;
    }

    LLDP_IF_CRIT_ENTER();
    CapArray<vtss_appl_lldp_port_counters_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> local_stat_cnt;
    lldp_get_stat_counters(&local_stat_cnt[0]);
    memcpy(statistics, &local_stat_cnt[ife.ordinal], sizeof(vtss_appl_lldp_port_counters_t));
    T_IG(TRACE_GRP_STAT, "port:%d statsFramesOutTotal:%d, statsFramesOutTotal:%d",
         ife.ordinal, local_stat_cnt[ife.ordinal].statsFramesOutTotal, statistics->statsFramesOutTotal);
    LLDP_IF_CRIT_EXIT();
    return rc;
}

#ifdef VTSS_SW_OPTION_LLDP_MED
mesa_rc vtss_appl_lldp_port_policies_itr(const vtss_lldpmed_policy_index_t *prev_policy, vtss_lldpmed_policy_index_t *next_policy)
{
    mesa_rc rc = VTSS_RC_OK;

    if (prev_policy == NULL) {
        *next_policy = LLDPMED_POLICY_MIN;
    } else if (*prev_policy >= LLDPMED_POLICY_MAX) {
        rc = VTSS_RC_ERROR;
    } else {
        *next_policy = *prev_policy + 1;
    }

    T_IG(TRACE_GRP_SNMP, "next_policy:%d, rc:%d", *next_policy, rc);
    return rc;
}

// See lldp_api.h
mesa_rc vtss_appl_lldp_med_catype_itr(const vtss_appl_lldp_med_catype_t *prev_catype, vtss_appl_lldp_med_catype_t *next_catype)
{
    mesa_rc rc = VTSS_RC_OK;

    if (prev_catype == NULL || *prev_catype == LLDPMED_CATYPE_UNDEFINED) {
        *next_catype = LLDPMED_CATYPE_A1;
    } else if (*prev_catype >=  LLDPMED_CATYPE_ADD_CODE) {
        rc = VTSS_RC_ERROR;
    } else {
        if (*prev_catype >= LLDPMED_CATYPE_A6 && *prev_catype < LLDPMED_CATYPE_PRD) {
            *next_catype = LLDPMED_CATYPE_PRD;
        } else {
            u16 catype = *prev_catype + 1; // Convert vtss_appl_lldp_med_catype_t for being able to add one.
            *next_catype = (vtss_appl_lldp_med_catype_t)catype;
        }
    }

    T_IG(TRACE_GRP_SNMP, "next_catype:%d, rc:%d", *next_catype, rc);
    return rc;
}

BOOL vtss_appl_lldp_policy_iter_get_next(u8 *policy_itr)
{

    if (*policy_itr < LLDPMED_POLICY_MAX) {
        *policy_itr = *policy_itr + 1;
        return TRUE;
    }

    return FALSE; // We have been through all policies
}
#endif

mesa_rc vtss_appl_lldp_global_stat_clr(void)
{
    // Clear global counters
    LLDP_CRIT_ENTER(); // Protect last_entry_change_time_this_switch
    last_entry_change_time_this_switch = msg_uptime_get(VTSS_ISID_LOCAL); // Set the last changed time to current time (equals last time changed is 0 sec.)
    lldp_mib_stats_clear();
    LLDP_CRIT_EXIT();
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_lldp_if_stat_clr(vtss_ifindex_t ifindex)
{
    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    // Clear counters
    LLDP_CRIT_ENTER();
    lldp_statistics_clear(ife.ordinal);
    LLDP_CRIT_EXIT();
    return VTSS_RC_OK;
}

// See lldp_api.h
vtss_appl_lldp_remote_entry_t *vtss_appl_lldp_entries_get()
{
    static vtss_appl_lldp_remote_entry_t *entries = NULL;
    static vtss_tick_count_t entries_time = 0;
    static vtss_appl_lldp_cap_t entries_cap;

    if (entries &&
        (vtss_current_time() - entries_time) <= VTSS_OS_MSEC2TICK(1000)) {
        return entries;
    }
    entries_time = vtss_current_time();
    vtss_appl_lldp_cap_get(&entries_cap);

    entries =  lldp_remote_get_entries();

    return entries;
}

// See vtss_lldp.h
mesa_rc vtss_appl_lldp_entry_get(vtss_ifindex_t ifindex, vtss_lldp_entry_index_t *lldp_entry_index, vtss_appl_lldp_remote_entry_t *lldp_entry, BOOL next)
{
    VTSS_RC(port_ifindex_valid(ifindex));
    T_RG(TRACE_GRP_STAT, "Getting entry for ifindex:%d", VTSS_IFINDEX_PRINTF_ARG(ifindex));
    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    if (lldp_entry_index == NULL) {
        return VTSS_APPL_LLDP_ERROR_NULL_POINTER;
    }

    vtss_appl_lldp_remote_entry_t *entries = lldp_remote_get_entries();

    memset(lldp_entry, 0, sizeof(vtss_appl_lldp_remote_entry_t)); // Will set the in_use to 0
    if (next) {
        for (u32 idx = *lldp_entry_index; idx < LLDP_REMOTE_ENTRIES; idx++) {
            T_DG(TRACE_GRP_STAT, "idx:%d, iport:%d", idx, ife.ordinal);
            if (entries[idx].receive_port == ife.ordinal && entries[idx].in_use) {
                T_DG_PORT(TRACE_GRP_STAT, entries[idx].receive_port,  "found :%d", idx);
                memcpy(lldp_entry, &entries[idx], sizeof(vtss_appl_lldp_remote_entry_t));
                *lldp_entry_index = idx;
                return VTSS_RC_OK;
            }
        }
    } else {
        // User is asking for a specific entry.
        if (entries[*lldp_entry_index].receive_port == ife.ordinal &&
            entries[*lldp_entry_index].in_use) {
            T_DG_PORT(TRACE_GRP_STAT, entries[*lldp_entry_index].receive_port, "%s", "found");
            memcpy(lldp_entry, &entries[*lldp_entry_index], sizeof(vtss_appl_lldp_remote_entry_t));
            return VTSS_RC_OK;
        }
    }
    T_NG(TRACE_GRP_STAT, "All done");
    return VTSS_APPL_LLDP_ERROR_ENTRY_INDEX;
}

//
// Function that returns the current configuration. You must always be semaphore locked when calling this function.
//
// In/out : conf - Pointer to configuration struct where the current configuration is copied to.
//
mesa_rc lldp_conf_get(vtss_appl_lldp_port_conf_t *conf)
{
    LLDP_CRIT_ASSERT_LOCKED();   // Check that we are semaphore locked.

    T_RG(TRACE_GRP_CONF, "Getting local conf");
    for (mesa_port_no_t i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        conf[i] = lldp_conf.user.port[i];
    }
    return VTSS_RC_OK;
}


#ifdef VTSS_SW_OPTION_LLDP_MED
// Function that returns if a policy is mapped for an interface. Used for internally be able to get if the policy enable within a mutex. You must always be semaphore locked when calling this function.
//
// In : ifindex - interface index
//      if_policy_index - policy index
// out : enabled - TRUE if the policy is mapped to the interface

mesa_rc lldp_conf_port_policy_get(mesa_port_no_t iport, u8 if_policy_index, BOOL *enabled)
{
    LLDP_CRIT_ASSERT_LOCKED();   // Check that we are semaphore locked.

    if (if_policy_index > LLDPMED_POLICY_MAX) {
        return VTSS_APPL_LLDP_ERROR_POLICY_OUT_OF_RANGE;
    }

    *enabled = lldp_conf.user.port_policies[iport][if_policy_index];
    T_NG(TRACE_GRP_CONF, "iport:%d, if_policy_index:%d, enabled:%d", iport, if_policy_index, *enabled);
    return VTSS_RC_OK;
}


mesa_rc lldp_conf_policy_get(u8                          policy_index,
                             vtss_appl_lldp_med_policy_t *conf)
{
    LLDP_CRIT_ASSERT_LOCKED();   // Check that we are semaphore locked.

    if (policy_index > LLDPMED_POLICY_MAX) {
        return VTSS_APPL_LLDP_ERROR_POLICY_OUT_OF_RANGE;
    }

    *conf = lldp_conf.user.policies_table[policy_index];
    return VTSS_RC_OK;
}


mesa_rc vtss_appl_lldp_conf_policy_get(u8                          policy_index,
                                       vtss_appl_lldp_med_policy_t *conf)
{
    mesa_rc rc = VTSS_RC_OK;
    LLDP_CRIT_ENTER();
    rc = lldp_conf_policy_get(policy_index, conf);
    LLDP_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_lldp_conf_policy_set(u8    policy_index,
                                       const vtss_appl_lldp_med_policy_t conf)
{
    VTSS_RC(lldp_med_policy_conf_valid(&conf, policy_index)); // Return if configuration is not valid

    LLDP_CRIT_ENTER();
    lldp_conf.user.policies_table[policy_index] = conf;

    // If a policy is deleted, then we want to remove a port_policies that is using the newly deleted policy (Bugzilla#21455)
    T_NG(TRACE_GRP_CONF, "policy_index:%d, in_use:%d", policy_index, conf.in_use);
    if (!conf.in_use) {
        for (auto port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            lldp_conf.user.port_policies[port_no][policy_index] = FALSE;
        }
    }
    LLDP_CRIT_EXIT();
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_lldp_conf_policy_del(u8 policy_index)
{
    vtss_appl_lldp_cap_t cap;
    (void)vtss_appl_lldp_cap_get(&cap);
    if (policy_index > cap.policies_cnt) {
        return VTSS_RC_ERROR;
    }

    LLDP_CRIT_ENTER();
    lldp_conf.user.policies_table[policy_index].in_use = FALSE;
    LLDP_CRIT_EXIT();
    return VTSS_RC_OK;
}
#endif
//
// Function that returns the current configuration for the local swtich. You must always be semaphore locked when calling this function.
//
// In/out : conf - Pointer to configuration struct where the current configuration is copied to.
//
mesa_rc lldp_common_local_conf_get(vtss_appl_lldp_common_conf_t *conf)
{
    LLDP_CRIT_ASSERT_LOCKED();   // Check that we are semaphore locked.
    memcpy(conf, &lldp_conf.user.common, sizeof(lldp_conf.user.common));

    T_NG(TRACE_GRP_CONF, " lldp_conf.user.common.txInter:%d",  lldp_conf.user.common.tx_sm.msgTxInterval);
    return VTSS_RC_OK;
}

// Management function for returning current configuration. Simply semphore locking before calling the
// real get_config function.
mesa_rc lldp_mgmt_conf_get(vtss_appl_lldp_port_conf_t *conf)
{
    mesa_rc rc = VTSS_RC_OK;
    LLDP_CRIT_ENTER();
    rc = lldp_conf_get(conf);
    LLDP_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_lldp_port_conf_get(vtss_ifindex_t ifindex,
                                     vtss_appl_lldp_port_conf_t *conf)
{
    CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> current_lldp_conf;

    VTSS_RC(port_ifindex_valid(ifindex));

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    mesa_rc rc = VTSS_RC_OK;
    LLDP_CRIT_ENTER();
    rc = lldp_conf_get(&current_lldp_conf[0]);
    LLDP_CRIT_EXIT();
    T_DG_PORT(TRACE_GRP_CONF, ife.ordinal, "admin:%d", current_lldp_conf[ife.ordinal].admin_states);
    *conf = current_lldp_conf[ife.ordinal];
    return rc;
}

mesa_rc vtss_appl_lldp_port_conf_set(vtss_ifindex_t ifindex,
                                     vtss_appl_lldp_port_conf_t *conf)
{
    CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> current_lldp_conf;

    VTSS_RC(port_ifindex_valid(ifindex));

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    LLDP_CRIT_ENTER(); // Protect lldp_conf
    for (u8 i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        current_lldp_conf[i] = lldp_conf.user.port[i];
    }
    LLDP_CRIT_EXIT();

    current_lldp_conf[ife.ordinal] = *conf;

    return lldp_mgmt_conf_set(&current_lldp_conf[0]);
}

//
// Function for getting an unique port id for each port.
//
// In : iport - Internal port number (starting from 0)
// Return : Unique port id
mesa_port_no_t lldp_mgmt_get_unique_port_id(mesa_port_no_t iport)
{
    return iport2uport(iport);
}

// Function for getting the latitude direction based on the latitude 2s complement as specified in rfc3825
vtss_appl_lldp_med_latitude_dir_t get_latitude_dir(u64 latitude)
{
    u64 res = 1LL << (LLDPMED_LONG_LATI_TUDE_RES - 1);
    T_IG(TRACE_GRP_CONF, "res:" VPRI64x", latitude:" VPRI64x, res, latitude);
    if (latitude & (res)) {
        T_IG(TRACE_GRP_CONF, "res:" VPRI64x, res);
        return SOUTH; // rfc3825, Section 2.1 - Negative number is south.
    }

    return NORTH;     // rfc3825, Section 2.1 - Positions number is north.
}

// Function for getting the longitude direction based on the latitude 2s complement as specified in rfc3825
vtss_appl_lldp_med_longitude_dir_t get_longitude_dir(u64 longitude)
{
    u64 res = 1LL << (LLDPMED_LONG_LATI_TUDE_RES - 1);
    if (longitude & (res)) {
        return WEST; // rfc3825, Section 2.1 - Negative number is west.
    }

    return EAST;     // rfc3825, Section 2.1 - Positions number is east.
}


// See header file
mesa_rc lldp_tudes_as_long(const vtss_appl_lldp_common_conf_t *conf, long *altitude, long *longitude, long *latitude)
{
#ifdef VTSS_SW_OPTION_LLDP_MED
    char value_str[25];

    //
    // Longitude
    //
    T_DG(TRACE_GRP_CONF, "conf->coordinate_location.longitude:" VPRI64x, conf->coordinate_location.longitude);

    if (conf->coordinate_location.longitude > 0x168000000) {
        T_IG(TRACE_GRP_CONF, "East out of range");
        return VTSS_APPL_LLDP_ERROR_LONGITUDE_OUT_OF_RANGE;
    }

    if (conf->coordinate_location.longitude < 0xfffffffe97ffffff && conf->coordinate_location.longitude < 0x0) {
        T_IG(TRACE_GRP_CONF, "West out of range");
        return VTSS_APPL_LLDP_ERROR_LONGITUDE_OUT_OF_RANGE;
    }

    (void) lldpmed_tude2decimal_str(LLDPMED_LONG_LATI_TUDE_RES, conf->coordinate_location.longitude, 9, 25, &value_str[0], TUDE_DIGIT, TRUE);

    VTSS_RC(mgmt_str_float2long(value_str, longitude, LLDPMED_LONGITUDE_VALUE_MIN, LLDPMED_LONGITUDE_VALUE_MAX, TUDE_DIGIT));

    //
    // Latitude
    //
    T_DG(TRACE_GRP_CONF, "conf->coordinate_location.latitude:" VPRI64x, conf->coordinate_location.latitude);

    if (conf->coordinate_location.latitude > 0xB4000000) {
        T_IG(TRACE_GRP_CONF, "North out of range");
        return VTSS_APPL_LLDP_ERROR_LATITUDE_OUT_OF_RANGE;
    }

    if (conf->coordinate_location.latitude < 0xffffffff4bffffff && conf->coordinate_location.latitude < 0x0) {
        T_IG(TRACE_GRP_CONF, "South out of range");
        return VTSS_APPL_LLDP_ERROR_LATITUDE_OUT_OF_RANGE;
    }

    (void) lldpmed_tude2decimal_str(LLDPMED_LONG_LATI_TUDE_RES, conf->coordinate_location.latitude, 9, 25, &value_str[0], TUDE_DIGIT, TRUE);

    VTSS_RC(mgmt_str_float2long(value_str, latitude, LLDPMED_LATITUDE_VALUE_MIN, LLDPMED_LATITUDE_VALUE_MAX, TUDE_DIGIT));

    //
    // Altitude
    //

    // We do the range check here because during the converting from 2' complement to "decimal" the precision is lost.
    if (conf->coordinate_location.altitude > 0x3FFFFFFF || conf->coordinate_location.altitude < -0x3FFFFFFF) {
        T_IG(TRACE_GRP_CONF, "Out of range");
        return VTSS_APPL_LLDP_ERROR_ALTITUDE_OUT_OF_RANGE;
    }

    (void) lldpmed_tude2decimal_str(LLDPMED_ALTITUDE_RES, conf->coordinate_location.altitude, 22, 8, &value_str[0], 1, FALSE);

    T_DG(TRACE_GRP_CONF, "value_str:%s, conf->coordinate_location.altitude:0x%X",
         value_str, conf->coordinate_location.altitude);

    // Cheating a little bit with the min and max values (the -1 and + 1), due the precision. The real check is done correctly just above.
    VTSS_RC(mgmt_str_float2long(value_str, altitude, (long)(LLDPMED_ALTITUDE_VALUE_MIN - 1), (ulong)( LLDPMED_ALTITUDE_VALUE_MAX + 1), 1));
#endif
    T_DG(TRACE_GRP_CONF, "Conf done");
    return VTSS_RC_OK;
}

// Function for checking if a configuration is valid
static mesa_rc lldp_conf_valid(const vtss_appl_lldp_common_conf_t *common_conf)
{
    T_IG(TRACE_GRP_CONF, "Enter");
    // txDelay must not be larger than 0.25 * TxInterval - IEEE 802.1AB-clause 10.5.4.2
    if (common_conf->tx_sm.txDelay > (common_conf->tx_sm.msgTxInterval / 4)) {
        T_IG(TRACE_GRP_CONF, "txDelay (%d) must not be larger than 0.25 * %d - IEEE 802.1AB-clause 10.5.4.2", common_conf->tx_sm.txDelay, common_conf->tx_sm.msgTxInterval);
        return VTSS_APPL_LLDP_ERROR_TX_DELAY;
    }

    if (common_conf->tx_sm.reInitDelay > VTSS_APPL_LLDP_REINIT_MAX || common_conf->tx_sm.reInitDelay < VTSS_APPL_LLDP_REINIT_MIN) {
        T_IG(TRACE_GRP_CONF, "common_conf->reInitDelay:%d", common_conf->tx_sm.reInitDelay);
        return VTSS_APPL_LLDP_ERROR_REINIT_VALUE;
    }

    if (common_conf->tx_sm.msgTxHold > VTSS_APPL_LLDP_TX_HOLD_MAX || common_conf->tx_sm.msgTxHold < VTSS_APPL_LLDP_TX_HOLD_MIN) {
        T_IG(TRACE_GRP_CONF, "common_conf->msgTxHold:%d", common_conf->tx_sm.msgTxHold);
        return VTSS_APPL_LLDP_ERROR_TX_HOLD_VALUE;
    }

    if (common_conf->tx_sm.msgTxInterval > VTSS_APPL_LLDP_TX_INTERVAL_MAX || common_conf->tx_sm.msgTxInterval < VTSS_APPL_LLDP_TX_INTERVAL_MIN) {
        T_IG(TRACE_GRP_CONF, "common_conf->msgTxInterval:%d", common_conf->tx_sm.msgTxInterval);
        return VTSS_APPL_LLDP_ERROR_TX_INTERVAL_VALUE;
    }

    if (common_conf->tx_sm.txDelay > VTSS_APPL_LLDP_TX_DELAY_MAX || common_conf->tx_sm.txDelay < VTSS_APPL_LLDP_TX_DELAY_MIN) {
        T_IG(TRACE_GRP_CONF, "common_conf->txDelay:%d", common_conf->tx_sm.txDelay);
        return VTSS_APPL_LLDP_ERROR_TX_DELAY_VALUE;
    }

#ifdef VTSS_SW_OPTION_LLDP_MED
    if (common_conf->medFastStartRepeatCount > VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_MAX ||
        common_conf->medFastStartRepeatCount < VTSS_APPL_LLDP_FAST_START_REPEAT_COUNT_MIN) {
        T_IG(TRACE_GRP_CONF, "common_conf->medFastStartRepeatCount:%d", common_conf->medFastStartRepeatCount);
        return VTSS_APPL_LLDP_ERROR_FAST_START_REPEAT_COUNT;
    }
#endif

#ifdef VTSS_SW_OPTION_LLDP_MED
    // From TIA1057, ANNEX B. - country code: The two-letter ISO 3166 country code in capital ASCII letters, e.g., DE or US.
    u16 country_code_len = strlen(common_conf->ca_country_code);


    if (country_code_len != (VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN - 1) && country_code_len != 0) { // minus 1 because VTSS_APPL_LLDP_CA_COUNTRY_CODE_LEN has space for \0
        T_IG(TRACE_GRP_CONF, "country_code_len:%d", country_code_len);
        return VTSS_APPL_LLDP_ERROR_COUNTRY_CODE_LETTER_SIZE;
    }

    u8 i;
    for (i = 0; i < country_code_len; i++) {
        T_IG(TRACE_GRP_CONF, "Country code i:%d, c:%d, isalpha:%d, isupper:%d",
             i,  common_conf->ca_country_code[i], isalpha(common_conf->ca_country_code[i]), isupper(common_conf->ca_country_code[i]));
        if (!isalpha(common_conf->ca_country_code[i]) || !isupper(common_conf->ca_country_code[i])) {
            T_IG(TRACE_GRP_CONF, "Country code not correct");
            return VTSS_APPL_LLDP_ERROR_COUNTRY_CODE_LETTER;
        }
    }



    // TIA1057, figure 11 - ecs is an numerical string and is max 25 characters long
    u16  ecs_len = strlen(common_conf->elin_location);
    if (country_code_len > VTSS_APPL_LLDP_ELIN_VALUE_LEN_MAX) {
        T_IG(TRACE_GRP_CONF, "country_code_len:%d", country_code_len);
        return VTSS_APPL_LLDP_ERROR_ELIN_SIZE;
    }


    for (i = 0; i < ecs_len; i++) {
        T_IG(TRACE_GRP_CONF, "ecs i:%d, c:%d, isdigi:%d",
             i,  common_conf->elin_location[i], isdigit(common_conf->elin_location[i]));

        if (!isdigit(common_conf->elin_location[i])) {
            T_IG(TRACE_GRP_CONF, "ECS not digit");
            return VTSS_APPL_LLDP_ERROR_ELIN;
        }
    }
#endif

    // Use lldp_tudes_as_long to check for valid range
    long dummy = 0;
    VTSS_RC(lldp_tudes_as_long(common_conf, &dummy, &dummy, &dummy));
    return VTSS_RC_OK;
}

// Function for setting configuration that is common for all switches in a stack
// In - common_conf : Pointer to where to new configuration
mesa_rc vtss_appl_lldp_common_conf_set(const vtss_appl_lldp_common_conf_t *const common_conf)
{

    VTSS_RC(lldp_conf_valid(common_conf));

    LLDP_CRIT_ENTER(); // Protect common_conf
    memcpy(&lldp_conf.user.common, common_conf, sizeof(vtss_appl_lldp_common_conf_t));
    LLDP_CRIT_EXIT(); // Protect common_conf

    return VTSS_RC_OK;
}

// Function for getting configuration that is common for all switches in a stack
// In - common_conf : Pointer to where to put the configuration
mesa_rc vtss_appl_lldp_common_conf_get(vtss_appl_lldp_common_conf_t *const common_conf)
{
    LLDP_CRIT_ENTER(); // Protect common_conf
    memcpy(common_conf, &lldp_conf.user.common, sizeof(vtss_appl_lldp_common_conf_t));
    T_IG(TRACE_GRP_CONF, "Longitude as 2':0x" VPRI64x, common_conf->coordinate_location.longitude);
    LLDP_CRIT_EXIT(); // Protect common_conf
    return VTSS_RC_OK;
}

//
// Setting the current configuration
//
mesa_rc lldp_mgmt_conf_set(vtss_appl_lldp_port_conf_t *conf)
{
    T_RG(TRACE_GRP_CONF, "Entering lldp_set_config");
    VTSS_RC(lldp_is_conf_set_valid());
#ifdef VTSS_SW_OPTION_VOICE_VLAN
    VTSS_RC(lldp_voice_vlan_chk(&conf[0]));
#endif

    // Ok now we can do the configuration
    LLDP_CRIT_ENTER(); // Protect lldp_conf
    for (mesa_port_no_t i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        lldp_conf.user.port[i] =  conf[i];
    }
    LLDP_CRIT_EXIT();

#ifdef VTSS_SW_OPTION_CDP
    // Add static mac address for CDP
    for (mesa_port_no_t port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
        if (lldp_conf.user.port[port_no].cdp_aware) {
            cdp_add_to_mac_table(VTSS_ISID_LOCAL, port_no, NULL);
        }
    }
#endif

    return VTSS_RC_OK;
}

//
// Notification interval for SNMP Trap.
//
// Setting a new notification interval.
mesa_rc lldp_mgmt_set_notification_interval(int notification_interval)
{
    T_NG(TRACE_GRP_CONF, "Entering lldp_mgmt_set_notification_interval");

    if (notification_interval < 5 || notification_interval > 3600) {
        // Interval defined by IEEE is 5-3600 sec.
        return VTSS_APPL_LLDP_ERROR_NOTIFICATION_INTERVAL_VALUE;
    }

    // Ok we are allowed to do configuration changes
    LLDP_CRIT_ENTER();
    lldp_conf.user.common.snmp_notification_interval = notification_interval;
    LLDP_CRIT_EXIT();

    return VTSS_RC_OK;
}

// Getting current notification interval.
int lldp_mgmt_get_notification_interval(BOOL crit_region_not_set)
{
    int current_notification_interval = 0;


    // get the current value
    if (crit_region_not_set) {
        LLDP_CRIT_ENTER();
        current_notification_interval = lldp_conf.user.common.snmp_notification_interval;
        LLDP_CRIT_EXIT();
    } else {
        LLDP_CRIT_ASSERT_LOCKED();
        current_notification_interval = lldp_conf.user.common.snmp_notification_interval;
    }

    // return the value.
    return current_notification_interval;
}

// Setting a new notification enable.
mesa_rc lldp_mgmt_set_notification_ena(int notification_ena, lldp_port_t port)
{
    T_NG(TRACE_GRP_CONF, "Entering lldp_mgmt_set_notification_interval_ena");
    VTSS_RC(lldp_is_conf_set_valid());

    if (notification_ena < 1 || notification_ena > 2) {
        return VTSS_APPL_LLDP_ERROR_NOTIFICATION_VALUE;
    }

    // Ok we are allowed to do configuration changes
    LLDP_CRIT_ENTER();
    lldp_conf.user.port[port].snmp_notification_ena = (notification_ena == 1) ? TRUE : FALSE;
    LLDP_CRIT_EXIT();

    return VTSS_RC_OK;
}

// Getting current notification interval.
int lldp_mgmt_get_notification_ena(lldp_port_t port, BOOL crit_region_not_set)
{
    BOOL snmp_notification_ena;

    T_NG(TRACE_GRP_CONF, "Entering lldp_mgmt_get_notification_ena");
    // get the current value
    if (crit_region_not_set) {
        LLDP_CRIT_ENTER();
        snmp_notification_ena = lldp_conf.user.port[port].snmp_notification_ena;
        LLDP_CRIT_EXIT();
    } else {
        LLDP_CRIT_ASSERT_LOCKED();
        snmp_notification_ena = lldp_conf.user.port[port].snmp_notification_ena;
    }

    int current_notification_ena = (snmp_notification_ena) ? 1 : 2;
    return current_notification_ena;
}

#ifdef VTSS_SW_OPTION_LLDP_MED
// lldpXMedMIB.c needs to get the location tlv information, and
// these functions must be called within a crtital region, so therefore
// there the 3 mgmt function below is made in order not to do direct calls.
lldp_u8_t lldp_mgmt_lldpmed_coordinate_location_tlv_add(lldp_u8_t *buf)
{
    lldp_u8_t result;
    LLDP_CRIT_ENTER();
    result = lldpmed_coordinate_location_tlv_add(buf);
    LLDP_CRIT_EXIT();
    return result;
}

lldp_u8_t lldp_mgmt_lldpmed_civic_location_tlv_add(lldp_u8_t *buf)
{
    lldp_u8_t result;
    LLDP_CRIT_ENTER();
    result = lldpmed_civic_location_tlv_add(buf);
    LLDP_CRIT_EXIT();
    return result;
}

lldp_u8_t lldp_mgmt_lldpmed_ecs_location_tlv_add(lldp_u8_t *buf)
{
    lldp_u8_t result;
    LLDP_CRIT_ENTER();
    result = lldpmed_ecs_location_tlv_add(buf);
    LLDP_CRIT_EXIT();
    return result;
}

//
// Setting notification enable bit for a port.
//
mesa_rc lldpmed_mgmt_set_notification_ena(int notification_ena, lldp_port_t port)
{
    T_NG(TRACE_GRP_CONF, "Entering lldp_mgmt_set_notification_interval_ena");
    VTSS_RC(lldp_is_conf_set_valid());

    if (notification_ena < 1 || notification_ena > 2) {
        T_W("Notification enable must be either 1 (enable) or 2 (disable) ");
        return VTSS_APPL_LLDP_ERROR_NOTIFICATION_VALUE;
    }

    // Ok we are allowed to do configuration changes
    LLDP_CRIT_ENTER();
    lldp_conf.user.port[port].lldpmed_snmp_notification_ena = (notification_ena == 1) ? TRUE : FALSE;
    LLDP_CRIT_EXIT();

    return VTSS_RC_OK;
}


// Getting current notification interval.
int lldpmed_mgmt_get_notification_ena(lldp_port_t port)
{
    BOOL lldpmed_snmp_notification_ena;
    // get the current value
    LLDP_CRIT_ENTER();
    lldpmed_snmp_notification_ena = lldp_conf.user.port[port].lldpmed_snmp_notification_ena;
    LLDP_CRIT_EXIT();
    int current_notification_ena = (lldpmed_snmp_notification_ena) ? 1 : 2;
    return current_notification_ena;
}
#endif // VTSS_SW_OPTION_LLDP_MED

//
// Callback list containing registered callback functions to be
// called when an entry is updated
//
typedef struct {
    lldp_callback_t callback_ptr; // Pointer to the callback function
    lldp_bool_t     active;       // Signaling that this entry in the list is assigned to a callback function
} lldp_entry_updated_callback_list_t;

static CapArray<lldp_entry_updated_callback_list_t, VTSS_APPL_CAP_LLDP_REMOTE_ENTRY_CNT> callback_list;  // Array containing the callback functions

//
// Function that loops through all the callback list and calls registered functions
//
// In : iport : The iport that the entry was mapped to.
//      entry : The entry that has been changed
//
static void lldp_call_entry_updated_callbacks(lldp_port_t iport, vtss_appl_lldp_remote_entry_t *entry)
{
    lldp_16_t callback_list_index;

    LLDP_CRIT_ASSERT_LOCKED(); // We MUST be crit locked in this function. Verify that we are locked.

    // Loop through all registered callbacks and execute the callback that.
    for (callback_list_index = 0; callback_list_index < callback_list.size(); callback_list_index++)   {
        if (callback_list[callback_list_index].active) {
            if (callback_list[callback_list_index].callback_ptr == NULL) {
                // Should never happen.
                T_E("Callback for callback_list_index=%d is NULL", callback_list_index);
            } else {
                // Execute the callback function
                callback_list[callback_list_index].callback_ptr(iport, entry);
            }
        }
    }
}


mesa_rc vtss_appl_lldp_cap_get(vtss_appl_lldp_cap_t *cap)
{
    cap->remote_entries_cnt = LLDP_REMOTE_ENTRIES;
#ifdef VTSS_SW_OPTION_LLDP_MED
    cap->policies_cnt       = LLDPMED_POLICY_MAX;
#endif
    return VTSS_RC_OK;
}

//
// Management function for registering a callback function.
//
// in : cb : The function that shall be called when an entry has been updated
//
void lldp_mgmt_entry_updated_callback_register(lldp_callback_t cb)
{
    lldp_16_t   free_idx = 0, idx;
    lldp_bool_t free_idx_found = FALSE;

    T_IG(TRACE_GRP_CONF, "Entering lldp_mgmt_entry_updated_callback_register");
    LLDP_CRIT_ENTER();

    for (idx = 0; idx < callback_list.size(); idx++) {
        // Search for a free index.
        if (callback_list[idx].active == 0) {
            free_idx = idx;
            free_idx_found = TRUE;
            break;
        }
    }

    if (!free_idx_found) {
        T_E("Trying to register too many lldp entry updated callbacks, please increase the callback array");
    } else {
        callback_list[free_idx].callback_ptr = cb; // Register the callback function
        callback_list[free_idx].active = 1;
    }
    LLDP_CRIT_EXIT();
}

#ifdef VTSS_SW_OPTION_LLDP_MED
mesa_rc vtss_appl_lldp_conf_port_policy_get(vtss_ifindex_t ifindex,
                                            vtss_lldpmed_policy_index_t if_policy_index,
                                            BOOL           *enabled)
{
    VTSS_RC(port_ifindex_valid(ifindex));

    if (if_policy_index > LLDPMED_POLICY_MAX) {
        return VTSS_APPL_LLDP_ERROR_POLICY_OUT_OF_RANGE;
    }

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    LLDP_CRIT_ENTER();
    mesa_rc rc = lldp_conf_port_policy_get(ife.ordinal, if_policy_index, enabled);
    LLDP_CRIT_EXIT();
    return rc;
}

// See vtss_lldp.h
const char *vtss_appl_lldp_location_civic_info_get(const vtss_appl_lldp_med_civic_tlv_format_t *const civic_tlv,
                                                   vtss_appl_lldp_med_catype_t type, u32 max_length, char *dest)
{
    vtss_appl_lldpmed_civic_t civic;

    // First we convert from the TLV format to the vtss_appl_lldpmed_civic_t, because that is easier to work with
    if (civic_tlv2civic(civic_tlv, &civic) != VTSS_RC_OK) {
        T_E("Issue converting");
        return "";
    }

    // Get a pointer to the right "entry" in the vtss_appl_lldpmed_civic_t struct
    char *ca_ptr = civic_ptr_get(&civic, type);

    if (ca_ptr == NULL) {
        return NULL;
    } else {
        // Copy the string
        strncpy(dest, ca_ptr, max_length);
        dest[max_length - 1] = '\0'; // strncpy doesn't add null character in case where maximum length is exceeded, so we do it manually.
    }

    T_NG(TRACE_GRP_CONF, "dest:%s", dest);
    return dest;
}

mesa_rc vtss_appl_lldp_conf_port_policy_set(vtss_ifindex_t ifindex,
                                            u32            if_policy_index,
                                            const BOOL     enabled)
{
    VTSS_RC(port_ifindex_valid(ifindex));

    if (if_policy_index > LLDPMED_POLICY_MAX) {
        return VTSS_APPL_LLDP_ERROR_POLICY_OUT_OF_RANGE;
    }

    vtss_ifindex_elm_t ife;
    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));
    VTSS_RC(lldp_is_conf_set_valid());

    T_NG(TRACE_GRP_CONF, "ife.ordinal:%d, if_policy_index:%d, enabled:%d", ife.ordinal, if_policy_index, enabled);

    vtss_appl_lldp_med_policy_t policy;
    LLDP_CRIT_ENTER();
    mesa_rc rc = lldp_conf_policy_get(if_policy_index, &policy);
    LLDP_CRIT_EXIT();
    VTSS_RC(rc); // Return in case of error from the lldp_conf_policy_get

    // We don't assign to an interface, the policy is not defined
    if (!policy.in_use && enabled) {
        return VTSS_APPL_LLDP_ERROR_POLICY_NOT_DEFINED;
    }

    LLDP_CRIT_ENTER();
    lldp_conf.user.port_policies[ife.ordinal][if_policy_index] = enabled;
    LLDP_CRIT_EXIT();
    return VTSS_RC_OK;
}
#endif
//
// Management function for unregistering a callback function.
//
// in : cb : The function that shall be called when an entry has been updated
//
void lldp_mgmt_entry_updated_callback_unregister(lldp_callback_t cb)
{
    lldp_16_t   cb_idx = 0, idx;
    lldp_bool_t cb_idx_found = FALSE;

    LLDP_CRIT_ENTER();

    for (idx = 0; idx < callback_list.size(); idx++) {
        // Search for a cb index.
        if (callback_list[cb_idx].callback_ptr == cb) {
            cb_idx = idx;
            cb_idx_found = TRUE;
            break;
        }
    }

    if (!cb_idx_found) {
        T_E("Trying to un-register a callback function that hasn't been registered");
    } else {
        callback_list[cb_idx].active = 0;
    }
    LLDP_CRIT_EXIT();
}

//
// Statistics
//

// When ever an entry is added or deleted this function is called.
// This is used for "stat counters" to show the time of the last entry change
static void lldp_stat_update_last_entry_time(lldp_port_t iport)
{
    LLDP_CRIT_ASSERT_LOCKED(); // We MUST be crit locked in this function. Verify that we are locked.
    T_NG(TRACE_GRP_STAT, "Transmitting LLDP entries changed");

    last_entry_change_time_this_switch = msg_uptime_get(VTSS_ISID_LOCAL);
#if defined(VTSS_SW_OPTION_SNMP)
    vtss_appl_lldp_global_counters_t stat;
    BOOL snmp_notification_ena;

//    LLDP_CRIT_ENTER();
    snmp_notification_ena = lldp_conf.user.port[iport].snmp_notification_ena;
    lldp_get_mib_stats(&stat);
//    LLDP_CRIT_EXIT();
    T_N("lldp_stat_update_last_entry_time(), iport = %u, enable = %d", iport, snmp_notification_ena);
    if (snmp_notification_ena) {
        snmpLLDPNotificationChange(VTSS_ISID_START, iport, &stat, lldp_conf.user.common.snmp_notification_interval);
#ifdef VTSS_SW_OPTION_SECURE_SNMP
#ifdef VTSS_SW_OPTION_LLDP_MED
        snmpLLDPXemMIBNotificationChange(iport);
#endif
#endif
    }
#endif
}

//
// When ever an entry is added or deleted this function MUST be called.
//
// In : entry : The entry that has been changed.
//
void lldp_entry_changed(vtss_appl_lldp_remote_entry_t *entry)
{
    T_NG_PORT(TRACE_GRP_RX, entry->receive_port, "%s", "Enter");
    LLDP_CRIT_ASSERT_LOCKED(); // We MUST be crit locked in this function. Verify that we are locked.
    lldp_stat_update_last_entry_time(entry->receive_port); //
    lldp_call_entry_updated_callbacks(entry->receive_port, entry);
    T_NG_PORT(TRACE_GRP_RX, entry->receive_port, "%s", "Done");
}

void lldp_send_frame(lldp_port_t port_idx, lldp_u8_t *frame, lldp_u16_t len)
{
    packet_tx_props_t tx_props;

    T_NG_PORT(TRACE_GRP_TX, (mesa_port_no_t)port_idx, "%s", "LLDP frame transmission start");

    if (!lldp_port_is_authorized(port_idx)) {
        return;
    }

    T_RG_PORT(TRACE_GRP_TX, (mesa_port_no_t) port_idx, "%s", "LLDP frame transmission: Port is authorized");

    // Allocate frame buffer. Exclude room for FCS.
#ifdef VTSS_SW_OPTION_PACKET
    uchar *frm_p;
    if ((frm_p = packet_tx_alloc(len < 64 ? 64 : len)) == NULL) {
        T_W("LLDP packet_tx_alloc failed.");
        return;
    } else {
        if (len < 64) {
            // Do manual padding since FDMA isn't able to do it, and the FDMA requires minimum 64 bytes.
            memset(frm_p, 0, 64);
            memcpy(frm_p, frame, len);
            len = 64;
        } else {
            memcpy(frm_p, frame, len);
        }

    }

    packet_tx_props_init(&tx_props);
    tx_props.packet_info.modid     = VTSS_MODULE_ID_LLDP;
    tx_props.packet_info.frm       = frm_p;
    tx_props.packet_info.len       = len;
    tx_props.tx_info.dst_port_mask = VTSS_BIT64(port_idx);
    tx_props.tx_info.cos           = MESA_PRIO_CNT;
    if (packet_tx(&tx_props) != VTSS_RC_OK) {
        T_E("LLDP frame was not transmitted correctly");
    } else {
        T_NG_PORT(TRACE_GRP_TX, port_idx, "%s", "LLDP frame transmistion done");
    }
#endif
}

// When the switch reboots, a LLDP shutdown frame must be transmitted. This function is call upon reboot
static void pre_shutdown(mesa_restart_t restart)
{
    port_iter_t      pit;

    // loop through all port and send a shutdown frame
    (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);

    LLDP_CRIT_ENTER();
    while (port_iter_getnext(&pit)) {
        lldp_pre_port_disabled(pit.iport);
    }
    LLDP_CRIT_EXIT();
    T_D("Going down..... ");
}

static void lldp_port_shutdown(mesa_port_no_t port_no)
{
    LLDP_CRIT_ENTER();
    lldp_pre_port_disabled(port_no - VTSS_PORT_NO_START);
    LLDP_CRIT_EXIT();
    T_D("port_no %u, shut down", port_no);
}

// Callback function for when a port changes state.
static void lldp_port_link(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    T_IG_PORT(TRACE_GRP_CONF, port_no, "link:%d", status->link);
    LLDP_CRIT_ENTER();
    lldp_set_port_enabled(port_no, status->link);
    LLDP_CRIT_EXIT();
}

extern "C" int lldpmed_icli_cmd_register();
extern "C" int lldp_icli_cmd_register();

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
mesa_rc lldp_init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_INIT:
        // Initialize critical regions
        critd_init(&lldp_crit, "lldp", VTSS_MODULE_ID_LLDP, CRITD_TYPE_MUTEX);

        // Initialize ICLI "show running" configuration
#ifdef VTSS_SW_OPTION_ICFG
        // Initialize ICFG
        VTSS_RC(lldp_icfg_init());
#ifdef VTSS_SW_OPTION_LLDP_MED
        VTSS_RC(lldpmed_icfg_init());
#endif
#endif //VTSS_SW_OPTION_ICFG

#ifdef VTSS_SW_OPTION_LLDP_MED
        lldpmed_icli_cmd_register();
#endif
        lldp_icli_cmd_register();

        critd_init(&lldp_if_crit, "lldp_if", VTSS_MODULE_ID_LLDP, CRITD_TYPE_MUTEX);

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register our private mib */
        lldp_mib_init();
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC) && defined(VTSS_SW_OPTION_LLDP_MED)
        vtss_appl_lldp_json_init();
#endif

        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           lldp_thread,
                           0,
                           "LLDP",
                           nullptr,
                           0,
                           &lldp_thread_handle,
                           &lldp_thread_block);

        T_D("exit, cmd=INIT");
        break;

    case INIT_CMD_START:
        T_D("enter, cmd=START");
        control_system_reset_register(pre_shutdown, VTSS_MODULE_ID_LLDP);       // Prepare callback function for switch reboot
        (void)port_shutdown_register(VTSS_MODULE_ID_LLDP, lldp_port_shutdown);  // Prepare callback function for port disable
        (void)port_change_register(VTSS_MODULE_ID_LLDP, lldp_port_link);        // Prepare callback function for link up/down for ports
        LLDP_CRIT_ENTER();
        sw_lldp_init();
        LLDP_CRIT_EXIT();
        break;

    case INIT_CMD_CONF_DEF:
        /* Reset configuration */
        lldp_conf_default_set();
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_I("INIT_CMD_ICFG_LOADING_PRE");
        lldp_conf_default_set();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

