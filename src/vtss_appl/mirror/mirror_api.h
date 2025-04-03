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

#ifndef _VTSS_MIRROR_API_H_
#define _VTSS_MIRROR_API_H_

#include "vtss/appl/rmirror.h"
#include "vtss/appl/mirror.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RMIRROR_USE_RMIRROR_UI_INSTEAD_OF_MIRROR_UI   1

/* Initialize module */
mesa_rc mirror_init(vtss_init_data_t *data);

/* which direction needs to enable for mirror source port */
typedef struct {
    BOOL    rx_enable;    /* Enable for source mirroring */
    BOOL    tx_enable;    /* Enable for detination mirroring */
} rmirror_source_port_t;

/* RMIRROR configurations for single switch */
typedef struct {
    ulong                   session_num;                            /* session number: index */
    mesa_vid_t              vid;                                    /* VLAN id*/
    vtss_appl_rmirror_switch_type_t   type;                         /* switch type: source, intermediate, destination */
    vtss_isid_t             rmirror_switch_id;                      /* internal switch id for RMIRROR */
    mesa_port_no_t          reflector_port;                         /* reflector port for source switch */
    CapArray<rmirror_source_port_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> source_port; /* source port for source switch */
    CapArray<BOOL, VTSS_APPL_CAP_VLAN_VID_CNT> source_vid;          /* source VLANs for source switch */
    CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT> intermediate_port;            /* Enable for intermediate link */
    CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT> destination_port;             /* monitor port for destination switch*/
    BOOL                    enabled;                                /* enabled or disabled */
    BOOL                    cpu_src_enable;                         /* Enable for CPU source mirroring */
    BOOL                    cpu_dst_enable;                         /* Enable for CPU destination mirroring */
} rmirror_switch_conf_t;

/* RMIRROR error text */
const char *mirror_error_txt(mesa_rc rc); // Convert Error code to text

/* Reset the configuration to default */
void rmirror_mgmt_default_set(rmirror_switch_conf_t *conf);

/* Check RMIRROR VLAN ID is conflict with other configurations */
mesa_rc rmirror_mgmt_is_valid_vid(mesa_vid_t vid);

/* Get Max RMIRROR Session */
mesa_rc rmirror_mgmt_max_session_get(void);

/* Get Min RMIRROR Session */
mesa_rc rmirror_mgmt_min_session_get(void);

/* Get Total RMIRROR Session */
mesa_rc rmirror_mgmt_max_cnt_get(void);

/* Get RMIRROR VLAN ID by Session */
mesa_rc rmirror_mgmt_rmirror_vid_get(ulong session);

/* Get RMIRROR Configuration */
mesa_rc rmirror_mgmt_conf_get(rmirror_switch_conf_t *conf);

/* Get Next RMIRROR Configuration */
mesa_rc rmirror_mgmt_next_conf_get(rmirror_switch_conf_t *conf);

/* Get RMIRROR Switch Configuration */
mesa_rc rmirror_mgmt_switch_conf_get(vtss_isid_t isid, rmirror_switch_conf_t *conf);

/* Get Next RMIRROR Switch Configuration */
mesa_rc rmirror_mgmt_next_switch_conf_get(vtss_isid_t isid, rmirror_switch_conf_t *conf);

/* Set RMIRROR Configuration */
mesa_rc rmirror_mgmt_conf_set(rmirror_switch_conf_t *new_conf);

/* Set RMIRROR Switch Configuration */
mesa_rc rmirror_mgmt_switch_conf_set(vtss_isid_t isid, rmirror_switch_conf_t *new_conf);

/* Get RMIRROR is enabled or not */
BOOL rmirror_mgmt_is_rmirror_enabled(void) ;

/* Check whether the port is a candidate reflector port or not */
BOOL rmirror_mgmt_is_valid_reflector_port(vtss_isid_t isid, mesa_port_no_t port_no);

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    RMIRROR_ERROR_MUST_BE_PRIMARY_SWITCH = VTSS_APPL_MIRROR_ERROR_MUST_BE_PRIMARY_SWITCH, /**< Operation is only allowed on the primary switch.               */
    RMIRROR_ERROR_ISID,                                                                   /**< isid parameter is invalid.                                     */
    RMIRROR_ERROR_INV_PARAM,                                                              /**< Invalid parameter.                                             */
    RMIRROR_ERROR_VID_IS_CONFLICT,                                                        /**< RMIRROR VID is conflict with MVR VID.                          */
    RMIRROR_ERROR_ALLOCATE_MEMORY,                                                        /**< Allocate memory error.                                         */
    RMIRROR_ERROR_CONFLICT_WITH_EEE,                                                      /**< RMIRROR only allowed when EEE is disabled (Jaguar1).           */
    RMIRROR_ERROR_SOURCE_SESSION_EXCEED_THE_LIMIT,                                        /**< Mirror or RMirror source session has reached the limit         */
    RMIRROR_ERROR_SOURCE_PORTS_ARE_USED_BY_OTHER_SESSIONS,                                /**< The source ports are used by other sessions.                   */
    RMIRROR_ERROR_DESTINATION_PORTS_ARE_USED_BY_OTHER_SESSIONS,                           /**< The destination ports are used by other sessions.              */
    RMIRROR_ERROR_REFLECTOR_PORT_IS_USED_BY_OTHER_SESSIONS,                               /**< The reflector port are used by other sessions.                 */
    RMIRROR_ERROR_REFLECTOR_PORT_IS_INCLUDED_IN_SRC_OR_DEST_PORTS,                        /**< The reflector port is included in source or destination ports  */
    RMIRROR_ERROR_REFLECTOR_PORT_IS_INVALID,                                              /**< The port can't be used as reflector port                       */
    RMIRROR_ERROR_SOURCE_VIDS_INCLUDE_RMIRROR_VID,                                        /**< RMirror VLAN can't be included in source VLAN                  */
    RMIRROR_ERROR_DESTINATION_PORTS_EXCEED_THE_LIMIT                                      /**< The destination ports of Mirror session have reached the limit */
};


#ifdef __cplusplus
}
#endif

#endif /* _VTSS_MIRROR_API_H_ */

