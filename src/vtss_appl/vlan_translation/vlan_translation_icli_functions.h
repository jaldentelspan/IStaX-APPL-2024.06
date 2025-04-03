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
 * \brief Vlan_Translation icli functions
 * \details This header file describes vlan_translation control functions
 */

#ifndef VTSS_ICLI_VLAN_TRANSLATION_H
#define VTSS_ICLI_VLAN_TRANSLATION_H

#include "icli_api.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief Function for mapping VLANs to a translation VLAN
 *
 * \param session_id [IN] Needed for being able to print error messages
 * \param gid        [IN] Group id containing the VLAN translation
 * \param vlan_list  [IN] List of VLANs to be translated
 * \param tvid       [IN] VLAN to translate to
 * \param no         [IN] TRUE to remove a translation, FALSE to add
 * \return Error code.
 **/
mesa_rc vlan_trans_icli_map_conf_obsolete(i32 session_id, u32 gid,
                                          icli_unsigned_range_t *vlan_list, u32 tvid,
                                          BOOL no);

mesa_rc vlan_trans_icli_map_conf(i32 session_id, u32 gid, BOOL has_both, BOOL has_ingress,
                                 BOOL has_egress, mesa_vid_t vid, mesa_vid_t tvid, BOOL no);

/**
 * \brief Function for mapping an interface to a VLAN translation group
 *
 * \param session_id [IN] Needed for being able to print error messages
 * \param gid        [IN] Group to map the interface to
 * \param plist      [IN] Containing port information
 * \param no         [IN] TRUE to set port to default group, FALSE to set to the provided one
 * \return Error code.
 **/
mesa_rc vlan_trans_icli_interface_conf(i32 session_id, u8 gid, icli_stack_port_range_t *plist, BOOL no);

/**
 * \brief Function for at runtime getting information about how many groups that is supported
 *
 * \param session_id [IN] The session id use by iCLI print.
 * \param ask        [IN] Asking
 * \param runtime    [IN] Pointer to where to put the "answer"
 * \return TRUE is runtime contains valid information.
 **/
BOOL vlan_trans_icli_runtime_groups(u32                session_id,
                                    icli_runtime_ask_t ask,
                                    icli_runtime_t     *runtime);

#ifdef __cplusplus
}
#endif
#endif /* VTSS_ICLI_VLAN_TRANSLATION_H */

