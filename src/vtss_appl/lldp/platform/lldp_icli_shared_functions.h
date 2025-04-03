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

/**
 * \file
 * \brief iCLI shared functions by LLDP and LLDP-MED
 * \details This header file describes LLDP shared iCLI functions
 */


#ifndef _VTSS_ICLI_SHARED_LLDP_H_
#define _VTSS_ICLI_SHARED_LLDP_H_

#ifdef __cplusplus
extern "C" {
#endif

VTSS_BEGIN_HDR
/**
 * \brief Function for getting local port as printable text
 *
 * \param  buf   [IN] Pointer to text buffer.
 * \param  entry [IN] Pointer to entry containing the information
 * \param  sit   [IN] Pointer to switch information'
 * \param  pit   [IN] Pointer to port information
 * return  Pointer to text buffer
 **/
char *lldp_local_interface_txt_get(char *buf, const vtss_appl_lldp_remote_entry_t *entry, const switch_iter_t *sit, const port_iter_t *pit);
VTSS_END_HDR

//****************************************************************************
#ifdef __cplusplus
}
#endif

#endif /* _VTSS_ICLI_SHARED_LLDP_H_ */

