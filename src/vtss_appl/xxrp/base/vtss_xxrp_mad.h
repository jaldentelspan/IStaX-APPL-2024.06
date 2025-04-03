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
/**
 * \file
 * \brief This file contains MAD structure definitions and MAD functions to manipulate MAD database.
 */
#ifndef _VTSS_XXRP_MAD_H_
#define _VTSS_XXRP_MAD_H_

#include "vtss_xxrp_types.h"
#include "l2proto_api.h" /* For L2_MAX_PORTS */
#include <vtss/appl/types.hxx>

/**
 * \brief function to create MAD structure of a port.
 *
 * \parm map_ports    [IN]  Application-specific MAD ports
 * \param port_no     [IN]  Port number.
 *
 * \return Return error code.
 **/
mesa_rc vtss_mrp_mad_create_port(vtss_mrp_t *appl, vtss_mrp_mad_t **mad_ports, u32 l2port, vtss::VlanList &vls);

/**
 * \brief function to destroy(free) MAD structure of a port.
 *
 * \parm map_ports    [IN]  Application-specific MAD ports
 * \param port_no     [IN]  Port number.
 *
 * \return Return error code.
 **/
mesa_rc vtss_mrp_mad_destroy_port(vtss_mrp_t *appl, vtss_mrp_mad_t **mad_ports, u32 l2port);
#endif /* _VTSS_XXRP_MAD_H_ */
