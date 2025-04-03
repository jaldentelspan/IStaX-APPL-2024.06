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

#ifndef VTSS_PTP_CLOCK_H
#define VTSS_PTP_CLOCK_H

#include "vtss_ptp_types.h"
#include "vtss/appl/ptp.h"
#include "vtss/appl/synce.h"
#include "vtss_ptp_internal_types.h"
#include "vtss_ptp_slave.h"
#include "vtss_ptp_unicast_master_table.h"
#include "vtss_ptp_unicast_slave_table.h"
#include "vtss_ptp_port.h"

// Chip clock domain used when software maintained clock is used by application.
#define SOFTWARE_CLK_DOMAIN 0

typedef struct ptp_clock_s {
    ptp_clock_default_ds_t defaultDS;
    vtss_appl_clock_quality announced_clock_quality;
    BOOL announced_currentUtcOffsetValid;
    BOOL announced_timeTraceable;

    CurrentDS currentDS;
    vtss_appl_ptp_clock_parent_ds_t parentDS;
    u16 parentStepsRemoved;
    PtpPort_t *parentRecPort;
    vtss_appl_ptp_clock_timeproperties_ds_t timepropertiesDS;
    u16 slavePort; /* == 0 if no slave ports, */
    u8 synce_clock_class;  // holds the latest clock class sent to synce
    vtss_appl_synce_ptp_ptsf_state_t ptsf_state;  // holds the latest ptsf state reported to Synce
    /* Other things we need for the protocol */
    PtpPort_t *ptpPort;
    struct ptp_init_clock_ds_t *clock_init;
    vtss_appl_ptp_clock_timeproperties_ds_t *time_prop;
    int localClockId;
    vtss_appl_ptp_protocol_adr_t ptp_primary;
    vtss_appl_ptp_protocol_adr_t ptp_pdelay;
    UnicastSlaveTable_t slave[MAX_UNICAST_MASTERS_PR_SLAVE];
    u16 selected_master;
    UnicastMasterTable_t master[MAX_UNICAST_SLAVES_PR_MASTER];
    ptp_slave_t ssm;             /* slave eventhandler state machine */
    ptp_tc_t    tcsm;            /* TC eventhandler state machine */
    u32         holdover_timeout_spec;  /* Holdover spec timer for G8275 profile */
    u8          local_clock_class;      /* local clock class based on the Synce clock QL */
    ptp_path_trace_t path_trace;        /* this node's path trace received in Announce from the actual master */
    ptp_follow_up_tlv_info_t follow_up_info;  /* this node's Follow_up tlv information from the actual master */
    u8 majorSdoId;                      /* see 802.1AS clause 8.1 */
    vtss_ptp_clock_802_1as_bmca_t bmca_802_1as;
    vtss_appl_ptp_802_1as_current_ds_t current_802_1as_ds;
    vtss_ptp_port_sync_sync_t   port_802_1as_sync_sync;
    bool                        localGMUpdate;
} ptp_clock_t;

#endif // VTSS_PTP_CLOCK_H
