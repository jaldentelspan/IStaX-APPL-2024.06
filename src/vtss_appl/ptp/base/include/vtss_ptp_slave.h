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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

/**
 * \file
 * \brief PTP Slave function.
 * \details Define the interface for an PTP slave protocol handler.
 */

#ifndef VTSS_PTP_SLAVE_H
#define VTSS_PTP_SLAVE_H

#include "vtss_ptp_types.h"
#include "vtss_ptp_packet_callout.h"
#include "vtss_ptp_sys_timer.h"
#include "vtss_ptp_servo.h"
#include <vtss/basics/ringbuf.hxx>

#ifdef __cplusplus
        extern "C" {
#endif

// Maximum number of delay request messages that can be sent without receiving acknowledgement.
// When the round-trip delay of receiving delay response message is more than  the time interval between successive delay request messages, then
// there is chance considering the delay request message as lost.
// To avoid such problems, Queue is created for delay request messages sent and not yet acknowledged with receiving delay response message.
#define DELAY_REQ_QUEUE_SIZE 16

typedef struct MsgHeader_s MsgHeader;    // Note: These are forward declarations to solve circular dependencies.
typedef struct PtpPort_s PtpPort_t;      //
typedef struct ptp_clock_s ptp_clock_t;  //

/**
 * \brief Slave internal Clock state
 */
typedef enum {
    VTSS_PTP_SLAVE_CLOCK_FREERUN,        /* Free run state (initial state) */
    VTSS_PTP_SLAVE_CLOCK_F_SETTLING,     /* Sync and DelayReq are ignored while the PTP time is settling in freq mode */
    VTSS_PTP_SLAVE_CLOCK_FREQ_LOCK_INIT, /* Frequency initial locking (adjust offset) */
    VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING,   /* Frequency Locking */
    VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING,  /* Phase Locking */
    VTSS_PTP_SLAVE_CLOCK_P_SETTLING,     /* Sync and DelayReq are ignored while the PTP time is settling in phase mode */
    VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED,    /* Frequency locked state (final state in "one way" mode and also in "two way" mode when advanced servo is configured to use "frequency mode") */
    VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED,   /* Phase locked state (final state in "two way" mode for basic servo and for advanced servo when locking mode is "phase mode"). */
    VTSS_PTP_SLAVE_CLOCK_HOLDOVER,       /* Holdover state */
    VTSS_PTP_SLAVE_CLOCK_RECOVERING,     /* Recovering Path delay after reconnecting to master in holdover mode */
} vtss_slave_clock_state_t;

typedef struct {
    uint16_t            seq_id;  //sequence id of delay request message.
    uint32_t            ts_id;   //needed for 2-step switch timestamps since switch does not return seq_id or signature currently.
    mesa_timestamp_t    tx_time;
    mesa_timestamp_t    rx_time;
    mesa_timeinterval_t corr;
    bool                tx_valid;// tx timestamp valid or not.
    bool                rx_valid;// rx timestamp valid or not.
} ptp_ts_entry_t;

typedef vtss::RingBuf<ptp_ts_entry_t, DELAY_REQ_QUEUE_SIZE> delay_req_ts_q;
typedef delay_req_ts_q::iterator delay_req_ts_q_itr_t;

/**
 * ptp slave implementation
 */
typedef struct ptp_slave_s {
    /* public data */
    bool twoStepFlag;           /* true if the transmit time is saved in the timestamp FIFO, otherwise use the correction field update */
    u8 protocol;
    u8 domainNumber;            /* Only accept packets from the right domain number */
    vtss_appl_ptp_port_identity *portIdentity_p;
    vtss_appl_ptp_port_identity *parent_portIdentity_p; /* Check if Sync comes from current parent */
    int localClockId;           /* PTP instance number indicating the HW clock id. */
    u16 versionNumber;
    u64 port_mask;
    ptp_clock_t *clock;         /* pointer to clock data */
    PtpPort_t *slave_port;      /* pointer to slave port dataset, NULL if no ports are in slave mode */
                                /* portDS.logSyncInterval: Test if the master config matches the slave config (syncIntervalError) */
                                /* portDS.logMinDelayReqInterval logMessageInterval received from master (used for relayReq timer) */
    bool *record_update;        /* tbd if needed  Indicate activity on port (recalculate bmc if either an announce or sync packet is received)*/
    bool two_way;               /* true if not oneway */
    ptp_servo *servo;
    int debugMode;              /* debugmode: 0 = no debug, 1 = slave logs raw offset from master and disables clock adjustment */
    bool activeDebug;            /* set when debugmode entered */
    FILE *logFile;              /* File descriptor for log file used by ptp timestamp logging command */
    bool keep_control;          /* keep on controlling the clock while logging packets, ogherwise force the slave into Holdover in the log period */
    vtss_ptp_sys_timer_t log_timer; /* system timer, used to control when the log stops. */
    /* private data */
    u16  last_delay_req_event_sequence_number; /* delay req sequence number */
    u16  parent_last_sync_sequence_number; /* Used to check if Sync and Followup matches */
    bool  wait_follow_up ;   /* state variable (indicating 2 step Sync received, and waiting for followup) */
    int lost_followup;       /* count number of lost followup messages */
    mesa_timeinterval_t  sync_correctionField; /* saved correction field for use in followup */
    mesa_timestamp_t     sync_tx_time; /* saved preciseOriginTimestamp (for debug) */
    mesa_timestamp_t     sync_receive_time; /* saved sync rx time to be used when follow_up is received */
    delay_req_ts_q       *dly_q;
    u32 random_seed;
    u16 delay_req_timer;
    mesa_timeinterval_t  master_to_slave_delay;
    bool master_to_slave_delay_valid;
    mesa_timeinterval_t  slave_to_master_delay;
    vtss_slave_clock_state_t clock_state;
    vtss_slave_clock_state_t old_clock_state; // used to keep track of changes in the clock_state

    vtss_ptp_sys_timer_t settle_timer;            /* timer for clock settling in basic servo */
    i32 clock_settle_time;                        /* settling time in sec */
    bool in_settling_period;            /* true while time phase is being adjusted this may take several seconds */

    /* private date for OAM slave */
    u32 timeout_cnt;        /* count timeouts from the OAM process cleared when timestamps received, incremented for each timeout*/

    /* tx buffer for sync messages*/
    ptp_tx_buffer_handle_t delay_req_buf;
    /* timer for Sync message timeout*/
    vtss_ptp_sys_timer_t sync_timer;
    i8 logSyncInterval;  // received logSyncInterval from master, to be used in the sync timeout handler.
    i8 logDelayReqInterval;  // received logMessageInterval from master, to be used in the delayresponse handler.
    u32 state;
    CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> *clockBase;  // needed for the callout to delayCalc.
    /* 2-step tx timestamp callback contexts */
    ptp_tx_timestamp_context_t del_req_ts_context;
    mesa_mac_t mac;
    vtss_appl_ptp_protocol_adr_t sender;  /* master protocol addr used for unicast transmission */
    bool first_sync_packet;               /* true until sync packet received from master */
    /* timer for Delay_Req message transmission*/
    vtss_ptp_sys_timer_t delay_req_sys_timer;
    /* Delay req option: true => Random delay request timer, false => DelayReq as soon as possible after Sync */
    bool random_relay_request_timer;
    vtss_ptp_slave_statistics_t statistics;
    bool ptsf_loss_of_announce;
    bool ptsf_loss_of_sync;
    bool ptsf_unusable;
    u32  g8275_holdover_trace_count;
    i32 sy_to;                          /* sync timeout calculated as 3 * the actual sync rate, though for high packet rates, the timeout is set to 1 sec, to be more tolerant to variation in the time between sync packets */
    bool prev_psettling;
    bool virtual_port_select; // set to true when virtual port is selected for synchronisation
} ptp_slave_t;

mesa_rc vtss_ptp_clock_slave_statistics_enable(ptp_clock_t *ptp, bool enable);
mesa_rc vtss_ptp_clock_slave_statistics_get(ptp_clock_t *ptp, vtss_ptp_slave_statistics_t *statistics, bool clear);
void debug_log_header_2_print(FILE *logFile);
void master_to_slave_delay_stati(ptp_slave_t *slave);
void slave_to_master_delay_stati(ptp_slave_t *slave);
const char *clock_state_to_string(vtss_slave_clock_state_t s);

void vtss_ptp_slave_create(ptp_slave_t *slave);
void vtss_ptp_slave_init(ptp_slave_t *slave, vtss_appl_ptp_protocol_adr_t *ptp_dest, ptp_tag_conf_t *tag_conf);
void vtss_ptp_slave_delete(ptp_slave_t *slave);

mesa_rc vtss_ptp_port_speed_to_txt(mesa_port_speed_t speed, const char **speed_txt);
bool vtss_ptp_slave_sync(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender);
bool vtss_ptp_slave_follow_up(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender);
bool vtss_ptp_slave_delay_resp(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, ptp_tx_buffer_handle_t *tx_buf, MsgHeader *header, vtss_appl_ptp_protocol_adr_t *sender);
int ptp_offset_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, mesa_timeinterval_t correction, i8 logMsgIntv, u16 sequenceId, bool virt_port);
void ptp_delay_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, mesa_timeinterval_t correction, i8 logMsgIntv);
bool clock_class_update(ptp_slave_t *ssm);

struct port_latencies_s {
    mesa_timeinterval_t ingress_latency;
    mesa_timeinterval_t egress_latency;
};

struct port_calibrations_s {
    port_latencies_s port_latencies_10m_cu;
    port_latencies_s port_latencies_100m_cu;
    port_latencies_s port_latencies_1g_cu;
    port_latencies_s port_latencies_1g;
    port_latencies_s port_latencies_2g5;
    port_latencies_s port_latencies_5g;
    port_latencies_s port_latencies_10g;
    port_latencies_s port_latencies_25g_nofec;
    port_latencies_s port_latencies_25g_rsfec;
};

struct ptp_port_calibration_s {
    u32 version;
    u32 crc32;
    u32 rs422_pps_delay;
    u32 sma_pps_delay;
    CapArray<port_calibrations_s, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_calibrations;
};

extern ptp_port_calibration_s ptp_port_calibration;

extern bool calib_t_plane_enabled;
extern bool calib_p2p_enabled;
extern bool calib_port_enabled;
extern bool calib_initiate;
extern i32  calib_cable_latency;
extern i32  calib_pps_offset;

#ifdef __cplusplus
}
#endif

#endif
