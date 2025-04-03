/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __SNMP_PLATFORM_VTSS_PRIVATE_TRAP_HXX__
#define __SNMP_PLATFORM_VTSS_PRIVATE_TRAP_HXX__

#include <string>
#include "main_types.h"

namespace vtss {
namespace appl {
namespace snmp_trap {

mesa_rc listen_add(const std::string &event_name);

mesa_rc listen_del(const std::string &event_name);

mesa_rc listen_get_next(std::string &event_name);

mesa_rc init(vtss_init_data_t *data);

}  // namespace snmp_trap
}  // namespace appl
}  // namespace vtss

extern "C" mesa_rc vtss_appl_snmp_private_trap_init(vtss_init_data_t *data);
extern "C" mesa_rc vtss_appl_snmp_private_trap_listen_add(char *source);
extern "C" mesa_rc vtss_appl_snmp_private_trap_listen_del(char *source);
extern "C" mesa_rc vtss_appl_snmp_private_trap_listen_get_next(char *source, size_t len);

#endif  // __SNMP_PLATFORM_VTSS_PRIVATE_TRAP_HXX__
