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

#ifndef _VTSS_NTP_API_H_
#define _VTSS_NTP_API_H_

#include "vtss_module_id.h"
#include "main.h"
#include "sysutil_api.h"    // For VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN
#include "vtss/appl/ntp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ntp management enabled/disabled */
#define NTP_MGMT_DISABLED      0
#define NTP_MGMT_ENABLED       1
#define NTP_MGMT_INITIAL       2

/* ntp ip type */
#define NTP_IP_TYPE_IPV4     0x0
#define NTP_IP_TYPE_IPV6     0x1

/* ntp error codes (mesa_rc) */
typedef enum {
    NTP_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_NTP),   /* Generic error code */
    NTP_ERROR_PARM,                                           /* Illegal parameter */
} ntp_error_t;

/* ntp configuration */
typedef struct {
    ulong       ip_type;                 /* IP type 0 is IPv4, 1 is IPv6 */
    char        ip_host_string[VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN + 1];
#ifdef VTSS_SW_OPTION_IPV6
    mesa_ipv6_t ipv6_addr;               /* IPv6 address */
#endif
} ntp_server_config_t;

typedef struct {
    BOOL                mode_enabled;
    ulong               interval_min;
    ulong               interval_max;
    ntp_server_config_t server[VTSS_APPL_NTP_SERVER_MAX_COUNT];
    char                drift_valid;   /* Indicates if drift_data valid */
    int                 drift_data;    /* default PPM */
    char                drift_trained; /* It is a test flag. When on, it means
                                          the NTP state will be initialized at S_FSET,
                                          Otherwise it will be at S_NSET which will takes
                                          15 minutes to calculate the frequency when enabling
                                          NTP.
                                        */
} ntp_conf_t;


/* ntp error text */
const char *ntp_error_txt(ntp_error_t rc);

/* Set ntp defaults */
void vtss_ntp_default_set(ntp_conf_t *conf);

/* Get ntp configuration */
mesa_rc ntp_mgmt_conf_get(ntp_conf_t *conf);

/* Set ntp configuration */
mesa_rc ntp_mgmt_conf_set(ntp_conf_t *conf);

/* Initialize module */
mesa_rc ntp_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_NTP_API_H_ */

