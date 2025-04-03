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

#ifndef _VTSS_PVLAN_API_H_
#define _VTSS_PVLAN_API_H_

#ifdef __cplusplus
extern "C" {
#endif
#define PVLAN_PORT_SIZE VTSS_PORT_BF_SIZE(PORT_NUMBER)

#define PVLAN_ID_START 1                               /* First VLAN ID */
#define PVLAN_ID_END   fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) /* Last VLAN ID */
#define PVLAN_ID_IS_LEGAL(x) ((x) >= PVLAN_ID_START && (x) <= PVLAN_ID_END)

/* PVLAN error codes (mesa_rc) */
typedef enum {
    PVLAN_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_PVLAN), /* Generic error code                     */
    PVLAN_ERROR_PARM,                                           /* Illegal parameter                      */
    PVLAN_ERROR_CONFIG_NOT_OPEN,                                /* Configuration open error               */
    PVLAN_ERROR_ENTRY_NOT_FOUND,                                /* Private VLAN not found                 */
    PVLAN_ERROR_PVLAN_TABLE_EMPTY,                              /* Private VLAN table empty               */
    PVLAN_ERROR_PVLAN_TABLE_FULL,                               /* Private VLAN table full                */
    PVLAN_ERROR_STACK_STATE,                                    /* Illegal primary/secondary switch state */
    PVLAN_ERROR_UNSUPPORTED,                                    /* Unsupported feature                    */
    PVLAN_ERROR_DEL_INSTEAD_OF_ADD,                             /* The PVLAN was deleted instead of added (because no ports were selected for the PVLAN). This might not be an error */
} pvlan_error_t;

/* PVLAN error text */
const char *pvlan_error_txt(mesa_rc rc);

#define PVLAN_SRC_MASK_ENA 1
typedef struct {
    mesa_pvlan_no_t  privatevid; /* Private VLAN ID */
    mesa_port_list_t ports;      /* Port list */
} pvlan_mgmt_entry_t;

/* PVLAN management functions */
mesa_rc pvlan_mgmt_pvlan_add(vtss_isid_t isid, pvlan_mgmt_entry_t *pvlan_mgmt_entry);

mesa_rc pvlan_mgmt_pvlan_del(mesa_pvlan_no_t privatevid);

mesa_rc pvlan_mgmt_pvlan_get(vtss_isid_t isid, mesa_pvlan_no_t privatevid, pvlan_mgmt_entry_t *pvlan_mgmt_entry, BOOL next);

mesa_rc pvlan_mgmt_isolate_conf_set(vtss_isid_t isid, mesa_port_list_t &list);

mesa_rc pvlan_mgmt_isolate_conf_get(vtss_isid_t isid, mesa_port_list_t &list);

/* Initialize module */
mesa_rc pvlan_init(vtss_init_data_t *data);


#ifdef __cplusplus
}
#endif
#endif /* _VTSS_PVLAN_API_H_ */

