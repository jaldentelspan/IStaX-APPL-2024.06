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

#include "iec_mrp_base.hxx"   /* For ourselves                       */
#include "iec_mrp_pdu.hxx"    /* For PDU Rx/Tx                       */
#include "iec_mrp_api.h"      /* For iec_mrp_util_XXX()              */
#include "iec_mrp_expose.hxx" /* For mrp_notification_status         */
#include "iec_mrp_lock.hxx"   /* For MRP_LOCK_SCOPE()                */
#include "iec_mrp_trace.h"    /* For trace                           */
#include "mac_utils.hxx"      /* For operator==(mesa_mac_t)          */
#include "mgmt_api.h"         /* For mgmt_iport_list2txt()           */
#include "misc_api.h"         /* For misc_mac_txt()                  */
#include "syslog_api.h"       /* For S_PORT_E()                      */
#include "vlan_api.h"         /* For vlan_mgmt_port_conf_get()/set() */

// If multiple MRMs are detected on the ring, a notification is raised. This
// notification times out if no remote MRMs are detected after this many
// microseconds.
#define MRP_MULTIPLE_MRMS_NOTIF_TIMEOUT_USEC 10000000

// If multiple MIMs are detected on the ring, a notification is raised. This
// notification times out if no remote MIMs are detected after this many
// micrseconds.
#define MRP_MULTIPLE_MIMS_NOTIF_TIMEOUT_USEC 10000000

/******************************************************************************/
// mrp_rx_pdu_info_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mrp_rx_pdu_info_t &rx_pdu_info)
{
    char buf[37];

    o << "{pdu_type = "       << iec_mrp_util_pdu_type_to_str(rx_pdu_info.pdu_type)
      << ", sa = "            << rx_pdu_info.sa
      << ", seqID = "         << rx_pdu_info.sequence_id
      << ", uuid = "          << iec_mrp_util_domain_id_to_uuid(buf, sizeof(buf), rx_pdu_info.domain_id)
      << ", other_prio = "    << rx_pdu_info.other_prio
      << ", other_sa = "      << rx_pdu_info.other_sa
      << ", timestamp = "     << rx_pdu_info.timestamp
      << ", prio = "          << rx_pdu_info.prio
      << ", interval_msec = " << rx_pdu_info.interval_msec
      << ", blocked = "       << rx_pdu_info.blocked
      << ", in_id = "         << rx_pdu_info.in_id
      << ", rx_time_msecs = " << rx_pdu_info.rx_time_msecs
      << "}";

    return o;
}

/******************************************************************************/
// mrp_rx_pdu_info_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mrp_rx_pdu_info_t *rx_pdu_info)
{
    o << *rx_pdu_info;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// MRP_BASE_bool_to_mesa_port_state()
/******************************************************************************/
static mesa_mrp_port_state_t MRP_BASE_bool_to_mesa_port_state(bool forward)
{
    return forward ? MESA_MRP_PORT_STATE_FORWARDING : MESA_MRP_PORT_STATE_BLOCKED;
}

/******************************************************************************/
// MRP_BASE_mesa_port_state_to_str()
/******************************************************************************/
static const char *MRP_BASE_mesa_port_state_to_str(mesa_mrp_port_state_t state)
{
    switch (state) {
    case MESA_MRP_PORT_STATE_DISABLED:
        return "Disabled";

    case MESA_MRP_PORT_STATE_BLOCKED:
        return "Blocked";

    case MESA_MRP_PORT_STATE_FORWARDING:
        return "Forwarding";

    default:
        T_EG(MRP_TRACE_GRP_API, "Invalid port_state (%u)", state);
        return "INVALID!";
    }
}

/******************************************************************************/
// MRP_BASE_mesa_ring_role_to_str()
/******************************************************************************/
static const char *MRP_BASE_mesa_ring_role_to_str(mesa_mrp_ring_role_t role)
{
    switch (role) {
    case MESA_MRP_RING_ROLE_DISABLED:
        return "Disabled";

    case MESA_MRP_RING_ROLE_CLIENT:
        return "Client";

    case MESA_MRP_RING_ROLE_MANAGER:
        return "Manager";

    default:
        T_EG(MRP_TRACE_GRP_API, "Invalid role (%u)", role);
        return "INVALID!";
    }
}

/******************************************************************************/
// mesa_mrp_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_mrp_conf_t &conf)
{
    o << "{ring_role = "        << MRP_BASE_mesa_ring_role_to_str(conf.ring_role)
      << ", in_ring_role = "    << MRP_BASE_mesa_ring_role_to_str(conf.in_ring_role)
      << ", mra = "             << conf.mra
      << ", mra_priority = "    << conf.mra_priority
      << ", p_port = "          << conf.p_port
      << ", s_port = "          << conf.s_port
      << ", i_port = "          << conf.i_port
      << ", mac = "             << conf.mac
      << "}";

    return o;
}

/******************************************************************************/
// mesa_mrp_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_mrp_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_mrp_tst_loc_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_mrp_tst_loc_conf_t &conf)
{
    o << "{tst_interval = "    << conf.tst_interval << " usec"
      << ", tst_mon_count = "  << conf.tst_mon_count
      << ", itst_interval = "  << conf.itst_interval << " usec"
      << ", itst_mon_count = " << conf.itst_mon_count
      << "}";

    return o;
}

/******************************************************************************/
// mesa_mrp_tst_loc_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_mrp_tst_loc_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_mrp_best_mrm_t::operator<<()
// Used for tracing
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_mrp_best_mrm_t &best)
{
    o << "{mac = "     << best.mac
      << ", prio = 0x" << vtss::hex(best.prio)
      << "}";

    return o;
}

/******************************************************************************/
// mesa_mrp_best_mrm_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_mrp_best_mrm_t *best)
{
    o << *best;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_mrp_tst_copy_conf_t::operator<<()
// Used for tracing
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_mrp_tst_copy_conf_t &conf)
{
    o << "{tst_to_cpu = "   << conf.tst_to_cpu
      << ", itst_to_cpu = " << conf.itst_to_cpu
      << "}";

    return o;
}

/******************************************************************************/
// mesa_mrp_tst_copy_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_mrp_tst_copy_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_mrp_port_status_t::operator<<()
// Used for tracing
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_mrp_port_status_t &status)
{
    o << "{tst_loc = "        << status.tst_loc
      << ", itst_loc = "      << status.itst_loc
      << ", mrp_seen = "      << status.mrp_seen
      << ", mrp_proc_seen = " << status.mrp_proc_seen
      << ", dmac_err_seen = " << status.dmac_err_seen
      << ", vers_err_seen = " << status.vers_err_seen
      << "}";

    return o;
}

/******************************************************************************/
// mesa_mrp_port_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_mrp_port_status_t *status)
{
    o << *status;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_mrp_status_t_t::operator<<()
// Used for tracing
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_mrp_status_t &status)
{
    o << "{p_status = "  << status.p_status
      << ", s_status = " << status.s_status
      << ", i_status = " << status.i_status
      << "}";

    return o;
}

/******************************************************************************/
// mesa_mrp_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_mrp_status_t *status)
{
    o << *status;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// MRP_BASE_sm_state_to_str()
/******************************************************************************/
static const char *MRP_BASE_sm_state_to_str(mrp_sm_state_t sm_state)
{
    switch (sm_state) {
    case MRP_SM_STATE_POWER_ON:
        return "Power On";

    case MRP_SM_STATE_AC_STAT1:
        return "AC_STAT1";

    case MRP_SM_STATE_PRM_UP:
        return "PRM_UP";

    case MRP_SM_STATE_CHK_RO:
        return "CHK_RO";

    case MRP_SM_STATE_CHK_RC:
        return "CHK_RC";

    case MRP_SM_STATE_DE_IDLE:
        return "DE_IDLE";

    case MRP_SM_STATE_PT:
        return "PT";

    case MRP_SM_STATE_DE:
        return "DE";

    case MRP_SM_STATE_PT_IDLE:
        return "PT_IDLE";

    default:
        T_EG(MRP_TRACE_GRP_BASE, "Invalid sm_state (%d)", sm_state);
        return "UNKNOWN!";
    }
}

/******************************************************************************/
// MRP_BASE_in_sm_state_to_str()
/******************************************************************************/
static const char *MRP_BASE_in_sm_state_to_str(mrp_in_sm_state_t in_sm_state)
{
    switch (in_sm_state) {
    case MRP_IN_SM_STATE_POWER_ON:
        return "Power On";

    case MRP_IN_SM_STATE_AC_STAT1:
        return "AC_STAT1";

    case MRP_IN_SM_STATE_CHK_IO:
        return "CHK_IO";

    case MRP_IN_SM_STATE_CHK_IC:
        return "CHK_IC";

    case MRP_IN_SM_STATE_PT:
        return "PT";

    case MRP_IN_SM_STATE_IP_IDLE:
        return "IP_IDLE";

    default:
        T_EG(MRP_TRACE_GRP_BASE, "Invalid in_sm_state (%d)", in_sm_state);
        return "UNKNOWN!";
    }
}

//*****************************************************************************/
// MRP_BASE_pdu_dmac_to_str()
//*****************************************************************************/
static const char *MRP_BASE_pdu_dmac_to_str(mrp_pdu_dmac_type_t dmac_type)
{
    switch (dmac_type) {
    case MRP_PDU_DMAC_TYPE_MC_TEST:
        return "MC_TEST";

    case MRP_PDU_DMAC_TYPE_MC_CONTROL:
        return "MC_CONTROL";

    case MRP_PDU_DMAC_TYPE_MC_INTEST:
        return "MC_INTEST";

    case MRP_PDU_DMAC_TYPE_MC_INCONTROL:
        return "MC_INCONTROL";

    default:
        T_EG(MRP_TRACE_GRP_BASE, "Invalid dmac_type (%d)", dmac_type);
        return "Unknown";
    }
}

//*****************************************************************************/
// MRP_BASE_fdb_flush()
// Corresponds to ClearLocalFDB() on p. 103.
/******************************************************************************/
static void MRP_BASE_fdb_flush(mrp_state_t *mrp_state)
{
    mrp_port_state_t               *port_state;
    vtss_appl_iec_mrp_port_type_t  port_type;
    mesa_rc                        rc;

    T_IG(MRP_TRACE_GRP_BASE, "%u: Flushing FDB", mrp_state->inst);

    // Count the number of flushes.
    mrp_state->status.flush_cnt++;

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        port_state = mrp_state->port_states[port_type];

        if (!port_state) {
            // Interconnection port not in use.
            continue;
        }

        T_RG(MRP_TRACE_GRP_API, "%u:%s: mesa_mac_table_port_flush(%u)", mrp_state->inst, port_type, port_state->port_no);
        if ((rc = mesa_mac_table_port_flush(nullptr, port_state->port_no)) != VTSS_RC_OK) {
            T_EG(MRP_TRACE_GRP_API, "%u:%s: mesa_mac_table_port_flush(%u) failed: %s", mrp_state->inst, port_type, port_state->port_no, error_txt(rc));
            mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        }
    }
}

//*****************************************************************************/
// MRP_BASE_fdb_clear()
// Corresponds to ClearFDB() on p. 103.
/******************************************************************************/
static void MRP_BASE_fdb_clear(mrp_state_t *mrp_state, uint16_t interval_msec)
{
    if (interval_msec == 0) {
        MRP_BASE_fdb_flush(mrp_state);
    } else {
        T_DG(MRP_TRACE_GRP_BASE, "%u: Starting Clear FDB timer (%u ms)", mrp_state->inst, interval_msec);
        mrp_timer_start(mrp_state->fdb_clear_timer, interval_msec * 1000, false);
    }
}

//*****************************************************************************/
// MRP_BASE_ace_update()

// Rules for MC_TEST and MC_CONTROL PDUs:
// [0] = Forwarding rule when PDUs arrive on ring ports. Can only go to other
//       ring port if both ring ports are forwarding.
// [1] = I/C block rule. These PDU types must not cross the I/C port.
//
// Rules for MC_INTEST PDUs:
// [0] = Forwarding rule when PDUs arrive on ring ports.
// [1] = Forwarding rule when PDUs arrive on I/C port.
//
// Rules for MC_INCONTROL PDUs:
// [0] = Copy-to-CPU rule for frames arriving on ring ports and I/C port. Are
//       always S/W-forwarded.
// [1] = Not used.
//
// One of the reasons for not forwarding PDUs is that if the user uses MEPs for
// signal fail, there's no guarantee that link is down when a signal fail
// occurs, so we must be sure to block the ports as needed - even when we are an
// MRC.
//
// This function needs only to be invoked when operational role or forwarding of
// a port changes.
//
// It does not change static, non-dynamic fields, so only CPU copy and
// destination ports need be changed. The rest is set up in
// MRP_BASE_ace_prepare_step2()
//*****************************************************************************/
static mesa_rc MRP_BASE_ace_update(mrp_state_t *mrp_state, mrp_pdu_dmac_type_t dmac_type, bool first_rule)
{
    mesa_port_no_t   port1_no, port2_no, porti_no;
    acl_entry_conf_t *ace_conf = &mrp_state->ace_conf[dmac_type - MRP_PDU_DMAC_TYPE_MC_TEST][first_rule ? 0 : 1];
    acl_entry_conf_t new_ace_conf = *ace_conf /* 228 bytes */;
    mesa_ace_id_t    old_ace_id   = new_ace_conf.id;
    bool             forward, port1_fwd, port2_fwd, porti_fwd;
    mesa_rc          rc;

    port1_no = mrp_state->port_states[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1]->port_no;
    port2_no = mrp_state->port_states[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2]->port_no;
    porti_no = mrp_state->using_port_type(VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION) ? mrp_state->port_states[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION]->port_no : MESA_PORT_NO_NONE;

    port1_fwd = mrp_state->status.port_status[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1].forwarding;
    port2_fwd = mrp_state->status.port_status[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2].forwarding;
    porti_fwd = mrp_state->status.port_status[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION].forwarding;

    switch (dmac_type) {
    case MRP_PDU_DMAC_TYPE_MC_TEST:
    case MRP_PDU_DMAC_TYPE_MC_CONTROL:
        // The MC_TEST DMAC type comprises MRP_Test, MRP_TestMgrNAck and
        // MRP_TestPropagate PDUs.

        // The MC_CONTROL DMAC type comprises MRP_LinkChange,
        // MRP_TopologyChange and MRP_Option PDUs other than MRP_TestMgrNAck
        // and MRP_TestPropagate.

        if (first_rule) {
            // Only forward between ring ports if we are an MRC and both ring
            // ports are forwarding
            forward = mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRC && port1_fwd && port2_fwd;
            new_ace_conf.action.port_list[port1_no] = forward;
            new_ace_conf.action.port_list[port2_no] = forward;

            if (dmac_type == MRP_PDU_DMAC_TYPE_MC_TEST) {
                // MC_TEST: Copy to CPU if we are an MRM or an MRC originally
                // configured as an MRA.
                new_ace_conf.action.force_cpu = mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRM || mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRA;
            }
        } else {
            // Second rule. Block all MC_TEST and MC_CONTROL PDUs from an I/C
            // port.
            if (porti_no == MESA_PORT_NO_NONE) {
                // We don't have an interconnection port. Don't create a rule
                return VTSS_RC_OK;
            }

            // No dynamic content, but must be installed.
        }

        break;

    case MRP_PDU_DMAC_TYPE_MC_INTEST:
        // This DMAC type comprises MRP_InTest PDUs.

        if (first_rule) {
            // Forwarding rule for PDUs arriving on ring ports.

            // If we are a MIM, we don't forward these PDUs anywhere.
            // If we are a MIC, we only forward them to the I/C port.
            // If we are neither, we forward them if both ring ports are
            // forwarding.
            if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM) {
                // No dynamic content
            } else if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIC) {
                // Only forward to I/C port if it's forwarding.
                new_ace_conf.action.port_list[porti_no] = porti_fwd;
            } else {
                // MRM/MRC w/o MIM/MIC.
                // Forward between ring ports only if both are forwarding.
                forward = port1_fwd && port2_fwd;
                new_ace_conf.action.port_list[port1_no] = forward;
                new_ace_conf.action.port_list[port2_no] = forward;
            }
        } else {
            // Forwarding rule for PDUs arriving on I/C port.
            if (porti_no == MESA_PORT_NO_NONE) {
                // We don't have an interconnection port. Don't create a rule
                return VTSS_RC_OK;
            }

            if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIC) {
                // Forward to the forwarding ring ports if a MIC
                new_ace_conf.action.port_list[port1_no] = port1_fwd;
                new_ace_conf.action.port_list[port2_no] = port2_fwd;
            } else {
                // A MIM consumes all MRP_InTest PDUs.
            }
        }

        break;

    case MRP_PDU_DMAC_TYPE_MC_INCONTROL:
        // This DMAC comprises MRP_InLinkChange, MRP_InLinkStatusPoll and
        // MRP_InTopologyChange PDUs.

        if (first_rule) {
            // If we are a MIM or a MIC, these PDUs are always S/W forwarded in
            // order to be able to support multiple interconnections (three or
            // more rings). The reason is that we cannot match on these PDUs'
            // IID, so we need to handle it all in S/W.
            // If we are an MRM/MRC w/o MIM/MIC, we handle forwarding in H/W.

            if (porti_no != MESA_PORT_NO_NONE) {
                // We are a MIM or a MIC. No dynamic content.
            } else {
                // We're an MRM/MRC without MIM/MIC. Forward blindly if both
                // ring ports are forwarding.
                forward = port1_fwd && port2_fwd;
                new_ace_conf.action.port_list[port1_no] = forward;
                new_ace_conf.action.port_list[port2_no] = forward;

                // An MRM needs the MRP_InTopologyChange PDU to possibly
                // transmit an MRP_TopologyChange PDU on its own ring.
                new_ace_conf.action.force_cpu = mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRM;
            }
        } else {
            // Second rule not used.
            return VTSS_RC_OK;
        }

        break;

    default:
        T_EG(MRP_TRACE_GRP_BASE, "Invalid dmac_type (%d)", dmac_type);
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        return VTSS_APPL_IEC_MRP_RC_INTERNAL_ERROR;
    }

    // Time to install it if not already installed.
    if (new_ace_conf.id != ACL_MGMT_ACE_ID_NONE) {
        // Already installed. Check to see if the configuration has changed.
        if (memcmp(&new_ace_conf, ace_conf, sizeof(new_ace_conf)) == 0) {
            // No changes
            T_DG(MRP_TRACE_GRP_ACL, "%u: No ACL changes for %s", mrp_state->inst, MRP_BASE_pdu_dmac_to_str(dmac_type));
            return VTSS_RC_OK;
        }
    }

    // The order of these rules doesn't matter, so just place it last within the
    // MRP group of ACEs.
    if ((rc = acl_mgmt_ace_add(ACL_USER_IEC_MRP, ACL_MGMT_ACE_ID_NONE, &new_ace_conf)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_ACL, "%u: acl_mgmt_ace_add(%s[%d], 0x%x) failed: %s", mrp_state->inst, MRP_BASE_pdu_dmac_to_str(dmac_type), first_rule, old_ace_id, error_txt(rc));
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        return VTSS_APPL_IEC_MRP_RC_HW_RESOURCES;
    }

    T_IG(MRP_TRACE_GRP_ACL, "%u: acl_mgmt_ace_add(%s[%d]) => ace_id = 0x%x->0x%x", mrp_state->inst, MRP_BASE_pdu_dmac_to_str(dmac_type), first_rule, old_ace_id, new_ace_conf.id);
    *ace_conf = new_ace_conf;

    return VTSS_RC_OK;
}

//*****************************************************************************/
// MRP_BASE_ace_update_all()
// This function only needs to be invoked when status.oper_role or forwarding of
// a given port changes.
/******************************************************************************/
static mesa_rc MRP_BASE_ace_update_all(mrp_state_t *mrp_state)
{
    mrp_pdu_dmac_type_t dmac_type;
    int                 i, j;

    for (i = 0; i < ARRSZ(mrp_state->ace_conf); i++) {
        dmac_type = (mrp_pdu_dmac_type_t)(i + MRP_PDU_DMAC_TYPE_MC_TEST);

        for (j = 0; j < ARRSZ(mrp_state->ace_conf[i]); j++) {
            VTSS_RC(MRP_BASE_ace_update(mrp_state, dmac_type, j == 0));
        }
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// MRP_BASE_ace_remove()
/******************************************************************************/
static mesa_rc MRP_BASE_ace_remove(mrp_state_t *mrp_state)
{
    mrp_pdu_dmac_type_t dmac_type;
    int                 i, j;
    mesa_rc             rc, first_encountered_rc = VTSS_RC_OK;

    // Delete all ACEs from the ACL module
    for (i = 0; i < ARRSZ(mrp_state->ace_conf); i++) {
        dmac_type = (mrp_pdu_dmac_type_t)(i + MRP_PDU_DMAC_TYPE_MC_TEST);

        for (j = 0; j < ARRSZ(mrp_state->ace_conf[i]); j++) {
            mesa_ace_id_t &id = mrp_state->ace_conf[i][j].id;

            if (id == ACL_MGMT_ACE_ID_NONE) {
                continue;
            }

            T_IG(MRP_TRACE_GRP_ACL, "%u: acl_mgmt_ace_del(%s[%d], 0x%x)", mrp_state->inst, MRP_BASE_pdu_dmac_to_str(dmac_type), j, id);
            if ((rc = acl_mgmt_ace_del(ACL_USER_IEC_MRP, id)) != VTSS_RC_OK) {
                T_EG(MRP_TRACE_GRP_ACL, "%u: acl_mgmt_ace_del(%s[%d], 0x%x) failed: %s", mrp_state->inst, MRP_BASE_pdu_dmac_to_str(dmac_type), j, id, error_txt(rc));
                mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;

                if (first_encountered_rc == VTSS_RC_OK) {
                    first_encountered_rc = rc;
                }
            }

            id = ACL_MGMT_ACE_ID_NONE;
        }
    }

    return first_encountered_rc;
}

//*****************************************************************************/
// MRP_BASE_ace_prepare_step2()
// This function prepares all the non-variable ingress/egress and CPU-copy for
// the ACEs, so that we only update the variable stuff in MRP_BASE_ace_update().
//
// See MRP_BASE_ace_update() for how to interpret the eight different rules, we
// are invoked with.
//*****************************************************************************/
static void MRP_BASE_ace_prepare_step2(mrp_state_t *mrp_state, mrp_pdu_dmac_type_t dmac_type, bool first_rule)
{
    mesa_port_no_t   port1_no, port2_no, porti_no;
    acl_entry_conf_t &ace_conf = mrp_state->ace_conf[dmac_type - MRP_PDU_DMAC_TYPE_MC_TEST][first_rule ? 0 : 1];

    // Match on the MRP PDU DMAC addresses. We have on ACE per DMAC in order
    // to be able to control per-type forwarding
    ace_conf.frame.etype.dmac.value[sizeof(ace_conf.frame.etype.dmac.value) - 1] = dmac_type;

    port1_no = mrp_state->port_states[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1]->port_no;
    port2_no = mrp_state->port_states[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2]->port_no;
    porti_no = mrp_state->using_port_type(VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION) ? mrp_state->port_states[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION]->port_no : MESA_PORT_NO_NONE;

    switch (dmac_type) {
    case MRP_PDU_DMAC_TYPE_MC_TEST:
    case MRP_PDU_DMAC_TYPE_MC_CONTROL:
        // The MC_TEST DMAC type comprises MRP_Test, MRP_TestMgrNAck and
        // MRP_TestPropagate PDUs.

        // The MC_CONTROL DMAC type comprises MRP_LinkChange,
        // MRP_TopologyChange and other MRP_Option PDUs than MRP_TestMgrNAck
        // and MRP_TestPropagate.

        if (first_rule) {
            // Rule that matches ring ports only
            ace_conf.port_list[port1_no] = true;
            ace_conf.port_list[port2_no] = true;

            if (dmac_type == MRP_PDU_DMAC_TYPE_MC_TEST) {
                // MC_TEST: Copy to CPU if we are an MRM or an MRC originally
                // configured as an MRA.
                ace_conf.action.force_cpu = mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRM || mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRA;
            } else {
                // MC_CONTROL: Always copy to CPU.
                ace_conf.action.force_cpu = true;
            }
        } else {
            // Second rule. Block all MC_TEST and MC_CONTROL PDUs from an I/C
            // port.
            if (porti_no == MESA_PORT_NO_NONE) {
                // We don't have an interconnection port. Don't create a rule
                return;
            }

            ace_conf.port_list[porti_no] = true;

            // force_cpu already false
        }

        break;

    case MRP_PDU_DMAC_TYPE_MC_INTEST:
        // This DMAC type comprises MRP_InTest PDUs.

        if (first_rule) {
            // Forwarding rule for PDUs arriving on ring ports.
            ace_conf.port_list[port1_no] = true;
            ace_conf.port_list[port2_no] = true;

            // If we are a MIM, we don't forward these PDUs anywhere.
            // If we are a MIC, we only forward them to the I/C port.
            // If we are neither, we forward them if both ring ports are
            // forwarding.
            if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM) {
                // Destination port list already cleared.
                // Copy to CPU if we're in RC mode.
                ace_conf.action.force_cpu = mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_RC;
            } else if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIC) {
                // Only variable stuff
            } else {
                // MRM/MRC w/o MIM/MIC.
                // Only variable stuff
            }
        } else {
            // Forwarding rule for PDUs arriving on I/C port.
            if (porti_no == MESA_PORT_NO_NONE) {
                // We don't have an interconnection port. Don't create a rule
                return;
            }

            ace_conf.port_list[porti_no] = true;

            // Only copy to CPU if MIM-RC
            ace_conf.action.force_cpu = mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM && mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_RC;
        }

        break;

    case MRP_PDU_DMAC_TYPE_MC_INCONTROL:
        // This DMAC comprises MRP_InLinkChange, MRP_InLinkStatusPoll and
        // MRP_InTopologyChange PDUs.

        if (first_rule) {
            // If we are a MIM or a MIC, these PDUs are always S/W forwarded in
            // order to be able to support multiple interconnections (three or
            // more rings). The reason is that we cannot match on these PDUs'
            // IID, so we need to handle it all in S/W.
            // If we are an MRM/MRC w/o MIM/MIC, we handle forwarding in H/W.

            // We match on all valid ports.
            ace_conf.port_list[port1_no] = true;
            ace_conf.port_list[port2_no] = true;

            if (porti_no != MESA_PORT_NO_NONE) {
                // We are a MIM or a MIC. Also match on I/C port.
                ace_conf.port_list[porti_no] = true;

                // But don't forward to any ports. We handle forwarding in S/W.
                // Destination port list already cleared.

                // MIMs and MICs need these frames.
                ace_conf.action.force_cpu = true;
            } else {
                // Only variable stuff
            }
        } else {
            // Second rule not used.
            return;
        }

        break;

    default:
        T_EG(MRP_TRACE_GRP_BASE, "Invalid dmac_type (%d)", dmac_type);
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        break;
    }
}

//*****************************************************************************/
// MRP_BASE_ace_prepare()
// This function prepares basic ACE configuration in ace_conf[][].
// This is subsequently used by MRP_BASE_ace_update() to add variable
// configuration and install to H/W.
/******************************************************************************/
static mesa_rc MRP_BASE_ace_prepare(mrp_state_t *mrp_state)
{
    acl_entry_conf_t    *ace_conf = &mrp_state->ace_conf[0][0]; // Start by filling out the first ACE.
    mrp_pdu_dmac_type_t dmac_type;
    int                 i, j;
    mesa_rc             rc;

    T_IG(MRP_TRACE_GRP_ACL, "%u: Enter", mrp_state->inst);

    if (ace_conf->id != ACL_MGMT_ACE_ID_NONE) {
        T_EG(MRP_TRACE_GRP_ACL, "%u: An ACE (ID = %u) already exists, but we are going to add a new", mrp_state->inst, ace_conf->id);
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        return VTSS_APPL_IEC_MRP_RC_INTERNAL_ERROR;
    }

    T_DG(MRP_TRACE_GRP_ACL, "%u: acl_mgmt_ace_init()", mrp_state->inst);
    if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ETYPE, ace_conf)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_ACL, "%u: acl_mgmt_ace_init() failed: %s", mrp_state->inst, error_txt(rc));
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        return VTSS_APPL_IEC_MRP_RC_INTERNAL_ERROR;
    }

    // Create the basic, non-variable ACE configuration for this MRP instance.
    ace_conf->port_list.clear_all();
    ace_conf->action.port_list.clear_all();

    ace_conf->isid                       = VTSS_ISID_LOCAL;
    ace_conf->frame.etype.etype.value[0] = (MRP_ETYPE >> 8) & 0xFF;
    ace_conf->frame.etype.etype.value[1] = (MRP_ETYPE >> 0) & 0xFF;
    ace_conf->frame.etype.etype.mask[0]  = 0xFF;
    ace_conf->frame.etype.etype.mask[1]  = 0xFF;
    ace_conf->id                         = ACL_MGMT_ACE_ID_NONE;

    // Prepare to forward only to ports in action.port_list - and possibly to
    // the CPU.
    ace_conf->action.port_action         = MESA_ACL_PORT_ACTION_FILTER;
    ace_conf->action.cpu_queue           = PACKET_XTR_QU_OAM;
    ace_conf->action.force_cpu           = false;

    // Never match on VID (see clause 8.1.2, Table 17.d).
    ace_conf->vid.value = 0;
    ace_conf->vid.mask  = 0;

    // Match on the MRP PDU DMAC addresses. In the loop below, we change the
    // last byte of every DMAC to match a particular DMAC type.
    memcpy(ace_conf->frame.etype.dmac.value, mrp_multicast_dmac.addr, sizeof(ace_conf->frame.etype.dmac.value));
    memset(ace_conf->frame.etype.dmac.mask,  0xFF,                    sizeof(ace_conf->frame.etype.dmac.mask));

    // Copy this first ACE to all the other ACEs.
    for (i = 0; i < ARRSZ(mrp_state->ace_conf); i++) {
        for (j = 0; j < ARRSZ(mrp_state->ace_conf[i]); j++) {
            if (i || j) {
                // The remaining ACEs are so far identical to the first.
                ace_conf  = &mrp_state->ace_conf[i][j];
                *ace_conf = mrp_state->ace_conf[0][0];
            }
        }
    }

    // Loop again, while configuring all the static ingress, egress and CPU copy
    // data per type
    for (i = 0; i < ARRSZ(mrp_state->ace_conf); i++) {
        dmac_type = (mrp_pdu_dmac_type_t)(i + MRP_PDU_DMAC_TYPE_MC_TEST);

        for (j = 0; j < ARRSZ(mrp_state->ace_conf[i]); j++) {
            // Do the second preparation step, which configures all the static
            // ingress, egress, and CPU copy data per type.
            MRP_BASE_ace_prepare_step2(mrp_state, dmac_type, j == 0);
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// MRP_BASE_appl_to_mesa_role()
/******************************************************************************/
static mesa_mrp_ring_role_t MRP_BASE_appl_to_mesa_role(mrp_state_t *mrp_state, vtss_appl_iec_mrp_role_t role)
{
    switch (role) {
    case VTSS_APPL_IEC_MRP_ROLE_MRC:
        return MESA_MRP_RING_ROLE_CLIENT;

    case VTSS_APPL_IEC_MRP_ROLE_MRM:
    case VTSS_APPL_IEC_MRP_ROLE_MRA:
        return MESA_MRP_RING_ROLE_MANAGER;

    default:
        T_EG(MRP_TRACE_GRP_API, "%u: Invalid role (%u)", mrp_state->inst, role);
        return MESA_MRP_RING_ROLE_DISABLED;
    }
}

/******************************************************************************/
// MRP_BASE_appl_to_mesa_in_role()
/******************************************************************************/
static mesa_mrp_ring_role_t MRP_BASE_appl_to_mesa_in_role(mrp_state_t *mrp_state, vtss_appl_iec_mrp_in_role_t in_role)
{
    switch (in_role) {
    case VTSS_APPL_IEC_MRP_IN_ROLE_NONE:
        return MESA_MRP_RING_ROLE_DISABLED;

    case VTSS_APPL_IEC_MRP_IN_ROLE_MIC:
        return MESA_MRP_RING_ROLE_CLIENT;

    case VTSS_APPL_IEC_MRP_IN_ROLE_MIM:
        return MESA_MRP_RING_ROLE_MANAGER;

    default:
        T_EG(MRP_TRACE_GRP_API, "%u: Invalid in_role (%u)", mrp_state->inst, in_role);
        return MESA_MRP_RING_ROLE_DISABLED;
    }
}

/******************************************************************************/
// MRP_BASE_port_type_to_port_no()
/******************************************************************************/
static mesa_port_no_t MRP_BASE_port_type_to_port_no(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type)
{
    if (mrp_state->port_states[port_type]) {
        return mrp_state->port_states[port_type]->port_no;
    }

    return MESA_PORT_NO_NONE;
}

/******************************************************************************/
// MRP_BASE_mesa_conf_add()
/******************************************************************************/
static mesa_rc MRP_BASE_mesa_conf_add(mrp_state_t *mrp_state)
{
    mesa_mrp_conf_t &conf = mrp_state->mesa_conf;
    mesa_rc         rc;

    vtss_clear(conf);

    conf.ring_role       = MRP_BASE_appl_to_mesa_role(   mrp_state, mrp_state->conf.role);
    conf.in_ring_role    = MRP_BASE_appl_to_mesa_in_role(mrp_state, mrp_state->conf.in_role);
    conf.mra             = mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRA;
    conf.mra_priority    = mrp_state->conf.mrm.prio;
    conf.in_rc_mode      = mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_RC;
    conf.p_port          = MRP_BASE_port_type_to_port_no(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1);
    conf.s_port          = MRP_BASE_port_type_to_port_no(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2);
    conf.i_port          = MRP_BASE_port_type_to_port_no(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION);
    conf.mac             = MRP_chassis_mac;

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_add(%u, %s)", mrp_state->inst, mrp_state->mrp_idx(), conf);
    if ((rc = mesa_mrp_add(nullptr, mrp_state->mrp_idx(), &conf)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_add(%u, %s) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), conf, error_txt(rc));

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        rc = VTSS_APPL_IEC_MRP_RC_INTERNAL_ERROR;
    }

    mrp_state->added_to_mesa = rc == VTSS_RC_OK;

    return rc;
}

/******************************************************************************/
// MRP_BASE_mesa_conf_del()
/******************************************************************************/
static mesa_rc MRP_BASE_mesa_conf_del(mrp_state_t *mrp_state)
{
    mesa_rc rc;

    if (!mrp_state->added_to_mesa) {
        return VTSS_RC_OK;
    }

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_del(%u)", mrp_state->inst, mrp_state->mrp_idx());
    if ((rc = mesa_mrp_del(nullptr, mrp_state->mrp_idx())) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_del(%u) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), error_txt(rc));

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        rc = VTSS_APPL_IEC_MRP_RC_INTERNAL_ERROR;
    }

    return rc;
}

/******************************************************************************/
// MRP_BASE_mesa_intr_set()
/******************************************************************************/
static mesa_rc MRP_BASE_mesa_intr_set(mrp_state_t *mrp_state, bool activate)
{
    bool     enable;
    uint32_t mask;
    mesa_rc  rc;

    if (!mrp_state->added_to_mesa) {
        // We don't have a live instance we can enable or disable interrupts on.
        return VTSS_RC_OK;
    }

    // We enable interrupts if we are activating and are either an MRM, an MRA
    // or a MIM.
    enable = activate && (mrp_state->conf.role != VTSS_APPL_IEC_MRP_ROLE_MRC || mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM);

    if (enable) {
        mask = MESA_MRP_EVENT_MASK_NONE;

        if (mrp_state->conf.role != VTSS_APPL_IEC_MRP_ROLE_MRC) {
            mask |= MESA_MRP_EVENT_MASK_TST_LOC;
        }

        if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM) {
            mask |= MESA_MRP_EVENT_MASK_ITST_LOC;
        }
    } else {
        // Disable all interrupts.
        mask = MESA_MRP_EVENT_MASK_ALL;
    }

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_event_mask_set(%u, 0x%x, %d)", mrp_state->inst, mrp_state->mrp_idx(), mask, enable);
    if ((rc = mesa_mrp_event_mask_set(nullptr, mrp_state->mrp_idx(), mask, enable)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_event_mask_set(%u, 0x%x, %d) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), mask, enable, error_txt(rc));

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        rc = VTSS_APPL_IEC_MRP_RC_INTERNAL_ERROR;
    }

    return rc;
}

/******************************************************************************/
// MRP_BASE_mesa_loc_conf_set()
/******************************************************************************/
static mesa_rc MRP_BASE_mesa_loc_conf_set(mrp_state_t *mrp_state, uint32_t tst_interval_usec, uint32_t itst_interval_usec)
{
    mesa_mrp_tst_loc_conf_t &loc_conf = mrp_state->mesa_loc_conf;
    mesa_rc                 rc;

    vtss_clear(loc_conf);

    // We can safely call this function whether or not we are MRM, MRA, or MIM.
    loc_conf.tst_interval   = tst_interval_usec;
    loc_conf.tst_mon_count  = mrp_state->mrm_timing.test_monitoring_cnt;
    loc_conf.itst_interval  = itst_interval_usec;
    loc_conf.itst_mon_count = mrp_state->mim_timing.test_monitoring_cnt;

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_tst_loc_conf_set(%u, %s)", mrp_state->inst, mrp_state->mrp_idx(), loc_conf);
    if ((rc = mesa_mrp_tst_loc_conf_set(nullptr, mrp_state->mrp_idx(), &loc_conf)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_tst_loc_conf_set(%u, %s) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), loc_conf, error_txt(rc));

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        return VTSS_APPL_IEC_MRP_RC_INTERNAL_ERROR;
    }

    return rc;
}

/******************************************************************************/
// MRP_BASE_mesa_ring_role_set()
/******************************************************************************/
static void MRP_BASE_mesa_ring_role_set(mrp_state_t *mrp_state)
{
    mesa_mrp_ring_role_t mesa_ring_role = MRP_BASE_appl_to_mesa_role(mrp_state, mrp_state->status.oper_role);
    mesa_rc              rc;

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_ring_role_set(%u, %s)", mrp_state->inst, mrp_state->mrp_idx(), MRP_BASE_mesa_ring_role_to_str(mesa_ring_role));
    if ((rc = mesa_mrp_ring_role_set(nullptr, mrp_state->mrp_idx(), mesa_ring_role)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_ring_role_set(%u, %s) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), MRP_BASE_mesa_ring_role_to_str(mesa_ring_role), error_txt(rc));

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
    }
}

/******************************************************************************/
// MRP_BASE_mesa_tst_hitme_once()
/******************************************************************************/
static void MRP_BASE_mesa_tst_hitme_once(mrp_state_t *mrp_state)
{
    mesa_rc rc;

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_tst_hitme_once(%u)", mrp_state->inst, mrp_state->mrp_idx());
    if ((rc = mesa_mrp_tst_hitme_once(nullptr, mrp_state->mrp_idx())) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_tst_hitme_once(%u) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), error_txt(rc));

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
    }
}

/******************************************************************************/
// MRP_BASE_mesa_itst_hitme_once()
/******************************************************************************/
static void MRP_BASE_mesa_itst_hitme_once(mrp_state_t *mrp_state)
{
    mesa_rc rc;

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_itst_hitme_once(%u)", mrp_state->inst, mrp_state->mrp_idx());
    if ((rc = mesa_mrp_itst_hitme_once(nullptr, mrp_state->mrp_idx())) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_itst_hitme_once(%u) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), error_txt(rc));

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
    }
}

/******************************************************************************/
// MRP_BASE_mesa_ring_ports_swap()
/******************************************************************************/
static void MRP_BASE_mesa_ring_ports_swap(mrp_state_t *mrp_state)
{
    mesa_port_no_t port_no = mrp_state->port_states[mrp_state->vars.prm_ring_port]->port_no;
    mesa_rc        rc;

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_primary_port_set(%u, %u)", mrp_state->inst, mrp_state->mrp_idx(), port_no);
    if ((rc = mesa_mrp_primary_port_set(nullptr, mrp_state->mrp_idx(), port_no)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_primary_port_set(%u, %u) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), port_no, error_txt(rc));

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
    }
}

/******************************************************************************/
// MRP_BASE_mesa_port_state_set()
/******************************************************************************/
static void MRP_BASE_mesa_port_state_set(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, bool forward)
{
    mesa_port_no_t        port_no = mrp_state->port_states[port_type]->port_no;
    mesa_mrp_port_state_t mesa_port_state = MRP_BASE_bool_to_mesa_port_state(forward);
    mesa_rc               rc;

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_port_state_set(%u, %u, %s)", mrp_state->inst, mrp_state->mrp_idx(), port_no, MRP_BASE_mesa_port_state_to_str(mesa_port_state));
    if ((rc = mesa_mrp_port_state_set(nullptr, mrp_state->mrp_idx(), port_no, mesa_port_state)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_port_state_set(%u, %u, %s) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), port_no, MRP_BASE_mesa_port_state_to_str(mesa_port_state), error_txt(rc));

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
    }
}

/******************************************************************************/
// MRP_BASE_mesa_best_mrm_set()
/******************************************************************************/
static void MRP_BASE_mesa_best_mrm_set(mrp_state_t *mrp_state)
{
    mesa_mrp_best_mrm_t mesa_best_mrm;
    mesa_rc             rc;

    if (!MRP_hw_support) {
        return;
    }

    vtss_clear(mesa_best_mrm);
    mesa_best_mrm.mac  = mrp_state->vars.HO_BestMRM_SA;
    mesa_best_mrm.prio = mrp_state->vars.HO_BestMRM_Prio;

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_best_mrm_set(%u, %s)", mrp_state->inst, mrp_state->mrp_idx(), mesa_best_mrm);
    if ((rc = mesa_mrp_best_mrm_set(nullptr, mrp_state->mrp_idx(), &mesa_best_mrm)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_best_mrm_set(%u, %s) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), mesa_best_mrm, error_txt(rc));

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
    }
}

/******************************************************************************/
// MRP_BASE_mesa_tst_copy_conf_set()
/******************************************************************************/
static void MRP_BASE_mesa_tst_copy_conf_set(mrp_state_t *mrp_state)
{
    mesa_rc rc;

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_tst_copy_conf_set(%u, %s)", mrp_state->inst, mrp_state->mrp_idx(), mrp_state->vars.mesa_copy_conf);
    if ((rc = mesa_mrp_tst_copy_conf_set(nullptr, mrp_state->mrp_idx(), &mrp_state->vars.mesa_copy_conf)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_tst_copy_conf_set(%u, %s) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), mrp_state->vars.mesa_copy_conf, error_txt(rc));

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
    }
}

/******************************************************************************/
// MRP_BASE_mesa_status_get()
/******************************************************************************/
static void MRP_BASE_mesa_status_get(mrp_state_t *mrp_state, mesa_mrp_status_t &status)
{
    mesa_rc rc;

    if ((rc = mesa_mrp_status_get(nullptr, mrp_state->mrp_idx(), &status)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mrp_status_get(%u) failed: %s", mrp_state->inst, mrp_state->mrp_idx(), error_txt(rc));

        vtss_clear(status);

        // Convert to something sensible
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        return;
    }

    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mrp_status_get(%u) => %s", mrp_state->inst, mrp_state->mrp_idx(), status);
}

/******************************************************************************/
// MRP_BASE_test_copy_conf_set()
/******************************************************************************/
static void MRP_BASE_test_copy_conf_set(mrp_state_t *mrp_state, bool enable)
{
    if (mrp_state->vars.mesa_copy_conf.tst_to_cpu == enable) {
        return;
    }

    mrp_state->vars.mesa_copy_conf.tst_to_cpu = enable;
    MRP_BASE_mesa_tst_copy_conf_set(mrp_state);
}

/******************************************************************************/
// MRP_BASE_in_test_copy_conf_set()
/******************************************************************************/
static void MRP_BASE_in_test_copy_conf_set(mrp_state_t *mrp_state, bool enable)
{
    if (mrp_state->vars.mesa_copy_conf.itst_to_cpu == enable) {
        return;
    }

    mrp_state->vars.mesa_copy_conf.itst_to_cpu = enable;
    MRP_BASE_mesa_tst_copy_conf_set(mrp_state);
}

/******************************************************************************/
// MRP_BASE_port_state_set()
// Corresponds to the standard's SetPortStateReq()
/******************************************************************************/
static void MRP_BASE_port_state_set(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, bool new_forward, bool force_mesa_update = false, bool use_old_conf = false, bool ace_update = true)
{
    mrp_port_state_t *port_state;
    mesa_bool_t      &cur_forward = mrp_state->status.port_status[port_type].forwarding;
    bool             do_change;
    mesa_rc          rc;

    do_change = new_forward != cur_forward;
    if (!do_change && !force_mesa_update) {
        // No changes to the forwarding state, so just return, unless we are
        // asked to invoke mesa_mrp_port_state_set() anyway.
        return;
    }

    port_state = use_old_conf ? mrp_state->old_port_states[port_type] : mrp_state->port_states[port_type];
    if (!port_state) {
        return;
    }

    if (do_change) {
        // Whether we have MRP H/W support or not, we must invoke the following
        // function, which enables or disables forwarding of any type of frame
        // received on the port in question, and no frames will be forwarded to that
        // port. The MRP H/W support can only block or forward MRP PDUs.
        T_IG(MRP_TRACE_GRP_API, "%u: mesa_port_forward_state_set(port_no = %u, forward = %d)", mrp_state->inst, port_state->port_no, new_forward);
        if ((rc = mesa_port_forward_state_set(nullptr, port_state->port_no, new_forward ? MESA_PORT_FORWARD_ENABLED : MESA_PORT_FORWARD_DISABLED)) != MESA_RC_OK) {
            T_EG(MRP_TRACE_GRP_API, "%u: mesa_port_forward_state_set(port_no = %u, forward = %d) failed: %s", mrp_state->inst, port_state->port_no, new_forward, error_txt(rc));
            mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        }

        cur_forward = new_forward;
    }

    if (ace_update) {
        if (MRP_hw_support) {
            // If we get here, we must always invoke mesa_mrp_port_state_set()
            // whether or not do_change is true.
            MRP_BASE_mesa_port_state_set(mrp_state, port_type, new_forward);
        } else if (do_change) {
            (void)MRP_BASE_ace_update_all(mrp_state);
        }
    }
}

/******************************************************************************/
// MRP_BASE_history_update()
/******************************************************************************/
static void MRP_BASE_history_update(mrp_state_t *mrp_state, const char *who)
{
    if (mrp_state->history.size()                   == 0                                             ||
        mrp_state->status.oper_role                 != mrp_state->last_history_element.oper_role     ||
        mrp_state->vars.sm_state                    != mrp_state->last_history_element.sm_state      ||
        mrp_state->status.ring_state                != mrp_state->last_history_element.ring_state    ||
        mrp_state->vars.prm_ring_port               != mrp_state->last_history_element.prm_ring_port ||
        mrp_state->status.port_status[0].forwarding != mrp_state->last_history_element.forwarding[0] ||
        mrp_state->status.port_status[1].forwarding != mrp_state->last_history_element.forwarding[1] ||
        mrp_state->status.port_status[2].forwarding != mrp_state->last_history_element.forwarding[2] ||
        mrp_state->status.port_status[0].sf         != mrp_state->last_history_element.sf[0]         ||
        mrp_state->status.port_status[1].sf         != mrp_state->last_history_element.sf[1]         ||
        mrp_state->status.port_status[2].sf         != mrp_state->last_history_element.sf[2]         ||
        mrp_state->vars.in_sm_state                 != mrp_state->last_history_element.in_sm_state   ||
        mrp_state->status.in_ring_state             != mrp_state->last_history_element.in_ring_state) {

        T_IG(MRP_TRACE_GRP_HIST, "%u: Update from %s -> %s by %s", mrp_state->inst, MRP_BASE_sm_state_to_str(mrp_state->last_history_element.sm_state), MRP_BASE_sm_state_to_str(mrp_state->vars.sm_state), who);
        mrp_state->last_history_element.oper_role     = mrp_state->status.oper_role;
        mrp_state->last_history_element.sm_state      = mrp_state->vars.sm_state;
        mrp_state->last_history_element.ring_state    = mrp_state->status.ring_state;
        mrp_state->last_history_element.prm_ring_port = mrp_state->vars.prm_ring_port;
        mrp_state->last_history_element.forwarding[0] = mrp_state->status.port_status[0].forwarding;
        mrp_state->last_history_element.forwarding[1] = mrp_state->status.port_status[1].forwarding;
        mrp_state->last_history_element.forwarding[2] = mrp_state->status.port_status[2].forwarding;
        mrp_state->last_history_element.sf[0]         = mrp_state->status.port_status[0].sf;
        mrp_state->last_history_element.sf[1]         = mrp_state->status.port_status[1].sf;
        mrp_state->last_history_element.sf[2]         = mrp_state->status.port_status[2].sf;
        mrp_state->last_history_element.in_sm_state   = mrp_state->vars.in_sm_state;
        mrp_state->last_history_element.in_ring_state = mrp_state->status.in_ring_state;
        mrp_state->last_history_element.who           = who;
        mrp_state->last_history_element.event_time_ms = vtss::uptime_milliseconds();

        mrp_state->history.push(mrp_state->last_history_element);
    }
}

/******************************************************************************/
// MRP_BASE_notif_status_update()
/******************************************************************************/
static mesa_rc MRP_BASE_notif_status_update(mrp_state_t *mrp_state)
{
    mesa_rc rc;

    T_IG(MRP_TRACE_GRP_BASE, "%u: Updating notification status", mrp_state->inst);

    if (mrp_state->notif_status.multiple_mrms) {
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_MULTIPLE_MRMS;
    } else {
        mrp_state->status.oper_warnings &= ~VTSS_APPL_IEC_MRP_OPER_WARNING_MULTIPLE_MRMS;
    }

    if (mrp_state->notif_status.multiple_mims) {
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_MULTIPLE_MIMS;
    } else {
        mrp_state->status.oper_warnings &= ~VTSS_APPL_IEC_MRP_OPER_WARNING_MULTIPLE_MIMS;
    }

    if ((rc = mrp_notification_status.set(mrp_state->inst, &mrp_state->notif_status)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_BASE, "%u: Unable to update notification status: %s", mrp_state->inst, error_txt(rc));
    }

    return rc;
}

/******************************************************************************/
// MRP_BASE_test_tx_check()
/******************************************************************************/
static void MRP_BASE_test_tx_check(mrp_state_t *mrp_state)
{
    bool     test_tx, test_monitor_rx;
    bool     at_least_one_link_up = mrp_state->vars.sm_state != MRP_SM_STATE_POWER_ON && mrp_state->vars.sm_state != MRP_SM_STATE_AC_STAT1;
    uint32_t usec = mrp_state->vars.add_test ? mrp_state->mrm_timing.test_interval_short_usec : mrp_state->mrm_timing.test_interval_default_usec;

    // We must send MRP_Test PDUs whenever we are an MRM and at least one ring
    // port has link up (PRM_UP, CHK_RO, CHK_RC).
    test_tx = mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRM && at_least_one_link_up;

    // We must monitor reception of MRP_Test PDUs whenever we are an MRA or MRM
    // and at least one link is up.
    test_monitor_rx = mrp_state->conf.role != VTSS_APPL_IEC_MRP_ROLE_MRC && at_least_one_link_up;

    if (MRP_hw_support) {
        if (test_monitor_rx) {
            // MESA_MRP_EVENT_MASK_TST_LOC is already enabled in MESA.
            // The following call will not only change the test interval if that
            // has changed. It will also clear the H/W miss count, so that we
            // get another LoC interrupt if we (still) don't get any MRP_Test
            // frames that clear the miss count, so we call it every time we
            // would re-start the Rx timer in a non-H/W based implementation.
            // The LoC interrupt causes a call to mrp_base_loc_set(), which
            // in turn may call MRP_BASE_test_timer_expired().
            // We cannot use mrp_timer_active(), because we never use the Rx
            // timer directly when having H/W support for MRP.
            if (!mrp_state->vars.test_rx_timer_active || usec != mrp_state->mesa_loc_conf.tst_interval) {
                T_IG(MRP_TRACE_GRP_BASE, "%u: Setting test_rx_timer_active = true", mrp_state->inst);
                (void)MRP_BASE_mesa_loc_conf_set(mrp_state, usec, mrp_state->mesa_loc_conf.itst_interval);
                mrp_state->vars.test_rx_timer_active = true;
            }
        } else {
            // Leave MESA_MRP_EVENT_MASK_TST_LOC enabled. If we get an interrupt
            // while we are not supposed to monitor Rx of MRP_Test PDUs, it
            // doesn't harm.
            T_IG(MRP_TRACE_GRP_BASE, "%u: Setting test_rx_timer_active = false", mrp_state->inst);
            mrp_state->vars.test_rx_timer_active = false;
        }
    } else {
        if (test_monitor_rx) {
            if (!mrp_timer_active(mrp_state->test_rx_timer) || usec != mrp_state->test_rx_timer.period_us) {
                // Start or re-start the timer.
                T_IG(MRP_TRACE_GRP_BASE, "%u: Starting test_rx_timer(%u usec)", mrp_state->inst, usec);
                mrp_timer_start(mrp_state->test_rx_timer, usec, true);
            } else {
                // Let the current timer run as is.
            }
        } else {
            T_IG(MRP_TRACE_GRP_BASE, "%u: Stopping test_rx_timer", mrp_state->inst);
            mrp_timer_stop(mrp_state->test_rx_timer);
        }
    }

    mrp_pdu_test_tx_update(mrp_state, test_tx, usec);
}

/******************************************************************************/
// MRP_BASE_in_test_tx_check()
/******************************************************************************/
static void MRP_BASE_in_test_tx_check(mrp_state_t *mrp_state)
{
    bool     in_test_rx_tx;
    bool     in_link_is_up = mrp_state->vars.in_sm_state == MRP_IN_SM_STATE_CHK_IO || mrp_state->vars.in_sm_state == MRP_IN_SM_STATE_CHK_IC;
    uint32_t usec = mrp_state->mim_timing.test_interval_default_usec;

    // We must send and monitor MRP_InTest PDUs whenever we are a MIM in RC mode
    // and the interconnection port is up (CHK_IO or CHK_IC).
    in_test_rx_tx = mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM && mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_RC && in_link_is_up;

    if (MRP_hw_support) {
        if (in_test_rx_tx) {
            // The call to MRP_BASE_mesa_loc_conf_set() not only changes the
            // in_test interval, but also clears H/W's MRP_InTest miss count, so
            // that we get another LoC interrupt if we (still) don't get any
            // MRP_InTest PDUs that clears the miss count, so we call that
            // function whenever we would re-start the Rx timer in a non-H/W-
            // based implementation.
            // The LoC interrupt causes a call to mrp_base_loc_set(), which in
            // turn may call MRP_BASE_in_test_timer_expired().
            // We cannot use mrp_timer_active(), because we never use the Rx
            // timer directly when having H/W support for MRP.
            if (!mrp_state->vars.in_test_rx_timer_active || usec != mrp_state->mesa_loc_conf.itst_interval) {
                T_IG(MRP_TRACE_GRP_BASE, "%u: Setting in_test_rx_timer_active = true", mrp_state->inst);
                (void)MRP_BASE_mesa_loc_conf_set(mrp_state, mrp_state->mesa_loc_conf.tst_interval, usec);
                mrp_state->vars.in_test_rx_timer_active = true;
            }
        } else {
            // Leave MESA_MRP_EVENT_MASK_ITST_LOC enabled. If we get an
            // interrupt while we are not supposed to monitor Rx of MRP_Test
            // PDUs, it doesn't harm.
            T_IG(MRP_TRACE_GRP_BASE, "%u: Setting in_test_rx_timer_active = false", mrp_state->inst);
            mrp_state->vars.in_test_rx_timer_active = false;
        }
    } else {
        // Control Rx timer
        if (in_test_rx_tx) {
            if (!mrp_timer_active(mrp_state->in_test_rx_timer) || usec != mrp_state->in_test_rx_timer.period_us) {
                // Start or re-start the timer.
                T_IG(MRP_TRACE_GRP_BASE, "%u: Starting in_test_rx_timer(%u usec)", mrp_state->inst, usec);
                mrp_timer_start(mrp_state->in_test_rx_timer, usec, true);
            } else {
                // Let the current timer run as is.
            }
        } else {
            T_IG(MRP_TRACE_GRP_BASE, "%u: Stopping in_test_rx_timer", mrp_state->inst);
            mrp_timer_stop(mrp_state->in_test_rx_timer);
        }
    }

    // Start, stop, or update transmission of MRP_InTest PDUs
    mrp_pdu_in_test_tx_update(mrp_state, in_test_rx_tx, usec);
}

/******************************************************************************/
// MRP_BASE_sm_state_update()
/******************************************************************************/
static void MRP_BASE_sm_state_update(mrp_state_t *mrp_state, mrp_sm_state_t new_sm_state, const char *who)
{
    vtss_appl_iec_mrp_ring_state_t new_ring_state;

    if (mrp_state->vars.sm_state != new_sm_state) {
        T_IG(MRP_TRACE_GRP_BASE, "%u: State update by %s: %s->%s", mrp_state->inst, who, MRP_BASE_sm_state_to_str(mrp_state->vars.sm_state), MRP_BASE_sm_state_to_str(new_sm_state));
        if (mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRM) {
            new_ring_state = new_sm_state == MRP_SM_STATE_CHK_RC ? VTSS_APPL_IEC_MRP_RING_STATE_CLOSED : VTSS_APPL_IEC_MRP_RING_STATE_OPEN;
            if (new_ring_state != mrp_state->status.ring_state) {
                mrp_state->status.transitions++;
                mrp_state->status.ring_state = new_ring_state;
                mrp_state->notif_status.ring_open = new_ring_state == VTSS_APPL_IEC_MRP_RING_STATE_OPEN;
                MRP_BASE_notif_status_update(mrp_state);
            }
        } else {
            // We're an MRC, so ring state must be set to undefined.
            mrp_state->status.ring_state = VTSS_APPL_IEC_MRP_RING_STATE_UNDEFINED;
        }

        mrp_state->vars.sm_state = new_sm_state;
    }

    // Figure out whether to start, keep on, or stop transmission of MRP_Test
    // PDUs while also checking whether the contents has changed.
    MRP_BASE_test_tx_check(mrp_state);

    MRP_BASE_history_update(mrp_state, who);
}

/******************************************************************************/
// MRP_BASE_in_sm_state_update()
/******************************************************************************/
static void MRP_BASE_in_sm_state_update(mrp_state_t *mrp_state, mrp_in_sm_state_t new_in_sm_state, const char *who, bool ace_update = true)
{
    vtss_appl_iec_mrp_ring_state_t new_in_ring_state;

    if (mrp_state->vars.in_sm_state != new_in_sm_state) {
        T_IG(MRP_TRACE_GRP_BASE, "InState update by %s: %s->%s", who, MRP_BASE_in_sm_state_to_str(mrp_state->vars.in_sm_state), MRP_BASE_in_sm_state_to_str(new_in_sm_state));
        if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM) {
            new_in_ring_state = new_in_sm_state == MRP_IN_SM_STATE_CHK_IC ? VTSS_APPL_IEC_MRP_RING_STATE_CLOSED : VTSS_APPL_IEC_MRP_RING_STATE_OPEN;
            if (new_in_ring_state != mrp_state->status.in_ring_state) {
                mrp_state->status.in_transitions++;
                mrp_state->status.in_ring_state = new_in_ring_state;
                mrp_state->notif_status.in_open = new_in_ring_state == VTSS_APPL_IEC_MRP_RING_STATE_OPEN;
                MRP_BASE_notif_status_update(mrp_state);
            }
        } else {
            // We're a MIC, so ring state must be set to undefined.
            mrp_state->status.in_ring_state = VTSS_APPL_IEC_MRP_RING_STATE_UNDEFINED;
        }

        mrp_state->vars.in_sm_state = new_in_sm_state;
    }

    // Figure out whether to start, keep on, or stopping transmission of
    // MRP_InTest PDUs while also checking whether the contents has changed.
    MRP_BASE_in_test_tx_check(mrp_state);

    MRP_BASE_history_update(mrp_state, who);
}

/******************************************************************************/
// MRP_BASE_topology_change_req()
// Corresponds to TopologyChangeReq() on p 102.
/******************************************************************************/
static void MRP_BASE_topology_change_req(mrp_state_t *mrp_state, uint16_t time_usec)
{
    // BUG: MRP_Interval is in milliseconds, but MRP_TOPchgT can be fractions of
    // a millisecond, so MRP_Interval may be 0 when MRP_TopologyChange PDU is
    // transmitted.
    mrp_pdu_topology_change_tx(mrp_state, (mrp_state->mrm_timing.topology_change_repeat_cnt * time_usec) / 1000);

    if (time_usec == 0) {
        MRP_BASE_fdb_flush(mrp_state);
    } else {
        mrp_timer_start(mrp_state->top_timer, time_usec, false);
    }
}

/******************************************************************************/
// MRP_BASE_test_mgr_nack_req()
// Corresponds to TestMgrNAckReq() on p 103.
/******************************************************************************/
static void MRP_BASE_test_mgr_nack_req(mrp_state_t *mrp_state, mrp_rx_pdu_info_t &rx_pdu_info)
{
    mrp_pdu_option_tx(mrp_state, MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK, 0, &rx_pdu_info.sa);
}

/******************************************************************************/
// MRP_BASE_test_propagate_req()
// Corresponds to TestPropagateReq() on p 104.
/******************************************************************************/
static void MRP_BASE_test_propagate_req(mrp_state_t *mrp_state)
{
    mrp_pdu_option_tx(mrp_state, MRP_PDU_SUB_TLV_TYPE_TEST_PROPAGATE, mrp_state->vars.HO_BestMRM_Prio, &mrp_state->vars.HO_BestMRM_SA);
}

/******************************************************************************/
// MRP_BASE_in_topology_change_req()
// Corresponds to InterconnTopologyChangeReq() on p 124.
/******************************************************************************/
static void MRP_BASE_in_topology_change_req(mrp_state_t *mrp_state, uint16_t time_usec)
{
    mrp_pdu_in_topology_change_tx(mrp_state, (mrp_state->mim_timing.topology_change_repeat_cnt * time_usec) / 1000);

    if (time_usec == 0) {
        MRP_BASE_fdb_flush(mrp_state);
    } else {
        mrp_timer_start(mrp_state->in_top_timer, time_usec, false);
    }
}

/******************************************************************************/
// MRP_BASE_in_link_status_poll_req()
// Corresponds to InterconnLinkStatusPollReq() on p 126.
/******************************************************************************/
static void MRP_BASE_in_link_status_poll_req(mrp_state_t *mrp_state)
{
    mrp_pdu_in_link_status_poll_tx(mrp_state);
    mrp_timer_start(mrp_state->in_link_status_timer, mrp_state->mim_timing.link_status_poll_interval_usec, false);
}

/******************************************************************************/
// MRP_BASE_mrc_power_on()
// Corresponds to MRC PowerOn, Table 43, p. 76
/******************************************************************************/
static void MRP_BASE_mrc_power_on(mrp_state_t *mrp_state)
{
    mrp_state->vars.prm_ring_port = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1;
    mrp_state->vars.sec_ring_port = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2;
    mrp_state->status.oper_role   = VTSS_APPL_IEC_MRP_ROLE_MRC;
    MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1, false);
    MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2, false);
    MRP_BASE_sm_state_update(mrp_state, MRP_SM_STATE_AC_STAT1, "MRC PowerOn");
}

/******************************************************************************/
// MRP_BASE_mrm_power_on()
// Corresponds to MRM PowerOn, Table 41, p. 66
/******************************************************************************/
static void MRP_BASE_mrm_power_on(mrp_state_t *mrp_state)
{
    mrp_state->vars.prm_ring_port   = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1;
    mrp_state->vars.sec_ring_port   = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2;
    mrp_state->vars.MRP_MRM_NRmax   = mrp_state->mrm_timing.test_monitoring_cnt - 1;
    mrp_state->vars.MRP_MRM_NReturn = 0;
    mrp_state->vars.add_test        = false;
    mrp_state->status.oper_role     = VTSS_APPL_IEC_MRP_ROLE_MRM;
    mrp_state->vars.TC_NReturn      = mrp_state->mrm_timing.topology_change_repeat_cnt - 1;
    MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1, false);
    MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2, false);
    MRP_BASE_sm_state_update(mrp_state, MRP_SM_STATE_AC_STAT1, "MRM PowerOn");

    if (MRP_hw_support) {
        // We want to start detecting multiple MRMs on the I/C ring if we are an
        // MRM. We do that by asking for MRP_Test PDUs from other MIMs to come
        // to the CPU.
        MRP_BASE_test_copy_conf_set(mrp_state, true);
    }
}

/******************************************************************************/
// MRP_BASE_mra_power_on()
// Corresponds to MRA PowerOn, Table 45, p. 83
/******************************************************************************/
static void MRP_BASE_mra_power_on(mrp_state_t *mrp_state)
{
    mrp_state->vars.prm_ring_port         = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1;
    mrp_state->vars.sec_ring_port         = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2;
    mrp_state->vars.MRP_MRM_NRmax         = mrp_state->mrm_timing.test_monitoring_cnt - 1;
    mrp_state->vars.MRP_MRM_NReturn       = 0;
    mrp_state->vars.add_test              = false;
    mrp_state->vars.MRP_BestMRM_Prio      = 0xFFFF;
    mrp_state->vars.HO_BestMRM_Prio       = 0xFFFF;
    mrp_state->vars.MRP_MonNReturn        = 0;
    mrp_state->status.oper_role           = VTSS_APPL_IEC_MRP_ROLE_MRM;
    mrp_state->status.mrm_mrc_transitions = 0;
    mrp_state->vars.TC_NReturn            = mrp_state->mrm_timing.topology_change_repeat_cnt - 1;
    memset(mrp_state->vars.MRP_BestMRM_SA.addr, 0xFF, sizeof(mrp_state->vars.MRP_BestMRM_SA.addr));
    memset(mrp_state->vars.HO_BestMRM_SA.addr,  0xFF, sizeof(mrp_state->vars.HO_BestMRM_SA));
    MRP_BASE_mesa_best_mrm_set(mrp_state);
    MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1, false);
    MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2, false);
    MRP_BASE_sm_state_update(mrp_state, MRP_SM_STATE_AC_STAT1, "MRA PowerOn");

    if (MRP_hw_support) {
        // We want to start detecting multiple MRMs on the I/C ring if we are an
        // MRM. We do that by asking for MRP_Test PDUs from other MIMs to come
        // to the CPU.
        MRP_BASE_test_copy_conf_set(mrp_state, true);
    }
}

/******************************************************************************/
// MRP_BASE_mim_power_on()
// Corresponds to MIM-LC PowerOn, Table 50, p. 109 and
// MIM-RC PowerOn, Table 51, p. 112.
/******************************************************************************/
static void MRP_BASE_mim_power_on(mrp_state_t *mrp_state)
{
    mrp_state->vars.IN_TC_NReturn          = mrp_state->mim_timing.topology_change_repeat_cnt  - 1;
    mrp_state->vars.MRP_IN_LNKSTAT_NReturn = mrp_state->mim_timing.link_status_poll_repeat_cnt - 1;
    MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION, false);
    MRP_BASE_in_sm_state_update(mrp_state, MRP_IN_SM_STATE_AC_STAT1, mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_LC ? "MIM-LC PowerOn" : "MIM-RC PowerOn", false);

    if (MRP_hw_support && mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_RC) {
        // We want to start detecting multiple MIMs on the I/C ring if we are an
        // MIM-RC (we don't care about them if we are an MIM-LC). We do that by
        // asking for MRP_InTest PDUs from other MIMs to come to the CPU.
        MRP_BASE_in_test_copy_conf_set(mrp_state, true);
    }
}

/******************************************************************************/
// MRP_BASE_mic_power_on()
// Corresponds to MIC-LC PowerOn, Table 53, p. 118 and
// MIC-RC PowerOn, Table 54, p. 121.
/******************************************************************************/
static void MRP_BASE_mic_power_on(mrp_state_t *mrp_state)
{
    MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION, false);
    MRP_BASE_in_sm_state_update(mrp_state, MRP_IN_SM_STATE_AC_STAT1, mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_LC ? "MIC-LC PowerOn" : "MIC-RC PowerOn", false);
}

/******************************************************************************/
// MRP_BASE_mrc_init()
// Corresponds to MRC_Init(), on p. 104
/******************************************************************************/
static void MRP_BASE_mrc_init(mrp_state_t *mrp_state)
{
    T_IG(MRP_TRACE_GRP_BASE, "%u", mrp_state->inst);
    mrp_state->vars.MRP_MonNReturn   = 0;
    mrp_state->vars.MRP_LNKNReturn   = 0;
    mrp_state->vars.MRP_BestMRM_SA   = mrp_state->vars.HO_BestMRM_SA;
    mrp_state->vars.MRP_BestMRM_Prio = mrp_state->vars.HO_BestMRM_Prio;
    mrp_state->status.oper_role      = VTSS_APPL_IEC_MRP_ROLE_MRC;
    mrp_state->status.mrm_mrc_transitions++;

    // When going from MRM to MRC, we also clear the round-trip time of MRP_Test
    // PDUs.
    vtss_clear(mrp_state->status.round_trip_time);

    // The number of transitions can no longer be counted, because we have no
    // clue whether the ring is open or closed anymore.
    mrp_state->status.transitions = 0;

    // Also, we can no longer raise ring_open or multiple_mrms events.
    mrp_state->notif_status.ring_open     = false;
    mrp_state->notif_status.multiple_mrms = false;
    mrp_timer_stop(mrp_state->multiple_mrms_timer);
    mrp_timer_stop(mrp_state->multiple_mrms_timer2);
    MRP_BASE_notif_status_update(mrp_state);

    if (MRP_hw_support) {
        // Tell H/W that we are now an MRC.
        MRP_BASE_mesa_ring_role_set(mrp_state);
    } else {
        // Whenever the operational role changes, we need to update ACEs
        (void)MRP_BASE_ace_update_all(mrp_state);
    }
}

/******************************************************************************/
// MRP_BASE_mrm_init()
// Corresponds to MRM_Init(), on p. 104
/******************************************************************************/
static void MRP_BASE_mrm_init(mrp_state_t *mrp_state)
{
    T_IG(MRP_TRACE_GRP_BASE, "%u", mrp_state->inst);
    mrp_state->vars.add_test         = false;
    mrp_state->vars.NO_TC            = false;
    mrp_state->vars.MRP_MRM_NReturn  = 0;
    mrp_state->vars.MRP_BestMRM_SA   = MRP_chassis_mac;
    mrp_state->vars.MRP_BestMRM_Prio = mrp_state->conf.mrm.prio;

    // BUG: If not resetting HO_BestMRM_Prio and HO_BestMRM_SA when we go from
    // MRC to MRM, and the currently stored HO_BestMRM_SA is demoted to an MRC
    // or taken out of the ring, we will forever toggle between PT_IDLE and
    // CHK_RO.
    mrp_state->vars.HO_BestMRM_Prio   = 0xFFFF;
    memset(mrp_state->vars.HO_BestMRM_SA.addr,  0xFF, sizeof(mrp_state->vars.HO_BestMRM_SA));
    MRP_BASE_mesa_best_mrm_set(mrp_state);
    mrp_state->status.oper_role = VTSS_APPL_IEC_MRP_ROLE_MRM;
    mrp_state->status.mrm_mrc_transitions++;

    if (MRP_hw_support) {
        // Tell H/W that we are now an MRM.
        MRP_BASE_mesa_ring_role_set(mrp_state);
    } else {
        // Whenever the operational role changes, we need to update ACEs
        (void)MRP_BASE_ace_update_all(mrp_state);
    }

    if (MRP_hw_support) {
        // We want to start detecting multiple MRMs on the I/C ring if we are an
        // MRM. We do that by asking for MRP_Test PDUs from other MIMs to come
        // to the CPU.
        MRP_BASE_test_copy_conf_set(mrp_state, true);
    }
}

/******************************************************************************/
// MRP_BASE_link_change()
// Corresponds to MAUTypeChangeInd()/MAUType_ChangeInd on p. 103
/******************************************************************************/
static void MRP_BASE_link_change(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, bool link_up)
{
    mrp_sm_state_t next_sm_state       = mrp_state->vars.sm_state;
    bool           topology_change_req = false;
    bool           link_change_tx      = false;
    bool           prm_change          = false, prm_fwd = false;
    bool           sec_change          = false, sec_fwd = false;
    bool           swap                = false;
    bool           force_mesa_update   = false;
    const char     *who;

    T_DG(MRP_TRACE_GRP_BASE, "%u. cur-state = %s. port_type = %s, link_up = %d", mrp_state->inst, MRP_BASE_sm_state_to_str(mrp_state->vars.sm_state), iec_mrp_util_port_type_to_str(port_type), link_up);

    switch (mrp_state->vars.sm_state) {
    case MRP_SM_STATE_AC_STAT1:
        if (link_up) {
            if (port_type != mrp_state->vars.prm_ring_port) {
                // Table 41, row #4
                // Table 43, row #4
                // Table 45, row #4
                mrp_state->vars.sec_ring_port = mrp_state->vars.prm_ring_port;
                mrp_state->vars.prm_ring_port = port_type;
                swap                          = true;
            }

            // Table 41, row #2 and row #4
            // Table 43, row #2 and row #4
            // Table 45, row #2 and row #4
            prm_change    = true;
            prm_fwd       = true;
            next_sm_state = mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRM ? MRP_SM_STATE_PRM_UP : MRP_SM_STATE_DE_IDLE;
        } else {
            // Table 41, row #3 and row #5
            // Table 43, row #3
            // Table 45, row #3 and row #5
            // Ignore
        }

        break;

    case MRP_SM_STATE_PRM_UP:
        if (port_type == mrp_state->vars.prm_ring_port) {
            if (link_up) {
                // Table 41, row #9
                // Table 45, row #9
                // Ignore
            } else {
                // Table 41, row #10
                // Table 45, row #10
                prm_change    = true;
                prm_fwd       = false;
                next_sm_state = MRP_SM_STATE_AC_STAT1;
            }
        } else {
            if (link_up) {
                // Table 41, row #12
                // Table 45, row #12
                mrp_state->vars.MRP_MRM_NRmax   = mrp_state->mrm_timing.test_monitoring_cnt - 1;
                mrp_state->vars.MRP_MRM_NReturn = 0;
                mrp_state->vars.NO_TC           = true;
                next_sm_state                   = MRP_SM_STATE_CHK_RC;
            } else {
                // Table 41, row #11
                // Table 45, row #11
                // Ignore
            }
        }

        break;

    case MRP_SM_STATE_CHK_RO:
        if (link_up) {
            // Table 41, row #22 and row #24
            // Table 45, row #26 and row #28
            // Ignore
        } else {
            if (port_type == mrp_state->vars.prm_ring_port) {
                // Table 41, row #23
                // Table 45, row #27
                mrp_state->vars.prm_ring_port = mrp_state->vars.sec_ring_port;
                mrp_state->vars.sec_ring_port = port_type;
                topology_change_req           = true;
                swap                          = true;
            }

            // Table 41, row #23 and row #25
            // Table 45, row #27 and row #29
            sec_change    = true;
            sec_fwd       = false;
            next_sm_state = MRP_SM_STATE_PRM_UP;
        }

        break;

    case MRP_SM_STATE_CHK_RC:
        if (port_type == mrp_state->vars.prm_ring_port) {
            if (link_up) {
                // Table 41, row #39
                // Table 45, row #45
                // Ignore
            } else {
                // Table 41, row #40
                // Table 45, row #46
                mrp_state->vars.prm_ring_port = mrp_state->vars.sec_ring_port;
                mrp_state->vars.sec_ring_port = port_type;
                prm_change                    = true;
                prm_fwd                       = true;
                sec_change                    = true;
                sec_fwd                       = false;
                topology_change_req           = true;
                swap                          = true;
                next_sm_state                 = MRP_SM_STATE_PRM_UP;
            }
        } else {
            if (link_up) {
                // Table 41, row #41
                // Table 45, row #47
                // Ignore
            } else {
                // Table 41, row #42
                // Table 45, row #48
                next_sm_state = MRP_SM_STATE_PRM_UP;
            }
        }

        break;

    case MRP_SM_STATE_DE_IDLE:
        if (port_type == mrp_state->vars.prm_ring_port) {
            if (link_up) {
                // Table 43, row #9
                // Table 45, row #66
                // Ignore
            } else {
                // Table 43, row #8
                // Table 45, row #65
                prm_change    = true;
                prm_fwd       = false;
                next_sm_state = MRP_SM_STATE_AC_STAT1;
            }
        } else {
            if (link_up) {
                // Table 43, row #6
                // Table 45, row #63
                mrp_timer_start(mrp_state->up_timer, mrp_state->mrc_timing.link_up_interval_usec, false);
                mrp_state->vars.MRP_LNKNReturn = mrp_state->mrc_timing.link_change_repeat_cnt;
                link_change_tx                 = true;
                next_sm_state                  = MRP_SM_STATE_PT;
            } else {
                // Table 43, row #7
                // Table 45, row #64
                // Ignore
            }
        }

        break;

    case MRP_SM_STATE_PT:
        if (link_up) {
            // Table 43, row #13 and row #16
            // Table 45, row #82 and row #85
            // Ignore
        } else {
            if (port_type == mrp_state->vars.prm_ring_port) {
                // Table 43, row #15
                // Table 45, row #84
                mrp_state->vars.prm_ring_port = mrp_state->vars.sec_ring_port;
                mrp_state->vars.sec_ring_port = port_type;
                prm_change                    = true;
                prm_fwd                       = true;
                swap                          = true;
            }

            // Table 43, row #14 and row #15
            // Table 45, row #83 and row #84
            mrp_timer_stop(mrp_state->up_timer);
            mrp_timer_start(mrp_state->down_timer, mrp_state->mrc_timing.link_down_interval_usec, false);
            mrp_state->vars.MRP_LNKNReturn = mrp_state->mrc_timing.link_change_repeat_cnt;
            sec_change                     = true;
            sec_fwd                        = false;
            link_change_tx                 = true;
            next_sm_state                  = MRP_SM_STATE_DE;
        }

        break;

    case MRP_SM_STATE_DE:
        if (port_type == mrp_state->vars.prm_ring_port) {
            if (link_up) {
                // Table 43, row #23
                // Table 45, row #103
                // Ignore
            } else {
                // Table 43, row #22
                // Table 45, row #102
                mrp_timer_stop(mrp_state->down_timer);
                mrp_state->vars.MRP_LNKNReturn = mrp_state->mrc_timing.link_change_repeat_cnt;
                prm_change                     = true;
                prm_fwd                        = false;
                next_sm_state                  = MRP_SM_STATE_AC_STAT1;
            }
        } else {
            if (link_up) {
                // Table 43, row #20
                // Table 45, row #100
                mrp_timer_stop(mrp_state->down_timer);
                mrp_timer_start(mrp_state->up_timer, mrp_state->mrc_timing.link_up_interval_usec, false);
                mrp_state->vars.MRP_LNKNReturn = mrp_state->mrc_timing.link_change_repeat_cnt;
                link_change_tx                 = true;
                next_sm_state                  = MRP_SM_STATE_PT;
            } else {
                // Table 43, row #21
                // Table 45, row #101
                // Ignore
            }
        }

        break;

    case MRP_SM_STATE_PT_IDLE:
        if (link_up) {
            // Table 43, row #25 and row #28
            // Table 45, row #116 and row #119
            // Ignore
        } else {
            if (port_type == mrp_state->vars.prm_ring_port) {
                // Table 43, row #27
                // Table 45, row #118
                mrp_state->vars.prm_ring_port = mrp_state->vars.sec_ring_port;
                mrp_state->vars.sec_ring_port = port_type;
                swap                          = true;
            }

            // Table 43, row #26 and row #27
            // Table 45, row #117 and row #118
            mrp_timer_start(mrp_state->down_timer, mrp_state->mrc_timing.link_down_interval_usec, false);
            mrp_state->vars.MRP_LNKNReturn = mrp_state->mrc_timing.link_change_repeat_cnt;
            sec_change                     = true;
            sec_fwd                        = false;
            link_change_tx                 = true;
            next_sm_state = MRP_SM_STATE_DE;
        }

        break;

    default:
        return;
    }

    if (MRP_hw_support && swap) {
        MRP_BASE_mesa_ring_ports_swap(mrp_state);

        // Once the above function has been called, we MUST enforce calls to
        // mesa_mrp_port_state_set() (sigh). This is done through
        // MRP_BASE_port_state_set().
        if (!sec_change) {
            // We won't get into the "if (sec_change)" below unless we do
            // something. Set the forwarding of that port to the same as now.
            sec_change = true;
            sec_fwd    = mrp_state->status.port_status[mrp_state->vars.sec_ring_port].forwarding;
        }

        if (!prm_change) {
            // We won't get into the "if (prm_change)" below unless we do
            // something. Set the forwarding of that port to the same as now.
            prm_change = true;
            prm_fwd    = mrp_state->status.port_status[mrp_state->vars.prm_ring_port].forwarding;
        }

        force_mesa_update = true;
    }

    // If both primary and secondary change port state, it's always secondary
    // that gets blocked, so we need to set that first to avoid potential loops.
    if (sec_change) {
        MRP_BASE_port_state_set(mrp_state, mrp_state->vars.sec_ring_port, sec_fwd, force_mesa_update);
    }

    if (prm_change) {
        MRP_BASE_port_state_set(mrp_state, mrp_state->vars.prm_ring_port, prm_fwd, force_mesa_update);
    }

    // The last argument to MRP_BASE_sm_state_update() must be a const string.
    if (port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1) {
        who = link_up ? "MAUTypeChangeInd(P1-Up)" : "MAUTypeChangeInd(P1-Down)";
    } else {
        who = link_up ? "MAUTypeChangeInd(P2-Up)" : "MAUTypeChangeInd(P2-Down)";
    }

    // If we have MRP H/W support, we must trigger a new LoC interrupt when we
    // have link changes. Otherwise, we might get stuck in CHK_RC upon the
    // second ring port's link up during boot, because both ring ports already
    // have fired their LoC interrupts. By setting the following variable false,
    // we will have MRP_BASE_test_tx_check() to call
    // MESA_BASE_mesa_loc_conf_set(), which will clear the current miss count
    // and potentially (if no MRP_Test PDUs that clear the miss counter arrive)
    // fire a new LoC interrupt.
    T_IG(MRP_TRACE_GRP_BASE, "%u: Setting test_rx_timer_active = false", mrp_state->inst);
    mrp_state->vars.test_rx_timer_active = false;

    MRP_BASE_sm_state_update(mrp_state, next_sm_state, who);

    if (link_change_tx) {
        mrp_pdu_link_change_tx(mrp_state, mrp_state->vars.prm_ring_port, link_up);
    }

    if (topology_change_req) {
        MRP_BASE_topology_change_req(mrp_state, mrp_state->mrm_timing.topology_change_interval_usec);
    }
}

/******************************************************************************/
// MRP_BASE_in_link_change()
// Corresponds to MAUTypeChangeInd()/MAUType_ChangeInd on p. 103
//
// MIM-LC: Table 50 on p. 109
// MIM-RC: Table 51 on p. 112
// MIC-LC: Table 53 on p. 118
// MIC-RC: Table 54 on p. 121
/******************************************************************************/
static void MRP_BASE_in_link_change(mrp_state_t *mrp_state, bool link_up)
{
    mrp_in_sm_state_t next_in_sm_state  = mrp_state->vars.in_sm_state;
    bool              in_change         = false;
    bool              in_top_tx         = false;
    bool              in_link_poll_tx   = false;
    bool              in_link_change_tx = false;

    switch (mrp_state->vars.in_sm_state) {
    case MRP_IN_SM_STATE_AC_STAT1:
        if (link_up) {
            // Table 50, row #2
            // Table 51, row #3
            // Table 53, row #4
            // Table 54, row #4
            if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM) {
                in_change        = true;
                next_in_sm_state = MRP_IN_SM_STATE_CHK_IC;

                if (mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_LC) {
                    // Table 50, row #2
                    in_link_poll_tx = true;
                } else {
                    // Table 51, row #3
                    mrp_state->vars.MRP_MIM_NRmax   = mrp_state->mim_timing.test_monitoring_cnt - 1;
                    mrp_state->vars.MRP_MIM_NReturn = 0;
                }
            } else {
                // Table 53, row #4
                // Table 54, row #4
                mrp_state->vars.MRP_InLNKNReturn = mrp_state->mic_timing.link_change_repeat_cnt;
                mrp_timer_stop(mrp_state->in_down_timer);
                mrp_timer_start(mrp_state->in_up_timer, mrp_state->mic_timing.link_up_interval_usec, false);
                in_link_change_tx = true;
                next_in_sm_state  = MRP_IN_SM_STATE_PT;
            }
        } else {
            // Table 50, row #3
            // Table 51, row #4
            // Table 53, row #5
            // Table 54, row #5
            // Ignore
        }

        break;

    case MRP_IN_SM_STATE_CHK_IO:
        if (link_up) {
            // Table 50, row #7
            // Table 51, row #12
            // Ignore
        } else {
            // Table 50, row #8
            // Table 51, row #13
            in_change        = true;
            next_in_sm_state = MRP_IN_SM_STATE_AC_STAT1;

            if (mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_LC) {
                // Table 50, row #8
                mrp_timer_stop(mrp_state->in_link_status_timer);
            } else {
                // Table 51, row #13
                // BUG: When entering AC_STAT1, I don't think we should send
                // MRP_InTest PDUs, since they can't return to us.
            }
        }

        break;

    case MRP_IN_SM_STATE_CHK_IC:
        if (link_up) {
            // Table 50, row #13
            // Table 51, row #23
            // Ignore
        } else {
            // Table 50, row #14
            // Table 51, row #24
            in_change        = true;
            next_in_sm_state = MRP_IN_SM_STATE_AC_STAT1;

            if (mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_RC) {
                // Table 51, row #24
                // BUG: When entering AC_STAT1, I don't think we should send
                // MRP_InTest PDUs, since they can't return to us.
                in_top_tx  = true;
            }
        }

        break;

    case MRP_IN_SM_STATE_PT:
    case MRP_IN_SM_STATE_IP_IDLE:
        if (link_up) {
            // Table 53, row #11, row #15
            // Table 54, row #10, row #13
            // Ignore
        } else {
            // Table 53, row #10, row #14
            // Table 54, row #9,  row #12
            mrp_timer_stop(mrp_state->in_up_timer);
            mrp_timer_start(mrp_state->in_down_timer, mrp_state->mic_timing.link_down_interval_usec, false);
            mrp_state->vars.MRP_InLNKNReturn = mrp_state->mic_timing.link_change_repeat_cnt;
            in_change                        = true;
            in_link_change_tx                = true;
            next_in_sm_state                 = MRP_IN_SM_STATE_AC_STAT1;
        }

        break;

    default:
        break;
    }

    if (in_change) {
        MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION, false /* always block in this function */);
    }

    // The last argument to MRP_BASE_in_sm_state_update() must be a const string.
    MRP_BASE_in_sm_state_update(mrp_state, next_in_sm_state, link_up ? "MAUTypeChangeInd(In-Up)" : "MAUTypeChangeInd(In-Down)");

    if (in_top_tx) {
        MRP_BASE_in_topology_change_req(mrp_state, mrp_state->mim_timing.topology_change_interval_usec);
    }

    if (in_link_poll_tx) {
        MRP_BASE_in_link_status_poll_req(mrp_state);
    }

    if (in_link_change_tx) {
        mrp_pdu_in_link_change_tx(mrp_state, link_up, mrp_state->vars.MRP_InLNKNReturn * (mrp_state->mic_timing.link_up_interval_usec / 1000));
    }
}

/******************************************************************************/
// MRP_BASE_test_timer_expired()
// Corresponds to "TestTimer expired" in Table 41 and Table 45.
/******************************************************************************/
static void MRP_BASE_test_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    mrp_sm_state_t next_sm_state       = mrp_state->vars.sm_state;
    bool           sec_change          = false;
    bool           topology_change_req = false;
    bool           mrm_init            = false;

    T_DG(MRP_TRACE_GRP_BASE, "%u: cur-state = %s. MRP_MRM_NReturn = %u, MRP_MonNReturn = %u", mrp_state->inst, MRP_BASE_sm_state_to_str(mrp_state->vars.sm_state), mrp_state->vars.MRP_MRM_NReturn, mrp_state->vars.MRP_MonNReturn);

    if (mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRC) {
        T_EG(MRP_TRACE_GRP_BASE, "%u: Test timer expired even though we are configured as an MRC", mrp_state->inst);
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        mrp_timer_stop(mrp_state->test_rx_timer);
        mrp_state->vars.test_rx_timer_active = false;
        return;
    }

    switch (mrp_state->vars.sm_state) {
    case MRP_SM_STATE_AC_STAT1:
        // Table 41 and Table 45, row #6
        // Ignore
        return;

    case MRP_SM_STATE_PRM_UP:
    case MRP_SM_STATE_CHK_RO:
        // Table 41, row #8 and row #21
        // Table 45, row #8 and row #25
        mrp_state->vars.add_test = false;
        break;

    case MRP_SM_STATE_CHK_RC:
        if (mrp_state->vars.MRP_MRM_NReturn >= mrp_state->vars.MRP_MRM_NRmax) {
            // Table 41, row #36 and row #37
            // Table 45, row #42 and row #43
            mrp_state->vars.MRP_MRM_NRmax   = mrp_state->mrm_timing.test_monitoring_cnt - 1;
            mrp_state->vars.MRP_MRM_NReturn = 0;
            mrp_state->vars.add_test        = false;
            sec_change                      = true;

            if (!mrp_state->vars.NO_TC) {
                // Table 41, row #36
                // Table 45, row #42
                topology_change_req = true;
            } else {
                // BUG: Table 45, row #43 states that a topology change PDU must
                // be sent even when NO_TC is true. This does not add up with
                // the MRM state machine's ditto row (Table 41, row #37).
            }

            next_sm_state = MRP_SM_STATE_CHK_RO;
        } else {
            // Table 41, row #38
            // Table 45, row #44
            mrp_state->vars.MRP_MRM_NReturn++;
            mrp_state->vars.add_test = false;
        }

        break;

    case MRP_SM_STATE_DE_IDLE:
    case MRP_SM_STATE_PT:
    case MRP_SM_STATE_DE:
    case MRP_SM_STATE_PT_IDLE:
        // BUG: Standard specifies MRP_MON_NR_MAX, but this is not defined.
        if (mrp_state->vars.MRP_MonNReturn <= mrp_state->mrm_timing.test_monitoring_cnt) {
            // Table 45, row #61, row #78, row #96, and row #114
            mrp_state->vars.MRP_MonNReturn++;
        } else {
            // Table 45, row #62, row #79, row #97, and row #115
            mrm_init = true;
            next_sm_state = mrp_state->vars.sm_state == MRP_SM_STATE_PT      ? MRP_SM_STATE_CHK_RC :
                            mrp_state->vars.sm_state == MRP_SM_STATE_PT_IDLE ? MRP_SM_STATE_CHK_RO : MRP_SM_STATE_PRM_UP;
        }

        break;

    default:
        return;
    }

    if (mrm_init) {
        MRP_BASE_mrm_init(mrp_state);
    }

    if (sec_change) {
        MRP_BASE_port_state_set(mrp_state, mrp_state->vars.sec_ring_port, true /* always set to forwarding in this function */);
    }

    MRP_BASE_sm_state_update(mrp_state, next_sm_state, "TestTimerExpired");

    if (topology_change_req) {
        MRP_BASE_topology_change_req(mrp_state, mrp_state->mrm_timing.topology_change_interval_usec);
    }
}

/******************************************************************************/
// MRP_BASE_up_timer_expired()
// Corresponds to "UpTimer expired" in Table 43 and Table 45.
/******************************************************************************/
static void MRP_BASE_up_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    if (mrp_state->vars.sm_state != MRP_SM_STATE_PT) {
        return;
    }

    if (mrp_state->vars.MRP_LNKNReturn) {
        // MRC: Table 43, row #12
        // MRA: Table 45, row #81
        mrp_timer_start(mrp_state->up_timer, mrp_state->mrc_timing.link_up_interval_usec, false);
        mrp_state->vars.MRP_LNKNReturn--;
        mrp_pdu_link_change_tx(mrp_state, mrp_state->vars.prm_ring_port, true);
    } else {
        // MRC: Table 43, row #11
        // MRA: Table 45, row #80
        mrp_state->vars.MRP_LNKNReturn = mrp_state->mrc_timing.link_change_repeat_cnt;
        MRP_BASE_port_state_set(mrp_state, mrp_state->vars.sec_ring_port, true);
        MRP_BASE_sm_state_update(mrp_state, MRP_SM_STATE_PT_IDLE, "UpTimerExpired");
    }
}

/******************************************************************************/
// MRP_BASE_down_timer_expired()
// Corresponds to "DownTimer expired" in Table 43 and Table 45.
/******************************************************************************/
static void MRP_BASE_down_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    if (mrp_state->vars.sm_state != MRP_SM_STATE_DE) {
        // Only for MRCs and MRAs acting as MRCs
        return;
    }

    if (mrp_state->vars.MRP_LNKNReturn) {
        // MRC: Table 43, row #19
        // MRA: Table 45, row #99
        mrp_timer_start(mrp_state->down_timer, mrp_state->mrc_timing.link_down_interval_usec, false);
        mrp_state->vars.MRP_LNKNReturn--;
        mrp_pdu_link_change_tx(mrp_state, mrp_state->vars.prm_ring_port, false);
    } else {
        // MRC: Table 43, row #18
        // MRA: Table 45, row #98
        mrp_state->vars.MRP_LNKNReturn = mrp_state->mrc_timing.link_change_repeat_cnt;
        MRP_BASE_sm_state_update(mrp_state, MRP_SM_STATE_DE_IDLE, "DownTimerExpired");
    }
}

/******************************************************************************/
// MRP_BASE_top_timer_expired()
// Corresponds to Table 48.
/******************************************************************************/
static void MRP_BASE_top_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    // BUG: According to Table 48, it only acts if the state machine is in IDLE,
    // which is a non-existing state, so I have chosen to let it act in any
    // state.
    if (mrp_state->vars.TC_NReturn) {
        mrp_pdu_topology_change_tx(mrp_state, (mrp_state->vars.TC_NReturn * mrp_state->mrm_timing.topology_change_interval_usec) / 1000);
        mrp_state->vars.TC_NReturn--;
        mrp_timer_start(mrp_state->top_timer, mrp_state->mrm_timing.topology_change_interval_usec, false);
    } else {
        mrp_state->vars.TC_NReturn = mrp_state->mrm_timing.topology_change_repeat_cnt - 1;
        MRP_BASE_fdb_clear(mrp_state, 0);
        mrp_pdu_topology_change_tx(mrp_state, 0);
        mrp_timer_stop(mrp_state->top_timer);
    }
}

//*****************************************************************************/
// MRP_BASE_fdb_clear_timer_expired()
// Clause 8.2.5
/******************************************************************************/
static void MRP_BASE_fdb_clear_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    MRP_BASE_fdb_flush(mrp_state);
}

/******************************************************************************/
// MRP_BASE_in_test_timer_expired()
// Corresponds to "InterconnTestTimer expired" in Table 51 on p. 112.
/******************************************************************************/
static void MRP_BASE_in_test_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM && mrp_state->conf.in_mode != VTSS_APPL_IEC_MRP_IN_MODE_RC) {
        T_EG(MRP_TRACE_GRP_BASE, "%u: InTest timer expired even though we are not configured as a MIM in RC mode", mrp_state->inst);
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        mrp_timer_stop(mrp_state->in_test_rx_timer);
        mrp_state->vars.in_test_rx_timer_active = false;
        return;
    }

    switch (mrp_state->vars.in_sm_state) {
    case MRP_IN_SM_STATE_AC_STAT1:
        // Table 51, row #2
        // Ignore
        // in_test_rx_timer already stopped.
        return;

    case MRP_IN_SM_STATE_CHK_IO:
        // Table 51, row #11
        break;

    case MRP_IN_SM_STATE_CHK_IC:
        if (mrp_state->vars.MRP_MIM_NReturn >= mrp_state->mim_timing.test_monitoring_cnt) {
            // Table 51, row #21
            MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION, true);
            mrp_state->vars.MRP_MIM_NRmax   = mrp_state->mim_timing.test_monitoring_cnt - 1;
            mrp_state->vars.MRP_MIM_NReturn = 0;
            MRP_BASE_in_sm_state_update(mrp_state, MRP_IN_SM_STATE_CHK_IO, "InterconnTestTimerExpired");
            MRP_BASE_in_topology_change_req(mrp_state, mrp_state->mim_timing.topology_change_interval_usec);
        } else {
            // Table 51, row #22
            mrp_state->vars.MRP_MIM_NReturn++;
        }

        break;

    default:
        break;
    }
}

/******************************************************************************/
// MRP_BASE_in_up_timer_expired()
// Corresponds to "InterconnUpTimer expired" in Table 53 on p. 118 and Table 54
// on p. 121.
/******************************************************************************/
static void MRP_BASE_in_up_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    if (mrp_state->conf.in_role != VTSS_APPL_IEC_MRP_IN_ROLE_MIC) {
        T_EG(MRP_TRACE_GRP_BASE, "%u: InUp timer expired even though we are not configured as a MIC", mrp_state->inst);
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        mrp_timer_stop(mrp_state->in_up_timer);
        return;
    }

    if (mrp_state->vars.in_sm_state != MRP_IN_SM_STATE_PT) {
        mrp_timer_stop(mrp_state->in_up_timer);
        return;
    }

    if (mrp_state->vars.MRP_InLNKNReturn) {
        // Table 53, row #9
        // Table 54, row #8
        mrp_state->vars.MRP_InLNKNReturn--;
        mrp_timer_start(mrp_state->in_up_timer, mrp_state->mic_timing.link_up_interval_usec, false);
        mrp_pdu_in_link_change_tx(mrp_state, true, mrp_state->vars.MRP_InLNKNReturn * (mrp_state->mic_timing.link_up_interval_usec / 1000));
    } else {
        // Table 53, row #8
        // Table 54, row #7
        mrp_state->vars.MRP_InLNKNReturn = mrp_state->mic_timing.link_change_repeat_cnt;
        MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION, true);
        MRP_BASE_in_sm_state_update(mrp_state, MRP_IN_SM_STATE_IP_IDLE, "InterconnUpTimerExpired");
    }
}

/******************************************************************************/
// MRP_BASE_in_down_timer_expired()
// Corresponds to "InterconnDownTimer expired" in Table 53 on p. 118 and Table
// 54 on p. 121.
/******************************************************************************/
static void MRP_BASE_in_down_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    if (mrp_state->conf.in_role != VTSS_APPL_IEC_MRP_IN_ROLE_MIC) {
        T_EG(MRP_TRACE_GRP_BASE, "%u: InDown timer expired even though we are not configured as a MIC", mrp_state->inst);
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        mrp_timer_stop(mrp_state->in_down_timer);
        return;
    }

    if (mrp_state->vars.in_sm_state != MRP_IN_SM_STATE_AC_STAT1) {
        mrp_timer_stop(mrp_state->in_down_timer);
        return;
    }

    if (mrp_state->vars.MRP_InLNKNReturn) {
        // Table 53, row #3
        // Table 54, row #3
        mrp_state->vars.MRP_InLNKNReturn--;
        mrp_timer_start(mrp_state->in_down_timer, mrp_state->mic_timing.link_down_interval_usec, false);
        mrp_pdu_in_link_change_tx(mrp_state, false, mrp_state->vars.MRP_InLNKNReturn * (mrp_state->mic_timing.link_down_interval_usec / 1000));
    } else {
        // Table 53, row #2
        // Table 54, row #2
        mrp_state->vars.MRP_InLNKNReturn = mrp_state->mic_timing.link_change_repeat_cnt;
    }
}

/******************************************************************************/
// MRP_BASE_in_top_timer_expired()
// Corresponds to Table 56 on p. 127.
/******************************************************************************/
static void MRP_BASE_in_top_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    // BUG: According to Table 56, it only acts if the state machine is in IDLE,
    // which is a non-existing state, so I have chosen to let it act in any
    // state.
    if (mrp_state->vars.IN_TC_NReturn) {
        mrp_pdu_in_topology_change_tx(mrp_state, (mrp_state->vars.IN_TC_NReturn * mrp_state->mim_timing.topology_change_interval_usec) / 1000);
        mrp_state->vars.IN_TC_NReturn--;
        mrp_timer_start(mrp_state->in_top_timer, mrp_state->mim_timing.topology_change_interval_usec, false);
    } else {
        mrp_state->vars.IN_TC_NReturn = mrp_state->mim_timing.topology_change_repeat_cnt - 1;
        MRP_BASE_fdb_clear(mrp_state, 0);
        mrp_pdu_in_topology_change_tx(mrp_state, 0);
        mrp_timer_stop(mrp_state->in_top_timer);
    }
}

/******************************************************************************/
// MRP_BASE_in_link_status_timer_expired()
// Corresponds to Table 57 on p. 127.
/******************************************************************************/
static void MRP_BASE_in_link_status_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    // BUG: According to Table 57, it only acts if the state machine is in IDLE,
    // which is a non-existing state, so I have chosen to let it act in any
    // state.
    if (mrp_state->vars.MRP_IN_LNKSTAT_NReturn) {
        MRP_BASE_in_link_status_poll_req(mrp_state);
        mrp_state->vars.MRP_IN_LNKSTAT_NReturn--;
    } else {
        mrp_state->vars.MRP_IN_LNKSTAT_NReturn = mrp_state->mim_timing.link_status_poll_repeat_cnt - 1;
        mrp_pdu_in_link_status_poll_tx(mrp_state);
    }
}

/******************************************************************************/
// MRP_BASE_multiple_mrms_timer_expired()
/******************************************************************************/
static void MRP_BASE_multiple_mrms_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    mrp_state->notif_status.multiple_mrms = false;
    MRP_BASE_notif_status_update(mrp_state);
}

/******************************************************************************/
// MRP_BASE_multiple_mrms_timer2_expired()
// Timer is called multiple_mrms_timer2.
/******************************************************************************/
static void MRP_BASE_multiple_mrms_timer2_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    // If we get here, we re-enable copying of MRP_Test PDUs from other MRMs to
    // the CPU in order to be able to detect multiple MRMs.
    // This can only happen on platforms that has MRP H/W support.
    MRP_BASE_test_copy_conf_set(mrp_state, true);
}

/******************************************************************************/
// MRP_BASE_multiple_mrms_update()
/******************************************************************************/
static void MRP_BASE_multiple_mrms_update(mrp_state_t *mrp_state)
{
    if (!mrp_state->notif_status.multiple_mrms) {
        mrp_state->notif_status.multiple_mrms = true;
        MRP_BASE_notif_status_update(mrp_state);
    }

    // (Re-)start a timer that - when it expires - clears it again.
    // Let the timer expire after 10 seconds.
    mrp_timer_start(mrp_state->multiple_mrms_timer, MRP_MULTIPLE_MRMS_NOTIF_TIMEOUT_USEC, false);

    if (MRP_hw_support) {
        // If we have H/W support for MRP_Test PDUs, we don't want to get all of
        // them to the CPU, but we still want to detect multiple MRMs. We do
        // that by stopping CPU copy of MRP_Test PDUs already handled by H/W and
        // start a timer that expires half the way through the normal expiration
        // of the notification, that is, after 5 seconds.
        MRP_BASE_test_copy_conf_set(mrp_state, false);
        mrp_timer_start(mrp_state->multiple_mrms_timer2, MRP_MULTIPLE_MRMS_NOTIF_TIMEOUT_USEC / 2, false);
    }
}

/******************************************************************************/
// MRP_BASE_multiple_mims_timer2_expired()
// Timer is called multiple_mims_timer2.
/******************************************************************************/
static void MRP_BASE_multiple_mims_timer2_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    // If we get here, we re-enable copying of MRP_InTest PDUs from other MIMs
    // to the CPU in order to be able to detect multiple MIMs.
    // This can only happen on platforms that has MRP H/W support.
    MRP_BASE_in_test_copy_conf_set(mrp_state, true);
}

/******************************************************************************/
// MRP_BASE_multiple_mims_timer_expired()
/******************************************************************************/
static void MRP_BASE_multiple_mims_timer_expired(mrp_timer_t &timer, mrp_state_t *mrp_state)
{
    mrp_state->notif_status.multiple_mims = false;
    MRP_BASE_notif_status_update(mrp_state);
}

/******************************************************************************/
// MRP_BASE_multiple_mims_update()
/******************************************************************************/
static void MRP_BASE_multiple_mims_update(mrp_state_t *mrp_state)
{
    if (!mrp_state->notif_status.multiple_mims) {
        mrp_state->notif_status.multiple_mims = true;
        MRP_BASE_notif_status_update(mrp_state);
    }

    // (Re-)start a timer that - when it expires - clears it again.
    // Let the timer expire after 10 seconds
    mrp_timer_start(mrp_state->multiple_mims_timer, MRP_MULTIPLE_MIMS_NOTIF_TIMEOUT_USEC, false);

    if (MRP_hw_support && mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_RC) {
        // If we have H/W support for MRP_InTest PDUs, we don't want to get all
        // of them to the CPU, but we still want to detect multiple MIMs. We do
        // that by stopping CPU copy of MRP_InTest PDUs already handled by H/W
        // and start a timer that expires half the way through the normal
        // expiration of the notification, that is, after 5 seconds.
        MRP_BASE_in_test_copy_conf_set(mrp_state, false);
        mrp_timer_start(mrp_state->multiple_mims_timer2, MRP_MULTIPLE_MIMS_NOTIF_TIMEOUT_USEC / 2, false);
    }
}

/******************************************************************************/
// MRP_BASE_remote_prio_better_than_own()
// Corresponds to REM_PRIO_BETTER_THAN_OWN_PRIO() on p. 105
/******************************************************************************/
static bool MRP_BASE_remote_prio_better_than_own(mrp_state_t *mrp_state, mrp_rx_pdu_info_t &rx_pdu_info)
{
    bool result = false;

    if (rx_pdu_info.prio < mrp_state->conf.mrm.prio) {
        result = true;
    } else if (rx_pdu_info.prio == mrp_state->conf.mrm.prio) {
        if (rx_pdu_info.sa < MRP_chassis_mac) {
            result = true;
        }
    }

    T_DG(MRP_TRACE_GRP_BASE, "%u: rx.prio = 0x%04x, my.prio = 0x%04x, rx.sa = %s, my.sa = %s => %d",
         mrp_state->inst,
         rx_pdu_info.prio, mrp_state->conf.mrm.prio,
         rx_pdu_info.sa,   MRP_chassis_mac,
         result);

    return result;
}

/******************************************************************************/
// MRP_BASE_remote_prio_better_than_ho()
// Corresponds to REM_PRIO_BETTER_THAN_HO_PRIO() on p. 105
/******************************************************************************/
static bool MRP_BASE_remote_prio_better_than_ho(mrp_state_t *mrp_state, mrp_rx_pdu_info_t &rx_pdu_info)
{
    bool result = false;

    if (rx_pdu_info.prio < mrp_state->vars.HO_BestMRM_Prio) {
        return true;
    } else if (rx_pdu_info.prio == mrp_state->vars.HO_BestMRM_Prio) {
        if (rx_pdu_info.sa < mrp_state->vars.HO_BestMRM_SA) {
            return true;
        }
    }

    T_DG(MRP_TRACE_GRP_BASE, "%u: rx.prio = 0x%04x, ho.prio = 0x%04x, rx.sa = %s, ho.sa = %s => %d",
         mrp_state->inst,
         rx_pdu_info.prio, mrp_state->vars.HO_BestMRM_Prio,
         rx_pdu_info.sa,   mrp_state->vars.HO_BestMRM_SA,
         result);

    return result;
}

/******************************************************************************/
// MRP_BASE_roundtrip_time_compute()
/******************************************************************************/
static void MRP_BASE_roundtrip_time_compute(mrp_rx_pdu_info_t &rx_pdu_info, vtss_appl_iec_mrp_round_trip_statistics_t &s)
{
    uint32_t trunc_rx_time_msecs = (uint32_t)rx_pdu_info.rx_time_msecs, diff_msecs;

    // This roundtrip time is only valid if we don't use the AFI, because if we
    // use the AFI, the MRP_Timestamp field cannot get updated in the Tx'd
    // packets.
    s.valid = !MRP_can_use_afi;

    if (!MRP_can_use_afi) {
        if (trunc_rx_time_msecs >= rx_pdu_info.timestamp) {
            diff_msecs = trunc_rx_time_msecs - rx_pdu_info.timestamp;
        } else {
            diff_msecs = rx_pdu_info.timestamp - trunc_rx_time_msecs;
        }

        if (s.last_update_secs == 0) {
            // This structure has never been updated.
            s.msec_min = s.msec_max = s.msec_last = diff_msecs;
        } else {
            if (diff_msecs < s.msec_min) {
                s.msec_min = diff_msecs;
            }

            if (diff_msecs > s.msec_max) {
                s.msec_max = diff_msecs;
            }

            s.msec_last = diff_msecs;
        }
    }

    s.last_update_secs = vtss::uptime_seconds();
}

/******************************************************************************/
// MRP_BASE_test_rx()
// Corresponds to TestRingInd() on p. 101
/******************************************************************************/
static bool MRP_BASE_test_rx(mrp_state_t *mrp_state, mrp_rx_pdu_info_t &rx_pdu_info, bool own_pdu)
{
    mrp_sm_state_t next_sm_state;
    bool           sec_change, top_tx;
    uint16_t       topology_change_usec;

    T_DG(MRP_TRACE_GRP_BASE, "%u: cur-state = %s. sa = %s, prio = 0x%x, HO_BestMRM_SA = %s, own_pdu = %d, role (oper/conf) = %s/%s, timestamp = %u", mrp_state->inst, MRP_BASE_sm_state_to_str(mrp_state->vars.sm_state), rx_pdu_info.sa, rx_pdu_info.prio, mrp_state->vars.HO_BestMRM_SA, own_pdu, iec_mrp_util_role_to_str(mrp_state->status.oper_role), iec_mrp_util_role_to_str(mrp_state->conf.role), rx_pdu_info.timestamp);

    if (mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRC) {
        // MRCs configured as MRCs don't care about MRP_Test PDUs
        return false;
    }

    if (!own_pdu) {
        if (mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRM) {
            // We are currently working as an MRM, so if we receive MRP_Test
            // PDUs from someone else, we raise a MULTIPLE_MANAGERS event.
            MRP_BASE_multiple_mrms_update(mrp_state);
        }

        if (mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRM) {
            // We are currently MRM configured as MRM.
            // Table 41, row #14, row #28, row #44
            // Ignore
            return true;
        }

        if (mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRM) {
            // We are currently MRM configured as MRA.
            if (MRP_BASE_remote_prio_better_than_own(mrp_state, rx_pdu_info)) {
                // Table 45, row #14, row #31, row #51
                // Ignore
                // BUG: I think the standard should have made us MRC right away,
                // instead of waiting for the other end to send an
                // MRP_TestMgrNAck PDU. An MRM configured as an MRM cannot
                // send MRP_TestMgrNAck PDUs, and then we will have two MRMs on
                // the ring indefinetely. By doing it as I suggest, we could
                // have all-but-one nodes being MRAs and one node being a pure
                // MRM.
            } else {
                // Table 45, row #15, row #32, row #50
                MRP_BASE_test_mgr_nack_req(mrp_state, rx_pdu_info);
            }
        } else {
            // We are currently MRC configured as MRA.
            if (rx_pdu_info.sa == mrp_state->vars.HO_BestMRM_SA) {
                if (MRP_BASE_remote_prio_better_than_own(mrp_state, rx_pdu_info)) {
                    // Table 45, row #72, row #90, row #108, row #124
                    mrp_state->vars.MRP_MonNReturn = 0;
                }

                // Table 45, row #71, row #72, row #89, row #90, row #107,
                // row #108, row #123, row #124
                mrp_state->vars.HO_BestMRM_Prio = rx_pdu_info.prio;
            } else {
                // Table 45, row #70, row #88, row #106, row #122
                // Ignore
                // RBNTBD: Not exactly what the conditions of these rows are
                // saying. The conditions are really odd.
            }
        }

        return true;
    }

    // rx_pdu_info.sa == our own MAC address

    if (mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRM) {
        // Compute roundtrip time
        MRP_BASE_roundtrip_time_compute(rx_pdu_info, mrp_state->status.round_trip_time);
    }

    next_sm_state        = mrp_state->vars.sm_state;
    topology_change_usec = 0;
    sec_change           = false;
    top_tx               = false;

    switch (mrp_state->vars.sm_state) {
    case MRP_SM_STATE_PRM_UP:
    case MRP_SM_STATE_CHK_RO:
    case MRP_SM_STATE_CHK_RC:
        // PRM_UP: Table 41, row #13 and Table 45, row #13
        // CHK_RO: Table 41, row #26 and row #27, and Table 45, row #30.
        // CHK_RC: Table 41, row #43 and Table 45, row #49
        mrp_state->vars.MRP_MRM_NRmax   = mrp_state->mrm_timing.test_monitoring_cnt - 1;
        mrp_state->vars.MRP_MRM_NReturn = 0;
        mrp_state->vars.NO_TC           = false;

        if (mrp_state->vars.sm_state == MRP_SM_STATE_CHK_RO) {
            // Table 41, row #26 and row #27, and Table 45, row #30.
            sec_change = true;
        }

        if (mrp_state->vars.sm_state == MRP_SM_STATE_PRM_UP || mrp_state->vars.sm_state == MRP_SM_STATE_CHK_RO) {
            // Table 41, row #13 and Table 45, row #13
            // Table 41, row #26 and row #27, and Table 45, row #30.
            next_sm_state = MRP_SM_STATE_CHK_RC;
        }

        if (mrp_state->vars.sm_state == MRP_SM_STATE_CHK_RO) {
            // Table 41, row #26 and row #27, and Table 45, row #30.
            top_tx = true;

            if (mrp_state->conf.mrm.react_on_link_change) {
                // Table 41, row #27
                // BUG: Table 45 is probably missing a row that indicates what
                // to do if conf.mrm.react_on_link_change is true/false. We
                // don't care whether we are configured as an MRM or an MRA, but
                // react on the configuration in both cases.
            } else {
                // Table 41, row #26 and Table 45, row #30
                topology_change_usec = mrp_state->mrm_timing.topology_change_interval_usec;
            }
        }

        break;

    default:
        // MRC states only reachable if configured as MRA.
        // Table 45, row #69, row #87, row #105, row #121
        // Ignore
        return true;
    }

    if (sec_change) {
        MRP_BASE_port_state_set(mrp_state, mrp_state->vars.sec_ring_port, false);
    }

    MRP_BASE_sm_state_update(mrp_state, next_sm_state, "TestRingInd");

    if (top_tx) {
        MRP_BASE_topology_change_req(mrp_state, topology_change_usec);
    }

    return true;
}

/******************************************************************************/
// MRP_BASE_topology_change_rx()
/******************************************************************************/
static bool MRP_BASE_topology_change_rx(mrp_state_t *mrp_state, mrp_rx_pdu_info_t &rx_pdu_info, bool own_pdu)
{
    mrp_sm_state_t next_sm_state = mrp_state->vars.sm_state;
    bool           fdb_clear     = false;
    bool           sec_change    = false;

    T_IG(MRP_TRACE_GRP_BASE, "%u: cur-state = %s. rx.sa = %s, rx.interval_msec = %u", mrp_state->inst, MRP_BASE_sm_state_to_str(mrp_state->vars.sm_state), rx_pdu_info.sa, rx_pdu_info.interval_msec);

    // Corresponds to TopologyChangeInd() on p. 102
    switch (mrp_state->vars.sm_state) {
    case MRP_SM_STATE_AC_STAT1:
        // Table 43, row #5
        // Not in Table 45.
        // Ignore
        return true;

    case MRP_SM_STATE_PRM_UP:
    case MRP_SM_STATE_CHK_RO:
    case MRP_SM_STATE_CHK_RC:
        if (mrp_state->conf.role != VTSS_APPL_IEC_MRP_ROLE_MRA) {
            // Table 41, row #20, row #35, and row #50
            // Ignore
            return true;
        } else {
            if (own_pdu) {
                // Table 45, row #18, row #35, row #54
                // Ignore
                return true;
            } else {
                // Table 45, row #19, row #36, row #55
                fdb_clear = true;
            }
        }

        break;

    case MRP_SM_STATE_DE_IDLE:
        // Table 43, row #10
        // Table 45, row #67, row #68
        // BUG: Table 45, row #68 talks about a linkup-hysteresis timer, but
        // it's not specified for how long to run this timer. It looks like a
        // left-over from a previous revision of the standard.
        fdb_clear = true;
        break;

    case MRP_SM_STATE_PT:
        // Table 43, row #17
        // Table 45, row #86
        mrp_timer_stop(mrp_state->up_timer);
        mrp_state->vars.MRP_LNKNReturn = mrp_state->mrc_timing.link_change_repeat_cnt;
        fdb_clear                      = true;
        sec_change                     = true;
        next_sm_state                  = MRP_SM_STATE_PT_IDLE;
        break;

    case MRP_SM_STATE_DE:
        // Table 43, row #24
        // Table 45, row #104
        mrp_timer_stop(mrp_state->down_timer);
        mrp_state->vars.MRP_LNKNReturn = mrp_state->mrc_timing.link_change_repeat_cnt;
        fdb_clear                      = true;
        next_sm_state                  = MRP_SM_STATE_DE_IDLE;
        break;

    case MRP_SM_STATE_PT_IDLE:
        // Table 43, row #29
        // Table 45, row #120
        fdb_clear = true;
        break;

    default:
        break;
    }

    if (fdb_clear) {
        MRP_BASE_fdb_clear(mrp_state, rx_pdu_info.interval_msec);
    }

    if (sec_change) {
        MRP_BASE_port_state_set(mrp_state, mrp_state->vars.sec_ring_port, true);
    }

    MRP_BASE_sm_state_update(mrp_state, next_sm_state, "TopologyChangeInd");

    return true;
}

/******************************************************************************/
// MRP_BASE_link_change_rx()
// BUG: The state machine for the MRA (Table 45) does not indicate support for
// whether the MRC is actually blocking or not blocking its interface, and it
// doesn't show any support for conf.mrm.react_on_link_change.
// I have chosen to implement the Rx of a MRP_LinkChange PDU the same as for the
// MRM state machine (Table 41).
/******************************************************************************/
static bool MRP_BASE_link_change_rx(mrp_state_t *mrp_state, mrp_rx_pdu_info_t &rx_pdu_info, bool link_up)
{
    mrp_sm_state_t next_sm_state = mrp_state->vars.sm_state;
    bool           topology_change_req   = false;
    bool           sec_change            = false, sec_fwd = false;

    if (mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRC) {
        // MRCs configured as MRCs don't care about MRP_LinkChange PDUs
        return false;
    }

    // Corresponds to LinkChangeInd on p. 102
    switch (mrp_state->vars.sm_state) {
    case MRP_SM_STATE_AC_STAT1:
        // Table 41, row #7
        // Table 45, row #7
        // Ignore
        return true;

    case MRP_SM_STATE_PRM_UP:
        if (rx_pdu_info.blocked) {
            if (mrp_state->vars.add_test) {
                // Table 41, row #16
                // Ignore
                return true;
            } else {
                // Table 41, row #15
                mrp_state->vars.add_test = true;
            }
        } else {
            if (link_up) {
                if (!mrp_state->vars.add_test) {
                    // Table 41, row #19
                    mrp_state->vars.add_test = true;
                }

                // Table 41, row #18 and row #19
                topology_change_req = true;
            } else {
                // Table 41, row #17
                // Ignore
                return true;
            }
        }

        break;

    case MRP_SM_STATE_CHK_RO:
        if (link_up) {
            if (rx_pdu_info.blocked) {
                if (mrp_state->vars.add_test) {
                    // Table 41, row #30
                    // Ignore
                    return true;
                } else {
                    // Table 41, row #29
                    mrp_state->vars.add_test = true;
                }
            } else {
                // Table 41, row #33 and row #34
                mrp_state->vars.MRP_MRM_NRmax       = mrp_state->mrm_timing.test_monitoring_extended_cnt - 1;
                mrp_state->vars.MRP_MRM_NReturn     = 0;
                sec_change                          = true;
                sec_fwd                             = false;
                mrp_state->vars.add_test            = true; // row #34
                topology_change_req                 = true;
                next_sm_state                       = MRP_SM_STATE_CHK_RC;
            }
        } else {
            if (mrp_state->vars.add_test) {
                // Table 41, row #31
                // Ignore
                return true;
            } else {
                // Table 41, row #32
                mrp_state->vars.add_test = true;
            }
        }

        break;

    case MRP_SM_STATE_CHK_RC:
        if (mrp_state->conf.mrm.react_on_link_change) {
            if (link_up) {
                if (rx_pdu_info.blocked) {
                    // Table 41, row #49
                    mrp_state->vars.MRP_MRM_NRmax = mrp_state->mrm_timing.test_monitoring_cnt - 1;
                } else {
                    // Table 41, row #48
                    mrp_state->vars.MRP_MRM_NRmax = mrp_state->mrm_timing.test_monitoring_extended_cnt - 1;
                }

                // Table 41, row #48 and row #49
                topology_change_req = true;
            } else {
                // Table 41, row #47
                sec_change           = true;
                sec_fwd              = true;
                topology_change_req  = true;
                next_sm_state        = MRP_SM_STATE_CHK_RO;
            }
        } else {
            if (rx_pdu_info.blocked) {
                if (mrp_state->vars.add_test) {
                    // Table 41, row #45
                    // Ignore
                    return true;
                } else {
                    // Table 41, row #46
                    mrp_state->vars.add_test = true;
                }
            }
        }

        break;

    case MRP_SM_STATE_DE_IDLE:
    case MRP_SM_STATE_PT:
    case MRP_SM_STATE_DE:
    case MRP_SM_STATE_PT_IDLE:
        // Table 45, row #77, row #95, row #113 and row #129
        // Ignore
        return true;

    default:
        return true;
    }

    if (sec_change) {
        MRP_BASE_port_state_set(mrp_state, mrp_state->vars.sec_ring_port, sec_fwd);
    }

    MRP_BASE_sm_state_update(mrp_state, next_sm_state, link_up ? "LinkChangeInd-Up" : "LinkChangeInd-Down");

    if (topology_change_req) {
        MRP_BASE_topology_change_req(mrp_state, 0);
    }

    return true;
}

/******************************************************************************/
// MRP_BASE_test_mgr_nack_rx()
// Corresponds to TestMgrNackInd() on p. 104.
/******************************************************************************/
static bool MRP_BASE_test_mgr_nack_rx(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, mrp_rx_pdu_info_t &rx_pdu_info, bool own_pdu)
{
    mrp_sm_state_t next_sm_state = mrp_state->vars.sm_state;

    T_DG(MRP_TRACE_GRP_BASE, "%u: cur-state = %s. sa = %s, prio = 0x%x, other_sa = %s, other_prio = 0x%x, HO_BestMRM_SA = %s, HO_BestMRM_Prio = 0x%x, role (oper/conf) = %s/%s",
         mrp_state->inst, MRP_BASE_sm_state_to_str(mrp_state->vars.sm_state), rx_pdu_info.sa, rx_pdu_info.prio, rx_pdu_info.other_sa, rx_pdu_info.other_prio,
         mrp_state->vars.HO_BestMRM_SA, mrp_state->vars.HO_BestMRM_Prio,
         iec_mrp_util_role_to_str(mrp_state->status.oper_role), iec_mrp_util_role_to_str(mrp_state->conf.role));

    if (mrp_state->conf.role != VTSS_APPL_IEC_MRP_ROLE_MRA) {
        // Only MRAs handle this.
        return false;
    }

    if (mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRC) {
        // Table 45, row #76, row #94, row #112, row #128
        // Ignore
        return true;
    }

    if (own_pdu) {
        // Table 45, row #20, row #37, row #56
        // Originally sent by ourselves
        // Ignore
        return true;
    }

    if (rx_pdu_info.other_sa != MRP_chassis_mac) {
        // Table 45, row #21, row #38, row #57
        // Not destined for us
        // Ignore
        return true;
    }

    if (MRP_BASE_remote_prio_better_than_ho(mrp_state, rx_pdu_info)) {
        // Table 45, row #23, row #40, row #59
        mrp_state->vars.HO_BestMRM_SA   = rx_pdu_info.sa;
        mrp_state->vars.HO_BestMRM_Prio = rx_pdu_info.prio;
        MRP_BASE_mesa_best_mrm_set(mrp_state);
    }

    // Table 45, row #22, row #23, row #39, row #40, row #58, row #59
    switch (mrp_state->vars.sm_state) {
    case MRP_SM_STATE_PRM_UP:
        // Table 45, row #22, row #23
        next_sm_state = MRP_SM_STATE_DE_IDLE;
        break;

    case MRP_SM_STATE_CHK_RO:
        // Table 45, row #39, row #40
        next_sm_state = MRP_SM_STATE_PT_IDLE;
        break;

    case MRP_SM_STATE_CHK_RC:
        // Table 45, row #58, row #59
        // BUG: Row #59 forgets to set secondary ring port as forwarding!
        MRP_BASE_port_state_set(mrp_state, mrp_state->vars.sec_ring_port, true);
        next_sm_state = MRP_SM_STATE_PT_IDLE;
        break;

    default:
        break;
    }

    mrp_timer_stop(mrp_state->top_timer);
    MRP_BASE_mrc_init(mrp_state);
    MRP_BASE_sm_state_update(mrp_state, next_sm_state, "TestMgrNAckInd");
    MRP_BASE_test_propagate_req(mrp_state);

    return true;
}

/******************************************************************************/
// MRP_BASE_test_propagate_rx()
// Corresponds to TestPropagateInd() on p. 104
/******************************************************************************/
static bool MRP_BASE_test_propagate_rx(mrp_state_t *mrp_state, mrp_rx_pdu_info_t &rx_pdu_info, bool own_pdu)
{
    T_DG(MRP_TRACE_GRP_BASE, "%u: sa = %s, prio = 0x%x, other_sa = %s, other_prio = 0x%x, own_pdu = %d", mrp_state->inst, rx_pdu_info.sa, rx_pdu_info.prio, rx_pdu_info.other_sa, rx_pdu_info.other_prio, own_pdu);

    if (mrp_state->conf.role != VTSS_APPL_IEC_MRP_ROLE_MRA) {
        // Only MRAs handle this PDU type
        return false;
    }

    switch (mrp_state->vars.sm_state) {
    case MRP_SM_STATE_PRM_UP:
    case MRP_SM_STATE_CHK_RO:
    case MRP_SM_STATE_CHK_RC:
        // Table 45, row #24, row #41, row #60
        // Ignore
        break;

    case MRP_SM_STATE_DE_IDLE:
    case MRP_SM_STATE_PT:
    case MRP_SM_STATE_DE:
    case MRP_SM_STATE_PT_IDLE:
        if (own_pdu) {
            // Table 45, row #73, row #91, row #109, row #125
            // Ignore
        } else {
            if (rx_pdu_info.sa == rx_pdu_info.other_sa) {
                // Table 45, row #75, row #93, row #111, row #127
                mrp_state->vars.HO_BestMRM_SA   = rx_pdu_info.other_sa;
                mrp_state->vars.HO_BestMRM_Prio = rx_pdu_info.other_prio;
                mrp_state->vars.MRP_MonNReturn  = 0;
                MRP_BASE_mesa_best_mrm_set(mrp_state);
            } else {
                // Table 45, row #74, row #92, row #110, row #126
                // Ignore
            }
        }

        break;

    default:
        break;
    }

    return true;
}

/******************************************************************************/
// MRP_BASE_in_test_rx()
// Corresponds to InterconnTestInd() on p. 124.
//
// MIM-RC: Table 51 on p. 112
/******************************************************************************/
static bool MRP_BASE_in_test_rx(mrp_state_t *mrp_state, mrp_rx_pdu_info_t &rx_pdu_info, bool own_pdu)
{
    mrp_in_sm_state_t next_in_sm_state;
    bool              in_change, in_top_tx;

    if (mrp_state->conf.in_role != VTSS_APPL_IEC_MRP_IN_ROLE_MIM || mrp_state->conf.in_mode != VTSS_APPL_IEC_MRP_IN_MODE_RC) {
        // Only MIMs in RC mode care about MRP_InTest PDUs.
        return false;
    }

    if (own_pdu) {
        // Compute roundtrip time
        MRP_BASE_roundtrip_time_compute(rx_pdu_info, mrp_state->status.in_round_trip_time);
    } else {
        // Detect multiple MIMs on the interconnection ring. It appears that it
        // is allowed to receive other interconnection rings' MRP_InXxx PDUs and
        // that we only should react on PDUs with the same Interconnection ID,
        // so we can only raise this notification if the IID is identical to
        // our own.
        MRP_BASE_multiple_mims_update(mrp_state);
    }

    next_in_sm_state = mrp_state->vars.in_sm_state;
    in_change        = false;
    in_top_tx        = false;

    switch (mrp_state->vars.in_sm_state) {
    case MRP_IN_SM_STATE_AC_STAT1:
        if (own_pdu) {
            // Table 51, row #5
            in_change = true;
            mrp_state->vars.MRP_MIM_NRmax   = mrp_state->mim_timing.test_monitoring_cnt - 1;
            mrp_state->vars.MRP_MIM_NReturn = 0;
            next_in_sm_state                = MRP_IN_SM_STATE_CHK_IC;
        } else {
            // Table 51, row #6, #7
            // Forwarding to other ports if it's not destined for our
            // interconnection (can be seen by own_pdu or - even better - IID).
            // This is not supported because we don't want to S/W forward these
            // very frequent MRP_InTest PDUs. Users must put the MIM in LC mode
            // instead, if the same ring interconnects two other rings.
        }

        break;

    case MRP_IN_SM_STATE_CHK_IO:
        if (own_pdu) {
            // Table 51, row #14
            mrp_state->vars.MRP_MIM_NRmax   = mrp_state->mim_timing.test_monitoring_cnt - 1;
            mrp_state->vars.MRP_MIM_NReturn = 0;
            in_change                       = true;
            in_top_tx                       = true;
            next_in_sm_state                = MRP_IN_SM_STATE_CHK_IC;
        } else {
            // Table 51, row #15, #16
            // See comment in MRP_IN_SM_STATE_AC_STAT1.
        }

        break;

    case MRP_IN_SM_STATE_CHK_IC:
        if (own_pdu) {
            // Table 51, row #25
            mrp_state->vars.MRP_MIM_NRmax   = mrp_state->mim_timing.test_monitoring_cnt - 1;
            mrp_state->vars.MRP_MIM_NReturn = 0;
        } else {
            // Table 51, row #26, #27
            // See comment in MRP_IN_SM_STATE_AC_STAT1.
        }

        break;

    default:
        break;
    }

    if (in_change) {
        MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION, false);
    }

    MRP_BASE_in_sm_state_update(mrp_state, next_in_sm_state, "InterconnTestInd");

    if (in_top_tx) {
        MRP_BASE_in_topology_change_req(mrp_state, mrp_state->mim_timing.topology_change_interval_usec);
    }

    return true;
}

/******************************************************************************/
// MRP_BASE_in_topology_change_rx()
// Corresponds to InterconnTopologyChangeInd() on p. 125.
/******************************************************************************/
static bool MRP_BASE_in_topology_change_rx(mrp_state_t *mrp_state, mrp_rx_pdu_info_t &rx_pdu_info, bool own_pdu)
{
    mrp_in_sm_state_t next_in_sm_state;
    bool              in_change;

    if (mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRC && mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
        // Used by MRM, MRA, MIM-LC, MIM-RC, MIC-LC, MIC-RC, but not MRC-only.
        return false;
    }

    // Detect multiple MIMs in LC mode.
    if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM &&
        mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_LC  &&
        !own_pdu                                                 &&
        rx_pdu_info.in_id == mrp_state->conf.in_id) {
        // In LC mode, we use MRP_InTopologyChange PDUs to detect multiple MIMs
        // on the interconnection ring, because these are only sent by MIMs. In
        // a ladder topology, it is indeed possible that the
        // MRP_InTopologyChange PDU is received on the ring ports, so if we
        // receive an MRP_InTopologyChange PDU with our interconnection ID that
        // is not our own, we have multiple MIMs.
        MRP_BASE_multiple_mims_update(mrp_state);
    }

    // MRM (and MRA) handling
    if (mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRM && !mrp_timer_active(mrp_state->top_timer)) {
        switch (mrp_state->vars.sm_state) {
        case MRP_SM_STATE_PRM_UP:
        case MRP_SM_STATE_CHK_RO:
        case MRP_SM_STATE_CHK_RC:
            // Table 41, row #51, row #52, row #57
            // BUG: MRA State machine does not specify this handling, but I
            // guess it should - hence check for status.oper_role rather than
            // conf.role above.
            MRP_BASE_topology_change_req(mrp_state, rx_pdu_info.interval_msec * 1000);
            break;

        default:
            break;
        }
    }

    if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
        // Nothing else to do.
        return true;
    }

    // MIM-LC and MIM-RC handling
    if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM) {
        switch (mrp_state->vars.in_sm_state) {
        case MRP_IN_SM_STATE_AC_STAT1:
        case MRP_IN_SM_STATE_CHK_IO:
        case MRP_IN_SM_STATE_CHK_IC:
            if (own_pdu) {
                // Table 50, row #6, row #12, row #18
                // Table 51, row #10, row #20, row #31
                // Ignore
            } else {
                // Table 50, row #5, row #11, row #17
                // Table 51, row #9, row #19, row #30
                MRP_BASE_fdb_clear(mrp_state, rx_pdu_info.interval_msec);
            }

            break;

        default:
            break;
        }

        return true;
    }

    // MIC-LC and MIC-RC handling
    next_in_sm_state = mrp_state->vars.in_sm_state;
    in_change        = false;

    switch (mrp_state->vars.in_sm_state) {
    case MRP_IN_SM_STATE_AC_STAT1:
        if (rx_pdu_info.in_id == mrp_state->conf.in_id) {
            // Table 53, row #6
            // Table 54, row #6
            mrp_timer_stop(mrp_state->in_down_timer);
        }

        break;

    case MRP_IN_SM_STATE_PT:
        if (rx_pdu_info.in_id == mrp_state->conf.in_id) {
            // Table 53, row #12
            // Table 54, row #11
            mrp_timer_stop(mrp_state->in_up_timer);
            mrp_state->vars.MRP_InLNKNReturn = mrp_state->mic_timing.link_change_repeat_cnt;
            in_change                        = true;
            next_in_sm_state                 = MRP_IN_SM_STATE_IP_IDLE;
        }

        break;

    case MRP_IN_SM_STATE_IP_IDLE:
    default:
        // Table 53, row #16
        // Table 54, row #14
        // Ignore
        return true;
    }

    if (in_change) {
        MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION, true);
    }

    MRP_BASE_in_sm_state_update(mrp_state, next_in_sm_state, "InterconnTopologyChangeInd");
    return true;
}

/******************************************************************************/
// MRP_BASE_in_link_change_rx()
// Corresponds to InterconnLinkChangeInd() on p. 126.
/******************************************************************************/
static bool MRP_BASE_in_link_change_rx(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, mrp_rx_pdu_info_t &rx_pdu_info, bool link_up)
{
    mrp_in_sm_state_t next_in_sm_state;
    bool              in_change, in_fwd, in_top_tx;

    if (mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRC && mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
        // Used by MRM, MRA, MIM-LC, MIM-RC, MIC-LC, MIC-RC, but not MRC-only.
        return false;
    }

    // The MRM state machine (Table 41) handles MRP_InLinkChange frames received
    // on one ring port by forwarding them to the other. The forwarding is
    // handled by MRP_BASE_sw_forward()
    // This covers Table 41, row #53, row #54.
    // BUG: I guess this functionality is also part of the MRA state machine.

    if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
        // Nothing else to do.
        return true;
    }

    // MIM-LC and MIM-RC handling

    next_in_sm_state = mrp_state->vars.in_sm_state;
    in_change        = false;
    in_fwd           = false;
    in_top_tx        = false;

    if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM) {
        switch (mrp_state->vars.in_sm_state) {
        case MRP_IN_SM_STATE_AC_STAT1:
            // Table 50, row #4
            // Table 51, row #8
            // Ignore
            return true;

        case MRP_IN_SM_STATE_CHK_IO:
            if (rx_pdu_info.in_id != mrp_state->conf.in_id) {
                break;
            }

            if (mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_LC) {
                if (link_up) {
                    // Table 50, row #9
                    mrp_timer_stop(mrp_state->in_link_status_timer);
                    in_change        = true;
                    in_fwd           = false;
                    in_top_tx        = true;
                    next_in_sm_state = MRP_IN_SM_STATE_CHK_IC;
                } else {
                    // Table 50, row #10
                    in_change = true;
                    in_fwd    = true;
                }
            } else {
                if (link_up) {
                    // Table 51, row #17
                } else {
                    // Table 51, row #18
                    // Ignore
                    return true;
                }
            }

            break;

        case MRP_IN_SM_STATE_CHK_IC:
            if (rx_pdu_info.in_id != mrp_state->conf.in_id) {
                break;
            }

            if (link_up) {
                if (mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_LC) {
                    // Table 50, row #16
                    // Ignore
                    return true;
                } else {
                    // Table 51, row #29
                    mrp_state->vars.MRP_MIM_NRmax = mrp_state->mim_timing.test_monitoring_cnt - 1;
                    in_top_tx                     = true;
                }
            } else {
                // Table 50, row #15
                // Table 51, row #28
                mrp_timer_stop(mrp_state->in_link_status_timer); // Only LC, but I guess it doesn't harm for RC.
                in_change        = true;
                in_fwd           = true;
                in_top_tx        = true;
                next_in_sm_state = MRP_IN_SM_STATE_CHK_IO;
            }

            break;

        default:
            return true;
        }

        if (in_change) {
            MRP_BASE_port_state_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION, in_fwd);
        }

        MRP_BASE_in_sm_state_update(mrp_state, next_in_sm_state, link_up ? "InterconnLinkChangeInd-Up" : "InterconnLinkChangeInd-Down");

        if (in_top_tx) {
            MRP_BASE_in_topology_change_req(mrp_state, mrp_state->mim_timing.topology_change_interval_usec);
        }

        return true;
    }

    // MIC-LC and MIC-RC handling
    // Table 53, row #18 and Table 54, row #15 state that in IP_IDLE,
    // MRP_InLinkChange PDUs must be forwarded from a non-interconnection port
    // (that is, from a ring port) to the interconnection port. This is handled
    // by MRP_BASE_sw_forward(), so nothing more to do.
    return true;
}

/******************************************************************************/
// MRP_BASE_in_link_status_poll_rx()
// Corresponds to InterconnLinkStatusPollInd() on p. 126.
/******************************************************************************/
static bool MRP_BASE_in_link_status_poll_rx(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, mrp_rx_pdu_info_t &rx_pdu_info, bool own_pdu)
{
    bool link_is_up;

    if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM &&
        mrp_state->conf.in_mode == VTSS_APPL_IEC_MRP_IN_MODE_LC  &&
        !own_pdu                                                 &&
        rx_pdu_info.in_id == mrp_state->conf.in_id) {
        // In LC mode, we use MRP_InLinkStatusPoll PDUs to detect multiple MIMs
        // on the interconnection ring, because these are only sent by MIMs. In
        // a ladder topology, it is indeed possible that the
        // MRP_InTopologyChange PDU is received on the ring ports, so if we
        // receive an MRP_InTopologyChange PDU with our interconnection ID that
        // is not our own, we have multiple MIMs.
        MRP_BASE_multiple_mims_update(mrp_state);
    }

    // Used by MRM, MRA, and MIC-LC.
    if (mrp_state->conf.role == VTSS_APPL_IEC_MRP_ROLE_MRC && (mrp_state->conf.in_role != VTSS_APPL_IEC_MRP_IN_ROLE_MIC || mrp_state->conf.in_mode != VTSS_APPL_IEC_MRP_IN_MODE_LC)) {
        return false;
    }

    // The MRM state machine (Table 41) handles MRP_InLinkStatusPoll frames
    // received on one ring port by forwarding them to the other. This is
    // handled by MRP_BASE_sw_forward().
    // This covers Table 41, row #55, row #56.
    // BUG: I guess this functionality is also part of the MRA state machine.

    if (mrp_state->conf.in_role != VTSS_APPL_IEC_MRP_IN_ROLE_MIC || mrp_state->conf.in_mode != VTSS_APPL_IEC_MRP_IN_MODE_LC) {
        // Besides MRM/MRA, it's only used by MIC-LC.
        return true;
    }

    if (rx_pdu_info.in_id != mrp_state->conf.in_id) {
        // Not for us.
        return true;
    }

    switch (mrp_state->vars.in_sm_state) {
    case MRP_IN_SM_STATE_AC_STAT1:
        // Table 53, row #7
        link_is_up = false;
        break;

    case MRP_IN_SM_STATE_PT:
        // Table 53, row #13
        link_is_up = true;
        break;

    case MRP_IN_SM_STATE_IP_IDLE:
        // Table 53, row #17
        link_is_up = true;

        // It also states that it should go to the interconnection port.
        // This is handled by MRP_BASE_sw_forward().
        break;

    default:
        T_EG(MRP_TRACE_GRP_BASE, "%u: Invalid SM state (%d)", mrp_state->inst, mrp_state->vars.in_sm_state);
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
        return false;
    }

    mrp_pdu_in_link_change_tx(mrp_state, link_is_up, 0);
    return true;
}

//*****************************************************************************/
// MRP_BASE_state_clear()
// Clears state maintained by base.
/******************************************************************************/
static void MRP_BASE_state_clear(mrp_state_t *mrp_state)
{
    vtss_appl_iec_mrp_port_type_t     port_type;
    vtss_appl_iec_mrp_oper_state_t    oper_state;
    vtss_appl_iec_mrp_oper_warnings_t oper_warnings;
    int                               i, j;

    T_IG(MRP_TRACE_GRP_BASE, "%u: Enter", mrp_state->inst);

    // Reset ourselves, but keep a couple of the fields, which are maintained by
    // iec_mrp.cxx and already configured.
    oper_state    = mrp_state->status.oper_state;
    oper_warnings = mrp_state->status.oper_warnings;

    vtss_clear(mrp_state->status);
    vtss_clear(mrp_state->notif_status);

    mrp_state->status.oper_state    = oper_state;
    mrp_state->status.oper_warnings = oper_warnings;

    // Other initializations
    mrp_state->status.ring_state    = VTSS_APPL_IEC_MRP_RING_STATE_DISABLED;
    mrp_state->status.in_ring_state = VTSS_APPL_IEC_MRP_RING_STATE_DISABLED;
    mrp_state->status.oper_role     = mrp_state->conf.role;

    vtss_clear(mrp_state->ring_port_states);
    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        mrp_state->status.port_status[port_type].forwarding = true;

        // Initially there's no link on the ports.
        mrp_state->status.port_status[port_type].sf = true;

        // Initially we haven't started any AFI flows.
        mrp_state->ring_port_states[port_type].test_afi_id = AFI_ID_NONE;
    }

    for (i = 0; i < ARRSZ(mrp_state->ace_conf); i++) {
        for (j = 0; j < ARRSZ(mrp_state->ace_conf[i]); j++) {
            mrp_state->ace_conf[i][j].id = ACL_MGMT_ACE_ID_NONE;
        }
    }

    vtss_clear(mrp_state->vars.mesa_copy_conf);
    mrp_state->vars.sm_state               = MRP_SM_STATE_POWER_ON;
    mrp_state->vars.in_sm_state            = MRP_IN_SM_STATE_POWER_ON;
    mrp_state->vars.MRP_LNKNReturn         = 0;
    mrp_state->vars.MRP_MRM_NRmax          = 0;
    mrp_state->vars.MRP_MRM_NReturn        = 0;
    mrp_state->vars.TC_NReturn             = 0;
    mrp_state->vars.HO_BestMRM_Prio        = 0;
    vtss_clear(mrp_state->vars.HO_BestMRM_SA);
    mrp_state->vars.add_test               = false;
    mrp_state->vars.NO_TC                  = false;
    vtss_clear(mrp_state->vars.MRP_BestMRM_SA);
    mrp_state->vars.MRP_BestMRM_Prio       = 0;
    mrp_state->vars.MRP_MonNReturn         = 0;
    mrp_state->vars.MRP_MIM_NRmax          = 0;
    mrp_state->vars.MRP_MIM_NReturn        = 0;
    mrp_state->vars.IN_TC_NReturn          = 0;
    mrp_state->vars.MRP_IN_LNKSTAT_NReturn = 0;
    mrp_state->vars.MRP_InLNKNReturn       = 0;
    mrp_state->vars.syslog_cnt             = 0;
    mrp_state->added_to_mesa               = false;

    // mrp_timer_init() also stops the timer if active.
    mrp_state->vars.test_rx_timer_active    = false;
    mrp_state->vars.in_test_rx_timer_active = false;
    mrp_timer_init(mrp_state->test_rx_timer,        "TestRx",        mrp_state->inst, MRP_BASE_test_timer_expired,           mrp_state);
    mrp_timer_init(mrp_state->up_timer,             "Up",            mrp_state->inst, MRP_BASE_up_timer_expired,             mrp_state);
    mrp_timer_init(mrp_state->down_timer,           "Down",          mrp_state->inst, MRP_BASE_down_timer_expired,           mrp_state);
    mrp_timer_init(mrp_state->top_timer,            "Top",           mrp_state->inst, MRP_BASE_top_timer_expired,            mrp_state);
    mrp_timer_init(mrp_state->fdb_clear_timer,      "FDB",           mrp_state->inst, MRP_BASE_fdb_clear_timer_expired,      mrp_state);
    mrp_timer_init(mrp_state->in_test_rx_timer,     "InTestRx",      mrp_state->inst, MRP_BASE_in_test_timer_expired,        mrp_state);
    mrp_timer_init(mrp_state->in_up_timer,          "InUp",          mrp_state->inst, MRP_BASE_in_up_timer_expired,          mrp_state);
    mrp_timer_init(mrp_state->in_down_timer,        "InDown",        mrp_state->inst, MRP_BASE_in_down_timer_expired,        mrp_state);
    mrp_timer_init(mrp_state->in_top_timer,         "InTop",         mrp_state->inst, MRP_BASE_in_top_timer_expired,         mrp_state);
    mrp_timer_init(mrp_state->in_link_status_timer, "InLinkStatus",  mrp_state->inst, MRP_BASE_in_link_status_timer_expired, mrp_state);
    mrp_timer_init(mrp_state->multiple_mrms_timer,  "MultipleMRMs",  mrp_state->inst, MRP_BASE_multiple_mrms_timer_expired,  mrp_state);
    mrp_timer_init(mrp_state->multiple_mrms_timer2, "MultipleMRMs2", mrp_state->inst, MRP_BASE_multiple_mrms_timer2_expired, mrp_state);
    mrp_timer_init(mrp_state->multiple_mims_timer,  "MultipleMIMs",  mrp_state->inst, MRP_BASE_multiple_mims_timer_expired,  mrp_state);
    mrp_timer_init(mrp_state->multiple_mims_timer2, "MultipleMIMs2", mrp_state->inst, MRP_BASE_multiple_mims_timer2_expired, mrp_state);

    mrp_state->history.clear();
    MRP_BASE_history_update(mrp_state, "Init");
}

//*****************************************************************************/
// MRP_BASE_do_deactivate()
/******************************************************************************/
static mesa_rc MRP_BASE_do_deactivate(mrp_state_t *mrp_state, bool use_old_conf)
{
    vtss_appl_iec_mrp_port_type_t port_type;
    mesa_rc                       rc, first_encountered_rc = VTSS_RC_OK;

    T_IG(MRP_TRACE_GRP_BASE, "%u: use_old_conf = %d", mrp_state->inst, use_old_conf);

    // Stop all timers
    mrp_state->vars.test_rx_timer_active    = false;
    mrp_state->vars.in_test_rx_timer_active = false;
    mrp_timer_stop(mrp_state->test_rx_timer);
    mrp_timer_stop(mrp_state->up_timer);
    mrp_timer_stop(mrp_state->down_timer);
    mrp_timer_stop(mrp_state->top_timer);
    mrp_timer_stop(mrp_state->fdb_clear_timer);
    mrp_timer_stop(mrp_state->in_test_rx_timer);
    mrp_timer_stop(mrp_state->in_up_timer);
    mrp_timer_stop(mrp_state->in_down_timer);
    mrp_timer_stop(mrp_state->in_top_timer);
    mrp_timer_stop(mrp_state->in_link_status_timer);
    mrp_timer_stop(mrp_state->multiple_mrms_timer);
    mrp_timer_stop(mrp_state->multiple_mrms_timer2);
    mrp_timer_stop(mrp_state->multiple_mims_timer);
    mrp_timer_stop(mrp_state->multiple_mims_timer2);

    // Free frame pointers.
    mrp_pdu_free_all(mrp_state);

    if (MRP_hw_support) {
        // Disable interrupts
        if ((rc = MRP_BASE_mesa_intr_set(mrp_state, false)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }

        // Remove MRP instance from API
        if ((rc = MRP_BASE_mesa_conf_del(mrp_state)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }

        mrp_state->added_to_mesa = false;
    } else {
        // Remove our ACEs.
        if ((rc = MRP_BASE_ace_remove(mrp_state)) != VTSS_RC_OK) {
            if (first_encountered_rc == VTSS_RC_OK) {
                first_encountered_rc = rc;
            }
        }
    }

    // Set all involved ports to forwarding
    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        // Back out of any blocking the port might have
        MRP_BASE_port_state_set(mrp_state, port_type, true, false /* don't force MESA updates */, use_old_conf, false /* don't update ACEs */);
    }

    // Clear the state
    MRP_BASE_state_clear(mrp_state);

    // And remove our entry from mrp_notificaiton_status
    mrp_notification_status.del(mrp_state->inst);

    return first_encountered_rc;
}

/******************************************************************************/
// MRP_BASE_do_activate()
/******************************************************************************/
mesa_rc MRP_BASE_do_activate(mrp_state_t *mrp_state, bool initial_sf_port1, bool initial_sf_port2, bool initial_sf_in)
{
    mesa_rc rc;

    T_IG(MRP_TRACE_GRP_BASE, "%u: SF: port1 = %d, port2 = %d, in = %d", mrp_state->inst, initial_sf_port1, initial_sf_port2, initial_sf_in);

    MRP_BASE_state_clear(mrp_state);

    // Create required PDUs. These are merely placeholders, and various fields
    // inside the PDUs will be updated on the fly.
    mrp_pdu_create_all(mrp_state);

    // Create an empty entry in mrp_notification_status
    MRP_BASE_notif_status_update(mrp_state);

    // Flush the entire FDB
    T_IG(MRP_TRACE_GRP_API, "%u: mesa_mac_table_flush()", mrp_state->inst);
    if ((rc = mesa_mac_table_flush(nullptr)) != VTSS_RC_OK) {
        T_EG(MRP_TRACE_GRP_API, "%u: mesa_mac_table_flush() failed: %s", mrp_state->inst, error_txt(rc));
        mrp_state->status.oper_warnings |= VTSS_APPL_IEC_MRP_OPER_WARNING_INTERNAL_ERROR;
    }

    if (MRP_hw_support) {
        // With H/W support, the API takes care of forwarding and CPU copying.
        VTSS_RC(MRP_BASE_mesa_conf_add(mrp_state));

        // We also need to configure timing parameters for Loss of Continuity
        // detection.
        VTSS_RC(MRP_BASE_mesa_loc_conf_set(mrp_state, mrp_state->mrm_timing.test_interval_default_usec, mrp_state->mim_timing.test_interval_default_usec));

        // Enable or disable interrupts
        VTSS_RC(MRP_BASE_mesa_intr_set(mrp_state, true));
    } else {
        // Prepare rules that capture MRP PDUs and possibly forwards them - one
        // per DMAC.
        VTSS_RC(MRP_BASE_ace_prepare(mrp_state));
    }

    switch (mrp_state->conf.role) {
    case VTSS_APPL_IEC_MRP_ROLE_MRC:
        MRP_BASE_mrc_power_on(mrp_state);
        break;

    case VTSS_APPL_IEC_MRP_ROLE_MRM:
        MRP_BASE_mrm_power_on(mrp_state);
        break;

    case VTSS_APPL_IEC_MRP_ROLE_MRA:
        MRP_BASE_mra_power_on(mrp_state);
        break;
    }

    switch (mrp_state->conf.in_role) {
    case VTSS_APPL_IEC_MRP_IN_ROLE_NONE:
        break;

    case VTSS_APPL_IEC_MRP_IN_ROLE_MIM:
        MRP_BASE_mim_power_on(mrp_state);
        break;

    case VTSS_APPL_IEC_MRP_IN_ROLE_MIC:
        MRP_BASE_mic_power_on(mrp_state);
        break;
    }

    // Set the initial value of SF of the two ring ports.
    mrp_base_sf_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1,      initial_sf_port1, true);
    mrp_base_sf_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2,      initial_sf_port2, true);
    mrp_base_sf_set(mrp_state, VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION, initial_sf_in,    true);

    return VTSS_RC_OK;
}

//*****************************************************************************/
// mrp_base_activate()
/******************************************************************************/
mesa_rc mrp_base_activate(mrp_state_t *mrp_state, bool initial_sf_port1, bool initial_sf_port2, bool initial_sf_in)
{
    mesa_rc rc;

    if ((rc = MRP_BASE_do_activate(mrp_state, initial_sf_port1, initial_sf_port2, initial_sf_in)) != VTSS_RC_OK) {
        (void)MRP_BASE_do_deactivate(mrp_state, false /* Use current conf */);
    }

    return rc;
}

/******************************************************************************/
// mrp_base_deactivate()
/******************************************************************************/
mesa_rc mrp_base_deactivate(mrp_state_t *mrp_state)
{
    return MRP_BASE_do_deactivate(mrp_state, true /* Use old_conf */);
}

/******************************************************************************/
// mrp_base_statistics_clear()
/******************************************************************************/
void mrp_base_statistics_clear(mrp_state_t *mrp_state)
{
    vtss_appl_iec_mrp_port_type_t port_type;

    for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
        vtss_clear(mrp_state->status.port_status[port_type].statistics);
    }

    // Also clear the global statistics
    mrp_state->status.flush_cnt                    = 0;
    mrp_state->status.mrm_mrc_transitions          = 0;
    mrp_state->status.transitions                  = 0; // Also affects MRP_Transition in PDUs
    mrp_state->status.in_transitions               = 0; // Also affects MRP_Transition in PDUs

    vtss_clear(mrp_state->status.round_trip_time);
    vtss_clear(mrp_state->status.in_round_trip_time);
}

/******************************************************************************/
// mrp_base_tx_frame_update()
/******************************************************************************/
void mrp_base_tx_frame_update(mrp_state_t *mrp_state)
{
    // Recreate frames. No need to free first, because that's handled by the
    // following function.
    mrp_pdu_create_all(mrp_state);
}

/******************************************************************************/
// mrp_base_recovery_profile_update()
/******************************************************************************/
void mrp_base_recovery_profile_update(mrp_state_t *mrp_state)
{
    // Either the recovery_profile or in_recovery_profile has been changed.
    // If we have MRP H/W support, we need to update MESA. Otherwise, we wait
    // until the new parameters are being used.
    if (MRP_hw_support) {
        (void)MRP_BASE_mesa_loc_conf_set(mrp_state, mrp_state->mrm_timing.test_interval_default_usec, mrp_state->mim_timing.test_interval_default_usec);
    }
}

/******************************************************************************/
// mrp_base_sf_set()
/******************************************************************************/
void mrp_base_sf_set(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, bool sf, bool first_invocation)
{
    if (!mrp_state->using_port_type(port_type)) {
        return;
    }

    // If we have sf on first invocation, we need to increment sf_cnt, hence the
    // check for first_invocation below. Also, it doesn't hurt to run the state
    // machine for this the first time.
    if (!first_invocation && mrp_state->status.port_status[port_type].sf == sf) {
        // No change
        return;
    }

    if (sf) {
        mrp_state->status.port_status[port_type].statistics.sf_cnt++;
    }

    mrp_state->status.port_status[port_type].sf = sf;

    if (port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION) {
        MRP_BASE_in_link_change(mrp_state, !sf);
    } else {
        MRP_BASE_link_change(mrp_state, port_type, !sf);
    }
}

/******************************************************************************/
// MRP_BASE_sw_forward()
/******************************************************************************/
static void MRP_BASE_sw_forward(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t rx_port_type, mrp_rx_pdu_info_t rx_pdu_info, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    vtss_appl_iec_mrp_port_type_t tx_port_type;
    bool                          forward[3] = {}, rx_on_blocked_ring_port;

    if (mrp_state->conf.in_role != VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
        // We are a MIM or a MIC. In general, if we receive an interconnection
        // PDU on our interconnection port, and it's not our own IID, we discard
        // it.
        if (rx_port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION && rx_pdu_info.in_id != mrp_state->conf.in_id) {
            return;
        }
    }

    // Determine termination or forwarding to interconnection port.
    if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIM) {
        // We are a MIM, so if the frame is our own, we don't forward it to
        // anyone.
        if (rx_pdu_info.in_id == mrp_state->conf.in_id) {
            return;
        }

        // It's not our own MRP_InXXX PDU, so let the MRM/MRC operational role
        // determine whether to forward to the other ring port.
    } else if (mrp_state->conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_MIC) {
        // We are a MIC, so we forward the frame to the interconnection port if
        // it was not received on that port and it indeed is our IID.
        forward[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION] = rx_port_type != VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION && rx_pdu_info.in_id == mrp_state->conf.in_id;
    }

    // Determine forwarding to ring ports.
    if (mrp_state->status.oper_role == VTSS_APPL_IEC_MRP_ROLE_MRM) {
        // If received on I/C port, we forward to non-blocked ring ports.
        // If received on non-blocked ring port, we forward to non-blocked
        // opposite ring port.
        rx_on_blocked_ring_port = rx_port_type != VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION && !mrp_state->status.port_status[rx_port_type].forwarding;
        forward[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1] = !rx_on_blocked_ring_port && rx_port_type != VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 && mrp_state->status.port_status[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1].forwarding;
        forward[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2] = !rx_on_blocked_ring_port && rx_port_type != VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 && mrp_state->status.port_status[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2].forwarding;
    } else {
        // As MRC, we don't obey the blocked state of the port, only the port on
        // which it was received.
        forward[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1] = rx_port_type != VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1;
        forward[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2] = rx_port_type != VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2;
    }

    // Now do the forwarding
    for (tx_port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; tx_port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; tx_port_type++) {
        if (!forward[tx_port_type]) {
            continue;
        }

        T_IG(MRP_TRACE_GRP_FRAME_TX, "%u: forward(%s->%s, %s)", mrp_state->inst, iec_mrp_util_port_type_to_str(rx_port_type, false, true), iec_mrp_util_port_type_to_str(tx_port_type, false, true), iec_mrp_util_pdu_type_to_str(rx_pdu_info.pdu_type));
        mrp_pdu_forward(mrp_state, tx_port_type, frm, rx_info->length, rx_pdu_info.pdu_type);
    }
}

/******************************************************************************/
// mrp_base_rx_frame()
/******************************************************************************/
void mrp_base_rx_frame(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    vtss_appl_iec_mrp_statistics_t &s = mrp_state->status.port_status[port_type].statistics;
    mrp_rx_pdu_info_t              rx_pdu_info;
    uint64_t                       now = vtss::uptime_milliseconds();
    bool                           handled, own_pdu, consider_sw_forwarding;
    char                           buf[1000];

    if (!mrp_pdu_rx_frame(mrp_state, port_type, frm, rx_info, rx_pdu_info)) {
        // Frame didn't pass validation
        s.rx_error_cnt++;

        if (mrp_state->vars.syslog_cnt < 20) {
            misc_mem_print(frm, rx_info->length, buf, sizeof(buf), false, true);
            S_PORT_E(VTSS_ISID_LOCAL, rx_info->port_no, "Media-Redundancy: Received erroneous MRP PDU of %u bytes (excl. FCS)\n%s", rx_info->length, buf);
            mrp_state->vars.syslog_cnt++;
        }

        if (rx_pdu_info.pdu_type == VTSS_APPL_IEC_MRP_PDU_TYPE_UNKNOWN) {
            // We didn't recognize the PDU type
            s.rx_cnt[rx_pdu_info.pdu_type]++;
        }

        return;
    }

    rx_pdu_info.rx_time_msecs = now;

    if (rx_pdu_info.pdu_type == VTSS_APPL_IEC_MRP_PDU_TYPE_TEST || rx_pdu_info.pdu_type == VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TEST) {
        // Gotta really want to see these to see them
        T_NG(MRP_TRACE_GRP_FRAME_RX, "%u: port = %s => rx_pdu_info = %s", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type), rx_pdu_info);
    } else {
        T_DG(MRP_TRACE_GRP_FRAME_RX, "%u: port = %s => rx_pdu_info = %s", mrp_state->inst, iec_mrp_util_port_type_to_str(port_type), rx_pdu_info);
    }

    // Some statistics
    s.rx_cnt[rx_pdu_info.pdu_type]++;

    own_pdu = rx_pdu_info.sa == MRP_chassis_mac;
    if (own_pdu) {
        s.rx_own_cnt++;
    } else {
        s.rx_others_cnt++;
    }

    handled                = false;
    consider_sw_forwarding = false;
    switch (rx_pdu_info.pdu_type) {
    case VTSS_APPL_IEC_MRP_PDU_TYPE_TEST:
        handled = MRP_BASE_test_rx(mrp_state, rx_pdu_info, own_pdu);
        break;

    case VTSS_APPL_IEC_MRP_PDU_TYPE_TOPOLOGY_CHANGE:
        handled = MRP_BASE_topology_change_rx(mrp_state, rx_pdu_info, own_pdu);
        break;

    case VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_DOWN:
    case VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_UP:
        handled = MRP_BASE_link_change_rx(mrp_state, rx_pdu_info, rx_pdu_info.pdu_type == VTSS_APPL_IEC_MRP_PDU_TYPE_LINK_UP ? true : false);
        break;

    case VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_MGR_NACK:
        handled = MRP_BASE_test_mgr_nack_rx(mrp_state, port_type, rx_pdu_info, own_pdu);
        break;

    case VTSS_APPL_IEC_MRP_PDU_TYPE_OPTION_TEST_PROPAGATE:
        handled = MRP_BASE_test_propagate_rx(mrp_state, rx_pdu_info, own_pdu);
        break;

    case VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TEST:
        handled = MRP_BASE_in_test_rx(mrp_state, rx_pdu_info, own_pdu);
        break;

    case VTSS_APPL_IEC_MRP_PDU_TYPE_IN_TOPOLOGY_CHANGE:
        handled = MRP_BASE_in_topology_change_rx(mrp_state, rx_pdu_info, own_pdu);
        consider_sw_forwarding = true;
        break;

    case VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_DOWN:
    case VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_UP:
        handled = MRP_BASE_in_link_change_rx(mrp_state, port_type, rx_pdu_info, rx_pdu_info.pdu_type == VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_UP ? true : false);
        consider_sw_forwarding = true;
        break;

    case VTSS_APPL_IEC_MRP_PDU_TYPE_IN_LINK_STATUS_POLL:
        handled = MRP_BASE_in_link_status_poll_rx(mrp_state, port_type, rx_pdu_info, own_pdu);
        consider_sw_forwarding = true;
        break;

    default:
        break;
    }

    if (!handled) {
        T_IG(MRP_TRACE_GRP_FRAME_RX, "%u: MRP PDU of type %s not handled in conf.role = %s", mrp_state->inst, iec_mrp_util_pdu_type_to_str(rx_pdu_info.pdu_type), iec_mrp_util_role_to_str(mrp_state->conf.role));
        s.rx_unhandled_cnt++;
    }

    if (consider_sw_forwarding && !own_pdu) {
        // We only forward PDUs that are not sent my ourselves.
        MRP_BASE_sw_forward(mrp_state, port_type, rx_pdu_info, frm, rx_info);
    }
}

/******************************************************************************/
// mrp_base_loc_set()
// Invoked on H/W-supported chips (only) to indicate that a given port no longer
// receives MRP_Test PDUs either from ourselves or from the current MRM.
// Also invoked if we are a MIM and we no longer receive MRP_InTest PDUs on the
// I/C port.
/******************************************************************************/
void mrp_base_loc_set(mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, bool mrp_in_test_pdus)
{
    mrp_timer_t       dummy_timer;
    mesa_mrp_status_t mesa_status;

    MRP_BASE_mesa_status_get(mrp_state, mesa_status);

    T_IG(MRP_TRACE_GRP_BASE, "%u: LoC: port = %s, PDU-type = %s, sm_state = %s, in_sm_state = %s, status = %s",
         mrp_state->inst,
         iec_mrp_util_port_type_to_str(port_type), mrp_in_test_pdus ? "MRP_InTest" : "MRP_Test",
         MRP_BASE_sm_state_to_str(mrp_state->vars.sm_state),
         MRP_BASE_in_sm_state_to_str(mrp_state->vars.in_sm_state),
         mesa_status);

    if (mrp_in_test_pdus) {
        // First, ask H/W to send the next MRP_InTest frame that clears the LoC
        // to the CPU.
        MRP_BASE_mesa_itst_hitme_once(mrp_state);

        // Get the status of all three ports (ring and I/C), because we only
        // signal loss of continuity if all three ports don't get the MRP_InTest
        // PDUs. This is the way the standard works. If MRP_InTest PDUs are
        // received on all three ports, three times as many PDUs are received
        // compared to what will keep the Rx monitor alive.
        if (!mesa_status.p_status.itst_loc || !mesa_status.s_status.itst_loc || !mesa_status.i_status.itst_loc) {
            // Not all three ports are missing MRP_InTest PDUs. Ignore.
            T_IG(MRP_TRACE_GRP_BASE, "%u: Ignoring LoC since not all three ports have it", mrp_state->inst);
            return;
        }

        // Then pretend that the in_test_rx_timer has expired entirely, since
        // H/W has done the counting for us.
        if (mrp_state->vars.in_sm_state == MRP_IN_SM_STATE_CHK_IC) {
            mrp_state->vars.MRP_MIM_NReturn = mrp_state->mim_timing.test_monitoring_cnt;
        }

        MRP_BASE_in_test_timer_expired(dummy_timer, mrp_state);
    } else {
        // First, ask H/W to send the next MRP_Test frame that clears the LoC to
        // the CPU.
        MRP_BASE_mesa_tst_hitme_once(mrp_state);

        // Get the status of both ring ports, because we only signal loss of
        // continuity if both ring ports don't get the MRP_Test PDUs. This is
        // the way the standard works. If MRP_Test PDUs are received on both
        // ring ports, twice as many PDUs are received compared to what will
        // keep the Rx monitor alive.
        if (!mesa_status.p_status.tst_loc || !mesa_status.s_status.tst_loc) {
            // Not both ports are missing MRP_Test PDUs. Ignore.
            T_IG(MRP_TRACE_GRP_BASE, "%u: Ignoring LoC, since not both ring ports have it", mrp_state->inst);
            return;
        }

        // Then pretend that the test_tx_timer has expired entirely, since H/W
        // has done the counting for us.
        if (mrp_state->vars.sm_state == MRP_SM_STATE_CHK_RC) {
            // So rather than incrementing MRP_MRM_NReturn in small steps, take
            // it directly to the limit for the sm_state that would otherwise
            // just increment it by one.
            mrp_state->vars.MRP_MRM_NReturn = mrp_state->vars.MRP_MRM_NRmax;
        } else if (mrp_state->vars.sm_state == MRP_SM_STATE_DE_IDLE ||
                   mrp_state->vars.sm_state == MRP_SM_STATE_PT      ||
                   mrp_state->vars.sm_state == MRP_SM_STATE_DE      ||
                   mrp_state->vars.sm_state == MRP_SM_STATE_PT_IDLE) {
            // So rather than incrementing MRP_MonNReturn in small steps, take
            // it directly to the limit for the sm_state that would otherwise
            // just increment it by one. Notice, that the code inside
            // MRP_BASE_test_timer_expired checks for MRP_MonNReturn ">=" rather
            // than just ">", so we must increment it past the max.
            mrp_state->vars.MRP_MonNReturn = mrp_state->mrm_timing.test_monitoring_cnt + 1;
        }

        MRP_BASE_test_timer_expired(dummy_timer, mrp_state);
    }
}

/******************************************************************************/
// mrp_base_history_dump()
/******************************************************************************/
void mrp_base_history_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    vtss_appl_iec_mrp_in_role_t   in_role;
    vtss_appl_iec_mrp_in_mode_t   in_mode;
    vtss_appl_iec_mrp_port_type_t port_type;
    mrp_itr_t                     itr;
    mrp_state_t                   *mrp_state;
    mrp_base_history_t            history;
    uint32_t                      inst = 0;
    mrp_base_history_itr_t        hist_itr;
    uint32_t                      cnt;
    bool                          first = true;
    char                          bufs[3][10];

    // Gotta do this odd construct, because we cannot take the MRP mutex for too
    // long due to the protocol's timing sensitivity.
    while (1) {
        {
            MRP_LOCK_SCOPE();

            if ((itr = MRP_map.greater_than(inst)) == MRP_map.end()) {
                return;
            }

            // Take a snapshot of the parameters before leaving scope.
            mrp_state = &itr->second;
            inst      = mrp_state->inst;
            in_role   = mrp_state->conf.in_role;
            in_mode   = mrp_state->conf.in_mode;
            history   = mrp_state->history;
        }

        if (first) {
            pr(session_id, "Now = " VPRI64u " ms\n", vtss::uptime_milliseconds());
            pr(session_id, "Inst   # Time [ms]      Role SM State Ring State InRole InSM State InRing State Prm   Port1  Port2  I/C    Changed by\n");
            pr(session_id, "---- --- -------------- ---- -------- ---------- ------ ---------- ------------ ----- ------ ------ ------ -------------------\n");
            first = false;
        }

        cnt = 1;
        for (hist_itr = history.begin(); hist_itr != history.end(); ++hist_itr) {
            for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
                if (port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION && in_role == VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
                    strcpy(bufs[port_type], "--/---");
                } else {
                    sprintf(bufs[port_type], "%s/%s", hist_itr->sf[port_type] ? "Dn" : "Up", hist_itr->forwarding[port_type] ? "Fwd" : "Blk");
                }
            }

            pr(session_id, "%4u %3u " VPRI64Fu("14") " %-4s %-8s %-10s %-6s %-10s %-12s %-5s %-6s %-6s %-6s %s\n",
               inst,
               cnt++,
               hist_itr->event_time_ms,
               iec_mrp_util_role_to_str(hist_itr->oper_role, true /* Capital letters */),
               MRP_BASE_sm_state_to_str(hist_itr->sm_state),
               iec_mrp_util_ring_state_to_str(hist_itr->ring_state),
               iec_mrp_util_in_role_and_mode_to_str(in_role, in_mode),
               MRP_BASE_in_sm_state_to_str(hist_itr->in_sm_state),
               iec_mrp_util_ring_state_to_str(hist_itr->in_ring_state),
               iec_mrp_util_port_type_to_str( hist_itr->prm_ring_port, true /* capital first letter */),
               bufs[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1],
               bufs[VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2],
               bufs[VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION],
               hist_itr->who);
        }

        if (cnt == 1) {
            pr(session_id, "%4u <No events registered yet>\n", inst);
        }
    }
}

/******************************************************************************/
// mrp_base_history_clear()
/******************************************************************************/
void mrp_base_history_clear(void)
{
    mrp_itr_t itr;

    MRP_LOCK_SCOPE();

    // Despite MRP's timing sensitivity, we clear all instances in one go.
    for (itr = MRP_map.begin(); itr != MRP_map.end(); ++itr) {
        itr->second.history.clear();
    }
}

/******************************************************************************/
// mrp_base_rules_dump()
// Dumps active rules.
/******************************************************************************/
void mrp_base_rules_dump(uint32_t session_id, int32_t (*pr)(uint32_t session_id, const char *fmt, ...))
{
    mrp_itr_t           itr;
    mrp_state_t         *mrp_state;
    uint32_t            inst = 0;
    mesa_ace_id_t       ace_ids[ARRSZ(mrp_state_t::ace_conf)][ARRSZ(mrp_state_t::ace_conf[0])];
    acl_entry_conf_t    ace_conf;
    mesa_ace_counter_t  ace_counter;
    mesa_ace_id_t       ace_id;
    char                ingr_buf[MGMT_PORT_BUF_SIZE], egr_buf[MGMT_PORT_BUF_SIZE];
    mesa_port_list_t    blocked_plist;
    mesa_port_forward_t forward;
    mesa_port_no_t      port_no;
    int                 i, j;
    bool                first = true;
    mesa_rc             rc;

    // Gotta do this odd construct, because we cannot take the MRP mutex for too
    // long due to the protocol's timing sensitivity.
    while (1) {
        {
            MRP_LOCK_SCOPE();

            if ((itr = MRP_map.greater_than(inst)) == MRP_map.end()) {
                return;
            }

            // Take a snapshot of the parameters before leaving scope.
            mrp_state = &itr->second;
            inst      = mrp_state->inst;
            for (i = 0; i < ARRSZ(ace_ids); i++) {
                for (j = 0; j < ARRSZ(ace_ids[i]); j++) {
                    ace_ids[i][j] = mrp_state->ace_conf[i][j].id;
                }
            }
        }

        if (first) {
            // Start by printing blocked ports
            blocked_plist.clear_all();
            for (port_no = 0; port_no < MRP_cap_port_cnt; port_no++) {
                if ((rc =  mesa_port_forward_state_get(nullptr, port_no, &forward)) != VTSS_RC_OK) {
                    T_EG(MRP_TRACE_GRP_BASE, "mesa_port_forward_state_get(%u) failed: %s", port_no, error_txt(rc));
                    forward = MESA_PORT_FORWARD_ENABLED;
                }

                blocked_plist[port_no] = forward != MESA_PORT_FORWARD_ENABLED;
            }

            pr(session_id, "Blocked ports: %s\n\n", mgmt_iport_list2txt(blocked_plist, ingr_buf));
            pr(session_id, "Inst ACE ID DMAC Type    Rule Ingress Ports     Egress Ports      CPU Copy Hit Count\n");
            pr(session_id, "---- ------ ------------ ---- ----------------- ----------------- -------- --------------\n");
        }

        for (i = 0; i < ARRSZ(ace_ids); i++) {
            for (j = 0; j < ARRSZ(ace_ids[i]); j++) {
                ace_id = ace_ids[i][j];

                if (ace_id == ACL_MGMT_ACE_ID_NONE) {
                    continue;
                }

                if ((rc = acl_mgmt_ace_get(ACL_USER_IEC_MRP, ace_id, &ace_conf, &ace_counter, FALSE)) != VTSS_RC_OK) {
                    T_EG(MRP_TRACE_GRP_ACL, "%u: acl_mgmt_ace_get(%u) failed: %s", inst, ace_id, error_txt(rc));
                    return;
                }

                pr(session_id, "%4u %6d %-12s %-4s %17s %17s %-8s %14u\n",
                   inst,
                   ace_conf.id,
                   MRP_BASE_pdu_dmac_to_str((mrp_pdu_dmac_type_t)(i + MRP_PDU_DMAC_TYPE_MC_TEST)),
                   j == 0 ? "1ST" : "2ND",
                   mgmt_iport_list2txt(ace_conf.port_list, ingr_buf),
                   mgmt_iport_list2txt(ace_conf.action.port_list, egr_buf),
                   ace_conf.action.force_cpu ? "Yes" : "No",
                   ace_counter);
            }
        }
    }
}

/******************************************************************************/
// mrp_base_state_dump()
// Dumps internal state.
/******************************************************************************/
void mrp_base_state_dump(uint32_t session_id, int32_t (*pr)(uint32_t session_id, const char *fmt, ...))
{
    mrp_itr_t                     itr;
    mrp_state_t                   *mrp_state;
    uint32_t                      inst = 0;
    mrp_vars_t                    vars;
    vtss_appl_iec_mrp_role_t      conf_role, oper_role;
    vtss_appl_iec_mrp_in_role_t   in_role;
    vtss_appl_iec_mrp_port_type_t port_type;
    mesa_mrp_status_t             mesa_status = {};
    vtss::StringStream            s;
    mesa_mrp_port_status_t        *mesa_port_status;
    const char                    *str;
    char                          buf[100];
    bool                          first = true;
    const int                     width = 24;

    // Gotta do this odd construct, because we cannot take the MRP mutex for too
    // long due to the protocol's timing sensitivity.
    while (1) {
        {
            MRP_LOCK_SCOPE();

            if ((itr = MRP_map.greater_than(inst)) == MRP_map.end()) {
                return;
            }

            // Take a snapshot of the parameters before leaving scope.
            mrp_state = &itr->second;
            inst      = mrp_state->inst;
            vars      = mrp_state->vars;
            conf_role = mrp_state->conf.role;
            oper_role = mrp_state->status.oper_role;
            in_role   = mrp_state->conf.in_role;

            if (MRP_hw_support) {
                MRP_BASE_mesa_status_get(mrp_state, mesa_status);
            }
        }

        if (first) {
            first = false;
        } else {
            pr(session_id, "\n--------------------------------------------------------\n");
        }

        pr(session_id, "%-*s %u\n",     width, "Instance:",                inst);
        pr(session_id, "%-*s %s\n",     width, "Own MAC:",                 misc_mac_txt(MRP_chassis_mac.addr, buf));
        pr(session_id, "%-*s %s\n",     width, "Conf. Role:",              iec_mrp_util_role_to_str(conf_role, true));
        pr(session_id, "%-*s %s\n",     width, "Oper. Role:",              iec_mrp_util_role_to_str(oper_role, true));
        pr(session_id, "%-*s %s\n",     width, "Primary Ring port:",       iec_mrp_util_port_type_to_str(vars.prm_ring_port, true));
        pr(session_id, "%-*s %s\n",     width, "SM State:",                MRP_BASE_sm_state_to_str(vars.sm_state));
        pr(session_id, "%-*s %d\n",     width, "MRP_Test CPU copy:",       vars.mesa_copy_conf.tst_to_cpu);
        pr(session_id, "%-*s %d\n",     width, "MRP_InTest CPU copy:",     vars.mesa_copy_conf.itst_to_cpu);
        pr(session_id, "%-*s %d\n",     width, "test_rx_timer_active:",    vars.test_rx_timer_active);
        pr(session_id, "%-*s %d\n",     width, "in_test_rx_timer_active:", vars.in_test_rx_timer_active);
        pr(session_id, "%-*s %u\n",     width, "LNKNReturn:",              vars.MRP_LNKNReturn);
        pr(session_id, "%-*s %u\n",     width, "MRM_NRmax:",               vars.MRP_MRM_NRmax);
        pr(session_id, "%-*s %u\n",     width, "MRM_NReturn:",             vars.MRP_MRM_NReturn);
        pr(session_id, "%-*s %u\n",     width, "TC_NReturn:",              vars.TC_NReturn);
        pr(session_id, "%-*s %s\n",     width, "HO_BestMRM_SA",            misc_mac_txt(vars.HO_BestMRM_SA.addr, buf));
        pr(session_id, "%-*s 0x%04x\n", width, "HO_BestMRM_Prio",          vars.HO_BestMRM_Prio);
        pr(session_id, "%-*s %u\n",     width, "add_test:",                vars.add_test);
        pr(session_id, "%-*s %u\n",     width, "NO_TC:",                   vars.NO_TC);
        pr(session_id, "%-*s %s\n",     width, "MRP_BestMRM_SA",           misc_mac_txt(vars.MRP_BestMRM_SA.addr, buf));
        pr(session_id, "%-*s 0x%04x\n", width, "MRP_BestMRM_Prio",         vars.MRP_BestMRM_Prio);
        pr(session_id, "%-*s %s\n",     width, "In Role",                  iec_mrp_util_in_role_to_str(in_role));
        pr(session_id, "%-*s %s\n",     width, "In SM State",              MRP_BASE_in_sm_state_to_str(vars.in_sm_state));
        pr(session_id, "%-*s %u\n",     width, "MON_NReturn:",             vars.MRP_MonNReturn);
        pr(session_id, "%-*s %u\n",     width, "MIM_NRmax:",               vars.MRP_MIM_NRmax);
        pr(session_id, "%-*s %u\n",     width, "MIM_NReturn:",             vars.MRP_MIM_NReturn);
        pr(session_id, "%-*s %u\n",     width, "IN_TC_NReturn:",           vars.IN_TC_NReturn);
        pr(session_id, "%-*s %u\n",     width, "IN_LNKSTAT_NReturn:",      vars.MRP_IN_LNKSTAT_NReturn);
        pr(session_id, "%-*s %u\n",     width, "InLNKNReturn:",            vars.MRP_InLNKNReturn);
        pr(session_id, "%-*s %u\n",     width, "Syslog entries:",          vars.syslog_cnt);

        if (MRP_hw_support) {
            for (port_type = VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1; port_type <= VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION; port_type++) {
                mesa_port_status = port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? &mesa_status.p_status : port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? &mesa_status.s_status    : &mesa_status.i_status;
                str              = port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1 ? "p_status:"           : port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2 ? "s_status:"              : "i_status:";

                s.clear();
                s << *mesa_port_status;
                pr(session_id, "%-*s %s\n", width, str, s.cstring());
            }
        }

    }
}

