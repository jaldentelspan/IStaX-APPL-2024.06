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

#ifndef _VTSS_COMMON_ITERATOR_HXX__
#define _VTSS_COMMON_ITERATOR_HXX__

#include "vtss/appl/types.h"
#include "vtss/appl/interface.h"

BOOL is_usid_exist(vtss_usid_t     usid);

mesa_rc vtss_appl_iterator_switch(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
);


mesa_rc vtss_appl_iterator_switch_all(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
);

mesa_rc vtss_appl_iterator_ifindex_front_port(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);

// Same as vtss_appl_iterator_ifindex_front_port, but skipping all switch which is currently not in the stack.
// This is useful when getting status information, where it doesn't make sense to get status from non-existing switches.
mesa_rc vtss_appl_iterator_ifindex_front_port_exist(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);


#ifdef VTSS_SW_OPTION_POE
mesa_rc vtss_appl_iterator_ifindex_poe_front_port_exist(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);
#endif


/*
    Check if ifindex of physical port is valid and existed.
    If elm is NULL, then do checking only.
    If valid and elm is not NULL, then get the corresponding isid and iport.

*/
mesa_rc vtss_appl_ifindex_port_exist(
    vtss_ifindex_t          ifindex,
    vtss_ifindex_elm_t      *elm
);

/*
    Check if ifindex of physical port is valid and configurable.
    If elm is NULL, then do checking only.
    If valid and elm is not NULL, then get the corresponding isid and iport.

*/
mesa_rc vtss_appl_ifindex_port_configurable(
    vtss_ifindex_t          ifindex,
    vtss_ifindex_elm_t      *elm
);

mesa_rc vtss_appl_ifindex_getnext_port_queue(
    const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
    const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue
);

#ifdef VTSS_SW_OPTION_CPUPORT
mesa_rc vtss_ifindex_iterator_cpu(
    const vtss_ifindex_t *const prev_ifindex,
    vtss_ifindex_t       *const next_ifindex);
#endif

#endif  // _VTSS_COMMON_ITERATOR_HXX__
