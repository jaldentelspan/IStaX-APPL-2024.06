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

#include "vtss_xxrp_types.hxx"
#include "vtss_mrp.hxx"
#include "subject.hxx"
#include "vtss/basics/synchronized.hxx"

void vtss_mrp_crit_assert();

vtss_appl_mrp_appl_t &operator++(vtss_appl_mrp_appl_t &prev) {
    if (prev == VTSS_APPL_MRP_APPL_LAST) {
        return prev;
    }
    prev = static_cast<vtss_appl_mrp_appl_t>(prev + 1);
    return prev;
}

namespace vtss {
namespace mrp {

/* The MVRP application object */
/* Implements the entire MVRP protocol */
SynchronizedSubjectRunner<MrpAppl> mvrp(
        &vtss::notifications::subject_locked_thread,
        vtss::notifications::subject_locked_thread);

mesa_rc global_state_get(BOOL &state) {
    SYNCHRONIZED(mvrp) { state = mvrp.global_state(); }
    return VTSS_RC_OK;
}

mesa_rc global_state_set(const BOOL &state) {
    SYNCHRONIZED(mvrp) {
        // To be done better in the future, when the platform is refactored
        // Disable any ports running MRP first, so that MADs get destructed
        // before the MAP
        if (state == FALSE) {
            for (int port = 0; port < L2_MAX_PORTS_; ++port) {
                if (mvrp.port_state(port)) {
                    mvrp.port_state(port, false);
                }
            }
        }
        return mvrp.global_state((bool)state);
    }
    return VTSS_RC_ERROR;
}

mesa_rc port_state_get(u32 port_no, BOOL &state) {
    SYNCHRONIZED(mvrp) {
        state = mvrp.port_state(port_no);
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc port_state_set(u32 port_no, const BOOL &state) {
    SYNCHRONIZED(mvrp) { return mvrp.port_state(port_no, (bool)state); }
    return VTSS_RC_ERROR;
}

mesa_rc port_timers_set(u32 port_no, const MrpTimeouts &t) {
    SYNCHRONIZED(mvrp) { return mvrp.port_timers(port_no, t); }
    return VTSS_RC_ERROR;
}

mesa_rc mgmt_periodic_state_get(vtss_isid_t isid, mesa_port_no_t iport,
                                bool &state) {
    u32 port_no = L2PORT2PORT(isid, iport);
    SYNCHRONIZED(mvrp) {
        state = mvrp.periodic_state(port_no);
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc mgmt_periodic_state_set(vtss_isid_t isid, mesa_port_no_t iport,
                                const bool &state) {
    u32 port_no = L2PORT2PORT(isid, iport);
    SYNCHRONIZED(mvrp) { return mvrp.periodic_state(port_no, state); }
    return VTSS_RC_ERROR;
}

mesa_rc vlan_list_set(const VlanList &vls) {
    SYNCHRONIZED(mvrp) {
        // Make a diff check first
        const VlanList &v = mvrp.vlan_list();
        for (int i = XXRP_VLAN_ID_MIN; i < XXRP_VLAN_ID_MAX; ++i) {
            if (v.get(i) != vls.get(i)) {
                return mvrp.vlan_list(vls);
            }
        }
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

const char *mgmt_appl_to_txt(vtss_appl_mrp_appl_t appl) {
    switch (appl) {
    case VTSS_APPL_MRP_APPL_MVRP:
        return "MVRP";
    default:
        return "Unknown";
    }
}

mesa_rc mgmt_stat_mvrp_get(MrpApplStat &stat) {
    SYNCHRONIZED(mvrp) {
        stat.global_state = mvrp.global_state();
        if (!stat.global_state) {
            // The rest is not relevant
            return VTSS_RC_OK;
        }
        for (int port = 0; port < L2_MAX_PORTS_; ++port) {
            stat.port_state[port] = mvrp.port_state(port);
        }
        stat.vlan_list = mvrp.vlan_list();
        stat.vlan2index = mvrp.vlan2index();
        for (int msti = 0; msti < MRP_MSTI_MAX; ++msti) {
            for (auto it = mvrp.map->ring[msti].cbegin();
                 it != mvrp.map->ring[msti].cend(); ++it) {
                stat.ring[msti].emplace_back(*it);
            }
        }
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc mgmt_debug_mvrp_get(vtss_isid_t isid, mesa_port_no_t iport,
                            MrpApplPortDebug &stat) {
    u32 port_no = L2PORT2PORT(isid, iport);
    SYNCHRONIZED(mvrp) {
        const Vector<MrpMadState> &d = mvrp.mad[port_no]->regApp.fetch_data();
        for (auto it = d.cbegin(); it != d.cend(); ++it) {
            stat.states.emplace_back(*it);
        }
        stat.leaveAllState = mvrp.mad[port_no]->leaveall_state;
        stat.periodicState = mvrp.mad[port_no]->periodic_state;
        if (mvrp.mad[port_no]->joinTimerFlag) {
            stat.join =
                    mvrp.mad[port_no]->remaining(&mvrp.mad[port_no]->joinTimer);
        } else {
            stat.join = (milliseconds)0;
        }
        if (mvrp.mad[port_no]->leaveTimerFlag) {
            stat.leave =
                    mvrp.mad[port_no]->remaining(&mvrp.mad[port_no]->leaveTimer);
        } else {
            stat.leave = (milliseconds)0;
        }
        if (mvrp.mad[port_no]->leaveAllTimerFlag) {
            stat.leaveAll = mvrp.mad[port_no]->remaining(
                    &mvrp.mad[port_no]->leaveAllTimer);
        } else {
            stat.leaveAll = (milliseconds)0;
        }
        if (mvrp.mad[port_no]->periodicTimerFlag) {
            stat.periodic = mvrp.mad[port_no]->remaining(
                    &mvrp.mad[port_no]->periodicTimer);
        } else {
            stat.periodic = (milliseconds)0;
        }
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

mesa_rc mgmt_stat_port_get(vtss_isid_t isid, mesa_port_no_t iport,
                           vtss_appl_mrp_appl_t appl, MrpApplPortStat &stat) {
    switch (appl) {
    case VTSS_APPL_MRP_APPL_MVRP:
#ifdef VTSS_SW_OPTION_MVRP
    {
        u32 port_no = L2PORT2PORT(isid, iport);
        SYNCHRONIZED(mvrp) {
            stat.failedRegistrations = 0;
            memset(stat.lastPduOrigin, 0, sizeof(stat.lastPduOrigin));
            if (mvrp.port_state(port_no)) {
                stat.failedRegistrations = 0;
                memcpy(stat.lastPduOrigin, mvrp.mad[port_no]->peer_mac_address,
                       sizeof(stat.lastPduOrigin));
            }
            return VTSS_RC_OK;
        }
    }
#else
        return VTSS_RC_ERROR;
#endif
    case VTSS_APPL_MRP_APPL_LAST:
    default:
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_ERROR;
}

mesa_rc mvrp_handle_vlan_change(u32 port_no, mesa_vid_t v,
                                vlan_registration_type_t t) {
    SYNCHRONIZED(mvrp) {
        /* Only process the change if MVRP is enabled globally and on the port
         */
        if (!mvrp.global_state()) {
            return VTSS_RC_OK;
        }
        if (!mvrp.port_state(port_no)) {
            return VTSS_RC_OK;
        }

        return mvrp.handle_vlan_change(port_no, v, t);
    }
    return VTSS_RC_ERROR;
}

mesa_rc mvrp_receive_frame(u32 port_no, const u8 *pdu, size_t length) {
    SYNCHRONIZED(mvrp) {
        if (!mvrp.global_state()) {
            return VTSS_RC_OK;
        }
        if (!mvrp.port_state(port_no)) {
            return VTSS_RC_OK;
        }

        return mvrp.receive_frame(port_no, pdu, length);
    }
    return VTSS_RC_ERROR;
}

mesa_rc mvrp_handle_port_state_change(u32 port_no, vtss_appl_mstp_msti_t msti,
                                      port_states state) {
    SYNCHRONIZED(mvrp) {
        return mvrp.handle_port_state_change(port_no, msti, state);
    }
    return VTSS_RC_ERROR;
}

void handle_port_role_change(u32 port_no, vtss_appl_mstp_msti_t msti,
                             port_roles role) {
    SYNCHRONIZED(mvrp) {
        return mvrp.handle_port_role_change(port_no, msti, role);
    }
}

}  // namespace mrp
}  // namespace vtss
