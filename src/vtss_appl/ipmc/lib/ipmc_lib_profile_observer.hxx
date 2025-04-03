/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _IPMC_LIB_PROFILE_OBSERVER_HXX_
#define _IPMC_LIB_PROFILE_OBSERVER_HXX_

#include <vtss/appl/ipmc_lib.h>
#include <vtss/basics/expose.hxx>

// The ipmc_lib_profile_change_notification allows observers to be informed
// whenever a profile change occurs, whether it is added, deleted or changed.
// The observer also gets notified whenever a range change occurs if the range
// is used by the profile.
// The observer also gets notified whenever a rule change occurs for the
// profile.
// The second parameter (uint32_t) is a dummy that gets incremented for every
// change just in order to cause an event for the observer.
typedef vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_appl_ipmc_lib_profile_key_t *>, vtss::expose::ParamVal<uint32_t>> ipmc_lib_profile_change_notification_t;
extern ipmc_lib_profile_change_notification_t ipmc_lib_profile_change_notification;

#endif // _IPMC_LIB_PROFILE_OBSERVER_HXX_
