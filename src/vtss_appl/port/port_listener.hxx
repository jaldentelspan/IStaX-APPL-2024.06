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

#ifndef __PORT_LISTENER_HXX__
#define __PORT_LISTENER_HXX__

#include <vtss/basics/vector.hxx>
#include "port_api.h"

// Invocations from port_instance.cxx and port.cxx
// Only the two first are invoked without the port module's lock taken.
// The last twoe must release the port module's lock before invoking callbacks.
void port_listener_change_notify(             mesa_port_no_t port_no, const vtss_appl_port_status_t &status);
void port_listener_initial_change_notify(     mesa_port_no_t port_no, const vtss_appl_port_status_t &status);
void port_listener_shutdown_notify(           mesa_port_no_t);
void port_listener_conf_change_notify(        mesa_port_no_t, const vtss_appl_port_conf_t &conf);

// The remainder is for debug purposes.

// In case it is used for other purposes, it is needed it to reimplement it
// because it's not thread safe and when a new listener is added the iterators
// are invalid.

// Internal state of a port change registrant
typedef struct port_listener_change_registrant {
    port_listener_change_registrant(vtss_module_id_t id, port_change_callback_t func)
        : callback {func}, module_id {id}, max_ms {}, port_done {} {}

    port_change_callback_t callback;   /* User callback function */
    vtss_module_id_t       module_id;  /* Module ID */
    uint64_t               max_ms;     /* Maximum number of ms */
    mesa_port_list_t       port_done;  /* Port mask for initial event done */
} port_listener_change_registrant_t;

// Internal state of a port shutdown registrant
typedef struct port_listener_shutdown_registrant {
    port_listener_shutdown_registrant(vtss_module_id_t id, port_shutdown_callback_t func)
        : callback {func}, module_id {id}, max_ms {} {}

    port_shutdown_callback_t callback;  /* User callback function */
    vtss_module_id_t         module_id; /* Module ID */
    uint64_t                 max_ms;    /* Maximum number of ms */
} port_listener_shutdown_registrant_t;

// Internal state of a port configuration change registrant
typedef struct port_listener_conf_change_registrant {
    port_listener_conf_change_registrant(vtss_module_id_t id, port_conf_change_callback_t func)
        : callback {func}, module_id {id}, max_ms {} {}

    port_conf_change_callback_t callback;  /* User callback function */
    vtss_module_id_t            module_id; /* Module ID */
    uint64_t                    max_ms;    /* Maximum number of ms */
} port_listener_conf_change_registrant_t;

using change_itr              = vtss::Vector<port_listener_change_registrant_t>::const_iterator;
using shutdown_itr            = vtss::Vector<port_listener_shutdown_registrant_t>::const_iterator;
using conf_change_itr         = vtss::Vector<port_listener_conf_change_registrant_t>::const_iterator;
using change_range_itr_t      = vtss::Pair<change_itr, change_itr>;
using shutdown_range_itr_t    = vtss::Pair<shutdown_itr, shutdown_itr>;
using conf_change_range_itr_t = vtss::Pair<conf_change_itr, conf_change_itr>;
change_range_itr_t      port_listener_change_registrants_get(void);
shutdown_range_itr_t    port_listener_shutdown_registrants_get(void);
conf_change_range_itr_t port_listener_conf_change_registrants_get(void);
void                    port_listener_clear_max_ms(void);

#endif
