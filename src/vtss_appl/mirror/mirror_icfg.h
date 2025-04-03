/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_MIRROR_ICFG_H_
#define _VTSS_MIRROR_ICFG_H_

/*
******************************************************************************

    Include files

******************************************************************************
*/

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/

/* RMIRROR management enabled/disabled */
#define RMIRROR_MGMT_ENABLED         (1)
#define RMIRROR_MGMT_DISABLED        (0)

#define RMIRROR_NO_FORM_TEXT              "no "
#define RMIRROR_GLOBAL_MODE_ENABLE_TEXT   "monitor session"

#define RMIRROR_MGMT_DEFAULT_SESSION_ICFG             1

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/**
 * \file rmirror_icfg.h
 * \brief This file defines the interface to the Remote Mirror module's ICFG commands.
 */

/**
  * \brief Initialization function.
  *
  * Call once, preferably from the INIT_CMD_INIT section of
  * the module's _init() function.
  */
mesa_rc vtss_rmirror_icfg_init(void);

#endif /* _VTSS_MIRROR_ICFG_H_ */
