/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _DOT1X_ACCT_H_
#define _DOT1X_ACCT_H_

#include "vtss_nas_platform_api.h" /* For nas_port_control_t and nas_eap_info_t */

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
// dot1x_acct_authorized_changed()
/****************************************************************************/
void dot1x_acct_authorized_changed(vtss_appl_nas_port_control_t admin_state, struct nas_sm *sm, BOOL authorized);

/****************************************************************************/
// dot1x_acct_radius_rx()
/****************************************************************************/
void dot1x_acct_radius_rx(u8 radius_handle, nas_eap_info_t *eap_info);

/****************************************************************************/
// dot1x_acct_append_radius_tlv()
/****************************************************************************/
BOOL dot1x_acct_append_radius_tlv(u8 radius_handle, nas_eap_info_t *eap_info);

/****************************************************************************/
// dot1x_acct_init()
/****************************************************************************/
void dot1x_acct_init(void);

#ifdef __cplusplus
}
#endif
#endif /* _DOT1X_ACCT_H_ */
