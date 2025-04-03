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

#ifndef _REDBOX_EXPOSE_HXX_
#define _REDBOX_EXPOSE_HXX_

#include <vtss/appl/redbox.h>
#include <vtss/basics/expose.hxx>
#include <vtss/basics/memcmp-operator.hxx>

typedef vtss::expose::TableStatus<vtss::expose::ParamKey<uint32_t>, vtss::expose::ParamVal<vtss_appl_redbox_notification_status_t *>> redbox_notification_status_t;
extern redbox_notification_status_t redbox_notification_status;

// Do simple memcmp() to figure out whether a notification has changed.
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_redbox_notification_status_t);

#endif /* _REDBOX_EXPOSE_HXX_ */

