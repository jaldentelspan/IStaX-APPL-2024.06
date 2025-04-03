/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss_xxrp_mad.h"
#include "vtss_xxrp_madtt.h"
#include "vtss_xxrp_callout.h"
#include "vtss_xxrp_timers.h"
#include "vtss_xxrp_os.h"
#include "vtss_xxrp.h"
#include "../platform/xxrp_trace.h"

/* TODO: connect_port also needs to be implemented */
/* This function is called with lock */
mesa_rc vtss_mrp_mad_create_port(vtss_mrp_t *appl, vtss_mrp_mad_t **mad_ports, u32 l2port, vtss::VlanList &vls)
{
    vtss_mrp_mad_t *mad;
    u32            machine_index;
    BOOL           is_attr_registered;
    static vlan_registration_type_t reg_admin_status[VTSS_APPL_VLAN_ID_MAX + 1];

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Creating MAD on port " << l2port2str(l2port);
    mad = (vtss_mrp_mad_t *)XXRP_SYS_MALLOC(sizeof(vtss_mrp_mad_t));
    memset(mad, 0, sizeof(vtss_mrp_mad_t));
    mad->machines = (vtss_mrp_mad_machine_t *)XXRP_SYS_MALLOC((appl->max_mad_index + 1) * sizeof(vtss_mrp_mad_machine_t));

    /* Initialize the applicant to Very Anxious Observer (VO) and the registrar to IN or MT */
#ifdef VTSS_SW_OPTION_MVRP
    (void)xxrp_mgmt_vlan_state(l2port, reg_admin_status);
#endif
    for (machine_index = 1; machine_index < (appl->max_mad_index + 1); machine_index++) {
        is_attr_registered = FALSE;
#ifdef VTSS_SW_OPTION_MVRP
        if (reg_admin_status[machine_index] == VLAN_REGISTRATION_TYPE_FIXED) {
            is_attr_registered = TRUE;
        }
#endif
        vtss_mrp_madtt_init_state_machine(&(mad->machines[machine_index]), is_attr_registered);
    }
    /* Initialize the leave all and periodic SMs to the passive state */
    vtss_mrp_madtt_init_la_and_periodic_state_machines(mad);

    mad->join_timeout = VTSS_MRP_JOIN_TIMER_DEF;
    mad->leave_timeout = VTSS_MRP_LEAVE_TIMER_DEF;
    mad->leaveall_timeout = VTSS_MRP_LEAVE_ALL_TIMER_DEF;
    mad->periodic_tx_timeout = VTSS_MRP_PERIODIC_TIMER_DEF;
    mad->port_no = l2port;
    mad_ports[l2port] = mad;

    /* In case of MVRP, we also initialize the Registrar administrative status */
    /* NOTE: This has to be done after the MAD is created for the given port */
#ifdef VTSS_SW_OPTION_MVRP
    for (machine_index = 1; machine_index < (appl->max_mad_index + 1); machine_index++) {
        (void)vtss_mrp_reg_admin_status_set(VTSS_MRP_APPL_MVRP, l2port, machine_index, reg_admin_status[machine_index]);
    }
#endif

    /* Start the LA timer */
    vtss_xxrp_start_leaveall_timer(mad, FALSE);

    return VTSS_RC_OK;
}

/* This function is called with lock */
mesa_rc vtss_mrp_mad_destroy_port(vtss_mrp_t *appl, vtss_mrp_mad_t **mad_ports, u32 l2port)
{
    mesa_rc rc;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Destroying MAD on port " << l2port2str(l2port);
    /* TODO: Send leave request on this port for all the registered attributes */
    rc = XXRP_SYS_FREE(mad_ports[l2port]->machines);
    mad_ports[l2port]->machines = NULL;
    rc = XXRP_SYS_FREE(mad_ports[l2port]);
    mad_ports[l2port] = NULL;
    return rc;
}
