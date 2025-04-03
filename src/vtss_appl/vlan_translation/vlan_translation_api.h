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

#ifndef _VLAN_TRANSLATION_API_H_
#define _VLAN_TRANSLATION_API_H_

#include "vtss/appl/vlan.h"
#include "vtss/appl/vlan_translation.h"
#include "main.h"
#include "port_api.h" /* For port_count_max() */

#ifdef __cplusplus
extern "C" {
#endif

/*********************** Macros to define errors ***************************************************************************/
#define VT_MODULE_ERROR_START                MODULE_ERROR_START(VTSS_MODULE_ID_VLAN_TRANSLATION)
#define VT_ERROR_INVALID_GROUP_ID            (VT_MODULE_ERROR_START + 10)
#define VT_ERROR_INVALID_VLAN_ID             (VT_MODULE_ERROR_START + 11)
#define VT_ERROR_INVALID_TRANSLATION_VLAN_ID (VT_MODULE_ERROR_START + 12)
#define VT_ERROR_INVALID_PORT_NO             (VT_MODULE_ERROR_START + 13)
#define VT_ERROR_INVALID_IF_TYPE             (VT_MODULE_ERROR_START + 14)
#define VT_ERROR_TVID_SAME_AS_VID            (VT_MODULE_ERROR_START + 15)
#define VT_ERROR_ENTRY_NOT_FOUND             (VT_MODULE_ERROR_START + 16)
#define VT_ERROR_ENTRY_CONFLICT              (VT_MODULE_ERROR_START + 17)
#define VT_ERROR_MAPPING_TABLE_FULL          (VT_MODULE_ERROR_START + 18)
#define VT_ERROR_API_IF_SET                  (VT_MODULE_ERROR_START + 19)
#define VT_ERROR_API_IF_DEF                  (VT_MODULE_ERROR_START + 20)
#define VT_ERROR_API_MAP_ADD                 (VT_MODULE_ERROR_START + 21)
#define VT_ERROR_API_MAP_DEL                 (VT_MODULE_ERROR_START + 22)

/********************** Macros to define maximum values and ranges ********************************************************/
#define VT_MAX_TRANSLATIONS 256
#define VT_GID_NULL         0
#define VT_GID_START        1
#define VT_GID_END          (VT_GID_START + port_count_max() - 1)
#define VT_VID_START        VTSS_APPL_VLAN_ID_MIN
#define VT_VID_END          (VT_VID_START + VTSS_APPL_VLAN_ID_MAX - 1)  /* VLAN range: 1-4095 */

/********************** Macros to check ranges *****************************************************************************/
#define VT_VALID_GROUP_ID_CHECK(gid) (((gid < VT_GID_START) || (gid > VT_GID_END)) ? FALSE : TRUE)

#define VT_VALID_VLAN_ID_CHECK(vid) (((vid < VT_VID_START) || (vid > VT_VID_END)) ? FALSE : TRUE)

#define VT_VALID_PORT_NO_CHECK(port) (((port == VTSS_PORT_NO_START) || (port > fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT))) ? FALSE : TRUE)

#define VT_PTR_CHECK(ptr) ((ptr == NULL) ? FALSE : TRUE)

/********************* Functions *******************************************************************************************/
mesa_rc vlan_trans_init(vtss_init_data_t *data);
#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
void vtss_vlan_trans_mib_init(void);
#endif
#if defined(VTSS_SW_OPTION_JSON_RPC)
void vtss_appl_vlan_trans_json_init(void);
#endif

const char *vlan_trans_error_txt(mesa_rc rc);
const char *vlan_trans_dir_txt(mesa_vlan_trans_dir_t dir);

/* VLAN Translation Mappings MGMT API */
mesa_rc vlan_trans_mgmt_group_conf_add(const vtss_appl_vlan_translation_group_mapping_key_t key, const mesa_vid_t tvid);
mesa_rc vlan_trans_mgmt_group_conf_del(const vtss_appl_vlan_translation_group_mapping_key_t key);
mesa_rc vlan_trans_mgmt_group_conf_get(const vtss_appl_vlan_translation_group_mapping_key_t key, mesa_vid_t *tvid);
mesa_rc vlan_trans_mgmt_group_conf_itr(const vtss_appl_vlan_translation_group_mapping_key_t *const in,
                                       vtss_appl_vlan_translation_group_mapping_key_t *const out, mesa_vid_t *const tvid);

/* Interface configuration MGMT API */
mesa_rc vlan_trans_mgmt_port_conf_set(const mesa_port_no_t port, const vtss_appl_vlan_translation_if_conf_value_t group);
mesa_rc vlan_trans_mgmt_port_conf_get(const mesa_port_no_t port, vtss_appl_vlan_translation_if_conf_value_t *const group);
mesa_rc vlan_trans_mgmt_port_conf_def(const mesa_port_no_t port);

#ifdef __cplusplus
}
#endif
#endif /* _VLAN_TRANSLATION_API_H_ */
