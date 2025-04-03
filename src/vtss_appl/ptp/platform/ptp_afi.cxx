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

/* Implement the Jaguar 2 AFI feature for PTP */

#include "main.h"
#include "ptp.h"
#include "ptp_afi.h"
#include "packet_api.h"
#include "ptp_api.h"

#if defined(VTSS_SW_OPTION_AFI)
#include "afi_api.h"
#endif

#define API_INST_DEFAULT PHY_INST
#define NO_OF_SEQUENCE_COUNTERS 256

static ptp_afi_conf_s ptp_afi[NO_OF_SEQUENCE_COUNTERS];

static bool sec_cntr_allocated [NO_OF_SEQUENCE_COUNTERS];


#if defined(VTSS_SW_OPTION_AFI)
static u64 logSyncInterval2fph(i8 logsyv)
{
    u64 interval [] = {128 * 3600LLU, 64 * 3600LLU, 32 * 3600LLU, 16 * 3600LLU, 8 * 3600LLU, 4 * 3600LLU, 2 * 3600LLU, 3600LLU, 1800LLU, 900LLU, 450LLU, 225LLU};
    int index = logsyv + 7;
    int tablesize = sizeof(interval)/sizeof(u64);
    if (index < 0) index = 0;
    if (index >= tablesize)  index = tablesize-1;
    return interval[index];
}
#endif

/******************************************************************************/
// ptp_afi_conf_s constructor
// Initialize the internal data structure for handling the Jr2 AFI feature
//
/******************************************************************************/
ptp_afi_conf_s::ptp_afi_conf_s(void)
{
    my_seq_cntr = -1;
}

ptp_afi_conf_s::ptp_afi_conf_s(u32 idx, u32 port_no) : single_id(0), active(false)
{
    my_seq_cntr = idx;
    my_port_no = port_no;
}


mesa_rc ptp_afi_alloc(ptp_afi_conf_s **afi, mesa_port_no_t port_no)
{
    u32 idx;
    for (idx = 0; idx < NO_OF_SEQUENCE_COUNTERS; ++idx) {
        if (!sec_cntr_allocated [idx]) {
            sec_cntr_allocated [idx] = true;
            *afi = new (ptp_afi + idx) ptp_afi_conf_s(idx, port_no);
            return VTSS_RC_OK;
        }
    }
    *afi = 0;
    return VTSS_RC_ERROR;
}

mesa_rc ptp_afi_free(ptp_afi_conf_s **afi)
{
    if (*afi != 0) {
        VTSS_ASSERT((*afi)->my_seq_cntr >= 0 && (*afi)->my_seq_cntr < NO_OF_SEQUENCE_COUNTERS);
        sec_cntr_allocated [(*afi)->my_seq_cntr] = false;
        (*afi)->my_seq_cntr = -1;
        *afi = 0;
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// ptp_afi_available()
//
// check if at least 'afi_request' number of AFI instances are available.
//Pseudocode:
// Start from the ent of the allocated list to check if any entried are available.
// The allocation is done from the baginning of the table, so this optimizes the number of iterations.
// If 'afi_request' is 0 then always return true.
//
/******************************************************************************/
mesa_rc ptp_afi_available(u32 afi_request, bool *available)
{
    u32 free = 0;
    i32 idx;
    for (idx = NO_OF_SEQUENCE_COUNTERS - 1; idx >= 0 ; --idx) {
        if (!sec_cntr_allocated [idx]) {
            ++free;
        }
        if (free >= afi_request) {
            *available = true;
            return VTSS_RC_OK;
        }
    }
    *available = false;
    return VTSS_RC_OK;
}

/******************************************************************************/
// ptp_afi_sync_conf_start()
//
// Uses AFI API configuration (afi_api.h)
//Pseudocode:
// Allocate resources identified by a 'single_id' using afi_single_alloc().
// Transmit frame with IFH.AFI_INJ=1, and rewriter Action:Fill in sequence number and increment.
//  If timestamping is done in the switch:
//      Set rewriter Action: Time of day.
//  If timestamping is done in the phy:
//      Set up PHY TS Action: BC with originTime option enabled.
// Start frame injection
//
/******************************************************************************/
mesa_rc ptp_afi_conf_s::ptp_afi_sync_conf_start(mesa_port_no_t port_no, ptp_tx_buffer_handle_t *ptp_buf_handle, const ptp_afi_setup_t *conf)
{
    mesa_rc rc = VTSS_RC_OK;
#if defined(VTSS_SW_OPTION_AFI)
    afi_single_conf_t afi_conf;
    T_DG(_I, "port_no: %d , active %d, single %d, my_seq_counter %d", port_no, active, single_id, my_seq_cntr);
    VTSS_ASSERT(my_seq_cntr >= 0 && my_seq_cntr < NO_OF_SEQUENCE_COUNTERS);
    if (active) {
        T_IG(_I, "Already active");
        rc = ptp_afi_sync_conf_stop();
        if (rc != VTSS_RC_OK) {
            T_WG(_I, "error returned from ptp_afi_sync_conf_stop");
            return rc;
        }
    }
    rc = afi_single_conf_init(&afi_conf);
    //T_WG(_I, "port_no: %d single %d, rc %s", port_no, single_id, error_txt(rc));

    afi_conf.tx_props.packet_info.modid     = VTSS_MODULE_ID_PTP;
    afi_conf.tx_props.packet_info.frm       = ptp_buf_handle->frame;
    afi_conf.tx_props.packet_info.len       = ptp_buf_handle->size;
    afi_conf.tx_props.tx_info.dst_port_mask = 1LL<<port_no;
    afi_conf.tx_props.tx_info.tag.tpid = 0;  /* In Serval or Jaguar2: must be 0 otherwise the FDMA inserts a tag, and the so do the Switch */
    afi_conf.tx_props.tx_info.tag.vid =  ptp_buf_handle->tag.vid;
    afi_conf.tx_props.tx_info.tag.pcp =  ptp_buf_handle->tag.pcp;
    afi_conf.tx_props.tx_info.pdu_offset = ptp_buf_handle->header_length;  /* Jaguar 2 needs to know the PTP header offset within the packet */
    afi_conf.tx_props.tx_info.ptp_domain = conf->clk_domain;
    T_IG(_I, "Tag tpid: %x , vid %d , pcp %d, offset %u", afi_conf.tx_props.tx_info.tag.tpid, afi_conf.tx_props.tx_info.tag.vid, afi_conf.tx_props.tx_info.tag.pcp, afi_conf.tx_props.tx_info.pdu_offset);
    if (conf->switch_port) {
        if (ptp_buf_handle->msg_type == VTSS_PTP_MSG_TYPE_GENERAL) {
            afi_conf.tx_props.tx_info.ptp_action = MESA_PACKET_PTP_ACTION_AFI_NONE; /* No PTP action, but AFI automatic sequence number update enabled */
        } else {
            afi_conf.tx_props.tx_info.ptp_action = MESA_PACKET_PTP_ACTION_ORIGIN_TIMESTAMP_SEQ; /* origin PTP action */
        }
    } else {
        afi_conf.tx_props.tx_info.ptp_action = MESA_PACKET_PTP_ACTION_AFI_NONE; /* PHY inserts originTimestamp: i.e. No PTP action, but AFI automatic sequence number update enabled */
    }
    T_DG(_I, "port_no: %d single %d, my_seq_cntr %d", port_no, single_id, my_seq_cntr);
    afi_conf.tx_props.tx_info.ptp_timestamp = my_seq_cntr; /* used as sequence number counter index */
    afi_conf.params.fph = logSyncInterval2fph(conf->log_sync_interval);
    T_DG(_I, "fph " VPRI64u " log_sync_interval %d", afi_conf.params.fph, conf->log_sync_interval);
    afi_conf.params.jitter = AFI_SINGLE_JITTER_NONE;
    T_DG(_I, "ptp_action %d , fps " VPRI64u ", jitter %d, counter idx %d", afi_conf.tx_props.tx_info.ptp_action, afi_conf.params.fph/ 3600, afi_conf.params.jitter, my_seq_cntr);
    rc = afi_single_alloc(&afi_conf, &single_id);
    //T_WG(_I, "port_no: %d single %d, rc %s", port_no, single_id, error_txt(rc));
    if ((rc = afi_single_pause_resume(single_id, FALSE)) != VTSS_RC_OK) {
        //T_WG(_I, "port_no: %d single %d, rc %s", port_no, single_id, error_txt(rc));
        PTP_RC(afi_single_free(single_id, NULL));
        return rc;
    }
    active = TRUE;

#else
    rc = VTSS_RC_ERROR;
#endif
    return rc;
}

/******************************************************************************/
// ptp_afi_sync_conf_stop()
//
// Uses AFI API configuration (afi_slow_inj_xxx)
//Pseudocode:
// Stop frame injection
// Free allocated resources identified by a 'single_id' using afi_single_free().
//
//
/******************************************************************************/
mesa_rc ptp_afi_conf_s::ptp_afi_sync_conf_stop(void)
{
    mesa_rc rc = VTSS_RC_OK;
#if defined(VTSS_SW_OPTION_AFI)
    T_IG(_I, "active %d, single %d", active, single_id);
    VTSS_ASSERT(my_seq_cntr >= 0 && my_seq_cntr < NO_OF_SEQUENCE_COUNTERS);
    if (active) {
        rc = afi_single_pause_resume(single_id, TRUE);
        //T_WG(_I, "single %d, rc %s", single_id, error_txt(rc));
        PTP_RC(afi_single_free(single_id, NULL));
        active = FALSE;
        T_IG(_I, "stop counter idx %d", my_seq_cntr);
    } else {
        T_IG(_I, "Not active");
    }
#else
    rc = VTSS_RC_ERROR;
#endif
    return rc;
}

mesa_rc ptp_afi_conf_s::ptp_afi_packet_tx(u32 *cnt_delta)
{
    mesa_rc rc = MESA_RC_OK;
#if defined(VTSS_SW_OPTION_AFI)
    u16 tx_cnt;
    u16 delta;
    if (active) {
        rc = mesa_ts_seq_cnt_get(NULL, (u32)my_port_no, &tx_cnt);
        // in 16 bit unsigned arithmetic, the delta calculation is correct also when the counter wraps
        delta = tx_cnt - my_tx_cnt;
        *cnt_delta = (u32) (delta);
        my_tx_cnt = tx_cnt;
    } else {
        rc = MESA_RC_ERROR;
    }
#else
    rc = MESA_RC_ERROR;
#endif
    return rc;
}
