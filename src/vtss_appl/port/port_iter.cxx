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

#include "port_iter.hxx"
#include "port_api.h"
#include "port_trace.h"
#include "msg_api.h"
#include "port_instance.hxx"
#include "topo_api.h"
#include "misc_api.h"

extern CapArray<port_instance_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_instances;

/******************************************************************************/
// PORT_ITER_isid_port_no_is_cpu_port()
/******************************************************************************/
static bool PORT_ITER_isid_port_no_is_cpu_port(vtss_isid_t isid, mesa_port_no_t port_no)
{
    // Check if port is within the number of ports for the switch.
    return (port_custom_table[port_no].cap & MEBA_PORT_CAP_CPU) == MEBA_PORT_CAP_CPU;
}

/******************************************************************************/
// PORT_ITER_init_internal()
/******************************************************************************/
static mesa_rc PORT_ITER_init_internal(port_iter_t *pit, switch_iter_t *sit, vtss_isid_t isid, port_iter_sort_order_t sort_order, uint32_t flags)
{
    uint32_t       port_cnt_max;
    uint32_t       port_cnt_real;
    mesa_port_no_t iport;

    T_NG(PORT_TRACE_GRP_ITER, "enter: isid: %u, sort_order: %d, flags: %02x", isid, sort_order, flags);

    if (pit == NULL) {
        T_EG(PORT_TRACE_GRP_ITER, "Invalid pit");
        return VTSS_INVALID_PARAMETER;
    }

    memset(pit, 0, sizeof(*pit)); // Initialize this before checking for errors. This will enable getnext() to work as expected.

    if (sit) {
        if (!msg_switch_is_primary()) {
            T_DG(PORT_TRACE_GRP_ITER, "Not primary switch");
            return VTSS_UNSPECIFIED_ERROR;
        }

        if ((isid != VTSS_ISID_GLOBAL) && !VTSS_ISID_LEGAL(isid)) {
            T_EG(PORT_TRACE_GRP_ITER, "Invalid isid");
            return VTSS_INVALID_PARAMETER;
        }
    } else {
        if ((isid != VTSS_ISID_LOCAL) && !msg_switch_is_primary()) {
            T_DG(PORT_TRACE_GRP_ITER, "Not primary switch");
            return VTSS_UNSPECIFIED_ERROR;
        }

        if ((isid != VTSS_ISID_LOCAL) && !VTSS_ISID_LEGAL(isid)) {
            T_EG(PORT_TRACE_GRP_ITER, "Invalid isid (%u)", isid);
            return VTSS_INVALID_PARAMETER;
        }
    }

    if (sort_order > PORT_ITER_SORT_ORDER_IPORT_ALL) {
        T_EG(PORT_TRACE_GRP_ITER, "Invalid sort_order");
        return VTSS_INVALID_PARAMETER;
    }

    pit->m_sit = sit;
    pit->m_isid = isid;
    pit->m_order = sort_order;
    pit->m_flags = (port_iter_flags_t) flags;

    if (sit && (isid == VTSS_ISID_GLOBAL)) {
        pit->m_state = PORT_ITER_STATE_INIT;
        T_NG(PORT_TRACE_GRP_ITER, "exit - has sit and global");
        return VTSS_RC_OK; // We will be called again with a 'legal' isid
    }

    port_cnt_real = port_count_max();
    port_cnt_max  = (sort_order == PORT_ITER_SORT_ORDER_IPORT_ALL ||
                     sort_order == PORT_ITER_SORT_ORDER_UPORT) ? fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) : port_cnt_real;

    // There is currently no difference in sorting order for iport and uport. Use same algorithm.
    for (iport = 0; iport < port_cnt_max; iport++) {
        // The following must be modified when we add support for other types than front and CPU:
        port_iter_type_t port_type;
        if (port_is_front_port(iport)) {
            port_type = PORT_ITER_TYPE_FRONT;
        } else if (PORT_ITER_isid_port_no_is_cpu_port(isid, iport)) {
            port_type = PORT_ITER_TYPE_CPU;
        } else {
            continue;
        }

        if ((flags & (1 << port_type)) == 0) {
            continue; // Port type not present in flags - skip it
        }

        if (port_instances[iport].has_link()) { // Port is up
            if (flags & PORT_ITER_FLAGS_DOWN) {
                continue; // We only want ports that are down - skip it
            }
        } else { // Port is down
            if (flags & PORT_ITER_FLAGS_UP) {
                continue; // We only want ports that are up - skip it
            }
        }

        pit->m_port_mask |= 1LLU << iport;
        T_DG(PORT_TRACE_GRP_ITER, "add iport %u type %d to m_port_mask", iport, port_type);
        if (iport < port_cnt_real || port_type == PORT_ITER_TYPE_CPU) {
            pit->m_exists_mask |= 1LLU << iport;
            T_DG(PORT_TRACE_GRP_ITER, "add iport %u type %d to m_exists_mask", iport, port_type);
        }
    }

    T_NG(PORT_TRACE_GRP_ITER, "exit");
    return VTSS_RC_OK;
}

/******************************************************************************/
// switch_iter_init()
/******************************************************************************/
mesa_rc switch_iter_init(switch_iter_t *sit, vtss_isid_t isid, switch_iter_sort_order_t sort_order)
{
    T_NG(PORT_TRACE_GRP_ITER, "enter: isid: %u, sort_order: %d", isid, sort_order);

    if (sit == NULL) {
        T_E("Invalid sit");
        return VTSS_INVALID_PARAMETER;
    }

    memset(sit, 0, sizeof(*sit)); // Initialize this before checking for errors. This will enable getnext() to work as expected.

    if ((isid != VTSS_ISID_GLOBAL) && !VTSS_ISID_LEGAL(isid)) {
        T_EG(PORT_TRACE_GRP_ITER, "Invalid isid:%d", isid);
        return VTSS_INVALID_PARAMETER;
    }

    if (sort_order > SWITCH_ITER_SORT_ORDER_END) {
        T_EG(PORT_TRACE_GRP_ITER, "Invalid sort_order");
        return VTSS_INVALID_PARAMETER;
    }

    sit->m_order = sort_order;

    if (isid == VTSS_ISID_GLOBAL) {
        switch (sort_order) {
        case SWITCH_ITER_SORT_ORDER_USID:
        case SWITCH_ITER_SORT_ORDER_USID_CFG: {
            uint32_t exists_mask = sort_order == SWITCH_ITER_SORT_ORDER_USID_CFG ? msg_configurable_switches() : msg_existing_switches();
            vtss_usid_t s;
            for (s = VTSS_USID_START; s < VTSS_USID_END; s++) {
                if (exists_mask & (1LU << topo_usid2isid(s))) {
                    sit->remaining++;
                    sit->m_switch_mask |= 1LU << s;
                    sit->m_exists_mask |= 1LU << s;
                    T_DG(PORT_TRACE_GRP_ITER, "add usid %u to m_xxxxx_mask", s);
                }
            }

            break;
        }

        case SWITCH_ITER_SORT_ORDER_ISID:
        case SWITCH_ITER_SORT_ORDER_ISID_CFG:
        case SWITCH_ITER_SORT_ORDER_ISID_ALL: {
            uint32_t exists_mask = sort_order == SWITCH_ITER_SORT_ORDER_ISID_CFG ? msg_configurable_switches() : msg_existing_switches();
            vtss_isid_t s;
            for (s = VTSS_ISID_START; s < VTSS_ISID_END; s++) {
                bool exists = (exists_mask & (1LU << s)) != 0;
                if ((sort_order == SWITCH_ITER_SORT_ORDER_ISID_ALL) || exists) {
                    sit->remaining++;
                    sit->m_switch_mask |= 1LU << s;
                    T_DG(PORT_TRACE_GRP_ITER, "add isid %u to m_switch_mask", s);
                    if (exists) {
                        sit->m_exists_mask |= 1LU << s;
                        T_DG(PORT_TRACE_GRP_ITER, "add isid %u to m_exists_mask", s);
                    }
                }
            }

            break;
        }
        } /* switch */
    } else {
        bool exists = FALSE;
        switch (sort_order) {
        case SWITCH_ITER_SORT_ORDER_USID:
        case SWITCH_ITER_SORT_ORDER_ISID:
            exists = msg_switch_exists(isid);
            break;

        case SWITCH_ITER_SORT_ORDER_USID_CFG:
        case SWITCH_ITER_SORT_ORDER_ISID_CFG:
            exists = msg_switch_configurable(isid);
            break;

        case SWITCH_ITER_SORT_ORDER_ISID_ALL:
            exists = TRUE;
            break;
        }

        if (exists) {
            sit->remaining = 1;
            sit->m_switch_mask = 1LU << isid;
            T_DG(PORT_TRACE_GRP_ITER, "add isid %u to m_switch_mask", isid);
        }
    }

    T_NG(PORT_TRACE_GRP_ITER, "exit");
    return VTSS_RC_OK;
}

/******************************************************************************/
// switch_iter_getnext()
/******************************************************************************/
bool switch_iter_getnext(switch_iter_t *sit)
{
    if (sit == NULL) {
        T_EG(PORT_TRACE_GRP_ITER, "Invalid sit");
        return FALSE;
    }

    T_NG(PORT_TRACE_GRP_ITER, "enter %d", sit->m_state);

    switch (sit->m_state) {
    case SWITCH_ITER_STATE_FIRST:
    case SWITCH_ITER_STATE_NEXT:
        if (sit->m_switch_mask) {
            // Handle the first call
            if (sit->m_state == SWITCH_ITER_STATE_FIRST) {
                sit->first = TRUE;
                sit->last = FALSE;
                sit->m_state = SWITCH_ITER_STATE_NEXT;
            } else {
                sit->first = FALSE;
            }

            // Skip non-existing switches
            while (!(sit->m_switch_mask & 1)) {
                sit->m_switch_mask >>= 1;
                sit->m_exists_mask >>= 1;
                sit->m_sid++;
            }

            // Update isid, usid and exists with info about the found switch
            if (sit->m_order == SWITCH_ITER_SORT_ORDER_USID || sit->m_order == SWITCH_ITER_SORT_ORDER_USID_CFG) {
                sit->isid = topo_usid2isid(sit->m_sid);
                sit->usid = sit->m_sid;
            } else {
                sit->isid = sit->m_sid;
                sit->usid = (sit->m_order == SWITCH_ITER_SORT_ORDER_ISID_ALL) ? 0 : topo_isid2usid(sit->m_sid);
            }

            sit->exists = (sit->m_exists_mask & 1);

            // Skip this switch
            sit->m_switch_mask >>= 1;
            sit->m_exists_mask >>= 1;
            sit->m_sid++;

            // Update the last flag
            if (!sit->m_switch_mask) {
                sit->last = TRUE;
                sit->m_state = SWITCH_ITER_STATE_LAST;
            }

            // Update the remaining counter
            if (sit->remaining) {
                sit->remaining--;
            } else {
                T_EG(PORT_TRACE_GRP_ITER, "Internal error in remaining counter");
            }

            T_DG(PORT_TRACE_GRP_ITER, "isid %u, usid %u, first %u, last %u, exists %u, remaining %u", sit->isid, sit->usid, sit->first, sit->last, sit->exists, sit->remaining);
            return TRUE; // We have a switch
        } else {
            sit->m_state = SWITCH_ITER_STATE_DONE;
        }

        break;

    case SWITCH_ITER_STATE_LAST:
        sit->m_state = SWITCH_ITER_STATE_DONE;
        break;

    case SWITCH_ITER_STATE_DONE:
    default:
        T_EG(PORT_TRACE_GRP_ITER, "Invalid state");
    }

    T_NG(PORT_TRACE_GRP_ITER, "exit FALSE");
    return FALSE;
}

/******************************************************************************/
// port_iter_init()
/******************************************************************************/
mesa_rc port_iter_init(port_iter_t *pit, switch_iter_t *sit, vtss_isid_t isid, port_iter_sort_order_t sort_order, uint32_t flags)
{
    // If sit != NULL then use isid = VTSS_ISID_GLOBAL no matter what isid was.
    return PORT_ITER_init_internal(pit, sit, (sit) ? VTSS_ISID_GLOBAL : isid, sort_order, flags);
}

/******************************************************************************/
// port_iter_init_local()
/******************************************************************************/
void port_iter_init_local(port_iter_t *pit)
{
    (void)port_iter_init(pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
}

/******************************************************************************/
// port_iter_init_local_all()
/******************************************************************************/
void port_iter_init_local_all(port_iter_t *pit)
{
    (void)port_iter_init(pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT_ALL, PORT_ITER_FLAGS_NORMAL);
}

/******************************************************************************/
// port_iter_getnext()
/******************************************************************************/
bool port_iter_getnext(port_iter_t *pit)
{
    if (pit == NULL) {
        T_EG(PORT_TRACE_GRP_ITER, "Invalid pit");
        return FALSE;
    }

    T_NG(PORT_TRACE_GRP_ITER, "enter %d", pit->m_state);

    while (TRUE) {
        T_DG(PORT_TRACE_GRP_ITER, "state %d", pit->m_state);
        switch (pit->m_state) {
        case PORT_ITER_STATE_INIT:
            if (switch_iter_getnext(pit->m_sit)) {
                // Reinitialize the port iterator with isid from current switch
                (void)PORT_ITER_init_internal(pit, pit->m_sit, pit->m_sit->isid, pit->m_order, pit->m_flags);
                // No return here. Take another round
            } else {
                pit->m_state = PORT_ITER_STATE_DONE;
                return FALSE;
            }

            break;

        case PORT_ITER_STATE_FIRST:
        case PORT_ITER_STATE_NEXT:
            if (pit->m_port_mask) {

                // Handle the first call
                if (pit->m_state == PORT_ITER_STATE_FIRST) {
                    pit->first = TRUE;
                    pit->last = FALSE;
                    pit->m_state = PORT_ITER_STATE_NEXT;
                } else {
                    pit->first = FALSE;
                }

                // Skip non-existing ports
                while (!(pit->m_port_mask & 1)) {
                    pit->m_port_mask >>= 1;
                    pit->m_exists_mask >>= 1;
                    pit->m_port++;
                }

                // Update iport, uport and exists with info about the found port
                pit->iport  = pit->m_port;
                pit->uport  = iport2uport(pit->m_port);
                pit->exists = (pit->m_exists_mask & 1);
                pit->link   = port_instances[pit->m_port].has_link();

                // The following must be modified when we add support for other types than front:
                pit->type = PORT_ITER_TYPE_FRONT;

                // Skip this port
                pit->m_port_mask >>= 1;
                pit->m_exists_mask >>= 1;
                pit->m_port++;

                // Update the last flag
                if (!pit->m_port_mask) {
                    pit->last = TRUE;
                    pit->m_state = PORT_ITER_STATE_LAST;
                }

                T_DG(PORT_TRACE_GRP_ITER, "iport %u, uport %u, first %u, last %u, exists %u", pit->iport, pit->uport, pit->first, pit->last, pit->exists);
                return TRUE; // We have a port
            } else {
                if (pit->m_sit) {
                    pit->m_state = PORT_ITER_STATE_INIT; // Try next switch
                    // No return here. Take another round
                } else {
                    pit->m_state = PORT_ITER_STATE_DONE;
                    return FALSE;
                }
            }

            break;

        case PORT_ITER_STATE_LAST:
            if (pit->m_sit) {
                pit->m_state = PORT_ITER_STATE_INIT; // Try next switch
                // No return here. Take another round
            } else {
                pit->m_state = PORT_ITER_STATE_DONE;
                return FALSE;
            }

            break;

        case PORT_ITER_STATE_DONE:
        default:
            T_EG(PORT_TRACE_GRP_ITER, "Invalid state:%d", pit->m_state);
            return FALSE;
        }
    }
}
