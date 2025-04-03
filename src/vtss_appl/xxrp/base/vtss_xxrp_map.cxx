/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss_xxrp_map.h"
#include "vtss_xxrp_os.h"
#include "vtss_xxrp_callout.h"
#include "vtss_xxrp_madtt.h"
#include "vtss_xxrp_mvrp.h"
#include "icli_api.h"
#include "../platform/xxrp_trace.h"

mesa_rc vtss_mrp_map_create_port(vtss_mrp_map_t **map_ports, u32 l2port)
{
    vtss_mrp_map_t *map;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Creating MAP on port " << l2port2str(l2port);
    map = (vtss_mrp_map_t *)XXRP_SYS_MALLOC(sizeof(vtss_mrp_map_t));
    memset(map, 0, sizeof(vtss_mrp_map_t));
    map->port = l2port;
    map_ports[l2port] = map;
    return VTSS_RC_OK;
}

mesa_rc vtss_mrp_map_destroy_port(vtss_mrp_map_t **map_ports, u32 l2port)
{
    mesa_rc rc;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Destroying MAP on port " << l2port2str(l2port);
    rc = XXRP_SYS_FREE(map_ports[l2port]);
    map_ports[l2port] = NULL;
    return rc;
}

mesa_rc vtss_mrp_map_connect_port(vtss_mrp_map_t **map_ports, u8 msti, u32 l2port)
{
    mesa_rc tmp_port, prev_port = L2_MAX_PORTS_ + 1;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAP Connecting port " << l2port2str(l2port) << " to MSTI " << msti;
    /* Traverse through the ring */
    for (tmp_port = (l2port + 1); tmp_port != l2port; ) {
        if (tmp_port == L2_MAX_PORTS_) { /* tmp_port reached maximum, start from the first port */
            tmp_port = 0;
            continue;
        }
        if (tmp_port == l2port) {
            break;
        }
        /* Check if any port is in the ring */
        if ((map_ports[tmp_port]) && (map_ports[tmp_port]->is_connected & (1 << msti))) {
            if (prev_port == (L2_MAX_PORTS_ + 1)) {
                map_ports[l2port]->next_in_connected_ring[msti] = map_ports[tmp_port];
            }
            prev_port = tmp_port;
        }
        tmp_port++;
    }
    if (prev_port == (L2_MAX_PORTS_ + 1)) {
        map_ports[l2port]->next_in_connected_ring[msti] = map_ports[l2port];
    } else {
        map_ports[prev_port]->next_in_connected_ring[msti] = map_ports[l2port];
    }
    map_ports[l2port]->is_connected |= (1 << msti);
    return VTSS_RC_OK;
}

mesa_rc vtss_mrp_map_disconnect_port(vtss_mrp_map_t **map_ports, u8 msti, u32 l2port)
{
    vtss_mrp_map_t *tmp_map, *prev_map = NULL;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAP Disconnecting port " << l2port2str(l2port) << " from MSTI " << msti;
    if (!map_ports[l2port]) { /* This should not happen in ideal case */
        return XXRP_ERROR_NOT_FOUND;
    }
    tmp_map = (vtss_mrp_map_t *)map_ports[l2port]->next_in_connected_ring[msti];
    if (tmp_map) {
        for (; tmp_map != map_ports[l2port]; tmp_map = (vtss_mrp_map_t *)tmp_map->next_in_connected_ring[msti]) {
            prev_map = tmp_map;
        }
        if (prev_map) { /* Multiple nodes in the list case */
            prev_map->next_in_connected_ring[msti] = map_ports[l2port]->next_in_connected_ring[msti];
        }
        map_ports[l2port]->next_in_connected_ring[msti] = NULL;
        map_ports[l2port]->is_connected &= ~(1U << msti);
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_mrp_map_find_port(vtss_mrp_map_t **map_ports, u8 msti, u32 l2port, vtss_mrp_map_t **map)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAP fetching port " << l2port << ", connected to MSTI " << msti;
    if (map_ports[l2port]->is_connected & (1 << msti)) {
        *map = map_ports[l2port];
        return VTSS_RC_OK;
    }
    return XXRP_ERROR_NOT_FOUND;
}

void vtss_mrp_map_print_msti_ports(vtss_mrp_map_t **map_ports, u8 msti)
{
    u32             tmp_port;
    vtss_mrp_map_t  *tmp_map = NULL, *first_port = NULL;

    /* Iterate in order to find the first port and fetch the connected ring */
    for (tmp_port = 0; tmp_port < L2_MAX_PORTS_; tmp_port++) {
        if (map_ports[tmp_port]) {
            if (map_ports[tmp_port]->is_connected & (1 << msti)) {
                printf("port_no:%u", (tmp_port + 1));
                tmp_map = (vtss_mrp_map_t *)map_ports[tmp_port]->next_in_connected_ring[msti];
                first_port = map_ports[tmp_port];
                break;
            }
        }
    }
    /* Now iterate through the connected ring and print the rest */
    if (tmp_port < L2_MAX_PORTS_) {
        while (tmp_map) {
            if (tmp_map == first_port) {
                break;
            }
            printf("=>");
            printf("port_no:%u", (tmp_map->port + 1));
            tmp_map = (vtss_mrp_map_t *)tmp_map->next_in_connected_ring[msti];
        }
    } else {
        printf("No ports were members of the msti = %u", msti);
    }
    printf("\n");
}

mesa_rc vtss_mrp_map_propagate_join(vtss_mrp_appl_t appl, vtss_mrp_map_t **map_ports, u32 l2port, u32 mad_indx)
{
    mesa_rc                   rc = VTSS_RC_OK;
#ifdef VTSS_SW_OPTION_MVRP
    vtss_mrp_map_t            *map, *current_port;
    u32                       tmp_port = l2port;
    vtss_mad_fsm_events       fsm_events;
    u8                        msti;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Enter propagate join on port " <<  l2port2str(l2port);
    //Get msti from the attribute
    vtss_mrp_get_msti_from_mad_fsm_index(mad_indx, &msti);
    if ((vtss_mrp_map_find_port(map_ports, msti, tmp_port, &map)) == VTSS_RC_OK) {
        current_port = map;
        map = (vtss_mrp_map_t *)map->next_in_connected_ring[msti];
        while (map != current_port) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAP is propagating Join event to port " << l2port2str(map->port);
            fsm_events.la_event = last_la_event;
            fsm_events.periodic_event = last_periodic_event;
            fsm_events.reg_event = last_reg_event;
            fsm_events.appl_event = join_app;
            rc += vtss_mrp_mad_process_events(appl, map->port, mad_indx, &fsm_events);
            map = (vtss_mrp_map_t *)map->next_in_connected_ring[msti];
        }
    }
#endif
    return rc;
}

mesa_rc vtss_mrp_map_propagate_leave(vtss_mrp_appl_t appl, vtss_mrp_map_t **map_ports, u32 l2port, u32 mad_indx)
{
    mesa_rc                     rc = VTSS_RC_OK;
#ifdef VTSS_SW_OPTION_MVRP
    vtss_mrp_map_t              *map, *current_port;
    u32                         tmp_port = l2port;
    vtss_mad_fsm_events         fsm_events;
    u8                          msti;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Enter propagate leave on port " << l2port2str(l2port);
    //Get msti from the attribute
    vtss_mrp_get_msti_from_mad_fsm_index(mad_indx, &msti);
    if ((vtss_mrp_map_find_port(map_ports, msti, tmp_port, &map)) == VTSS_RC_OK) {
        current_port = map;
        map = (vtss_mrp_map_t *)map->next_in_connected_ring[msti];
        while (map != current_port) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MAP is propagating leave event to port " << l2port2str(map->port);
            fsm_events.la_event = last_la_event;
            fsm_events.periodic_event = last_periodic_event;
            fsm_events.reg_event = last_reg_event;
            fsm_events.appl_event = lv_app;
            rc += vtss_mrp_mad_process_events(appl, map->port, mad_indx, &fsm_events);
            map = (vtss_mrp_map_t *)map->next_in_connected_ring[msti];
        }
    }
#endif
    return rc;
}
