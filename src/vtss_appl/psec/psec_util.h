/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <vtss/appl/psec.h>

#ifndef __PSEC_UTIL_H__
#define __PSEC_UTIL_H__

/**
 * psec_util_users_included_get()
 * This function figures out which PSEC user modules is included in this
 * build, and caches the result, since it never changes throughout the session,
 * and may take a while the first time.
 */
u32 psec_util_users_included_get(void);

/**
 * psec_util_user_abbr_get()
 * This function returns the abbreviation letter for a PSEC user module. Since
 * this is called frequently and would otherwise be a bit slow, it fetches it
 * once and caches the result.
 */
char psec_util_user_abbr_get(vtss_appl_psec_user_t user);

/**
 * psec_util_user_name_get()
 * This function returns the name of a PSEC user module. Since this is called
 * frequently and would otherwise be a bit slow, it fetches it once and caches
 * the result.
 */
const char *psec_util_user_name_get(vtss_appl_psec_user_t user);

/**
 * psec_util_user_abbr_str_populate()
 * Given a users mask, it populates a string of at least
 * VTSS_APPL_PSEC_USER_LAST bytes with '-' if not included in users mask and
 * PSEC user module abbreviation letter if included.
 * It returns a pointer to \p buf.
 */
char *psec_util_user_abbr_str_populate(char *buf, u32 users_mask);

/**
 * psec_util_mac_type_to_str()
 * Converts a vtss_appl_psec_mac_type_t to a string.
 */
const char *psec_util_mac_type_to_str(vtss_appl_psec_mac_type_t mac_type);

#endif /* __PSEC_UTIL_H__ */

