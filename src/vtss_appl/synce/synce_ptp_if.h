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

#ifndef _VTSS_SYNCE_PTP_IF_H_
#define _VTSS_SYNCE_PTP_IF_H_

#include "vtss/appl/synce.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint ssm_rx;
    vtss_appl_synce_ptp_ptsf_state_t ptsf;
} synce_ptp_port_state_t;

mesa_rc vtss_synce_ptp_port_state_get(const uint idx, synce_ptp_port_state_t *state);

mesa_rc vtss_synce_ptp_clock_ssm_ql_set(const uint idx, u8 clockClass);

const char *vtss_sync_ptsf_state_2_txt(vtss_appl_synce_ptp_ptsf_state_t s);
mesa_rc vtss_synce_ptp_clock_ptsf_state_set(const uint idx, vtss_appl_synce_ptp_ptsf_state_t ptsfState);
mesa_rc vtss_synce_ptp_clock_hybrid_mode_set(BOOL hybrid);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_SYNCE_PTP_IF_H_ */

