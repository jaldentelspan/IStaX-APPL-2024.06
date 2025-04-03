/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef VTSS_PTP_PORT_H
#define VTSS_PTP_PORT_H

#include "vtss/appl/ptp.h"
#include "vtss_ptp_master.h"
#include "vtss_ptp_filters.hxx"

typedef enum {
    PTP_PDELAY_MECH_802_1AS_CMLDS=0x1,
    PTP_PDELAY_MECH_802_1AS_INST,
    PTP_PDELAY_MECH_NON_802_1AS,
    PTP_PDELAY_MECH_MAX_CNT
} ptp_pdelay_mech_t;

typedef struct {
    vtss_appl_ptp_status_port_ds_t status;
} PortDS;

/*
 * Peer delay mechanism data.
 */
typedef struct {
    struct ptp_clock_s *clock;     /* pointer to clock data */
    struct PtpPort_s  *ptp_port;         /* pointer to port instance data */
    mesa_timestamp_t       t1;
    mesa_timestamp_t       t4;
    mesa_timestamp_t       t2;
    mesa_timestamp_t       t3;
    mesa_timeinterval_t    correctionField;
    u64             t2_cnt;
    bool            waitingForPdelayRespFollow;
    u16  last_pdelay_req_event_sequence_number;
    u32  pdelay_event_sequence_errors;
    u32  pdelay_missed_response_cnt;
    vtss_appl_ptp_port_identity peer_port_id;
    /* tx buffer for sync messages*/
    ptp_tx_buffer_handle_t pdel_req_buf;
    /* tx buffer for follow_up messages*/
    ptp_tx_buffer_handle_t follow_buf;
    /* timer for transmission of Sync messages*/
    vtss_ptp_sys_timer_t pdelay_req_timer;
    u32 requester_state;
    u32 responder_state;
    u32 random_seed;
    /* 2-step tx timestamp callback contexts */
    ptp_tx_timestamp_context_t pdel_req_ts_context;
    ptp_tx_timestamp_context_t pdel_resp_ts_context;
    mesa_mac_t mac;
    bool t1_valid, t2_t3_valid, t4_valid;
    bool peerDelayNotMeasured;  // used internally in the Peer delay proces to keep track of the neighbor's status (set if more responses are received or the measured delay is outside the configured limits.
    u16 current_measure_status;
    u32 requester_set_time_count; // used to detect if the setTime or stepTime has been called since the last pdelay_req was transmitted.
    u32 responder_set_time_count; // used to detect if the setTime or stepTime has been called since the last pdelay_req was received.
    // used to calculate the neighborRateRatio in 2-step mode
    mesa_timestamp_t       oldt3;
    mesa_timestamp_t       oldt4;
    bool            old_t3_t4_ok;
    u32             ratio_settle_time;
    u32             ratio_clear_time;
    bool            neighbor_rate_ratio_valid;
    vtss_ptp_filters::vtss_lowpass_filter_t neighbor_rate_filter;
    double fratio;
#if defined(VTSS_SW_OPTION_P802_1_AS)
    u32  detectedFaults;
    u8   lastReceivedMajorSdoId;
    vtss_ptp_sys_timer_t pdelay_resp_timer;
    u8 pdelay_multiple_response_cnt;
    bool pdelay_multiple_responses;
#endif
    /*CMLDS */
    ptp_pdelay_mech_t pdelay_mech;
    struct ptp_cmlds_port_ds_s *port_cmlds;
    mesa_timeinterval_t prev_delay;
} PeerDelayData;

#define PTP_CLOCK_INSTANCES 4
typedef struct ptp_cmlds_port_ds_s {
    vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t       *conf;
    vtss_appl_ptp_802_1as_cmlds_status_port_ds_t     status;
    vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t statistics;
    bool                                             cmlds_in_use[PTP_CLOCK_INSTANCES];
    PeerDelayData                                    pDelay;
    vtss_ptp_filters::vtss_lowpass_filter_t          delay_filter;
    mesa_bool_t                                      peer_delay_ok;
    bool                                             p2p_state;
    u16                                              uport;
    bool                                             conf_modified;
    u16                                              port_signaling_message_sequence_number;
} ptp_cmlds_port_ds_t;

typedef struct {
    bool                enable;     /* true if wireless variable delay option is enabled */
    u32                 delay_pre;  /* set to 0 in 'pre-notification', set to 2 in 'delay_set', set to 0 in the PTP process */
    /* wireless delay for a packet = base_delay + packet_length * incr_delay */
    mesa_timeinterval_t base_delay; /* base wireless delay */
    mesa_timeinterval_t incr_delay; /* incremental wireless delay */
} WirelessData;

typedef struct {
    vtss_appl_clock_identity grandmasterIdentity;
    u8 priority1;
    vtss_appl_clock_quality clockQuality;
    u8 priority2;
    u16 stepsRemoved;
    vtss_appl_ptp_port_identity sourcePortIdentity;    /* current parent sourcePort identity */
    u8 localPriority;                                  /* local priority used in G.8275 BMCA */
} ClockDataSet;

typedef struct {
    u16 foreignMasterAnnounceMessages; /* used in announce message accept */
    bool qualified;                             /* Indicates that at least FOREIGN_MASTER_THRESHOLD announce messages has been recived within FOREIGN_MASTER_TIME_WINDOW */
    ClockDataSet ds;
    i16 currentUtcOffset;
    u8 flagField[2];
    u8 timeSource;
} ForeignMasterDS;

typedef struct PtpPort_s {
    PortDS portDS;
    ForeignMasterDS *foreignMasterDS;
    vtss_appl_ptp_config_port_ds_t *port_config;
    vtss_appl_ptp_status_port_statistics_t port_statistics;
    /* Port configuration data set */

    i16  max_foreign_records;     /* max number of entries in foreign records */
    i16  number_foreign_records;  /* current number of entries in foreign records */
    i16  foreign_record_i;        /* index in foreign records, where a new entry is added */
    i16  foreign_record_best;     /* the currently best foreign master */

    u16  parent_last_announce_sequence_number;
    u16  parent_last_sync_sequence_number;
    u16  parent_last_follow_up_sequence_number;
    /* peer delay data */
    PeerDelayData pDelay;
    bool designatedEnabled;
    bool virtual_port;
    bool linkState;
    bool p2p_state;                            /* true is the P2p delay process is active */
    bool awaitingFollowUp;              /* true from receiving a sync until followUp has been receieved */
    u64 port_mask;                      /* port mask used for the transmission functions */

    struct ptp_clock_s *parent;         /* pointer to clock instance data */
    WirelessData  wd;                   /* wireless delay configuration */
    ptp_master_t msm;                   /* master eventhandler state machine */
    ptp_announce_t ansm;                /* announce issue eventhandler state machine */
    gptp_t         gptpsm;
    /* timer for Annnounce message timeout*/
    vtss_ptp_sys_timer_t announce_to_timer;
    /* timer for Annnounce qualification timer*/
    vtss_ptp_sys_timer_t announce_qual_timer;
    /* timer for state decission calculation*/
    vtss_ptp_sys_timer_t announce_bmca_timer;
    // 802.1AS message interval request data
    u16  port_signaling_message_sequence_number;
    bool first_message_interval_request;
    i8 transmittedLogAnnounceInterval;
    i8 transmittedLogSyncInterval;
    i8 transmittedLogPDelayReqInterval;
    vtss_ptp_port_802_1as_bmca_t bmca_802_1as;
    vtss_ptp_sys_timer_t bmca_802_1as_sync_timer;
    vtss_ptp_sys_timer_t bmca_802_1as_in_timeout_sync_timer;
    mesa_bool_t          neighborGptpCapable;
} PtpPort_t;

#endif // VTSS_PTP_PORT_H
