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

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr_router_serializer.hxx"
#include "vtss/appl/router.h"

/******************************************************************************/
/** Enum descriptor text mapping                                              */
/******************************************************************************/
/* Notice that the naming rule of enumeration should refer to
 * RFC 2578 - Structure of Management Information Version 2 (SMIv2).
 * Section 7.1.4.  The BITS construct
 *      A label for a named-number enumeration must consist of one
 *      or more letters or digits, up to a maximum of 64 characters, and the
 *      initial character must be a lower-case letter.  (However, labels
 *      longer than 32 characters are not recommended.)  Note that hyphens
 *      are not allowed by this specification.
 */

//----------------------------------------------------------------------------
//** Access-list
//----------------------------------------------------------------------------
vtss_enum_descriptor_t vtss_appl_router_access_list_mode_txt[] {
    {VTSS_APPL_ROUTER_ACCESS_LIST_MODE_DENY, "deny"},
    {VTSS_APPL_ROUTER_ACCESS_LIST_MODE_PERMIT, "permit"},
    {0, 0}
};

