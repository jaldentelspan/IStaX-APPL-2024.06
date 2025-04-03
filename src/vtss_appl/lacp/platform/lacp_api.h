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

#ifndef _LACP_API_H_
#define _LACP_API_H_

#include "vtss/appl/lacp.h"
#include "vtss_lacp.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    LACP_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_LACP),  /* Generic error code            */
    LACP_ERROR_STATIC_AGGR_ENABLED,                            /* Static aggregation is enabled */
    LACP_ERROR_DOT1X_ENABLED,                                  /* DOT1X is enabled              */
    LACP_ERROR_INVALID_PRT_IFTYPE,                             /* Invalid input key type, expect PORT */
    LACP_ERROR_INVALID_PRT_KEY,                                /* Invalid LACP port key value   */
    LACP_ERROR_INVALID_PRIO,                                   /* Invalid LACP priority value   */
    LACP_ERROR_INVALID_AGGR_ID,                                /* Invalid Aggregation ID        */
    LACP_ERROR_ENTRY_NOT_FOUND,                                /* Entry not found               */
    LACP_ERROR_MEMBER_OVERFLOW,                                /* To many port members          */
    LACP_ERROR_INVALID_KEY,                                    /* Invalid key                   */
    LACP_ERROR_MAX_BUNDLE_OVERFLOW                             /* To many in max-bundle         */
};

mesa_rc lacp_init(vtss_init_data_t *data);

/* Get the system configuration  */
mesa_rc lacp_mgmt_system_conf_get(vtss_lacp_system_config_t *conf);

/* Set the system configuration  */
mesa_rc lacp_mgmt_system_conf_set(const vtss_lacp_system_config_t *conf);

/* Get the port configuration  */
mesa_rc lacp_mgmt_port_conf_get(vtss_isid_t isid, mesa_port_no_t port_no, vtss_lacp_port_config_t *pconf);

/* Set the port configuration  */
mesa_rc lacp_mgmt_port_conf_set(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_lacp_port_config_t *pconf);

/* Get the default port configuration  */
mesa_rc lacp_mgmt_port_conf_get_default(vtss_lacp_port_config_t *pconf);

/* Get the system aggregation status  */
vtss_common_bool_t lacp_mgmt_aggr_status_get(unsigned int aid, vtss_lacp_aggregatorstatus_t *stat);

/* Get the port status  */
mesa_rc lacp_mgmt_port_status_get(l2_port_no_t l2port, vtss_lacp_portstatus_t *stat);

/* Set the group config */
mesa_rc lacp_mgmt_group_conf_set(u16 key, const vtss_appl_lacp_group_conf_t *const conf);

/* Clear the statistics  */
void lacp_mgmt_statistics_clear(l2_port_no_t l2port);

/* aggr error text */
const char *lacp_error_txt(mesa_rc rc);

mesa_rc debug_vtss_lacp_dump(void);

#ifdef __cplusplus
}
#endif
#endif /* _LACP_API_H_ */
