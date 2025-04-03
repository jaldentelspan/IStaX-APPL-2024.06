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

#include <vtss/appl/cfm.hxx>
#include "cfm_base.hxx"
#include "cfm_trace.h"
#include "cfm_ccm.hxx"
#include "cfm_lock.hxx"
#include "cfm_api.h"    /* For CFM_ETYPE          */
#include "acl_api.h"    /* For acl_mgmt_ace_XXX() */
#include "misc_api.h"
#include "port_api.h"   /* For port_count_max()   */
#include <microchip/ethernet/switch/api.h>

static cfm_timer_t     CFM_BASE_copy_ccm_to_cpu_timer;
static uint32_t        CFM_BASE_cap_port_cnt;
static mesa_iflow_id_t CFM_BASE_dummy_iflow_id;

// Any arbitrary number, since we have to allocate VCE IDs ourselves, and since
// this may otherwise clash with the VCL module. Currently, there is no way to
// ensure that this module's rules are placed consecutively in the TCAM, because
// the API does not use the VCE ID for ordering, only for identification. The
// ordering comes from the ID argument one gives to mesa_vce_add(), which must
// either be an existing ID or MESA_VCE_ID_LAST.
// What is odd is that it's not possible to obtain the already used IDs from the
// API, and it's not possible to obtain the configuration either. Anyway, this
// module uses VCE IDs in the range [CFM_VCE_ID_START; CFM_VCE_ID_END - 1].
#define CFM_VCE_ID_START 0xFFFF0000
#define CFM_VCE_ID_END   CFM_VCE_ID_NONE
static mesa_vce_id_t CFM_BASE_vce_id = CFM_VCE_ID_START - 1;

// Save the VCE configuration as the value in the map, because the API doesn't
// have a get()-method.
typedef vtss::Map<cfm_base_vce_id_key_t, mesa_vce_t> cfm_base_vce_id_map_t;
typedef cfm_base_vce_id_map_t::iterator cfm_base_vce_id_itr_t;
static cfm_base_vce_id_map_t CFM_BASE_vce_id_map;

// See also discussion for VCE IDs above
// Currently no other modules use TCEs, so we are not that hard hit.
#define CFM_TCE_ID_START 0xFFFF0000
#define CFM_TCE_ID_END   CFM_TCE_ID_NONE
static mesa_tce_id_t CFM_BASE_tce_id = CFM_TCE_ID_START - 1;

// Save the TCE configuration as the value in the map, because the API doesn't
// have a get()-method.
typedef vtss::Map<cfm_base_tce_id_key_t, mesa_tce_t> cfm_base_tce_id_map_t;
typedef cfm_base_tce_id_map_t::iterator cfm_base_tce_id_itr_t;
static cfm_base_tce_id_map_t CFM_BASE_tce_id_map;

// We keep track of all the ACL rules we insert, so that we can do it in the
// correct order. We insert ACEs for both match-and-copy-to-CPU and for leak-
// prevention purposes.
static cfm_base_ace_map_t CFM_BASE_ace_map;

/******************************************************************************/
// CFM_BASE_vce_insertion_order_to_str()
/******************************************************************************/
static const char *CFM_BASE_vce_insertion_order_to_str(cfm_base_vce_insertion_order_t insertion_order)
{
    switch (insertion_order) {
    case CFM_BASE_VCE_INSERTION_ORDER_PORT_DOWN_MEP:
        return "Port-Down-MEP";

    case CFM_BASE_VCE_INSERTION_ORDER_VLAN_DOWN_MEP:
        return "VLAN-Down-MEP";

    case CFM_BASE_VCE_INSERTION_ORDER_PORT_DOWN_MEP_OTHER_PORTS:
        return "Leak-Port-MEP";

    case CFM_BASE_VCE_INSERTION_ORDER_VLAN_DOWN_MEP_OTHER_PORTS:
        return "Leak-VLAN-MEP";

    default:
        T_EG(CFM_TRACE_GRP_BASE, "Unknown insertion order (%d)", insertion_order);
        return "Unknown";
    }
}

/******************************************************************************/
// CFM_BASE_tce_insertion_order_to_str()
/******************************************************************************/
static const char *CFM_BASE_tce_insertion_order_to_str(cfm_base_tce_insertion_order_t insertion_order)
{
    switch (insertion_order) {
    case CFM_BASE_TCE_INSERTION_ORDER_VLAN_DOWN_MEP:
        return "VLAN-Down-MEP";

    case CFM_BASE_TCE_INSERTION_ORDER_PORT_DOWN_MEP:
        return "Port-Down-MEP";

    default:
        T_EG(CFM_TRACE_GRP_BASE, "Unknown insertion order (%d)", insertion_order);
        return "Unknown";
    }
}

/******************************************************************************/
// CFM_BASE_ace_insertion_order_to_str()
/******************************************************************************/
static const char *CFM_BASE_ace_insertion_order_to_str(cfm_base_ace_insertion_order_t insertion_order)
{
    switch (insertion_order) {
    case CFM_BASE_ACE_INSERTION_ORDER_PORT_DOWN_MEP:
        return "Port-Down-MEP";

    case CFM_BASE_ACE_INSERTION_ORDER_VLAN_DOWN_MEP:
        return "VLAN-Down-MEP";

    case CFM_BASE_ACE_INSERTION_ORDER_LEAK_DOWN_MEP:
        return "Leak-Down-MEP";

    default:
        T_EG(CFM_TRACE_GRP_BASE, "Unknown insertion order (%d)", insertion_order);
        return "Unknown";
    }
}

/******************************************************************************/
// CFM_BASE_oam_detect_to_str()
/******************************************************************************/
static const char *CFM_BASE_oam_detect_to_str(mesa_oam_detect_t oam_detect)
{
    switch (oam_detect) {
    case MESA_OAM_DETECT_NONE:
        return "None";

    case MESA_OAM_DETECT_UNTAGGED:
        return "Untagged";

    case MESA_OAM_DETECT_SINGLE_TAGGED:
        return "Single";

    case MESA_OAM_DETECT_DOUBLE_TAGGED:
        return "Double";

    case MESA_OAM_DETECT_TRIPLE_TAGGED:
        return "Triple";

    default:
        T_EG(CFM_TRACE_GRP_BASE, "Unknown oam_detect (%d)", oam_detect);
        return "Unknown";
    }
}

/******************************************************************************/
// cfm_base_vce_id_key_t::operator<
// Used for sorting entries in CFM_BASE_vce_id_map.
/******************************************************************************/
static bool operator<(const cfm_base_vce_id_key_t &lhs, const cfm_base_vce_id_key_t &rhs)
{
    // First sort by insertion order
    if (lhs.insertion_order != rhs.insertion_order) {
        return lhs.insertion_order < rhs.insertion_order;
    }

    // Then by VCE_ID
    return lhs.vce_id < rhs.vce_id;
}

/******************************************************************************/
// CFM_BASE_vce_key_del()
// Deletes an existing VCE ID from our internal VCE map.
/******************************************************************************/
static mesa_rc CFM_BASE_vce_key_del(cfm_mep_state_t *mep_state, const cfm_base_vce_id_key_t &key)
{
    cfm_base_vce_id_itr_t itr;

    if ((itr = CFM_BASE_vce_id_map.find(key)) == CFM_BASE_vce_id_map.end()) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Internal error: Cannot lookup <%s, 0x%x> in VCE map", mep_state->key, CFM_BASE_vce_insertion_order_to_str(key.insertion_order), key.vce_id);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    CFM_BASE_vce_id_map.erase(itr);
    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_vce_itr_get()
// Gets an existing or allocates a new VCE ID.
/******************************************************************************/
static mesa_rc CFM_BASE_vce_itr_get(cfm_mep_state_t *mep_state, cfm_base_vce_id_key_t &key, cfm_base_vce_id_itr_t &itr)
{
    bool already_wrapped = false, unused_found, alloc_new_id = true;

    if (key.vce_id != CFM_VCE_ID_NONE) {
        // Should be findable in the map. It may be that the insertion order has
        // changed because the MEP changed from being a Port MEP to being a VLAN
        // MEP or vice versa. That's why we must look up by ID and not by key.
        for (itr = CFM_BASE_vce_id_map.begin(); itr != CFM_BASE_vce_id_map.end(); ++itr) {
            if (itr->first.vce_id != key.vce_id) {
                continue;
            }

            if (itr->first.insertion_order == key.insertion_order) {
                // Found the one caller is looking for
                return VTSS_RC_OK;
            }

            // Gotta delete the old one and create a new with the same VCE ID
            // but another insertion order.
            alloc_new_id = false;
            break;
        }

        if (alloc_new_id) {
            // If the loop above didn't clear alloc_new_id, the VCE ID the
            // caller was looking for is not in the map.
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Internal error: Cannot lookup <%s, 0x%x> in VCE map", mep_state->key, CFM_BASE_vce_insertion_order_to_str(key.insertion_order), key.vce_id);
            return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
        } else {
            // Delete the old from the map. A new will be created shortly with
            // the new insertion order.
            CFM_BASE_vce_id_map.erase(itr);
        }
    }

    // The caller wants to allocate a new VCE ID. We have our own little pool of
    // such IDs, where each of them may or may not already be used. The
    // following will attempt to find an unused.
    while (alloc_new_id) {
        if (++CFM_BASE_vce_id == CFM_VCE_ID_END) {
            if (already_wrapped) {
                T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Internal error: No free VCE IDs in our range (size of map = %zu, range size = %u)", mep_state->key, CFM_BASE_vce_id_map.size(), CFM_VCE_ID_END - CFM_VCE_ID_START);
                return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
            }

            CFM_BASE_vce_id = CFM_VCE_ID_START;
            already_wrapped = true;
        }

        // Check to see if this key is already used
        unused_found = true;
        for (itr = CFM_BASE_vce_id_map.begin(); itr != CFM_BASE_vce_id_map.end(); ++itr) {
            if (itr->first.vce_id == CFM_BASE_vce_id) {
                // Already in use. Find another
                unused_found = false;
                break;
            }
        }

        if (unused_found) {
            key.vce_id = CFM_BASE_vce_id;
            break;
        }
    }

    // Create a new entry
    if ((itr = CFM_BASE_vce_id_map.get(key)) == CFM_BASE_vce_id_map.end()) {
        // Out of memory
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Out of memory when attempting to allocate a new VCE entry (%d:0x%x)", mep_state->key, key.insertion_order, key.vce_id);
        return VTSS_APPL_CFM_RC_OUT_OF_MEMORY;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_vce_insertion_point_get()
/******************************************************************************/
static mesa_vce_id_t CFM_BASE_vce_insertion_point_get(cfm_mep_state_t *mep_state, cfm_base_vce_id_itr_t &itr)
{
    cfm_base_vce_id_itr_t next_itr = itr;

    ++next_itr;
    if (next_itr == CFM_BASE_vce_id_map.end()) {
        return MESA_VCE_ID_LAST;
    }

    return next_itr->first.vce_id;
}

/******************************************************************************/
// CFM_BASE_vce_do_deactivate()
/******************************************************************************/
static mesa_rc CFM_BASE_vce_do_deactivate(cfm_mep_state_t *mep_state, int which)
{
    cfm_base_vce_id_key_t *vce_key = &mep_state->vce_keys[which];
    mesa_rc               first_encountered_rc = VTSS_RC_OK, rc;

    if (vce_key->vce_id == CFM_VCE_ID_NONE) {
        return VTSS_RC_OK;
    }

    // Delete it from the API
    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_vce_del(%s:0x%x)", mep_state->key, CFM_BASE_vce_insertion_order_to_str(vce_key->insertion_order), vce_key->vce_id);
    if ((rc = mesa_vce_del(nullptr, vce_key->vce_id)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_vce_del(%d:0x%x) failed: %s", mep_state->key, vce_key->insertion_order, vce_key->vce_id, error_txt(rc));
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = VTSS_APPL_CFM_RC_INTERNAL_ERROR;
        }
    }

    // Delete it from our map
    if ((rc = CFM_BASE_vce_key_del(mep_state, *vce_key)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    vce_key->vce_id = CFM_VCE_ID_NONE;

    return first_encountered_rc;
}

/******************************************************************************/
// CFM_BASE_vce_deactivate()
/******************************************************************************/
static mesa_rc CFM_BASE_vce_deactivate(cfm_mep_state_t *mep_state)
{
    int     i;
    mesa_rc first_encountered_rc = VTSS_RC_OK, rc;

    for (i = 0; i < ARRSZ(mep_state->vce_keys); i++) {
        if ((rc = CFM_BASE_vce_do_deactivate(mep_state, i)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// CFM_BASE_vce_do_activate()
// which == 0 => Update/create VCE for the MEP's residence port
// which == 1 => Update/create VCE for the other ports whose PVID matches ours
// which == 2 => Update/create VCE for the other ports where the frame tag matches ours
/******************************************************************************/
static mesa_rc CFM_BASE_vce_do_activate(cfm_mep_state_t *mep_state, mesa_vce_t *new_vce_conf, int which)
{
    cfm_base_vce_id_itr_t vce_id_itr;
    cfm_base_vce_id_key_t *vce_key = &mep_state->vce_keys[which];
    cfm_port_state_t      *port_state;
    mesa_vce_id_t         insertion_point;
    mesa_vid_t            mep_classified_vid = mep_state->classified_vid_get();
    mesa_port_no_t        port_no;
    bool                  tagged, is_s_port, create_rule = false;
    mesa_rc               rc;

    // Clear the port members of new_vce_conf no matter which of the rules we
    // are configuring.
    new_vce_conf->key.port_list.clear_all();

    switch (which) {
    case 0:
        // We are creating/updating a rule for the MEP's residence port.
        // This rule will match CFM frames on a particular level and lower and
        // on a particular VLAN and TPID and map them to the iflow, which points
        // to the VOE. It will not (and shall not) match on a particular PCP or
        // DEI value.

        tagged    = mep_state->is_tagged();
        is_s_port = mep_state->port_state->vlan_conf.port_type == VTSS_APPL_VLAN_PORT_TYPE_S || mep_state->port_state->vlan_conf.port_type == VTSS_APPL_VLAN_PORT_TYPE_S_CUSTOM;

        new_vce_conf->key.port_list[mep_state->port_state->port_no] = TRUE;

        // If VLAN Down-MEP, we match on a particular VID and a particular Tag
        // type (corresponding to the port's type).
        new_vce_conf->key.tag.tagged    = tagged              ? MESA_VCAP_BIT_1 : MESA_VCAP_BIT_0;
        new_vce_conf->key.tag.s_tag     = tagged && is_s_port ? MESA_VCAP_BIT_1 : MESA_VCAP_BIT_0;
        new_vce_conf->key.tag.vid.value = mep_state->mep_conf->mep_conf.vlan ? mep_state->mep_conf->mep_conf.vlan : mep_state->ma_conf->vlan;
        new_vce_conf->key.tag.vid.mask  = tagged ? 0xFFFF : 0x0000;

        // Let all frames with a level equal to or smaller than our level hit
        // the VOE, so that it can discard those with a level smaller than ours
        // and handle those with a level equal to ours (MEG Level-filtering).
        new_vce_conf->key.frame.etype.mel.mask  = ~(VTSS_BIT(mep_state->md_conf->level) - 1);

        // The actions are to point to an IFLOW that points to our OAM-handling
        // VOE and mark the frame as OAM behind zero or one tag.
        new_vce_conf->action.flow_id    = mep_state->iflow_id;
        new_vce_conf->action.oam_detect = tagged ? MESA_OAM_DETECT_SINGLE_TAGGED : MESA_OAM_DETECT_UNTAGGED;

        // Port MEPs are hit before VLAN MEPs.
        vce_key->insertion_order = mep_state->is_port_mep() ? CFM_BASE_VCE_INSERTION_ORDER_PORT_DOWN_MEP : CFM_BASE_VCE_INSERTION_ORDER_VLAN_DOWN_MEP;

        create_rule = true;
        break;

    case 1:
        // We are creating/updating a rule for all other ports but the MEP's
        // residence port. This time it's a rule that matches untagged frames
        // that get classified to the MEP's classified VID.

        // The rule we are configuring is for untagged frames.
        new_vce_conf->key.tag.tagged   = MESA_VCAP_BIT_0;
        new_vce_conf->key.tag.s_tag    = MESA_VCAP_BIT_ANY;
        new_vce_conf->key.tag.vid.mask = 0x0000;

        // Match on all MEG level, because on egress it will be filtered or not
        // filtered based on MEG level anyway.
        new_vce_conf->key.frame.etype.mel.mask  = 0x00;

        // The iflow, we set here is a dummy iflow, which is needed because we
        // need the API to set a non-exposed bit that causes the frame to be
        // marked as independent MEL, to prevent a possible Port MEP from
        // level filtering a frame that is not indended for that MEP, because
        // it's in a different VLAN - remember - if a frame doesn't hit a
        // service MEP, it will always hit the ingress port's Port MEP/Port VOE
        // whether or not that VOE is supposed to handle the frame. By setting
        // a dummy iflow, we can prevent the port MEP from level filtering the
        // frame.
        // When we start supporting Up-MIPs, we allocate an iflow for the
        // Up-MIP that is supposed to handle the frame, and then it's that iflow
        // we use in this rule. When the frame comes to the CPU, because it has
        // hit an Up-MIP, the chip has replaced the original ingress port number
        // with the Up-MIP's port number, so that the only way to distinguish
        // frames that hit the Up-MIP from frames that hit a corresponding
        // Down-MEP is through the iflow number embedded in the IFH.
        new_vce_conf->action.flow_id = CFM_BASE_dummy_iflow_id;

        // Let egress detect OAM behind 0 tags
        new_vce_conf->action.oam_detect = MESA_OAM_DETECT_UNTAGGED;

        // Fill in port list
        new_vce_conf->key.port_list.clear_all();
        for (port_no = 0; port_no < CFM_BASE_cap_port_cnt; port_no++) {
            port_state = &CFM_port_state[port_no];

            if (port_state->vlan_conf.pvid != mep_classified_vid) {
                // An untagged frame will not get classified to the MEP's
                // classified VID, so this port should not be included in the
                // VCE.
                continue;
            }

            if (port_state->vlan_conf.frame_type == MESA_VLAN_FRAME_TAGGED) {
                // This port will only accept tagged frames, so no need to create a
                // rule that will match on untagged frames.
                continue;
            }

            create_rule = true;
            new_vce_conf->key.port_list[port_no] = TRUE;
        }

        if (create_rule) {
            // Let port MEPs be hit before VLAN MEPs (I don't think it really
            // matters).
            vce_key->insertion_order = mep_state->is_port_mep() ? CFM_BASE_VCE_INSERTION_ORDER_PORT_DOWN_MEP_OTHER_PORTS : CFM_BASE_VCE_INSERTION_ORDER_VLAN_DOWN_MEP_OTHER_PORTS;
        }

        break;

    case 2:
        // We are creating/updating a rule for all other ports but the MEP's
        // residence port. This time it's a rule that matches tagged frames that
        // get classified to the MEP's classified VID.

        // The rule we are configuring is for untagged frames.
        new_vce_conf->key.tag.tagged    = MESA_VCAP_BIT_1;
        new_vce_conf->key.tag.s_tag     = MESA_VCAP_BIT_ANY;
        new_vce_conf->key.tag.vid.value = mep_classified_vid;
        new_vce_conf->key.tag.vid.mask  = 0xFFFF;

        // Match on all MEG level, because on egress it will be filtered or not
        // filtered based on MEG level anyway.
        new_vce_conf->key.frame.etype.mel.mask  = 0x00;

        // See comment in "case 1".
        new_vce_conf->action.flow_id = CFM_BASE_dummy_iflow_id;

        // Let egress detect OAM behind 1 tag
        new_vce_conf->action.oam_detect = MESA_OAM_DETECT_SINGLE_TAGGED;

        // Fill in port list
        new_vce_conf->key.port_list.clear_all();
        for (port_no = 0; port_no < CFM_BASE_cap_port_cnt; port_no++) {
            port_state = &CFM_port_state[port_no];

            if (port_state->vlan_conf.frame_type == MESA_VLAN_FRAME_UNTAGGED) {
                // This port will only accept untagged frames, so no need to
                // create a rule that will match on tagged frames.
                continue;
            }

            create_rule = true;
            new_vce_conf->key.port_list[port_no] = TRUE;
        }

        if (create_rule) {
            // Let port MEPs be hit before VLAN MEPs (I don't think it really
            // matters).
            vce_key->insertion_order = mep_state->is_port_mep() ? CFM_BASE_VCE_INSERTION_ORDER_PORT_DOWN_MEP_OTHER_PORTS : CFM_BASE_VCE_INSERTION_ORDER_VLAN_DOWN_MEP_OTHER_PORTS;
        }

        break;

    default:
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Unknown which (%d)", mep_state->key, which);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    if (!create_rule) {
        // We are not going to create a rule, but if a rule already exists for
        // this type of entry, we need to delete it.
        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Deleting possibly existing VCE for %s:0x%x", mep_state->key, which, new_vce_conf->id);
        (void)CFM_BASE_vce_do_deactivate(mep_state, which);
        return VTSS_RC_OK;
    }

    // Get an iterator to the current rule or create a new one
    VTSS_RC(CFM_BASE_vce_itr_get(mep_state, *vce_key, vce_id_itr));

    new_vce_conf->id = vce_id_itr->first.vce_id;

    if (memcmp(&vce_id_itr->second, new_vce_conf, sizeof(vce_id_itr->second)) == 0) {
        // No change
        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: No VCE conf change for %d:0x%x", mep_state->key, which, new_vce_conf->id);
        return VTSS_RC_OK;
    }

    // Find the insertion point, which must be before the next VCE
    insertion_point = CFM_BASE_vce_insertion_point_get(mep_state, vce_id_itr);

    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_vce_add(0x%x, %d:0x%x)", mep_state->key, insertion_point, which, new_vce_conf->id);
    if ((rc = mesa_vce_add(nullptr, insertion_point, new_vce_conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_vce_add(0x%x, %d:0x%x) failed: %s", mep_state->key, insertion_point, which, new_vce_conf->id, error_txt(rc));
        return VTSS_APPL_CFM_RC_HW_RESOURCES;
    }

    // Save the new configuration
    vce_id_itr->second = *new_vce_conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_vce_activate()
// A VCE becomes a CLM rule (formerly known as MCE rule).
// VCEs point to an IFLOW, which essentially is an ISDX.
// An IFLOW points to a VOE.
/******************************************************************************/
static mesa_rc CFM_BASE_vce_activate(cfm_mep_state_t *mep_state)
{
    mesa_vce_t new_vce_conf; // 144 bytes long
    int        i;
    mesa_rc    rc;

    // Initialize the VCE configuration
    if ((rc = mesa_vce_init(nullptr, MESA_VCE_TYPE_ETYPE, &new_vce_conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_vce_init() failed: %s", mep_state->key, error_txt(rc));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    // Initialize VCE configuration fields common to all three VCE rules we can
    // install for this MEP.

    // Match on CFM Ethertype.
    new_vce_conf.key.type                       = MESA_VCE_TYPE_ETYPE;
    new_vce_conf.key.frame.etype.etype.value[0] = (CFM_ETYPE >> 8) & 0xFF;
    new_vce_conf.key.frame.etype.etype.value[1] = (CFM_ETYPE >> 0) & 0xFF;
    new_vce_conf.key.frame.etype.etype.mask[0]  = 0xFF;
    new_vce_conf.key.frame.etype.etype.mask[1]  = 0xFF;

    // We only support OAM behind up to one tag, so it can never have an inner
    // tag.
    new_vce_conf.key.inner_tag.tagged = MESA_VCAP_BIT_0;

    // We control what to hit through mel.mask, not mel.value.
    new_vce_conf.key.frame.etype.mel.value = 0x00;

    // Create up to three VCE rules: One for the port on which the MEP resides,
    // and possibly one for untagged frames on other ports and possibly one for
    // tagged frames on other ports.
    for (i = 0; i < ARRSZ(mep_state->vce_keys); i++) {
        VTSS_RC(CFM_BASE_vce_do_activate(mep_state, &new_vce_conf, i));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// cfm_base_tce_id_key_t::operator<
// Used for sorting entries in CFM_BASE_tce_id_map.
/******************************************************************************/
static bool operator<(const cfm_base_tce_id_key_t &lhs, const cfm_base_tce_id_key_t &rhs)
{
    // First sort by insertion order
    if (lhs.insertion_order != rhs.insertion_order) {
        return lhs.insertion_order < rhs.insertion_order;
    }

    // Then by TCE_ID
    return lhs.tce_id < rhs.tce_id;
}

/******************************************************************************/
// CFM_BASE_tce_key_del()
// Deletes an existing TCE ID from our internal TCE map.
/******************************************************************************/
static mesa_rc CFM_BASE_tce_key_del(cfm_mep_state_t *mep_state, const cfm_base_tce_id_key_t &key)
{
    cfm_base_tce_id_itr_t itr;

    if ((itr = CFM_BASE_tce_id_map.find(key)) == CFM_BASE_tce_id_map.end()) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Internal error: Cannot lookup <%s, %u> in TCE map", mep_state->key, CFM_BASE_tce_insertion_order_to_str(key.insertion_order), key.tce_id);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    CFM_BASE_tce_id_map.erase(itr);
    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_tce_itr_get()
// Gets an existing or allocates a new TCE ID.
/******************************************************************************/
static mesa_rc CFM_BASE_tce_itr_get(cfm_mep_state_t *mep_state, cfm_base_tce_id_key_t &key, cfm_base_tce_id_itr_t &itr)
{
    bool already_wrapped = false, unused_found, alloc_new_id = true;

    if (key.tce_id != CFM_TCE_ID_NONE) {
        // Should be findable in the map. It may be that the insertion order has
        // changed because the MEP changed from being a Port MEP to being a VLAN
        // MEP or vice versa. That's why we must look up by ID and not by key.
        for (itr = CFM_BASE_tce_id_map.begin(); itr != CFM_BASE_tce_id_map.end(); ++itr) {
            if (itr->first.tce_id != key.tce_id) {
                continue;
            }

            if (itr->first.insertion_order == key.insertion_order) {
                // Found the one caller is looking for
                return VTSS_RC_OK;
            }

            // Gotta delete the old one and create a new with the same TCE ID
            // but another insertion order.
            alloc_new_id = false;
            break;
        }

        if (alloc_new_id) {
            // If the loop above didn't clear alloc_new_id, the TCE ID the
            // caller was looking for is not in the map.
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Internal error: Cannot lookup <%s, 0x%x> in TCE map", mep_state->key, CFM_BASE_tce_insertion_order_to_str(key.insertion_order), key.tce_id);
            return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
        } else {
            // Delete the old from the map. A new will be created shortly with
            // the new insertion order.
            CFM_BASE_tce_id_map.erase(itr);
        }
    }

    // The caller wants to allocate a new TCE ID. We have our own little pool of
    // such IDs, where each of them may or may not already be used. The
    // following will attempt to find an unused.
    while (alloc_new_id) {
        if (++CFM_BASE_tce_id == CFM_TCE_ID_END) {
            if (already_wrapped) {
                T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Internal error: No free TCE IDs in our range (size of map = %zu, range size = %u)", mep_state->key, CFM_BASE_tce_id_map.size(), CFM_TCE_ID_END - CFM_TCE_ID_START);
                return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
            }

            CFM_BASE_tce_id = CFM_TCE_ID_START;
            already_wrapped = true;
        }

        // Check to see if this key is already used
        unused_found = true;
        for (itr = CFM_BASE_tce_id_map.begin(); itr != CFM_BASE_tce_id_map.end(); ++itr) {
            if (itr->first.tce_id == CFM_BASE_tce_id) {
                // Already in use. Find another
                unused_found = false;
                break;
            }
        }

        if (unused_found) {
            key.tce_id = CFM_BASE_tce_id;
            break;
        }
    }

    // Create a new entry
    if ((itr = CFM_BASE_tce_id_map.get(key)) == CFM_BASE_tce_id_map.end()) {
        // Out of memory
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Out of memory when attempting to allocate a new TCE entry", mep_state->key);
        return VTSS_APPL_CFM_RC_OUT_OF_MEMORY;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_tce_insertion_point_get()
/******************************************************************************/
static mesa_tce_id_t CFM_BASE_tce_insertion_point_get(cfm_mep_state_t *mep_state, cfm_base_tce_id_itr_t &itr)
{
    cfm_base_tce_id_itr_t next_itr = itr;

    ++next_itr;
    if (next_itr == CFM_BASE_tce_id_map.end()) {
        return MESA_TCE_ID_LAST;
    }

    return next_itr->first.tce_id;
}

/******************************************************************************/
// CFM_BASE_tce_do_deactivate()
/******************************************************************************/
static mesa_rc CFM_BASE_tce_do_deactivate(cfm_mep_state_t *mep_state, int which)
{
    cfm_base_tce_id_key_t *tce_key = &mep_state->tce_keys[which];
    mesa_rc               first_encountered_rc = VTSS_RC_OK, rc;

    if (tce_key->tce_id == CFM_TCE_ID_NONE) {
        return VTSS_RC_OK;
    }

    // Delete it from the API
    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_tce_del(%d:0x%x)", mep_state->key, tce_key->insertion_order, tce_key->tce_id);
    if ((rc = mesa_tce_del(nullptr, tce_key->tce_id)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_tce_del(%d:0x%x) failed: %s", mep_state->key, tce_key->insertion_order, tce_key->tce_id, error_txt(rc));
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = VTSS_APPL_CFM_RC_INTERNAL_ERROR;
        }
    }

    // Delete it from our map
    if ((rc = CFM_BASE_tce_key_del(mep_state, *tce_key)) != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_RC_OK) {
            first_encountered_rc = rc;
        }
    }

    tce_key->tce_id = CFM_TCE_ID_NONE;

    return first_encountered_rc;
}

/******************************************************************************/
// CFM_BASE_tce_deactivate()
/******************************************************************************/
static mesa_rc CFM_BASE_tce_deactivate(cfm_mep_state_t *mep_state)
{
    int     i;
    mesa_rc first_encountered_rc = VTSS_RC_OK, rc;

    for (i = 0; i < ARRSZ(mep_state->tce_keys); i++) {
        if ((rc = CFM_BASE_tce_do_deactivate(mep_state, i)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// CFM_BASE_tce_do_activate()
// TCE = Tag Control Entry (Egress), a.k.a. ES0 rule.
//
// Whether we need 0, 1, or 2 TCEs depends on both the chip and the MEP-type.
// Lu26 do not use TCEs at all, so we only get invoked on Serval-1 and JR2.
//
// Port MEPs:
//   Serval-1: We don't need any TCEs at all, because all frames will by default
//   hit the Port VOE. On Serval-1, all Port MEPs run in the same MEG level and
//   any given VLAN MEP runs in a higher MEG level, so frames ingressing on
//   other ports that don't hit a VLAN MEP will hit the Port MEP and get
//   level-filtered in case it's an OAM frame (marked on ingress with VCEs).
//   JR-2: By default all frames will also hit the Port VOE on JR2. However,
//   since we allow a port MEP on one port to be in a MEG level higher than e.g.
//   a VLAN MEP (or Port MEP) on another port, we do need rules to mark the
//   frame as independent_mel = false.
//   Furthermore, since we can have both a Port MEP and a VLAN MEP on the same
//   port in the same VID (MEG level of VLAN MEP must be higher than that of the
//   Port MEP, though), we must have an ISDX-based rule that allows for
//   injecting CCMs and other OAM frames from the CPU.
//   So on JR2, we install two TCEs.
//
// VLAN MEPs:
//   Both Serval-1 and JR2: We need a TCE that matches the MEP's classified VID
//   and marks OAM frames as independent_mel = false to get them handled by the
//   VLAN VOE. This will prevent leaking of OAM frames at a VOE's level and
//   lower when these frames arrive on another port than the port on which the
//   MEP is installed.
//   Serval-1:
//   We need one additional TCE for CPU-injection, because of technicalities
//   when looping LBM (I'm not 100% aware of the problem). When having such an
//   ISDX-based rule, we use that for injecting CCMs and other OAM frames from
//   the CPU.
//
// which == 0 => Create classified VID TCE - if needed.
// which == 1 => Create IFLOW-based TCE - if needed.
/******************************************************************************/
static mesa_rc CFM_BASE_tce_do_activate(cfm_mep_state_t *mep_state, mesa_tce_t *new_tce_conf, int which)
{
    cfm_base_tce_id_itr_t tce_id_itr;
    cfm_base_tce_id_key_t *tce_key = &mep_state->tce_keys[which];
    mesa_tce_id_t         insertion_point;
    bool                  free_it = false;
    mesa_rc               rc;

    if (mep_state->is_port_mep()) {
        if (mep_state->global_state->has_vop_v1) {
            // Serval-1 doesn't need any TCEs for Port MEPs at all.
            free_it = true;
        }
    } else {
        // VLAN MEP.
        if (mep_state->global_state->has_vop_v2 && which == 1) {
            // JR2 doesn't need an ISDX-based TCE for VLAN MEPs.
            free_it = true;
        }
    }

    if (free_it) {
        // Done if this is not a Port VOE. Free an existing TCE (if MEP changed
        // from a Port to a VLAN MEP).
        (void)CFM_BASE_tce_do_deactivate(mep_state, which);
        return VTSS_RC_OK;
    }

    tce_key->insertion_order = mep_state->is_port_mep() ? CFM_BASE_TCE_INSERTION_ORDER_PORT_DOWN_MEP : CFM_BASE_TCE_INSERTION_ORDER_VLAN_DOWN_MEP;

    switch (which) {
    case 0:
        // Create a classified-VID-based TCE that helps us prevent leaking and
        // on which we inject frames on JR2 if it's a VLAN MEP.
        // The idea is to have these classified-VID TCEs to follow the port's
        // tagging for frames that come from other ports. This is controlled
        // through having action.tag.tpid set to MESA_TPID_SEL_PORT.
        new_tce_conf->action.tag.tpid    = MESA_TPID_SEL_PORT;
        new_tce_conf->key.vid            = mep_state->classified_vid_get();
        new_tce_conf->key.flow_enable    = false;
        new_tce_conf->key.flow_id        = MESA_IFLOW_ID_NONE;
        break;

    case 1:
        // ISDX-based rule used for CPU-controlled injection of OAM frames.
        // It is used for VLAN MEPs on Serval-1 and Port MEPs on JR2.
        // If it's a Port MEP, we let the CPU-injection code take care of
        // tagging OAM frames sent by us, so we always force it to not tag
        // anything.
        // If it's a VLAN MEP, the CPU-injection code does not take care of
        // any tagging, so we must choose the tag-control according to whether
        // the MEP's VLAN will get tagged on egress or not.
        new_tce_conf->action.tag.tpid  = mep_state->is_port_mep() || !mep_state->classified_vid_gets_tagged() ? MESA_TPID_SEL_NONE : MESA_TPID_SEL_PORT;
        new_tce_conf->key.vid          = 0; // Not used when flow_enable is true.
        new_tce_conf->key.flow_enable  = true;
        new_tce_conf->key.flow_id      = mep_state->iflow_id;
        break;

    default:
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Unknown which (%d)", mep_state->key, which);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    // Get an iterator to the current rule or create a new one
    VTSS_RC(CFM_BASE_tce_itr_get(mep_state, *tce_key, tce_id_itr));

    new_tce_conf->id = tce_id_itr->first.tce_id;

    if (memcmp(&tce_id_itr->second, new_tce_conf, sizeof(tce_id_itr->second)) == 0) {
        // No change
        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: No TCE conf change for %d:0x%x", mep_state->key, which, new_tce_conf->id);
        return VTSS_RC_OK;
    }

    // Find the insertion point, which must be before the next TCE
    insertion_point = CFM_BASE_tce_insertion_point_get(mep_state, tce_id_itr);

    T_DG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_tce_add(0x%x, %d:0x%x)", mep_state->key, insertion_point, which, new_tce_conf->id);
    if ((rc = mesa_tce_add(nullptr, insertion_point, new_tce_conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_tce_add(0x%x, %d:0x%x) failed: %s", mep_state->key, insertion_point, which, new_tce_conf->id, error_txt(rc));
        return VTSS_APPL_CFM_RC_HW_RESOURCES;
    }

    // Save the new configuration
    tce_id_itr->second = *new_tce_conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_tce_activate()
/******************************************************************************/
static mesa_rc CFM_BASE_tce_activate(cfm_mep_state_t *mep_state)
{
    mesa_tce_t new_tce_conf; // 88 bytes long;
    int        i;
    mesa_rc    rc;

    T_DG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_tce_init()", mep_state->key);
    if ((rc = mesa_tce_init(nullptr, &new_tce_conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_tce_init() failed: %s", mep_state->key, error_txt(rc));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    // Common for both TCEs:
    new_tce_conf.key.port_list[mep_state->port_state->port_no] = TRUE;
    new_tce_conf.action.tag.pcp_sel = MESA_PCP_SEL_PORT;    // Let the port decide PCP
    new_tce_conf.action.tag.dei_sel = MESA_DEI_SEL_PORT;    // Let the port decide DEI
    new_tce_conf.action.tag.map_id  = MESA_QOS_MAP_ID_NONE; // Don't use a QoS map.
    new_tce_conf.action.flow_id     = mep_state->eflow_id;  // This is what makes it hit the correct VOE (VLAN MEPs) or mark the frame as independent_mel == false (Port MEPs on JR2).

    for (i = 0; i < ARRSZ(mep_state->tce_keys); i++) {
        VTSS_RC(CFM_BASE_tce_do_activate(mep_state, &new_tce_conf, i));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_eflow_deactivate()
/******************************************************************************/
static mesa_rc CFM_BASE_eflow_deactivate(cfm_mep_state_t *mep_state)
{
    mesa_rc rc;

    if (mep_state->eflow_id == MESA_EFLOW_ID_NONE) {
        return VTSS_RC_OK;
    }

    T_DG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_eflow_free(%u)", mep_state->key, mep_state->eflow_id);
    if ((rc = mesa_eflow_free(nullptr, mep_state->eflow_id)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_eflow_free(%u) failed: %s", mep_state->key, mep_state->eflow_id, error_txt(rc));
        rc = VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    mep_state->eflow_id = MESA_EFLOW_ID_NONE;
    return rc;
}

/******************************************************************************/
// CFM_BASE_eflow_activate()
/******************************************************************************/
static mesa_rc CFM_BASE_eflow_activate(cfm_mep_state_t *mep_state)
{
    mesa_eflow_conf_t old_eflow_conf, new_eflow_conf;
    mesa_rc           rc;

    if (mep_state->eflow_id == MESA_EFLOW_ID_NONE) {
        // Allocate a new EFLOW
        if ((rc = mesa_eflow_alloc(nullptr, &mep_state->eflow_id)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_eflow_alloc() failed: %s", mep_state->key, error_txt(rc));
            return VTSS_APPL_CFM_RC_HW_RESOURCES;
        }

        T_DG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_eflow_alloc() => %u", mep_state->key, mep_state->eflow_id);
    }

    // Get old configuration
    T_DG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_eflow_conf_get(%u)", mep_state->key, mep_state->eflow_id);
    if ((rc = mesa_eflow_conf_get(nullptr, mep_state->eflow_id, &old_eflow_conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_eflow_conf_get() failed: %s", mep_state->key, error_txt(rc));
        rc = VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    memset(&new_eflow_conf, 0, sizeof(new_eflow_conf));
    new_eflow_conf.voe_idx = mep_state->voe_idx;

    if (mep_state->global_state->has_vop_v2) {
        // Doesn't exist on vop_v1
        new_eflow_conf.voi_idx = MESA_VOI_IDX_NONE;
    }

    if (memcmp(&old_eflow_conf, &new_eflow_conf, sizeof(old_eflow_conf)) == 0) {
        return VTSS_RC_OK;
    }

    T_DG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_eflow_conf_set(%u)", mep_state->key, mep_state->eflow_id);
    if ((rc = mesa_eflow_conf_set(nullptr, mep_state->eflow_id, &new_eflow_conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_eflow_conf_get() failed: %s", mep_state->key, error_txt(rc));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_iflow_deactivate()
/******************************************************************************/
static mesa_rc CFM_BASE_iflow_deactivate(cfm_mep_state_t *mep_state)
{
    mesa_rc rc;

    if (mep_state->iflow_id == MESA_IFLOW_ID_NONE) {
        return VTSS_RC_OK;
    }

    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_iflow_free(%u)", mep_state->key, mep_state->iflow_id);
    if ((rc = mesa_iflow_free(nullptr, mep_state->iflow_id)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_iflow_free(%u) failed: %s", mep_state->key, mep_state->iflow_id, error_txt(rc));
        rc = VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    mep_state->iflow_id = MESA_IFLOW_ID_NONE;
    return rc;
}

/******************************************************************************/
// CFM_BASE_iflow_activate()
/******************************************************************************/
static mesa_rc CFM_BASE_iflow_activate(cfm_mep_state_t *mep_state)
{
    mesa_iflow_conf_t old_iflow_conf, new_iflow_conf;
    mesa_rc           rc;

    if (mep_state->iflow_id == MESA_IFLOW_ID_NONE) {
        // Allocate a new IFLOW
        if ((rc = mesa_iflow_alloc(nullptr, &mep_state->iflow_id)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_iflow_alloc() failed: %s", mep_state->key, error_txt(rc));
            return VTSS_APPL_CFM_RC_HW_RESOURCES;
        }

        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_iflow_alloc() => %u", mep_state->key, mep_state->iflow_id);
    }

    // Get old configuration
    if ((rc = mesa_iflow_conf_get(nullptr, mep_state->iflow_id, &old_iflow_conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_iflow_conf_get() failed: %s", mep_state->key, error_txt(rc));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    memset(&new_iflow_conf, 0, sizeof(new_iflow_conf));
    new_iflow_conf.voe_idx = mep_state->voe_idx;

    if (mep_state->global_state->has_vop_v2) {
        new_iflow_conf.voi_idx = MESA_VOI_IDX_NONE;
    }

    if (memcmp(&old_iflow_conf, &new_iflow_conf, sizeof(old_iflow_conf)) == 0) {
        return VTSS_RC_OK;
    }

    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_iflow_conf_set(%u)", mep_state->key, mep_state->iflow_id);
    if ((rc = mesa_iflow_conf_set(nullptr, mep_state->iflow_id, &new_iflow_conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_iflow_conf_set() failed: %s", mep_state->key, error_txt(rc));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_voe_deactivate()
/******************************************************************************/
static mesa_rc CFM_BASE_voe_deactivate(cfm_mep_state_t *mep_state)
{
    mesa_rc rc;

    if (mep_state->voe_idx == MESA_VOE_IDX_NONE) {
        return VTSS_RC_OK;
    }

    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_voe_free(%u)", mep_state->key, mep_state->voe_idx);
    if ((rc = mesa_voe_free(nullptr, mep_state->voe_idx)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_voe_free(%u) failed: %s", mep_state->key, mep_state->voe_idx, error_txt(rc));
        rc = VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    mep_state->voe_idx = MESA_VOE_IDX_NONE;
    return rc;
}

/******************************************************************************/
// CFM_BASE_voe_activate()
/******************************************************************************/
static mesa_rc CFM_BASE_voe_activate(cfm_mep_state_t *mep_state)
{
    mesa_voe_allocation_t new_voe_alloc;
    mesa_voe_conf_t       old_voe_conf, new_voe_conf;
    mesa_rc               rc = VTSS_RC_OK;

    memset(&new_voe_alloc, 0, sizeof(new_voe_alloc));

    // We pick a Port VOE if the MA dictates port MEPs or a Service VOE if the
    // MA dictates VLAN MEPs.
    new_voe_alloc.type      = mep_state->is_port_mep() ? MESA_VOE_TYPE_PORT : MESA_VOE_TYPE_SERVICE;
    new_voe_alloc.port      = mep_state->port_state->port_no;
    new_voe_alloc.direction = mep_state->mep_conf->mep_conf.direction == VTSS_APPL_CFM_DIRECTION_DOWN ? MESA_OAM_DIRECTION_DOWN : MESA_OAM_DIRECTION_UP;

    if (mep_state->voe_idx == MESA_VOE_IDX_NONE || memcmp(&mep_state->voe_alloc, &new_voe_alloc, sizeof(mep_state->voe_alloc))) {
        // There's a change in the VOE configuration. Free the old one - if one
        // has been allocated. Don't care about the return value.
        (void)CFM_BASE_voe_deactivate(mep_state);

        // Allocate a new
        if ((rc = mesa_voe_alloc(nullptr, &new_voe_alloc, &mep_state->voe_idx)) != VTSS_RC_OK) {
            // This trace error may be demoted, but I need to see it in action first
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_voe_alloc() failed: %s", mep_state->key, error_txt(rc));
            mep_state->voe_idx = MESA_VOE_IDX_NONE; // Better safe than sorry
            return VTSS_APPL_CFM_RC_HW_RESOURCES;
        }

        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_voe_alloc() => %u", mep_state->key, mep_state->voe_idx);
    }

    mep_state->voe_alloc = new_voe_alloc;

    if ((rc = mesa_voe_conf_get(nullptr, mep_state->voe_idx, &old_voe_conf)) != VTSS_RC_OK) {
        // This trace error may be demoted, but I need to see it in action first
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_voe_conf_get() failed: %s", mep_state->key, error_txt(rc));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    memset(&new_voe_conf, 0, sizeof(new_voe_conf));
    new_voe_conf.enable          = mep_state->mep_conf->mep_conf.admin_active;
    new_voe_conf.unicast_mac     = cfm_base_smac_get(mep_state);
    new_voe_conf.meg_level       = mep_state->md_conf->level;
    new_voe_conf.dmac_check_type = MESA_VOE_DMAC_CHECK_BOTH;
    new_voe_conf.loop_iflow_id   = MESA_IFLOW_ID_NONE;
    new_voe_conf.block_mel_high  = false; // VOP_V2
    new_voe_conf.tagging         = mep_state->is_tagged() ? MESA_PORT_MAX_TAGS_ONE : MESA_PORT_MAX_TAGS_NONE; // LAN966x only.

    if (memcmp(&old_voe_conf, &new_voe_conf, sizeof(old_voe_conf)) != 0) {
        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_voe_conf_set(%u)", mep_state->key, mep_state->voe_idx);
        if ((rc = mesa_voe_conf_set(nullptr, mep_state->voe_idx, &new_voe_conf)) != VTSS_RC_OK) {
            // This trace error may be demoted, but I need to see it in action first
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_voe_conf_set(%u) failed: %s", mep_state->key, mep_state->voe_idx, error_txt(rc));
            return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// cfm_base_ace_map_key_t::fmt()
// Used for tracing.
/******************************************************************************/
size_t fmt(vtss::ostream &stream, const vtss::Fmt &fmt, const cfm_base_ace_map_key_t *key)
{
    char   buf[256];
    size_t sz;

    sz = snprintf(buf, sizeof(buf), "%s::%u::%u", CFM_BASE_ace_insertion_order_to_str(key->insertion_order), key->vid, key->level);
    return stream.write(buf, buf + MIN(sz, sizeof(buf)));
}

/******************************************************************************/
// cfm_base_ace_map_key_t::operator<
// Used for sorting entries in CFM_BASE_ace_map.
/******************************************************************************/
static bool operator<(const cfm_base_ace_map_key_t &lhs, const cfm_base_ace_map_key_t &rhs)
{
    // First sort by insertion order
    if (lhs.insertion_order != rhs.insertion_order) {
        return lhs.insertion_order < rhs.insertion_order;
    }

    // Then by vid
    if (lhs.vid != rhs.vid) {
        return lhs.vid < rhs.vid;
    }

    // And finally by level
    return lhs.level < rhs.level;
}

/******************************************************************************/
// CFM_BASE_ace_insertion_point_get()
/******************************************************************************/
static mesa_ace_id_t CFM_BASE_ace_insertion_point_get(const cfm_base_ace_map_key_t &key)
{
    cfm_base_ace_itr_t itr;

    if ((itr = CFM_BASE_ace_map.greater_than(key)) == CFM_BASE_ace_map.end()) {
        // Insert last
        return ACL_MGMT_ACE_ID_NONE;
    }

    return itr->second.ace_ids[0];
}

/******************************************************************************/
// CFM_BASE_ace_remove()
/******************************************************************************/
static mesa_rc CFM_BASE_ace_remove(cfm_mep_state_t *mep_state, cfm_base_ace_itr_t itr)
{
    mesa_ace_id_t ace_id;
    int           i;
    mesa_rc       first_encountered_rc = VTSS_RC_OK, rc;

    // Remote the ACEs from the API
    for (i = 0; i < ARRSZ(itr->second.ace_ids); i++) {
        ace_id = itr->second.ace_ids[i];

        if (ace_id == ACL_MGMT_ACE_ID_NONE) {
            continue;
        }

        // Delete it from the ACL module
        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: acl_mgmt_ace_del(%s: 0x%x)", mep_state->key, itr->first, ace_id);
        if ((rc = acl_mgmt_ace_del(ACL_USER_CFM, ace_id)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: acl_mgmt_ace_del(%s: 0x%x) failed: %s", mep_state->key, itr->first, ace_id, error_txt(rc));
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = VTSS_APPL_CFM_RC_INTERNAL_ERROR;
            }
        }

        itr->second.ace_ids[i] = ACL_MGMT_ACE_ID_NONE;
    }

    return first_encountered_rc;
}

/******************************************************************************/
// CFM_BASE_ace_add()
// itr->first.insertion_order == PORT_DOWN_MEP or VLAN_DOWN_MEP:
//    Create ACEs for the MEP's residence port (copy-to-CPU rules)
// itr->first.insertion_order == OTHER_PORTS:
//    Create ACEs for the other ports (leak-prevention rules)
/******************************************************************************/
static mesa_rc CFM_BASE_ace_add(cfm_mep_state_t *mep_state, cfm_base_ace_itr_t itr)
{
    acl_entry_conf_t new_ace_conf; // 228 bytes
    mesa_ace_id_t    ace_id, insertion_point;
    uint8_t          val[ARRSZ(cfm_base_ace_map_value_t::ace_ids)], msk[ARRSZ(cfm_base_ace_map_value_t::ace_ids)];
    bool             entry_used[ARRSZ(cfm_base_ace_map_value_t::ace_ids)];
    int              i;
    mesa_rc          rc;

    // We don't have a H/W VOE to detect changes to CCM frames and provide
    // interrupts to us whenever such changes occur.
    // Instead, we must copy all CCM PDUs to the CPU using ACL rules.
    // An ACL rule does not support direct match on a MEP's MD/MEG level or
    // below. An analysis shows that it requires up to three ACEs to match on
    // particular levels and below.

    // Also, we don't have H/W VOE to detect leaking and discard OAM frames, so
    // we must also install ACL rules for that, which also takes up to three
    // ACEs.

    memset(entry_used, 0, sizeof(entry_used));
    entry_used[0] = true;

    // The mask/value principle follows this formula:
    // hit = (configured_mask & configured_val) == (configured_mask & received_val).
    switch (mep_state->md_conf->level) {
    case 0:
        // Get only level 0 to CPU.
        // The following matches level 0b000 (0).
        val[0] = 0x0; // 0b000
        msk[0] = 0x7; // 0b111
        break;

    case 1:
        // Get levels [0; 1] to CPU.
        // The following matches level 0b00X, that is, 0b000 and 0b001 (0, 1).
        val[0] = 0x1; // 0b001
        msk[0] = 0x6; // 0b11X
        break;

    case 2:
        // Get levels [0; 2] to CPU.
        // The following matches level 0b0X0, that is, 0b000 and 0b010 (0, 2).
        val[0] = 0x2; // 0b010
        msk[0] = 0x5; // 0b1X1

        // The following matches level 0b001 (1).
        val[1] = 0x1; // 0b001
        msk[1] = 0x7; // 0b111
        entry_used[1] = true;
        break;

    case 3:
        // Get levels [0; 3] to CPU.
        // The following matches level 0b0XX, that is, 0b000, 0b001, 0b010, and
        // 0b011 (0, 1, 2, 3).
        val[0] = 0x3; // 0b011
        msk[0] = 0x4; // 0b1XX
        break;

    case 4:
        // Get levels [0; 4] to CPU.
        // The following matches level 0b0XX, that is, 0b000, 0b001, 0b010, and
        // 0b011 (0, 1, 2, 3).
        val[0] = 0x3; // 0b011
        msk[0] = 0x4; // 0b1XX

        // The following matches level 0b100 (4).
        val[1] = 0x4; // 0b100
        msk[1] = 0x7; // 0b111
        entry_used[1] = true;
        break;

    case 5:
        // Get level [0; 5] to CPU.
        // The following matches level 0bX0X, that is, 0b000, 0b001, 0b100, and
        // 0b101 (0, 1, 4, 5).
        val[0] = 0x5; // 0b101
        msk[0] = 0x2; // 0bX1X

        // The following matches 0b01X, that is, 0b010 and 0b011 (2, 3).
        val[1] = 0x2; // 0b010
        msk[1] = 0x6; // 0b11X
        entry_used[1] = true;
        break;

    case 6:
        // Get level [0; 6] to CPU.
        // The following matches level 0bXX0, that is, 0b000, 0b010, 0b100, and
        // 0b110 (0, 2, 4, 6).
        val[0] = 0x6; // 0b110
        msk[0] = 0x1; // 0bXX1

        // The following matches level 0bX0X, that is 0b000, 0b001, 0b100, and
        // 0b101 (0, 1, 4, 5).
        val[1] = 0x5; // 0b101
        msk[1] = 0x2; // 0bX0X
        entry_used[1] = true;

        // The following matches level 0b011 (3).
        val[2] = 0x3; // 0b011
        msk[2] = 0x7; // 0b111
        entry_used[2] = true;
        break;

    case 7:
        // Get level [0; 7] to CPU.
        // The following matches all levels
        val[0] = 0x7; // 0b111
        msk[0] = 0x0; // 0bXXX
        break;
    }

    // One time initializations common to all ACEs of both types
    T_DG(CFM_TRACE_GRP_BASE, "MEP %s: acl_mgmt_ace_init(%s)", mep_state->key, itr->first);
    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ETYPE, &new_ace_conf)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: acl_mgmt_ace_init(%s) failed: %s", mep_state->key, itr->first, error_txt(rc));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    // Fields that are common to both types of rules that we create
    // IS2 rules (such as the ones we are about to create) match on classified
    // VID (whereas CLM and IS1 rules match on what's in the tag of the frame)
    new_ace_conf.vid.value                  = mep_state->classified_vid_get();
    new_ace_conf.vid.mask                   = 0xFFFF;
    new_ace_conf.isid                       = VTSS_ISID_LOCAL;
    new_ace_conf.frame.etype.etype.value[0] = (CFM_ETYPE >> 8) & 0xFF;
    new_ace_conf.frame.etype.etype.value[1] = (CFM_ETYPE >> 0) & 0xFF;
    new_ace_conf.frame.etype.etype.mask[0]  = 0xFF;
    new_ace_conf.frame.etype.etype.mask[1]  = 0xFF;
    new_ace_conf.action.port_action         = MESA_ACL_PORT_ACTION_FILTER;

    // If we were to match only on CCM opcode, the following code would be
    // needed, but since we need to terminate all CFM PDUs (APS, CCM LBM, etc.),
    // we should just match on EtherType and VLAN
    // new_ace_conf.frame.etype.data.value[1] = CFM_OPCODE_CCM;
    // new_ace_conf.frame.etype.data.mask[1]  = 0xFF;

    switch (itr->first.insertion_order) {
    case CFM_BASE_ACE_INSERTION_ORDER_PORT_DOWN_MEP:
    case CFM_BASE_ACE_INSERTION_ORDER_VLAN_DOWN_MEP:
        // We are creating a rule for the MEP's residence port.

        new_ace_conf.port_list.clear_all();
        new_ace_conf.port_list[mep_state->port_state->port_no] = TRUE;

        // Redirect all CCMs that hit this ACE to the CPU. This happens by first
        // setting force_cpu, which causes the CCMs to be copied to the CPU and
        // then setting action.port_action to MESA_ACL_PORT_ACTION_FILTER with
        // an empty action-port-list, so that they don't get forwarded anywhere.
        new_ace_conf.action.force_cpu   = true;
        new_ace_conf.action.cpu_queue   = PACKET_XTR_QU_OAM;
        new_ace_conf.action.port_list.clear_all();
        break;

    case CFM_BASE_ACE_INSERTION_ORDER_LEAK_DOWN_MEP:
        // We are creating a rule for all other ports than the MEP's residence
        // port (for leak-prevention purposes)

        // In the key, we add all ports, because it doesn't matter whether the
        // frame enters the port on which we have installed the corresponding
        // MEP ACE (which == 0), because this rule will be inserted afterwards,
        // so it won't be hit on the MEP's residence port.
        new_ace_conf.port_list.set_all();

        // For the destination port list, we set all ports initially and remove
        // our own port.
        // Later on, as other MEPs contribute to the same rule, other ports may
        // be set to FALSE as well.
        new_ace_conf.action.port_list.set_all();
        new_ace_conf.action.port_list[mep_state->port_state->port_no] = FALSE;
        break;

    default:
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Unknown insertion_order (%d)", mep_state->key, itr->first.insertion_order);
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    // Find the ACE that we need to insert all the new ACEs before.
    insertion_point = CFM_BASE_ace_insertion_point_get(itr->first);

    // Create up to three rules using (almost) this conf
    for (i = 0; i < ARRSZ(cfm_base_ace_map_value_t::ace_ids); i++) {
        ace_id = itr->second.ace_ids[i];

        if (ace_id != ACL_MGMT_ACE_ID_NONE) {
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: ace_ids[%d] is %d, not %d for key = %s", mep_state->key, i, ace_id, ACL_MGMT_ACE_ID_NONE, itr->first);
            // Just leak
        }

        if (!entry_used[i]) {
            // Don't make a rule for this one.
            continue;
        }

        // Things that vary from entry to entry.
        new_ace_conf.frame.etype.data.value[0] = val[i] << 5;
        new_ace_conf.frame.etype.data.mask[0]  = msk[i] << 5;
        new_ace_conf.id                        = ACL_MGMT_ACE_ID_NONE;

        if ((rc = acl_mgmt_ace_add(ACL_USER_CFM, insertion_point, &new_ace_conf)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: acl_mgmt_ace_add(0x%x, %s) failed: %s", mep_state->key, insertion_point, itr->first, error_txt(rc));
            return VTSS_APPL_CFM_RC_HW_RESOURCES;
        }

        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: acl_mgmt_ace_add(0x%x, %s) => ace_id = 0x%x", mep_state->key, insertion_point, itr->first, new_ace_conf.id);
        itr->second.ace_ids[i] = new_ace_conf.id;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_ace_update()
/******************************************************************************/
static mesa_rc CFM_BASE_ace_update(cfm_mep_state_t *mep_state, cfm_base_ace_itr_t itr, mesa_port_no_t port_no)
{
    acl_entry_conf_t ace_conf;
    mesa_ace_id_t    ace_id, insertion_point;
    mesa_port_no_t   port_no_itr;
    bool             is_leak_rule = itr->first.insertion_order == CFM_BASE_ACE_INSERTION_ORDER_LEAK_DOWN_MEP;
    int              i;
    mesa_rc          first_encountered_rc = VTSS_RC_OK, rc;

    if (itr->second.owner_ref_cnt == 0) {
        // Remove entire rule
        if ((rc = CFM_BASE_ace_remove(mep_state, itr)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }

        // Also remove itr from the map.
        CFM_BASE_ace_map.erase(itr);
        return first_encountered_rc;
    }

    // We need to insert the ACEs below before whatever the next itr is.
    insertion_point = CFM_BASE_ace_insertion_point_get(itr->first);

    for (i = 0; i < ARRSZ(itr->second.ace_ids); i++) {
        ace_id = itr->second.ace_ids[i];
        if (ace_id == ACL_MGMT_ACE_ID_NONE) {
            continue;
        }

        T_DG(CFM_TRACE_GRP_BASE, "MEP %s: acl_mgmt_ace_get(%s: 0x%x)", mep_state->key, itr->first, ace_id);
        if ((rc = acl_mgmt_ace_get(ACL_USER_CFM, ace_id, &ace_conf, nullptr, FALSE)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: acl_mgmt_ace_get(%s: 0x%x) failed: %s", mep_state->key, itr->first, ace_id, error_txt(rc));
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = VTSS_APPL_CFM_RC_INTERNAL_ERROR;
            }

            continue;
        }

        if (is_leak_rule) {
            // This is a leak-prevention rule. Gotta update the destination port
            // list according to the itr's port-ref-cnt, so that if no MEPs are
            // installed on the destination port (ref_cnt == 0), we do allow
            // OAM frames to go to that port.
            if (port_no == MESA_PORT_NO_NONE) {
                // Update the entire destination port list.
                for (port_no_itr = 0; port_no_itr < CFM_BASE_cap_port_cnt; port_no_itr++) {
                    ace_conf.action.port_list[port_no_itr] = itr->second.port_ref_cnt[port_no_itr] == 0 ? 1 : 0;
                }
            } else {
                // Update only this port.
                ace_conf.action.port_list[port_no] = itr->second.port_ref_cnt[port_no] == 0 ? 1 : 0;
            }
        } else {
            // This is an ingress rule for the port on which the MEP is
            // installed. Gotta update the source port list for port_no
            // according to its reference count, so that if no MEPs are
            // installed on the source port (ref_cnt == 0), we won't hit the
            // rule.
            ace_conf.port_list[port_no] = itr->second.port_ref_cnt[port_no] == 0 ? 0 : 1;
        }

        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: acl_mgmt_ace_add(%s: 0x%x, 0x%x)", mep_state->key, itr->first, insertion_point, ace_id);
        if ((rc = acl_mgmt_ace_add(ACL_USER_CFM, insertion_point, &ace_conf)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: acl_mgmt_ace_add(%s: 0x%x, 0x%x) failed: %s", mep_state->key, itr->first, insertion_point, ace_id, error_txt(rc));
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = VTSS_APPL_CFM_RC_HW_RESOURCES;
            }
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// CFM_BASE_ace_contrib_remove()
/******************************************************************************/
static mesa_rc CFM_BASE_ace_contrib_remove(cfm_mep_state_t *mep_state, cfm_base_ace_itr_t contrib_itr)
{
    cfm_base_ace_itr_t itr, next_itr;
    bool               is_leak_rule = contrib_itr->first.insertion_order == CFM_BASE_ACE_INSERTION_ORDER_LEAK_DOWN_MEP;
    mesa_rc            first_encountered_rc = VTSS_RC_OK, rc;

    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Update of %s", mep_state->key, contrib_itr->first);

    if (is_leak_rule) {
        // For leak-rules, we need to remove contribution from out own AND from
        // possibly lower-levelled rules on the same VID.
        itr = CFM_BASE_ace_map.begin();
    } else {
        // We only need to remove contribution from this particular set of ACEs.
        itr = contrib_itr;
    }

    while (itr != CFM_BASE_ace_map.end()) {
        // Gotta keep a pointer to the next iterator, because it may happen that
        // we delete this one.
        next_itr = itr;
        ++next_itr;

        if (is_leak_rule &&
            (itr->first.insertion_order != contrib_itr->first.insertion_order ||
             itr->first.vid             != contrib_itr->first.vid             ||
             itr->first.level            > contrib_itr->first.level)) {
            // We don't contribute to this rule. One could optimize a bit by
            // breaking out when itr's insertion order > our insertion order and
            // itr's VID > our VID and itr's level > our level, because we know
            // that we cannot hit more entries at that point, because of the way
            // the keys are sorted, but let's leave that sub-optimization for
            // another good time.
            goto do_next;
        }

        // In order to have contributed to this rule, the port_ref_cnt and
        // owner-ref-cnt cannot be 0.
        if (itr->second.port_ref_cnt[mep_state->old_port_no] == 0 || itr->second.owner_ref_cnt == 0) {
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: MEP says it contributes to ACE rules for %s, port = %u", mep_state->key, itr->first, mep_state->old_port_no);
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = VTSS_APPL_CFM_RC_INTERNAL_ERROR;
            }

            goto do_next;
        }

        if (itr->first.level == contrib_itr->first.level) {
            // This is our primary rule (only one primary rule for Port/VLAN
            // MEPs, but several for leak-prevention ACEs).
            itr->second.owner_ref_cnt--;
        }

        itr->second.port_ref_cnt[mep_state->old_port_no]--;

        // If the port-ref-count or the owner-ref-count is now 0, we need to
        // either update the rule or remove the rule entirely (owner-ref-count
        // is 0). That's what the following function does.
        if (itr->second.port_ref_cnt[mep_state->old_port_no] == 0 || itr->second.owner_ref_cnt == 0) {
            T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Remove-updating ACE for %s", mep_state->key, itr->first);
            if ((rc = CFM_BASE_ace_update(mep_state, itr, mep_state->old_port_no)) != VTSS_RC_OK) {
                if (first_encountered_rc == VTSS_RC_OK) {
                    first_encountered_rc = rc;
                }
            }
        }

do_next:
        if (!is_leak_rule) {
            break;
        }

        itr = next_itr;
    }

    return first_encountered_rc;
}

/******************************************************************************/
// CFM_BASE_ace_contrib_add()
/******************************************************************************/
static mesa_rc CFM_BASE_ace_contrib_add(cfm_mep_state_t *mep_state, cfm_base_ace_map_key_t &key)
{
    cfm_base_ace_itr_t itr, itr2;
    int                i;
    bool               new_just_created = false, requires_update;
    bool               is_leak_rule = key.insertion_order == CFM_BASE_ACE_INSERTION_ORDER_LEAK_DOWN_MEP;
    mesa_port_no_t     port_no;
    mesa_rc            first_encountered_rc = VTSS_RC_OK, rc;

    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Add of %s", mep_state->key, key);

    if ((itr = CFM_BASE_ace_map.find(key)) == CFM_BASE_ace_map.end()) {
        // Rule doesn't exist.
        // First create a new entry in our map for it
        if ((itr = CFM_BASE_ace_map.get(key)) == CFM_BASE_ace_map.end()) {
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Unable to allocate ACE map entry for %s", mep_state->key, key);
            return VTSS_APPL_CFM_RC_OUT_OF_MEMORY;
        }

        // Initialize it
        itr->second.owner_ref_cnt = 1;
        itr->second.port_ref_cnt.clear();
        itr->second.port_ref_cnt[mep_state->port_state->port_no] = 1;

        for (i = 0; i < ARRSZ(itr->second.ace_ids); i++) {
            itr->second.ace_ids[i] = ACL_MGMT_ACE_ID_NONE;
        }

        // Then add rules
        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Adding rules for %s", mep_state->key, itr->first);
        if ((rc = CFM_BASE_ace_add(mep_state, itr)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }

        new_just_created = true;

        if (!is_leak_rule) {
            // MEP ACEs don't contribute to other ACEs, only leak-prevention
            // rules may.
            return first_encountered_rc;
        }
    }

    // Time to add ourselves to possibly existing rule with our VID and
    //  - if leak-rules: level == ours unless we've just added ourselves above
    //    and all leak-rules with levels < ours.
    //  - if port-rules: Levels == ours unless we've just added ourselves above.
    if (is_leak_rule) {
        // Gotta go through all ACEs, because we also need to add contribution
        // to lower level rules on the same VID.
        itr = CFM_BASE_ace_map.begin();
    } else {
        // itr is already pointing to our rule. Update it.
    }

    while (itr != CFM_BASE_ace_map.end()) {
        // Search for leak rules that (almost) match our key.
        if (is_leak_rule &&
            (itr->first.insertion_order != key.insertion_order ||
             itr->first.vid             != key.vid             ||
             itr->first.level           >  key.level)) {
            goto do_next;
        }

        if (itr->first.level == key.level) {
            if (new_just_created) {
                // We've just created and added ourselves to this rule, so
                // nothing more to do.
                goto do_next;
            } else {
                // Someone else has created this rule. Add ourselves as owner
                // since it matches our level.
                itr->second.owner_ref_cnt++;
            }
        }

        if (itr->second.port_ref_cnt[mep_state->port_state->port_no]++ == 0) {
            // Reference count changed from 0 to 1 by us. We gotta update the
            // rule.
            T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Add-updating ACE for %s", mep_state->key, itr->first);
            if ((rc = CFM_BASE_ace_update(mep_state, itr, mep_state->port_state->port_no)) != VTSS_RC_OK) {
                if (first_encountered_rc == VTSS_RC_OK) {
                    first_encountered_rc = rc;
                }
            }
        }

do_next:
        if (!is_leak_rule) {
            break;
        }

        ++itr;
    }

    if (!is_leak_rule || !new_just_created) {
        // Nothing more to do.
        return first_encountered_rc;
    }

    // We have just created a new leak-rule. Time to add other leak-rules with
    // the same VID and a higher level to ours
    itr = CFM_BASE_ace_map.find(key);
    requires_update = false;
    for (itr2 = CFM_BASE_ace_map.begin(); itr2 != CFM_BASE_ace_map.end(); ++itr2) {
        if (itr2->first.insertion_order != key.insertion_order ||
            itr2->first.vid             != key.vid             ||
            itr2->first.level           <= key.level) {
            continue;
        }

        // Here, we have another leak-rule at the same VID but with a level
        // higher than the rule we just created. Gotta add port-refs from that
        // other leak-rule to ours. This will not add ownership, only port-refs.
        requires_update = true;
        for (port_no = 0; port_no < CFM_BASE_cap_port_cnt; port_no++) {
            itr->second.port_ref_cnt[port_no] += itr2->second.port_ref_cnt[port_no];
        }
    }

    if (requires_update) {
        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Add-updating ACE for %s due to other MEPs", mep_state->key, itr->first);
        if ((rc = CFM_BASE_ace_update(mep_state, itr, MESA_PORT_NO_NONE)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// CFM_BASE_ace_deactivate()
/******************************************************************************/
static mesa_rc CFM_BASE_ace_deactivate(cfm_mep_state_t *mep_state, bool leak_rules)
{
    cfm_base_ace_itr_t &itr = leak_rules ? mep_state->ace_leak_itr : mep_state->ace_port_itr;
    mesa_rc            rc;

    if (itr == CFM_BASE_ace_map.end()) {
        return VTSS_RC_OK;
    }

    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Deactivating %s-rules", mep_state->key, leak_rules ? "Leak" : "Port");
    rc = CFM_BASE_ace_contrib_remove(mep_state, itr);
    itr = CFM_BASE_ace_map.end();

    return rc;
}

/******************************************************************************/
// CFM_BASE_ace_activate()
/******************************************************************************/
static mesa_rc CFM_BASE_ace_activate(cfm_mep_state_t *mep_state, bool leak_rules)
{
    cfm_base_ace_map_key_t key;
    cfm_base_ace_itr_t     &itr = leak_rules ? mep_state->ace_leak_itr : mep_state->ace_port_itr;

    // Create a key that reflects the current state of the MEP.
    // Port MEP ACEs are hit before VLAN MEP ACEs, which are hit before leak
    // ACEs.
    key.insertion_order = leak_rules ? CFM_BASE_ACE_INSERTION_ORDER_LEAK_DOWN_MEP : mep_state->is_port_mep() ? CFM_BASE_ACE_INSERTION_ORDER_PORT_DOWN_MEP : CFM_BASE_ACE_INSERTION_ORDER_VLAN_DOWN_MEP;
    key.vid             = mep_state->classified_vid_get();
    key.level           = mep_state->md_conf->level;

    if (itr != CFM_BASE_ace_map.end()) {
        // We are currently contributing to a rule. See if anything has changed.
        if (mep_state->port_state->port_no  != mep_state->old_port_no ||
            itr->first.insertion_order      != key.insertion_order    ||
            itr->first.vid                  != key.vid                ||
            itr->first.level                != key.level) {
            // MEP type, port, classified VID, or MD level has changed, so we
            // need to back out of the current rule(s)
            T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Removing contrib from %s", mep_state->key, itr->first);
            VTSS_RC(CFM_BASE_ace_contrib_remove(mep_state, itr));
            itr = CFM_BASE_ace_map.end();
        } else {
            T_IG(CFM_TRACE_GRP_BASE, "MEP %s: No change for %s", mep_state->key, itr->first);
        }
    }

    // Time to add our contribution if not already added.
    if (itr == CFM_BASE_ace_map.end()) {
        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Adding contrib for %s", mep_state->key, key);
        VTSS_RC(CFM_BASE_ace_contrib_add(mep_state, key));
        itr = CFM_BASE_ace_map.find(key);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_activate()
/******************************************************************************/
static mesa_rc CFM_BASE_activate(cfm_mep_state_t *mep_state, cfm_mep_state_change_t change)
{
    bool run_update;

    run_update = false;
    switch (change) {
    case CFM_MEP_STATE_CHANGE_CONF:
    case CFM_MEP_STATE_CHANGE_CONF_NO_RESET:
    case CFM_MEP_STATE_CHANGE_CONF_RMEP:
    case CFM_MEP_STATE_CHANGE_ENABLE_RMEP_DEFECT:
    case CFM_MEP_STATE_CHANGE_TPID:
    case CFM_MEP_STATE_CHANGE_IP_ADDR:
    case CFM_MEP_STATE_CHANGE_PVID_OR_ACCEPTABLE_FRAME_TYPE:
    case CFM_MEP_STATE_CHANGE_PORT_TYPE:
        run_update = true;
        break;

    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_PERIOD:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LOC:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MEPID:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MAID:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LEVEL:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_RDI:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_PORT_STATUS:
    case CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_IF_STATUS:
        // Changes to these do not affect our configuration
        break;

    default:
        T_EG(CFM_TRACE_GRP_BASE, "Unsupported state change enum (%d = %s)", change, cfm_util_mep_state_change_to_str(change));
        return VTSS_APPL_CFM_RC_INTERNAL_ERROR;
    }

    if (!run_update) {
        return VTSS_RC_OK;
    }

    if (mep_state->global_state->has_vop) {
        // Update/setup VOE
        VTSS_RC(CFM_BASE_voe_activate(mep_state));

        // LAN966x doesn't need any of the following.
        if (!mep_state->global_state->has_vop_v0) {
            // Update/setup an IFLOW
            VTSS_RC(CFM_BASE_iflow_activate(mep_state));

            // Update/setup VCEs
            VTSS_RC(CFM_BASE_vce_activate(mep_state));

            // Update/setup EFLOWs
            VTSS_RC(CFM_BASE_eflow_activate(mep_state));

            // Update/setup TCEs
            VTSS_RC(CFM_BASE_tce_activate(mep_state));
        }
    } else {
        // Update/setup residence port ACEs
        VTSS_RC(CFM_BASE_ace_activate(mep_state, false));

        // Update/setup leak-prevention ACEs
        VTSS_RC(CFM_BASE_ace_activate(mep_state, true));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// CFM_BASE_deactivate()
/******************************************************************************/
static mesa_rc CFM_BASE_deactivate(cfm_mep_state_t *mep_state)
{
    mesa_rc rc, first_encountered_rc = VTSS_RC_OK;

    if (mep_state->global_state->has_vop) {
        // LAN966x hasn't allocated any of the following.
        if (!mep_state->global_state->has_vop_v0) {
            // Tear down TCEs
            if ((rc = CFM_BASE_tce_deactivate(mep_state)) != VTSS_RC_OK) {
                if (first_encountered_rc == VTSS_RC_OK) {
                    first_encountered_rc = rc;
                }
            }

            // Tear down EFLOW
            if ((rc = CFM_BASE_eflow_deactivate(mep_state)) != VTSS_RC_OK) {
                if (first_encountered_rc == VTSS_RC_OK) {
                    first_encountered_rc = rc;
                }
            }

            // Tear down VCEs
            if ((rc = CFM_BASE_vce_deactivate(mep_state)) != VTSS_RC_OK) {
                if (first_encountered_rc == VTSS_RC_OK) {
                    first_encountered_rc = rc;
                }
            }

            // Tear down an IFLOW
            if ((rc = CFM_BASE_iflow_deactivate(mep_state)) != VTSS_RC_OK) {
                if (first_encountered_rc == VTSS_RC_OK) {
                    first_encountered_rc = rc;
                }
            }
        }

        // Unconfigure VOE
        if ((rc = CFM_BASE_voe_deactivate(mep_state)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    } else {
        // No VOE.

        // Deactivate leak-prevention ACEs
        if ((rc = CFM_BASE_ace_deactivate(mep_state, true)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }

        // Deactivate residence port ACEs
        if ((rc = CFM_BASE_ace_deactivate(mep_state, false)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// CFM_BASE_ccm_copy_to_cpu_timeout()
// Invoked every second on VOP_V1.
// #timer is CFM_BASE_copy_ccm_to_cpu_timer.
/******************************************************************************/
static void CFM_BASE_ccm_copy_to_cpu_timeout(cfm_timer_t &timer, void *context)
{
    cfm_mep_state_itr_t mep_state_itr;
    mesa_rc             rc;

    // Loop through all active VOEs and ask the VOE to copy the next CCM to CPU
    // provided the MEP doesn't have multiple RMEPs, in which case, all frames
    // get copied already.
    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        mesa_voe_idx_t voe_idx = mep_state_itr->second.voe_idx;

        if (voe_idx == MESA_VOE_IDX_NONE) {
            continue;
        }

        if (mep_state_itr->second.rmep_states.size() > 1) {
            // The VOE is already configured to send all CCMs to the CPU.
            continue;
        }

        if ((rc = mesa_voe_cc_cpu_copy_next_set(nullptr, voe_idx)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_BASE, "mesa_voe_cc_cpu_copy_next_set(%u) failed: %s", voe_idx, error_txt(rc));
        }
    }
}

/******************************************************************************/
// cfm_base_mep_state_init()
/******************************************************************************/
mesa_rc cfm_base_mep_state_init(cfm_mep_state_t *mep_state)
{
    int i;

    CFM_LOCK_ASSERT_LOCKED("MEP %s", mep_state->key);

    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Initializing state", mep_state->key);

    mep_state->voe_idx  = MESA_VOE_IDX_NONE;
    mep_state->iflow_id = MESA_IFLOW_ID_NONE;

    for (i = 0; i < ARRSZ(mep_state->vce_keys); i++) {
        mep_state->vce_keys[i].vce_id = CFM_VCE_ID_NONE;
    }

    mep_state->eflow_id = MESA_EFLOW_ID_NONE;

    for (i = 0; i < ARRSZ(mep_state->tce_keys); i++) {
        mep_state->tce_keys[i].tce_id = CFM_TCE_ID_NONE;
    }

    // We don't contribute to any ACE rules yet.
    mep_state->ace_port_itr = CFM_BASE_ace_map.end();
    mep_state->ace_leak_itr = CFM_BASE_ace_map.end();

    VTSS_RC(cfm_ccm_state_init(mep_state));

    return VTSS_RC_OK;
}

/******************************************************************************/
// cfm_base_mep_update()
/******************************************************************************/
mesa_rc cfm_base_mep_update(cfm_mep_state_t *mep_state, cfm_mep_state_change_t change)
{
    mesa_rc rc, first_encountered_rc = VTSS_RC_OK;
    bool    hw_based_meg, hw_based_mel, hw_based_mepid, hw_based_interval, old_active;

    CFM_LOCK_ASSERT_LOCKED("MEP %s", mep_state->key);

    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Updating MEP due to %s", mep_state->key, cfm_util_mep_state_change_to_str(change));

    // The MEP is active if we are not about to tear it down and if it's
    // administratively up and creatable.
    old_active = mep_state->status.mep_active;
    mep_state->status.mep_active = mep_state->mep_conf && mep_state->mep_conf->mep_conf.admin_active && mep_state->status.errors.mep_creatable;

    // Update the MEP status' Source MAC address, which may be changed because
    // of a configuration change of the configured SMAC or the interface on
    // which the MEP resides.
    if (mep_state->mep_conf && (change == CFM_MEP_STATE_CHANGE_CONF || change == CFM_MEP_STATE_CHANGE_CONF_NO_RESET)) {
        mep_state->status.smac = cfm_base_smac_get(mep_state);
    }

    if (!mep_state->status.mep_active) {
        // The MEP is not creatable for one or another reason. Go tear it down
        // if it exists.
        goto do_exit;
    }

    // We are not about to be torn down.

    // Now figure out, whether we can use the VOE for events.
    if (mep_state->rmep_states.size() > 1) {
        // If we have more than one Remote MEP, we cannot use the VOE to
        // generate events, because the H/W only supports one.
        // This means that we have to have all CCMs copied to the CPU.
        mep_state->voe_event_mask = 0;
    } else {
        // We have one single remote MEP, so we can safely enable desired VOE
        // events.
        // H/W may or not support the following events, which we don't take
        // advantage of in this code (reason follows):
        //  - MESA_VOE_EVENT_MASK_CCM_ZERO_PERIOD, because 802.1Q specifies that
        //    such frames must be discarded without further action.
        //  - MESA_VOE_EVENT_MASK_CCM_PRIORITY, because 802.1Q doesn't care.
        //  - MESA_VOE_EVENT_MASK_CCM_SRC_PORT_MOVE, for no particular reason
        //  - MESA_VOE_EVENT_MASK_CCM_TLV_PORT_STATUS, because the H/W does not
        //    provide interrupts when this TLV disappears from the CCM PDU.
        //  - MESA_VOE_EVENT_MASK_CCM_TLV_IF_STATUS, because the H/W does not
        //    provide interrupts when this TLV disappears from the CCM PDU.
        mep_state->voe_event_mask = MESA_VOE_EVENT_MASK_CCM_PERIOD          |
                                    MESA_VOE_EVENT_MASK_CCM_LOC             |
                                    MESA_VOE_EVENT_MASK_CCM_MEP_ID          |
                                    MESA_VOE_EVENT_MASK_CCM_MEG_ID          |
                                    MESA_VOE_EVENT_MASK_CCM_MEG_LEVEL       |
                                    MESA_VOE_EVENT_MASK_CCM_RX_RDI;
    }

    // The VOE may not support all events, so mask out those that it doesn't.
    // The voe_event_support_mask is all-zeros if the chip we are running on
    // doesn't have VOE-support.
    mep_state->voe_event_mask &= mep_state->global_state->voe_event_support_mask;

    // Both MEG_ID and MEG_LEVEL interrupts may give rise to a cross-connect
    // (xcon) event, but either they must both be zero or both be one, since
    // this implementation doesn't support one of them in H/W and the other one
    // in S/W.
    hw_based_mel = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_MEG_LEVEL) != 0;
    hw_based_meg = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_MEG_ID)    != 0;
    if (hw_based_meg != hw_based_mel) {
        // Disable both and rely on S/W.
        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Disabling both MEG_ID and MEG_LEVEL events", mep_state->key);
        mep_state->voe_event_mask &= ~MESA_VOE_EVENT_MASK_CCM_MEG_ID;
        mep_state->voe_event_mask &= ~MESA_VOE_EVENT_MASK_CCM_MEG_LEVEL;
    }

    // Both MEP_ID and CCM Interval interrupts may give rise to am errorCCM
    // event, but either they must both be zero or both be one, since this
    // implementation doesn't support one of them in H/W and the other one in
    // S/W.
    hw_based_mepid    = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_MEP_ID) != 0;
    hw_based_interval = (mep_state->voe_event_mask & MESA_VOE_EVENT_MASK_CCM_PERIOD) != 0;
    if (hw_based_mepid != hw_based_interval) {
        // Disable both and rely on S/W.
        T_EG(CFM_TRACE_GRP_BASE, "MEP %s: Disabling both MEP_ID and CCM_PERIOD events", mep_state->key);
        mep_state->voe_event_mask &= ~MESA_VOE_EVENT_MASK_CCM_MEP_ID;
        mep_state->voe_event_mask &= ~MESA_VOE_EVENT_MASK_CCM_PERIOD;
    }

    // The thing here is that if something goes wrong, the relevant function
    // sets mep_state->status.errors.mep_creatable to false and returns
    // something != VTSS_RC_OK.

    // (Re-)configure base
    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Invoking CFM_BASE_activate()", mep_state->key);
    if ((rc = CFM_BASE_activate(mep_state, change)) != VTSS_RC_OK) {
        first_encountered_rc = rc;
        goto do_exit;
    }

    // (Re-)activate CCM
    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Invoking cfm_ccm_update()", mep_state->key);
    if ((rc = cfm_ccm_update(mep_state, change)) != VTSS_RC_OK) {
        first_encountered_rc = rc;
        goto do_exit;
    }

do_exit:
    if (first_encountered_rc != VTSS_RC_OK) {
        if (first_encountered_rc == VTSS_APPL_CFM_RC_HW_RESOURCES) {
            mep_state->status.errors.hw_resources = true;
        } else {
            mep_state->status.errors.internal_error = true;
        }

        mep_state->status.errors.mep_creatable = false;
    }

    // Re-compute mep_active given that the value of mep_creatable may have
    // changed. The MEP is active if it is enabled by the user and nothing went
    // wrong during the configuration of the MEP.
    mep_state->status.mep_active = mep_state->mep_conf && mep_state->mep_conf->mep_conf.admin_active && mep_state->status.errors.mep_creatable;

    if (mep_state->status.mep_active != old_active) {
        // Create a notification.
        cfm_base_mep_ok_notification_update(mep_state);
    }

    T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Done. mep_active = %d", mep_state->key, mep_state->status.mep_active);

    if (!mep_state->status.mep_active) {
        // Something went wrong - or we are actively taking this MEP down.

        // None of these deactivation steps can change
        // mep_state->status.errors.mep_creatable.
        if ((rc = cfm_ccm_update(mep_state, CFM_MEP_STATE_CHANGE_CONF /* Any change will do */)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }

        T_IG(CFM_TRACE_GRP_BASE, "MEP %s: Invoking CFM_BASE_deactivate()", mep_state->key);
        if ((rc = CFM_BASE_deactivate(mep_state)) != VTSS_RC_OK) {
            if (first_encountered_rc != VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    return first_encountered_rc;
}

/******************************************************************************/
// cfm_base_mep_statistics_clear()
/******************************************************************************/
mesa_rc cfm_base_mep_statistics_clear(cfm_mep_state_t *mep_state)
{
    return cfm_ccm_statistics_clear(mep_state);
}

/******************************************************************************/
// cfm_base_frame_rx()
// Only invoked if MEP is active (mep_state->status.mep_active == true).
/******************************************************************************/
void cfm_base_frame_rx(cfm_mep_state_t *mep_state, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    uint8_t level, opcode;

    CFM_LOCK_ASSERT_LOCKED("MEP %s", mep_state->key);

    // If this CFM PDU is not received on our port, it's not for us.
    if (rx_info->port_no != mep_state->port_state->port_no) {
        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s. Rx on port_no %u, MEP on port_no %u", mep_state->key, rx_info->port_no, mep_state->port_state->port_no);
        return;
    }

    // Active Level Demultiplexer
    level = frm[14] >> 5;

    if (level > mep_state->md_conf->level) {
        // Should just have passed through to passive multiplexer
        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP %s. Rx with level %u, which is greater than our level (%u)", mep_state->key, level, mep_state->md_conf->level);
        return;
    }

    // As long as we can't receive a frame behind two tags, the CFM PDU contents
    // always starts at frm[14], because the packet module strips the outer tag.
    opcode = frm[15];

    if (level < mep_state->md_conf->level) {
        // Low OpCode Demultiplexer (19.2.7)

        // The only CFM PDU the Low OpCode Demultiplexer handles is CCM. PDUs
        // with other opcodes are discarded.
        if (opcode == CFM_OPCODE_CCM) {
            // The two variables set here should have been CCMreceivedLow and
            // CCMlowPDU, but they are not needed, because they would have been
            // cleared once cfm_ccm_rx_frame() had handled the frame.
            cfm_ccm_rx_frame(mep_state, frm, rx_info);
        }

        return;
    }

    // Equal OpCode Demultiplexer (19.2.7)
    switch (opcode) {
    case CFM_OPCODE_CCM:
        // The two variables set here should have been CCMreceivedEqual and
        // CCMequalPDU, but they are not needed, because they would have been
        // cleared once cfm_ccm_rx_frame() had handled the frame.
        cfm_ccm_rx_frame(mep_state, frm, rx_info);
        break;

    default:
        T_DG(CFM_TRACE_GRP_FRAME_RX, "MEP = %s. Unhandled CFM opcode %u", mep_state->key, opcode);
        break;
    }
}

/******************************************************************************/
// cfm_base_smac_get()
// Get our own MAC address, which may be overridden by configuration.
/******************************************************************************/
mesa_mac_t cfm_base_smac_get(const cfm_mep_state_t *mep_state)
{
    static mesa_mac_t zero_mac;

    if (memcmp(mep_state->mep_conf->mep_conf.smac.addr, zero_mac.addr, sizeof(mep_state->mep_conf->mep_conf.smac))) {
        return mep_state->mep_conf->mep_conf.smac;
    }

    return mep_state->port_state->smac;
}

/******************************************************************************/
// cfm_base_mep_ok_notification_update()
/******************************************************************************/
void cfm_base_mep_ok_notification_update(cfm_mep_state_t *mep_state)
{
    vtss_appl_cfm_mep_notification_status_t notif_status;
    bool                                    old_mep_ok;
    mesa_rc                                 rc;
    char                                    buf[10];

    if ((rc = cfm_mep_notification_status.get(mep_state->key, &notif_status)) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_NOTIF, "MEP %s: Unable to get notification status: %s", mep_state->key, error_txt(rc));
    }

    // The MEP is up and running if:
    //   - it's active, that is, administratively enabled and created
    //   - enableRmepDefect is true, that is, we have no local errors.
    //   - There are no defects from remote end.
    old_mep_ok = notif_status.mep_ok;
    notif_status.mep_ok = mep_state->status.mep_active && mep_state->status.errors.enableRmepDefect && mep_state->status.defects == 0;

    if (notif_status.mep_ok != old_mep_ok) {
        T_IG(CFM_TRACE_GRP_NOTIF, "MEP %s: Setting mep_ok = %d (defects = %s)", mep_state->key, notif_status.mep_ok, cfm_util_mep_defects_to_str(mep_state->status.defects, buf));
        if ((rc = cfm_mep_notification_status.set(mep_state->key, &notif_status)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_NOTIF, "MEP %s: Unable to set mep_ok notification status to %d: %s", mep_state->key, notif_status.mep_ok, error_txt(rc));
        }
    }
}

/******************************************************************************/
// CFM_BASE_vce_dump()
// Dumps active VCE rules.
/******************************************************************************/
static void CFM_BASE_vce_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    cfm_mep_state_itr_t   mep_state_itr;
    cfm_mep_state_t       *mep_state;
    cfm_base_vce_id_itr_t vce_id_itr;
    int                   i;

    pr(session_id, "VCEs\n");
    pr(session_id, "Domain          Service         MEP-ID Ins. Order    OAM-detect VCE-ID     IFLOW\n");
    pr(session_id, "--------------- --------------- ------ ------------- ---------- ---------- ----------\n");

    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        mep_state = &mep_state_itr->second;
        for (i = 0; i < ARRSZ(mep_state->vce_keys); i++) {
            if (mep_state->vce_keys[i].vce_id == CFM_VCE_ID_NONE) {
                continue;
            }

            if ((vce_id_itr = CFM_BASE_vce_id_map.find(mep_state->vce_keys[i])) == CFM_BASE_vce_id_map.end()) {
                T_EG(CFM_TRACE_GRP_BASE, "%s: What?!?", mep_state->key);
                continue;
            }

            pr(session_id, "%-15s %-15s %6u %13s %-10s 0x%08x 0x%08x\n",
               mep_state->key.md.c_str(),
               mep_state->key.ma.c_str(),
               mep_state->key.mepid,
               CFM_BASE_vce_insertion_order_to_str(mep_state->vce_keys[i].insertion_order),
               CFM_BASE_oam_detect_to_str(vce_id_itr->second.action.oam_detect),
               mep_state->vce_keys[i].vce_id,
               vce_id_itr->second.action.flow_id);
        }
    }

    pr(session_id, "\n");
}

/******************************************************************************/
// CFM_BASE_tce_dump()
// Dumps active TCE rules.
/******************************************************************************/
static void CFM_BASE_tce_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    cfm_mep_state_itr_t   mep_state_itr;
    cfm_mep_state_t       *mep_state;
    cfm_base_tce_id_itr_t tce_id_itr;
    int                   i;

    pr(session_id, "TCEs\n");
    pr(session_id, "Domain          Service         MEP-ID Ins. Order    TCE-ID     iPort IFLOW      EFLOW\n");
    pr(session_id, "--------------- --------------- ------ ------------- ---------- ----- ---------- ----------\n");

    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        mep_state = &mep_state_itr->second;
        for (i = 0; i < ARRSZ(mep_state->tce_keys); i++) {
            if (mep_state->tce_keys[i].tce_id == CFM_TCE_ID_NONE) {
                continue;
            }

            if ((tce_id_itr = CFM_BASE_tce_id_map.find(mep_state->tce_keys[i])) == CFM_BASE_tce_id_map.end()) {
                T_EG(CFM_TRACE_GRP_BASE, "%s: What?!?", mep_state->key);
                continue;
            }

            pr(session_id, "%-15s %-15s %6u %13s 0x%08x %5u 0x%08x 0x%08x\n",
               mep_state->key.md.c_str(),
               mep_state->key.ma.c_str(),
               mep_state->key.mepid,
               CFM_BASE_tce_insertion_order_to_str(mep_state->tce_keys[i].insertion_order),
               mep_state->tce_keys[i].tce_id,
               mep_state->port_state->port_no,
               tce_id_itr->second.key.flow_id,
               tce_id_itr->second.action.flow_id);
        }
    }

    pr(session_id, "\n");
}

/******************************************************************************/
// CFM_BASE_iflow_dump()
// Dumps active IFLOWs.
/******************************************************************************/
static void CFM_BASE_iflow_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    cfm_mep_state_itr_t mep_state_itr;
    cfm_mep_state_t     *mep_state;
    mesa_iflow_conf_t   iflow_conf;
    mesa_rc             rc;

    pr(session_id, "IFLOWs\n");
    pr(session_id, "Domain          Service         MEP-ID IFLOW      VOE-Idx\n");
    pr(session_id, "--------------- --------------- ------ ---------- ----------\n");

    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        mep_state = &mep_state_itr->second;

        if (mep_state->iflow_id == MESA_IFLOW_ID_NONE) {
            continue;
        }

        if ((rc = mesa_iflow_conf_get(nullptr, mep_state->iflow_id, &iflow_conf)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_BASE, "MEP %s: mesa_iflow_conf_get() failed: %s", mep_state->key, error_txt(rc));
            continue;
        }

        pr(session_id, "%-15s %-15s %6u 0x%08x 0x%08x\n",
           mep_state->key.md.c_str(),
           mep_state->key.ma.c_str(),
           mep_state->key.mepid,
           mep_state->iflow_id,
           iflow_conf.voe_idx);
    }

    pr(session_id, "\n");
}

/******************************************************************************/
// CFM_BASE_voe_dump()
// Dumps active VOEs.
/******************************************************************************/
static void CFM_BASE_voe_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    cfm_mep_state_itr_t mep_state_itr;
    cfm_mep_state_t     *mep_state;

    pr(session_id, "VOEs\n");
    pr(session_id, "Domain          Service         MEP-ID VOE-Idx    Type iPort Dir\n");
    pr(session_id, "--------------- --------------- ------ ---------- ---- ----- ----\n");

    for (mep_state_itr = CFM_mep_state_map.begin(); mep_state_itr != CFM_mep_state_map.end(); ++mep_state_itr) {
        mep_state = &mep_state_itr->second;

        if (mep_state->voe_idx == MESA_VOE_IDX_NONE) {
            continue;
        }

        pr(session_id, "%-15s %-15s %6u 0x%08x %4s %5u %-s\n",
           mep_state->key.md.c_str(),
           mep_state->key.ma.c_str(),
           mep_state->key.mepid,
           mep_state->voe_idx,
           mep_state->voe_alloc.type == MESA_VOE_TYPE_PORT ? "Port" : "VLAN",
           mep_state->voe_alloc.port,
           mep_state->voe_alloc.direction == MESA_OAM_DIRECTION_DOWN ? "Down" : "Up");
    }

    pr(session_id, "\n");
}

/******************************************************************************/
// CFM_BASE_vce_map_dump()
// Dumps our VCE map.
/******************************************************************************/
static void CFM_BASE_vce_map_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    cfm_base_vce_id_itr_t itr;

    pr(session_id, "VCE Map\n");
    pr(session_id, "VCE-ID     Ins. Order    OAM-detect IFLOW\n");
    pr(session_id, "---------- ------------- ---------- ----------\n");

    for (itr = CFM_BASE_vce_id_map.begin(); itr != CFM_BASE_vce_id_map.end(); ++itr) {
        pr(session_id, "0x%08x %13s %-10s 0x%08x\n",
           itr->first.vce_id,
           CFM_BASE_vce_insertion_order_to_str(itr->first.insertion_order),
           CFM_BASE_oam_detect_to_str(itr->second.action.oam_detect),
           itr->second.action.flow_id);
    }

    pr(session_id, "\n");
}

/******************************************************************************/
// CFM_BASE_tce_map_dump()
// Dumps our TCE map.
/******************************************************************************/
static void CFM_BASE_tce_map_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    cfm_base_tce_id_itr_t itr;

    pr(session_id, "TCE Map\n");
    pr(session_id, "TCE-ID     Ins. Order    IFLOW      EFLOW\n");
    pr(session_id, "---------- ------------- ---------- ----------\n");

    for (itr = CFM_BASE_tce_id_map.begin(); itr != CFM_BASE_tce_id_map.end(); ++itr) {
        pr(session_id, "0x%08x %13s 0x%08x 0x%08x\n",
           itr->first.tce_id,
           CFM_BASE_tce_insertion_order_to_str(itr->first.insertion_order),
           itr->second.key.flow_id,
           itr->second.action.flow_id);
    }

    pr(session_id, "\n");
}

/******************************************************************************/
// CFM_BASE_ace_dump()
// Dumps active ACE rules.
/******************************************************************************/
static void CFM_BASE_ace_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    cfm_base_ace_itr_t itr;
    mesa_port_no_t     port_no;
    int                cnt = 1;

    pr(session_id, "ACEs\n");
    pr(session_id, "#   Ins. Order    VID  Lvl ID[0] ID[1] ID[2] Owners Port:Refs-list\n");
    pr(session_id, "--- ------------- ---- --- ----- ----- ----- ------ --------------\n");

    for (itr = CFM_BASE_ace_map.begin(); itr != CFM_BASE_ace_map.end(); ++itr) {
        pr(session_id, "%3d %13s %4u %3u %5u %5u %5u %6u",
           cnt++,
           CFM_BASE_ace_insertion_order_to_str(itr->first.insertion_order),
           itr->first.vid,
           itr->first.level,
           itr->second.ace_ids[0],
           itr->second.ace_ids[1],
           itr->second.ace_ids[2],
           itr->second.owner_ref_cnt);

        for (port_no = 0; port_no < CFM_BASE_cap_port_cnt; port_no++) {
            if (itr->second.port_ref_cnt[port_no]) {
                pr(session_id, " %u:%u", port_no, itr->second.port_ref_cnt[port_no]);
            }
        }

        pr(session_id, "\n");
    }
}

/******************************************************************************/
// cfm_rules_debug_dump()
// Dumps active rules.
/******************************************************************************/
void cfm_rules_debug_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    CFM_LOCK_SCOPE();

    if (fast_cap(MESA_CAP_VOP)) {
        // Show per MEP rules
        CFM_BASE_vce_dump(  session_id, pr);
        CFM_BASE_tce_dump(  session_id, pr);
        CFM_BASE_iflow_dump(session_id, pr);
        CFM_BASE_voe_dump(  session_id, pr);

        // Show map contents
        CFM_BASE_vce_map_dump(session_id, pr);
        CFM_BASE_tce_map_dump(session_id, pr);
    } else {
        CFM_BASE_ace_dump(session_id, pr);
    }
}

/******************************************************************************/
// cfm_base_init()
/******************************************************************************/
void cfm_base_init(cfm_global_state_t *global_state)
{
    mesa_iflow_conf_t dummy_iflow_conf;
    mesa_vop_conf_t   vop_conf;
    mesa_rc           rc;

    CFM_LOCK_ASSERT_LOCKED("CRIT NOT LOCKED");

    // Do not use fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), because that one will
    // return a number >= actual port count, and then some functions may fail
    // (e.g. vlan_mgmt_port_conf_get()).
    CFM_BASE_cap_port_cnt = port_count_max();

    if (!global_state->has_vop) {
        return;
    }

    if (global_state->has_vop_v2) {
        // We need a dummy iflow that VCE rules installed on "other ports" will
        // point to, so that when a frame ingresses a port with a Port MEP, but that
        // frame is not intended for that MEP, it will be marked as independent MEL,
        // so that the Port VOE won't level filter it.
        // See more comments where this dummy iflow is used.
        if ((rc = mesa_iflow_alloc(nullptr, &CFM_BASE_dummy_iflow_id)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_BASE, "mesa_iflow_alloc() of dummy iflow failed: %s", error_txt(rc));
        }

        // Get dummy iflow configuration
        if ((rc = mesa_iflow_conf_get(nullptr, CFM_BASE_dummy_iflow_id, &dummy_iflow_conf)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_BASE, "mesa_iflow_conf_get() of dummy iflow failed: %s", error_txt(rc));
        }

        // Set dummy iflow configuration
        if ((rc = mesa_iflow_conf_set(nullptr, CFM_BASE_dummy_iflow_id, &dummy_iflow_conf)) != VTSS_RC_OK) {
            T_EG(CFM_TRACE_GRP_BASE, "mesa_iflow_conf_set() of dummy iflow failed: %s", error_txt(rc));
        }
    } else {
        // On other platforms, especially Serval-1/Ocelot, we must not mark the
        // frames with an ISDX, because then a subsequent ACL lookup will have
        // to use the ISDX for matching instead of the VLAN, and that will cause
        // problems for at least ERPS if multiple ring instances use the same
        // ring ports, because a R-APS PDU meant for VLAN X and another one
        // meant for VLAN Y both would use the same dummy ISDX, so it wouldn't
        // be possible to differentiate R-APS PDU forwarding between the two
        // ring instances.
        // The good thing with Serval-1/Ocelot is that we don't need the ISDX to
        // have the VCE leaking entry mark the frame as OAM behind X tags, but
        // we do on JR2 and FA.
        CFM_BASE_dummy_iflow_id = MESA_IFLOW_ID_NONE;
    }

    if (mesa_vop_conf_get(nullptr, &vop_conf) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "mesa_vop_conf_get() failed");
    }

    vop_conf.multicast_dmac = cfm_multicast_dmac;

    if (global_state->has_vop_v0 || global_state->has_vop_v1) {
        // VOP_V0 and VOP_V1 don't have auto-copy-to-CPU-features. Instead, we
        // need to periodically invoke a function
        // (mesa_voe_cc_cpu_copy_next_set()), which - after the call - causes
        // the next CCM PDU to be copied to the CPU. Since we get most defects
        // through VOE events, this is actually only used to get the RMEP's SMAC
        // and TLVs not fetched by the chip.
        cfm_timer_init(CFM_BASE_copy_ccm_to_cpu_timer, "CCM-Copy-to-CPU", 0, CFM_BASE_ccm_copy_to_cpu_timeout, NULL);
        cfm_timer_start(CFM_BASE_copy_ccm_to_cpu_timer, 1000, true);
    } else {
        // With VOP_V2, we can have the API to send us frames every so many
        // microseconds.
        // It has two 'timers' - one meant for fast and one meant for slow
        // periods.
        vop_conf.auto_copy_period[0] = 1000000; // 1 second period (fast)
        vop_conf.auto_copy_period[1] = 3000000; // 3 second period (slow)

        // No need to get valid CCM PDUs to CPU faster than every 3 seconds,
        // because we rely on events from the VOE - except for the SMAC, but I
        // think it's good enough to get this updated every 3 seconds, because
        // it's only used for RMEP status, and it doesn't participate in defect
        // calculations.
        vop_conf.auto_copy_ccm_valid = 1;

        // We cannot rely on the VOE's detection of Port Status and Interface
        // Status TLVs, because it doesn't detect if they were present in one
        // CCM PDU and not present in the next, so we get these to the CPU every
        // 1 second.
        vop_conf.auto_copy_ccm_tlv   = 0;

        // No need to get invalid CCM PDUs to the CPU faster than every 3
        // seconds, because we rely on events from the VOE.
        vop_conf.auto_copy_ccm_err   = 1;
    }

    vop_conf.voe_queue_ccm = PACKET_XTR_QU_OAM;
    vop_conf.voe_queue_lt  = PACKET_XTR_QU_OAM;
    vop_conf.voe_queue_lbm = PACKET_XTR_QU_OAM;
    vop_conf.voe_queue_lbr = PACKET_XTR_QU_OAM - 1;
    vop_conf.voe_queue_aps = PACKET_XTR_QU_OAM;
    vop_conf.voe_queue_err = PACKET_XTR_QU_OAM;
    vop_conf.voi_queue     = PACKET_XTR_QU_OAM; // (VOP_V2)

    if (mesa_vop_conf_set(nullptr, &vop_conf) != VTSS_RC_OK) {
        T_EG(CFM_TRACE_GRP_BASE, "mesa_vop_conf_set() failed");
    }
}

