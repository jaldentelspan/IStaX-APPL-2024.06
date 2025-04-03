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
#include "psec_util.h"
#include "psec_trace.h"
#include <vtss/basics/enum_macros.hxx>

// Allow operator++ on vtss_appl_psec_user_t
VTSS_ENUM_INC(vtss_appl_psec_user_t);

/**
 * psec_util_users_included_get()
 * This function figures out which PSEC user modules is included in this
 * build, and caches the result, since it never changes throughout the session,
 * and may take a while the first time.
 */
u32 psec_util_users_included_get(void)
{
    static u32                    mask;
    static BOOL                   initialized;
    vtss_appl_psec_capabilities_t cap;
    mesa_rc                       rc;

    if (!initialized) {
        initialized = TRUE;
        if ((rc = vtss_appl_psec_capabilities_get(&cap)) == VTSS_RC_OK) {
            mask = cap.users;
        } else {
            T_E("%s", error_txt(rc));
        }
    }

    return mask;
}

/**
 * psec_util_user_abbr_get()
 * This function returns the abbreviation letter for a PSEC user module. Since
 * this is called frequently and would otherwise be a bit slow, it fetches it
 * once and caches the result.
 */
char psec_util_user_abbr_get(vtss_appl_psec_user_t user)
{
    static char abbr[VTSS_APPL_PSEC_USER_LAST];

    if (abbr[0] == '\0') {
        // Not initialized
        vtss_appl_psec_user_info_t info;
        vtss_appl_psec_user_t      u;
        mesa_rc                    rc;

        for (u = (vtss_appl_psec_user_t)0; u < VTSS_APPL_PSEC_USER_LAST; u++) {
            if ((rc = vtss_appl_psec_user_info_get(u, &info)) != VTSS_RC_OK) {
                T_E("%s", error_txt(rc));
                abbr[u] = 'X';
            } else {
                abbr[u] = info.abbr;
            }
        }
    }

    return user < VTSS_APPL_PSEC_USER_LAST ? abbr[user] : 'X';
}

/**
 * psec_util_user_name_get()
 * This function returns the name of a PSEC user module. Since this is called
 * frequently and would otherwise be a bit slow, it fetches it once and caches
 * the result.
 */
const char *psec_util_user_name_get(vtss_appl_psec_user_t user)
{
    vtss_appl_psec_user_info_t info;
    static char                name[VTSS_APPL_PSEC_USER_LAST][sizeof(info.name)];

    if (name[0][0] == '\0') {
        // Not initialized
        vtss_appl_psec_user_t u;
        mesa_rc               rc;

        for (u = (vtss_appl_psec_user_t)0; u < VTSS_APPL_PSEC_USER_LAST; u++) {
            if ((rc = vtss_appl_psec_user_info_get(u, &info)) != VTSS_RC_OK) {
                T_E("%s", error_txt(rc));
                strncpy(name[u], "Unknown", sizeof(info.name) - 1);
            } else {
                strcpy(name[u], info.name);
            }
        }
    }

    return user < VTSS_APPL_PSEC_USER_LAST ? name[user] : "Unknown";
}

/**
 * psec_util_user_abbr_str_populate()
 * Given a users mask, it populates a string of at least
 * VTSS_APPL_PSEC_USER_LAST bytes with '-' if not included in users mask and
 * PSEC user module abbreviation letter if included.
 * It returns a pointer to \p buf.
 */
char *psec_util_user_abbr_str_populate(char *buf, u32 users_mask)
{
    u32                   included_in_build_mask;
    vtss_appl_psec_user_t user;
    char                  *p = buf;

    // Get users included in this build.
    included_in_build_mask = psec_util_users_included_get();

    for (user = (vtss_appl_psec_user_t)0; user < VTSS_APPL_PSEC_USER_LAST; user++) {
        if (!(included_in_build_mask & VTSS_BIT(user))) {
            // This user is not included in this build. Nothing to populate
            continue;
        }

        *(p++) = (users_mask & VTSS_BIT(user)) ? psec_util_user_abbr_get(user) : '-';
    }

    *p = '\0';

    return buf;
}

/******************************************************************************/
// psec_util_mac_type_to_str()
/******************************************************************************/
const char *psec_util_mac_type_to_str(vtss_appl_psec_mac_type_t mac_type)
{
    switch (mac_type) {
    case VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC:
        return "Dynamic";
    case VTSS_APPL_PSEC_MAC_TYPE_STATIC:
        return "Static";
    case VTSS_APPL_PSEC_MAC_TYPE_STICKY:
        return "Sticky";
    default:
        T_E("Unknown mac-type (%d)", mac_type);
        return "Unknown";
    }
}

