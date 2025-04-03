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

#ifndef _IEC_MRP_BASE_HXX_
#define _IEC_MRP_BASE_HXX_

#include "main_types.h"
#include <vtss/appl/iec_mrp.h>
#include <vtss/basics/map.hxx>     /* For vtss::Map()                        */
#include <vtss/basics/ringbuf.hxx> /* For vtss::RingBuf()                    */
#include "iec_mrp_timer.hxx"       /* For mrp_timer_t                        */
#include "iec_mrp_api.h"           /* For ENUM_INC on port_type              */
#include "acl_api.h"               /* For acl_entry_conf_t                   */
#include "afi_api.h"               /* For afi_single_conf_t/afi_multi_conf_t */
#include "packet_api.h"            /* For packet_tx_props_t                  */
#include <vtss/appl/vlan.h>        /* For vtss_appl_vlan_port_type_t         */

/**
 * Timing parameters used when the MRP instance is in MRM and/or MIM state.
 */
typedef struct {
    /**
     * Selects the interval - in microseconds - with which MRP_TopologyChange
     * PDUs are transmitted if this struct is used for MRM and the interval with
     * which MRP_InTopologyChange PDUs are transmitted if this struct is used
     * for MIM.
     *
     * Corresponds to the standard's MRP_TOPchgT for MRM and MRP_IN_TOPchgT for
     * MIM.
     *
     * Valid range is [500; 20000] usec.
     */
    uint32_t topology_change_interval_usec;

    /**
     * Selects the topology change repeat count.
     *
     * Corresponds to the standard's MRP_TOPNRmax for MRM and MRP_IN_TOPNRmax
     * for MIM.
     *
     * Valid range is [1; 5].
     */
    uint16_t topology_change_repeat_cnt;

    /**
     * Selects the short interval - in microseconds - with which MRP_Test PDUs
     * are transmitted if this struct is used for MRM.
     *
     * Corresponds to the standard's MRP_TSTshortT for MRM. Not used for MIM.
     *
     * Valid range is [500; 30000] usec.
     */
    uint32_t test_interval_short_usec;

    /**
     * Selects the default interval - in microseconds - with which MRP_Test PDUs
     * are transmitted if this struct is used for MRM and the default interval
     * with which MRP_InTest PDUs are transmitted if this struct is used for
     * MIM.
     *
     * Corresponds to the standard's MRP_TSTdefaultT for MRM and
     * MRP_IN_TSTdefaultT for MIM.
     *
     * Valid range is [1000; 50000] usec.
     */
    uint32_t test_interval_default_usec;

    /**
     * Specifies the interval count for monitoring the reception of MRP_Test
     * PDUs if this struct is used for MRM and the interval count for monitoring
     * reception of MRP_InTest PDUs if this struct is used for MIM.
     *
     * Corresponds to the standard's MRP_TSTNRmax for MRM, MRP_MON_NR_MAX for
     * MRA currently in MRC state and MRP_IN_TSTNRmax for MIM.
     *
     * Valid range is [1; 5].
     */
    uint16_t test_monitoring_cnt;

    /**
     * Selects the extended interval count for monitoring the reception of
     * MRP_Test PDUs if this struct is used for MRM.
     *
     * Corresponds to the standard's MRP_TSTExtNRmax. Not used for MIM.
     *
     * Valid range is [1; 15].
     */
    uint16_t test_monitoring_extended_cnt;

    /**
     * Selects the interval - in microseconds - with which MRP_InLinkStatusPoll
     * PDUs are transmitted if this struct is used for MIM.
     *
     * Corresponds to the standard's MRP_IN_LNKSTATchgT for MIM. Not used for
     * MRM.
     *
     * Valid range is [1000; 50000] usec.
     */
    uint32_t link_status_poll_interval_usec;

    /**
     * Selects the number of MRP_InLinkStatusPoll PDUs are transmitted if this
     * struct is used for MIM.
     *
     * Corresponds to the standard's MRP_IN_LNKSTATNRmax for MIM. Not used for
     * MRM.
     *
     * Valid range is [1; 10].
     */
    uint16_t link_status_poll_repeat_cnt;
} mrp_manager_timing_t;

/**
 * Timing parameters used when the MRP instance is in MRC and/or MIC state.
 */
typedef struct {
    /**
     * Selects the interval - in microseconds - with which MRP_LinkDown PDUs are
     * transmitted if this struct is used for MRC and the interval with which
     * MRP_IN_LinkDown PDUs are transmitted if this struct is used for MIC.
     *
     * Corresponds to the standard's MRP_LNKdownT for MRC and MRP_IN_LNKdownT
     * for MIC.
     *
     * Valid range is [1000; 50000] usec.
     */
    uint32_t link_down_interval_usec;

    /**
     * Selects the interval - in microseconds - with which MRP_LinkUp PDUs are
     * transmitted if this struct is used for MRC and the interval with which
     * MRP_IN_LinkUp PDUs are transmitted if this struct is used for MIC.
     *
     * Corresponds to the standard's MRP_LNKupT for MRC and MRP_IN_LNKupT
     * for MIC.
     *
     * Valid range is [1000; 50000] usec.
     */
    uint16_t link_up_interval_usec;

    /**
     * Selects the number of MRP_LinkChange PDUs to send if this struct is used
     * for MRC and the number of MRP_InLinkChange PDUs to send if this struct is
     * used for MIC.
     *
     * Corresponds to the standard's MRP_LNKNRmax for MRM and MRP_IN_LNKNRmax
     * for MIC.
     *
     * Valid range is [1; 10].
     */
    uint16_t link_change_repeat_cnt;
} mrp_client_timing_t;

// DMACs (table 19).
typedef enum {
    MRP_PDU_DMAC_TYPE_MC_TEST      = 1, /** => 01-15-4E-00-00-01. MRP_Test and certain MRP_Option PDUs (MRP_TestPropagate and MRP_TestMgrNAck) */
    MRP_PDU_DMAC_TYPE_MC_CONTROL   = 2, /** => 01-15-4E-00-00-02. MRP_LinkChange, MRP_TopologyChange, MRP_Option                               */
    MRP_PDU_DMAC_TYPE_MC_INTEST    = 3, /** => 01-15-4E-00-00-03. MRP_InTest                                                                   */
    MRP_PDU_DMAC_TYPE_MC_INCONTROL = 4, /** => 01-15-4E-00-00-04. MRP_InLinkChange, MRP_InTopologyChange, MRP_InLinkStatus_poll                */
} mrp_pdu_dmac_type_t;

typedef struct {
    mesa_port_no_t                      port_no;
    mesa_mac_t                          smac;
    bool                                link;
    vtss_appl_vlan_port_detailed_conf_t vlan_conf;
    mesa_etype_t                        tpid;
} mrp_port_state_t;

// MRP ether type. See clause 8.1.6.
#define MRP_ETYPE 0x88e3u

// Cannot enumerate values directly inside this one, because it's also used to
// size an index an array of MRP_SequenceID.
typedef enum {
    MRP_PDU_TLV_TYPE_END,
    MRP_PDU_TLV_TYPE_COMMON,
    MRP_PDU_TLV_TYPE_TEST,
    MRP_PDU_TLV_TYPE_TOPOLOGY_CHANGE,
    MRP_PDU_TLV_TYPE_LINK_DOWN,
    MRP_PDU_TLV_TYPE_LINK_UP,
    MRP_PDU_TLV_TYPE_IN_TEST,
    MRP_PDU_TLV_TYPE_IN_TOPOLOGY_CHANGE,
    MRP_PDU_TLV_TYPE_IN_LINK_DOWN,
    MRP_PDU_TLV_TYPE_IN_LINK_UP,
    MRP_PDU_TLV_TYPE_IN_LINK_STATUS_POLL,
    MRP_PDU_TLV_TYPE_OPTION,
    MRP_PDU_TLV_TYPE_NONE
} mrp_pdu_tlv_type_t;

// Enum values match the ones we use in sub-tlv type.
typedef enum {
    MRP_PDU_SUB_TLV_TYPE_NONE           = 0x0,
    MRP_PDU_SUB_TLV_TYPE_TEST_MGR_NACK  = 0x1,
    MRP_PDU_SUB_TLV_TYPE_TEST_PROPAGATE = 0x2,
    MRP_PDU_SUB_TLV_TYPE_AUTO_MGR       = 0x3,
} mrp_pdu_sub_tlv_type_t;

typedef enum {
    MRP_SM_STATE_POWER_ON, /**< MRM, MRC, MRA. Initialization                  */
    MRP_SM_STATE_AC_STAT1, /**< MRM, MRC, MRA. Awaiting Connection State 1     */
    MRP_SM_STATE_PRM_UP,   /**< MRM,      MRA. Primary Ring Port with Link Up  */
    MRP_SM_STATE_CHK_RO,   /**< MRM,      MRA. Check Ring, Ring Open state     */
    MRP_SM_STATE_CHK_RC,   /**< MRM,      MRA. Check Ring, Ring Closed state   */
    MRP_SM_STATE_DE_IDLE,  /**<      MRC, MRA. Data Exchange Idle state        */
    MRP_SM_STATE_PT,       /**<      MRC, MRA. Pass Through                    */
    MRP_SM_STATE_DE,       /**<      MRC, MRA. Data Exchange                   */
    MRP_SM_STATE_PT_IDLE,  /**<      MRC, MRA. Pass Through Idle state         */
} mrp_sm_state_t;

typedef enum {
    MRP_IN_SM_STATE_POWER_ON, /**< MIM, MIC Initialization                      */
    MRP_IN_SM_STATE_AC_STAT1, /**< MIM, MIC Awaiting Connection State 1         */
    MRP_IN_SM_STATE_CHK_IO,   /**< MIM      Check Interconnection, Open state   */
    MRP_IN_SM_STATE_CHK_IC,   /**< MIM      Check Interconnection, Closed state */
    MRP_IN_SM_STATE_PT,       /**<      MIC Pass Through                        */
    MRP_IN_SM_STATE_IP_IDLE,  /**<      MIC Interconnection port Idle state     */
} mrp_in_sm_state_t;

extern       mesa_mac_t MRP_chassis_mac;
extern const mesa_mac_t mrp_multicast_dmac;
extern       uint32_t   MRP_cap_port_cnt;
extern       bool       MRP_can_use_afi;
extern       bool       MRP_hw_support;

// State of ring interconnection ports.
typedef struct {
    // MRP_SequenceID - one per PDU type per port
    // The standards is not very specific about this, but since Maserati has a
    // counter per port and since it auto-increments it for MRP_Test PDUs, I
    // think we should have one per type per port.
    uint16_t sequence_id[MRP_PDU_TLV_TYPE_NONE];

    // S/W forwarded MRP_InXXX PDUs.
    packet_tx_props_t fwd_tx_props;

    // MRP_Test PDU to Tx.
    // The test_XXX_ptr are for variable fields in the PDU.
    // Whether or not we use the AFI for MRP_Test PDUs, we use an AFI structure
    // to hold the frame and Tx properties.
    afi_multi_conf_t  test_afi_conf;
    uint32_t          test_afi_id;
    uint8_t           *test_port_role_ptr;                  // Location in frame of MRP_PortRole
    uint8_t           *test_ring_state_ptr;                 // Location in frame of MRP_RingState
    uint8_t           *test_transition_ptr;                 // Location in frame of MRP_Transition
    uint8_t           *test_timestamp_ptr;                  // Location in frame of MRP_Timestamp
    uint8_t           *test_sequence_id_ptr;                // Location in frame of MRP_SequenceID

    // MRP_TopologyChange PDU to Tx.
    // The topology_change_XXX_ptr are for variable fields in the PDU.
    packet_tx_props_t topology_change_tx_props;
    uint8_t           *topology_change_interval_ptr;        // Location in frame of MRP_Interval
    uint8_t           *topology_change_sequence_id_ptr;     // Location in frame of MRP_SequenceID

    // MRP_LinkChange PDU to Tx.
    // The link_change_XXX_ptr are for variable fields in the PDUs.
    packet_tx_props_t link_change_tx_props;
    uint8_t           *link_change_tlv_type_ptr;            // Location in frame of TLVHeader.Type
    uint8_t           *link_change_port_role_ptr;           // Location in frame of MRP_PortRole
    uint8_t           *link_change_interval_ptr;            // Location in frame of MRP_Interval
    uint8_t           *link_change_sequence_id_ptr;         // Location in frame of MRP_SequenceID

    // MRP_TestMgrNAck PDU to Tx.
    // The test_mgr_nack_XXX_ptr are for variable fields in the PDU.
    packet_tx_props_t test_mgr_nack_tx_props;
    uint8_t           *test_mgr_nack_other_prio_ptr;        // Location in frame of MRP_OtherMRMPrio
    uint8_t           *test_mgr_nack_other_mac_ptr;         // Location in frame of MRP_OtherMRMSA
    uint8_t           *test_mgr_nack_sequence_id_ptr;       // Location in frame of MRP_SequenceID

    // MRP_Test_Propagate PDUs to Tx.
    packet_tx_props_t test_propagate_tx_props;
    uint8_t           *test_propagate_other_prio_ptr;       // Location in frame of MRP_OtherMRMPrio
    uint8_t           *test_propagate_other_mac_ptr;        // Location in frame of MRP_OtherMRMSA
    uint8_t           *test_propagate_sequence_id_ptr;      // Location in frame of MRP_SequenceID

    // MRP_InTest PDU to Tx.
    // The in_test_XXX_ptr are for variable fields in the PDU.
    // Whether or not we use the AFI for MRP_InTest PDUs, we use an AFI
    // structure to hold the frame and Tx properties.
    afi_single_conf_t in_test_afi_conf;
    uint32_t          in_test_afi_id;
    uint8_t           *in_test_port_role_ptr;               // Location in frame of MRP_PortRole
    uint8_t           *in_test_in_state_ptr;                // Location in frame of MRP_InState
    uint8_t           *in_test_transition_ptr;              // Location in frame of MRP_Transition
    uint8_t           *in_test_timestamp_ptr;               // Location in frame of MRP_Timestamp
    uint8_t           *in_test_sequence_id_ptr;             // Location in frame of MRP_SequenceID

    // MRP_InTopologyChange PDU to Tx.
    // The in_topology_change_XXX_ptr are for variable fields in the PDU.
    packet_tx_props_t in_topology_change_tx_props;
    uint8_t           *in_topology_change_interval_ptr;     // Location in frame of MRP_Interval
    uint8_t           *in_topology_change_sequence_id_ptr;  // Location in frame of MRP_SequenceID

    // MRP_InLinkChange PDU to Tx.
    // The in_link_change_XXX_ptr are for variable fields in the PDUs.
    packet_tx_props_t in_link_change_tx_props;
    uint8_t           *in_link_change_tlv_type_ptr;         // Location in frame of TLVHeader.Type
    uint8_t           *in_link_change_port_role_ptr;        // Location in frame of MRP_PortRole
    uint8_t           *in_link_change_interval_ptr;         // Location in frame of MRP_Interval
    uint8_t           *in_link_change_sequence_id_ptr;      // Location in frame of MRP_SequenceID

    // MRP_InLinkStatusPoll PDU to Tx.
    // The in_link_status_poll_XXX_ptr are for variable fields in the PDUs.
    packet_tx_props_t in_link_status_poll_tx_props;
    uint8_t           *in_link_status_poll_port_role_ptr;   // Location in frame of MRP_PortRole
    uint8_t           *in_link_status_poll_sequence_id_ptr; // Location in frame of MRP_SequenceID
} mrp_ring_port_state_t;

typedef struct {
    uint64_t                       event_time_ms;
    vtss_appl_iec_mrp_role_t       oper_role;
    mrp_sm_state_t                 sm_state;
    vtss_appl_iec_mrp_ring_state_t ring_state;
    vtss_appl_iec_mrp_port_type_t  prm_ring_port;
    bool                           forwarding[3];
    bool                           sf[3];
    mrp_in_sm_state_t              in_sm_state;
    vtss_appl_iec_mrp_ring_state_t in_ring_state;
    const char                     *who;
} mrp_base_history_element_t;

typedef vtss::RingBuf<mrp_base_history_element_t, 50> mrp_base_history_t;
typedef mrp_base_history_t::iterator mrp_base_history_itr_t;

// Info about a received MRP PDU
typedef struct {
    vtss_appl_iec_mrp_pdu_type_t   pdu_type;                                               // Combination of PDU.FirstTLV.Type and possibly also PDU.MRP_Option.FirstSubTLV.Type
    mesa_mac_t                     sa;                                                     // From PDU.[range-of-PDUs].MRP_SA
    uint16_t                       sequence_id;                                            // From PDU.MRP_Common.MRP_SequenceID
    uint8_t                        domain_id[sizeof(vtss_appl_iec_mrp_conf_t::domain_id)]; // From PDU.MRP_Common.MRP_DomainUUID
    uint8_t                        other_prio;                                             // From PDU.MRP_Option.[MRP_TestMgrNAck|MRP_TestPropagate].MRP_OtherMRMPrio
    mesa_mac_t                     other_sa;                                               // From PDU.MRP_Option.[MRP_TestMgrNAck|MRP_TestPropagate].MRP_OtherMRMSA
    uint32_t                       timestamp;                                              // From PDU.[MRP_Test|MRP_InTest].MRP_Timestamp
    uint16_t                       prio;                                                   // From PDU.[MRP_TestMgrNAck|MRP_TestPropagate|MRP_Test|MRP_TopologyChange].MRP_Prio
    uint16_t                       interval_msec;                                          // From PDU.[MRP_TopologyChange|MRP_LinkChange|MRP_InTopologyChange|MRP_InLinkChange].MRP_Interval
    bool                           blocked;                                                // From PDU.MRP_LinkChange.MRP_Blocked
    uint16_t                       in_id;                                                  // From PDU.[MRP_InTest|MRP_InLinkChange|MRP_InLinkStatusPoll|MRP_InTopologyChange].MRP_InID
    uint64_t                       rx_time_msecs;                                          // Timestamp (in millieseconds) of reception of this PDU.
} mrp_rx_pdu_info_t;

// State machine variables
typedef struct {
    mrp_sm_state_t    sm_state;
    mrp_in_sm_state_t in_sm_state;

    vtss_appl_iec_mrp_port_type_t prm_ring_port;
    vtss_appl_iec_mrp_port_type_t sec_ring_port;

    // To figure out whether to update an MRP_Test PDU, we need to save the old
    // state.
    vtss_appl_iec_mrp_port_type_t  test_tx_prm_ring_port;
    vtss_appl_iec_mrp_ring_state_t test_tx_ring_state;
    uint16_t                       test_tx_transitions;
    uint32_t                       test_tx_interval_us;
    bool                           test_tx_in_progress;

    // To figure out whether to update an MRP_InTest PDU, we need to save the
    // old state.
    vtss_appl_iec_mrp_port_type_t  in_test_tx_prm_ring_port;
    vtss_appl_iec_mrp_ring_state_t in_test_tx_ring_state;
    uint16_t                       in_test_tx_transitions;
    uint32_t                       in_test_tx_interval_us;
    bool                           in_test_tx_in_progress;

    // Copy MRP_Test/MRP_InTest PDUs to the CPU to be able to detect multiple
    // MRMs/MIMs (only used if H/W support)?
    mesa_mrp_tst_copy_conf_t mesa_copy_conf;

    // MRC/MRA variables
    uint16_t   MRP_LNKNReturn;

    // MRM/MRA variables
    bool       test_rx_timer_active; // Used instead of test_rx_timer if we have H/W support.
    uint16_t   MRP_MRM_NRmax;
    uint16_t   MRP_MRM_NReturn;
    uint16_t   TC_NReturn;
    mesa_mac_t HO_BestMRM_SA;
    uint16_t   HO_BestMRM_Prio;
    bool       add_test;
    bool       NO_TC;

    // MRA variables
    mesa_mac_t MRP_BestMRM_SA;
    uint16_t   MRP_BestMRM_Prio;
    uint16_t   MRP_MonNReturn;

    // MIM variables
    bool       in_test_rx_timer_active; // Used instead of in_test_rx_timer if we have H/W support.
    uint16_t   MRP_MIM_NRmax;
    uint16_t   MRP_MIM_NReturn;
    uint16_t   IN_TC_NReturn;
    uint16_t   MRP_IN_LNKSTAT_NReturn;

    // MIC variables
    uint16_t   MRP_InLNKNReturn;

    // Number of entries we've put into the syslog - in order not to flood it.
    uint32_t   syslog_cnt;
} mrp_vars_t;

// This structure contains the entire state of an MRP instance.
typedef struct mrp_state_s {
    bool using_port_type(vtss_appl_iec_mrp_port_type_t port_type) const
    {
        if (port_type == VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION && conf.in_role == VTSS_APPL_IEC_MRP_IN_ROLE_NONE) {
            return false;
        }

        return true;
    }

    mesa_mrp_idx_t mrp_idx(void) const
    {
        // The instance we use in the API is the same as our instance number
        // less one, because ours is 1-based, and the API is 0-based.
        return inst - 1;
    }

    uint32_t                   inst;                // For internal trace
    vtss_appl_iec_mrp_conf_t   conf;                // Public configuration
    vtss_appl_iec_mrp_conf_t   old_conf;            // Used when (temporarily) deactivating
    vtss_appl_iec_mrp_status_t status;              // Public status
    mrp_port_state_t           *old_port_states[3]; // Used when (temporarily) deactivating
    mrp_manager_timing_t       mrm_timing;
    mrp_manager_timing_t       mim_timing;
    mrp_client_timing_t        mrc_timing;
    mrp_client_timing_t        mic_timing;
    mrp_ring_port_state_t      ring_port_states[3];
    mrp_port_state_t           *port_states[3];

    // State machine variables
    mrp_vars_t vars;

    // We add ACL rules (ACEs) for capturing and forwarding of MRP PDUs. We need
    // one per PDU DMAC address, because we need to control the forwarding
    // individually (first index) and one for blocked ports and another for
    // forwarding ports (second index). Second index == 0, is for the forwarding
    // and second index == 1 is for the blocking rule.
    acl_entry_conf_t ace_conf[4][2];

    // When we have H/W MRP support, we let the API control forwarding and CPU
    // copying/redirection instead of using ACEs.
    mesa_mrp_conf_t         mesa_conf;
    mesa_mrp_tst_loc_conf_t mesa_loc_conf;
    bool                    added_to_mesa;

    // Timers
    mrp_timer_t test_rx_timer;        // A.k.a. TestTimer
    mrp_timer_t test_tx_timer;        // As TestTimer, but controls Tx of manually injected MRP_Test PDUs (not used when using AFI)
    mrp_timer_t up_timer;             // A.k.a. UpTimer
    mrp_timer_t down_timer;           // A.k.a. DownTimer
    mrp_timer_t top_timer;            // A.k.a. TopTimer
    mrp_timer_t fdb_clear_timer;      // A.k.a. FDBClearTimer
    mrp_timer_t in_test_rx_timer;     // A.k.a. InterconnTestTimer
    mrp_timer_t in_test_tx_timer;     // As InterconnTestTimer, but controls Tx of manually injected MRP_InTest PDUs (not used when using AFI)
    mrp_timer_t in_up_timer;          // A.k.a. InterconnUpTimer
    mrp_timer_t in_down_timer;        // A.k.a. InterconnDownTimer
    mrp_timer_t in_top_timer;         // A.k.a. InterconnTopTimer
    mrp_timer_t in_link_status_timer; // A.k.a. InterconnLinkStatusTimer

    // More timers for deassert multiple MRMs and multiple MIMs events.
    mrp_timer_t multiple_mrms_timer;
    mrp_timer_t multiple_mrms_timer2;
    mrp_timer_t multiple_mims_timer;
    mrp_timer_t multiple_mims_timer2;

    vtss_appl_iec_mrp_notification_status_t notif_status;

    // For debugging purposes
    mrp_base_history_t         history;
    mrp_base_history_element_t last_history_element;
} mrp_state_t;

typedef vtss::Map<uint32_t, mrp_state_t> mrp_map_t;
typedef mrp_map_t::iterator mrp_itr_t;
extern  mrp_map_t           MRP_map;

mesa_rc mrp_base_activate(               mrp_state_t *mrp_state, bool initial_sf_port1, bool initial_sf_port2, bool initial_sf_in);
mesa_rc mrp_base_deactivate(             mrp_state_t *mrp_state);
void    mrp_base_statistics_clear(       mrp_state_t *mrp_state);
void    mrp_base_tx_frame_update(        mrp_state_t *mrp_state);
void    mrp_base_recovery_profile_update(mrp_state_t *mrp_state);
void    mrp_base_rx_frame(               mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info);
void    mrp_base_sf_set(                 mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, bool sf, bool first_invocation = false);
void    mrp_base_loc_set(                mrp_state_t *mrp_state, vtss_appl_iec_mrp_port_type_t port_type, bool mrp_in_test_pdus);
void    mrp_base_history_dump(           mrp_state_t *mrp_state, uint32_t session_id, int32_t (*pr)(uint32_t session_id, const char *fmt, ...), bool print_hdr);
void    mrp_base_history_clear(          mrp_state_t *mrp_state);

#endif /* _IEC_MRP_BASE_HXX_ */

