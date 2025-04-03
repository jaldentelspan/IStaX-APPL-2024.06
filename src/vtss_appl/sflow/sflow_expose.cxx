/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "sflow_serializer.hxx"
#include "vtss/appl/sflow.h"
#include "sflow_trace.h"

/* {------------ Enum to text definitions --------------*/
vtss_enum_descriptor_t sflow_flow_sampling_type_txt[] {
    {MESA_SFLOW_TYPE_NONE,  "None"},
    {MESA_SFLOW_TYPE_RX,    "RX"},
    {MESA_SFLOW_TYPE_TX,    "TX"},
    {MESA_SFLOW_TYPE_ALL,   "ALL"},
    {0, 0},
};
/* }---------------------***********------------------------*/

/* {--------------------------- Table iterators ---------------------------*/
mesa_rc vtss_appl_sflow_rcvr_itr(const u32 *const rcvr_idx_in, u32 *const rcvr_idx_out)
{
    vtss::expose::snmp::IteratorComposeRange<u32> itr(0, VTSS_APPL_SFLOW_RECEIVER_CNT);
    return itr(rcvr_idx_in, rcvr_idx_out);
}

mesa_rc sflow_instance_itr(const u16 *const prev_instance, u16 *const next_instance)
{
    vtss::expose::snmp::IteratorComposeRange<u16> itr(0, VTSS_APPL_SFLOW_INSTANCE_CNT);
    return itr(prev_instance, next_instance);
}

mesa_rc vtss_appl_sflow_instance_itr(const vtss_ifindex_t *const prev_ifindex,
                                     vtss_ifindex_t *const next_ifindex,
                                     const u16 *const prev_instance,
                                     u16 *const next_instance)
{
    mesa_rc             rc = VTSS_RC_OK;

    vtss::IteratorComposeN<vtss_ifindex_t, u16> itr(vtss_appl_iterator_ifindex_front_port, sflow_instance_itr);

    T_D("Enter: prev_ifindex = %d, prev_instance = %d\n", prev_ifindex ? vtss_ifindex_cast_to_u32(*prev_ifindex) : -1, prev_instance ? *prev_instance : -1);
    rc = itr(prev_ifindex, next_ifindex, prev_instance, next_instance);
    T_D("Exit (rc = %d): *prev_ifindex = %d, *next_ifindex = %u, *prev_instance = %d *next_instance = %u\n", rc, prev_ifindex ? vtss_ifindex_cast_to_u32(*prev_ifindex) : -1, vtss_ifindex_cast_to_u32(*next_ifindex), prev_instance ? *prev_instance : -1, *next_instance);

    return rc;
}
/* }---------------------***********------------------------*/

/* {----------------- dummy get methods, for WriteOnly tables ---------------*/
mesa_rc sflow_rcvr_statistics_clr_dummy_get(u32 rcvr_idx, BOOL *const clear) {
    if (clear && *clear) {
        *clear = FALSE;
    }
    return VTSS_RC_OK;
}

mesa_rc sflow_instance_statistics_clr_dummy_get(
    vtss_ifindex_t ifindex,
    u16 instance,
    BOOL *const clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }
    return VTSS_RC_OK;
}
/* }---------------------***********------------------------*/

/* {---------------- sflow_xyz_clr_set methods, for WriteOnly tables -----------------*/
mesa_rc sflow_instance_statistics_clr_set(vtss_ifindex_t ifindex, u16 instance, const BOOL *const clear) {
    if (clear && *clear) {  // Make sure the Clear is not a NULL pointer
        if (!vtss_ifindex_is_port(ifindex)) {
            return VTSS_RC_ERROR;
        }
        return vtss_appl_sflow_instance_statistics_clear(ifindex, instance);
    }
    return VTSS_RC_ERROR;
}

mesa_rc sflow_rcvr_statistics_clr_set(u32 rcvr_idx, const BOOL *const clear) {
    if (clear && *clear) {  // Make sure the Clear is not a NULL pointer

        return vtss_appl_sflow_rcvr_statistics_clear(rcvr_idx);
    }
    return VTSS_RC_ERROR;
}
/* }---------------------***********------------------------*/
