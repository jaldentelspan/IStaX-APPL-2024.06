/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef VTSS_PTP_MASTER_H
#define VTSS_PTP_MASTER_H

#include "vtss_ptp_types.h"
#include "vtss_ptp_packet_callout.h"
#include "vtss_ptp_sys_timer.h"

#ifdef __cplusplus
        extern "C" {
#endif
/**
 * ptp master implementation
 */
typedef struct ptp_master_s {
    /* public data */
    bool org_time;          /* true if the Origin time option is used in Sync messages, otherwise use correction field update */
    struct ptp_clock_s *clock;     /* pointer to clock data */
    struct PtpPort_s *ptp_port;      /* pointer to port data */
    i8 sync_log_msg_period;        /* Sync interval (if == 0x80, then no sync packets are sent out it only responds to delay requests)  */
    /* private data */
    u16  last_sync_event_sequence_number;
    /* tx buffer for sync messages*/
    ptp_tx_buffer_handle_t sync_buf;
    /* tx buffer for follow_up messages*/
    ptp_tx_buffer_handle_t follow_buf;
    size_t  follow_up_allocated_size;  /* actual size of allocated follow_up buffer */
    /* timer for transmission of Sync messages*/
    vtss_ptp_sys_timer_t sync_timer;
    u32 state;
    /* 2-step tx timestamp callback contexts */
    ptp_tx_timestamp_context_t sync_ts_context;
    mesa_mac_t mac;
    ptp_tag_conf_t tag_conf;
    void *afi;
    bool afi_in_use;
    mesa_bool_t  syncSlowdown;
    uint16_t     numberSyncTransmissions;
} ptp_master_t;

void vtss_ptp_master_create(ptp_master_t *master, vtss_appl_ptp_protocol_adr_t *ptp_dest, ptp_tag_conf_t *tag_conf);
void vtss_ptp_master_delete(ptp_master_t *master);

bool vtss_ptp_master_delay_req(ptp_master_t *master, ptp_tx_buffer_handle_t *tx_buf);

typedef struct {
    u8 flags[2];
    i16 currentUtcOffset;
    u8 grandmaster_priority1;
    vtss_appl_clock_quality grandmaster_clockQuality;
    u8 grandmaster_priority2;
    vtss_appl_clock_identity grandmaster_identity;
    u16 steps_removed;
    u8 time_source;
} AnnounceDS;

/**
 * ptp announce state machine implementation
 */
typedef struct ptp_announce_s {
    /* public data */
    struct ptp_clock_s *clock;     /* pointer to clock data */
    struct PtpPort_s *ptp_port;      /* pointer to port data */
    i8 ann_log_msg_period;         /* Announce interval */

    /* private data */
    u16  last_announce_event_sequence_number;
    /* tx buffer for announce messages*/
    ptp_tx_buffer_handle_t ann_buf;
    size_t  allocated_size;  /* actual size of allocated buffer */
    /* timer for transmission of Announce messages*/
    vtss_ptp_sys_timer_t ann_timer;
    u32 state;
    mesa_mac_t mac;
    void *afi;
    bool afi_in_use;
    bool afi_refresh;
    AnnounceDS announced;
    ptp_path_trace_t announced_path_trace;        /* this port's latest path trace transmitted in Announce */
    bool announced_path_trace_enabled;
    uint16_t    numberAnnounceTransmissions;
    mesa_bool_t announceSlowdown;

} ptp_announce_t;

typedef struct gptp_s {
    vtss_ptp_sys_timer_t bmca_802_1as_gptp_rx_timeout_timer;
    vtss_ptp_sys_timer_t bmca_802_1as_gptp_tx_timer;
    mesa_bool_t          gPtpCapableMessageSlowdown;
    uint16_t             numberGptpCapableMessageTransmissions;
    i8                   gptp_log_msg_period;
} gptp_t;

void vtss_ptp_announce_create(ptp_announce_t *announce, vtss_appl_ptp_protocol_adr_t *ptp_dest, ptp_tag_conf_t *tag_conf);
void vtss_ptp_announce_delete(ptp_announce_t *announce);
#ifdef __cplusplus
}
#endif

#endif // VTSS_PTP_MASTER_H
