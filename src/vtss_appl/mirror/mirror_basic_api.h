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

#ifndef _VTSS_MIRROR_BASIC_API_H_
#define _VTSS_MIRROR_BASIC_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize module */
mesa_rc mirror_basic_init(vtss_init_data_t *data);

//Default configuration values
#define MIRROR_SWITCH_DEFAULT  mirror_switch_default()
#define MIRROR_PORT_DEFAULT VTSS_PORT_NO_NONE
#define MIRROR_SRC_ENA_DEFAULT FALSE
#define MIRROR_DST_ENA_DEFAULT FALSE
#define MIRROR_CPU_SRC_ENA_DEFAULT FALSE
#define MIRROR_CPU_DST_ENA_DEFAULT FALSE



// Configuration for a single switch
typedef struct {
    mesa_port_list_t src_enable;     // Enable for source mirroring
    mesa_port_list_t dst_enable;     // Enable for detination mirroring
    BOOL             cpu_src_enable; // Enable for CPU source mirroring
    BOOL             cpu_dst_enable; // Enable for CPU source mirroring
} mirror_switch_conf_t;


/* Mirror configuration */
typedef struct {
    vtss_isid_t    mirror_switch;            // Switch id (isid) for the switch with the active mirror port.
    mesa_port_no_t dst_port;                 /* Mirroring port. Port VTSS_PORT_NO_NONE is disable mirroring */
} mirror_conf_t;


// Clobal configuration (mirror switch and port )
void mirror_mgmt_conf_get(mirror_conf_t *conf);
mesa_rc mirror_mgmt_conf_set(mirror_conf_t *conf);

//  Switch configuration (source and destination port enable):
mesa_rc mirror_mgmt_switch_conf_get(vtss_isid_t isid, mirror_switch_conf_t *conf);
void mirror_mgmt_switch_conf_set(vtss_isid_t isid, mirror_switch_conf_t *conf);

// Function that returns the default mirror switch
vtss_isid_t mirror_switch_default(void);
/****************************************************************************/
// API Error Return Codes (mesa_rc)
/****************************************************************************/
//char *mirror_error_txt(mesa_rc rc); // Convert Error code to text
enum {
    MIRROR_ERROR_INV_PARAM = MODULE_ERROR_START(VTSS_MODULE_ID_MIRROR), // NULL parameter passed to one of the mirror_XXX functions, where a non-NULL was expected.
    MIRROR_ERROR_PORT_CONF_MUST_BE_PRIMARY_SWITCH,                      // Management operation is not valid on secondary switches.
    MIRROR_ERROR_INVALID_MIRROR_PORT,                                   // Invalid mirror destination port.
    MIRROR_ERROR_INVALID_MIRROR_SWITCH,                                 // Invalid mirror destination switch.
};

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_MIRROR_BASIC_API_H_ */

