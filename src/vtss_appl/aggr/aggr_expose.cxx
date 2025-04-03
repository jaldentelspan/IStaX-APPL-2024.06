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

#include "aggr_serializer.hxx"
#include "vtss/appl/aggr.h"
#include "aggr_serializer.hxx"

mesa_rc aggregation_iface_itr_lag_idx(const vtss_ifindex_t *prev_ifindex,
                                      vtss_ifindex_t *next_ifindex) {
    return vtss_ifindex_getnext_by_type(prev_ifindex, next_ifindex,
                                        VTSS_IFINDEX_GETNEXT_GLAGS | VTSS_IFINDEX_GETNEXT_LLAGS);
}

mesa_rc vtss_appl_aggregation_status_itr(const vtss_ifindex_t *const prev,
                                         vtss_ifindex_t       *const next) {
    return aggregation_iface_itr_lag_idx(prev, next);
}

mesa_rc vtss_appl_aggregation_port_members_itr(const vtss_ifindex_t *const prev,
                                               vtss_ifindex_t       *const next) {
    return aggregation_iface_itr_lag_idx(prev, next);
}
