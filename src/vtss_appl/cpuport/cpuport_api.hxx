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

#ifndef __CPUPORT_API_H__
#define __CPUPORT_API_H__

#include <vtss/basics/map.hxx>
#include "vtss/appl/module_id.h"
/**
 * Function for converting a cpuport  module error
 * (see CPUPORT_RC_xxx above) to a textual string.
 * Only errors in the Thread Load Monitor module's range can
 * be converted.
 *
 * \param rc [IN] Binary form of error
 *
 * \return Static string containing textual representation of #rc.
 */
const char *cpuport_error_txt(mesa_rc rc);

#ifndef VTSS_BASICS_STANDALONE
/* Initialize module */
mesa_rc cpuport_init(vtss_init_data_t *data);

vtss::str cpuport_os_interface_name(vtss::Buf *b, vtss_ifindex_t ifidx);
const char *cpuport_os_interface_name(vtss_ifindex_t ifidx);
bool vtss_cpuport_is_interface(const char *ifname, int ifname_size, vtss_ifindex_t *ifidx);
mesa_rc vtss_cpuport_get_interface_flags(vtss_ifindex_t ifidx, short *flags);
#endif

#endif /* !defined(__CPUPORT_API_H__) */
