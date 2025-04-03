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

#ifndef __PSEC_EXPOSE_HXX__
#define __PSEC_EXPOSE_HXX__

#include <vtss/appl/psec.h>
#include <vtss/basics/expose.hxx>
#include "psec_api.h"   /* For psec_semi_public_interface_status_t */

typedef vtss::expose::StructStatus<vtss::expose::ParamVal<vtss_appl_psec_global_notification_status_t *>> PsecGlobalNotificationStatus;
extern PsecGlobalNotificationStatus psec_global_notification_status;

// The Port Security Interface status is part of psec_semi_public_interface_status_t, but the whole semi-public structure
// is managed by the Expose framework, and only the public part is exposed to SNMP and JSON.
typedef vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_ifindex_t>, vtss::expose::ParamVal<psec_semi_public_interface_status_t *>> PsecInterfaceStatus;
extern PsecInterfaceStatus psec_semi_public_interface_status;
mesa_rc psec_expose_interface_status_get(vtss_ifindex_t ifindex, psec_semi_public_interface_status_t *const status);

typedef vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_ifindex_t>, vtss::expose::ParamVal<vtss_appl_psec_interface_notification_status_t *>> PsecInterfaceNotificationStatus;
extern PsecInterfaceNotificationStatus psec_interface_notification_status;

// Wrappers around vtss_appl_psec_global_control_mac_clear()
mesa_rc psec_expose_global_control_mac_clear_get(vtss_appl_psec_global_control_mac_clear_t *info);

extern vtss_enum_descriptor_t psec_expose_violation_mode_txt[];
extern vtss_enum_descriptor_t psec_expose_mac_type_txt[];

#endif  // __PSEC_EXPOSE_HXX__
