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

#ifndef VTSS_PTP_UNICAST_MASTER_TABLE_H
#define VTSS_PTP_UNICAST_MASTER_TABLE_H

#include "vtss/appl/ptp.h"
#include "vtss_ptp_master.h"

typedef struct UnicastMasterTable_s {
    vtss_appl_ptp_protocol_adr_t  slave;
    u16      port;
    struct ptp_clock_s *parent;         /* pointer to clock instance data */
    ptp_announce_t  ansm;
    ptp_master_t    msm;                   /* master eventhandler state machine */
    /* timer for Announce request message timeout*/
    vtss_ptp_sys_timer_t ann_req_timer;
    i32   ann_timer_cnt;     /* announce time counter in seconds */
    /* timer for Sync request message timeout*/
    vtss_ptp_sys_timer_t sync_req_timer;
    bool    master_active;  /* indicates if the master process is running */
    vtss_appl_ptp_port_identity targetPortIdentity;
} UnicastMasterTable_t;

#endif // VTSS_PTP_UNICAST_MASTER_TABLE_H
