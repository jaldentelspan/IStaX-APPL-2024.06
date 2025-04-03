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
#ifndef _VTSS_XXRP_MAP_H_
#define _VTSS_XXRP_MAP_H_

#include "vtss_xxrp_types.h"
#include "vtss_xxrp_api.h"

mesa_rc vtss_mrp_map_create_port(vtss_mrp_map_t **map_ports, u32 l2port);
mesa_rc vtss_mrp_map_destroy_port(vtss_mrp_map_t **map_ports, u32 l2port);
void vtss_mrp_map_print_msti_ports(vtss_mrp_map_t **map_ports, u8 msti);

/**
 * \brief function to create MAP structure of a port.
 *
 * \parm map_ports    [IN]  Application-specific MAP ports
 * \param msti        [IN]  MSTI instance.
 * \param l2port     [IN]  Port number.
 *
 * \return Return error code.
 **/
mesa_rc vtss_mrp_map_connect_port(vtss_mrp_map_t **map_ports, u8 msti, u32 l2port);

/**
 * \brief function to destroy(free) MAP structure of a port.
 *
 * \parm map_ports    [IN]  Application-specific MAP ports
 * \param msti        [IN]  MSTI instance.
 * \param l2port     [IN]  Port number.
 *
 * \return Return error code.
 **/
mesa_rc vtss_mrp_map_disconnect_port(vtss_mrp_map_t **map_ports, u8 msti, u32 l2port);

/**
 * \brief function to find MAP structure of a port.
 *
 * \parm map_ports    [IN]  Application-specific MAP ports
 * \param msti        [IN]  MSTI instance.
 * \param l2port     [IN]  Port number.
 * \param map         [OUT] pointer to a pointer of MAP structure of a port.
 *
 * \return Return error code.
 **/
mesa_rc vtss_mrp_map_find_port(vtss_mrp_map_t **map_ports, u8 msti, u32 l2port, vtss_mrp_map_t **map);

mesa_rc vtss_mrp_map_propagate_join(vtss_mrp_appl_t appl, vtss_mrp_map_t **map_ports, u32 l2port, u32 mad_indx);
mesa_rc vtss_mrp_map_propagate_leave(vtss_mrp_appl_t appl, vtss_mrp_map_t **map_ports, u32 l2port, u32 mad_indx);
#endif /* _VTSS_XXRP_MAP_H_ */
