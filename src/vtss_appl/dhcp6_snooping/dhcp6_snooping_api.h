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

#ifndef _DHCP6_SNOOPING_API_H_
#define _DHCP6_SNOOPING_API_H_

#include "vtss/appl/dhcp6_snooping.h"
#include "dhcp6_snooping_expose.h"

/**
 * \file dhcp6_snooping_api.h
 * \brief This file defines the internal API for the DHCPv6 Snooping module
 */

/**
  * \brief Initialize the DHCPv6 Snooping module. Called from the main module.
  *
  * \param data [IN]: Initial data point.
  *
  * \return
  *    VTSS_RC_OK.
  */
mesa_rc dhcp6_snooping_init(vtss_init_data_t *data);

/**
  * \brief Internal get-next DHCP snooping registered clients entry
  *
  * \return
  *   VTSS_RC_OK on success.\n
  *   Anything else if get fails.\n
  */
mesa_rc dhcp6_snooping_registered_clients_info_getnext(const dhcp_duid_t *const prev,
                                                       dhcp_duid_t       *const next,
                                                       registered_clients_info_t *info);

#endif /* _DHCP6_SNOOPING_API_H_ */

