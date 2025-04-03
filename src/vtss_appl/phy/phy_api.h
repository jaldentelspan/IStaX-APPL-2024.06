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

#ifndef _PHY_API_H_
#define _PHY_API_H_

#include "main_types.h"
#include "main.h"
#include "microchip/ethernet/switch/api.h"
#include "main_types.h"

#define VTSS_SW_ICLI_PHY_INVISIBLE /* undef 'VTSS_SW_ICLI_PHY_INVISIBLE' to make PHY's module icli cmds visible */
/* To control visibility of PHY config & interface mode icli cmds */
#ifdef VTSS_SW_ICLI_PHY_INVISIBLE
#define ICLI_PHY_NONE_SHOW_CMDS_PROPERTY ICLI_CMD_PROP_INVISIBLE
#else
#define ICLI_PHY_NONE_SHOW_CMDS_PROPERTY ICLI_CMD_PROP_VISIBLE
#endif

/* if VTSS_SW_OPTION_PHY does not exist then PHY_INST=NULL */
#define PHY_INST phy_mgmt_inst_get()

/* Initialize module */
mesa_rc phy_init(vtss_init_data_t *data);

/* Get the instance  */
vtss_inst_t phy_mgmt_inst_get(void);

#endif /* _PHY_API_H_ */

