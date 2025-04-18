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

#ifndef _FRR_RIP_EXPOSE_HXX_
#define _FRR_RIP_EXPOSE_HXX_

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "vtss/appl/rip.h"
#include "vtss/basics/expose.hxx"

/******************************************************************************/
/** Enum descriptor text mapping                                              */
/******************************************************************************/
//----------------------------------------------------------------------------
//** RIP split horizon
//----------------------------------------------------------------------------
extern vtss_enum_descriptor_t vtss_appl_rip_split_horizon_mode_txt[];

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------
extern vtss_enum_descriptor_t vtss_appl_rip_db_proto_type_txt[];
extern vtss_enum_descriptor_t vtss_appl_rip_db_proto_subtype_txt[];

//----------------------------------------------------------------------------
//** RIP version
//----------------------------------------------------------------------------
extern vtss_enum_descriptor_t vtss_appl_rip_global_ver_txt[];
extern vtss_enum_descriptor_t vtss_appl_rip_intf_recv_ver_txt[];
extern vtss_enum_descriptor_t vtss_appl_rip_intf_send_ver_txt[];

//----------------------------------------------------------------------------
//** RIP authentication
//----------------------------------------------------------------------------
extern vtss_enum_descriptor_t vtss_appl_rip_auth_type_txt[];

//----------------------------------------------------------------------------
//** RIP metric manipulation: Offset-list
//----------------------------------------------------------------------------
extern vtss_enum_descriptor_t vtss_appl_rip_offset_direction_txt[];

#endif  // _FRR_RIP_EXPOSE_HXX_

