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

#ifndef _VTSS_UPNP_API_H_
#define _VTSS_UPNP_API_H_

#include "vtss_module_id.h"
#include "main.h"
/* for public APIs */
#include "vtss/appl/upnp.h"
//#include "vtss_upnp.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifndef VTSS_BOOL
#define VTSS_BOOL
#endif

/* UPNP management enabled/disabled */
#define UPNP_MGMT_ENABLED       1
#define UPNP_MGMT_DISABLED      0

#define UPNP_MGMT_DEFAULT_TTL   4   /* 1..255*/
#define UPNP_MGMT_MAX_TTL       255
#define UPNP_MGMT_MIN_TTL       1

#define UPNP_MGMT_DEFAULT_INT   100 /* 100..86400  */
#define UPNP_MGMT_MIN_INT       100 /* 100..86400  */
#define UPNP_MGMT_MAX_INT       86400 /* 100..86400 */

#define UPNP_MGMT_DEF_VLAN_ID   1
#define UPNP_MGMT_MIN_VLAN_ID   1
#define UPNP_MGMT_MAX_VLAN_ID   4095

#define UPNP_MGMT_UDNSTR_SIZE   (41 + 1) /* "UUID="(5char) + uuid(16bytes/32char) + 4 *'-' + 1 * '\0' */
#define UPNP_MGMT_IPSTR_SIZE    (15 + 1) /* xxx.xxx.xxx.xxx + 1 * '\0' */
#define UPNP_UDP_PORT           1900

/* UPNP error text */
const char *upnp_error_txt(mesa_rc rc);

/* Get default value for all params */
void upnp_default_get(vtss_appl_upnp_param_t *conf);

/* Initialize module */
mesa_rc upnp_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_UPNP_API_H_ */

