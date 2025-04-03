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

#ifndef _FRR_OSPF_EXPOSE_HXX_
#define _FRR_OSPF_EXPOSE_HXX_

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "vtss/appl/ospf.h"
#include "vtss/basics/expose.hxx"

/******************************************************************************/
/** Enum descriptor text mapping                                              */
/******************************************************************************/
extern vtss_enum_descriptor_t vtss_appl_ospf_auth_type_txt[];
extern vtss_enum_descriptor_t vtss_appl_ospf_interface_state_txt[];
extern vtss_enum_descriptor_t vtss_appl_ospf_neighbor_state_txt[];
extern vtss_enum_descriptor_t vtss_appl_ospf_area_type_txt[];
extern vtss_enum_descriptor_t vtss_appl_ospf_redist_metric_type_txt[];
extern vtss_enum_descriptor_t vtss_appl_ospf_nssa_translator_role_text[];
extern vtss_enum_descriptor_t vtss_appl_ospf_nssa_translator_state_text[];
extern vtss_enum_descriptor_t vtss_appl_ospf_route_type_txt[];
extern vtss_enum_descriptor_t vtss_appl_ospf_route_border_router_type_txt[];
extern vtss_enum_descriptor_t vtss_appl_ospf_lsdb_type_txt[];

#endif  // _FRR_OSPF_EXPOSE_HXX_

