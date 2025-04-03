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

#include <vtss/appl/aps.h>
#include "aps_base.hxx"
#include "aps_laps.hxx"
#include "aps_trace.h"
#include "aps_api.h"
#include "cfm_api.h"    /* For CFM_ETYPE             */
#include "mgmt_api.h"   /* For mgmt_iport_list2txt() */
#include <microchip/ethernet/switch/api.h>

//*****************************************************************************/
// APS_BASE_request_to_str()
/******************************************************************************/
static const char *APS_BASE_request_to_str(aps_base_request_t request)
{
    switch (request) {
    case APS_BASE_REQUEST_NR:
        return "NR";

    case APS_BASE_REQUEST_DNR:
        return "DNR";

    case APS_BASE_REQUEST_RR:
        return "RR";

    case APS_BASE_REQUEST_CLEAR:
        return "Clear";

    case APS_BASE_REQUEST_EXER:
        return "EXER";

    case APS_BASE_REQUEST_WTR_EXPIRED:
        return "WTR-Expired";

    case APS_BASE_REQUEST_WTR:
        return "WTR";

    case APS_BASE_REQUEST_MS_TO_W:
        return "MS-to-W";

    case APS_BASE_REQUEST_MS_TO_P:
        return "MS-to-P";

    case APS_BASE_REQUEST_SD_W_OFF:
        return "SD-W Off";

    case APS_BASE_REQUEST_SD_W_ON:
        return "SD-W";

    case APS_BASE_REQUEST_SD_P_OFF:
        return "SD-P Off";

    case APS_BASE_REQUEST_SD_P_ON:
        return "SD-P";

    case APS_BASE_REQUEST_SF_W_OFF:
        return "SF-W Off";

    case APS_BASE_REQUEST_SF_W_ON:
        return "SF-W";

    case APS_BASE_REQUEST_FS:
        return "FS";

    case APS_BASE_REQUEST_SF_P_OFF:
        return "SF-P Off";

    case APS_BASE_REQUEST_SF_P_ON:
        return "SF-P";

    case APS_BASE_REQUEST_LO:
        return  "LO";

    default:
        T_EG(APS_TRACE_GRP_BASE, "Invalid request prio (%d)", request);
        return "Unknown";
    }
}

//*****************************************************************************/
// APS_BASE_calc_defect_state()
/******************************************************************************/
static vtss_appl_aps_defect_state_t APS_BASE_calc_defect_state(bool sf, bool sd)
{
    if (sf) {
        return VTSS_APPL_APS_DEFECT_STATE_SF;
    } else if (sd) {
        return VTSS_APPL_APS_DEFECT_STATE_SD;
    } else {
        return VTSS_APPL_APS_DEFECT_STATE_OK;
    }
}

//*****************************************************************************/
// APS_BASE_request_internal_to_external_set()
/******************************************************************************/
static void APS_BASE_request_internal_to_external_set(aps_state_t *aps_state, bool update_tx)
{
    uint8_t                  *aps     = update_tx ? aps_state->tx_aps_info    : aps_state->rx_aps_info;
    vtss_appl_aps_aps_info_t *request = update_tx ? &aps_state->status.tx_aps : &aps_state->status.rx_aps;

    request->re_signal = aps[1];
    request->br_signal = aps[2];

    // The enumeration of vtss_appl_aps_request_t corresponds to the encoding
    // values, so we can directly translate from aps[0] to request->request.
    // Here, we cannot have an aps[0] which is not amongst the valid coding
    // points (if it's Tx, we generate aps[0] ourselves, if it's Rx, there is
    // a check in aps_base_rx_frame() of the values in the received APS info).
    request->request = (vtss_appl_aps_request_t)((aps[0] >> 4) & 0xF);
}

//*****************************************************************************/
// APS_BASE_tx_info_update()
/******************************************************************************/
static void APS_BASE_tx_info_update(aps_state_t *aps_state, vtss_appl_aps_prot_state_t state, bool shutting_down)
{
    bool    one_plus_one = aps_state->conf.mode != VTSS_APPL_APS_MODE_ONE_FOR_ONE;
    uint8_t *aps = aps_state->tx_aps_info;

    memset(aps, 0, sizeof(*aps));

    if (aps_state->tx_laps_pdus()) {
        aps[0] |= 0x08;
    }

    if (aps_state->conf.mode == VTSS_APPL_APS_MODE_ONE_FOR_ONE) {
        aps[0] |= 0x04;
    }

    if (aps_state->conf.mode != VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL) {
        // Bidirectional
        aps[0] |= 0x02;
    }

    if (aps_state->conf.revertive) {
        aps[0] |= 0x01;
    }

    switch (state) {
    case VTSS_APPL_APS_PROT_STATE_NR_W:
        aps[1] = 0;
        aps[2] = 0;

        if (one_plus_one) {
            aps[2] = 1;
        }

        break;

    case VTSS_APPL_APS_PROT_STATE_NR_P:
        aps[1] = 1;
        aps[2] = 1;
        break;

    case VTSS_APPL_APS_PROT_STATE_LO:
        aps[0] |= 0xF0;
        aps[1] = 0;
        aps[2] = 0;

        if (one_plus_one) {
            aps[2] = 1;
        }

        break;

    case VTSS_APPL_APS_PROT_STATE_FS:
        aps[0] |= 0xD0;
        aps[1] = 1;
        aps[2] = 1;
        break;

    case VTSS_APPL_APS_PROT_STATE_SF_W:
        aps[0] |= 0xB0;
        aps[1] = 1;
        aps[2] = 1;
        break;

    case VTSS_APPL_APS_PROT_STATE_SF_P:
        aps[0] |= 0xE0;
        aps[1] = 0;
        aps[2] = 0;

        if (one_plus_one) {
            aps[2] = 1;
        }

        break;

    case VTSS_APPL_APS_PROT_STATE_MS_TO_P:
        aps[0] |= 0x70;
        aps[1] = 1;
        aps[2] = 1;
        break;

    case VTSS_APPL_APS_PROT_STATE_MS_TO_W:
        aps[0] |= 0x70;
        aps[1] = 0;
        aps[2] = 0;

        if (one_plus_one) {
            aps[2] = 1;
        }

        break;

    case VTSS_APPL_APS_PROT_STATE_WTR:
        aps[0] |= 0x50;
        aps[1] = 1;
        aps[2] = 1;
        break;

    case VTSS_APPL_APS_PROT_STATE_EXER_W:
        aps[0] |= 0x40;
        aps[1] = 0;
        aps[2] = 0;

        if (one_plus_one) {
            aps[2] = 1;
        }

        break;

    case VTSS_APPL_APS_PROT_STATE_EXER_P:
        aps[0] |= 0x40;
        aps[1] = 1;
        aps[2] = 1;
        break;

    case VTSS_APPL_APS_PROT_STATE_RR_W:
        aps[0] |= 0x20;
        aps[1] = 0;
        aps[2] = 0;

        if (one_plus_one) {
            aps[2] = 1;
        }

        break;

    case VTSS_APPL_APS_PROT_STATE_RR_P:
        aps[0] |= 0x20;
        aps[1] = 1;
        aps[2] = 1;
        break;

    case VTSS_APPL_APS_PROT_STATE_DNR:
        aps[0] |= 0x10;
        aps[1] = 1;
        aps[2] = 1;
        break;

    default:
        T_EG(APS_TRACE_GRP_BASE, "%u: Invalid state (%u)", aps_state->inst, state);
    }

    T_DG(APS_TRACE_GRP_BASE, "%u: state = %s => aps[] = {0x%02x, 0x%02x, 0x%02x, 0x%02x}", aps_state->inst, aps_util_prot_state_to_str(state), aps[0], aps[1], aps[2], aps[3]);

    // Calculate transmitting APS request for management
    APS_BASE_request_internal_to_external_set(aps_state, true /* Tx part */);

    aps_laps_tx_info_update(aps_state, shutting_down);
}

//*****************************************************************************/
// APS_BASE_ace_remove()
/******************************************************************************/
static mesa_rc APS_BASE_ace_remove(aps_state_t *aps_state)
{
    mesa_rc rc;

    if (aps_state->ace_conf.id == ACL_MGMT_ACE_ID_NONE) {
        return VTSS_RC_OK;
    }

    // Delete it from the ACL module
    T_IG(APS_TRACE_GRP_ACL, "%u: acl_mgmt_ace_del(0x%x)", aps_state->inst, aps_state->ace_conf.id);
    if ((rc = acl_mgmt_ace_del(ACL_USER_APS, aps_state->ace_conf.id)) != VTSS_RC_OK) {
        T_EG(APS_TRACE_GRP_ACL, "%u: acl_mgmt_ace_del(0x%x) failed: %s", aps_state->inst, aps_state->ace_conf.id, error_txt(rc));
    }

    aps_state->ace_conf.id = ACL_MGMT_ACE_ID_NONE;

    return rc;
}

//*****************************************************************************/
// APS_BASE_ace_update()
// Updates VID and MEG level.
/******************************************************************************/
static mesa_rc APS_BASE_ace_update(aps_state_t *aps_state)
{
    acl_entry_conf_t new_ace_conf = aps_state->ace_conf /* 228 bytes */;
    mesa_ace_id_t    old_ace_id = new_ace_conf.id;
    mesa_rc          rc;

    // Fill in the variable MEG level (variable, because we don't reset
    // everything when this property changes).
    new_ace_conf.frame.etype.data.value[0] = aps_state->conf.level << 5;

    // In principle, we should have two rules if L-APS PDUs are untagged and the
    // PVID of W differs from PVID of P, but that must be in a future revision,
    // because the reception of L-APS PDUs on Working "only" gives rise to a
    // dFOP_CM and doesn't have anything to do with the ability to do protection
    // switching.
    new_ace_conf.vid.value = aps_state->classified_vid_get();

    if (new_ace_conf.id != ACL_MGMT_ACE_ID_NONE) {
        // Already installed. Check to see if the configuration has changed.
        if (memcmp(&new_ace_conf, &aps_state->ace_conf, sizeof(new_ace_conf)) == 0) {
            // No changes
            T_DG(APS_TRACE_GRP_ACL, "%u: No ACL changes", aps_state->inst);
            return VTSS_RC_OK;
        }
    }

    // The order of these rules doesn't matter, so just place it last within the
    // APS group of ACEs (ACL_MGMT_ACE_ID_NONE).
    if ((rc = acl_mgmt_ace_add(ACL_USER_APS, ACL_MGMT_ACE_ID_NONE, &new_ace_conf)) != VTSS_RC_OK) {
        T_EG(APS_TRACE_GRP_ACL, "%u: acl_mgmt_ace_add(0x%x, VLAN = %u) failed: %s", aps_state->inst, old_ace_id, new_ace_conf.vid.value, error_txt(rc));
        return VTSS_APPL_APS_RC_HW_RESOURCES;
    }

    T_IG(APS_TRACE_GRP_ACL, "%u: acl_mgmt_ace_add(VLAN = %u) => ace_id = 0x%x->0x%x", aps_state->inst, new_ace_conf.vid.value, old_ace_id, new_ace_conf.id);
    aps_state->ace_conf = new_ace_conf;

    return VTSS_RC_OK;
}

//*****************************************************************************/
// APS_BASE_ace_add()
/******************************************************************************/
static mesa_rc APS_BASE_ace_add(aps_state_t *aps_state)
{
    acl_entry_conf_t &ace_conf = aps_state->ace_conf;
    mesa_rc          rc;

    T_IG(APS_TRACE_GRP_BASE, "%u: Enter", aps_state->inst);

    if (aps_state->ace_conf.id != ACL_MGMT_ACE_ID_NONE) {
        T_EG(APS_TRACE_GRP_BASE, "%u: An ACE (ID = %u) already exists, but we are going to add a new", aps_state->inst, aps_state->ace_conf.id);
        return VTSS_APPL_APS_RC_INTERNAL_ERROR;
    }

    T_DG(APS_TRACE_GRP_BASE, "%u: acl_mgmt_ace_init()", aps_state->inst);
    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ETYPE, &ace_conf)) != VTSS_RC_OK) {
        T_EG(APS_TRACE_GRP_BASE, "%u: acl_mgmt_ace_init() failed: %s", aps_state->inst, error_txt(rc));
        return VTSS_APPL_APS_RC_INTERNAL_ERROR;
    }

    // Create the basic, non-variable ACE configuration for this APS instance.
    ace_conf.isid                       = VTSS_ISID_LOCAL;
    ace_conf.frame.etype.etype.value[0] = (CFM_ETYPE >> 8) & 0xFF;
    ace_conf.frame.etype.etype.value[1] = (CFM_ETYPE >> 0) & 0xFF;
    ace_conf.frame.etype.etype.mask[0]  = 0xFF;
    ace_conf.frame.etype.etype.mask[1]  = 0xFF;
    ace_conf.id                         = ACL_MGMT_ACE_ID_NONE;

    // Prepare to match only on three MSbits of data[0], which is the MEG level.
    ace_conf.frame.etype.data.mask[0]   = 0x7 << 5;

    // Terminate frames and only forward to CPU.
    ace_conf.action.port_list.clear_all();
    ace_conf.action.port_action         = MESA_ACL_PORT_ACTION_FILTER;
    ace_conf.action.force_cpu           = true;
    ace_conf.action.cpu_queue           = PACKET_XTR_QU_OAM;

    ace_conf.port_list.clear_all();
    ace_conf.port_list[aps_state->w_port_state->port_no] = true;
    ace_conf.port_list[aps_state->p_port_state->port_no] = true;

    // Always match on VID. The actual VID is variable, because we can have
    // untagged L-APS PDUs, so if the PVID changes, the classified VID also may
    // change.
    ace_conf.vid.mask = 0xFFFF;

    // Then let APS_BASE_ace_update() take care of classified VID and MEG level.
    return APS_BASE_ace_update(aps_state);
}

//*****************************************************************************/
// APS_BASE_calc_selector_state()
/******************************************************************************/
static mesa_eps_selector_t APS_BASE_calc_selector_state(uint32_t state)
{
    switch (state) {
    case VTSS_APPL_APS_PROT_STATE_NR_W:
        return MESA_EPS_SELECTOR_WORKING;

    case VTSS_APPL_APS_PROT_STATE_NR_P:
        return MESA_EPS_SELECTOR_PROTECTION;

    case VTSS_APPL_APS_PROT_STATE_LO:
        return MESA_EPS_SELECTOR_WORKING;

    case VTSS_APPL_APS_PROT_STATE_FS:
        return MESA_EPS_SELECTOR_PROTECTION;

    case VTSS_APPL_APS_PROT_STATE_SF_W:
        return MESA_EPS_SELECTOR_PROTECTION;

    case VTSS_APPL_APS_PROT_STATE_SF_P:
        return MESA_EPS_SELECTOR_WORKING;

    case VTSS_APPL_APS_PROT_STATE_MS_TO_W:
        return MESA_EPS_SELECTOR_WORKING;

    case VTSS_APPL_APS_PROT_STATE_MS_TO_P:
        return MESA_EPS_SELECTOR_PROTECTION;

    case VTSS_APPL_APS_PROT_STATE_WTR:
        return MESA_EPS_SELECTOR_PROTECTION;

    case VTSS_APPL_APS_PROT_STATE_EXER_W:
        return MESA_EPS_SELECTOR_WORKING;

    case VTSS_APPL_APS_PROT_STATE_EXER_P:
        return MESA_EPS_SELECTOR_PROTECTION;

    case VTSS_APPL_APS_PROT_STATE_RR_W:
        return MESA_EPS_SELECTOR_WORKING;

    case VTSS_APPL_APS_PROT_STATE_RR_P:
        return MESA_EPS_SELECTOR_PROTECTION;

    case VTSS_APPL_APS_PROT_STATE_DNR:
        return MESA_EPS_SELECTOR_PROTECTION;

    default:
        T_EG(APS_TRACE_GRP_BASE, "Invalid state (%u)", state);
        return MESA_EPS_SELECTOR_WORKING;
    }
}

//*****************************************************************************/
// APS_BASE_dfop_nr_update()
// fop = failure of protocol
// nr = No Response
/******************************************************************************/
static void APS_BASE_dfop_nr_update(aps_state_t *aps_state)
{
    if (aps_state->status.dFOP_PM || aps_state->status.dFOP_TO || aps_state->conf.mode == VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL) {
        // The received L-APS PDU is not worth looking at or we haven't received
        // one (ever or for 17.5 seconds) or we are not in a mode where we
        // should update dFOP_NR.
        return;
    }

    if (aps_state->tx_aps_info[1] != aps_state->rx_aps_info[1]) {
        // Mismatch in transmitted and received requested signal. Start a 50 ms
        // timer if not already started and if dFOP_NR is not already set.
        if (!aps_timer_active(aps_state->dFOP_NR_timer) && !aps_state->status.dFOP_NR) {
            aps_timer_start(aps_state->dFOP_NR_timer, 50, false);
        }
    } else {
        aps_state->status.dFOP_NR = false;
        aps_timer_stop(aps_state->dFOP_NR_timer);
    }
}

//*****************************************************************************/
// APS_BASE_selector_state_set()
/******************************************************************************/
static void APS_BASE_selector_state_set(aps_state_t *aps_state, mesa_eps_selector_t selector, bool old_config = false)
{
    mesa_port_no_t w_port_no = old_config ? aps_state->old_w_port_no : aps_state->w_port_state->port_no;
    mesa_rc        rc;

    T_DG(APS_TRACE_GRP_API, "%u: mesa_eps_port_selector_set(w_port_no = %u, selector = %u", aps_state->inst, w_port_no, selector);
    if ((rc = mesa_eps_port_selector_set(NULL, w_port_no, selector)) != VTSS_RC_OK) {
        T_EG(APS_TRACE_GRP_API, "%u: mesa_eps_port_selector_set(%u, %u) failed: %s", aps_state->inst, w_port_no, selector, error_txt(rc));
    }

    if (old_config) {
        return;
    }
}

//*****************************************************************************/
// APS_BASE_local_request_greater_than_or_equal_to_far_end_request()
//*****************************************************************************/
static bool APS_BASE_local_request_greater_than_or_equal_to_far_end_request(aps_state_t *aps_state, aps_base_request_t local_request, aps_base_request_t far_end_request)
{
    // This function cannot be called with local_request equal to
    //   - APS_BASE_REQUEST_CLEAR
    //   - APS_BASE_REQUEST_WTR_EXPIRED
    //   - APS_BASE_REQUEST_SD_W_OFF
    //   - APS_BASE_REQUEST_SD_P_OFF
    //   - APS_BASE_REQUEST_SF_W_OFF
    //   - APS_BASE_REQUEST_SF_P_OFF
    //   - any of the far-end-only requests
    switch (local_request) {
    case APS_BASE_REQUEST_NR:
        // If we don't have any local requests, run the far end SM to get the
        // selector set accordingly.
        return false;

    case APS_BASE_REQUEST_EXER:
        return far_end_request <= APS_BASE_REQUEST_EXER;

    case APS_BASE_REQUEST_MS_TO_W:
    case APS_BASE_REQUEST_MS_TO_P:
        return far_end_request <= APS_BASE_REQUEST_MS_TO_P;

    case APS_BASE_REQUEST_SD_W_ON:
    case APS_BASE_REQUEST_SD_P_ON:
        return far_end_request <= APS_BASE_REQUEST_SD_P_ON;

    case APS_BASE_REQUEST_SF_W_ON:
        return far_end_request <= APS_BASE_REQUEST_SF_W_ON;

    case APS_BASE_REQUEST_FS:
        return far_end_request <= APS_BASE_REQUEST_FS;

    case APS_BASE_REQUEST_SF_P_ON:
        return far_end_request <= APS_BASE_REQUEST_SF_P_ON;

    case APS_BASE_REQUEST_LO:
        return far_end_request <= APS_BASE_REQUEST_LO;

    default:
        T_EG(APS_TRACE_GRP_BASE, "%u: Invalid local request (%d = %s)", aps_state->inst, local_request, APS_BASE_request_to_str(local_request));
        return true;
    }
}

//*****************************************************************************/
// APS_BASE_local_request_get()
// Returns this highest priority local request given the current state.
// This means that we cannot return e.g. APS_BASE_REQUEST_EXER unless currently
// in a state where the EXER command is allowed, even though the current command
// is VTSS_APPL_APS_COMMAND_EXER.
/******************************************************************************/
static aps_base_request_t APS_BASE_local_request_get(aps_state_t *aps_state)
{
    vtss_appl_aps_prot_state_t cur_state = aps_state->status.prot_state;

    if (cur_state == VTSS_APPL_APS_PROT_STATE_LO) {
        // The only command that can get us out of a lockout is a clear.
        if (aps_state->command == VTSS_APPL_APS_COMMAND_CLEAR) {
            return APS_BASE_REQUEST_CLEAR;
        }

        // All other requests are overruled.
        return APS_BASE_REQUEST_NR;
    }

    if (aps_state->command == VTSS_APPL_APS_COMMAND_LO) {
        return APS_BASE_REQUEST_LO;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_SF_P) {
        if (aps_state->status.p_state < VTSS_APPL_APS_DEFECT_STATE_SF) {
            // The only event (not covered above) that can get us out of SF-P is
            // a recover from SF-P.
            return APS_BASE_REQUEST_SF_P_OFF;
        }

        // All other requests are overruled
        return APS_BASE_REQUEST_NR;
    }

    if (aps_state->status.p_state == VTSS_APPL_APS_DEFECT_STATE_SF) {
        return APS_BASE_REQUEST_SF_P_ON;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_FS) {
        // The only command or event (not covered above) that can get us out of
        // FS is a clear.
        if (aps_state->command == VTSS_APPL_APS_COMMAND_CLEAR) {
            return APS_BASE_REQUEST_CLEAR;
        }

        // All other requests are overruled.
        return APS_BASE_REQUEST_NR;
    }

    if (aps_state->command == VTSS_APPL_APS_COMMAND_FS) {
        return APS_BASE_REQUEST_FS;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_SF_W) {
        if (aps_state->status.w_state < VTSS_APPL_APS_DEFECT_STATE_SF) {
            // The only event (not covered above) that can get us out of SF-W is
            // a recover from SF-W.
            return APS_BASE_REQUEST_SF_W_OFF;
        }

        // All other requests are overruled
        return APS_BASE_REQUEST_NR;
    }

    if (aps_state->status.w_state == VTSS_APPL_APS_DEFECT_STATE_SF) {
        return APS_BASE_REQUEST_SF_W_ON;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_SD_P) {
        if (aps_state->status.p_state < VTSS_APPL_APS_DEFECT_STATE_SD) {
            // The only event (not covered above) that can get us out of SD-P is
            // a recover from SD-P.
            return APS_BASE_REQUEST_SD_P_OFF;
        }

        // All other requests are overruled
        return APS_BASE_REQUEST_NR;
    }

    if (aps_state->status.p_state == VTSS_APPL_APS_DEFECT_STATE_SD) {
        return APS_BASE_REQUEST_SD_P_ON;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_SD_W) {
        if (aps_state->status.w_state < VTSS_APPL_APS_DEFECT_STATE_SD) {
            // The only event (not covered above) that can get us out of SD-W is
            // a recover from SD-W.
            return APS_BASE_REQUEST_SD_W_OFF;
        }

        // All other requests are overruled
        return APS_BASE_REQUEST_NR;
    }

    if (aps_state->status.w_state == VTSS_APPL_APS_DEFECT_STATE_SD) {
        return APS_BASE_REQUEST_SD_W_ON;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_MS_TO_P || cur_state == VTSS_APPL_APS_PROT_STATE_MS_TO_W) {
        // The only thing that can get us out of a MS-to-W/P is a clear command.
        if (aps_state->command == VTSS_APPL_APS_COMMAND_CLEAR) {
            return APS_BASE_REQUEST_CLEAR;
        }

        // All other requests are overruled
        return APS_BASE_REQUEST_NR;
    }

    if (aps_state->command == VTSS_APPL_APS_COMMAND_MS_TO_P) {
        return APS_BASE_REQUEST_MS_TO_P;
    }

    if (aps_state->command == VTSS_APPL_APS_COMMAND_MS_TO_W) {
        return APS_BASE_REQUEST_MS_TO_W;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_WTR) {
        // The only two things left that can get us out of a WTR are a clear
        // command and an expiration of the WTR timer.
        if (aps_state->wtr_event) {
            return APS_BASE_REQUEST_WTR_EXPIRED;
        }

        if (aps_state->command == VTSS_APPL_APS_COMMAND_CLEAR) {
            return APS_BASE_REQUEST_CLEAR;
        }

        // All other requests are overruled
        return APS_BASE_REQUEST_NR;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_NR_W ||
        cur_state == VTSS_APPL_APS_PROT_STATE_DNR  ||
        cur_state == VTSS_APPL_APS_PROT_STATE_RR_W ||
        cur_state == VTSS_APPL_APS_PROT_STATE_RR_P) {
        // The only thing left that can get us out of DNR, NR_W, RR_W or RR_P is
        // an EXER command, which only works in bidirectional mode.
        if (aps_state->conf.mode != VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL && aps_state->command == VTSS_APPL_APS_COMMAND_EXER) {
            return APS_BASE_REQUEST_EXER;
        }

        // All other requests are overruled
        return APS_BASE_REQUEST_NR;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_EXER_W ||
        cur_state == VTSS_APPL_APS_PROT_STATE_EXER_P) {
        // The only thing left that can get us out of an EXER_W/P is a clear,
        // which only works in bidirectional mode
        if (aps_state->conf.mode != VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL && aps_state->command == VTSS_APPL_APS_COMMAND_CLEAR) {
            return APS_BASE_REQUEST_CLEAR;
        }

        // All other requests are overruled
        return APS_BASE_REQUEST_NR;
    }

    // The only state that is not covered above is NR_P.
    if (cur_state != VTSS_APPL_APS_PROT_STATE_NR_P) {
        T_EG(APS_TRACE_GRP_BASE, "%u: State = %d = %s", aps_state->inst, cur_state, aps_util_prot_state_to_str(cur_state));
    }

    return APS_BASE_REQUEST_NR;
}

//*****************************************************************************/
// APS_BASE_local_sm_run()
/******************************************************************************/
static vtss_appl_aps_prot_state_t APS_BASE_local_sm_run(aps_state_t *aps_state, aps_base_request_t local_request)
{
    vtss_appl_aps_prot_state_t cur_state = aps_state->status.prot_state;
    bool                       revertive = aps_state->conf.revertive;

    switch (local_request) {
    case APS_BASE_REQUEST_NR:
        return cur_state;

    case APS_BASE_REQUEST_CLEAR:
        // Can either go to A, E, F, J, P, or Q
        if (aps_state->status.p_state == VTSS_APPL_APS_DEFECT_STATE_SF) {
            // Go to F
            return VTSS_APPL_APS_PROT_STATE_SF_P;
        }

        if (aps_state->status.w_state == VTSS_APPL_APS_DEFECT_STATE_SF) {
            // Go to E
            return VTSS_APPL_APS_PROT_STATE_SF_W;
        }

        if (aps_state->status.p_state == VTSS_APPL_APS_DEFECT_STATE_SD) {
            // Go to Q
            return VTSS_APPL_APS_PROT_STATE_SD_P;
        }

        if (aps_state->status.w_state == VTSS_APPL_APS_DEFECT_STATE_SD) {
            // Go to P
            return VTSS_APPL_APS_PROT_STATE_SD_W;
        }

        if (revertive) {
            // Always go to A in revertive mode.
            return VTSS_APPL_APS_PROT_STATE_NR_W;
        }

        if (cur_state == VTSS_APPL_APS_PROT_STATE_FS      ||
            cur_state == VTSS_APPL_APS_PROT_STATE_MS_TO_P ||
            cur_state == VTSS_APPL_APS_PROT_STATE_EXER_P) {
            // When in non-revertive mode and coming from a mode where the P
            // port is active and W is standby, go to J
            return VTSS_APPL_APS_PROT_STATE_DNR;
        }

        // Go to A
        return VTSS_APPL_APS_PROT_STATE_NR_W;

    case APS_BASE_REQUEST_EXER:
        // It is already checked that we indeed are in a bidirectional mode, as
        // EXER is not allowed in a unidirectional mode.
        // We can go either to K or L.
        if (cur_state == VTSS_APPL_APS_PROT_STATE_DNR || cur_state == VTSS_APPL_APS_PROT_STATE_RR_P) {
            // Go to L
            return VTSS_APPL_APS_PROT_STATE_EXER_P;
        }

        // Go to K
        return VTSS_APPL_APS_PROT_STATE_EXER_W;

    case APS_BASE_REQUEST_WTR_EXPIRED:
        // We are currently in VTSS_APPL_APS_PROT_STATE_WTR. Go to A.
        return VTSS_APPL_APS_PROT_STATE_NR_W;

    case APS_BASE_REQUEST_MS_TO_W:
        // Go to H
        return VTSS_APPL_APS_PROT_STATE_MS_TO_W;

    case APS_BASE_REQUEST_MS_TO_P:
        return VTSS_APPL_APS_PROT_STATE_MS_TO_P;

    case APS_BASE_REQUEST_SD_W_OFF:
        // Can go to I, J or Q
        if (aps_state->status.p_state == VTSS_APPL_APS_DEFECT_STATE_SD) {
            // Go to Q
            return VTSS_APPL_APS_PROT_STATE_SD_P;
        }

        if (revertive) {
            // Go to I
            return VTSS_APPL_APS_PROT_STATE_WTR;
        }

        // Non-revertive.
        // Go to J
        return VTSS_APPL_APS_PROT_STATE_DNR;

    case APS_BASE_REQUEST_SD_W_ON:
        // Go to P
        return VTSS_APPL_APS_PROT_STATE_SD_W;

    case APS_BASE_REQUEST_SD_P_OFF:
        // Can go to A or P
        if (aps_state->status.w_state == VTSS_APPL_APS_DEFECT_STATE_SD) {
            // Go to P
            return VTSS_APPL_APS_PROT_STATE_SD_W;
        }

        // Go to A
        return VTSS_APPL_APS_PROT_STATE_NR_W;

    case APS_BASE_REQUEST_SD_P_ON:
        // Go to Q
        return VTSS_APPL_APS_PROT_STATE_SD_P;

    case APS_BASE_REQUEST_SF_W_OFF:
        // Go to I, J, P or Q. Here, it's not possible to go to SF-P, because
        // that's detected earlier.
        if (aps_state->status.p_state == VTSS_APPL_APS_DEFECT_STATE_SD) {
            // Go to Q
            return VTSS_APPL_APS_PROT_STATE_SD_P;
        }

        if (aps_state->status.w_state == VTSS_APPL_APS_DEFECT_STATE_SD) {
            // Go to P
            return VTSS_APPL_APS_PROT_STATE_SD_W;
        }

        if (revertive) {
            // Go to to I
            return VTSS_APPL_APS_PROT_STATE_WTR;
        }

        // Non-revertive
        // Go to J
        return VTSS_APPL_APS_PROT_STATE_DNR;

    case APS_BASE_REQUEST_SF_W_ON:
        // Go to E
        return VTSS_APPL_APS_PROT_STATE_SF_W;

    case APS_BASE_REQUEST_FS:
        // Go to D
        return VTSS_APPL_APS_PROT_STATE_FS;

    case APS_BASE_REQUEST_SF_P_OFF:
        // Go to A, E, P or Q.
        if (aps_state->status.w_state == VTSS_APPL_APS_DEFECT_STATE_SF) {
            // Go to E
            return VTSS_APPL_APS_PROT_STATE_SF_W;
        }

        if (aps_state->status.p_state == VTSS_APPL_APS_DEFECT_STATE_SD) {
            // Go to Q
            return VTSS_APPL_APS_PROT_STATE_SD_P;
        }

        if (aps_state->status.w_state == VTSS_APPL_APS_DEFECT_STATE_SD) {
            // Go to P
            return VTSS_APPL_APS_PROT_STATE_SD_W;
        }

        // Go to A
        return VTSS_APPL_APS_PROT_STATE_NR_W;

    case APS_BASE_REQUEST_SF_P_ON:
        // Go to F
        return VTSS_APPL_APS_PROT_STATE_SF_P;

    case APS_BASE_REQUEST_LO:
        return VTSS_APPL_APS_PROT_STATE_LO;

    default:
        T_EG(APS_TRACE_GRP_BASE, "%u: Invalid local APS reuquest (%d = %s)", aps_state->inst, local_request, APS_BASE_request_to_str(local_request));
        return cur_state;
    }
}

//*****************************************************************************/
// APS_BASE_far_end_sm_do_run()
/******************************************************************************/
static vtss_appl_aps_prot_state_t APS_BASE_far_end_sm_do_run(aps_state_t *aps_state, aps_base_request_t far_end_request, bool far_end_is_null)
{
    vtss_appl_aps_prot_state_t cur_state = aps_state->status.prot_state;
    bool                       revertive = aps_state->conf.revertive;

    T_DG(APS_TRACE_GRP_BASE, "%u: Running far-end SM", aps_state->inst);

    if (cur_state == VTSS_APPL_APS_PROT_STATE_LO) {
        // No remote request can change a lockout state
        return VTSS_APPL_APS_PROT_STATE_LO;
    }

    if (far_end_request == APS_BASE_REQUEST_LO) {
        return VTSS_APPL_APS_PROT_STATE_NR_W;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_SF_P) {
        // If still here, no other remote request can change a SF-P state.
        return VTSS_APPL_APS_PROT_STATE_SF_P;
    }

    if (far_end_request == APS_BASE_REQUEST_SF_P_ON) {
        return VTSS_APPL_APS_PROT_STATE_NR_W;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_FS) {
        // If still here, no other remote request can change a FS state.
        return VTSS_APPL_APS_PROT_STATE_FS;
    }

    if (far_end_request == APS_BASE_REQUEST_FS) {
        return VTSS_APPL_APS_PROT_STATE_NR_P;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_SF_W) {
        // No other remote request can change an SF-W state.
        return VTSS_APPL_APS_PROT_STATE_SF_W;
    }

    if (far_end_request == APS_BASE_REQUEST_SF_W_ON) {
        return VTSS_APPL_APS_PROT_STATE_NR_P;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_SD_W || cur_state == VTSS_APPL_APS_PROT_STATE_SD_P) {
        // No other far-end request can change current state.
        return cur_state;
    }

    if (far_end_request == APS_BASE_REQUEST_SD_P_ON) {
        return VTSS_APPL_APS_PROT_STATE_NR_P;
    }

    if (far_end_request == APS_BASE_REQUEST_SD_W_ON) {
        return VTSS_APPL_APS_PROT_STATE_NR_W;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_MS_TO_P) {
        if (far_end_request == APS_BASE_REQUEST_MS_TO_W) {
            // If far-end request is MS-to-W, the state diagrams say that it
            // should either remain in state G (MS_TO_P) or go to state A (NR_W)
            // if the far-end request is due to the simultaneous application of
            // a MS-to-W command at the far end, that is, no NR request
            // acknowledging the local MS state received previously from the far
            // end).
            // This has not been implemented, as we don't really look at and
            // respond to NR acks from the far end.
            // RBNTBD:
        }

        // No other far-end request can change this state
        return cur_state;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_MS_TO_W) {
        // No other far-end request can change this state
        return cur_state;
    }

    if (far_end_request == APS_BASE_REQUEST_MS_TO_P) {
        return VTSS_APPL_APS_PROT_STATE_NR_P;
    }

    if (far_end_request == APS_BASE_REQUEST_MS_TO_W) {
        return VTSS_APPL_APS_PROT_STATE_NR_W;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_WTR) {
        // No other far-end request can change change this state
        return VTSS_APPL_APS_PROT_STATE_WTR;
    }

    if (far_end_request == APS_BASE_REQUEST_WTR) {
        return VTSS_APPL_APS_PROT_STATE_NR_P;
    }

    if (cur_state == VTSS_APPL_APS_PROT_STATE_EXER_P || cur_state == VTSS_APPL_APS_PROT_STATE_EXER_W) {
        // No other far-end request can change this state
        return cur_state;
    }

    if (far_end_request == APS_BASE_REQUEST_EXER) {
        if (far_end_is_null) {
            // EXER_W
            if (cur_state == VTSS_APPL_APS_PROT_STATE_NR_W) {
                return VTSS_APPL_APS_PROT_STATE_RR_W;
            }
        } else {
            // EXER_P
            if (cur_state == VTSS_APPL_APS_PROT_STATE_DNR) {
                return VTSS_APPL_APS_PROT_STATE_RR_P;
            }
        }

        // Any other state results in staying in that state for this far-end
        // request.
        return cur_state;
    }

    if (far_end_request == APS_BASE_REQUEST_RR) {
        if (far_end_is_null) {
            // RR_W
            if (cur_state == VTSS_APPL_APS_PROT_STATE_RR_W) {
                return VTSS_APPL_APS_PROT_STATE_NR_W;
            }
        } else {
            // RR_P
            if (cur_state == VTSS_APPL_APS_PROT_STATE_RR_P) {
                return VTSS_APPL_APS_PROT_STATE_DNR;
            }
        }

        // Any other state results in staying in that state for this far-end
        // request.
        return cur_state;
    }

    if (far_end_request == APS_BASE_REQUEST_DNR) {
        if (revertive) {
            if (cur_state == VTSS_APPL_APS_PROT_STATE_NR_W) {
                return VTSS_APPL_APS_PROT_STATE_NR_P;
            }
        } else {
            // Non-revertive
            if (cur_state == VTSS_APPL_APS_PROT_STATE_NR_W || cur_state == VTSS_APPL_APS_PROT_STATE_NR_P || cur_state == VTSS_APPL_APS_PROT_STATE_RR_P) {
                return VTSS_APPL_APS_PROT_STATE_DNR;
            }
        }

        // Any other state results in staying in that state for this far-end
        // request.
        return cur_state;
    }

    if (far_end_request == APS_BASE_REQUEST_NR) {
        if (far_end_is_null) {
            // NR_W
            if (cur_state == VTSS_APPL_APS_PROT_STATE_NR_P || cur_state == VTSS_APPL_APS_PROT_STATE_RR_W) {
                return VTSS_APPL_APS_PROT_STATE_NR_W;
            }
        } else {
            // NR_P
            if (cur_state == VTSS_APPL_APS_PROT_STATE_NR_P) {
                // Can go to A, I, or J
                if (revertive) {
                    // Can go to NR_W og WTR. WTR if conditions in G.8031,
                    // clause 11.13 are fulfilled.
                    if (aps_state->coming_from_sf) {
                        // Start of the WTR timer happens by the caller of this
                        // function.
                        return VTSS_APPL_APS_PROT_STATE_WTR;
                    } else {
                        return VTSS_APPL_APS_PROT_STATE_NR_W;
                    }
                } else {
                    // Non-revertive always go to DNR.
                    return VTSS_APPL_APS_PROT_STATE_DNR;
                }
            }
        }

        // Any other state results in staying in that state for this far-end
        // request.
        return cur_state;
    }

    // If we are still here, something is wrong, because the code should have
    // handled all cases.
    T_EG(APS_TRACE_GRP_BASE, "%u: Not handled: cur_state = %s, far-end-request = %s", aps_state->inst, aps_util_prot_state_to_str(cur_state), APS_BASE_request_to_str(far_end_request));
    return cur_state;
}

//*****************************************************************************/
// APS_BASE_far_end_sm_run()
/******************************************************************************/
static vtss_appl_aps_prot_state_t APS_BASE_far_end_sm_run(aps_state_t *aps_state, aps_base_request_t far_end_request, bool far_end_is_null)
{
    vtss_appl_aps_prot_state_t result = APS_BASE_far_end_sm_do_run(aps_state, far_end_request, far_end_is_null);

    T_IG(APS_TRACE_GRP_BASE, "%u: far-end-request = %s:%s => state = %s->%s",
         aps_state->inst,
         APS_BASE_request_to_str(far_end_request),
         far_end_is_null ? "null" : "normal",
         aps_util_prot_state_to_str(aps_state->status.prot_state),
         aps_util_prot_state_to_str(result));

    return result;
}

//*****************************************************************************/
// APS_BASE_far_end_request_get()
/******************************************************************************/
static aps_base_request_t APS_BASE_far_end_request_get(aps_state_t *aps_state, bool &is_null)
{
    uint8_t *aps = aps_state->rx_aps_info;

    // If just one of them is 0, it's a null, indicating working.
    is_null = aps[1] == 0 || aps[2] == 0;

    switch (aps[0] & 0xF0) {
    case 0xF0:
        return APS_BASE_REQUEST_LO;

    case 0xE0:
        return APS_BASE_REQUEST_SF_P_ON;

    case 0xD0:
        return APS_BASE_REQUEST_FS;

    case 0xB0:
        return APS_BASE_REQUEST_SF_W_ON;

    case 0x90:
        return is_null ? APS_BASE_REQUEST_SD_W_ON  : APS_BASE_REQUEST_SD_P_ON;

    case 0x70:
        return is_null ? APS_BASE_REQUEST_MS_TO_W : APS_BASE_REQUEST_MS_TO_P;

    case 0x50:
        return APS_BASE_REQUEST_WTR;

    case 0x40:
        return APS_BASE_REQUEST_EXER;

    case 0x20:
        return APS_BASE_REQUEST_RR;

    case 0x10:
        return APS_BASE_REQUEST_DNR;

    case 0x00:
        return APS_BASE_REQUEST_NR;

    default:
        // Should have been filtered out already.
        T_EG(APS_TRACE_GRP_BASE, "%u: Unsupported APS (0x%02x)", aps_state->inst, aps[0]);
        return APS_BASE_REQUEST_NR;
    }
}

//*****************************************************************************/
// APS_BASE_history_update()
/******************************************************************************/
static void APS_BASE_history_update(aps_state_t *aps_state, aps_base_request_t local_request, aps_base_request_t far_end_request, vtss_appl_aps_prot_state_t new_state)
{
    if (aps_state->history.size() == 0                                     ||
        local_request   != aps_state->last_history_element.local_request   ||
        far_end_request != aps_state->last_history_element.far_end_request ||
        new_state       != aps_state->last_history_element.prot_state) {

        aps_state->last_history_element.local_request   = local_request;
        aps_state->last_history_element.far_end_request = far_end_request;
        aps_state->last_history_element.prot_state      = new_state;
        aps_state->last_history_element.event_time_ms   = vtss::uptime_milliseconds();

        aps_state->history.push(aps_state->last_history_element);
    }
}

//*****************************************************************************/
// APS_BASE_run_state_machine()
/******************************************************************************/
static void APS_BASE_run_state_machine(aps_state_t *aps_state)
{
    vtss_appl_aps_prot_state_t old_state, new_state;
    aps_base_request_t         local_request, far_end_request;
    bool                       far_end_is_null = true, dont_run_far_end_sm, clr_command;

    if (aps_state->command == VTSS_APPL_APS_COMMAND_FREEZE) {
        // Local Freeze, so no change in state
        return;
    }

    // Normalize in case someone has changed the configuration from revertive to
    // non-revertive or vice versa.
    if (aps_state->conf.revertive) {
        if (aps_state->status.prot_state == VTSS_APPL_APS_PROT_STATE_DNR) {
            aps_state->status.prot_state = VTSS_APPL_APS_PROT_STATE_WTR;
            aps_timer_start(aps_state->wtr_timer, aps_state->conf.wtr_secs * 1000, false);
        }
    } else {
        if (aps_state->status.prot_state == VTSS_APPL_APS_PROT_STATE_WTR) {
            aps_state->status.prot_state = VTSS_APPL_APS_PROT_STATE_DNR;
            aps_timer_stop(aps_state->wtr_timer);
        }
    }

    if (aps_state->status.dFOP_PM) {
        // Release the selector and stop.
        APS_BASE_selector_state_set(aps_state, MESA_EPS_SELECTOR_WORKING);
    }

    // We can not run the far end state machine if we haven't received a valid
    // far-end PDU (dFOP_TO set) or there is a provisioning mismatch (dFOP_PM
    // set) or we are in unidirectional mode (because in that case, we only use
    // the far end APS PDU for informational purposes).
    dont_run_far_end_sm = aps_state->status.dFOP_TO || aps_state->status.dFOP_PM || aps_state->conf.mode == VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL;

    old_state = aps_state->status.prot_state;

    // Once and for all, get the top-priority far-end request.
    far_end_request = dont_run_far_end_sm ? APS_BASE_REQUEST_NR : APS_BASE_far_end_request_get(aps_state, far_end_is_null);

    // First get the top-priority local request.
    local_request = APS_BASE_local_request_get(aps_state);

    if (local_request == APS_BASE_REQUEST_CLEAR    ||
        local_request == APS_BASE_REQUEST_SF_W_OFF ||
        local_request == APS_BASE_REQUEST_SF_P_OFF ||
        local_request == APS_BASE_REQUEST_SD_W_OFF ||
        local_request == APS_BASE_REQUEST_SD_P_OFF ||
        local_request == APS_BASE_REQUEST_WTR_EXPIRED) {
        // G.8031, clause 11.2.1.a:
        // If top-prio local request is one of the above, run the SM once based
        // on the local request in order to get an intermediate state.
        aps_state->status.prot_state = APS_BASE_local_sm_run(aps_state, local_request);
        T_IG(APS_TRACE_GRP_BASE, "%u: First run of local SM with local request = %s => state = %s->%s",
             aps_state->inst,
             APS_BASE_request_to_str(local_request),
             aps_util_prot_state_to_str(old_state),
             aps_util_prot_state_to_str(aps_state->status.prot_state));

        // G.8031, clause 11.2.1.a, cont'd:
        if (dont_run_far_end_sm || local_request == APS_BASE_REQUEST_SF_P_OFF) {
            // This intermediate state is the final state in case of clearance of
            // SF-P (or if we must not use the far-end information, because it's
            // unidirectional or not valid).
        } else {
            // Starting at this intermediate state, the last received far-end
            // request and the state-machine table for far-end requests are used
            // to calculate the final state.
            aps_state->status.prot_state = APS_BASE_far_end_sm_run(aps_state, far_end_request, far_end_is_null);
        }
    } else {
        // G.8031, clause 11.2.1.b
        // If the top priority local request is none of the above, then the
        // 'global priority logic' compares the top-priority local request with
        // the request of the last received "request/state" information based on
        // Table 11-1.
        if (dont_run_far_end_sm || APS_BASE_local_request_greater_than_or_equal_to_far_end_request(aps_state, local_request, far_end_request)) {
            // G.8031, clause 11.21.b.i
            // If the top-priority local request has higher or equal priority,
            // (or if we must not use the far-end information, because it's
            // unidirectional or invalid) the top-priority local request is used
            // with the state transition table for local requests in Annex A to
            // determine the final state.
            aps_state->status.prot_state = APS_BASE_local_sm_run(aps_state, local_request);
        } else {
            // G.8031, clause 11.21.b.ii
            // Otherwise, the request of the last received "request/state"
            // information is used with the state transition table for far-end
            // requests defined in Annex A to determine the final state.
            aps_state->status.prot_state = APS_BASE_far_end_sm_run(aps_state, far_end_request, far_end_is_null);
        }
    }

    new_state = aps_state->status.prot_state;

    // G.8031, clause 11.11 says: If a command is overridden by a condition or
    // APS request, that command is forgotten.
    clr_command = false;
    switch (aps_state->command) {
    case VTSS_APPL_APS_COMMAND_NR:
        // Already cleared
        break;

    case VTSS_APPL_APS_COMMAND_LO:
        // Cannot be overridden
        break;

    case VTSS_APPL_APS_COMMAND_FS:
        clr_command = local_request > APS_BASE_REQUEST_FS || far_end_request > APS_BASE_REQUEST_FS;
        break;

    case VTSS_APPL_APS_COMMAND_MS_TO_W:
    case VTSS_APPL_APS_COMMAND_MS_TO_P:
        clr_command = local_request > APS_BASE_REQUEST_MS_TO_P || far_end_request > APS_BASE_REQUEST_MS_TO_P;
        break;

    case VTSS_APPL_APS_COMMAND_EXER:
        clr_command = local_request > APS_BASE_REQUEST_EXER || far_end_request > APS_BASE_REQUEST_EXER;
        break;

    case VTSS_APPL_APS_COMMAND_CLEAR:
        // Always clear a clear
        clr_command = true;
        break;

    case VTSS_APPL_APS_COMMAND_FREEZE:
        // Unreachable
        break;

    case VTSS_APPL_APS_COMMAND_FREEZE_CLEAR:
        // Handled by aps_base_command_set()
        break;

    default:
        T_EG(APS_TRACE_GRP_BASE, "%u: Invalid command (%d)", aps_state->inst, aps_state->command);
        clr_command = true;
        break;
    }

    if (clr_command) {
        T_IG(APS_TRACE_GRP_BASE, "%u: Clearing command %s", aps_state->inst, aps_util_command_to_str(aps_state->command));
        aps_state->command = VTSS_APPL_APS_COMMAND_NR;
    }

    // This handles the case where both ends detect SF working OFF at the same
    // time. See G.8031 clause 11.13
    aps_state->coming_from_sf = aps_state->conf.revertive && old_state == VTSS_APPL_APS_PROT_STATE_SF_W && new_state == VTSS_APPL_APS_PROT_STATE_NR_P;

    // If we are entering WTR state the wtr timer must start
    if (old_state != new_state && new_state == VTSS_APPL_APS_PROT_STATE_WTR) {
        aps_timer_start(aps_state->wtr_timer, aps_state->conf.wtr_secs * 1000, false);
    }

    // If we are not in WTR state the wtr timer must stop
    if (new_state != VTSS_APPL_APS_PROT_STATE_WTR) {
        aps_timer_stop(aps_state->wtr_timer);
        aps_state->wtr_event = false;
    }

    T_IG(APS_TRACE_GRP_BASE, "%u: local-request = %s, far-end-request = %s:%s => state: %s->%s",
         aps_state->inst,
         APS_BASE_request_to_str(local_request),
         APS_BASE_request_to_str(far_end_request),
         far_end_is_null ? "null" : "normal",
         aps_util_prot_state_to_str(old_state),
         aps_util_prot_state_to_str(new_state));

    APS_BASE_history_update(aps_state, local_request, far_end_request, new_state);

    // Control the traffic selector
    APS_BASE_selector_state_set(aps_state, APS_BASE_calc_selector_state(new_state));

    if (aps_state->tx_laps_pdus()) {
        // Calculate transmitted APS for this state
        APS_BASE_tx_info_update(aps_state, new_state, false /* not shutting down */);

        // Calculate request FOP (fop_nr)
        APS_BASE_dfop_nr_update(aps_state);
    }
}

//*****************************************************************************/
// APS_BASE_mesa_port_conf_set()
/******************************************************************************/
static mesa_rc APS_BASE_mesa_port_conf_set(aps_state_t *aps_state, bool old_config = false)
{
    vtss_appl_aps_mode_t mode      = old_config ? aps_state->old_conf.mode : aps_state->conf.mode;
    mesa_port_no_t       w_port_no = old_config ? aps_state->old_w_port_no : aps_state->w_port_state->port_no;
    mesa_port_no_t       p_port_no = old_config ? VTSS_PORT_NO_NONE        : aps_state->p_port_state->port_no;
    mesa_eps_port_conf_t port_conf;
    mesa_rc              rc;

    memset(&port_conf, 0, sizeof(port_conf));
    port_conf.type    = mode == VTSS_APPL_APS_MODE_ONE_FOR_ONE ? MESA_EPS_PORT_1_FOR_1 : MESA_EPS_PORT_1_PLUS_1;
    port_conf.port_no = p_port_no;

    T_DG(APS_TRACE_GRP_API, "%u: mesa_eps_port_conf_set(w_port_no = %u, type = %u, p_port_no = %u)", aps_state->inst, w_port_no, port_conf.type, p_port_no);
    if ((rc = mesa_eps_port_conf_set(NULL, w_port_no, &port_conf)) != VTSS_RC_OK) {
        T_EG(APS_TRACE_GRP_API, "%u: mesa_eps_port_conf_set(%u, %u, %u) failed: %s", aps_state->inst, w_port_no, port_conf.type, p_port_no, error_txt(rc));
    }

    return rc;
}

//*****************************************************************************/
// APS_BASE_wtr_timeout()
// Fires when wtr_timer expires.
/******************************************************************************/
static void APS_BASE_wtr_timeout(aps_timer_t &timer, void *context)
{
    aps_state_t *aps_state = (aps_state_t *)context;

    VTSS_ASSERT(aps_state);

    aps_state->wtr_event = true;
    APS_BASE_run_state_machine(aps_state);
}

//*****************************************************************************/
// APS_BASE_hoff_timeout()
// Fires when hoff_w_timer or hoff_p_timer expires
/******************************************************************************/
static void APS_BASE_hoff_timeout(aps_timer_t &timer, void *context)
{
    aps_state_t                  *aps_state = (aps_state_t *)context;
    vtss_appl_aps_defect_state_t *state_ptr, new_state, old_state;
    bool                         cur_sf, cur_sd;

    VTSS_ASSERT(aps_state);

    if (&timer == &aps_state->hoff_w_timer) {
        // This timeout was caused by the hoff_w_timer
        state_ptr = &aps_state->status.w_state;
        cur_sf    = aps_state->sf_w;
        cur_sd    = aps_state->sd_w;
    } else {
        state_ptr = &aps_state->status.p_state;
        cur_sf    = aps_state->sf_p;
        cur_sd    = aps_state->sd_p;
    }

    old_state = *state_ptr;
    new_state = APS_BASE_calc_defect_state(cur_sf, cur_sd);

    if (new_state > old_state) {
        // Hold-off timeout can only go from less severe to more severe defects.
        // The opposite direction is handled directly in aps_base_sf_sd_set().
        *state_ptr = new_state;
        APS_BASE_run_state_machine(aps_state);
    }
}

//*****************************************************************************/
// APS_BASE_dfop_nr_timeout()
// Fires when dFOP_NR_timer expires
/******************************************************************************/
static void APS_BASE_dfop_nr_timeout(aps_timer_t &timer, void *context)
{
    aps_state_t *aps_state = (aps_state_t *)context;

    VTSS_ASSERT(aps_state);

    aps_state->status.dFOP_NR = TRUE;
}

//*****************************************************************************/
// APS_BASE_state_clear()
// Clears state maintained by base.
/******************************************************************************/
static void APS_BASE_state_clear(aps_state_t *aps_state)
{
    vtss_appl_aps_oper_state_t   oper_state;
    vtss_appl_aps_oper_warning_t oper_warning;

    aps_state->command = VTSS_APPL_APS_COMMAND_NR;

    // Reset ourselves, but keep oper_state and oper_warning.
    oper_state   = aps_state->status.oper_state;
    oper_warning = aps_state->status.oper_warning;
    memset(&aps_state->status, 0, sizeof(aps_state->status));
    aps_state->status.oper_state   = oper_state;
    aps_state->status.oper_warning = oper_warning;

    memset(aps_state->rx_aps_info, 0, sizeof(aps_state->rx_aps_info));
    memset(aps_state->tx_aps_info, 0, sizeof(aps_state->tx_aps_info));

    aps_state->sf_w            = false;
    aps_state->sf_p            = false;
    aps_state->sd_w            = false;
    aps_state->sd_p            = false;
    aps_state->wtr_event       = false;
    aps_state->coming_from_sf  = false;
    aps_state->ace_conf.id     = ACL_MGMT_ACE_ID_NONE;

    // aps_timer_init() also stops the timer if active.
    aps_timer_init(aps_state->wtr_timer,     "WTR",     aps_state->inst, APS_BASE_wtr_timeout,     aps_state);
    aps_timer_init(aps_state->hoff_w_timer,  "HoffW",   aps_state->inst, APS_BASE_hoff_timeout,    aps_state);
    aps_timer_init(aps_state->hoff_p_timer,  "HoffP",   aps_state->inst, APS_BASE_hoff_timeout,    aps_state);
    aps_timer_init(aps_state->dFOP_NR_timer, "dFOP-NR", aps_state->inst, APS_BASE_dfop_nr_timeout, aps_state);

    aps_state->history.clear();

    aps_laps_state_init(aps_state);
}

//*****************************************************************************/
// APS_BASE_do_deactivate()
/******************************************************************************/
static mesa_rc APS_BASE_do_deactivate(aps_state_t *aps_state, bool use_old_conf)
{
    mesa_rc rc, first_encountered_rc = VTSS_RC_OK;

    APS_BASE_selector_state_set(aps_state, MESA_EPS_SELECTOR_WORKING, use_old_conf);

    if ((rc = APS_BASE_mesa_port_conf_set(aps_state, use_old_conf)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    APS_BASE_tx_info_update(aps_state, VTSS_APPL_APS_PROT_STATE_NR_W, true /* shutting down */);

    // Remove our ACE.
    if ((rc = APS_BASE_ace_remove(aps_state)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    APS_BASE_state_clear(aps_state);

    return first_encountered_rc;
}

//*****************************************************************************/
// APS_BASE_do_activate()
/******************************************************************************/
static mesa_rc APS_BASE_do_activate(aps_state_t *aps_state, bool initial_sf_w, bool initial_sf_p)
{
    VTSS_RC(APS_BASE_mesa_port_conf_set(aps_state));

    APS_BASE_selector_state_set(aps_state, MESA_EPS_SELECTOR_WORKING);

    APS_BASE_state_clear(aps_state);

    // In most modes, we need APS from the remote end. Since we are activating
    // now, we haven't yet received any. However, if we are running
    // unidirectional, we don't expect any, so it can never be set.
    aps_state->status.dFOP_TO = aps_state->conf.mode != VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL;

    // Create a rule that captures and terminates L-APS PDUs
    VTSS_RC(APS_BASE_ace_add(aps_state));

    // Create a new L-APS PDU. This will not transmit any frames, only create the
    // PDU.
    VTSS_RC(aps_laps_tx_frame_update(aps_state, false));

    // Set the initial values of SF and SD
    // The following two function calls may end up calling
    // APS_BASE_run_state_machine(), which in turn might end up sending new
    // frames.
    aps_base_sf_sd_set(aps_state, true,  initial_sf_w, false);
    aps_base_sf_sd_set(aps_state, false, initial_sf_p, false);

    // If aps_base_sf_sd_set() did not call APS_BASE_run_state_machine(), we
    // need to do it here to get it all initialized.
    APS_BASE_run_state_machine(aps_state);

    return VTSS_RC_OK;
}

//*****************************************************************************/
// aps_base_deactivate()
/******************************************************************************/
mesa_rc aps_base_deactivate(aps_state_t *aps_state)
{
    return APS_BASE_do_deactivate(aps_state, true /* Use old_conf */);
}

//*****************************************************************************/
// aps_base_activate()
/******************************************************************************/
mesa_rc aps_base_activate(aps_state_t *aps_state, bool initial_sf_w, bool initial_sf_p)
{
    mesa_rc rc;

    if ((rc = APS_BASE_do_activate(aps_state, initial_sf_w, initial_sf_p)) != VTSS_RC_OK) {
        (void)APS_BASE_do_deactivate(aps_state, false /* Use current conf */);
    }

    return rc;
}

//*****************************************************************************/
// aps_base_command_set()
/******************************************************************************/
mesa_rc aps_base_command_set(aps_state_t *aps_state, vtss_appl_aps_command_t new_cmd)
{
    bool                    clear_first;
    vtss_appl_aps_command_t old_cmd = aps_state->command;

    T_IG(APS_TRACE_GRP_BASE, "%u: Command: %s->%s", aps_state->inst, aps_util_command_to_str(old_cmd), aps_util_command_to_str(new_cmd));

    if (new_cmd == old_cmd) {
        return VTSS_RC_OK;
    }

    clear_first = false;
    switch (old_cmd) {
    case VTSS_APPL_APS_COMMAND_LO:
    case VTSS_APPL_APS_COMMAND_FS:
    case VTSS_APPL_APS_COMMAND_MS_TO_W:
    case VTSS_APPL_APS_COMMAND_MS_TO_P:
    case VTSS_APPL_APS_COMMAND_EXER:
        clear_first = true;
        break;

    default:
        break;
    }

    if (clear_first && new_cmd != VTSS_APPL_APS_COMMAND_CLEAR) {
        // When changing from some command-driven state to some other command-
        // driven state, we must clear the old state in the state machine before
        // going to the new state.
        aps_state->command = VTSS_APPL_APS_COMMAND_CLEAR;
        APS_BASE_run_state_machine(aps_state);
    }

    if (new_cmd == VTSS_APPL_APS_COMMAND_FREEZE_CLEAR) {
        // This un-freezes the state machine.
        aps_state->command = VTSS_APPL_APS_COMMAND_NR;
    } else {
        aps_state->command = new_cmd;
    }

    APS_BASE_run_state_machine(aps_state);

    return VTSS_RC_OK;
}

//*****************************************************************************/
// aps_base_matching_update()
/******************************************************************************/
mesa_rc aps_base_matching_update(aps_state_t *aps_state)
{
    // Possibly update classified VID or MEG level in our ACE.
    return APS_BASE_ace_update(aps_state);
}

//*****************************************************************************/
// aps_base_statistics_clear()
/******************************************************************************/
void aps_base_statistics_clear(aps_state_t *aps_state)
{
    aps_laps_statistics_clear(aps_state);
}

//*****************************************************************************/
// aps_base_rx_frame()
// Only invoked if either W- or P-port matches.
/******************************************************************************/
void aps_base_rx_frame(aps_state_t *aps_state, const uint8_t *frm, const mesa_packet_rx_info_t *const rx_info)
{
    uint8_t aps[APS_LAPS_DATA_LENGTH];
    bool    use_aps_info;
    bool    a_cleared, a_expected_to_be_set;
    bool    b_set,     b_expected_to_be_set;
    bool    d_set,     d_expected_to_be_set;

    use_aps_info = aps_laps_rx_frame(aps_state, frm, rx_info, aps);

    if (rx_info->port_no == aps_state->w_port_state->port_no) {
        // Received a L-APS PDU on the working port. Set the dFOP-CM defect.
        // See G.8021, table 6-2. The aps_base_rx_timeout() will be invoked with
        // working == true whenever it's time to clear it again.
        aps_state->status.dFOP_CM = true;

        // Nothing more to do.
        return;
    }

    // Received on the protect port.
    if (!use_aps_info) {
        // Didn't pass the validation criteria.
        return;
    }

    // Received a valid L-APS PDU on protect port. No timeout (anymore).
    aps_state->status.dFOP_TO = false;

    // Expect that everything goes OK in the validation process below.
    // What we do with the A and D bits are not exactly according to G.8031,
    // which suggests (in clause 11.4), that mismatches in these bits should be
    // handled by the APS-expecting/bidirectional side will fall back to
    // unidirectional switching. That must be a future implementation.
    // The following will also catch when
    // ABD == 0b001, 0b010, 0b011, or 0b110, which are all invalid combinations.
    aps_state->status.dFOP_PM = false;

    if (aps_state->conf.mode != VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL) {
        // Only set dFOP_PM when in bidirectional mode
        a_cleared = (aps[0] & 0x08) == 0;
        a_expected_to_be_set = aps_state->conf.mode != VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL;
        if (a_cleared && a_expected_to_be_set) {
            // We expect A to be set in bidirectional switching, but it is actually
            // cleared.
            T_IG(APS_TRACE_GRP_FRAME_RX, "%u: Unexpected A-bit. APS[0] = 0x%02x", aps_state->inst, aps[0]);
            aps_state->status.dFOP_PM = true;
        }

        b_set = (aps[0] & 0x04) != 0;
        b_expected_to_be_set = aps_state->conf.mode == VTSS_APPL_APS_MODE_ONE_FOR_ONE;
        if (b_set != b_expected_to_be_set) {
            // We expected B to be set but it's not or we didn't expect B to be set,
            // but it is.
            T_IG(APS_TRACE_GRP_FRAME_RX, "%u: Unexpected B-bit. APS[0] = 0x%02x", aps_state->inst, aps[0]);
            aps_state->status.dFOP_PM = true;
        }

        d_set = (aps[0] & 0x02) != 0;
        d_expected_to_be_set = aps_state->conf.mode != VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL;
        if (d_set != d_expected_to_be_set) {
            // This is the same story as for the A-bit.
            T_IG(APS_TRACE_GRP_FRAME_RX, "%u: Unexpected D-bit. APS[0] = 0x%02x", aps_state->inst, aps[0]);
            aps_state->status.dFOP_PM = true;
        }
    }

    memcpy(aps_state->rx_aps_info, aps, sizeof(aps_state->rx_aps_info));
    APS_BASE_dfop_nr_update(aps_state);

    APS_BASE_request_internal_to_external_set(aps_state, false /* Rx part */);
    APS_BASE_run_state_machine(aps_state);
}

//*****************************************************************************/
// aps_base_rx_timeout()
// Invoked by aps_laps 17.5 seconds after the last L-APS PDU has been seen on
// either the Working or Protect port.
/******************************************************************************/
void aps_base_rx_timeout(aps_state_t *aps_state, bool working)
{
    if (working) {
        // It's been 17.5 seconds since we last received a L-APS PDU on working.
        // Clear the defect.
        aps_state->status.dFOP_CM = false;
        return;
    }

    if (aps_state->conf.mode != VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL) {
        // We don't expect APS PDUs in unidirectional 1+1, so we cannot set a
        // timeout. Otherwise a timeout on  the protect interface indicates that
        // we haven't received a L-APS PDU for the past 17.5 seconds after the
        // protect port is supposed to be up and working.
        aps_state->status.dFOP_TO = true;
    }
}

//*****************************************************************************/
// aps_base_tx_frame_update()
/******************************************************************************/
void aps_base_tx_frame_update(aps_state_t *aps_state)
{
    // This might require changes to our ACE rule as well as the L-APS PDU.
    APS_BASE_ace_update(aps_state);

    (void)aps_laps_tx_frame_update(aps_state, true /* Do transmit the updated PDU right away */);
}

//*****************************************************************************/
// aps_base_exercise_sm()
/******************************************************************************/
void aps_base_exercise_sm(aps_state_t *aps_state)
{
    // Some configuration change has occurred in the platform module which may
    // cause a state change (currently only revertive to non-revertive or vice
    // versa).
    APS_BASE_run_state_machine(aps_state);
}

//*****************************************************************************/
// aps_base_sf_sd_set()
/******************************************************************************/
void aps_base_sf_sd_set(aps_state_t *aps_state, bool working, bool new_sf, bool new_sd)
{
    bool                         &old_sf   = working ? aps_state->sf_w           : aps_state->sf_p;
    bool                         &old_sd   = working ? aps_state->sd_w           : aps_state->sd_p;
    aps_timer_t                  *timer    = working ? &aps_state->hoff_w_timer  : &aps_state->hoff_p_timer;
    vtss_appl_aps_defect_state_t old_state = working ? aps_state->status.w_state : aps_state->status.p_state;
    vtss_appl_aps_defect_state_t new_state;

    // Propagate it to L-APS
    aps_laps_sf_sd_set(aps_state, working, new_sf, new_sd);

    if (new_sf == old_sf && new_sd == old_sd) {
        // Nothing has happened since last time. Done.
        return;
    }

    new_state = APS_BASE_calc_defect_state(new_sf, new_sd);

    old_sf = new_sf;
    old_sd = new_sd;

    if (new_state > old_state && aps_state->conf.hold_off_msecs != 0) {
        // The state enumerations are selected so that the higher numbers, the
        // more severe, so here, we now have a more severe defect.
        // According to G.8031, clause 11.12, we should (re-)start the hold-off
        // timer, which runs APS_BASE_hoff_timeout() when it expires.
        aps_timer_start(*timer, aps_state->conf.hold_off_msecs, false);
        return;
    }

    if (new_state < old_state || aps_state->conf.hold_off_msecs == 0) {
        // Either the hold-off timer is disabled or we are going from a more
        // severe to a less severe state (that is, SF to SD, SF to OK, or SD to
        // OK). Either way, we stop the hold-off timer, and pass the new state
        // directly into the public state used by the state machine.
        aps_timer_stop(*timer);
        if (working) {
            aps_state->status.w_state = new_state;
        } else {
            aps_state->status.p_state = new_state;
        }
    }

    // Time to run the state machine, which uses both the public state
    APS_BASE_run_state_machine(aps_state);
}

/******************************************************************************/
// aps_base_history_dump()
/******************************************************************************/
void aps_base_history_dump(aps_state_t *aps_state, uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...), bool print_hdr)
{
    aps_base_history_itr_t hist_itr;
    uint32_t               cnt;

    if (print_hdr) {
        pr(session_id, "Now = " VPRI64u " ms\n", vtss::uptime_milliseconds());
        pr(session_id, "Inst   # Time [ms]      Local Req   Far-end Req Prot State\n");
        pr(session_id, "---- --- -------------- ----------- ----------- ----------\n");
    }

    cnt = 1;
    for (hist_itr = aps_state->history.begin(); hist_itr != aps_state->history.end(); ++hist_itr) {
        pr(session_id, "%4u %3u " VPRI64Fu("14") " %-11s %-11s %s\n",
           aps_state->inst,
           cnt++,
           hist_itr->event_time_ms,
           APS_BASE_request_to_str(hist_itr->local_request),
           APS_BASE_request_to_str(hist_itr->far_end_request),
           aps_util_prot_state_to_str(hist_itr->prot_state));
    }

    if (cnt == 1) {
        pr(session_id, "%4u <No events registered yet>\n", aps_state->inst);
    }
}

/******************************************************************************/
// aps_base_rules_dump()
// Dumps active rules.
/******************************************************************************/
void aps_base_rule_dump(aps_state_t *aps_state, uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...), bool print_hdr)
{
    acl_entry_conf_t   ace_conf;
    mesa_ace_counter_t ace_counter;
    char               ingr_buf[MGMT_PORT_BUF_SIZE];
    mesa_rc            rc;

    if (print_hdr) {
        pr(session_id, "Inst ACE ID Ingress Ports VLAN Hit count\n");
        pr(session_id, "---- ------ ------------- ---- --------------\n");
    }

    if (aps_state->ace_conf.id == ACL_MGMT_ACE_ID_NONE) {
        pr(session_id, "%4u <None>\n", aps_state->inst);
    }

    if ((rc = acl_mgmt_ace_get(ACL_USER_APS, aps_state->ace_conf.id, &ace_conf, &ace_counter, FALSE)) != VTSS_RC_OK) {
        T_EG(APS_TRACE_GRP_BASE, "%u: acl_mgmt_ace_get(%u) failed: %s", aps_state->inst, aps_state->ace_conf.id, error_txt(rc));
        return;
    }

    pr(session_id, "%4u %6d %13s %4d %14u\n",
       aps_state->inst,
       ace_conf.id,
       mgmt_iport_list2txt(ace_conf.port_list, ingr_buf),
       ace_conf.vid.value,
       ace_counter);
}

