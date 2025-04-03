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

#ifndef _VTSS_SYNCE_API_H_
#define _VTSS_SYNCE_API_H_

#include "vtss/appl/synce.h"

#ifdef __cplusplus
extern "C" {
#endif


/****************************************************************************/
// API Error Return Codes (mesa_rc)
/****************************************************************************/

typedef struct
{
    bool                             top_selected;   /* Time over packet selected as source */
    vtss_appl_synce_quality_level_t  ql;             /* quality of the selected clock */
} synce_mgmt_clock_top_state_t;


/******************************************************************************
 * Description: Get station clock time over packet selector state
 *
 * \param top_state (OUT)   : Synce time over packet selector state
 * \return : Return code.
 ******************************************************************************/
mesa_rc synce_mgmt_clock_top_state_get(synce_mgmt_clock_top_state_t *top_state);

/******************************************************************************
 * Description: Set frequency adjust value
 *
 * \param adj [IN]          : Clock ratio frequency offset in units of scaled ppb
 *                            (parts pr billion) i.e. ppb*2*-16. 
 *                            ratio > 0 => clock runs faster. 
 * \return : Return code.
 ******************************************************************************/
mesa_rc synce_mgmt_clock_adjtimer_set(i64 adj);

/******************************************************************************
 * Description: Get current synce clock quality
 *
 * \param ql [OUT]          : Current SyncE quality level.
 * \return : Return code.
 ******************************************************************************/
mesa_rc synce_mgmt_clock_ql_get(vtss_appl_synce_quality_level_t *ql);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_SYNCE_H_ */

