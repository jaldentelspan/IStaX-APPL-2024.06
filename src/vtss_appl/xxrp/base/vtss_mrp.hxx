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

/**
 * This enum identifies MRP applications.
 *
 * A MRP application is a protocol that is utilizing the MRP framework
 * and with the purpose of a dynamic attribute propagation and registration
 * in a given network. E.g. MVRP is such an application with the task of
 * managing the dynamic VLAN registrations across MVRP bridges in the network.
 */
typedef enum {
	VTSS_APPL_MRP_APPL_MVRP, /**< MVRP */

	// For the sake of persistent layout of SNMP OIDs,
	// new MRP applications must come just before this line.
	VTSS_APPL_MRP_APPL_LAST /**< May be needed for iterations across this enum. Must come last */
} vtss_appl_mrp_appl_t;

vtss_appl_mrp_appl_t &operator++(vtss_appl_mrp_appl_t &prev);

namespace vtss {
namespace mrp {

struct MrpApplStat {
    bool global_state = false;
    Array<bool, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_state = {};
    VlanList vlan_list = {};
    Array<uint16_t, 4096> vlan2index = {};
    Array<Vector<u32>, MRP_MSTI_MAX> ring;
};

struct MrpApplPortDebug {
	milliseconds join;
	milliseconds leave;
	milliseconds leaveAll;
	milliseconds periodic;
	Vector<const MrpMadState> states;
	bool leaveAllState;
	bool periodicState;
};

struct MrpApplPortStat {
	u64 failedRegistrations;
	u8  lastPduOrigin[VTSS_MRP_PEER_MAC_ADDRESS_SIZE];
};

mesa_rc global_state_get(BOOL &state);
mesa_rc global_state_set(const BOOL &state);

mesa_rc port_state_get(u32 port_no, BOOL &state);
mesa_rc port_state_set(u32 port_no, const BOOL &state);

mesa_rc port_timers_set(u32 port_no, const MrpTimeouts &t);

mesa_rc mgmt_periodic_state_get(vtss_isid_t isid, mesa_port_no_t iport, bool &state);
mesa_rc mgmt_periodic_state_set(vtss_isid_t isid, mesa_port_no_t iport, const bool &state);

mesa_rc vlan_list_set(const VlanList &vls);

const char *mgmt_appl_to_txt(vtss_appl_mrp_appl_t appl);
mesa_rc mgmt_stat_mvrp_get(MrpApplStat &stat);
mesa_rc mgmt_debug_mvrp_get(vtss_isid_t isid, mesa_port_no_t iport,
                            MrpApplPortDebug &stat);
mesa_rc mgmt_stat_port_get(vtss_isid_t isid, mesa_port_no_t iport,
                           vtss_appl_mrp_appl_t appl,
                           MrpApplPortStat &stat);

mesa_rc mvrp_handle_vlan_change(u32 port_no, mesa_vid_t v, vlan_registration_type_t t);

mesa_rc mvrp_receive_frame(u32 port_no, const u8 *pdu, size_t length);

mesa_rc mvrp_handle_port_state_change(u32 port_no, vtss_appl_mstp_msti_t msti,
                                      port_states state);

void handle_port_role_change(u32 port_no, vtss_appl_mstp_msti_t msti,
	                             port_roles role);
}
}
