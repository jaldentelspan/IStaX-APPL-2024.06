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

#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"
#include "vtss/appl/lldp.h"
#include "vtss_common_iterator.hxx"

#ifdef VTSS_SW_OPTION_LLDP_MED

mesa_rc vtss_appl_lldp_port_policies_list_itr(const vtss_ifindex_t *prev_ifindex,             vtss_ifindex_t *next_ifindex,
                                              const vtss_lldpmed_policy_index_t *prev_policy, vtss_lldpmed_policy_index_t *next_policy)
{
    vtss::IteratorComposeN<vtss_ifindex_t, vtss_lldpmed_policy_index_t> itr(vtss_appl_iterator_ifindex_front_port, vtss_appl_lldp_port_policies_itr);

    return itr(prev_ifindex, next_ifindex, prev_policy, next_policy);
}
#endif // #ifdef VTSS_SW_OPTION_LLDP_MED
