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

#ifndef _CFM_BASE_HXX_
#define _CFM_BASE_HXX_

#include "main_types.h"                    /* For vtss_init_data_t            */
#include "afi_api.h"                       /* For afi_single_conf_t           */
#include <vtss/basics/map.hxx>             /* For vtss::Map()                 */
#include <vtss/appl/vlan.h>                /* For vtss_appl_vlan_port_type_t  */
#include <vtss/appl/cfm.hxx>               /* For ourselves                   */
#include <vtss/appl/ip.h>                  /* For vtss_appl_ip_if_status_t    */
#include "cfm_timer.hxx"                   /* For cfm_timer_t                 */
#include "cfm_expose.hxx"                  /* For cfm_mep_notification_status */
#include "cfm_trace.h"                     /* For T_EG()                      */
#include <vtss/basics/memcmp-operator.hxx> /* For VTSS_BASICS_MEMCMP_OPERATOR */

// The following provides inline functions for comparing two
// vtss_appl_cfm_mep_notification_status_t structures.
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_cfm_mep_notification_status_t);

#define CFM_VCE_ID_NONE 0xFFFFFFFF
#define CFM_TCE_ID_NONE 0xFFFFFFFF

#define CFM_OPCODE_CCM 1
#define CFM_OPCODE_LBR 2
#define CFM_OPCODE_LBM 3
#define CFM_OPCODE_LTR 4
#define CFM_OPCODE_LTM 5

extern const mesa_mac_t cfm_multicast_dmac;

typedef enum {
    CFM_MEP_STATE_CHANGE_CONF,
    CFM_MEP_STATE_CHANGE_CONF_NO_RESET,
    CFM_MEP_STATE_CHANGE_CONF_RMEP,
    CFM_MEP_STATE_CHANGE_IP_ADDR,
    CFM_MEP_STATE_CHANGE_TPID,
    CFM_MEP_STATE_CHANGE_PORT_TYPE,
    CFM_MEP_STATE_CHANGE_PVID_OR_ACCEPTABLE_FRAME_TYPE,
    CFM_MEP_STATE_CHANGE_ENABLE_RMEP_DEFECT,
    CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_PERIOD,
    CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_ZERO_PERIOD,
    CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_PRIORITY,
    CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LOC,
    CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MEPID,
    CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_MAID,
    CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_LEVEL,
    CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_RDI,
    CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_SRC_PORT_MOVE,
    CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_PORT_STATUS,
    CFM_MEP_STATE_CHANGE_VOE_EVENT_CCM_TLV_IF_STATUS,
} cfm_mep_state_change_t;

typedef struct {
    cfm_timer_t rMEPwhile_timer; // 20.5.10, 20.20.

    /**
     * Sequence number contained in last CCM received from this RMEP.
     */
    uint32_t    sequence_number;           // 20.17.1:e
    bool        rMEPCCMdefect;             // 20.19.1
    bool        rMEPportStatusDefect;      // 20.19.8
    bool        rMEPinterfaceStatusDefect; // 20.19.9

    // Public remote MEP status
    vtss_appl_cfm_rmep_status_t status;
} cfm_rmep_state_t;

typedef vtss::Map<vtss_appl_cfm_mepid_t, cfm_rmep_state_t> cfm_rmep_state_map_t;
typedef cfm_rmep_state_map_t::iterator cfm_rmep_state_itr_t;

typedef struct {
    afi_single_conf_t    afi_conf;
    u32                  afi_id;
    cfm_timer_t          rx_counter_poll_timer;
    cfm_timer_t          CCIwhile_timer;      // 20.5.2, 20.12
    cfm_timer_t          errorCCMwhile_timer; // 20.5.3, 20.22
    cfm_timer_t          xconCCMwhile_timer;  // 20.5.4, 20.24
    cfm_timer_t          FNGwhile_timer;      // 20.5.6, 20.37

    // The offset into the PDU where the sequence number is located.
    // This allows for updating the sequence number on platforms where CCM
    // frames are transmitted 'manually'.
    uint32_t seq_number_offset;

    // We also need the next sequence number to put in the frame, because if the
    // contents of the frame gets updated, the sequence number also gets reset
    // inside the frame, so we can't just add 1 to the frame contents.
    uint32_t next_seq_number;

    /**
     * 20.21.3 errorCCMdefect
     * A Boolean flag set and cleared by the Remote MEP Error state machine to
     * indicate that one or more invalid CCMs has been received, and that 3.5
     * times that CCM's transmission interval has not yet expired.
     * This variable is readable as a managed object [item r) in 12.14.7.1.3].
     */
    bool errorCCMdefect;

    /**
     * 20.23.3 xconCCMdefect
     * A Boolean flag set and cleared by the MEP Cross Connect state machine to
     * indicate that one or more cross connect CCMs has been received, and that
     * 3.5 times of at least one of those CCMs' transmission interval has not
     * yet expired.
     * This variable is readable as a managed object [item s) in 12.14.7.1.3].
     */
    bool xconCCMdefect;

    /**
     * 20.35.5 someRMEPCCMdefect
     * A Boolean indicating the aggregate state of the Remote MEP state
     * machines. True indicates that at least one of the Remote MEP state
     * machines is not receiving valid CCMs from its remote MEP, and false that
     * all Remote MEP state machines are receiving valid CCMs. someRMEPCCMdefect
     * is the logical OR of all of the rMEPCCMdefect variables for all of the
     * Remote MEP state machines on this MEP. This variable is readable as a
     * managed object [item q) in 12.14.7.1.3]
     */
    bool someRMEPCCMdefect;

    /**
     * 20.35.6 someMACstatusDefect
     * A Boolean indicating that one or more of the remote MEPs is reporting a
     * failure in its Port Status TLV (21.5.4) or Interface Status TLV (21.5.5).
     * It is true either if some remote MEP is reporting that its interface is
     * not isUp (i.e., at least one remote MEP's interface is unavailable), or
     * if all remote MEPs are reporting a Port Status TLV that contains some
     * value other than psUp (i.e., all remote MEPs' Bridge Ports are not
     * forwarding data). It is thus the logical OR of the following two terms:
     *   a) The logical AND, across all remote MEPs, of the rMEPportStatusDefect
     *      variable OR
     *   b) The logical OR, across all remote MEPs, of the
     *      rMEPinterfaceStatusDefect variable.
     * This variable is readable as a managed object [item p) in 12.14.7.1.3].
     */
    bool someMACstatusDefect;

    /**
     * 20.35.7 someRDIdefect
     * A Boolean indicating the aggregate health of the remote MEPs. True
     * indicates that at least one of the Remote MEP state machines is receiving
     * valid CCMs from its remote MEP that has the RDI bit set, and false that
     * no Remote MEP state machines are receiving valid CCMs with the RDI bit
     * set. someRDIdefect is the logical OR of all of the rMEPlastRDI and
     * rMEPlastRDI[i] variables for all of the Remote MEP state machines on this
     * MEP.
     * This variable is readable as a managed object [item o) in 12.14.7.1.3].
     */
    bool someRDIdefect;

    /**
     * 20.9.3 MAdefectIndication.
     * A Boolean indicating the operational state of the MEP's MA. True
     * indicates that at least one of the remote MEPs configured on this MEP's
     * MA has failed, and false indicates either that all are functioning, or
     * that the MEP has been active for less than the time-out period.
     * MAdefectIndication is true whenever an enabled defect is indicated.
     * That is, MAdefectIndication is true if and only if, for one or more of
     * the variables someRDIdefect, someRMEPCCMdefect, someMACstatusDefect,
     * errorCCMdefect, or xconCCMdefect, that variable is true and the
     * corresponding priority of that variable from Table 20-1 is greater than
     * or equal to the value of the variable lowestAlarmPri.
     */
    bool MAdefectIndication;

    /**
     * 20.9.4 allRMEPsDead
     * A Boolean indicating that this MEP is receiving none of the remote MEPs'
     * CCMs. allRMEPsDead is the logical AND of all of the rMEPCCMdefect
     * variables.
     *
     * According to 19.2.8, this is used only by a Port Down-MEP to signal to
     * it's passive SAP that MAC_operational is false, that is, to make the port
     * look as if it has no link). Currently not used for anything.
     */
    bool allRMEPsDead;
} cfm_ccm_state_t;

typedef struct {
    vtss_appl_ip_if_status_t ip_status; // Our IPv4 or IPv6 address used in Sender ID TLVs
    mesa_mac_t               smac;      // Our global MAC address.

    // VOP capabilities
    bool has_vop;    // true if chip has VOP support (all platforms but Caracal)
    bool has_vop_v0; // true if chip has VOP support and it's Maserati (LAN966x)
    bool has_vop_v1; // true if chip has VOP support and it's Serval1
    bool has_vop_v2; // true if chip has VOP support and is JR2 or later
    uint32_t voe_event_support_mask; // Mask of MESA_VOE_EVENT_MASK_xxx supported on this chip

    // AFI capabilities
    // See CFM_global_state_init() for description
    // If requested CCM interval is <= ccm_afi_interval_max, there is AFI
    // support for that CCM interval. Otherwise, injection must be made
    // manually.
    vtss_appl_cfm_ccm_interval_t ccm_afi_interval_max;
} cfm_global_state_t;

typedef struct {
    mesa_port_no_t                      port_no;
    mesa_mac_t                          smac;
    bool                                link;
    vtss_appl_vlan_port_detailed_conf_t vlan_conf;
    mesa_etype_t                        tpid;
} cfm_port_state_t;

extern CapArray<cfm_port_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> CFM_port_state;

typedef vtss::Map<vtss_appl_cfm_rmep_key_t, vtss_appl_cfm_rmep_conf_t> cfm_rmep_conf_map_t;
typedef cfm_rmep_conf_map_t::iterator cfm_rmep_conf_itr_t;

// We need to wrap vtss_appl_cfm_mep_conf_t in another structure, because the
// Remote MEP array cannot be modelled in the serializer otherwise.
typedef struct {
    vtss_appl_cfm_mep_conf_t mep_conf;
    cfm_rmep_conf_map_t      rmep_confs;
} cfm_mep_conf_t;

/******************************************************************************/
// cfm_base_vce_insertion_order_t
// Used in chips with VOE support.
// The insertion order of VCE controls the insertion point in the CLM VCAP.
// When a given MEP changes, it is desired that only that MEP will have its VCE
// entries updated, so that other MEPs can have their VCE entries unchanged.
// In order to be able to do that, we must insert the entries in a given order
// so that VCE entries belonging to a MEP's residence port are matched against
// first and after that comes VCE entries that belong to all-other-ports-but-
// the-mep's-residence port. These latter VCE entries are used in order to be
// able to have Up-MIPs and to be able to avoid leaking of CFM PDUs arriving on
// other ports than a given MEP's residence port.
//
// For MEP's on a given residence port, we must match on Port Down-MEPs before
// VLAN Down-MEPs - also for the all-other-ports-but-the-mep's-residence-port
// VCEs.
//
// The lower the insertion order enumeration, the higher priority.
/******************************************************************************/
typedef enum {
    CFM_BASE_VCE_INSERTION_ORDER_PORT_DOWN_MEP,
    CFM_BASE_VCE_INSERTION_ORDER_VLAN_DOWN_MEP,
    CFM_BASE_VCE_INSERTION_ORDER_PORT_DOWN_MEP_OTHER_PORTS,
    CFM_BASE_VCE_INSERTION_ORDER_VLAN_DOWN_MEP_OTHER_PORTS
} cfm_base_vce_insertion_order_t;

/******************************************************************************/
// cfm_base_vce_id_key_t
/******************************************************************************/
typedef struct {
    cfm_base_vce_insertion_order_t insertion_order;
    mesa_vce_id_t                  vce_id;
} cfm_base_vce_id_key_t;

/******************************************************************************/
// cfm_tce_insertion_order_t
// Used in chips with VOE support.
// The insertion order of TCE controls the insertion point in the ES0 VCAP.
// When an OAM frame is coming "from behind" and is about to egress a port,
// where VLAN and Port MEPs are instantiated, it must hit VLAN MEPs before it
// hits Port MEPs in order to get level filtered in case the VLAN MEP and Port
// MEP belong to the same VLAN. The reason for this is that a VLAN MEP must be
// on a higher level than a port level in the same VLAN, so it makes sense to
// hit the VLAN MEP and not the port MEP in that case.
//
// The lower the insertion order enumeration, the higher priority.
/******************************************************************************/
typedef enum {
    CFM_BASE_TCE_INSERTION_ORDER_VLAN_DOWN_MEP,
    CFM_BASE_TCE_INSERTION_ORDER_PORT_DOWN_MEP
} cfm_base_tce_insertion_order_t;

/******************************************************************************/
// cfm_base_tce_id_key_t
/******************************************************************************/
typedef struct {
    cfm_base_tce_insertion_order_t insertion_order;
    mesa_tce_id_t                  tce_id;
} cfm_base_tce_id_key_t;

/******************************************************************************/
// cfm_ace_insertion_order_t
// Used in chips without VOE support.
// The insertion order of ACE controls the insertion point in the IS2 VCAP.
// The ACEs are used to get OAM frames to the CPU when they ingress a port on
// which a MEP is installed. Here, port MEPs must be hit before VLAN MEPs.
//
// For each MEP, we install another set of ACEs to control forwarding and
// discarding of OAM frames arriving on other ports than the one where the MEP
// resides - to avoid leaking.
//
// The lower the insertion order enumeration, the higher priority.
/******************************************************************************/
typedef enum {
    CFM_BASE_ACE_INSERTION_ORDER_PORT_DOWN_MEP,
    CFM_BASE_ACE_INSERTION_ORDER_VLAN_DOWN_MEP,
    CFM_BASE_ACE_INSERTION_ORDER_LEAK_DOWN_MEP
} cfm_base_ace_insertion_order_t;

/******************************************************************************/
// cfm_base_ace_map_key_t
// For chips without VOE support, we use ACL rules to control CPU copying and
// leak-avoidance. The ACL module controls where our rules are inserted amongst
// other modules' rules, and we control the internal ordering of our rules
// according to cfm_base_ace_insertion_order_t.
//
// In order to limit the number of ACEs used, one MEP may contribute to another
// MEP's rules, in case they are installed with the same insertion_order, VID,
// and level.
/******************************************************************************/
typedef struct {
    cfm_base_ace_insertion_order_t insertion_order;
    mesa_vid_t                     vid;
    uint8_t                        level;
} cfm_base_ace_map_key_t;

typedef struct {
    // It requires up to three ACE IDs to match on a given MEP, because ACL
    // rules can't necessarily match all levels up to a given level in one go
    // because of the ACE's mask/value implementation.
    mesa_ace_id_t ace_ids[3];

    // How many MEPs have this rule as their primary rule?
    // A Port or VLAN MEP may re-use another MEP's rules and just add itself as
    // owner and set the ACE's key/action rules accordingly.
    // A MEP may also re-use another MEP's leak-prevention rules. It not only
    // needs to do it on its own level, but all possible rules at lower levels
    // as well. If it does it at its own level, the owner_ref_cnt increases, but
    // if it does it at a lower level, the owner_ref_cnt stays at its value.
    // When the last owner has backed out of the rule, owner_ref_cnt will be 0,
    // and the rule will be removed entirely.
    uint32_t owner_ref_cnt;

    // How many MEPs have a contribution to a given port?
    // For Port and VLAN MEPs, there can only by one MEP that matches a given
    // vid, level and port, so the port_ref_cnt can be 0 or 1.
    // For leaking rules, there can be 0, 1, or 2 MEPs contributing to the same
    // port on the same VID, because there can be one port/VLAN MEP on level X,
    // and one port/VLAN MEP on level Y, which both contribute to the rule with
    // the lowest level.
    // When port_ref_cnt[X] is 0, the ACE's action-port-list is updated to allow
    // forwarding of OAM frames to port X. Otherwise denied.
    CapArray<uint32_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_ref_cnt;
} cfm_base_ace_map_value_t;

typedef vtss::Map<cfm_base_ace_map_key_t, cfm_base_ace_map_value_t> cfm_base_ace_map_t;
typedef cfm_base_ace_map_t::iterator cfm_base_ace_itr_t;

typedef struct {
    // Configuration and state handled by cfm.cxx
    vtss_appl_cfm_mep_key_t     key; // Identifies this MEP
    vtss_appl_cfm_global_conf_t *global_conf;
    vtss_appl_cfm_md_conf_t     *md_conf;
    vtss_appl_cfm_ma_conf_t     *ma_conf;
    cfm_mep_conf_t              *mep_conf;
    cfm_global_state_t          *global_state;
    cfm_port_state_t            *port_state;

    // State handled by the MEP itself.
    vtss_appl_cfm_mep_status_t  status;
    cfm_ccm_state_t             ccm_state;
    uint32_t                    voe_event_mask; // Tells us which VOE events we have enabled.
    cfm_rmep_state_map_t        rmep_states;

    // Managed by cfm_base.cxx
    // On chips with VOE:
    mesa_voe_allocation_t       voe_alloc;
    mesa_voe_idx_t              voe_idx;
    mesa_iflow_id_t             iflow_id;
    cfm_base_vce_id_key_t       vce_keys[3]; // We can create up to three VCEs per MEP.
    mesa_eflow_id_t             eflow_id;
    cfm_base_tce_id_key_t       tce_keys[2]; // We can create up to two TCEs per MEP

    // On chips without VOE:
    cfm_base_ace_itr_t          ace_port_itr; // Points to the entry in the ACE port map that we are currently using. CFM_BASE_ace_map.end() if unused.
    cfm_base_ace_itr_t          ace_leak_itr; // Points to the entry in the ACE leak map that we are currently using. CFM_BASE_ace_map.end() if unused.
    mesa_port_no_t              old_port_no;  // This is needed to be able to remove contribution from a given port when a MEP moves from one port to another.

    // Managed by cfm_ccm.cxx
    uint8_t                     maid[MESA_OAM_MEG_ID_LENGTH];
    uint64_t                    fph; // ma_conf->ccm_interval converted to frames per hour.

    bool is_port_mep(void) const
    {
        // If MA's VLAN is non-zero, it's a VLAN MEP, otherwise it's a port MEP.
        return ma_conf->vlan == 0;
    }

    bool is_tagged(void) const
    {
        // If MA's VLAN is non-zero, it's a VLAN MEP, which is always tagged.
        // If MA's VLAN is zero, it's a port MEP, which may or not be tagged.
        return ma_conf->vlan != 0 || mep_conf->mep_conf.vlan != 0;
    }

    mesa_vid_t classified_vid_get(void) const
    {
        // Get the classified VID. If it's an untagged port MEP, it's the PVID.
        // Otherwise it's the MEP's VLAN, which can either be that of the MEP
        // itself or that of the MA.
        if (is_tagged()) {
            return mep_conf->mep_conf.vlan ? mep_conf->mep_conf.vlan : ma_conf->vlan;
        } else {
            return port_state->vlan_conf.pvid;
        }
    }

    bool classified_vid_gets_tagged(void) const
    {
        // Determine whether the MEP's classified VID gets tagged or not on
        // egress.
        vtss_appl_vlan_port_detailed_conf_t *vlan_conf = &port_state->vlan_conf;
        mesa_vid_t                          vid = classified_vid_get();

        switch (vlan_conf->tx_tag_type) {
        case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_THIS:
            // Only .untagged_vid becomes untagged
            return vid != vlan_conf->untagged_vid;

        case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_THIS:
            // If .untagged_vid == .pvid everything becomes tagged, otherwise PVID
            // becomes untagged.
            return vlan_conf->untagged_vid == vlan_conf->pvid ? true : vid != vlan_conf->pvid;

        case VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL:
            // Everything becomes tagged
            return true;

        case VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL:
            return false;

        default:
            T_EG(CFM_TRACE_GRP_CCM, "%s: Invalid tx_tag_type (%d)", key, vlan_conf->tx_tag_type);
            return false;
        }
    }
} cfm_mep_state_t;

const char *cfm_util_mep_state_change_to_str(cfm_mep_state_change_t change);
mesa_mac_t cfm_base_smac_get(const cfm_mep_state_t *mep_state);
mesa_rc    cfm_base_mep_state_init(cfm_mep_state_t *mep_state);
mesa_rc    cfm_base_mep_update(cfm_mep_state_t *mep_state, cfm_mep_state_change_t change);
mesa_rc    cfm_base_mep_statistics_clear(cfm_mep_state_t *mep_state);
void       cfm_base_frame_rx(cfm_mep_state_t *mep_state, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info);
void       cfm_base_mep_ok_notification_update(cfm_mep_state_t *mep_state);
void       cfm_base_init(cfm_global_state_t *global_state);

typedef vtss::Map<vtss_appl_cfm_mep_key_t, cfm_mep_state_t> cfm_mep_state_map_t;
typedef cfm_mep_state_map_t::iterator cfm_mep_state_itr_t;
extern cfm_mep_state_map_t CFM_mep_state_map;
extern cfm_global_state_t  CFM_global_state;

#endif /* _CFM_BASE_HXX_ */

