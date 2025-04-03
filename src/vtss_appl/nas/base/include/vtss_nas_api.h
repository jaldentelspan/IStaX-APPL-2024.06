/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \brief NAS API
 * \details This header file defines the public types needed by both the
 *          platform-specific code and the platform-specific code's own
 *          API.
 */

#ifndef _VTSS_NAS_API_H_
#define _VTSS_NAS_API_H_

#if defined(VTSS_SW_OPTION_NAS_DOT1X_SINGLE) || defined(VTSS_SW_OPTION_NAS_DOT1X_MULTI) || defined(VTSS_SW_OPTION_NAS_MAC_BASED)
// At least one mode that uses the MAC table (and thereby the PSEC module) is defined
#define NAS_USES_PSEC
#else
#undef  NAS_USES_PSEC
#endif

#if defined(VTSS_SW_OPTION_NAS_DOT1X_MULTI) || defined(VTSS_SW_OPTION_NAS_MAC_BASED)
// At least one mode that allows more than one client attached to the port
#define NAS_MULTI_CLIENT
#else
#undef  NAS_MULTI_CLIENT
#endif

#if defined(VTSS_SW_OPTION_NAS_DOT1X_SINGLE) || defined(VTSS_SW_OPTION_NAS_DOT1X_MULTI)
#define NAS_DOT1X_SINGLE_OR_MULTI
#else
#undef  NAS_DOT1X_SINGLE_OR_MULTI
#endif

#if defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_RFC4675) || defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS_CUSTOM)
#define VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
#else
#undef  VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_QOS
#endif

#if defined(VTSS_SW_OPTION_NAS_BACKEND_ASSIGNED_VLAN) || defined(VTSS_SW_OPTION_NAS_GUEST_VLAN)
#define NAS_USES_VLAN
#else
#undef  NAS_USES_VLAN
#endif

#include <time.h>        /* For time_t               */
#include "main_types.h"  /* For uXXX, iXXX, and BOOL */
#include "microchip/ethernet/switch/api.h"    /* For mesa_vid_mac_t       */
#include "dot1x_api.h" // For NAS management functions



/******************************************************************************/
// NAS_PORT_CONTROL_IS_SINGLE_CLIENT()
// Useful "macro" to determine whether at most one client is allowed to be
// attached on a port.
/******************************************************************************/
static inline BOOL NAS_PORT_CONTROL_IS_SINGLE_CLIENT(vtss_appl_nas_port_control_t admin_state)
{
    if (0
#ifdef VTSS_SW_OPTION_NAS_DOT1X_SINGLE
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_DOT1X_SINGLE
#endif
#ifdef VTSS_SW_OPTION_DOT1X
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_AUTO
#endif
       ) {
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************/
// NAS_PORT_CONTROL_IS_MULTI_CLIENT()
// Useful "macro" to determine if a given admin state allows more than one
// MAC address on the port.
/******************************************************************************/
static inline BOOL NAS_PORT_CONTROL_IS_MULTI_CLIENT(vtss_appl_nas_port_control_t admin_state)
{
    if (0
#ifdef VTSS_SW_OPTION_NAS_DOT1X_MULTI
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_DOT1X_MULTI
#endif
#ifdef VTSS_SW_OPTION_NAS_MAC_BASED
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_MAC_BASED
#endif
       ) {
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************/
// NAS_PORT_CONTROL_IS_MAC_TABLE_BASED()
// Useful "macro" to determine if a given admin state requires the MAC table
// (i.e. the PSEC module) to store info about clients.
/******************************************************************************/
static inline BOOL NAS_PORT_CONTROL_IS_MAC_TABLE_BASED(vtss_appl_nas_port_control_t admin_state)
{
    if (0
#ifdef VTSS_SW_OPTION_NAS_DOT1X_SINGLE
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_DOT1X_SINGLE
#endif
#ifdef VTSS_SW_OPTION_NAS_DOT1X_MULTI
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_DOT1X_MULTI
#endif
#ifdef VTSS_SW_OPTION_NAS_MAC_BASED
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_MAC_BASED
#endif
       ) {
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************/
// NAS_PORT_CONTROL_IS_BPDU_BASED()
// Useful "macro" to determine if a given admin state requires BPDUs or not.
/******************************************************************************/
static inline BOOL NAS_PORT_CONTROL_IS_BPDU_BASED(vtss_appl_nas_port_control_t admin_state)
{
    if (0
#ifdef VTSS_SW_OPTION_DOT1X
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_AUTO
#endif
#ifdef VTSS_SW_OPTION_NAS_DOT1X_SINGLE
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_DOT1X_SINGLE
#endif
#ifdef VTSS_SW_OPTION_NAS_DOT1X_MULTI
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_DOT1X_MULTI
#endif
       ) {
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************/
// NAS_PORT_CONTROL_IS_MAC_TABLE_AND_BPDU_BASED()
// Useful "macro" to determine whether a given admin state requires BPDUs and
// is MAC-table based or not.
/******************************************************************************/
static inline BOOL NAS_PORT_CONTROL_IS_MAC_TABLE_AND_BPDU_BASED(vtss_appl_nas_port_control_t admin_state)
{
    if (0
#ifdef VTSS_SW_OPTION_NAS_DOT1X_SINGLE
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_DOT1X_SINGLE
#endif
#ifdef VTSS_SW_OPTION_NAS_DOT1X_MULTI
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_DOT1X_MULTI
#endif
       ) {
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************/
// NAS_PORT_CONTROL_IS_ACCOUNTABLE()
// Useful "macro" to determine whether a given admin state requires accounting
// or not.
/******************************************************************************/
static inline BOOL NAS_PORT_CONTROL_IS_ACCOUNTABLE(vtss_appl_nas_port_control_t admin_state)
{
    if (0
#ifdef VTSS_SW_OPTION_DOT1X
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_AUTO
#endif
#ifdef VTSS_SW_OPTION_NAS_DOT1X_SINGLE
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_DOT1X_SINGLE
#endif
#ifdef VTSS_SW_OPTION_NAS_DOT1X_MULTI
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_DOT1X_MULTI
#endif
#ifdef VTSS_SW_OPTION_NAS_MAC_BASED
        || admin_state == VTSS_APPL_NAS_PORT_CONTROL_MAC_BASED
#endif
       ) {
        return TRUE;
    }
    return FALSE;
}

#endif /* _VTSS_NAS_API_H__ */
