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

#include "vtss_ptp_api.h"
#include "vtss_ptp_os.h"
#include "vtss_ptp_path_trace.hxx"
#include "vtss_ptp_pack_unpack.h"

/*
 * Forward declarations
 */

/**
 * \brief process the Path Trace TLV received in an Announce message.
 *
 * \param tlv [IN]  the received Path Trace Tlv
 * \param path_sequence  [OUT]  the received path trace.
 * \return void
 **/
mesa_rc vtss_ptp_tlv_path_trace_process(TLV *tlv, ptp_path_trace_t *path_sequence)
{
    int path_no;
    mesa_rc rc = VTSS_RC_OK;
    switch (tlv->tlvType) {
        case TLV_PATH_TRACE:
            path_sequence->size = tlv->lengthField / sizeof(vtss_appl_clock_identity);
            for (path_no = 0; path_no < path_sequence->size; path_no++) {
                memcpy(&path_sequence->pathSequence[path_no], tlv->valueField + sizeof(vtss_appl_clock_identity) * path_no, sizeof(vtss_appl_clock_identity));
            }
            break;
        default:
            T_IG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "Unknown signalling TLV type received: tlvType %d", tlv->tlvType);
            rc = VTSS_RC_ERROR;
            break;
    }
    return rc;
}

/**
 * \brief check if the Path Trace TLV shows a loop, i.e. if the local clockIdentity is included in the received path trace.
 *
 * \param clockIdentity [IN]  the local clock identity
 * \param path_sequence  [IN]  the received path trace.
 * \return true if loop detected, false otherwise.
 **/
bool vtss_ptp_path_trace_loop_check(vtss_appl_clock_identity clockIdentity, ptp_path_trace_t *path_sequence)
{
    int i;
    for (i = 0; i < path_sequence->size; i++) {
        if (memcmp(clockIdentity, path_sequence->pathSequence[i], sizeof(vtss_appl_clock_identity)) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * \brief insert the Path Trace TLV in the tx buffer. I.e. insert received path trace list and append local clock identity to the list.
 *
 * \param tx_buf [OUT]  the transmit buffer
 * \param buflen  [IN]  length of the transmit buffer.
 * \param path_sequence  [IN]  the received path trace.
 * \param clockIdentity [IN]  the local clock identity
 * \return size ot the TLV
 **/
size_t vtss_ptp_tlv_path_trace_insert(u8 *tx_buf, size_t buflen, ptp_path_trace_t *path_sequence, vtss_appl_clock_identity   clockIdentity)
{
    int i;
    TLV tlv;
    u8 tlv_value[PTP_PATH_TRACE_MAX_SIZE * sizeof(vtss_appl_clock_identity)];
    /* Insert TLV field */
    tlv.tlvType = TLV_PATH_TRACE;
    tlv.lengthField = sizeof(vtss_appl_clock_identity) * (path_sequence->size + 1);
    tlv.valueField = tlv_value;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "buflen " VPRIz ", path_sequence size %d", buflen, path_sequence->size);
    if (path_sequence->size < PTP_PATH_TRACE_MAX_SIZE) {
        for (i = 0; i < path_sequence->size; i++) {
            memcpy(tlv_value + i * sizeof(vtss_appl_clock_identity), path_sequence->pathSequence[i], sizeof(vtss_appl_clock_identity));
        }
        memcpy(tlv_value + i * sizeof(vtss_appl_clock_identity), clockIdentity, sizeof(vtss_appl_clock_identity));
        if (VTSS_RC_OK == vtss_ptp_pack_tlv(tx_buf, buflen, &tlv)) {
            return 4 + tlv.lengthField;
        }
    } else if (path_sequence->size == PTP_PATH_TRACE_MAX_SIZE) {
        tlv.lengthField = sizeof(vtss_appl_clock_identity) * (path_sequence->size);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "Path trace has reached size limit");
        for (i = 0; i < path_sequence->size; i++) {
            memcpy(tlv_value + i * sizeof(vtss_appl_clock_identity), path_sequence->pathSequence[i], sizeof(vtss_appl_clock_identity));
        }
        if (VTSS_RC_OK == vtss_ptp_pack_tlv(tx_buf, buflen, &tlv)) {
            return 4 + tlv.lengthField;
        }
    } else {
        T_WG(VTSS_TRACE_GRP_PTP_BASE_802_1AS, "Path trace size too big");
    }
    return 0;
}

bool vtss_ptp_tlv_path_trace_check(ptp_path_trace_t *path_trace_a, ptp_path_trace_t *path_trace_b)
{
    if (path_trace_a->size != path_trace_b->size) {
        return true;
    }
    return (memcmp(path_trace_a->pathSequence, path_trace_b->pathSequence, path_trace_a->size * sizeof(vtss_appl_clock_identity)) != 0);
}
