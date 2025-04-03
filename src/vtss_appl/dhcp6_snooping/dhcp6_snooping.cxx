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

#include "main.h"
#include <vtss/basics/vector.hxx>
#include <time.h>

#include "critd_api.h"
#include "qos_api.h"
#include "mgmt_api.h"
#include "packet_api.h"
#include "misc_api.h"
#include "port_api.h"
#include "vtss_timer_api.h"

#include "vtss/appl/dhcp6_snooping.h"
#include "dhcp6_snooping_icfg.h"
#include "dhcp6_snooping_priv.h"
#include "dhcp6_snooping_expose.h"
#include "dhcp6_snooping_frame.h"

#include "vtss/basics/memcmp-operator.hxx"

#include "vtss_common_iterator.hxx"
#include "vtss/appl/vcap_types.h"
#include "vtss/appl/qos.h"
#include "packet_parser.hxx"

using namespace vtss;

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_DHCP6_SNOOPING

VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_dhcp6_snooping_port_conf_t);

namespace dhcp6_snooping
{

/* **************************************************************************
 * Trace definitions
 * **************************************************************************
 */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "dhcp6_snoop", "DHCPv6 Snooping"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_TXPKT] = {
        "txpkt",
        "TX packet",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_RXPKT] = {
        "rxpkt",
        "RX packet",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_CFG] = {
        "cfg",
        "Configuration",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_STATE] = {
        "state",
        "State Machine",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_TIMER] = {
        "timer",
        "Timers",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/* **************************************************************************
 * Critical section definitions
 * **************************************************************************
 */

static critd_t crit;

#define DHCP6_SNOOPING_CRIT_ENTER() critd_enter(&dhcp6_snooping::crit, __FILE__, __LINE__)
#define DHCP6_SNOOPING_CRIT_EXIT()  critd_exit( &dhcp6_snooping::crit, __FILE__, __LINE__)

/*
 * Used to automatically exit critical section when current code goes out of scope
 */
struct CritdEnterExit {
    CritdEnterExit(int line)
    {
        critd_enter(&crit, __FILE__, line);
        enterLine = line;
    }

    ~CritdEnterExit()
    {
        critd_exit(&crit, __FILE__, enterLine);
    }

    u32 enterLine;
};
#define CRIT_ENTER_EXIT() dhcp6_snooping::CritdEnterExit __lock_guard__(__LINE__)

/*
 * Used to automatically (re-)enter critical section when current code goes out of scope
 */
struct CritdExitEnter {
    CritdExitEnter(int line)
    {
        critd_exit(&crit, __FILE__, line);
        exitLine = line;
    }

    ~CritdExitEnter()
    {
        critd_enter(&crit, __FILE__, exitLine);
    }

    u32 exitLine;
};
#define CRIT_EXIT_ENTER() dhcp6_snooping::CritdExitEnter __lock_guard__(__LINE__)


/* **************************************************************************
 * Global data structures
 * **************************************************************************
 */

#define QCE_ID_CLIENT_SESSION_START 1
#define QCE_ID_CLIENT_SESSION_MAX   3

/**
 * \brief DHCPv6 snooping global configuration. This configuration applies to
 * the whole switch.
 */
static vtss_appl_dhcp6_snooping_conf_t     global_conf;

/**
 * \brief DHCPv6 snooping configuration table for individual ports.
 */
static vtss::expose::TableStatus <
vtss::expose::ParamKey<mesa_port_no_t>,
     vtss::expose::ParamVal<vtss_appl_dhcp6_snooping_port_conf_t *>
     > port_conf_table("port_conf_table", VTSS_MODULE_ID_DHCP6_SNOOPING);

/**
 * \brief DHCPv6 snooping registered clients info table. This table contains
 * the snooped information about DHCP clients and their assigned IP addresses.
 */

/*
 * Table of registered DHCP clients and their assigned IP addresses
 */
typedef vtss::Map<dhcp_duid_t, registered_clients_info_t> registered_clients_table_t;
typedef vtss::Map<dhcp_duid_t, registered_clients_info_t>::iterator registered_clients_iter_t;
static registered_clients_table_t registered_clients_table;

/*
 * Port statistics list. The index of a port entry is the internal port number.
 */
static vtss::Vector<vtss_appl_dhcp6_snooping_port_statistics_t> port_stats_table;

/**
 * \brief Global dynamic snooping data structure.
 */
static struct global_data_t {
    // QCE (CLM) rule IDs for classifying frames
    vtss::Vector<mesa_qce_id_t> qce_id_used_list;

    // Free QCE (CLM) rule IDs for allocating to clients
    vtss::Vector<mesa_qce_id_t> qce_id_free_list;

    // ACL rule IDs for redirecting or dropping packets
    vtss::Vector<mesa_ace_id_t> ace_id_used_list;

    // Packet filter definition for obtaining packets redirected to the CPU
    packet_rx_filter_t rx_filter;

    // Packet filter ID
    void *rx_filter_id = nullptr;

    // List of port trust state - used for fast internal iteration
    mesa_port_list_t trusted_ports;

    // Timer used to check lease time expiry
    vtss::Timer lease_timer;

    // Timer used to cleanup stale client entries
    vtss::Timer cleanup_timer;

    //Timestamp for last change to snooping table status.
    time_t      last_change_ts = 0;

    // TODO
    u32 frame_info_cnt = 0;

} global_data;


/* **************************************************************************
 * Callback registration
 * **************************************************************************
 */

/*
 * Max number of callback registrations. Currently we only expect the IPv6
 * Source Guard moduel to register.
 */
const int max_callback_list_entries = 1;

/*
 * List of registered callbacks. Registered callback will be called whenever
 * the snooping state of an assigned IP address changes.
 */
vtss::Vector<vtss_appl_dhcp6_snooping_ip_assigned_info_callback_t> callback_list;

typedef vtss::Vector<vtss_appl_dhcp6_snooping_ip_assigned_info_callback_t>::iterator callback_list_iter_t;

/*
 * Notify all registered users about a change in DHCP address registration
 */
static void notify_registered_callbacks(const vtss_appl_dhcp6_snooping_client_info_t *client_info,
                                        const vtss_appl_dhcp6_snooping_assigned_ip_t *address_info,
                                        vtss_appl_dhcp6_snooping_info_reason_t reason)
{
    T_NG(TRACE_GRP_STATE, "notify_registered_callbacks, reason %u", reason);
    for (auto it = callback_list.begin(); it != callback_list.end(); it++) {
        (*it)(client_info, address_info, reason);
    }
}


/* **************************************************************************
 * Timer functions
 * **************************************************************************
 */

static const u32 IDLE_CLIENTS_CHECK_TIMER_SECS  = 60;   // Interval to check for idle entries
static const u32 IDLE_CLIENTS_TIMEOUT_SECS      = 180;  // Idle time after which clients are declared stale

// Forward declarations
static void set_next_lease_expiry_timeout();

/*
 * Return the current timestamp expressed in whole seconds. We use the
 * monotonic clock to avoid problems with NTP or users changing the clock.
 */
static time_t get_curr_timestamp()
{
    timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
        T_N("Failed to get monotonic time - return time() value");
        return time(nullptr);
    }
    return ts.tv_sec;
}

/*
 * Update the last changed timestamp for the snooping table
 */
static void set_snooping_table_timestamp()
{
    global_data.last_change_ts = get_curr_timestamp();
}

static void lease_expiry_timer_callback(struct vtss::Timer *timer)
{
    DHCP6_SNOOPING_CRIT_ENTER();

    uint32_t curr_timestamp = get_curr_timestamp();

    T_NG(TRACE_GRP_TIMER, "Checking all client entries for expiry at %u", curr_timestamp);

    // Make a copy of the table keys as we may want to remove entries while iterating
    vtss::Vector<dhcp_duid_t> keys;
    for (auto it = registered_clients_table.begin(); it != registered_clients_table.end(); it++) {
        keys.push_back(it->first);
    }

    for (auto key_iter = keys.begin(); key_iter != keys.end(); key_iter++) {
        registered_clients_info_t *client_info = &registered_clients_table[*key_iter];
        /*
         * Loop through all assigned address items and check for expired entries.
         * Note the special handling of the iterator since erasing the item which
         * the iterator currently points to will invalidate the iterator! (i.e.
         * "iter++" will not work)
         */
        for (auto ita = client_info->if_map.begin(), next_ita = client_info->if_map.begin();
             ita != client_info->if_map.end(); ita = next_ita) {

            // advance to next item in advance (sic)
            next_ita = ita;
            ++next_ita;

            if (ita->second.address_state != DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED) {
                // only check entries with addresses assigned
                continue;
            }

            uint32_t curr_expiry = client_info->get_curr_expiry_time(ita->second);
            if (curr_expiry == 0) {
                // item is not active but may be negotiating with a server
                continue;
            }

            if (curr_expiry <= curr_timestamp) {
                T_DG(TRACE_GRP_TIMER, "Client entry expired - removing");

                // make local copy of entry to pass to callbacks as we are going to delete it
                auto address_info = ita->second;
                client_info->if_map.erase(ita->first);

                // Update snooping table last-changed timestamp
                set_snooping_table_timestamp();

                DHCP6_SNOOPING_CRIT_EXIT();
                notify_registered_callbacks(client_info, &address_info, DHCP6_SNOOPING_INFO_REASON_LEASE_TIMEOUT);
                DHCP6_SNOOPING_CRIT_ENTER();
            }
        }

        // Remove clients without any addresses
        if (client_info->if_map.size() == 0) {
            registered_clients_table.erase(*key_iter);

            // Update snooping table last-changed timestamp
            set_snooping_table_timestamp();
        }
    }

    // start lease-time expiry timer
    set_next_lease_expiry_timeout();

    DHCP6_SNOOPING_CRIT_EXIT();
}

/*
 * Stop the lease expiry timer if it is running
 */
static void stop_lease_expiry_timer()
{
    if (vtss_timer_cancel(&global_data.lease_timer) != VTSS_RC_OK) {
        T_D("vtss_timer_cancel() failed");
    }
}

/*
 * Start a timer for check for client entry lease timer expiry
 */
static void start_lease_expiry_timer(u32 timeout_secs)
{
    stop_lease_expiry_timer();

    T_NG(TRACE_GRP_TIMER, "Starting expiry timer, %u secs", timeout_secs);

    global_data.lease_timer.set_repeat(false);
    global_data.lease_timer.set_period(vtss::seconds(timeout_secs));
    global_data.lease_timer.callback = lease_expiry_timer_callback;
    global_data.lease_timer.modid = VTSS_MODULE_ID_DHCP6_SNOOPING;

    if (vtss_timer_start(&global_data.lease_timer) != VTSS_RC_OK) {
        T_D("vtss_timer_start() failed");
    }
}

/*
 * Start expiry timer with value for item with nearest expiry time
 */
static void set_next_lease_expiry_timeout()
{
    const registered_clients_info_t *client_info;
    uint32_t closest_expiry = 0;
    uint32_t curr_timestamp = get_curr_timestamp();
    uint32_t curr_expiry;

    T_NG(TRACE_GRP_TIMER, "Got current timestamp, %u secs", curr_timestamp);

    // Find item with expiry time closest to current time
    for (auto it = registered_clients_table.begin(); it != registered_clients_table.end(); it++) {
        client_info = &it->second;

        for (auto ita = client_info->if_map.begin(); ita != client_info->if_map.end(); ita++) {
            curr_expiry = client_info->get_curr_expiry_time(ita->second);

            if (curr_expiry > 0 && curr_expiry > curr_timestamp && (closest_expiry > curr_expiry || closest_expiry == 0)) {
                closest_expiry = curr_expiry;
            }
        }
    }

    if (closest_expiry > 0) {
        auto timeout = closest_expiry - curr_timestamp;
        start_lease_expiry_timer(timeout);
    }
}

static void client_cleanup_timer_callback(struct vtss::Timer *timer)
{
    CRIT_ENTER_EXIT();

    uint32_t curr_timestamp = get_curr_timestamp();

    T_NG(TRACE_GRP_TIMER, "Checking for stale client entries at %u", curr_timestamp);

    if (global_conf.snooping_mode != DHCP6_SNOOPING_MODE_ENABLED) {
        // Snooping has been disabled since we we started - just return
        return;
    }

    // Make a copy of the table keys as we may want to remove entries while iterating
    vtss::Vector<dhcp_duid_t> keys;
    for (auto it = registered_clients_table.begin(); it != registered_clients_table.end(); it++) {
        keys.push_back(it->first);
    }

    for (auto key_iter = keys.begin(); key_iter != keys.end(); key_iter++) {
        registered_clients_info_t *client_info = &registered_clients_table[*key_iter];

        if (client_info->has_assigned_addresses()) {
            // don't handle clients with assigned addresses as they are maintained using the lease timeout
            continue;
        }

        time_t idle_time = client_info->get_idle_time(curr_timestamp);
        if (idle_time < IDLE_CLIENTS_TIMEOUT_SECS) {
            // still time left for this entry
            continue;
        }

        T_DG(TRACE_GRP_TIMER, "Removing stale client entry %s", dhcp_duid_t(client_info->duid).to_string().c_str());
        registered_clients_table.erase(*key_iter);

        // Update snooping table last-changed timestamp
        set_snooping_table_timestamp();
    }
}

/*
 * Stop the client cleanup timer if it is running
 */
static void stop_client_cleanup_timer()
{
    if (vtss_timer_cancel(&global_data.cleanup_timer) != VTSS_RC_OK) {
        T_D("vtss_timer_cancel() cleanup_timer failed");
    }
}

/*
 * Start a timer for check for stale client entries, i.e. clients that never got an address
 */
static void start_client_cleanup_timer(u32 timeout_secs)
{
    stop_client_cleanup_timer();

    T_NG(TRACE_GRP_TIMER, "Starting client cleanup timer, %u secs", timeout_secs);

    global_data.cleanup_timer.set_repeat(true);
    global_data.cleanup_timer.set_period(vtss::seconds(timeout_secs));
    global_data.cleanup_timer.callback = client_cleanup_timer_callback;
    global_data.cleanup_timer.modid = VTSS_MODULE_ID_DHCP6_SNOOPING;

    if (vtss_timer_start(&global_data.cleanup_timer) != VTSS_RC_OK) {
        T_D("vtss_timer_start() cleanup_timer failed");
    }
}


/* **************************************************************************
 * Utility functions
 * **************************************************************************
 */

/*
 * Convert an ifindex to an internal port number
 */
static mesa_port_no_t get_port_from_ifindex(vtss_ifindex_t ifindex)
{
    vtss_ifindex_elm_t ife;
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return MESA_PORT_NO_NONE;
    }
    return ife.ordinal;
}

/*
 * Convert an internal port number to an ifindex
 */
static vtss_ifindex_t get_ifindex_from_port(mesa_port_no_t port_no)
{
    vtss_ifindex_t ifindex;
    (void)vtss_ifindex_from_port(VTSS_ISID_START, port_no, &ifindex);
    return ifindex;
}

// Local string buffer for print formatting
static char _strbuffer[80];

/*
 * Format a MAC address as a string for printing
 */
static char *mac2str(mesa_mac_t mac)
{
    return misc_mac_txt(mac.addr, _strbuffer);
}

/*
 * Pack an unsigned 16 bit value into a byte buffer.
 */
static void pack16(u16 v, u8 *buf)
{
    buf[0] = (v >> 8) & 0xff;
    buf[1] = v & 0xff;
}


/* **************************************************************************
 * Switch CLM and ACL rule setup
 * **************************************************************************
 */

static void setup_free_qceids()
{
    global_data.qce_id_free_list.clear();
    for (uint32_t count = 0; count < QCE_ID_CLIENT_SESSION_MAX; count++) {
        mesa_qce_id_t qce_id = QCE_ID_CLIENT_SESSION_START + count;
        global_data.qce_id_free_list.push_back(qce_id);
    }
}

/*
 * Get a QCE ID from the free list
 */
static mesa_qce_id_t get_free_qce_id()
{
    if (global_data.qce_id_free_list.size() > 0) {
        mesa_qce_id_t qce_id = global_data.qce_id_free_list.back();
        global_data.qce_id_free_list.pop_back();
        return qce_id;
    }

    return VTSS_APPL_QOS_QCE_ID_NONE;
}

/*
 * Remove all QCE (CLM) and ACL rules from the switch
 */
static mesa_rc remove_switch_rules()
{
    T_NG(TRACE_GRP_CFG, "Removing all switch rules");

    mesa_rc rc;
    for (auto it = global_data.qce_id_used_list.begin(); it != global_data.qce_id_used_list.end(); it++) {
        T_NG(TRACE_GRP_CFG, "Remove qce_id rule %u", *it);
        if ((rc = vtss_appl_qos_qce_intern_del(VTSS_ISID_GLOBAL, VTSS_APPL_QOS_QCL_USER_DHCP6_SNOOP, *it)) != VTSS_RC_OK) {
            T_DG(TRACE_GRP_CFG, "Failed to remove qce_id_client rule, %s", vtss_appl_qos_error_txt(rc));
        }
    }

    global_data.qce_id_used_list.clear();
    setup_free_qceids();

    for (auto it = global_data.ace_id_used_list.begin(); it != global_data.ace_id_used_list.end(); it++) {
        T_NG(TRACE_GRP_CFG, "Remove ace_id_cpu rule %u", *it);
        if ((rc = acl_mgmt_ace_del(ACL_USER_DHCPV6_SNOOP, *it)) != VTSS_RC_OK) {
            T_DG(TRACE_GRP_CFG, "Failed to remove ace_id_cpu rule, rc: %u", rc);
        }
    }

    global_data.ace_id_used_list.clear();

    return VTSS_RC_OK;
}

/*
 * Add the basic (i.e. not client-specific) QCE (CLM) and ACL rules to the switch
 */
static mesa_rc add_basic_switch_rules()
{
    mesa_rc rc;
    vtss_appl_qos_qce_conf_t qce_conf;
    acl_entry_conf_t acl_conf;
    mesa_port_no_t port_no;

    // Remove all existing rules first. This makes it easier to ensure that
    // everything is consistent.
    remove_switch_rules();

    {
        /*
         * Set CLM rule to classify DHCPv6 client frames to the "redirect to CPU"
         * ACL policy value
         */

        T_NG(TRACE_GRP_CFG, "Create QCE client rule");
        vtss_appl_qos_qce_conf_get_default(&qce_conf);

        qce_conf.qce_id = get_free_qce_id();
        if (qce_conf.qce_id == VTSS_APPL_QOS_QCE_ID_NONE) {
            T_EG(TRACE_GRP_STATE, "Error: Cannot add new QCE rule - no free QCE IDs");
            return DHCP6_SNOOPING_ERROR_RESOURCES;
        }

        // Set user and key type
        qce_conf.user_id = VTSS_APPL_QOS_QCL_USER_DHCP6_SNOOP;
        qce_conf.key.type = VTSS_APPL_QOS_QCE_TYPE_IPV6;

        // Set rule on all ports
        memset(qce_conf.key.port_list.data, 0xFF, sizeof(qce_conf.key.port_list.data));

        // Match packets sent to the IPv6 "All_DHCP_Relay_Agents_and_Servers" address
        qce_conf.key.frame.ipv6.dip.value = dhcp6::dhcp6_linkscope_relay_agents_and_servers;
        memset(&(qce_conf.key.frame.ipv6.dip.mask), 0xFF, sizeof(qce_conf.key.frame.ipv6.dip.mask));

        // Set a policy value to hit the associated ACL rule
        qce_conf.action.policy_no_enable = true;
        qce_conf.action.policy_no = ACL_POLICY_DHCPV6_SNOOP_CPU;

        rc = vtss_appl_qos_qce_conf_add(qce_conf.qce_id, &qce_conf);
        if (rc != VTSS_RC_OK) {
            T_EG(TRACE_GRP_CFG, "Error adding QCE: %s (%d)", error_txt(rc), rc);
            return rc;
        }

        global_data.qce_id_used_list.push_back(qce_conf.qce_id);
        T_NG(TRACE_GRP_CFG, "QCE client rule ID: %u", qce_conf.qce_id);
    }

    {
        bool untrusted_port_added = false;
        /*
         * Set CLM rule to classify untrusted DHCPv6 server frames to the "drop"
         * ACL policy value
         */

        T_NG(TRACE_GRP_CFG, "Create QCE untrusted server rule");
        vtss_appl_qos_qce_conf_get_default(&qce_conf);

        // Set user and key type
        qce_conf.user_id = VTSS_APPL_QOS_QCL_USER_DHCP6_SNOOP;
        qce_conf.key.type = VTSS_APPL_QOS_QCE_TYPE_IPV6;

        // Set rule on all untrusted ports
        memset(qce_conf.key.port_list.data, 0, sizeof(qce_conf.key.port_list.data));

        for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            T_NG(TRACE_GRP_CFG, "Checking port no %d", port_no);

            if (global_data.trusted_ports.get(port_no)) {
                T_NG(TRACE_GRP_CFG, "Port is trusted - don't add to port mask");
                continue;
            }

            T_NG(TRACE_GRP_CFG, "Port is untrusted - add to port mask");
            mgmt_types_port_list_bit_value_set(&qce_conf.key.port_list, VTSS_ISID_START, port_no);
            untrusted_port_added = true;
        }

        if (untrusted_port_added) {
            qce_conf.qce_id = get_free_qce_id();
            if (qce_conf.qce_id == VTSS_APPL_QOS_QCE_ID_NONE) {
                T_EG(TRACE_GRP_STATE, "Error: Cannot add new QCE rule - no free QCE IDs");
                return DHCP6_SNOOPING_ERROR_RESOURCES;
            }

            // Match on IPv6/UDP frames sent to the client UDP port
            qce_conf.key.frame.ipv6.proto.value = VTSS_IPV6_HEADER_NXTHDR_UDP;
            qce_conf.key.frame.ipv6.proto.mask = 0xFF;
            qce_conf.key.frame.ipv6.dport.match = VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC;
            qce_conf.key.frame.ipv6.dport.low = qce_conf.key.frame.ipv6.dport.high = VTSS_DHCP6_CLIENT_UDP_PORT;

            // Drop these frames unconditionally
            qce_conf.action.policy_no_enable = true;
            qce_conf.action.policy_no = ACL_POLICY_DHCPV6_SNOOP_DROP;

            rc = vtss_appl_qos_qce_conf_add(qce_conf.qce_id, &qce_conf);
            if (rc != VTSS_RC_OK) {
                T_EG(TRACE_GRP_CFG, "Error adding QCE: %s (%d)", error_txt(rc), rc);
                return rc;
            }

            global_data.qce_id_used_list.push_back(qce_conf.qce_id);
            T_NG(TRACE_GRP_CFG, "QCE untrusted server rule ID: %u", qce_conf.qce_id);
        }
    }

    {
        bool port_added = false;
        /*
         * Set CLM rule to classify trusted DHCPv6 server frames to the
         * "redirect to CPU" ACL policy value
         */

        T_NG(TRACE_GRP_CFG, "Create QCE trusted server rule");
        vtss_appl_qos_qce_conf_get_default(&qce_conf);

        // Set user and key type
        qce_conf.user_id = VTSS_APPL_QOS_QCL_USER_DHCP6_SNOOP;
        qce_conf.key.type = VTSS_APPL_QOS_QCE_TYPE_IPV6;

        // Set rule on all trusted ports
        memset(qce_conf.key.port_list.data, 0, sizeof(qce_conf.key.port_list.data));

        for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            T_NG(TRACE_GRP_CFG, "Checking port no %d", port_no);

            if (!global_data.trusted_ports.get(port_no)) {
                T_NG(TRACE_GRP_CFG, "Port is not trusted - don't add to port mask");
                continue;
            }

            T_NG(TRACE_GRP_CFG, "Port is trusted - add to port mask");
            mgmt_types_port_list_bit_value_set(&qce_conf.key.port_list, VTSS_ISID_START, port_no);
            port_added = true;
        }

        if (port_added) {
            // QoS will deny adding this rule unless there is at least one
            // trusted port, so only do the remaining if that's the case.
            qce_conf.qce_id = get_free_qce_id();
            if (qce_conf.qce_id == VTSS_APPL_QOS_QCE_ID_NONE) {
                T_EG(TRACE_GRP_STATE, "Error: Cannot add new QCE rule - no free QCE IDs");
                return DHCP6_SNOOPING_ERROR_RESOURCES;
            }

            // Match on IPv6/UDP frames sent to the client UDP port
            qce_conf.key.frame.ipv6.proto.value = VTSS_IPV6_HEADER_NXTHDR_UDP;
            qce_conf.key.frame.ipv6.proto.mask = 0xFF;
            qce_conf.key.frame.ipv6.dport.match = VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC;
            qce_conf.key.frame.ipv6.dport.low = qce_conf.key.frame.ipv6.dport.high = VTSS_DHCP6_CLIENT_UDP_PORT;

            // Drop these frames unconditionally
            qce_conf.action.policy_no_enable = true;
            qce_conf.action.policy_no = ACL_POLICY_DHCPV6_SNOOP_CPU;

            rc = vtss_appl_qos_qce_conf_add(qce_conf.qce_id, &qce_conf);
            if (rc != VTSS_RC_OK) {
                T_EG(TRACE_GRP_CFG, "Error adding QCE: %s (%d)", error_txt(rc), rc);
                return rc;
            }

            global_data.qce_id_used_list.push_back(qce_conf.qce_id);
            T_NG(TRACE_GRP_CFG, "QCE trusted server rule ID: %u", qce_conf.qce_id);
        }
    }

    {
        T_NG(TRACE_GRP_CFG, "Create ACL redir CPU rule");
        // Set ACL rule to redirect frames with indicated policy to CPU
        if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ANY, &acl_conf)) != MESA_RC_OK) {
            T_EG(TRACE_GRP_CFG, "acl_mgmt_ace_init failed: %s (%d)", error_txt(rc), rc);
            return rc;
        }

        acl_conf.isid = VTSS_ISID_LOCAL;
        acl_conf.id = ACL_MGMT_ACE_ID_NONE;
        acl_conf.port_list.set_all();
        acl_conf.policy.value = ACL_POLICY_DHCPV6_SNOOP_CPU;
        acl_conf.policy.mask = 0xFF;

        acl_conf.action.force_cpu = true;
        acl_conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
        acl_conf.action.inject_manual = TRUE;
        acl_conf.action.inject_into_ip_stack = TRUE;
        acl_conf.action.port_list.clear_all();
        acl_conf.action.cpu_queue = PACKET_XTR_QU_ACL_REDIR;

        if (acl_mgmt_ace_add(ACL_USER_DHCPV6_SNOOP, ACL_MGMT_ACE_ID_NONE, &acl_conf) != VTSS_RC_OK) {
            T_EG(TRACE_GRP_CFG, "Add ACL redir CPU rule fail.\n");
        }

        global_data.ace_id_used_list.push_back(acl_conf.id);
        T_NG(TRACE_GRP_CFG, "ACL redir CPU rule ID: %u", acl_conf.id);
    }

    {
        T_NG(TRACE_GRP_CFG, "Create ACL drop rule");
        // Set ACL rule to drop frames with indicated policy unconditionally
        if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ANY, &acl_conf)) != MESA_RC_OK) {
            T_EG(TRACE_GRP_CFG, "acl_mgmt_ace_init failed: %s (%d)", error_txt(rc), rc);
            return rc;
        }

        acl_conf.isid = VTSS_ISID_LOCAL;
        acl_conf.id = ACL_MGMT_ACE_ID_NONE;
        acl_conf.port_list.set_all();
        acl_conf.policy.value = ACL_POLICY_DHCPV6_SNOOP_DROP;
        acl_conf.policy.mask = 0xFF;

        acl_conf.action.force_cpu = false;
        acl_conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
        acl_conf.action.port_list.clear_all();

        if (acl_mgmt_ace_add(ACL_USER_DHCPV6_SNOOP, ACL_MGMT_ACE_ID_NONE, &acl_conf) != VTSS_RC_OK) {
            T_EG(TRACE_GRP_CFG, "Add ACL drop rule fail.\n");
        }

        global_data.ace_id_used_list.push_back(acl_conf.id);
        T_NG(TRACE_GRP_CFG, "ACL drop rule ID: %u", acl_conf.id);
    }
    return VTSS_RC_OK;
}

/* **************************************************************************
 * Rx filter register functions
 * **************************************************************************
 */

static BOOL handle_rx_packet(const u8 *const packet, const mesa_packet_rx_info_t *const rx_info);

static BOOL rx_packet_callback(void *contxt, const u8 *const packet, const mesa_packet_rx_info_t *const rx_info)
{
    // TODO: Move to separate snooping thread?
    return handle_rx_packet(packet, rx_info);
}

static void rx_filter_register(bool do_register)
{
    mesa_rc rc;

    if (do_register && global_data.rx_filter_id == nullptr) {
        T_DG(TRACE_GRP_CFG, "Register packet filter");

        packet_rx_filter_init(&global_data.rx_filter);

        global_data.rx_filter.modid = VTSS_MODULE_ID_DHCP6_SNOOPING;
        global_data.rx_filter.match = PACKET_RX_FILTER_MATCH_ACL | PACKET_RX_FILTER_MATCH_VLAN_TAG_ANY | PACKET_RX_FILTER_MATCH_IP_ANY | PACKET_RX_FILTER_MATCH_ETYPE;
        global_data.rx_filter.etype = ETYPE_IPV6;
        global_data.rx_filter.prio  = PACKET_RX_FILTER_PRIO_HIGH;
        global_data.rx_filter.cb    = rx_packet_callback;

        if ((rc = packet_rx_filter_register(&global_data.rx_filter, &global_data.rx_filter_id)) != VTSS_RC_OK) {
            T_EG(TRACE_GRP_CFG, "Error registering packet filter: %u", rc);
        }
    } else if (!do_register && global_data.rx_filter_id != nullptr) {
        T_DG(TRACE_GRP_CFG, "Unregister packet filter");

        if ((rc = packet_rx_filter_unregister(global_data.rx_filter_id)) != VTSS_RC_OK) {
            T_EG(TRACE_GRP_CFG, "Error unregistering packet filter: %u", rc);
        }

        global_data.rx_filter_id = NULL;
    }
}

/* **************************************************************************
 * Internal configuration handler functions
 * **************************************************************************
 */

static mesa_rc status_get(vtss_appl_dhcp6_snooping_global_status_t *const status)
{
    status->last_change_ts = global_data.last_change_ts;

    return VTSS_RC_OK;
}

static void conf_apply(void)
{
    // At this point we expect that the configuration has changed in some way.

    T_NG(TRACE_GRP_CFG, "Applying configuration");

    if (global_conf.snooping_mode == DHCP6_SNOOPING_MODE_ENABLED) {
        // Snooping is enabled as a whole
        add_basic_switch_rules();
        rx_filter_register(true);

        start_client_cleanup_timer(IDLE_CLIENTS_CHECK_TIMER_SECS);
    } else {
        // Snooping is now disabled as a whole
        remove_switch_rules();
        rx_filter_register(false);

        for (auto it = registered_clients_table.begin(); it != registered_clients_table.end(); it++) {
            for (auto ita = it->second.if_map.begin(); ita != it->second.if_map.end(); ita++) {
                notify_registered_callbacks(&it->second, &ita->second, DHCP6_SNOOPING_INFO_REASON_MODE_DISABLED);
            }
        }

        registered_clients_table.clear();

        stop_client_cleanup_timer();
    }

    set_snooping_table_timestamp();
}

/*
 * Get DHCP snooping defaults
 */
static void conf_get_default(vtss_appl_dhcp6_snooping_conf_t *conf)
{
    // clear all memory claimed by the conf struct due to struct alignment
    vtss_clear(*conf);

    conf->snooping_mode = DHCP6_SNOOPING_MODE_DISABLED;
    conf->nh_unknown_mode = DHCP6_SNOOPING_NH_UNKNOWN_MODE_DROP;
}

static mesa_rc conf_get(vtss_appl_dhcp6_snooping_conf_t *conf)
{
    T_NG(TRACE_GRP_CFG, "enter");

    if (conf == NULL) {
        T_W("Config pointer is NULL");
        return DHCP6_SNOOPING_ERROR_INV_PARAM;
    }

    *conf = global_conf;

    return VTSS_RC_OK;
}

static bool is_conf_changed(const vtss_appl_dhcp6_snooping_conf_t *old_conf,
                            const vtss_appl_dhcp6_snooping_conf_t *new_conf)
{
    return (memcmp(new_conf, old_conf, sizeof(*new_conf)) != 0);
}

static mesa_rc conf_set(const vtss_appl_dhcp6_snooping_conf_t *const conf)
{
    T_NG(TRACE_GRP_CFG, "enter");

    if (conf == NULL) {
        T_W("Config pointer is NULL");
        return DHCP6_SNOOPING_ERROR_INV_PARAM;
    }

    if (!is_conf_changed(&global_conf, conf)) {
        return VTSS_RC_OK;
    }

    global_conf = *conf;

    // Apply configuration
    conf_apply();

    return VTSS_RC_OK;
}

static void port_conf_get_default(vtss_appl_dhcp6_snooping_port_conf_t *port_cfg)
{
    // clear all memory claimed by the port_cfg struct due to struct alignment
    memset(port_cfg, 0, sizeof(*port_cfg));

    port_cfg->trust_mode = DHCP6_SNOOPING_PORT_MODE_UNTRUSTED;
}

static mesa_rc port_conf_get(mesa_port_no_t port_no, vtss_appl_dhcp6_snooping_port_conf_t *port_cfg)
{
    T_NG(TRACE_GRP_CFG, "enter");

    if (port_cfg == NULL) {
        T_W("Config pointer is NULL");
        return DHCP6_SNOOPING_ERROR_INV_PARAM;
    }

    port_conf_table.get(port_no, port_cfg);

    return VTSS_RC_OK;
}

static bool is_port_conf_changed(const vtss_appl_dhcp6_snooping_port_conf_t *const old_conf,
                                 const vtss_appl_dhcp6_snooping_port_conf_t *const new_conf)
{
    return (memcmp(new_conf, old_conf, sizeof(*new_conf)) != 0);
}

static mesa_rc port_conf_set(mesa_port_no_t port_no, const vtss_appl_dhcp6_snooping_port_conf_t *port_conf)
{
    mesa_rc rc;
    vtss_appl_dhcp6_snooping_port_conf_t old_conf;

    /* check parameter */
    if (port_conf == NULL) {
        T_W("port_conf == NULL\n");
        return DHCP6_SNOOPING_ERROR_INV_PARAM;
    }

    if ((rc = port_conf_get(port_no, &old_conf)) != VTSS_RC_OK) {
        return rc;
    }

    if (!is_port_conf_changed(&old_conf, port_conf)) {
        return VTSS_RC_OK;
    }

    port_conf_table.set(port_no, port_conf);

    // update cached trust state for port
    if (port_conf->trust_mode == DHCP6_SNOOPING_PORT_MODE_UNTRUSTED) {
        global_data.trusted_ports.clr(port_no);
    } else {
        global_data.trusted_ports.set(port_no);
    }

    // Apply configuration
    conf_apply();

    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize our private mib */
VTSS_PRE_DECLS void dhcp6_snooping_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_dhcp6_snooping_json_init(void);
#endif

/*
 * Initialize snooping module at system startup
 */
static void snooping_init()
{
    vtss_appl_dhcp6_snooping_conf_t conf;
    vtss_appl_dhcp6_snooping_port_conf_t  default_port_conf;

    T_DG(TRACE_GRP_CFG, "Enter");

    callback_list.clear();

    setup_free_qceids();

    conf_get_default(&conf);
    global_conf = conf;

    port_conf_get_default(&default_port_conf);
    for (mesa_port_no_t portno = 0; portno < port_count_max(); portno++) {
        // set default port config entry
        port_conf_table.set(portno, &default_port_conf);

        // set default port stats entry
        auto stats_entry = vtss_appl_dhcp6_snooping_port_statistics_t();
        vtss_clear(stats_entry);
        port_stats_table.push_back(stats_entry);
    }
}

/*
 * Start snooping module
 */
static void snooping_start()
{
    T_DG(TRACE_GRP_CFG, "Enter");

    // TODO ?

}

/*
 * Restore snooping configuration to default
 */
static void restore_to_default(void)
{
    T_DG(TRACE_GRP_CFG, "Enter");

    vtss_appl_dhcp6_snooping_conf_t conf;
    vtss_appl_dhcp6_snooping_port_conf_t  default_port_conf;

    conf_get_default(&conf);
    global_conf = conf;

    port_conf_table.clear();
    global_data.trusted_ports.clear_all();

    port_conf_get_default(&default_port_conf);

    for (mesa_port_no_t portno = 0; portno < port_count_max(); portno++) {
        port_conf_table.set(portno, &default_port_conf);
    }

    stop_lease_expiry_timer();

    conf_apply();
}

/* **************************************************************************
 * Packet handling functions
 * **************************************************************************
 */

static void _incr_stats_counter(mesa_port_no_t port_no, dhcp6::MessageType message_type, bool is_tx)
{
    if (port_no >= port_stats_table.size()) {
        return;
    }

    vtss_appl_dhcp6_snooping_port_statistics_dir_t *cnt = is_tx ?
                                                          &port_stats_table[port_no].tx : &port_stats_table[port_no].rx;

    switch (message_type) {
    case dhcp6::DHCP6SOLICIT:
        cnt->solicit++;
        break;
    case dhcp6::DHCP6ADVERTISE:
        cnt->advertise++;
        break;
    case dhcp6::DHCP6REQUEST:
        cnt->request++;
        break;
    case dhcp6::DHCP6CONFIRM:
        cnt->confirm++;
        break;
    case dhcp6::DHCP6RENEW:
        cnt->renew++;
        break;
    case dhcp6::DHCP6REBIND:
        cnt->rebind++;
        break;
    case dhcp6::DHCP6REPLY:
        cnt->reply++;
        break;
    case dhcp6::DHCP6RELEASE:
        cnt->release++;
        break;
    case dhcp6::DHCP6DECLINE:
        cnt->decline++;
        break;
    case dhcp6::DHCP6RECONFIGURE:
        cnt->reconfigure++;
        break;
    case dhcp6::DHCP6INFORMATION_REQUEST:
        cnt->infoRequest++;
        break;

    default:
        // ignore rest for now
        break;
    }
}

/*
 * Increment TX statistics counters for the message type
 */
static void incr_tx_stats_counter(mesa_port_no_t port_no, dhcp6::MessageType message_type)
{
    _incr_stats_counter(port_no, message_type, true);
}

/*
 * Increment RX statistics counters for the message type
 */
static void incr_rx_stats_counter(mesa_port_no_t port_no, dhcp6::MessageType message_type)
{
    _incr_stats_counter(port_no, message_type, false);
}

static void collect_address_entry(registered_clients_info_t *pclient,
                                  vtss_appl_dhcp6_snooping_iaid_t iaid,
                                  const client_interface_info_t &rx_state,
                                  dhcp6_address_state_t address_state,
                                  int32_t transaction_id = -1,
                                  int32_t vid = -1,
                                  bool rapid_commit = false)
{
    T_NG(TRACE_GRP_STATE, "Collect if_map entry for IAID %u, state %u", iaid, address_state);

    auto iter = pclient->if_map.find(iaid);
    if (iter != pclient->if_map.end()) {
        // item already exist - update selected address values
        T_NG(TRACE_GRP_STATE, "updating if_map entry for IAID %u", iaid);
        pclient->if_map[iaid].iaid = iaid;
        pclient->if_map[iaid].valid = rx_state.valid;
        pclient->if_map[iaid].ip_address = rx_state.ip_address;

        if (address_state != DHCP6_SNOOPING_ADDRESS_STATE_NOSTATE) {
            pclient->if_map[iaid].address_state = address_state;
            pclient->if_map[iaid].lease_time = rx_state.lease_time;
        }
    } else {
        // item did not exist - create new item
        T_NG(TRACE_GRP_STATE, "creating new if_map entry for IAID %u", iaid);
        pclient->if_map.set(iaid, rx_state);

        if (address_state == DHCP6_SNOOPING_ADDRESS_STATE_NOSTATE) {
            pclient->if_map[iaid].address_state = DHCP6_SNOOPING_ADDRESS_STATE_REQUEST;
        } else {
            pclient->if_map[iaid].address_state = address_state;
        }
    }

    pclient->if_map[iaid].rapid_commit = rapid_commit;

    if (transaction_id >= 0) {
        pclient->if_map[iaid].transaction_id = (uint32_t)transaction_id;
    }

    // Set assigned VID
    if (vid >= 0) {
        T_NG(TRACE_GRP_STATE, "Assigning VID %u to interface %u", vid, iaid);
        pclient->if_map[iaid].vid = (mesa_vid_t)vid;
    }

    if (address_state == DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED) {
        pclient->if_map[iaid].assigned_time = get_curr_timestamp();
    }
}

/*
 * Add or update indicated address items to map but leave existing ones in place,
 * as they will be removed by the cleanup and expiry timers.
 */
static void collect_address_entries(registered_clients_info_t *pclient,
                                    const interface_address_map_t &if_map,
                                    dhcp6_address_state_t address_state,
                                    int32_t transaction_id,
                                    int32_t vid,
                                    bool rapid_commit = false)
{
    if (if_map.size() == 0) {
        T_NG(TRACE_GRP_STATE, "if_map is empty - cannot collect address entries");
        return;
    }

    for (auto aiter = if_map.begin(); aiter != if_map.end(); aiter++) {
        collect_address_entry(pclient, aiter->first, aiter->second, address_state,
                              transaction_id, vid, rapid_commit);
    }
}

static void release_assigned_address(registered_clients_info_t *pclient,
                                     vtss_appl_dhcp6_snooping_iaid_t iaid)
{
    vtss_appl_dhcp6_snooping_assigned_ip_t address_entry;

    // Remove the address entries from the server message
    auto iter = pclient->if_map.find(iaid);
    if (iter != pclient->if_map.end()) {
        // make a copy of the entry for the sake of callbacks
        address_entry = iter->second;

        // Erase the entry
        pclient->if_map.erase(iaid);

        DHCP6_SNOOPING_CRIT_EXIT();
        notify_registered_callbacks(pclient, &address_entry, DHCP6_SNOOPING_INFO_REASON_RELEASE);
        DHCP6_SNOOPING_CRIT_ENTER();
    }
}

/*
 * Handle message received from a DHCP client
 */
static void handle_client_message(const u8 *const packet, packet_parser &parser,
                                  const mesa_packet_rx_info_t *const rx_info)
{
    EthHeader   *eth_h;
    registered_clients_info_t *pclient;
    dhcp_duid_t client_duid = parser.get_client_duid();
    uint32_t transaction_id = parser.get_transaction_id();
    dhcp6::MessageType message_type = parser.get_dhcp_message_type();
    dhcp6::StatusCode status_code = parser.get_dhcp_status_code();
    int32_t client_vid = rx_info->tag.vid;

    eth_h = (EthHeader *)packet;

    T_DG(TRACE_GRP_STATE, "Client message on port %u from %s, VID %u, Tr.ID 0x%06X, STATUS %u",
         rx_info->port_no, mac2str(eth_h->smac), client_vid, transaction_id,
         status_code);

    if (status_code != dhcp6::STATUS_SUCCESS) {
        return;
    }

    // find associated entry in client snooping table
    auto iter = registered_clients_table.find(client_duid);
    if (iter == registered_clients_table.end()) {
        // entry not found - setup a new entry
        T_NG(TRACE_GRP_STATE, "Unknown client - adding new entry");

        registered_clients_info_t client_item;

        vtss_clear(client_item);
        client_item.duid = client_duid;
        client_item.mac = eth_h->smac;
        client_item.port_no = rx_info->port_no;
        client_item.if_index = get_ifindex_from_port(rx_info->port_no);
        client_item.link_local_ip_address = parser.get_ipv6_src_address();

        registered_clients_table[parser.get_client_duid()] = client_item;
    }

    pclient = &registered_clients_table[parser.get_client_duid()];

    if (iter != registered_clients_table.end()) {
        // Check for various changes in the existing client state
        if (pclient->port_no != rx_info->port_no) {
            T_NG(TRACE_GRP_STATE, "Client changed ingress port from %u to %u", pclient->port_no, rx_info->port_no);

            pclient->port_no = rx_info->port_no;
        }
    }

    pclient->last_access_time = get_curr_timestamp();

    switch (message_type) {
    case dhcp6::DHCP6SOLICIT:
        T_DG(TRACE_GRP_STATE, "Client SOLICIT message");

        collect_address_entries(pclient, parser.get_if_map(),
                                DHCP6_SNOOPING_ADDRESS_STATE_SOLICIT,
                                parser.get_transaction_id(),
                                client_vid,
                                parser.get_rapid_commit());
        break;

    case dhcp6::DHCP6REQUEST:
        T_DG(TRACE_GRP_STATE, "Client REQUEST message");

        collect_address_entries(pclient, parser.get_if_map(),
                                DHCP6_SNOOPING_ADDRESS_STATE_REQUEST,
                                parser.get_transaction_id(),
                                client_vid);
        break;

    case dhcp6::DHCP6INFORMATION_REQUEST:
        T_DG(TRACE_GRP_STATE, "Client INFO REQUEST message");
        break;

    case dhcp6::DHCP6CONFIRM:
        T_DG(TRACE_GRP_STATE, "Client CONFIRM message");

        collect_address_entries(pclient, parser.get_if_map(),
                                DHCP6_SNOOPING_ADDRESS_STATE_CONFIRM,
                                parser.get_transaction_id(),
                                client_vid);
        break;

    case dhcp6::DHCP6RENEW:
        T_DG(TRACE_GRP_STATE, "Client RENEW message");

        // Collect new or update existing entry.
        collect_address_entries(pclient, parser.get_if_map(),
                                DHCP6_SNOOPING_ADDRESS_STATE_NOSTATE,
                                parser.get_transaction_id(),
                                client_vid);
        break;

    case dhcp6::DHCP6REBIND:
        T_DG(TRACE_GRP_STATE, "Client REBIND message");

        // Collect new or update existing entry.
        collect_address_entries(pclient, parser.get_if_map(),
                                DHCP6_SNOOPING_ADDRESS_STATE_REBIND,
                                parser.get_transaction_id(),
                                client_vid);
        break;

    case dhcp6::DHCP6DECLINE:
        T_DG(TRACE_GRP_STATE, "Client DECLINE message");
        break;

    case dhcp6::DHCP6RELEASE:
        T_DG(TRACE_GRP_STATE, "Client RELEASE message");

        collect_address_entries(pclient, parser.get_if_map(),
                                DHCP6_SNOOPING_ADDRESS_STATE_RELEASING,
                                parser.get_transaction_id(),
                                client_vid);
        break;

    default:
        T_DG(TRACE_GRP_STATE, "Unhandled client message (%u)", message_type);
        break;
    }

    incr_rx_stats_counter(pclient->port_no, message_type);

    set_snooping_table_timestamp();
}

/*
 * Handle a server REPLY message depending on the client state
 */
static void handle_server_reply_message(packet_parser &parser,
                                        registered_clients_info_t *pclient,
                                        bool &erase_pclient)
{
    dhcp6::StatusCode status_code = parser.get_dhcp_status_code();
    const interface_address_map_t &rx_if_map = parser.get_if_map();
    uint32_t transaction_id = parser.get_transaction_id();

    // update last access time
    pclient->last_access_time = get_curr_timestamp();

    if (status_code != dhcp6::STATUS_SUCCESS) {
        T_DG(TRACE_GRP_STATE, "Server REPLY message, status_code is %u", status_code);
        return;
    }

    // check all interface address entries on this client for transaction ID
    auto ait = pclient->if_map.begin();
    while (ait != pclient->if_map.end()) {
        // get convenience variables
        auto iaid = ait->first;
        auto address_entry = &ait->second;
        auto next_ait = ait;
        ++next_ait;

        // only deal with address entries belong to transaction ID in received REPLY
        if (transaction_id != address_entry->transaction_id) {
            ait = next_ait;
            continue;
        }

        // pre-fetch any matching entry in received address map
        auto rx_ait = rx_if_map.find(iaid);

        switch (address_entry->address_state) {
        case DHCP6_SNOOPING_ADDRESS_STATE_SOLICIT:
            if (rx_ait == rx_if_map.end()) {
                // no address entries in this reply for this IAID
                T_DG(TRACE_GRP_STATE, "Server REPLY message for requesting IAID %u, no matching entry", iaid);
                break;
            }

            // if the client issued a SOLICIT with a RapidCommit option we may get
            // an address in this reply.
            if (!address_entry->rapid_commit || !parser.get_rapid_commit()) {
                T_DG(TRACE_GRP_STATE, "Server REPLY message while SOLICITING for requesting IAID %u, no RapidCommit %u/%u",
                     iaid, address_entry->rapid_commit, parser.get_rapid_commit());
                break;
            }

            // collect data from REPLY
            collect_address_entry(pclient, iaid, rx_ait->second, DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED);

            // update DHCP server address from parser
            address_entry->dhcp_server_ip = parser.get_ipv6_src_address();

            DHCP6_SNOOPING_CRIT_EXIT();
            notify_registered_callbacks(pclient, address_entry, DHCP6_SNOOPING_INFO_REASON_ASSIGNED);
            DHCP6_SNOOPING_CRIT_ENTER();
            break;

        case DHCP6_SNOOPING_ADDRESS_STATE_REBIND:
        case DHCP6_SNOOPING_ADDRESS_STATE_REQUEST:
            // we are waiting for an address - update this entry
            if (rx_ait == rx_if_map.end()) {
                // no address entries in this reply for this IAID
                T_DG(TRACE_GRP_STATE, "Server REPLY message for requesting IAID %u, no matching entry", iaid);
                break;
            }

            // collect data from REPLY
            collect_address_entry(pclient, iaid, rx_ait->second, DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED);

            // update DHCP server address from parser
            address_entry->dhcp_server_ip = parser.get_ipv6_src_address();

            DHCP6_SNOOPING_CRIT_EXIT();
            notify_registered_callbacks(pclient, address_entry, DHCP6_SNOOPING_INFO_REASON_ASSIGNED);
            DHCP6_SNOOPING_CRIT_ENTER();
            break;

        case DHCP6_SNOOPING_ADDRESS_STATE_CONFIRM:
            // Client wants to that the address is still OK.
            // Confirm replies do not contain IAID entries.
            address_entry->address_state = DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED;
            address_entry->assigned_time = get_curr_timestamp();
            address_entry->dhcp_server_ip = parser.get_ipv6_src_address();
            break;

        case DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED:
            // Probably a reply to a simple renew message
            if (rx_ait == rx_if_map.end()) {
                // no address entries in this reply for this IAID
                T_DG(TRACE_GRP_STATE, "Server REPLY message for requesting IAID %u, no matching entry", iaid);
                break;
            }

            address_entry->assigned_time = get_curr_timestamp();
            break;

        case DHCP6_SNOOPING_ADDRESS_STATE_RELEASING:
            // we are releasing this address
            T_NG(TRACE_GRP_STATE, "Address entry is in state RELEASING");
            release_assigned_address(pclient, iaid);

            if (pclient->if_map.size() == 0) {
                // No more addresses for this client - remove it
                T_NG(TRACE_GRP_STATE, "Client has no more addresses - removing");
                erase_pclient = true;
            } else {
                T_NG(TRACE_GRP_STATE, "Client still has addresses");
            }

            break;

        default:
            // ignore
            break;
        }

        ait = next_ait;
    }

    // start lease-time expiry timer
    set_next_lease_expiry_timeout();

    // Update snooping table last-changed timestamp
    set_snooping_table_timestamp();
}

/*
 * Handle message received from a DHCP server.
 * Return true if the packet should be forwarded and false if it should be dropped.
 * Returns a reference to the associated client entry if found.
 */
static bool handle_server_message(const u8 *const packet, packet_parser &parser,
                                  const mesa_packet_rx_info_t *const rx_info,
                                  registered_clients_info_t *&pclient,
                                  bool &erase_pclient)
{
    registered_clients_iter_t citer;
    dhcp6::MessageType message_type = parser.get_dhcp_message_type();
    uint32_t transaction_id = parser.get_transaction_id();

    T_DG(TRACE_GRP_STATE, "Server message on port %u with Tr.ID 0x%06X", rx_info->port_no, transaction_id);

    incr_rx_stats_counter(rx_info->port_no, message_type);

    pclient = nullptr;

    bool is_trusted_port = global_data.trusted_ports.get(rx_info->port_no);
    if (!is_trusted_port) {
        T_DG(TRACE_GRP_STATE, "Server message received from untrusted port - discarding");
        port_stats_table[rx_info->port_no].rxDiscardUntrust++;
        return false;
    }

    citer = registered_clients_table.find(parser.get_client_duid());
    if (citer == registered_clients_table.end()) {
        // We did not know this client
        T_DG(TRACE_GRP_STATE, "Could not find client DUID");
        return true;
    }

    pclient = &citer->second;

    switch (message_type) {
    case dhcp6::DHCP6ADVERTISE:
        T_DG(TRACE_GRP_STATE, "Server ADVERTISE message");
        break;

    case dhcp6::DHCP6REPLY:
        T_DG(TRACE_GRP_STATE, "Server REPLY message");

        handle_server_reply_message(parser, pclient, erase_pclient);
        // pclient is still valid after this call, but if erase_pclient is true,
        // the caller of us must erase it from the map.
        break;

    default:
        T_RG(TRACE_GRP_STATE, "Unhandled server message (%u)", message_type);
        break;
    }

    return true;
}


/*
 * Forward a packet and let the switch determine which TX ports to use.
 */
static bool forward_packet_switch(const u8 *const packet, const size_t len,
                                  const mesa_packet_rx_info_t *const rx_info)
{
    mesa_rc rc;
    u8 *buffer;
    uchar vlan_tag[4];
    size_t full_len = len;
    packet_tx_props_t tx_props;
    bool add_tag = false;

    T_NG(TRACE_GRP_TXPKT, "Forwaring packet through the switch");

    packet_tx_props_init(&tx_props);

    if (rx_info->stripped_tag.vid != MESA_VID_NULL) {
        add_tag = true;
        full_len += sizeof(vlan_tag);
    }

    // Allocate new buffer for each port and copy received packet
    buffer = packet_tx_alloc(full_len);
    if (!buffer) {
        T_DG(TRACE_GRP_TXPKT, "Error: Could not allocate TX buffer");
        return false;
    }

    if (add_tag) {
        memset(vlan_tag, 0x0, sizeof(vlan_tag));
        pack16(rx_info->stripped_tag.tpid, vlan_tag);
        pack16(rx_info->stripped_tag.vid, vlan_tag + 2);
    }

    memcpy(buffer, packet, 12); // DMAC & SMAC
    if (add_tag) {
        memcpy(buffer + 12, vlan_tag, sizeof(vlan_tag)); // VLAN Header
        memcpy(buffer + 12 + sizeof(vlan_tag), packet + 12, len - 12); // Remainder of frame
    } else {
        memcpy(buffer + 12, packet + 12, len - 12); // Remainder of frame
    }

    tx_props.packet_info.modid      = VTSS_MODULE_ID_DHCP6_SNOOPING;
    tx_props.packet_info.frm        = buffer;
    tx_props.packet_info.no_free    = FALSE;
    tx_props.packet_info.len        = len;

    tx_props.tx_info.tag            = rx_info->tag;
    tx_props.tx_info.switch_frm     = TRUE;
    tx_props.tx_info.dst_port_mask  = 0;

    if ((rc = packet_tx(&tx_props)) != VTSS_RC_OK) {
        T_DG(TRACE_GRP_TXPKT, "Error: Frame transmit failed, rc:%u", rc);
        packet_tx_free(buffer);
        return false;
    }

    return true;
}

/*
 * Forward a packet to a specific client port
 */
static bool forward_packet_client_port(const u8 *const packet, const size_t len,
                                       packet_parser &parser,
                                       registered_clients_info_t *pclient,
                                       const mesa_packet_rx_info_t *const rx_info)
{
    bool success = true;
    mesa_rc rc;
    u8 *buffer;
    packet_tx_props_t tx_props;

    T_NG(TRACE_GRP_TXPKT, "Forwarding packet to client port %u", pclient->port_no);

    packet_tx_props_init(&tx_props);

    // Allocate new buffer for each port and copy received packet
    buffer = packet_tx_alloc(len);
    if (!buffer) {
        T_DG(TRACE_GRP_TXPKT, "Error: Could not allocate TX buffer");
        return false;
    }
    memcpy(buffer, packet, len);

    tx_props.packet_info.modid      = VTSS_MODULE_ID_DHCP6_SNOOPING;
    tx_props.packet_info.frm        = buffer;
    tx_props.packet_info.no_free    = FALSE;
    tx_props.packet_info.len        = len;

    tx_props.tx_info.switch_frm     = FALSE;
    tx_props.tx_info.dst_port_mask  = VTSS_BIT64(pclient->port_no);

    tx_props.tx_info.tag.tpid       = 0;
    tx_props.tx_info.tag.vid        = rx_info->tag.vid;

    if ((rc = packet_tx(&tx_props)) != VTSS_RC_OK) {
        T_DG(TRACE_GRP_TXPKT, "Error: Frame transmit on port %d failed, rc:%u", pclient->port_no, rc);
        success = false;

        packet_tx_free(buffer);
    } else {
        // Packet TX OK - update TX statistics
        incr_tx_stats_counter(pclient->port_no, parser.get_dhcp_message_type());
    }

    return success;
}

/*
 * Forward a packet to trusted ports only.
 */
static bool forward_packet_trusted_ports(const u8 *const packet, const size_t len,
                                         packet_parser &parser,
                                         const mesa_packet_rx_info_t *const rx_info)
{
    bool success = true;
    mesa_rc rc;
    u8 *buffer;
    mesa_port_no_t port_no;
    packet_tx_props_t tx_props;
    mesa_packet_frame_info_t frame_info;
    mesa_packet_filter_t filter;

    T_NG(TRACE_GRP_TXPKT, "Forwarding packet (%u bytes) to trusted ports only", len);

    for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
        // Don't forward to ingress port
        if (port_no == rx_info->port_no) {
            continue;
        }
        // don't forward to untrusted ports
        if (!global_data.trusted_ports.get(port_no)) {
            T_RG(TRACE_GRP_TXPKT, "Block forwarding packet to untrusted port %u", port_no);
            continue;
        }

        mesa_packet_frame_info_init(&frame_info);
        frame_info.port_no = rx_info->port_no;
        frame_info.port_tx = port_no;
        frame_info.vid = rx_info->tag.vid;

        rc = mesa_packet_frame_filter(NULL, &frame_info, &filter);

        if (rc == VTSS_RC_OK && filter == MESA_PACKET_FILTER_DISCARD) {
            T_DG(TRACE_GRP_TXPKT, "Packet discarded by frame filter on port %d", port_no);
            continue;
        }

        T_NG(TRACE_GRP_TXPKT, "Forwarding packet to port %u", port_no);

        // Allocate new buffer for each port and copy received packet
        buffer = packet_tx_alloc(len);
        if (!buffer) {
            T_DG(TRACE_GRP_TXPKT, "Error: Could not allocate TX buffer");
            return false;
        }
        memcpy(buffer, packet, len);

        packet_tx_props_init(&tx_props);
        tx_props.packet_info.modid      = VTSS_MODULE_ID_DHCP6_SNOOPING;
        tx_props.packet_info.frm        = buffer;
        tx_props.packet_info.no_free    = FALSE;
        tx_props.packet_info.len        = len;

        tx_props.tx_info.switch_frm     = FALSE;
        tx_props.tx_info.dst_port_mask  = VTSS_BIT64(port_no);

        if (filter == MESA_PACKET_FILTER_TAGGED) {
            // packets need a VID to be forwarded on the server port
            tx_props.tx_info.tag.tpid       = rx_info->tag.tpid;
            tx_props.tx_info.tag.vid        = rx_info->tag.vid;
            tx_props.tx_info.tag.pcp        = rx_info->tag.pcp;
            tx_props.tx_info.tag.dei        = rx_info->tag.dei;
        } else {
            // packets should be forwarded untagged on the server port
            tx_props.tx_info.tag.tpid       = 0;
            tx_props.tx_info.tag.vid        = rx_info->tag.vid;
        }

        if ((rc = packet_tx(&tx_props)) != VTSS_RC_OK) {
            if (rc == VTSS_RC_INV_STATE) {
                // Packet TX was discarded on port due to switch configuration.
                // This is OK, since we did not bother to figure this out ourselves,
                // but we need to free the packet buffer.
                T_NG(TRACE_GRP_TXPKT, "Frame discarded on port %d", port_no);
            } else {
                T_DG(TRACE_GRP_TXPKT, "Error: Frame transmit on port %d failed, rc:%u", port_no, rc);
                success = false;
            }

            packet_tx_free(buffer);
        } else {
            // Packet TX OK - update TX statistics
            incr_tx_stats_counter(port_no, parser.get_dhcp_message_type());
        }
    }

    return success;
}

/*
 * Handle packets received from the packet module
 */
static BOOL handle_rx_packet(const u8 *const packet, const mesa_packet_rx_info_t *const rx_info)
{
    const size_t len = rx_info->length;
    parse_result_t parse_result;
    bool is_consumed = false;
    registered_clients_info_t *pclient = nullptr;
    bool erase_pclient = false;

    CRIT_ENTER_EXIT();

    T_NG(TRACE_GRP_RXPKT, "--------------------------");
    T_NG(TRACE_GRP_RXPKT, "RX packet, port %u, ACL hit:%d", rx_info->port_no, rx_info->acl_hit);
    T_RG_HEX(TRACE_GRP_RXPKT, packet, len);

    if (rx_info->port_no == MESA_PORT_NO_NONE) {
        // Cannot handle packets with no ingress port info
        T_DG(TRACE_GRP_RXPKT, "RX packet port is NONE");
        return false;
    }

    packet_parser parser(IPV6_MAX_EXT_HEADER_DEPTH);
    parse_result = parser.parse_packet(packet, len);

    switch (parse_result) {
    case PARSE_RESULT_COMPLETE:
        switch (parser.get_dhcp_packet_type()) {
        case PACKET_TYPE_DHCP6_CLIENT_MESSAGE:
            handle_client_message(packet, parser, rx_info);

            // We consume all DHCPv6 messages
            is_consumed = true;

            // forward client messages to trusted ports only
            forward_packet_trusted_ports(packet, len, parser, rx_info);
            break;

        case PACKET_TYPE_DHCP6_SERVER_MESSAGE:
            if (handle_server_message(packet, parser, rx_info, pclient, erase_pclient)) {
                // server message OK - we should forward it to client(s)
                if (pclient != nullptr) {
                    // This message is intended for a specific client
                    forward_packet_client_port(packet, len, parser, pclient, rx_info);
                    if (erase_pclient) {
                        registered_clients_table.erase(pclient->duid);
                    }
                } else {
                    // we don't know the client - let the switch forward the packet
                    forward_packet_switch(packet, len, rx_info);
                }
            }
            // We consume all DHCPv6 messages
            is_consumed = true;
            break;
        }
        break;

    case PARSE_RESULT_COMPLETE_NOT_DHCP6:
        is_consumed = false;
        // forward the packet
        forward_packet_switch(packet, len, rx_info);
        break;

    case PARSE_RESULT_INCOMPLETE_EH:
        if (global_conf.nh_unknown_mode == DHCP6_SNOOPING_NH_UNKNOWN_MODE_DROP) {
            // consume and drop the packet
            T_NG(TRACE_GRP_RXPKT, "Dropping packet with incomplete ETH header");
            return true;
        }

        is_consumed = false;
        forward_packet_switch(packet, len, rx_info);
        break;
    }

    return is_consumed;
}


} // namespace dhcp6_snooping


/* **************************************************************************
 * Public API funktions
 * **************************************************************************
 */

mesa_rc vtss_appl_dhcp6_snooping_global_status_get(
    vtss_appl_dhcp6_snooping_global_status_t   *const status)
{
    CRIT_ENTER_EXIT();
    return dhcp6_snooping::status_get(status);
}

mesa_rc vtss_appl_dhcp6_snooping_conf_get(vtss_appl_dhcp6_snooping_conf_t *const conf)
{
    CRIT_ENTER_EXIT();
    return dhcp6_snooping::conf_get(conf);
}

void vtss_appl_dhcp6_snooping_conf_get_default(vtss_appl_dhcp6_snooping_conf_t *conf)
{
    dhcp6_snooping::conf_get_default(conf);
}

mesa_rc vtss_appl_dhcp6_snooping_conf_set(const vtss_appl_dhcp6_snooping_conf_t *const conf)
{
    CRIT_ENTER_EXIT();
    return dhcp6_snooping::conf_set(conf);
}

bool vtss_appl_dhcp6_snooping_conf_changed(const vtss_appl_dhcp6_snooping_conf_t *const old_conf,
                                           const vtss_appl_dhcp6_snooping_conf_t *const new_conf)
{
    return dhcp6_snooping::is_conf_changed(old_conf, new_conf);
}

mesa_rc vtss_appl_dhcp6_snooping_port_conf_get(vtss_ifindex_t  ifindex,
                                               vtss_appl_dhcp6_snooping_port_conf_t *const port_conf)
{
    CRIT_ENTER_EXIT();

    mesa_port_no_t port_no = dhcp6_snooping::get_port_from_ifindex(ifindex);
    if (port_no == MESA_PORT_NO_NONE) {
        return VTSS_RC_ERROR;
    }

    return dhcp6_snooping::port_conf_get(port_no, port_conf);
}

void vtss_appl_dhcp6_snooping_port_conf_get_default(vtss_appl_dhcp6_snooping_port_conf_t *port_conf)
{
    dhcp6_snooping::port_conf_get_default(port_conf);
}

bool vtss_appl_dhcp6_snooping_port_conf_changed(const vtss_appl_dhcp6_snooping_port_conf_t *const old_conf,
                                                const vtss_appl_dhcp6_snooping_port_conf_t *const new_conf)
{
    return dhcp6_snooping::is_port_conf_changed(old_conf, new_conf);
}

mesa_rc vtss_appl_dhcp6_snooping_port_conf_itr(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex)
{
    return vtss_appl_iterator_ifindex_front_port(prev_ifindex, next_ifindex);
}

mesa_rc vtss_appl_dhcp6_snooping_port_conf_set(vtss_ifindex_t  ifindex,
                                               const vtss_appl_dhcp6_snooping_port_conf_t      *const port_conf)
{
    CRIT_ENTER_EXIT();

    mesa_port_no_t port_no = dhcp6_snooping::get_port_from_ifindex(ifindex);
    if (port_no == MESA_PORT_NO_NONE) {
        return VTSS_RC_ERROR;
    }

    return dhcp6_snooping::port_conf_set(port_no, port_conf);
}

mesa_rc vtss_appl_dhcp6_snooping_assigned_ip_info_register(vtss_appl_dhcp6_snooping_ip_assigned_info_callback_t cb)
{
    CRIT_ENTER_EXIT();

    if (dhcp6_snooping::callback_list.size() >= dhcp6_snooping::max_callback_list_entries) {
        return DHCP6_SNOOPING_ERROR_CB_COUNT;
    }

    dhcp6_snooping::callback_list.push_back(cb);

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_dhcp6_snooping_assigned_ip_info_unregister(vtss_appl_dhcp6_snooping_ip_assigned_info_callback_t cb)
{
    CRIT_ENTER_EXIT();

    auto it = find(dhcp6_snooping::callback_list.begin(), dhcp6_snooping::callback_list.end(), cb);

    if (it == dhcp6_snooping::callback_list.end()) {
        return DHCP6_SNOOPING_ERROR_CB_NOTFOUND;
    }

    dhcp6_snooping::callback_list.erase(it);

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_dhcp6_snooping_client_info_itr(
    const vtss_appl_dhcp6_snooping_duid_t   *const prev_duid,
    vtss_appl_dhcp6_snooping_duid_t         *const next_duid)
{
    CRIT_ENTER_EXIT();

    auto duid = (prev_duid != nullptr) ? dhcp_duid_t(prev_duid) : dhcp_duid_t();
    registered_clients_info_t *info;

    if (next_duid == nullptr) {
        return VTSS_RC_ERR_PARM;
    }

    // skip the incomplete entries in the table
    do {
        auto iter = vtss::upper_bound(dhcp6_snooping::registered_clients_table.begin(),
                                      dhcp6_snooping::registered_clients_table.end(), duid,
                                      compare_registered_clients_info);
        if (iter == dhcp6_snooping::registered_clients_table.end()) {
            return VTSS_RC_ERROR;
        }

        duid = iter->first;
        info = &iter->second;

    } while (!info->has_assigned_addresses());

    *next_duid = (vtss_appl_dhcp6_snooping_duid_t)duid;

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_dhcp6_snooping_client_info_get(const vtss_appl_dhcp6_snooping_duid_t   &duid_val,
                                                 vtss_appl_dhcp6_snooping_client_info_t  *const client_info)
{
    auto duid = dhcp_duid_t(duid_val);

    if (client_info == nullptr) {
        return VTSS_RC_ERR_PARM;
    }

    CRIT_ENTER_EXIT();

    auto iter = dhcp6_snooping::registered_clients_table.find(duid);
    if (iter == dhcp6_snooping::registered_clients_table.end()) {
        return VTSS_RC_ERROR;
    }

    *client_info = (vtss_appl_dhcp6_snooping_client_info_t)iter->second;

    return VTSS_RC_OK;
}


mesa_rc vtss_appl_dhcp6_snooping_assigned_ip_itr(
    const vtss_appl_dhcp6_snooping_duid_t   *const prev_duid,
    vtss_appl_dhcp6_snooping_duid_t         *const next_duid,
    const vtss_appl_dhcp6_snooping_iaid_t   *const prev_iaid,
    vtss_appl_dhcp6_snooping_iaid_t         *const next_iaid)
{
    CRIT_ENTER_EXIT();

    if (next_duid == nullptr || next_iaid == nullptr) {
        return VTSS_RC_ERR_PARM;
    }

    vtss::Map<dhcp_duid_t, registered_clients_info_t>::iterator iter;
    auto duid = (prev_duid != nullptr) ? dhcp_duid_t(prev_duid) : dhcp_duid_t();
    auto is_first = true;
    auto got_next_duid = false;

    // iterate over table until next entry is found
    while (true) {
        if (is_first and prev_duid != nullptr) {
            // if first iteration and prev_duid is non-NULL we need to continue iterating its addresses
            is_first = false;
            iter = dhcp6_snooping::registered_clients_table.find(prev_duid);
            if (iter == dhcp6_snooping::registered_clients_table.end()) {
                // no such DUID - continue with normal iteration
                continue;
            }
        } else {
            // get next DUID in table
            iter = vtss::upper_bound(dhcp6_snooping::registered_clients_table.begin(),
                                     dhcp6_snooping::registered_clients_table.end(),
                                     duid, compare_registered_clients_info);

            if (iter == dhcp6_snooping::registered_clients_table.end()) {
                // no more entries
                return VTSS_RC_ERROR;
            }

            got_next_duid = true;
        }

        // Now we have a client DUID. Then we need to iterate addresses for that DUID
        duid = iter->first;
        auto curr_if_map = &iter->second.if_map;

        if (prev_iaid == nullptr || got_next_duid) {
            // prev_iaid is NULL or we skipped to the next DUID - select the first entry in the if_map (if any)
            if (curr_if_map->size() > 0) {
                auto address_entry = &curr_if_map->begin()->second;
                if (address_entry->address_state == DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED) {
                    // this is a valid assigned address entry
                    *next_iaid = curr_if_map->begin()->first;
                    break;
                }
            }
        }

        auto used_iaid = prev_iaid == nullptr ? 0 : *prev_iaid;
        bool found_iaid = false;
        while (true) {
            auto itera = vtss::upper_bound(curr_if_map->begin(), curr_if_map->end(),
                                           used_iaid, compare_assigned_ip);

            if (itera == curr_if_map->end()) {
                break;
            }
            auto address_entry = itera->second;
            if (address_entry.address_state == DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED) {
                *next_iaid = itera->first;
                found_iaid = true;
                break;
            }

            used_iaid = itera->first;
        }

        if (found_iaid) {
            break;
        }
    }

    *next_duid = duid;

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_dhcp6_snooping_assigned_ip_get(
    const vtss_appl_dhcp6_snooping_duid_t   &duid,
    const vtss_appl_dhcp6_snooping_iaid_t   iaid,
    vtss_appl_dhcp6_snooping_assigned_ip_t  *const address_info)
{
    CRIT_ENTER_EXIT();

    auto iter = dhcp6_snooping::registered_clients_table.find(duid);
    if (iter == dhcp6_snooping::registered_clients_table.end()) {
        return VTSS_RC_ERR_PARM;
    }

    auto itera = iter->second.if_map.find(iaid);
    if (itera == iter->second.if_map.end()) {
        return VTSS_RC_ERR_PARM;
    }

    *address_info = itera->second;
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_dhcp6_snooping_port_statistics_itr(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex)
{
    CRIT_ENTER_EXIT();
    return vtss_appl_iterator_ifindex_front_port(prev_ifindex, next_ifindex);
}

mesa_rc vtss_appl_dhcp6_snooping_port_statistics_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_dhcp6_snooping_port_statistics_t    *const port_statistics)
{
    CRIT_ENTER_EXIT();

    if (port_statistics == nullptr) {
        return VTSS_RC_ERR_PARM;
    }

    mesa_port_no_t port_no = dhcp6_snooping::get_port_from_ifindex(ifindex);
    if (port_no == MESA_PORT_NO_NONE || port_no >= dhcp6_snooping::port_stats_table.size()) {
        return VTSS_RC_ERR_PARM;
    }

    *port_statistics = dhcp6_snooping::port_stats_table[port_no];

    // This must always be false when read
    port_statistics->clear_stats = false;

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_dhcp6_snooping_port_statistics_set(
    vtss_ifindex_t  ifindex,
    const vtss_appl_dhcp6_snooping_port_statistics_t    *const port_statistics)
{
    CRIT_ENTER_EXIT();

    if (!port_statistics->clear_stats) {
        return VTSS_RC_ERR_PARM;
    }

    mesa_port_no_t port_no = dhcp6_snooping::get_port_from_ifindex(ifindex);
    if (port_no == MESA_PORT_NO_NONE || port_no >= dhcp6_snooping::port_stats_table.size()) {
        return VTSS_RC_ERR_PARM;
    }

    vtss_clear(dhcp6_snooping::port_stats_table[port_no]);

    return VTSS_RC_OK;
}


/* **************************************************************************
 * Module-public API functions (mainly used for debug)
 * **************************************************************************
 */

mesa_rc dhcp6_snooping_registered_clients_info_getnext(const dhcp_duid_t *const prev,
                                                       dhcp_duid_t       *const next,
                                                       registered_clients_info_t *info)
{
    CRIT_ENTER_EXIT();

    if (next == nullptr || info == nullptr) {
        return VTSS_RC_ERR_PARM;
    }

    auto duid = (prev != nullptr) ? *prev : dhcp_duid_t();
    auto iter = vtss::upper_bound(dhcp6_snooping::registered_clients_table.begin(),
                                  dhcp6_snooping::registered_clients_table.end(), duid,
                                  compare_registered_clients_info);
    if (iter == dhcp6_snooping::registered_clients_table.end()) {
        return VTSS_RC_ERROR;
    }

    *next = iter->first;
    *info = iter->second;

    return VTSS_RC_OK;
}

/* **************************************************************************
 * Initialization functions
 * **************************************************************************
 */

extern "C" int dhcp6_snooping_icli_cmd_register();

/* Initialize module */
mesa_rc dhcp6_snooping_init(vtss_init_data_t *data)
{
    mesa_rc     rc = VTSS_RC_OK;
    vtss_isid_t isid = data->isid;

    T_D("enter, cmd: %d, isid: %u, flags: 0x%x", data->cmd, data->isid, data->flags);
    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");

        critd_init(&dhcp6_snooping::crit, "dhcp6_snooping", VTSS_MODULE_ID_DHCP6_SNOOPING, CRITD_TYPE_MUTEX);
        DHCP6_SNOOPING_CRIT_ENTER();

        dhcp6_snooping::snooping_init();

#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = dhcp6_snooping_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling dhcp6_snooping_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        dhcp6_snooping::dhcp6_snooping_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        dhcp6_snooping::vtss_appl_dhcp6_snooping_json_init();
#endif
        dhcp6_snooping_icli_cmd_register();

        DHCP6_SNOOPING_CRIT_EXIT();
        break;
    case INIT_CMD_START:
        T_D("START");
        dhcp6_snooping::snooping_start();
        break;
    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
            dhcp6_snooping::restore_to_default();
        }
        break;
    case INIT_CMD_SUSPEND_RESUME:
        T_D("SUSPEND_RESUME");
        break;
    default:
        break;
    }

    T_D("exit");

    return rc;
}
