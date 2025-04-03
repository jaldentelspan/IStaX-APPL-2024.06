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

#include <vtss/appl/errdisable.h>
#include "errdisable_api.h"
#include "port_trace.h"
#include "critd_api.h"
#include <vtss/basics/list.hxx>
#include "link_flap_detect_api.h"

static critd_t LFD_critd;  // Critial region for the shared working data

struct LFD_lock {
    LFD_lock(int line)
    {
        critd_enter(&LFD_critd, __FUNCTION__, line);
    }

    ~LFD_lock(void)
    {
        critd_exit( &LFD_critd, __FUNCTION__, 0);
    }
};

#define LFD_LOCK_SCOPE()  LFD_lock __lock_guard__(__LINE__)

/******************************************************************************/
// LFD_state
/******************************************************************************/
static struct {
    vtss_appl_errdisable_client_conf_t conf;

    /**
     * This array of vectors holds the times for link changes
     * on the ports.
     * One instance is considered a circular buffer, where the
     * oldest timestamp marks the beginning of the buffer and
     * the newest marks the end. The size of the list corresponds
     * to the number of flaps detected.
     */
    CapArray<vtss::List<vtss_tick_count_t>, MEBA_CAP_BOARD_PORT_MAP_COUNT> flap_lists;

    CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT> shut_down;
} LFD_state;

/******************************************************************************/
// LFD_state_clear()
/******************************************************************************/
static void LFD_state_clear(mesa_port_no_t port_no)
{
    mesa_port_no_t port_no_iter_min, port_no_iter_max, port_no_iter;

    if (port_no == VTSS_PORT_NO_NONE) {
        port_no_iter_min = 0;
        port_no_iter_max = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) - 1;
    } else {
        port_no_iter_min = port_no;
        port_no_iter_max = port_no;
    }

    for (port_no_iter = port_no_iter_min; port_no_iter <= port_no_iter_max; port_no_iter++) {
        LFD_state.flap_lists[port_no_iter].clear();
        T_DG(PORT_TRACE_GRP_LINK_FLAP_DETECT, "Changing max-size from %zu to %u", LFD_state.flap_lists[port_no_iter].max_size(), LFD_state.conf.flap_cnt);
        // Multiply by 2, since internally we count single up- or down- events, but Cisco's user-space counts up-and-down cycles.
        LFD_state.flap_lists[port_no_iter].max_size(2 * LFD_state.conf.flap_cnt);
        LFD_state.shut_down[port_no_iter] = FALSE;
    }
}

/******************************************************************************/
// LFD_errdisable_conf_change()
/******************************************************************************/
static mesa_rc LFD_errdisable_conf_change(const vtss_appl_errdisable_client_conf_t *client_conf)
{
    BOOL start_over;

    LFD_LOCK_SCOPE();

    start_over = client_conf->detect != LFD_state.conf.detect || client_conf->flap_cnt != LFD_state.conf.flap_cnt || client_conf->time_secs != LFD_state.conf.time_secs;
    LFD_state.conf = *client_conf;

    if (start_over) {
        LFD_state_clear(VTSS_PORT_NO_NONE);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// LFD_errdisable_on_shut_down_clear()
/******************************************************************************/
static void LFD_errdisable_on_shut_down_clear(mesa_port_no_t port_no, const errdisable_interface_client_status_t *new_client_status)
{
    LFD_LOCK_SCOPE();
    LFD_state_clear(port_no); // Start over
}

/******************************************************************************/
// LFD_errdisable_on_capabilities_get()
/******************************************************************************/
static void LFD_errdisable_on_capabilities_get(vtss_appl_errdisable_client_capabilities_t *cap)
{
    // We can assume that #cap is non-NULL, and that #cap is memset() to
    // zeros on entry, so that we only have to set those that we actually support.
    // This module supports changes in detect and flap-values.
    cap->detect            = TRUE;
    cap->flap_values       = TRUE;
    cap->flap_cnt_min      =    2;
    cap->flap_cnt_max      = 1000;
    cap->flap_cnt_default  =    5;
    cap->time_secs_min     =    1;
    cap->time_secs_max     = 1000;
    cap->time_secs_default =   10;
    strncpy(cap->name, "link-flap", sizeof(cap->name) - 1);
}

/******************************************************************************/
// LFD_errdisable_set()
/******************************************************************************/
static void LFD_errdisable_set(mesa_port_no_t port_no, BOOL shut_down)
{
    errdisable_interface_client_status_t errdisable_client_status;
    vtss_ifindex_t                       ifindex;

    if (vtss_ifindex_from_port(VTSS_ISID_START, port_no, &ifindex) != VTSS_RC_OK) {
        T_E("Unable to convert <isid, port> = <%u, %u> to ifindex", VTSS_ISID_START, port_no);
        return;
    }

    if (errdisable_interface_client_status_get(ifindex, VTSS_APPL_ERRDISABLE_CLIENT_LINK_FLAP, &errdisable_client_status) != VTSS_RC_OK) {
        T_EG(PORT_TRACE_GRP_LINK_FLAP_DETECT, "Unable to obtain errdisable status for %u", port_no);
        // Go on anyway. What else to do?
        memset(&errdisable_client_status, 0, sizeof(errdisable_client_status));
    }

    T_IG(PORT_TRACE_GRP_LINK_FLAP_DETECT, "%u port status before = %d, after = %d", port_no, errdisable_client_status.shut_down, shut_down);

    errdisable_client_status.shut_down = shut_down;
    LFD_state.shut_down[port_no] = shut_down;

    if (errdisable_interface_client_status_set(ifindex, VTSS_APPL_ERRDISABLE_CLIENT_LINK_FLAP, &errdisable_client_status) != VTSS_RC_OK) {
        T_EG(PORT_TRACE_GRP_LINK_FLAP_DETECT, "Unable to set errdisable conf for %u", port_no);
        // Go on anyway. What else to do?
    }
}

/******************************************************************************/
// LFD_link_state_change_callback()
/******************************************************************************/
static void LFD_link_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    vtss_tick_count_t             now = vtss_current_time(), window_size_ticks;
    vtss::List<vtss_tick_count_t> &flap_list = LFD_state.flap_lists[port_no];

    // Shall be called at link state event in the port module. Used for detecting link flaps for a specific port.
    LFD_LOCK_SCOPE();

    if (!LFD_state.conf.detect) {
        // We're not enabled. Nothing to do.
        return;
    }

    if (LFD_state.shut_down[port_no]) {
        T_IG(PORT_TRACE_GRP_LINK_FLAP_DETECT, "Port %u: Already shut down by us", port_no);
    }

    window_size_ticks = VTSS_OS_MSEC2TICK(LFD_state.conf.time_secs * 1000);

    // First remove entries that don't count anymore from the list.
    // An entry doesn't count anymore, when it's too old compared
    // to our window size.
    for (auto itr = flap_list.begin(); itr != flap_list.end();) {
        if (now - *itr > window_size_ticks) {
            // Entry is too old to count
            T_IG(PORT_TRACE_GRP_LINK_FLAP_DETECT, "Aging out an entry on port %u", port_no);
            flap_list.erase(itr++);
        } else {
            ++itr;
        }
    }

    if (flap_list.size() == flap_list.max_size()) {
        // The list is full, and we got a new link state change.
        // Time to shut down the port.
        T_IG(PORT_TRACE_GRP_LINK_FLAP_DETECT, "Shutting down port %u", port_no);
        LFD_errdisable_set(port_no, TRUE);
    } else {
        // Just add it to the end.
        flap_list.emplace_back(now);
    }

    T_IG(PORT_TRACE_GRP_LINK_FLAP_DETECT, "Port %u. Flaps the last %u seconds: %zu out of %zu", port_no, LFD_state.conf.time_secs, flap_list.size(), flap_list.max_size());
}

/******************************************************************************/
// link_flap_detect_init()
/******************************************************************************/
void link_flap_detect_init(vtss_init_data_t *data)
{
    errdisable_callbacks_t errdisable_callbacks;

    switch (data->cmd) {
    case INIT_CMD_EARLY_INIT:
        critd_init(&LFD_critd, "Link flap detect", VTSS_MODULE_ID_PORT, CRITD_TYPE_MUTEX);
        break;

    case INIT_CMD_START:
        // Register for changes in configuration of Link Flap detection and get
        // called back when our shut-down status gets cleared.
        memset(&errdisable_callbacks, 0, sizeof(errdisable_callbacks));
        errdisable_callbacks.on_conf_change      = LFD_errdisable_conf_change;
        errdisable_callbacks.on_shut_down_clear  = LFD_errdisable_on_shut_down_clear;
        errdisable_callbacks.on_capabilities_get = LFD_errdisable_on_capabilities_get;
        if (errdisable_callbacks_set(VTSS_APPL_ERRDISABLE_CLIENT_LINK_FLAP, &errdisable_callbacks) != VTSS_RC_OK) {
            T_E("Bummer");
        }

        // Register for port link-state change events
        (void)port_change_register(VTSS_MODULE_ID_PORT, LFD_link_state_change_callback);
        break;

    default:
        break;
    }
}

