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

#include <microchip/ethernet/switch/api.h>
#include "erps_base.hxx"
#include "erps_raps.hxx"
#include "erps_trace.h"
#include "erps_lock.hxx"
#include "cfm_api.h"     /* For CFM_ETYPE             */
#include "mac_utils.hxx" /* For operator<(mesa_mac_t) */
#include "misc_api.h"    /* For misc_mac_txt()        */
#include "mgmt_api.h"    /* For mgmt_iport_list2txt() */
#include "vlan_api.h"    /* For vlan_mgmt_XXX()       */

// We need to ref-count how many times we have enabled ingress filtering on a
// given port, so that we can enable in the VLAN module once it changes from 0
// to 1 and disable once it changes from 1 to 0.
static CapArray<uint32_t, MEBA_CAP_BOARD_PORT_COUNT> ERPS_BASE_vlan_ingress_filtering_ref_cnt;

/******************************************************************************/
// ERPS_BASE_bool_to_yes_no()
/******************************************************************************/
static const char *ERPS_BASE_bool_to_yes_no(bool val)
{
    return val ? "Yes" : "No";
}

/******************************************************************************/
// ERPS_BASE_bool_to_yes_no_dash()
/******************************************************************************/
static const char *ERPS_BASE_bool_to_yes_no_dash(erps_state_t *erps_state, bool val)
{
    return erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB && !erps_state->conf.virtual_channel ? "-" : val ? "Yes" : "No";
}

//*****************************************************************************/
// ERPS_BASE_request_to_str()
/******************************************************************************/
static const char *ERPS_BASE_request_to_str(erps_base_request_t request)
{
    switch (request) {
    case ERPS_BASE_REQUEST_NONE:
        return "None";

    case ERPS_BASE_REQUEST_REMOTE_NR:
        return "R-NR";

    case ERPS_BASE_REQUEST_REMOTE_NR_RB:
        return "R-NR-RB";

    case ERPS_BASE_REQUEST_LOCAL_WTB_RUNNING:
        return "L-WTB-Running";

    case ERPS_BASE_REQUEST_LOCAL_WTB_EXPIRES:
        return "L-WTB-Expired";

    case ERPS_BASE_REQUEST_LOCAL_WTR_RUNNING:
        return "L-WTR-Running";

    case ERPS_BASE_REQUEST_LOCAL_WTR_EXPIRES:
        return "L-WTR-Expired";

    case ERPS_BASE_REQUEST_LOCAL_MS:
        return "L-MS";

    case ERPS_BASE_REQUEST_REMOTE_MS:
        return "R-MS";

    case ERPS_BASE_REQUEST_REMOTE_SF:
        return "R-SF";

    case ERPS_BASE_REQUEST_LOCAL_SF_CLEAR:
        return "L-SF-Clear";

    case ERPS_BASE_REQUEST_LOCAL_SF:
        return "L-SF";

    case ERPS_BASE_REQUEST_REMOTE_FS:
        return "R-FS";

    case ERPS_BASE_REQUEST_LOCAL_FS:
        return "L-FS";

    case ERPS_BASE_REQUEST_LOCAL_CLEAR:
        return "L-Clear";

    default:
        T_EG(ERPS_TRACE_GRP_BASE, "Invalid request prio (%d)", request);
        return "Unknown";
    }
}

//*****************************************************************************/
// ERPS_BASE_flush_reason_to_str()
/******************************************************************************/
static const char *ERPS_BASE_flush_reason_to_str(erps_base_flush_reason_t flush_reason)
{
    switch (flush_reason) {
    case ERPS_BASE_FLUSH_REASON_NONE:
        return "No";

    case ERPS_BASE_FLUSH_REASON_FSM:
        return "FSM";

    case ERPS_BASE_FLUSH_REASON_EVENT:
        return "Event";

    case ERPS_BASE_FLUSH_REASON_NODE_ID:
        return "NodeID";

    default:
        T_EG(ERPS_TRACE_GRP_BASE, "Unknown flush reason (%d)", flush_reason);
        return "Unknown";
    }
}

//*****************************************************************************/
// ERPS_BASE_opposite_ring_port
/******************************************************************************/
static vtss_appl_erps_ring_port_t ERPS_BASE_opposite_ring_port(vtss_appl_erps_ring_port_t ring_port)
{
    return ring_port == VTSS_APPL_ERPS_RING_PORT0 ? VTSS_APPL_ERPS_RING_PORT1 : VTSS_APPL_ERPS_RING_PORT0;
}

/******************************************************************************/
// ERPS_BASE_rx_timer_start()
/******************************************************************************/
static void ERPS_BASE_rx_timer_start(erps_state_t *erps_state)
{
    erps_timer_start(erps_state->rx_timer, ERPS_RAPS_RX_TIMEOUT_MS, false);
}

//*****************************************************************************/
// ERPS_BASE_cfop_to_update()
//*****************************************************************************/
static void ERPS_BASE_cfop_to_update(erps_state_t *erps_state)
{
    // G.8021, clause 9.1.3:
    // cFOP-TO is not reported if a ring port has a link level failure or is
    // administratively locked or blocked from R-APS.
    //
    // I want to question the usefulness of "blocked from R-APS", because all
    // ring ports do get R-APS PDUs to the CPU even if the R-APS channel is
    // Tx-wise blocked.
    //
    // I have chosen to bend the recommendation in this area: G.8021 specifies
    // a cFOP-TO[0..1], which must mean that there is such a timeout per ring
    // port. However, in a normal ethernet ring, R-APS PDUs only travel in one
    // direction (because of the R-APS channel blocking on the RPL neighbor), so
    // if I made this per ring port, there would be an alarm on the ring port
    // closest to the RPL neighbor in a normally functioning ring. To me, this
    // doesn't make sense. I have looked into how other vendors do it, and at
    // least one does it like I have chosen to do it: Report one such FOP-TO
    // for a given ethernet ring instance and not for a given ring port.
    // So if we expect R-APS PDUs but haven't received a R-APS PDU for the last
    // 17.5 seconds on either ring port, we report dFOP-TO.
    erps_state->status.cFOP_TO = erps_state->dFOP_TO && erps_state->expect_raps_rx;
}

//*****************************************************************************/
// ERPS_BASE_dfop_to_set()
//*****************************************************************************/
static void ERPS_BASE_dfop_to_set(erps_state_t *erps_state, bool new_dfop_to)
{
    if (new_dfop_to != erps_state->dFOP_TO) {
        erps_state->dFOP_TO = new_dfop_to;
        ERPS_BASE_cfop_to_update(erps_state);
    }
}

//*****************************************************************************/
// ERPS_BASE_expect_raps_rx_update()
// This one computes whether we expect reception of R-APS PDUs. There are two
// sources of contribution to this, namely SF on ring port(s) and the kind of
// R-APS PDU we are currently transmitting.
//*****************************************************************************/
static void ERPS_BASE_expect_raps_rx_update(erps_state_t *erps_state)
{
    bool expect_raps_rx_sf_contrib;
    bool expect_raps_rx_pdu_tx_contrib;
    bool new_expect_raps_rx;

    // Compute whether we expect R-APS PDUs based on SF on our ring port(s).
    switch (erps_state->conf.ring_type) {
    case VTSS_APPL_ERPS_RING_TYPE_MAJOR:
    case VTSS_APPL_ERPS_RING_TYPE_SUB:
        // Both of these two ring types use both ring ports, so the only case
        // where we don't expect R-APS PDUs is when both ring ports have SF.
        expect_raps_rx_sf_contrib = !erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT0].sf || !erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].sf;
        break;

    case VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB:
        // We are an interconnected sub-ring. Whether we expect R-APS PDUs
        // depends on whether we have a virtual channel.
        if (erps_state->conf.virtual_channel) {
            // We do, so always expect R-APS PDUs to arrive independent of SF on
            // port0.
            expect_raps_rx_sf_contrib = true;
        } else {
            // We don't so only expect R-APS PDUs to arrive if there is not SF
            // on port0.
            expect_raps_rx_sf_contrib = !erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT0].sf;
        }

        break;
    }

    // Compute whether we expect R-APS PDUs based on RPL mode and the current
    // R-APS PDU we are transmitting.
    if (erps_state->conf.rpl_mode                == VTSS_APPL_ERPS_RPL_MODE_OWNER &&
        erps_state->status.tx_raps_info.request  == VTSS_APPL_ERPS_REQUEST_NR     &&
        erps_state->status.tx_raps_info.rb) {
        // We are RPL owner, and we send R-APS(NR, RB), so do not expect
        // R-APS PDUs to return to us (if we have a configured neighbor on
        // the ring, which will absorb the PDUs).
        expect_raps_rx_pdu_tx_contrib = false;
    } else {
        expect_raps_rx_pdu_tx_contrib = true;
    }

    // If both expect_raps_rx_pdu_tx_contrib and expect_raps_rx_sf_contrib are
    // true, we do indeed expect reception of R-APS PDUs.
    new_expect_raps_rx = expect_raps_rx_sf_contrib && expect_raps_rx_pdu_tx_contrib;

    T_IG(ERPS_TRACE_GRP_BASE, "%u: exp_rx(sf) = %d, exp_rx(tx) = %d => new_exp_rx = %d (old was %d)",
         erps_state->inst,
         expect_raps_rx_sf_contrib,
         expect_raps_rx_pdu_tx_contrib,
         new_expect_raps_rx,
         erps_state->expect_raps_rx);

    if (new_expect_raps_rx == erps_state->expect_raps_rx) {
        // No change. Done.
        return;
    }

    erps_state->expect_raps_rx = new_expect_raps_rx;

    // If we transitioned from not expecting R-APS PDUs to expecting R-APS PDUs,
    // we must re-start the Rx timer, so that we can signal cFOP-TO if we don't
    // get one within the next 17.5 seconds.
    if (new_expect_raps_rx) {
        erps_state->dFOP_TO = false;
        ERPS_BASE_rx_timer_start(erps_state);
    }

    ERPS_BASE_cfop_to_update(erps_state);
}

//*****************************************************************************/
// ERPS_BASE_sf_set()
/******************************************************************************/
static void ERPS_BASE_sf_set(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, bool new_sf)
{
    if (new_sf == erps_state->ring_port_state[ring_port].sf) {
        return;
    }

    erps_state->ring_port_state[ring_port].sf = new_sf;
    ERPS_BASE_expect_raps_rx_update(erps_state);
}

//*****************************************************************************/
// ERPS_BASE_ace_ingress_port_list_get()
/******************************************************************************/
static void ERPS_BASE_ace_ingress_port_list_get(erps_state_t *erps_state, acl_entry_conf_t &ace_conf)
{
    erps_port_state_t          *port_state;
    vtss_appl_erps_ring_port_t ring_port;
    int                        p;

    // Ingress port list
    ace_conf.port_list.clear_all();

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        for (p = 0; p < ARRSZ(erps_state->ring_port_state[ring_port].port_states); p++) {
            port_state = erps_state->ring_port_state[ring_port].port_states[p];

            if (port_state == nullptr) {
                continue;
            }

            // Populate ingress port list with ring ports. In case of an
            // interconnected sub-ring with a virtual channel, ring port1's
            // port_states[] point to the connected ring's ring ports, which we
            // also match on ingress-wise then. In other cases, port_states[1]
            // is always nullptr and we won't get here.
            ace_conf.port_list[port_state->port_no] = TRUE;
        }
    }
}

//*****************************************************************************/
// ERPS_BASE_ace_egress_port_list_get()
/******************************************************************************/
static void ERPS_BASE_ace_egress_port_list_get(erps_state_t *erps_state, acl_entry_conf_t &ace_conf)
{
    erps_port_state_t          *port_state;
    vtss_appl_erps_ring_port_t ring_port;
    int                        p;
    bool                       forward_to_this_port[2], forward_between_ring_ports, blocked;

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        blocked = erps_state->status.ring_port_status[ring_port].blocked;

        // First figure out whether we should forward R-APS PDUs bewteen ring
        // ports. This depends a.o. on whether a ring port is blocked, the ring
        // type, and virtual channel mode.
        switch (erps_state->conf.ring_type) {
        case VTSS_APPL_ERPS_RING_TYPE_MAJOR:
            // Do not forward R-APS PDUs to a blocked ring port.
            // G.8032, clause 9.5.
            forward_to_this_port[ring_port] = !blocked;
            break;

        case VTSS_APPL_ERPS_RING_TYPE_SUB:
            if (erps_state->conf.virtual_channel) {
                // This is just like a major ring, where we only forward
                // R-APS PDUs if the ring port is not blocked
                forward_to_this_port[ring_port] = !blocked;
            } else {
                // G.8032, clause 10.1.14.
                // Always forward R-APS PDUs from one ring-port to another
                // whether they are blocked or not, EXCEPT when highest
                // priority request is local SF or local FS, where it
                // follows normal service traffic blocking.
                if (erps_state->top_prio_request == ERPS_BASE_REQUEST_LOCAL_SF || erps_state->top_prio_request == ERPS_BASE_REQUEST_LOCAL_FS) {
                    forward_to_this_port[ring_port] = !blocked;
                } else {
                    forward_to_this_port[ring_port] = true;
                }
            }

            break;

        case VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB:
            // G.8032, clause 9.5.
            if (erps_state->conf.virtual_channel) {
                forward_to_this_port[ring_port] = !blocked;
            } else {
                // Always terminate R-APS PDUs at an interconnected sub-ring
                // without a virtual channel.
                forward_to_this_port[ring_port] = false;
            }

            break;

        default:
            T_EG(ERPS_TRACE_GRP_BASE, "%u: Unknown ring type (%d)", erps_state->inst, erps_state->conf.ring_type);
            return;
        }
    }

    // We only forward between ring ports if both port0 and port1 should be
    // forwarded to.
    forward_between_ring_ports = forward_to_this_port[VTSS_APPL_ERPS_RING_PORT0] && forward_to_this_port[VTSS_APPL_ERPS_RING_PORT1];

    // First clear the egress port list
    ace_conf.action.port_list.clear_all();

    if (!forward_between_ring_ports) {
        // Nothing more to do.
        return;
    }

    // Loop once again, where we set-up whether to forward R-APS PDUs between
    // the ring ports.
    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        for (p = 0; p < ARRSZ(erps_state->ring_port_state[ring_port].port_states); p++) {
            port_state = erps_state->ring_port_state[ring_port].port_states[p];

            if (port_state == nullptr) {
                continue;
            }

            // This allows ring ports to forward R-APS PDUs to each other. In
            // case of an interconnected sub-ring with virtual channel, we also
            // enable forwarding between the connected ring's ring ports. That
            // should be OK, because our control VLAN is amongst the connected
            // ring's protected VLANs, so if that instance has blocked a ring
            // port, the ERPS API state for that instance has not included our
            // control VLAN on the blocked port, so if we create an ACL rule
            // that forwards to that blocked port, it will have no effect.
            ace_conf.action.port_list[port_state->port_no] = TRUE;
        }
    }
}

//*****************************************************************************/
// ERPS_BASE_ace_remove()
/******************************************************************************/
static mesa_rc ERPS_BASE_ace_remove(erps_state_t *erps_state)
{
    mesa_rc rc;

    if (erps_state->ace_conf.id == ACL_MGMT_ACE_ID_NONE) {
        return VTSS_RC_OK;
    }

    // Delete it from the ACL module
    T_IG(ERPS_TRACE_GRP_ACL, "%u: acl_mgmt_ace_del(0x%x)", erps_state->inst, erps_state->ace_conf.id);
    if ((rc = acl_mgmt_ace_del(ACL_USER_ERPS, erps_state->ace_conf.id)) != VTSS_RC_OK) {
        T_EG(ERPS_TRACE_GRP_ACL, "%u: acl_mgmt_ace_del(0x%x) failed: %s", erps_state->inst, erps_state->ace_conf.id, error_txt(rc));
    }

    erps_state->ace_conf.id = ACL_MGMT_ACE_ID_NONE;

    return rc;
}

//*****************************************************************************/
// ERPS_BASE_ace_update()
// Updates ring ID, MEG level and ingress and egress port lists.
/******************************************************************************/
static mesa_rc ERPS_BASE_ace_update(erps_state_t *erps_state)
{
    acl_entry_conf_t new_ace_conf = erps_state->ace_conf /* 228 bytes */;
    mesa_ace_id_t    old_ace_id = new_ace_conf.id;
    mesa_rc          rc;

    // Fill in the variable ring ID and MEG level (variable, because we don't
    // reset everything when these two configuration properties change).
    new_ace_conf.frame.etype.dmac.value[5] = erps_state->conf.ring_id;
    new_ace_conf.frame.etype.data.value[0] = erps_state->conf.level << 5;

    // Fill in the ingress port list, which depends on the ring ports, and - in
    // case of an interconnected sub-ring with a virtual channel - also the
    // connected ring's ring ports, which is why the ingress port list is
    // runtime variable.
    ERPS_BASE_ace_ingress_port_list_get(erps_state, new_ace_conf);

    // And the egress port list, which depends on ring type and blocked state,
    // which are runtime variable properties.
    ERPS_BASE_ace_egress_port_list_get(erps_state, new_ace_conf);

    // Time to install it if not already installed.
    if (new_ace_conf.id != ACL_MGMT_ACE_ID_NONE) {
        // Already installed. Check to see if the configuration has changed.
        if (memcmp(&new_ace_conf, &erps_state->ace_conf, sizeof(new_ace_conf)) == 0) {
            // No changes
            T_DG(ERPS_TRACE_GRP_ACL, "%u: No ACL changes", erps_state->inst);
            return VTSS_RC_OK;
        }
    }

    // The order of these rules doesn't matter, so just place it last within the
    // ERPS group of ACEs (ACL_MGMT_ACE_ID_NONE).
    if ((rc = acl_mgmt_ace_add(ACL_USER_ERPS, ACL_MGMT_ACE_ID_NONE, &new_ace_conf)) != VTSS_RC_OK) {
        T_EG(ERPS_TRACE_GRP_ACL, "%u: acl_mgmt_ace_add(0x%x, VLAN = %u) failed: %s", erps_state->inst, old_ace_id, new_ace_conf.vid.value, error_txt(rc));
        return VTSS_APPL_ERPS_RC_HW_RESOURCES;
    }

    T_IG(ERPS_TRACE_GRP_ACL, "%u: acl_mgmt_ace_add(VLAN = %u) => ace_id = 0x%x->0x%x", erps_state->inst, new_ace_conf.vid.value, old_ace_id, new_ace_conf.id);
    erps_state->ace_conf = new_ace_conf;

    return VTSS_RC_OK;
}

//*****************************************************************************/
// ERPS_BASE_ace_add()
// This function adds basic ACE configuration to ace_conf.
// This is subsequently used by ERPS_BASE_ace_update() to add variable
// configuration.
/******************************************************************************/
static mesa_rc ERPS_BASE_ace_add(erps_state_t *erps_state)
{
    acl_entry_conf_t &ace_conf = erps_state->ace_conf;
    mesa_rc          rc;

    T_IG(ERPS_TRACE_GRP_ACL, "%u: Enter", erps_state->inst);

    if (erps_state->ace_conf.id != ACL_MGMT_ACE_ID_NONE) {
        T_EG(ERPS_TRACE_GRP_ACL, "%u: An ACE (ID = %u) already exists, but we are going to add a new", erps_state->inst, erps_state->ace_conf.id);
        return VTSS_APPL_ERPS_RC_INTERNAL_ERROR;
    }

    T_DG(ERPS_TRACE_GRP_ACL, "%u: acl_mgmt_ace_init()", erps_state->inst);
    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ETYPE, &ace_conf)) != VTSS_RC_OK) {
        T_EG(ERPS_TRACE_GRP_ACL, "%u: acl_mgmt_ace_init() failed: %s", erps_state->inst, error_txt(rc));
        return VTSS_APPL_ERPS_RC_INTERNAL_ERROR;
    }

    // Create the basic, non-variable ACE configuration for this ERPS instance.
    ace_conf.isid                       = VTSS_ISID_LOCAL;
    ace_conf.frame.etype.etype.value[0] = (CFM_ETYPE >> 8) & 0xFF;
    ace_conf.frame.etype.etype.value[1] = (CFM_ETYPE >> 0) & 0xFF;
    ace_conf.frame.etype.etype.mask[0]  = 0xFF;
    ace_conf.frame.etype.etype.mask[1]  = 0xFF;
    ace_conf.id                         = ACL_MGMT_ACE_ID_NONE;

    // Prepare to match only on three MSbits of data[0], which is the MEG level.
    ace_conf.frame.etype.data.mask[0] = 0x7 << 5;

    // Prepare to forward only to ports in action.port_list and to CPU.
    ace_conf.action.port_action         = MESA_ACL_PORT_ACTION_FILTER;
    ace_conf.action.force_cpu           = true;
    ace_conf.action.cpu_queue           = PACKET_XTR_QU_OAM;

    // Match on the R-APS PDU DMAC address. Later on, the last byte of the DMAC
    // gets updated with the Ring ID.
    memcpy(ace_conf.frame.etype.dmac.value, erps_raps_dmac.addr, sizeof(ace_conf.frame.etype.dmac.value));
    memset(ace_conf.frame.etype.dmac.mask, 0xFF,                 sizeof(ace_conf.frame.etype.dmac.mask));

    // Always match on VID.
    ace_conf.vid.value = erps_state->conf.control_vlan;
    ace_conf.vid.mask  = 0xFFFF;

    // Then let ERPS_BASE_ace_update() take care of level, ring ID used in DMAC,
    // and port forwarding, which all may change dynamically.
    return ERPS_BASE_ace_update(erps_state);
}

//*****************************************************************************/
// ERPS_BASE_mesa_state_set()
/******************************************************************************/
static mesa_rc ERPS_BASE_mesa_state_set(erps_state_t *erps_state, mesa_port_no_t port_no, mesa_erps_state_t mesa_state)
{
    const char  *str = mesa_state == MESA_ERPS_STATE_FORWARDING ? "Forward" : "Block";
    mesa_erpi_t erpi = erps_state->erpi();
    mesa_rc     rc;

    T_IG(ERPS_TRACE_GRP_API, "%u: mesa_erps_port_state_set(%u, %u, %s)", erps_state->inst, erpi, port_no, str);
    if ((rc = mesa_erps_port_state_set(nullptr, erpi, port_no, mesa_state)) != VTSS_RC_OK) {
        T_EG(ERPS_TRACE_GRP_API, "%u: mesa_erps_port_state_set(%u, %u, %s) failed: %s", erps_state->inst, erpi, port_no, str, error_txt(rc));

        // Change rc to something sensible
        rc = VTSS_APPL_ERPS_RC_INTERNAL_ERROR;
    }

    return rc;
}

//*****************************************************************************/
// ERPS_BASE_ring_port_block()
// This corresponds to G.8032, clause 10.1.14.
// It blocks/unblocks the ring port in question based on the block argument,
// and checks whether it should also block/unblock the R-APS channel.
//
// enforce_update == true is meant to be used to make sure we call into
// ERPS_BASE_ace_update() and ERPS_BASE_mesa_state_set(), so that various
// state gets updated upon initialization.
//
// enforce_update == false is meant to be used in other cases, where we may call
// this function again and again with the same state.
/******************************************************************************/
static void ERPS_BASE_ring_port_block(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, bool block, bool enforce_update = false)
{
    erps_ring_port_state_t            *ring_port_state = &erps_state->ring_port_state[ring_port];
    vtss_appl_erps_ring_port_status_t *ring_port_status = &erps_state->status.ring_port_status[ring_port];
    bool                              was_blocked = ring_port_status->blocked;

    T_IG(ERPS_TRACE_GRP_BASE, "%u:%s: %sblock", erps_state->inst, ring_port, block ? "" : "un");

    // The thing is that the ACE may change whether block changes or not, so
    // we always have to invoke ERPS_BASE_ace_update(), which expects
    // erps_state->blocked to be updated
    ring_port_status->blocked = block;

    (void)ERPS_BASE_ace_update(erps_state);

    if (!enforce_update && was_blocked == block) {
        // Ring port already has the desired state, and we are not asked to
        // enforce an update, so nothing to do.
        return;
    }

    if (ring_port == VTSS_APPL_ERPS_RING_PORT1 && erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
        // We don't decide the port state of port1 when we are an interconnected
        // sub-ring.
        return;
    }

    if (!ring_port_state->port_states[0]) {
        T_EG(ERPS_TRACE_GRP_BASE, "%u: port_states[0] is nullptr", erps_state->inst);
        return;
    }

    (void)ERPS_BASE_mesa_state_set(erps_state, ring_port_state->port_states[0]->port_no, block ? MESA_ERPS_STATE_DISCARDING : MESA_ERPS_STATE_FORWARDING);

    if (block) {
        // G.8032, clause 10.1.10 (Flush logic):
        // When the ring port is changed to be blocked - as indicated by the
        // block/unblock ring ports signal - the flush logic deletes the current
        // <node ID, BPR> pair on both ring ports.
        for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
            vtss_clear(erps_state->ring_port_state[ring_port].rx_node_id_bpr);
        }
    }
}

//*****************************************************************************/
// ERPS_BASE_raps_tx_stop()
/******************************************************************************/
static void ERPS_BASE_raps_tx_stop(erps_state_t *erps_state)
{
    // Signal to erps_raps.cxx not to transmit any frames on any of our ring
    // ports anymore (leave the tx_timer running, because it doesn't consume
    // any/much CPU power).
    erps_state->status.tx_raps_active = false;
}

//*****************************************************************************/
// ERPS_BASE_raps_tx_start()
/******************************************************************************/
static void ERPS_BASE_raps_tx_start(erps_state_t *erps_state, vtss_appl_erps_raps_info_t &new_tx_raps_info)
{
    vtss_appl_erps_ring_port_t ring_port;
    vtss_appl_erps_raps_info_t &cur_tx_raps_info = erps_state->status.tx_raps_info;
    bool                       changed;

    if (erps_state->conf.rpl_mode != VTSS_APPL_ERPS_RPL_MODE_OWNER && new_tx_raps_info.rb) {
        // G.8032, clause 10.3, bullet h.1.
        // RB: RPL Blocked
        //    I. RB = 1 indicates that the RPL is blocked.
        //   II. RB = 0 indicates that the RPL is unblocked.
        //   This bit should be 0 when transmitted by non-RPL owner nodes.
        T_EG(ERPS_TRACE_GRP_BASE, "%u: Tx R-APS PDU with RB set on non-RPL owner node", erps_state->inst);
    }

    // From G.8032, clause 10.3, bullet h.3:
    // BPR: Blocked port reference
    //    I. BPR = 0 corresponds to ring link 0 blocked.
    //   II. BPR = 1 corresponds to ring link 1 blocked.
    //   This bit shall be set to 0 on messages transmitted from interconnection
    //   nodes on the subring's Ethernet ring nodes.
    //   If two ring links are blocked, the encoded value can be either value
    if (erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
        new_tx_raps_info.bpr = VTSS_APPL_ERPS_RING_PORT0;
    } else {
        new_tx_raps_info.bpr = erps_state->status.ring_port_status[VTSS_APPL_ERPS_RING_PORT0].blocked ? VTSS_APPL_ERPS_RING_PORT0 : VTSS_APPL_ERPS_RING_PORT1;
    }

    // We should not change the current tx_raps_active if we are transmitting an
    // event. Also, events are always transmitted whether tx_raps_active is
    // false or true.
    if (new_tx_raps_info.request != VTSS_APPL_ERPS_REQUEST_EVENT) {
        // Also update the public Tx info if it has changed
        changed = !erps_state->status.tx_raps_active                   ||
                  cur_tx_raps_info.update_time_secs == 0               ||
                  cur_tx_raps_info.request != new_tx_raps_info.request ||
                  cur_tx_raps_info.rb      != new_tx_raps_info.rb      ||
                  cur_tx_raps_info.dnf     != new_tx_raps_info.dnf     ||
                  cur_tx_raps_info.bpr     != new_tx_raps_info.bpr;

        erps_state->status.tx_raps_active = true;

        if (changed) {
            cur_tx_raps_info.update_time_secs = vtss::uptime_seconds();
            cur_tx_raps_info.request          = new_tx_raps_info.request;
            cur_tx_raps_info.rb               = new_tx_raps_info.rb;
            cur_tx_raps_info.dnf              = new_tx_raps_info.dnf;
            cur_tx_raps_info.bpr              = new_tx_raps_info.bpr;
            // cur_tx_raps_info.node_id and cur_tx_raps_info.smac are not
            // updated by this function.
        }
    } else {
        changed = true;
    }

    // Ask erps_raps.cxx to actually transmit the updated R-APS PDUs.

    if (changed) {
        for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
            erps_state->ring_port_state[ring_port].raps_pdu_transmitted = false;
        }

        erps_raps_tx_info_update(erps_state, new_tx_raps_info);
        ERPS_BASE_expect_raps_rx_update(erps_state);
    }
}

//*****************************************************************************/
// ERPS_BASE_fdb_do_flush()
/******************************************************************************/
static void ERPS_BASE_fdb_do_flush(erps_state_t *erps_state)
{
    vtss_appl_erps_ring_port_t ring_port, ring_port_max;
    mesa_port_no_t             port_no;
    mesa_vid_t                 vid;
    mesa_rc                    rc;

    T_DG(ERPS_TRACE_GRP_BASE, "%u: Flushing FDB", erps_state->inst);

    // We only control the flushing of one ring port if we are an interconnected
    // sub-ring, so don't count to more than port0.
    ring_port_max = erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB ? VTSS_APPL_ERPS_RING_PORT0 : VTSS_APPL_ERPS_RING_PORT1;

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
        // Count the number of flushes. Since we have statistics per ring port
        // count it on both.
        erps_state->status.ring_port_status[ring_port].statistics.flush_cnt++;
    }

    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        if (!VTSS_BF_GET(erps_state->conf.protected_vlans, vid)) {
            continue;
        }

        for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
            port_no = erps_state->ring_port_state[ring_port].port_states[0]->port_no;

            T_RG(ERPS_TRACE_GRP_API, "%u:%s: mesa_mac_table_vlan_port_flush(%u, %u)", erps_state->inst, ring_port, port_no, vid);
            if ((rc = mesa_mac_table_vlan_port_flush(nullptr, port_no, vid)) != VTSS_RC_OK) {
                T_EG(ERPS_TRACE_GRP_API, "%u:%s: mesa_mac_table_vlan_port_flush(%u, %u) failed: %s", erps_state->inst, ring_port, port_no, vid, error_txt(rc));
            }
        }
    }
}

//*****************************************************************************/
// ERPS_BASE_fdb_connected_ring_flush()
/******************************************************************************/
static void ERPS_BASE_fdb_connected_ring_flush(erps_state_t *erps_state)
{
    vtss_appl_erps_raps_info_t tx_raps_info;
    erps_state_t               *connected_ring_erps_state;
    erps_itr_t                 itr;

    if ((itr = ERPS_map.find(erps_state->conf.interconnect_conf.connected_ring_inst)) == ERPS_map.end()) {
        // Connected instance not found. This can also be seen with this
        // instance's status.oper_warning.
        return;
    }

    connected_ring_erps_state = &itr->second;

    if (connected_ring_erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
        // Connected instance is also an interconnected sub-ring. That doesn't
        // work out. This can also be seen with this instance's
        // status.oper_warning.
        return;
    }

    if (connected_ring_erps_state->status.oper_state != VTSS_APPL_ERPS_OPER_STATE_ACTIVE) {
        // Connected instance is not active. This can also be seen with this
        // instance's status.oper_warning.
        return;
    }

    // Flush connected ring's FDB
    T_IG(ERPS_TRACE_GRP_BASE, "%u: Flushing connected ring (%u)", erps_state->inst, connected_ring_erps_state->inst);
    ERPS_BASE_fdb_do_flush(connected_ring_erps_state);

    // Check whether we (interconnected sub-ring) are configured to let the
    // referenced connected ring propagate topology changes as R-APS PDU events.
    if (erps_state->conf.interconnect_conf.tc_propagate) {
        // Tx R-APS EVENT
        memset(&tx_raps_info, 0, sizeof(tx_raps_info));
        tx_raps_info.request = VTSS_APPL_ERPS_REQUEST_EVENT;
        ERPS_BASE_raps_tx_start(connected_ring_erps_state, tx_raps_info);
    }
}

//*****************************************************************************/
// ERPS_BASE_fdb_flush()
/******************************************************************************/
static void ERPS_BASE_fdb_flush(erps_state_t *erps_state)
{
    // Do flush our own FDB
    ERPS_BASE_fdb_do_flush(erps_state);

    // And if we are an interconnected sub-ring, also flush the referenced
    // connected ring.
    if (erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
        if (erps_timer_active(erps_state->tc_timer)) {
            // There is push-back on the times we can ask a connected ring to
            // flush its FDB.
            return;
        }

        ERPS_BASE_fdb_connected_ring_flush(erps_state);

        // Hold-back these notifications to the connected ring for 10 ms
        // (G.8032-2015, clause 10.1.12).
        erps_timer_start(erps_state->tc_timer, 10, false);
    }
}

//*****************************************************************************/
// operator==(erps_base_node_id_bpr_t)
/******************************************************************************/
static bool operator==(erps_base_node_id_bpr_t &lhs, erps_base_node_id_bpr_t &rhs)
{
    // Using operator!= from ip_utils.hxx
    if (lhs.node_id != rhs.node_id) {
        return false;
    }

    return lhs.bpr == rhs.bpr;
}

//*****************************************************************************/
// operator!=(vtss_appl_erps_raps_info_t)
/******************************************************************************/
static bool operator!=(vtss_appl_erps_raps_info_t &lhs, vtss_appl_erps_raps_info_t &rhs)
{
    // We take the shortcut here and assume the update time is always changed
    // when something changes in this structure, so that we don't have to
    // compare the remaining fields.
    if (lhs.update_time_secs != rhs.update_time_secs) {
        return true;
    }

    return false;
}

//*****************************************************************************/
// ERPS_BASE_history_update()
/******************************************************************************/
static void ERPS_BASE_history_update(erps_state_t *erps_state, erps_base_request_t local_request, erps_base_request_t remote_request, erps_base_node_id_bpr_t *rx_node_id_bpr, vtss_appl_erps_ring_port_t rx_ring_port, erps_base_flush_reason_t flush_reason)
{
    erps_base_history_element_t &h                   = erps_state->last_history_element;
    vtss_appl_erps_node_state_t new_node_state       = erps_state->status.node_state;
    bool                        new_port0_sf         = erps_state->status.ring_port_status[VTSS_APPL_ERPS_RING_PORT0].sf;
    bool                        new_port1_sf         = erps_state->status.ring_port_status[VTSS_APPL_ERPS_RING_PORT1].sf;
    bool                        new_port0_blocked    = erps_state->status.ring_port_status[VTSS_APPL_ERPS_RING_PORT0].blocked;
    bool                        new_port1_blocked    = erps_state->status.ring_port_status[VTSS_APPL_ERPS_RING_PORT1].blocked;
    bool                        new_tx_raps_active   = erps_state->status.tx_raps_active;
    bool                        tx_raps_info_changed = erps_state->status.tx_raps_info != h.tx_raps_info;
    char                        str[26];

    if (erps_state->history.size() == 0                   ||
        local_request      != h.local_request             ||
        remote_request     != h.remote_request            ||
        new_node_state     != h.node_state                ||
        new_port0_sf       != h.port0_sf                  ||
        new_port1_sf       != h.port1_sf                  ||
        new_port0_blocked  != h.port0_blocked             ||
        new_port1_blocked  != h.port1_blocked             ||
        new_tx_raps_active != h.tx_raps_active            ||
        flush_reason       != ERPS_BASE_FLUSH_REASON_NONE ||
        tx_raps_info_changed) {

        T_IG(ERPS_TRACE_GRP_HIST, "%4u %-13s %-13s %-5s %-10s %-5s %-5s %-7s %-7s %-6s %s",
             erps_state->inst,
             ERPS_BASE_request_to_str(local_request),
             ERPS_BASE_request_to_str(remote_request),
             erps_util_ring_port_to_str(rx_ring_port),
             erps_util_node_state_to_str(new_node_state),
             ERPS_BASE_bool_to_yes_no(new_port0_sf),
             ERPS_BASE_bool_to_yes_no_dash(erps_state, new_port1_sf),
             ERPS_BASE_bool_to_yes_no(new_port0_blocked),
             ERPS_BASE_bool_to_yes_no_dash(erps_state, new_port1_blocked),
             ERPS_BASE_flush_reason_to_str(flush_reason),
             erps_util_raps_info_to_str(erps_state->status.tx_raps_info, str, erps_state->status.tx_raps_active, true /* include BPR info */));

        h.event_time_ms  = vtss::uptime_milliseconds();
        h.local_request  = local_request;
        h.remote_request = remote_request;
        h.rx_ring_port   = rx_ring_port;
        h.node_state     = new_node_state;
        h.port0_sf       = new_port0_sf;
        h.port1_sf       = new_port1_sf;
        h.port0_blocked  = new_port0_blocked;
        h.port1_blocked  = new_port1_blocked;
        h.flush_reason   = flush_reason;
        h.tx_raps_active = new_tx_raps_active;
        h.tx_raps_info   = erps_state->status.tx_raps_info;

        if (rx_node_id_bpr) {
            h.rx_node_id_bpr = *rx_node_id_bpr;
        } else {
            vtss_clear(h.rx_node_id_bpr);
        }

        erps_state->history.push(h);
    }
}

//*****************************************************************************/
// ERPS_BASE_rx_flush_logic()
/******************************************************************************/
static bool ERPS_BASE_rx_flush_logic(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, vtss_appl_erps_raps_info_t &rx_raps_info)
{
    erps_base_node_id_bpr_t rx_node_id_bpr;
    erps_ring_port_state_t  *ring_port_state          = &erps_state->ring_port_state[ring_port];
    erps_ring_port_state_t  *opposite_ring_port_state = &erps_state->ring_port_state[ERPS_BASE_opposite_ring_port(ring_port)];

    // For details, see G.8032-201708-Cor1, clause 10.1.10 (Flush logic).

    // A R-APS(NR) PDU received by this process does not cause a flush FDB,
    // however, it causes the deletion of the current <node ID, BPR> pair on the
    // receiving ring port. Nonetheless, the received <node ID, BPR> pair is not
    // stored.
    if (rx_raps_info.request == VTSS_APPL_ERPS_REQUEST_NR && !rx_raps_info.rb) {
        vtss_clear(ring_port_state->rx_node_id_bpr);
        return false;
    }

    rx_node_id_bpr.node_id = rx_raps_info.node_id;
    rx_node_id_bpr.bpr     = rx_raps_info.bpr;

    // See operator==(erps_base_node_id_bpr_t)
    if (rx_node_id_bpr == ring_port_state->rx_node_id_bpr) {
        return false;
    }

    // Received <node ID, BPR> pair differs from previous <node ID, BPR>  pair
    // received on this ring port. Delete previous pair and store the newly
    // received pair.
    ring_port_state->rx_node_id_bpr = rx_node_id_bpr;

    // If received R-APS PDU has DNF set, don't flush.
    if (rx_raps_info.dnf) {
        // Done.
        return false;
    }

    // For other ring types only flush if the received <node ID, BPR> pair
    // differs from previous <node ID, BPR> pair received on the other ring port
    if (erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB && !erps_state->conf.virtual_channel) {
        // For interconnected sub-rings without a virtual channel, it's time to
        // flush now.
    } else if (rx_node_id_bpr == opposite_ring_port_state->rx_node_id_bpr) {
        return false;
    }

    // Either an interconnected sub-ring without a virtual channel or the
    // received <node ID, BPR> pair differs from the previous <node ID, BPR>
    // pair stored on the other ring port.
    // We do a flush FDB unless DNF is set (checked above) or node ID equals
    // own node ID  (we won't get into this function in that case).
    // We wish to flush:
    return true;
}

//*****************************************************************************/
// ERPS_BASE_wtr_timer_start()
/******************************************************************************/
static void ERPS_BASE_wtr_timer_start(erps_state_t *erps_state)
{
    erps_timer_start(erps_state->wtr_timer, erps_state->conf.wtr_secs * 1000, false);
}

//*****************************************************************************/
// ERPS_BASE_run_state_machine_init()
// This is invoked only when node_state == VTSS_APPL_ERPS_NODE_STATE_INIT.
/******************************************************************************/
static void ERPS_BASE_run_state_machine_init(erps_state_t *erps_state)
{
    vtss_appl_erps_raps_info_t tx_raps_info;

    switch (erps_state->conf.rpl_mode) {
    case VTSS_APPL_ERPS_RPL_MODE_OWNER:
    case VTSS_APPL_ERPS_RPL_MODE_NEIGHBOR:
        ERPS_BASE_ring_port_block(erps_state, erps_state->conf.rpl_port,                                true, true /* enforce update */);
        ERPS_BASE_ring_port_block(erps_state, ERPS_BASE_opposite_ring_port(erps_state->conf.rpl_port), false, true /* enforce update */);

        if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER && erps_state->conf.revertive) {
            ERPS_BASE_wtr_timer_start(erps_state);
        }

        break;

    case VTSS_APPL_ERPS_RPL_MODE_NONE:
        // Block one ring port and unblock the other.
        // For the sake of interconnected sub-ring w/o virtual channel, we block
        // port0.
        ERPS_BASE_ring_port_block(erps_state, VTSS_APPL_ERPS_RING_PORT0,  true, true /* enforce update */);
        ERPS_BASE_ring_port_block(erps_state, VTSS_APPL_ERPS_RING_PORT1, false, true /* enforce update */);
        break;
    }

    memset(&tx_raps_info, 0, sizeof(tx_raps_info));
    tx_raps_info.request = VTSS_APPL_ERPS_REQUEST_NR;
    ERPS_BASE_raps_tx_start(erps_state, tx_raps_info);

    // Goto E.
    erps_state->status.node_state = VTSS_APPL_ERPS_NODE_STATE_PENDING;

    // Even though it doesn't say in the standard, I think it's a good idea to
    // flush the FDB initially.
    ERPS_BASE_fdb_flush(erps_state);

    ERPS_BASE_history_update(erps_state, ERPS_BASE_REQUEST_NONE, ERPS_BASE_REQUEST_NONE, nullptr, VTSS_APPL_ERPS_RING_PORT0, ERPS_BASE_FLUSH_REASON_FSM);
}

//*****************************************************************************/
// ERPS_BASE_do_run_state_machine()
// This corresponds to R-APS request processing of G.8032, figure 10-1.
/******************************************************************************/
static bool ERPS_BASE_do_run_state_machine(erps_state_t *erps_state, bool remote_node_id_higher, bool do_flush)
{
    vtss_appl_erps_ring_port_t  ring_port, ring_port_max;
    vtss_appl_erps_ring_port_t  rpl_port, non_rpl_port;
    vtss_appl_erps_ring_port_t  cmd_port, non_cmd_port;
    vtss_appl_erps_node_state_t current_state = erps_state->status.node_state, next_state;
    bool                        was_blocked[2], block[2], unblock[2], has_sf[2];
    bool                        tx_raps_start, tx_raps_stop, wtr_start, wtr_stop;
    bool                        wtb_start, wtb_stop, guard_start, fdb_flush;
    vtss_appl_erps_raps_info_t  tx_raps_info;

    // If we are running an interconnected sub-ring w/o virtual channel, we
    // don't use port1. In all other cases, we use port1, but in the case of an
    // interconnected sub-ring w/ virtual channel only to be able to forward
    // frames between port0 and the virtual channel (ERPS_BASE_ace_update()).
    ring_port_max = erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB  && !erps_state->conf.virtual_channel ? VTSS_APPL_ERPS_RING_PORT0 : VTSS_APPL_ERPS_RING_PORT1;

    // Before we start on the real SM, get some commonly used params of our
    // current state.
    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        was_blocked[ring_port] = erps_state->status.ring_port_status[ring_port].blocked;
        has_sf[ring_port]      = erps_state->status.ring_port_status[ring_port].sf;
        block[ring_port]       = false;
        unblock[ring_port]     = false;
    }

    tx_raps_start = false;
    tx_raps_stop  = false;
    wtr_start     = false;
    wtr_stop      = false;
    wtb_start     = false;
    wtb_stop      = false;
    guard_start   = false;
    fdb_flush     = false;
    rpl_port      = erps_state->conf.rpl_port;
    non_rpl_port  = ERPS_BASE_opposite_ring_port(rpl_port);
    next_state    = current_state;

    // FS|MS-to-port0 means: Block port1, unblock port0.
    // cmd_port is the port to block.
    cmd_port      = erps_state->command == VTSS_APPL_ERPS_COMMAND_FS_TO_PORT0 || erps_state->command == VTSS_APPL_ERPS_COMMAND_MS_TO_PORT0 ? VTSS_APPL_ERPS_RING_PORT1 : VTSS_APPL_ERPS_RING_PORT0;
    non_cmd_port  = ERPS_BASE_opposite_ring_port(cmd_port);

    memset(&tx_raps_info, 0, sizeof(tx_raps_info));

    // Node states:
    // - == Init
    // A == Idle
    // B == Protection
    // C == MS
    // D == FS
    // E == Pending

    // G.8032, Table 10-2, State Machine.
    // The "Row X" numbers indicate the "Row" column in G.8032, Table 10-2.
    switch (erps_state->top_prio_request) {
    case ERPS_BASE_REQUEST_LOCAL_CLEAR:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_MS ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_FS) {
            // Row 30 (C) and Row 44 (D)
            // If any ring port blocked
            //   Start Guard Timer
            //   Tx R-APS(NR)
            //   If RPL Owner node and Revertive mode:
            //     Start WTB
            //
            // Goto E
            next_state = VTSS_APPL_ERPS_NODE_STATE_PENDING;

            for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
                if (was_blocked[ring_port]) {
                    break;
                }
            }

            if (ring_port > ring_port_max) {
                // No ring port blocked
                break;
            }

            tx_raps_info.request = VTSS_APPL_ERPS_REQUEST_NR;
            tx_raps_start        = true;

            if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER && erps_state->conf.revertive) {
                wtb_start = true;
            }
        } else if (current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING) {
            // Row 58 (E)
            // If RPL owner node:
            //   Stop WTR
            //   Stop WTB
            //   If RPL port is blocked
            //     Tx R-APS(NR, RB, DNF)
            //     Unblock non-RPL port
            //   Else
            //     Block RPL port
            //     Tx R-APS(NR, RB)
            //     Unblock non-RPL port
            //     Flush FDB
            //   End
            // End
            //
            // Goto A
            next_state = VTSS_APPL_ERPS_NODE_STATE_IDLE;

            if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER) {
                wtr_stop              = true;
                wtb_stop              = true;
                tx_raps_info.request  = VTSS_APPL_ERPS_REQUEST_NR;
                tx_raps_info.rb       = true;
                tx_raps_start         = true;
                block[rpl_port]       = true;
                unblock[non_rpl_port] = true;

                if (was_blocked[rpl_port]) {
                    tx_raps_info.dnf = true;
                } else {
                    fdb_flush = true;
                }
            }
        } else {
            // Row 2 (A) and Row 16 (B)
            // No action, stay in A or B.
        }

        break;

    case ERPS_BASE_REQUEST_LOCAL_FS:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_IDLE       ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_PROTECTION ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_MS         ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING) {
            // Row 3 (A), Row 17 (B), Row 31 (C), and Row 59 (E)
            // If requested ring port is already blocked:
            //   Tx R-APS(FS, DNF)
            //   Unblock non-requested ring port
            // Else
            //   Block requested ring port
            //   Tx R-APS(FS)
            //   Unblock non-requested ring port
            //   Flush FDB
            // End
            //
            // If current-state is pending and RPL owner node (Row 59)
            //   Stop WTR
            //   Stop WTB
            // End
            //
            // Goto D
            next_state = VTSS_APPL_ERPS_NODE_STATE_FS;

            tx_raps_info.request  = VTSS_APPL_ERPS_REQUEST_FS;
            tx_raps_start         = true;
            block[cmd_port]       = true;
            unblock[non_cmd_port] = true;

            if (was_blocked[cmd_port]) {
                tx_raps_info.dnf = true;
            } else {
                fdb_flush = true;
            }

            if (current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING && erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER) {
                wtr_stop = true;
                wtb_stop = true;
            }
        } else if (current_state == VTSS_APPL_ERPS_NODE_STATE_FS) {
            // Row 45 (D)
            // Block requested ring port
            // Tx R-APS(FS)
            // Flush FDB
            //
            // Stay in D
            next_state = VTSS_APPL_ERPS_NODE_STATE_FS;

            block[cmd_port]      = true;
            tx_raps_info.request = VTSS_APPL_ERPS_REQUEST_FS;
            tx_raps_start        = true;
            fdb_flush            = true;
        }

        break;

    case ERPS_BASE_REQUEST_REMOTE_FS:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_IDLE       ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_PROTECTION ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_MS         ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING) {
            // Row 4 (A), Row 18 (B), Row 32 (C), and Row 60 (E)
            // Unblock ring ports
            // Stop Tx R-APS
            //
            // If current-state is pending and RPL owner node (Row 60)
            //   Stop WTR
            //   Stop WTB
            // End
            //
            // Goto D
            next_state = VTSS_APPL_ERPS_NODE_STATE_FS;

            unblock[VTSS_APPL_ERPS_RING_PORT0] = true;
            unblock[VTSS_APPL_ERPS_RING_PORT1] = true;
            tx_raps_stop                       = true;

            if (current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING && erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER) {
                wtr_stop = true;
                wtb_stop = true;
            }
        } else {
            // Row 46 (D)
            // No action, stay in D.
        }

        break;

    case ERPS_BASE_REQUEST_LOCAL_SF:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_IDLE       ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_PROTECTION ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_MS         ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING) {
            // Row 5 (A), Row 19 (B), Row 33 (C), and Row 61 (E)
            // If failed ring port is already blocked:
            //   Tx R-APS(SF, DNF)
            //   Unblock non-failed ring port
            // Else
            //   Block failed ring port
            //   Tx R-APS(SF)
            //   Unblock non-failed ring port
            //   Flush FDB
            // End
            //
            // If current-state is pending and RPL owner node (Row 61)
            //   Stop WTR
            //   Stop WTB
            // End
            //
            // Goto B
            next_state = VTSS_APPL_ERPS_NODE_STATE_PROTECTION;

            // As it so happens, one or both ring ports may be blocked. This
            // piece of code should take care of both cases.
            for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
                block  [ring_port] =  has_sf[ring_port];
                unblock[ring_port] = !has_sf[ring_port];

                if (has_sf[ring_port] && !was_blocked[ring_port]) {
                    fdb_flush = true;
                }
            }

            tx_raps_info.request = VTSS_APPL_ERPS_REQUEST_SF;
            tx_raps_info.dnf     = !fdb_flush;
            tx_raps_start        = true;

            if (current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING && erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER) {
                wtr_stop = true;
                wtb_stop = true;
            }
        } else {
            // Row 47 (D)
            // No action, stay in D.
        }

        break;

    case ERPS_BASE_REQUEST_LOCAL_SF_CLEAR:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_PROTECTION) {
            // Row 20 (B)
            // Start Guard Timer
            // Tx R-APS(NR)
            // If RPL owner node and revertive mode
            //   Start WTR
            // End
            //
            // Goto E
            next_state = VTSS_APPL_ERPS_NODE_STATE_PENDING;

            guard_start          = true;
            tx_raps_info.request = VTSS_APPL_ERPS_REQUEST_NR;
            tx_raps_start        = true;

            if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER && erps_state->conf.revertive) {
                wtr_start = true;
            }
        } else {
            // Row 6 (A), Row 34 (C), Row 48 (D), and Row 62 (E):
            // No Action, stay in A, C, D, or E.
        }

        break;

    case ERPS_BASE_REQUEST_REMOTE_SF:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_IDLE ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_MS   ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING) {
            // Row 7 (A), Row 35 (C), and Row 63 (E)
            // Unblock non-failed ring port
            // Stop Tx R-APS
            //
            // If current-state is pending and RPL owner node (Row 63)
            //   Stop WTR
            //   Stop WTB
            // End
            //
            // Goto B
            next_state = VTSS_APPL_ERPS_NODE_STATE_PROTECTION;

            // As it so happens, one or both ring ports may be blocked. This
            // piece of code should take care of both cases.
            for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
                unblock[ring_port] = !has_sf[ring_port];
            }

            tx_raps_stop = true;

            if (current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING && erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER) {
                wtr_stop = true;
                wtb_stop = true;
            }
        } else {
            // Row 21 (B) and Row 49 (D)
            // No action, stay in B or D.
        }

        break;

    case ERPS_BASE_REQUEST_REMOTE_MS:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_IDLE ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING) {
            // Row 8 (A) and Row 64 (E)
            // Unblock non-failed ring port
            // Stop Tx R-APS
            //
            // If current-state is pending and RPL owner node (Row 64)
            //    Stop WTR
            //    Stop WTB
            // End
            //
            // Goto C
            next_state = VTSS_APPL_ERPS_NODE_STATE_MS;

            // As it so happens, one or both ring ports may be blocked. This
            // piece of code should take care of both cases.
            for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
                unblock[ring_port] = !has_sf[ring_port];
            }

            tx_raps_stop = true;

            if (current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING && erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER) {
                wtr_stop = true;
                wtb_stop = true;
            }
        } else if (current_state == VTSS_APPL_ERPS_NODE_STATE_MS) {
            // Row 36 (C)
            // If any ring port blocked
            //   Start guard timer
            //   Tx R-APS(NR)
            //   If RPL owner node and revertive mode
            //     Start WTB
            //   End
            // End
            //
            // Goto C if both ring ports are unblocked, E if not.

            for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
                if (was_blocked[ring_port]) {
                    break;
                }
            }

            if (ring_port <= ring_port_max) {
                // At least one ring port is currently blocked. Goto E
                next_state = VTSS_APPL_ERPS_NODE_STATE_PENDING;

                guard_start          = true;
                tx_raps_info.request = VTSS_APPL_ERPS_REQUEST_NR;
                tx_raps_start        = true;

                if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER && erps_state->conf.revertive) {
                    wtb_start = true;
                }
            } else {
                // No ring ports are blocked. Goto C
                next_state = VTSS_APPL_ERPS_NODE_STATE_MS;
            }
        } else {
            // Row 22 (B) and Row 50 (D)
            // No action, stay in B or D.
        }

        break;

    case ERPS_BASE_REQUEST_LOCAL_MS:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_IDLE ||
            current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING) {
            // Row 9 (A) and Row 65 (E)
            // If requested ring port is already blocked
            //   Tx R-APS(MS, DNF)
            //   Unblock non-requested ring port
            // Else
            //   Block requested ring port
            //   Tx R-APS(MS)
            //   Unblock non-requested ring port
            //   Flush FDB
            // End
            //
            // If current-state is pending and RPL owner node (Row 65)
            //   Stop WTR
            //   Stop WTB
            // End
            //
            // Goto C
            next_state = VTSS_APPL_ERPS_NODE_STATE_MS;

            tx_raps_info.request  = VTSS_APPL_ERPS_REQUEST_MS;
            tx_raps_start         = true;
            block[cmd_port]       = true;
            unblock[non_cmd_port] = true;

            if (was_blocked[cmd_port]) {
                tx_raps_info.dnf = true;
            } else {
                fdb_flush = true;
            }

            if (current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING && erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER) {
                wtr_stop = true;
                wtb_stop = true;
            }
        } else {
            // Row 23 (B), Row 37 (C), and Row 51 (D)
            // No action, stay in B, C or E.
        }

        break;

    case ERPS_BASE_REQUEST_LOCAL_WTR_EXPIRES:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING) {
            // Row 66 (E)
            // If RPL owner node
            //   Stop WTB
            //   If RPL port is blocked
            //     Tx R-APS(NR, RB, DNF)
            //     Unblock non-RPL port
            //   Else
            //     Block RPL port
            //     Tx R-APS(NR, RB)
            //     Unblock non-RPL port
            //     Flush FDB
            //   End
            // End
            //
            // Goto A
            next_state = VTSS_APPL_ERPS_NODE_STATE_IDLE;

            if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER) {
                wtb_stop              = true;
                tx_raps_info.request  = VTSS_APPL_ERPS_REQUEST_NR;
                tx_raps_info.rb       = true;
                tx_raps_start         = true;
                block[rpl_port]       = true;
                unblock[non_rpl_port] = true;

                if (was_blocked[rpl_port]) {
                    tx_raps_info.dnf = true;
                } else {
                    fdb_flush = true;
                }
            }
        } else {
            // Row 10 (A), Row 24 (B), Row 38 (C), and Row 52 (D)
            // No action, stay in A, B, C, or D.
        }

        break;

    case ERPS_BASE_REQUEST_LOCAL_WTR_RUNNING:
        // Row 11 (A), Row 25 (B), Row 39 (C), Row 53 (D), Row 67 (E)
        // No action, stay in A, B, C, D, or E.
        // So this state is just here in order to prevent any lower-priority
        // events from taking us out of this state.
        break;

    case ERPS_BASE_REQUEST_LOCAL_WTB_EXPIRES:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING) {
            // Row 68 (E)
            // If RPL owner node
            //   Stop WTR
            //   If RPL port is blocked
            //     Tx R-APS(NR, RB, DNF)
            //     Unblock non-RPL port
            //   Else
            //     Block RPL port
            //     Tx R-APS(NR, RB)
            //     Unblock non-RPL port
            //     Flush FDB
            //   End
            // End
            //
            // Goto A
            next_state = VTSS_APPL_ERPS_NODE_STATE_IDLE;

            if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER) {
                wtr_stop              = true;
                tx_raps_info.request  = VTSS_APPL_ERPS_REQUEST_NR;
                tx_raps_info.rb       = true;
                tx_raps_start         = true;
                block[rpl_port]       = true;
                unblock[non_rpl_port] = true;

                if (was_blocked[rpl_port]) {
                    tx_raps_info.dnf = true;
                } else {
                    fdb_flush = true;
                }
            }
        } else {
            // Row 12 (A), Row 26 (B), Row 40 (C), and Row 54 (D)
            // No action, stay in A, B, C, or D.
        }

        break;

    case ERPS_BASE_REQUEST_LOCAL_WTB_RUNNING:
        // Row 13 (A), Row 27 (B), Row 41 (C), Row 55 (D), and Row 69 (E)
        // No action, stay in A, B, C, D, or E.
        // So this state is just here in order to prevent any lower-priority
        // events from taking us out of this state.
        break;

    case ERPS_BASE_REQUEST_REMOTE_NR_RB:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_IDLE) {
            // Row 14 (A)
            // Unblock non-RPL port
            // If Not RPL owner node
            //   Stop Tx R-APS
            // End
            //
            // Goto A
            next_state = VTSS_APPL_ERPS_NODE_STATE_IDLE;

            if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_NONE) {
                for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
                    unblock[ring_port] = true;
                }
            } else {
                unblock[non_rpl_port] = true;
            }

            if (erps_state->conf.rpl_mode != VTSS_APPL_ERPS_RPL_MODE_OWNER) {
                tx_raps_stop = true;
            }
        } else if (current_state == VTSS_APPL_ERPS_NODE_STATE_PENDING) {
            // Row 70 (E)
            // If RPL owner node
            //   Stop WTR
            //   Stop WTB
            // End
            // If neither RPL owner node nor RPL neighbor node:
            //   Unblock ring ports
            //   Stop Tx R-APS
            // End
            // If RPL neighbor node
            //   Block RPL port
            //   Unblock non-RPL port
            //   Stop Tx R-APS
            // End
            //
            // Goto A
            next_state = VTSS_APPL_ERPS_NODE_STATE_IDLE;

            if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER) {
                wtr_stop = true;
                wtb_stop = true;
            } else if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_NEIGHBOR) {
                block[rpl_port]       = true;
                unblock[non_rpl_port] = true;
                tx_raps_stop          = true;
            } else {
                for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
                    unblock[ring_port] = true;
                }

                tx_raps_stop = true;
            }
        } else {
            // Row 28 (B), Row 42 (C), Row 56 (D)
            // No action, but goto E
            next_state = VTSS_APPL_ERPS_NODE_STATE_PENDING;
        }

        break;

    case ERPS_BASE_REQUEST_REMOTE_NR:
        if (current_state == VTSS_APPL_ERPS_NODE_STATE_IDLE) {
            // Row 15 (A)
            // If neither RPL owner node nor RPL neighbor node and remote node ID is higher than own node ID
            //   Unblock non-failed ring port
            //   Stop Tx R-APS
            // End
            //
            // Goto A
            next_state = VTSS_APPL_ERPS_NODE_STATE_IDLE;

            if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_NONE && remote_node_id_higher) {
                for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
                    unblock[ring_port] = !has_sf[ring_port];
                }

                tx_raps_stop = true;
            }
        } else if (current_state == VTSS_APPL_ERPS_NODE_STATE_PROTECTION) {
            // Row 29 (B)
            // If RPL owner node and revertive mode
            //   Start WTR
            // End
            //
            // Goto E
            next_state = VTSS_APPL_ERPS_NODE_STATE_PENDING;

            if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER && erps_state->conf.revertive) {
                wtr_start = true;
            }
        } else if (current_state == VTSS_APPL_ERPS_NODE_STATE_MS ||
                   current_state == VTSS_APPL_ERPS_NODE_STATE_FS) {
            // Row 43 (C) and Row 57 (D)
            // If RPL owner node and revertive mode
            //   Start WTB
            // End
            //
            // Goto E
            next_state = VTSS_APPL_ERPS_NODE_STATE_PENDING;

            if (erps_state->conf.rpl_mode == VTSS_APPL_ERPS_RPL_MODE_OWNER && erps_state->conf.revertive) {
                wtb_start = true;
            }
        } else {
            // Row 71 (E)
            // If remote node ID is higher than own node ID
            //   Unblock non-failed ring port
            //   Stop Tx R-APS
            // End
            //
            // Goto E
            next_state = VTSS_APPL_ERPS_NODE_STATE_PENDING;

            if (remote_node_id_higher) {
                for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
                    unblock[ring_port] = !has_sf[ring_port];
                }

                tx_raps_stop = true;
            }
        }

        break;

    case ERPS_BASE_REQUEST_NONE:
        // No specific reason for calling this FSM => no action
        break;

    default:
        T_EG(ERPS_TRACE_GRP_BASE, "%u: Invalid top priority request (%d)", erps_state->inst, erps_state->top_prio_request);
        return false;
    }

    erps_state->status.node_state = next_state;

    // Ring ports are always blocked before they are unblocked.
    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        if (block[ring_port]) {
            if (unblock[ring_port]) {
                T_EG(ERPS_TRACE_GRP_BASE, "%u: Requested to block and unblock ring port %s simultaneously (came from %s with top-prio-request = %s)", erps_state->inst, ring_port, erps_util_node_state_to_str(current_state), ERPS_BASE_request_to_str(erps_state->top_prio_request));
            }

            ERPS_BASE_ring_port_block(erps_state, ring_port, true);
        }
    }

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        if (unblock[ring_port]) {
            ERPS_BASE_ring_port_block(erps_state, ring_port, false);
        }
    }

    if (wtr_start && wtr_stop) {
        T_EG(ERPS_TRACE_GRP_BASE, "%u: wtr_start and wtr_stop are both true at the same time (came from %s with top-prio-request = %s)", erps_state->inst, erps_util_node_state_to_str(current_state), ERPS_BASE_request_to_str(erps_state->top_prio_request));
    }

    if (wtr_start) {
        // G.8032, clause 10.1.2, bullet j says:
        // If the WTR timer is already running, no action is taken.
        if (!erps_timer_active(erps_state->wtr_timer)) {
            ERPS_BASE_wtr_timer_start(erps_state);
        }
    } else if (wtr_stop) {
        erps_timer_stop(erps_state->wtr_timer);
    }

    if (wtb_start && wtb_stop) {
        T_EG(ERPS_TRACE_GRP_BASE, "%u: wtb_start and wtb_stop are both true at the same time (came from %s with top-prio request = %s)", erps_state->inst, erps_util_node_state_to_str(current_state), ERPS_BASE_request_to_str(erps_state->top_prio_request));
    }

    if (wtb_start) {
        // G.8032, clause 10.1.2, bullet l says:
        // If the WTB timer is already running, no action is taken.
        if (!erps_timer_active(erps_state->wtb_timer)) {
            // G.8032, clause 10.1.4, bullet b says:
            // The WTB timer is defined to be 5 seconds longer than the guard
            // timer.
            erps_timer_start(erps_state->wtb_timer, 5000 + erps_state->conf.guard_time_msecs, false);
        }
    } else if (wtb_stop) {
        erps_timer_stop(erps_state->wtb_timer);
    }

    if (guard_start) {
        // G.8032, clause 10.1.2, bullet n says:
        // Start the guard timer, that is, do not leave it running if already
        // running, like the WTR and WTB timers.
        erps_timer_start(erps_state->guard_timer, erps_state->conf.guard_time_msecs, false);
    }

    if (tx_raps_start && tx_raps_stop) {
        T_EG(ERPS_TRACE_GRP_BASE, "%u: tx_raps_start and tx_raps_stop are both true at the same time (came from %s with top-prio request = %s", erps_state->inst, erps_util_node_state_to_str(current_state), ERPS_BASE_request_to_str(erps_state->top_prio_request));
    }

    if (tx_raps_start) {
        ERPS_BASE_raps_tx_start(erps_state, tx_raps_info);
    } else if (tx_raps_stop) {
        ERPS_BASE_raps_tx_stop(erps_state);
    }

    if (do_flush || fdb_flush) {
        ERPS_BASE_fdb_flush(erps_state);
    }

    erps_state->status.node_state = next_state;

    return fdb_flush;
}

//*****************************************************************************/
// ERPS_BASE_local_request_get()
// Returns this highest priority local request given the current state.
/******************************************************************************/
static erps_base_request_t ERPS_BASE_local_request_get(erps_state_t *erps_state)
{
    vtss_appl_erps_ring_port_t ring_port, ring_port_max;

    // We only use port1 if we are not an interconnected sub-ring.
    ring_port_max = erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB ? VTSS_APPL_ERPS_RING_PORT0 : VTSS_APPL_ERPS_RING_PORT1;

    if (erps_state->command == VTSS_APPL_ERPS_COMMAND_CLEAR) {
        return ERPS_BASE_REQUEST_LOCAL_CLEAR;
    }

    if (erps_state->command == VTSS_APPL_ERPS_COMMAND_FS_TO_PORT0 ||
        erps_state->command == VTSS_APPL_ERPS_COMMAND_FS_TO_PORT1) {
        return ERPS_BASE_REQUEST_LOCAL_FS;
    }

    // G.8032, Table 10-1, note a): If an Ethernet ring node is in the FS state,
    // local SF is ignored.
    if (erps_state->status.node_state != VTSS_APPL_ERPS_NODE_STATE_FS) {
        // Check for SF on either active ring port (this must be the hold-off'ed
        // version from public status).
        for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
            if (erps_state->status.ring_port_status[ring_port].sf) {
                return ERPS_BASE_REQUEST_LOCAL_SF;
            }
        }
    }

    // Check for SF->OK transition on either ring port
    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= ring_port_max; ring_port++) {
        if (erps_state->ring_port_state[ring_port].sf_clearing) {
            return ERPS_BASE_REQUEST_LOCAL_SF_CLEAR;
        }
    }

    if (erps_state->command == VTSS_APPL_ERPS_COMMAND_MS_TO_PORT0 ||
        erps_state->command == VTSS_APPL_ERPS_COMMAND_MS_TO_PORT1) {
        return ERPS_BASE_REQUEST_LOCAL_MS;
    }

    if (erps_state->wtr_event) {
        return ERPS_BASE_REQUEST_LOCAL_WTR_EXPIRES;
    }

    if (erps_timer_active(erps_state->wtr_timer)) {
        return ERPS_BASE_REQUEST_LOCAL_WTR_RUNNING;
    }

    if (erps_state->wtb_event) {
        return ERPS_BASE_REQUEST_LOCAL_WTB_EXPIRES;
    }

    if (erps_timer_active(erps_state->wtb_timer)) {
        return ERPS_BASE_REQUEST_LOCAL_WTB_RUNNING;
    }

    return ERPS_BASE_REQUEST_NONE;
}

//*****************************************************************************/
// ERPS_BASE_remote_request_get()
/******************************************************************************/
static erps_base_request_t ERPS_BASE_remote_request_get(erps_state_t *erps_state, vtss_appl_erps_raps_info_t *rx_raps_info, bool &remote_node_id_higher, erps_base_node_id_bpr_t &rx_node_id_bpr)
{
    remote_node_id_higher = false;
    vtss_clear(rx_node_id_bpr);

    if (rx_raps_info == nullptr) {
        // No remote request
        return ERPS_BASE_REQUEST_NONE;
    }

    // Utilize operator< from ip_utils.hxx
    remote_node_id_higher = erps_state->status.tx_raps_info.node_id < rx_raps_info->node_id;

    rx_node_id_bpr.node_id = rx_raps_info->node_id;
    rx_node_id_bpr.bpr     = rx_raps_info->bpr;

    switch (rx_raps_info->request) {
    case VTSS_APPL_ERPS_REQUEST_NR:
        return rx_raps_info->rb ? ERPS_BASE_REQUEST_REMOTE_NR_RB : ERPS_BASE_REQUEST_REMOTE_NR;

    case VTSS_APPL_ERPS_REQUEST_MS:
        return ERPS_BASE_REQUEST_REMOTE_MS;

    case VTSS_APPL_ERPS_REQUEST_SF:
        return ERPS_BASE_REQUEST_REMOTE_SF;

    case VTSS_APPL_ERPS_REQUEST_FS:
        return ERPS_BASE_REQUEST_REMOTE_FS;

    case VTSS_APPL_ERPS_REQUEST_EVENT:
    default:
        // Unknown requests and VTSS_APPL_ERPS_REQUEST_EVENT must have been
        // filtered out by now by erps_raps.cxx and erps_base_rx_frame().
        T_EG(ERPS_TRACE_GRP_BASE, "%u: Unknown R-APS request (%d)", erps_state->inst, rx_raps_info->request);
        break;
    }

    return ERPS_BASE_REQUEST_NONE;
}

//*****************************************************************************/
// ERPS_BASE_run_state_machine()
// This corresponds to Local priority logic, Priority logic and R-APS request
// processing of G.8032, Figure 10-1.
//
// rx_raps_info is only non-NULL when this function is invoked by a R-APS PDU
// Rx (from erps_base_rx_frame()).
/******************************************************************************/
static void ERPS_BASE_run_state_machine(erps_state_t *erps_state, vtss_appl_erps_ring_port_t rx_ring_port = VTSS_APPL_ERPS_RING_PORT0, vtss_appl_erps_raps_info_t *rx_raps_info = nullptr, bool do_flush = false)
{
    erps_base_request_t         local_request, remote_request;
    vtss_appl_erps_ring_port_t  ring_port;
    vtss_appl_erps_node_state_t old_state = erps_state->status.node_state;
    bool                        remote_node_id_higher, clr_command, fdb_was_fsm_flushed;
    erps_base_node_id_bpr_t     rx_node_id_bpr;

    ERPS_LOCK_ASSERT_LOCKED("%u", erps_state->inst);

    if (old_state == VTSS_APPL_ERPS_NODE_STATE_INIT) {
        ERPS_BASE_run_state_machine_init(erps_state);
        old_state = erps_state->status.node_state;

        // Continue with whatever was requested
    }

    local_request  = ERPS_BASE_local_request_get(erps_state);
    remote_request = ERPS_BASE_remote_request_get(erps_state, rx_raps_info, remote_node_id_higher, rx_node_id_bpr);

    // G.8032, clause 10.1.1 states the following:
    //   Received R-APS request/state and status are not stored in this process.
    //   As a result, after the change of a local request, R-APS request/state
    //   and status received previously are not taken into consideration for the
    //   definition of the new top priority request.
    // This means, that we only calculate a new remote request if this SM
    // invocation is due to a received R-APS PDU (rx_raps_info != nullptr).
    erps_state->top_prio_request = MAX(local_request, remote_request);

    // Clear various flags
    erps_state->wtr_event = false;
    erps_state->wtb_event = false;
    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        erps_state->ring_port_state[ring_port].sf_clearing = false;
    }

    // Also clear control commands if the new request has higher priority than
    // the command, according to G.8032, clause 10.1.1, first paragraph just
    // below Table 10-1).
    clr_command = false;
    switch (erps_state->command) {
    case VTSS_APPL_ERPS_COMMAND_NR:
        // Already cleared
        break;

    case VTSS_APPL_ERPS_COMMAND_FS_TO_PORT0:
    case VTSS_APPL_ERPS_COMMAND_FS_TO_PORT1:
        clr_command = erps_state->top_prio_request > ERPS_BASE_REQUEST_LOCAL_FS;
        break;

    case VTSS_APPL_ERPS_COMMAND_MS_TO_PORT0:
    case VTSS_APPL_ERPS_COMMAND_MS_TO_PORT1:
        clr_command = erps_state->top_prio_request > ERPS_BASE_REQUEST_LOCAL_MS;
        break;

    case VTSS_APPL_ERPS_COMMAND_CLEAR:
        // Always clear a clear
        clr_command = true;
        break;

    default:
        T_EG(ERPS_TRACE_GRP_BASE, "%u: Invalid command (%d)", erps_state->inst, erps_state->command);
        clr_command = true;
        break;
    }

    if (clr_command) {
        T_IG(ERPS_TRACE_GRP_BASE, "%u: Clearing command %s", erps_state->inst, erps_util_command_to_str(erps_state->command));
        erps_state->command = VTSS_APPL_ERPS_COMMAND_NR;
    }

    fdb_was_fsm_flushed = ERPS_BASE_do_run_state_machine(erps_state, remote_node_id_higher, do_flush);

    T_IG(ERPS_TRACE_GRP_BASE, "%u: local-request = %s, remote-request = %s => state: %s->%s",
         erps_state->inst,
         ERPS_BASE_request_to_str(local_request),
         ERPS_BASE_request_to_str(remote_request),
         erps_util_node_state_to_str(old_state),
         erps_util_node_state_to_str(erps_state->status.node_state));

    ERPS_BASE_history_update(erps_state, local_request, remote_request, rx_raps_info ? &rx_node_id_bpr : nullptr, rx_ring_port, fdb_was_fsm_flushed ? ERPS_BASE_FLUSH_REASON_FSM : do_flush ? ERPS_BASE_FLUSH_REASON_NODE_ID : ERPS_BASE_FLUSH_REASON_NONE);
}

//*****************************************************************************/
// ERPS_BASE_wtr_timeout()
// Fires when wtr_timer expires.
/******************************************************************************/
static void ERPS_BASE_wtr_timeout(erps_timer_t &timer, void *context)
{
    erps_state_t *erps_state = (erps_state_t *)context;

    VTSS_ASSERT(erps_state);

    erps_state->wtr_event = true;
    ERPS_BASE_run_state_machine(erps_state);
}

//*****************************************************************************/
// ERPS_BASE_guard_timeout()
// Fires when guard_timer expires.
/******************************************************************************/
static void ERPS_BASE_guard_timeout(erps_timer_t &timer, void *context)
{
    // When the guard timer expires, the SM starts to receive R-APS PDUs again,
    // so nothing else to do here.
}

//*****************************************************************************/
// ERPS_BASE_tc_timeout()
// Fires when tc_timer expires.
/******************************************************************************/
static void ERPS_BASE_tc_timeout(erps_timer_t &timer, void *context)
{
    // When the TC (Topology Change) timer expires, the SM may send Topology
    // Change Notifications to the connected ring again, so nothing else to do
    // here.
}

//*****************************************************************************/
// ERPS_BASE_wtb_timeout()
// Fires when wtb_timer expires.
/******************************************************************************/
static void ERPS_BASE_wtb_timeout(erps_timer_t &timer, void *context)
{
    erps_state_t *erps_state = (erps_state_t *)context;

    VTSS_ASSERT(erps_state);

    erps_state->wtb_event = true;
    ERPS_BASE_run_state_machine(erps_state);
}

//*****************************************************************************/
// ERPS_BASE_hoff_timeout()
// Fires when a ring-port hoff_timer expires
/******************************************************************************/
static void ERPS_BASE_hoff_timeout(erps_timer_t &timer, void *context)
{
    erps_state_t                      *erps_state = (erps_state_t *)context;
    vtss_appl_erps_ring_port_t        ring_port;
    bool                              new_sf, old_sf;
    vtss_appl_erps_ring_port_status_t *ring_port_status;
    erps_ring_port_state_t            *ring_port_state;

    VTSS_ASSERT(erps_state);

    // Figure out which ring port this hoff-timer belongs to
    ring_port = &timer == &erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT0].hoff_timer ? VTSS_APPL_ERPS_RING_PORT0 : VTSS_APPL_ERPS_RING_PORT1;

    ring_port_status = &erps_state->status.ring_port_status[ring_port];
    ring_port_state  = &erps_state->ring_port_state[ring_port];

    old_sf = ring_port_status->sf;
    new_sf = ring_port_state->sf;

    if (new_sf > old_sf) {
        // Hold-off timeout can only go from less severe to more severe defects.
        // The opposite direction is handled directly in erps_base_sf_set().
        ring_port_status->sf = new_sf;
        ERPS_BASE_run_state_machine(erps_state);
    }
}

/******************************************************************************/
// ERPS_BASE_rx_timeout()
// Timer == rx_timer
/******************************************************************************/
static void ERPS_BASE_rx_timeout(erps_timer_t &timer, void *context)
{
    erps_state_t *erps_state = (erps_state_t *)context;

    VTSS_ASSERT(erps_state);

    // We haven't received a R-APS PDU on any ring port for the last 17.5
    // seconds,  so it's time to report a dFOP-TO, which may result in a
    // cFOP_TO.
    ERPS_BASE_dfop_to_set(erps_state, true);
}

//*****************************************************************************/
// ERPS_BASE_pm_timeout()
// Timer = per-ring-port pm_timer
/******************************************************************************/
static void ERPS_BASE_pm_timeout(erps_timer_t &timer, void *context)
{
    erps_state_t               *erps_state = (erps_state_t *)context;
    vtss_appl_erps_ring_port_t ring_port;

    VTSS_ASSERT(erps_state);

    // Figure out which ring port this timer belongs to
    ring_port = &timer == &erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT0].pm_timer ? VTSS_APPL_ERPS_RING_PORT0 : VTSS_APPL_ERPS_RING_PORT1;

    // We haven't received a provisioning-mismatch R-APS PDU for the last 17.5
    // seconds, so we can clear cFOP-PM.
    erps_state->status.ring_port_status[ring_port].cFOP_PM = false;
}

//*****************************************************************************/
// ERPS_BASE_cfop_pm_check_failed()
/******************************************************************************/
static bool ERPS_BASE_cfop_pm_check_failed(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, vtss_appl_erps_raps_info_t &rx_raps_info)
{
    // Update cFOP_PM.
    // According to G.8021, clause 6.1.4.3.1 (and to some extent also G.8032,
    // clause 10.4), dFOP-PM must be set if we:
    //    Are RPL owner AND
    //    Rx R-APS PDU is a NR, RB frame AND
    //    Rx R-APS PDU's node ID is different from ours.
    //
    // The ITU recs say that this is a status for this ring instance, and not
    // for this ring port on this ring instance. However, we improve the
    // recommendations so that it becomes a status per ring port per ring
    // instance.
    if (erps_state->conf.rpl_mode != VTSS_APPL_ERPS_RPL_MODE_OWNER) {
        // We are not RPL owner
        return false;
    }

    if (rx_raps_info.request != VTSS_APPL_ERPS_REQUEST_NR) {
        // It's not an NR frame
        return false;
    }

    if (!rx_raps_info.rb) {
        // It's not an RB frame
        return false;
    }

    // It has already been checked by the caller of us that this R-APS PDU is
    // not sent by ourselves.

    // Bummer.
    // Set the defect and (re-)start the PM timer with a timeout of 17.5
    // seconds, which is the same as a R-APS PDU Rx timeout
    erps_state->status.ring_port_status[ring_port].cFOP_PM = true;
    erps_timer_start(erps_state->ring_port_state[ring_port].pm_timer, ERPS_RAPS_RX_TIMEOUT_MS, false);
    return true;
}

/******************************************************************************/
// ERPS_BASE_rx_statistics_update()
/******************************************************************************/
static void ERPS_BASE_rx_statistics_update(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, vtss_appl_erps_raps_info_t &rx_info)
{
    vtss_appl_erps_statistics_t &s = erps_state->status.ring_port_status[ring_port].statistics;

    switch (rx_info.request) {
    case VTSS_APPL_ERPS_REQUEST_NR:
        if (rx_info.rb) {
            s.rx_nr_rb_cnt++;
        } else {
            s.rx_nr_cnt++;
        }

        break;

    case VTSS_APPL_ERPS_REQUEST_MS:
        s.rx_ms_cnt++;
        break;

    case VTSS_APPL_ERPS_REQUEST_SF:
        s.rx_sf_cnt++;
        break;

    case VTSS_APPL_ERPS_REQUEST_FS:
        s.rx_fs_cnt++;
        break;

    case VTSS_APPL_ERPS_REQUEST_EVENT:
        s.rx_event_cnt++;
        break;

    default:
        T_EG(ERPS_TRACE_GRP_FRAME_RX, "%u:%s: Invalid request (%d)", erps_state->inst, ring_port, rx_info.request);
        break;
    }
}

//*****************************************************************************/
// ERPS_BASE_state_clear()
// Clears state maintained by base.
/******************************************************************************/
static void ERPS_BASE_state_clear(erps_state_t *erps_state)
{
    erps_ring_port_state_t        *ring_port_state;
    vtss_appl_erps_oper_state_t   oper_state;
    vtss_appl_erps_oper_warning_t oper_warning;
    vtss_appl_erps_ring_port_t    ring_port;
    uint8_t                       version;
    mesa_mac_t                    node_id, smac;

    T_IG(ERPS_TRACE_GRP_BASE, "%u: Enter", erps_state->inst);

    erps_state->command = VTSS_APPL_ERPS_COMMAND_NR;

    // Reset ourselves, but keep a couple of the fields, which are maintained by
    // erps.cxx and already configured.
    oper_state   = erps_state->status.oper_state;
    oper_warning = erps_state->status.oper_warning;
    version      = erps_state->status.tx_raps_info.version;
    node_id      = erps_state->status.tx_raps_info.node_id;
    smac         = erps_state->status.tx_raps_info.smac;
    memset(&erps_state->status, 0, sizeof(erps_state->status));
    erps_state->status.oper_state           = oper_state;
    erps_state->status.oper_warning         = oper_warning;
    erps_state->status.tx_raps_info.version = version;
    erps_state->status.tx_raps_info.node_id = node_id;
    erps_state->status.tx_raps_info.smac    = smac;

    // Other initializations
    erps_state->status.node_state = VTSS_APPL_ERPS_NODE_STATE_INIT;

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        ring_port_state = &erps_state->ring_port_state[ring_port];
        ring_port_state->sf = false;
        vtss_clear(ring_port_state->rx_node_id_bpr);
        vtss_clear(ring_port_state->rx_raps_info);
        erps_timer_init(ring_port_state->hoff_timer, "Hoff", erps_state->inst, ERPS_BASE_hoff_timeout, erps_state);
        erps_timer_init(ring_port_state->pm_timer,   "PM",   erps_state->inst, ERPS_BASE_pm_timeout,   erps_state);
    }

    erps_state->ace_conf.id      = ACL_MGMT_ACE_ID_NONE;
    erps_state->wtr_event        = false;
    erps_state->wtb_event        = false;
    erps_state->top_prio_request = ERPS_BASE_REQUEST_NONE;

    // erps_timer_init() also stops the timer if active.
    erps_timer_init(erps_state->wtr_timer,   "WTR",   erps_state->inst, ERPS_BASE_wtr_timeout,   erps_state);
    erps_timer_init(erps_state->guard_timer, "Guard", erps_state->inst, ERPS_BASE_guard_timeout, erps_state);
    erps_timer_init(erps_state->wtb_timer,   "WTB",   erps_state->inst, ERPS_BASE_wtb_timeout,   erps_state);
    erps_timer_init(erps_state->tc_timer,    "TC",    erps_state->inst, ERPS_BASE_tc_timeout,    erps_state);
    erps_timer_init(erps_state->rx_timer,    "Rx",    erps_state->inst, ERPS_BASE_rx_timeout,    erps_state);

    erps_state->history.clear();
    ERPS_BASE_history_update(erps_state, ERPS_BASE_REQUEST_NONE, ERPS_BASE_REQUEST_NONE, nullptr, VTSS_APPL_ERPS_RING_PORT0, ERPS_BASE_FLUSH_REASON_NONE);

    erps_raps_state_init(erps_state);
}

//*****************************************************************************/
// ERPS_BASE_mesa_vlan_set()
/******************************************************************************/
static mesa_rc ERPS_BASE_mesa_vlan_set(erps_state_t *erps_state, mesa_vid_t vid, bool add)
{
    mesa_rc rc;

    T_NG(ERPS_TRACE_GRP_API, "%u: mesa_erps_vlan_member_set(%u, %u, %d)", erps_state->inst, erps_state->erpi(), vid, add);
    if ((rc = mesa_erps_vlan_member_set(nullptr, erps_state->erpi(), vid, add)) != VTSS_RC_OK) {
        T_EG(ERPS_TRACE_GRP_API, "%u: mesa_erps_vlan_member_set(%u, %u, %d) failed: %s", erps_state->inst, erps_state->erpi(), vid, add, error_txt(rc));
        // Convert into something sensible
        return VTSS_APPL_ERPS_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// ERPS_BASE_protected_vlans_set()
/******************************************************************************/
static mesa_rc ERPS_BASE_protected_vlans_set(erps_state_t *erps_state, uint8_t *vlans, bool add)
{
    mesa_vid_t vid;
    mesa_rc    rc, first_encountered_rc = VTSS_RC_OK;

    T_IG(ERPS_TRACE_GRP_BASE, "%u: Enter", erps_state->inst);

    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        if (!VTSS_BF_GET(vlans, vid)) {
            continue;
        }

        if ((rc = ERPS_BASE_mesa_vlan_set(erps_state, vid, add)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    return first_encountered_rc;
}

//*****************************************************************************/
// ERPS_BASE_protected_vlans_update()
/******************************************************************************/
static mesa_rc ERPS_BASE_protected_vlans_update(erps_state_t *erps_state)
{
    uint8_t    *old_vlans = erps_state->old_conf.protected_vlans;
    uint8_t    *new_vlans = erps_state->conf.protected_vlans;
    bool       add;
    mesa_vid_t vid;
    mesa_rc    rc, first_encountered_rc = VTSS_RC_OK;

    T_IG(ERPS_TRACE_GRP_BASE, "%u: Enter", erps_state->inst);

    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; vid++) {
        add = VTSS_BF_GET(new_vlans, vid);

        if (VTSS_BF_GET(old_vlans, vid) == add) {
            // No change for this VLAN.
            continue;
        }

        if ((rc = ERPS_BASE_mesa_vlan_set(erps_state, vid, add)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    return first_encountered_rc;
}

//*****************************************************************************/
// ERPS_BASE_protected_vlans_add()
/******************************************************************************/
static mesa_rc ERPS_BASE_protected_vlans_add(erps_state_t *erps_state)
{
    return ERPS_BASE_protected_vlans_set(erps_state, erps_state->conf.protected_vlans, true);
}

//*****************************************************************************/
// ERPS_BASE_protected_vlans_remove()
/******************************************************************************/
static mesa_rc ERPS_BASE_protected_vlans_remove(erps_state_t *erps_state, bool use_old_conf)
{
    uint8_t  *vlans = use_old_conf ? erps_state->old_conf.protected_vlans : erps_state->conf.protected_vlans;

    return ERPS_BASE_protected_vlans_set(erps_state, vlans, false);
}

//*****************************************************************************/
// ERPS_BASE_vlan_ingress_filtering_set()
// This one enables or disables a given ERPS instance's contribution to whether
// VLAN Ingress Filtering is enabled on a given ring port.
// When we enable ERPS, we must also force ingress filtering on ring ports,
// because otherwise - if user has administratively disabled ingress filtering -
// any frame will be accepted when it arrives on a ring port - even when the
// ring port is blocked (not member of that VLAN). This will cause loops in the
// network; loops that we try to avoid when using ERPS.
//
// Since several ERPS instances may use the same ring ports, we must reference
// count the number of ERPS instances that have contributed to VLAN ingress
// filtering on a given port, hence the global CapArray called
// ERPS_BASE_vlan_ingress_filtering_ref_cnt.
/******************************************************************************/
static mesa_rc ERPS_BASE_vlan_ingress_filtering_set(erps_state_t *erps_state, bool enable, bool use_old_conf = false)
{
    vtss_appl_vlan_port_conf_t vlan_port_conf;
    vtss_appl_erps_ring_port_t ring_port;
    vtss_appl_erps_conf_t      *conf = use_old_conf ? &erps_state->old_conf : &erps_state->conf;
    erps_ring_port_state_t     *ring_port_state;
    mesa_port_no_t             port_no;
    uint32_t                   cur_ref_cnt;
    bool                       invoke_vlan_module;
    mesa_rc                    rc, first_encountered_rc = VTSS_RC_OK;

    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        if (ring_port == VTSS_APPL_ERPS_RING_PORT1 && conf->ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
            // We don't use port1
            continue;
        }

        ring_port_state = use_old_conf ? &erps_state->old_ring_port_state[ring_port] : &erps_state->ring_port_state[ring_port];
        if (ring_port_state->port_states[0] == nullptr) {
            continue;
        }

        port_no = ring_port_state->port_states[0]->port_no;
        cur_ref_cnt = ERPS_BASE_vlan_ingress_filtering_ref_cnt[port_no];

        invoke_vlan_module = false;
        if (enable) {
            ERPS_BASE_vlan_ingress_filtering_ref_cnt[port_no]++;

            if (cur_ref_cnt == 0) {
                // Transitioning from 0 to 1
                invoke_vlan_module = true;
            }
        } else {
            if (cur_ref_cnt == 0) {
                T_EG(ERPS_TRACE_GRP_BASE, "%u:%s. Trying to disable VLAN ingress filtering on port_no %u, but ref_cnt is already 0", erps_state->inst, ring_port, port_no);
                if (first_encountered_rc == VTSS_RC_OK) {
                    first_encountered_rc = VTSS_APPL_ERPS_RC_INTERNAL_ERROR;
                }

                continue;
            }

            ERPS_BASE_vlan_ingress_filtering_ref_cnt[port_no]--;

            if (cur_ref_cnt == 1) {
                // Transitioning from 1 to 0
                invoke_vlan_module = true;
            }
        }

        if (!invoke_vlan_module) {
            continue;
        }

        T_IG(ERPS_TRACE_GRP_BASE, "%u:%s. vlan_mgmt_port_conf_get(%u)", erps_state->inst, ring_port, port_no);
        if ((rc = vlan_mgmt_port_conf_get(VTSS_ISID_START, port_no, &vlan_port_conf, VTSS_APPL_VLAN_USER_ERPS, TRUE)) != VTSS_RC_OK) {
            T_EG(ERPS_TRACE_GRP_BASE, "%u:%s. vlan_mgmt_port_conf_get(%u) failed: %s", erps_state->inst, ring_port, port_no, error_txt(rc));
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = VTSS_APPL_ERPS_RC_INTERNAL_ERROR;
            }

            continue;
        }

        vlan_port_conf.hybrid.flags = enable ? VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT : 0;
        vlan_port_conf.hybrid.ingress_filter = enable;

        // VLAN module calls ourselves back in erps.cxx whenever we change the
        // VLAN port configuration, so we avoid taking our own mutex twice by
        // setting this boolean.
        extern volatile bool ERPS_vlan_being_configured_by_ourselves;
        ERPS_vlan_being_configured_by_ourselves = true;

        T_IG(ERPS_TRACE_GRP_BASE, "%u:%s. vlan_mgmt_vlan_port_conf_set(%u, %d)", erps_state->inst, ring_port, port_no, enable);
        if ((rc = vlan_mgmt_port_conf_set(VTSS_ISID_START, port_no, &vlan_port_conf, VTSS_APPL_VLAN_USER_ERPS)) != VTSS_RC_OK) {
            T_EG(ERPS_TRACE_GRP_BASE, "%u:%s. vlan_mgmt_vlan_port_conf_set(%u, %d) failed: %s", erps_state->inst, ring_port, port_no, enable, error_txt(rc));
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = VTSS_APPL_ERPS_RC_INTERNAL_ERROR;
            }
        }

        ERPS_vlan_being_configured_by_ourselves = false;
    }

    return first_encountered_rc;
}

//*****************************************************************************/
// ERPS_BASE_do_deactivate()
/******************************************************************************/
static mesa_rc ERPS_BASE_do_deactivate(erps_state_t *erps_state, bool use_old_conf)
{
    vtss_appl_erps_ring_port_t ring_port;
    erps_ring_port_state_t     *ring_port_state;
    vtss_appl_erps_conf_t      *conf;
    mesa_rc                    rc, first_encountered_rc = VTSS_RC_OK;

    T_IG(ERPS_TRACE_GRP_BASE, "%u: use_old_conf = %d", erps_state->inst, use_old_conf);

    // Stop R-APS Tx and free frame pointers.
    // Here, we should always use current conf, because it has the same frame
    // pointers as the old conf.
    erps_raps_deactivate(erps_state);

    // Remove our ACE.
    if ((rc = ERPS_BASE_ace_remove(erps_state)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    // Remove all VLANs from the ERPS instance.
    if ((rc = ERPS_BASE_protected_vlans_remove(erps_state, use_old_conf)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    // Set both ring ports to forwarding
    // This is really not needed as long as all VLANs are removed from a given
    // ERPS instance.
    for (ring_port = VTSS_APPL_ERPS_RING_PORT0; ring_port <= VTSS_APPL_ERPS_RING_PORT1; ring_port++) {
        ring_port_state = use_old_conf ? &erps_state->old_ring_port_state[ring_port] : &erps_state->ring_port_state[ring_port];
        conf            = use_old_conf ? &erps_state->old_conf                       : &erps_state->conf;

        if (ring_port == VTSS_APPL_ERPS_RING_PORT1 && conf->ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
            // We don't decide the port state of port1 when we are an
            // interconnected sub-ring.
            continue;
        }

        if (!ring_port_state->port_states[0]) {
            // We haven't messed with this port before we got deactivated.
            continue;
        }

        // Don't go through the ERPS_BASE_ring_port_block() function, because
        // it checks the current state before blocking/unblocking, and because
        // it always uses the current configuration and state and not
        // necessarily the old one, so here we need to enforce it.
        if ((rc = ERPS_BASE_mesa_state_set(erps_state, ring_port_state->port_states[0]->port_no, MESA_ERPS_STATE_FORWARDING)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    // Remove this instance's contribution to VLAN ingress filtering on ring
    // ports.
    if ((rc = ERPS_BASE_vlan_ingress_filtering_set(erps_state, false, use_old_conf)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    // And clear the state
    ERPS_BASE_state_clear(erps_state);

    return first_encountered_rc;
}

//*****************************************************************************/
// ERPS_BASE_non_ring_ports_unblock()
// Despite its name, this function not only unblocks non-ring ports. It also
// blocks ring ports.
/******************************************************************************/
static mesa_rc ERPS_BASE_non_ring_ports_unblock(erps_state_t *erps_state)
{
    uint32_t          port_cnt;
    mesa_erps_state_t mesa_state;
    mesa_port_no_t    port_no, port0_port_no, port1_port_no;
    mesa_rc           rc;

    // We must set all other ports than port0 and port1 to forwarding once and
    // for all, because that's not the default in the API. If other ports
    // weren't forwarding, they would never become members of the protected
    // VLANs no matter how the end-user configures VLANs.

    // Use MEBA_CAP_BOARD_PORT_MAP_COUNT rather than MEBA_CAP_BOARD_PORT_CNT,
    // because we want to include possible mirror loop ports as well.
    port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    // First get our ring port port numbers, so that we can set those two ports
    // in discarding, and let the FSM control their state.
    port0_port_no = erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT0].port_states[0]->port_no;

    if (erps_state->conf.ring_type == VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB) {
        // An interconnected sub-ring doesn't have a port1 that we control.
        port1_port_no = MESA_PORT_NO_NONE;
    } else {
        port1_port_no = erps_state->ring_port_state[VTSS_APPL_ERPS_RING_PORT1].port_states[0]->port_no;
    }

    for (port_no = 0; port_no < port_cnt; port_no++) {
        mesa_state = port_no == port0_port_no || port_no == port1_port_no ? MESA_ERPS_STATE_DISCARDING : MESA_ERPS_STATE_FORWARDING;

        if ((rc = ERPS_BASE_mesa_state_set(erps_state, port_no, mesa_state)) != VTSS_RC_OK) {
            return rc;
        }
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// ERPS_BASE_do_activate()
/******************************************************************************/
static mesa_rc ERPS_BASE_do_activate(erps_state_t *erps_state, bool initial_sf_port0, bool initial_sf_port1)
{
    T_IG(ERPS_TRACE_GRP_BASE, "%u: SF: port0 = %d, port1 = %d", erps_state->inst, initial_sf_port0, initial_sf_port1);

    ERPS_BASE_state_clear(erps_state);

    // This one ref-counts, so it must be done before anything else that can
    // cause a premature exit of this function.
    VTSS_RC(ERPS_BASE_vlan_ingress_filtering_set(erps_state, true));

    // We must initialize the API and set all ports to forwarding, except for
    // our two ring ports, which we set in discarding before we start adding
    // protected VLANs to the ring instance. If we didn't do so, the API would
    // exclude the protected VLANs from non-ring ports.
    VTSS_RC(ERPS_BASE_non_ring_ports_unblock(erps_state));

    // Install VLANs in conf.protected_vlans into the API
    VTSS_RC(ERPS_BASE_protected_vlans_add(erps_state));

    // Create a rule that captures R-APS PDUs and possibly forwards them
    VTSS_RC(ERPS_BASE_ace_add(erps_state));

    // Create a new R-APS PDU. This will not transmit any frames, only create
    // the PDU.
    VTSS_RC(erps_raps_tx_frame_update(erps_state, false));

    // Set the initial value of SF of the two ring ports.
    // The following two function calls may end up calling
    // ERPS_BASE_run_state_machine(), which in turn might end up sending new
    // frames.
    erps_base_sf_set(erps_state, VTSS_APPL_ERPS_RING_PORT0, initial_sf_port0);
    erps_base_sf_set(erps_state, VTSS_APPL_ERPS_RING_PORT1, initial_sf_port1);

    // If erps_base_sf_set() did not call ERPS_BASE_run_state_machine(), we
    // need to do it here to get it all initialized.
    ERPS_BASE_run_state_machine(erps_state);

    return VTSS_RC_OK;
}

//*****************************************************************************/
// erps_base_deactivate()
/******************************************************************************/
mesa_rc erps_base_deactivate(erps_state_t *erps_state)
{
    return ERPS_BASE_do_deactivate(erps_state, true /* Use old_conf */);
}

//*****************************************************************************/
// erps_base_activate()
/******************************************************************************/
mesa_rc erps_base_activate(erps_state_t *erps_state, bool initial_sf_port0, bool initial_sf_port1)
{
    mesa_rc rc;

    if ((rc = ERPS_BASE_do_activate(erps_state, initial_sf_port0, initial_sf_port1)) != VTSS_RC_OK) {
        (void)ERPS_BASE_do_deactivate(erps_state, false /* Use current conf */);
    }

    return rc;
}

//*****************************************************************************/
// erps_base_command_set()
/******************************************************************************/
mesa_rc erps_base_command_set(erps_state_t *erps_state, vtss_appl_erps_command_t new_cmd)
{
    bool                     clear_first;
    vtss_appl_erps_command_t old_cmd = erps_state->command;

    T_IG(ERPS_TRACE_GRP_BASE, "%u: Command: %s->%s", erps_state->inst, erps_util_command_to_str(old_cmd), erps_util_command_to_str(new_cmd));

    if (new_cmd == old_cmd) {
        return VTSS_RC_OK;
    }

    clear_first = false;
    switch (old_cmd) {
    case VTSS_APPL_ERPS_COMMAND_FS_TO_PORT0:
    case VTSS_APPL_ERPS_COMMAND_FS_TO_PORT1:
    case VTSS_APPL_ERPS_COMMAND_MS_TO_PORT0:
    case VTSS_APPL_ERPS_COMMAND_MS_TO_PORT1:
        clear_first = true;
        break;

    default:
        break;
    }

    if (clear_first && new_cmd != VTSS_APPL_ERPS_COMMAND_CLEAR) {
        // When changing from some command-driven state to some other command-
        // driven state, we must clear the old state in the state machine before
        // going to the new state.
        erps_state->command = VTSS_APPL_ERPS_COMMAND_CLEAR;
        ERPS_BASE_run_state_machine(erps_state);
    }

    erps_state->command = new_cmd;

    ERPS_BASE_run_state_machine(erps_state);

    return VTSS_RC_OK;
}

//*****************************************************************************/
// erps_base_statistics_clear()
/******************************************************************************/
void erps_base_statistics_clear(erps_state_t *erps_state)
{
    erps_raps_statistics_clear(erps_state);
}

//*****************************************************************************/
// erps_base_rx_frame()
// Only invoked if either port0 or port1 matches for major rings and
// non-interconnected sub-rings.
// For interconnected sub-rings with virtual channel, it may have been received
// on one of the connected ring's ring ports, in which case ring_port is port1.
/******************************************************************************/
void erps_base_rx_frame(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, const uint8_t *frm, const mesa_packet_rx_info_t *const rx_info)
{
    vtss_appl_erps_ring_port_status_t &ring_port_status = erps_state->status.ring_port_status[ring_port];
    vtss_appl_erps_statistics_t       &s = ring_port_status.statistics;
    vtss_appl_erps_raps_info_t        rx_raps_info;
    bool                              do_flush;

    if (!erps_raps_rx_frame(erps_state, ring_port, frm, rx_info, &rx_raps_info)) {
        // Frame didn't pass validation
        s.rx_error_cnt++;
        return;
    }

    // (Re-)start ERPS Rx Timer if received a PDU.
    // According to G.8032, clause 10.4 and G.8021, Table 6-2, dFOP-TO
    // detection, we must set the dFOP-TO defect if we haven't received a valid
    // R-APS PDU for 3.5 * 5 = 17.5 seconds.
    ERPS_BASE_rx_timer_start(erps_state);

    // Received a valid R-APS PDU on protect port. No Rx timeout (anymore).
    ERPS_BASE_dfop_to_set(erps_state, false);

    if (rx_raps_info.node_id == erps_state->status.tx_raps_info.node_id) {
        // It's sent by ourselves
        T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u: Discarding frame because the node ID is our own", erps_state->inst);
        s.rx_own_cnt++;
        return;
    }

    // If the guard-timer is running, we need to discard this frame.
    if (erps_timer_active(erps_state->guard_timer)) {
        T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u: Discarding frame because guard timer is running", erps_state->inst);
        s.rx_guard_cnt++;
        return;
    }

    if (ERPS_BASE_cfop_pm_check_failed(erps_state, ring_port, rx_raps_info)) {
        // There are two RPL owners on the ring, however, the state machine must
        // still be invoked, because it seems that if remote request is
        // ERPS_BASE_REQUEST_REMOTE_NR_RB there are still special actions to
        // take if we are RPL owner.
        // Let's count it anyway. This is the only case where the same frame is
        // counted twice (Both as fop_pm_cnt and as rx_nr_rb_cnt).
        T_IG(ERPS_TRACE_GRP_FRAME_RX, "%u: FOP-PM detected (two RPL owners on ring)", erps_state->inst);
        s.rx_fop_pm_cnt++;
    }

    // All is good. Save the Rx info to the public status for this ring port
    ring_port_status.rx_raps_info = rx_raps_info;

    ERPS_BASE_rx_statistics_update(erps_state, ring_port, rx_raps_info);

    // Now, if it's an event, we don't pass it to the state machine, but handle
    // it right now and get on with our lives.
    // See G.8032, clause 10.1.6 (Validity check) and last line of clause
    // 10.1.10 (Flush logic).
    if (rx_raps_info.request == VTSS_APPL_ERPS_REQUEST_EVENT) {
        T_DG(ERPS_TRACE_GRP_FRAME_RX, "%u: Rx Event. Flushing FDB.", erps_state->inst);
        ERPS_BASE_fdb_flush(erps_state);
        ERPS_BASE_history_update(erps_state, ERPS_BASE_REQUEST_NONE, ERPS_BASE_REQUEST_NONE, nullptr, ring_port, ERPS_BASE_FLUSH_REASON_EVENT);
        return;
    }

    // Perform G.8032-201708-Cor1, clause 10.1.10 (Flush logic).
    do_flush = ERPS_BASE_rx_flush_logic(erps_state, ring_port, rx_raps_info);

    // This is the only call to ERPS_BASE_run_state_machine() with a non-default
    // request parameter
    ERPS_BASE_run_state_machine(erps_state, ring_port, &rx_raps_info, do_flush);
}

//*****************************************************************************/
// erps_base_tx_frame_update()
/******************************************************************************/
void erps_base_tx_frame_update(erps_state_t *erps_state)
{
    (void)erps_raps_tx_frame_update(erps_state, true /* Do transmit the updated PDU right away */);
}

//*****************************************************************************/
// erps_base_sf_set()
/******************************************************************************/
void erps_base_sf_set(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, bool new_sf)
{
    vtss_appl_erps_ring_port_status_t *ring_port_status = &erps_state->status.ring_port_status[ring_port];
    erps_ring_port_state_t            *ring_port_state  = &erps_state->ring_port_state[ring_port];
    bool                              old_sf;
    bool                              no_change = new_sf == ring_port_state->sf;

    ERPS_BASE_sf_set(erps_state, ring_port, new_sf);

    if (no_change) {
        // Nothing has happened since last time. Done.
        return;
    }

    if (new_sf) {
        ring_port_status->statistics.sf_cnt++;
    }

    old_sf = ring_port_status->sf;

    if (new_sf > old_sf && erps_state->conf.hold_off_msecs != 0) {
        // Going to a more severe defect.
        // According to G.8032, clause 10.1.8, we should (re-)start the hold-off
        // timer, which runs ERPS_BASE_hoff_timeout() when it expires.
        erps_timer_start(ring_port_state->hoff_timer, erps_state->conf.hold_off_msecs, false);
        return;
    }

    if (new_sf < old_sf || erps_state->conf.hold_off_msecs == 0) {
        // Either the hold-off timer is disabled or we are going from a more
        // severe to a less severe state (that is, SF to OK). Either way, we
        // stop the hold-off timer, and pass the new state directly into the
        // public state used by the state machine.
        erps_timer_stop(ring_port_state->hoff_timer);
        ring_port_status->sf = new_sf;
    }

    if (!new_sf) {
        // A SF-to-OK transition needed by EPRS_BASE_run_state_machine().
        ring_port_state->sf_clearing = true;
    }

    // Time to run the state machine, which uses both the public state
    ERPS_BASE_run_state_machine(erps_state);
}

//*****************************************************************************/
// erps_base_protected_vlans_update()
/******************************************************************************/
mesa_rc erps_base_protected_vlans_update(erps_state_t *erps_state)
{
    return ERPS_BASE_protected_vlans_update(erps_state);
}

//*****************************************************************************/
// erps_base_matching_update()
/******************************************************************************/
mesa_rc erps_base_matching_update(erps_state_t *erps_state)
{
    // Update match on ring ID, level, and possibly also connected ring's ports.
    return ERPS_BASE_ace_update(erps_state);
}

//*****************************************************************************/
// erps_base_connected_ring_ports_update()
/******************************************************************************/
mesa_rc erps_base_connected_ring_ports_update(erps_state_t *erps_state)
{
    mesa_rc rc;

    // Update ingress port matching and egress port forwarding in our ACE.
    rc = ERPS_BASE_ace_update(erps_state);

    // Change of this interconnected sub-ring's connected ring ports also
    // affects frame Tx.
    erps_base_tx_frame_update(erps_state);

    return rc;
}

/******************************************************************************/
// erps_base_history_dump()
/******************************************************************************/
void erps_base_history_dump(erps_state_t *erps_state, uint32_t relative_to, uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...), bool print_hdr)
{
    erps_base_history_itr_t hist_itr;
    uint32_t                cnt;
    uint64_t                start_time_ms, now;
    char                    str[26], str2[26], str3[30];

    now = vtss::uptime_milliseconds();
    if (print_hdr) {
        pr(session_id, "Now = " VPRI64u " ms\n", now);
        pr(session_id, "                                                                                              Port0 Port1 Port0   Port1\n");
        pr(session_id, "Inst   # Time [ms]      Flush  Local Req     Remote Req    NodeID-BPR              Node State SF    SF    Blocked Blocked Tx R-APS PDU\n");
        pr(session_id, "---- --- -------------- ------ ------------- ------------- ----------------------- ---------- ----- ----- ------- ------- ----------------------\n");
    }

    if (relative_to == 0) {
        // Use absolute times.
        start_time_ms = 0;
    } else {
        // Use relative times. Default to current time.
        start_time_ms = now;

        if (relative_to == -1) {
            // User wants last entry.
            relative_to = erps_state->history.size();
        }

        // Search for the entry.
        cnt = 1;
        for (hist_itr = erps_state->history.begin(); hist_itr != erps_state->history.end(); ++hist_itr) {
            if (cnt++ == relative_to) {
                start_time_ms = hist_itr->event_time_ms;
                break;
            }
        }
    }

    cnt = 1;
    for (hist_itr = erps_state->history.begin(); hist_itr != erps_state->history.end(); ++hist_itr) {
        sprintf(str2, "%s", ERPS_BASE_request_to_str(hist_itr->remote_request));
        if (hist_itr->remote_request != ERPS_BASE_REQUEST_NONE) {
            strcat(str2, "-");
            strcat(str2, erps_util_ring_port_to_str(hist_itr->rx_ring_port));
        }

        if (hist_itr->remote_request != ERPS_BASE_REQUEST_NONE || hist_itr->flush_reason == ERPS_BASE_FLUSH_REASON_NODE_ID) {
            (void)misc_mac_txt(hist_itr->rx_node_id_bpr.node_id.addr, str3);
            strcat(str3, "-");
            strcat(str3, erps_util_ring_port_to_str(hist_itr->rx_node_id_bpr.bpr));
        } else {
            str3[0] = '\0';
        }

        pr(session_id, "%4u %3u " VPRI64Fd("14") " %-6s %-13s %-13s %-23s %-10s %-5s %-5s %-7s %-7s %s\n",
           erps_state->inst,
           cnt++,
           hist_itr->event_time_ms - start_time_ms,
           ERPS_BASE_flush_reason_to_str(hist_itr->flush_reason),
           ERPS_BASE_request_to_str(hist_itr->local_request),
           str2,
           str3,
           erps_util_node_state_to_str(hist_itr->node_state),
           ERPS_BASE_bool_to_yes_no(hist_itr->port0_sf),
           ERPS_BASE_bool_to_yes_no_dash(erps_state, hist_itr->port1_sf),
           ERPS_BASE_bool_to_yes_no(hist_itr->port0_blocked),
           ERPS_BASE_bool_to_yes_no_dash(erps_state, hist_itr->port1_blocked),
           erps_util_raps_info_to_str(hist_itr->tx_raps_info, str, hist_itr->tx_raps_active, true /* include BPR info */));
    }

    if (cnt == 1) {
        pr(session_id, "%4u <No events registered yet>\n", erps_state->inst);
    }
}

/******************************************************************************/
// erps_base_history_clear()
/******************************************************************************/
void erps_base_history_clear(erps_state_t *erps_state)
{
    erps_state->history.clear();
}

/******************************************************************************/
// erps_base_rules_dump()
// Dumps active rules.
/******************************************************************************/
void erps_base_rule_dump(erps_state_t *erps_state, uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...), bool print_hdr)
{
    acl_entry_conf_t   ace_conf;
    mesa_ace_counter_t ace_counter;
    char               ingr_buf[MGMT_PORT_BUF_SIZE], egr_buf[MGMT_PORT_BUF_SIZE];
    mesa_rc            rc;

    if (print_hdr) {
        pr(session_id, "Inst ACE ID Ingress Ports     Egress Ports      Hit count\n");
        pr(session_id, "---- ------ ----------------- ----------------- --------------\n");
    }

    if (erps_state->ace_conf.id == ACL_MGMT_ACE_ID_NONE) {
        pr(session_id, "%4u <None>\n", erps_state->inst);
        return;
    }

    if ((rc = acl_mgmt_ace_get(ACL_USER_ERPS, erps_state->ace_conf.id, &ace_conf, &ace_counter, FALSE)) != VTSS_RC_OK) {
        T_EG(ERPS_TRACE_GRP_BASE, "%u: acl_mgmt_ace_get(%u) failed: %s", erps_state->inst, erps_state->ace_conf.id, error_txt(rc));
        return;
    }

    pr(session_id, "%4u %6d %17s %17s %14u\n",
       erps_state->inst,
       ace_conf.id,
       mgmt_iport_list2txt(ace_conf.port_list, ingr_buf),
       mgmt_iport_list2txt(ace_conf.action.port_list, egr_buf),
       ace_counter);
}

