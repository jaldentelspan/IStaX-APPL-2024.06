/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
* \brief Public SSH API
* \details This header file describes SSH control functions and types.
*/

#ifndef _VTSS_APPL_SSH_H_
#define _VTSS_APPL_SSH_H_

#include <vtss/appl/types.h>

/*! \brief SSH public configuration. */
typedef struct {
    mesa_bool_t mode; /**< SSH global administrative mode */
} vtss_appl_ssh_conf_t;

/**
  * \brief Get the global SSH configuration.
  *
  * \param ssh_cfg [OUT]: Pointer to structure that retrieves
  *                       the current SSH configuration.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    Otherwise, returns the corresponding error code from SSH.\n
  */
mesa_rc vtss_appl_ssh_conf_get(vtss_appl_ssh_conf_t *const ssh_cfg);

/**
  * \brief Set the global SSH configuration.
  *
  * \param ssh_cfg [IN]: Pointer to structure that contains the
  *                      global configuration to be applied in
  *                      SSH module.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    Otherwise, returns the corresponding error code from SSH.\n
  */
mesa_rc vtss_appl_ssh_conf_set(const vtss_appl_ssh_conf_t *const ssh_cfg);

#endif /* _VTSS_APPL_SSH_H_ */
