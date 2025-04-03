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

#ifndef VTSS_PTP_UNICAST_H
#define VTSS_PTP_UNICAST_H

#include "vtss_ptp_internal_types.h"
#include "vtss_ptp_clock.h"
#include "vtss_ptp_unicast_master_table.h"
#include "vtss_ptp_unicast_slave_table.h"

typedef struct {
    u32 inst;
    u32 ip;
} vtss_ptp_master_table_key_t;

/**
 * ptp unicast master negotiation implementation
 */
void masterTableInit(UnicastMasterTable_t *list, ptp_clock_t *parent);
void slaveTableInit(UnicastSlaveTable_t *list, ptp_clock_t *parent);

void vtss_ptp_tlv_process(MsgHeader *header, TLV *tlv, ptp_clock_t *ptpClock, PtpPort_t *ptpPort, vtss_appl_ptp_protocol_adr_t *sender);

void debugIssueCancelUnicast(ptp_clock_t *ptpClock, uint slave_index, u8 message_type);

void vtss_ptp_unicast_slave_conf_upd(ptp_clock_t *ptpClock, u32 slaveIndex);

i16 masterTableEntryFind(UnicastMasterTable_t *list, u32 ip);

i16 slaveTableEntryFind(UnicastSlaveTable_t *list, u32 ip);
i16 slaveTableEntryFindClockId(UnicastSlaveTable_t *list, vtss_appl_ptp_port_identity *id);

/**
 * \brief Read clock slave-master communication table
 * Purpose: To obtain information regarding the clock's current slave table
 * \param ptp       The PTP instance data.
 * \param uni_slave_table [OUT]  pointer to a structure containing the table for
 *                  the slave-master communication.
 * \param ix        The index in the slave table.
 */
mesa_rc vtss_ptp_clock_unicast_table_get(const ptp_clock_t *ptp, vtss_appl_ptp_unicast_slave_table_t *uni_slave_table, int ix);

mesa_rc vtss_ptp_clock_status_unicast_master_table_get(vtss_ptp_master_table_key_t key, vtss_appl_ptp_unicast_master_table_t *uni_master_table);

/**
 * \brief Read clock master-slave communication table
 * Purpose: To obtain information regarding the clock's current master table
 * \param ptp       The PTP instance data.
 * \param uni_master_table [OUT]  pointer to a structure containing the table for
 *                  the master-slave communication.
 * \param slave     The index in the master table.
 */
mesa_rc vtss_ptp_clock_unicast_master_table_get(vtss_ptp_master_table_key_t key, UnicastMasterTable_t **uni_master_table);

void vtss_ptp_master_table_traverse(void);
mesa_rc vtss_appl_ptp_clock_slave_itr_get(const vtss_ptp_master_table_key_t *const prev, vtss_ptp_master_table_key_t *const next);

void vtss_ptp_cancel_unicast_master(ptp_clock_t *ptpClock, PtpPort_t *ptpPort);

// Delete unicast master.
void vtss_ptp_unicast_master_delete(UnicastMasterTable_t *master);
#endif
