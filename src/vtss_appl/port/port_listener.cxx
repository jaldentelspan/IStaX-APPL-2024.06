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

#include "port_listener.hxx"
#include "port_lock.hxx"
#include <vtss/basics/utility.hxx>
#include "port_trace.h"

static vtss::Vector<port_listener_change_registrant_t>      _change_listeners;
static vtss::Vector<port_listener_shutdown_registrant_t>    _shutdown_listeners;
static vtss::Vector<port_listener_conf_change_registrant_t> _conf_change_listeners;

static bool PORT_LISTENER_new_listeners;

/******************************************************************************/
// PORT_LISTENER_module_id_valid()
/******************************************************************************/
static mesa_rc PORT_LISTENER_module_id_valid(vtss_module_id_t module_id)
{
    if (module_id < VTSS_MODULE_ID_NONE) {
        return VTSS_RC_OK;
    }

    T_EG(PORT_TRACE_GRP_CALLBACK, "Invalid module_id: %d", module_id);
    return VTSS_APPL_PORT_RC_PARM;
}

/******************************************************************************/
// PORT_LISTENER_change_callback()
/******************************************************************************/
static void PORT_LISTENER_change_callback(port_listener_change_registrant_t &e, mesa_port_no_t port_no, const vtss_appl_port_status_t &status)
{
    uint64_t ms = vtss::uptime_milliseconds();

    e.callback(port_no, &status);
    e.port_done[port_no] = 1; // (Initial) callback done
    ms = vtss::uptime_milliseconds() - ms;

    if (ms > e.max_ms) {
        e.max_ms = ms;
    }
}

/******************************************************************************/
// port_change_register()
/******************************************************************************/
mesa_rc port_change_register(vtss_module_id_t module_id, port_change_callback_t callback)
{
    VTSS_RC(PORT_LISTENER_module_id_valid(module_id));

    PORT_CALLBACK_LOCK_SCOPE();
    PORT_LISTENER_new_listeners = true;
    return _change_listeners.emplace_back(module_id, callback) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

/******************************************************************************/
// port_shutdown_register()
/******************************************************************************/
mesa_rc port_shutdown_register(vtss_module_id_t module_id, port_shutdown_callback_t callback)
{
    VTSS_RC(PORT_LISTENER_module_id_valid(module_id));

    PORT_CALLBACK_LOCK_SCOPE();
    return _shutdown_listeners.emplace_back(module_id, callback) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

/******************************************************************************/
// port_conf_change_register()
/******************************************************************************/
mesa_rc port_conf_change_register(vtss_module_id_t module_id, port_conf_change_callback_t callback)
{
    VTSS_RC(PORT_LISTENER_module_id_valid(module_id));

    PORT_CALLBACK_LOCK_SCOPE();
    return _conf_change_listeners.emplace_back(module_id, callback) ? VTSS_RC_OK : VTSS_RC_ERROR;
}

/******************************************************************************/
// port_listener_initial_change_notify()
// This is called over and over again to notify new registrants of the current
// link status, so that they don't have to call vtss_appl_port_status_get() to
// get the initial link status.
// Registrants will only be called once.
/******************************************************************************/
void port_listener_initial_change_notify(mesa_port_no_t port_no, const vtss_appl_port_status_t &status)
{
    PORT_CALLBACK_LOCK_SCOPE();

    if (!PORT_LISTENER_new_listeners) {
        return;
    }

    for (port_listener_change_registrant_t &e : _change_listeners) {
        if (e.port_done[port_no] == 0) {
            // This module has never been notified. Do it.
            T_DG_PORT(PORT_TRACE_GRP_CALLBACK, port_no, "%s: link = %d", vtss_module_names[e.module_id], status.link);
            PORT_LISTENER_change_callback(e, port_no, status);
        }
    }

    PORT_LISTENER_new_listeners = false;
}

/******************************************************************************/
// port_listener_change_notify()
/******************************************************************************/
void port_listener_change_notify(mesa_port_no_t port_no, const vtss_appl_port_status_t &status)
{
    T_IG_PORT(PORT_TRACE_GRP_CALLBACK, port_no, "link = %d", status.link);

    PORT_CALLBACK_LOCK_SCOPE();
    for (port_listener_change_registrant_t &e : _change_listeners) {
        PORT_LISTENER_change_callback(e, port_no, status);
    }
}

/******************************************************************************/
// port_listener_shutdown_notify()
/******************************************************************************/
void port_listener_shutdown_notify(mesa_port_no_t port_no)
{
    T_IG_PORT(PORT_TRACE_GRP_CALLBACK, port_no, "shutdown", port_no);

    // This function is called with the Port's lock taken. Gotta release it
    // while we call back.
    PORT_UNLOCK_SCOPE();
    PORT_CALLBACK_LOCK_SCOPE();
    for (auto &e : _shutdown_listeners) {
        uint64_t ms = vtss::uptime_milliseconds();
        e.callback(port_no);
        ms = vtss::uptime_milliseconds() - ms;
        if (ms > e.max_ms) {
            e.max_ms = ms;
        }
    }
}

/******************************************************************************/
// port_listener_conf_change_notify()
/******************************************************************************/
void port_listener_conf_change_notify(mesa_port_no_t port_no, const vtss_appl_port_conf_t &conf)
{
    T_IG_PORT(PORT_TRACE_GRP_CALLBACK, port_no, "conf_change", port_no);

    // This function is called with the Port's lock taken. Gotta release it
    // while we call back.
    PORT_UNLOCK_SCOPE();
    PORT_CALLBACK_LOCK_SCOPE();
    for (auto &e : _conf_change_listeners) {
        uint64_t ms = vtss::uptime_milliseconds();
        e.callback(port_no, &conf);
        ms = vtss::uptime_milliseconds() - ms;
        if (ms > e.max_ms) {
            e.max_ms = ms;
        }
    }
}

/******************************************************************************/
// CLEAR_MAX_MS()
// For debug purposes
/******************************************************************************/
#define CLEAR_MAX_MS(V)                 \
    do {                                \
        for (auto &e : V) e.max_ms = 0; \
    } while (0)

/******************************************************************************/
// port_listener_clear_max_ms()
// For debug purposes
/******************************************************************************/
void port_listener_clear_max_ms(void)
{
    CLEAR_MAX_MS(_change_listeners);
    CLEAR_MAX_MS(_shutdown_listeners);
    CLEAR_MAX_MS(_conf_change_listeners);
}

/******************************************************************************/
// port_listener_change_registrants_get()
// For debug purposes
/******************************************************************************/
change_range_itr_t port_listener_change_registrants_get(void)
{
    return vtss::make_pair(_change_listeners.cbegin(), _change_listeners.cend());
}

/******************************************************************************/
// port_listener_shutdown_registrants_get()
// For debug purposes
/******************************************************************************/
shutdown_range_itr_t port_listener_shutdown_registrants_get(void)
{
    return vtss::make_pair(_shutdown_listeners.cbegin(), _shutdown_listeners.cend());
}

/******************************************************************************/
// port_listener_conf_change_registrants_get()
// For debug purposes
/******************************************************************************/
conf_change_range_itr_t port_listener_conf_change_registrants_get(void)
{
    return vtss::make_pair(_conf_change_listeners.cbegin(), _conf_change_listeners.cend());
}

