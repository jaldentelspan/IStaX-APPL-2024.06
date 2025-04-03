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

#ifndef __XXRP_BASE_VTSS_XXRP_TYPES_HXX__
#define __XXRP_BASE_VTSS_XXRP_TYPES_HXX__

#include <vtss/basics/common.hxx>
#include <vtss/basics/vector.hxx>
#include <vtss/basics/array.hxx>
#include <vtss/basics/utility.hxx>
#include <vtss/appl/types.hxx>
#include "vtss/appl/mstp.h"
#include "mstp_api.h"
#include "vlan_api.h"
#include "l2proto_api.h"
#include "vtss/basics/notifications.hxx"
#include <stdarg.h>

#if !defined(XXRP_ATTRIBUTE_PACKED)
#define XXRP_ATTRIBUTE_PACKED __attribute__((packed)) /* GCC defined */
#endif

#define XXRP_VLAN_ID_MIN 1
#define XXRP_VLAN_ID_MAX 4095
#define MRP_MSTI_MAX VTSS_APPL_MSTP_MAX_MSTI
#define VTSS_MRP_PEER_MAC_ADDRESS_SIZE 6
#define VTSS_XXRP_MAC_ADDR_LEN                 6
#define VTSS_XXRP_ETH_TYPE_LEN                 2

#define VTSS_MVRP_LA_AND_NUM_OF_VALS_FLD_SIZE 2
#define VTSS_MVRP_ATTR_LEN_VLAN               2
#define VTSS_MVRP_MAX_VECTORS                 ((VLAN_ENTRY_CNT+2) / 3)

#define MRP_EVENT_NEW     0     /* New event      */
#define MRP_EVENT_JOININ  1     /* JoinIn event   */
#define MRP_EVENT_IN      2     /* In event       */
#define MRP_EVENT_JOINMT  3     /* JoinMt event   */
#define MRP_EVENT_MT      4     /* Mt event       */
#define MRP_EVENT_LV      5     /* Leave event    */
#define MRP_EVENT_INVALID 6     /* Invalid event  */
#define MRP_EVENT_LA      0xFF  /* LeaveAll event */

#define MRP_SET_EVENT(arr, indx, val)   ((indx % 2) ? (arr[indx/2] = ((arr[indx/2] & 0x0F) | (val << 4))) \
                                                          : (arr[indx/2] = ((arr[indx/2] & 0xF0) | val)))
#define MRP_GET_EVENT(arr, indx)        ((indx % 2) ? (((arr[indx/2]) >> 4) & 0xF) : ((arr[indx/2] & 0xF)))
#define MRP_EVENT_INVALID_BYTE ((MRP_EVENT_INVALID << 4) | MRP_EVENT_INVALID)

void vtss_mrp_crit_enter();
void vtss_mrp_crit_exit();

namespace vtss {
namespace mrp {

struct LockRef {
    void lock() { vtss_mrp_crit_enter(); }
    void unlock() { vtss_mrp_crit_exit(); }
};

typedef notifications::SubjectRunner TimerService;
typedef LockRef Lock;

template <typename L>
struct ScopeLock {
    ScopeLock(L &l) : l_(l) { l_.lock(); }
    ~ScopeLock() { l_.unlock(); }

private:
    L &l_;
};

struct MrpTimeouts {
    milliseconds join = (milliseconds) 200;
    milliseconds leave = (milliseconds) 600;
    milliseconds leaveAll = (milliseconds) 10000;
};

enum port_states { MRP_FORWARDING, MRP_DISCARDING };
enum port_roles { MRP_DESIGNATED, MRP_ROOT_ALTERNATE };

/* MVRP vector attribute */
typedef struct {
    u8 la_and_num_of_vals[VTSS_MVRP_LA_AND_NUM_OF_VALS_FLD_SIZE];
    u8 first_value[VTSS_MVRP_ATTR_LEN_VLAN];
    u8 vectors[VTSS_MVRP_MAX_VECTORS];
} XXRP_ATTRIBUTE_PACKED mvrp_pdu_vector_attr_t;

enum xxrp_registrar_states { XXRP_IN, XXRP_LV, XXRP_MT, XXRP_REG_ST_CNT };

enum xxrp_registrar_admin { XXRP_NORMAL, XXRP_FIXED, XXRP_FORBIDDEN };

enum xxrp_applicant_states {
    XXRP_VO,
    XXRP_VP,
    XXRP_VN,
    XXRP_AN,
    XXRP_AA,
    XXRP_QA,
    XXRP_LA,
    XXRP_AO,
    XXRP_QO,
    XXRP_AP,
    XXRP_QP,
    XXRP_LO,
    XXRP_APP_ST_CNT
};

enum xxrp_bool_states { XXRP_PASSIVE, XXRP_ACTIVE, XXRP_BOOL_ST_CNT };

enum xxrp_registrar_events {
    XXRP_EV_REG_BEGIN,
    XXRP_EV_REG_RNEW,
    XXRP_EV_REG_RJOININ,
    XXRP_EV_REG_RJOINMT,
    XXRP_EV_REG_RLEAVE,
    XXRP_EV_REG_RLEAVEALL,
    XXRP_EV_REG_TX_LEAVEALL,
    XXRP_EV_REG_REDECLARE,
    XXRP_EV_REG_FLUSH,
    XXRP_EV_REG_TIMER,
    XXRP_EV_REG_CNT
};

ostream &operator<<(ostream &o, xxrp_registrar_events &event);

enum xxrp_applicant_events {
    XXRP_EV_APP_BEGIN,
    XXRP_EV_APP_NEW,
    XXRP_EV_APP_JOIN,
    XXRP_EV_APP_LEAVE,
    XXRP_EV_APP_RNEW,
    XXRP_EV_APP_RJOININ,
    XXRP_EV_APP_RIN,
    XXRP_EV_APP_RJOINMT,
    XXRP_EV_APP_RMT,
    XXRP_EV_APP_RLEAVE,
    XXRP_EV_APP_RLEAVEALL,
    XXRP_EV_APP_REDECLARE,
    XXRP_EV_APP_PERIODIC,
    XXRP_EV_APP_TX,
    XXRP_EV_APP_TXLA,
    XXRP_EV_APP_TXLAF,
    XXRP_EV_APP_CNT
};

ostream &operator<<(ostream &o, xxrp_applicant_events &event);

enum xxrp_leaveall_events {
    XXRP_EV_LA_BEGIN,
    XXRP_EV_LA_TX,
    XXRP_EV_LA_RX,
    XXRP_EV_LA_TIMER,
    XXRP_EV_LA_CNT
};

enum xxrp_periodic_events {
    XXRP_EV_PER_BEGIN,
    XXRP_EV_PER_ENABLED,
    XXRP_EV_PER_DISABLED,
    XXRP_EV_PER_TIMER,
    XXRP_EV_PER_CNT
};

enum xxrp_registrar_cbs {
    XXRP_CB_REG_DO_NOTHING,
    XXRP_CB_REG_NEW,
    XXRP_CB_REG_JOIN,
    XXRP_CB_REG_LEAVE,
    XXRP_CB_REG_START_TIMER,
    XXRP_CB_REG_STOP_TIMER,
    XXRP_CB_REG_STOP_TIMER_NEW
};

enum xxrp_applicant_cbs {
    XXRP_CB_APP_DO_NOTHING,
    XXRP_CB_APP_SEND_NEW,
    XXRP_CB_APP_SEND_JOIN,
    XXRP_CB_APP_SEND,
    XXRP_CB_APP_SEND_LEAVE,
    XXRP_CB_APP_SEND_INVALID
};

enum xxrp_leaveall_cbs {
    XXRP_CB_LA_DO_NOTHING,
    XXRP_CB_LA_START_TIMER,
    XXRP_CB_LA_SLA
};

enum xxrp_periodic_cbs {
    XXRP_CB_PER_DO_NOTHING,
    XXRP_CB_PER_START,
    XXRP_CB_PER_START_TRIGGER
};

struct MrpRegStm {
    xxrp_registrar_states state;
    xxrp_registrar_cbs cb_enum;
};

struct MrpAppStm {
    xxrp_applicant_states state;
    xxrp_applicant_cbs cb_enum;
};

struct MrpLeaveAllStm {
    xxrp_bool_states state;
    xxrp_leaveall_cbs cb_enum;
};

struct MrpPeriodicStm {
    xxrp_bool_states state;
    xxrp_periodic_cbs cb_enum;
};

// One instance per (port, vlan) - instantiated in MrpMadMachines
struct MrpMadState {
    MrpMadState(xxrp_registrar_states r, xxrp_applicant_states a,
                xxrp_registrar_admin admin) {
        data = 0;
        registrar(r);
        applicant(a);
        registrar_admin(admin);
    }

    xxrp_applicant_states applicant() const;
    void applicant(xxrp_applicant_states s);

    xxrp_registrar_states registrar() const;
    void registrar(xxrp_registrar_states s);

    xxrp_registrar_admin registrar_admin() const;
    void registrar_admin(xxrp_registrar_admin s);

    u8 data;
};

struct MrpMad;

// One instance per port! Each instance contains state-machines for every VLAN
// which has been MVRP-enabled. The number of MrpMadMachines is static and can
// not change! (the instances can be enabled/disabled)
//
// This class is owned by MrpAppl
struct MrpMadMachines {
    /* Constructor */
    MrpMadMachines(MrpMad &mad);

    MrpMad &parent_;

    // Get/set applicant state for a given VLAN - will assert if called for a
    // non-MVRP enabled VLAN!
    xxrp_applicant_states applicant(mesa_vid_t v) const {
        return data[vlan2index(v)].applicant();
    }
    void applicant(mesa_vid_t v, xxrp_applicant_states s) {
        data[vlan2index(v)].applicant(s);
    }

    // Get/set registrar state for a given VLAN - will assert if called for a
    // non-MVRP enabled VLAN!
    xxrp_registrar_states registrar(mesa_vid_t v) const {
        return data[vlan2index(v)].registrar();
    }
    void registrar(mesa_vid_t v, xxrp_registrar_states s) {
        data[vlan2index(v)].registrar(s);
    }

    // Get/set registrar administrative status for a given VLAN - will assert if called for a
    // non-MVRP enabled VLAN!
    xxrp_registrar_admin registrar_admin(mesa_vid_t v) const {
        return data[vlan2index(v)].registrar_admin();
    }
    void registrar_admin(mesa_vid_t v, xxrp_registrar_admin s) {
        data[vlan2index(v)].registrar_admin(s);
    }

    // private helper method
    mesa_rc registrar_do_callback(xxrp_registrar_cbs cb_enum, u32 port_no, mesa_vid_t v);
    mesa_rc applicant_do_callback(xxrp_applicant_cbs cb_enum);
    mesa_rc applicant_do_callback(xxrp_applicant_cbs cb_enum, mesa_vid_t v, u8 *all_events,
                                  u32 &total_events);

    template <typename F>
    void for_all_machines(F f);

    template <typename F>
    void for_all_machines_in_msti(F f, vtss_appl_mstp_msti_t msti);

    template <typename F>
    void for_all_machines_forwarding(F f);

    /* Called when an event needs to be triggered */
    /* Message reception or MAD.request */
    mesa_rc handle_event_reg(xxrp_registrar_events event, u32 port_no, mesa_vid_t v);
    mesa_rc handle_event_app(xxrp_applicant_events event, mesa_vid_t v);
    mesa_rc handle_event_app(xxrp_applicant_events event, mesa_vid_t v, u8 * all_events,
                             u32 & total_events);

    mesa_rc join_indication(u32 port_no, mesa_vid_t v);

    mesa_rc leave_indication(u32 port_no, mesa_vid_t v);

    bool not_declaring(mesa_vid_t v);

    Vector<MrpMadState> &fetch_data() { return data; }
private:
    bool requires_tx(xxrp_applicant_states state);

    // State machine transitions as specified in 802.1Q-2014
    static Pair<xxrp_registrar_states, xxrp_registrar_cbs>
    registrar_next_state(xxrp_registrar_states state,
                         xxrp_registrar_events event);

    // State machine transitions as specified in 802.1Q-2014
    static Pair<xxrp_applicant_states, xxrp_applicant_cbs>
    applicant_next_state(xxrp_applicant_states state,
                         xxrp_applicant_events event);

    size_t vlan2index(mesa_vid_t v) const;
    Vector<MrpMadState> data;  // stores only the managed VLANs
};

struct MrpAppl;

// one per port!
struct MrpMad : public notifications::EventHandler {
    // LOCKING: This class expects that the lock reference provided in the
    // constructor is already locked, before a member function is called. With
    // exception of the execute(Timer *t) function.

    // Constructor
    MrpMad(TimerService &ts, u32 p, MrpAppl &parent);

    // Destructor
    ~MrpMad();

    void update_peer_mac(u8 * mac_addr);
    void set_participant();

    TimerService &timer_service;

    typedef TimerService::clock_t Clock;

    notifications::Timer joinTimer;
    notifications::Timer leaveTimer;
    notifications::Timer periodicTimer;
    notifications::Timer leaveAllTimer;

    // Need these to keep track of the timer states (running, stopped)
    bool joinTimerFlag = false;
    bool leaveTimerFlag = false;
    bool periodicTimerFlag = false;
    bool leaveAllTimerFlag = false;
    bool executing_ = false;

    u32 leaveTimerCount = 0;

    void request_tx(); // i.e. start join timer
    void start_leave_timer();
    void stop_leave_timer();
    void start_periodic_timer();
    void start_leaveall_timer();

    milliseconds remaining(notifications::Timer *t) { return timer_service.get_remaining(t); }

    mesa_rc redeclare(vtss_appl_mstp_msti_t msti);
    mesa_rc flush(vtss_appl_mstp_msti_t msti);

    mesa_rc transmit_leave(vtss_appl_mstp_msti_t msti);

    MrpAppl &parent_;
    u32 port_;

    // WARNING, this is a async call, and must be locked inside this class
    void execute(notifications::Timer * t);

    MrpMadMachines regApp;

    u64 failedRegistrations = 0;
    u8 peer_mac_address[VTSS_MRP_PEER_MAC_ADDRESS_SIZE] = {0, 0, 0, 0, 0, 0};

    // State machine transitions as specified in 802.1Q-2014
    static Pair<xxrp_bool_states, xxrp_leaveall_cbs>
    leaveall_next_state(xxrp_bool_states state, xxrp_leaveall_events event);
    // private helper method
    mesa_rc leaveall_do_callback(xxrp_leaveall_cbs cb_enum);
    mesa_rc handle_event_leaveall(xxrp_leaveall_events event);

    // State machine transitions as specified in 802.1Q-2014
    static Pair<xxrp_bool_states, xxrp_periodic_cbs>
    periodic_next_state(xxrp_bool_states state, xxrp_periodic_events event);
    // private helper method
    mesa_rc periodic_do_callback(xxrp_periodic_cbs cb_enum);
    mesa_rc handle_event_periodic(xxrp_periodic_events event);

    xxrp_bool_states leaveall_state;
    xxrp_bool_states periodic_state;
private:
    bool participant = false;
    bool operPointToPointMAC();
};

struct MrpMap {
    // Constructor
    MrpMap(MrpAppl &parent);
    // Destructor
    ~MrpMap()
    {
        /* Clear all MSTI vectors */
        for (int msti = 0; msti < MRP_MSTI_MAX; ++msti) {
            ring[msti].clear();
        }
    }

    MrpAppl &parent_;

    Array<Vector<u32>, MRP_MSTI_MAX> ring;

    bool last_of_set(u32 port_no, mesa_vid_t v);
    mesa_rc propagate_join(u32 port_no, vtss_appl_mstp_msti_t msti);
    mesa_rc propagate_join(u32 port_no, mesa_vid_t v);
    mesa_rc propagate_leave(u32 port_no, vtss_appl_mstp_msti_t msti);
    mesa_rc propagate_leave(u32 port_no, mesa_vid_t v);
    mesa_rc trigger_join(u32 port_no, vtss_appl_mstp_msti_t msti);
    mesa_rc trigger_leave(u32 port_no, vtss_appl_mstp_msti_t msti);
    mesa_rc add_port_to_map(u32 port_no, vtss_appl_mstp_msti_t msti);
    mesa_rc remove_port_from_map(u32 port_no, vtss_appl_mstp_msti_t msti);

    /* True = Forwarding, False = Discarding */
    bool mstp_port_state(vtss_appl_mstp_msti_t msti, u32 port_no);
private:
};

struct MrpAppl {
    // Constructor
    MrpAppl(TimerService &ts)
        : ts_(ts)
    {
        // Initialize MVRP port state array to all ports disabled
        /* Mad and Map structures will be initialized when the MRP appl is enabled */
        /* Also initialize vlan_list_ here. It will be updated later if needed */
        vlan_list_.clear_all();
        for (int v = XXRP_VLAN_ID_MIN; v < XXRP_VLAN_ID_MAX; ++v) {
            vlan_list_.set(v);
            vlan2index_[v] = v - 1;
            index2vlan_[v - 1] = v;
        }
    }

    CapArray<MrpMad *, MEBA_CAP_BOARD_PORT_MAP_COUNT> mad = {};
    MrpMap *map = nullptr;

    // Get/Set Global MRP application state
    bool global_state() { return global_state_; }
    mesa_rc global_state(bool state);

    // Get/Set Port MRP application state
    bool port_state(u32 port_no) {
        return port_state_[port_no];
    }
    mesa_rc port_state(u32 port_no, bool state);

    // Set Global MRP timer timeouts
    mesa_rc port_timers(u32 port_no,
                        MrpTimeouts t);

    // Get/Set Port MRP periodic state
    bool periodic_state(u32 port_no)  {
        return periodic_state_[port_no];
    }
    mesa_rc periodic_state(u32 port_no, const bool state);

    // Update VLAN List
    mesa_rc vlan_list(const VlanList &vls);
    // VLAN list helper functions
    const VlanList &vlan_list() const { return vlan_list_; }
    const Array<uint16_t, 4096> &vlan2index() const { return vlan2index_; }
    size_t vlan2index(mesa_vid_t v) { return vlan2index_[v]; }

    // VLAN fixed registration change
    mesa_rc handle_vlan_change(u32 port_no, mesa_vid_t v,
                               vlan_registration_type_t t);

    // Frame reception
    mesa_rc receive_frame(u32 port_no, const u8 *pdu, size_t length);

    // Port state change - Forwarding/Discarding
    mesa_rc handle_port_state_change(u32 port_no, vtss_appl_mstp_msti_t msti,
                                     port_states state);
    // Port role change - Designated/RootAlternate
    void handle_port_role_change(u32 port_no, vtss_appl_mstp_msti_t msti,
                                 port_roles role);

    mesa_rc vtss_mvrp_tx(u32 l2port, u8 *all_attr_events, u32 total_events, BOOL la_flag);

    CapArray<MrpTimeouts, MEBA_CAP_BOARD_PORT_MAP_COUNT> timeouts;

    CapArray<bool, MEBA_CAP_BOARD_PORT_MAP_COUNT> periodic_state_ = {};
private:
    TimerService &ts_;
    bool global_state_ = false;
    CapArray<bool, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_state_ = {};
    VlanList vlan_list_;
    Array<uint16_t, 4096> vlan2index_ = {};
    Array<mesa_vid_t, 4096> index2vlan_ = {};

    mesa_rc initialize_participant(u32 port_no);

    mesa_rc process_vector(u32 l2port, u16 first_value,
                           u16 num_of_valid_vlans, u8 vector);
    mesa_rc process_vector_attribute(u32 l2port,
                                     mvrp_pdu_vector_attr_t *vec_attr, u16 *size);

    void update_vlan2index();
    mesa_rc add_managed_vlan(const mesa_vid_t vid);
    mesa_rc remove_managed_vlan(const mesa_vid_t vid);
    void print_vlan2index();
};

template <typename F>
void MrpMadMachines::for_all_machines(F f) {
    const VlanList &v = parent_.parent_.vlan_list();

    for (auto it = v.begin(); it != v.end(); ++it) {
        f(*it, data[vlan2index(*it)]);
    }
}

template <typename F>
void MrpMadMachines::for_all_machines_forwarding(F f) {
    const VlanList &v = parent_.parent_.vlan_list();
    mstp_msti_config_t msti_config;
    vtss_appl_mstp_msti_t msti;
    Array<bool, MRP_MSTI_MAX> mstp_state = {};

    // Have an array of forwarding/discarding state for each MSTI of the port ready
    for (msti = 0; msti < MRP_MSTI_MAX; ++msti) {
        mstp_state[msti] = parent_.parent_.map->mstp_port_state(msti, parent_.port_);
    }
    if (vtss_appl_mstp_msti_config_get(&msti_config, NULL) == VTSS_RC_OK) {
        for (auto it = v.begin(); it != v.end(); ++it) {
            // Only call f() if the vlan belongs to a "forwarding" msti
            // Index the array created above to get that status
            if (mstp_state[msti_config.map.map[*it]]) {
                f(*it, data[vlan2index(*it)]);
            }
        }
    }
}

} // namespace mrp
} // namespace vtss

#endif  // __XXRP_BASE_VTSS_XXRP_TYPES_HXX__
