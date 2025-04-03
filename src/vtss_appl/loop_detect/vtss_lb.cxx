/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/****************************************************************************
The vtss_lb module (lb =  loopback) is used for detecting loopback between
port 1 and port 2 at the primary switch.
This is used for doing a "restore to defaults" for managed switches, and doing
a software upgrade for unmanaged switches.

The loopback must be applied doing switch boot, because the loop detection will
only be done within the first 60 seconds. After that the thread is terminated.

It is possible for other modules to "subscribe" for a callback function when
a loopback is detected.

The callback function is registered with "vtss_lb_callback_register".

The ossp packet used for detecting loopback is described in AS0036.
*****************************************************************************/

/*lint -esym(459,restore_to_def_flag) */ // We don't have to protect this variable. It can only be changed from 0 to 1.

#include "main.h"
#include "conf_api.h"
#include "control_api.h"
#include "critd_api.h"
#include "msg_api.h"
#include "vtss_lb.h"
#include "port_api.h"
#include "misc_api.h"
#ifdef VTSS_SW_OPTION_PACKET
#include "packet_api.h"
#endif
#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

/****************************************************************************/
/*  TRACE system                                                            */
/****************************************************************************/
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "loopback", "loopback"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define LOOPBACK_CRIT_ENTER()         critd_enter(        &mem_loopback_conf.crit, __FILE__, __LINE__)
#define LOOPBACK_CRIT_EXIT()          critd_exit(         &mem_loopback_conf.crit, __FILE__, __LINE__)
#define LOOPBACK_CRIT_ASSERT_LOCKED() critd_assert_locked(&mem_loopback_conf.crit, __FILE__, __LINE__)

/****************************************************************************/
/*  Global variables for vtss_lb */
/****************************************************************************/

static vtss_lb_t          mem_loopback_conf; // used for critical regions
static vtss_lb_callback_list_t callback_list[2];  // Array containing the callback functions
static void *filter_id = NULL;               // Filter id for subscribing ossp packet.
static BOOL active;
static BOOL restore_to_def_flag = 0;

// Definition for multicast protocol frame
const u8  vtss_slow_protocol_addr[6]    = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x02};
const u8  vtss_slow_protocol_type[2]    = {0x88, 0x09};
const int vtss_slow_protocol_type_int   = 0x8809;
const u8  vtss_oui[3]                   = {0x00, 0x01, 0xC1 } ;


// Define which two ports that is used for loopback detecion
const mesa_port_no_t loopback_port_a = VTSS_PORT_NO_START + 1; // Front port 2
const mesa_port_no_t loopback_port_b = VTSS_PORT_NO_START;     // Front port 1

/****************************************************************************/
/*  Ossp packet  - See AS0036                                              */
/****************************************************************************/

// Callback function registered in the packet module ( for ossp packets ).
static BOOL vtss_lb_rx_pkt(void  *contxt,
                           const uchar *const frm_p,
                           const mesa_packet_rx_info_t *const rx_info)

{
    T_D("Got ossp packet from packet module");
    int callback_list_index;
    bool ossp_pkt_recv = true;

    /* Check for correct subtype field */
    if (frm_p[14] != 0x0A) {
        ossp_pkt_recv = false;
        T_D("Subtype did not match - Subtype = %d", frm_p[14]);
    };

    /* Check OUI */
    if (memcmp(&frm_p[15], &vtss_oui[0], sizeof(vtss_oui)) !=  0) {
        ossp_pkt_recv = false;
        T_D("OUI did not match, %x %x %x", frm_p[15], frm_p[16], frm_p[17]);
    }


    // if the ossp packet is received then execute all callback functions that have registered on this VTSS type.
    if (ossp_pkt_recv) {
        LOOPBACK_CRIT_ENTER();

        T_D("Ossp packet verified - Checking for VTSS type");
        for (callback_list_index = 0; callback_list_index < (int) ARRSZ(callback_list); callback_list_index++) {
            // Check for VTSS type
            if (callback_list[callback_list_index].active) {
                if (frm_p[19] == callback_list[callback_list_index].vtss_type) {
                    T_N("VTSS type verified- Type = %u -%d", callback_list[callback_list_index].vtss_type, frm_p[19]);
                    if (callback_list[callback_list_index].callback == NULL) {
                        T_E("Callback for callback_list_index=%d is NULL", callback_list_index);
                    } else {
                        callback_list[callback_list_index].callback();
                    }
                } else {
                    T_N("VTSS type did not match - Type = %u - %d", callback_list[callback_list_index].vtss_type, frm_p[19]);
                }
            }
        }

        LOOPBACK_CRIT_EXIT();
        return 1; // Don't allow other subscribers to receive the packet
    } else {
        T_D("Wrong ossp packet");
        return 0; // Allow other subscribers to receive the packet
    }

} // vtss_lb_rx_pkt

// Function for transmitting a "OSSP packet"
void vtss_lb_tx_ossp_pck(int vtss_type)
{
    unsigned char *pkt_buf; /* Packet buffer for the ossp packets */
    static ushort pkt_len = 60; // Ossp packet must be 60 bytes, giving 64 byte frame when the packet module applies FCS.
    packet_tx_props_t tx_props;

    // Get MAC afdress for this switch
    uchar mac_addr[6];
    (void) conf_mgmt_mac_addr_get(mac_addr, (uint) 0);

    T_D("Transmitting ossp packet");
    if  ( (pkt_buf = packet_tx_alloc(pkt_len) ) ) {

        /* See packet definition in AS0036 */

        /* Set the destination MAC Address to slow protocol multicast address */
        memcpy(&pkt_buf[0], vtss_slow_protocol_addr, sizeof(vtss_slow_protocol_addr));

        /* Set the source MAC Address */
        memcpy(&pkt_buf[6], mac_addr, sizeof(mac_addr));

        /* Set type length field */
        memcpy(&pkt_buf[12], vtss_slow_protocol_type, sizeof(vtss_slow_protocol_type));

        /* Set subtype field */
        pkt_buf[14] = 0x0A;

        /* Set OUI */
        memcpy(&pkt_buf[15], &vtss_oui[0], 3);


        /*Set VTSS type */
        char vtss_type_c[2];
        vtss_type_c[1] = vtss_type & 0xFF;
        vtss_type_c[0] = vtss_type >> 8 & 0xFF;

        memcpy(&pkt_buf[18], &vtss_type_c[0], sizeof(vtss_type_c));

        packet_tx_props_init(&tx_props);
        tx_props.packet_info.modid     = VTSS_MODULE_ID_VTSS_LB;
        tx_props.packet_info.frm       = pkt_buf;
        tx_props.packet_info.len       = pkt_len;
        tx_props.tx_info.dst_port_mask = VTSS_BIT64(loopback_port_a);
        T_D("Ossp packet transmitted");
        (void)packet_tx(&tx_props);
    }
}

/* Return whether port is (actively) being monitored by this module */
BOOL vtss_lb_port(mesa_port_no_t port_no)
{
    return
        active &&
        ((port_no == loopback_port_a) || (port_no == loopback_port_b));
}

// Function for doing a subscribing of the "ossp packet" from the packet module.
static void vtss_lb_subscribe_for_packets(void)
{
    packet_rx_filter_t rx_filter;

    LOOPBACK_CRIT_ASSERT_LOCKED();

    if (filter_id == NULL) {
        T_N("Subscribing for ossp packets");
        // Get switchs MAC address
        u8 mac_addr[6];
        (void) conf_mgmt_mac_addr_get(mac_addr, 0);

        /* frames registration, ossp frame is defined in AS036.
           Loopback must be done between port 1 and 2, and the ossp frame is transmitted from port 1 to port 2.*/
        packet_rx_filter_init(&rx_filter);

        rx_filter.etype = vtss_slow_protocol_type_int;
        rx_filter.modid = VTSS_MODULE_ID_VTSS_LB;
        rx_filter.match = PACKET_RX_FILTER_MATCH_DMAC | PACKET_RX_FILTER_MATCH_SMAC | PACKET_RX_FILTER_MATCH_ETYPE | PACKET_RX_FILTER_MATCH_SRC_PORT ;
        VTSS_PORT_BF_SET(rx_filter.src_port_mask, loopback_port_b, 1); // Ossp packet must be sent from port 1 to port 2.

        memcpy(rx_filter.dmac, vtss_slow_protocol_addr, sizeof(vtss_slow_protocol_addr));
        memcpy(rx_filter.smac, mac_addr, sizeof(mac_addr));
        rx_filter.prio   = PACKET_RX_FILTER_PRIO_NORMAL;

        rx_filter.cb   = vtss_lb_rx_pkt;

        if (packet_rx_filter_register(&rx_filter, &filter_id) != VTSS_RC_OK) {
            T_E("Not possible to register for ossp packets. VTSS loopback will not work !");
        }
    }
    T_N("vtss_lb_subscribe_for_packets done");
}

BOOL vtss_lb_any_active_callbacks(void)
{
    int callback_list_index;

    LOOPBACK_CRIT_ASSERT_LOCKED();

    for (callback_list_index = 0; callback_list_index < (int) ARRSZ(callback_list); callback_list_index++) {
        // If at least one callback is still active then skip the unregistering
        if (callback_list[callback_list_index].active == 1) {
            T_N("There are still active callbacks");
            return 1;
        }
    }
    return 0; // No active callbacks
}

// Function for unregistering callbacks
void vtss_lb_callback_unregister(int vtss_type)
{

    int callback_list_index;

    LOOPBACK_CRIT_ENTER();
    for (callback_list_index = 0; callback_list_index < (int) ARRSZ(callback_list); callback_list_index++) {
        // "unregister" this callback
        if (callback_list[callback_list_index].active && callback_list[callback_list_index].vtss_type == vtss_type) {
            callback_list[callback_list_index].active = 0;
            T_D("Setting active to 0 for callback_list_index = %d", callback_list_index);
            break;
        }
    }

    if (!vtss_lb_any_active_callbacks() && filter_id != NULL) {
        T_D("Unsubscribing ossp packets");
        if (packet_rx_filter_unregister(filter_id) != VTSS_RC_OK) {
            T_W("Did not unregister");
        }
        filter_id = NULL;
        T_N("Unsubscribing ossp packet done");
    }

    LOOPBACK_CRIT_EXIT();
}
/****************************************************************************/
/*  Callback                                                                */
/****************************************************************************/

// Callback register function
void vtss_lb_callback_register(vtss_lb_callback_t cb, int vtss_type)
{
    int free_idx = 0, idx;
    BOOL free_idx_found = FALSE;

    LOOPBACK_CRIT_ENTER();

    for (idx = 0; idx < (int) ARRSZ(callback_list); idx++) {
        // Search for a free index.
        if (!free_idx_found && callback_list[idx].active == 0) {
            free_idx = idx;
            free_idx_found = TRUE;
            // Don't break here.
        }

        // Check for two registrants of the same callback
        if (callback_list[idx].active && callback_list[idx].vtss_type) {
            T_E("Already another registrant of the same type (%d)", vtss_type);
            free_idx_found = FALSE; // Way to break out without re-registering (even if another error message comes)
            break;
        }
    }

    if (!free_idx_found) {
        T_E("Trying to register too many vtss_lb callbacks, please increase the callback array");
    } else {
        callback_list[free_idx].callback = cb; // Register the callback function
        callback_list[free_idx].vtss_type = vtss_type; // Register the VTSS frame type that the frame must match for doing the callback.
        callback_list[free_idx].active = 1;
        vtss_lb_subscribe_for_packets();
    }

    LOOPBACK_CRIT_EXIT();
}

/**********************************************************************
 ** Restore to default when a ossp packet is received.
 **********************************************************************/
static void vtss_lb_restore_default (void)
{
    T_D("Setting Restoring to default flag");
    restore_to_def_flag = 1;
}

/**********************************************************************
 ** Thread
 **********************************************************************/
static vtss_handle_t vtss_lb_thread_handle;
static vtss_thread_t vtss_lb_thread_block;

static void vtss_lb_thread(vtss_addrword_t data)
{
    vtss_appl_port_status_t port_1_status, port_2_status;
    vtss_ifindex_t          port_1_ifindex, port_2_ifindex;
    int                     time_tick_counter;

    // Wait until we're the primary switch
    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_POST, VTSS_MODULE_ID_VTSS_LB);

    (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, loopback_port_a, &port_1_ifindex);
    (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, loopback_port_b, &port_2_ifindex);

    vtss_lb_callback_register(vtss_lb_restore_default, 0x0001); // Register the restore to default for callback

    T_N("Entering vtss_lb_thread");

    // Try to transmit ossp packet for maximum 60 sec.
    for (time_tick_counter = 0; time_tick_counter < 60; time_tick_counter++ ) {
        VTSS_OS_MSLEEP(1000); // Wait one sec.

        T_N("Checking if ports are up");
        if (vtss_appl_port_status_get(port_1_ifindex, &port_1_status) != VTSS_RC_OK ||
            vtss_appl_port_status_get(port_2_ifindex, &port_2_status) != VTSS_RC_OK ) {
            T_W("Did not get port status");
            continue;
        }

        if (!port_1_status.link || !port_2_status.link) {
            continue;
        }

        T_N("Ports is up");
        vtss_lb_tx_ossp_pck(0x0001); // Send the OSSP packet.
        VTSS_OS_MSLEEP(2000); // Wait 2 seconds to make sure that the ossp packet is received.
        LOOPBACK_CRIT_ENTER();
        // Restore to default. The restore to default flag will be set if the OSSP frame was received.
        if (restore_to_def_flag) {
            // Make sure Uports print pretty (lowest first) in warning
            mesa_port_no_t a = iport2uport(loopback_port_a);
            mesa_port_no_t b = iport2uport(loopback_port_b);
            if (a > b) {
                mesa_port_no_t b2 = b;
                b = a;
                a = b2;
            }

#if defined(VTSS_SW_OPTION_SYSLOG)
            char buf[256], *p = &buf[0];
            p += sprintf(p, "LOOP_DETECT-LOOP_BACK_DETECTED: Loop-back detected on ");
            p += sprintf(p, "Interface %s%2d/%2d%s",
                         SYSLOG_PORT_INFO_REPLACE_TAG_START, VTSS_ISID_LOCAL, a, SYSLOG_PORT_INFO_REPLACE_TAG_END);
            p += sprintf(p, "Interface %s%2d/%2d%s",
                         SYSLOG_PORT_INFO_REPLACE_TAG_START, VTSS_ISID_LOCAL, b, SYSLOG_PORT_INFO_REPLACE_TAG_END);
            p += sprintf(p, ", loading default configuration.");
            S_N("%s", buf);
#endif /* VTSS_SW_OPTION_SYSLOG */
            T_D("Loopback detected on ports %u and %u -- loading default configuration.", a, b);
            printf("Loopback detected on ports %u and %u -- loading default configuration.\n", a, b);
            control_config_reset(VTSS_USID_ALL, 0);
            T_D("Restoring to default done");
        }

        LOOPBACK_CRIT_EXIT();
        break; // Exit thread
    }

    vtss_lb_callback_unregister(0x0001); // Stop OSSP packet detection. Loopback detection shall only be done upon boot.
    active = FALSE;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

/* Initialize module */
mesa_rc vtss_lb_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        memset(callback_list, 0, sizeof(callback_list));
        critd_init(&mem_loopback_conf.crit, "mem_loopback_conf", VTSS_MODULE_ID_VTSS_LB, CRITD_TYPE_MUTEX);

        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           vtss_lb_thread,
                           0,
                           "VTSS Loopback",
                           nullptr,
                           0,
                           &vtss_lb_thread_handle,
                           &vtss_lb_thread_block);
        active = TRUE;
    }

    return VTSS_RC_OK;
}

