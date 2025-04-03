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
#include "frr_rip_serializer.hxx"
#include "vtss/appl/rip.h"

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
//** RIP split horizon
//----------------------------------------------------------------------------
vtss_enum_descriptor_t vtss_appl_rip_split_horizon_mode_txt[] {
    {VTSS_APPL_RIP_SPLIT_HORIZON_MODE_SIMPLE, "splitHorizon"},
    {VTSS_APPL_RIP_SPLIT_HORIZON_MODE_POISONED_REVERSE, "poisonedReverse"},
    {VTSS_APPL_RIP_SPLIT_HORIZON_MODE_DISABLED, "disabled"},
    {0, 0}
};

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------
vtss_enum_descriptor_t vtss_appl_rip_db_proto_type_txt[] {
    {VTSS_APPL_RIP_DB_PROTO_TYPE_RIP, "rip"},
    {VTSS_APPL_RIP_DB_PROTO_TYPE_CONNECTED, "connected"},
    {VTSS_APPL_RIP_DB_PROTO_TYPE_STATIC, "static"},
    {VTSS_APPL_RIP_DB_PROTO_TYPE_OSPF, "ospf"},
    {VTSS_APPL_RIP_DB_PROTO_TYPE_COUNT, "unknown"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_rip_db_proto_subtype_txt[] {
    {VTSS_APPL_RIP_DB_PROTO_SUBTYPE_STATIC, "static"},
    {VTSS_APPL_RIP_DB_PROTO_SUBTYPE_NORMAL, "normal"},
    {VTSS_APPL_RIP_DB_PROTO_SUBTYPE_DEFAULT, "default"},
    {VTSS_APPL_RIP_DB_PROTO_SUBTYPE_REDIST, "redistribute"},
    {VTSS_APPL_RIP_DB_PROTO_SUBTYPE_INTF, "interface"},
    {VTSS_APPL_RIP_DB_PROTO_SUBTYPE_COUNT, "unknown"},
    {0, 0}
};

//----------------------------------------------------------------------------
//** RIP version
//----------------------------------------------------------------------------
vtss_enum_descriptor_t vtss_appl_rip_global_ver_txt[] {
    {VTSS_APPL_RIP_GLOBAL_VER_DEFAULT, "default"},
    {VTSS_APPL_RIP_GLOBAL_VER_1, "v1"},
    {VTSS_APPL_RIP_GLOBAL_VER_2, "v2"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_rip_intf_recv_ver_txt[] {
    {VTSS_APPL_RIP_INTF_RECV_VER_NONE, "none"},
    {VTSS_APPL_RIP_INTF_RECV_VER_1, "v1"},
    {VTSS_APPL_RIP_INTF_RECV_VER_2, "v2"},
    {VTSS_APPL_RIP_INTF_RECV_VER_BOTH, "both"},
    {VTSS_APPL_RIP_INTF_RECV_VER_NOT_SPECIFIED, "notSpecified"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_rip_intf_send_ver_txt[] {
    {VTSS_APPL_RIP_INTF_SEND_VER_1, "v1"},
    {VTSS_APPL_RIP_INTF_SEND_VER_2, "v2"},
    {VTSS_APPL_RIP_INTF_SEND_VER_BOTH, "both"},
    {VTSS_APPL_RIP_INTF_SEND_VER_NOT_SPECIFIED, "notSpecified"},
    {0, 0}
};

vtss_enum_descriptor_t vtss_appl_rip_auth_type_txt[] {
    {VTSS_APPL_RIP_AUTH_TYPE_NULL, "nullAuth"},
    {VTSS_APPL_RIP_AUTH_TYPE_SIMPLE_PASSWORD, "simplePasswordAuth"},
    {VTSS_APPL_RIP_AUTH_TYPE_MD5, "md5Auth"},
    {0, 0}
};

//----------------------------------------------------------------------------
//** RIP metric manipulation: Offset-list
//----------------------------------------------------------------------------
vtss_enum_descriptor_t vtss_appl_rip_offset_direction_txt[] {
    {VTSS_APPL_RIP_OFFSET_DIRECTION_IN, "in"},
    {VTSS_APPL_RIP_OFFSET_DIRECTION_OUT, "out"},
    {0, 0}
};

