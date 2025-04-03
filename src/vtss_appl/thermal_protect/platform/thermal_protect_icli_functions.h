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

/**
 * \file
 * \brief mirror icli functions
 * \details This header file describes PHY control functions
 */


#ifndef _VTSS_ICLI_THERMAL_PROTECT_H_
#define _VTSS_ICLI_THERMAL_PROTECT_H_

#include "icli_api.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * \brief Function for showing thermal_protect status (chip temperature and port status)
 *
 * \param session_id [IN]  The session id.
 * \param interface [IN]  TRUE if user has specified a specific interface(s)
 * \param port_list [IN]  Port list
 * \return None.
 **/
void thermal_protect_status(i32 session_id, BOOL interface, icli_stack_port_range_t *port_list);


/**
 * \brief Function for setting thermal_protect temperature for a specific group
 *
 *
 * \param grp_list [IN]  List of groups
 * \param new_temp [IN] New temperature for when to shut port down for the given group(is)
 * \param no [IN] TRUE if the no command is used
 * \return Error code.
 **/
mesa_rc thermal_protect_temp(i32 session_id, icli_unsigned_range_t *grp_list, i16 new_temp, BOOL no);


/**
 * \brief Function for setting group for ports
 *
 *
 * \param grp_list [IN]  Port list
 * \param grp [IN] Group to be set for the ports in the port list.
 * \param no [IN] TRUE if the no command is used
 * \return None.
 **/
void thermal_protect_grp(icli_stack_port_range_t *port_list, u8 grp, BOOL no);

#ifdef __cplusplus
}
#endif

/**
 * \brief Init function ICLI CFG
 * \return Error code.
 **/
mesa_rc thermal_protect_icfg_init(void);

#endif /* _VTSS_ICLI_THERMAL_PROTECT_H_ */

