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

#include "tsn_serializer.hxx"
#include "tsn_fp_serializer.hxx"
#include "tsn_tas_serializer.hxx"

const vtss_enum_descriptor_t vtss_appl_tsn_mm_status_verify_txt[] = {
    {VTSS_APPL_TSN_MM_STATUS_VERIFY_INITIAL,   "initial"},
    {VTSS_APPL_TSN_MM_STATUS_VERIFY_IDLE,      "idle"},
    {VTSS_APPL_TSN_MM_STATUS_VERIFY_SEND,      "send"},
    {VTSS_APPL_TSN_MM_STATUS_VERIFY_WAIT,      "wait"},
    {VTSS_APPL_TSN_MM_STATUS_VERIFY_SUCCEEDED, "succeeded"},
    {VTSS_APPL_TSN_MM_STATUS_VERIFY_FAILED,    "failed"},
    {VTSS_APPL_TSN_MM_STATUS_VERIFY_DISABLED,  "disabled"},
    {VTSS_APPL_TSN_MM_STATUS_VERIFY_UNKNOWN,   "indeterminate"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_tsn_tas_gco_txt[] = {
    {MESA_QOS_TAS_GCO_SET_GATE_STATES,          "set"},
    {MESA_QOS_TAS_GCO_SET_AND_HOLD_MAC,         "setAndHold"},
    {MESA_QOS_TAS_GCO_SET_AND_RELEASE_MAC,      "setAndRelease"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_tsn_procedure_txt[] = {
    {VTSS_APPL_TSN_PROCEDURE_NONE,              "none"},
    {VTSS_APPL_TSN_PROCEDURE_TIME_ONLY,         "timeonly"},
    {VTSS_APPL_TSN_PROCEDURE_TIME_AND_PTP,      "timeAndPtp"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_tsn_start_state_txt[] = {
    {VTSS_APPL_TSN_INITIAL,                     "initial"},
    {VTSS_APPL_TSN_START_IMMEDIATELY,           "immediately"},
    {VTSS_APPL_TSN_WAITING_FOR_TIMEOUT,         "waitingForTimeout"},
    {VTSS_APPL_TSN_TIMED_OUT,                   "timedOut"},
    {VTSS_APPL_TSN_PTP_WAITING_FOR_LOCK,        "waitingForLock"},
    {VTSS_APPL_TSN_PTP_LOCKING,                 "ptpLocking"},
    {VTSS_APPL_TSN_PTP_LOCKED,                  "ptpLocked"},
    {VTSS_APPL_TSN_PTP_TIMED_OUT,               "ptpTimedOut"},
    {0, 0}
};

